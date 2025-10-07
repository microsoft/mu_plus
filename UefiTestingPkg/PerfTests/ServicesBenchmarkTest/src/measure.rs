use mu_rust_helpers::perf_timer::{Arch, ArchFunctionality};

use crate::{bench_fn::bench_connect_controller, error::BenchError};

// A BenchFn returns total cycles for one call
// Takes in number of calls to make to measured fn
type BenchFn = fn(usize) -> Result<u64, BenchError>;

#[derive(Copy, Clone)]
pub(crate) struct BenchFnWrapper {
    pub(crate) func: BenchFn,
    pub(crate) name: &'static str,
}

pub static BENCH_FNS: [(BenchFnWrapper, usize); 1] =
    [(BenchFnWrapper { func: bench_connect_controller, name: "bench_connect_controller" }, 500)];
