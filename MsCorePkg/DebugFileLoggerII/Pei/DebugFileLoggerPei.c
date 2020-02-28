/** @file DebugFileLoggerPei.c

    Copyright (C) Microsoft Corporation. All rights reserved.
    SPDX-License-Identifier: BSD-2-Clause-Patent

    This file contains functions for logging debug print messages to a file.

**/

#include <Uefi.h>

#include "DebugFileLoggerPei.h"

/**
    Capture the data from a logging event and store the information in ASCII form in the logging buffer.

    @param    PeiServices Pointer to an array of PEI Foundation service function pointers.
    @param    CodeType    Indicates the type of status code being reported.
    @param    Value       Describes the current status of a hardware or software entity.
                          This included information about the class and subclass that is used to
                          classify the entity as well as an operation.
    @param    Instance    The enumeration of a hardware or software entity within
                          the system. Valid instance numbers start with 1.
    @param    CallerId    This optional parameter may be used to identify the caller.
                          This parameter allows the status code driver to apply different rules to
                          different callers.
    @param    Data        This optional parameter may be used to pass additional data.

    @retval   EFI_STATUS              Event data was successfully captured.
    @retval   EFI_DEVICE_ERROR        Function has been called a second+ time (reentrancy error)
    @retval   EFI_OUT_OF_RESOURCES    Insufficient resources to complete the operation.
    @retval   Others                  The operation failed.

**/
EFI_STATUS
EFIAPI
PeiLoggingBufferEventCapture (
    IN CONST EFI_PEI_SERVICES         **PeiServices,
    IN       EFI_STATUS_CODE_TYPE     CodeType,
    IN       EFI_STATUS_CODE_VALUE    Value,
    IN       UINT32                   Instance,
    IN CONST EFI_GUID                 *CallerId, OPTIONAL
    IN CONST EFI_STATUS_CODE_DATA     *Data      OPTIONAL
    )
{
    EFI_STATUS                        Status = EFI_SUCCESS;
    UINT32                            BytesWritten = 0;
    EFI_HOB_GUID_TYPE*                GuidHob = NULL;
    CHAR8*                            LoggingBuffer = NULL;
    EFI_DEBUG_FILELOGGING_HEADER*     LoggingBufferHeader = NULL;
    EFI_PEI_RSC_HANDLER_PPI*          mMsRscHandlerPpi;

    //
    // Get the hob to get the logging buffer and meta data. Locate the MS debug
    // logger hand-off-block.
    //
    GuidHob = GetFirstGuidHob (&gMuDebugLoggerGuid);
    if (GuidHob) {

        //
        // Find the PEI buffer in the HoB.
        //
        LoggingBufferHeader  = (EFI_DEBUG_FILELOGGING_HEADER *)(GET_GUID_HOB_DATA (GuidHob));
        if (LoggingBufferHeader != NULL){
            LoggingBuffer = (CHAR8*) (LoggingBufferHeader + 1);

        } else {
            DEBUG((DEBUG_ERROR, "%a: Cannot find the Logging buffer header for debug logging to FS\n", __FUNCTION__));
            goto CleanUp;
        }
    } else {
        DEBUG((DEBUG_ERROR, "%a: Cannot find the HOB for debug logging to FS\n", __FUNCTION__));
        goto CleanUp;
    }

    if ((LoggingBufferHeader->BytesWritten + EFI_STATUS_CODE_DATA_MAX_SIZE) > PEI_BUFFER_SIZE_DEBUG_FILE_LOGGING) {

        if (LoggingBufferHeader->BytesWritten & EFI_DEBUG_FILE_LOGGER_OVERFLOW) {
            Status = EFI_OUT_OF_RESOURCES;
            goto CleanUp;
        }

        LoggingBufferHeader->BytesWritten |= EFI_DEBUG_FILE_LOGGER_OVERFLOW;

        //
        // Locate the PEI report status code handler.
        //
        Status = PeiServicesLocatePpi (&gEfiPeiRscHandlerPpiGuid,
                                        0,
                                        NULL,
                                        (VOID **)&mMsRscHandlerPpi);

        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR,  "%a: Failed to locate PEI report status code handler: %r\n", __FUNCTION__, Status));
            goto CleanUp;
        }

        DEBUG((DEBUG_ERROR, "%a: Debug logging buffer is full, truncating at %d bytes.\n", __FUNCTION__, LoggingBufferHeader->BytesWritten));
        Status = mMsRscHandlerPpi->Unregister (PeiLoggingBufferEventCapture);
        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "%a: Failed to unregister debug file logger status code handler: %r \n", __FUNCTION__, Status));
            goto CleanUp;
        }

        Status = EFI_OUT_OF_RESOURCES;
        goto CleanUp;
    }

    //
    // Capture the event information as ASCII text.
    //
    CHAR8 *LoggingBufferWriteLoc = (CHAR8 *)(LoggingBuffer + LoggingBufferHeader->BytesWritten);
	if ((CodeType & EFI_STATUS_CODE_TYPE_MASK) == EFI_DEBUG_CODE) {
        BytesWritten = WriteStatusCodeToBuffer (CodeType,
                                                Value,
                                                Instance,
                                                (CONST EFI_GUID*) CallerId,
                                                Data,
                                                LoggingBufferWriteLoc,
                                                EFI_STATUS_CODE_DATA_MAX_SIZE);
	}

    LoggingBufferHeader->BytesWritten += BytesWritten;

