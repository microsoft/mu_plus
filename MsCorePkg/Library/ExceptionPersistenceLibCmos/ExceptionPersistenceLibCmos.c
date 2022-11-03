/**@file ExceptionPersistenceLibCmos.c

Library provides access to the exception information which may exist
in the platform-specific early store (in this case, CMOS).

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi/UefiBaseType.h>
#include <Uefi/UefiSpec.h>

#include <Library/ExceptionPersistenceLib.h>
#include <Library/BaseLib.h>
#include <Library/IoLib.h>

typedef UINT16  EXCEPTION_PERSISTENCE_VAL;
typedef UINT16  EXCEPTION_PERSISTENCE_VAL_CHECKSUM;

// Bit definitions for EXCEPTION_PERSISTENCE_VAL
#define EX_PERSIST_VALID_BIT         BIT0  // Is the override valid
#define EX_PERSIST_IGNORE_NEXT_PF    BIT6  // Ignore and clear the next page fault (requires Memory Protection Nonstop Protocol)
#define EX_PERSIST_PF_HIT_BIT        BIT10 // A page fault exception occurred
#define EX_PERSIST_STACK_COOKIE_BIT  BIT11 // I2C occurred
#define EX_PERSIST_OTHER_EX_BIT      BIT15 // An unknown exception occurred

#define EX_PERSIST_EXCEPTION_BITS  (EX_PERSIST_PF_HIT_BIT | EX_PERSIST_STACK_COOKIE_BIT | EX_PERSIST_OTHER_EX_BIT)

#define CMOS_EX_PERSIST_CHECKSUM_START  0x10
#define CMOS_EX_PERSIST_CHECKSUM_SIZE   sizeof (EXCEPTION_PERSISTENCE_VAL_CHECKSUM)
#define CMOS_EX_PERSIST_DATA_START      CMOS_EX_PERSIST_CHECKSUM_START + CMOS_EX_PERSIST_CHECKSUM_SIZE
#define CMOS_EX_PERSIST_DATA_SIZE       sizeof (EXCEPTION_PERSISTENCE_VAL)
#define CMOS_EX_PERSIST_TEST_START      CMOS_EX_PERSIST_DATA_START + CMOS_EX_PERSIST_DATA_SIZE
#define CMOS_EX_PERSIST_TEST_SIZE       sizeof (UINT8)
#define CMOS_EX_PERSIST_TEST_VAL        0x99

#define PCAT_RTC_LO_ADDRESS_PORT  0x70
#define PCAT_RTC_LO_DATA_PORT     0x71

#define ONLY_ONE_BIT_SET(a)  (!(a & (a - 1)))

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
ExPersistCmosRead (
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
ExPersistCmosWrite (
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
ExPersistTestCmos (
  VOID
  )
{
  UINTN  TestVal;
  UINTN  ReturnVal;

  TestVal   = CMOS_EX_PERSIST_TEST_VAL;
  ReturnVal = 0;

  ExPersistCmosWrite (
    &TestVal,
    CMOS_EX_PERSIST_TEST_SIZE,
    CMOS_EX_PERSIST_TEST_START
    );

  ExPersistCmosRead (
    &ReturnVal,
    CMOS_EX_PERSIST_TEST_SIZE,
    CMOS_EX_PERSIST_TEST_START
    );

  return TestVal == ReturnVal;
}

/**
  Sums across bytes in memory protection CMOS region.

  @retval the checksum value
**/
STATIC
EXCEPTION_PERSISTENCE_VAL_CHECKSUM
ExPersistSum (
  VOID
  )
{
  UINT8                               i, Address;
  EXCEPTION_PERSISTENCE_VAL_CHECKSUM  Sum = 0;

  Address = CMOS_EX_PERSIST_DATA_START;

  for (i = 0; i < CMOS_EX_PERSIST_DATA_SIZE; i++) {
    IoWrite8 (PCAT_RTC_LO_ADDRESS_PORT, Address + i);
    Sum += IoRead8 (PCAT_RTC_LO_DATA_PORT);
  }

  return Sum;
}

/**
  Returns if the checksum region value matches the sum of memory protection CMOS data

  @retval TRUE   The CMOS checksum is valid
  @retval FALSE  The CMOS checksum is not valid
**/
STATIC
BOOLEAN
ExPersistIsChecksumValid (
  VOID
  )
{
  EXCEPTION_PERSISTENCE_VAL_CHECKSUM  Sum;
  EXCEPTION_PERSISTENCE_VAL_CHECKSUM  Checksum;

  ExPersistCmosRead (
    &Checksum,
    CMOS_EX_PERSIST_CHECKSUM_SIZE,
    CMOS_EX_PERSIST_CHECKSUM_START
    );

  Sum = ExPersistSum ();

  return Checksum == Sum;
}

/**
  Routine to update the checksum based on the memory protection CMOS bytes.
**/
STATIC
VOID
ExPersistUpdateChecksum (
  VOID
  )
{
  EXCEPTION_PERSISTENCE_VAL_CHECKSUM  Checksum = 0;

  Checksum = ExPersistSum ();

  ExPersistCmosWrite (
    &Checksum,
    CMOS_EX_PERSIST_CHECKSUM_SIZE,
    CMOS_EX_PERSIST_CHECKSUM_START
    );
}

