//! HID I/O support for HID input driver.
//!
//! This module manages interactions with the lower-layer drivers that produce the HidIo protocol.
//!
//! ## License
//!
//! Copyright (c) Microsoft Corporation. All rights reserved.
//! SPDX-License-Identifier: BSD-2-Clause-Patent
//!
use core::{ffi::c_void, slice::from_raw_parts};

use alloc::{boxed::Box, vec};

use r_efi::{efi, protocols::driver_binding, system};
use rust_advanced_logger_dxe::{debugln, DEBUG_ERROR, DEBUG_WARN};

use crate::{keyboard, keyboard::KeyboardContext, pointer, pointer::PointerContext, BOOT_SERVICES};

pub struct HidContext {
  pub hid_io: *mut hid_io::protocol::Protocol,
  pub keyboard_context: *mut KeyboardContext,
  pub pointer_context: *mut PointerContext,
}

/// Initialize HID support
///
/// Reads the HID Report Descriptor from the device, parse it, and initialize keyboard and pointer handlers for it.
pub fn initialize(controller: efi::Handle, driver_binding: &driver_binding::Protocol) -> Result<(), efi::Status> {
  // retrieve a reference to boot services.
  // Safety: BOOT_SERVICES must have been initialized to point to the UEFI Boot Services table.
  // Caller should have ensured this, so just expect on failure.
  let boot_services = unsafe { BOOT_SERVICES.as_mut().expect("BOOT_SERVICES not properly initialized") };

  // retrieve the HidIo instance for the given controller.
  let mut hid_io_ptr: *mut hid_io::protocol::Protocol = core::ptr::null_mut();
  let status = (boot_services.open_protocol)(
    controller,
    &hid_io::protocol::GUID as *const efi::Guid as *mut efi::Guid,
    core::ptr::addr_of_mut!(hid_io_ptr) as *mut *mut c_void,
    driver_binding.driver_binding_handle,
    controller,
    system::OPEN_PROTOCOL_BY_DRIVER,
  );
  if status.is_error() {
    debugln!(DEBUG_ERROR, "[hid::initialize] Unexpected error opening HidIo protocol: {:#?}", status);
    return Err(status);
  }

  let hid_io = unsafe { hid_io_ptr.as_mut().expect("bad hidio pointer.") };

  //determine report descriptor size.
  let mut report_descriptor_size: usize = 0;
  let status =
    (hid_io.get_report_descriptor)(hid_io_ptr, core::ptr::addr_of_mut!(report_descriptor_size), core::ptr::null_mut());

  match status {
    efi::Status::BUFFER_TOO_SMALL => (),
    _ => {
      let _ = release_hid_io(controller, driver_binding);
      return Err(efi::Status::DEVICE_ERROR);
    }
  }

  //read report descriptor.
  let mut report_descriptor_buffer = vec![0u8; report_descriptor_size];
  let report_descriptor_buffer_ptr = report_descriptor_buffer.as_mut_ptr();

  let status = (hid_io.get_report_descriptor)(
    hid_io_ptr,
    core::ptr::addr_of_mut!(report_descriptor_size),
    report_descriptor_buffer_ptr as *mut c_void,
  );

  if status.is_error() {
    let _ = release_hid_io(controller, driver_binding);
    return Err(status);
  }

  // parse the descriptor
  let descriptor = hidparser::parse_report_descriptor(&report_descriptor_buffer).map_err(|err| {
    debugln!(DEBUG_WARN, "[hid::initialize] failed to parse report descriptor: {:x?}.", err);
    let _ = release_hid_io(controller, driver_binding);
    efi::Status::DEVICE_ERROR
  })?;

  // create hid context
  let hid_context_ptr = Box::into_raw(Box::new(HidContext {
    hid_io: hid_io_ptr,
    keyboard_context: core::ptr::null_mut(),
    pointer_context: core::ptr::null_mut(),
  }));

  //initialize report handlers
  let keyboard = keyboard::initialize(controller, &descriptor, hid_context_ptr);
  let pointer = pointer::initialize(controller, &descriptor, hid_context_ptr);

  if keyboard.is_err() && pointer.is_err() {
    debugln!(DEBUG_WARN, "[hid::initialize] no devices supported");
    //no devices supported.
    let _ = release_hid_io(controller, driver_binding);
    unsafe { drop(Box::from_raw(hid_context_ptr)) };
    Err(efi::Status::UNSUPPORTED)?;
  }

  // register for input reports
  let status = (hid_io.register_report_callback)(hid_io_ptr, on_input_report, hid_context_ptr as *mut c_void);

  if status.is_error() {
    debugln!(DEBUG_WARN, "[hid::initialize] failed to register for input reports: {:x?}", status);
    let _ = destroy(controller, driver_binding);
    return Err(status);
  }

  Ok(())
}

