/** @file -- MsWheaEarlyStorageLib.h

This header defines APIs to utilize special memory for MsWheaReport during early stage.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __MS_WHEA_EARLY_STORAGE_LIB__
#define __MS_WHEA_EARLY_STORAGE_LIB__

#pragma pack(1)

/**

 Header of early storage, reserved for area sanity check, storage full check, etc.

 Signature:         Needs to be MS_WHEA_EARLY_STORAGE_SIGNATURE to indicate this is valid.
 IsStorageFull:     Indicator whether this early storage is full or not, if full, a error
                    report will be generated.
 FullPhase:         Phase of boot process when early storage is full.

**/
typedef struct _MS_WHEA_EARLY_STORAGE_HEADER {
  UINT32                              Signature;
  UINT32                              ActiveRange;
  UINT8                               IsStorageFull;
  UINT8                               FullPhase;
  UINT16                              Checksum;
  UINT32                              Reserved;
} MS_WHEA_EARLY_STORAGE_HEADER;

/**

 Minimal information reported for reported status codes under Rev 0 and fatal severity

 Rev:               Revision used for parser to identify supplied payload format.
 Phase:             Phase of boot process reporting module, will be filled at the backend.
 ErrorStatusValue:  Reported Status Code Value upon calling ReportStatusCode*
 AdditionalInfo1:   Critical information to be filled by caller
 AdditionalInfo2:   AdditionalInfo2 to be filled by caller, same usage as AdditionalInfo1
 PartitionID:       IHV Guid for the party reporting this error
 ModuleID:          Driver Guid that reports this error

**/
typedef struct MS_WHEA_EARLY_STORAGE_ENTRY_V0_T_DEF {
  UINT8                               Rev;
  UINT8                               Phase;
  UINT16                              Reserved;
  UINT32                              ErrorStatusValue;
  UINT64                              AdditionalInfo1;
  UINT64                              AdditionalInfo2;
  EFI_GUID                            ModuleID;
  EFI_GUID                            PartitionID;
} MS_WHEA_EARLY_STORAGE_ENTRY_V0, MS_WHEA_EARLY_STORAGE_ENTRY_COMMON;

#pragma pack()

/**

This routine returns the maximum number of bytes that can be stored in the early storage area.

@retval Count    The maximum number of bytes that can be stored in the MS WHEA store.

**/
UINT8
EFIAPI
MsWheaEarlyStorageGetMaxSize (
  VOID
);

/**

This routine reads the specified data region from the MS WHEA store.

@param[in]  Ptr                       The pointer to hold intended read data
@param[in]  Size                      The size of intended read data
@param[in]  Offset                    The offset of read data, ranging from 0 to PcdMsWheaReportEarlyStorageCapacity

@retval EFI_SUCCESS                   Operation is successful
@retval EFI_INVALID_PARAMETER         Null pointer or zero or over length request detected

**/
EFI_STATUS
EFIAPI
MsWheaEarlyStorageRead (
  VOID                                *Ptr,
  UINT8                               Size,
  UINT8                               Offset
);

/**

This routine writes the specified data region from the MS WHEA store.

@param[in]  Ptr                       The pointer to hold intended written data
@param[in]  Size                      The size of intended written data
@param[in]  Offset                    The offset of written data, ranging from 0 to PcdMsWheaReportEarlyStorageCapacity

@retval EFI_SUCCESS                   Operation is successful
@retval EFI_INVALID_PARAMETER         Null pointer or zero or over length request detected

**/
EFI_STATUS
EFIAPI
MsWheaEarlyStorageWrite (
  VOID                                *Ptr,
  UINT8                               Size,
  UINT8                               Offset
);

/**

This routine clears the specified data region from the MS WHEA store to PcdMsWheaEarlyStorageDefaultValue.

@param[in]  Size                      The size of intended clear data
@param[in]  Offset                    The offset of clear data, ranging from 0 to PcdMsWheaReportEarlyStorageCapacity

@retval EFI_SUCCESS                   Operation is successful
@retval EFI_INVALID_PARAMETER         Null pointer or zero or over length request detected

**/
EFI_STATUS
EFIAPI
MsWheaEarlyStorageClear (
  UINT8                               Size,
  UINT8                               Offset
);

/**

This routine checks the checksum of early storage region: starting from the signature of header to
the last byte of active range (excluding checksum field).

**/
EFI_STATUS
EFIAPI
MsWheaESCalculateChecksum16 (
  MS_WHEA_EARLY_STORAGE_HEADER    *Header,
  UINT16                          *Checksum
);

/**

This is a helper function that returns the maximal capacity for header excluded data.

@retval Count    The maximum number of bytes that can be stored in the MS WHEA store.

**/
UINT8
EFIAPI
MsWheaESGetMaxDataCount (
  VOID
);

/**

This routine finds a contiguous memory that has default value of specified size in data region
from the MS WHEA store.

@param[in]  Size                      The size of intended clear data
@param[out] Offset                    The pointer to receive returned offset value, starting from
                                      Early MS_WHEA_EARLY_STORAGE_DATA_OFFSET

@retval EFI_SUCCESS                   Operation is successful
@retval EFI_OUT_OF_RESOURCES          Null pointer or zero or over length request detected

**/
EFI_STATUS
EFIAPI
MsWheaESFindSlot (
  IN UINT8 Size,
  IN UINT8 *Offset
);

#endif // __MS_WHEA_EARLY_STORAGE_LIB__
