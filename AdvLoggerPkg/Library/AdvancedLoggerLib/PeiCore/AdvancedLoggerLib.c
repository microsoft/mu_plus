/** @file
  PEI CORE implementation of the Advanced Logger Library

  Copyright (c) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>

#include <AdvancedLoggerInternal.h>

#include <Ppi/AdvancedLogger.h>
#include <Library/BaseLib.h>
#include <Library/AdvancedLoggerLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/PcdLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/SynchronizationLib.h>

#include "../AdvancedLoggerCommon.h"

EFI_STATUS
EFIAPI
InstallPermanentMemoryBuffer (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
);

ADVANCED_LOGGER_PPI                 mAdvancedLoggerPpi = {
    AdvancedLoggerWrite,
    DebugVPrint,
    DebugAssert,
    DebugDumpMemory
};

CONST EFI_PEI_PPI_DESCRIPTOR        mAdvancedLoggerPpiList[] = {
    {
        (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
        &gAdvancedLoggerPpiGuid,
        &mAdvancedLoggerPpi
    }
};

CONST EFI_PEI_NOTIFY_DESCRIPTOR     mMemoryDiscoveredNotifyList[] = {
    {
        (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_DISPATCH | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
        &gEfiPeiMemoryDiscoveredPpiGuid,
        InstallPermanentMemoryBuffer
    }
};

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
  IN EFI_PEI_SERVICES          **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
    UINTN                       BufferSize;
    UINTN                       DebugLevel;
    EFI_HOB_GUID_TYPE          *GuidHob;
    ADVANCED_LOGGER_INFO       *LoggerInfo;
    ADVANCED_LOGGER_PTR        *LogPtr;
    EFI_PHYSICAL_ADDRESS        NewLogBuffer;
    ADVANCED_LOGGER_INFO       *NewLoggerInfo;
    EFI_PHYSICAL_ADDRESS        OldLoggerInfo;
    EFI_STATUS                  Status;

    DEBUG((DEBUG_INFO, "%a: Find PeiCore HOB...\n",  __FUNCTION__));
    GuidHob = GetFirstGuidHob (&gAdvancedLoggerHobGuid);
    if (GuidHob == NULL) {
        DEBUG((DEBUG_ERROR, "%a: Advanced Logger Hob not found\n",  __FUNCTION__));
    } else {
        LogPtr = (ADVANCED_LOGGER_PTR *) GET_GUID_HOB_DATA(GuidHob);
        LoggerInfo = ALI_FROM_PA(LogPtr->LoggerInfo);
        if (!LoggerInfo->InPermanentRAM) {
            //
            // Must be PeiCore allocated small memory buffer
            //
            Status = PeiServicesAllocatePages (EfiReservedMemoryType,
                                               FixedPcdGet32 (PcdAdvancedLoggerPages),
                                              &NewLogBuffer);
            if (!EFI_ERROR(Status)) {
                NewLoggerInfo = ALI_FROM_PA(NewLogBuffer);
                CopyMem ((VOID *)NewLoggerInfo, (VOID *)LoggerInfo, sizeof (ADVANCED_LOGGER_INFO));
                BufferSize = (UINTN) (LoggerInfo->LogCurrent - LoggerInfo->LogBuffer);
                NewLoggerInfo->LogBuffer = PA_FROM_PTR( (CHAR8 *) (NewLoggerInfo + 1));

                if (BufferSize > 0) {
                    CopyMem (PTR_FROM_PA(NewLoggerInfo->LogBuffer),
                             PTR_FROM_PA(LoggerInfo->LogBuffer),
                            (BufferSize));
                }

                NewLoggerInfo->LogBufferSize = EFI_PAGES_TO_SIZE (FixedPcdGet32 (PcdAdvancedLoggerPages)) - sizeof(ADVANCED_LOGGER_INFO);
                NewLoggerInfo->LogCurrent = PA_FROM_PTR( CHAR8_FROM_PA(NewLogBuffer) + BufferSize );
                NewLoggerInfo->InPermanentRAM = TRUE;
                //
                // Update the HOB pointer
                //
                OldLoggerInfo = LogPtr->LoggerInfo;
                LogPtr->LoggerInfo = PA_FROM_PTR (NewLoggerInfo);
                PeiServicesFreePages (OldLoggerInfo,
                                      FixedPcdGet32 (PcdAdvancedLoggerPreMemPages));

                if (NewLoggerInfo->DiscardedSize != 0) {
                    DebugLevel = DEBUG_ERROR;
                } else {
                    DebugLevel = DEBUG_INFO;
                }

                DEBUG((DebugLevel, "%a: - New Info=%p, Buffer=%lx, Current=%lx, Size=%d, Discarded=%d\n",
                    __FUNCTION__,
                    NewLoggerInfo,
                    NewLoggerInfo->LogBuffer,
                    NewLoggerInfo->LogCurrent,
                    NewLoggerInfo->LogBufferSize,
                    NewLoggerInfo->DiscardedSize));
            }
        }
    }

    return EFI_SUCCESS;
}

/**
  Get the SEC Logger Information block

 **/
