//! Provides Pointer HID support.
//!
//! This module handles the core logic for processing pointer input from HID
//! devices.
//!
//! ## License
//!
//! Copyright (C) Microsoft Corporation. All rights reserved.
//!
//! SPDX-License-Identifier: BSD-2-Clause-Patent
//!
mod absolute_pointer;

use alloc::{
  collections::{BTreeMap, BTreeSet},
  vec::Vec,
};
use hidparser::{
  report_data_types::{ReportId, Usage},
  ReportDescriptor, ReportField, VariableField,
};
use r_efi::{efi, protocols};

use rust_advanced_logger_dxe::{debugln, function, DEBUG_ERROR};

use crate::{
  boot_services::UefiBootServices,
  hid_io::{HidIo, HidReportReceiver},
};

use self::absolute_pointer::PointerContext;

// Usages supported by this module.
const GENERIC_DESKTOP_X: u32 = 0x00010030;
const GENERIC_DESKTOP_Y: u32 = 0x00010031;
const GENERIC_DESKTOP_Z: u32 = 0x00010032;
const GENERIC_DESKTOP_WHEEL: u32 = 0x00010038;
const BUTTON_PAGE: u16 = 0x0009;
const BUTTON_MIN: u32 = 0x00090001;
const BUTTON_MAX: u32 = 0x00090020; //Per spec, the Absolute Pointer protocol supports a 32-bit button state field.

// number of points on the X/Y axis for this implementation.
const AXIS_RESOLUTION: u64 = 1024;
const CENTER: u64 = AXIS_RESOLUTION / 2;

// Maps a given field to a routine that handles input from it.
#[derive(Debug, Clone)]
struct ReportFieldWithHandler {
  field: VariableField,
  report_handler: fn(&mut PointerHidHandler, field: VariableField, report: &[u8]),
}

// Defines a report and the fields of interest within it.
#[derive(Debug, Default, Clone)]
struct PointerReportData {
  report_id: Option<ReportId>,
  report_size: usize,
  relevant_fields: Vec<ReportFieldWithHandler>,
}

/// Pointer HID Handler
pub struct PointerHidHandler {
  boot_services: &'static dyn UefiBootServices,
  agent: efi::Handle,
  controller: Option<efi::Handle>,
  input_reports: BTreeMap<Option<ReportId>, PointerReportData>,
  supported_usages: BTreeSet<Usage>,
  report_id_present: bool,
  state_changed: bool,
  current_state: protocols::absolute_pointer::State,
}

impl PointerHidHandler {
  /// Instantiates a new Pointer HID handler. `agent` is the handle that owns the handler (typically image_handle).
  pub fn new(boot_services: &'static dyn UefiBootServices, agent: efi::Handle) -> Self {
    let mut handler = Self {
      boot_services,
      agent,
      controller: None,
      input_reports: BTreeMap::new(),
      supported_usages: BTreeSet::new(),
      report_id_present: false,
      state_changed: false,
      current_state: Default::default(),
    };
    handler.reset_state();
    handler
  }

  // Processes the report descriptor to determine whether this is a supported device, and if so, extract the information
  // required to process reports.
  fn process_descriptor(&mut self, descriptor: ReportDescriptor) -> Result<(), efi::Status> {
    let multiple_reports = descriptor.input_reports.len() > 1;

    for report in &descriptor.input_reports {
      let mut report_data = PointerReportData { report_id: report.report_id, ..Default::default() };

      self.report_id_present = report.report_id.is_some();

      if multiple_reports && !self.report_id_present {
        //invalid to have None ReportId if multiple reports present.
        Err(efi::Status::DEVICE_ERROR)?;
      }

      report_data.report_size = report.size_in_bits.div_ceil(8);

      for field in &report.fields {
        if let ReportField::Variable(field) = field {
          match field.usage.into() {
            GENERIC_DESKTOP_X => {
              let field_handler = ReportFieldWithHandler { field: field.clone(), report_handler: Self::x_axis_handler };
              report_data.relevant_fields.push(field_handler);
              self.supported_usages.insert(field.usage);
            }
            GENERIC_DESKTOP_Y => {
              let field_handler = ReportFieldWithHandler { field: field.clone(), report_handler: Self::y_axis_handler };
              report_data.relevant_fields.push(field_handler);
              self.supported_usages.insert(field.usage);
            }
            GENERIC_DESKTOP_Z | GENERIC_DESKTOP_WHEEL => {
              let field_handler = ReportFieldWithHandler { field: field.clone(), report_handler: Self::z_axis_handler };
              report_data.relevant_fields.push(field_handler);
              self.supported_usages.insert(field.usage);
            }
            BUTTON_MIN..=BUTTON_MAX => {
              let field_handler = ReportFieldWithHandler { field: field.clone(), report_handler: Self::button_handler };
              report_data.relevant_fields.push(field_handler);
              self.supported_usages.insert(field.usage);
            }
            _ => (), //other usages irrelevant
          }
        }
      }

      if !report_data.relevant_fields.is_empty() {
        self.input_reports.insert(report_data.report_id, report_data);
      }
    }

    if !self.input_reports.is_empty() {
      Ok(())
    } else {
      Err(efi::Status::UNSUPPORTED)
    }
  }

