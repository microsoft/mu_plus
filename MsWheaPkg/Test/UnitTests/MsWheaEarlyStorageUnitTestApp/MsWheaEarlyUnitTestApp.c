/** @file -- MsWheaEarlyUnitTestApp.c

Tests for MS WHEA early storage management.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Pi/PiStatusCode.h>
#include <Guid/MsWheaReportDataType.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UnitTestLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/MsWheaEarlyStorageLib.h>
#include <MsWheaErrorStatus.h>

#include "../../../MsWheaReport/MsWheaEarlyStorageMgr.h"

#ifndef INTERNAL_UNIT_TEST
#error Make sure to build thie with INTERNAL_UNIT_TEST enabled! Otherwise, some important tests may be skipped!
#endif

#define UNIT_TEST_APP_NAME            "MsWhea Early Storage Test"
#define UNIT_TEST_APP_VERSION         "0.1"

#define   TEST_ERROR_STATUS_VALUE     0xA0A0A0A0
#define   TEST_ADDITIONAL_INFO_1      0xDEADBEEF
#define   TEST_ADDITIONAL_INFO_2      0xFEEDF00D

MS_WHEA_EARLY_STORAGE_HEADER          UnitTestHeader;
UINT16                                TestDataArray[5] = {1, 2, 3, 4, 5};
MS_WHEA_ERROR_ENTRY_MD                *pMsWheaEntryMD = NULL;

///================================================================================================
///================================================================================================
///
/// HELPER FUNCTIONS
///
///================================================================================================
///================================================================================================

VOID
EFIAPI
MsWheaESDump (
  VOID
  );

UINT8
MsWheaESGetMaxDataCount (
  VOID
  );

VOID
MsWheaESClearAllData (
  VOID
  );

EFI_STATUS
MsWheaESReadData (
  VOID                                *Ptr,
  UINT8                               Size,
  UINT8                               Offset
  );

EFI_STATUS
MsWheaESWriteData (
  VOID                                *Ptr,
  UINT8                               Size,
  UINT8                               Offset
  );

VOID
MsWheaESReadHeader (
  MS_WHEA_EARLY_STORAGE_HEADER        *Header
  );

VOID
MsWheaESWriteHeader (
  MS_WHEA_EARLY_STORAGE_HEADER        *Header
  );

EFI_STATUS
MsWheaESChecksum16 (
  MS_WHEA_EARLY_STORAGE_HEADER    *Header,
  UINT16                          *Checksum
  );

VOID
MsWheaESContentChangeChecksumHelper (
  UINT16*         Buffer,
  UINTN           Length
  );

VOID
MsWheaESHeaderChangeChecksumHelper (
  MS_WHEA_EARLY_STORAGE_HEADER    *Header
  );

BOOLEAN
MsWheaESRegionIsValid (
  OUT MS_WHEA_EARLY_STORAGE_HEADER *OutPutHeader OPTIONAL
  );

EFI_STATUS
MsWheaESFindSlot (
  IN UINT8 Size,
  IN UINT8 *Offset
  );

EFI_STATUS
EFIAPI
TestReportFunction (
  IN MS_WHEA_ERROR_ENTRY_MD           *InMsWheaEntryMD
  )
{
  if (InMsWheaEntryMD == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  pMsWheaEntryMD = AllocatePool (sizeof(MS_WHEA_ERROR_ENTRY_MD));
  if (InMsWheaEntryMD == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (pMsWheaEntryMD, InMsWheaEntryMD, sizeof(MS_WHEA_ERROR_ENTRY_MD));

  return EFI_SUCCESS;
}

///================================================================================================
///================================================================================================
///
/// PRE REQ FUNCTIONS
///
///================================================================================================
///================================================================================================

/**
  Function verifies that the early storage starts from a good condition

  @param[in] Context                  Test context applied for this test case

  @retval UNIT_TEST_PASSED            The entry point executed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED Null pointer detected.

**/
UNIT_TEST_STATUS
EFIAPI
MsWheaESVerify (
  IN UNIT_TEST_CONTEXT                Context
  )
{
  MS_WHEA_EARLY_STORAGE_HEADER          Header;

  UT_ASSERT_TRUE (MsWheaESRegionIsValid(&Header));
  UT_ASSERT_EQUAL (Header.ActiveRange, 0);

  return UNIT_TEST_PASSED;
} // MsWheaESVerify ()

///================================================================================================
///================================================================================================
///
/// CLEANUP FUNCTIONS
///
///================================================================================================
///================================================================================================

