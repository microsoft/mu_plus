/** @file
FileLogger.c

This application will prepare a device for UEFI loggong, or turn off UEFI
logging to a device.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DebugFileLogger.h"


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
  CheckIfUSB

  Checks if the device path is a USB device path.

  @param Handle     Handle with Device Path protocol

  @retval  TRUE     Device is USB or Unable to get Device Path
  @retval  FALSE    Device is NOT USB.

  **/
STATIC
BOOLEAN
CheckIfUSB (
    IN EFI_HANDLE Handle
    )
{
    EFI_DEVICE_PATH_PROTOCOL *DevicePath;

    DevicePath = DevicePathFromHandle (Handle);
    if (DevicePath == NULL) {
        return TRUE;
    }

    while (!IsDevicePathEnd(DevicePath)) {
        if ((MESSAGING_DEVICE_PATH == DevicePathType(DevicePath)) &&
            (MSG_USB_DP == DevicePathSubType(DevicePath)) ) {
            return TRUE;
        }
        DevicePath = NextDevicePathNode(DevicePath);
    }

    return FALSE;
}

/**
  VolumeFromFileSystemHandle

  Handle should have a FileSystemProtocol on it.  Get the FileSystemPRotocol
  and then get the VolumeHandle.  Make sure there is a Log directory.
  Create the log directory if not a USB device.

  @param FileSystemHandle   Handle with a File System Protocol installed

  @retval VolumeHandle      Handle to the Volume for I/O
  **/
STATIC
EFI_FILE *
VolumeFromFileSystemHandle (
    IN EFI_HANDLE FileSystemHandle
    )
{
    EFI_FILE                         *File;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem;
    EFI_STATUS                        Status;
    EFI_FILE                         *Volume;

    Status = gBS->OpenProtocol(FileSystemHandle,
                              &gEfiSimpleFileSystemProtocolGuid,
                               (VOID**)&FileSystem,
                               gImageHandle,
                               NULL,
                               EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

    if (EFI_ERROR(Status)) {
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
        // Log doesn't exist.  If USB, no logging available
        if (CheckIfUSB (FileSystemHandle)) {
            DEBUG((DEBUG_ERROR, "Logs directory not found on USB device.  No logging to USB\n"));
            return NULL;
        }

        Status = Volume->Open (Volume,
                              &File,
                               LOG_DIRECTORY_NAME,
                               EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE,
                               EFI_FILE_DIRECTORY | EFI_FILE_HIDDEN);
        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "Unable to create Log directory. Code=%r\n", Status));
            return NULL;
        }
    }

    // Close directory
    File->Close(File);

    return Volume;
}

/**
  ValidateLogFile

  Validate the log file.  Delete it if it is invalid.

  File is flushed and closed upon exit.

  @param File       Open File handle.

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
        DEBUG((DEBUG_ERROR,"Log File Size incorrect.  Deleting Index File\n"));
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

    File->Flush (File);
    File->Close (File);

    return Status;
}

/**
  InitializeLogIndexFile

  @param File       Open File handle.

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

    DataBufferSize = END_OF_FILE_MARKER_SIZE;
    Status = File->Write (File,
                         &DataBufferSize,
                          END_OF_FILE_MARKER);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR,"%a: Failed to write beginning of log=%r \n", __FUNCTION__ , Status));
        goto CleanUp;
    }

    if (DataBufferSize != END_OF_FILE_MARKER_SIZE) {
        DEBUG((DEBUG_ERROR, "Not all bytes of EOF written to log.\n"));
        Status = EFI_BAD_BUFFER_SIZE;
        goto CleanUp;
    }

    Status = File->SetPosition (File, FileSize);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "Error restoring end of file. Code=%r\n", Status));
    }

CleanUp:

    File->Flush (File);
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

    Volume = VolumeFromFileSystemHandle (LogDevice->Handle);
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
    switch (FileIndex) {
    case '0':              // 0 only occurs the first time the Index is read, or in an error case
        FileIndex = '1';
        break;
    case '1':
        FileIndex = '2';
        break;
    case '2':
        FileIndex = '3';
        break;
    case '3':
        FileIndex = '4';
        break;
    case '4':
        FileIndex = '5';
        break;
    case '5':
        FileIndex = '6';
        break;
    case '6':
        FileIndex = '7';
        break;
    case '7':
        FileIndex = '8';
        break;
    case '8':
        FileIndex = '9';
        break;
    case '9':
        FileIndex = '1';
        break;
    default:
        DEBUG((DEBUG_ERROR, "%a: Debug log file index appears to be corrupted, using log 1. Code=%r \n", __FUNCTION__, Status));
        FileIndex = '1';
        break;
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
  @param   LogBuffer        The log data to be written
  @param   LogBufferLength  The number of bytes to write to the log

  @retval  EFI_SUCCESS      The log was updated
  @retval  other            An error occurred.  The log device was disabled

  **/
