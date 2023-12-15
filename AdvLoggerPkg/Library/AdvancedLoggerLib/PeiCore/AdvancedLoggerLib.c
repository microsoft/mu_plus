/** @file
  PEI CORE implementation of the Advanced Logger Library

  Copyright (c) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Uefi.h>

#include <AdvancedLoggerInternal.h>

/**
  Including the PeiMain.h from PeiCore in order to access the Platform Blob data member.

  This is breaking the rules, but PeiCore on a system using ROM for PeiPreMem has no place
  to store long term data besides the Hob or Ppi list.  Accessing these list for high
  frequency operations is a performance issue.
**/
#include <Core/Pei/PeiMain.h>

#include <Ppi/AdvancedLogger.h>

#include <Library/AdvancedLoggerHdwPortLib.h>
#include <Library/BaseLib.h>
#include <Library/AdvancedLoggerLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/MmUnblockMemoryLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/PcdLib.h>
#include <Library/PrintLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/SynchronizationLib.h>

#include "../AdvancedLoggerCommon.h"

//
// Prototype function used in Memory Discovered Ppi
//
EFI_STATUS
EFIAPI
InstallPermanentMemoryBuffer (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  );

//
// Prototype functions used in Advanced Logger Ppi
//
VOID
EFIAPI
AdvancedLoggerWritePpi (
  IN        UINTN  ErrorLevel,
  IN  CONST CHAR8  *Buffer,
  IN        UINTN  NumberOfBytes
  );

VOID
EFIAPI
AdvancedLoggerPrintPpi (
  IN        UINTN    ErrorLevel,
  IN  CONST CHAR8    *Format,
  IN        VA_LIST  VaListMarker
  );

VOID
EFIAPI
AdvancedLoggerAssertPpi (
  IN CONST CHAR8  *FileName,
  IN       UINTN  LineNumber,
  IN CONST CHAR8  *Description
  );

//
// Ppi and Notification lists
//
ADVANCED_LOGGER_PPI  mAdvancedLoggerPpi = {
  ADVANCED_LOGGER_PPI_SIGNATURE,
  ADVANCED_LOGGER_PPI_VERSION,
  AdvancedLoggerWritePpi,
  AdvancedLoggerPrintPpi,
  AdvancedLoggerAssertPpi,
};