  // Helper routine that handles projecting relative and absolute axis reports onto the fixed
  // absolute report axis that this driver produces.
  fn resolve_axis(current_value: u64, field: VariableField, report: &[u8]) -> Option<u64> {
    if field.attributes.relative {
      //for relative, just update and clamp the current state.
      let new_value = current_value as i64 + field.field_value(report)?;
      Some(new_value.clamp(0, AXIS_RESOLUTION as i64) as u64)
    } else {
      //for absolute, project onto 0..AXIS_RESOLUTION
      let mut new_value = field.field_value(report)?;

      //translate to zero.
      new_value = new_value.checked_sub(i32::from(field.logical_minimum) as i64)?;

      //scale to AXIS_RESOLUTION
      new_value = (new_value * AXIS_RESOLUTION as i64 * 1000) / (field.field_range()? as i64 * 1000);

      Some(new_value.clamp(0, AXIS_RESOLUTION as i64) as u64)
    }
  }

  // handles x_axis inputs
  fn x_axis_handler(&mut self, field: VariableField, report: &[u8]) {
    if let Some(x_value) = Self::resolve_axis(self.current_state.current_x, field, report) {
      if self.current_state.current_x != x_value {
        self.current_state.current_x = x_value;
        self.state_changed = true;
      }
    }
  }

  // handles y_axis inputs
  fn y_axis_handler(&mut self, field: VariableField, report: &[u8]) {
    if let Some(y_value) = Self::resolve_axis(self.current_state.current_y, field, report) {
      if self.current_state.current_y != y_value {
        self.current_state.current_y = y_value;
        self.state_changed = true;
      }
    }
  }

  // handles z_axis inputs
  fn z_axis_handler(&mut self, field: VariableField, report: &[u8]) {
    if let Some(z_value) = Self::resolve_axis(self.current_state.current_z, field, report) {
      if self.current_state.current_z != z_value {
        self.current_state.current_z = z_value;
        self.state_changed = true;
      }
    }
  }

  // handles button inputs
  fn button_handler(&mut self, field: VariableField, report: &[u8]) {
    let shift: u32 = field.usage.into();
    if !(BUTTON_MIN..=BUTTON_MAX).contains(&shift) {
      return;
    }

    if let Some(button_value) = field.field_value(report) {
      let button_value = button_value as u32;

      let shift = shift - BUTTON_MIN;
      if shift > u32::BITS {
        return;
      }
      let button_value = button_value << shift;

      let new_buttons = self.current_state.active_buttons
        & !(1 << shift)  // zero the relevant bit in the button state field.
        | button_value; // or in the current button state into that bit position.

      if new_buttons != self.current_state.active_buttons {
        self.current_state.active_buttons = new_buttons;
        self.state_changed = true;
      }
    }
  }

  fn reset_state(&mut self) {
    self.current_state = Default::default();
    // initialize pointer to center of screen
    self.current_state.current_x = CENTER;
    self.current_state.current_y = CENTER;
    self.state_changed = false;
  }
}

