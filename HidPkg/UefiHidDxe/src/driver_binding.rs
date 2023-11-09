//! Driver binding support for HID input driver.
//!
//! This module manages the UEFI Driver Binding for the HID input driver.
//!
//! ## License
//!
//! Copyright (c) Microsoft Corporation. All rights reserved.
//! SPDX-License-Identifier: BSD-2-Clause-Patent
//!

use core::ffi::c_void;

use alloc::boxed::Box;
use r_efi::{
  efi,
  protocols::{device_path, driver_binding},
};
use rust_advanced_logger_dxe::{debugln, DEBUG_ERROR, DEBUG_INFO};

use crate::{hid, BOOT_SERVICES};

/// Initialize and install driver binding for this driver.
///
/// Installs this drivers driver binding on the given image handle.
pub fn initialize_driver_binding(image_handle: efi::Handle) -> Result<(), efi::Status> {
  let boot_services = unsafe { BOOT_SERVICES.as_mut().ok_or(efi::Status::NOT_READY)? };

  let mut driver_binding_handle = image_handle;
  let driver_binding_ptr = Box::into_raw(Box::new(driver_binding::Protocol {
    supported: uefi_hid_driver_binding_supported,
    start: uefi_hid_driver_binding_start,
    stop: uefi_hid_driver_binding_stop,
    version: 1,
    image_handle: driver_binding_handle,
    driver_binding_handle,
  }));

  let status = (boot_services.install_protocol_interface)(
    core::ptr::addr_of_mut!(driver_binding_handle),
    &driver_binding::PROTOCOL_GUID as *const efi::Guid as *mut efi::Guid,
    efi::NATIVE_INTERFACE,
    driver_binding_ptr as *mut c_void,
  );

  if status.is_error() {
    drop(unsafe { Box::from_raw(driver_binding_ptr) });
    return Err(status);
  }

  Ok(())
}

// Checks whether the given controller supports HidIo. Implements driver_binding::supported.
extern "efiapi" fn uefi_hid_driver_binding_supported(
  this: *mut driver_binding::Protocol,
  controller: efi::Handle,
  _remaining_device_path: *mut device_path::Protocol,
) -> efi::Status {
  // retrieve a reference to boot services.
  // Safety: BOOT_SERVICES must have been initialized to point to the UEFI Boot Services table.
  let boot_services = unsafe {
    match BOOT_SERVICES.as_mut() {
      Some(boot_services) => boot_services,
      None => return efi::Status::NOT_READY,
    }
  };

  // retrieve a reference to the driver binding.
  // Safety: the driver binding pointer passed here must point to the binding installed in initialize_driver_binding.
  let driver_binding = unsafe {
    match this.as_mut() {
      Some(driver_binding) => driver_binding,
      None => return efi::Status::INVALID_PARAMETER,
    }
  };

  // Check to see if this controller is supported by attempting to open HidIo on it.
  let mut hid_io_ptr: *mut hid_io::protocol::Protocol = core::ptr::null_mut();
  let status = (boot_services.open_protocol)(
    controller,
    &hid_io::protocol::GUID as *const efi::Guid as *mut efi::Guid,
    core::ptr::addr_of_mut!(hid_io_ptr) as *mut *mut c_void,
    driver_binding.driver_binding_handle,
    controller,
    efi::OPEN_PROTOCOL_BY_DRIVER,
  );

  // if HidIo could not be opened then it is either in use or not present.
  if status.is_error() {
    return status;
  }

  // HidIo is available, so this controller is supported. Further checking that requires actual device interaction is
  // done in uefi_hid_driver_binding_start. close the protocol used for the supported test and exit with success.
  let status = (boot_services.close_protocol)(
    controller,
    &hid_io::protocol::GUID as *const efi::Guid as *mut efi::Guid,
    driver_binding.driver_binding_handle,
    controller,
  );
  if status.is_error() {
    debugln!(DEBUG_ERROR, "Unexpected error from CloseProtocol: {:?}", status); //message, but no further action to handle.
  }

  efi::Status::SUCCESS
}

// Start this driver managing given handle. Implements driver_binding::start.
extern "efiapi" fn uefi_hid_driver_binding_start(
  this: *mut driver_binding::Protocol,
  controller: efi::Handle,
  _remaining_device_path: *mut device_path::Protocol,
) -> efi::Status {
  // retrieve a reference to the driver binding.
  // Safety: the driver binding pointer passed here must point to the binding installed in initialize_driver_binding.
  let driver_binding = unsafe {
    match this.as_mut() {
      Some(driver_binding) => driver_binding,
      None => return efi::Status::INVALID_PARAMETER,
    }
  };

  // initialize the hid stack
  let status = hid::initialize(controller, driver_binding);
  if let Err(status) = status {
    debugln!(DEBUG_INFO, "[hid::driver_binding_start] failed to initialize hid: {:x?}", status);
    return status;
  }

  efi::Status::SUCCESS
}

// Stops this driver from managing the given handle. Implements driver_binding::stop.
extern "efiapi" fn uefi_hid_driver_binding_stop(
  this: *mut driver_binding::Protocol,
  controller: efi::Handle,
  _num_children: usize,
  _child_handle_buffer: *mut efi::Handle,
) -> efi::Status {
  let driver_binding = unsafe { this.as_mut().expect("driver binding pointer is bad in uefi_hid_driver_binding_stop") };

  //destroy the hid stack.
  let status = hid::destroy(controller, driver_binding);
  if let Err(status) = status {
    debugln!(DEBUG_INFO, "[hid::driver_binding_stop] failed to destroy hid: {:x?}", status);
    return status;
  }

  efi::Status::SUCCESS
}