// Handler function for input reports. Dispatches them to the keyboard and pointer modules for handling.
extern "efiapi" fn on_input_report(report_buffer_size: u16, report_buffer: *mut c_void, context: *mut c_void) {
  let hid_context_ptr = context as *mut HidContext;
  let hid_context = unsafe { hid_context_ptr.as_mut().expect("[hid::on_input_report: invalid context pointer") };

  let report = unsafe { from_raw_parts(report_buffer as *mut u8, report_buffer_size as usize) };

  let keyboard_context = unsafe { hid_context.keyboard_context.as_mut() };
  if let Some(keyboard_context) = keyboard_context {
    keyboard_context.handler.process_input_report(report);
  }

  let pointer_context = unsafe { hid_context.pointer_context.as_mut() };
  if let Some(pointer_context) = pointer_context {
    pointer_context.handler.process_input_report(report);
  }
}

// Unregister report callback from HID layer to shutdown input reports.
fn shutdown_input_reports(hid_context: &mut HidContext) -> Result<(), efi::Status> {
  let hid_io =
    unsafe { hid_context.hid_io.as_mut().expect("hid_context has bad hid_io pointer in hid::shutdown_input_reports") };
  // shutdown input reports.
  let status = (hid_io.unregister_report_callback)(hid_context.hid_io, on_input_report);
  if status.is_error() {
    debugln!(DEBUG_ERROR, "[hid::destroy] unexpected error from hid_io.unregister_report_callback: {:?}", status);
    return Err(status);
  }
  Ok(())
}

//Shutdown Keyboard and Pointer handling.
fn shutdown_handlers(hid_context: &mut HidContext) -> Result<(), efi::Status> {
  // shutdown keyboard.
  let mut status = efi::Status::SUCCESS;
  match keyboard::deinitialize(hid_context.keyboard_context) {
    Err(err) if err != efi::Status::UNSUPPORTED => {
      debugln!(DEBUG_ERROR, "[hid::destroy] unexpected error from keyboard::deinitialize: {:?}", err);
      status = efi::Status::DEVICE_ERROR;
    }
    _ => (),
  };

  // shutdown pointer.
  match pointer::deinitialize(hid_context.pointer_context) {
    Err(err) if err != efi::Status::UNSUPPORTED => {
      debugln!(DEBUG_ERROR, "[hid::destroy] unexpected error from pointer::deinitialize: {:?}", err);
      status = efi::Status::DEVICE_ERROR;
    }
    _ => (),
  };

  if status.is_error() {
    return Err(status);
  }

  Ok(())
}

// Release HidIo instance by closing the HidIo protocol on the given controller.
fn release_hid_io(controller: efi::Handle, driver_binding: &driver_binding::Protocol) -> Result<(), efi::Status> {
  // retrieve a reference to boot services.
  // Safety: BOOT_SERVICES must have been initialized to point to the UEFI Boot Services table.
  // Caller should have ensured this, so just expect on failure.
  let boot_services = unsafe { BOOT_SERVICES.as_mut().expect("BOOT_SERVICES not properly initialized") };

  // release HidIo
  match (boot_services.close_protocol)(
    controller,
    &hid_io::protocol::GUID as *const efi::Guid as *mut efi::Guid,
    driver_binding.driver_binding_handle,
    controller,
  ) {
    efi::Status::SUCCESS => (),
    err => {
      debugln!(DEBUG_ERROR, "[hid::release_hid_io] unexpected error from boot_services.close_protocol: {:?}", err);
      return Err(efi::Status::DEVICE_ERROR);
    }
  }

  Ok(())
}

/// Tears down HID support.
///
/// De-initializes keyboard and pointer handlers and releases HidIo instance.
pub fn destroy(controller: efi::Handle, driver_binding: &driver_binding::Protocol) -> Result<(), efi::Status> {
  let mut context = pointer::attempt_to_retrieve_hid_context(controller, driver_binding);
  if context.is_err() {
    context = keyboard::attempt_to_retrieve_hid_context(controller, driver_binding);
  }

  let hid_context_ptr = context?;
  let hid_context = unsafe { hid_context_ptr.as_mut().expect("invalid hid_context_ptr in hid::destroy") };

  let _ = shutdown_input_reports(hid_context);
  let _ = shutdown_handlers(hid_context);
  let _ = release_hid_io(controller, driver_binding);

  // take back the hid_context (it will be released when it goes out of scope).
  unsafe { drop(Box::from_raw(hid_context_ptr)) };

  Ok(())
}
