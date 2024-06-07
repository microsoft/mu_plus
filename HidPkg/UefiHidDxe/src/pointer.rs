//! Pointer support for HID input driver.
//!
//! This module manages pointer input for the HID input driver.
//!
//! ## License
//!
//! Copyright (c) Microsoft Corporation. All rights reserved.
//! SPDX-License-Identifier: BSD-2-Clause-Patent
//!
use alloc::{
  boxed::Box,
  collections::{BTreeMap, BTreeSet},
  vec::Vec,
};
use core::ffi::c_void;
use hidparser::{
  report_data_types::{ReportId, Usage},
  ReportDescriptor, ReportField, VariableField,
};
use memoffset::{offset_of, raw_field};
use r_efi::{
  efi,
  protocols::{absolute_pointer, driver_binding},
  system,
};
use rust_advanced_logger_dxe::{debugln, DEBUG_INFO, DEBUG_WARN};

use crate::{hid::HidContext, BOOT_SERVICES};

// Usages supported by this driver.
const GENERIC_DESKTOP_X: u32 = 0x00010030;
const GENERIC_DESKTOP_Y: u32 = 0x00010031;
const GENERIC_DESKTOP_Z: u32 = 0x00010032;
const GENERIC_DESKTOP_WHEEL: u32 = 0x00010038;
const BUTTON_PAGE: u16 = 0x0009;
const BUTTON_MIN: u32 = 0x00090001;
const BUTTON_MAX: u32 = 0x00090020; //Per spec, the Absolute Pointer protocol supports a 32-bit button state field.

// number of points on the X/Y axis for this implementation.
const AXIS_RESOLUTION: u64 = 1024;

// Maps a given field to a routine that handles input from it.
#[derive(Debug, Clone)]
struct ReportFieldWithHandler {
  field: VariableField,
  report_handler: fn(&mut PointerHandler, field: VariableField, report: &[u8]),
}

// Defines a report and the fields of interest within it.
#[derive(Debug, Default, Clone)]
struct PointerReportData {
  report_id: Option<ReportId>,
  report_size: usize,
  relevant_fields: Vec<ReportFieldWithHandler>,
}

/// Context structure used to track data for this pointer device.
/// Safety: this structure is shared across FFI boundaries, and pointer arithmetic is used on its contents, so it must
/// remain #[repr(C)], and Rust aliasing and concurrency rules must be manually enforced.
#[repr(C)]
pub struct PointerContext {
  absolute_pointer: absolute_pointer::Protocol,
  pub handler: PointerHandler,
  controller: efi::Handle,
  hid_context: *mut HidContext,
}

/// Tracks all the input reports for this device as well as pointer state.
#[derive(Debug, Default)]
pub struct PointerHandler {
  input_reports: BTreeMap<Option<ReportId>, PointerReportData>,
  supported_usages: BTreeSet<Usage>,
  report_id_present: bool,
  state_changed: bool,
  current_state: absolute_pointer::State,
}