/**
  Clear all the HwErrRec entries on flash

  @param[in] Context                  Test context applied for this test case

  @retval UNIT_TEST_PASSED            The entry point executed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED Null pointer detected.

**/
VOID
EFIAPI
MsWheaESCleanUp (
  IN UNIT_TEST_CONTEXT                Context
  )
{
  MsWheaESDump();

  // This is needed incase there is leftover garbage in default/failed cases
  MsWheaESClearAllData();

  // Zero all the fields in the
  SetMem(&UnitTestHeader, sizeof(MS_WHEA_EARLY_STORAGE_HEADER), 0);

  // Sign the header signature.
  UnitTestHeader.Signature = MS_WHEA_EARLY_STORAGE_SIGNATURE;
  UnitTestHeader.ActiveRange = 0;
  MsWheaESHeaderChangeChecksumHelper (&UnitTestHeader);

  if (pMsWheaEntryMD != NULL) {
    FreePool (pMsWheaEntryMD);
  }

} // MsWheaESCleanUp ()

///================================================================================================
///================================================================================================
///
/// TEST CASES
///
///================================================================================================
///================================================================================================


/**
  This routine should verify internal checksum vs BaseLib checksum
**/
UNIT_TEST_STATUS
EFIAPI
MsWheaESChecksumTest (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  UINT16* Data;
  EFI_STATUS Status;
  UINT16 Checksum16;
  MS_WHEA_EARLY_STORAGE_HEADER *Header;

  Data = AllocatePool (MsWheaEarlyStorageGetMaxSize());
  MsWheaEarlyStorageRead(Data, MsWheaEarlyStorageGetMaxSize(), 0);

  // Zero the checksum before calculation
  Header = (MS_WHEA_EARLY_STORAGE_HEADER*) Data;
  Header->Checksum = 0;
  Header->Checksum = CalculateCheckSum16(Data, sizeof(MS_WHEA_EARLY_STORAGE_HEADER) + Header->ActiveRange);

  MsWheaESReadHeader(&UnitTestHeader);
  Status = MsWheaESChecksum16 (&UnitTestHeader, &Checksum16);

  // ES calculated, Base lib return and stored values should be the same
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_EQUAL (Checksum16, Header->Checksum);
  UT_ASSERT_EQUAL (Checksum16, UnitTestHeader.Checksum);

  return UNIT_TEST_PASSED;
}

/**
  This routine should verify ES will ignore inactive data region corruption
**/
UNIT_TEST_STATUS
EFIAPI
MsWheaESDataCorruptTest (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  // All test cases here:
  UINT8 Data;
  Data = 1;

  MsWheaESReadHeader(&UnitTestHeader);

  MsWheaESWriteData(&Data, (UINT8) (PcdGet8(PcdMsWheaEarlyStorageDefaultValue) - 1), (UINT8) UnitTestHeader.ActiveRange);
  // should pass
  UT_ASSERT_TRUE (MsWheaESRegionIsValid(NULL));

  MsWheaESWriteData(&Data, (UINT8) (PcdGet8(PcdMsWheaEarlyStorageDefaultValue) - 2), MsWheaESGetMaxDataCount() - 1);
  // should pass
  UT_ASSERT_TRUE (MsWheaESRegionIsValid(NULL));

  return UNIT_TEST_PASSED;
}

/**
  This routine should verify ES can catch header region corruption
**/
UNIT_TEST_STATUS
EFIAPI
MsWheaESHeaderCorruptTest (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  // Header signature corruption
  MsWheaESReadHeader(&UnitTestHeader);
  UnitTestHeader.Signature = SIGNATURE_32('W','H','E','A');
  MsWheaESWriteHeader(&UnitTestHeader);

  // should fail
  UT_ASSERT_FALSE (MsWheaESRegionIsValid(NULL));

  UnitTestHeader.Signature = MS_WHEA_EARLY_STORAGE_SIGNATURE;

  // Header checksum corruption
  UnitTestHeader.Checksum = 0;
  MsWheaESWriteHeader(&UnitTestHeader);

  // should fail
  UT_ASSERT_FALSE (MsWheaESRegionIsValid(NULL));

  return UNIT_TEST_PASSED;
}