/**
  Reads Value from early store

  @param[out] Val EXCEPTION_PERSISTENCE_VAL value to write

  @retval EFI_SUCCESS            Value was read
  @retval EFI_INVALID_PARAMETER  Checksum was invalid
  @retval EFI_DEVICE_ERROR       Can't write/read CMOS
**/
STATIC
EFI_STATUS
ExPersistRead (
  OUT EXCEPTION_PERSISTENCE_VAL  *Val
  )
{
  if (!ExPersistTestCmos ()) {
    return EFI_DEVICE_ERROR;
  }

  if (!ExPersistIsChecksumValid () || (Val == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  ExPersistCmosRead (
    Val,
    CMOS_EX_PERSIST_DATA_SIZE,
    CMOS_EX_PERSIST_DATA_START
    );

  return EFI_SUCCESS;
}

/**
  Writes Input Value to early store

  @param[in] Val EXCEPTION_PERSISTENCE_VAL value to write

  @retval EFI_SUCCESS            Value written
  @retval EFI_DEVICE_ERROR       Can't write/read CMOS
**/
STATIC
EFI_STATUS
ExPersistWrite (
  IN EXCEPTION_PERSISTENCE_VAL  Val
  )
{
  if (!ExPersistTestCmos ()) {
    return EFI_DEVICE_ERROR;
  }

  ExPersistCmosWrite (
    &Val,
    CMOS_EX_PERSIST_DATA_SIZE,
    CMOS_EX_PERSIST_DATA_START
    );

  ExPersistUpdateChecksum ();

  return EFI_SUCCESS;
}

// ---------------------
//
// PUBLIC API
//
// ---------------------

/**
  Check if an exception was hit on a previous boot.

  @param[out]  Exception          EXCEPTION_TYPE in persistent storage
                                  NOTE: Exception IS NOT updated if the function returns an error.

  @retval EFI_SUCCESS             Exception contains the result of the check
  @retval EFI_INVALID_PARAMETER   Unable to validate persistent storage contents or Exception is NULL
  @retval EFI_UNSUPPORTED         Platform-specific persistent storage is unresponsive or NULL implementation called
  @retval EFI_DEVICE_ERROR        Can't write/read platform-specific persistent storage
**/
EFI_STATUS
EFIAPI
ExPersistGetException (
  OUT   EXCEPTION_TYPE  *Exception
  )
{
  EFI_STATUS                 Status;
  EXCEPTION_PERSISTENCE_VAL  CmosVal       = 0;
  UINT16                     ExceptionBits = 0;

  if (Exception == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = ExPersistRead (&CmosVal);

  if (!EFI_ERROR (Status)) {
    if ((CmosVal & EX_PERSIST_VALID_BIT) == 0) {
      return EFI_SUCCESS;
    }

    ExceptionBits = CmosVal & EX_PERSIST_EXCEPTION_BITS;

    // Ensure only one exception bit is set
    if (ONLY_ONE_BIT_SET (ExceptionBits)) {
      switch (ExceptionBits) {
        case EX_PERSIST_PF_HIT_BIT:
          *Exception = ExceptionPersistPageFault;
          break;
        case EX_PERSIST_OTHER_EX_BIT:
          *Exception = ExceptionPersistOther;
          break;
        case EX_PERSIST_STACK_COOKIE_BIT:
          *Exception = ExceptionPersistStackCookie;
          break;
        case 0:
          *Exception = ExceptionPersistNone;
          break;
        default:
          // Should never get here
          Status = EFI_INVALID_PARAMETER;
          break;
      }
    } else {
      Status = EFI_INVALID_PARAMETER;
    }
  }

  return Status;
}

/**
  Write input EXCEPTION_TYPE to platform-specific persistent storage.

  @param[in]  Exception   EXCEPTION_TYPE to set in persistent storage
                          NOTE: ExceptionPersistNone has the same effect as ExPersistClearExceptions()

  @retval EFI_SUCCESS             Value set
  @retval EFI_UNSUPPORTED         NULL implementation called
  @retval EFI_INVALID_PARAMETER   Platform-specific persistent storage contents are invalid or
                                  input EXCEPTION_TYPE is invalid
  @retval EFI_DEVICE_ERROR        Can't write/read platform-specific persistent storage
**/
EFI_STATUS
EFIAPI
ExPersistSetException (
  IN  EXCEPTION_TYPE  Exception
  )
{
  EXCEPTION_PERSISTENCE_VAL  CmosVal = 0;
  EFI_STATUS                 Status;
  UINT16                     ExceptionBit = 0;

  switch (Exception) {
    case ExceptionPersistPageFault:
      ExceptionBit = EX_PERSIST_PF_HIT_BIT;
      break;
    case ExceptionPersistStackCookie:
      ExceptionBit = EX_PERSIST_STACK_COOKIE_BIT;
      break;
    case ExceptionPersistOther:
      ExceptionBit = EX_PERSIST_OTHER_EX_BIT;
      break;
    case ExceptionPersistNone:
      return ExPersistClearExceptions ();
    default:
      return EFI_INVALID_PARAMETER;
  }

  Status = ExPersistRead (&CmosVal);

  return (EFI_ERROR (Status)) ? Status : ExPersistWrite (CmosVal | (EX_PERSIST_VALID_BIT | ExceptionBit));
}

/**
  Clears from the platform-specific persistent storage of all exception info.

  @retval EFI_SUCCESS       Value cleared
  @retval EFI_UNSUPPORTED   Platform-specific persistent storage contents are invalid or NULL implementation called
  @retval EFI_DEVICE_ERROR  Can't write/read platform-specific persistent storage
**/
EFI_STATUS
EFIAPI
ExPersistClearExceptions (
  VOID
  )
{
  EXCEPTION_PERSISTENCE_VAL  CmosVal = 0;
  EFI_STATUS                 Status;

  Status = ExPersistRead (&CmosVal);

  return (EFI_ERROR (Status)) ? Status : ExPersistWrite (CmosVal & ~EX_PERSIST_EXCEPTION_BITS);
}

/**
  Checks if the next page fault should be ignored and cleared. Using platform-specific persistent storage
  is a means for communicating to the exception handler that the page fault was intentional. Resuming execution
  requires faulting pages to have their attributes cleared so execution can continue.

  @param[out]  IgnoreNextPageFault  Boolean TRUE if next page fault should be ignored, FALSE otherwise.
                                    Persistent storage IS NOT updated if the function returns an error.

  @retval EFI_SUCCESS             IgnoreNextPageFault contains the result of the check
  @retval EFI_INVALID_PARAMETER   Unable to validate persistent storage contents
  @retval EFI_UNSUPPORTED         Platform-specific persistent storage is unresponsive or NULL implementation called
  @retval EFI_DEVICE_ERROR        Can't write/read platform-specific persistent storage
**/
EFI_STATUS
EFIAPI
ExPersistGetIgnoreNextPageFault (
  OUT BOOLEAN  *IgnoreNextPageFault
  )
{
  EFI_STATUS                 Status;
  EXCEPTION_PERSISTENCE_VAL  CmosVal = 0;

  if (IgnoreNextPageFault == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = ExPersistRead (&CmosVal);

  if (!EFI_ERROR (Status)) {
    *IgnoreNextPageFault = (CmosVal & (EX_PERSIST_VALID_BIT | EX_PERSIST_IGNORE_NEXT_PF)) ==
                           (EX_PERSIST_VALID_BIT | EX_PERSIST_IGNORE_NEXT_PF);
  }

  return Status;
}

/**
  Updates the platform-specific persistent storage to indicate that the next page fault should be ignored and cleared.
  Using platform-specific persistent storage is a means for communicating to the exception handler that the
  page fault was intentional. Resuming execution requires faulting pages to have their attributes cleared
  so execution can continue.

  @retval EFI_SUCCESS       Value set
  @retval EFI_UNSUPPORTED   Platform-specific persistent storage is unresponsive or NULL implementation called
  @retval EFI_DEVICE_ERROR  Can't write/read platform-specific persistent storage
**/
EFI_STATUS
EFIAPI
ExPersistSetIgnoreNextPageFault (
  VOID
  )
{
  EXCEPTION_PERSISTENCE_VAL  CmosVal = 0;
  EFI_STATUS                 Status;

  Status = ExPersistRead (&CmosVal);

  return (EFI_ERROR (Status)) ? Status : ExPersistWrite (CmosVal | (EX_PERSIST_VALID_BIT | EX_PERSIST_IGNORE_NEXT_PF));
}

/**
  Clears from the platform-specific persistent storage the value indicating that the
  next page fault should be ignored.

  @retval EFI_SUCCESS       Value cleared
  @retval EFI_UNSUPPORTED   Platform-specific persistent storage is unresponsive or NULL implementation called
  @retval EFI_DEVICE_ERROR  Can't write/read platform-specific persistent storage
**/
EFI_STATUS
EFIAPI
ExPersistClearIgnoreNextPageFault (
  VOID
  )
{
  EXCEPTION_PERSISTENCE_VAL  CmosVal = 0;
  EFI_STATUS                 Status;

  Status = ExPersistRead (&CmosVal);

  return (EFI_ERROR (Status)) ? Status : ExPersistWrite (CmosVal & ~EX_PERSIST_IGNORE_NEXT_PF);
}

/**
  Clears all values from the platform-specific persistent storage.

  @retval EFI_SUCCESS       Successfully cleared the exception bytes
  @retval EFI_UNSUPPORTED   Platform-specific persistent storage is unresponsive or NULL implementation called
  @retval EFI_DEVICE_ERROR  Can't write/read platform-specific persistent storage
**/
EFI_STATUS
EFIAPI
ExPersistClearAll (
  VOID
  )
{
  return ExPersistWrite ((EXCEPTION_PERSISTENCE_VAL)0);
}
