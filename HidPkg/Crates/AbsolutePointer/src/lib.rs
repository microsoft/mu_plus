//!
//! ## Summary
//! This module defines the Absolute Pointer protocol.
//!
//! Refer to UEFI spec version 2.10 section 12.7.
//!
//! ## License
//! Copyright (c) Microsoft Corporation. All rights reserved.
//! SPDX-License-Identifier: BSD-2-Clause-Patent
//!
//!
#![no_std]

pub mod protocol {
  use r_efi::efi::{Event, Guid, Status};

  /// Absolute pointer interface GUID: 8D59D32B-C655-4AE9-9B15-F25904992A43
  pub const PROTOCOL_GUID: Guid =
    Guid::from_fields(0x8D59D32B, 0xC655, 0x4AE9, 0x9B, 0x15, &[0xF2, 0x59, 0x04, 0x99, 0x2A, 0x43]);

  /// This function resets the pointer device hardware. As part of initialization process, the firmware/device will make
  /// a quick but reasonable attempt to verify that the device is functioning. If the ExtendedVerification flag is TRUE
  /// the firmware may take an extended amount of time to verify the device is operating on reset. Otherwise the reset
  /// operation is to occur as quickly as possible. The hardware verification process is not defined by this
  /// specification and is left up to the platform firmware or driver to implement.
  ///
  /// # Arguments
  /// * `this` - A pointer to the AbsolutePointer Instance
  /// * `extended_verification` - indicates whether extended reset is requested.
  ///
  /// # Return Values
  /// * `Status::SUCCESS` - The device was reset
  /// * `Status::DEVICE_ERROR` - The device is not functioning correctly and could not be reset.
  ///
  pub type AbsolutePointerReset = extern "efiapi" fn(this: *const Protocol, extended_verification: bool) -> Status;

  /// This function retrieves the current state of a pointer device. This includes information on the active state
  /// associated with the pointer device and the current position of the axes associated with the pointer device.
  /// If the state of the pointer device has not changed since the last call to GetState(), then EFI_NOT_READY is
  /// returned. If the state of the pointer device has changed since the last call to GetState(), then the state
  /// information is placed in State, and efi::Status::SUCCESS is returned. If a device error occurs while attempting to
  /// retrieve the state information, then efi::Status::DEVICE_ERROR is returned.
  ///
  /// # Arguments
  /// * `this` - A pointer to the AbsolutePointer Instance
  /// * `state` - A pointer to the state information on the pointer device.
  ///
  /// # Return Values
  /// * `Status::SUCCESS` - The state of the pointer device was returned in state.
  /// * `Status::NOT_READY` - The state of the pointer device has not changed since the last call to this function.
  /// * `Status::DEVICE_ERROR` - A device error occurred while attempting to retrieve the pointer device current state.
  ///
  pub type AbsolutePointerGetState =
    extern "efiapi" fn(this: *const Protocol, state: *mut AbsolutePointerState) -> Status;

  /// Describes the current state of the pointer.
  #[derive(Debug, Default, Clone, Copy)]
  #[repr(C)]
  pub struct AbsolutePointerState {
    /// The unsigned position of the activation on the x-axis. If the absolute_min_x and the absolute_max_x fields of
    /// the AbsolutePointerMode structure are both 0, then this pointer device does not support an x-axis, and this
    /// field must be ignored.
    pub current_x: u64,
    /// The unsigned position of the activation on the y-axis. If the absolute_min_y and the absolute_max_y fields of
    /// the AbsolutePointerMode structure are both 0, then this pointer device does not support an y-axis, and this
    /// field must be ignored.
    pub current_y: u64,
    /// The unsigned position of the activation on the z-axis. If the absolute_min_z and the absolute_max_z fields of
    /// the AbsolutePointerMode structure are both 0, then this pointer device does not support an z-axis, and this
    /// field must be ignored.
    pub current_z: u64,
    /// Bits are set to 1 in this field to indicate that device buttons are active.
    pub active_buttons: u32,
  }

  /// Describes the mode of the pointer.
  #[derive(Debug, Default, Clone, Copy)]
  #[repr(C)]
  pub struct AbsolutePointerMode {
    pub absolute_min_x: u64,
    /// The Absolute Minimum of the device on the x-axis
    pub absolute_min_y: u64,
    /// The Absolute Minimum of the device on the y-axis.
    pub absolute_min_z: u64,
    /// The Absolute Minimum of the device on the z-axis.
    /// The Absolute Maximum of the device on the x-axis. If 0, and absolute_min_x is 0, then x-axis is unsupported.
    pub absolute_max_x: u64,
    /// The Absolute Maximum of the device on the y-axis. If 0, and absolute_min_y is 0, then y-axis is unsupported.
    pub absolute_max_y: u64,
    /// The Absolute Maximum of the device on the z-axis. If 0, and absolute_min_z is 0, then z-axis is unsupported.
    pub absolute_max_z: u64,
    /// Supported device attributes.
    pub attributes: u32,
  }

  /// If set in [`AbsolutePointerMode::attributes`], indicates this device supports an alternate button input.
  pub const SUPPORTS_ALT_ACTIVE: u32 = 0x00000001;
  /// If set in [`AbsolutePointerMode::attributes`], indicates this device returns pressure data in current_z.
  pub const SUPPORTS_PRESSURE_AS_Z: u32 = 0x00000002;

  /// The EFI_ABSOLUTE_POINTER_PROTOCOL provides a set of services for a pointer device that can be used as an input
  /// device from an application written to this specification. The services include the ability to: reset the pointer
  /// device, retrieve the state of the pointer device, and retrieve the capabilities of the pointer device. The service
  /// also provides certain data items describing the device.
  #[derive(Debug)]
  #[repr(C)]
  pub struct Protocol {
    pub reset: AbsolutePointerReset,
    pub get_state: AbsolutePointerGetState,
    /// Event to use with WaitForEvent() to wait for input from the pointer device.
    pub wait_for_input: Event,
    pub mode: *mut AbsolutePointerMode,
  }
}
