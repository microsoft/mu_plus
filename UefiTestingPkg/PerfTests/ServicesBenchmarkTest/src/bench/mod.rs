use r_efi::efi;

pub(crate) mod controller;
pub(crate) mod event;
pub(crate) mod image;
pub(crate) mod memory;
pub(crate) mod misc;
pub(crate) mod protocol;
pub(crate) mod tpl;

/// Some static test guids for protocols.
const TEST_GUID1: efi::Guid =
    efi::Guid::from_fields(0x12345678, 0x1234, 0x5678, 0x9a, 0xbc, &[0xde, 0xf0, 0x12, 0x34, 0x56, 0x78]);
const TEST_GUID2: efi::Guid =
    efi::Guid::from_fields(0x87654321, 0x4321, 0x8765, 0xba, 0x98, &[0x76, 0x54, 0x32, 0x10, 0xfe, 0xdc]);