CONST EFI_PEI_PPI_DESCRIPTOR  mAdvancedLoggerPpiList[] = {
  {
    (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gAdvancedLoggerPpiGuid,
    &mAdvancedLoggerPpi
  }
};

CONST EFI_PEI_NOTIFY_DESCRIPTOR  mMemoryDiscoveredNotifyList[] = {
  {
    (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_DISPATCH | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiPeiMemoryDiscoveredPpiGuid,
    InstallPermanentMemoryBuffer
  }
};

/**
  Ppi Function for routing Advanced Logger Write

  @param  ErrorLevel      The error level of the debug message.
  @param  Buffer          The debug message to log.
  @param  NumberOfBytes   Number of bytes in the debug message.

**/
VOID
EFIAPI
AdvancedLoggerWritePpi (
  IN        UINTN  ErrorLevel,
  IN  CONST CHAR8  *Buffer,
  IN        UINTN  NumberOfBytes
  )
{
  AdvancedLoggerWrite (ErrorLevel, Buffer, NumberOfBytes);
}

/**
  Ppi Function pointer for routing DebugVPrint

  @param  ErrorLevel    The error level of the debug message.
  @param  Format        Format string for the debug message to print.
  @param  VaListMarker  VA_LIST marker for the variable argument list.

**/
VOID
EFIAPI
AdvancedLoggerPrintPpi (
  IN  UINTN        ErrorLevel,
  IN  CONST CHAR8  *Format,
  VA_LIST          VaListMarker
  )
{
  DebugVPrint (ErrorLevel, Format, VaListMarker);
}

/**
  Ppi Function for routing DebugAssert function

  @param  FileName        The pointer to the name of the source file that generated the assert condition.
  @param  LineNumber      The line number in the source file that generated the assert condition
  @param  Description     The pointer to the description of the assert condition.

**/
VOID
EFIAPI
AdvancedLoggerAssertPpi (
  IN CONST CHAR8  *FileName,
  IN       UINTN  LineNumber,
  IN CONST CHAR8  *Description
  )
{
  DebugAssert (FileName, LineNumber, Description);
}

/**
   This function installs the full Advanced Logger memory buffer.

   This function is only called when there is no SEC Advanced Logger, and PEI needs
   to transition from a small buffer, to the full in memory buffer.

   @param  PeiServices      Indirect reference to the PEI Services Table.
   @param  NotifyDescriptor Address of the notification descriptor data structure.
   @param  Ppi              Address of the PPI that was installed.

   @return EFI_SUCCESS      The PPIs were installed successfully.
   @return Others           Some error occurs during the execution of this function.

**/
EFI_STATUS
EFIAPI
InstallPermanentMemoryBuffer (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  UINTN                 CurrentLogOffset;
  UINTN                 DebugLevel;
  EFI_HOB_GUID_TYPE     *GuidHob;
  ADVANCED_LOGGER_INFO  *LoggerInfo;
  ADVANCED_LOGGER_PTR   *LogPtr;
  EFI_PHYSICAL_ADDRESS  NewLogBuffer;
  ADVANCED_LOGGER_INFO  *NewLoggerInfo;
  EFI_PHYSICAL_ADDRESS  OldLoggerBuffer;
  PEI_CORE_INSTANCE     *PeiCoreInstance;
  EFI_STATUS            Status;

  DEBUG ((DEBUG_INFO, "%a: Find PeiCore HOB for Install Permanent Buffer...\n", __FUNCTION__));
  GuidHob = GetFirstGuidHob (&gAdvancedLoggerHobGuid);
  if (GuidHob == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Advanced Logger Hob not found\n", __FUNCTION__));
  } else {
    LogPtr     = (ADVANCED_LOGGER_PTR *)GET_GUID_HOB_DATA (GuidHob);
    LoggerInfo = ALI_FROM_PA (LogPtr->LogBuffer);
    if (!LoggerInfo->InPermanentRAM) {
      //
      // Must be PeiCore allocated small memory buffer
      //
      Status = PeiServicesAllocatePages (
                 EfiReservedMemoryType,
                 FixedPcdGet32 (PcdAdvancedLoggerPages),
                 &NewLogBuffer
                 );
      if (!EFI_ERROR (Status)) {
        NewLoggerInfo = ALI_FROM_PA (NewLogBuffer);
        CopyMem ((VOID *)NewLoggerInfo, (VOID *)LoggerInfo, sizeof (ADVANCED_LOGGER_INFO));
        CurrentLogOffset         = (UINTN)(LoggerInfo->LogCurrent - LoggerInfo->LogBuffer);
        NewLoggerInfo->LogBuffer = PA_FROM_PTR ((CHAR8 *)(NewLoggerInfo + 1));

        if (CurrentLogOffset > 0) {
          CopyMem (
            PTR_FROM_PA (NewLoggerInfo->LogBuffer),
            PTR_FROM_PA (LoggerInfo->LogBuffer),
            (CurrentLogOffset)
            );
        }

        NewLoggerInfo->LogBufferSize  = EFI_PAGES_TO_SIZE (FixedPcdGet32 (PcdAdvancedLoggerPages)) - sizeof (ADVANCED_LOGGER_INFO);
        NewLoggerInfo->LogCurrent     = PA_FROM_PTR (CHAR8_FROM_PA (NewLoggerInfo->LogBuffer) + CurrentLogOffset);
        NewLoggerInfo->InPermanentRAM = TRUE;

        PeiCoreInstance               = PEI_CORE_INSTANCE_FROM_PS_THIS (PeiServices);
        PeiCoreInstance->PlatformBlob = PA_FROM_PTR (NewLoggerInfo);

        //
        // Update the HOB pointer
        //
        OldLoggerBuffer   = LogPtr->LogBuffer;
        LogPtr->LogBuffer = NewLogBuffer;

        Status = MmUnblockMemoryRequest (NewLogBuffer, FixedPcdGet32 (PcdAdvancedLoggerPages));
        if (EFI_ERROR (Status)) {
          if (Status != EFI_UNSUPPORTED) {
            DEBUG ((DEBUG_ERROR, "%a: Unable to notify StandaloneMM. Code=%r\n", __FUNCTION__, Status));
          }
        } else {
          DEBUG ((DEBUG_INFO, "%a: StandaloneMM Hob data published\n", __FUNCTION__));
        }

        PeiServicesFreePages (
          OldLoggerBuffer,
          FixedPcdGet32 (PcdAdvancedLoggerPreMemPages)
          );

        if (NewLoggerInfo->DiscardedSize != 0) {
          DebugLevel = DEBUG_ERROR;
        } else {
          DebugLevel = DEBUG_INFO;
        }

        DEBUG ((
          DebugLevel,
          "%a: - New Info=%p, Buffer=%lx, Current=%lx, Size=%d, Discarded=%d\n",
          __FUNCTION__,
          NewLoggerInfo,
          NewLoggerInfo->LogBuffer,
          NewLoggerInfo->LogCurrent,
          NewLoggerInfo->LogBufferSize,
          NewLoggerInfo->DiscardedSize
          ));
      }
    }
  }

  return EFI_SUCCESS;
}

/**
  Validate Info Blocks

  The address of the ADVANCE_LOGGER_INFO block pointer is captured during the first debug print.  The
  pointers LogBuffer and LogCurrent, and LogBufferSize, could be written to by untrusted code.  Here,
  we check that the pointers are within the allocated mLoggerInfo space, and that LogBufferSize, which
  is used in multiple places to see if a new message will fit into the log buffer, is valid.

  @param          LoggerInfo  Logger information pointer needs to be validated.

  @return         BOOLEAN     TRUE = mLoggerInfo Block passes security checks
  @return         BOOLEAN     FALSE= mLoggerInfo Block failed security checks

**/
STATIC
BOOLEAN
ValidateInfoBlock (
  IN  ADVANCED_LOGGER_INFO  *LoggerInfo
  )
{
  if (LoggerInfo == NULL) {
    return FALSE;
  }

  if (LoggerInfo->Signature != ADVANCED_LOGGER_SIGNATURE) {
    return FALSE;
  }

  if (LoggerInfo->LogBuffer != (PA_FROM_PTR (LoggerInfo + 1))) {
    return FALSE;
  }

  if ((LoggerInfo->LogCurrent > LoggerInfo->LogBuffer + LoggerInfo->LogBufferSize) ||
      (LoggerInfo->LogCurrent < LoggerInfo->LogBuffer))
  {
    return FALSE;
  }

  return TRUE;
}

/**
  Get the SEC Logger Information block

  @param    NONE

  @return   ADVANCED_LOGGER_INFO *    Pointer to the SEC Logger Info Block

 **/
STATIC
ADVANCED_LOGGER_INFO *
GetSecLoggerInfo (
  VOID
  )
{
  ADVANCED_LOGGER_INFO  *LoggerInfoSec;
  ADVANCED_LOGGER_PTR   *LogPtr;

  //
  // Locate the SEC Logger Information block.
  //
  // The PCD AdvancedLoggerBase MAY be a 64 bit address.  However, it is
  // trimmed to be a pointer the size of the actual platform - and the Pcd is expected
  // to be set accordingly.
  LoggerInfoSec = NULL;
  LogPtr        = (ADVANCED_LOGGER_PTR *)(VOID *)FixedPcdGet64 (PcdAdvancedLoggerBase);
  if (!FeaturePcdGet (PcdAdvancedLoggerFixedInRAM)) {
    if ((LogPtr != NULL) &&
        (LogPtr->Signature == ADVANCED_LOGGER_PTR_SIGNATURE) &&
        (LogPtr->LogBuffer != 0ULL))
    {
      LoggerInfoSec = ALI_FROM_PA (LogPtr->LogBuffer);
      if (!LoggerInfoSec->HdwPortInitialized) {
        AdvancedLoggerHdwPortInitialize ();
        LoggerInfoSec->HdwPortInitialized = TRUE;
      }
    }
  } else {
    if ((LogPtr != NULL) && ValidateInfoBlock ((ADVANCED_LOGGER_INFO *)LogPtr)) {
      LoggerInfoSec = (ADVANCED_LOGGER_INFO *)(VOID *)LogPtr;
    }
  }

  return LoggerInfoSec;
}

/**
  UpdateSecLoggerInfo

  Updates the SEC LoggerInfo pointer with the one used by PEI.  This
  way, additional SEC messages will occur in the proper place in the
  log buffer.

  @param  LoggerInfo  - Pointer to the new Logger Info block

  @return NONE
  **/
STATIC
VOID
UpdateSecLoggerInfo (
  ADVANCED_LOGGER_INFO  *LoggerInfo
  )
{
  ADVANCED_LOGGER_PTR  *LogPtr;

  //
  // Locate the SEC Logger Information block.
  //
  // The PCD AdvancedLoggerBase MAY be a 64 bit address.  However, it is
  // trimmed to be a pointer the size of the actual platform - and the Pcd is expected
  // to be set accordingly.
  LogPtr = (ADVANCED_LOGGER_PTR *)(VOID *)FixedPcdGet64 (PcdAdvancedLoggerBase);
  if (LogPtr != NULL) {
    LogPtr->LogBuffer = PA_FROM_PTR (LoggerInfo);
  }
}

/**
  Get the Logger Information block

  If the PeiCore ADVANCED_LOGGER_INFO block has not been created, create the a new one. Then
  store a pointer to the block in the Hob for DXE, and update the saved Log Pointer to SEC (if
  present) and in the PeiCoreInstance.

  @param  NONE

  @return  ADVANCED_LOGGER_INFO *   - Advanced Logger Info pointer.

 **/
ADVANCED_LOGGER_INFO *
EFIAPI
AdvancedLoggerGetLoggerInfo (
  VOID
  )
{
  UINTN                   BufferSize;
  EFI_HOB_GUID_TYPE       *GuidHob;
  PEI_CORE_INSTANCE       *PeiCoreInstance;
  ADVANCED_LOGGER_INFO    *LoggerInfo;
  ADVANCED_LOGGER_INFO    *LoggerInfoSec;
  ADVANCED_LOGGER_PTR     *LogPtr;
  EFI_PHYSICAL_ADDRESS    NewLoggerInfo;
  UINTN                   Pages;
  CONST EFI_PEI_SERVICES  **PeiServices;
  EFI_STATUS              Status;

  // Try to do the minimum work at the start of this function as this
  // is called quite often.
  PeiServices = GetPeiServicesTablePointer ();
  if (PeiServices == NULL) {
    // In reality, the GetPeiServicesTablePointer will ASSERT if the
    // PeiServices table pointer is NULL.  So, this would fail miserably
    // on DEBUG builds.  It doesn't, so that means there are no DEBUG prints
    // from PEI_CORE before the PeiServices table pointer is set.

    LoggerInfoSec = GetSecLoggerInfo ();

    // Return with possible SEC LoggerInfo
    return LoggerInfoSec;
  }

  PeiCoreInstance = PEI_CORE_INSTANCE_FROM_PS_THIS (PeiServices);
  LoggerInfo      = ALI_FROM_PA (PeiCoreInstance->PlatformBlob);
  if ((LoggerInfo != NULL) && (LoggerInfo->Signature == ADVANCED_LOGGER_SIGNATURE)) {
    // Logger Info was saved from an earlier call - Return LoggerInfo.
    return LoggerInfo;
  }

  // In the time between PeiCore TemporaryRamDonePpi->TemporaryRamDone
  // and gEfiPeiMemoryDiscoveredPpiGuid being installed, CAR (and PlatformBlob)
  // may become invalid due to CAR relocations. Check if gAdvancedLoggerHobGuid
  // already exists, but that the CAR pointers are no longer valid and update
  // pointers
  GuidHob = GetFirstGuidHob (&gAdvancedLoggerHobGuid);
  if (GuidHob != NULL) {
    EFI_PEI_HOB_POINTERS  Hob;
    Hob.Raw = GetHobList ();
    while ((Hob.Raw = GetNextHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, Hob.Raw)) != NULL) {
      // Go through Allocation Hobs, attempting to find an allocation that matches
      //  the expected Advanced Logger Signature
      LoggerInfo = ALI_FROM_PA (Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress);
      if (LoggerInfo->Signature == ADVANCED_LOGGER_SIGNATURE) {
        // Update the PlatformBlob pointer to point to the allocation that was relocated to
        // real memory
        (PEI_CORE_INSTANCE_FROM_PS_THIS (PeiServices))->PlatformBlob = PA_FROM_PTR (LoggerInfo);

        // return the pointer
        return LoggerInfo;
      }

      Hob.Raw = GET_NEXT_HOB (Hob);
    }

    // Nothing valid was found, ensure LoggerInfo is still NULL to not interfere with fallthrough
    // in next code section.
    LoggerInfo = NULL;
  }

  //
  // No Logger Info - this must be the time to allocate a new LoggerInfo and save
  // the pointer in the PeiCoreInstance.
  //
  LoggerInfoSec = GetSecLoggerInfo ();
  Status        = PeiServicesCreateHob (
                    EFI_HOB_TYPE_GUID_EXTENSION,
                    (UINT16)(sizeof (EFI_HOB_GUID_TYPE) + sizeof (ADVANCED_LOGGER_PTR)),
                    (VOID **)&GuidHob
                    );
  if (!EFI_ERROR (Status)) {
    LogPtr = (ADVANCED_LOGGER_PTR *)GET_GUID_HOB_DATA (GuidHob);
    if (LoggerInfoSec == NULL) {
      //
      // This is the "No SEC Debug Agent" path.
      //
      // If PEI Core has the available RAM, the PeiAdvancedLoggerPeiInRAM PCD can be set
      // to allow the full Logger Buffer to be allocated here.  Otherwise, PEI Core must
      // allocate the small temporary buffer for the in memory log, and register for the
      // Memory Discovered Ppi.  At that time, the full in memory log buffer is allocated.
      //

      if (FeaturePcdGet (PcdAdvancedLoggerPeiInRAM)) {
        Pages =  FixedPcdGet32 (PcdAdvancedLoggerPages);
      } else {
        Pages =  FixedPcdGet32 (PcdAdvancedLoggerPreMemPages);
      }

      BufferSize = EFI_PAGES_TO_SIZE (Pages);

      Status = PeiServicesAllocatePages (
                 EfiReservedMemoryType,
                 Pages,
                 &NewLoggerInfo
                 );
      if (!EFI_ERROR (Status)) {
        LoggerInfo = ALI_FROM_PA (NewLoggerInfo);
        ZeroMem ((VOID *)LoggerInfo, BufferSize);
        LoggerInfo->Signature     = ADVANCED_LOGGER_SIGNATURE;
        LoggerInfo->Version       = ADVANCED_LOGGER_VERSION;
        LoggerInfo->LogBuffer     = PA_FROM_PTR (LoggerInfo + 1);
        LoggerInfo->LogBufferSize = BufferSize - sizeof (ADVANCED_LOGGER_INFO);
        LoggerInfo->LogCurrent    = LoggerInfo->LogBuffer;
        LoggerInfo->HwPrintLevel  = FixedPcdGet32 (PcdAdvancedLoggerHdwPortDebugPrintErrorLevel);
        AdvancedLoggerHdwPortInitialize ();
        LoggerInfo->HdwPortInitialized = TRUE;
      }
    } else {
      LoggerInfo = LoggerInfoSec;
    }

    if (LoggerInfo != NULL) {
      // Mark the Hob valid by setting its GUID

      CopyGuid (&GuidHob->Name, &gAdvancedLoggerHobGuid);
      //
      // Update the HOB pointers to point to the current LoggerInfo
      //
      LogPtr->LogBuffer                                            = PA_FROM_PTR (LoggerInfo);
      LogPtr->Signature                                            = ADVANCED_LOGGER_PTR_SIGNATURE;
      (PEI_CORE_INSTANCE_FROM_PS_THIS (PeiServices))->PlatformBlob = PA_FROM_PTR (LoggerInfo);

      //
      // If LoggerInfo from SEC, then update the SEC pointer to point to the new
      // PEI Core version of LoggerInfo
      if ((LoggerInfoSec != NULL) && !(LoggerInfoSec->InPermanentRAM)) {
        UpdateSecLoggerInfo (LoggerInfo);
      }

      //
      // Publish the Advanced Logger Ppi
      //
      Status = PeiServicesInstallPpi (mAdvancedLoggerPpiList);
      ASSERT_EFI_ERROR (Status);

      if (FeaturePcdGet (PcdAdvancedLoggerPeiInRAM)) {
        LoggerInfo->InPermanentRAM = TRUE;
        Status                     = MmUnblockMemoryRequest (NewLoggerInfo, Pages);
        if (EFI_ERROR (Status)) {
          if (Status != EFI_UNSUPPORTED) {
            DEBUG ((DEBUG_ERROR, "%a: Unable to notify StandaloneMM. Code=%r\n", __FUNCTION__, Status));
          }
        } else {
          DEBUG ((DEBUG_INFO, "%a: StandaloneMM Hob data published\n", __FUNCTION__));
        }
      } else if (FeaturePcdGet (PcdAdvancedLoggerFixedInRAM)) {
        DEBUG ((DEBUG_INFO, "%a: Standalone MM Hob of fixed data published\n", __FUNCTION__));
      } else {
        PeiServicesNotifyPpi (mMemoryDiscoveredNotifyList);
      }
    } else {
      DEBUG ((DEBUG_ERROR, "Error creating Advanced Logger Info Block 1\n"));
    }
  } else {
    DEBUG ((DEBUG_ERROR, "Error creating Advanced Logger Info Block 2\n"));
  }

  return LoggerInfo;
}
