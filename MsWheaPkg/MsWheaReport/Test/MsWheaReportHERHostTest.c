/** @file -- MsWheaReportHERHostTest.c
Host-based UnitTest for HER routines in the MsWheaReport driver.

Copyright (c) Microsoft Corporation
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/UnitTestLib.h>
#include <Library/ReportStatusCodeLib.h>

#include "../MsWheaReportHER.h"

#include <MsWheaHostTestCommon.h>

#ifndef INTERNAL_UNIT_TEST
#error Make sure to build this with INTERNAL_UNIT_TEST enabled! Otherwise, some important tests may be skipped!
#endif

#define UNIT_TEST_NAME     "MsWheaReport HER Unit Test"
#define UNIT_TEST_VERSION  "0.1"

//
// Prototypes of Internal Functions
//   These functions are normally STATIC, but we're testing them anyway.
//
EFI_STATUS
MsWheaFindNextAvailableSlot (
  OUT UINT16      *next
  );

VOID *
MsWheaAnFBuffer (
  CONST IN MS_WHEA_ERROR_ENTRY_MD     *MsWheaEntryMD,
  IN OUT UINT32                       *PayloadSize
  );


/**
A mocked version of GetVariable.

@retval EFI_NOT_READY                 If requested service is not yet available
@retval Others                        See EFI_GET_VARIABLE for more details

**/
EFI_STATUS
EFIAPI
WheaGetVariable (
  IN     CHAR16                      *VariableName,
  IN     EFI_GUID                    *VendorGuid,
  OUT    UINT32                      *Attributes,    OPTIONAL
  IN OUT UINTN                       *DataSize,
  OUT    VOID                        *Data           OPTIONAL
  )
{
  EFI_STATUS    ReturnStatus;

  ReturnStatus = (EFI_STATUS)mock();

  return ReturnStatus;
}

/**
A mocked version of GetNextVariableName.

@retval EFI_NOT_READY                 If requested service is not yet available
@retval Others                        See EFI_GET_NEXT_VARIABLE_NAME for more details

**/
EFI_STATUS
EFIAPI
WheaGetNextVariableName (
  IN OUT UINTN                    *VariableNameSize,
  IN OUT CHAR16                   *VariableName,
  IN OUT EFI_GUID                 *VendorGuid
  )
{
  return EFI_ABORTED;
}

/**
A mocked version of SetVariable.

@retval EFI_NOT_READY                 If requested service is not yet available
@retval Others                        See EFI_SET_VARIABLE for more details

**/
EFI_STATUS
EFIAPI
WheaSetVariable (
  IN  CHAR16                       *VariableName,
  IN  EFI_GUID                     *VendorGuid,
  IN  UINT32                       Attributes,
  IN  UINTN                        DataSize,
  IN  VOID                         *Data
  )
{
  return EFI_ABORTED;
}

BOOLEAN
PopulateTime (
  EFI_TIME  *CurrentTime
  )
{
  // Stub implementation.
  return FALSE;
}

EFI_STATUS
GetRecordID (
  UINT64    *RecordID,
  EFI_GUID  *RecordIDGuid
  )
{
  // Stub implementation.
  return EFI_ABORTED;
}

EFI_STATUS
EFIAPI
MsWheaESStoreEntry (
  IN MS_WHEA_ERROR_ENTRY_MD           *MsWheaEntryMD
  )
{
  // Stub implementation.
  return EFI_SUCCESS;
}

//
//
// UNIT TEST CASES
//
//

