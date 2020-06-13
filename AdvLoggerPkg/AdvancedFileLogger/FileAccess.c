/** @file FileAccess.c

This file contains the code to write the debug messages to the log file.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "AdvancedFileLogger.h"


STATIC DEBUG_LOG_FILE_INFO  mLogFiles[] = { { L"\\Logs\\UEFI_Index.txt", INDEX_FILE_SIZE },
                                            { L"\\Logs\\UEFI_Log1.txt", DEBUG_LOG_FILE_SIZE },
                                            { L"\\Logs\\UEFI_Log2.txt", DEBUG_LOG_FILE_SIZE },
                                            { L"\\Logs\\UEFI_Log3.txt", DEBUG_LOG_FILE_SIZE },
                                            { L"\\Logs\\UEFI_Log4.txt", DEBUG_LOG_FILE_SIZE },
                                            { L"\\Logs\\UEFI_Log5.txt", DEBUG_LOG_FILE_SIZE },
                                            { L"\\Logs\\UEFI_Log6.txt", DEBUG_LOG_FILE_SIZE },
                                            { L"\\Logs\\UEFI_Log7.txt", DEBUG_LOG_FILE_SIZE },
                                            { L"\\Logs\\UEFI_Log8.txt", DEBUG_LOG_FILE_SIZE },
                                            { L"\\Logs\\UEFI_Log9.txt", DEBUG_LOG_FILE_SIZE } };
#define DEBUG_LOG_FILE_COUNT ARRAY_SIZE(mLogFiles)


/**
  CheckIfNVME

  Checks if the device path is an NVME device path.

  @param Handle     Handle with Device Path protocol

  @retval  TRUE     Device is NVME
  @retval  FALSE    Device is NOT NVME or Unable to get Device Path.

  **/
STATIC
BOOLEAN
CheckIfNVME (
    IN EFI_HANDLE Handle
    )
{
    EFI_DEVICE_PATH_PROTOCOL *DevicePath;

    DevicePath = DevicePathFromHandle (Handle);
    if (DevicePath == NULL) {
        return FALSE;
    }

    while (!IsDevicePathEnd(DevicePath)) {
        if ((MESSAGING_DEVICE_PATH == DevicePathType(DevicePath)) &&
            (MSG_NVME_NAMESPACE_DP == DevicePathSubType(DevicePath)) ) {
            return TRUE;
        }
        DevicePath = NextDevicePathNode(DevicePath);
    }

    return FALSE;
}

/**
  VolumeFromFileSystemHandle

  LogDevice->Handle must have a FileSystemProtocol on it.  Get the FileSystemProtocol
  and then get the VolumeHandle.

  For a USB device to get log files, it must have a Logs directory in the root
  of the USB file system.

  For a non USB device to get Log files, it too must have a Logs directory in the root of the
  file system.  If PcdAdvancedFileLoggerForceEnable is TRUE, create the Logs directory
  in the root of the file system.

  If there is no Logs directory, or one cannot be made, return error.

  @param LogDevice          Internal control block with the device handle needed

  @retval VolumeHandle      Handle to the Volume for I/O

  **/
