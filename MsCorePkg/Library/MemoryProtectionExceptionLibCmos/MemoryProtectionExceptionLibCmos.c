/**@file

Library provides access to the memory protection setting which may exist
in the platform-specific early store (in this case, CMOS) due to a memory related exception being triggered.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi/UefiBaseType.h>
#include <Uefi/UefiSpec.h>

#include <Library/MemoryProtectionExceptionLib.h>
#include <Library/BaseLib.h>
#include <Library/IoLib.h>

typedef UINT16  MEMORY_PROTECTION_OVERRIDE;
typedef UINT16  MEMORY_PROTECTION_OVERRIDE_CHECKSUM;

// Bit definitions for MEMORY_PROTECTION_OVERRIDE
#define MEM_PROT_VALID_BIT       BIT0   // Is the override valid
#define MEM_PROT_IGNORE_NEXT_EX  BIT6   // Don't indicate an exception was hit
                                        // for the next exception
#define MEM_PROT_EX_HIT_BIT  BIT7       // Was an exception hit

#define CMOS_MEM_PROT_CHECKSUM_START  0x10
#define CMOS_MEM_PROT_CHECKSUM_SIZE   sizeof (MEMORY_PROTECTION_OVERRIDE_CHECKSUM)
#define CMOS_MEM_PROT_DATA_START      CMOS_MEM_PROT_CHECKSUM_START + CMOS_MEM_PROT_CHECKSUM_SIZE
#define CMOS_MEM_PROT_DATA_SIZE       sizeof (MEMORY_PROTECTION_OVERRIDE)
#define CMOS_MEM_PROT_TEST_START      CMOS_MEM_PROT_DATA_START + CMOS_MEM_PROT_DATA_SIZE
#define CMOS_MEM_PROT_TEST_SIZE       sizeof (UINT8)
#define CMOS_MEM_PROT_TEST_VAL        0x99

#define PCAT_RTC_LO_ADDRESS_PORT  0x70
#define PCAT_RTC_LO_DATA_PORT     0x71

// ---------------------
//
// PRIVATE API
//
// ---------------------

/**
  Reads bytes from CMOS. FOR INTERNAL USE ONLY - does not check the input sanity.

  @param[in, out]   Ptr                       The pointer to data
  @param[in]        Size                      Number of bytes to read
  @param[in]        Address                   The CMOS address to read

**/
STATIC
VOID
MemProtExCmosRead (
  OUT   VOID       *Ptr,
  IN        UINT8  Size,
  IN        UINT8  Address
  )
{
  UINT8  *buffer = Ptr;
  UINT8  i;

  for (i = 0; i < Size; i++) {
    IoWrite8 (PCAT_RTC_LO_ADDRESS_PORT, Address + i);
    buffer[i] = IoRead8 (PCAT_RTC_LO_DATA_PORT);
  }
}

/**
  Writes bytes to CMOS. FOR INTERNAL USE ONLY - does not check the input sanity.

  @param[in]  Ptr                       The pointer to data
  @param[in]  Size                      Number of bytes to write
  @param[in]  Address                   The CMOS address to write to

**/
STATIC
VOID
MemProtExCmosWrite (
  IN  VOID   *Ptr,
  IN  UINT8  Size,
  IN  UINT8  Address
  )
{
  UINT8  *buffer = Ptr;
  UINT8  i;

  for (i = 0; i < Size; i++) {
    IoWrite8 (PCAT_RTC_LO_ADDRESS_PORT, Address + i);
    IoWrite8 (PCAT_RTC_LO_DATA_PORT, buffer[i]);
  }
}

/**
  Performs a write and read to verify CMOS is working properly.

  @retval TRUE  Write and read was successful
  @retval FALSE Write and read was unsuccessful

**/
STATIC
BOOLEAN
MemProtExTestCmos (
  VOID
  )
{
  UINTN  TestVal;
  UINTN  ReturnVal;

  TestVal   = CMOS_MEM_PROT_TEST_VAL;
  ReturnVal = 0;

  MemProtExCmosWrite (
    &TestVal,
    CMOS_MEM_PROT_TEST_SIZE,
    CMOS_MEM_PROT_TEST_START
    );

  MemProtExCmosRead (
    &ReturnVal,
    CMOS_MEM_PROT_TEST_SIZE,
    CMOS_MEM_PROT_TEST_START
    );

  if (TestVal == ReturnVal) {
    return TRUE;
  }

  return FALSE;
}

