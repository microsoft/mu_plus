//! Absolute Pointer Protocol FFI Support.
//!
//! This module manages the Absolute Pointer Protocol FFI.
//!
//! ## License
//!
//! Copyright (c) Microsoft Corporation. All rights reserved.
//! SPDX-License-Identifier: BSD-2-Clause-Patent
//!
use alloc::boxed::Box;
use core::ffi::c_void;

use hidparser::report_data_types::Usage;
use r_efi::{efi, protocols};
use rust_advanced_logger_dxe::{debugln, DEBUG_ERROR, DEBUG_INFO, DEBUG_WARN};

use crate::boot_services::UefiBootServices;

use super::PointerHidHandler;

// FFI context
// Safety: a pointer to PointerHidHandler is included in the context so that it can be reclaimed in the absolute_pointer
// API implementation. Care must be taken to ensure that rust invariants are respected when accessing the
// PointerHidHandler. In particular, the design must ensure mutual exclusion on the PointerHidHandler between callbacks
// running at different TPL; this is accomplished by ensuring all access to the structure is at TPL_NOTIFY once
// initialization is complete - for this reason the context structure includes a direct reference to boot_services so
// that TPL can be enforced without access to the *mut PointerHidHandle.
//
// In addition, the absolute_pointer element needs to be the first element in the structure so that the full structure
// can be recovered by simple casting for absolute_pointer FFI interfaces that only receive a pointer to the
// absolute_pointer structure.
#[repr(C)]
pub struct PointerContext {
  absolute_pointer: protocols::absolute_pointer::Protocol,
  boot_services: &'static dyn UefiBootServices,
  pointer_handler: *mut PointerHidHandler,
}

impl Drop for PointerContext {
  fn drop(&mut self) {
    if !self.absolute_pointer.mode.is_null() {
      drop(unsafe { Box::from_raw(self.absolute_pointer.mode) });
    }
  }
}

impl PointerContext {
  /// Installs the absolute pointer protocol
  pub fn install(
    boot_services: &'static dyn UefiBootServices,
    controller: efi::Handle,
    pointer_handler: &mut PointerHidHandler,
  ) -> Result<(), efi::Status> {
    // Create pointer context.
    let pointer_ctx = PointerContext {
      absolute_pointer: protocols::absolute_pointer::Protocol {
        reset: Self::absolute_pointer_reset,
        get_state: Self::absolute_pointer_get_state,
        mode: Box::into_raw(Box::new(Self::initialize_mode(pointer_handler))),
        wait_for_input: core::ptr::null_mut(),
      },
      boot_services,
      pointer_handler: pointer_handler as *mut PointerHidHandler,
    };

    let absolute_pointer_ptr = Box::into_raw(Box::new(pointer_ctx));

    // create event for wait_for_input.
    let mut wait_for_pointer_input_event: efi::Event = core::ptr::null_mut();
    let status = boot_services.create_event(
      efi::EVT_NOTIFY_WAIT,
      efi::TPL_NOTIFY,
      Some(Self::wait_for_pointer),
      absolute_pointer_ptr as *mut c_void,
      core::ptr::addr_of_mut!(wait_for_pointer_input_event),
    );
    if status.is_error() {
      drop(unsafe { Box::from_raw(absolute_pointer_ptr) });
      return Err(status);
    }

    unsafe { (*absolute_pointer_ptr).absolute_pointer.wait_for_input = wait_for_pointer_input_event };

    // install the absolute_pointer protocol.
    let mut controller = controller;
    let status = boot_services.install_protocol_interface(
      core::ptr::addr_of_mut!(controller),
      &protocols::absolute_pointer::PROTOCOL_GUID as *const efi::Guid as *mut efi::Guid,
      efi::NATIVE_INTERFACE,
      absolute_pointer_ptr as *mut c_void,
    );

    if status.is_error() {
      let _ = boot_services.close_event(wait_for_pointer_input_event);
      drop(unsafe { Box::from_raw(absolute_pointer_ptr) });
      return Err(status);
    }

    Ok(())
  }

