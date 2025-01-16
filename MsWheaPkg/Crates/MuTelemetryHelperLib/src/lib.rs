//! Rust MU Telemetry Helper
//!
//! Rust helper library for logging telemetry.
//!
//! ## Examples and Usage
//!
//! ```no_run
//! use mu_telemetry_helper_lib::{init_telemetry, log_telemetry};
//! use r_efi::{efi, system};
//! pub extern "efiapi" fn efi_main(
//!     _image_handle: efi::Handle,
//!     system_table: *const system::SystemTable,
//!  ) -> efi::Status {
//!
//!    //Initialize Boot Services
//!    unsafe {
//!        init_telemetry((*system_table).boot_services.as_ref().unwrap());
//!    }
//!
//!    //if (some_failure) {
//!        let _ = log_telemetry(false, 0xA1A2A3A4, 0xB1B2B3B4B5B6B7B8, 0xC1C2C3C4C5C6C7C8, None, None, None);
//!    //}
//!
//!    efi::Status::SUCCESS
//! }
//! ```
//!
//! ## License
//!
//! Copyright (C) Microsoft Corporation.

//!
//! SPDX-License-Identifier: BSD-2-Clause-Patent
//!
#![cfg_attr(target_os = "uefi", no_std)]

mod status_code_runtime;

use boot_services::{BootServices, StandardBootServices};
use mu_pi::{
    protocols::status_code::{EfiStatusCodeType, EfiStatusCodeValue},
    status_code::{EFI_ERROR_CODE, EFI_ERROR_MAJOR, EFI_ERROR_MINOR},
};
use mu_rust_helpers::{guid, guid::guid};
use r_efi::efi;
use status_code_runtime::{ReportStatusCode, StatusCodeRuntimeProtocol};

static BOOT_SERVICES: StandardBootServices = StandardBootServices::new_uninit();

/// Matches gMsWheaRSCDataTypeGuid in MsWheaPkg\MsWheaPkg.dec
/// Matches MS_WHEA_RSC_DATA_TYPE in MsWheaPkg\Private\Guid\MsWheaReportDataType.h
const MS_WHEA_RSC_DATA_TYPE_GUID: efi::Guid = guid!("91DEEA05-8C0A-4DCD-B91E-F21CA0C68405");

const MS_WHEA_ERROR_STATUS_TYPE_INFO: EfiStatusCodeType = EFI_ERROR_MINOR | EFI_ERROR_CODE;
const MS_WHEA_ERROR_STATUS_TYPE_FATAL: EfiStatusCodeType = EFI_ERROR_MAJOR | EFI_ERROR_CODE;

/**
 Internal RSC Extended Data Buffer format used by Project Mu firmware WHEA infrastructure.

 A Buffer of this format should be passed to ReportStatusCodeWithExtendedData

 library_id:        GUID of the library reporting the error. If not from a library use zero guid
 ihv_sharing_guid:  GUID of the partner to share this with. If none use zero guid
 additional_info1:  64 bit value used for caller to include necessary interrogative information
 additional_info2:  64 bit value used for caller to include necessary interrogative information
**/
// #pragma pack(1)
// typedef struct {
//     EFI_GUID    LibraryID;
//     EFI_GUID    IhvSharingGuid;
//     UINT64      AdditionalInfo1;
//     UINT64      AdditionalInfo2;
//   } MS_WHEA_RSC_INTERNAL_ERROR_DATA;
// #pragma pack()

#[repr(C)]
struct MsWheaRscInternalErrorData {
    library_id: efi::Guid,
    ihv_sharing_guid: efi::Guid,
    additional_info1: u64,
    additional_info2: u64,
}

/// Log telemetry
///
///   @param[in]  is_fatal      This should be set to TRUE if the event will prevent a successful boot.
///   @param[in]  class_id      An EFI_STATUS_CODE_VALUE representing the event that has occurred. This
///                             value will occupy the same space as EventId from LogCriticalEvent(), and
///                             should be unique enough to identify a module or region of code.
///   @param[in]  extra_data1   [Optional] This should be data specific to the cause. Ideally, used to contain contextual
///                             or runtime data related to the event (e.g. register contents, failure codes, etc.).
///                             It will be persisted.
///   @param[in]  extra_data2   [Optional] Another UINT64 similar to ExtraData1.
///   @param[in]  component_id  [Optional] This identifier should uniquely identify the module that is emitting this
///                             event. When this is passed in as NULL, report status code will automatically populate
///                             this field with gEfiCallerIdGuid.
///   @param[in]  library_id    This should identify the library that is emitting this event.
///   @param[in]  ihv_id        This should identify the Ihv related to this event if applicable. For example,
///                             this would typically be used for TPM and SOC specific events.
#[cfg(not(tarpaulin_include))]
pub fn log_telemetry(
    is_fatal: bool,
    class_id: EfiStatusCodeValue,
    extra_data1: u64,
    extra_data2: u64,
    component_id: Option<&efi::Guid>,
    library_id: Option<&efi::Guid>,
    ihv_id: Option<&efi::Guid>,
) -> Result<(), efi::Status> {
    log_telemetry_internal(
        &BOOT_SERVICES,
        is_fatal,
        class_id,
        extra_data1,
        extra_data2,
        component_id,
        library_id,
        ihv_id,
    )
}