impl HidReportReceiver for PointerHidHandler {
  fn initialize(&mut self, controller: efi::Handle, hid_io: &dyn HidIo) -> Result<(), efi::Status> {
    let descriptor = hid_io.get_report_descriptor()?;
    self.process_descriptor(descriptor)?;

    PointerContext::install(self.boot_services, controller, self)?;

    self.controller = Some(controller);

    Ok(())
  }
  fn receive_report(&mut self, report: &[u8], _hid_io: &dyn HidIo) {
    let old_tpl = self.boot_services.raise_tpl(efi::TPL_NOTIFY);

    'report_processing: {
      if report.is_empty() {
        break 'report_processing;
      }

      // determine whether report includes report id byte and adjust the buffer as needed.
      let (report_id, report) = match self.report_id_present {
        true => (Some(ReportId::from(&report[0..1])), &report[1..]),
        false => (None, &report[0..]),
      };

      if report.is_empty() {
        break 'report_processing;
      }

      if let Some(report_data) = self.input_reports.get(&report_id).cloned() {
        if report.len() != report_data.report_size {
          break 'report_processing;
        }

        // hand the report data to the handler for each relevant field for field-specific processing.
        for field in report_data.relevant_fields {
          (field.report_handler)(self, field.field, report);
        }
      }
    }

    self.boot_services.restore_tpl(old_tpl);
  }
}

impl Drop for PointerHidHandler {
  fn drop(&mut self) {
    if let Some(controller) = self.controller {
      let status = PointerContext::uninstall(self.boot_services, self.agent, controller);
      if status.is_err() {
        debugln!(DEBUG_ERROR, "{:?}: Failed to uninstall absolute_pointer: {:?}", function!(), status);
      }
    }
  }
}

#[cfg(test)]
mod test {
  use core::{cmp::min, ffi::c_void};

  use crate::{
    boot_services::MockUefiBootServices,
    hid_io::{HidReportReceiver, MockHidIo},
    pointer::{AXIS_RESOLUTION, CENTER},
  };
  use r_efi::efi;

  use super::PointerHidHandler;

  static MINIMAL_BOOT_KEYBOARD_REPORT_DESCRIPTOR: &[u8] = &[
    0x05, 0x01, // USAGE_PAGE (Generic Desktop)
    0x09, 0x06, // USAGE (Keyboard)
    0xa1, 0x01, // COLLECTION (Application)
    0x75, 0x01, //    REPORT_SIZE (1)
    0x95, 0x08, //    REPORT_COUNT (8)
    0x05, 0x07, //    USAGE_PAGE (Key Codes)
    0x19, 0xE0, //    USAGE_MINIMUM (224)
    0x29, 0xE7, //    USAGE_MAXIMUM (231)
    0x15, 0x00, //    LOGICAL_MINIMUM (0)
    0x25, 0x01, //    LOGICAL_MAXIMUM (1)
    0x81, 0x02, //    INPUT (Data, Var, Abs) (Modifier Byte)
    0xc0, // END_COLLECTION
  ];

  static MOUSE_REPORT_DESCRIPTOR: &[u8] = &[
    0x05, 0x01, // USAGE_PAGE (Generic Desktop)
    0x09, 0x02, // USAGE (Mouse)
    0xa1, 0x01, // COLLECTION (Application)
    0x09, 0x01, //   USAGE(Pointer)
    0xa1, 0x00, //   COLLECTION (Physical)
    0x05, 0x09, //     USAGE_PAGE (Button)
    0x19, 0x01, //     USAGE_MINIMUM(1)
    0x29, 0x05, //     USAGE_MAXIMUM(5)
    0x15, 0x00, //     LOGICAL_MINIMUM(0)
    0x25, 0x01, //     LOGICAL_MAXIMUM(1)
    0x95, 0x05, //     REPORT_COUNT(5)
    0x75, 0x01, //     REPORT_SIZE(1)
    0x81, 0x02, //     INPUT(Data, Variable, Absolute)
    0x95, 0x01, //     REPORT_COUNT(1)
    0x75, 0x03, //     REPORT_SIZE(3)
    0x81, 0x01, //     INPUT(Constant, Array, Absolute)
    0x05, 0x01, //     USAGE_PAGE (Generic Desktop)
    0x09, 0x30, //     USAGE (X)
    0x09, 0x31, //     USAGE (Y)
    0x09, 0x38, //     USAGE (Wheel)
    0x15, 0x81, //     LOGICAL_MINIMUM (-127)
    0x25, 0x7f, //     LOGICAL_MAXIMUM (127)
    0x75, 0x08, //     REPORT_SIZE (8)
    0x95, 0x03, //     REPORT_COUNT (3)
    0x81, 0x06, //     INPUT(Data, Variable, Relative)
    0xc0, //   END_COLLECTION
    0xc0, // END_COLLECTION
  ];

