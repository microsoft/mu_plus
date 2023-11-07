//! Keyboard support for HID input driver.
//!
//! This module manages keyboard input for the HID input driver.
//!
//! ## License
//!
//! Copyright (c) Microsoft Corporation. All rights reserved.
//! SPDX-License-Identifier: BSD-2-Clause-Patent
//!

use core::ffi::c_void;

use alloc::{
  boxed::Box,
  collections::{BTreeMap, BTreeSet},
  vec,
  vec::Vec,
};
use hidparser::{
  report_data_types::{ReportId, Usage},
  ArrayField, ReportDescriptor, ReportField, VariableField,
};
use hii_keyboard_layout::HiiKeyboardLayout;
use memoffset::{offset_of, raw_field};
use r_efi::{
  efi, hii,
  protocols::{
    self, driver_binding,
    simple_text_input_ex::{KeyData, LEFT_SHIFT_PRESSED, RIGHT_SHIFT_PRESSED},
  },
  system,
};
use rust_advanced_logger_dxe::{debugln, DEBUG_ERROR, DEBUG_WARN};

use crate::{
  hid::HidContext,
  key_queue::{self, OrdKeyData},
  BOOT_SERVICES,
};

// usages supported by this module
const KEYBOARD_MODIFIER_USAGE_MIN: u32 = 0x000700E0;
const KEYBOARD_MODIFIER_USAGE_MAX: u32 = 0x000700E7;
const KEYBOARD_USAGE_MIN: u32 = 0x00070001;
const KEYBOARD_USAGE_MAX: u32 = 0x00070065;
const LED_USAGE_MIN: u32 = 0x00080001;
const LED_USAGE_MAX: u32 = 0x00080005;

// maps a given field to a routine that handles input from it.
#[derive(Debug, Clone)]
struct ReportFieldWithHandler<T> {
  field: T,
  report_handler: fn(handler: &mut KeyboardHandler, field: T, report: &[u8]),
}

// maps a given field to a routine that builds output reports from it.
#[derive(Debug, Clone)]
struct ReportFieldBuilder<T> {
  field: T,
  field_builder: fn(&mut KeyboardHandler, field: T, report: &mut [u8]),
}

// Defines an input report and the fields of interest in it.
#[derive(Debug, Default, Clone)]
struct KeyboardReportData {
  report_id: Option<ReportId>,
  report_size: usize,
  relevant_variable_fields: Vec<ReportFieldWithHandler<VariableField>>,
  relevant_array_fields: Vec<ReportFieldWithHandler<ArrayField>>,
}

// Defines an output report and the fields of interest in it.
#[derive(Debug, Default, Clone)]
struct KeyboardOutputReportBuilder {
  report_id: Option<ReportId>,
  report_size: usize,
  relevant_variable_fields: Vec<ReportFieldBuilder<VariableField>>,
}

/// Context structure used to track data for this pointer device.
/// Safety: this structure is shared across FFI boundaries, and pointer arithmetic is used on its contents, so it must
/// remain #[repr(C)], and Rust aliasing and concurrency rules must be manually enforced.
#[repr(C)]
pub struct KeyboardContext {
  simple_text_in: protocols::simple_text_input::Protocol,
  simple_text_in_ex: protocols::simple_text_input_ex::Protocol,
  pub handler: KeyboardHandler,
  controller: efi::Handle,
  hid_context: *mut HidContext,
}

/// Tracks all the input reports for this device as well as pointer state.
#[derive(Debug)]
pub struct KeyboardHandler {
  input_reports: BTreeMap<Option<ReportId>, KeyboardReportData>,
  output_builders: Vec<KeyboardOutputReportBuilder>,
  report_id_present: bool,
  last_keys: BTreeSet<Usage>,
  current_keys: BTreeSet<Usage>,
  led_state: BTreeSet<Usage>,
  key_queue: key_queue::KeyQueue,
  notification_callbacks: BTreeMap<usize, (OrdKeyData, protocols::simple_text_input_ex::KeyNotifyFunction)>,
  next_notify_handle: usize,
  key_notify_event: efi::Event,
  hid_io: *mut hid_io::protocol::Protocol,
}