STATIC
EFI_FILE *
VolumeFromFileSystemHandle (
    IN LOG_DEVICE    *LogDevice
    )
{
    EFI_FILE                         *File;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem;
    EFI_TPL                           OldTpl;
    EFI_STATUS                        Status;
    EFI_FILE                         *Volume;


    File = NULL;
    Volume = NULL;

    //
    // The RaiseTPL/RestoreTPL is used to clear all pending events.  When a USB device is,
    // removed, there may be pending events to tear down the USB file system.  Just calling
    // OpenProtocol to get the EfiSimpleFileSystemProtocol causes OpenProtocol to find the
    // EfiSimpleFileSystemProtocol interface address. Then OpenProtocol calls RestoreTPL which
    // will dispatch the USB cleanup code and uninstall the EfiSimpleFileSystemProtocol.
    // OpenProtocol then returns the pointer to what is now free memory.  Accessing this free
    // memory as if it was an EfiSimpleFileSystemProtocol causes exceptions.
    //

    OldTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
    gBS->RestoreTPL (OldTpl);

    Status = gBS->OpenProtocol(LogDevice->Handle,
                              &gEfiSimpleFileSystemProtocolGuid,
                               (VOID**)&FileSystem,
                               gImageHandle,
                               NULL,
                               EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

    if (EFI_ERROR(Status)) {
        LogDevice->Valid = FALSE;
        DEBUG((DEBUG_ERROR,"%a: Failed to get FileSystem protocol. Code=%r \n", __FUNCTION__ , Status));
        return NULL;
    }

    //
    // Open the volume.
    //
    Status = FileSystem->OpenVolume(FileSystem, &Volume);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR,"%a: Failed to open volume. Code=%r \n", __FUNCTION__ , Status));
        return NULL;
    }

    Status = Volume->Open (Volume,
                          &File,
                           LOG_DIRECTORY_NAME,
                           EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE,
                           EFI_FILE_DIRECTORY | EFI_FILE_HIDDEN);

    if (EFI_ERROR(Status)) {
        // Logs directory doesn't exist.  If NVME, allow forced logging
        if (!CheckIfNVME (LogDevice->Handle)) {
            DEBUG((DEBUG_INFO, "Logs directory not found on device.  No logging.\n"));
            Volume->Close (Volume);
            return NULL;
        }

        // Logs directory doesn't exist, see if we can create the Logs directory.
        if (!FeaturePcdGet(PcdAdvancedFileLoggerForceEnable)) {
            DEBUG((DEBUG_INFO, "Creating the Logs directory is not allowed.\n"));
            Volume->Close (Volume);
            return NULL;
        }

        Status = Volume->Open (Volume,
                              &File,
                               LOG_DIRECTORY_NAME,
                               EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE,
                               EFI_FILE_DIRECTORY | EFI_FILE_HIDDEN);
        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "Unable to create Log directory. Code=%r\n", Status));
            Volume->Close (Volume);
            return NULL;
        }
    }

    // Close directory
    File->Close(File);
    LogDevice->Volume = Volume;

    return Volume;
}

/**
  ValidateLogFile

  Validate the log file.  Delete it if it is invalid.

  File is flushed and closed upon exit.

  @param File          - Open File handle.
  @param ExpectedSize  - Expected size of existing log file

  @retval           EFI_SUCCESS
  @retval           <other>
  **/
STATIC
EFI_STATUS
ValidateLogFile (
    IN EFI_FILE    *File,
    IN UINT64       ExpectedSize
    )
{
    UINT64          FileSize;
    EFI_STATUS      Status;

    //
    // Validate the file size for the pre-allocated index file.
    //
    Status = File->SetPosition (File, MAX_UINT64);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "Error Setting to end of file. Code=%r\n", Status));
        goto CleanUp;
    }

    Status = File->GetPosition (File, &FileSize);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "Error getting file size. Code=%r\n", Status));
        goto CleanUp;
    }

    if (FileSize != ExpectedSize) {
        DEBUG((DEBUG_INFO,"Log File Size incorrect.  Deleting Index File\n"));
        File->Delete (File);
        File = NULL;              // Delete closes the file
        Status = EFI_NOT_FOUND;
    }

CleanUp:
    if (NULL != File) {
        File->Close (File);
    }

    return Status;
}

/**
  InitializeLogIndexFile

  Initialize the Index file.

  File is flushed and closed upon exit.

  @param File       Open File handle.

  @retval           EFI_SUCCESS
  @retval           <other>

  **/
STATIC
EFI_STATUS
InitializeLogIndexFile (
    IN EFI_FILE *File
    )
{
    UINT64          DataBufferSize;
    EFI_STATUS      Status;

    //
    // Initialize the contents of the Index file to log 0.
    //
    DataBufferSize = INDEX_FILE_SIZE;
    Status = File->Write (File,
                         &DataBufferSize,
                          INDEX_FILE_VALUE);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR,"%a: Failed to create index file. Code=%r \n", __FUNCTION__ , Status));
    } else {
        if  (DataBufferSize != INDEX_FILE_SIZE) {
            Status = EFI_ABORTED;
        }
    }

    File->Close (File);

    return Status;
}

/**
    WriteEndOfFileMarker - Construct the END_OF_LOG message

    @param File           - Open File handle.
    @param RoomLeft       - Space left in the log file

    @return EFI_STATUS
 **/