CleanUp:
    return Status;
}

/**
    Main entry point for this driver.

    @param    FileHandle      PEI file handle.
    @param    PeiServices     Pointer to an array of PEI Foundation service function pointers.

    @retval   EFI_STATUS      Initialization was successful.
    @retval   Others          The operation failed.

**/
EFI_STATUS
EFIAPI
DebugFileLoggerPeiEntry (
    IN  EFI_PEI_FILE_HANDLE     FileHandle,
    IN  CONST EFI_PEI_SERVICES  **PeiServices
    )
{

    EFI_HOB_GUID_TYPE            *GuidHob;
    EFI_DEBUG_FILELOGGING_HEADER *LoggingBufferHeader;
    EFI_PEI_RSC_HANDLER_PPI      *MsRscHandlerPpi;
    EFI_STATUS                    Status = EFI_SUCCESS;

    DEBUG((DEBUG_INFO, "%a: enter... PeiServices:0x%p\n", __FUNCTION__, PeiServices));

    if (*PeiServices == NULL) {
        DEBUG((DEBUG_INFO, "%a: no pei services... leaving\n", __FUNCTION__));
        goto CleanUp;
    }

    //
    // Locate the PEI report status code handler.
    //
    Status = PeiServicesLocatePpi (&gEfiPeiRscHandlerPpiGuid,
                                    0,
                                    NULL,
                                    (VOID **) &MsRscHandlerPpi);

    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "%a: Failed to locate PEI report status code handler: %r \n", __FUNCTION__, Status));
        goto CleanUp;
    }

    //
    // Create a HoB for passing the PEI debug log up to DXE.
    //
    DEBUG((DEBUG_INFO, "%a: create hob...\n",  __FUNCTION__));
    Status = PeiServicesCreateHob (EFI_HOB_TYPE_GUID_EXTENSION,
                                   (UINT16) (sizeof(EFI_HOB_GUID_TYPE) + sizeof(EFI_DEBUG_FILELOGGING_HEADER) + PEI_BUFFER_SIZE_DEBUG_FILE_LOGGING),
                                   (VOID **)&GuidHob);

    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "%a: Failed to create HoB for passing PEI debug log to DXE: %r \n", __FUNCTION__, Status));
        goto CleanUp;
    }

    //
    // Initialize debug log accounting.
    //
    GuidHob->Name = gMuDebugLoggerGuid;
    LoggingBufferHeader = (EFI_DEBUG_FILELOGGING_HEADER *)(GuidHob + 1);
    LoggingBufferHeader->BytesWritten  = 0;
    if (MsRscHandlerPpi == NULL) {
        DEBUG((DEBUG_ERROR, "%a: MsRscHandlerPpi is NULL!\n", __FUNCTION__));
    }


    //
    // Register our logging event handler callback function.
    //
    if (MsRscHandlerPpi != NULL && MsRscHandlerPpi->Register != NULL) {
        DEBUG((DEBUG_INFO, "%a: register RSC handler... MsRscHandlerPpi:0x%p MsRscHandlerPpi->Register:0x%p\n", __FUNCTION__, MsRscHandlerPpi, MsRscHandlerPpi->Register));
        Status = MsRscHandlerPpi->Register (PeiLoggingBufferEventCapture);
        DEBUG((DEBUG_INFO, "%a: Status=0x%x\n", __FUNCTION__, Status));
    }

CleanUp:
    return Status;
}
