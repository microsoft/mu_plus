use core::{ffi::c_void, num, ptr, str::FromStr};

use alloc::vec;
use patina_sdk::{
    base::UEFI_PAGE_SIZE,
    boot_services::{
        BootServices, allocation::MemoryType, event::EventType, protocol_handler::HandleSearchType, tpl::Tpl,
    },
};
use perf_timer::{Arch, ArchFunctionality};
use r_efi::efi::{self, BOOT_SERVICES_CODE};
use rust_advanced_logger_dxe::{DEBUG_ERROR, debugln};
use uuid::Uuid;

use crate::{BOOT_SERVICES, error::BenchError};
use alloc::boxed::Box;

pub(crate) fn bench_connect_controller(handle: efi::Handle, num_calls: usize) -> Result<u64, BenchError> {
    let mut controller_handle = core::ptr::null_mut();
    let handles = BOOT_SERVICES.locate_handle_buffer(HandleSearchType::AllHandle).unwrap();
    // Iterate until we find one that works
    for &h in handles.iter() {
        unsafe {
            let res = BOOT_SERVICES.connect_controller(h, vec![], core::ptr::null_mut(), true);
            if res.is_ok() {
                controller_handle = h;
                break;
            }
        }
    }

    if controller_handle.is_null() {
        return Err(BenchError::InvalidData("No controller handle found to connect to."));
    }

    let mut tot_cycles = 0;
    for _ in 0..num_calls {
        let start = Arch::cpu_count();
        unsafe {
            BOOT_SERVICES
                .connect_controller(controller_handle, vec![], core::ptr::null_mut(), true)
                .map_err(|e| BenchError::BenchFnFailure("connect_controller failed."))
        }?;
        let end = Arch::cpu_count();
        tot_cycles += end - start;
    }
    Ok(tot_cycles)
}

pub(crate) fn bench_check_event(_handle: efi::Handle, num_calls: usize) -> Result<u64, BenchError> {
    extern "efiapi" fn test_notify(_event: efi::Event, _context: *mut c_void) {}
    let mut tot_cycles = 0;
    for _ in 0..num_calls {
        let event_handle = unsafe {
            BOOT_SERVICES.create_event_unchecked(
                EventType::NOTIFY_WAIT,
                Tpl::NOTIFY,
                Some(test_notify),
                ptr::null_mut(),
            )
        }
        .map_err(|e| {
            debugln!(DEBUG_ERROR, "{:?}", e);
            BenchError::InvalidData("Failed to create event.")
        })?;
        BOOT_SERVICES.signal_event(event_handle).map_err(|e| BenchError::InvalidData("Failed to signal event."))?;

        let start = Arch::cpu_count();
        BOOT_SERVICES.check_event(event_handle).map_err(|e| BenchError::BenchFnFailure("check_event failed."))?;
        let end = Arch::cpu_count();
        tot_cycles += end - start;

        BOOT_SERVICES.close_event(event_handle).map_err(|e| BenchError::InvalidData("Failed to close event."))?;
    }
    Ok(tot_cycles)
}

pub(crate) fn bench_create_event(_handle: efi::Handle, num_calls: usize) -> Result<u64, BenchError> {
    extern "efiapi" fn test_notify(_event: efi::Event, _context: *mut c_void) {}
    let mut tot_cycles = 0;
    for _ in 0..num_calls {
        let start = Arch::cpu_count();
        let event_handle = unsafe {
            BOOT_SERVICES.create_event_unchecked(
                EventType::NOTIFY_WAIT,
                Tpl::NOTIFY,
                Some(test_notify),
                ptr::null_mut(),
            )
        }
        .map_err(|e| BenchError::InvalidData("Failed to create event."))?;
        let end = Arch::cpu_count();
        tot_cycles += end - start;

        BOOT_SERVICES.close_event(event_handle).map_err(|e| BenchError::InvalidData("Failed to close event."))?;
    }
    Ok(tot_cycles)
}

