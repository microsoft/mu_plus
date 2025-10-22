//! UefiHidDxe - Human Interface Device support.
//!
//! This crate provides a UEFI driver to support HID devices. At present, it has
//! support for pointer and keyboard devices. Devices are supported in Report
//! mode (as opposed to Boot mode) and the report descriptor is used to
//! inform the parsing of arbitrary input reports from the device.
//!
//! ## Usage
//!
//! To use this crate, a device must expose an instance of the HidIo protocol:
//! <https://github.com/microsoft/mu_plus/blob/14c187b8ac4858d154612cd67a96820f78fe5584/HidPkg/Include/Protocol/HidIo.h>
//!
//! This driver will use that interface to query device report descriptors and
//! instantiate handling for keyboard, pointer, or both as appropriate.
//!
//! ## License
//!
//! Copyright (c) Microsoft Corporation. All rights reserved.
//!
//! SPDX-License-Identifier: BSD-2-Clause-Patent
//!
#![cfg_attr(target_os = "uefi", no_std)]

extern crate alloc;
use alloc::vec::Vec;
use core::fmt::Write;

use patina_sdk::boot_services::StandardBootServices;
use r_efi::{efi, system};
use rust_advanced_logger_dxe::{DEBUG_ERROR, DEBUG_INFO, debugln};

use crate::{error::BenchError, measure::BENCH_FNS};
use alloc::string::String;

/// Global instance of UEFI Boot Services.
pub static BOOT_SERVICES: StandardBootServices = StandardBootServices::new_uninit();

pub fn bench_start(handle: efi::Handle, st: *const system::SystemTable) -> Result<(), BenchError> {
    debugln!(DEBUG_INFO, "Starting Services Benchmark Test...");

    let mut output_buf = String::new();

    // Write fixed-width markdown table
    writeln!(
        &mut output_buf,
        "| {:<30} | {:>14} | {:>12} | {:>15} |",
        "Name", "Total cycles", "Total calls", "Cycles/op"
    )
    .map_err(|e| BenchError::WriteFailure("Write table header failed", e))?;
    writeln!(&mut output_buf, "|{:-<28}|{:-<16}|{:-<14}|{:-<17}|", "-", "-", "-", "-")
        .map_err(|e| BenchError::WriteFailure("Write table header failed", e))?;

    for (bf, num_calls) in BENCH_FNS {
        let (bench_name, bench_func) = (bf.name, bf.func);
        let cycles_res = bench_func(handle, num_calls);
        match cycles_res {
            Ok(cycles) => {
                writeln!(
                    &mut output_buf,
                    "| {:<30} | {:>14} | {:>12} | {:>15} |",
                    bench_name,
                    cycles,
                    num_calls,
                    cycles / num_calls as u64
                )
                .map_err(|e| BenchError::WriteFailure("Write table header failed", e))?;
            }
            Err(e) => {
                debugln!(DEBUG_ERROR, "Benchmark {} failed: {:?}", bench_name, e);
                debug_assert!(false);
            }
        }
    }

    debugln!(DEBUG_INFO, "{}", output_buf);
    // SAFETY: `st` is a valid pointer to SystemTable provided by UEFI firmware in `efi_main`.
    unsafe { print_to_console(st, &output_buf.as_str()) };

    Ok(())
}

/// Print a message to the UEFI console output.
/// SAFETY: Caller must ensure that `system_table` is a valid pointer to a SystemTable.
pub unsafe fn print_to_console(system_table: *const system::SystemTable, message: &str) {
    // SAFETY: `system_table` is validated by the caller.
    let con_out = (unsafe { &*system_table }).con_out;
    // UEFI expects UCS-2 (UTF-16), not UTF-8.
    let mut wide: Vec<u16> = message.encode_utf16().chain(core::iter::once(0)).collect();
    // SAFETY: `system_table` is valid, so `con_out` is valid.
    ((unsafe { &*con_out }).output_string)(con_out, wide.as_mut_ptr());
}

mod bench_fn;
mod error;
mod measure;