  static ABS_POINTER_REPORT_DESCRIPTOR: &[u8] = &[
    0x05, 0x01, // USAGE_PAGE (Generic Desktop)
    0x09, 0x02, // USAGE (Mouse)
    0xa1, 0x01, // COLLECTION (Application)
    0x09, 0x01, //   USAGE(Pointer)
    0xa1, 0x00, //   COLLECTION (Physical)
    0x05, 0x09, //     USAGE_PAGE (Button)
    0x19, 0x01, //     USAGE_MINIMUM(1)
    0x29, 0x05, //     USAGE_MAXIMUM(5)
    0x15, 0x00, //     LOGICAL_MINIMUM(0)
    0x25, 0x01, //     LOGICAL_MAXIMUM(1)
    0x95, 0x05, //     REPORT_COUNT(5)
    0x75, 0x01, //     REPORT_SIZE(1)
    0x81, 0x02, //     INPUT(Data, Variable, Absolute)
    0x95, 0x01, //     REPORT_COUNT(1)
    0x75, 0x03, //     REPORT_SIZE(3)
    0x81, 0x01, //     INPUT(Constant, Array, Absolute)
    0x05, 0x01, //     USAGE_PAGE (Generic Desktop)
    0x09, 0x30, //     USAGE (X)
    0x09, 0x31, //     USAGE (Y)
    0x09, 0x38, //     USAGE (Wheel)
    0x15, 0x00, //     LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x0f, // LOGICAL_MAXIMUM (4095)
    0x75, 0x10, //     REPORT_SIZE (16)
    0x95, 0x03, //     REPORT_COUNT (3)
    0x81, 0x02, //     INPUT(Data, Variable, Absolute)
    0xc0, //   END_COLLECTION
    0xc0, // END_COLLECTION
  ];