/**
  Sums across bytes in memory protection CMOS region.

  @retval the checksum value

**/
STATIC
MEMORY_PROTECTION_OVERRIDE_CHECKSUM
MemProtExSum (
  VOID
  )
{
  UINT8                                i, Address;
  MEMORY_PROTECTION_OVERRIDE_CHECKSUM  Sum = 0;

  Address = CMOS_MEM_PROT_DATA_START;

  for (i = 0; i < CMOS_MEM_PROT_DATA_SIZE; i++) {
    IoWrite8 (PCAT_RTC_LO_ADDRESS_PORT, Address + i);
    Sum += IoRead8 (PCAT_RTC_LO_DATA_PORT);
  }

  return Sum;
}

/**
  Returns if the checksum region value matches the sum of memory protection CMOS data

  @retval TRUE   The memory protection CMOS checksum is valid
  @retval FALSE  The memory protection CMOS checksum is not valid

**/
STATIC
BOOLEAN
MemProtIsChecksumValid (
  VOID
  )
{
  MEMORY_PROTECTION_OVERRIDE_CHECKSUM  Sum;
  MEMORY_PROTECTION_OVERRIDE_CHECKSUM  Checksum;

  MemProtExCmosRead (
    &Checksum,
    CMOS_MEM_PROT_CHECKSUM_SIZE,
    CMOS_MEM_PROT_CHECKSUM_START
    );

  Sum = MemProtExSum ();

  return Checksum == Sum;
}

/**
  Routine to update the checksum based on the memory protection CMOS bytes.

**/
STATIC
VOID
MemProtExUpdateChecksum (
  VOID
  )
{
  MEMORY_PROTECTION_OVERRIDE_CHECKSUM  Checksum = 0;

  Checksum = MemProtExSum ();

  MemProtExCmosWrite (
    &Checksum,
    CMOS_MEM_PROT_CHECKSUM_SIZE,
    CMOS_MEM_PROT_CHECKSUM_START
    );
}

