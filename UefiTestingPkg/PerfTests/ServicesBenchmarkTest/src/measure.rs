use mu_rust_helpers::perf_timer::{Arch, ArchFunctionality};

use crate::bench_fn::bench_core_connect_controller;

pub fn measure_single_fn(measure_f: BenchFn) -> u64 {
    let start_ct = Arch::cpu_count();
    for _ in 0..1000 {
        measure_f();
    }
    let end_ct = Arch::cpu_count();
    end_ct - start_ct
}

type BenchFn = fn();

pub static BENCH_FNS: [BenchFn; 1] = [bench_core_connect_controller];