/**
  This routine should verify ES header can be updated with proper API
**/
UNIT_TEST_STATUS
EFIAPI
MsWheaESHeaderUpdateTest (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  MsWheaESReadHeader(&UnitTestHeader);

  // Try to update the active range the header
  UnitTestHeader.ActiveRange = sizeof(TestDataArray);
  MsWheaESHeaderChangeChecksumHelper (&UnitTestHeader);

  // should pass
  UT_ASSERT_TRUE (MsWheaESRegionIsValid(NULL));

  // change active range without updating checksum
  UnitTestHeader.ActiveRange = 0;
  MsWheaESWriteHeader(&UnitTestHeader);

  // should fail
  UT_ASSERT_FALSE (MsWheaESRegionIsValid(NULL));

  return UNIT_TEST_PASSED;
}

/**
  This routine should verify ES content can be updated with proper API and catch data corruption
**/
UNIT_TEST_STATUS
EFIAPI
MsWheaESContentUpdateTest (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  UINT8 OriginData;
  UINT8 Data;

  MsWheaESWriteData(TestDataArray, sizeof(TestDataArray), 0);
  MsWheaESContentChangeChecksumHelper(TestDataArray, sizeof(TestDataArray));

  MsWheaESReadHeader(&UnitTestHeader);

  // should pass
  UT_ASSERT_TRUE (MsWheaESRegionIsValid(NULL));
  UT_ASSERT_EQUAL (UnitTestHeader.ActiveRange, sizeof(TestDataArray));

  // Tamper active data region
  Data = PcdGet8(PcdMsWheaEarlyStorageDefaultValue);
  MsWheaESReadData(&OriginData, sizeof(OriginData), 0);
  MsWheaESWriteData(&Data, sizeof(Data), 0);

  // should fail
  UT_ASSERT_FALSE (MsWheaESRegionIsValid(NULL));

  // Recover corrupted byte
  MsWheaESWriteData(&OriginData, sizeof(OriginData), 0);

  // should pass
  UT_ASSERT_TRUE (MsWheaESRegionIsValid(NULL));

  // Corrupt another place within active range
  MsWheaESReadData(&OriginData, sizeof(OriginData), (UINT8)UnitTestHeader.ActiveRange-1);
  MsWheaESWriteData(&Data, sizeof(Data), (UINT8)UnitTestHeader.ActiveRange-1);

  // should fail
  UT_ASSERT_FALSE (MsWheaESRegionIsValid(NULL));

  // Restore the change byte
  MsWheaESWriteData(&OriginData, sizeof(OriginData), (UINT8)UnitTestHeader.ActiveRange-1);

  // should pass
  UT_ASSERT_TRUE (MsWheaESRegionIsValid(NULL));

  return UNIT_TEST_PASSED;
}

/**
  This routine should verify ES find slot plays well with active range field
**/
UNIT_TEST_STATUS
EFIAPI
MsWheaESFindSlotTest (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  UINT8 Slot;

  MsWheaESWriteData(TestDataArray, sizeof(TestDataArray), 0);
  MsWheaESContentChangeChecksumHelper(TestDataArray, sizeof(TestDataArray));

  MsWheaESReadHeader(&UnitTestHeader);

  MsWheaESFindSlot(sizeof(MS_WHEA_EARLY_STORAGE_ENTRY_COMMON), &Slot);

  UT_ASSERT_EQUAL (Slot, UnitTestHeader.ActiveRange);

  return UNIT_TEST_PASSED;
}

/**
  This routine should verify ES recover bad state early storage properly
**/
UNIT_TEST_STATUS
EFIAPI
MsWheaESInitTest (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  // Tamper the early storage first
  MsWheaESReadHeader(&UnitTestHeader);
  UnitTestHeader.Checksum = 0;
  MsWheaESWriteHeader(&UnitTestHeader);

  // Should fail
  UT_ASSERT_FALSE (MsWheaESRegionIsValid(NULL));

  // Holy grail to retore everything
  MsWheaESInit ();

  MsWheaESReadHeader(&UnitTestHeader);
  UT_ASSERT_EQUAL (0, UnitTestHeader.ActiveRange);
  UT_ASSERT_TRUE (MsWheaESRegionIsValid(NULL));

  return UNIT_TEST_PASSED;
}