impl KeyboardHandler {
  // processes a report descriptor and yields a KeyboardHandler instance if this descriptor describes input
  // that can be handled by this KeyboardHandler.
  fn process_descriptor(descriptor: &ReportDescriptor) -> Result<Self, efi::Status> {
    let mut handler: KeyboardHandler = KeyboardHandler {
      input_reports: BTreeMap::new(),
      output_builders: Vec::new(),
      report_id_present: false,
      last_keys: BTreeSet::new(),
      current_keys: BTreeSet::new(),
      led_state: BTreeSet::new(),
      key_queue: Default::default(),
      notification_callbacks: BTreeMap::new(),
      next_notify_handle: 0,
      key_notify_event: core::ptr::null_mut(),
      hid_io: core::ptr::null_mut(),
    };

    let multiple_reports =
      descriptor.input_reports.len() > 1 || descriptor.output_reports.len() > 1 || descriptor.features.len() > 1;

    for report in &descriptor.input_reports {
      let mut report_data = KeyboardReportData { report_id: report.report_id, ..Default::default() };

      handler.report_id_present = report.report_id.is_some();

      if multiple_reports && !handler.report_id_present {
        //Invalid to have None ReportId if multiple reports present.
        Err(efi::Status::DEVICE_ERROR)?;
      }

      report_data.report_size = report.size_in_bits.div_ceil(8);

      for field in &report.fields {
        match field {
          //Variable fields (typically used for modifier Usages)
          ReportField::Variable(field) => {
            match field.usage.into() {
              KEYBOARD_MODIFIER_USAGE_MIN..=KEYBOARD_MODIFIER_USAGE_MAX => {
                report_data.relevant_variable_fields.push(ReportFieldWithHandler::<VariableField> {
                  field: field.clone(),
                  report_handler: Self::handle_variable_key,
                })
              }
              _ => (), // other usages irrelevant.
            }
          }
          //Array fields (typically used for key strokes)
          ReportField::Array(field) => {
            for usage_list in &field.usage_list {
              if usage_list.contains(Usage::from(KEYBOARD_USAGE_MIN))
                || usage_list.contains(Usage::from(KEYBOARD_USAGE_MAX))
              {
                report_data.relevant_array_fields.push(ReportFieldWithHandler::<ArrayField> {
                  field: field.clone(),
                  report_handler: Self::handle_array_key,
                });
                break;
              }
            }
          }
          ReportField::Padding(_) => (), // padding irrelevant.
        }
      }
      if report_data.relevant_variable_fields.len() > 0 || report_data.relevant_array_fields.len() > 0 {
        handler.input_reports.insert(report_data.report_id, report_data);
      }
    }

    for report in &descriptor.output_reports {
      let mut report_builder = KeyboardOutputReportBuilder { report_id: report.report_id, ..Default::default() };

      handler.report_id_present = report.report_id.is_some();

      if multiple_reports && !handler.report_id_present {
        //invalid to have None ReportId if multiple reports present.
        Err(efi::Status::DEVICE_ERROR)?;
      }

      report_builder.report_size = report.size_in_bits / 8;
      if (report.size_in_bits % 8) != 0 {
        report_builder.report_size += 1;
      }

      for field in &report.fields {
        match field {
          //Variable fields in output reports (typically used for LEDs).
          ReportField::Variable(field) => {
            match field.usage.into() {
              LED_USAGE_MIN..=LED_USAGE_MAX => {
                report_builder.relevant_variable_fields.push(
                  ReportFieldBuilder {
                    field: field.clone(),
                    field_builder: Self::build_led_report
                  }
                )
              },
              _=> (), //other usages irrelevant.
            }
          },
          ReportField::Array(_) | // No support for array field report outputs; could be added if required.
          ReportField::Padding(_) => (), // padding fields irrelevant.
        }
      }
      if report_builder.relevant_variable_fields.len() > 0 {
        handler.output_builders.push(report_builder);
      }
    }

    if handler.input_reports.len() > 0 || handler.output_builders.len() > 0 {
      Ok(handler)
    } else {
      Err(efi::Status::UNSUPPORTED)
    }
  }