  // Initializes the absolute_pointer mode structure.
  fn initialize_mode(pointer_handler: &PointerHidHandler) -> protocols::absolute_pointer::Mode {
    let mut mode: protocols::absolute_pointer::Mode = Default::default();

    if pointer_handler.supported_usages.contains(&Usage::from(super::GENERIC_DESKTOP_X)) {
      mode.absolute_max_x = super::AXIS_RESOLUTION;
      mode.absolute_min_x = 0;
    } else {
      debugln!(DEBUG_WARN, "No x-axis usages found in the report descriptor.");
    }

    if pointer_handler.supported_usages.contains(&Usage::from(super::GENERIC_DESKTOP_Y)) {
      mode.absolute_max_y = super::AXIS_RESOLUTION;
      mode.absolute_min_y = 0;
    } else {
      debugln!(DEBUG_WARN, "No y-axis usages found in the report descriptor.");
    }

    if (pointer_handler.supported_usages.contains(&Usage::from(super::GENERIC_DESKTOP_Z)))
      || (pointer_handler.supported_usages.contains(&Usage::from(super::GENERIC_DESKTOP_WHEEL)))
    {
      mode.absolute_max_z = super::AXIS_RESOLUTION;
      mode.absolute_min_z = 0;
      //TODO: Z-axis is interpreted as pressure data. This is for compat with reference implementation in C, but
      //could consider e.g. looking for actual digitizer tip pressure usages or something.
      mode.attributes |= 0x02;
    } else {
      debugln!(DEBUG_INFO, "No z-axis usages found in the report descriptor.");
    }

    let button_count = pointer_handler.supported_usages.iter().filter(|x| x.page() == super::BUTTON_PAGE).count();

    if button_count > 1 {
      mode.attributes |= 0x01; // alternate button exists.
    }

    mode
  }

  /// Uninstalls the absolute pointer protocol
  pub fn uninstall(
    boot_services: &'static dyn UefiBootServices,
    agent: efi::Handle,
    controller: efi::Handle,
  ) -> Result<(), efi::Status> {
    let mut absolute_pointer_ptr: *mut PointerContext = core::ptr::null_mut();
    let status = boot_services.open_protocol(
      controller,
      &protocols::absolute_pointer::PROTOCOL_GUID as *const efi::Guid as *mut efi::Guid,
      core::ptr::addr_of_mut!(absolute_pointer_ptr) as *mut *mut c_void,
      agent,
      controller,
      efi::OPEN_PROTOCOL_GET_PROTOCOL,
    );
    if status.is_error() {
      //No protocol is actually installed on this controller, so nothing to clean up.
      return Ok(());
    }

    //Attempt to uninstall the absolute_pointer interface - this should disconnect any drivers using it and release
    //the interface.
    let status = boot_services.uninstall_protocol_interface(
      controller,
      &protocols::absolute_pointer::PROTOCOL_GUID as *const efi::Guid as *mut efi::Guid,
      absolute_pointer_ptr as *mut c_void,
    );
    if status.is_error() {
      //An error here means some other driver might be holding on to the absolute_pointer_ptr.
      //Mark the instance invalid by setting the pointer_handler raw pointer to null, but leak the PointerContext
      //instance. Leaking context allows calls through the pointers on absolute_pointer_ptr to continue to resolve
      //and return error based on observing pointer_handler is null.
      debugln!(DEBUG_ERROR, "Failed to uninstall absolute_pointer interface, status: {:x?}", status);

      unsafe {
        (*absolute_pointer_ptr).pointer_handler = core::ptr::null_mut();
      }
      //return without tearing down the context.
      return Err(status);
    }

    let wait_for_input_event: efi::Handle = unsafe { (*absolute_pointer_ptr).absolute_pointer.wait_for_input };
    let status = boot_services.close_event(wait_for_input_event);
    if status.is_error() {
      //An error here means the event was not uninstalled, so in theory the notification_callback on it could still be
      //fired.
      //Mark the instance invalid by setting the pointer_handler raw pointer to null, but leak the PointerContext
      //instance. Leaking context allows calls through the pointers on absolute_pointer_ptr to continue to resolve
      //and return error based on observing pointer_handler is null.
      debugln!(DEBUG_ERROR, "Failed to close absolute_pointer.wait_for_input event, status: {:x?}", status);
      unsafe {
        (*absolute_pointer_ptr).pointer_handler = core::ptr::null_mut();
      }
      return Err(status);
    }

    // None of the parts of absolute pointer are in use, so it is safe to reclaim it.
    drop(unsafe { Box::from_raw(absolute_pointer_ptr) });
    Ok(())
  }