EFI_STATUS
WriteEndOfFileMarker (IN EFI_FILE *File,
                      IN UINTN     RoomLeft) {

    UINTN       DataBufferSize;
    CHAR8       EndOfLogMessage[64];
    UINTN       EndOfLogMessageLen;
    EFI_STATUS  Status;
    EFI_TIME    Time;

    Status = gRT->GetTime(&Time, NULL);
    if (EFI_ERROR(Status)) {
        ZeroMem (&Time, sizeof(Time));
    }

    EndOfLogMessageLen = AsciiSPrint (
                    EndOfLogMessage,
                    sizeof(EndOfLogMessage),
                    "\n\n === END_OF_LOG === @ === %4d/%02d/%02d %d:%02d:%02d ===\n\n",
                    (UINTN) Time.Year,
                    (UINTN) Time.Month,
                    (UINTN) Time.Day,
                    (UINTN) Time.Hour,
                    (UINTN) Time.Minute,
                    (UINTN) Time.Second);

    if (EndOfLogMessageLen > RoomLeft) {
        EndOfLogMessageLen = RoomLeft;
    }

    Status = EFI_SUCCESS;
    if (EndOfLogMessageLen > 0) {
        DataBufferSize = EndOfLogMessageLen;
        Status = File->Write (File,
                             &DataBufferSize,
                              EndOfLogMessage);
        if (!EFI_ERROR(Status)) {
            if (DataBufferSize != EndOfLogMessageLen) {
                DEBUG((DEBUG_ERROR, "Not all bytes of EOF written to log.\n"));
                Status = EFI_BAD_BUFFER_SIZE;
            }
        }
    }

    DEBUG((DEBUG_INFO, "End Of File written. Code=%r\n", Status));
    return Status;
};

/**
  InitializeLogFile

  @param File       Open File handle.
  @param DataBuffer Initialized buffer for an empty file.

                    Initialize the Index file.

                    File is flushed and closed upon exit.

  @retval           EFI_SUCCESS
  @retval           <other>

  **/
STATIC
EFI_STATUS
InitializeLogFile (
    IN EFI_FILE *File,
    IN CHAR8    *DataBuffer
    )
{
    UINT64          DataBufferSize;
    UINTN           i;
    UINT64          FileSize;
    EFI_STATUS      Status;

    //
    // Initialize the log file.
    //
    for (i = 0; i < DEBUG_LOG_FILE_SIZE; i += DEBUG_LOG_CHUNK_SIZE) {
        DataBufferSize = DEBUG_LOG_CHUNK_SIZE;
        Status = File->Write (File,
                             &DataBufferSize,
                              DataBuffer);
        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "Error writing chunk to log. Code=%r\n", Status));
            goto CleanUp;
        }

        if (DataBufferSize != DEBUG_LOG_CHUNK_SIZE) {
            DEBUG((DEBUG_ERROR, "Not all bytes of chunk written to log.\n"));
            Status = EFI_BAD_BUFFER_SIZE;
            goto CleanUp;
        }
    }

    Status = File->GetPosition (File, &FileSize);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "Error getting end of file. Code=%r\n", Status));
        goto CleanUp;
    }

    if (FileSize != DEBUG_LOG_FILE_SIZE) {
        DEBUG((DEBUG_ERROR, "File Size not as expected.\n"));
        Status = EFI_BAD_BUFFER_SIZE;
        goto CleanUp;
    }

    Status = File->SetPosition (File, 0);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "Error Setting to beginning of file. Code=%r\n", Status));
        goto CleanUp;
    }

    Status = WriteEndOfFileMarker (File, DEBUG_LOG_FILE_SIZE);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR,"%a: Failed to write end of file marker=%r \n", __FUNCTION__ , Status));
        goto CleanUp;
    }

    Status = File->SetPosition (File, FileSize);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "Error restoring end of file. Code=%r\n", Status));
    }

CleanUp:

    File->Close (File);

    return Status;
}

/**
  DetermineLogFile

  Read the index value from the log index file.

  @param   LogDevice        Which log device to write the log to

  @retval  EFI_SUCCESS      The LogData->FileIndex was updated
  @retval  other            An error occurred.  The log device was disabled

  **/
