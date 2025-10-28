use r_efi::efi;
use rolling_stats::Stats;

use crate::{
    bench::{
        controller::bench_connect_controller,
        event::{bench_check_event, bench_close_event, bench_create_event, bench_signal_event},
        image::{bench_load_image, bench_start_image_and_exit},
        memory::{
            bench_allocate_pages, bench_allocate_pool, bench_copy_mem, bench_free_pages, bench_free_pool,
            bench_get_memory_map, bench_set_mem,
        },
        misc::{bench_calculate_crc32, bench_install_configuration_table},
        protocol::{
            bench_close_protocol, bench_handle_protocol, bench_install_protocol_interface, bench_locate_device_path,
            bench_open_protocol, bench_open_protocol_information, bench_protocols_per_handle,
            bench_register_protocol_notify, bench_reinstall_protocol_interface, bench_uninstall_protocol_interface,
        },
        tpl::{bench_raise_tpl, bench_restore_tpl},
    },
    error::BenchError,
};

// A BenchFn returns total cycles for one call
// Takes in number of calls to make to measured fn
type BenchFn = fn(efi::Handle, usize) -> Result<Stats<f64>, BenchError>;

#[derive(Copy, Clone)]
pub(crate) struct BenchFnWrapper {
    pub(crate) func: BenchFn,
    pub(crate) name: &'static str,
}

pub static BENCH_FNS: [(BenchFnWrapper, usize); 28] = [
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
    (BenchFnWrapper { func: bench_copy_mem, name: "copy_mem" }, 10),
    (BenchFnWrapper { func: bench_set_mem, name: "set_mem" }, 10),
    (BenchFnWrapper { func: bench_get_memory_map, name: "get_memory_map" }, 10),
    /* MISC SERVICES */
    (BenchFnWrapper { func: bench_calculate_crc32, name: "calculate_crc32" }, 100),
    (BenchFnWrapper { func: bench_install_configuration_table, name: "install_configuration_table" }, 10),
    /* PROTOCOL SERVICES */
    (BenchFnWrapper { func: bench_install_protocol_interface, name: "install_protocol_interface" }, 100),
    (BenchFnWrapper { func: bench_open_protocol, name: "open_protocol" }, 10_000),
    (BenchFnWrapper { func: bench_handle_protocol, name: "handle_protocol" }, 10_000),
    (BenchFnWrapper { func: bench_close_protocol, name: "close_protocol" }, 100),
    (BenchFnWrapper { func: bench_locate_device_path, name: "locate_device_path" }, 100),
    (BenchFnWrapper { func: bench_open_protocol_information, name: "open_protocol_information" }, 100),
    (BenchFnWrapper { func: bench_protocols_per_handle, name: "protocols_per_handle" }, 100),
    (BenchFnWrapper { func: bench_register_protocol_notify, name: "register_protocol_notify" }, 10),
    (BenchFnWrapper { func: bench_reinstall_protocol_interface, name: "reinstall_protocol_interface" }, 100),
    (BenchFnWrapper { func: bench_uninstall_protocol_interface, name: "uninstall_protocol_interface" }, 10),
    (BenchFnWrapper { func: bench_raise_tpl, name: "raise_tpl" }, 1_000_000),
    (BenchFnWrapper { func: bench_restore_tpl, name: "restore_tpl" }, 1_000_000),
];