pub(crate) fn bench_close_event(_handle: efi::Handle, num_calls: usize) -> Result<u64, BenchError> {
    extern "efiapi" fn test_notify(_event: efi::Event, _context: *mut c_void) {}
    let mut tot_cycles = 0;
    for _ in 0..num_calls {
        let event_handle = unsafe {
            BOOT_SERVICES.create_event_unchecked(
                EventType::NOTIFY_WAIT,
                Tpl::NOTIFY,
                Some(test_notify),
                ptr::null_mut(),
            )
        }
        .map_err(|e| BenchError::InvalidData("Failed to create event."))?;

        let start = Arch::cpu_count();
        BOOT_SERVICES.close_event(event_handle).map_err(|e| BenchError::InvalidData("Failed to close event."))?;
        let end = Arch::cpu_count();
        tot_cycles += end - start;
    }
    Ok(tot_cycles)
}

pub(crate) fn bench_signal_event(_handle: efi::Handle, num_calls: usize) -> Result<u64, BenchError> {
    extern "efiapi" fn test_notify(_event: efi::Event, _context: *mut c_void) {}
    let mut tot_cycles = 0;
    for _ in 0..num_calls {
        let event_handle = unsafe {
            BOOT_SERVICES.create_event_unchecked(
                EventType::NOTIFY_WAIT,
                Tpl::NOTIFY,
                Some(test_notify),
                ptr::null_mut(),
            )
        }
        .map_err(|e| BenchError::InvalidData("Failed to create event."))?;

        let start = Arch::cpu_count();
        BOOT_SERVICES.signal_event(event_handle).map_err(|e| BenchError::InvalidData("Failed to signal event."))?;
        let end = Arch::cpu_count();
        tot_cycles += end - start;

        BOOT_SERVICES.close_event(event_handle).map_err(|e| BenchError::InvalidData("Failed to close event."))?;
    }
    Ok(tot_cycles)
}

// This is hard to bench seperately
pub(crate) fn bench_start_image_and_exit(parent_handle: efi::Handle, num_calls: usize) -> Result<u64, BenchError> {
    let mut tot_cycles = 0;
    for _ in 0..num_calls {
        let image_bytes = include_bytes!("../resources/TempTest.efi");
        let loaded_image_handle = BOOT_SERVICES
            .load_image(false, parent_handle, core::ptr::null_mut(), Some(image_bytes))
            .map_err(|e| BenchError::InvalidData("Failed to load image."))?;

        let start = Arch::cpu_count();
        BOOT_SERVICES
            .start_image(loaded_image_handle)
            .map_err(|e| BenchError::InvalidData("Failed to start image."))?;
        let end = Arch::cpu_count();

        tot_cycles += end - start;
    }
    Ok(tot_cycles)
}

pub(crate) fn bench_load_image(parent_handle: efi::Handle, num_calls: usize) -> Result<u64, BenchError> {
    let mut tot_cycles = 0;
    for _ in 0..num_calls {
        let image_bytes = include_bytes!("../resources/TempTest.efi");
        let start = Arch::cpu_count();
        let loaded_image_handle = BOOT_SERVICES
            .load_image(false, parent_handle, core::ptr::null_mut(), Some(image_bytes))
            .map_err(|e| BenchError::InvalidData("Failed to load image."))?;
        let end = Arch::cpu_count();
        tot_cycles += end - start;
    }
    Ok(tot_cycles)
}

pub(crate) fn bench_allocate_pages(_handle: efi::Handle, num_calls: usize) -> Result<u64, BenchError> {
    let mut tot_cycles = 0;
    for _ in 0..num_calls {
        let start = Arch::cpu_count();
        let pages = BOOT_SERVICES
            .allocate_pages(patina_sdk::boot_services::allocation::AllocType::AnyPage, MemoryType::ACPI_MEMORY_NVS, 1)
            .map_err(|e| BenchError::InvalidData("Failed to allocate pages."))?;
        let end = Arch::cpu_count();
        tot_cycles += end - start;

        BOOT_SERVICES.free_pages(pages, 1).map_err(|e| BenchError::InvalidData("Failed to free pages."))?;
    }
    Ok(tot_cycles)
}