fn log_telemetry_internal<B: BootServices>(
    boot_services: &B,
    is_fatal: bool,
    class_id: EfiStatusCodeValue,
    extra_data1: u64,
    extra_data2: u64,
    component_id: Option<&efi::Guid>,
    library_id: Option<&efi::Guid>,
    ihv_id: Option<&efi::Guid>,
) -> Result<(), efi::Status> {
    let status_code_type: EfiStatusCodeType =
        if is_fatal { MS_WHEA_ERROR_STATUS_TYPE_FATAL } else { MS_WHEA_ERROR_STATUS_TYPE_INFO };

    let error_data = MsWheaRscInternalErrorData {
        library_id: *library_id.unwrap_or(&guid::ZERO),
        ihv_sharing_guid: *ihv_id.unwrap_or(&guid::ZERO),
        additional_info1: extra_data1,
        additional_info2: extra_data2,
    };

    StatusCodeRuntimeProtocol::report_status_code(
        boot_services,
        status_code_type,
        class_id,
        0,
        component_id,
        MS_WHEA_RSC_DATA_TYPE_GUID,
        error_data,
    )
}

#[cfg(not(tarpaulin_include))]
pub fn init_telemetry(efi_boot_services: &efi::BootServices) {
    BOOT_SERVICES.initialize(efi_boot_services)
}

#[cfg(test)]
mod test {
    use boot_services::MockBootServices;
    use mu_pi::protocols::{
        status_code,
        status_code::{EfiStatusCodeData, EfiStatusCodeType, EfiStatusCodeValue},
    };
    use mu_rust_helpers::guid::guid;
    use r_efi::efi;

    use crate::{
        log_telemetry_internal, status_code_runtime::StatusCodeRuntimeProtocol, MsWheaRscInternalErrorData,
        MS_WHEA_ERROR_STATUS_TYPE_FATAL,
    };
    use core::mem::size_of;

    const DATA_SIZE: usize = size_of::<EfiStatusCodeData>() + size_of::<MsWheaRscInternalErrorData>();
    const MOCK_CALLER_ID: efi::Guid = guid!("d0d1d2d3-d4d5-d6d7-d8d9-dadbdcdddedf");
    const MOCK_STATUS_CODE_VALUE: EfiStatusCodeValue = 0xa0a1a2a3;

    extern "efiapi" fn mock_report_status_code(
        r#type: EfiStatusCodeType,
        value: EfiStatusCodeValue,
        instance: u32,
        caller_id: *const efi::Guid,     // Optional
        _data: *const EfiStatusCodeData, // Optional
    ) -> efi::Status {
        assert_eq!(value, MOCK_STATUS_CODE_VALUE);
        assert_eq!(instance, 0);
        assert_eq!(unsafe { *caller_id }, MOCK_CALLER_ID);
        if r#type == MS_WHEA_ERROR_STATUS_TYPE_FATAL {
            efi::Status::SUCCESS
        } else {
            efi::Status::INVALID_PARAMETER
        }
    }

    static MOCK_STATUS_CODE_RUNTIME_INTERFACE: status_code::Protocol =
        status_code::Protocol { report_status_code: mock_report_status_code };

    #[test]
    fn try_log_telemetry() {
        let mut mock_boot_services: MockBootServices = MockBootServices::new();

        mock_boot_services.expect_locate_protocol().returning(|_: &StatusCodeRuntimeProtocol, registration| unsafe {
            assert_eq!(registration, None);
            Ok((&MOCK_STATUS_CODE_RUNTIME_INTERFACE as *const status_code::Protocol as *mut status_code::Protocol)
                .as_mut()
                .unwrap())
        });

        // Test sizes of "repr(C)" structs
        assert_eq!(size_of::<MsWheaRscInternalErrorData>(), 48);
        assert_eq!(size_of::<[u8; 68]>(), DATA_SIZE);

        // Test Deref trait
        assert_eq!(*StatusCodeRuntimeProtocol, status_code::PROTOCOL_GUID);
        assert_eq!(
            Ok(()),
            log_telemetry_internal(
                &mock_boot_services,
                true,
                MOCK_STATUS_CODE_VALUE,
                0xb0b1b2b3b4b5b6b7,
                0xc0c1c2c3c4c5c6c7,
                Some(&MOCK_CALLER_ID),
                Some(&guid!("e0e1e2e3-e4e5-e6e7-e8e9-eaebecedeeef")),
                Some(&guid!("f0f1f2f3-f4f5-f6f7-f8f9-fafbfcfdfeff"))
            )
        );
        assert_eq!(
            Err(efi::Status::INVALID_PARAMETER),
            log_telemetry_internal(
                &mock_boot_services,
                false,
                MOCK_STATUS_CODE_VALUE,
                0xb0b1b2b3b4b5b6b7,
                0xc0c1c2c3c4c5c6c7,
                Some(&MOCK_CALLER_ID),
                None,
                None
            )
        );
    }

    #[test]
    fn test_protocol_not_found() {
        let mut mock_boot_services: MockBootServices = MockBootServices::new();

        mock_boot_services.expect_locate_protocol().returning(|_: &StatusCodeRuntimeProtocol, registration| {
            assert_eq!(registration, None);
            //Simulate "marker protocol" without an Interface
            Err(efi::Status::NOT_FOUND)
        });
        assert_eq!(
            Err(efi::Status::NOT_FOUND),
            log_telemetry_internal(
                &mock_boot_services,
                false,
                MOCK_STATUS_CODE_VALUE,
                0xb0b1b2b3b4b5b6b7,
                0xc0c1c2c3c4c5c6c7,
                Some(&MOCK_CALLER_ID),
                None,
                None
            )
        );
    }
}
