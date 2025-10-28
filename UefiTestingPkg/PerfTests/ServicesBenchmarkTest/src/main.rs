//! UEFI Services Benchmark Test Application
//!
//! This crate provides a benchmark test application for evaluating the performance of UEFI boot services.
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
    use core::panic::PanicInfo;
    use r_efi::{efi, system};
    use rust_advanced_logger_dxe::{DEBUG_ERROR, debugln, init_debug};
    use rust_boot_services_allocator_dxe::GLOBAL_ALLOCATOR;
    use services_benchmark_test::{BOOT_SERVICES, bench_start};

    #[unsafe(no_mangle)]
    pub extern "efiapi" fn efi_main(
        image_handle: efi::Handle,
        system_table: *const system::SystemTable,
    ) -> efi::Status {
        // Safety: This block is unsafe because it assumes that system_table and (*system_table).boot_services are correct,
        // and because it mutates/accesses the global BOOT_SERVICES static.
        unsafe {
            BOOT_SERVICES.init(&*((*system_table).boot_services));
            GLOBAL_ALLOCATOR.init((*system_table).boot_services);
            init_debug((*system_table).boot_services);
        }

        bench_start(image_handle, system_table);

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
