# TPM Replay Event Log Documentation

The TPM Replay Event Log schema represents information that maps to a TPM event log.

## `events` (Array)

An array of event objects in the TPM event log.

- **Type**: Array of objects
- **Required**: Yes

## Event Object

### `type` (String)

The Event Log type. It's based on the TCG PC Client Platform Firmware Profile Specification.

- **Type**: String
- **Required**: Yes

#### Possible Event Types

- **EV_POST_CODE**: PCR[0] only. Typically used to log BIOS POST code, embedded SMM code, ACPI flash data, BIS code, or
  manufacturer-controlled embedded option ROMs as a binary image.
- **EV_NO_ACTION**: PCRs[0,6]. Used for informative data that is not extended into a PCR.
- **EV_SEPARATOR**: PCRs[0,1,2,3,4,5,6,7]. Used to mark an error or pre-OS to OS transition.
- **EV_ACTION**: PCRs[1,2,3,4,5,6]. Signifies that a certain action measured as a string has been performed.
- **EV_EVENT_TAG**: Defined for Host OS and SW usage. An event tag indicating a particular type or category of event.
- **EV_S_CRTM_CONTENTS**: PCR[0] only. The contents of the Core Root of Trust for Measurement (CRTM).
- **EV_S_CRTM_VERSION**: PCR[0] only. The version of the Core Root of Trust for Measurement (CRTM).
- **EV_CPU_MICROCODE**: PCR[1] only. CPU microcode.
- **EV_PLATFORM_CONFIG_FLAGS**: PCR[1] only. Flags indicating the configuration of the platform.
- **EV_TABLE_OF_DEVICES**: PCR[1] only. List or table of devices present in the system.
- **EV_COMPACT_HASH**: Used for PCRs other than [0,1,2,3]. A compact representation of a hash value.
- **EV_IPL**: Deprecated. Initial Program Load event, often related to the boot process.
- **EV_IPL_PARTITION_DATA**: Deprecated. Data related to an Initial Program Load (IPL) partition.
- **EV_NONHOST_CODE**: PCRs[0,2]. Code that is not part of the host system, such as firmware or embedded code.
- **EV_NONHOST_CONFIG**: PCRs[1,3]. Configuration data for non-host code.
- **EV_NONHOST_INFO**: PCR[0] only. Information or metadata about non-host code.
- **EV_OMIT_BOOT_DEVICE_EVENTS**: PCR[4] only. Indicator that events from boot devices are omitted.
- **EV_EFI_EVENT_BASE**: Base value for all UEFI platform event types.
- **EV_EFI_VARIABLE_DRIVER_CONFIG**: PCRs[1,3,5,7]. Used to measure configuration for UEFI variables.
- **EV_EFI_VARIABLE_BOOT**: PCR[1] only. Deprecated. Measures a UEFI boot variable.
- **EV_EFI_BOOT_SERVICES_APPLICATION**: PCRs[2,4] only. Contains the tagged hash of the normalized code from the loaded
  UEFI application.
- **EV_EFI_BOOT_SERVICES_DRIVER**: PCRs[0,2] only. Contains the tagged hash of the normalized code from the loaded UEFI
  Boot Services driver.
- **EV_EFI_RUNTIME_SERVICES_DRIVER**: PCRs[0,2] only. Contains the tagged hash of the normalized code from the loaded
  UEFI Runtime Services driver.
- **EV_EFI_GPT_EVENT**: PCR[5] only. Measures information about the UEFI GUID Partition Table (GPT).
- **EV_EFI_ACTION**: PCRs[1,2,3,4,5,6,7]. An EFI event related to a specific action taken by the EFI firmware.
- **EV_EFI_PLATFORM_FIRMWARE_BLOB**: PCRs[0,2,4]. Deprecated. An EFI event related to a platform firmware blob.
- **EV_EFI_HANDOFF_TABLES**: PCR[1] only. Deprecated. An EFI event related to handoff tables between different
  components.
