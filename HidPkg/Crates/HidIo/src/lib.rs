//! ## Summary
//!  This protocol provides access to HID devices.
//!
//! ## License
//! Copyright (c) Microsoft Corporation. All rights reserved.
//! SPDX-License-Identifier: BSD-2-Clause-Patent
//!
#![no_std]

pub mod protocol {
  use core::ffi::c_void;

  use r_efi::efi::{Guid, Status};

  /// HidIo interface GUID: 3EA93936-6BF4-49D6-AA50-D9F5B9AD8CFF
  pub const GUID: Guid =
    Guid::from_fields(0x3ea93936, 0x6bf4, 0x49d6, 0xaa, 0x50, &[0xd9, 0xf5, 0xb9, 0xad, 0x8c, 0xff]);

  #[derive(Debug, PartialEq, Eq, Clone, Copy)]
  #[repr(C)]
  pub enum HidReportType {
    InputReport = 1,
    OutputReport = 2,
    Feature = 3,
  }

  /// Retrieve the HID Report Descriptor from the device.
  ///
  /// # Arguments
  ///
  /// * `this` - A pointer to the HidIo Instance
  /// * `report_descriptor_size` - On input, the size of the buffer allocated to hold the descriptor. On output, the
  ///                              actual size of the descriptor. May be set to zero to query the required size for the
  ///                              descriptor.
  /// * `report_descriptor_buffer` - A pointer to the buffer to hold the descriptor. May be NULL if ReportDescriptorSize
  ///                                is zero.
  /// # Return values
  /// * `Status::SUCCESS` - Report descriptor successfully returned.
  /// * `Status::BUFFER_TOO_SMALL` - The provided buffer is not large enough to hold the descriptor.
  /// * `Status::INVALID_PARAMETER` - Invalid input parameters.
  /// * `Status::NOT_FOUND` - The device does not have a report descriptor.
  /// * Other - Unexpected error reading descriptor.
  ///
  pub type HidIoGetReportDescriptor = extern "efiapi" fn(
    this: *const Protocol,
    report_descriptor_size: *mut usize,
    report_descriptor_buffer: *mut c_void,
  ) -> Status;

  /// Retrieves a single report from the device.
  ///
  /// # Arguments
  ///
  /// * `this` - A pointer to the HidIo Instance
  /// * `report_id` - Specifies which report to return if the device supports multiple input reports. Set to zero if
  ///                 ReportId is not present.
  /// * `report_type` - Indicates the type of report type to retrieve. 1-Input, 3-Feature.
  /// * `report_buffer_size` - Indicates the size of the provided buffer to receive the report.
  /// * `report_buffer` - Pointer to the buffer to receive the report.
  ///
  /// # Return values
  /// * `Status::SUCCESS` - Report successfully returned.
  /// * `Status::OUT_OF_RESOURCES` - The provided buffer is not large enough to hold the report.
  /// * `Status::INVALID_PARAMETER` - Invalid input parameters.
  /// * Other - Unexpected error reading report.
  ///
  pub type HidIoGetReport = extern "efiapi" fn(
    this: *const Protocol,
    report_id: u8,
    report_type: HidReportType,
    report_buffer_size: usize,
    report_buffer: *mut c_void,
  ) -> Status;

  /// Sends a single report to the device.
  ///
  /// # Arguments
  ///
  /// * `this` - A pointer to the HidIo Instance
  /// * `report_id` - Specifies which report to send if the device supports multiple input reports. Set to zero if
  ///                 ReportId is not present.
  /// * `report_type` - Indicates the type of report type to retrieve. 2-Output, 3-Feature.
  /// * `report_buffer_size` - Indicates the size of the provided buffer holding the report to send.
  /// * `report_buffer` - Pointer to the buffer holding the report to send.
  ///
  /// # Return values
  /// * `Status::SUCCESS` - Report successfully transmitted.
  /// * `Status::INVALID_PARAMETER` - Invalid input parameters.
  /// * Other - Unexpected error transmitting report.
  ///
  pub type HidIoSetReport = extern "efiapi" fn(
    this: *const Protocol,
    report_id: u8,
    report_type: HidReportType,
    report_buffer_size: usize,
    report_buffer: *mut c_void,
  ) -> Status;

  /// Report received callback function.
  ///
  /// # Arguments
  ///
  /// * `report_buffer_size` - Indicates the size of the provided buffer holding the received report.
  /// * `report_buffer` - Pointer to the buffer holding the report.
  /// * `context` - Context provided when the callback was registered.
  ///
  pub type HidIoReportCallback =
    extern "efiapi" fn(report_buffer_size: u16, report_buffer: *mut c_void, context: *mut c_void);

  /// Registers a callback function to receive asynchronous input reports from the device. The device driver will do any
  /// necessary initialization to configure the device to send reports.
  ///
  /// # Arguments
  ///
  /// * `this` - A pointer to the HidIo Instance
  /// * `callback` - Callback function to handle reports as they are received.
  /// * `context` - Context that will be provided to the callback function.
  ///
  /// # Return values
  /// * `Status::SUCCESS` - Callback successfully registered.
  /// * `Status::INVALID_PARAMETER` - Invalid input parameters.
  /// * `Status::ALREADY_STARTED` - Callback function is already registered.
  /// * Other - Unexpected error registering callback or initiating report generation from device.
  ///
  pub type HidIoRegisterReportCallback =
    extern "efiapi" fn(this: *const Protocol, callback: HidIoReportCallback, context: *mut c_void) -> Status;

  /// Unregisters a previously registered callback function. The device driver will do any necessary initialization to
  /// configure the device to stop sending reports.
  ///
  /// # Arguments
  ///
  /// * `this` - A pointer to the HidIo Instance
  /// * `callback` - Callback function to unregister.
  ///
  /// # Return values
  /// * `Status::SUCCESS` - Callback successfully unregistered.
  /// * `Status::INVALID_PARAMETER` - Invalid input parameters.
  /// * `Status::NOT_STARTED` - Callback function was not previously registered.
  /// * Other - Unexpected error unregistering report or disabling report generation from device.
  ///
  pub type HidIoUnregisterReportCallback =
    extern "efiapi" fn(this: *const Protocol, callback: HidIoReportCallback) -> Status;

  /// The HID_IO protocol provides a set of services for interacting with a HID device.
  #[repr(C)]
  pub struct Protocol {
    pub get_report_descriptor: HidIoGetReportDescriptor,
    pub get_report: HidIoGetReport,
    pub set_report: HidIoSetReport,
    pub register_report_callback: HidIoRegisterReportCallback,
    pub unregister_report_callback: HidIoUnregisterReportCallback,
  }
}
