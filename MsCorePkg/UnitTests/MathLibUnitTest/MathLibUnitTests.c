/**
Unit-tests UEFI shell app for MathLib.


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
#include "MathLibUnitTests.h"
#include "TestData.h"
#include <Library/MathLib.h>
#include <Library/UnitTestLib.h>
#include <Library/UnitTestLogLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Library/UnitTestAssertLib.h>



/**
Test sine function
**/
UNIT_TEST_STATUS
EFIAPI
TestSine(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
)
{
  UT_LOG_INFO( "Testing Sine function\n" );
  UINTN Index = 0;
  double current;
  MathLibContext *mathContext = (MathLibContext*)Context;
  double result;
  double totalErrorSquared = 0;
  double maxError = 0;
  double totalAllowedError = mathContext->maxTotalError;
  double maxAllowedError = mathContext->maxSingleError;
  double error = 0;


  for (current= mathContext->start;current < mathContext->stop; current+= mathContext->step){
    result = sin_d(current);
    error = result - mathContext->data[Index];
    error *= error;
    totalErrorSquared += error;
    if (error > maxError) {
      maxError = error;
    }
    if (maxError >= maxAllowedError){
      UT_LOG_WARNING("COS at %llx = %llx",current,result);
    }
    
    UT_ASSERT_TRUE(maxError < maxAllowedError);
    Index++;
  }
  UT_LOG_WARNING("TOTAL ERROR: %llx",totalErrorSquared);
  UT_ASSERT_TRUE(totalErrorSquared < totalAllowedError);

  return UNIT_TEST_PASSED;
}
/**
Test sqrt function
**/
UNIT_TEST_STATUS
EFIAPI
TestSqrt(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
)
{
  UT_LOG_INFO( "Testing Square Root function\n" );
  UINTN Index = 0;
  double current;
  MathLibContext *mathContext = (MathLibContext*)Context;  
  double result;
  double totalErrorSquared = 0;
  double maxError = 0;
  double totalAllowedError = mathContext->maxTotalError;
  double maxAllowedError = mathContext->maxSingleError;
  double error = 0;

  for (current= mathContext->start;current < mathContext->stop; current+= mathContext->step){
    result = sqrt_d(current);
    error = result - mathContext->data[Index];
    error *= error;
    totalErrorSquared += error;
    if (error > maxError) {
      maxError = error;
    }
    if (maxError >= maxAllowedError){
      UT_LOG_WARNING("SQRT at %llx = %llx",current,result);
    }
    UT_ASSERT_TRUE(maxError < maxAllowedError);
    Index++;
  }
  UT_LOG_WARNING("TOTAL ERROR: %llx",totalErrorSquared);
  UT_ASSERT_TRUE(totalErrorSquared < totalAllowedError);

  return UNIT_TEST_PASSED;
}

/**
Test sqrt function
**/
UNIT_TEST_STATUS
EFIAPI
TestSqrt32(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
)
{
  UT_LOG_INFO( "Testing Square Root Unsigned 32 function\n" );
  UINTN Index = 0;
  UINT32 current;
  MathLibContextUnsigned *mathContext = (MathLibContextUnsigned*)Context;  
  UINT32 result;
  UINT32 totalErrorSquared = 0;
  UINT32 maxError = 0;
  UINT32 totalAllowedError = mathContext->maxTotalError;
  UINT32 maxAllowedError = mathContext->maxSingleError;
  UINT32 error = 0;

  for (current= mathContext->start;current < mathContext->stop; current+= mathContext->step){
    result = sqrt32(current);
    error = result - mathContext->data[Index];
    error *= error;
    totalErrorSquared += error;
    if (error > maxError) {
      maxError = error;
    }
    if (maxError >= maxAllowedError){
      UT_LOG_WARNING("SQRT32 at %d = %d",current,result);
    }
    UT_ASSERT_TRUE(maxError < maxAllowedError);
    Index++;
  }
  UT_LOG_WARNING("TOTAL ERROR: %d",totalErrorSquared);
  UT_ASSERT_TRUE(totalErrorSquared < totalAllowedError);

  return UNIT_TEST_PASSED;
}

