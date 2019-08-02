/**
Unit-tests UEFI shell app for XmlTreeQueryLib.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef XML_TREE_QUERY_LIB_UNIT_TESTS_H
#define XML_TREE_QUERY_LIB_UNIT_TESTS_H

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/PrintLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

#include <UnitTestTypes.h>
#include <Library/UnitTestLib.h>
#include <Library/UnitTestAssertLib.h>

#include <XmlTypes.h>
#include <Library/XmlTreeLib.h>
#include <Library/XmlTreeQueryLib.h>


#define UNIT_TEST_APP_NAME        L"XML Query Lib Unit Test Application"
#define UNIT_TEST_APP_VERSION     L"0.1"

//global node tree so we only have to create once
extern XmlNode *mNode; 

/**
Simple clean up method to make sure string parsing tests clean up even if interrupted and fail in the middle.
**/
UNIT_TEST_STATUS
EFIAPI
PreReqNodeTreeIsValid(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
);

EFI_STATUS
RegisterAttributeTests(UNIT_TEST_SUITE           *TestSuite);

EFI_STATUS
RegisterElementTests(UNIT_TEST_SUITE           *TestSuite);

#endif