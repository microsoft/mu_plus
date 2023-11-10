//! Provides a trait interface and FFI support for the UEFI Driver Binding used
//! in this crate.
//!
//! ## License
//!
//! Copyright (C) Microsoft Corporation. All rights reserved.
//!
//! SPDX-License-Identifier: BSD-2-Clause-Patent
//!
use alloc::boxed::Box;
use core::ffi::c_void;

#[cfg(test)]
use mockall::automock;
use r_efi::{efi, protocols};

use crate::boot_services::UefiBootServices;

/// Abstracts the UEFI driver binding interface.
///
/// Clients of this module can implement this interface and pass it to an
/// instance of [`UefiDriverBinding`] which will manage FFI for the driver
/// binding with the UEFI core.
///
/// Reference: <https://uefi.org/specs/UEFI/2.10/11_Protocols_UEFI_Driver_Model.html#protocols-uefi-driver-model>
#[cfg_attr(test, automock)]
pub trait DriverBinding {
  /// Reference: <https://uefi.org/specs/UEFI/2.10/11_Protocols_UEFI_Driver_Model.html#efi-driver-binding-protocol-supported>
  fn driver_binding_supported(
    &mut self,
    boot_services: &'static dyn UefiBootServices,
    controller: efi::Handle,
  ) -> Result<(), efi::Status>;
  /// Reference: <https://uefi.org/specs/UEFI/2.10/11_Protocols_UEFI_Driver_Model.html#efi-driver-binding-protocol-start>
  fn driver_binding_start(
    &mut self,
    boot_services: &'static dyn UefiBootServices,
    controller: efi::Handle,
  ) -> Result<(), efi::Status>;
  /// Reference: <https://uefi.org/specs/UEFI/2.10/11_Protocols_UEFI_Driver_Model.html#efi-driver-binding-protocol-stop>
  fn driver_binding_stop(
    &mut self,
    boot_services: &'static dyn UefiBootServices,
    controller: efi::Handle,
  ) -> Result<(), efi::Status>;
}

/// Manages FFI for a driver binding instance with UEFI core.
///
/// ## Examples and Usage
///
/// ```ignore
/// use uefi_hid_dxe::driver_binding::*;
///
/// let driver_binding = Box::new(MyDriverBinding::new());
/// let uefi_driver_binding = UefiDriverBinding::new(
///   boot_services,
///   driver_binding,
///   driver_handle
/// );
/// let raw_binding = uefi_driver_binding.install().unwrap();
/// // driver_binding can now be invoked by UEFI core.
/// let driver_binding = uninstall(raw_binding).unwrap();
/// // driver_binding is now uninstalled from core and can be safely dropped.
///```
#[repr(C)]
pub struct UefiDriverBinding {
  uefi_binding: protocols::driver_binding::Protocol,
  boot_services: &'static dyn UefiBootServices,
  binding: Box<dyn DriverBinding>,
}

impl UefiDriverBinding {
  /// Creates a new UefiDriverBinding that manages the given binding.
  pub fn new(
    boot_services: &'static dyn UefiBootServices,
    binding: Box<dyn DriverBinding>,
    handle: efi::Handle,
  ) -> Self {
    let uefi_binding = protocols::driver_binding::Protocol {
      supported: Self::driver_binding_supported,
      start: Self::driver_binding_start,
      stop: Self::driver_binding_stop,
      version: 1,
      image_handle: handle,
      driver_binding_handle: handle,
    };
    Self { uefi_binding, boot_services, binding }
  }

  /// Installs the binding with the UEFI core.
  pub fn install(self) -> Result<*mut UefiDriverBinding, efi::Status> {
    let mut handle = self.uefi_binding.driver_binding_handle;
    let uefi_driver_binding_mgr_ptr = Box::into_raw(Box::new(self));
    let boot_services = &unsafe { uefi_driver_binding_mgr_ptr.as_ref().unwrap() }.boot_services;
    let status = boot_services.install_protocol_interface(
      core::ptr::addr_of_mut!(handle),
      &protocols::driver_binding::PROTOCOL_GUID as *const efi::Guid as *mut efi::Guid,
      efi::NATIVE_INTERFACE,
      uefi_driver_binding_mgr_ptr as *mut c_void,
    );
    if status.is_error() {
      Err(status)
    } else {
      Ok(uefi_driver_binding_mgr_ptr)
    }
  }

  /// Uninstalls the binding from the UEFI core.
  /// # Safety
  /// uefi_binding must be the same pointer returned from [`Self::install`].
  pub unsafe fn uninstall(uefi_binding: *mut UefiDriverBinding) -> Result<Self, efi::Status> {
    let ptr = uefi_binding;
    let binding = Box::from_raw(uefi_binding);
    let status = binding.boot_services.uninstall_protocol_interface(
      binding.uefi_binding.driver_binding_handle,
      &protocols::driver_binding::PROTOCOL_GUID as *const efi::Guid as *mut efi::Guid,
      ptr as *mut c_void,
    );
    if status.is_error() {
      Err(status)
    } else {
      Ok(*binding)
    }
  }

