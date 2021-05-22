/** @file -- MsWheaEarlyStorageLib.c

This header defines APIs to utilize special memory for MsWheaReport during
early stage.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi/UefiBaseType.h>

#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/BaseLib.h>
#include <Library/MsWheaEarlyStorageLib.h>

#define PCAT_RTC_LO_ADDRESS_PORT              0x70
#define PCAT_RTC_LO_DATA_PORT                 0x71
#define PCAT_RTC_HI_ADDRESS_PORT              0x72
#define PCAT_RTC_HI_DATA_PORT                 0x73

#define MS_WHEA_EARLY_STORAGE_OFFSET          0x40

#define MS_WHEA_EARLY_STORAGE_HEADER_SIZE     (sizeof(MS_WHEA_EARLY_STORAGE_HEADER))
#define MS_WHEA_EARLY_STORAGE_DATA_OFFSET     MS_WHEA_EARLY_STORAGE_HEADER_SIZE

/**

This routine has the highest privilege to read any byte(s) on the CMOS

@param[in]  Ptr                       The pointer to hold read data
@param[in]  Size                      The size of intended read data
@param[in]  Offset                    The offset of read data, starting from the beginning of CMOS

@retval EFI_SUCCESS                   Operation is successful
@retval EFI_INVALID_PARAMETER         Null pointer or zero or over length request detected

**/
STATIC
EFI_STATUS
__MsWheaCMOSRawRead (
  VOID                                *Ptr,
  UINT8                               Size,
  UINT8                               Offset
  )
{
  UINT8       mIndex;
  UINT8       i;
  UINT8       *mBuf;
  EFI_STATUS  Status;

  mBuf = Ptr;
  if ((mBuf == NULL) ||
      (Size == 0) ||
      ((UINT8)(PcdGet32(PcdMsWheaReportEarlyStorageCapacity) - Size) < Offset)) {
    Status = EFI_INVALID_PARAMETER;
    goto Cleanup;
  }

  for (i = 0; i < Size; i++) {
    mIndex = Offset + i;
    if ((mIndex >= 0) && (mIndex <= 127)) {
      IoWrite8(PCAT_RTC_LO_ADDRESS_PORT, mIndex);
      mBuf[i] = IoRead8(PCAT_RTC_LO_DATA_PORT);
    }
    else {
      IoWrite8(PCAT_RTC_HI_ADDRESS_PORT, mIndex);
      mBuf[i] = IoRead8(PCAT_RTC_HI_DATA_PORT);
    }
  }
  Status = EFI_SUCCESS;

Cleanup:
  return Status;
}

/**

This routine has the highest privilege to write any byte(s) on the CMOS

@param[in]  Ptr                       The pointer to hold intended written data
@param[in]  Size                      The size of intended written data
@param[in]  Offset                    The offset of written data, starting from the beginning of CMOS

@retval EFI_SUCCESS                   Operation is successful
@retval EFI_INVALID_PARAMETER         Null pointer or zero or over length request detected

**/
STATIC
EFI_STATUS
__MsWheaCMOSRawWrite (
  VOID                                *Ptr,
  UINT8                               Size,
  UINT8                               Offset
  )
{
  UINT8       mIndex;
  UINT8       i;
  UINT8       *mBuf;
  EFI_STATUS  Status;

  mBuf = Ptr;
  if ((mBuf == NULL) ||
      (Size == 0) ||
      ((UINT8)(PcdGet32(PcdMsWheaReportEarlyStorageCapacity) - Size) < Offset)) {
    Status = EFI_INVALID_PARAMETER;
    goto Cleanup;
  }

  for (i = 0; i < Size; i++) {
    mIndex = Offset + i;
    if ((mIndex >= 0) && (mIndex <= 127)) {
      IoWrite8(PCAT_RTC_LO_ADDRESS_PORT, mIndex);
      IoWrite8(PCAT_RTC_LO_DATA_PORT, mBuf[i]);
    }
    else {
      IoWrite8(PCAT_RTC_HI_ADDRESS_PORT, mIndex);
      IoWrite8(PCAT_RTC_HI_DATA_PORT, mBuf[i]);
    }
  }
  Status = EFI_SUCCESS;

Cleanup:
  return Status;
}

/**

This routine has the highest privilege to 'clear' any byte(s) on the CMOS

@param[in]  Size                      The size of intended clear region
@param[in]  Offset                    The offset of clear data, starting from the beginning of CMOS

@retval EFI_SUCCESS                   Operation is successful
@retval EFI_INVALID_PARAMETER         Null pointer or zero or over length request detected

**/
STATIC
EFI_STATUS
__MsWheaCMOSRawClear (
  UINT8                               Size,
  UINT8                               Offset
  )
{
  UINT8       mIndex;
  UINT8       i;
  EFI_STATUS  Status;

  if ((Size == 0) ||
      ((UINT8)(PcdGet32(PcdMsWheaReportEarlyStorageCapacity) - Size) < Offset)) {
    Status = EFI_INVALID_PARAMETER;
    goto Cleanup;
  }

  for (i = 0; i < Size; i++) {
    mIndex = Offset + i;
    if ((mIndex >= 0) && (mIndex <= 127)) {
      IoWrite8(PCAT_RTC_LO_ADDRESS_PORT, mIndex);
      IoWrite8(PCAT_RTC_LO_DATA_PORT, PcdGet8(PcdMsWheaEarlyStorageDefaultValue));
    }
    else {
      IoWrite8(PCAT_RTC_HI_ADDRESS_PORT, mIndex);
      IoWrite8(PCAT_RTC_HI_DATA_PORT, PcdGet8(PcdMsWheaEarlyStorageDefaultValue));
    }
  }
  Status = EFI_SUCCESS;
Cleanup:
  return Status;
}

