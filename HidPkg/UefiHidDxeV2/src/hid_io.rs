//! Provides HidIo interface support.
//!
//! This module defines traits for abstractions that support the HidIo protocol
//! as well as concrete implementations that provide those traits by wrapping
//! the HidIo protocol from external devices.
//!
//! ## License
//!
//! Copyright (C) Microsoft Corporation. All rights reserved.
//!
//! SPDX-License-Identifier: BSD-2-Clause-Patent
//!
use core::{ffi::c_void, slice::from_raw_parts_mut};

use alloc::{boxed::Box, vec};

use hid_io::protocol::HidReportType;
use hidparser::ReportDescriptor;
use r_efi::efi;
use rust_advanced_logger_dxe::{debugln, DEBUG_ERROR};

#[cfg(test)]
use mockall::automock;

use crate::boot_services::UefiBootServices;

/// Defines an interface to be implemented by logic that wants to receive hid reports.
#[cfg_attr(test, automock)]
pub trait HidReportReceiver {
  /// Initializes the receiver. After this call, the receiver should be prepared to receive reports.
  fn initialize(&mut self, controller: efi::Handle, hid_io: &dyn HidIo) -> Result<(), efi::Status>;
  /// Called to pass a report to the receiver.
  fn receive_report(&mut self, report: &[u8], hid_io: &dyn HidIo);
}

/// Defines an interface to abstract interaction with the HidIo protocol.
///
/// Refer to: <https://github.com/microsoft/mu_plus/blob/14c187b8ac4858d154612cd67a96820f78fe5584/HidPkg/Include/Protocol/HidIo.h>
#[cfg_attr(test, automock)]
pub trait HidIo {
  /// Returns the parsed report descriptor for the device.
  fn get_report_descriptor(&self) -> Result<ReportDescriptor, efi::Status>;
  /// sends an output report to the device.
  fn set_output_report(&self, id: Option<u8>, report: &[u8]) -> Result<(), efi::Status>;
  /// configures a receiver to receive reports from the device and configures the device to send reports.
  fn set_report_receiver(&mut self, receiver: Box<dyn HidReportReceiver>) -> Result<(), efi::Status>;
  /// removes the receiver and stops the device from sending reports.
  fn take_report_receiver(&mut self) -> Option<Box<dyn HidReportReceiver>>;
}

/// Defines a factory interface for producing HidIo instances on a given controller.
#[cfg_attr(test, automock)]
pub trait HidIoFactory {
  /// Creates a new instance of HidIo on the given controller. If `owned` is set, then the implementation
  /// expects to have ownership of the device (to be released when dropped). If not `owned`, then the
  /// HidIo instance is being opened for a transient non-owned usage and does not need to be released.
  fn new_hid_io(&self, controller: efi::Handle, owned: bool) -> Result<Box<dyn HidIo>, efi::Status>;
}

/// Implements the HidIoFactory interface using UEFI boot services to open HidIo protocols on supported controllers.
pub struct UefiHidIoFactory {
  boot_services: &'static dyn UefiBootServices,
  agent: efi::Handle,
}

impl UefiHidIoFactory {
  /// Creates a new UefiHidIoFactory. `agent` represents the owner of the factory (typically the image_handle).
  pub fn new(boot_services: &'static dyn UefiBootServices, agent: efi::Handle) -> Self {
    UefiHidIoFactory { boot_services, agent }
  }
}

impl HidIoFactory for UefiHidIoFactory {
  /// instantiate a new UefiHidIo instance on the given controller.
  fn new_hid_io(&self, controller: efi::Handle, owned: bool) -> Result<Box<dyn HidIo>, efi::Status> {
    let hid_io = UefiHidIo::new(self.boot_services, self.agent, controller, owned)?;
    Ok(Box::new(hid_io))
  }
}

/// Implements the HidIo interface on top of the HidIo protocol.
pub struct UefiHidIo {
  hid_io: &'static mut hid_io::protocol::Protocol,
  boot_services: &'static dyn UefiBootServices,
  controller: efi::Handle,
  agent: efi::Handle,
  receiver: Option<Box<dyn HidReportReceiver>>,
  owned: bool,
}