  // event handler for wait_for_pointer event that is part of the absolute pointer interface.
  extern "efiapi" fn wait_for_pointer(event: efi::Event, context: *mut c_void) {
    let pointer_ctx = unsafe { (context as *mut PointerContext).as_mut().expect("bad context") };
    // raise to notify to protect access to pointer_handler, and check if event should be signalled.
    let old_tpl = pointer_ctx.boot_services.raise_tpl(efi::TPL_NOTIFY);
    {
      let pointer_handler = unsafe { pointer_ctx.pointer_handler.as_mut() };
      if let Some(pointer_handler) = pointer_handler {
        if pointer_handler.state_changed {
          pointer_ctx.boot_services.signal_event(event);
        }
      } else {
        // implies that this API was invoked after pointer handler was dropped.
        debugln!(DEBUG_ERROR, "absolute_pointer_reset invoked after pointer dropped.");
      }
    }
    pointer_ctx.boot_services.restore_tpl(old_tpl);
  }

  // resets the pointer state - part of the absolute pointer interface.
  extern "efiapi" fn absolute_pointer_reset(
    this: *mut protocols::absolute_pointer::Protocol,
    _extended_verification: bool,
  ) -> efi::Status {
    if this.is_null() {
      return efi::Status::INVALID_PARAMETER;
    }
    let pointer_ctx = unsafe { (this as *mut PointerContext).as_mut().expect("bad context") };
    let mut status = efi::Status::SUCCESS;
    {
      // raise to notify to protect access to pointer_handler and reset pointer handler state
      let old_tpl = pointer_ctx.boot_services.raise_tpl(efi::TPL_NOTIFY);

      let pointer_handler = unsafe { pointer_ctx.pointer_handler.as_mut() };
      if let Some(pointer_handler) = pointer_handler {
        pointer_handler.reset_state();
      } else {
        // implies that this API was invoked after pointer handler was dropped.
        debugln!(DEBUG_ERROR, "absolute_pointer_reset invoked after pointer dropped.");
        status = efi::Status::DEVICE_ERROR;
      }

      pointer_ctx.boot_services.restore_tpl(old_tpl);
    }
    status
  }

  // returns the current pointer state in the `state` buffer provided by the caller - part of the absolute pointer
  // interface.
  extern "efiapi" fn absolute_pointer_get_state(
    this: *mut protocols::absolute_pointer::Protocol,
    state: *mut protocols::absolute_pointer::State,
  ) -> efi::Status {
    if state.is_null() || state.is_null() {
      return efi::Status::INVALID_PARAMETER;
    }

    let pointer_ctx = unsafe { (this as *mut PointerContext).as_mut().expect("bad context") };
    let mut status = efi::Status::SUCCESS;
    {
      // raise to notify to protect access to pointer_handler, and retrieve pointer handler state.
      let old_tpl = pointer_ctx.boot_services.raise_tpl(efi::TPL_NOTIFY);

      let pointer_handler = unsafe { pointer_ctx.pointer_handler.as_mut() };
      if let Some(pointer_handler) = pointer_handler {
        if pointer_handler.state_changed {
          unsafe {
            state.write(pointer_handler.current_state);
          }
          pointer_handler.state_changed = false;
        } else {
          status = efi::Status::NOT_READY;
        }
      } else {
        // implies that this API was invoked after pointer handler was dropped.
        debugln!(DEBUG_ERROR, "absolute_pointer_get_state invoked after pointer dropped.");
        status = efi::Status::DEVICE_ERROR;
      }

      pointer_ctx.boot_services.restore_tpl(old_tpl);
    }
    status
  }
}

#[cfg(test)]
mod test {

  use core::ffi::c_void;

  use r_efi::{efi, protocols};

  use super::*;

  use crate::{
    boot_services::MockUefiBootServices,
    hid_io::{HidReportReceiver, MockHidIo},
    pointer::CENTER,
  };

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