/**

This routine clears all bytes in Data region

**/
STATIC
VOID
MsWheaCMOSStoreClearAll (
  VOID
  )
{
  __MsWheaCMOSRawClear(MsWheaEarlyStorageGetMaxSize(), MS_WHEA_EARLY_STORAGE_OFFSET);
}

/**

This routine returns the maximum number of bytes that can be stored in the early storage area.

@retval Count    The maximum number of bytes that can be stored in the MS WHEA store.

**/
UINT8
EFIAPI
MsWheaEarlyStorageGetMaxSize (
  VOID
  )
{
  return (UINT8)((PcdGet32(PcdMsWheaReportEarlyStorageCapacity) - (MS_WHEA_EARLY_STORAGE_OFFSET)) & 0xFF);
}

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
  )
{
  EFI_STATUS Status;
  if (Offset >= MsWheaEarlyStorageGetMaxSize()) {
    Status = EFI_INVALID_PARAMETER;
    goto Cleanup;
  }

  Status = __MsWheaCMOSRawRead(Ptr, Size, MS_WHEA_EARLY_STORAGE_OFFSET + Offset);

Cleanup:
  return Status;
}

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
  )
{
  EFI_STATUS Status;
  if (Offset >= MsWheaEarlyStorageGetMaxSize()) {
    Status = EFI_INVALID_PARAMETER;
    goto Cleanup;
  }

  Status = __MsWheaCMOSRawWrite(Ptr, Size, MS_WHEA_EARLY_STORAGE_OFFSET + Offset);

Cleanup:
  return Status;
}

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
  )
{
  EFI_STATUS Status;
  if (Offset >= MsWheaEarlyStorageGetMaxSize()) {
    Status = EFI_INVALID_PARAMETER;
    goto Cleanup;
  }

  Status = __MsWheaCMOSRawClear(Size, MS_WHEA_EARLY_STORAGE_OFFSET + Offset);

Cleanup:
  return Status;
}

/**

This is a helper function that returns the maximal capacity for header excluded data.

@retval Count    The maximum number of bytes that can be stored in the MS WHEA store.

**/
UINT8
EFIAPI
MsWheaESGetMaxDataCount (
  VOID
  )
{
  return (UINT8)((MsWheaEarlyStorageGetMaxSize() - (MS_WHEA_EARLY_STORAGE_DATA_OFFSET)) & 0xFF);
}

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
  )
{
  EFI_STATUS  Status = EFI_OUT_OF_RESOURCES;
  MS_WHEA_EARLY_STORAGE_HEADER Header;

  MsWheaEarlyStorageRead(&Header, MS_WHEA_EARLY_STORAGE_HEADER_SIZE, 0);

  if (Header.ActiveRange + Size <= MsWheaESGetMaxDataCount()) {
    *Offset = (UINT8) Header.ActiveRange;
    Status = EFI_SUCCESS;
  }
  return Status;
}

/**

This routine checks the checksum of early storage region: starting from the signature of header to
the last byte of active range (excluding checksum field).

**/
EFI_STATUS
EFIAPI
MsWheaESCalculateChecksum16 (
  MS_WHEA_EARLY_STORAGE_HEADER    *Header,
  UINT16                          *Checksum
  )
{
  UINT16      Data;
  UINT8       Index;
  UINT16      Sum;
  EFI_STATUS  Status;

  DEBUG((DEBUG_INFO, "%a Calculate sum...\n", __FUNCTION__));

  Status = EFI_SUCCESS;

  if ((Checksum == NULL) || (Header == NULL)) {
    Status = EFI_INVALID_PARAMETER;
    goto Cleanup;
  }
  else if ((Header->ActiveRange > MsWheaEarlyStorageGetMaxSize()) ||
           ((Header->ActiveRange & BIT0) != 0)) {
    Status = EFI_BAD_BUFFER_SIZE;
    goto Cleanup;
  }

  // Clear the checksum field for calculation then restore...
  *Checksum = Header->Checksum;
  Header->Checksum = 0;
  Sum = CalculateSum16 ((UINT16*)Header, MS_WHEA_EARLY_STORAGE_HEADER_SIZE);
  Header->Checksum = *Checksum;

  for (Index = 0; Index < Header->ActiveRange; Index += sizeof(Data)) {
    Status = MsWheaEarlyStorageRead(&Data, sizeof(Data), MS_WHEA_EARLY_STORAGE_DATA_OFFSET + Index);
    if (EFI_ERROR(Status) != FALSE) {
      DEBUG((DEBUG_ERROR, "%a: Reading Early Storage %d failed %r\n", __FUNCTION__, Index, Status));
      goto Cleanup;
    }
    Sum = Sum + Data;
  }

  *Checksum = (UINT16) (0x10000 - Sum);

Cleanup:
  return Status;
}