/**
Test sqrt function
**/
UNIT_TEST_STATUS
EFIAPI
TestSqrt64(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
)
{
  UT_LOG_INFO( "Testing Square Root Unsigned 64 function\n" );
  UINTN Index = 0;
  UINT32 current;
  MathLibContextUnsigned *mathContext = (MathLibContextUnsigned*)Context;  
  UINT64 result;
  UINT32 totalErrorSquared = 0;
  UINT32 maxError = 0;
  UINT32 totalAllowedError = mathContext->maxTotalError;
  UINT32 maxAllowedError = mathContext->maxSingleError;
  UINT32 error = 0;

  for (current= mathContext->start;current < mathContext->stop; current+= mathContext->step){
    result = sqrt64(current);
    error = (UINT32)(result - (UINT64)mathContext->data[Index]);
    error *= error;
    totalErrorSquared += error;
    if (error > maxError) {
      maxError = error;
    }
    if (maxError >= maxAllowedError){
      UT_LOG_WARNING("SQRT64 at %d = %d",current,result);
    }
    UT_ASSERT_TRUE(maxError < maxAllowedError);
    Index++;
  }
  UT_LOG_WARNING("TOTAL ERROR: %d",totalErrorSquared);
  UT_ASSERT_TRUE(totalErrorSquared < totalAllowedError);

  return UNIT_TEST_PASSED;
}

/**
Test Cosine function
**/
UNIT_TEST_STATUS
EFIAPI
TestCos(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
)
{
  //MathLibContext *MathContext = (MathLibContext*)Context;  
  UT_LOG_INFO( "Testing Cosine function\n" );
  UINTN Index = 0;
  double current;
  MathLibContext *mathContext = (MathLibContext*)Context;  
  double result;
  double totalErrorSquared = 0;
  double maxError = 0;
  double totalAllowedError = mathContext->maxTotalError;
  double maxAllowedError = mathContext->maxSingleError;
  double error = 0;


  for (current= mathContext->start;current < mathContext->stop; current+= mathContext->step){
    result = cos_d(current);
    error = result - mathContext->data[Index];
    error *= error;
    totalErrorSquared += error;
    if (error > maxError) maxError = error;
    if (maxError >= maxAllowedError){
      DEBUG(( DEBUG_INFO, "COS %llx = %llx",current,result));
      UT_LOG_WARNING("COS at %llx = %llx",current,result);
    }
    
    UT_ASSERT_TRUE(maxError < maxAllowedError);
    Index++;
  }
  UT_LOG_WARNING("TOTAL ERROR: %llx",totalErrorSquared);
  UT_ASSERT_TRUE(totalErrorSquared < totalAllowedError);

  return UNIT_TEST_PASSED;
}


//----------------------------------------------------
// UEFI main
//----------------------------------------------------
EFI_STATUS
EFIAPI
UefiMain(
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE* SystemTable)
{
  EFI_STATUS                Status;
  UNIT_TEST_FRAMEWORK       *Fw = NULL;
  UNIT_TEST_SUITE           *TestSuite;
  CHAR16  ShortName[100];
  ShortName[0] = L'\0';

  UnicodeSPrint(&ShortName[0], sizeof(ShortName), L"%a", gEfiCallerBaseName); 
    DEBUG((DEBUG_INFO, "%s v%s\n", UNIT_TEST_APP_NAME, UNIT_TEST_APP_VERSION));

  //
  // Start setting up the test framework for running the tests.
  //
  Status = InitUnitTestFramework(&Fw, UNIT_TEST_APP_NAME, ShortName, UNIT_TEST_APP_VERSION);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "Failed in InitUnitTestFramework. Status = %r\n", Status));
    goto EXIT;
  }

  Status = CreateUnitTestSuite(&TestSuite, Fw, L"Math Lib Test Suite ", L"Common.MathLib", NULL, NULL);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "Failed in CreateUnitTestSuite for Math Lib Test Suite %r\n", Status));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  AddTestCase(TestSuite, L"Check Sine is within a reasonable error", L"Common.MathLib.Sine", TestSine, NULL, NULL, &SIN_CONTEXT);

  AddTestCase(TestSuite, L"Check cosine is within a reasonable error", L"Common.MathLib.Cos", TestCos, NULL, NULL, &COS_CONTEXT);

  AddTestCase(TestSuite, L"Check sqrt is within a reasonable error", L"Common.MathLib.Sqrt", TestSqrt, NULL, NULL, &SQRT_CONTEXT);

  AddTestCase(TestSuite, L"Check sqrt64 is within a reasonable error", L"Common.MathLib.Sqrt64", TestSqrt32, NULL, NULL, &SQRTUNSIGNED_CONTEXT);
  
  AddTestCase(TestSuite, L"Check sqrt32 is within a reasonable error", L"Common.MathLib.Sqrt32", TestSqrt64, NULL, NULL, &SQRTUNSIGNED_CONTEXT);
  
  //Run Tests
  Status = RunAllTestSuites(Fw);

  
 
EXIT:

  if (Fw != NULL)
  {
    FreeUnitTestFramework(Fw);
  }

  return Status;
}