  //Create the Keyboard Context and install the SimpleTextIn/SimpleTextInEx interfaces.
  fn install_keyboard_interfaces(
    self,
    controller: efi::Handle,
    hid_context: *mut HidContext,
  ) -> Result<*mut KeyboardContext, efi::Status> {
    // retrieve a reference to boot services.
    // Safety: BOOT_SERVICES must have been initialized to point to the UEFI Boot Services table.

    // Caller should have ensured this, so just expect on failure.
    let boot_services = unsafe { BOOT_SERVICES.as_mut().expect("BOOT_SERVICES not properly initialized") };

    // Create keyboard context. This context is shared across FFI boundary, so use Box::into_raw.
    // After creation, the only way to access the context (including the handler instance) is via raw pointer.
    let context_ptr = Box::into_raw(Box::new(KeyboardContext {
      handler: self,
      simple_text_in: protocols::simple_text_input::Protocol {
        reset: simple_text_in_reset,
        read_key_stroke: simple_text_in_read_key_stroke,
        wait_for_key: core::ptr::null_mut(),
      },
      simple_text_in_ex: protocols::simple_text_input_ex::Protocol {
        reset: simple_text_in_ex_reset,
        read_key_stroke_ex: simple_text_in_ex_read_key_stroke,
        wait_for_key_ex: core::ptr::null_mut(),
        set_state: simple_text_in_ex_set_state,
        register_key_notify: simple_text_in_ex_register_key_notify,
        unregister_key_notify: simple_text_in_ex_unregister_key_notify,
      },
      controller,
      hid_context,
    }));

    let context = unsafe { context_ptr.as_mut().expect("freshly boxed context pointer is null") };
    //create wait_for_key events.
    let mut wait_for_key_event: efi::Event = core::ptr::null_mut();
    let status = (boot_services.create_event)(
      system::EVT_NOTIFY_WAIT,
      system::TPL_NOTIFY,
      Some(wait_for_key),
      context_ptr as *mut c_void,
      core::ptr::addr_of_mut!(wait_for_key_event),
    );
    if status.is_error() {
      drop(unsafe { Box::from_raw(context_ptr) });
      return Err(status);
    }
    context.simple_text_in.wait_for_key = wait_for_key_event;

    let mut wait_for_key_ex_event: efi::Event = core::ptr::null_mut();
    let status = (boot_services.create_event)(
      system::EVT_NOTIFY_WAIT,
      system::TPL_NOTIFY,
      Some(wait_for_key),
      context_ptr as *mut c_void,
      core::ptr::addr_of_mut!(wait_for_key_ex_event),
    );
    if status.is_error() {
      (boot_services.close_event)(wait_for_key_event);
      drop(unsafe { Box::from_raw(context_ptr) });
      return Err(status);
    }
    context.simple_text_in_ex.wait_for_key_ex = wait_for_key_ex_event;

    //create deferred dispatch event for key notifies. Key notify callbacks are required to be invoked at TPL_CALLBACK
    //per UEFI spec 2.10 section 12.2.5. The keyboard handler interfaces may run at a higher TPL, so this event is used
    //to dispatch the key notifies at the required TPL level.
    let mut key_notify_event: efi::Event = core::ptr::null_mut();
    let status = (boot_services.create_event)(
      system::EVT_NOTIFY_SIGNAL,
      system::TPL_CALLBACK,
      Some(process_key_notifies),
      context_ptr as *mut c_void,
      core::ptr::addr_of_mut!(key_notify_event),
    );
    if status.is_error() {
      (boot_services.close_event)(wait_for_key_event);
      (boot_services.close_event)(wait_for_key_ex_event);
      drop(unsafe { Box::from_raw(context_ptr) });
      return Err(status);
    }
    context.handler.key_notify_event = key_notify_event;

    //install simple_text_in and simple_text_in_ex
    let mut controller = controller;
    let simple_text_in_ptr = raw_field!(context_ptr, KeyboardContext, simple_text_in);
    let status = (boot_services.install_protocol_interface)(
      core::ptr::addr_of_mut!(controller),
      &protocols::simple_text_input::PROTOCOL_GUID as *const efi::Guid as *mut efi::Guid,
      efi::NATIVE_INTERFACE,
      simple_text_in_ptr as *mut c_void,
    );

    if status.is_error() {
      let _ = deinitialize(context_ptr);
      return Err(status);
    }

    let simple_text_in_ex_ptr = raw_field!(context_ptr, KeyboardContext, simple_text_in_ex);
    let status = (boot_services.install_protocol_interface)(
      core::ptr::addr_of_mut!(controller),
      &protocols::simple_text_input_ex::PROTOCOL_GUID as *const efi::Guid as *mut efi::Guid,
      efi::NATIVE_INTERFACE,
      simple_text_in_ex_ptr as *mut c_void,
    );

    if status.is_error() {
      let _ = deinitialize(context_ptr);
      return Err(status);
    }

    context.handler.hid_io = unsafe { (*hid_context).hid_io };

    Ok(context_ptr)
  }

  // Uninstall the SimpleTextIn and SimpleTextInEx interfaces and free the Keyboard context.
  fn uninstall_keyboard_interfaces(keyboard_context: *mut KeyboardContext) -> Result<(), efi::Status> {
    // retrieve a reference to boot services.
    // Safety: BOOT_SERVICES must have been initialized to point to the UEFI Boot Services table.
    // Caller should have ensured this, so just expect on failure.
    let boot_services = unsafe { BOOT_SERVICES.as_mut().expect("BOOT_SERVICES not properly initialized") };

    let simple_text_in_ptr = raw_field!(keyboard_context, KeyboardContext, simple_text_in);
    let simple_text_in_ex_ptr = raw_field!(keyboard_context, KeyboardContext, simple_text_in_ex);

    let mut overall_status = efi::Status::SUCCESS;

    // close the wait_for_key events
    let status = (boot_services.close_event)(unsafe { (*simple_text_in_ptr).wait_for_key });
    if status.is_error() {
      overall_status = status;
    }
    let status = (boot_services.close_event)(unsafe { (*simple_text_in_ex_ptr).wait_for_key_ex });
    if status.is_error() {
      overall_status = status;
    }
    let status = (boot_services.close_event)(unsafe { (*keyboard_context).handler.key_notify_event });
    if status.is_error() {
      overall_status = status;
    }

    //uninstall the protocol interfaces
    let status = (boot_services.uninstall_protocol_interface)(
      unsafe { (*keyboard_context).controller },
      &protocols::simple_text_input::PROTOCOL_GUID as *const efi::Guid as *mut efi::Guid,
      simple_text_in_ptr as *mut c_void,
    );
    if status.is_error() {
      overall_status = status;
    }
    let status = (boot_services.uninstall_protocol_interface)(
      unsafe { (*keyboard_context).controller },
      &protocols::simple_text_input_ex::PROTOCOL_GUID as *const efi::Guid as *mut efi::Guid,
      simple_text_in_ex_ptr as *mut c_void,
    );
    if status.is_error() {
      overall_status = status;
    }

    // take back the context raw pointer
    drop(unsafe { Box::from_raw(keyboard_context) });

    if overall_status.is_error() {
      Err(overall_status)
    } else {
      Ok(())
    }
  }

  // helper routine to handle variable keyboard input report fields
  fn handle_variable_key(&mut self, field: VariableField, report: &[u8]) {
    match field.field_value(report) {
      Some(x) if x != 0 => {
        self.current_keys.insert(field.usage);
      }
      None | Some(_) => (),
    }
  }

