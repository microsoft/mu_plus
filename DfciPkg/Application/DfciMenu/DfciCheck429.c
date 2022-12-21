/** @file
DfciSARecovery.c

Device Firmware Configuration Interface - Stand Alone driver that can be loaded
at the UEFI Shell

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HttpLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiLib.h>

#include "DfciPrivate.h"
#include "DfciRequest.h"

// *---------------------------------------------------------------------------------------*
// * Application Global Variables                                                          *
// *---------------------------------------------------------------------------------------*
DFCI_NETWORK_REQUEST  mDfciNetworkRequest = {0};
STATIC CHAR8                 mCheck429Url[] = "http://mikeytbds3.eastus.cloudapp.azure.com/return_429";

/**
*  This function is the main entry of the DfciCheck429 application.
*
* @param[in]   ImageHandle
* @param[in]   SystemTable
*
**/
EFI_STATUS
EFIAPI
DfciCheck429Entry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  ZeroMem (&mDfciNetworkRequest, sizeof(mDfciNetworkRequest));

  mDfciNetworkRequest.HttpRequest.Url = AllocateCopyPool (sizeof(mCheck429Url), &mCheck429Url);
  mDfciNetworkRequest.MainLogic       = Check429Logic;

  // Try every NIC in the system until one fills the first part of the request.
  Status = TryEachNICThenProcessRequest (&mDfciNetworkRequest);

  DEBUG ((DEBUG_INFO, "Url   %a\n", mDfciNetworkRequest.HttpRequest.Url));
  DEBUG ((DEBUG_INFO, "HttpStatus = %d\n", mDfciNetworkRequest.HttpStatus.HttpStatus));
  DEBUG ((DEBUG_INFO, "HttpStatus = %a\n", GetHttpErrorMsg (mDfciNetworkRequest.HttpStatus.HttpStatus)));

  if (mDfciNetworkRequest.HttpStatus.HttpStatus == HTTP_STATUS_429_TOO_MANY_REQUESTS) {
    AsciiPrint ("TEST PASSED. Network stack returned 429 as expected.\n");
  } else {
    if (mDfciNetworkRequest.HttpStatus.HttpStatus == HTTP_STATUS_UNSUPPORTED_STATUS) {
      AsciiPrint ("TEST FAILED.  Http status could not be retrieved.\n");
    } else {
      AsciiPrint ("TEST FAILED.  Unexpected status = %a\n",  GetHttpErrorMsg (mDfciNetworkRequest.HttpStatus.HttpStatus));
    }
  }

  // Right now, this is a driver due to the libraries used.  So, never load.
  return EFI_NOT_FOUND;
}