/**
  Reads Value from early store

  @param[out] Val MEMORY_PROTECTION_OVERRIDE value to write

  @retval EFI_SUCCESS            Value was read
  @retval EFI_INVALID_PARAMETER  Checksum was invalid
  @retval EFI_UNSUPPORTED        Can't write/read CMOS or NULL implementation called

**/
STATIC
EFI_STATUS
MemProtExRead (
  OUT MEMORY_PROTECTION_OVERRIDE  *Val
  )
{
  if (!MemProtExTestCmos ()) {
    return EFI_UNSUPPORTED;
  }

  if (!MemProtIsChecksumValid () || (Val == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  MemProtExCmosRead (
    Val,
    CMOS_MEM_PROT_DATA_SIZE,
    CMOS_MEM_PROT_DATA_START
    );

  return EFI_SUCCESS;
}

/**
  Writes Input Value to early store

  @param[in] Val MEMORY_PROTECTION_OVERRIDE value to write

  @retval EFI_SUCCESS       Value written
  @retval EFI_UNSUPPORTED   Can't write/read CMOS or NULL implementation called

**/
STATIC
EFI_STATUS
MemProtExWrite (
  IN MEMORY_PROTECTION_OVERRIDE  Val
  )
{
  if (!MemProtExTestCmos ()) {
    return EFI_UNSUPPORTED;
  }

  MemProtExCmosWrite (
    &Val,
    CMOS_MEM_PROT_DATA_SIZE,
    CMOS_MEM_PROT_DATA_START
    );

  MemProtExUpdateChecksum ();

  return EFI_SUCCESS;
}

// ---------------------
//
// PUBLIC API
//
// ---------------------

/**
  Checks if an exception was hit on a previous boot.

  @param[out]  ExceptionOccurred  Boolean TRUE if an exception occurred, FALSE otherwise.
                                  ExceptionOccurred IS NOT updated if the function returns an error.

  @retval EFI_SUCCESS             ExceptionOccurred contains the result of the check
  @retval EFI_INVALID_PARAMETER   Unable to validate persistent storage contents
  @retval EFI_UNSUPPORTED         Platform-specific persistent storage is unresponsive or NULL implementation called

**/
EFI_STATUS
EFIAPI
MemProtExGetExceptionOccurred (
  OUT   BOOLEAN  *ExceptionOccurred
  )
{
  EFI_STATUS                  Status;
  MEMORY_PROTECTION_OVERRIDE  CmosVal = 0;

  Status = MemProtExRead (&CmosVal);

  if (!EFI_ERROR (Status)) {
    *ExceptionOccurred = (CmosVal & (MEM_PROT_VALID_BIT | MEM_PROT_EX_HIT_BIT)) ==
                         (MEM_PROT_VALID_BIT | MEM_PROT_EX_HIT_BIT);
  }

  return Status;
}

/**
  Sets memory protection exception value in platform-specific persistent storage to indicate that an
  exception has occurred.

  @retval EFI_SUCCESS       Value set
  @retval EFI_UNSUPPORTED   Platform-specific persistent storage is unresponsive or NULL implementation called

**/
EFI_STATUS
EFIAPI
MemProtExSetExceptionOccurred (
  VOID
  )
{
  MEMORY_PROTECTION_OVERRIDE  CmosVal = 0;
  EFI_STATUS                  Status;

  Status = MemProtExRead (&CmosVal);

  return (EFI_ERROR (Status)) ? Status : MemProtExWrite (CmosVal | (MEM_PROT_VALID_BIT | MEM_PROT_EX_HIT_BIT));
}

/**
  Clears from the platform-specific persistent storage the memory protection exception value indicating that
  a memory violation exception occurred on a previous boot.

  @retval EFI_SUCCESS       Value cleared
  @retval EFI_UNSUPPORTED   Platform-specific persistent storage is unresponsive or NULL implementation called

**/
EFI_STATUS
EFIAPI
MemProtExClearExceptionOccurred (
  VOID
  )
{
  MEMORY_PROTECTION_OVERRIDE  CmosVal = 0;
  EFI_STATUS                  Status;

  Status = MemProtExRead (&CmosVal);

  return (EFI_ERROR (Status)) ? Status : MemProtExWrite (CmosVal & ~MEM_PROT_EX_HIT_BIT);
}

/**
  Checks if the exception handler should ignore the next memory guard violation exception.

  @param[out]  IgnoreNextException  Boolean TRUE if next exception should be ignored, FALSE otherwise.
                                    IgnoreNextException IS NOT updated if the function returns an error.

  @retval EFI_SUCCESS             IgnoreNextException contains the result of the check
  @retval EFI_INVALID_PARAMETER   Unable to validate persistent storage contents
  @retval EFI_UNSUPPORTED         Platform-specific persistent storage is unresponsive or NULL implementation called

**/
EFI_STATUS
EFIAPI
MemProtExGetIgnoreNextException (
  OUT BOOLEAN  *IgnoreNextException
  )
{
  EFI_STATUS                  Status;
  MEMORY_PROTECTION_OVERRIDE  CmosVal = 0;

  Status = MemProtExRead (&CmosVal);

  if (!EFI_ERROR (Status)) {
    *IgnoreNextException = (CmosVal & (MEM_PROT_VALID_BIT | MEM_PROT_IGNORE_NEXT_EX)) ==
                           (MEM_PROT_VALID_BIT | MEM_PROT_IGNORE_NEXT_EX);
  }

  return Status;
}

/**
  Sets memory protection exception value in platform-specific persistent storage to indicate that the
  next memory guard violation exception should be ignored, meaning when the exception occurs, the call
  to MemProtExGetExceptionOccurred () will not reflect that an exception occurred the previous boot
  (assuming that MemProtExGetExceptionOccurred () would have returned FALSE prior to the last exception).

  @retval EFI_SUCCESS       Value set
  @retval EFI_UNSUPPORTED   Platform-specific persistent storage is unresponsive or NULL implementation called

**/
EFI_STATUS
EFIAPI
MemProtExSetIgnoreNextException (
  VOID
  )
{
  MEMORY_PROTECTION_OVERRIDE  CmosVal = 0;
  EFI_STATUS                  Status;

  Status = MemProtExRead (&CmosVal);

  return (EFI_ERROR (Status)) ? Status : MemProtExWrite (CmosVal | (MEM_PROT_VALID_BIT | MEM_PROT_IGNORE_NEXT_EX));
}

/**
  Clears from the platform-specific persistent storage the memory protection exception value indicating that the
  next memory guard violation exception should be ignored.

  @retval EFI_SUCCESS       Value cleared
  @retval EFI_UNSUPPORTED   Platform-specific persistent storage is unresponsive or NULL implementation called

**/
EFI_STATUS
EFIAPI
MemProtExClearIgnoreNextException (
  VOID
  )
{
  MEMORY_PROTECTION_OVERRIDE  CmosVal = 0;
  EFI_STATUS                  Status;

  Status = MemProtExRead (&CmosVal);

  return (EFI_ERROR (Status)) ? Status : MemProtExWrite (CmosVal & ~MEM_PROT_IGNORE_NEXT_EX);
}

/**
  Clears all memory protection exception values from the platform-specific persistent storage.

  @retval EFI_SUCCESS       Successfully cleared the exception bytes
  @retval EFI_UNSUPPORTED   Platform-specific persistent storage is unresponsive or NULL implementation called

**/
EFI_STATUS
EFIAPI
MemProtExClearAll (
  VOID
  )
{
  return MemProtExWrite ((MEMORY_PROTECTION_OVERRIDE)0);
}