UNIT_TEST_STATUS
EFIAPI
FindNextShouldFailOnError (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT16  Result;

  will_return_always(WheaGetVariable, EFI_ABORTED);
  UT_ASSERT_TRUE(EFI_ERROR(MsWheaFindNextAvailableSlot(&Result)));

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
FindNextShouldReturnFirstSlotIfThereAreNone (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT16  Result;

  will_return_always(WheaGetVariable, EFI_NOT_FOUND);
  UT_ASSERT_NOT_EFI_ERROR(MsWheaFindNextAvailableSlot(&Result));
  UT_ASSERT_EQUAL(Result, 0);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
FindNextShouldReturnSlotNumberOfNextSlot (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT16  Result;

  will_return_count(WheaGetVariable, EFI_SUCCESS, 0x12);
  will_return_always(WheaGetVariable, EFI_NOT_FOUND);

  UT_ASSERT_NOT_EFI_ERROR(MsWheaFindNextAvailableSlot(&Result));
  UT_ASSERT_EQUAL(Result, 0x12);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
FindNextShouldFailIfItRunsOutOfSlots (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT16  Result;

  will_return_always(WheaGetVariable, EFI_SUCCESS);
  UT_ASSERT_STATUS_EQUAL(MsWheaFindNextAvailableSlot(&Result), EFI_OUT_OF_RESOURCES);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
AnFHandleOutOfResources (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  // Not yet implemented.
  // OUT_OF_RESOURCES - Will require Allocate*() mocks
  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
AnFCorrectlyPopulatesFixedSizedData (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  MS_WHEA_ERROR_ENTRY_MD    TestEntry;
  UINT32                    BufferSize;
  UINT8                     *Buffer;
  UINTN                     Off;

  // Set up the test data.
  ZeroMem(&TestEntry, sizeof(TestEntry));
  // For now we'll skip PayloadSize because its use seems erratic.
  // TestEntry.PayloadSize = 0;
  TestEntry.Phase             = MS_WHEA_PHASE_DXE;
  TestEntry.ErrorSeverity     = EFI_GENERIC_ERROR_FATAL;
  TestEntry.ErrorStatusValue  = TEST_RSC_CRITICAL_5;
  TestEntry.AdditionalInfo1   = 0xDEADBEEFDEADBEEF;
  TestEntry.AdditionalInfo2   = 0xFEEDF00DFEEDF00D;
  CopyGuid(&TestEntry.ModuleID, &mTestGuid1);
  CopyGuid(&TestEntry.LibraryID, &mTestGuid2);
  CopyGuid(&TestEntry.IhvSharingGuid, &mTestGuid3);

  Buffer = MsWheaAnFBuffer(&TestEntry, &BufferSize);
  UT_ASSERT_NOT_NULL(Buffer);

  UT_ASSERT_EQUAL(BufferSize, (sizeof(EFI_COMMON_ERROR_RECORD_HEADER) + 
                                sizeof(EFI_ERROR_SECTION_DESCRIPTOR) +
                                sizeof(MU_TELEMETRY_CPER_SECTION_DATA)));

  // Validate the CPER Main Header
  UT_ASSERT_EQUAL(ReadUnaligned32((UINT32*)&Buffer[0]), EFI_ERROR_RECORD_SIGNATURE_START);   // SignatureStart
  UT_ASSERT_EQUAL(ReadUnaligned16((UINT16*)&Buffer[4]), EFI_ERROR_RECORD_REVISION);          // Revision
  UT_ASSERT_EQUAL(ReadUnaligned32((UINT32*)&Buffer[6]), EFI_ERROR_RECORD_SIGNATURE_END);     // SignatureEnd
  UT_ASSERT_EQUAL(ReadUnaligned16((UINT16*)&Buffer[10]), 1);                                 // SectionCount
  UT_ASSERT_EQUAL(ReadUnaligned32((UINT32*)&Buffer[12]), EFI_GENERIC_ERROR_FATAL);           // ErrorSeverity
  // UINT32                                   // ValidationBits;
  UT_ASSERT_EQUAL(ReadUnaligned32((UINT32*)&Buffer[20]), BufferSize);                        // RecordLength
  // EFI_ERROR_TIME_STAMP                     // TimeStamp;
  UT_ASSERT_TRUE(CompareGuid((EFI_GUID*)&Buffer[32], PcdGetPtr(PcdDeviceIdentifierGuid)));   // PlatformID;
  UT_ASSERT_TRUE(CompareGuid((EFI_GUID*)&Buffer[48], &mTestGuid3));                          // PartitionID;
  UT_ASSERT_TRUE(CompareGuid((EFI_GUID*)&Buffer[64], &gMsWheaReportServiceGuid));            // CreatorID;
  UT_ASSERT_TRUE(CompareGuid((EFI_GUID*)&Buffer[80], &gEfiEventNotificationTypeBootGuid));   // NotificationType;
  // UINT64                                   // RecordID;
  // UINT32                                   // Flags;
  // UINT64                                   // PersistenceInfo;
  
  // Validate the CPER MU Telemetry Section Header
  Off = sizeof(EFI_COMMON_ERROR_RECORD_HEADER);
  UT_ASSERT_EQUAL(ReadUnaligned32((UINT32*)&Buffer[Off+0]), sizeof(EFI_COMMON_ERROR_RECORD_HEADER) +
                                                                sizeof(EFI_ERROR_SECTION_DESCRIPTOR)); // SectionOffset;
  UT_ASSERT_EQUAL(ReadUnaligned32((UINT32*)&Buffer[Off+4]), sizeof(MU_TELEMETRY_CPER_SECTION_DATA));   // SectionLength;
  UT_ASSERT_EQUAL(ReadUnaligned16((UINT16*)&Buffer[Off+8]), MS_WHEA_SECTION_REVISION);       // Revision;
  // UINT8                  SecValidMask;
  // UINT8                  Resv1;
  // UINT32                 SectionFlags;
  UT_ASSERT_TRUE(CompareGuid((EFI_GUID*)&Buffer[Off+16], &gMuTelemetrySectionTypeGuid));     // SectionType;
  // EFI_GUID               FruId;
  UT_ASSERT_EQUAL(ReadUnaligned32((UINT32*)&Buffer[Off+48]), EFI_GENERIC_ERROR_FATAL);       // Severity;
  // CHAR8                  FruString[20];
  
  // Validate the CPER MU Telemetry Section
  Off += sizeof(EFI_ERROR_SECTION_DESCRIPTOR);
  UT_ASSERT_TRUE(CompareGuid((EFI_GUID*)&Buffer[Off+0], &mTestGuid1));                       // ComponentID;
  UT_ASSERT_TRUE(CompareGuid((EFI_GUID*)&Buffer[Off+16], &mTestGuid2));                      // SubComponentID;
  // UINT32                              Reserved;
  UT_ASSERT_EQUAL(ReadUnaligned32((UINT32*)&Buffer[Off+36]), TEST_RSC_CRITICAL_5);           // ErrorStatusValue;
  UT_ASSERT_EQUAL(ReadUnaligned64((UINT64*)&Buffer[Off+40]), 0xDEADBEEFDEADBEEF);            // AdditionalInfo1;
  UT_ASSERT_EQUAL(ReadUnaligned64((UINT64*)&Buffer[Off+48]), 0xFEEDF00DFEEDF00D);            // AdditionalInfo2;

  if (Buffer != NULL) {
    FreePool(Buffer);
  }

  // Finish me.
  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
AnFCorrectlyPopulatesDynamicallySizedData (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  MS_WHEA_ERROR_EXTRA_SECTION_DATA    *ExtraData;
  CHAR8                               ExtraDataContents[] = "<Note>This is my dummy packed data.</Note><Structure>DEADBEEF</Structure>";
  MS_WHEA_ERROR_ENTRY_MD              TestEntry;
  UINT32                              BufferSize;
  UINT8                               *Buffer;
  UINTN                               Off;

  // Set up the test data.
  ZeroMem(&TestEntry, sizeof(TestEntry));
  // For now we'll skip PayloadSize because its use seems erratic.
  // TestEntry.PayloadSize = 0;
  TestEntry.Phase             = MS_WHEA_PHASE_DXE_VAR;
  TestEntry.ErrorSeverity     = EFI_GENERIC_ERROR_FATAL;
  TestEntry.ErrorStatusValue  = TEST_RSC_CRITICAL_B;
  TestEntry.AdditionalInfo1   = 0x1234567890ABCDEF;
  TestEntry.AdditionalInfo2   = 0xFEDCBA0987654321;
  CopyGuid(&TestEntry.ModuleID, &mTestGuid3);
  CopyGuid(&TestEntry.LibraryID, &mTestGuid1);
  CopyGuid(&TestEntry.IhvSharingGuid, &mTestGuid2);

  // Set up the extra data.
  ExtraData = AllocatePool(sizeof(*ExtraData) + sizeof(ExtraDataContents));
  ExtraData->DataSize = sizeof(ExtraDataContents);
  CopyGuid(&ExtraData->SectionGuid, &mTestGuid2);
  CopyMem(&ExtraData->Data, ExtraDataContents, ExtraData->DataSize);
  TestEntry.ExtraSection = (EFI_PHYSICAL_ADDRESS) (UINTN) ExtraData;

  Buffer = MsWheaAnFBuffer(&TestEntry, &BufferSize);
  UT_ASSERT_NOT_NULL(Buffer);

  UT_ASSERT_EQUAL(BufferSize, (sizeof(EFI_COMMON_ERROR_RECORD_HEADER) + 
                                sizeof(EFI_ERROR_SECTION_DESCRIPTOR) +
                                sizeof(MU_TELEMETRY_CPER_SECTION_DATA) + 
                                sizeof(EFI_ERROR_SECTION_DESCRIPTOR) +
                                ExtraData->DataSize));

  // Validate the CPER Main Header
  UT_ASSERT_EQUAL(ReadUnaligned32((UINT32*)&Buffer[0]), EFI_ERROR_RECORD_SIGNATURE_START);   // SignatureStart
  UT_ASSERT_EQUAL(ReadUnaligned16((UINT16*)&Buffer[4]), EFI_ERROR_RECORD_REVISION);          // Revision
  UT_ASSERT_EQUAL(ReadUnaligned32((UINT32*)&Buffer[6]), EFI_ERROR_RECORD_SIGNATURE_END);     // SignatureEnd
  UT_ASSERT_EQUAL(ReadUnaligned16((UINT16*)&Buffer[10]), 2);                                 // SectionCount
  UT_ASSERT_EQUAL(ReadUnaligned32((UINT32*)&Buffer[12]), EFI_GENERIC_ERROR_FATAL);           // ErrorSeverity
  // UINT32                                   // ValidationBits;
  UT_ASSERT_EQUAL(ReadUnaligned32((UINT32*)&Buffer[20]), BufferSize);                        // RecordLength
  // EFI_ERROR_TIME_STAMP                     // TimeStamp;
  UT_ASSERT_TRUE(CompareGuid((EFI_GUID*)&Buffer[32], PcdGetPtr(PcdDeviceIdentifierGuid)));   // PlatformID;
  UT_ASSERT_TRUE(CompareGuid((EFI_GUID*)&Buffer[48], &mTestGuid2));                          // PartitionID;
  UT_ASSERT_TRUE(CompareGuid((EFI_GUID*)&Buffer[64], &gMsWheaReportServiceGuid));            // CreatorID;
  UT_ASSERT_TRUE(CompareGuid((EFI_GUID*)&Buffer[80], &gEfiEventNotificationTypeBootGuid));   // NotificationType;
  // UINT64                                   // RecordID;
  // UINT32                                   // Flags;
  // UINT64                                   // PersistenceInfo;
  Off = sizeof(EFI_COMMON_ERROR_RECORD_HEADER);
  
  // Validate the CPER MU Telemetry Section Header
  UT_ASSERT_EQUAL(ReadUnaligned32((UINT32*)&Buffer[Off+0]), sizeof(EFI_COMMON_ERROR_RECORD_HEADER) +
                                                                sizeof(EFI_ERROR_SECTION_DESCRIPTOR) +
                                                                sizeof(EFI_ERROR_SECTION_DESCRIPTOR)); // SectionOffset;
  UT_ASSERT_EQUAL(ReadUnaligned32((UINT32*)&Buffer[Off+4]), sizeof(MU_TELEMETRY_CPER_SECTION_DATA));   // SectionLength;
  UT_ASSERT_EQUAL(ReadUnaligned16((UINT16*)&Buffer[Off+8]), MS_WHEA_SECTION_REVISION);       // Revision;
  // UINT8                  SecValidMask;
  // UINT8                  Resv1;
  // UINT32                 SectionFlags;
  UT_ASSERT_TRUE(CompareGuid((EFI_GUID*)&Buffer[Off+16], &gMuTelemetrySectionTypeGuid));     // SectionType;
  // EFI_GUID               FruId;
  UT_ASSERT_EQUAL(ReadUnaligned32((UINT32*)&Buffer[Off+48]), EFI_GENERIC_ERROR_FATAL);       // Severity;
  // CHAR8                  FruString[20];
  Off += sizeof(EFI_ERROR_SECTION_DESCRIPTOR);
  
  // Validate the CPER MU Telemetry Extra Section Header
  UT_ASSERT_EQUAL(ReadUnaligned32((UINT32*)&Buffer[Off+0]), sizeof(EFI_COMMON_ERROR_RECORD_HEADER) +
                                                                sizeof(EFI_ERROR_SECTION_DESCRIPTOR) +
                                                                sizeof(EFI_ERROR_SECTION_DESCRIPTOR) +
                                                                sizeof(MU_TELEMETRY_CPER_SECTION_DATA)); // SectionOffset;
  UT_ASSERT_EQUAL(ReadUnaligned32((UINT32*)&Buffer[Off+4]), sizeof(ExtraDataContents));      // SectionLength;
  UT_ASSERT_EQUAL(ReadUnaligned16((UINT16*)&Buffer[Off+8]), MS_WHEA_SECTION_REVISION);       // Revision;
  // UINT8                  SecValidMask;
  // UINT8                  Resv1;
  // UINT32                 SectionFlags;
  UT_ASSERT_TRUE(CompareGuid((EFI_GUID*)&Buffer[Off+16], &mTestGuid2)); // SectionType;
  // EFI_GUID               FruId;
  UT_ASSERT_EQUAL(ReadUnaligned32((UINT32*)&Buffer[Off+48]), EFI_GENERIC_ERROR_FATAL);        // Severity;
  // CHAR8                  FruString[20];
  Off += sizeof(EFI_ERROR_SECTION_DESCRIPTOR);
  
  // Validate the CPER MU Telemetry Section
  UT_ASSERT_TRUE(CompareGuid((EFI_GUID*)&Buffer[Off+0], &mTestGuid3));                       // ComponentID;
  UT_ASSERT_TRUE(CompareGuid((EFI_GUID*)&Buffer[Off+16], &mTestGuid1));                      // SubComponentID;
  // UINT32                              Reserved;
  UT_ASSERT_EQUAL(ReadUnaligned32((UINT32*)&Buffer[Off+36]), TEST_RSC_CRITICAL_B);           // ErrorStatusValue;
  UT_ASSERT_EQUAL(ReadUnaligned64((UINT64*)&Buffer[Off+40]), 0x1234567890ABCDEF);            // AdditionalInfo1;
  UT_ASSERT_EQUAL(ReadUnaligned64((UINT64*)&Buffer[Off+48]), 0xFEDCBA0987654321);            // AdditionalInfo2;
  Off += sizeof(MU_TELEMETRY_CPER_SECTION_DATA);
  
  // Validate the CPER MU Telemetry Section
  UT_ASSERT_MEM_EQUAL(&Buffer[Off], ExtraDataContents, sizeof(ExtraDataContents));

  if (Buffer != NULL) {
    FreePool(Buffer);
  }
  if (ExtraData != NULL) {
    FreePool(ExtraData);
  }

  // Finish me.
  return UNIT_TEST_PASSED;
}

// TODO: Test MsWheaAnFBuffer for exceeding the MaxHwRecErrSize. ASSERT in these cases
//      because this should be caught in dev.

// TODO: Add a test for MsWheaReportHERAdd that checks LCD saving.
//    If out of resources, drop extra data.
//    If still out of resources, drop MuTelemetry data.

/**
  Initialize the unit test framework, suite, and unit tests for the
  sample unit tests and run the unit tests.

  @retval  EFI_SUCCESS           All test cases were dispatched.
  @retval  EFI_OUT_OF_RESOURCES  There are not enough resources available to
                                 initialize the unit tests.
**/
EFI_STATUS
EFIAPI
UefiTestMain (
  VOID
  )
{
  EFI_STATUS                  Status;
  UNIT_TEST_FRAMEWORK_HANDLE  Framework;
  UNIT_TEST_SUITE_HANDLE      FindNextSuite;
  UNIT_TEST_SUITE_HANDLE      AnFBufferSuite;

  Framework = NULL;

  DEBUG(( DEBUG_INFO, "%a v%a\n", UNIT_TEST_NAME, UNIT_TEST_VERSION ));

  //
  // Start setting up the test framework for running the tests.
  //
  Status = InitUnitTestFramework (&Framework, UNIT_TEST_NAME, gEfiCallerBaseName, UNIT_TEST_VERSION);
  if (EFI_ERROR (Status)) {
    DEBUG((DEBUG_ERROR, "Failed in InitUnitTestFramework. Status = %r\n", Status));
    goto EXIT;
  }

  //
  // Populate the FindNextSuite Unit Test Suite.
  //
  Status = CreateUnitTestSuite (&FindNextSuite, Framework, "MsWheaFindNextAvailableSlot Tests", "FindNext.General", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for FindNextSuite\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }
  AddTestCase (FindNextSuite, "Should fail if GetVariable fails", "FailOnError", FindNextShouldFailOnError, NULL, NULL, NULL);
  AddTestCase (FindNextSuite, "Should return first slot if there are none", "ReturnFirst", FindNextShouldReturnFirstSlotIfThereAreNone, NULL, NULL, NULL);
  AddTestCase (FindNextSuite, "Should return slot number of next slot", "ReturnNext", FindNextShouldReturnSlotNumberOfNextSlot, NULL, NULL, NULL);
  AddTestCase (FindNextSuite, "Should fail if it runs out of slots", "FailOnNone", FindNextShouldFailIfItRunsOutOfSlots, NULL, NULL, NULL);

  //
  // Populate the AnFBufferSuite Unit Test Suite.
  //
  Status = CreateUnitTestSuite (&AnFBufferSuite, Framework, "MsWheaAnFBuffer Tests", "AnFBuffer.General", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for AnFBufferSuite\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }
  AddTestCase (AnFBufferSuite, "AnFHandleOutOfResources", "OutOfResources", AnFHandleOutOfResources, NULL, NULL, NULL);
  AddTestCase (AnFBufferSuite, "AnFCorrectlyPopulatesFixedSizedData", "FixedSizedData", AnFCorrectlyPopulatesFixedSizedData, NULL, NULL, NULL);
  AddTestCase (AnFBufferSuite, "AnFCorrectlyPopulatesDynamicallySizedData", "DynamicallySizedData", AnFCorrectlyPopulatesDynamicallySizedData, NULL, NULL, NULL);

  //
  // Execute the tests.
  //
  Status = RunAllTestSuites (Framework);

EXIT:
  if (Framework) {
    FreeUnitTestFramework (Framework);
  }

  return Status;
}

/**
  Standard POSIX C entry point for host based unit test execution.
**/
int
main (
  int argc,
  char *argv[]
  )
{
  return UefiTestMain ();
}
