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
use core::fmt::Write;

use patina_sdk::boot_services::StandardBootServices;
use rust_advanced_logger_dxe::{DEBUG_ERROR, DEBUG_INFO, debugln};

use crate::measure::BENCH_FNS;
use alloc::string::String;

/// Global instance of UEFI Boot Services.
pub static BOOT_SERVICES: StandardBootServices = StandardBootServices::new_uninit();

// sherry: current idea is to collect everything then dump it in a single go to shell using debugln!?
pub fn bench_start() {
    debugln!(DEBUG_INFO, "Starting Services Benchmark Test...");

    let mut output_buf = String::new();

    // Write fixed-width markdown table
    // SHERRY: figure out how to fix write issues
    writeln!(
        &mut output_buf,
        "| {:<24} | {:>14} | {:>12} | {:>15} |",
        "Name", "Total cycles", "Total calls", "Cycles/op"
    );
    writeln!(&mut output_buf, "|{:-<26}|{:-<16}|{:-<14}|{:-<17}|", "-", "-", "-", "-");

    for (bf, num_calls) in BENCH_FNS {
        let (bench_name, bench_func) = (bf.name, bf.func);
        let cycles_res = bench_func(num_calls);
        match cycles_res {
            Ok(cycles) => {
                writeln!(
                    &mut output_buf,
                    "| {:<24} | {:>14} | {:>12} | {:>15} |",
                    bench_name,
                    cycles,
                    num_calls,
                    cycles / num_calls as u64
                );
            }
            Err(e) => {
                debugln!(DEBUG_ERROR, "Benchmark {} failed: {:?}", bench_name, e);
            }
        }
    }

    debugln!(DEBUG_INFO, "{}", output_buf);
}

mod bench_fn;
mod error;
mod measure;
