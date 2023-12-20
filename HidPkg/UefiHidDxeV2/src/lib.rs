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

pub mod boot_services;
pub mod driver_binding;
pub mod hid;
pub mod hid_io;
pub mod keyboard;
pub mod pointer;

use boot_services::StandardUefiBootServices;
use core::sync::atomic::AtomicPtr;
use r_efi::efi;

/// Global instance of UEFI Boot Services.
pub static BOOT_SERVICES: StandardUefiBootServices = StandardUefiBootServices::new();

/// Global instance of UEFI Runtime Services.
pub static RUNTIME_SERVICES: AtomicPtr<efi::RuntimeServices> = AtomicPtr::new(core::ptr::null_mut());
