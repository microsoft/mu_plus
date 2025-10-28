use mu_rust_helpers::perf_timer::{Arch, ArchFunctionality as _};
use patina::boot_services::BootServices;
use r_efi::efi;
use rolling_stats::Stats;

use crate::{
    BOOT_SERVICES,
    bench::{TEST_GUID1, TEST_GUID2},
    error::BenchError,
};

pub(crate) fn bench_connect_controller(_handle: efi::Handle, num_calls: usize) -> Result<Stats<f64>, BenchError> {
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
            .map_err(|e| BenchError::BenchSetupFailure("Failed to install protocol interface for controller", e))
    }?;
    let driver_handle = unsafe {
        BOOT_SERVICES
            .install_protocol_interface_unchecked(
                None,
                &efi::protocols::device_path::PROTOCOL_GUID,
                0x2222 as *mut core::ffi::c_void,
            )
            .map_err(|e| BenchError::BenchSetupFailure("Failed to install protocol interface for driver", e))
    }?;

    let image_handle = unsafe {
        BOOT_SERVICES.install_protocol_interface_unchecked(
            None,
            &TEST_GUID2,
            core::ptr::null_mut(), // Dummy protocol data for test
        )
    }
    .map_err(|e| BenchError::BenchSetupFailure("Failed to install protocol interface for image", e))?;
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
            .map_err(|e| BenchError::BenchSetupFailure("Failed to install protocol interface for driver binding", e))?;
    }

    let mut stats: Stats<f64> = Stats::new();
    for _ in 0..num_calls {
        let start = Arch::cpu_count();
        unsafe {
            BOOT_SERVICES
                .connect_controller(controller_handle, vec![driver_handle], core::ptr::null_mut(), false)
                .map_err(|e| BenchError::BenchFailure("Failed to connect controller", e))?;
        }
        let end = Arch::cpu_count();
        stats.update((end - start) as f64);
        stats.update((end - start) as f64);
        BOOT_SERVICES
            .disconnect_controller(controller_handle, None, None)
            .map_err(|e| BenchError::BenchCleanupFailure("Failed to disconnect controller", e))?;
    }

    // Uninstall protocols to prevent side effects.
    unsafe {
        BOOT_SERVICES
            .uninstall_protocol_interface_unchecked(
                driver_handle,
                &efi::protocols::device_path::PROTOCOL_GUID,
                0x2222 as *mut core::ffi::c_void,
            )
            .map_err(|e| BenchError::BenchCleanupFailure("Failed to uninstall protocol interfacee", e))?
    };

    Ok(stats)
}
