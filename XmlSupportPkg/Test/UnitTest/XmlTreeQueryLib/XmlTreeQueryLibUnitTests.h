/**
Unit-tests for XmlTreeQueryLib.

Copyright (C) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef XML_TREE_QUERY_LIB_UNIT_TESTS_H
#define XML_TREE_QUERY_LIB_UNIT_TESTS_H

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/PrintLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UnitTestLib.h>

#include <XmlTypes.h>
#include <Library/XmlTreeLib.h>
#include <Library/XmlTreeQueryLib.h>


#define UNIT_TEST_APP_NAME        "XML Query Lib Unit Test Application"
#define UNIT_TEST_APP_VERSION     "0.2"

//global node tree so we only have to create once
extern XmlNode *mNode;

/**
Simple clean up method to make sure string parsing tests clean up even if interrupted and fail in the middle.
**/
UNIT_TEST_STATUS
EFIAPI
PreReqNodeTreeIsValid(
  IN UNIT_TEST_CONTEXT           Context
);

EFI_STATUS
EFIAPI
RegisterAttributeTests(UNIT_TEST_SUITE_HANDLE TestSuite);

EFI_STATUS
EFIAPI
RegisterElementTests(UNIT_TEST_SUITE_HANDLE TestSuite);

#endif