EFI_STATUS
WriteALogFile (
    IN LOG_DEVICE *LogDevice,
    IN CHAR8      *LogBuffer,
    IN UINT64      LogBufferLength
    )
{
    CHAR8          *Buffer;
    UINT64          BufferSize;
    EFI_FILE       *File;
    UINT64          RoomLeft;
    EFI_STATUS      Status;
    EFI_FILE       *Volume;


    if (!LogDevice->Valid) {
        return EFI_DEVICE_ERROR;
    }

    Volume = VolumeFromFileSystemHandle (LogDevice->Handle);
    if (NULL == Volume) {
        return EFI_INVALID_PARAMETER;
    }

    if (LogDevice->FileIndex == 0) {
        Status = DetermineLogFile (LogDevice);
        if (EFI_ERROR(Status)) {
            LogDevice->Valid = FALSE;
            return Status;
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
        LogDevice->Valid = FALSE;
        return Status;
    }

    //
    // Reposition the log file to the current offset.
    //
    Status = File->SetPosition(File, LogDevice->CurrentOffset);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "%a: Failed to seek to current offset: %r !\n", __FUNCTION__ , Status));
        LogDevice->Valid = FALSE;
        File->Close(File);
        return Status;
    }

    //
    // Comput amount of data to write.  We cannot write more data
    // than the existing file size.  We do not want to alter the
    // metadata on the disk (new FAT entries etc).
    //
    if (LogBufferLength > DEBUG_LOG_FILE_SIZE) {
        LogBufferLength = DEBUG_LOG_FILE_SIZE;
    }

    RoomLeft = DEBUG_LOG_FILE_SIZE - LogDevice->CurrentOffset;
    BufferSize = LogBufferLength - LogDevice->CurrentOffset;

    if (BufferSize > RoomLeft) {
        BufferSize = RoomLeft;
        DEBUG((DEBUG_ERROR, "Log file truncated\n"));
    }

    RoomLeft -= BufferSize;

    if (BufferSize > 0) {
        Buffer = &LogBuffer[LogDevice->CurrentOffset];
        Status = File->Write(File, &BufferSize, (VOID*)Buffer);
        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "%a: Failed to write to log file: %r !\n", __FUNCTION__, Status));
            File->Close(File);
            LogDevice->Valid = FALSE;
            return Status;
        }
        LogDevice->CurrentOffset += BufferSize;
        //
        // Write End Of Buffer file mark.
        //
        BufferSize = END_OF_FILE_MARKER_SIZE;
        if (BufferSize > RoomLeft) {
            BufferSize = RoomLeft;
        }
        if (BufferSize > 0) {
            Status = File->Write(File, &BufferSize, (VOID*)END_OF_FILE_MARKER);
            if (EFI_ERROR(Status)) {
                DEBUG((DEBUG_ERROR, "%a: Failed to write end of buffer marker: %r !\n", __FUNCTION__, Status));
                File->Close(File);
                LogDevice->Valid = FALSE;
                return Status;
            }
        }
    }

    File->Close(File);

    return Status;
}

/**
  Create the UEFI Log Files.  10 files are validated or created:

  UEFI_Log_Index.txt
  UEFI_Log1.txt
  UEFI_Log2.txt
  UEFI_Log3.txt
  ...
  UEFI_Log9.txt

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
    EFI_FILE       *File   = NULL;
    UINTN           i;
    EFI_STATUS      Status;
    EFI_FILE       *Volume;

    DataBuffer = (CHAR8 *) AllocatePages (EFI_SIZE_TO_PAGES(DEBUG_LOG_CHUNK_SIZE));
    if (DataBuffer == NULL) {
        DEBUG((DEBUG_ERROR, "Unable to allocate working buffer\n"));
        return EFI_OUT_OF_RESOURCES;
    }

    Volume = VolumeFromFileSystemHandle (LogDevice->Handle);
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
                FreePool (DataBuffer);
                return Status;
            }

            if (i == 0) {
                Status = InitializeLogIndexFile (File);
            } else {
                Status = InitializeLogFile (File, DataBuffer);
            }

            DEBUG((DEBUG_INFO, "Debug file %s created, Code=%r\n", mLogFiles[i].LogFileName, Status));
        }

    }

    FreePages (DataBuffer, EFI_SIZE_TO_PAGES(DEBUG_LOG_CHUNK_SIZE));

    return Status;
}
