//! Simple Text In Ex Protocol FFI Support.
//!
//! This module manages the Simple Text In Ex FFI.
//!
//! ## License
//!
//! Copyright (c) Microsoft Corporation. All rights reserved.
//! SPDX-License-Identifier: BSD-2-Clause-Patent
//!
use alloc::{boxed::Box, vec::Vec};
use core::ffi::c_void;

use r_efi::{efi, protocols};
use rust_advanced_logger_dxe::{debugln, DEBUG_ERROR};

use crate::{
  boot_services::UefiBootServices,
  hid_io::{HidIoFactory, UefiHidIoFactory},
  keyboard::KeyboardHidHandler,
};

/// FFI context
/// # Safety
/// A pointer to KeyboardHidHandler is included in the contexts so that it can be reclaimed in the simple_text_in API
/// implementations. Care must be taken to ensure that rust invariants are respected when accessing the
/// KeyboardHidHandler. In particular, the design must ensure mutual exclusion on the KeyboardHidHandler between
/// callbacks running at different TPL; this is accomplished by ensuring all access to the structure is at TPL_NOTIFY
/// once initialization is complete - for this reason the context structure includes a direct reference to boot_services
/// so that TPL can be enforced without access to the *mut KeyboardHidHandler.
///
/// In addition, the simple_text_in_ex protocol element needs to be the first element in the structure so that the full
/// structure can be recovered by simple casting for simple_text_in FFI interfaces that only receive a pointer to the
/// simple_text_in_ex protocol structure.
#[repr(C)]
pub struct SimpleTextInExFfi {
  simple_text_in_ex: protocols::simple_text_input_ex::Protocol,
  boot_services: &'static dyn UefiBootServices,
  key_notify_event: efi::Event,
  keyboard_handler: *mut KeyboardHidHandler,
}

impl SimpleTextInExFfi {
  /// Installs the simple text in ex protocol
  pub fn install(
    boot_services: &'static dyn UefiBootServices,
    controller: efi::Handle,
    keyboard_handler: &mut KeyboardHidHandler,
  ) -> Result<(), efi::Status> {
    //Create simple_text_in context
    let simple_text_in_ex_ctx = SimpleTextInExFfi {
      simple_text_in_ex: protocols::simple_text_input_ex::Protocol {
        reset: Self::simple_text_in_ex_reset,
        read_key_stroke_ex: Self::simple_text_in_ex_read_key_stroke,
        wait_for_key_ex: core::ptr::null_mut(),
        set_state: Self::simple_text_in_ex_set_state,
        register_key_notify: Self::simple_text_in_ex_register_key_notify,
        unregister_key_notify: Self::simple_text_in_ex_unregister_key_notify,
      },
      boot_services,
      key_notify_event: core::ptr::null_mut(),
      keyboard_handler: keyboard_handler as *mut KeyboardHidHandler,
    };

    let simple_text_in_ex_ptr = Box::into_raw(Box::new(simple_text_in_ex_ctx));

    //create event for wait_for_key
    let mut wait_for_key_event: efi::Event = core::ptr::null_mut();
    let status = boot_services.create_event(
      efi::EVT_NOTIFY_WAIT,
      efi::TPL_NOTIFY,
      Some(Self::simple_text_in_ex_wait_for_key),
      simple_text_in_ex_ptr as *mut c_void,
      core::ptr::addr_of_mut!(wait_for_key_event),
    );
    if status.is_error() {
      drop(unsafe { Box::from_raw(simple_text_in_ex_ptr) });
      return Err(status);
    }

    unsafe { (*simple_text_in_ex_ptr).simple_text_in_ex.wait_for_key_ex = wait_for_key_event };

    //Key notifies are required to dispatch at TPL_CALLBACK per UEFI spec 2.10 section 12.2.5. The keyboard handler
    //interfaces run at TPL_NOTIFY and issue a boot_services.signal_event() on this event to pend key notifies to be
    //serviced at TPL_CALLBACK.
    let mut key_notify_event: efi::Event = core::ptr::null_mut();
    let status = boot_services.create_event(
      efi::EVT_NOTIFY_SIGNAL,
      efi::TPL_CALLBACK,
      Some(Self::process_key_notifies),
      simple_text_in_ex_ptr as *mut c_void,
      core::ptr::addr_of_mut!(key_notify_event),
    );
    if status.is_error() {
      let _ = boot_services.close_event(wait_for_key_event);
      drop(unsafe { Box::from_raw(simple_text_in_ex_ptr) });
    }

    unsafe { (*simple_text_in_ex_ptr).key_notify_event = key_notify_event };

    //install the simple_text_in_ex protocol
    let mut controller = controller;
    let status = boot_services.install_protocol_interface(
      core::ptr::addr_of_mut!(controller),
      &protocols::simple_text_input_ex::PROTOCOL_GUID as *const efi::Guid as *mut efi::Guid,
      efi::NATIVE_INTERFACE,
      simple_text_in_ex_ptr as *mut c_void,
    );

    if status.is_error() {
      let _ = boot_services.close_event(wait_for_key_event);
      let _ = boot_services.close_event(key_notify_event);
      drop(unsafe { Box::from_raw(simple_text_in_ex_ptr) });
      return Err(status);
    }

    Ok(())
  }