impl UefiHidIo {
  // creates a new HidIo - private, intended only to be invoked by the [`UefiHidIoFactory`] implementation.
  // if `owned`, then the interface is opened BY_DRIVER, otherwise it is opened with GET_PROTOCOL.
  fn new(
    boot_services: &'static dyn UefiBootServices,
    agent: efi::Handle,
    controller: efi::Handle,
    owned: bool,
  ) -> Result<Self, efi::Status> {
    let mut hid_io_ptr: *mut hid_io::protocol::Protocol = core::ptr::null_mut();

    let attributes = {
      if owned {
        efi::OPEN_PROTOCOL_BY_DRIVER
      } else {
        efi::OPEN_PROTOCOL_GET_PROTOCOL
      }
    };

    let status = boot_services.open_protocol(
      controller,
      &hid_io::protocol::GUID as *const efi::Guid as *mut efi::Guid,
      core::ptr::addr_of_mut!(hid_io_ptr) as *mut *mut c_void,
      agent,
      controller,
      attributes,
    );

    if status.is_error() {
      return Err(status);
    }

    let hid_io = unsafe { hid_io_ptr.as_mut().expect("bad hid_io ptr") };
    Ok(Self { hid_io, boot_services, controller, agent, receiver: None, owned })
  }

  // the report callback FFI interface that is submitted to the HidIo instance to receive callbacks for reports.
  extern "efiapi" fn report_callback(report_buffer_size: u16, report_buffer: *mut c_void, context: *mut c_void) {
    let hid_io = unsafe { (context as *mut Self).as_mut().expect("bad context") };
    if let Some(mut receiver) = hid_io.receiver.take() {
      let report = unsafe { from_raw_parts_mut(report_buffer as *mut u8, report_buffer_size as usize) };
      receiver.receive_report(report, hid_io);
      hid_io.receiver = Some(receiver);
    }
  }
}

impl Drop for UefiHidIo {
  // Closes the HidIo interface if owned.
  fn drop(&mut self) {
    if self.owned {
      let _ = self.take_report_receiver();
      let status = self.boot_services.close_protocol(
        self.controller,
        &hid_io::protocol::GUID as *const efi::Guid as *mut efi::Guid,
        self.agent,
        self.controller,
      );
      if status.is_error() {
        debugln!(DEBUG_ERROR, "Unexpected error closing hid_io: {:x?}", status);
      }
    }
  }
}

impl HidIo for UefiHidIo {
  fn get_report_descriptor(&self) -> Result<ReportDescriptor, efi::Status> {
    let mut report_descriptor_size: usize = 0;
    match (self.hid_io.get_report_descriptor)(
      self.hid_io,
      core::ptr::addr_of_mut!(report_descriptor_size),
      core::ptr::null_mut(),
    ) {
      efi::Status::BUFFER_TOO_SMALL => (),
      efi::Status::SUCCESS => return Err(efi::Status::DEVICE_ERROR),
      err => return Err(err),
    }

    let mut report_descriptor_buffer = vec![0u8; report_descriptor_size];
    let report_descriptor_buffer_ptr = report_descriptor_buffer.as_mut_ptr();

    match (self.hid_io.get_report_descriptor)(
      self.hid_io,
      core::ptr::addr_of_mut!(report_descriptor_size),
      report_descriptor_buffer_ptr as *mut c_void,
    ) {
      efi::Status::SUCCESS => (),
      err => return Err(err),
    }

    hidparser::parse_report_descriptor(&report_descriptor_buffer).map_err(|_| efi::Status::DEVICE_ERROR)
  }

  fn set_output_report(&self, id: Option<u8>, report: &[u8]) -> Result<(), efi::Status> {
    match (self.hid_io.set_report)(
      self.hid_io,
      id.unwrap_or(0),
      HidReportType::OutputReport,
      report.len(),
      report.as_ptr() as *mut c_void,
    ) {
      efi::Status::SUCCESS => Ok(()),
      err => Err(err),
    }
  }

  fn set_report_receiver(&mut self, receiver: Box<dyn HidReportReceiver>) -> Result<(), efi::Status> {
    if !self.owned {
      return Err(efi::Status::ACCESS_DENIED);
    }
    let self_ptr = self as *mut UefiHidIo;

    //always attempt uninstall. Failure is ok if not already installed. This shuts down report callback generation
    //(if any) so that callbacks are not occurring while the new receiver is installed.
    let _ = (self.hid_io.unregister_report_callback)(self.hid_io, Self::report_callback);

    match (self.hid_io.register_report_callback)(self.hid_io, Self::report_callback, self_ptr as *mut c_void) {
      efi::Status::SUCCESS => (),
      err => return Err(err),
    }
    self.receiver = Some(receiver);

    Ok(())
  }
  fn take_report_receiver(&mut self) -> Option<Box<dyn HidReportReceiver>> {
    if !self.owned {
      return None;
    }
    //always attempt uninstall. Failure is ok if not already installed. This shuts down report callback generation
    //(if any) so that callbacks are not occurring before the receiver is removed.
    let _ = (self.hid_io.unregister_report_callback)(self.hid_io, Self::report_callback);
    self.receiver.take()
  }
}

#[cfg(test)]
mod test {
  use core::{
    ffi::c_void,
    slice::{from_raw_parts, from_raw_parts_mut},
  };

