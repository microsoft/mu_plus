use alloc::vec;
use patina_sdk::boot_services::BootServices;
use r_efi::efi;

use crate::BOOT_SERVICES;

fn dummy_handle(id: usize) -> efi::Handle {
    id as efi::Handle
}

pub(crate) fn bench_core_connect_controller() {
    // sherry: this needs more realistic data
    let handle = core::ptr::null_mut();
    let driver_handles = vec![dummy_handle(1), dummy_handle(2)];
    let remaining_device_path = core::ptr::null_mut();
    let recursive = false;
    unsafe {
        BOOT_SERVICES.connect_controller(handle, driver_handles, remaining_device_path, recursive.into());
    };
}
