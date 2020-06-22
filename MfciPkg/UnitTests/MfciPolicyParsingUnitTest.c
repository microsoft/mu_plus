/** @file
  Unit tests of the MfciPolicyParsingLib of the MfciPkg

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>

#include <Library/UnitTestLib.h>

#include <Include/MfciPolicyType.h>

#include "Private/MfciPolicyFields.h"
#include "Private/Library/MfciPolicyParsingLib.h"
#include "Private/Library/MfciPolicyParsingLib/MfciPolicyParsingLibInternal.h"

#include "data/certs/CA_NotTrusted.cer.h"
#include "data/packets/policy_good_manufacturing.bin.h"
#include "data/packets/policy_good_manufacturing.bin.p7.h"
#include "data/packets/policy_badFormatVersion.bin.h"
#include "data/packets/policy_badPolicyVersion.bin.h"
#include "data/packets/policy_badPolicyPublisher.bin.h"
#include "data/packets/policy_badReserved1Count.bin.h"
#include "data/packets/policy_badReserved2Count.bin.h"
#include "data/packets/policy_badOptionFlags.bin.h"
#include "data/packets/policy_RuleCount0.bin.h"
#include "data/packets/policy_RuleCountFFFF.bin.h"
#include "data/packets/policy_badRuleRootKey.bin.h"
#include "data/packets/policy_badRuleSubKeyNameOffsetFFFFFFFF.bin.h"
#include "data/packets/policy_badRuleValueNameOffsetFFFFFFFF.bin.h"
#include "data/packets/policy_badRuleValueOffsetFFFFFFFF.bin.h"
#include "data/packets/policy_badSubKeySizeFFFF.bin.h"
#include "data/packets/policy_badValueNameSizeFFFF.bin.h"
#include "data/packets/policy_badValueType.bin.h"
#include "data/packets/policy_badValueStringSizeFFFF.bin.h"

#define UNIT_TEST_APP_NAME        "MfciPolicyParsingLibUnitTest"
#define UNIT_TEST_APP_VERSION     "1.0"


typedef struct {
  CHAR8          *Description;
  EFI_STATUS      ExpectedStatus;
  CONST UINT8    *SignedBlob;
  UINTN           BlobSize;
  CONST UINT8   **TrustAnchor; // DER Cert
  UINTN          *TrustAnchorSize;
  CONST CHAR8   **EKU;
} VALIDATEBLOB_TEST_CONTEXT;

typedef struct {
  CHAR8          *Description;
  EFI_STATUS      ExpectedStatus;
  CONST UINT8    *Blob;
  UINTN           Size;
} SANITYCHECK_TEST_CONTEXT;

typedef struct {
  CHAR8          *Description;
  EFI_STATUS      ExpectedStatus;
  CONST UINT8    *SignedBlob;
  UINTN           BlobSize;
  CONST CHAR16   *ValueName;
  UINT64          ExpectedValue;
 } EXTRACT_UINT64_TEST_CONTEXT;

typedef struct {
  CHAR8          *Description;
  EFI_STATUS      ExpectedStatus;
  CONST UINT8    *SignedBlob;
  UINTN           BlobSize;
  CONST CHAR16   *ValueName;
  CONST CHAR16   *ExpectedValue;
  CHAR16         *ExtractedString;
} EXTRACT_CHAR16_TEST_CONTEXT;


///================================================================================================
///================================================================================================
///
/// HELPER FUNCTIONS
///
///================================================================================================
///================================================================================================


UNIT_TEST_STATUS
EFIAPI
TestValidateSignature (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  VALIDATEBLOB_TEST_CONTEXT *btx = (VALIDATEBLOB_TEST_CONTEXT*)Context;

  UT_LOG_VERBOSE( "\n%a()", __FUNCTION__ );
  UT_LOG_VERBOSE( "\nParameters: 0x%p, 0x%X, 0x%p, 0x%X, 0x%p", btx->SignedBlob, btx->BlobSize, *(btx->TrustAnchor), *(btx->TrustAnchorSize), *(btx->EKU));
  DEBUG((DEBUG_INFO,"\n%a( 0x%p, 0x%X, 0x%p, 0x%X, 0x%p )\n", __FUNCTION__, btx->SignedBlob, btx->BlobSize, *(btx->TrustAnchor), *(btx->TrustAnchorSize), *(btx->EKU) ));

  Status = ValidateSignature (btx->SignedBlob, btx->BlobSize, *(btx->TrustAnchor), *(btx->TrustAnchorSize), *(btx->EKU));

  UT_ASSERT_STATUS_EQUAL(Status, btx->ExpectedStatus);

  return UNIT_TEST_PASSED;
} // TestValidateSignature()

UNIT_TEST_STATUS
EFIAPI
TestSanityCheckPolicy (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  SANITYCHECK_TEST_CONTEXT *btx = (SANITYCHECK_TEST_CONTEXT*)Context;
  
  UT_LOG_VERBOSE( "\n%a()\n", __FUNCTION__ );
  UT_LOG_VERBOSE( "Parameters: 0x%p , 0x%X\n", btx->Blob, btx->Size);

  Status = SanityCheckPolicy ((CONST MfciPolicyBlob*)btx->Blob, btx->Size);

  UT_ASSERT_STATUS_EQUAL(Status, btx->ExpectedStatus);

  return UNIT_TEST_PASSED;
} // TestSanityCheckPolicy()

UNIT_TEST_STATUS
EFIAPI
TestExtractUint64 (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  EXTRACT_UINT64_TEST_CONTEXT *btx = (EXTRACT_UINT64_TEST_CONTEXT*)Context;
  UINT64 u64 = MFCI_POLICY_VALUE_INVALID;
  
  UT_LOG_VERBOSE( "\n%a():  %a\n", __FUNCTION__, btx->Description);
  DEBUG((DEBUG_INFO, "\n%a():  %a", __FUNCTION__, btx->Description ) );

  Status = ExtractUint64(btx->SignedBlob, btx->BlobSize, btx->ValueName, &u64);

  UT_ASSERT_STATUS_EQUAL(Status, btx->ExpectedStatus);

  UT_ASSERT_EQUAL(btx->ExpectedValue, u64);

  return UNIT_TEST_PASSED;
} // TestExtractUint64

UNIT_TEST_STATUS
EFIAPI
TestExtractChar16 (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  EXTRACT_CHAR16_TEST_CONTEXT *btx = (EXTRACT_CHAR16_TEST_CONTEXT*)Context;

  UT_LOG_VERBOSE( "%a():  %a\n", __FUNCTION__, btx->Description);
  DEBUG((DEBUG_INFO, "%a():  %a\n", __FUNCTION__, btx->Description ) );

  Status = ExtractChar16(btx->SignedBlob, btx->BlobSize, btx->ValueName, &btx->ExtractedString);

  UT_ASSERT_STATUS_EQUAL(Status, btx->ExpectedStatus);

  if (btx->ExpectedValue == NULL || btx->ExtractedString == NULL) {
    UT_ASSERT_EQUAL(btx->ExpectedValue, btx->ExtractedString);
  } 
  else {
    DEBUG((DEBUG_INFO, "Found String: '%s'\n", btx->ExtractedString));
    DEBUG((DEBUG_INFO, "Expected String: '%s'\n", btx->ExpectedValue));
    UINTN StringCompare = StrnCmp(btx->ExtractedString, btx->ExpectedValue, POLICY_STRING_MAX_LENGTH);
    DEBUG((DEBUG_INFO, "StringCompare: 0x%x\n", StringCompare));
    UT_ASSERT_TRUE(StringCompare == 0);
  }

  if (btx->ExtractedString) {
    FreePool(btx->ExtractedString);
    btx->ExtractedString = NULL;
  }

  return UNIT_TEST_PASSED;
} // TestExtractChar16

VOID
EFIAPI
CleanUpExtractChar16 (
IN UNIT_TEST_CONTEXT           Context
)
{
  EXTRACT_CHAR16_TEST_CONTEXT *btx = (EXTRACT_CHAR16_TEST_CONTEXT*)Context;

  if (btx != NULL)
  {
    //free string if set
    if (btx->ExtractedString != NULL)
    {
      FreePool(btx->ExtractedString);
      btx->ExtractedString = NULL;
    }
  }
}

///================================================================================================
///================================================================================================
///
/// TEST ENGINE
///
///================================================================================================
///================================================================================================


/**
  EntryPoint

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point executed successfully.
  @retval other           Some error occurred when executing this entry point.

**/
EFI_STATUS
EFIAPI
EntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                  Status;
  UNIT_TEST_FRAMEWORK_HANDLE  Framework = NULL;
  UNIT_TEST_SUITE_HANDLE      SignatureVerificationTests;
  UNIT_TEST_SUITE_HANDLE      SanityVerificationTests;
  UNIT_TEST_SUITE_HANDLE      ExtractValueTests;

  AsciiPrint ("%a v%a\n", UNIT_TEST_APP_NAME, UNIT_TEST_APP_VERSION );
  DEBUG((DEBUG_ERROR, "%a v%a\n", UNIT_TEST_APP_NAME, UNIT_TEST_APP_VERSION ));

  //
  // Start setting up the test framework for running the tests.
  //
  Status = InitUnitTestFramework( &Framework, UNIT_TEST_APP_NAME, gEfiCallerBaseName, UNIT_TEST_APP_VERSION );
  if (EFI_ERROR( Status ))
  {
    DEBUG((DEBUG_ERROR, "Failed in InitUnitTestFramework. Status = %r\n", Status));
    goto EXIT;
  }

  //
  // Populate the Signature Verification Unit Test Suite.
  //
  Status = CreateUnitTestSuite( &SignatureVerificationTests, Framework, "Signature Verification Tests", "MfciPolicy.ParserLib.SignatureVerification", NULL, NULL );
  if (EFI_ERROR( Status ))
  {
    DEBUG((DEBUG_ERROR, "Failed in CreateUnitTestSuite for SignatureVerificationTests, Status = %r\n", Status));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  //*----------------------------------------------------------------------------------*
  //* Test Contexts                                                                    *
  //*----------------------------------------------------------------------------------*
  
  // below is inspired/borrowed from FmpDxe.c
  CONST UINT8* CONST PublicKeyDataXdr = FixedPcdGetPtr(PcdMfciPkcs7CertBufferXdr);
  CONST UINT8* CONST PublicKeyDataXdrEnd = PublicKeyDataXdr + FixedPcdGetSize(PcdMfciPkcs7CertBufferXdr);

  if (PublicKeyDataXdr == NULL || ((PublicKeyDataXdr + sizeof (UINT32)) > PublicKeyDataXdrEnd) ) {
    DEBUG ((DEBUG_ERROR, "Pcd PcdMfciPkcs7CertBufferXdr NULL or invalid size\n"));
    Status = EFI_ABORTED;
    goto EXIT;
  }

  // Read key length stored in big-endian format
  //
  CONST UINTN PublicKeyDataLength = SwapBytes32 (*(UINT32 *)(PublicKeyDataXdr));
  //
  // Point to the start of the key data
  //
  CONST UINT8* CONST PublicKeyData = PublicKeyDataXdr + sizeof (UINT32);

  // Only 1 certificate is supported
  // Length + sizeof(CHAR8) because there is a terminating NULL byte
  if (PublicKeyData + PublicKeyDataLength + sizeof(CHAR8) != PublicKeyDataXdrEnd) {
    DEBUG ((DEBUG_ERROR, "PcdMfciPkcs7CertBufferXdr size mismatch: PublicKeyData(0x%x) PublicKeyDataLength(0x%x) PublicKeyDataXdrEnd(0x%x)", PublicKeyData, PublicKeyDataLength, PublicKeyDataXdrEnd));
    Status = EFI_ABORTED;
    goto EXIT;
  }
  // above is inspired/borrowed from FmpDxe.c


  // The test case data contexts are large, to be nice to the stack we define them as static
  // Some of the data comes from PCDs (not constant), and it was necessary to pass that data
  // via static pointers to the data.  Unfortinately to test a 0 or NULL, this required declaring
  // static pointers to 0 and NULL.
  //
  // TODO: any suggestions on friendler code or comment for below code?

  static CONST VOID* CONST  mNullPtr = NULL;
  static CONST UINT8* mCertCA;
  mCertCA = PublicKeyData;
  static UINTN mCertCaLength;
  mCertCaLength = PublicKeyDataLength;
  // TODO: is there a better way to handle the below pointer to const array?
  // static CONST UINT8*  (*mCertInvalidCaPtr)[sizeof(mCertCA_NotTrusted)] = NULL;
  static CONST UINT8    *mCertInvalidCaPtr = &mCertCA_NotTrusted[0];
  static UINTN           mCertInvalidCaLength = sizeof(mCertCA_NotTrusted);
  static CONST CHAR8    *mTestEKU = FixedPcdGetPtr(PcdMfciPkcs7RequiredLeafEKU);
  static CONST CHAR8    *mTestInvalidEKU = "1.3.6.1.4.1.311.45.255.0";

  static VALIDATEBLOB_TEST_CONTEXT mTestVB1  = { "Good Signature",        EFI_SUCCESS            , mSigned_policy_good_manufacturing, sizeof(mSigned_policy_good_manufacturing),   &mCertCA,            &mCertCaLength           , &mTestEKU};
  static VALIDATEBLOB_TEST_CONTEXT mTestVB2  = { "*SignedPolicy NULL",    EFI_INVALID_PARAMETER  , NULL                             , sizeof(mSigned_policy_good_manufacturing),   &mCertCA,            &mCertCaLength           , &mTestEKU};
  static VALIDATEBLOB_TEST_CONTEXT mTestVB3  = { "SignedPolicySize 0",    EFI_INVALID_PARAMETER  , mSigned_policy_good_manufacturing, 0                                        ,   &mCertCA,            &mCertCaLength           , &mTestEKU};
  static VALIDATEBLOB_TEST_CONTEXT mTestVB4  = { "*TrustAnchorCert NULL", EFI_INVALID_PARAMETER  , mSigned_policy_good_manufacturing, sizeof(mSigned_policy_good_manufacturing),   (UINT8**) &mNullPtr, &mCertCaLength           , &mTestEKU};
  static VALIDATEBLOB_TEST_CONTEXT mTestVB5  = { "TrustAnchorCertSize 0", EFI_INVALID_PARAMETER  , mSigned_policy_good_manufacturing, sizeof(mSigned_policy_good_manufacturing),   &mCertCA,            (UINTN*) &mNullPtr       , &mTestEKU};
  static VALIDATEBLOB_TEST_CONTEXT mTestVB6  = { "*EKU NULL",             EFI_INVALID_PARAMETER  , mSigned_policy_good_manufacturing, sizeof(mSigned_policy_good_manufacturing),   &mCertCA,            &mCertCaLength           , (UINT8**) &mNullPtr};
  static VALIDATEBLOB_TEST_CONTEXT mTestVB7  = { "Policy Unsigned",       EFI_COMPROMISED_DATA   , mBin_policy_good_manufacturing   , sizeof(mBin_policy_good_manufacturing)   ,   &mCertCA,            &mCertCaLength           , &mTestEKU};
  static VALIDATEBLOB_TEST_CONTEXT mTestVB8  = { "Incorrect Trust Anchor",EFI_SECURITY_VIOLATION , mSigned_policy_good_manufacturing, sizeof(mSigned_policy_good_manufacturing),   &mCertInvalidCaPtr,  &mCertInvalidCaLength    , &mTestEKU};
  static VALIDATEBLOB_TEST_CONTEXT mTestVB9  = { "Different EKUs    ",    EFI_NOT_FOUND          , mSigned_policy_good_manufacturing, sizeof(mSigned_policy_good_manufacturing),   &mCertCA,            &mCertCaLength           , &mTestInvalidEKU};


               //-----------Suite----------Description-----------------Class------------Test Function-------Pre---Clean---Context
  AddTestCase( SignatureVerificationTests, mTestVB1.Description, "MfciPolicy.ParserLib.SignatureVerification", TestValidateSignature, NULL, NULL, &mTestVB1 );
  AddTestCase( SignatureVerificationTests, mTestVB2.Description, "MfciPolicy.ParserLib.SignatureVerification", TestValidateSignature, NULL, NULL, &mTestVB2 );
  AddTestCase( SignatureVerificationTests, mTestVB3.Description, "MfciPolicy.ParserLib.SignatureVerification", TestValidateSignature, NULL, NULL, &mTestVB3 );
  AddTestCase( SignatureVerificationTests, mTestVB4.Description, "MfciPolicy.ParserLib.SignatureVerification", TestValidateSignature, NULL, NULL, &mTestVB4 );
  AddTestCase( SignatureVerificationTests, mTestVB5.Description, "MfciPolicy.ParserLib.SignatureVerification", TestValidateSignature, NULL, NULL, &mTestVB5 );
  AddTestCase( SignatureVerificationTests, mTestVB6.Description, "MfciPolicy.ParserLib.SignatureVerification", TestValidateSignature, NULL, NULL, &mTestVB6 );
  AddTestCase( SignatureVerificationTests, mTestVB7.Description, "MfciPolicy.ParserLib.SignatureVerification", TestValidateSignature, NULL, NULL, &mTestVB7 );
  AddTestCase( SignatureVerificationTests, mTestVB8.Description, "MfciPolicy.ParserLib.SignatureVerification", TestValidateSignature, NULL, NULL, &mTestVB8 );
  AddTestCase( SignatureVerificationTests, mTestVB9.Description, "MfciPolicy.ParserLib.SignatureVerification", TestValidateSignature, NULL, NULL, &mTestVB9 );

  
  //
  // Populate the SanityTest Unit Test Suite.
  //
  Status = CreateUnitTestSuite( &SanityVerificationTests, Framework, "Sanity Parsing Verification Tests", "MfciPolicy.ParserLib.SanityVerification", NULL, NULL );
  if (EFI_ERROR( Status ))
  {
    DEBUG((DEBUG_ERROR, "Failed in CreateUnitTestSuite for SanityVerificationTests, Status = %r\n", Status));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  //*----------------------------------------------------------------------------------*
  //* Test Contexts                                                                    *
  //*----------------------------------------------------------------------------------*
  
  static SANITYCHECK_TEST_CONTEXT  mTestSC1  = { "Good Policy"                  , EFI_SUCCESS          , mBin_policy_good_manufacturing, sizeof(mBin_policy_good_manufacturing) };
  static SANITYCHECK_TEST_CONTEXT  mTestSC2  = { "Policy Pointer NULL"          , EFI_INVALID_PARAMETER, NULL,                           sizeof(mBin_policy_good_manufacturing) };
  static SANITYCHECK_TEST_CONTEXT  mTestSC3  = { "Policy Too Small: Size 0"     , EFI_INVALID_PARAMETER, mBin_policy_good_manufacturing, 0 };
  static SANITYCHECK_TEST_CONTEXT  mTestSC4  = { "Policy Too Small: Size 1"     , EFI_BAD_BUFFER_SIZE  , mBin_policy_good_manufacturing, 1 };
  static SANITYCHECK_TEST_CONTEXT  mTestSC5  = { "Policy Too Small: MinSize-1"  , EFI_BAD_BUFFER_SIZE  , mBin_policy_good_manufacturing, POLICY_BLOB_MIN_SIZE-1 };
  static SANITYCHECK_TEST_CONTEXT  mTestSC6  = { "Unsupported Format Version"   , EFI_COMPROMISED_DATA , mBin_policy_badFormatVersion,   sizeof(mBin_policy_badFormatVersion) };
  static SANITYCHECK_TEST_CONTEXT  mTestSC7  = { "Unsupported Policy Version"   , EFI_COMPROMISED_DATA , mBin_policy_badPolicyVersion,   sizeof(mBin_policy_badPolicyVersion) };
  static SANITYCHECK_TEST_CONTEXT  mTestSC8  = { "Unsupported Policy Publisher" , EFI_COMPROMISED_DATA , mBin_policy_badPolicyPublisher, sizeof(mBin_policy_badPolicyPublisher) };
  static SANITYCHECK_TEST_CONTEXT  mTestSC9  = { "Non-zero Reserved1 Count"     , EFI_COMPROMISED_DATA , mBin_policy_badReserved1Count,  sizeof(mBin_policy_badReserved1Count) };
  static SANITYCHECK_TEST_CONTEXT  mTestSC10 = { "Non-zero Reserved2 Count"     , EFI_COMPROMISED_DATA , mBin_policy_badReserved2Count,  sizeof(mBin_policy_badReserved2Count) };
  static SANITYCHECK_TEST_CONTEXT  mTestSC11 = { "Unsupported OptionFlags"      , EFI_COMPROMISED_DATA , mBin_policy_badOptionFlags,     sizeof(mBin_policy_badOptionFlags) };
  static SANITYCHECK_TEST_CONTEXT  mTestSC12 = { "0 Rules Good"                 , EFI_SUCCESS          , mBin_policy_RuleCount0,         sizeof(mBin_policy_RuleCount0) };
  static SANITYCHECK_TEST_CONTEXT  mTestSC13 = { "FFFF Rules Bad"               , EFI_COMPROMISED_DATA , mBin_policy_RuleCountFFFF,      sizeof(mBin_policy_RuleCountFFFF) };
  static SANITYCHECK_TEST_CONTEXT  mTestSC14 = { "Unsupported Root Key"         , EFI_COMPROMISED_DATA , mBin_policy_badRuleRootKey,     sizeof(mBin_policy_badRuleRootKey) };
  static SANITYCHECK_TEST_CONTEXT  mTestSC15 = { "Offset to SubKey too large"   , EFI_COMPROMISED_DATA , mBin_policy_badRuleSubKeyNameOffsetFFFFFFFF, sizeof(mBin_policy_badRuleSubKeyNameOffsetFFFFFFFF) };
  static SANITYCHECK_TEST_CONTEXT  mTestSC16 = { "Offset to ValueName too large", EFI_COMPROMISED_DATA , mBin_policy_badRuleValueNameOffsetFFFFFFFF,sizeof(mBin_policy_badRuleValueNameOffsetFFFFFFFF) };
  static SANITYCHECK_TEST_CONTEXT  mTestSC17 = { "Offset to Value too large"    , EFI_COMPROMISED_DATA , mBin_policy_badRuleValueOffsetFFFFFFFF, sizeof(mBin_policy_badRuleValueOffsetFFFFFFFF) };
  static SANITYCHECK_TEST_CONTEXT  mTestSC18 = { "SubKeyName String too large"  , EFI_COMPROMISED_DATA , mBin_policy_badSubKeySizeFFFF,      sizeof(mBin_policy_badSubKeySizeFFFF) };
  static SANITYCHECK_TEST_CONTEXT  mTestSC19 = { "ValueName String too large"   , EFI_COMPROMISED_DATA , mBin_policy_badValueNameSizeFFFF,   sizeof(mBin_policy_badValueNameSizeFFFF) };
  static SANITYCHECK_TEST_CONTEXT  mTestSC20 = { "Value TYPE not supported"     , EFI_COMPROMISED_DATA , mBin_policy_badValueType,           sizeof(mBin_policy_badValueType) };
  static SANITYCHECK_TEST_CONTEXT  mTestSC21 = { "Value String too large"       , EFI_COMPROMISED_DATA , mBin_policy_badValueStringSizeFFFF, sizeof(mBin_policy_badValueStringSizeFFFF) };

  AddTestCase( SanityVerificationTests, mTestSC1.Description,  "MfciPolicy.ParserLib.PolicyVerification", TestSanityCheckPolicy, NULL, NULL, &mTestSC1 );
  AddTestCase( SanityVerificationTests, mTestSC2.Description,  "MfciPolicy.ParserLib.PolicyVerification", TestSanityCheckPolicy, NULL, NULL, &mTestSC2 );
  AddTestCase( SanityVerificationTests, mTestSC3.Description,  "MfciPolicy.ParserLib.PolicyVerification", TestSanityCheckPolicy, NULL, NULL, &mTestSC3 );
  AddTestCase( SanityVerificationTests, mTestSC4.Description,  "MfciPolicy.ParserLib.PolicyVerification", TestSanityCheckPolicy, NULL, NULL, &mTestSC4 );
  AddTestCase( SanityVerificationTests, mTestSC5.Description,  "MfciPolicy.ParserLib.PolicyVerification", TestSanityCheckPolicy, NULL, NULL, &mTestSC5 );
  AddTestCase( SanityVerificationTests, mTestSC6.Description,  "MfciPolicy.ParserLib.PolicyVerification", TestSanityCheckPolicy, NULL, NULL, &mTestSC6 );
  AddTestCase( SanityVerificationTests, mTestSC7.Description,  "MfciPolicy.ParserLib.PolicyVerification", TestSanityCheckPolicy, NULL, NULL, &mTestSC7 );
  AddTestCase( SanityVerificationTests, mTestSC8.Description,  "MfciPolicy.ParserLib.PolicyVerification", TestSanityCheckPolicy, NULL, NULL, &mTestSC8 );
  AddTestCase( SanityVerificationTests, mTestSC9.Description,  "MfciPolicy.ParserLib.PolicyVerification", TestSanityCheckPolicy, NULL, NULL, &mTestSC9 );
  AddTestCase( SanityVerificationTests, mTestSC10.Description, "MfciPolicy.ParserLib.PolicyVerification", TestSanityCheckPolicy, NULL, NULL, &mTestSC10 );
  AddTestCase( SanityVerificationTests, mTestSC11.Description, "MfciPolicy.ParserLib.PolicyVerification", TestSanityCheckPolicy, NULL, NULL, &mTestSC11 );
  AddTestCase( SanityVerificationTests, mTestSC12.Description, "MfciPolicy.ParserLib.PolicyVerification", TestSanityCheckPolicy, NULL, NULL, &mTestSC12 );
  AddTestCase( SanityVerificationTests, mTestSC13.Description, "MfciPolicy.ParserLib.PolicyVerification", TestSanityCheckPolicy, NULL, NULL, &mTestSC13 );
  AddTestCase( SanityVerificationTests, mTestSC14.Description, "MfciPolicy.ParserLib.PolicyVerification", TestSanityCheckPolicy, NULL, NULL, &mTestSC14 );
  AddTestCase( SanityVerificationTests, mTestSC15.Description, "MfciPolicy.ParserLib.PolicyVerification", TestSanityCheckPolicy, NULL, NULL, &mTestSC15 );
  AddTestCase( SanityVerificationTests, mTestSC16.Description, "MfciPolicy.ParserLib.PolicyVerification", TestSanityCheckPolicy, NULL, NULL, &mTestSC16 );
  AddTestCase( SanityVerificationTests, mTestSC17.Description, "MfciPolicy.ParserLib.PolicyVerification", TestSanityCheckPolicy, NULL, NULL, &mTestSC17 );
  AddTestCase( SanityVerificationTests, mTestSC18.Description, "MfciPolicy.ParserLib.PolicyVerification", TestSanityCheckPolicy, NULL, NULL, &mTestSC18 );
  AddTestCase( SanityVerificationTests, mTestSC19.Description, "MfciPolicy.ParserLib.PolicyVerification", TestSanityCheckPolicy, NULL, NULL, &mTestSC19 );
  AddTestCase( SanityVerificationTests, mTestSC20.Description, "MfciPolicy.ParserLib.PolicyVerification", TestSanityCheckPolicy, NULL, NULL, &mTestSC20 );
  AddTestCase( SanityVerificationTests, mTestSC21.Description, "MfciPolicy.ParserLib.PolicyVerification", TestSanityCheckPolicy, NULL, NULL, &mTestSC21 );


  //
  // Populate the ExtractValues Unit Test Suite.
  //
  Status = CreateUnitTestSuite( &ExtractValueTests, Framework, "Extract Values Verification Tests", "MfciPolicy.ParserLib.ExtractValueTests", NULL, NULL );
  if (EFI_ERROR( Status ))
  {
    DEBUG((DEBUG_ERROR, "Failed in CreateUnitTestSuite for SanityVerificationTests, Status = %r\n", Status));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  //*----------------------------------------------------------------------------------*
  //* Test Contexts                                                                    *
  //*----------------------------------------------------------------------------------*

  static EXTRACT_CHAR16_TEST_CONTEXT mTestChar01 = { "Good Blob & Params", EFI_SUCCESS          , mSigned_policy_good_manufacturing, sizeof(mSigned_policy_good_manufacturing), gPolicyBlobFieldName[MFCI_POLICY_TARGET_MANUFACTURER], L"Contoso Computers, LLC", NULL};
  static EXTRACT_CHAR16_TEST_CONTEXT mTestChar02 = { "SignedPolicy NULL" , EFI_INVALID_PARAMETER, NULL                             , sizeof(mSigned_policy_good_manufacturing), gPolicyBlobFieldName[MFCI_POLICY_TARGET_MANUFACTURER], NULL, NULL};
  static EXTRACT_CHAR16_TEST_CONTEXT mTestChar03 = { "SignedPolicySize 0", EFI_INVALID_PARAMETER, mSigned_policy_good_manufacturing, 0                                        , gPolicyBlobFieldName[MFCI_POLICY_TARGET_MANUFACTURER], NULL, NULL};
  static EXTRACT_CHAR16_TEST_CONTEXT mTestChar04 = { "ValueName NULL"    , EFI_INVALID_PARAMETER, mSigned_policy_good_manufacturing, sizeof(mSigned_policy_good_manufacturing), NULL                   , NULL, NULL};
  static EXTRACT_CHAR16_TEST_CONTEXT mTestChar05 = { "Not Present Name"  , EFI_NOT_FOUND        , mSigned_policy_good_manufacturing, sizeof(mSigned_policy_good_manufacturing), L"Target\\NotPresent"  , NULL, NULL};
  static EXTRACT_CHAR16_TEST_CONTEXT mTestChar06 = { "No Separator"      , EFI_NOT_FOUND        , mSigned_policy_good_manufacturing, sizeof(mSigned_policy_good_manufacturing), L"SeparatorNotPresent" , NULL, NULL};
  static EXTRACT_CHAR16_TEST_CONTEXT mTestChar07 = { "Nothing after Sep" , EFI_NOT_FOUND        , mSigned_policy_good_manufacturing, sizeof(mSigned_policy_good_manufacturing), L"Before\\"            , NULL, NULL};
  static EXTRACT_CHAR16_TEST_CONTEXT mTestChar08 = { "Nothing before Sep", EFI_NOT_FOUND        , mSigned_policy_good_manufacturing, sizeof(mSigned_policy_good_manufacturing), L"\\After"             , NULL, NULL};
  static EXTRACT_CHAR16_TEST_CONTEXT mTestChar09 = { "Empty String"      , EFI_NOT_FOUND        , mSigned_policy_good_manufacturing, sizeof(mSigned_policy_good_manufacturing), L""                    , NULL, NULL};

  static EXTRACT_UINT64_TEST_CONTEXT mTestUint01 = { "Good Blob"         , EFI_SUCCESS          , mSigned_policy_good_manufacturing, sizeof(mSigned_policy_good_manufacturing), gPolicyBlobFieldName[MFCI_POLICY_FIELD_UEFI_POLICY], MFCI_POLICY_VALUE_ACTION_SECUREBOOT_CLEAR | MFCI_POLICY_VALUE_ACTION_TPM_CLEAR};
  static EXTRACT_UINT64_TEST_CONTEXT mTestUint02 = { "SignedPolicy NULL" , EFI_INVALID_PARAMETER, NULL                             , sizeof(mSigned_policy_good_manufacturing), gPolicyBlobFieldName[MFCI_POLICY_FIELD_UEFI_POLICY], MFCI_POLICY_VALUE_INVALID};
  static EXTRACT_UINT64_TEST_CONTEXT mTestUint03 = { "SignedPolicySize 0", EFI_INVALID_PARAMETER, mSigned_policy_good_manufacturing, 0                                        , gPolicyBlobFieldName[MFCI_POLICY_FIELD_UEFI_POLICY], MFCI_POLICY_VALUE_INVALID};
  static EXTRACT_UINT64_TEST_CONTEXT mTestUint04 = { "SignedPolicy NULL" , EFI_INVALID_PARAMETER, mSigned_policy_good_manufacturing, sizeof(mSigned_policy_good_manufacturing), NULL                              , MFCI_POLICY_VALUE_INVALID};


               //-----------Suite----------Description-----------------Class------------Test Function-------Pre---Clean---Context
  AddTestCase( ExtractValueTests, mTestChar01.Description, "CHAR16", TestExtractChar16, NULL, CleanUpExtractChar16, &mTestChar01 );
  AddTestCase( ExtractValueTests, mTestChar02.Description, "CHAR16", TestExtractChar16, NULL, CleanUpExtractChar16, &mTestChar02 );
  AddTestCase( ExtractValueTests, mTestChar03.Description, "CHAR16", TestExtractChar16, NULL, CleanUpExtractChar16, &mTestChar03 );
  AddTestCase( ExtractValueTests, mTestChar04.Description, "CHAR16", TestExtractChar16, NULL, CleanUpExtractChar16, &mTestChar04 );
  AddTestCase( ExtractValueTests, mTestChar05.Description, "CHAR16", TestExtractChar16, NULL, CleanUpExtractChar16, &mTestChar05 );
  AddTestCase( ExtractValueTests, mTestChar06.Description, "CHAR16", TestExtractChar16, NULL, CleanUpExtractChar16, &mTestChar06 );
  AddTestCase( ExtractValueTests, mTestChar07.Description, "CHAR16", TestExtractChar16, NULL, CleanUpExtractChar16, &mTestChar07 );
  AddTestCase( ExtractValueTests, mTestChar08.Description, "CHAR16", TestExtractChar16, NULL, CleanUpExtractChar16, &mTestChar08 );
  AddTestCase( ExtractValueTests, mTestChar09.Description, "CHAR16", TestExtractChar16, NULL, CleanUpExtractChar16, &mTestChar09 );

  AddTestCase( ExtractValueTests, mTestUint01.Description, "Uint64", TestExtractUint64, NULL, NULL, &mTestUint01 );
  AddTestCase( ExtractValueTests, mTestUint02.Description, "Uint64", TestExtractUint64, NULL, NULL, &mTestUint02 );
  AddTestCase( ExtractValueTests, mTestUint03.Description, "Uint64", TestExtractUint64, NULL, NULL, &mTestUint03 );
  AddTestCase( ExtractValueTests, mTestUint04.Description, "Uint64", TestExtractUint64, NULL, NULL, &mTestUint04 );


  //
  // Execute the tests.
  //

  DEBUG((DEBUG_INFO, "\nSTART: About To run Tests\n"));
  Status = RunAllTestSuites( Framework );
  DEBUG((DEBUG_INFO, "\nEND: Tests Complete\n"));

EXIT:

  if (Framework)
  {
    FreeUnitTestFramework( Framework );
  }

  return Status;
}