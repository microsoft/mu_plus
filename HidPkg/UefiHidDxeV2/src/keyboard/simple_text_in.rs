//! Simple Text In Protocol FFI Support.
//!
//! This module manages the Simple Text In FFI.
//!
//! ## License
//!
//! Copyright (c) Microsoft Corporation. All rights reserved.
//! SPDX-License-Identifier: BSD-2-Clause-Patent
//!

use alloc::boxed::Box;
use core::{ffi::c_void, ptr};

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
/// In addition, the simple_text_in protocol element needs to be the first element in the structure so that the full
/// structure can be recovered by simple casting for simple_text_in_ex FFI interfaces that only receive a pointer to the
/// simple_text_in protocol structure.
#[repr(C)]
pub struct SimpleTextInFfi {
    simple_text_in: protocols::simple_text_input::Protocol,
    boot_services: &'static dyn UefiBootServices,
    keyboard_handler: *mut KeyboardHidHandler,
}

impl SimpleTextInFfi {
    /// Installs the simple text in protocol
    pub fn install(
        boot_services: &'static dyn UefiBootServices,
        controller: efi::Handle,
        keyboard_handler: &mut KeyboardHidHandler,
    ) -> Result<(), efi::Status> {
        //Create simple_text_in context
        let simple_text_in_ctx = SimpleTextInFfi {
            simple_text_in: protocols::simple_text_input::Protocol {
                reset: Self::simple_text_in_reset,
                read_key_stroke: Self::simple_text_in_read_key_stroke,
                wait_for_key: ptr::null_mut(),
            },
            boot_services,
            keyboard_handler: keyboard_handler as *mut KeyboardHidHandler,
        };

        let simple_text_in_ptr = Box::into_raw(Box::new(simple_text_in_ctx));

        //create event for wait_for_key
        let mut wait_for_key_event: efi::Event = ptr::null_mut();
        let status = boot_services.create_event(
            efi::EVT_NOTIFY_WAIT,
            efi::TPL_NOTIFY,
            Some(Self::simple_text_in_wait_for_key),
            simple_text_in_ptr as *mut c_void,
            ptr::addr_of_mut!(wait_for_key_event),
        );
        if status.is_error() {
            drop(unsafe { Box::from_raw(simple_text_in_ptr) });
            return Err(status);
        }

        unsafe { (*simple_text_in_ptr).simple_text_in.wait_for_key = wait_for_key_event };

        //install the simple_text_in protocol
        let mut controller = controller;
        let status = boot_services.install_protocol_interface(
            ptr::addr_of_mut!(controller),
            &protocols::simple_text_input::PROTOCOL_GUID as *const efi::Guid as *mut efi::Guid,
            efi::NATIVE_INTERFACE,
            simple_text_in_ptr as *mut c_void,
        );

        if status.is_error() {
            let _ = boot_services.close_event(wait_for_key_event);
            drop(unsafe { Box::from_raw(simple_text_in_ptr) });
            return Err(status);
        }

        Ok(())
    }

    /// Uninstalls the simple text in protocol
    pub fn uninstall(
        boot_services: &'static dyn UefiBootServices,
        agent: efi::Handle,
        controller: efi::Handle,
    ) -> Result<(), efi::Status> {
        //Controller is set - that means initialize() was called, and there is potential state exposed thru FFI that needs
        //to be cleaned up.
        let mut simple_text_in_ptr: *mut SimpleTextInFfi = ptr::null_mut();
        let status = boot_services.open_protocol(
            controller,
            &protocols::simple_text_input::PROTOCOL_GUID as *const efi::Guid as *mut efi::Guid,
            ptr::addr_of_mut!(simple_text_in_ptr) as *mut *mut c_void,
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
            &protocols::simple_text_input::PROTOCOL_GUID as *const efi::Guid as *mut efi::Guid,
            simple_text_in_ptr as *mut c_void,
        );
        if status.is_error() {
            //An error here means some other driver might be holding on to the simple_text_in_ptr.
            //Mark the instance invalid by setting the keyboard_handler raw pointer to null, but leak the PointerContext
            //instance. Leaking context allows calls through the pointers on absolute_pointer_ptr to continue to resolve
            //and return error based on observing keyboard_handler is null.
            debugln!(DEBUG_ERROR, "Failed to uninstall simple_text_in interface, status: {:x?}", status);

            unsafe {
                (*simple_text_in_ptr).keyboard_handler = ptr::null_mut();
            }
            //return without tearing down the context.
            return Err(status);
        }

        let wait_for_key_event: efi::Handle = unsafe { (*simple_text_in_ptr).simple_text_in.wait_for_key };
        let status = boot_services.close_event(wait_for_key_event);
        if status.is_error() {
            //An error here means the event was not closed, so in theory the notification_callback on it could still be
            //fired.
            //Mark the instance invalid by setting the keyboard_handler raw pointer to null, but leak the PointerContext
            //instance. Leaking context allows calls through the pointers on simple_text_in_ptr to continue to resolve
            //and return error based on observing keyboard_handler is null.
            debugln!(DEBUG_ERROR, "Failed to close simple_text_in_ptr.wait_for_key event, status: {:x?}", status);
            unsafe {
                (*simple_text_in_ptr).keyboard_handler = ptr::null_mut();
            }
            return Err(status);
        }
        // None of the parts of simple_text_in_ptr are in use, so it is safe to drop it.
        drop(unsafe { Box::from_raw(simple_text_in_ptr) });
        Ok(())
    }

