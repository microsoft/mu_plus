//! Keyboard support for HID input driver.
//!
//! This module manages keyboard input for the HID input driver.
//!
//! ## License
//!
//! Copyright (c) Microsoft Corporation. All rights reserved.
//! SPDX-License-Identifier: BSD-2-Clause-Patent
//!

use hidparser::ReportDescriptor;
use r_efi::{efi, protocols::driver_binding};

use crate::hid::HidContext;

pub struct KeyboardContext {
  pub handler: KeyboardHandler,
}

pub struct KeyboardHandler {}

impl KeyboardHandler {
  pub fn process_input_report(&mut self, _report_buffer: &[u8]) {
    todo!()
  }
}

pub fn initialize(
  _controller: efi::Handle,
  _descriptor: &ReportDescriptor,
  _hid_context: *mut HidContext,
) -> Result<*mut KeyboardContext, efi::Status> {
  Err(efi::Status::UNSUPPORTED)
}

pub fn deinitialize(_context: *mut KeyboardContext) -> Result<(), efi::Status> {
  Err(efi::Status::UNSUPPORTED)
}

pub fn attempt_to_retrieve_hid_context(
  _controller: efi::Handle,
  _driver_binding: &driver_binding::Protocol,
) -> Result<*mut HidContext, efi::Status> {
  Err(efi::Status::UNSUPPORTED)
}