impl PointerHandler {
  // processes a report descriptor and yields a PointerHandler instance if this descriptor describes input
  // that can be handled by this PointerHandler.
  fn process_descriptor(descriptor: &ReportDescriptor) -> Result<Self, efi::Status> {
    let mut handler: PointerHandler = Default::default();
    let multiple_reports = descriptor.input_reports.len() > 1;

    for report in &descriptor.input_reports {
      let mut report_data =
        PointerReportData { report_id: report.report_id, report_size: report.size_in_bits / 8, ..Default::default() };

      handler.report_id_present = report.report_id.is_some();

      if multiple_reports && !handler.report_id_present {
        //invalid to have None ReportId if multiple reports present.
        Err(efi::Status::DEVICE_ERROR)?;
      }

      for field in &report.fields {
        match field {
          ReportField::Variable(field) => {
            match field.usage.into() {
              GENERIC_DESKTOP_X => {
                let field_handler =
                  ReportFieldWithHandler { field: field.clone(), report_handler: Self::x_axis_handler };
                report_data.relevant_fields.push(field_handler);
                handler.supported_usages.insert(field.usage);
                //debugln!(DEBUG_INFO, "x-axis field {:#?}", field);
              }
              GENERIC_DESKTOP_Y => {
                let field_handler =
                  ReportFieldWithHandler { field: field.clone(), report_handler: Self::y_axis_handler };
                report_data.relevant_fields.push(field_handler);
                handler.supported_usages.insert(field.usage);
                //debugln!(DEBUG_INFO, "y-axis field {:#?}", field);
              }
              GENERIC_DESKTOP_Z | GENERIC_DESKTOP_WHEEL => {
                let field_handler =
                  ReportFieldWithHandler { field: field.clone(), report_handler: Self::z_axis_handler };
                report_data.relevant_fields.push(field_handler);
                handler.supported_usages.insert(field.usage);
                //debugln!(DEBUG_INFO, "z-axis field {:#?}", field);
              }
              BUTTON_MIN..=BUTTON_MAX => {
                let field_handler =
                  ReportFieldWithHandler { field: field.clone(), report_handler: Self::button_handler };
                report_data.relevant_fields.push(field_handler);
                handler.supported_usages.insert(field.usage);
                //debugln!(DEBUG_INFO, "button field {:#?}", field);
              }
              _ => (), //other usages irrelevant
            }
          }
          _ => (), // other field types irrelevant
        }
      }

      if report_data.relevant_fields.len() > 0 {
        handler.input_reports.insert(report_data.report_id, report_data);
      }
    }

    if handler.input_reports.len() > 0 {
      Ok(handler)
    } else {
      debugln!(DEBUG_INFO, "No relevant fields for handler: {:#?}", handler);
      Err(efi::Status::UNSUPPORTED)
    }
  }

  // Create PointerContext structure and install Absolute Pointer interface.
  fn install_pointer_interfaces(
    self,
    controller: efi::Handle,
    hid_context: *mut HidContext,
  ) -> Result<*mut PointerContext, efi::Status> {
    // retrieve a reference to boot services.
    // Safety: BOOT_SERVICES must have been initialized to point to the UEFI Boot Services table.
    // Caller should have ensured this, so just expect on failure.
    let boot_services = unsafe { BOOT_SERVICES.as_mut().expect("BOOT_SERVICES not properly initialized") };

    // Create pointer context. This context is shared across FFI boundary, so use Box::into_raw.
    // After creation, the only way to access the context (including the handler instance) is via a raw pointer.
    let context_ptr = Box::into_raw(Box::new(PointerContext {
      absolute_pointer: absolute_pointer::Protocol {
        reset: absolute_pointer_reset,
        get_state: absolute_pointer_get_state,
        mode: Box::into_raw(Box::new(self.initialize_mode())),
        wait_for_input: core::ptr::null_mut(),
      },
      handler: self,
      controller: controller,
      hid_context: hid_context,
    }));

    let context = unsafe { context_ptr.as_mut().expect("freshly boxed context pointer is null.") };

    // create event for wait_for_input.
    let mut wait_for_pointer_input_event: efi::Event = core::ptr::null_mut();
    let status = (boot_services.create_event)(
      system::EVT_NOTIFY_WAIT,
      system::TPL_NOTIFY,
      Some(wait_for_pointer),
      context_ptr as *mut c_void,
      core::ptr::addr_of_mut!(wait_for_pointer_input_event),
    );
    if status.is_error() {
      drop(unsafe { Box::from_raw(context_ptr) });
      return Err(status);
    }
    context.absolute_pointer.wait_for_input = wait_for_pointer_input_event;

    // install the absolute_pointer protocol.
    let mut controller = controller;
    let absolute_pointer_ptr = raw_field!(context_ptr, PointerContext, absolute_pointer);
    let status = (boot_services.install_protocol_interface)(
      core::ptr::addr_of_mut!(controller),
      &absolute_pointer::PROTOCOL_GUID as *const efi::Guid as *mut efi::Guid,
      efi::NATIVE_INTERFACE,
      absolute_pointer_ptr as *mut c_void,
    );

    if status.is_error() {
      let _ = deinitialize(context_ptr);
      return Err(status);
    }

    Ok(context_ptr)
  }