  /// Uninstalls the simple text in ex protocol
  pub fn uninstall(
    boot_services: &'static dyn UefiBootServices,
    agent: efi::Handle,
    controller: efi::Handle,
  ) -> Result<(), efi::Status> {
    //Controller is set - that means initialize() was called, and there is potential state exposed thru FFI that needs
    //to be cleaned up.
    let mut simple_text_in_ex_ptr: *mut SimpleTextInExFfi = core::ptr::null_mut();
    let status = boot_services.open_protocol(
      controller,
      &protocols::simple_text_input_ex::PROTOCOL_GUID as *const efi::Guid as *mut efi::Guid,
      core::ptr::addr_of_mut!(simple_text_in_ex_ptr) as *mut *mut c_void,
      agent,
      controller,
      efi::OPEN_PROTOCOL_GET_PROTOCOL,
    );
    if status.is_error() {
      //No protocol is actually installed on this controller, so nothing to clean up.
      return Ok(());
    }

    //Attempt to uninstall the simple_text_in interface - this should disconnect any drivers using it and release
    //the interface.
    let status = boot_services.uninstall_protocol_interface(
      controller,
      &protocols::simple_text_input_ex::PROTOCOL_GUID as *const efi::Guid as *mut efi::Guid,
      simple_text_in_ex_ptr as *mut c_void,
    );
    if status.is_error() {
      //An error here means some other driver might be holding on to the simple_text_in_ptr.
      //Mark the instance invalid by setting the keyboard_handler raw pointer to null, but leak the PointerContext
      //instance. Leaking context allows calls through the pointers on absolute_pointer_ptr to continue to resolve
      //and return error based on observing keyboard_handler is null.
      debugln!(DEBUG_ERROR, "Failed to uninstall simple_text_in interface, status: {:x?}", status);

      unsafe {
        (*simple_text_in_ex_ptr).keyboard_handler = core::ptr::null_mut();
      }
      //return without tearing down the context.
      return Err(status);
    }

    let wait_for_key_event: efi::Handle = unsafe { (*simple_text_in_ex_ptr).simple_text_in_ex.wait_for_key_ex };
    let status = boot_services.close_event(wait_for_key_event);
    if status.is_error() {
      //An error here means the event was not closed, so in theory the notification_callback on it could still be
      //fired.
      //Mark the instance invalid by setting the keyboard_handler raw pointer to null, but leak the PointerContext
      //instance. Leaking context allows calls through the pointers on simple_text_in_ptr to continue to resolve
      //and return error based on observing keyboard_handler is null.
      debugln!(DEBUG_ERROR, "Failed to close simple_text_in_ptr.wait_for_key event, status: {:x?}", status);
      unsafe {
        (*simple_text_in_ex_ptr).keyboard_handler = core::ptr::null_mut();
      }
      return Err(status);
    }

    let key_notify_event: efi::Handle = unsafe { (*simple_text_in_ex_ptr).key_notify_event };
    let status = boot_services.close_event(key_notify_event);
    if status.is_error() {
      //An error here means the event was not closed, so in theory the notification_callback on it could still be
      //fired.
      //Mark the instance invalid by setting the keyboard_handler raw pointer to null, but leak the PointerContext
      //instance. Leaking context allows calls through the pointers on simple_text_in_ptr to continue to resolve
      //and return error based on observing keyboard_handler is null.
      debugln!(DEBUG_ERROR, "Failed to close key_notify_event event, status: {:x?}", status);
      unsafe {
        (*simple_text_in_ex_ptr).keyboard_handler = core::ptr::null_mut();
      }
      return Err(status);
    }

    // None of the parts of simple_text_in_ptr simple_text_in_ptr are in use, so it is safe to reclaim it.
    drop(unsafe { Box::from_raw(simple_text_in_ex_ptr) });
    Ok(())
  }