/**
  This routine should verify ES store/converted supported metadata properly
**/
UNIT_TEST_STATUS
EFIAPI
MsWheaESStoreEntryTest (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  UINT8 *Data;
  MS_WHEA_ERROR_ENTRY_MD  MsWheaEntryMD;
  MS_WHEA_EARLY_STORAGE_ENTRY_V0 ESEntry;
  MS_WHEA_EARLY_STORAGE_ENTRY_V0 *tESEntry;

  SetMem (&MsWheaEntryMD, sizeof(MS_WHEA_ERROR_ENTRY_MD), 0);
  SetMem (&ESEntry, sizeof(MS_WHEA_EARLY_STORAGE_ENTRY_V0), 0);

  MsWheaEntryMD.Rev = ESEntry.Rev = MS_WHEA_REV_0;
  MsWheaEntryMD.Phase = ESEntry.Phase = 0;
  MsWheaEntryMD.ErrorStatusValue = ESEntry.ErrorStatusValue = TEST_ERROR_STATUS_VALUE;
  MsWheaEntryMD.AdditionalInfo1 = ESEntry.AdditionalInfo1 = TEST_ADDITIONAL_INFO_1;
  MsWheaEntryMD.AdditionalInfo2 = ESEntry.AdditionalInfo2 = TEST_ADDITIONAL_INFO_2;

  CopyGuid(&(MsWheaEntryMD.ModuleID), &gEfiCallerIdGuid);
  CopyGuid(&(MsWheaEntryMD.IhvSharingGuid), &gMsWheaReportServiceGuid);
  CopyGuid(&(ESEntry.ModuleID), &gEfiCallerIdGuid);
  CopyGuid(&(ESEntry.PartitionID), &gMsWheaReportServiceGuid);

  MsWheaESStoreEntry (&MsWheaEntryMD);
  MsWheaESStoreEntry (&MsWheaEntryMD);

  MsWheaESReadHeader (&UnitTestHeader);
  UT_ASSERT_EQUAL(sizeof(MS_WHEA_EARLY_STORAGE_ENTRY_V0) + sizeof(MS_WHEA_EARLY_STORAGE_ENTRY_V0),
                  UnitTestHeader.ActiveRange);

  Data = AllocatePool(MsWheaEarlyStorageGetMaxSize());
  MsWheaEarlyStorageRead(Data, MsWheaEarlyStorageGetMaxSize(), 0);

  tESEntry = (MS_WHEA_EARLY_STORAGE_ENTRY_V0*) (Data + sizeof(MS_WHEA_EARLY_STORAGE_HEADER));
  UT_ASSERT_MEM_EQUAL (tESEntry, &ESEntry, sizeof(MS_WHEA_EARLY_STORAGE_ENTRY_V0));

  tESEntry = tESEntry + 1;
  UT_ASSERT_MEM_EQUAL (tESEntry, &ESEntry, sizeof(MS_WHEA_EARLY_STORAGE_ENTRY_V0));

  return UNIT_TEST_PASSED;
}

/**
  This routine should verify ES restore save ES records to metadata properly
**/
UNIT_TEST_STATUS
EFIAPI
MsWheaESProcessTest (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  MS_WHEA_ERROR_ENTRY_MD  MsWheaEntryMD;

  SetMem (&MsWheaEntryMD, sizeof(MS_WHEA_ERROR_ENTRY_MD), 0);

  MsWheaEntryMD.Rev = MS_WHEA_REV_0;
  MsWheaEntryMD.Phase = 0;
  MsWheaEntryMD.ErrorStatusValue = TEST_ERROR_STATUS_VALUE;
  MsWheaEntryMD.AdditionalInfo1 = TEST_ADDITIONAL_INFO_1;
  MsWheaEntryMD.AdditionalInfo2 = TEST_ADDITIONAL_INFO_2;
  CopyGuid(&(MsWheaEntryMD.ModuleID), &gEfiCallerIdGuid);
  CopyGuid(&(MsWheaEntryMD.IhvSharingGuid), &gMsWheaReportServiceGuid);

  MsWheaESStoreEntry (&MsWheaEntryMD);

  MsWheaESProcess (TestReportFunction);

  UT_ASSERT_NOT_NULL (pMsWheaEntryMD);

  // These two fields are populated by the process routine
  MsWheaEntryMD.ErrorSeverity = EFI_GENERIC_ERROR_FATAL;
  MsWheaEntryMD.PayloadSize = sizeof(MS_WHEA_ERROR_ENTRY_MD);
  UT_ASSERT_MEM_EQUAL (pMsWheaEntryMD, &MsWheaEntryMD, sizeof(MS_WHEA_ERROR_ENTRY_MD));

  MsWheaESReadHeader(&UnitTestHeader);

  UT_ASSERT_EQUAL (0, UnitTestHeader.ActiveRange);

  return UNIT_TEST_PASSED;
}

