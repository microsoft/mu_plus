use mu_rust_helpers::perf_timer::{Arch, ArchFunctionality};
use r_efi::efi;

use crate::{
    bench_fn::{
        bench_allocate_pages, bench_allocate_pool, bench_calculate_crc32, bench_check_event, bench_close_event,
        bench_connect_controller, bench_copy_mem, bench_create_event, bench_free_pages, bench_free_pool,
        bench_get_memory_map, bench_install_configuration_table, bench_install_protocol_interface, bench_load_image,
        bench_open_protocol, bench_set_mem, bench_signal_event, bench_start_image_and_exit,
    },
    error::BenchError,
};

// A BenchFn returns total cycles for one call
// Takes in number of calls to make to measured fn
type BenchFn = fn(efi::Handle, usize) -> Result<u64, BenchError>;

#[derive(Copy, Clone)]
pub(crate) struct BenchFnWrapper {
    pub(crate) func: BenchFn,
    pub(crate) name: &'static str,
}

pub static BENCH_FNS: [(BenchFnWrapper, usize); 18] = [
    /* CONTROLLER SERVICES */
    (BenchFnWrapper { func: bench_connect_controller, name: "connect_controller" }, 100),
    /* EVENT SERVICES */
    (BenchFnWrapper { func: bench_check_event, name: "check_event" }, 10_000),
    (BenchFnWrapper { func: bench_create_event, name: "create_event" }, 1000),
    (BenchFnWrapper { func: bench_close_event, name: "close_event" }, 1000),
    (BenchFnWrapper { func: bench_signal_event, name: "signal_event" }, 100_000),
    /* IMAGE SERVICES */
    (BenchFnWrapper { func: bench_start_image_and_exit, name: "start_image, exit" }, 100),
    (BenchFnWrapper { func: bench_load_image, name: "load_image" }, 100),
    /* MEMORY SERVICES */
    (BenchFnWrapper { func: bench_allocate_pages, name: "allocate_pages" }, 1000),
    (BenchFnWrapper { func: bench_allocate_pool, name: "allocate_pool" }, 10_000),
    (BenchFnWrapper { func: bench_free_pages, name: "free_pages" }, 100),
    (BenchFnWrapper { func: bench_free_pool, name: "free_pool" }, 10_000),
    (BenchFnWrapper { func: bench_copy_mem, name: "copy_mem" }, 1), // i don't see these two being called much so probably not useful to bench
    (BenchFnWrapper { func: bench_set_mem, name: "set_mem" }, 1),
    (BenchFnWrapper { func: bench_get_memory_map, name: "get_memory_map" }, 10),
    /* MISC SERVICES */
    (BenchFnWrapper { func: bench_calculate_crc32, name: "calculate_crc32" }, 100),
    (BenchFnWrapper { func: bench_install_configuration_table, name: "install_configuration_table" }, 10),
    /* PROTOCOL SERVICES */
    (BenchFnWrapper { func: bench_install_protocol_interface, name: "install_protocol_interface" }, 100),
    (BenchFnWrapper { func: bench_open_protocol, name: "open_protocol" }, 10_000),
];
