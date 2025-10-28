use mu_rust_helpers::perf_timer::{Arch, ArchFunctionality as _};
use patina::boot_services::BootServices;
use r_efi::efi;
use rolling_stats::Stats;

use crate::{BOOT_SERVICES, error::BenchError};

///  `start_image` is diffcult to bench individually.
/// The image `TempTest.efi` is a no-op image that exits immediately.
pub(crate) fn bench_start_image_and_exit(
    parent_handle: efi::Handle,
    num_calls: usize,
) -> Result<Stats<f64>, BenchError> {
    let mut stats: Stats<f64> = Stats::new();
    for _ in 0..num_calls {
        let image_bytes = include_bytes!("../../resources/TempTest.efi");
        let loaded_image_handle = BOOT_SERVICES
            .load_image(false, parent_handle, core::ptr::null_mut(), Some(image_bytes))
            .map_err(|e| BenchError::BenchSetupFailure("Failed to load image", e))?;

        let start = Arch::cpu_count();
        BOOT_SERVICES
            .start_image(loaded_image_handle)
            .map_err(|e| BenchError::BenchFailure("Failed to start image", e.0))?;
        let end = Arch::cpu_count();
        stats.update((end - start) as f64);
    }
    Ok(stats)
}

pub(crate) fn bench_load_image(parent_handle: efi::Handle, num_calls: usize) -> Result<Stats<f64>, BenchError> {
    let mut stats: Stats<f64> = Stats::new();
    for _ in 0..num_calls {
        let image_bytes = include_bytes!("../../resources/TempTest.efi");
        let start = Arch::cpu_count();
        let _loaded_image_handle = BOOT_SERVICES
            .load_image(false, parent_handle, core::ptr::null_mut(), Some(image_bytes))
            .map_err(|e| BenchError::BenchFailure("Failed to load image", e))?;
        let end = Arch::cpu_count();
        stats.update((end - start) as f64);
    }
    Ok(stats)
}
