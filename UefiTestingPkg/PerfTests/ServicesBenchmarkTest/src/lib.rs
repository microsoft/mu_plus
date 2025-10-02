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
use patina_sdk::boot_services::StandardBootServices;
use rust_advanced_logger_dxe::{DEBUG_ERROR, debugln};

use crate::measure::BENCH_FNS;

/// Global instance of UEFI Boot Services.
pub static BOOT_SERVICES: StandardBootServices = StandardBootServices::new_uninit();

pub fn bench_start() {
    for bf in BENCH_FNS {
        let cycles = measure::measure_single_fn(bf);
        debugln!(DEBUG_ERROR, "Cycles: {}", cycles); // sherry: this shoudl be logged to a file
    }
}

mod bench_fn;
mod measure;