  // Driver Binding Supported FFI function
  // Handles driver_binding_supported calls from the UEFI core by passing them to the trait implementation.
  extern "efiapi" fn driver_binding_supported(
    this: *mut protocols::driver_binding::Protocol,
    controller: efi::Handle,
    _remaining_device_path: *mut protocols::device_path::Protocol,
  ) -> efi::Status {
    let uefi_binding = unsafe { (this as *mut UefiDriverBinding).as_mut() }.expect("bad this pointer");
    match uefi_binding.binding.driver_binding_supported(uefi_binding.boot_services, controller) {
      Ok(_) => efi::Status::SUCCESS,
      Err(err) => err,
    }
  }

  // Driver Binding Start FFI function
  // Handles driver_binding_start calls from the UEFI core by passing them to the trait implementation.
  extern "efiapi" fn driver_binding_start(
    this: *mut protocols::driver_binding::Protocol,
    controller: efi::Handle,
    _remaining_device_path: *mut protocols::device_path::Protocol,
  ) -> efi::Status {
    let uefi_binding = unsafe { (this as *mut UefiDriverBinding).as_mut() }.expect("bad this pointer");
    match uefi_binding.binding.driver_binding_start(uefi_binding.boot_services, controller) {
      Ok(_) => efi::Status::SUCCESS,
      Err(err) => err,
    }
  }

  // Driver Binding Stop FFI function
  // Handles driver_binding_stop calls from the UEFI core by passing them to the trait implementation.
  extern "efiapi" fn driver_binding_stop(
    this: *mut protocols::driver_binding::Protocol,
    controller: efi::Handle,
    _num_children: usize,
    _child_handle_buffer: *mut efi::Handle,
  ) -> efi::Status {
    let uefi_binding = unsafe { (this as *mut UefiDriverBinding).as_mut() }.expect("bad this pointer");
    match uefi_binding.binding.driver_binding_stop(uefi_binding.boot_services, controller) {
      Ok(_) => efi::Status::SUCCESS,
      Err(err) => err,
    }
  }
}

#[cfg(test)]
mod test {

  use super::{MockDriverBinding, UefiDriverBinding};
  use crate::boot_services::MockUefiBootServices;
  use r_efi::{efi, protocols};