  // helper routine to handle array keyboard input report fields
  fn handle_array_key(&mut self, field: ArrayField, report: &[u8]) {
    match field.field_value(report) {
      Some(index) if index != 0 => {
        let mut index = (index as u32 - u32::from(field.logical_minimum)) as usize;
        let usage = field.usage_list.iter().find_map(|x| {
          let range_size = (x.end() - x.start()) as usize;
          if index <= range_size {
            x.range().nth(index)
          } else {
            index = index - range_size as usize;
            None
          }
        });
        if let Some(usage) = usage {
          self.current_keys.insert(Usage::from(usage));
        }
      }
      None | Some(_) => (),
    }
  }

  // process the given input report buffer and handle input from it.
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

      //reset currently active keys to empty set.
      self.current_keys.clear();

      // hand the report data to the handler for each relevant field for field-specific processing.
      for field in report_data.relevant_variable_fields {
        (field.report_handler)(self, field.field, report);
      }

      for field in report_data.relevant_array_fields {
        (field.report_handler)(self, field.field, report);
      }

      //check if any key state has changed.
      if self.last_keys != self.current_keys {
        // process keys that are not in both sets: that is the set of keys that have changed.
        // XOR on the sets yields a set of keys that are in either last or current keys, but not both.

        // Modifier keys need to be processed first so that normal key processing includes modifiers that showed up in
        // the same report. The key sets are sorted by Usage, and modifier keys all have higher usages than normal keys
        // - so use a reverse iterator to process the modifier keys first.
        for changed_key in (&self.last_keys ^ &self.current_keys).into_iter().rev() {
          if self.last_keys.contains(&changed_key) {
            //In the last key list, but not in current. This is a key release.
            self.key_queue.keystroke(changed_key, key_queue::KeyAction::KeyUp);
          } else {
            //Not in last, so must be in current. This is a key press.
            self.key_queue.keystroke(changed_key, key_queue::KeyAction::KeyDown);
          }
        }
        //after processing all the key strokes, check if any keys were pressed that should trigger the notifier callback
        //and if so, signal the event to trigger notify processing at the appropriate TPL.
        if self.key_queue.peek_notify_key().is_some() {
          let boot_services = unsafe { BOOT_SERVICES.as_mut().expect("bad boot services pointer") };
          (boot_services.signal_event)(self.key_notify_event);
        }

        //after processing all the key strokes, send updated LED state if required.
        self.generate_led_output_report();
      }
      //after all key handling is complete for this report, update the last key set to match the current key set.
      self.last_keys = self.current_keys.clone();
    }
  }

  //helper routine that updates the fields in the given report buffer for the given field (called for each field for
  //every LED usage that was discovered in the output report descriptor).
  fn build_led_report(&mut self, field: VariableField, report: &mut [u8]) {
    let status = field.set_field_value(self.led_state.contains(&field.usage).into(), report);
    if status.is_err() {
      debugln!(DEBUG_WARN, "keyobard::build_led_report: failed to set field value: {:?}", status);
    }
  }

  //generates and sends an output report for each output report in the report descriptor that indicated an LED usage
  //for an LED supported by this driver.
  pub fn generate_led_output_report(&mut self) {
    let hid_io = unsafe { self.hid_io.as_mut().expect("bad hidio pointer") };
    let current_leds: BTreeSet<Usage> = self.key_queue.get_active_leds().iter().cloned().collect();
    if current_leds != self.led_state {
      self.led_state = current_leds;
      for output_builder in self.output_builders.clone() {
        let mut report_buffer = vec![0u8; output_builder.report_size];
        for field_builder in &output_builder.relevant_variable_fields {
          (field_builder.field_builder)(self, field_builder.field.clone(), report_buffer.as_mut_slice());
        }
        let report_id: u32 = output_builder.report_id.unwrap_or(0.into()).into();
        let status = (hid_io.set_report)(
          self.hid_io,
          report_id as u8,
          hid_io::protocol::HidReportType::OutputReport,
          report_buffer.len(),
          report_buffer.as_mut_ptr() as *mut c_void,
        );
        if status.is_error() {
          debugln!(DEBUG_WARN, "keyboard::generate_led_report: Set Report failed: {:?}", status);
        }
      }
    }
  }

  // indicates whether this keyboard handler has been initialized with a layout.
  pub(crate) fn is_layout_installed(&self) -> bool {
    self.key_queue.get_layout().is_some()
  }

  // sets the layout to be used by this keyboard handler.
  pub(crate) fn set_layout(&mut self, new_layout: Option<HiiKeyboardLayout>) {
    self.key_queue.set_layout(new_layout);
  }
}

/// Initializes a keyboard handler on the given `controller` to handle reports described by `descriptor`.
///
/// If the device doesn't provide the necessary reports for keyboards, or if there was an error processing the report
/// descriptor data or initializing the pointer handler or installing the simple text protocol instances into the
/// protocol database, an error is returned.
///
/// Otherwise, a [`KeyboardContext`] that can be used to interact with this handler is returned. See [`KeyboardContext`]
/// documentation for constraints on interactions with it.
pub fn initialize(
  controller: efi::Handle,
  descriptor: &ReportDescriptor,
  hid_context_ptr: *mut HidContext,
) -> Result<*mut KeyboardContext, efi::Status> {
  let handler = KeyboardHandler::process_descriptor(descriptor)?;

  let context = handler.install_keyboard_interfaces(controller, hid_context_ptr)?;

  if let Err(err) = initialize_keyboard_layout(context) {
    let _ = deinitialize(context);
    return Err(err);
  }

  let hid_context = unsafe { hid_context_ptr.as_mut().expect("[keyboard::initialize]: bad hid context pointer") };

  hid_context.keyboard_context = context;

  Ok(context)
}