STATIC
ADVANCED_LOGGER_INFO *
GetSecLoggerInfo (
    VOID
  ) {
    ADVANCED_LOGGER_INFO       *LoggerInfoSec;
    ADVANCED_LOGGER_PTR        *LogPtr;

    //
    // Locate the SEC Logger Information block.
    //
    // The PCD AdvancedLoggerBase MAY be a 64 bit address.  However, it is
    // trimmed to be a pointer the size of the actual platform - and the Pcd is expected
    // to be set accordingly.
    LogPtr = (ADVANCED_LOGGER_PTR *) (VOID *) FixedPcdGet64 (PcdAdvancedLoggerBase);
    LoggerInfoSec = NULL;
    if (LogPtr != NULL) {
        LoggerInfoSec = ALI_FROM_PA(LogPtr->LoggerInfo);
    }

    return LoggerInfoSec;
}

/**
  UpdateSecLoggerInfo

  Updates the SEC LoggerInfo pointer with the one used by PEI.  This
  way, additional SEC messages will occur in the proper place in the
  log buffer.
  **/
STATIC
VOID
UpdateSecLoggerInfo (
    ADVANCED_LOGGER_INFO * LoggerInfo
  ) {
    ADVANCED_LOGGER_PTR        *LogPtr;

    //
    // Locate the SEC Logger Information block.
    //
    // The PCD AdvancedLoggerBase MAY be a 64 bit address.  However, it is
    // trimmed to be a pointer the size of the actual platform - and the Pcd is expected
    // to be set accordingly.
    LogPtr = (ADVANCED_LOGGER_PTR *) (VOID *) FixedPcdGet64 (PcdAdvancedLoggerBase);
    if (LogPtr != NULL) {
        LogPtr->LoggerInfo = PA_FROM_PTR (LoggerInfo);
    }
}

/**
  Get the Logger Information block

 **/

