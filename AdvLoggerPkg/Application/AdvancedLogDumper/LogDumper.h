/** @file LogDumper.h

  LogDumper UEFI headers/common functionality

  Copyright (C) Microsoft Corporation. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __LOG_DUMPER_H__
#define __LOG_DUMPER_H__

#include <Uefi.h>

#include <AdvancedLoggerInternal.h>

#include <Protocol/AdvancedLogger.h>

#include <Library/AdvancedLoggerAccessLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/PrintLib.h>
#include <Library/ShellLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiHiiServicesLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/HiiLib.h>

EFI_STATUS
EFIAPI
AdvLogDumperInternalWorker (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

EFI_HII_HANDLE
InitializeHiiPackage (
  EFI_HANDLE  ImageHandle
  );

#endif // __LOG_DUMPER_H__