  // resets the keyboard state - part of the simple_text_in_ex protocol interface.
  extern "efiapi" fn simple_text_in_ex_reset(
    this: *mut protocols::simple_text_input_ex::Protocol,
    extended_verification: efi::Boolean,
  ) -> efi::Status {
    if this.is_null() {
      return efi::Status::INVALID_PARAMETER;
    }
    let context = unsafe { (this as *mut SimpleTextInExFfi).as_mut() }.expect("bad pointer");
    let old_tpl = context.boot_services.raise_tpl(efi::TPL_NOTIFY);
    let mut status = efi::Status::DEVICE_ERROR;
    '_reset_processing: {
      let keyboard_handler = unsafe { context.keyboard_handler.as_mut() };
      if let Some(keyboard_handler) = keyboard_handler {
        //reset requires an instance of hid_io to allow for LED updating, so use UefiHidIoFactory to build one.
        if let Some(controller) = keyboard_handler.get_controller() {
          let hid_io =
            UefiHidIoFactory::new(context.boot_services, keyboard_handler.get_agent()).new_hid_io(controller, false);
          if let Ok(hid_io) = hid_io {
            if let Err(err) = keyboard_handler.reset(hid_io.as_ref(), extended_verification.into()) {
              status = err;
            } else {
              status = efi::Status::SUCCESS;
            }
          }
        }
      }
    }
    context.boot_services.restore_tpl(old_tpl);
    status
  }

  // reads a key stroke - part of the simple_text_in_ex protocol interface.
  extern "efiapi" fn simple_text_in_ex_read_key_stroke(
    this: *mut protocols::simple_text_input_ex::Protocol,
    key_data: *mut protocols::simple_text_input_ex::KeyData,
  ) -> efi::Status {
    if this.is_null() || key_data.is_null() {
      return efi::Status::INVALID_PARAMETER;
    }
    let context = unsafe { (this as *mut SimpleTextInExFfi).as_mut() }.expect("bad pointer");
    let mut status = efi::Status::SUCCESS;
    let old_tpl = context.boot_services.raise_tpl(efi::TPL_NOTIFY);
    'read_key_stroke: {
      let keyboard_handler = unsafe { context.keyboard_handler.as_mut() };
      if let Some(keyboard_handler) = keyboard_handler {
        if let Some(key) = keyboard_handler.pop_key() {
          unsafe { key_data.write(key) }
        } else {
          let key = protocols::simple_text_input_ex::KeyData {
            key_state: keyboard_handler.get_key_state(),
            ..Default::default()
          };
          unsafe { key_data.write(key) };
          status = efi::Status::NOT_READY
        }
      } else {
        status = efi::Status::DEVICE_ERROR;
        break 'read_key_stroke;
      }
    }
    context.boot_services.restore_tpl(old_tpl);
    status
  }

  // sets the keyboard state - part of the simple_text_in_ex protocol interface.
  extern "efiapi" fn simple_text_in_ex_set_state(
    this: *mut protocols::simple_text_input_ex::Protocol,
    key_toggle_state: *mut protocols::simple_text_input_ex::KeyToggleState,
  ) -> efi::Status {
    if this.is_null() || key_toggle_state.is_null() {
      return efi::Status::INVALID_PARAMETER;
    }
    let context = unsafe { (this as *mut SimpleTextInExFfi).as_mut() }.expect("bad pointer");
    let mut status = efi::Status::DEVICE_ERROR;
    let old_tpl = context.boot_services.raise_tpl(efi::TPL_NOTIFY);
    '_set_state_processing: {
      if let Some(keyboard_handler) = unsafe { context.keyboard_handler.as_mut() } {
        if let Some(controller) = keyboard_handler.get_controller() {
          let hid_io =
            UefiHidIoFactory::new(context.boot_services, keyboard_handler.get_agent()).new_hid_io(controller, false);
          if let Ok(hid_io) = hid_io {
            status = efi::Status::SUCCESS;
            keyboard_handler.set_key_toggle_state(unsafe { key_toggle_state.read() });
            let result = keyboard_handler.update_leds(hid_io.as_ref());
            if let Err(result) = result {
              status = result;
            }
          }
        }
      }
    }
    context.boot_services.restore_tpl(old_tpl);
    status
  }

  // registers a key notification callback function - part of the simple_text_in_ex protocol interface.
  extern "efiapi" fn simple_text_in_ex_register_key_notify(
    this: *mut protocols::simple_text_input_ex::Protocol,
    key_data_ptr: *mut protocols::simple_text_input_ex::KeyData,
    key_notification_function: protocols::simple_text_input_ex::KeyNotifyFunction,
    notify_handle: *mut *mut c_void,
  ) -> efi::Status {
    if this.is_null() || key_data_ptr.is_null() || notify_handle.is_null() || key_notification_function as usize == 0 {
      return efi::Status::INVALID_PARAMETER;
    }

    let context = unsafe { (this as *mut SimpleTextInExFfi).as_mut() }.expect("bad pointer");
    let old_tpl = context.boot_services.raise_tpl(efi::TPL_NOTIFY);
    let status;
    {
      if let Some(keyboard_handler) = unsafe { context.keyboard_handler.as_mut() } {
        let key_data = unsafe { key_data_ptr.read() };
        let handle = keyboard_handler.insert_key_notify_callback(key_data, key_notification_function);
        unsafe { notify_handle.write(handle as *mut c_void) };
        status = efi::Status::SUCCESS;
      } else {
        status = efi::Status::DEVICE_ERROR;
      }
    }
    context.boot_services.restore_tpl(old_tpl);
    status
  }

  // Unregisters a key notification callback function - part of the simple_text_in_ex protocol interface.
  extern "efiapi" fn simple_text_in_ex_unregister_key_notify(
    this: *mut protocols::simple_text_input_ex::Protocol,
    notification_handle: *mut c_void,
  ) -> efi::Status {
    if this.is_null() {
      return efi::Status::INVALID_PARAMETER;
    }
    let status;
    let context = unsafe { (this as *mut SimpleTextInExFfi).as_mut() }.expect("bad pointer");
    let old_tpl = context.boot_services.raise_tpl(efi::TPL_NOTIFY);
    if let Some(keyboard_handler) = unsafe { context.keyboard_handler.as_mut() } {
      if let Err(err) = keyboard_handler.remove_key_notify_callback(notification_handle as usize) {
        status = err;
      } else {
        status = efi::Status::SUCCESS;
      }
    } else {
      status = efi::Status::DEVICE_ERROR;
    }
    context.boot_services.restore_tpl(old_tpl);
    status
  }

  // Event handler function for the wait_for_key_event
  extern "efiapi" fn simple_text_in_ex_wait_for_key(event: efi::Event, context: *mut c_void) {
    if context.is_null() {
      debugln!(DEBUG_ERROR, "simple_text_in_ex_wait_for_key invoked with invalid context");
      return;
    }
    let context = unsafe { (context as *mut SimpleTextInExFfi).as_mut() }.expect("bad pointer");

    let old_tpl = context.boot_services.raise_tpl(efi::TPL_NOTIFY);
    {
      if let Some(keyboard_handler) = unsafe { context.keyboard_handler.as_mut() } {
        while let Some(key_data) = keyboard_handler.peek_key() {
          if key_data.key.unicode_char == 0 && key_data.key.scan_code == 0 {
            // consume (and ignore) the partial stroke.
            let _ = keyboard_handler.pop_key();
            continue;
          } else {
            // valid keystroke
            context.boot_services.signal_event(event);
            break;
          }
        }
      }
    }
    context.boot_services.restore_tpl(old_tpl);
  }

  // Event callback function for handling registered key notifications. Iterates over the queue of keys to be notified,
  // and invokes the registered callback function for each of those keys.
  extern "efiapi" fn process_key_notifies(_event: efi::Event, context: *mut c_void) {
    if let Some(context) = unsafe { (context as *mut SimpleTextInExFfi).as_mut() } {
      loop {
        let mut pending_key = None;
        let mut pending_callbacks = Vec::new();

        let old_tpl = context.boot_services.raise_tpl(efi::TPL_NOTIFY);
        if let Some(keyboard_handler) = unsafe { context.keyboard_handler.as_mut() } {
          (pending_key, pending_callbacks) = keyboard_handler.get_pending_callbacks();
        } else {
          debugln!(DEBUG_ERROR, "process_key_notifies event called without a valid keyboard_handler");
        }
        context.boot_services.restore_tpl(old_tpl);

        //dispatch notifies (if any) at the TPL this event callback was invoked at.
        if let Some(mut pending_key) = pending_key {
          let key_ptr = &mut pending_key as *mut protocols::simple_text_input_ex::KeyData;
          for callback in pending_callbacks {
            let _ = callback(key_ptr);
          }
        } else {
          // no pending notifies to process
          break;
        }
      }
    }
  }
}

