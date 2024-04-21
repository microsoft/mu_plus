/** @file
  Advanced Logger initialization for SEC


  Copyright (C) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>

#include <AdvancedLoggerInternal.h>

/**
  Including the PeiMain.h from PeiCore in order to access the Platform Blob data member.

  This is breaking the rules, but PeiCore on a system using ROM for PeiPremem has no place
  to store long term data besides the Hob or Ppi list.  Accessing these list for high
  frequency operations is a performance issue.
**/
#include <Core/Pei/PeiMain.h>

#include <Library/AdvancedLoggerHdwPortLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DebugAgentLib.h>
#include <Library/HobLib.h>
#include <Library/MmUnblockMemoryLib.h>
#include <Library/PeiServicesLib.h>

#include "AdvancedLoggerSecDebugAgent.h"

/**
  Initialize debug agent.

  This function is used to set up the Advanced Logger.

  If the parameter Function is not NULL, Debug Agent Library instance will invoke it by
  passing in the Context to be its parameter.

  If Function() is NULL, Debug Agent Library instance will return after setup debug
  environment.

  @param[in] InitFlag     InitFlag is used to decide the initialize process.
  @param[in] Context      Context needed according to InitFlag; it was optional.
  @param[in] Function     Continue function called by debug agent library; it was
                          optional.

**/
VOID
EFIAPI
InitializeDebugAgent (
  IN UINT32                InitFlag,
  IN VOID                  *Context   OPTIONAL,
  IN DEBUG_AGENT_CONTINUE  Function  OPTIONAL
  )
{
  EFI_PHYSICAL_ADDRESS    CarBase;
  UINTN                   DebugLevel;
  EFI_HOB_GUID_TYPE       *GuidHob;
  UINTN                   LogBufferSize;
  ADVANCED_LOGGER_INFO    *LoggerInfo;
  ADVANCED_LOGGER_PTR     *LogPtr;
  ADVANCED_LOGGER_PTR     *LogPtrSec;
  EFI_PHYSICAL_ADDRESS    NewLogBuffer;
  ADVANCED_LOGGER_INFO    *NewLoggerInfo;
  PEI_CORE_INSTANCE       *PeiCoreInstance;
  CONST EFI_PEI_SERVICES  **PeiServices;
  EFI_SEC_PEI_HAND_OFF    *SecCoreData;
  EFI_PHYSICAL_ADDRESS    SecLogBuffer;
  EFI_STATUS              Status;
  CHAR8                   *TargetLog;

  if (InitFlag == DEBUG_AGENT_INIT_PREMEM_SEC) {
    SecCoreData = (EFI_SEC_PEI_HAND_OFF *)Context;

    // At SEC entry
    // |-------------------|---->
    // |IDT Table          |
    // |-------------------|
    // |PeiService Pointer |    PeiStackSize
    // |-------------------|
    // |                   |
    // |      Stack        |
    // |-------------------|---->
    // |                   |
    // |                   |
    // |      Heap         |    PeiTemporayRamSize
    // |                   |
    // |                   |
    // |-------------------|---->  TempRamBase

    ASSERT (SecCoreData->PeiTemporaryRamBase == (VOID *)(UINTN)FixedPcdGet64 (PcdAdvancedLoggerBase));

    LogPtr = (ADVANCED_LOGGER_PTR *)SecCoreData->PeiTemporaryRamBase;

    DEBUG ((DEBUG_ERROR, "%a Initializing AdvancedLogger.\n", __FUNCTION__));

    SecCoreData->PeiTemporaryRamBase  = (VOID *)(((UINTN)SecCoreData->PeiTemporaryRamBase) + sizeof (ADVANCED_LOGGER_PTR));
    SecCoreData->PeiTemporaryRamSize -= sizeof (ADVANCED_LOGGER_PTR);

    // After carving out Logger Buffer Pointer

    // |----------------------|---->
    // |IDT Table             |
    // |----------------------|
    // |PeiService Pointer    |    PeiStackSize
    // |----------------------|
    // |                      |
    // |      Stack           |
    // |----------------------|---->
    // |                      |
    // |                      |
    // |      Heap            |    PeiTemporayRamSize
    // |                      |
    // |                      |
    // |----------------------|----> New TempRamBase
    // | ADVANCED_LOGGER_PTR  |----> Contains physical address of LoggerInfo
    // |----------------------|---->

    LogBufferSize = EFI_PAGES_TO_SIZE (FixedPcdGet64 (PcdAdvancedLoggerPreMemPages));
    CarBase       = (EFI_PHYSICAL_ADDRESS)FixedPcdGet64 (PcdAdvancedLoggerCarBase);

    NewLogBuffer = AllocateRamForSEC (CarBase, LogBufferSize);
    if (NewLogBuffer != 0ULL) {
      LoggerInfo = ALI_FROM_PA (NewLogBuffer);
      ZeroMem ((VOID *)LoggerInfo, sizeof (ADVANCED_LOGGER_INFO));
      LoggerInfo->Signature          = ADVANCED_LOGGER_SIGNATURE;
      LoggerInfo->Version            = ADVANCED_LOGGER_VERSION;
      LoggerInfo->LogBufferSize      = (UINT32)(LogBufferSize - sizeof (ADVANCED_LOGGER_INFO));
      LoggerInfo->LogBufferOffset          = EXPECTED_LOG_BUFFER_OFFSET (LoggerInfo);
      LoggerInfo->LogCurrentOffset         = LoggerInfo->LogBufferOffset;
      LoggerInfo->HdwPortInitialized = TRUE;
      LoggerInfo->HwPrintLevel       = FixedPcdGet32 (PcdAdvancedLoggerHdwPortDebugPrintErrorLevel);
      LogPtr->LogBuffer              = NewLogBuffer; // Set physical address of Logger Memory at TemporaryRamBase
      LogPtr->Signature              = ADVANCED_LOGGER_PTR_SIGNATURE;

      DEBUG ((DEBUG_INFO, "%a: Start. SecLogInfo=%p\n", __FUNCTION__, LoggerInfo));
      DUMP_HEX (DEBUG_INFO, 0, (VOID *)LoggerInfo, sizeof (ADVANCED_LOGGER_INFO), "");
      // From this point until the PeiDebugLib constructor creates the Logger Info HOB, the access
      // to the LoggerInfo is via PCD to the well known address.  However, there is some overlap between
      // PEI and SEC due to SEC PPI callbacks. There will be two transition of logging:
      //
      //  1. When the PeiCore AdvancedLoggerLib constructor is called, a HOB is created with a Logger Info block.
      //     The Logger Info block created here is copied to the HOB, and the Sec LoggerInfo PTR is updated
      //     to point to the HOB version of the Logger Info block. All future SEC references will reference
      //     the HOB version of the Logger Info block. This way, even if SEC code were to access the well
      //     known PCD address, the current Logger Info block will be used.
      //
      //  2. When Memory is available, the PeiCore constructor is called again, and the Logger Info HOB is
      //     updated to a proper in memory buffer, and the temporary RAM buffer is copied into the proper
      //     memory buffer. This memory buffer will be used for the rest of boot.
      //
      //  3. The last transition is to DXE at DXE_CORE initialization.  The HOB information is duplicated into a
      //     DXE version of the Logger Info block, and a protocol is published for the normal DXE DebugLib.
    }
  } else if (InitFlag == DEBUG_AGENT_INIT_POSTMEM_SEC) {
    //
    // Update HOB for use after memory has been allocated
    //
    // We have to copy the log buffer from temporary to permanent RAM here
    // and not in PeiCore gEfiPeiMemoryDiscoveredPpiGuid install notification as PeiMain
    // calls SEC to terminate cache as RAM before it publishes the Memory
    // Discovered GUID.
    //
    DEBUG ((DEBUG_INFO, "%a: Find PeiCore HOB...\n", __FUNCTION__));
    GuidHob = GetFirstGuidHob (&gAdvancedLoggerHobGuid);
    if (GuidHob != NULL) {
      DEBUG ((DEBUG_INFO, "%a: Updating PeiCore HOB...%p\n", __FUNCTION__, GuidHob));
      LogPtr     = (ADVANCED_LOGGER_PTR *)PTR_FROM_PA (GET_GUID_HOB_DATA (GuidHob));
      LoggerInfo = ALI_FROM_PA (LogPtr->LogBuffer);
      if (LoggerInfo->Signature == ADVANCED_LOGGER_SIGNATURE) {
        //
        // Reserved memory type is used to grant the SMM Library to access the logger
        // buffer.  The downside of this, is that this memory is both available to
        // the OS to read, but is it not available for the OS to use.
        //
        Status = PeiServicesAllocatePages (
                   EfiReservedMemoryType,
                   FixedPcdGet32 (PcdAdvancedLoggerPages),
                   &NewLogBuffer
                   );
        if (!EFI_ERROR (Status)) {
          DEBUG ((
            DEBUG_INFO,
            "%a: - Old Info=%p Buffer Offset=%X, Current Offset=%X, Size=%d, Used=%d\n",
            __FUNCTION__,
            LoggerInfo,
            LoggerInfo->LogBufferOffset,
            LoggerInfo->LogCurrentOffset,
            LoggerInfo->LogBufferSize,
            USED_LOG_SIZE (LoggerInfo)
            ));

          NewLoggerInfo = ALI_FROM_PA (NewLogBuffer);
          CopyMem ((VOID *)NewLoggerInfo, (VOID *)LoggerInfo, sizeof (ADVANCED_LOGGER_INFO));
          NewLoggerInfo->LogBufferOffset = EXPECTED_LOG_BUFFER_OFFSET (LoggerInfo);
          TargetLog                = CHAR8_FROM_PA (LOG_BUFFER_FROM_ALI (NewLoggerInfo));

          if (LoggerInfo->LogCurrentOffset > 0) {
            CopyMem (
              TargetLog,
              CHAR8_FROM_PA (LOG_BUFFER_FROM_ALI (LoggerInfo)),
              USED_LOG_SIZE (LoggerInfo)
              );
          }

          NewLoggerInfo->LogBufferSize  = (EFI_PAGE_SIZE * FixedPcdGet32 (PcdAdvancedLoggerPages)) - sizeof (ADVANCED_LOGGER_INFO);
          NewLoggerInfo->LogCurrentOffset     = LoggerInfo->LogCurrentOffset;
          NewLoggerInfo->InPermanentRAM = TRUE;

          PeiServices                   = GetPeiServicesTablePointer ();
          PeiCoreInstance               = PEI_CORE_INSTANCE_FROM_PS_THIS (PeiServices);
          PeiCoreInstance->PlatformBlob = PA_FROM_PTR (NewLoggerInfo);

          //
          // Update the HOB Buffer LoggerInfo pointers
          //
          LogPtr->LogBuffer = NewLogBuffer;

          // Update the SEC pointer and free the RamForSEC buffer.
          //
          LogPtrSec    = (ADVANCED_LOGGER_PTR *)(VOID *)(UINTN)FixedPcdGet64 (PcdAdvancedLoggerBase);
          SecLogBuffer = LogPtrSec->LogBuffer;

          // Update Sec LogPtr to point to NewLogBuffer
          LogPtrSec->LogBuffer = NewLogBuffer;

          FreeRamForSEC (SecLogBuffer);

          Status = MmUnblockMemoryRequest (NewLogBuffer, FixedPcdGet32 (PcdAdvancedLoggerPages));
          if (EFI_ERROR (Status)) {
            if (Status != EFI_UNSUPPORTED) {
              DEBUG ((DEBUG_ERROR, "%a: Unable to notify StandaloneMM. Code=%r\n", __FUNCTION__, Status));
            }
          } else {
            DEBUG ((DEBUG_INFO, "%a: StandaloneMM Hob data published\n", __FUNCTION__));
          }

          if (NewLoggerInfo->DiscardedSize != 0) {
            DebugLevel = DEBUG_ERROR;
          } else {
            DebugLevel = DEBUG_INFO;
          }

          DEBUG ((
            DebugLevel,
            "%a: - New Info=%p, Buffer=%X, Current=%X, Size=%d, Discarded=%d\n",
            __FUNCTION__,
            NewLoggerInfo,
            NewLoggerInfo->LogBufferOffset,
            NewLoggerInfo->LogCurrentOffset,
            NewLoggerInfo->LogBufferSize,
            NewLoggerInfo->DiscardedSize
            ));
          DUMP_HEX (DEBUG_INFO, 0, (VOID *)LogPtr, sizeof (ADVANCED_LOGGER_PTR), "");
          DUMP_HEX (DEBUG_INFO, 0, (VOID *)NewLoggerInfo, sizeof (ADVANCED_LOGGER_INFO), "");
        }
      }
    } else {
      DEBUG ((DEBUG_INFO, "%a: PeiCore HOB not found...\n", __FUNCTION__));
    }
  }

  if (Function != NULL) {
    Function (Context);
  }
}

/**
  Enable/Disable the interrupt of debug timer and return the interrupt state
  prior to the operation.

  If EnableStatus is TRUE, enable the interrupt of debug timer.
  If EnableStatus is FALSE, disable the interrupt of debug timer.

  @param[in] EnableStatus    Enable/Disable.

  @retval TRUE  Debug timer interrupt were enabled on entry to this call.
  @retval FALSE Debug timer interrupt were disabled on entry to this call.

**/
BOOLEAN
EFIAPI
SaveAndSetDebugTimerInterrupt (
  IN BOOLEAN  EnableStatus
  )
{
  return FALSE;
}
