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

use patina::boot_services::StandardBootServices;

/// Global instance of UEFI Boot Services.
pub static BOOT_SERVICES: StandardBootServices = StandardBootServices::new_uninit();

#[cfg(target_os = "uefi")]
mod uefi_entry {
    use crate::BOOT_SERVICES;
    use core::panic::PanicInfo;
    use patina_sdk::boot_services::BootServices;
    use r_efi::{efi, system};
    use rust_boot_services_allocator_dxe::GLOBAL_ALLOCATOR;

    #[unsafe(no_mangle)]
    pub extern "efiapi" fn efi_main(
        image_handle: efi::Handle,
        system_table: *const system::SystemTable,
    ) -> efi::Status {
        // Safety: This block is unsafe because it assumes that system_table and (*system_table).boot_services are correct,
        // and because it mutates/accesses the global BOOT_SERVICES static.
        unsafe {
            BOOT_SERVICES.init(&*((*system_table).boot_services));
        }

        BOOT_SERVICES.exit(image_handle, efi::Status::SUCCESS, None);
        efi::Status::SUCCESS
    }

    #[panic_handler]
    fn panic(info: &PanicInfo) -> ! {
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
