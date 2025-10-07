use core::str::FromStr;

use alloc::vec;
use patina_sdk::boot_services::{BootServices, protocol_handler::HandleSearchType};
use perf_timer::{Arch, ArchFunctionality};
use r_efi::efi;
use rust_advanced_logger_dxe::{DEBUG_ERROR, debugln};
use uuid::Uuid;

use crate::{BOOT_SERVICES, error::BenchError};
use alloc::boxed::Box;

const TEST_GUID: efi::Guid =
    efi::Guid::from_fields(0x12345678, 0x1234, 0x5678, 0x9a, 0xbc, &[0xde, 0xf0, 0x12, 0x34, 0x56, 0x78]);

fn dummy_handle(id: usize) -> efi::Handle {
    id as efi::Handle
}

fn create_driver_binding(
    version: u32,
    handle: efi::Handle,
    supported_fn: extern "efiapi" fn(
        *mut efi::protocols::driver_binding::Protocol,
        efi::Handle,
        *mut efi::protocols::device_path::Protocol,
    ) -> efi::Status,
    start_fn: extern "efiapi" fn(
        *mut efi::protocols::driver_binding::Protocol,
        efi::Handle,
        *mut efi::protocols::device_path::Protocol,
    ) -> efi::Status,
    stop_fn: extern "efiapi" fn(
        *mut efi::protocols::driver_binding::Protocol,
        efi::Handle,
        usize,
        *mut efi::Handle,
    ) -> efi::Status,
) -> Box<efi::protocols::driver_binding::Protocol> {
    // Create a unique image handle by installing a protocol with arbitrary GUID
    let image_handle = match unsafe {
        BOOT_SERVICES.install_protocol_interface_unchecked(
            None,
            &TEST_GUID,
            core::ptr::null_mut(), // Dummy protocol data for test
        )
    } {
        Ok(handle) => handle,
        Err(_) => 1 as efi::Handle, // Fallback to some default
    };

    Box::new(efi::protocols::driver_binding::Protocol {
        version,
        supported: supported_fn,
        start: start_fn,
        stop: stop_fn,
        driver_binding_handle: handle,
        image_handle,
    })
}

extern "efiapi" fn mock_supported_success(
    _this: *mut efi::protocols::driver_binding::Protocol,
    _controller_handle: efi::Handle,
    _remaining_device_path: *mut efi::protocols::device_path::Protocol,
) -> efi::Status {
    efi::Status::SUCCESS
}

extern "efiapi" fn mock_start_success(
    _this: *mut efi::protocols::driver_binding::Protocol,
    _controller_handle: efi::Handle,
    _remaining_device_path: *mut efi::protocols::device_path::Protocol,
) -> efi::Status {
    efi::Status::SUCCESS
}

extern "efiapi" fn mock_stop_success(
    _this: *mut efi::protocols::driver_binding::Protocol,
    _controller_handle: efi::Handle,
    _num_children: usize,
    _child_handle_buffer: *mut efi::Handle,
) -> efi::Status {
    efi::Status::SUCCESS
}

pub(crate) fn bench_connect_controller(num_calls: usize) -> Result<u64, BenchError> {
    // let mut controller_handle = core::ptr::null_mut(); // SHERRY: this should not be null
    // // also, some EFI_DRIVER_BINDING_PROTOCOL handles must exist
    // let driver_handles = vec![]; // according to spec, is usually NULL
    // let remaining_device_path = core::ptr::null_mut(); // when NULL, connects all children
    // let recursive = true; // when true, calls until entire tree is created
    // let handles = BOOT_SERVICES.locate_handle_buffer(HandleSearchType::AllHandle).unwrap();

    let controller_handle = unsafe {
        BOOT_SERVICES.install_protocol_interface_unchecked(
            None,
            &efi::protocols::device_path::PROTOCOL_GUID,
            0x1111 as *mut core::ffi::c_void,
        )
    }
    .map_err(|_| BenchError::InvalidData("Failed to initialize controller handle"))?;

    let driver_handle = unsafe {
        BOOT_SERVICES.install_protocol_interface_unchecked(
            None,
            &efi::protocols::device_path::PROTOCOL_GUID,
            0x2222 as *mut core::ffi::c_void,
        )
    }
    .map_err(|_| BenchError::InvalidData("Failed to initialize device handle"))?;

    // Create and install a driver binding protocol
    let binding =
        create_driver_binding(10, driver_handle, mock_supported_success, mock_start_success, mock_stop_success);
    let binding_ptr = Box::into_raw(binding) as *mut core::ffi::c_void;

    unsafe {
        BOOT_SERVICES
            .install_protocol_interface_unchecked(
                Some(driver_handle),
                &efi::protocols::driver_binding::PROTOCOL_GUID,
                binding_ptr,
            )
            .map_err(|_| BenchError::InvalidData("Failed to set up driver binding protocol"))
    }?;

    // for &handle in handles.iter() {
    //     // Try treating it as a controller
    //     if unsafe { BOOT_SERVICES.handle_protocol::<efi::protocols::pci_io::Protocol>(handle).is_ok() }
    //         && unsafe { BOOT_SERVICES.handle_protocol::<efi::protocols::device_path::Protocol>(handle).is_ok() }
    //     {
    //         // how to write to file????
    //         controller_handle = handle;
    //         break; // just bench one for now
    //     }
    // }

    let mut tot_cycles = 0;
    for _ in 0..num_calls {
        unsafe {
            BOOT_SERVICES
                .disconnect_controller(controller_handle, None, None)
                .map_err(|_| BenchError::BenchFnFailure("Failed to disconnect controller"));
        }
        let start_cycles = Arch::cpu_count();
        debugln!(
            DEBUG_ERROR,
            "Connecting controller handle {:p} with driver handle {:p}",
            controller_handle,
            driver_handle
        );
        unsafe {
            BOOT_SERVICES
                .connect_controller(controller_handle, vec![driver_handle], core::ptr::null_mut(), false)
                .map_err(|_| BenchError::BenchFnFailure("Failed to connect controller"))
        }?;
        debugln!(
            DEBUG_ERROR,
            "DONE CONNECTING controller handle {:p} with driver handle {:p}",
            controller_handle,
            driver_handle
        );
        let end_cycles = Arch::cpu_count();
        tot_cycles += end_cycles - start_cycles;
    }

    if controller_handle.is_null() {
        return Err(BenchError::InvalidData("No controller handle found"));
    }

    Ok(tot_cycles)
}