/// De-initializes a keyboard handler described by `context` on the given `controller`.
pub fn deinitialize(context: *mut KeyboardContext) -> Result<(), efi::Status> {
  if context.is_null() {
    return Err(efi::Status::NOT_STARTED);
  }
  KeyboardHandler::uninstall_keyboard_interfaces(context)
}

/// Attempt to retrieve a *mut HidContext for the given controller by locating the simple text input interfaces
/// associated with the controller (if any) and deriving a PointerContext from it (which contains a pointer to the
/// HidContext).
pub fn attempt_to_retrieve_hid_context(
  controller: efi::Handle,
  driver_binding: &driver_binding::Protocol,
) -> Result<*mut HidContext, efi::Status> {
  // retrieve a reference to boot services.
  // Safety: BOOT_SERVICES must have been initialized to point to the UEFI Boot Services table.
  // Caller should have ensured this, so just expect on failure.
  let boot_services = unsafe { BOOT_SERVICES.as_mut().expect("BOOT_SERVICES not properly initialized") };

  let mut simple_text_in_ptr: *mut protocols::simple_text_input::Protocol = core::ptr::null_mut();
  let status = (boot_services.open_protocol)(
    controller,
    &protocols::simple_text_input::PROTOCOL_GUID as *const efi::Guid as *mut efi::Guid,
    core::ptr::addr_of_mut!(simple_text_in_ptr) as *mut *mut c_void,
    driver_binding.driver_binding_handle,
    controller,
    system::OPEN_PROTOCOL_GET_PROTOCOL,
  );

  match status {
    efi::Status::SUCCESS => {
      // retrieve a reference to the pointer context.
      // Safety: `this` must point to an instance of simple_text_in that is contained in a KeyboardContext struct.
      // the following is the equivalent of the `CR` (contained record) macro in the EDK2 reference implementation.
      let context_ptr = unsafe { (simple_text_in_ptr as *mut u8).sub(offset_of!(KeyboardContext, simple_text_in)) }
        as *mut KeyboardContext;
      return Ok(unsafe { (*context_ptr).hid_context });
    }
    err => return Err(err),
  }
}

// Event handler function for the wait_for_key_event that is part of the simple text input interfaces. The same handler
// is used for both simple_text_in and simple_text_in_ex.
extern "efiapi" fn wait_for_key(event: efi::Event, context: *mut c_void) {
  // retrieve a reference to the
  let keyboard_context = unsafe { (context as *mut KeyboardContext).as_mut().expect("Invalid context pointer") };

  // retrieve a reference to boot services.
  // Safety: BOOT_SERVICES must have been initialized to point to the UEFI Boot Services table.
  // Caller should have ensured this, so just expect on failure.
  let boot_services = unsafe { BOOT_SERVICES.as_mut().expect("BOOT_SERVICES not properly initialized") };

  let old_tpl = (boot_services.raise_tpl)(system::TPL_NOTIFY);

  loop {
    if let Some(key_data) = keyboard_context.handler.key_queue.peek_key() {
      //skip partials
      if key_data.key.unicode_char == 0 && key_data.key.scan_code == 0 {
        // consume (and ignore) the partial stroke.
        let _ = keyboard_context.handler.key_queue.pop_key();
        continue;
      }
      //live non-partial key at front of queue; so signal event.
      (boot_services.signal_event)(event);
    }
    break;
  }

  (boot_services.restore_tpl)(old_tpl);
}

// Event callback function for handling registered key notifications. Iterates over the queue of keys to be notified,
// and invokes the registered callback function for each of those keys.
extern "efiapi" fn process_key_notifies(_event: efi::Event, context: *mut c_void) {
  let keyboard_context = unsafe { (context as *mut KeyboardContext).as_mut().expect("Invalid context pointer") };

  // retrieve a reference to boot services.
  // Safety: BOOT_SERVICES must have been initialized to point to the UEFI Boot Services table.
  // Caller should have ensured this, so just expect on failure.
  let boot_services = unsafe { BOOT_SERVICES.as_mut().expect("BOOT_SERVICES not properly initialized") };

  // loop through all the keys in the notification function queue and invoke the associated callback if found
  loop {
    //Safety: access to the key_queue needs to be at TPL_NOTIFY to ensure mutual exclusion, but this routine runs at
    //TPL_CALLBACK. So raise TPL to TPL_NOTIFY for the queue pop and callback function search.
    let old_tpl = (boot_services.raise_tpl)(system::TPL_NOTIFY);

    let (key, callback) = match keyboard_context.handler.key_queue.pop_notifiy_key() {
      Some(key) => {
        let mut result = (None, None);
        for (registered_key, callback) in keyboard_context.handler.notification_callbacks.values() {
          if OrdKeyData(key).matches_registered_key(registered_key) {
            result = (Some(key), Some(callback));
            break;
          }
        }
        result
      }
      None => (None, None),
    };
    //restore TPL back to TPL_CALLBACK before actually invoking the callback.
    (boot_services.restore_tpl)(old_tpl);

    //Invoke the callback, if found.
    //Safety: this assumes that a caller doesn't "unregister" a callback and render the pointer invalid at TPL_NOTIFY
    //between the search above that returns the callback pointer and the actual callback invocation here. This is a
    //spec requirement (see UEFI spec 2.10 table 7.3).
    if let (Some(mut key), Some(callback)) = (key, callback) {
      let key_data_ptr = &mut key as *mut KeyData;
      let _status = callback(key_data_ptr);
    } else {
      return;
    }
  }
}