- **EV_EFI_PLATFORM_FIRMWARE_BLOB2**: PCRs[0,2,4]. Measures information about non-PE/COFF images. Allows for
  non-PE/COFF images in PCR[2] or PCR[4].
- **EV_EFI_HANDOFF_TABLES2**: PCR[1] only. System configuration tables referenced by entries in
  UEFI_HANDOFF_TABLE_POINTERS2 for each PCR bank.
- **EV_EFI_VARIABLE_BOOT2**: PCR[1] only. Used to measure configuration for UEFI variables including the UEFI Boot####
  variables and the BootOrder variable.
- **EV_EFI_HCRTM_EVENT**: PCR[0] only. Records an event for the digest extended to PCR[0] as part of an H-CRTM event.
- **EV_EFI_VARIABLE_AUTHORITY**: PCR[7] only. Describes the Secure Boot UEFI variables. Digest fields contain the
  tagged hash of the UEFI_SIGNATURE_DATA value from UEFI_SIGNATURE_LIST used to validate the loaded image for each PCR
  bank.
- **EV_EFI_SPDM_FIRMWARE_BLOB**: PCR[2] only. Records an extended digest for the embedded firmware of a device that
  supports SPDT GET_MEASUREMENTS functionality.
- **EV_EFI_SPDM_FIRMWARE_CONFIG**: PCR[3] only. Records an extended digest of the configuration of embedded firmware
  for an add-in device that supports SPDM GET_MEASUREMENTS functionality.

### `pcr` (Integer)

The PCR index for this event. PCRs 0-7 are allowed.

- **Type**: Integer
- **Range**: 0 to 7
- **Required**: Yes

### `description` (String)

An optional informational comment about this event entry. Used only for documentation in the YAML/JSON file.

- **Type**: String
- **Required**: No

### `data` (Object)

Information about the event data.

- **Type**: Object
- **Required**: Yes

#### `data` - `type` (String)

The event data type. Either a string, a base64 encoded value, or UEFI variable data.

- **For Strings**:

  - `include_null_char`: (Optional, Boolean) Indicates a null character should be appended to the string.
    Default is `false`.
  - `encoding`: (Optional, String) Specifies the encoding to use for the data string. Default is `utf-8` and can be
    either `"utf-8"` or `"utf-16"`.

- **For Base64**: Base64 encoded values should be used for binary data.

- **For UEFI variables**:

  - `variable_name`: (Required, String) The UEFI vendor GUID (name in TCG specifications). In the following format:
      `{0xD719B2CB, 0x3D3A, 0x4596, {0xA3, 0xBC, 0xDA, 0xD0, 0x0E, 0x67, 0x65, 0x6F}}`
  - `variable_unicode_name_length`: (Required, Integer) The number of Unicode characters in the
    `variable_unicode_name` value.
  - `variable_data_length`: (Required, Integer) The size in bytes of the data in the `value` data.
  - `variable_unicode_name`: (Required, String) The UEFI variable name string (actual variable name). This is passed
    as a normal string and converted to UTF-16 during encoding in the script.

- **Required**: Yes

#### `data` - `value` (String)

The actual event data in string (or base64) representation.

- **Type**: String
- **Required**: Yes

### `prehash` (Object)

Specifies the pre-hashed values for chosen (and supported) algorithms.

- **sha1**: Must be a SHA-1 length string that begins with `'0x'`.
- **sha256**: Must be a SHA-256 length string that begins with `'0x'`.
- **sha384**: Must be a SHA-384 length string that begins with `'0x'`.

### `hash` (Array)

An array of algorithms that should be used to hash the given event data.

- **Type**: Array of strings
- **Possible Values**: `"sha1"`, `"sha256"`, `"sha384"`
- **Required**: Yes, if `"prehash"` is not provided.

## Schema Constraints

- `events` is a required field.
- Each event object must have at least one of the `"hash"` or `"prehash"` fields, but not both.
- No additional properties are allowed in the main object or the event object that are not defined in the schema.
