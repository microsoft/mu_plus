/** @file
  Advanced Logger initialization for SEC


  Copyright (C) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>

#include <AdvancedLoggerInternal.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DebugAgentLib.h>
#include <Library/HobLib.h>
#include <Library/PeiServicesLib.h>

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
    IN VOID                 *Context   OPTIONAL,
    IN DEBUG_AGENT_CONTINUE  Function  OPTIONAL
  ) {
    UINTN                       BufferSize;
    UINTN                       DebugLevel;
    EFI_HOB_GUID_TYPE          *GuidHob;
    ADVANCED_LOGGER_INFO       *LoggerInfo;
    ADVANCED_LOGGER_PTR        *LogPtr;
    EFI_PHYSICAL_ADDRESS        NewLogBuffer;
    ADVANCED_LOGGER_INFO       *NewLoggerInfo;
    UINTN                       PreMemSize;
    EFI_SEC_PEI_HAND_OFF       *SecCoreData;
    EFI_STATUS                  Status;
    CHAR8                      *TargetLog;

    if (InitFlag == DEBUG_AGENT_INIT_PREMEM_SEC) {
        SecCoreData = (EFI_SEC_PEI_HAND_OFF *) Context;

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

        PreMemSize = EFI_PAGES_TO_SIZE (FixedPcdGet32 (PcdAdvancedLoggerPreMemPages));
        ASSERT (SecCoreData->PeiTemporaryRamBase == (VOID *) FixedPcdGet64 (PcdAdvancedLoggerBase));
        ASSERT (SecCoreData->PeiTemporaryRamSize >  PreMemSize);

        if ((SecCoreData->PeiTemporaryRamBase == (VOID *) FixedPcdGet64 (PcdAdvancedLoggerBase)) &&
            (SecCoreData->PeiTemporaryRamSize >  PreMemSize)) {
            // So we don't have to save the PeiTemporaryRamBase in some exotic manner, this code
            // assumes that PeiTemproaryRamBase is still the same as the PCD defined value

            LogPtr = (ADVANCED_LOGGER_PTR *) SecCoreData->PeiTemporaryRamBase;
            LoggerInfo = (ADVANCED_LOGGER_INFO *) (LogPtr + 1);
            LogPtr->LoggerInfo = PA_FROM_PTR (LoggerInfo);
            SecCoreData->PeiTemporaryRamBase =  (VOID *) (((UINTN) SecCoreData->PeiTemporaryRamBase) + PreMemSize);
            SecCoreData->PeiTemporaryRamSize -= PreMemSize;
            ZeroMem ((VOID *) LoggerInfo, sizeof (ADVANCED_LOGGER_INFO));
            LoggerInfo->Signature = ADVANCED_LOGGER_SIGNATURE;
            LoggerInfo->Version = ADVANCED_LOGGER_VERSION;
            LoggerInfo->LogBufferSize = PreMemSize - sizeof(ADVANCED_LOGGER_INFO) - sizeof(LogPtr);
            LoggerInfo->LogBuffer = PA_FROM_PTR ( (LoggerInfo + 1));
            LoggerInfo->LogCurrent = LoggerInfo->LogBuffer;

            DEBUG((DEBUG_INFO, "%a: Start. SecLogInfo=%p\n", __FUNCTION__, LoggerInfo));
            DEBUG_BUFFER(DEBUG_INFO, (VOID *) LoggerInfo, sizeof(ADVANCED_LOGGER_INFO), DEBUG_DM_PRINT_ADDRESS | DEBUG_DM_PRINT_ASCII);

            // After carving out Logger Buffer

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
            // |                   |
            // |    Logger Buffer  |
            // |                   |
            // |-------------------|---->
            // |   LoggerInfo      |<---+   Temp Logger Info until Pei hob created
            // |-------------------|    |
            // |   LoggerInfo PTR  |----+
            // |-------------------|----> LoggerInfo->


            // From this point until the PeiDebugLib constructor creates the Logger Info HOB, the access
            // to the LoggerInfo is via PCD to the well known address.  However, there is some overlap between
            // PEI and SEC due to SEC PPI callbacks. There will be two transition of logging:
            //
            //  1. When the PeiCore DebugLib constructor is called, a HOB is created with a Logger Info block.
            //     The Logger Info block created here is copied to the HOB, and the Sec LoggerInfo PTR is updated
            //     to point to the HOB version of the Logger Info block. All future SEC references will reference
            //     the HOB version of the Logger Info block. This way, even if SEC code were to access the well
            //     know PCD address, the one current Logger Info block will be used.
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
        DEBUG((DEBUG_INFO, "%a: Find PeiCore HOB...\n",  __FUNCTION__));
        GuidHob = GetFirstGuidHob (&gAdvancedLoggerHobGuid);
        if (GuidHob != NULL) {
            DEBUG((DEBUG_INFO, "%a: Updating PeiCore HOB...%p\n",  __FUNCTION__, GuidHob));
            LogPtr = (ADVANCED_LOGGER_PTR *) PTR_FROM_PA( GET_GUID_HOB_DATA(GuidHob));
            LoggerInfo = ALI_FROM_PA(LogPtr->LoggerInfo);
            if (LoggerInfo->Signature == ADVANCED_LOGGER_SIGNATURE) {
                //
                // Reserved memory type is used to grant the SMM Library to access the logger
                // buffer.  The downside of this, is that this memory is both available to
                // the OS to read, but is it not available for the OS to use.
                //
                Status = PeiServicesAllocatePages (EfiReservedMemoryType,
                                                   FixedPcdGet32(PcdAdvancedLoggerPages),
                                                  &NewLogBuffer);
                if (!EFI_ERROR(Status)) {
                    BufferSize = (UINTN) (LoggerInfo->LogCurrent - LoggerInfo->LogBuffer);
                    DEBUG((DEBUG_INFO, "%a: - Old Info=%p Buffer=%LX, Current=%LX, Size=%d\n",
                        __FUNCTION__,
                        LoggerInfo,
                        LoggerInfo->LogBuffer,
                        LoggerInfo->LogCurrent,
                        LoggerInfo->LogBufferSize));

                    NewLoggerInfo = ALI_FROM_PA(NewLogBuffer);
                    CopyMem ((VOID *) NewLoggerInfo, (VOID *) LoggerInfo, sizeof (ADVANCED_LOGGER_INFO));
                    NewLoggerInfo->LogBuffer = PA_FROM_PTR( (NewLoggerInfo + 1));
                    if ((NewLoggerInfo->Signature == LoggerInfo->Signature) &&
                        (NewLoggerInfo->LogBuffer == LoggerInfo->LogBuffer) &&
                        (NewLoggerInfo->LogBufferSize == LoggerInfo->LogBufferSize) &&
                        (((UINTN) (NewLoggerInfo->LogCurrent - NewLoggerInfo->LogBuffer)) <
                            (LoggerInfo->LogBufferSize / 2))) {
                        // Looks like the log buffer is in the same place, and may have some
                        // Valid Data from the previous boot. Leave that log there,
                        // and append the new SEC buffer at the end.
                        TargetLog = CHAR8_FROM_PA(NewLoggerInfo->LogCurrent);
                    } else {
                        TargetLog = CHAR8_FROM_PA(NewLoggerInfo->LogBuffer);
                    }

                    if (BufferSize > 0) {
                        CopyMem (TargetLog,
                                 CHAR8_FROM_PA(LoggerInfo->LogBuffer),
                                (BufferSize));
                    }

                    NewLoggerInfo->LogBufferSize = (EFI_PAGE_SIZE * FixedPcdGet32(PcdAdvancedLoggerPages));
                    NewLoggerInfo->LogCurrent = PA_FROM_PTR( TargetLog + BufferSize );
                    NewLoggerInfo->InPermanentRAM = TRUE;

                    //
                    // Update the HOB pointer
                    //
                    LogPtr->LoggerInfo = PA_FROM_PTR (NewLoggerInfo);
                    //
                    // Update the SEC pointer
                    //
                    LogPtr = (ADVANCED_LOGGER_PTR *) (VOID *) FixedPcdGet64 (PcdAdvancedLoggerBase);
                    LogPtr->LoggerInfo = PA_FROM_PTR (LoggerInfo);

                    if (NewLoggerInfo->DiscardedSize != 0) {
                        DebugLevel = DEBUG_ERROR;
                    } else {
                        DebugLevel = DEBUG_INFO;
                    }

                    DEBUG((DebugLevel, "%a: - New Info=%p, Buffer=%LX, Current=%LX, Size=%d, Discarded=%d\n",
                        __FUNCTION__,
                        NewLoggerInfo,
                        NewLoggerInfo->LogBuffer,
                        NewLoggerInfo->LogCurrent,
                        NewLoggerInfo->LogBufferSize,
                        NewLoggerInfo->DiscardedSize));
                    DEBUG_BUFFER(DEBUG_INFO, (VOID *) LogPtr, sizeof(ADVANCED_LOGGER_PTR), DEBUG_DM_PRINT_ADDRESS | DEBUG_DM_PRINT_ASCII);
                    DEBUG_BUFFER(DEBUG_INFO, (VOID *) NewLoggerInfo, sizeof(ADVANCED_LOGGER_INFO), DEBUG_DM_PRINT_ADDRESS | DEBUG_DM_PRINT_ASCII);
                }
            }
        } else {
            DEBUG((DEBUG_INFO, "%a: PeiCore HOB not found...\n",  __FUNCTION__));
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
  IN BOOLEAN                EnableStatus
  ) {

  return FALSE;
}