    // resets the keyboard state - part of the simple_text_in protocol interface.
    extern "efiapi" fn simple_text_in_reset(
        this: *mut protocols::simple_text_input::Protocol,
        extended_verification: efi::Boolean,
    ) -> efi::Status {
        if this.is_null() {
            return efi::Status::INVALID_PARAMETER;
        }
        let context = unsafe { (this as *mut SimpleTextInFfi).as_mut() }.expect("bad pointer");
        let old_tpl = context.boot_services.raise_tpl(efi::TPL_NOTIFY);
        let mut status = efi::Status::DEVICE_ERROR;
        '_reset_processing: {
            let keyboard_handler = unsafe { context.keyboard_handler.as_mut() };
            if let Some(keyboard_handler) = keyboard_handler {
                //reset requires an instance of hid_io to allow for LED updating, so use UefiHidIoFactory to build one.
                if let Some(controller) = keyboard_handler.controller() {
                    let hid_io = UefiHidIoFactory::new(context.boot_services, keyboard_handler.agent())
                        .new_hid_io(controller, false);
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

    // reads a key stroke - part of the simple_text_in protocol interface.
    extern "efiapi" fn simple_text_in_read_key_stroke(
        this: *mut protocols::simple_text_input::Protocol,
        key: *mut protocols::simple_text_input::InputKey,
    ) -> efi::Status {
        if this.is_null() || key.is_null() {
            return efi::Status::INVALID_PARAMETER;
        }
        let context = unsafe { (this as *mut SimpleTextInFfi).as_mut() }.expect("bad pointer");
        let status;
        let old_tpl = context.boot_services.raise_tpl(efi::TPL_NOTIFY);
        'read_key_stroke: {
            let keyboard_handler = unsafe { context.keyboard_handler.as_mut() };
            if let Some(keyboard_handler) = keyboard_handler {
                loop {
                    if let Some(mut key_data) = keyboard_handler.pop_key() {
                        // skip partials
                        if key_data.key.unicode_char == 0 && key_data.key.scan_code == 0 {
                            continue;
                        }
                        //translate ctrl-alpha to corresponding control value. ctrl-a = 0x0001, ctrl-z = 0x001A
                        const CONTROL_PRESSED: u32 = protocols::simple_text_input_ex::RIGHT_CONTROL_PRESSED
                            | protocols::simple_text_input_ex::LEFT_CONTROL_PRESSED;
                        if (key_data.key_state.key_shift_state & CONTROL_PRESSED) != 0 {
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
                        status = efi::Status::SUCCESS;
                    } else {
                        status = efi::Status::NOT_READY;
                    }
                    break 'read_key_stroke;
                }
            } else {
                status = efi::Status::DEVICE_ERROR;
                break 'read_key_stroke;
            }
        }
        context.boot_services.restore_tpl(old_tpl);
        status
    }

    // Event handler function for the wait_for_key_event
    extern "efiapi" fn simple_text_in_wait_for_key(event: efi::Event, context: *mut c_void) {
        if context.is_null() {
            debugln!(DEBUG_ERROR, "simple_text_in_wait_for_key invoked with invalid context");
            return;
        }
        let context = unsafe { (context as *mut SimpleTextInFfi).as_mut() }.expect("bad pointer");
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
}

#[cfg(test)]
mod test {
    use core::{
        ffi::c_void,
        mem::MaybeUninit,
        ptr,
        sync::atomic::{AtomicBool, AtomicPtr, Ordering},
    };

    use crate::{
        boot_services::MockUefiBootServices,
        hid_io::{HidReportReceiver, MockHidIo},
        keyboard::KeyboardHidHandler,
    };
    use r_efi::{efi, protocols};

    use super::SimpleTextInFfi;

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
    fn install_should_install_simple_text_in_interface() {
        let boot_services = create_fake_static_boot_service();
        boot_services.expect_create_event().returning(|_, _, _, _, _| efi::Status::SUCCESS);
        boot_services.expect_install_protocol_interface().returning(|_, protocol, _, _| {
            assert_eq!(unsafe { protocol.read() }, protocols::simple_text_input::PROTOCOL_GUID);
            efi::Status::SUCCESS
        });
        let mut keyboard_handler = KeyboardHidHandler::new(boot_services, 1 as efi::Handle);

        SimpleTextInFfi::install(boot_services, 2 as efi::Handle, &mut keyboard_handler).unwrap();
    }

    #[test]
    fn uninstall_should_uninstall_simple_text_in_interface() {
        static CONTEXT_PTR: AtomicPtr<c_void> = AtomicPtr::new(ptr::null_mut());
        let boot_services = create_fake_static_boot_service();

        // used in install
        boot_services.expect_create_event().returning(|_, _, _, _, _| efi::Status::SUCCESS);
        boot_services.expect_install_protocol_interface().returning(|_, _, _, interface| {
            CONTEXT_PTR.store(interface, Ordering::SeqCst);
            efi::Status::SUCCESS
        });

        // used in uninstall
        boot_services.expect_open_protocol().returning(|_, protocol, interface, _, _, _| {
            unsafe {
                assert_eq!(protocol.read(), protocols::simple_text_input::PROTOCOL_GUID);
                interface.write(CONTEXT_PTR.load(Ordering::SeqCst));
            }
            efi::Status::SUCCESS
        });
        boot_services.expect_uninstall_protocol_interface().returning(|_, protocol, interface| {
            unsafe {
                assert_eq!(protocol.read(), protocols::simple_text_input::PROTOCOL_GUID);
                assert_eq!(interface, CONTEXT_PTR.load(Ordering::SeqCst));
            }
            efi::Status::SUCCESS
        });
        boot_services.expect_close_event().returning(|_| efi::Status::SUCCESS);

        let mut keyboard_handler = KeyboardHidHandler::new(boot_services, 1 as efi::Handle);

        SimpleTextInFfi::install(boot_services, 2 as efi::Handle, &mut keyboard_handler).unwrap();
        assert_ne!(CONTEXT_PTR.load(Ordering::SeqCst), ptr::null_mut());

        SimpleTextInFfi::uninstall(boot_services, 1 as efi::Handle, 2 as efi::Handle).unwrap();
    }

    #[test]
    fn reset_should_invoke_reset() {
        static CONTEXT_PTR: AtomicPtr<c_void> = AtomicPtr::new(ptr::null_mut());
        let boot_services = create_fake_static_boot_service();

        // used in install
        boot_services.expect_create_event().returning(|_, _, _, _, _| efi::Status::SUCCESS);
        boot_services.expect_install_protocol_interface().returning(|_, _, _, interface| {
            CONTEXT_PTR.store(interface, Ordering::SeqCst);
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
                assert_eq!(protocol.read(), protocols::simple_text_input::PROTOCOL_GUID);
                assert_eq!(interface, CONTEXT_PTR.load(Ordering::SeqCst));
            }
            efi::Status::SUCCESS
        });
        boot_services.expect_close_event().returning(|_| efi::Status::SUCCESS);

        let mut keyboard_handler = KeyboardHidHandler::new(boot_services, 1 as efi::Handle);
        keyboard_handler.set_controller(Some(2 as efi::Handle));

        SimpleTextInFfi::install(boot_services, 2 as efi::Handle, &mut keyboard_handler).unwrap();
        assert_ne!(CONTEXT_PTR.load(Ordering::SeqCst), ptr::null_mut());

        let this = CONTEXT_PTR.load(Ordering::SeqCst) as *mut protocols::simple_text_input::Protocol;
        SimpleTextInFfi::simple_text_in_reset(this, efi::Boolean::FALSE);

        // avoid keyboard drop uninstall flows
        keyboard_handler.set_controller(None);
    }

    #[test]
    fn read_key_stroke_should_read_keystrokes() {
        static CONTEXT_PTR: AtomicPtr<c_void> = AtomicPtr::new(ptr::null_mut());
        let boot_services = create_fake_static_boot_service();

        // used in install
        boot_services.expect_create_event().returning(|_, _, _, _, _| efi::Status::SUCCESS);
        boot_services.expect_install_protocol_interface().returning(|_, _, _, interface| {
            CONTEXT_PTR.store(interface, Ordering::SeqCst);
            efi::Status::SUCCESS
        });

        // used in reset
        boot_services.expect_raise_tpl().returning(|_| efi::TPL_APPLICATION);
        boot_services.expect_restore_tpl().returning(|_| ());

        // used in keyboard init and uninstall
        boot_services.expect_create_event_ex().returning(|_, _, _, _, _, _| efi::Status::SUCCESS);
        boot_services.expect_signal_event().returning(|_| efi::Status::SUCCESS);
        boot_services.expect_open_protocol().returning(|_, _, _, _, _, _| efi::Status::NOT_FOUND);
        boot_services.expect_locate_protocol().returning(|_, _, _| efi::Status::NOT_FOUND);

        let mut keyboard_handler = KeyboardHidHandler::new(boot_services, 1 as efi::Handle);

        let mut hid_io = MockHidIo::new();
        hid_io.expect_set_output_report().returning(|_, _| Ok(()));
        hid_io
            .expect_get_report_descriptor()
            .returning(|| Ok(hidparser::parse_report_descriptor(&BOOT_KEYBOARD_REPORT_DESCRIPTOR).unwrap()));

        keyboard_handler.set_layout(Some(hii_keyboard_layout::get_default_keyboard_layout()));
        keyboard_handler.initialize(2 as efi::Handle, &hid_io).unwrap();

        SimpleTextInFfi::install(boot_services, 2 as efi::Handle, &mut keyboard_handler).unwrap();
        assert_ne!(CONTEXT_PTR.load(Ordering::SeqCst), ptr::null_mut());

        let report: &[u8] = &[0x00, 0x00, 0x04, 0x05, 0x06, 0x00, 0x00, 0x00];
        keyboard_handler.receive_report(report, &hid_io);
        //release keys and push ctrl
        let report: &[u8] = &[0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00];
        keyboard_handler.receive_report(report, &hid_io);

        //read with simple_text_in
        let this = CONTEXT_PTR.load(Ordering::SeqCst) as *mut protocols::simple_text_input::Protocol;
        let mut input_key: protocols::simple_text_input::InputKey = Default::default();

        let status = SimpleTextInFfi::simple_text_in_read_key_stroke(
            this,
            &mut input_key as *mut protocols::simple_text_input::InputKey,
        );
        assert_eq!(status, efi::Status::SUCCESS);
        assert_eq!(input_key.unicode_char, 'c' as u16);
        assert_eq!(input_key.scan_code, 0);

        let status = SimpleTextInFfi::simple_text_in_read_key_stroke(
            this,
            &mut input_key as *mut protocols::simple_text_input::InputKey,
        );
        assert_eq!(status, efi::Status::SUCCESS);
        assert_eq!(input_key.unicode_char, 'b' as u16);
        assert_eq!(input_key.scan_code, 0);

        let status = SimpleTextInFfi::simple_text_in_read_key_stroke(
            this,
            &mut input_key as *mut protocols::simple_text_input::InputKey,
        );
        assert_eq!(status, efi::Status::SUCCESS);
        assert_eq!(input_key.unicode_char, 'a' as u16);
        assert_eq!(input_key.scan_code, 0);

        let status = SimpleTextInFfi::simple_text_in_read_key_stroke(
            this,
            &mut input_key as *mut protocols::simple_text_input::InputKey,
        );
        assert_eq!(status, efi::Status::NOT_READY);

        //send ctrl-a - expect it to be switched to control-character 0x01
        let report: &[u8] = &[0x01, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00];
        keyboard_handler.receive_report(report, &hid_io);
        //release keys
        let report: &[u8] = &[0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00];
        keyboard_handler.receive_report(report, &hid_io);
        let status = SimpleTextInFfi::simple_text_in_read_key_stroke(
            this,
            &mut input_key as *mut protocols::simple_text_input::InputKey,
        );
        assert_eq!(status, efi::Status::SUCCESS);
        assert_eq!(input_key.unicode_char, 0x1);
        assert_eq!(input_key.scan_code, 0);

        //send ctrl-shift-Z - expect it to be switched to control-character 0x1A
        let report: &[u8] = &[0x03, 0x00, 0x1D, 0x00, 0x00, 0x00, 0x00, 0x00];
        keyboard_handler.receive_report(report, &hid_io);
        //release keys
        let report: &[u8] = &[0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00];
        keyboard_handler.receive_report(report, &hid_io);
        let status = SimpleTextInFfi::simple_text_in_read_key_stroke(
            this,
            &mut input_key as *mut protocols::simple_text_input::InputKey,
        );
        assert_eq!(status, efi::Status::SUCCESS);
        assert_eq!(input_key.unicode_char, 0x1a);
        assert_eq!(input_key.scan_code, 0);

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

        // should get only the 'a', partial keystroke should be dropped.
        let status = SimpleTextInFfi::simple_text_in_read_key_stroke(
            this,
            &mut input_key as *mut protocols::simple_text_input::InputKey,
        );
        assert_eq!(status, efi::Status::SUCCESS);
        assert_eq!(input_key.unicode_char, 'a' as u16);
        assert_eq!(input_key.scan_code, 0);

        let status = SimpleTextInFfi::simple_text_in_read_key_stroke(
            this,
            &mut input_key as *mut protocols::simple_text_input::InputKey,
        );
        assert_eq!(status, efi::Status::NOT_READY);
    }

    #[test]
    fn wait_for_key_should_wait_for_key() {
        static CONTEXT_PTR: AtomicPtr<c_void> = AtomicPtr::new(ptr::null_mut());
        static RECEIVED_EVENT: AtomicBool = AtomicBool::new(false);

        let boot_services = create_fake_static_boot_service();

        // used in install
        boot_services.expect_create_event().returning(|_, _, _, _, _| efi::Status::SUCCESS);
        boot_services.expect_locate_protocol().returning(|_, _, _| efi::Status::NOT_FOUND);
        boot_services.expect_install_protocol_interface().returning(|_, protocol, _, interface| {
            if unsafe { *protocol } == protocols::simple_text_input::PROTOCOL_GUID {
                CONTEXT_PTR.store(interface, Ordering::SeqCst);
            }
            efi::Status::SUCCESS
        });
        boot_services.expect_create_event_ex().returning(|_, _, _, _, _, _| efi::Status::SUCCESS);
        boot_services.expect_raise_tpl().returning(|_| efi::TPL_APPLICATION);
        boot_services.expect_restore_tpl().returning(|_| ());

        boot_services.expect_signal_event().returning(|_| {
            RECEIVED_EVENT.store(true, Ordering::SeqCst);
            efi::Status::SUCCESS
        });

        let mut keyboard_handler = KeyboardHidHandler::new(boot_services, 1 as efi::Handle);

        let mut hid_io = MockHidIo::new();
        hid_io.expect_set_output_report().returning(|_, _| Ok(()));
        hid_io
            .expect_get_report_descriptor()
            .returning(|| Ok(hidparser::parse_report_descriptor(&BOOT_KEYBOARD_REPORT_DESCRIPTOR).unwrap()));

        keyboard_handler.set_layout(Some(hii_keyboard_layout::get_default_keyboard_layout()));
        keyboard_handler.set_controller(Some(2 as efi::Handle));
        keyboard_handler.initialize(2 as efi::Handle, &hid_io).unwrap();

        RECEIVED_EVENT.store(false, Ordering::SeqCst);
        SimpleTextInFfi::simple_text_in_wait_for_key(3 as efi::Event, CONTEXT_PTR.load(Ordering::SeqCst));
        assert!(!RECEIVED_EVENT.load(Ordering::SeqCst));

        // press the a key
        let report: &[u8] = &[0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00];
        keyboard_handler.receive_report(report, &hid_io);

        //release keys
        let report: &[u8] = &[0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00];
        keyboard_handler.receive_report(report, &hid_io);

        SimpleTextInFfi::simple_text_in_wait_for_key(3 as efi::Event, CONTEXT_PTR.load(Ordering::SeqCst));
        assert!(RECEIVED_EVENT.load(Ordering::SeqCst));

        // avoid keyboard drop uninstall flows
        keyboard_handler.set_controller(None);
    }
}