  // In this module, the usage model for boot_services is global static, and so &'static dyn UefiBootServices is used
  // throughout the API. For testing, each test will have a different set of expectations on the UefiBootServices mock
  // object, and the mock object itself expects to be "mut", which makes it hard to handle as a single global static.
  // Instead, raw pointers are used to simulate a MockUefiBootServices instance with 'static lifetime.
  // This object needs to outlive anything that uses it - once created, it will live until the end of the program.
  fn create_fake_static_boot_service() -> &'static mut MockUefiBootServices {
    unsafe { Box::into_raw(Box::new(MockUefiBootServices::new())).as_mut().unwrap() }
  }

  #[test]
  fn pointer_initialize_should_fail_if_report_descriptor_not_supported() {
    let boot_services = create_fake_static_boot_service();

    let agent = 0x1 as efi::Handle;
    let mut pointer_handler = PointerHidHandler::new(boot_services, agent);
    let mut hid_io = MockHidIo::new();
    hid_io
      .expect_get_report_descriptor()
      .returning(|| Ok(hidparser::parse_report_descriptor(&MINIMAL_BOOT_KEYBOARD_REPORT_DESCRIPTOR).unwrap()));

    let controller = 0x2 as efi::Handle;
    assert_eq!(pointer_handler.initialize(controller, &hid_io), Err(efi::Status::UNSUPPORTED));
  }

  #[test]
  fn successful_pointer_initialize_should_install_protocol_and_drop_should_tear_it_down() {
    let boot_services = create_fake_static_boot_service();
    static mut ABS_PTR_INTERFACE: *mut c_void = core::ptr::null_mut();

    // expected on PointerHidHandler::initialize().
    boot_services.expect_create_event().returning(|_, _, _, _, _| efi::Status::SUCCESS);
    boot_services.expect_install_protocol_interface().returning(|_, _, _, interface| {
      unsafe { ABS_PTR_INTERFACE = interface };
      efi::Status::SUCCESS
    });

    // expected on PointerHidHandler::drop().
    boot_services.expect_open_protocol().returning(|_, _, interface, _, _, _| {
      unsafe { *interface = ABS_PTR_INTERFACE };
      efi::Status::SUCCESS
    });
    boot_services.expect_uninstall_protocol_interface().returning(|_, _, _| efi::Status::SUCCESS);
    boot_services.expect_close_event().returning(|_| efi::Status::SUCCESS);

    let agent = 0x1 as efi::Handle;
    let mut pointer_handler = PointerHidHandler::new(boot_services, agent);
    let mut hid_io = MockHidIo::new();
    hid_io
      .expect_get_report_descriptor()
      .returning(|| Ok(hidparser::parse_report_descriptor(&MOUSE_REPORT_DESCRIPTOR).unwrap()));

    let controller = 0x2 as efi::Handle;
    assert_eq!(pointer_handler.initialize(controller, &hid_io), Ok(()));

    assert_ne!(unsafe { ABS_PTR_INTERFACE }, core::ptr::null_mut());
  }

  #[test]
  fn receive_report_should_process_relative_reports() {
    let boot_services = create_fake_static_boot_service();

    static mut ABS_PTR_INTERFACE: *mut c_void = core::ptr::null_mut();

    // expected on PointerHidHandler::initialize().
    boot_services.expect_create_event().returning(|_, _, _, _, _| efi::Status::SUCCESS);
    boot_services.expect_install_protocol_interface().returning(|_, _, _, interface| {
      unsafe { ABS_PTR_INTERFACE = interface };
      efi::Status::SUCCESS
    });

    // expected on PointerHidHandler::drop().
    boot_services.expect_open_protocol().returning(|_, _, interface, _, _, _| {
      unsafe { *interface = ABS_PTR_INTERFACE };
      efi::Status::SUCCESS
    });
    boot_services.expect_uninstall_protocol_interface().returning(|_, _, _| efi::Status::SUCCESS);
    boot_services.expect_close_event().returning(|_| efi::Status::SUCCESS);

    // expected on PointerHidHandler::receive_report
    boot_services.expect_raise_tpl().returning(|new_tpl| {
      assert_eq!(new_tpl, efi::TPL_NOTIFY);
      efi::TPL_APPLICATION
    });

    boot_services.expect_restore_tpl().returning(|new_tpl| {
      assert_eq!(new_tpl, efi::TPL_APPLICATION);
      ()
    });

    let agent = 0x1 as efi::Handle;
    let mut pointer_handler = PointerHidHandler::new(boot_services, agent);
    let mut hid_io = MockHidIo::new();
    hid_io
      .expect_get_report_descriptor()
      .returning(|| Ok(hidparser::parse_report_descriptor(&MOUSE_REPORT_DESCRIPTOR).unwrap()));

    let controller = 0x2 as efi::Handle;
    assert_eq!(pointer_handler.initialize(controller, &hid_io), Ok(()));

    assert_eq!(pointer_handler.current_state.active_buttons, 0);
    assert_eq!(pointer_handler.current_state.current_x, CENTER);
    assert_eq!(pointer_handler.current_state.current_y, CENTER);
    assert_eq!(pointer_handler.current_state.current_z, 0);
    assert_eq!(pointer_handler.state_changed, false);

    //click two buttons and move the cursor (+32,+32)
    let report: &[u8] = &[0x05, 0x20, 0x20, 0];
    pointer_handler.receive_report(report, &hid_io);

    assert_eq!(pointer_handler.current_state.active_buttons, 0x05);
    assert_eq!(pointer_handler.current_state.current_x, CENTER + 32);
    assert_eq!(pointer_handler.current_state.current_y, CENTER + 32);
    assert_eq!(pointer_handler.current_state.current_z, 0);
    assert_eq!(pointer_handler.state_changed, true);

    //un-click and move the cursor (+32,-16) and wheel(+32).
    let report: &[u8] = &[0x00, 0x20, 0xF0, 0x20]; //0xF0 = -16.
    pointer_handler.receive_report(report, &hid_io);

    assert_eq!(pointer_handler.current_state.active_buttons, 0);
    assert_eq!(pointer_handler.current_state.current_x, CENTER + 64);
    assert_eq!(pointer_handler.current_state.current_y, CENTER + 16);
    assert_eq!(pointer_handler.current_state.current_z, 32);

    //un-click and move the cursor (+32,-32) and wheel(+32).
    let report: &[u8] = &[0x00, 0x20, 0xE0, 0x20]; //0xE0 = -32.
    pointer_handler.receive_report(report, &hid_io);

    assert_eq!(pointer_handler.current_state.active_buttons, 0);
    assert_eq!(pointer_handler.current_state.current_x, CENTER + 96);
    assert_eq!(pointer_handler.current_state.current_y, CENTER - 16);
    assert_eq!(pointer_handler.current_state.current_z, 64);

    //move the cursor (0,127) until is past saturation, and check the value each time
    let report: &[u8] = &[0x00, 0x00, 0x7F, 0x0]; //0x7F = +127.
    let starting_y = pointer_handler.current_state.current_y;
    for i in 0..AXIS_RESOLUTION {
      //starts near the center, so moving it well past saturation point
      pointer_handler.receive_report(report, &hid_io);
      let expected_y = min(AXIS_RESOLUTION, starting_y.saturating_add((i + 1) * 127));
      assert_eq!(pointer_handler.current_state.current_y, expected_y);
    }

    //move the cursor(0,-127) until it saturates at zero, and check the value each time.
    let report: &[u8] = &[0x00, 0x00, 0x81, 0x0]; //0x80 = -127.
    let starting_y = pointer_handler.current_state.current_y;
    for i in 0..AXIS_RESOLUTION {
      // starts at max, but moving 127 each time, so well past saturation point.
      pointer_handler.receive_report(report, &hid_io);
      let expected_y = starting_y.saturating_sub((i + 1) * 127);
      assert_eq!(pointer_handler.current_state.current_y, expected_y);
    }
  }

  #[test]
  fn receive_report_should_process_absolute_reports() {
    let boot_services = create_fake_static_boot_service();
    static mut ABS_PTR_INTERFACE: *mut c_void = core::ptr::null_mut();

    // expected on PointerHidHandler::initialize().
    boot_services.expect_create_event().returning(|_, _, _, _, _| efi::Status::SUCCESS);
    boot_services.expect_install_protocol_interface().returning(|_, _, _, interface| {
      unsafe { ABS_PTR_INTERFACE = interface };
      efi::Status::SUCCESS
    });

    // expected on PointerHidHandler::drop().
    boot_services.expect_open_protocol().returning(|_, _, interface, _, _, _| {
      unsafe { *interface = ABS_PTR_INTERFACE };
      efi::Status::SUCCESS
    });
    boot_services.expect_uninstall_protocol_interface().returning(|_, _, _| efi::Status::SUCCESS);
    boot_services.expect_close_event().returning(|_| efi::Status::SUCCESS);

    // expected on PointerHidHandler::receive_report
    boot_services.expect_raise_tpl().returning(|new_tpl| {
      assert_eq!(new_tpl, efi::TPL_NOTIFY);
      efi::TPL_APPLICATION
    });

    boot_services.expect_restore_tpl().returning(|new_tpl| {
      assert_eq!(new_tpl, efi::TPL_APPLICATION);
      ()
    });

    let agent = 0x1 as efi::Handle;
    let mut pointer_handler = PointerHidHandler::new(boot_services, agent);
    let mut hid_io = MockHidIo::new();
    hid_io
      .expect_get_report_descriptor()
      .returning(|| Ok(hidparser::parse_report_descriptor(&ABS_POINTER_REPORT_DESCRIPTOR).unwrap()));

    let controller = 0x2 as efi::Handle;
    assert_eq!(pointer_handler.initialize(controller, &hid_io), Ok(()));

    assert_eq!(pointer_handler.current_state.active_buttons, 0);
    assert_eq!(pointer_handler.current_state.current_x, CENTER);
    assert_eq!(pointer_handler.current_state.current_y, CENTER);
    assert_eq!(pointer_handler.current_state.current_z, 0);
    assert_eq!(pointer_handler.state_changed, false);

    //click two buttons and move the cursor (1024, 1024).
    let report: &[u8] = &[0x05, 0x00, 0x04, 0x00, 0x04, 0x00, 0x00];
    pointer_handler.receive_report(report, &hid_io);

    assert_eq!(pointer_handler.current_state.active_buttons, 0x05);
    // input x from range 0-4095 projected on to 0-1024 axis is (x/4095) * 1024. For x=1024, result is 256.
    assert_eq!(pointer_handler.current_state.current_x, 256);
    assert_eq!(pointer_handler.current_state.current_y, 256);
    assert_eq!(pointer_handler.current_state.current_z, 0);
    assert_eq!(pointer_handler.state_changed, true);
  }

  #[test]
  fn bad_reports_should_be_ignored() {
    let boot_services = create_fake_static_boot_service();
    static mut ABS_PTR_INTERFACE: *mut c_void = core::ptr::null_mut();

    // expected on PointerHidHandler::initialize().
    boot_services.expect_create_event().returning(|_, _, _, _, _| efi::Status::SUCCESS);
    boot_services.expect_install_protocol_interface().returning(|_, _, _, interface| {
      unsafe { ABS_PTR_INTERFACE = interface };
      efi::Status::SUCCESS
    });

    // expected on PointerHidHandler::drop().
    boot_services.expect_open_protocol().returning(|_, _, interface, _, _, _| {
      unsafe { *interface = ABS_PTR_INTERFACE };
      efi::Status::SUCCESS
    });
    boot_services.expect_uninstall_protocol_interface().returning(|_, _, _| efi::Status::SUCCESS);
    boot_services.expect_close_event().returning(|_| efi::Status::SUCCESS);

    // expected on PointerHidHandler::receive_report
    boot_services.expect_raise_tpl().returning(|new_tpl| {
      assert_eq!(new_tpl, efi::TPL_NOTIFY);
      efi::TPL_APPLICATION
    });

    boot_services.expect_restore_tpl().returning(|new_tpl| {
      assert_eq!(new_tpl, efi::TPL_APPLICATION);
      ()
    });

    let agent = 0x1 as efi::Handle;
    let mut pointer_handler = PointerHidHandler::new(boot_services, agent);
    let mut hid_io = MockHidIo::new();
    hid_io
      .expect_get_report_descriptor()
      .returning(|| Ok(hidparser::parse_report_descriptor(&ABS_POINTER_REPORT_DESCRIPTOR).unwrap()));

    let controller = 0x2 as efi::Handle;
    assert_eq!(pointer_handler.initialize(controller, &hid_io), Ok(()));

    assert_eq!(pointer_handler.current_state.active_buttons, 0);
    assert_eq!(pointer_handler.current_state.current_x, CENTER);
    assert_eq!(pointer_handler.current_state.current_y, CENTER);
    assert_eq!(pointer_handler.current_state.current_z, 0);
    assert_eq!(pointer_handler.state_changed, false);

    //move the cursor (4096, 4096, 0) - changed fields are out of range
    let report: &[u8] = &[0x00, 0x00, 0x10, 0x00, 0x10, 0x00, 0x00];
    pointer_handler.receive_report(report, &hid_io);

    assert_eq!(pointer_handler.current_state.active_buttons, 0);
    assert_eq!(pointer_handler.current_state.current_x, CENTER);
    assert_eq!(pointer_handler.current_state.current_y, CENTER);
    assert_eq!(pointer_handler.current_state.current_z, 0);
    assert_eq!(pointer_handler.state_changed, false);

    //report too long
    let report: &[u8] = &[0x00, 0x00, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10];
    pointer_handler.receive_report(report, &hid_io);

    assert_eq!(pointer_handler.current_state.active_buttons, 0);
    assert_eq!(pointer_handler.current_state.current_x, CENTER);
    assert_eq!(pointer_handler.current_state.current_y, CENTER);
    assert_eq!(pointer_handler.current_state.current_z, 0);
    assert_eq!(pointer_handler.state_changed, false);

    //report too short
    let report: &[u8] = &[0x00, 0x10, 0x00, 0x10, 0x00, 0x10];
    pointer_handler.receive_report(report, &hid_io);

    assert_eq!(pointer_handler.current_state.active_buttons, 0);
    assert_eq!(pointer_handler.current_state.current_x, CENTER);
    assert_eq!(pointer_handler.current_state.current_y, CENTER);
    assert_eq!(pointer_handler.current_state.current_z, 0);
    assert_eq!(pointer_handler.state_changed, false);
  }
}
