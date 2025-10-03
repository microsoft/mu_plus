use alloc::vec;
use patina_sdk::boot_services::{BootServices, protocol_handler::HandleSearchType};
use r_efi::efi;

use crate::{BOOT_SERVICES, error::BenchError};

fn dummy_handle(id: usize) -> efi::Handle {
    id as efi::Handle
}

pub(crate) fn bench_core_connect_controller() -> Result<(), BenchError> {
    let mut controller_handle = core::ptr::null_mut(); // SHERRY: this should not be null
    // also, some EFI_DRIVER_BINDING_PROTOCOL handles must exist
    let driver_handles = vec![]; // according to spec, is usually NULL
    let remaining_device_path = core::ptr::null_mut(); // when NULL, connects all children
    let recursive = true; // when true, calls until entire tree is created
    let handles = BOOT_SERVICES.locate_handle_buffer(HandleSearchType::AllHandle).unwrap();

    for &handle in handles.iter() {
        // Try treating it as a controller
        if unsafe { BOOT_SERVICES.handle_protocol::<efi::protocols::driver_binding::Protocol>(handle).is_ok() } {
            controller_handle = handle;
            break; // just bench one for now
        }
    }

    unsafe {
        BOOT_SERVICES
            .connect_controller(controller_handle, driver_handles, remaining_device_path, recursive)
            .map_err(|_| BenchError::BenchFnFailure("Failed to connect controller"))
    }?;

    if controller_handle.is_null() {
        return Err(BenchError::InvalidData("No controller handle found"));
    }
    Ok(())
}