  // In this module, the usage model for boot_services is global static, and so &'static dyn UefiBootServices is used
  // throughout the API. For testing, each test will have a different set of expectations on the UefiBootServices mock
  // object, and the mock object itself expects to be "mut", which makes it hard to handle as a single global static.
  // Instead, raw pointers are used to simulate a MockUefiBootServices instance with 'static lifetime.
  // This object needs to outlive anything that uses it - once created, it will live until the end of the program.
  fn create_fake_static_boot_service() -> &'static mut MockUefiBootServices {
    unsafe { Box::into_raw(Box::new(MockUefiBootServices::new())).as_mut().unwrap() }
  }

  #[test]
  fn new_should_instantiate_new_uefi_driver_binding() {
    let boot_services = create_fake_static_boot_service();
    let binding = MockDriverBinding::new();
    let handle = 0x1234 as efi::Handle;
    let driver_binding = UefiDriverBinding::new(boot_services, Box::new(binding), handle);

    assert!(driver_binding.uefi_binding.supported == UefiDriverBinding::driver_binding_supported);
    assert!(driver_binding.uefi_binding.start == UefiDriverBinding::driver_binding_start);
    assert!(driver_binding.uefi_binding.stop == UefiDriverBinding::driver_binding_stop);
    assert_eq!(driver_binding.uefi_binding.version, 1);
    assert_eq!(driver_binding.uefi_binding.image_handle, handle);
    assert_eq!(driver_binding.uefi_binding.driver_binding_handle, handle);
  }

  #[test]
  fn install_should_install_the_driver_binding() {
    let boot_services = create_fake_static_boot_service();
    //expect a call to install_protocol_interface
    boot_services
      .expect_install_protocol_interface()
      .withf(|handle, protocol, interface_type, interface| {
        assert_ne!(*handle, core::ptr::null_mut());
        assert_eq!(unsafe { **protocol }, protocols::driver_binding::PROTOCOL_GUID);
        assert_eq!(*interface_type, efi::NATIVE_INTERFACE);
        assert_ne!(*interface, core::ptr::null_mut());
        true
      })
      .returning(|_, _, _, _| efi::Status::SUCCESS);

    let handle = 0x1234 as efi::Handle;

    let binding = MockDriverBinding::new();
    let driver_binding = UefiDriverBinding::new(boot_services, Box::new(binding), handle);
    driver_binding.install().unwrap();
  }

  #[test]
  fn install_should_report_failures_to_install_the_driver_binding() {
    let boot_services = create_fake_static_boot_service();

    //expect a call to install_protocol_interface
    boot_services.expect_install_protocol_interface().returning(|_, _, _, _| efi::Status::OUT_OF_RESOURCES);

    let handle = 0x1234 as efi::Handle;

    let binding = MockDriverBinding::new();
    let driver_binding = UefiDriverBinding::new(boot_services, Box::new(binding), handle);
    assert_eq!(driver_binding.install(), Err(efi::Status::OUT_OF_RESOURCES));
  }

  #[test]
  fn uninstall_should_uninstall_the_driver_binding() {
    let boot_services = create_fake_static_boot_service();
    //expect a call to install_protocol_interface
    boot_services.expect_install_protocol_interface().returning(|handle, protocol, interface_type, interface| {
      unsafe {
        assert_ne!(handle.read(), core::ptr::null_mut());
        assert_eq!(protocol.read(), protocols::driver_binding::PROTOCOL_GUID);
        assert_eq!(interface_type, efi::NATIVE_INTERFACE);
        assert_ne!(interface, core::ptr::null_mut());
      }
      efi::Status::SUCCESS
    });
    boot_services.expect_uninstall_protocol_interface().returning(|handle, protocol, interface| {
      assert_ne!(handle, core::ptr::null_mut());
      assert_eq!(unsafe { protocol.read() }, protocols::driver_binding::PROTOCOL_GUID);
      assert_ne!(interface, core::ptr::null_mut());
      efi::Status::SUCCESS
    });
    let handle = 0x1234 as efi::Handle;

    let binding = MockDriverBinding::new();
    let driver_binding = UefiDriverBinding::new(boot_services, Box::new(binding), handle);
    let binding_ptr = driver_binding.install().unwrap();

    let driver_binding = unsafe { UefiDriverBinding::uninstall(binding_ptr) }.unwrap();

    assert!(driver_binding.uefi_binding.supported == UefiDriverBinding::driver_binding_supported);
    assert!(driver_binding.uefi_binding.start == UefiDriverBinding::driver_binding_start);
    assert!(driver_binding.uefi_binding.stop == UefiDriverBinding::driver_binding_stop);
    assert_eq!(driver_binding.uefi_binding.version, 1);
    assert_eq!(driver_binding.uefi_binding.image_handle, handle);
    assert_eq!(driver_binding.uefi_binding.driver_binding_handle, handle);
  }

  #[test]
  fn uninstall_should_report_failures_to_uninstall_the_driver() {
    let boot_services = create_fake_static_boot_service();

    //expect a call to install_protocol_interface
    boot_services.expect_install_protocol_interface().returning(|_, _, _, _| efi::Status::SUCCESS);
    boot_services.expect_uninstall_protocol_interface().returning(|_, _, _| efi::Status::INVALID_PARAMETER);

    let handle = 0x1234 as efi::Handle;

    let binding = MockDriverBinding::new();
    let driver_binding = UefiDriverBinding::new(boot_services, Box::new(binding), handle);
    let binding_ptr = driver_binding.install().unwrap();

    assert_eq!(unsafe { UefiDriverBinding::uninstall(binding_ptr) }.err(), Some(efi::Status::INVALID_PARAMETER));
  }

  #[test]
  fn driver_binding_should_call_driver_binding_routines() {
    let boot_services = create_fake_static_boot_service();
    //expect a call to install_protocol_interface
    boot_services.expect_install_protocol_interface().returning(|_, _, _, _| efi::Status::SUCCESS);

    let mut binding = MockDriverBinding::new();
    binding.expect_driver_binding_supported().returning(|_, _| Ok(()));
    binding.expect_driver_binding_start().returning(|_, _| Ok(()));
    binding.expect_driver_binding_stop().returning(|_, _| Ok(()));

    let handle = 0x1234 as efi::Handle;
    let driver_binding = UefiDriverBinding::new(boot_services, Box::new(binding), handle);
    let binding_ptr = driver_binding.install().unwrap();

    let driver_binding_ref = unsafe { binding_ptr.as_ref().unwrap() };
    let this_ptr = binding_ptr as *mut protocols::driver_binding::Protocol;

    let controller_handle = 0x4321 as efi::Handle;
    assert_eq!(
      (driver_binding_ref.uefi_binding.supported)(this_ptr, controller_handle, core::ptr::null_mut()),
      efi::Status::SUCCESS
    );
    assert_eq!(
      (driver_binding_ref.uefi_binding.start)(this_ptr, controller_handle, core::ptr::null_mut()),
      efi::Status::SUCCESS
    );
    assert_eq!(
      (driver_binding_ref.uefi_binding.stop)(this_ptr, controller_handle, 0, core::ptr::null_mut()),
      efi::Status::SUCCESS
    );
  }
}