#[cfg(test)]
mod test {
  use core::{
    ffi::c_void,
    mem::MaybeUninit,
    sync::atomic::{AtomicBool, AtomicPtr},
  };

  use r_efi::{efi, protocols};

  use crate::{
    boot_services::MockUefiBootServices,
    hid_io::{HidReportReceiver, MockHidIo},
    keyboard::KeyboardHidHandler,
  };

  use super::SimpleTextInExFfi;

  static BOOT_KEYBOARD_REPORT_DESCRIPTOR: &[u8] = &[
    0x05, 0x01, // USAGE_PAGE (Generic Desktop)
    0x09, 0x06, // USAGE (Keyboard)
    0xa1, 0x01, // COLLECTION (Application)
    0x75, 0x01, //    REPORT_SIZE (1)
    0x95, 0x08, //    REPORT_COUNT (8)
    0x05, 0x07, //    USAGE_PAGE (Key Codes)
    0x19, 0xE0, //    USAGE_MINIMUM (224)
    0x29, 0xE7, //    USAGE_MAXIMUM (231)
    0x15, 0x00, //    LOGICAL_MAXIMUM (0)
    0x25, 0x01, //    LOGICAL_MINIMUM (1)
    0x81, 0x02, //    INPUT (Data, Var, Abs) (Modifier Byte)
    0x95, 0x01, //    REPORT_COUNT (1)
    0x75, 0x08, //    REPORT_SIZE (8)
    0x81, 0x03, //    INPUT (Const) (Reserved Byte)
    0x95, 0x05, //    REPORT_COUNT (5)
    0x75, 0x01, //    REPORT_SIZE (1)
    0x05, 0x08, //    USAGE_PAGE (LEDs)
    0x19, 0x01, //    USAGE_MINIMUM (1)
    0x29, 0x05, //    USAGE_MAXIMUM (5)
    0x91, 0x02, //    OUTPUT (Data, Var, Abs) (LED report)
    0x95, 0x01, //    REPORT_COUNT (1)
    0x75, 0x03, //    REPORT_SIZE (3)
    0x91, 0x02, //    OUTPUT (Constant) (LED report padding)
    0x95, 0x06, //    REPORT_COUNT (6)
    0x75, 0x08, //    REPORT_SIZE (8)
    0x15, 0x00, //    LOGICAL_MINIMUM (0)
    0x26, 0xff, 00, //    LOGICAL_MAXIMUM (255)
    0x05, 0x07, //    USAGE_PAGE (Key Codes)
    0x19, 0x00, //    USAGE_MINIMUM (0)
    0x2a, 0xff, 00, //    USAGE_MAXIMUM (255)
    0x81, 0x00, //    INPUT (Data, Array)
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
  fn install_should_install_simple_text_in_ex_interface() {
    let boot_services = create_fake_static_boot_service();
    boot_services.expect_create_event().returning(|_, _, _, _, _| efi::Status::SUCCESS);
    boot_services.expect_install_protocol_interface().returning(|_, protocol, _, _| {
      assert_eq!(unsafe { protocol.read() }, protocols::simple_text_input_ex::PROTOCOL_GUID);
      efi::Status::SUCCESS
    });
    let mut keyboard_handler = KeyboardHidHandler::new(boot_services, 1 as efi::Handle);

    SimpleTextInExFfi::install(boot_services, 2 as efi::Handle, &mut keyboard_handler).unwrap();
  }

  #[test]
  fn uninstall_should_uninstall_simple_text_in_interface() {
    static CONTEXT_PTR: AtomicPtr<c_void> = AtomicPtr::new(core::ptr::null_mut());
    let boot_services = create_fake_static_boot_service();

    // used in install
    boot_services.expect_create_event().returning(|_, _, _, _, _| efi::Status::SUCCESS);
    boot_services.expect_install_protocol_interface().returning(|_, _, _, interface| {
      CONTEXT_PTR.store(interface, core::sync::atomic::Ordering::SeqCst);
      efi::Status::SUCCESS
    });

    // used in uninstall
    boot_services.expect_open_protocol().returning(|_, protocol, interface, _, _, _| {
      unsafe {
        assert_eq!(protocol.read(), protocols::simple_text_input_ex::PROTOCOL_GUID);
        interface.write(CONTEXT_PTR.load(core::sync::atomic::Ordering::SeqCst));
      }
      efi::Status::SUCCESS
    });
    boot_services.expect_uninstall_protocol_interface().returning(|_, protocol, interface| {
      unsafe {
        assert_eq!(protocol.read(), protocols::simple_text_input_ex::PROTOCOL_GUID);
        assert_eq!(interface, CONTEXT_PTR.load(core::sync::atomic::Ordering::SeqCst));
      }
      efi::Status::SUCCESS
    });
    boot_services.expect_close_event().returning(|_| efi::Status::SUCCESS);

    let mut keyboard_handler = KeyboardHidHandler::new(boot_services, 1 as efi::Handle);

    SimpleTextInExFfi::install(boot_services, 2 as efi::Handle, &mut keyboard_handler).unwrap();
    assert_ne!(CONTEXT_PTR.load(core::sync::atomic::Ordering::SeqCst), core::ptr::null_mut());

    SimpleTextInExFfi::uninstall(boot_services, 1 as efi::Handle, 2 as efi::Handle).unwrap();
  }

  #[test]
  fn reset_should_invoke_reset() {
    static CONTEXT_PTR: AtomicPtr<c_void> = AtomicPtr::new(core::ptr::null_mut());
    let boot_services = create_fake_static_boot_service();

    // used in install
    boot_services.expect_create_event().returning(|_, _, _, _, _| efi::Status::SUCCESS);
    boot_services.expect_install_protocol_interface().returning(|_, _, _, interface| {
      CONTEXT_PTR.store(interface, core::sync::atomic::Ordering::SeqCst);
      efi::Status::SUCCESS
    });

    // used in reset
    boot_services.expect_raise_tpl().returning(|_| efi::TPL_APPLICATION);
    boot_services.expect_restore_tpl().returning(|_| ());

    extern "efiapi" fn mock_set_report(
      _this: *const hid_io::protocol::Protocol,
      _report_id: u8,
      _report_type: hid_io::protocol::HidReportType,
      _report_buffer_size: usize,
      _report_buffer: *mut c_void,
    ) -> efi::Status {
      efi::Status::SUCCESS
    }

    // used in reset and uninstall
    boot_services.expect_open_protocol().returning(|_, protocol, interface, _, _, attributes| {
      unsafe {
        match protocol.read() {
          hid_io::protocol::GUID => {
            assert_eq!(attributes, efi::OPEN_PROTOCOL_GET_PROTOCOL);
            let hid_io = MaybeUninit::<hid_io::protocol::Protocol>::zeroed();
            let mut hid_io = hid_io.assume_init();
            hid_io.set_report = mock_set_report;
            // note: this will leak a hid_io instance
            interface.write(Box::into_raw(Box::new(hid_io)) as *mut c_void);
          }
          unrecognized_guid => panic!("Unexpected protocol: {:?}", unrecognized_guid),
        }
      }
      efi::Status::SUCCESS
    });

    // used in uninstall.
    boot_services.expect_uninstall_protocol_interface().returning(|_, protocol, interface| {
      unsafe {
        assert_eq!(protocol.read(), protocols::simple_text_input_ex::PROTOCOL_GUID);
        assert_eq!(interface, CONTEXT_PTR.load(core::sync::atomic::Ordering::SeqCst));
      }
      efi::Status::SUCCESS
    });
    boot_services.expect_close_event().returning(|_| efi::Status::SUCCESS);

    let mut keyboard_handler = KeyboardHidHandler::new(boot_services, 1 as efi::Handle);
    keyboard_handler.set_controller(Some(2 as efi::Handle));

    SimpleTextInExFfi::install(boot_services, 2 as efi::Handle, &mut keyboard_handler).unwrap();
    assert_ne!(CONTEXT_PTR.load(core::sync::atomic::Ordering::SeqCst), core::ptr::null_mut());

    let this = CONTEXT_PTR.load(core::sync::atomic::Ordering::SeqCst) as *mut protocols::simple_text_input_ex::Protocol;
    SimpleTextInExFfi::simple_text_in_ex_reset(this, efi::Boolean::FALSE);

    // avoid keyboard drop uninstall flows
    keyboard_handler.set_controller(None);
  }

  #[test]
  fn read_key_stroke_should_read_keystrokes() {
    static CONTEXT_PTR: AtomicPtr<c_void> = AtomicPtr::new(core::ptr::null_mut());
    let boot_services = create_fake_static_boot_service();

    // used in install
    boot_services.expect_create_event().returning(|_, _, _, _, _| efi::Status::SUCCESS);
    boot_services.expect_install_protocol_interface().returning(|_, _, _, interface| {
      CONTEXT_PTR.store(interface, core::sync::atomic::Ordering::SeqCst);
      efi::Status::SUCCESS
    });

    // used in read key stroke
    boot_services.expect_raise_tpl().returning(|_| efi::TPL_APPLICATION);
    boot_services.expect_restore_tpl().returning(|_| ());

    // used in keyboard init and uninstall
    boot_services.expect_create_event_ex().returning(|_, _, _, _, _, _| efi::Status::SUCCESS);
    boot_services.expect_signal_event().returning(|_| efi::Status::SUCCESS);
    boot_services.expect_open_protocol().returning(|_, _, _, _, _, _| efi::Status::NOT_FOUND);

    let mut keyboard_handler = KeyboardHidHandler::new(boot_services, 1 as efi::Handle);

    let mut hid_io = MockHidIo::new();
    hid_io
      .expect_get_report_descriptor()
      .returning(|| Ok(hidparser::parse_report_descriptor(&BOOT_KEYBOARD_REPORT_DESCRIPTOR).unwrap()));

    keyboard_handler.set_layout(Some(hii_keyboard_layout::get_default_keyboard_layout()));
    keyboard_handler.initialize(2 as efi::Handle, &hid_io).unwrap();

    SimpleTextInExFfi::install(boot_services, 2 as efi::Handle, &mut keyboard_handler).unwrap();
    assert_ne!(CONTEXT_PTR.load(core::sync::atomic::Ordering::SeqCst), core::ptr::null_mut());

    let report: &[u8] = &[0x00, 0x00, 0x04, 0x05, 0x06, 0x00, 0x00, 0x00];
    keyboard_handler.receive_report(report, &hid_io);
    //release keys and push ctrl
    let report: &[u8] = &[0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00];
    keyboard_handler.receive_report(report, &hid_io);

    //read with simple_text_in
    let this = CONTEXT_PTR.load(core::sync::atomic::Ordering::SeqCst) as *mut protocols::simple_text_input_ex::Protocol;
    let mut input_key: protocols::simple_text_input_ex::KeyData = Default::default();

    let status = SimpleTextInExFfi::simple_text_in_ex_read_key_stroke(
      this,
      &mut input_key as *mut protocols::simple_text_input_ex::KeyData,
    );
    assert_eq!(status, efi::Status::SUCCESS);
    assert_eq!(input_key.key.unicode_char, 'c' as u16);
    assert_eq!(input_key.key.scan_code, 0);

    let status = SimpleTextInExFfi::simple_text_in_ex_read_key_stroke(
      this,
      &mut input_key as *mut protocols::simple_text_input_ex::KeyData,
    );
    assert_eq!(status, efi::Status::SUCCESS);
    assert_eq!(input_key.key.unicode_char, 'b' as u16);
    assert_eq!(input_key.key.scan_code, 0);

    let status = SimpleTextInExFfi::simple_text_in_ex_read_key_stroke(
      this,
      &mut input_key as *mut protocols::simple_text_input_ex::KeyData,
    );
    assert_eq!(status, efi::Status::SUCCESS);
    assert_eq!(input_key.key.unicode_char, 'a' as u16);
    assert_eq!(input_key.key.scan_code, 0);

    let status = SimpleTextInExFfi::simple_text_in_ex_read_key_stroke(
      this,
      &mut input_key as *mut protocols::simple_text_input_ex::KeyData,
    );
    assert_eq!(status, efi::Status::NOT_READY);

    //send ctrl-a - expect it not to be switched to control-character 0x01
    let report: &[u8] = &[0x01, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00];
    keyboard_handler.receive_report(report, &hid_io);
    //release keys
    let report: &[u8] = &[0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00];
    keyboard_handler.receive_report(report, &hid_io);
    let status = SimpleTextInExFfi::simple_text_in_ex_read_key_stroke(
      this,
      &mut input_key as *mut protocols::simple_text_input_ex::KeyData,
    );
    assert_eq!(status, efi::Status::SUCCESS);
    assert_eq!(input_key.key.unicode_char, 'a' as u16);
    assert_eq!(input_key.key.scan_code, 0);
    assert_eq!(
      input_key.key_state.key_shift_state,
      protocols::simple_text_input_ex::SHIFT_STATE_VALID | protocols::simple_text_input_ex::LEFT_CONTROL_PRESSED
    );

    //send ctrl-shift-Z - expect it not to be switched to control-character 0x1A
    let report: &[u8] = &[0x03, 0x00, 0x1D, 0x00, 0x00, 0x00, 0x00, 0x00];
    keyboard_handler.receive_report(report, &hid_io);
    //release keys
    let report: &[u8] = &[0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00];
    keyboard_handler.receive_report(report, &hid_io);
    let status = SimpleTextInExFfi::simple_text_in_ex_read_key_stroke(
      this,
      &mut input_key as *mut protocols::simple_text_input_ex::KeyData,
    );
    assert_eq!(status, efi::Status::SUCCESS);
    assert_eq!(input_key.key.unicode_char, 'Z' as u16);
    assert_eq!(input_key.key.scan_code, 0);
    assert_eq!(
      input_key.key_state.key_shift_state,
      protocols::simple_text_input_ex::SHIFT_STATE_VALID | protocols::simple_text_input_ex::LEFT_CONTROL_PRESSED
    );

    keyboard_handler.set_key_toggle_state(protocols::simple_text_input_ex::KEY_STATE_EXPOSED);

    // press the right logo key
    let report: &[u8] = &[0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00];
    keyboard_handler.receive_report(report, &hid_io);

    //release keys
    let report: &[u8] = &[0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00];
    keyboard_handler.receive_report(report, &hid_io);

    // press the a key
    let report: &[u8] = &[0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00];
    keyboard_handler.receive_report(report, &hid_io);

    //release keys
    let report: &[u8] = &[0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00];
    keyboard_handler.receive_report(report, &hid_io);

    // should get partial keystroke.
    let status = SimpleTextInExFfi::simple_text_in_ex_read_key_stroke(
      this,
      &mut input_key as *mut protocols::simple_text_input_ex::KeyData,
    );
    assert_eq!(status, efi::Status::SUCCESS);
    assert_eq!(input_key.key.unicode_char, 0);
    assert_eq!(input_key.key.scan_code, 0);
    assert_eq!(
      input_key.key_state.key_shift_state,
      protocols::simple_text_input_ex::SHIFT_STATE_VALID | protocols::simple_text_input_ex::RIGHT_LOGO_PRESSED
    );

    // followed by 'a' key.
    let status = SimpleTextInExFfi::simple_text_in_ex_read_key_stroke(
      this,
      &mut input_key as *mut protocols::simple_text_input_ex::KeyData,
    );
    assert_eq!(status, efi::Status::SUCCESS);
    assert_eq!(input_key.key.unicode_char, 'a' as u16);
    assert_eq!(input_key.key.scan_code, 0);
    assert_eq!(input_key.key_state.key_shift_state, protocols::simple_text_input_ex::SHIFT_STATE_VALID);
  }

  #[test]
  fn set_state_should_set_state() {
    static CONTEXT_PTR: AtomicPtr<c_void> = AtomicPtr::new(core::ptr::null_mut());
    let boot_services = create_fake_static_boot_service();

    // used in install
    boot_services.expect_create_event().returning(|_, _, _, _, _| efi::Status::SUCCESS);
    boot_services.expect_install_protocol_interface().returning(|_, _, _, interface| {
      CONTEXT_PTR.store(interface, core::sync::atomic::Ordering::SeqCst);
      efi::Status::SUCCESS
    });

    // used in set state
    boot_services.expect_raise_tpl().returning(|_| efi::TPL_APPLICATION);
    boot_services.expect_restore_tpl().returning(|_| ());

    // used in keyboard init and uninstall
    boot_services.expect_create_event_ex().returning(|_, _, _, _, _, _| efi::Status::SUCCESS);
    boot_services.expect_signal_event().returning(|_| efi::Status::SUCCESS);
    extern "efiapi" fn mock_set_report(
      _this: *const hid_io::protocol::Protocol,
      _report_id: u8,
      _report_type: hid_io::protocol::HidReportType,
      _report_buffer_size: usize,
      _report_buffer: *mut c_void,
    ) -> efi::Status {
      efi::Status::SUCCESS
    }

    // used in set_state and uninstall
    boot_services.expect_open_protocol().returning(|_, protocol, interface, _, _, attributes| {
      unsafe {
        match protocol.read() {
          hid_io::protocol::GUID => {
            assert_eq!(attributes, efi::OPEN_PROTOCOL_GET_PROTOCOL);
            let hid_io = MaybeUninit::<hid_io::protocol::Protocol>::zeroed();
            let mut hid_io = hid_io.assume_init();
            hid_io.set_report = mock_set_report;
            // note: this will leak a hid_io instance
            interface.write(Box::into_raw(Box::new(hid_io)) as *mut c_void);
          }
          _unrecognized_guid => return efi::Status::NOT_FOUND,
        }
      }
      efi::Status::SUCCESS
    });

    let mut keyboard_handler = KeyboardHidHandler::new(boot_services, 1 as efi::Handle);

    let mut hid_io = MockHidIo::new();
    hid_io
      .expect_get_report_descriptor()
      .returning(|| Ok(hidparser::parse_report_descriptor(&BOOT_KEYBOARD_REPORT_DESCRIPTOR).unwrap()));

    keyboard_handler.set_layout(Some(hii_keyboard_layout::get_default_keyboard_layout()));
    keyboard_handler.initialize(2 as efi::Handle, &hid_io).unwrap();

    SimpleTextInExFfi::install(boot_services, 2 as efi::Handle, &mut keyboard_handler).unwrap();
    assert_ne!(CONTEXT_PTR.load(core::sync::atomic::Ordering::SeqCst), core::ptr::null_mut());

    let this = CONTEXT_PTR.load(core::sync::atomic::Ordering::SeqCst) as *mut protocols::simple_text_input_ex::Protocol;
    let mut key_toggle_state: protocols::simple_text_input_ex::KeyToggleState =
      protocols::simple_text_input_ex::KEY_STATE_EXPOSED;

    let status = SimpleTextInExFfi::simple_text_in_ex_set_state(this, core::ptr::addr_of_mut!(key_toggle_state));
    assert_eq!(status, efi::Status::SUCCESS);
    assert_eq!(
      keyboard_handler.get_key_state().key_toggle_state,
      key_toggle_state | protocols::simple_text_input_ex::TOGGLE_STATE_VALID
    );

    key_toggle_state =
      protocols::simple_text_input_ex::CAPS_LOCK_ACTIVE | protocols::simple_text_input_ex::NUM_LOCK_ACTIVE;
    let status = SimpleTextInExFfi::simple_text_in_ex_set_state(this, core::ptr::addr_of_mut!(key_toggle_state));
    assert_eq!(status, efi::Status::SUCCESS);
    assert_eq!(
      keyboard_handler.get_key_state().key_toggle_state,
      key_toggle_state | protocols::simple_text_input_ex::TOGGLE_STATE_VALID
    );
  }

  #[test]
  fn register_key_notify_should_register_key() {
    const NOTIFY_EVENT: efi::Event = 1 as efi::Event;
    static CONTEXT_PTR: AtomicPtr<c_void> = AtomicPtr::new(core::ptr::null_mut());
    static KEY_NOTIFIED: AtomicBool = AtomicBool::new(false);
    static KEY2_NOTIFIED: AtomicBool = AtomicBool::new(false);

    let boot_services = create_fake_static_boot_service();

    // used in install
    boot_services.expect_create_event().returning(|_, _, _, _, _| efi::Status::SUCCESS);
    boot_services.expect_install_protocol_interface().returning(|_, _, _, interface| {
      CONTEXT_PTR.store(interface, core::sync::atomic::Ordering::SeqCst);
      efi::Status::SUCCESS
    });

    // used in read key stroke
    boot_services.expect_raise_tpl().returning(|_| efi::TPL_APPLICATION);
    boot_services.expect_restore_tpl().returning(|_| ());

    // used in keyboard init and uninstall
    boot_services.expect_create_event_ex().returning(|_, _, _, _, _, _| efi::Status::SUCCESS);
    boot_services.expect_signal_event().returning(|event| {
      if event == NOTIFY_EVENT {
        SimpleTextInExFfi::process_key_notifies(event, CONTEXT_PTR.load(core::sync::atomic::Ordering::SeqCst));
      }
      efi::Status::SUCCESS
    });
    boot_services.expect_open_protocol().returning(|_, _, _, _, _, _| efi::Status::NOT_FOUND);

    let mut keyboard_handler = KeyboardHidHandler::new(boot_services, 1 as efi::Handle);

    let mut hid_io = MockHidIo::new();
    hid_io
      .expect_get_report_descriptor()
      .returning(|| Ok(hidparser::parse_report_descriptor(&BOOT_KEYBOARD_REPORT_DESCRIPTOR).unwrap()));

    keyboard_handler.set_layout(Some(hii_keyboard_layout::get_default_keyboard_layout()));
    keyboard_handler.initialize(2 as efi::Handle, &hid_io).unwrap();
    keyboard_handler.set_notify_event(NOTIFY_EVENT);

    SimpleTextInExFfi::install(boot_services, 2 as efi::Handle, &mut keyboard_handler).unwrap();
    assert_ne!(CONTEXT_PTR.load(core::sync::atomic::Ordering::SeqCst), core::ptr::null_mut());

    extern "efiapi" fn key_notify_callback_a(key_data: *mut protocols::simple_text_input_ex::KeyData) -> efi::Status {
      let key = unsafe { key_data.read() };
      assert_eq!(key.key.unicode_char, 'a' as u16);
      KEY_NOTIFIED.store(true, core::sync::atomic::Ordering::SeqCst);
      efi::Status::SUCCESS
    }

    extern "efiapi" fn key_notify_callback_a_and_b(
      key_data: *mut protocols::simple_text_input_ex::KeyData,
    ) -> efi::Status {
      let key = unsafe { key_data.read() };
      assert!((key.key.unicode_char == 'a' as u16) || (key.key.unicode_char == 'b' as u16));
      KEY2_NOTIFIED.store(true, core::sync::atomic::Ordering::SeqCst);
      efi::Status::SUCCESS
    }

    let this = CONTEXT_PTR.load(core::sync::atomic::Ordering::SeqCst) as *mut protocols::simple_text_input_ex::Protocol;

    let mut key_data: protocols::simple_text_input_ex::KeyData = Default::default();
    key_data.key.unicode_char = 'a' as u16;
    let mut notify_handle = core::ptr::null_mut();

    let status = SimpleTextInExFfi::simple_text_in_ex_register_key_notify(
      this,
      core::ptr::addr_of_mut!(key_data),
      key_notify_callback_a,
      core::ptr::addr_of_mut!(notify_handle),
    );
    assert_eq!(status, efi::Status::SUCCESS);
    assert_eq!(notify_handle as usize, 1);

    notify_handle = core::ptr::null_mut();
    let status = SimpleTextInExFfi::simple_text_in_ex_register_key_notify(
      this,
      core::ptr::addr_of_mut!(key_data),
      key_notify_callback_a_and_b,
      core::ptr::addr_of_mut!(notify_handle),
    );
    assert_eq!(status, efi::Status::SUCCESS);
    assert_eq!(notify_handle as usize, 2);

    notify_handle = core::ptr::null_mut();
    key_data.key.unicode_char = 'b' as u16;
    let status = SimpleTextInExFfi::simple_text_in_ex_register_key_notify(
      this,
      core::ptr::addr_of_mut!(key_data),
      key_notify_callback_a_and_b,
      core::ptr::addr_of_mut!(notify_handle),
    );
    assert_eq!(status, efi::Status::SUCCESS);
    assert_eq!(notify_handle as usize, 3);

    //send 'b'
    let report: &[u8] = &[0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00];
    keyboard_handler.receive_report(report, &hid_io);

    //release
    let report: &[u8] = &[0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00];
    keyboard_handler.receive_report(report, &hid_io);

    assert!(!KEY_NOTIFIED.load(core::sync::atomic::Ordering::SeqCst));
    assert!(KEY2_NOTIFIED.load(core::sync::atomic::Ordering::SeqCst));

    KEY_NOTIFIED.store(false, core::sync::atomic::Ordering::SeqCst);
    KEY2_NOTIFIED.store(false, core::sync::atomic::Ordering::SeqCst);

    //send 'a'
    let report: &[u8] = &[0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00];
    keyboard_handler.receive_report(report, &hid_io);

    //release
    let report: &[u8] = &[0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00];
    keyboard_handler.receive_report(report, &hid_io);

    assert!(KEY_NOTIFIED.load(core::sync::atomic::Ordering::SeqCst));
    assert!(KEY2_NOTIFIED.load(core::sync::atomic::Ordering::SeqCst));

    KEY_NOTIFIED.store(false, core::sync::atomic::Ordering::SeqCst);
    KEY2_NOTIFIED.store(false, core::sync::atomic::Ordering::SeqCst);

    //remove the 'a'-only callback
    let status = SimpleTextInExFfi::simple_text_in_ex_unregister_key_notify(this, 1 as *mut c_void);
    assert_eq!(status, efi::Status::SUCCESS);

    //send 'a'
    let report: &[u8] = &[0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00];
    keyboard_handler.receive_report(report, &hid_io);

    //release
    let report: &[u8] = &[0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00];
    keyboard_handler.receive_report(report, &hid_io);

    assert!(!KEY_NOTIFIED.load(core::sync::atomic::Ordering::SeqCst));
    assert!(KEY2_NOTIFIED.load(core::sync::atomic::Ordering::SeqCst));
  }

  #[test]
  fn wait_for_key_should_wait_for_key() {
    static CONTEXT_PTR: AtomicPtr<c_void> = AtomicPtr::new(core::ptr::null_mut());
    static RECEIVED_EVENT: AtomicBool = AtomicBool::new(false);

    let boot_services = create_fake_static_boot_service();

    // used in install
    boot_services.expect_create_event().returning(|_, _, _, _, _| efi::Status::SUCCESS);
    boot_services.expect_install_protocol_interface().returning(|_, protocol, _, interface| {
      if unsafe { *protocol } == protocols::simple_text_input_ex::PROTOCOL_GUID {
        CONTEXT_PTR.store(interface, core::sync::atomic::Ordering::SeqCst);
      }
      efi::Status::SUCCESS
    });
    boot_services.expect_create_event_ex().returning(|_, _, _, _, _, _| efi::Status::SUCCESS);
    boot_services.expect_raise_tpl().returning(|_| efi::TPL_APPLICATION);
    boot_services.expect_restore_tpl().returning(|_| ());

    boot_services.expect_signal_event().returning(|_| {
      RECEIVED_EVENT.store(true, core::sync::atomic::Ordering::SeqCst);
      efi::Status::SUCCESS
    });

    let mut keyboard_handler = KeyboardHidHandler::new(boot_services, 1 as efi::Handle);

    let mut hid_io = MockHidIo::new();
    hid_io
      .expect_get_report_descriptor()
      .returning(|| Ok(hidparser::parse_report_descriptor(&BOOT_KEYBOARD_REPORT_DESCRIPTOR).unwrap()));

    keyboard_handler.set_layout(Some(hii_keyboard_layout::get_default_keyboard_layout()));
    keyboard_handler.set_controller(Some(2 as efi::Handle));
    keyboard_handler.initialize(2 as efi::Handle, &hid_io).unwrap();

    RECEIVED_EVENT.store(false, core::sync::atomic::Ordering::SeqCst);
    SimpleTextInExFfi::simple_text_in_ex_wait_for_key(
      3 as efi::Event,
      CONTEXT_PTR.load(core::sync::atomic::Ordering::SeqCst),
    );
    assert!(!RECEIVED_EVENT.load(core::sync::atomic::Ordering::SeqCst));

    // press the a key
    let report: &[u8] = &[0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00];
    keyboard_handler.receive_report(report, &hid_io);

    //release keys
    let report: &[u8] = &[0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00];
    keyboard_handler.receive_report(report, &hid_io);

    SimpleTextInExFfi::simple_text_in_ex_wait_for_key(
      3 as efi::Event,
      CONTEXT_PTR.load(core::sync::atomic::Ordering::SeqCst),
    );
    assert!(RECEIVED_EVENT.load(core::sync::atomic::Ordering::SeqCst));

    // avoid keyboard drop uninstall flows
    keyboard_handler.set_controller(None);
  }
}