// resets the keyboard state - part of the simple_text_in protocol interface.
extern "efiapi" fn simple_text_in_reset(
  this: *mut protocols::simple_text_input::Protocol,
  _extended_verification: efi::Boolean,
) -> efi::Status {
  if this.is_null() {
    return efi::Status::INVALID_PARAMETER;
  }
  // retrieve a reference to the keyboard context.
  // Safety: `this` must point to an instance of simple_text)input that is contained in a KeyboardContext struct.
  // the following is the equivalent of the `CR` (contained record) macro in the EDK2 reference implementation.
  let context_ptr =
    unsafe { (this as *mut u8).sub(offset_of!(KeyboardContext, simple_text_in)) } as *mut KeyboardContext;

  let keyboard_context = unsafe { context_ptr.as_mut().expect("Invalid context pointer") };

  keyboard_context.handler.last_keys.clear();
  keyboard_context.handler.current_keys.clear();
  keyboard_context.handler.key_queue.reset();

  efi::Status::SUCCESS
}

// reads a key stroke - part of the simple_text_in protocol interface.
extern "efiapi" fn simple_text_in_read_key_stroke(
  this: *mut protocols::simple_text_input::Protocol,
  key: *mut protocols::simple_text_input::InputKey,
) -> efi::Status {
  if this.is_null() || key.is_null() {
    return efi::Status::INVALID_PARAMETER;
  }
  // retrieve a reference to the keyboard context.
  // Safety: `this` must point to an instance of simple_text)input that is contained in a KeyboardContext struct.
  // the following is the equivalent of the `CR` (contained record) macro in the EDK2 reference implementation.
  let context_ptr =
    unsafe { (this as *mut u8).sub(offset_of!(KeyboardContext, simple_text_in)) } as *mut KeyboardContext;

  let keyboard_context = unsafe { context_ptr.as_mut().expect("Invalid context pointer") };

  loop {
    if let Some(mut key_data) = keyboard_context.handler.key_queue.pop_key() {
      // skip partials
      if key_data.key.unicode_char == 0 && key_data.key.scan_code == 0 {
        continue;
      }
      //translate ctrl-alpha to corresponding control value. ctrl-a = 0x0001, ctrl-z = 0x001A
      if (key_data.key_state.key_shift_state & (LEFT_SHIFT_PRESSED | RIGHT_SHIFT_PRESSED)) != 0 {
        if key_data.key.unicode_char >= 0x0061 && key_data.key.unicode_char <= 0x007a {
          //'a' to 'z'
          key_data.key.unicode_char = (key_data.key.unicode_char - 0x0061) + 1;
        }
        if key_data.key.unicode_char >= 0x0041 && key_data.key.unicode_char <= 0x005a {
          //'A' to 'Z'
          key_data.key.unicode_char = (key_data.key.unicode_char - 0x0041) + 1;
        }
      }
      unsafe { key.write(key_data.key) }
      return efi::Status::SUCCESS;
    } else {
      return efi::Status::NOT_READY;
    }
  }
}

// resets the keyboard state - part of the simple_text_in_ex protocol interface.
extern "efiapi" fn simple_text_in_ex_reset(
  this: *mut protocols::simple_text_input_ex::Protocol,
  _extended_verification: efi::Boolean,
) -> efi::Status {
  if this.is_null() {
    return efi::Status::INVALID_PARAMETER;
  }
  // retrieve a reference to the keyboard context.
  // Safety: `this` must point to an instance of simple_text)input that is contained in a KeyboardContext struct.
  // the following is the equivalent of the `CR` (contained record) macro in the EDK2 reference implementation.
  let context_ptr =
    unsafe { (this as *mut u8).sub(offset_of!(KeyboardContext, simple_text_in_ex)) } as *mut KeyboardContext;

  let keyboard_context = unsafe { context_ptr.as_mut().expect("Invalid context pointer") };

  keyboard_context.handler.last_keys.clear();
  keyboard_context.handler.current_keys.clear();
  keyboard_context.handler.key_queue.reset();

  efi::Status::SUCCESS
}

// reads a key stroke - part of the simple_text_in_ex protocol interface.
extern "efiapi" fn simple_text_in_ex_read_key_stroke(
  this: *mut protocols::simple_text_input_ex::Protocol,
  key_data: *mut protocols::simple_text_input_ex::KeyData,
) -> efi::Status {
  if this.is_null() || key_data.is_null() {
    return efi::Status::INVALID_PARAMETER;
  }
  // retrieve a reference to the keyboard context.
  // Safety: `this` must point to an instance of simple_text)input that is contained in a KeyboardContext struct.
  // the following is the equivalent of the `CR` (contained record) macro in the EDK2 reference implementation.
  let context_ptr =
    unsafe { (this as *mut u8).sub(offset_of!(KeyboardContext, simple_text_in_ex)) } as *mut KeyboardContext;

  let keyboard_context = unsafe { context_ptr.as_mut().expect("Invalid context pointer") };

  if let Some(key) = keyboard_context.handler.key_queue.pop_key() {
    unsafe { key_data.write(key) };
    efi::Status::SUCCESS
  } else {
    let mut key: KeyData = Default::default();
    key.key_state = keyboard_context.handler.key_queue.init_key_state();
    unsafe { key_data.write(key) };
    efi::Status::NOT_READY
  }
}