STATIC
EFI_STATUS
DetermineLogFile (
    IN LOG_DEVICE *LogDevice
    )
{
    UINTN           BufferSize;
    EFI_FILE       *File;
    CHAR8           FileIndex;
    EFI_STATUS      Status;
    EFI_FILE       *Volume;

    Volume = VolumeFromFileSystemHandle (LogDevice);
    if (NULL == Volume) {
        LogDevice->Valid = FALSE;
        return EFI_INVALID_PARAMETER;
    }

    //
    // Open Index File
    //
    Status = Volume->Open (Volume,
                          &File,
                           mLogFiles[0].LogFileName,
                           EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE,
                           0);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "%a: Unable to open log index file. Code = %r\n", __FUNCTION__, Status));
        LogDevice->Valid = FALSE;
        return Status;
    }

    //
    // Read the last index value.
    //
    BufferSize = sizeof(FileIndex);
    Status = File->Read(File,
                       &BufferSize,
                       (VOID*)&FileIndex);

    if (EFI_ERROR (Status) || (BufferSize != sizeof(FileIndex))) {
        DEBUG((DEBUG_ERROR, "%a: Failed to read the log file index. Using log 1. Code=%r\n", __FUNCTION__, Status));
        FileIndex = '0';
    }

    //
    // Increment the file index and roll-over at the maximum value.
    //

    if ((FileIndex < '1') || (FileIndex >= '9')) {
        FileIndex = '1';
    } else {
        FileIndex++;
    }

    //
    // Reposition the file index to the head of the index file.
    //
    Status = File->SetPosition(File, 0);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "%a: Failed to update debug log index file: %r !\n", __FUNCTION__ , Status));
    } else {
        //
        // Store the new index value.
        //
        BufferSize = sizeof(FileIndex);
        Status = File->Write(File, &BufferSize, (VOID*)&FileIndex);
        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, ": Failed to update debug log index file: %r !\n", __FUNCTION__, Status));
        }
    }

    //
    // Ignore the later errors and use log file 1 and hope for the best.
    //
    File->Close(File);
    LogDevice->FileIndex = FileIndex - '0';
    return EFI_SUCCESS;
}

/**
  WriteALogFIle

  Writes the currently unwritten part of the log file.

  @param   LogDevice        Which log device to write the log to

  @retval  EFI_SUCCESS      The log was updated
  @retval  other            An error occurred.  The log device was disabled

  **/
EFI_STATUS
WriteALogFile (
    IN LOG_DEVICE *LogDevice
    )
{
    EFI_FILE                             *File;
    UINTN                                 WriteSize;
    UINT64                                RoomLeft;
    EFI_STATUS                            Status;
    EFI_FILE                             *Volume;

    if (!LogDevice->Valid) {
        return EFI_DEVICE_ERROR;
    }

    File = NULL;
    Volume = VolumeFromFileSystemHandle (LogDevice);
    if (NULL == Volume) {
        Status = EFI_INVALID_PARAMETER;
        goto CloseAndExit;
    }

    if (LogDevice->FileIndex == 0) {
        Status = DetermineLogFile (LogDevice);
        if (EFI_ERROR(Status)) {
            goto CloseAndExit;
        }
    }

    //
    // Open selected log file
    //
    Status = Volume->Open (Volume,
                          &File,
                           mLogFiles[LogDevice->FileIndex].LogFileName,
                           EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE,
                           0);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "%a: Unable to open log file. Code = %r\n", __FUNCTION__, Status));
        goto CloseAndExit;
    }

    //
    // Reposition the log file to the current offset.
    //
    Status = File->SetPosition(File, LogDevice->CurrentOffset);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "%a: Failed to seek to current offset: %r !\n", __FUNCTION__ , Status));
        goto CloseAndExit;
    }

    RoomLeft = DEBUG_LOG_FILE_SIZE - LogDevice->CurrentOffset;
    Status = AdvancedLoggerAccessLibGetNextFormattedLine (&LogDevice->AccessEntry);

    while (Status == EFI_SUCCESS) {

        WriteSize = LogDevice->AccessEntry.MessageLen;
        if (WriteSize > RoomLeft) {
            WriteSize = RoomLeft;
            DEBUG((DEBUG_ERROR, "Log file truncated\n"));
        }

        if (WriteSize > 0) {
            Status = File->Write(File, &WriteSize, (VOID *) LogDevice->AccessEntry.Message);
            if (EFI_ERROR(Status)) {
                DEBUG((DEBUG_ERROR, "%a: Failed to write to log file: %r !\n", __FUNCTION__, Status));
                goto CloseAndExit;
            }

            LogDevice->CurrentOffset += WriteSize;
            RoomLeft -= WriteSize;
        }

        Status = AdvancedLoggerAccessLibGetNextFormattedLine (&LogDevice->AccessEntry);

    };

    if (Status == EFI_END_OF_FILE) {
        //
        // Write End Of Buffer file mark.
        //
        Status = WriteEndOfFileMarker (File, RoomLeft);
        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "%a: Failed to write end of file marker: %r !\n", __FUNCTION__, Status));
        }
    }

    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "%a: Failed to write to log file: %r !\n", __FUNCTION__, Status));
        goto CloseAndExit;
    }

