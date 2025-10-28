use core::{ffi::c_void, ptr};

use mu_rust_helpers::perf_timer::{Arch, ArchFunctionality as _};
use patina::boot_services::{BootServices, event::EventType, tpl::Tpl};
use r_efi::efi;
use rolling_stats::Stats;

use crate::{BOOT_SERVICES, error::BenchError};

pub(crate) fn bench_check_event(_handle: efi::Handle, num_calls: usize) -> Result<Stats<f64>, BenchError> {
    extern "efiapi" fn test_notify(_event: efi::Event, _context: *mut c_void) {}
    let mut stats: Stats<f64> = Stats::new();
    for _ in 0..num_calls {
        let event_handle = unsafe {
            BOOT_SERVICES.create_event_unchecked(
                EventType::NOTIFY_WAIT,
                Tpl::NOTIFY,
                Some(test_notify),
                ptr::null_mut(),
            )
        }
        .map_err(|e| BenchError::BenchSetupFailure("Failed to create event", e))?;
        BOOT_SERVICES
            .signal_event(event_handle)
            .map_err(|e| BenchError::BenchSetupFailure("Failed to signal event", e))?;

        let start = Arch::cpu_count();
        BOOT_SERVICES.check_event(event_handle).map_err(|e| BenchError::BenchFailure("check_event failed", e))?;
        let end = Arch::cpu_count();
        stats.update((end - start) as f64);

        BOOT_SERVICES
            .close_event(event_handle)
            .map_err(|e| BenchError::BenchCleanupFailure("Failed to close event", e))?;
    }
    Ok(stats)
}

pub(crate) fn bench_create_event(_handle: efi::Handle, num_calls: usize) -> Result<Stats<f64>, BenchError> {
    extern "efiapi" fn test_notify(_event: efi::Event, _context: *mut c_void) {}
    let mut stats: Stats<f64> = Stats::new();
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
        .map_err(|e| BenchError::BenchFailure("Failed to create event", e))?;
        let end = Arch::cpu_count();
        stats.update((end - start) as f64);

        BOOT_SERVICES
            .close_event(event_handle)
            .map_err(|e| BenchError::BenchCleanupFailure("Failed to close event", e))?;
    }
    Ok(stats)
}

pub(crate) fn bench_close_event(_handle: efi::Handle, num_calls: usize) -> Result<Stats<f64>, BenchError> {
    extern "efiapi" fn test_notify(_event: efi::Event, _context: *mut c_void) {}
    let mut stats: Stats<f64> = Stats::new();
    for _ in 0..num_calls {
        let event_handle = unsafe {
            BOOT_SERVICES.create_event_unchecked(
                EventType::NOTIFY_WAIT,
                Tpl::NOTIFY,
                Some(test_notify),
                ptr::null_mut(),
            )
        }
        .map_err(|e| BenchError::BenchSetupFailure("Failed to create event", e))?;
        let start = Arch::cpu_count();
        BOOT_SERVICES.close_event(event_handle).map_err(|e| BenchError::BenchFailure("Failed to close event", e))?;
        let end = Arch::cpu_count();
        stats.update((end - start) as f64);
    }
    Ok(stats)
}

pub(crate) fn bench_signal_event(_handle: efi::Handle, num_calls: usize) -> Result<Stats<f64>, BenchError> {
    extern "efiapi" fn test_notify(_event: efi::Event, _context: *mut c_void) {}
    let mut stats: Stats<f64> = Stats::new();
    for _ in 0..num_calls {
        let event_handle = unsafe {
            BOOT_SERVICES.create_event_unchecked(
                EventType::NOTIFY_WAIT,
                Tpl::NOTIFY,
                Some(test_notify),
                ptr::null_mut(),
            )
        }
        .map_err(|e| BenchError::BenchSetupFailure("Failed to create evente", e))?;

        let start = Arch::cpu_count();
        BOOT_SERVICES.signal_event(event_handle).map_err(|e| BenchError::BenchFailure("Failed to signal event", e))?;
        let end = Arch::cpu_count();
        stats.update((end - start) as f64);

        BOOT_SERVICES
            .close_event(event_handle)
            .map_err(|e| BenchError::BenchCleanupFailure("Failed to close event", e))?;
    }
    Ok(stats)
}
