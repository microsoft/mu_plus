use mu_rust_helpers::perf_timer::{Arch, ArchFunctionality as _};
use patina::boot_services::{BootServices as _, tpl::Tpl};
use r_efi::efi::{self};
use rolling_stats::Stats;

use crate::{BOOT_SERVICES, error::BenchError};

pub(crate) fn bench_raise_tpl(_handle: efi::Handle, num_calls: usize) -> Result<Stats<f64>, BenchError> {
    let mut stats: Stats<f64> = Stats::new();
    for _ in 0..num_calls {
        let start = Arch::cpu_count();
        let old_tpl = BOOT_SERVICES.raise_tpl(Tpl::NOTIFY);
        let end = Arch::cpu_count();
        stats.update((end - start) as f64);

        BOOT_SERVICES.restore_tpl(old_tpl);
    }

    Ok(stats)
}

pub(crate) fn bench_restore_tpl(_handle: efi::Handle, num_calls: usize) -> Result<Stats<f64>, BenchError> {
    let mut stats: Stats<f64> = Stats::new();
    for _ in 0..num_calls {
        let old_tpl = BOOT_SERVICES.raise_tpl(Tpl::NOTIFY);

        let start = Arch::cpu_count();
        BOOT_SERVICES.restore_tpl(old_tpl);
        let end = Arch::cpu_count();
        stats.update((end - start) as f64);
    }

    Ok(stats)
}