/**
  MsWheaEarlyUnitTestAppEntryPoint

  @param[in] ImageHandle              The firmware allocated handle for the EFI image.
  @param[in] SystemTable              A pointer to the EFI System Table.

  @retval EFI_SUCCESS                 The entry point executed successfully.
  @retval other                       Some error occurred when executing this entry point.

**/
EFI_STATUS
EFIAPI
MsWheaEarlyUnitTestAppEntryPoint (
  IN EFI_HANDLE                       ImageHandle,
  IN EFI_SYSTEM_TABLE                 *SystemTable
  )
{
  EFI_STATUS                  Status = EFI_ABORTED;
  UNIT_TEST_FRAMEWORK_HANDLE  Fw = NULL;
  UNIT_TEST_SUITE_HANDLE      Misc = NULL;

  DEBUG((DEBUG_ERROR, "%a enter\n", __FUNCTION__));

  DEBUG(( DEBUG_ERROR, "%a %a v%a\n", __FUNCTION__, UNIT_TEST_APP_NAME, UNIT_TEST_APP_VERSION ));

  // Start setting up the test framework for running the tests.
  Status = InitUnitTestFramework( &Fw, UNIT_TEST_APP_NAME, gEfiCallerBaseName, UNIT_TEST_APP_VERSION );
  if (EFI_ERROR(Status) != FALSE) {
    DEBUG((DEBUG_ERROR, "%a Failed in InitUnitTestFramework. Status = %r\n", __FUNCTION__, Status));
    goto Cleanup;
  }

  // Misc test suite for all tests.
  CreateUnitTestSuite( &Misc, Fw, "MS WHEA Early Storage Checksum Test cases", "MsWhea.Miscellaneous", NULL, NULL);

  if (Misc == NULL) {
    DEBUG((DEBUG_ERROR, "%a Failed in CreateUnitTestSuite for TestSuite\n", __FUNCTION__));
    Status = EFI_OUT_OF_RESOURCES;
    goto Cleanup;
  }

  AddTestCase(Misc, "Checksum calculation test", "MsWhea.Miscellaneous.MsWheaESChecksumTest",
              MsWheaESChecksumTest, MsWheaESVerify, MsWheaESCleanUp, NULL );

  AddTestCase(Misc, "Inactive data corruption test", "MsWhea.Miscellaneous.MsWheaESDataCorruptTest",
              MsWheaESDataCorruptTest, MsWheaESVerify, MsWheaESCleanUp, NULL );

  AddTestCase(Misc, "Header corruption test", "MsWhea.Miscellaneous.MsWheaESHeaderCorruptTest",
              MsWheaESHeaderCorruptTest, MsWheaESVerify, MsWheaESCleanUp, NULL );

  AddTestCase(Misc, "Header update test", "MsWhea.Miscellaneous.MsWheaESHeaderUpdateTest",
              MsWheaESHeaderUpdateTest, MsWheaESVerify, MsWheaESCleanUp, NULL );

  AddTestCase(Misc, "Content update and corrupt", "MsWhea.Miscellaneous.MsWheaESContentUpdateTest",
              MsWheaESContentUpdateTest, MsWheaESVerify, MsWheaESCleanUp, NULL );

  AddTestCase(Misc, "Free ES slot find", "MsWhea.Miscellaneous.MsWheaESFindSlotTest",
              MsWheaESFindSlotTest, MsWheaESVerify, MsWheaESCleanUp, NULL );

  AddTestCase(Misc, "MsWhea ES Init", "MsWhea.Miscellaneous.MsWheaESInitTest",
              MsWheaESInitTest, MsWheaESVerify, MsWheaESCleanUp, NULL );

  AddTestCase(Misc, "MsWhea ES store entry", "MsWhea.Miscellaneous.MsWheaESStoreEntryTest",
              MsWheaESStoreEntryTest, MsWheaESVerify, MsWheaESCleanUp, NULL );

  AddTestCase(Misc, "MsWhea ES process entry", "MsWhea.Miscellaneous.MsWheaESProcessTest",
              MsWheaESProcessTest, MsWheaESVerify, MsWheaESCleanUp, NULL );

  //
  // Execute the tests.
  //
  Status = RunAllTestSuites(Fw);

Cleanup:
  if (Fw != NULL) {
    FreeUnitTestFramework(Fw);
  }
  DEBUG((DEBUG_ERROR, "%a exit\n", __FUNCTION__));
  return Status;
} // MsWheaEarlyUnitTestAppEntryPoint ()