  // In this module, the usage model for boot_services is global static, and so &'static dyn UefiBootServices is used
  // throughout the API. For testing, each test will have a different set of expectations on the UefiBootServices mock
  // object, and the mock object itself expects to be "mut", which makes it hard to handle as a single global static.
  // Instead, raw pointers are used to simulate a MockUefiBootServices instance with 'static lifetime.
  // This object needs to outlive anything that uses it - once created, it will live until the end of the program.
  fn create_fake_static_boot_service() -> &'static mut MockUefiBootServices {
    unsafe { Box::into_raw(Box::new(MockUefiBootServices::new())).as_mut().unwrap() }
  }

  #[test]
  fn wait_for_event_should_wait_for_event() {
    let boot_services = create_fake_static_boot_service();
    const AGENT_HANDLE: efi::Handle = 0x01 as efi::Handle;
    const CONTROLLER_HANDLE: efi::Handle = 0x02 as efi::Handle;
    const POINTER_EVENT: efi::Event = 0x03 as efi::Event;

    static mut ABS_PTR_INTERFACE: *mut c_void = core::ptr::null_mut();
    static mut EVENT_CONTEXT: *mut c_void = core::ptr::null_mut();
    static mut EVENT_SIGNALED: bool = false;

    // expected on PointerHidHandler::initialize().
    boot_services.expect_create_event().returning(|_, _, wait_for_ptr, context, event_ptr| {
      assert!(wait_for_ptr == Some(PointerContext::wait_for_pointer));
      assert_ne!(context, core::ptr::null_mut());
      unsafe {
        EVENT_CONTEXT = context;
        event_ptr.write(POINTER_EVENT);
      }
      efi::Status::SUCCESS
    });

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

    boot_services.expect_signal_event().returning(|event| {
      assert_eq!(event, POINTER_EVENT);
      unsafe { EVENT_SIGNALED = true };
      efi::Status::SUCCESS
    });

    let agent = AGENT_HANDLE;
    let mut pointer_handler = PointerHidHandler::new(boot_services, agent);
    let mut hid_io = MockHidIo::new();
    hid_io
      .expect_get_report_descriptor()
      .returning(|| Ok(hidparser::parse_report_descriptor(&MOUSE_REPORT_DESCRIPTOR).unwrap()));

    let controller = CONTROLLER_HANDLE;
    assert_eq!(pointer_handler.initialize(controller, &hid_io), Ok(()));

    let absolute_pointer = unsafe { (ABS_PTR_INTERFACE as *mut PointerContext).as_mut() }.unwrap();
    assert_eq!(absolute_pointer.absolute_pointer.wait_for_input, POINTER_EVENT);

    // no pointer state change - should not signal event.
    PointerContext::wait_for_pointer(POINTER_EVENT, unsafe { EVENT_CONTEXT });

    assert_eq!(unsafe { EVENT_SIGNALED }, false);

    //click two buttons and move the cursor (+32,+32)
    let report: &[u8] = &[0x05, 0x20, 0x20, 0];
    pointer_handler.receive_report(report, &hid_io);

    // pointer state change in place - should signal event.
    PointerContext::wait_for_pointer(POINTER_EVENT, unsafe { EVENT_CONTEXT });
    assert_eq!(unsafe { EVENT_SIGNALED }, true);
  }

  #[test]
  fn absolute_pointer_reset_should_reset_pointer_state() {
    let boot_services = create_fake_static_boot_service();
    const AGENT_HANDLE: efi::Handle = 0x01 as efi::Handle;
    const CONTROLLER_HANDLE: efi::Handle = 0x02 as efi::Handle;
    const EVENT_HANDLE: efi::Handle = 0x03 as efi::Handle;

    static mut ABS_PTR_INTERFACE: *mut c_void = core::ptr::null_mut();
    static mut EVENT_CONTEXT: *mut c_void = core::ptr::null_mut();

    // expected on PointerHidHandler::initialize().
    boot_services.expect_create_event().returning(|_, _, wait_for_ptr, context, event_ptr| {
      assert!(wait_for_ptr == Some(PointerContext::wait_for_pointer));
      assert_ne!(context, core::ptr::null_mut());
      unsafe {
        EVENT_CONTEXT = context;
        event_ptr.write(EVENT_HANDLE);
      }
      efi::Status::SUCCESS
    });

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

    let agent = AGENT_HANDLE;
    let mut pointer_handler = PointerHidHandler::new(boot_services, agent);
    let mut hid_io = MockHidIo::new();
    hid_io
      .expect_get_report_descriptor()
      .returning(|| Ok(hidparser::parse_report_descriptor(&MOUSE_REPORT_DESCRIPTOR).unwrap()));

    let controller = CONTROLLER_HANDLE;
    assert_eq!(pointer_handler.initialize(controller, &hid_io), Ok(()));

    assert_eq!(pointer_handler.current_state.active_buttons, 0);
    assert_eq!(pointer_handler.current_state.current_x, CENTER);
    assert_eq!(pointer_handler.current_state.current_y, CENTER);
    assert_eq!(pointer_handler.current_state.current_z, 0);
    assert_eq!(pointer_handler.state_changed, false);

    //click two buttons and move the cursor (+32,+32,+32)
    let report: &[u8] = &[0x05, 0x20, 0x20, 0x20];
    pointer_handler.receive_report(report, &hid_io);

    assert_eq!(pointer_handler.current_state.active_buttons, 0x5);
    assert_eq!(pointer_handler.current_state.current_x, CENTER + 0x20);
    assert_eq!(pointer_handler.current_state.current_y, CENTER + 0x20);
    assert_eq!(pointer_handler.current_state.current_z, 0x20);
    assert_eq!(pointer_handler.state_changed, true);

    //reset state
    let status = PointerContext::absolute_pointer_reset(
      unsafe { ABS_PTR_INTERFACE as *mut protocols::absolute_pointer::Protocol },
      false,
    );
    assert_eq!(status, efi::Status::SUCCESS);

    assert_eq!(pointer_handler.current_state.active_buttons, 0);
    assert_eq!(pointer_handler.current_state.current_x, CENTER);
    assert_eq!(pointer_handler.current_state.current_y, CENTER);
    assert_eq!(pointer_handler.current_state.current_z, 0);
    assert_eq!(pointer_handler.state_changed, false);
  }

  #[test]
  fn absolute_pointer_get_state_should_return_current_state_and_clear_changed_flag() {
    let boot_services = create_fake_static_boot_service();
    const AGENT_HANDLE: efi::Handle = 0x01 as efi::Handle;
    const CONTROLLER_HANDLE: efi::Handle = 0x02 as efi::Handle;
    const EVENT_HANDLE: efi::Handle = 0x03 as efi::Handle;

    static mut ABS_PTR_INTERFACE: *mut c_void = core::ptr::null_mut();
    static mut EVENT_CONTEXT: *mut c_void = core::ptr::null_mut();

    // expected on PointerHidHandler::initialize().
    boot_services.expect_create_event().returning(|_, _, wait_for_ptr, context, event_ptr| {
      assert!(wait_for_ptr == Some(PointerContext::wait_for_pointer));
      assert_ne!(context, core::ptr::null_mut());
      unsafe {
        EVENT_CONTEXT = context;
        event_ptr.write(EVENT_HANDLE);
      }
      efi::Status::SUCCESS
    });

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

    let agent = AGENT_HANDLE;
    let mut pointer_handler = PointerHidHandler::new(boot_services, agent);
    let mut hid_io = MockHidIo::new();
    hid_io
      .expect_get_report_descriptor()
      .returning(|| Ok(hidparser::parse_report_descriptor(&MOUSE_REPORT_DESCRIPTOR).unwrap()));

    let controller = CONTROLLER_HANDLE;
    assert_eq!(pointer_handler.initialize(controller, &hid_io), Ok(()));

    assert_eq!(pointer_handler.current_state.active_buttons, 0);
    assert_eq!(pointer_handler.current_state.current_x, CENTER);
    assert_eq!(pointer_handler.current_state.current_y, CENTER);
    assert_eq!(pointer_handler.current_state.current_z, 0);
    assert_eq!(pointer_handler.state_changed, false);

    //click two buttons and move the cursor (+32,+32,+32)
    let report: &[u8] = &[0x05, 0x20, 0x20, 0x20];
    pointer_handler.receive_report(report, &hid_io);

    assert_eq!(pointer_handler.current_state.active_buttons, 0x5);
    assert_eq!(pointer_handler.current_state.current_x, CENTER + 0x20);
    assert_eq!(pointer_handler.current_state.current_y, CENTER + 0x20);
    assert_eq!(pointer_handler.current_state.current_z, 0x20);
    assert_eq!(pointer_handler.state_changed, true);

    let mut absolute_pointer_state: protocols::absolute_pointer::State = Default::default();
    let status = PointerContext::absolute_pointer_get_state(
      unsafe { ABS_PTR_INTERFACE as *mut protocols::absolute_pointer::Protocol },
      &mut absolute_pointer_state as *mut protocols::absolute_pointer::State,
    );
    assert_eq!(status, efi::Status::SUCCESS);

    assert_eq!(absolute_pointer_state.current_x, pointer_handler.current_state.current_x);
    assert_eq!(absolute_pointer_state.current_y, pointer_handler.current_state.current_y);
    assert_eq!(absolute_pointer_state.current_z, pointer_handler.current_state.current_z);
    assert_eq!(absolute_pointer_state.active_buttons, pointer_handler.current_state.active_buttons);
    assert_eq!(pointer_handler.state_changed, false);

    //if get_state is attempted when there are no changes to state, it should return NOT_READY.
    let mut absolute_pointer_state: protocols::absolute_pointer::State = Default::default();
    let status = PointerContext::absolute_pointer_get_state(
      unsafe { ABS_PTR_INTERFACE as *mut protocols::absolute_pointer::Protocol },
      &mut absolute_pointer_state as *mut protocols::absolute_pointer::State,
    );
    assert_eq!(status, efi::Status::NOT_READY);
  }
}