// sets the keyboard state - part of the simple_text_in_ex protocol interface.
extern "efiapi" fn simple_text_in_ex_set_state(
  this: *mut protocols::simple_text_input_ex::Protocol,
  key_toggle_state: *mut protocols::simple_text_input_ex::KeyToggleState,
) -> efi::Status {
  if this.is_null() || key_toggle_state.is_null() {
    return efi::Status::INVALID_PARAMETER;
  }
  // retrieve a reference to the keyboard context.
  // Safety: `this` must point to an instance of simple_text)input that is contained in a KeyboardContext struct.
  // the following is the equivalent of the `CR` (contained record) macro in the EDK2 reference implementation.
  let context_ptr =
    unsafe { (this as *mut u8).sub(offset_of!(KeyboardContext, simple_text_in_ex)) } as *mut KeyboardContext;

  let keyboard_context = unsafe { context_ptr.as_mut().expect("Invalid context pointer") };

  let key_toggle_state = unsafe { key_toggle_state.as_mut().expect("Invalid key toggle state pointer") };

  keyboard_context.handler.key_queue.set_key_toggle_state(*key_toggle_state);

  keyboard_context.handler.generate_led_output_report();

  efi::Status::SUCCESS
}

// registers a key notification callback function - part of the simple_text_in_ex protocol interface.
extern "efiapi" fn simple_text_in_ex_register_key_notify(
  this: *mut protocols::simple_text_input_ex::Protocol,
  key_data_ptr: *mut protocols::simple_text_input_ex::KeyData,
  key_notification_function: protocols::simple_text_input_ex::KeyNotifyFunction,
  notify_handle: *mut *mut c_void,
) -> efi::Status {
  //TODO: should check key_notification_function against NULL, but the RUST way to do that would be to declare it
  //as Option<KeyNotifyFunction> - which would require a change to r_efi to do properly.
  if this.is_null() || key_data_ptr.is_null() || notify_handle.is_null() {
    return efi::Status::INVALID_PARAMETER;
  }

  // retrieve a reference to the keyboard context.
  // Safety: `this` must point to an instance of simple_text)input that is contained in a KeyboardContext struct.
  // the following is the equivalent of the `CR` (contained record) macro in the EDK2 reference implementation.
  let context_ptr =
    unsafe { (this as *mut u8).sub(offset_of!(KeyboardContext, simple_text_in_ex)) } as *mut KeyboardContext;

  let keyboard_context = unsafe { context_ptr.as_mut().expect("Invalid context pointer") };

  let key_data = OrdKeyData(*unsafe { key_data_ptr.as_mut().expect("Bad key_data_ptr") });

  for (handle, entry) in &keyboard_context.handler.notification_callbacks {
    if entry.0 == key_data && entry.1 == key_notification_function {
      //if callback already exists, just return current handle
      unsafe { notify_handle.write(*handle as *mut c_void) };
      return efi::Status::SUCCESS;
    }
  }

  // key_data/callback combo doesn't already exist; create a new registration for it.
  keyboard_context.handler.next_notify_handle += 1;

  keyboard_context
    .handler
    .notification_callbacks
    .insert(keyboard_context.handler.next_notify_handle, (key_data.clone(), key_notification_function));

  keyboard_context.handler.key_queue.add_notify_key(key_data);

  efi::Status::SUCCESS
}

// unregisters a key notification callback function - part of the simple_text_in_ex protocol interface.
extern "efiapi" fn simple_text_in_ex_unregister_key_notify(
  this: *mut protocols::simple_text_input_ex::Protocol,
  notification_handle: *mut c_void,
) -> efi::Status {
  if this.is_null() || notification_handle as usize == 0 {
    return efi::Status::INVALID_PARAMETER;
  }

  // retrieve a reference to the keyboard context.
  // Safety: `this` must point to an instance of simple_text)input that is contained in a KeyboardContext struct.
  // the following is the equivalent of the `CR` (contained record) macro in the EDK2 reference implementation.
  let context_ptr =
    unsafe { (this as *mut u8).sub(offset_of!(KeyboardContext, simple_text_in_ex)) } as *mut KeyboardContext;

  let keyboard_context = unsafe { context_ptr.as_mut().expect("Invalid context pointer") };

  let handle = notification_handle as usize;

  let entry = keyboard_context.handler.notification_callbacks.remove(&handle);
  let Some(entry) = entry else {
    return efi::Status::INVALID_PARAMETER;
  };

  let other_handlers_exist =
    keyboard_context.handler.notification_callbacks.values().any(|(key, _callback)| *key == entry.0);

  if !other_handlers_exist {
    keyboard_context.handler.key_queue.remove_notify_key(&entry.0);
  }

  efi::Status::SUCCESS
}