pub(crate) fn bench_allocate_pool(_handle: efi::Handle, num_calls: usize) -> Result<u64, BenchError> {
    let mut tot_cycles = 0;
    for _ in 0..num_calls {
        let start = Arch::cpu_count();
        let pool = BOOT_SERVICES
            .allocate_pool(MemoryType::ACPI_MEMORY_NVS, UEFI_PAGE_SIZE / 4)
            .map_err(|e| BenchError::InvalidData("Failed to allocate pool."))?;
        let end = Arch::cpu_count();
        tot_cycles += end - start;

        BOOT_SERVICES.free_pool(pool).map_err(|e| BenchError::InvalidData("Failed to free pool."))?;
    }
    Ok(tot_cycles)
}

pub(crate) fn bench_free_pages(_handle: efi::Handle, num_calls: usize) -> Result<u64, BenchError> {
    let mut tot_cycles = 0;
    for _ in 0..num_calls {
        let pages = BOOT_SERVICES
            .allocate_pages(patina_sdk::boot_services::allocation::AllocType::AnyPage, MemoryType::ACPI_MEMORY_NVS, 1)
            .map_err(|e| BenchError::InvalidData("Failed to allocate pages."))?;

        let start = Arch::cpu_count();
        BOOT_SERVICES.free_pages(pages, 1).map_err(|e| BenchError::InvalidData("Failed to free pages."))?;
        let end = Arch::cpu_count();
        tot_cycles += end - start;
    }
    Ok(tot_cycles)
}

pub(crate) fn bench_free_pool(_handle: efi::Handle, num_calls: usize) -> Result<u64, BenchError> {
    let mut tot_cycles = 0;
    for _ in 0..num_calls {
        let pool = BOOT_SERVICES
            .allocate_pool(MemoryType::ACPI_MEMORY_NVS, UEFI_PAGE_SIZE / 4)
            .map_err(|e| BenchError::InvalidData("Failed to allocate pool."))?;

        let start = Arch::cpu_count();
        BOOT_SERVICES.free_pool(pool).map_err(|e| BenchError::InvalidData("Failed to free pool."))?;
        let end = Arch::cpu_count();
        tot_cycles += end - start;
    }
    Ok(tot_cycles)
}

pub(crate) fn bench_copy_mem(_handle: efi::Handle, num_calls: usize) -> Result<u64, BenchError> {
    let mut tot_cycles = 0;
    let src: u64 = 5678;
    let mut dst: u64 = 1234;
    for _ in 0..num_calls {
        let start = Arch::cpu_count();
        unsafe {
            BOOT_SERVICES.copy_mem::<u64>(&mut dst, &src);
        }
        let end = Arch::cpu_count();
        tot_cycles += end - start;
    }
    Ok(tot_cycles)
}

pub(crate) fn bench_set_mem(_handle: efi::Handle, num_calls: usize) -> Result<u64, BenchError> {
    let mut tot_cycles = 0;
    let mut dst: [u8; 128] = [0; 128];
    for _ in 0..num_calls {
        let start = Arch::cpu_count();
        unsafe {
            BOOT_SERVICES.set_mem(&mut dst, 1);
        }
        let end = Arch::cpu_count();
        tot_cycles += end - start;
    }
    Ok(tot_cycles)
}

pub(crate) fn bench_get_memory_map(_handle: efi::Handle, num_calls: usize) -> Result<u64, BenchError> {
    let mut tot_cycles = 0;
    for _ in 0..num_calls {
        let start = Arch::cpu_count();
        BOOT_SERVICES.get_memory_map().map_err(|e| BenchError::InvalidData("Failed to get memory map."))?;
        let end = Arch::cpu_count();
        tot_cycles += end - start;
    }
    Ok(tot_cycles)
}
