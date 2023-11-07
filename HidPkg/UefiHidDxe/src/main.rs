//! HID input driver for UEFI
//!
//! This crate provides input handlers for HID 1.1 compliant keyboards and pointers.
//!
//! ## License
//!
//! Copyright (c) Microsoft Corporation. All rights reserved.
//! SPDX-License-Identifier: BSD-2-Clause-Patent
//!

#![no_std]
#![no_main]
#![allow(non_snake_case)]

extern crate alloc;

use core::panic::PanicInfo;

use driver_binding::initialize_driver_binding;
use r_efi::{efi, system};

use rust_advanced_logger_dxe::{debugln, init_debug, DEBUG_ERROR};
use rust_boot_services_allocator_dxe::GLOBAL_ALLOCATOR;

mod driver_binding;
mod hid;
mod key_queue;
mod keyboard;
mod pointer;

static mut BOOT_SERVICES: *mut system::BootServices = core::ptr::null_mut();
static mut RUNTIME_SERVICES: *mut system::RuntimeServices = core::ptr::null_mut();

#[no_mangle]
pub extern "efiapi" fn efi_main(image_handle: efi::Handle, system_table: *const system::SystemTable) -> efi::Status {
  // Safety: This block is unsafe because it assumes that system_table and (*system_table).boot_services are correct,
  // and because it mutates/accesses the global BOOT_SERVICES static.
  unsafe {
    BOOT_SERVICES = (*system_table).boot_services;
    RUNTIME_SERVICES = (*system_table).runtime_services;
    GLOBAL_ALLOCATOR.init(BOOT_SERVICES);
    init_debug(BOOT_SERVICES);
  }

  let status = initialize_driver_binding(image_handle);

  if status.is_err() {
    debugln!(DEBUG_ERROR, "[UefiHidMain]: failed to initialize driver binding.\n");
  }

  efi::Status::SUCCESS
}

//Workaround for https://github.com/rust-lang/rust/issues/98254
#[rustversion::before(1.73)]
#[no_mangle]
pub extern "efiapi" fn __chkstk() {}

#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
  debugln!(DEBUG_ERROR, "Panic: {:?}", info);
  loop {}
}
