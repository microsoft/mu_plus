/** @file -- MsWheaEarlyStorageLib.c

This header defines APIs to utilize special memory for MsWheaReport during 
early stage.

Copyright (c) 2018, Microsoft Corporation

All rights reserved.
Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

**/

#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/MsWheaEarlyStorageLib.h>

#define PCAT_RTC_LO_ADDRESS_PORT              0x70
#define PCAT_RTC_LO_DATA_PORT                 0x71
#define PCAT_RTC_HI_ADDRESS_PORT              0x72
#define PCAT_RTC_HI_DATA_PORT                 0x73

#define MS_WHEA_EARLY_STORAGE_OFFSET          0x40

/**

This routine has the highest previlege to read any byte(s) on the CMOS

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

This routine has the highest previlege to write any byte(s) on the CMOS

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

This routine has the highest previlege to 'clear' any byte(s) on the CMOS

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