  // Initializes the absolute_pointer mode structure.
  fn initialize_mode(&self) -> absolute_pointer::Mode {
    let mut mode: absolute_pointer::Mode = Default::default();

    if self.supported_usages.contains(&Usage::from(GENERIC_DESKTOP_X)) {
      mode.absolute_max_x = AXIS_RESOLUTION;
      mode.absolute_min_x = 0;
    } else {
      debugln!(DEBUG_WARN, "No x-axis usages found in the report descriptor.");
    }

    if self.supported_usages.contains(&Usage::from(GENERIC_DESKTOP_Y)) {
      mode.absolute_max_y = AXIS_RESOLUTION;
      mode.absolute_min_y = 0;
    } else {
      debugln!(DEBUG_WARN, "No y-axis usages found in the report descriptor.");
    }

    if (self.supported_usages.contains(&Usage::from(GENERIC_DESKTOP_Z)))
      || (self.supported_usages.contains(&Usage::from(GENERIC_DESKTOP_WHEEL)))
    {
      mode.absolute_max_z = AXIS_RESOLUTION;
      mode.absolute_min_z = 0;
      //TODO: Z-axis is interpreted as pressure data. This is for compat with reference implementation in C, but
      //could consider e.g. looking for actual digitizer tip pressure usages or something.
      mode.attributes = mode.attributes | 0x02;
    } else {
      debugln!(DEBUG_INFO, "No z-axis usages found in the report descriptor.");
    }

    let button_count = self.supported_usages.iter().filter(|x| x.page() == BUTTON_PAGE).count();

    if button_count > 1 {
      mode.attributes = mode.attributes | 0x01; // alternate button exists.
    }

    mode
  }

  // Helper routine that handles projecting relative and absolute axis reports onto the fixed
  // absolute report axis that this driver produces.
  fn resolve_axis(current_value: u64, field: VariableField, report: &[u8]) -> Option<u64> {
    if field.attributes.relative {
      //for relative, just update and clamp the current state.
      let new_value = current_value as i64 + field.field_value(report)?;
      return Some(new_value.clamp(0, AXIS_RESOLUTION as i64) as u64);
    } else {
      //for absolute, project onto 0..AXIS_RESOLUTION
      let mut new_value = field.field_value(report)?;

      //translate to zero.
      new_value = new_value.checked_sub(i32::from(field.logical_minimum) as i64)?;

      //scale to AXIS_RESOLUTION
      new_value = (new_value * AXIS_RESOLUTION as i64 * 1000) / (field.field_range()? as i64 * 1000);

      return Some(new_value.clamp(0, AXIS_RESOLUTION as i64) as u64);
    }
  }

  // handles x_axis inputs
  fn x_axis_handler(&mut self, field: VariableField, report: &[u8]) {
    if let Some(x_value) = Self::resolve_axis(self.current_state.current_x, field, report) {
      self.current_state.current_x = x_value;
      self.state_changed = true;
    }
  }

  // handles y_axis inputs
  fn y_axis_handler(&mut self, field: VariableField, report: &[u8]) {
    if let Some(y_value) = Self::resolve_axis(self.current_state.current_y, field, report) {
      self.current_state.current_y = y_value;
      self.state_changed = true;
    }
  }

  // handles z_axis inputs
  fn z_axis_handler(&mut self, field: VariableField, report: &[u8]) {
    if let Some(z_value) = Self::resolve_axis(self.current_state.current_z, field, report) {
      self.current_state.current_z = z_value;
      self.state_changed = true;
    }
  }

  // handles button inputs
  fn button_handler(&mut self, field: VariableField, report: &[u8]) {
    let shift: u32 = field.usage.into();
    if (shift < BUTTON_MIN) || (shift > BUTTON_MAX) {
      return;
    }

    if let Some(button_value) = field.field_value(report) {
      let button_value = button_value as u32;

      let shift = shift - BUTTON_MIN;
      if shift > u32::BITS {
        return;
      }
      let button_value = button_value << shift;

      self.current_state.active_buttons = self.current_state.active_buttons
        & !(1 << shift)  // zero the relevant bit in the button state field.
        | button_value; // or in the current button state into that bit position.

      self.state_changed = true;
    }
  }

