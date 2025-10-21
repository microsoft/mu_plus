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

const TEST_GUID1: efi::Guid =
    efi::Guid::from_fields(0x12345678, 0x1234, 0x5678, 0x9a, 0xbc, &[0xde, 0xf0, 0x12, 0x34, 0x56, 0x78]);
const TEST_GUID2: efi::Guid =
    efi::Guid::from_fields(0x87654321, 0x4321, 0x8765, 0xba, 0x98, &[0x76, 0x54, 0x32, 0x10, 0xfe, 0xdc]);

pub(crate) fn bench_connect_controller(_handle: efi::Handle, num_calls: usize) -> Result<u64, BenchError> {
    extern "efiapi" fn mock_supported(
        _this: *mut efi::protocols::driver_binding::Protocol,
        _controller_handle: efi::Handle,
        _remaining_device_path: *mut efi::protocols::device_path::Protocol,
    ) -> efi::Status {
        efi::Status::SUCCESS
    }

    extern "efiapi" fn mock_start(
        _this: *mut efi::protocols::driver_binding::Protocol,
        _controller_handle: efi::Handle,
        _remaining_device_path: *mut efi::protocols::device_path::Protocol,
    ) -> efi::Status {
        efi::Status::SUCCESS
    }

    extern "efiapi" fn mock_stop(
        _this: *mut efi::protocols::driver_binding::Protocol,
        _controller_handle: efi::Handle,
        _num_children: usize,
        _child_handle_buffer: *mut efi::Handle,
    ) -> efi::Status {
        efi::Status::SUCCESS
    }

    let controller_handle = unsafe {
        BOOT_SERVICES
            .install_protocol_interface_unchecked(None, &TEST_GUID1, 0x1111 as *mut core::ffi::c_void)
            .map_err(|e| BenchError::InvalidData("Failed to install controller protocol interface."))
    }?;
    let driver_handle = unsafe {
        BOOT_SERVICES
            .install_protocol_interface_unchecked(
                None,
                &efi::protocols::device_path::PROTOCOL_GUID,
                0x2222 as *mut core::ffi::c_void,
            )
            .map_err(|e| BenchError::InvalidData("Failed to install driver protocol interface."))
    }?;

    let image_handle = unsafe {
        BOOT_SERVICES.install_protocol_interface_unchecked(
            None,
            &TEST_GUID2,
            core::ptr::null_mut(), // Dummy protocol data for test
        )
    }
    .map_err(|e| BenchError::InvalidData("Failed to install driver binding protocol."))?;
    let binding = Box::new(efi::protocols::driver_binding::Protocol {
        version: 10,
        supported: mock_supported,
        start: mock_start,
        stop: mock_stop,
        driver_binding_handle: driver_handle,
        image_handle,
    });
    let binding_ptr = Box::into_raw(binding) as *mut core::ffi::c_void;

    unsafe {
        BOOT_SERVICES
            .install_protocol_interface_unchecked(
                Some(driver_handle),
                &efi::protocols::driver_binding::PROTOCOL_GUID,
                binding_ptr,
            )
            .map_err(|e| BenchError::InvalidData("Failed to install driver binding protocol."))?;
    }

    let mut tot_cycles = 0;
    for _ in 0..num_calls {
        let start = Arch::cpu_count();
        unsafe {
            BOOT_SERVICES
                .connect_controller(controller_handle, vec![driver_handle], core::ptr::null_mut(), false)
                .map_err(|e| BenchError::InvalidData("Failed to connect controller."))?;
        }
        let end = Arch::cpu_count();
        tot_cycles += end - start;
        BOOT_SERVICES
            .disconnect_controller(controller_handle, None, None)
            .map_err(|_| BenchError::InvalidData("Failed to disconnect controller."))?;
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

pub(crate) fn bench_calculate_crc32(_handle: efi::Handle, num_calls: usize) -> Result<u64, BenchError> {
    let mut tot_cycles = 0;
    let data: [u8; 128] = [0; 128];
    for _ in 0..num_calls {
        let start = Arch::cpu_count();
        let _crc = unsafe {
            BOOT_SERVICES.calculate_crc_32(&data).map_err(|e| BenchError::InvalidData("Failed to calculate CRC32."))
        }?;
        let end = Arch::cpu_count();
        tot_cycles += end - start;
    }
    Ok(tot_cycles)
}

pub(crate) fn bench_install_configuration_table(_handle: efi::Handle, num_calls: usize) -> Result<u64, BenchError> {
    let mut tot_cycles = 0;
    let table: u64 = 0xDEADBEEF;
    for _ in 0..num_calls {
        let start = Arch::cpu_count();
        unsafe {
            BOOT_SERVICES
                .install_configuration_table(&TEST_GUID1, &table as *const u64 as *mut c_void)
                .map_err(|e| BenchError::InvalidData("Failed to install configuration table."))?;
        }
        let end = Arch::cpu_count();
        tot_cycles += end - start;
    }
    Ok(tot_cycles)
}

pub(crate) fn bench_install_protocol_interface(_handle: efi::Handle, num_calls: usize) -> Result<u64, BenchError> {
    let mut tot_cycles = 0;
    for _ in 0..num_calls {
        let start = Arch::cpu_count();
        unsafe {
            BOOT_SERVICES
                .install_protocol_interface_unchecked(None, &TEST_GUID1, ptr::null_mut())
                .map_err(|e| BenchError::InvalidData("Failed to close protocol."))?;
        }
        let end = Arch::cpu_count();
        tot_cycles += end - start;
    }
    Ok(tot_cycles)
}

pub(crate) fn bench_open_protocol(handle: efi::Handle, num_calls: usize) -> Result<u64, BenchError> {
    let mut tot_cycles = 0;
    let interface1: *mut c_void = 0x1234 as *mut c_void;
    let agent_handle = unsafe { BOOT_SERVICES.install_protocol_interface_unchecked(None, &TEST_GUID1, interface1) }
        .map_err(|e| BenchError::InvalidData("Failed to close protocol."))?;
    let controller_handle =
        unsafe { BOOT_SERVICES.install_protocol_interface_unchecked(None, &TEST_GUID1, interface1) }
            .map_err(|e| BenchError::InvalidData("Failed to close protocol."))?;
    let protocol_handle = unsafe { BOOT_SERVICES.install_protocol_interface_unchecked(None, &TEST_GUID1, interface1) }
        .map_err(|e| BenchError::InvalidData("Failed to close protocol."))?;
    for _ in 0..num_calls {
        let start = Arch::cpu_count();
        unsafe {
            BOOT_SERVICES
                .open_protocol_unchecked(
                    protocol_handle,
                    &TEST_GUID1,
                    agent_handle,
                    controller_handle,
                    efi::OPEN_PROTOCOL_BY_DRIVER,
                )
                .map_err(|e| BenchError::InvalidData("Failed to open protocol."))?;
        }
        let end = Arch::cpu_count();
        tot_cycles += end - start;

        BOOT_SERVICES
            .close_protocol(protocol_handle, &TEST_GUID1, agent_handle, controller_handle)
            .map_err(|_| BenchError::InvalidData("Failed to close protocol."))?;
    }
    Ok(tot_cycles)
}

pub(crate) fn bench_close_protocol(handle: efi::Handle, num_calls: usize) -> Result<u64, BenchError> {
    let mut tot_cycles = 0;
    let interface1: *mut c_void = 0x1234 as *mut c_void;
    let agent_handle = unsafe { BOOT_SERVICES.install_protocol_interface_unchecked(None, &TEST_GUID1, interface1) }
        .map_err(|e| BenchError::InvalidData("Failed to close protocol."))?;
    let controller_handle =
        unsafe { BOOT_SERVICES.install_protocol_interface_unchecked(None, &TEST_GUID1, interface1) }
            .map_err(|e| BenchError::InvalidData("Failed to close protocol."))?;
    let protocol_handle = unsafe { BOOT_SERVICES.install_protocol_interface_unchecked(None, &TEST_GUID1, interface1) }
        .map_err(|e| BenchError::InvalidData("Failed to close protocol."))?;
    for _ in 0..num_calls {
        unsafe {
            BOOT_SERVICES
                .open_protocol_unchecked(
                    protocol_handle,
                    &TEST_GUID1,
                    agent_handle,
                    controller_handle,
                    efi::OPEN_PROTOCOL_BY_DRIVER,
                )
                .map_err(|e| BenchError::InvalidData("Failed to open protocol."))?;
        }

        let start = Arch::cpu_count();
        BOOT_SERVICES
            .close_protocol(protocol_handle, &TEST_GUID1, agent_handle, controller_handle)
            .map_err(|_| BenchError::InvalidData("Failed to close protocol."))?;
        let end = Arch::cpu_count();
        tot_cycles += end - start;
    }
    Ok(tot_cycles)
}

pub(crate) fn bench_handle_protocol(handle: efi::Handle, num_calls: usize) -> Result<u64, BenchError> {
    let mut tot_cycles = 0;
    let interface1: *mut c_void = 0x1234 as *mut c_void;
    let protocol_handle = unsafe { BOOT_SERVICES.install_protocol_interface_unchecked(None, &TEST_GUID1, interface1) }
        .map_err(|e| BenchError::InvalidData("Failed to close protocol."))?;
    for _ in 0..num_calls {
        let start = Arch::cpu_count();

        unsafe {
            BOOT_SERVICES
                .handle_protocol_unchecked(protocol_handle, &TEST_GUID1)
                .map_err(|e| BenchError::InvalidData("Failed to open protocol."))?;
        }

        let end = Arch::cpu_count();
        tot_cycles += end - start;
    }
    Ok(tot_cycles)
}

pub(crate) fn bench_locate_device_path(handle: efi::Handle, num_calls: usize) -> Result<u64, BenchError> {
    let loaded_image_protocol = unsafe {
        BOOT_SERVICES
            .handle_protocol::<efi::protocols::loaded_image::Protocol>(handle)
            .map_err(|_| BenchError::InvalidData("Failed to get loaded image protocol."))?
    };
    let mut device_path_protocol = unsafe {
        BOOT_SERVICES
            .handle_protocol::<efi::protocols::device_path::Protocol>(loaded_image_protocol.device_handle)
            .map_err(|_| BenchError::InvalidData("Failed to get device path protocol."))?
    };

    let mut tot_cycles = 0;
    for _ in 0..num_calls {
        let mut device_path_ptr = device_path_protocol as *mut efi::protocols::device_path::Protocol;
        let start = Arch::cpu_count();
        unsafe {
            BOOT_SERVICES
                .locate_device_path(&efi::protocols::device_path::PROTOCOL_GUID, &mut device_path_ptr as *mut _)
                .map_err(|e| BenchError::InvalidData("Failed to locate device path."))
        }?;
        let end = Arch::cpu_count();
        tot_cycles += end - start;
    }

    Ok(tot_cycles)
}

pub(crate) fn bench_open_protocol_information(handle: efi::Handle, num_calls: usize) -> Result<u64, BenchError> {
    let mut tot_cycles = 0;
    for _ in 0..num_calls {
        let start = Arch::cpu_count();
        let _info = BOOT_SERVICES
            .open_protocol_information(handle, &efi::protocols::loaded_image::PROTOCOL_GUID)
            .map_err(|e| BenchError::InvalidData("Failed to get open protocol information."))?;
        let end = Arch::cpu_count();
        tot_cycles += end - start;
    }

    Ok(tot_cycles)
}

pub(crate) fn bench_protocols_per_handle(handle: efi::Handle, num_calls: usize) -> Result<u64, BenchError> {
    let mut tot_cycles = 0;
    for _ in 0..num_calls {
        let start = Arch::cpu_count();
        let _protocols = BOOT_SERVICES
            .protocols_per_handle(handle)
            .map_err(|e| BenchError::InvalidData("Failed to get protocols per handle."))?;
        let end = Arch::cpu_count();
        tot_cycles += end - start;
    }

    Ok(tot_cycles)
}

pub(crate) fn bench_register_protocol_notify(_handle: efi::Handle, num_calls: usize) -> Result<u64, BenchError> {
    extern "efiapi" fn mock_notify(_ptr: *mut c_void, _data: *mut i32) {}

    let mut tot_cycles = 0;
    for _ in 0..num_calls {
        let event = unsafe {
            BOOT_SERVICES
                .create_event_unchecked::<i32>(
                    EventType::NOTIFY_SIGNAL,
                    Tpl::NOTIFY,
                    Some(mock_notify),
                    &mut 0 as *mut i32,
                )
                .map_err(|e| {
                    debugln!(DEBUG_ERROR, "{:?}", e);
                    BenchError::InvalidData("Failed to create valid event.")
                })
        }?;
        let start = Arch::cpu_count();
        BOOT_SERVICES
            .register_protocol_notify(&efi::protocols::loaded_image::PROTOCOL_GUID, event)
            .map_err(|e| BenchError::InvalidData("Failed to register protocol notify."))?;
        let end = Arch::cpu_count();
        tot_cycles += end - start;
    }

    Ok(tot_cycles)
}

pub(crate) fn bench_reinstall_protocol_interface(_handle: efi::Handle, num_calls: usize) -> Result<u64, BenchError> {
    let mut tot_cycles = 0;
    let mut prev_interface: *mut c_void = 0x1234 as *mut c_void;
    let mut new_interface = 0x5678 as *mut c_void;
    let protocol_handle =
        unsafe { BOOT_SERVICES.install_protocol_interface_unchecked(None, &TEST_GUID1, prev_interface) }.map_err(
            |e| {
                debugln!(DEBUG_ERROR, "{:?}", e);
                BenchError::InvalidData("Failed to install dummy protocol.")
            },
        )?;
    for _ in 0..num_calls {
        let start = Arch::cpu_count();
        unsafe {
            BOOT_SERVICES
                .reinstall_protocol_interface_unchecked(protocol_handle, &TEST_GUID1, prev_interface, new_interface)
                .map_err(|e| {
                    debugln!(DEBUG_ERROR, "{:?}", e);
                    BenchError::InvalidData("Failed to reinstall protocol interface.")
                })?;
        }
        prev_interface = new_interface;
        new_interface = 0x5678 as *mut c_void;
        let end = Arch::cpu_count();
        tot_cycles += end - start;
    }
    Ok(tot_cycles)
}

pub(crate) fn bench_uninstall_protocol_interface(_handle: efi::Handle, num_calls: usize) -> Result<u64, BenchError> {
    let mut tot_cycles = 0;
    let interface1: *mut c_void = 0x1234 as *mut c_void;
    let mut protocol_handle =
        unsafe { BOOT_SERVICES.install_protocol_interface_unchecked(None, &TEST_GUID1, interface1) }
            .map_err(|e| BenchError::InvalidData("Failed to install dummy protocol."))?;
    for _ in 0..num_calls {
        let start = Arch::cpu_count();
        unsafe {
            BOOT_SERVICES
                .uninstall_protocol_interface_unchecked(protocol_handle, &TEST_GUID1, interface1)
                .map_err(|e| BenchError::InvalidData("Failed to uninstall protocol interface."))?;
        }
        let end = Arch::cpu_count();
        tot_cycles += end - start;

        // Reinstall for next iteration
        unsafe {
            protocol_handle = BOOT_SERVICES
                .install_protocol_interface_unchecked(None, &TEST_GUID1, interface1)
                .map_err(|e| BenchError::InvalidData("Failed to install a new dummy protocol."))?;
        }
    }
    Ok(tot_cycles)
}

pub(crate) fn bench_raise_tpl(_handle: efi::Handle, num_calls: usize) -> Result<u64, BenchError> {
    let mut tot_cycles = 0;
    for _ in 0..num_calls {
        let start = Arch::cpu_count();
        let old_tpl = BOOT_SERVICES.raise_tpl(Tpl::NOTIFY);
        let end = Arch::cpu_count();
        tot_cycles += end - start;

        BOOT_SERVICES.restore_tpl(old_tpl);
    }

    Ok(tot_cycles)
}

pub(crate) fn bench_restore_tpl(_handle: efi::Handle, num_calls: usize) -> Result<u64, BenchError> {
    let mut tot_cycles = 0;
    for _ in 0..num_calls {
        let old_tpl = BOOT_SERVICES.raise_tpl(Tpl::NOTIFY);

        let start = Arch::cpu_count();
        BOOT_SERVICES.restore_tpl(old_tpl);
        let end = Arch::cpu_count();
        tot_cycles += end - start;
    }

    Ok(tot_cycles)
}