/*
   No special security checks before EndOfDxe
*/
ADVANCED_LOGGER_INFO *
EFIAPI
AdvancedLoggerGetLoggerInfo (
    VOID
) {
    UINTN                       BufferSize;
    EFI_HOB_GUID_TYPE          *GuidHob;
    VOID                       *HobList;
    ADVANCED_LOGGER_INFO       *LoggerInfo;
    ADVANCED_LOGGER_INFO       *LoggerInfoSec;
    ADVANCED_LOGGER_PTR        *LogPtr;
    EFI_PHYSICAL_ADDRESS        NewLoggerInfo;
    UINTN                       Pages;
    CONST EFI_PEI_SERVICES    **PeiServices;
    EFI_STATUS                  Status;

    STATIC ADVANCED_LOGGER_INFO *mLoggerInfo = NULL;

    //
    // Once memory is installed, the static variable will work.
    //
    if (mLoggerInfo != NULL) {
        return mLoggerInfo;
    }

    // Debug messages prior to the proper initialization of the HOB
    // is valid.  If there is a SEC Logger Info block, use it.
    //
    LoggerInfoSec = GetSecLoggerInfo ();
    PeiServices = GetPeiServicesTablePointer ();
    if (PeiServices == NULL) {
        // Return with possible SEC LoggerInfo
        return LoggerInfoSec;
    }

    HobList = NULL;
    Status = (*PeiServices)->GetHobList (PeiServices, &HobList);
    if (EFI_ERROR(Status) || HobList == NULL) {
        // Return with possible SEC LoggerInfo
        return LoggerInfoSec;
    }

    //
    // If the above tests pass, PEI should be initialized enough to create the HOB,
    // create the Ppi to capture and display PEIM Debug messages, and register for
    // the memory available notification.
    //
    LoggerInfo = LoggerInfoSec;
    GuidHob = GetFirstGuidHob (&gAdvancedLoggerHobGuid);
    if (GuidHob != NULL) {
        LogPtr = (ADVANCED_LOGGER_PTR *) GET_GUID_HOB_DATA(GuidHob);
        LoggerInfo = ALI_FROM_PA(LogPtr->LoggerInfo);
    } else {
        Status = PeiServicesCreateHob (EFI_HOB_TYPE_GUID_EXTENSION,
                                       (UINT16) (sizeof(EFI_HOB_GUID_TYPE) + sizeof(ADVANCED_LOGGER_PTR)),
                                       (VOID **) &GuidHob);
        if (!EFI_ERROR(Status)) {
            LogPtr = (ADVANCED_LOGGER_PTR *) GET_GUID_HOB_DATA(GuidHob);
            if (LoggerInfoSec == NULL) {
                //
                // This is the "No SEC Debug Agent" path.
                //
                // PEI Core must allocate the small temporary buffer for the in memory log,
                // and register for the Memory Discovered Ppi.  At that time, the full
                // in memory log buffer is allocated.
                //

                if (FeaturePcdGet(PcdAdvancedLoggerPeiInRAM)) {
                    Pages =  FixedPcdGet32 (PcdAdvancedLoggerPages);
                } else {
                    Pages =  FixedPcdGet32 (PcdAdvancedLoggerPreMemPages);
                }

                BufferSize = EFI_PAGES_TO_SIZE(Pages);

                Status = PeiServicesAllocatePages (EfiReservedMemoryType,
                                                   Pages,
                                                   &NewLoggerInfo);
                if (!EFI_ERROR(Status)) {
                    LoggerInfo = (ADVANCED_LOGGER_INFO *) (UINTN) NewLoggerInfo;
                    ZeroMem ((VOID *)LoggerInfo, BufferSize);
                    LoggerInfo->Signature = ADVANCED_LOGGER_SIGNATURE;
                    LoggerInfo->Version = ADVANCED_LOGGER_VERSION;
                    LoggerInfo->LogBuffer = PA_FROM_PTR(LoggerInfo + 1);
                    LoggerInfo->LogBufferSize = BufferSize - sizeof(ADVANCED_LOGGER_INFO);
                    LoggerInfo->LogCurrent = LoggerInfo->LogBuffer;

                    if (FeaturePcdGet(PcdAdvancedLoggerPeiInRAM)) {
                        LoggerInfo->InPermanentRAM = TRUE;
                    } else {
                        PeiServicesNotifyPpi (mMemoryDiscoveredNotifyList);
                    }
                }
            } else {
                LoggerInfo = LoggerInfoSec;
            }

            if (LoggerInfo != NULL) {
                //
                // Update the HOB pointer to point to current LoggerInfo
                //
                LogPtr->LoggerInfo = PA_FROM_PTR (LoggerInfo);

                //
                // Publish the mAdvancedLoggerPpiList
                GuidHob->Name = gAdvancedLoggerHobGuid;
                Status = PeiServicesInstallPpi (mAdvancedLoggerPpiList);
                ASSERT_EFI_ERROR(Status);
                mLoggerInfo = LoggerInfo;

                if (LoggerInfoSec != NULL) {
                    UpdateSecLoggerInfo (LoggerInfo);
                }
            }
        }
    }

    return LoggerInfo;
}