  /// Processes the given input report buffer and handles input from it.
  pub fn process_input_report(&mut self, report_buffer: &[u8]) {
    if report_buffer.len() == 0 {
      return;
    }

    // determine whether report includes report id byte and adjust the buffer as needed.
    let (report_id, report) = match self.report_id_present {
      true => (Some(ReportId::from(&report_buffer[0..1])), &report_buffer[1..]),
      false => (None, &report_buffer[0..]),
    };

    if report.len() == 0 {
      return;
    }

    if let Some(report_data) = self.input_reports.get(&report_id).cloned() {
      if report.len() != report_data.report_size {
        return;
      }

      // hand the report data to the handler for each relevant field for field-specific processing.
      for field in report_data.relevant_fields {
        (field.report_handler)(self, field.field, report);
      }
    }
  }

  // Uninstall the absolute pointer interface and free the Pointer context.
  fn uninstall_pointer_interfaces(pointer_context: *mut PointerContext) -> Result<(), efi::Status> {
    // retrieve a reference to boot services.
    // Safety: BOOT_SERVICES must have been initialized to point to the UEFI Boot Services table.
    // Caller should have ensured this, so just expect on failure.
    let boot_services = unsafe { BOOT_SERVICES.as_mut().expect("BOOT_SERVICES not properly initialized") };

    let absolute_pointer_ptr = raw_field!(pointer_context, PointerContext, absolute_pointer);

    let mut overall_status = efi::Status::SUCCESS;

    // close the wait_for_input event
    let status = (boot_services.close_event)(unsafe { (*absolute_pointer_ptr).wait_for_input });
    if status.is_error() {
      overall_status = status;
    }

    // uninstall absolute pointer protocol
    let status = (boot_services.uninstall_protocol_interface)(
      unsafe { (*pointer_context).controller },
      &absolute_pointer::PROTOCOL_GUID as *const efi::Guid as *mut efi::Guid,
      absolute_pointer_ptr as *mut c_void,
    );
    if status.is_error() {
      overall_status = status;
    }

    // take back the context and mode raw pointers
    let context = unsafe { Box::from_raw(pointer_context) };
    drop(unsafe { Box::from_raw(context.absolute_pointer.mode) });
    drop(context);

    if overall_status.is_error() {
      Err(overall_status)
    } else {
      Ok(())
    }
  }
}

/// Initializes a pointer handler on the given `controller` to handle reports described by `descriptor`.
///
/// If the device doesn't provide the necessary reports for pointers, or if there was an error processing the report
/// descriptor data or initializing the pointer handler or installing the absolute pointer protocol instance into the
/// protocol database, an error is returned.
///
/// Otherwise, a [`PointerContext`] that can be used to interact with this handler is returned. See [`PointerContext`]
/// documentation for constraints on interactions with it.
pub fn initialize(
  controller: efi::Handle,
  descriptor: &ReportDescriptor,
  hid_context_ptr: *mut HidContext,
) -> Result<*mut PointerContext, efi::Status> {
  let handler = PointerHandler::process_descriptor(descriptor)?;

  let context = handler.install_pointer_interfaces(controller, hid_context_ptr)?;

  let hid_context = unsafe { hid_context_ptr.as_mut().expect("[pointer::initialize]: bad hid context pointer") };

  hid_context.pointer_context = context;

  Ok(context)
}

/// De-initializes a pointer handler described by `context` on the given `controller`.
pub fn deinitialize(context: *mut PointerContext) -> Result<(), efi::Status> {
  if context.is_null() {
    return Err(efi::Status::NOT_STARTED);
  }
  PointerHandler::uninstall_pointer_interfaces(context)
}

