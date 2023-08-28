//! Hello World Rust DXE Driver
//!
//! Demonstrates how to build a DXE driver written in Rust.
//!
//! ## License
//!
//! Copyright (c) Microsoft Corporation. All rights reserved.
//!
//! SPDX-License-Identifier: BSD-2-Clause-Patent
//!
#![no_std]
#![no_main]
#![allow(non_snake_case)]

extern crate alloc;

use alloc::vec;
use core::panic::PanicInfo;
use r_efi::efi::Status;
use rust_advanced_logger_dxe::{debugln, init_debug, DEBUG_INFO};

#[no_mangle]
pub extern "efiapi" fn efi_main(
  _image_handle: *const core::ffi::c_void,
  _system_table: *const r_efi::system::SystemTable,
) -> u64 {
  rust_boot_services_allocator_dxe::GLOBAL_ALLOCATOR.init(unsafe { (*_system_table).boot_services });
  init_debug(unsafe { (*_system_table).boot_services });

  debugln!(DEBUG_INFO, "Hello, World. This is Rust in UEFI.");

  debugln!(DEBUG_INFO, "file: {:} line: {:} as hex: {:x}", file!(), line!(), line!());

  let mut foo = vec!["asdf", "xyzpdq", "abcdefg", "thxyzb"];

  debugln!(DEBUG_INFO, "Unsorted vec: {:?}", foo);
  foo.sort();
  debugln!(DEBUG_INFO, "Sorted vec: {:?}", foo);

  Status::SUCCESS.as_usize() as u64
}

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
  loop {}
}