// Initializes keyboard layout support. Creates an event to fire a callback when a new keyboard layout is installed
// into HII database, and then installs a default keyboard layout if one is not already present.
pub(crate) fn initialize_keyboard_layout(context_ptr: *mut KeyboardContext) -> Result<(), efi::Status> {
  // retrieve a reference to boot services.
  // Safety: BOOT_SERVICES must have been initialized to point to the UEFI Boot Services table.
  // Caller should have ensured this, so just expect on failure.
  let boot_services = unsafe { BOOT_SERVICES.as_mut().expect("BOOT_SERVICES not properly initialized") };

  //create layout update event.
  let mut layout_change_event: efi::Event = core::ptr::null_mut();
  let status = (boot_services.create_event_ex)(
    system::EVT_NOTIFY_SIGNAL,
    system::TPL_NOTIFY,
    Some(on_layout_update),
    context_ptr as *mut c_void,
    &protocols::hii_database::SET_KEYBOARD_LAYOUT_EVENT_GUID,
    core::ptr::addr_of_mut!(layout_change_event),
  );
  if status.is_error() {
    Err(status)?;
  }

  //signal event to pick up any existing layout.
  let status = (boot_services.signal_event)(layout_change_event);
  if status.is_error() {
    Err(status)?;
  }

  //if no layout installed, install the default layout.
  if !unsafe { (*context_ptr).handler.is_layout_installed() } {
    install_default_layout(boot_services)?;
  }

  Ok(())
}

// Installs a default keyboard layout into the HII database.
pub(crate) fn install_default_layout(boot_services: &mut system::BootServices) -> Result<(), efi::Status> {
  let mut hii_database_protocol_ptr: *mut protocols::hii_database::Protocol = core::ptr::null_mut();

  let status = (boot_services.locate_protocol)(
    &protocols::hii_database::PROTOCOL_GUID as *const efi::Guid as *mut efi::Guid,
    core::ptr::null_mut(),
    core::ptr::addr_of_mut!(hii_database_protocol_ptr) as *mut *mut c_void,
  );

  if status.is_error() {
    debugln!(
      DEBUG_ERROR,
      "keyboard::install_default_layout: Could not locate hii_database protocol to install keyboard layout: {:x?}",
      status
    );
    Err(status)?;
  }

  let hii_database_protocol =
    unsafe { hii_database_protocol_ptr.as_mut().expect("Bad pointer returned from successful locate protocol.") };

  let mut hii_handle: hii::Handle = core::ptr::null_mut();
  let status = (hii_database_protocol.new_package_list)(
    hii_database_protocol_ptr,
    hii_keyboard_layout::get_default_keyboard_pkg_list_buffer().as_ptr() as *const hii::PackageListHeader,
    core::ptr::null_mut(),
    core::ptr::addr_of_mut!(hii_handle),
  );

  if status.is_error() {
    debugln!(DEBUG_ERROR, "keyboard::install_default_layout: Failed to install keyboard layout package: {:x?}", status);
    Err(status)?;
  }

  let status = (hii_database_protocol.set_keyboard_layout)(
    hii_database_protocol_ptr,
    &hii_keyboard_layout::DEFAULT_KEYBOARD_LAYOUT_GUID as *const efi::Guid as *mut efi::Guid,
  );

  if status.is_error() {
    debugln!(DEBUG_ERROR, "keyboard::install_default_layout: Failed to set keyboard layout: {:x?}", status);
    Err(status)?;
  }

  Ok(())
}

// Event callback that is fired on keyboard layout update event in HII.
extern "efiapi" fn on_layout_update(_event: efi::Event, context: *mut c_void) {
  let keyboard_context = unsafe { (context as *mut KeyboardContext).as_mut().expect("Bad context pointer") };

  // retrieve a reference to boot services.
  // Safety: BOOT_SERVICES must have been initialized to point to the UEFI Boot Services table.
  // Caller should have ensured this, so just expect on failure.
  let boot_services = unsafe { BOOT_SERVICES.as_mut().expect("BOOT_SERVICES not properly initialized") };

  let mut hii_database_protocol_ptr: *mut protocols::hii_database::Protocol = core::ptr::null_mut();
  let status = (boot_services.locate_protocol)(
    &protocols::hii_database::PROTOCOL_GUID as *const efi::Guid as *mut efi::Guid,
    core::ptr::null_mut(),
    core::ptr::addr_of_mut!(hii_database_protocol_ptr) as *mut *mut c_void,
  );

  if status.is_error() {
    //nothing to do if there is no hii protocol.
    return;
  }

  let hii_database_protocol =
    unsafe { hii_database_protocol_ptr.as_mut().expect("Bad pointer returned from successful locate protocol.") };

  let mut keyboard_layout_buffer = vec![0u8; 4096];
  let mut layout_buffer_len: u16 = keyboard_layout_buffer.len() as u16;

  let status = (hii_database_protocol.get_keyboard_layout)(
    hii_database_protocol_ptr,
    core::ptr::null_mut(),
    &mut layout_buffer_len as *mut u16,
    keyboard_layout_buffer.as_mut_ptr() as *mut protocols::hii_database::KeyboardLayout<0>,
  );

  if status.is_error() {
    return;
  }

  let keyboard_layout = hii_keyboard_layout::keyboard_layout_from_buffer(&keyboard_layout_buffer);
  match keyboard_layout {
    Ok(keyboard_layout) => {
      keyboard_context.handler.set_layout(Some(keyboard_layout));
    }
    Err(_) => {
      debugln!(DEBUG_WARN, "keyboard::on_layout_update: Could not parse keyboard layout buffer.");
      return;
    }
  }
}