/// Attempt to retrieve a *mut HidContext for the given controller by locating the absolute_pointer interface associated
/// with the controller (if any) and deriving a PointerContext from it (which contains a pointer to the HidContext).
pub fn attempt_to_retrieve_hid_context(
  controller: efi::Handle,
  driver_binding: &driver_binding::Protocol,
) -> Result<*mut HidContext, efi::Status> {
  // retrieve a reference to boot services.
  // Safety: BOOT_SERVICES must have been initialized to point to the UEFI Boot Services table.
  // Caller should have ensured this, so just expect on failure.
  let boot_services = unsafe { BOOT_SERVICES.as_mut().expect("BOOT_SERVICES not properly initialized") };

  let mut absolute_pointer_ptr: *mut absolute_pointer::Protocol = core::ptr::null_mut();
  let status = (boot_services.open_protocol)(
    controller,
    &absolute_pointer::PROTOCOL_GUID as *const efi::Guid as *mut efi::Guid,
    core::ptr::addr_of_mut!(absolute_pointer_ptr) as *mut *mut c_void,
    driver_binding.driver_binding_handle,
    controller,
    system::OPEN_PROTOCOL_GET_PROTOCOL,
  );

  match status {
    efi::Status::SUCCESS => {
      // retrieve a reference to the pointer context.
      // Safety: `absolute_pointer_ptr` must point to an instance of absolute_pointer that is contained in a
      // PointerContext struct. The following is the equivalent of the `CR` (contained record) macro in the EDK2 C
      // reference implementation.
      let context_ptr = unsafe { (absolute_pointer_ptr as *mut u8).sub(offset_of!(PointerContext, absolute_pointer)) }
        as *mut PointerContext;
      return Ok(unsafe { (*context_ptr).hid_context });
    }
    err => return Err(err),
  }
}

// event handler for wait_for_pointer event that is part of the absolute pointer interface.
extern "efiapi" fn wait_for_pointer(event: efi::Event, context: *mut c_void) {
  // retrieve a reference to boot services.
  // Safety: BOOT_SERVICES must have been initialized to point to the UEFI Boot Services table.
  // Caller should have ensured this, so just expect on failure.
  let boot_services = unsafe { BOOT_SERVICES.as_mut().expect("BOOT_SERVICES not properly initialized") };

  // retrieve a reference to pointer context.
  // Safety: Pointer Context must have been initialized before this event could fire, so just expect on failure.
  let pointer_context = unsafe { (context as *mut PointerContext).as_mut().expect("Pointer Context is bad.") };

  if pointer_context.handler.state_changed {
    (boot_services.signal_event)(event);
  }
}

// resets the pointer state - part of the absolute pointer interface.
extern "efiapi" fn absolute_pointer_reset(
  this: *mut absolute_pointer::Protocol,
  _extended_verification: bool,
) -> efi::Status {
  if this.is_null() {
    return efi::Status::INVALID_PARAMETER;
  }
  // retrieve a reference to the pointer context.
  // Safety: `this` must point to an instance of absolute_pointer that is contained in a PointerContext struct.
  // the following is the equivalent of the `CR` (contained record) macro in the EDK2 reference implementation.
  let context_ptr =
    unsafe { (this as *mut u8).sub(offset_of!(PointerContext, absolute_pointer)) } as *mut PointerContext;

  let pointer_context = unsafe { context_ptr.as_mut().expect("Context pointer is bad.") };

  pointer_context.handler.current_state = Default::default();

  //stick the pointer in the middle of the screen.
  pointer_context.handler.current_state.current_x = AXIS_RESOLUTION / 2;
  pointer_context.handler.current_state.current_y = AXIS_RESOLUTION / 2;
  pointer_context.handler.current_state.current_z = 0;

  pointer_context.handler.state_changed = false;

  efi::Status::SUCCESS
}

// returns the current pointer state in the `state` buffer provided by the caller - part of the absolute pointer
// interface.
extern "efiapi" fn absolute_pointer_get_state(
  this: *mut absolute_pointer::Protocol,
  state: *mut absolute_pointer::State,
) -> efi::Status {
  if this.is_null() || state.is_null() {
    return efi::Status::INVALID_PARAMETER;
  }
  // retrieve a reference to the pointer context.
  // Safety: `this` must point to an instance of absolute_pointer that is contained in a PointerContext struct.
  // the following is the equivalent of the `CR` (contained record) macro in the EDK2 reference implementation.
  let context_ptr =
    unsafe { (this as *mut u8).sub(offset_of!(PointerContext, absolute_pointer)) } as *mut PointerContext;

  let pointer_context = unsafe { context_ptr.as_mut().expect("Context pointer is bad.") };

  // Safety: state pointer is assumed to be a valid buffer to receive the current state.
  if pointer_context.handler.state_changed {
    unsafe { state.write(pointer_context.handler.current_state) };
    pointer_context.handler.state_changed = false;
    efi::Status::SUCCESS
  } else {
    efi::Status::NOT_READY
  }
}