  use super::{HidIo, MockHidReportReceiver, UefiHidIo};

  use crate::boot_services::MockUefiBootServices;

  use r_efi::efi;

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

  static TEST_REPORT0: &[u8] = &[0x0, 0x1, 0x2, 0x3, 0x4];
  static TEST_REPORT1: &[u8] = &[0x4, 0x3, 0x2, 0x1, 0x0];

  // In this module, the usage model for boot_services is global static, and so &'static dyn UefiBootServices is used
  // throughout the API. For testing, each test will have a different set of expectations on the UefiBootServices mock
  // object, and the mock object itself expects to be "mut", which makes it hard to handle as a single global static.
  // Instead, raw pointers are used to simulate a MockUefiBootServices instance with 'static lifetime.
  // This object needs to outlive anything that uses it - once created, it will live until the end of the program.
  fn create_fake_static_boot_service() -> &'static mut MockUefiBootServices {
    unsafe { Box::into_raw(Box::new(MockUefiBootServices::new())).as_mut().unwrap() }
  }

  // Mock the HidIo FFI interface.
  fn mock_hid_io() -> hid_io::protocol::Protocol {
    extern "efiapi" fn mock_get_report_descriptor(
      this: *const hid_io::protocol::Protocol,
      report_descriptor_size: *mut usize,
      report_descriptor_buffer: *mut c_void,
    ) -> efi::Status {
      assert_ne!(this, core::ptr::null());
      unsafe {
        if *report_descriptor_size < MINIMAL_BOOT_KEYBOARD_REPORT_DESCRIPTOR.len() {
          *report_descriptor_size = MINIMAL_BOOT_KEYBOARD_REPORT_DESCRIPTOR.len();
          return efi::Status::BUFFER_TOO_SMALL;
        } else {
          *report_descriptor_size = MINIMAL_BOOT_KEYBOARD_REPORT_DESCRIPTOR.len();
          let slice = from_raw_parts_mut(report_descriptor_buffer as *mut u8, *report_descriptor_size);
          slice.copy_from_slice(MINIMAL_BOOT_KEYBOARD_REPORT_DESCRIPTOR);
          return efi::Status::SUCCESS;
        }
      }
    }

    extern "efiapi" fn mock_get_report(
      _this: *const hid_io::protocol::Protocol,
      _report_id: u8,
      _report_type: hid_io::protocol::HidReportType,
      _report_buffer_size: usize,
      _report_buffer: *mut c_void,
    ) -> efi::Status {
      panic!("This implementation does not use get_report.");
    }

    extern "efiapi" fn mock_set_report(
      this: *const hid_io::protocol::Protocol,
      report_id: u8,
      report_type: hid_io::protocol::HidReportType,
      report_buffer_size: usize,
      report_buffer: *mut c_void,
    ) -> efi::Status {
      assert_ne!(this, core::ptr::null());
      assert_eq!(report_type, hid_io::protocol::HidReportType::OutputReport);
      assert_ne!(report_buffer_size, 0);
      assert_ne!(report_buffer, core::ptr::null_mut());

      let report_slice = unsafe { from_raw_parts(report_buffer as *mut u8, report_buffer_size) };

      match report_id {
        0 => {
          assert_eq!(report_slice, TEST_REPORT0);
          efi::Status::SUCCESS
        }
        1 => {
          assert_eq!(report_slice, TEST_REPORT1);
          efi::Status::SUCCESS
        }
        _ => efi::Status::UNSUPPORTED,
      }
    }

    extern "efiapi" fn mock_register_report_callback(
      this: *const hid_io::protocol::Protocol,
      callback: hid_io::protocol::HidIoReportCallback,
      context: *mut c_void,
    ) -> efi::Status {
      assert_ne!(this, core::ptr::null());
      assert_ne!(context, core::ptr::null_mut());
      assert!(callback == UefiHidIo::report_callback);

      callback(TEST_REPORT0.len() as u16, TEST_REPORT0.as_ptr() as *mut c_void, context);

      efi::Status::SUCCESS
    }

    extern "efiapi" fn mock_unregister_report_callback(
      this: *const hid_io::protocol::Protocol,
      callback: hid_io::protocol::HidIoReportCallback,
    ) -> efi::Status {
      assert_ne!(this, core::ptr::null());
      assert!(callback == UefiHidIo::report_callback);
      efi::Status::SUCCESS
    }

    hid_io::protocol::Protocol {
      get_report_descriptor: mock_get_report_descriptor,
      get_report: mock_get_report,
      set_report: mock_set_report,
      register_report_callback: mock_register_report_callback,
      unregister_report_callback: mock_unregister_report_callback,
    }
  }

  #[test]
  fn new_should_instantiate_new_uefi_hid_io() {
    let boot_services = create_fake_static_boot_service();
    let controller: efi::Handle = 0x1234 as efi::Handle;
    let agent: efi::Handle = 0x4321 as efi::Handle;

    boot_services.expect_open_protocol().returning(|handle, protocol, interface, agent, controller, attributes| {
      assert_eq!(handle, 0x1234 as efi::Handle);
      assert_eq!(unsafe { *protocol }, hid_io::protocol::GUID);
      assert_ne!(interface, core::ptr::null_mut());
      assert_eq!(agent, 0x4321 as efi::Handle);
      assert_eq!(controller, 0x1234 as efi::Handle);
      assert_eq!(attributes, efi::OPEN_PROTOCOL_BY_DRIVER);

      //note: this leaks; but easier than trying to share it between the closure and the environment.
      let hid_io = Box::into_raw(Box::new(mock_hid_io()));
      unsafe { *interface = hid_io as *mut c_void };
      efi::Status::SUCCESS
    });

    boot_services.expect_close_protocol().returning(|handle, protocol, agent, controller| {
      assert_eq!(handle, 0x1234 as efi::Handle);
      assert_eq!(unsafe { *protocol }, hid_io::protocol::GUID);
      assert_eq!(agent, 0x4321 as efi::Handle);
      assert_eq!(controller, 0x1234 as efi::Handle);
      efi::Status::SUCCESS
    });

    let uefi_hid_io = UefiHidIo::new(boot_services, agent, controller, true).unwrap();
    drop(uefi_hid_io);
  }

  #[test]
  fn get_report_descriptor_should_return_report_descriptor() {
    let boot_services = create_fake_static_boot_service();
    let controller: efi::Handle = 0x1234 as efi::Handle;
    let agent: efi::Handle = 0x4321 as efi::Handle;

    boot_services.expect_open_protocol().returning(|_, _, interface, _, _, _| {
      let hid_io = mock_hid_io();
      //note: this leaks; but easier than trying to share it between the closure and the environment.
      unsafe { *interface = Box::into_raw(Box::new(hid_io)) as *mut c_void };
      efi::Status::SUCCESS
    });

    boot_services.expect_close_protocol().returning(|_, _, _, _| efi::Status::SUCCESS);

    let uefi_hid_io = UefiHidIo::new(boot_services, agent, controller, true).unwrap();
    let descriptor = uefi_hid_io.get_report_descriptor().unwrap();
    assert_eq!(descriptor, hidparser::parse_report_descriptor(&MINIMAL_BOOT_KEYBOARD_REPORT_DESCRIPTOR).unwrap());
    drop(uefi_hid_io);
  }
  #[test]
  fn set_report_should_set_report() {
    let boot_services = create_fake_static_boot_service();
    let controller: efi::Handle = 0x1234 as efi::Handle;
    let agent: efi::Handle = 0x4321 as efi::Handle;

    boot_services.expect_open_protocol().returning(|_, _, interface, _, _, _| {
      let hid_io = mock_hid_io();
      unsafe { *interface = Box::into_raw(Box::new(hid_io)) as *mut c_void };
      efi::Status::SUCCESS
    });

    boot_services.expect_close_protocol().returning(|_, _, _, _| efi::Status::SUCCESS);

    let uefi_hid_io = UefiHidIo::new(boot_services, agent, controller, true).unwrap();

    uefi_hid_io.set_output_report(None, &TEST_REPORT0).unwrap();
    uefi_hid_io.set_output_report(Some(1), &TEST_REPORT1).unwrap();
    assert_eq!(uefi_hid_io.set_output_report(Some(2), &TEST_REPORT0), Err(efi::Status::UNSUPPORTED));

    drop(uefi_hid_io);
  }

  #[test]
  fn set_receiver_should_install_receiver() {
    let boot_services = create_fake_static_boot_service();
    let controller: efi::Handle = 0x1234 as efi::Handle;
    let agent: efi::Handle = 0x4321 as efi::Handle;

    boot_services.expect_open_protocol().returning(|_, _, interface, _, _, _| {
      let hid_io = mock_hid_io();
      unsafe { *interface = Box::into_raw(Box::new(hid_io)) as *mut c_void };
      efi::Status::SUCCESS
    });

    boot_services.expect_close_protocol().returning(|_, _, _, _| efi::Status::SUCCESS);

    let mut uefi_hid_io = UefiHidIo::new(boot_services, agent, controller, true).unwrap();

    let mut mock_receiver = MockHidReportReceiver::new();
    mock_receiver
      .expect_receive_report()
      .withf(|report, _| {
        assert_eq!(report, TEST_REPORT0);
        true
      })
      .returning(|_, _| ());

    uefi_hid_io.set_report_receiver(Box::new(mock_receiver)).unwrap();

    drop(uefi_hid_io);
  }
}