CloseAndExit:

    if (EFI_ERROR(Status)) {
        LogDevice->Valid = FALSE;
    }

    if (File != NULL) {
        File->Close(File);
    }

    if (Volume != NULL) {
        LogDevice->Volume = NULL;
        Volume->Close (Volume);
    }

    return Status;
}

/**
  EnableLoggingOnThisDevice

  Create or validate the UEFI Log Files.  10 files are validated or created:

  UEFI_Index.txt
  UEFI_Log1.txt
  UEFI_Log2.txt
  UEFI_Log3.txt
  ...
  UEFI_Log9.txt

  Invalid UEFI log files are erased, and re-created.

  If any file operation fails, return an error.

  @param[in] FileSystem   Handle where FileSystemProtocol is installed

  @retval   EFI_SUCCESS   All four log files created

  **/
EFI_STATUS
EFIAPI
EnableLoggingOnThisDevice (
    IN LOG_DEVICE  *LogDevice
    )
{
    CHAR8          *DataBuffer;
    EFI_FILE       *File;
    UINTN           i;
    EFI_STATUS      Status;
    EFI_FILE       *Volume;

    File = NULL;
    Volume = NULL;
    DataBuffer = (CHAR8 *) AllocatePages (EFI_SIZE_TO_PAGES(DEBUG_LOG_CHUNK_SIZE));
    if (DataBuffer == NULL) {
        DEBUG((DEBUG_ERROR, "Unable to allocate working buffer\n"));
        return EFI_OUT_OF_RESOURCES;
    }

    Volume = VolumeFromFileSystemHandle (LogDevice);
    if (NULL == Volume) {
        FreePages (DataBuffer, EFI_SIZE_TO_PAGES(DEBUG_LOG_CHUNK_SIZE));
        return EFI_INVALID_PARAMETER;
    }

    SetMem (DataBuffer, DEBUG_LOG_CHUNK_SIZE, ' ');

    //
    // Break up the free space into lines.  Editors like VSCODE and SUBLIME get confused with a 4MB line
    //
    for (i=0; i < (DEBUG_LOG_CHUNK_SIZE - 1); i+=72) {
        DataBuffer[i]   = '\r';
        DataBuffer[i+1] = '\n';
    }

    Status = EFI_SUCCESS;
    for (i = 0; (i < DEBUG_LOG_FILE_COUNT) && (!EFI_ERROR(Status)); i++) {
        //
        // Check if file exists and is the correct size.
        //
        Status = Volume->Open (Volume,
                              &File,
                               mLogFiles[i].LogFileName,
                               EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE,
                               0);

        if (EFI_SUCCESS == Status) {
            if (i == 0) {
                Status = ValidateLogFile (File, INDEX_FILE_SIZE);
            } else {
                Status = ValidateLogFile (File, DEBUG_LOG_FILE_SIZE);
            }
        }

        if (EFI_NOT_FOUND == Status) {
            //
            // Create the file with RW permissions.
            //
            Status = Volume->Open (Volume,
                                  &File,
                                   mLogFiles[i].LogFileName,
                                   EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE,
                                   0);
            if (EFI_ERROR(Status)) {
                DEBUG((DEBUG_ERROR,"%a: Failed to create log file %s. Code=%r \n", __FUNCTION__ , mLogFiles[i].LogFileName, Status));
                goto ErrorExit;
            }

            if (i == 0) {
                Status = InitializeLogIndexFile (File);
            } else {
                Status = InitializeLogFile (File, DataBuffer);
            }

            DEBUG((DEBUG_INFO, "Debug file %s created, Code=%r\n", mLogFiles[i].LogFileName, Status));
        }
    }

ErrorExit:

    FreePages (DataBuffer, EFI_SIZE_TO_PAGES(DEBUG_LOG_CHUNK_SIZE));

    LogDevice->Volume = NULL;
    Volume->Close (Volume);

    return Status;
}
