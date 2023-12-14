//! HID input driver for UEFI
//!
//! This crate provides input handlers for HID 1.1 compliant keyboards and pointers.
//!
//! ## License
//!
//! Copyright (c) Microsoft Corporation. All rights reserved.
//! SPDX-License-Identifier: BSD-2-Clause-Patent
//!

#![cfg_attr(target_os = "uefi", no_std)]
#![cfg_attr(target_os = "uefi", no_main)]
#![allow(non_snake_case)]

#[cfg(target_os = "uefi")]
mod uefi_entry {
  extern crate alloc;
  use core::{panic::PanicInfo, sync::atomic::Ordering};

  use alloc::{boxed::Box, vec::Vec};

  use r_efi::{efi, system};

  use rust_advanced_logger_dxe::{debugln, init_debug, DEBUG_ERROR};
  use rust_boot_services_allocator_dxe::GLOBAL_ALLOCATOR;
  use uefi_hid_dxe_v2::{
    boot_services::UefiBootServices,
    driver_binding::UefiDriverBinding,
    hid::{HidFactory, HidReceiverFactory},
    hid_io::{HidReportReceiver, UefiHidIoFactory},
    keyboard::KeyboardHidHandler,
    pointer::PointerHidHandler,
    BOOT_SERVICES, RUNTIME_SERVICES,
  };

  struct UefiReceivers {
    boot_services: &'static dyn UefiBootServices,
    agent: efi::Handle,
  }
  impl HidReceiverFactory for UefiReceivers {
    fn new_hid_receiver_list(&self, _controller: efi::Handle) -> Result<Vec<Box<dyn HidReportReceiver>>, efi::Status> {
      let mut receivers: Vec<Box<dyn HidReportReceiver>> = Vec::new();
      receivers.push(Box::new(PointerHidHandler::new(self.boot_services, self.agent)));
      receivers.push(Box::new(KeyboardHidHandler::new(self.boot_services, self.agent)));
      Ok(receivers)
    }
  }

  #[no_mangle]
  pub extern "efiapi" fn efi_main(image_handle: efi::Handle, system_table: *const system::SystemTable) -> efi::Status {
    // Safety: This block is unsafe because it assumes that system_table and (*system_table).boot_services are correct,
    // and because it mutates/accesses the global BOOT_SERVICES static.
    unsafe {
      BOOT_SERVICES.initialize((*system_table).boot_services);
      RUNTIME_SERVICES.store((*system_table).runtime_services, Ordering::SeqCst);
      GLOBAL_ALLOCATOR.init((*system_table).boot_services);
      init_debug((*system_table).boot_services);
    }

    let hid_io_factory = Box::new(UefiHidIoFactory::new(&BOOT_SERVICES, image_handle));
    let receiver_factory = Box::new(UefiReceivers { boot_services: &BOOT_SERVICES, agent: image_handle });
    let hid_factory = Box::new(HidFactory::new(hid_io_factory, receiver_factory, image_handle));

    let hid_binding = UefiDriverBinding::new(&BOOT_SERVICES, hid_factory, image_handle);
    hid_binding.install().expect("failed to install HID driver binding");

    efi::Status::SUCCESS
  }

  #[panic_handler]
  fn panic(info: &PanicInfo) -> ! {
    debugln!(DEBUG_ERROR, "Panic: {:?}", info);
    loop {}
  }
}

#[cfg(not(target_os = "uefi"))]
fn main() {
  //do nothing.
}

#[cfg(test)]
mod test {
  use crate::main;

  #[test]
  fn main_should_do_nothing() {
    main();
  }
}
