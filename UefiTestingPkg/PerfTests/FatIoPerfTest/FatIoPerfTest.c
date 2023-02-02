/** @file -- FatPerfTest.c

Do Fat read/write transfers and time them for performance evaluation.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Protocol/DevicePath.h>
#include <Protocol/SimpleFileSystem.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/ShellLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiBootManagerLib.h>
#include <Library/UefiLib.h>

#define ONE_MICROSECOND  (1000)
#define ONE_MILLISECOND  (1000 * ONE_MICROSECOND)
#define ONE_SECOND       (1000 * ONE_MILLISECOND)

#define GET_SECONDS(a)       (DivU64x32 ((a), ONE_SECOND))
#define GET_MILLISECONDS(a)  (DivU64x32 ((a), ONE_MILLISECOND))
#define GET_MICROSECONDS(a)  (DivU64x32 ((a), ONE_MICROSECOND))

//
// NOTE:
//
// The test assumes a media that can boot.  Therefore, the test looks for
// the boot file expected on that type of media.  The 1MB read file that
// is checked in with the source is required for the ReadBlob test.
// If a required to read file or sub directory are not present, the
// test is aborted.
//
#define BOOT_MANAGER_FILE_NAME  L"\\EFI\\Microsoft\\Boot\\bootmgfw.efi"
#define TEST_1_FILE             L"\\EFI\\Boot\\BootX64.efi"
#define TEST_2_FILE             L"\\PerfTest\\Test2File.txt"
#define TEST_3_FILE             L"\\PerfTest\\Test3File.txt"

#define ONE_MEGABYTE  (1024 * 1024)

/**
  PrintTimeFromNs  - Print the time with the input duration in Nanoseconds.

  @param  TimeInNs  - The raw performance counter data in nanoseconds.

**/
VOID
PrintTimeFromNs (
  IN UINT64  TimeInNs
  )
{
  UINT64  Sec           = 0;
  UINT64  Milli         = 0;
  UINT64  Micro         = 0;
  UINT64  Nano          = 0;
  UINT64  RemainingTime = TimeInNs;

  if (RemainingTime > ONE_SECOND) {
    Sec            = GET_SECONDS (RemainingTime);
    RemainingTime -= MultU64x32 (Sec, ONE_SECOND);
  }

  if (RemainingTime > ONE_MILLISECOND) {
    Milli          = GET_MILLISECONDS (RemainingTime);
    RemainingTime -= MultU64x32 (Milli, ONE_MILLISECOND);
  }

  if (RemainingTime > ONE_MICROSECOND) {
    Micro          = GET_MICROSECONDS (RemainingTime);
    RemainingTime -= MultU64x32 (Micro, ONE_MICROSECOND);
  }

  if (RemainingTime > 0) {
    Nano = RemainingTime;
  }

  if (Sec > 0) {
    Print (
      L"%d.%3.3d seconds\n",
      Sec,
      Milli
      );
  } else if (Milli > 0) {
    Print (
      L"%d.%3.3d milliseconds\n",
      Milli,
      Micro
      );
  } else if (Micro > 0) {
    Print (
      L"%d.%3.3d microseconds\n",
      Micro,
      Nano
      );
  } else {
    Print (
      L"%d nanoseconds\n",
      Nano
      );
  }
}

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
  IN EFI_HANDLE  Handle
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;

  DevicePath = DevicePathFromHandle (Handle);
  if (DevicePath == NULL) {
    return FALSE;
  }

  while (!IsDevicePathEnd (DevicePath)) {
    if ((MESSAGING_DEVICE_PATH == DevicePathType (DevicePath)) &&
        (MSG_NVME_NAMESPACE_DP == DevicePathSubType (DevicePath)))
    {
      return TRUE;
    }

    DevicePath = NextDevicePathNode (DevicePath);
  }

  return FALSE;
}

/**
  Test Read of data blob

  This is a basic test of reading a 1MB file.

  @param    Handle       - Handle where File System Protocol is installed
  @param    FileSystem   - File System Protocol

**/
VOID
TestReadDataBlob (
  IN EFI_HANDLE                       *Handle,
  IN EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem,
  IN CHAR16                           *FileName
  )
{
  UINT64         End;
  EFI_FILE       *File;
  UINT8          *FileBuffer;
  UINTN          FileBufferSize;
  EFI_FILE_INFO  *FileInfo;
  UINTN          FileInfoSize;
  UINT64         Start;
  EFI_STATUS     Status;
  EFI_FILE       *Volume;

  if (FileSystem == NULL) {
    Print (
      L"%a: SimpleFileSystem is NULL\n"
      __FUNCTION__
      );

    return;
  }

  End        = 0;
  File       = NULL;
  FileBuffer = NULL;
  FileInfo   = NULL;
  Volume     = NULL;

  Start = GetPerformanceCounter ();

  //
  // Open the volume.
  //
  Status = FileSystem->OpenVolume (FileSystem, &Volume);
  if (EFI_ERROR (Status)) {
    Print (
      L"%a: Failed to open volume. Code=%r \n",
      __FUNCTION__,
      Status
      );

    return;
  }

  Status = Volume->Open (
                     Volume,
                     &File,
                     FileName,
                     EFI_FILE_MODE_READ,
                     0
                     );

  if (EFI_ERROR (Status)) {
    Print (
      L"%a: Failed to open %s on this volume. Code=%r \n",
      __FUNCTION__,
      FileName,
      Status
      );

    goto ReadCloseAndExit;
  }

  FileInfoSize = 0;
  Status       = File->GetInfo (
                         File,
                         &gEfiFileInfoGuid,
                         &FileInfoSize,
                         FileInfo
                         );

  if (Status != EFI_BUFFER_TOO_SMALL) {
    Print (
      L"%a: Unexpected return code from GetInfo. Code = %r\n",
      __FUNCTION__,
      Status
      );

    goto ReadCloseAndExit;
  }

  FileInfo = (EFI_FILE_INFO *)AllocatePool (FileInfoSize);
  if (FileInfo == NULL) {
    Print (
      L"%a: Failed to allocate a buffer for FileInfo\n",
      __FUNCTION__
      );

    goto ReadCloseAndExit;
  }

  Status = File->GetInfo (
                   File,
                   &gEfiFileInfoGuid,
                   &FileInfoSize,
                   FileInfo
                   );

  if (EFI_ERROR (Status)) {
    Print (
      L"%a: Failed to get the %s file information. SizeOf(EFI_FILE_INFO) is %d SizeRequired is %d. Code=%r \n",
      __FUNCTION__,
      FileName,
      sizeof (EFI_FILE_INFO),
      FileInfoSize,
      Status
      );

    goto ReadCloseAndExit;
  }

  FileBuffer = (UINT8 *)AllocatePages (EFI_SIZE_TO_PAGES ((UINTN)FileInfo->FileSize));

  if (FileBuffer == NULL) {
    Print (
      L"%a: Failed to allocate a buffer for the file.\n",
      __FUNCTION__
      );

    goto ReadCloseAndExit;
  }

  FileBufferSize = (UINTN)FileInfo->FileSize;
  Status         = File->Read (
                           File,
                           &FileBufferSize,
                           FileBuffer
                           );

  if (EFI_ERROR (Status)) {
    Print (
      L"%a: Failed to read file %s. Code=%r \n",
      __FUNCTION__,
      FileName,
      Status
      );

    goto ReadCloseAndExit;
  }

  End = GetPerformanceCounter ();

  Print (
    L"%a: Time to open and load %s is ",
    __FUNCTION__,
    FileName
    );

  PrintTimeFromNs (GetTimeInNanoSecond (End-Start));

ReadCloseAndExit:

  Start = GetPerformanceCounter ();

  if (FileInfo != NULL) {
    if (FileBuffer != NULL) {
      FreePages (FileBuffer, EFI_SIZE_TO_PAGES ((UINTN)FileInfo->FileSize));
    }

    FreePool (FileInfo);
  }

  if (File != NULL) {
    File->Close (File);
  }

  if (Volume != NULL) {
    Volume->Close (Volume);
  }

  //
  // If there was a printed measurement for time to read, then print
  // the time to close.
  //
  if (End != 0) {
    End = GetPerformanceCounter ();

    Print (
      L"%a: Time to close after reading %s is ",
      __FUNCTION__,
      FileName
      );

    PrintTimeFromNs (GetTimeInNanoSecond (End-Start));
  }
}

/**
  Test writing a 1MB blob of data

  This is a basic test of what Advanced Logger does at Exit Boot Services.

  @param    Handle       - Handle where File System Protocol is installed
  @param    FileSystem   - File System Protocol

**/
VOID
TestWriteDataBlob (
  IN EFI_HANDLE                       *Handle,
  IN EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem,
  IN CHAR16                           *FileName
  )
{
  UINT64      End;
  EFI_FILE    *File;
  UINT64      Start;
  EFI_STATUS  Status;
  UINTN       TotalSize;
  EFI_FILE    *Volume;
  CHAR8       WriteBuffer[256];
  UINTN       WriteSize;

  if (FileSystem == NULL) {
    Print (
      L"%a: SimpleFileSystem is NULL\n",
      __FUNCTION__
      );

    return;
  }

  Volume = NULL;
  File   = NULL;
  End    = 0;

  //
  // Open the volume.
  //
  Status = FileSystem->OpenVolume (FileSystem, &Volume);
  if (EFI_ERROR (Status)) {
    Print (
      L"%a: Failed to open volume. Code=%r \n",
      __FUNCTION__,
      Status
      );

    return;
  }

  Start = GetPerformanceCounter ();

  Status = Volume->Open (
                     Volume,
                     &File,
                     FileName,
                     EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE,
                     0
                     );

  if (EFI_ERROR (Status)) {
    Print (
      L"%a: Failed to create %s on this volume. Code=%r \n",
      __FUNCTION__,
      FileName,
      Status
      );

    goto WriteCloseAndExit;
  }

  TotalSize = 0;

  while (TotalSize < ONE_MEGABYTE) {
    WriteSize = AsciiSPrint (
                  WriteBuffer,
                  sizeof (WriteBuffer),
                  "This is a data line that is roughly 130 bytes.  The size so far is %d, for a total of about 1MB of data.  Thank you for reading\n",
                  TotalSize
                  );

    if (WriteSize == 0) {
      Print (
        L"%a: Write Size cannot be zero.  Test failed\n"
        __FUNCTION__
        );

      goto WriteCloseAndExit;
    }

    Status = File->Write (
                     File,
                     &WriteSize,
                     (VOID *)WriteBuffer
                     );
    if (EFI_ERROR (Status)) {
      Print (
        L"%a: Failed to write data line to test output file: Code=%r\n",
        __FUNCTION__,
        Status
        );

      goto WriteCloseAndExit;
    }

    TotalSize += WriteSize;
  }

  End = GetPerformanceCounter ();

  Print (
    L"%a: Time to write %s is ",
    __FUNCTION__,
    FileName
    );

  PrintTimeFromNs (GetTimeInNanoSecond (End-Start));

WriteCloseAndExit:

  Start = GetPerformanceCounter ();

  if (File != NULL) {
    File->Close (File);
  }

  if (Volume != NULL) {
    Volume->Close (Volume);
  }

  //
  // If there was a printed measurement for time to write, then print
  // the time to close.
  //
  if (End != 0) {
    End = GetPerformanceCounter ();

    Print (
      L"%a: Time to close %s is ",
      __FUNCTION__,
      FileName
      );

    PrintTimeFromNs (GetTimeInNanoSecond (End-Start));
  }
}

/**
  TestSimpleFileSystem

  This runs the tests against a SimpleFileSystem protocol

  @param    Handle       - Handle where File System Protocol is installed
  @param    FileSystem   - File System Protocol

**/
VOID
TestSimpleFileSystem (
  IN EFI_HANDLE                       *Handle,
  IN EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem
  )
{
  CHAR16  *FileName;

  if (CheckIfNVME (Handle)) {
    FileName = BOOT_MANAGER_FILE_NAME;
  } else {
    FileName = TEST_1_FILE;
  }

  //
  // Test 1  - Test Read Time of bootmgfw
  //
  Print (
    L"\n%a: Test 1 - Reading %s\n",
    __FUNCTION__,
    FileName
    );

  TestReadDataBlob (Handle, FileSystem, FileName);

  //
  // Test 2  - Test writing a 1MB file to this file system
  //
  Print (
    L"\n%a: Test 2 - Writing test Data to %s\n",
    __FUNCTION__,
    TEST_2_FILE
    );

  TestWriteDataBlob (Handle, FileSystem, TEST_2_FILE);

  //
  // Test 3  - Test reading a 1MB file from this file system.
  //
  Print (
    L"\n%a: Test 3 - Reading test Data from %s\n",
    __FUNCTION__,
    TEST_3_FILE
    );

  TestReadDataBlob (Handle, FileSystem, TEST_3_FILE);
}

/**
  Test entry point.

  @param[in]  ImageHandle     The image handle.
  @param[in]  SystemTable     The system table.

  @retval EFI_SUCCESS            Command completed successfully.

**/
EFI_STATUS
EFIAPI
TestMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  UINT64                           Start;
  UINT64                           End;
  EFI_STATUS                       Status;
  UINTN                            PreviousSimpleFileSystemHandleCount;
  UINTN                            SimpleFileSystemHandleCount;
  EFI_HANDLE                       *SimpleFileSystemBuffer = NULL;
  UINTN                            Index;
  EFI_DEVICE_PATH_PROTOCOL         *SimpleFileSystemDevicePath = NULL;
  CHAR16                           *DevicePathString           = NULL;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *SimpleFileSystemProtocol   = NULL;

  //
  // Initialize the shell lib (we must be in non-auto-init...)
  //  NOTE: This may not be necessary, but whatever.
  //
  Status = ShellInitialize ();
  if (EFI_ERROR (Status)) {
    Print (
      L"%a: Failed to initialize the Shell. %r\n",
      __FUNCTION__,
      Status
      );

    return Status;
  }

  // locate all handles with Simple File System protocol
  Status = gBS->LocateHandleBuffer (ByProtocol, &gEfiSimpleFileSystemProtocolGuid, NULL, &SimpleFileSystemHandleCount, &SimpleFileSystemBuffer);
  if (EFI_ERROR (Status) || (SimpleFileSystemHandleCount == 0) || (SimpleFileSystemBuffer == NULL)) {
    //
    // If there was an error or there are no device handles that support
    // the BLOCK_IO Protocol, then return.
    //
    Print (
      L"%a: No Simple File System protocols in this system\n",
      __FUNCTION__
      );

    return EFI_SUCCESS;
  }

  PreviousSimpleFileSystemHandleCount = SimpleFileSystemHandleCount;

  Print (
    L"%a: Found %d Simple File System handles on first look\n",
    __FUNCTION__,
    SimpleFileSystemHandleCount
    );

  //
  // Disconnect all controllers with Simple File System protocols
  //
  // This is done to unload all of the Simple File System protocol to insure
  // there is no left over cache data.

  //
  // Loop through all the device handles that support the SIMPLE_FILE_SYSTEM Protocol
  // and disconnect all of the controller.
  //
  for (Index = 0; Index < SimpleFileSystemHandleCount; Index++) {
    Status = gBS->DisconnectController (SimpleFileSystemBuffer[Index], NULL, NULL);
    if (EFI_ERROR (Status)) {
      Print (
        L"%a: Error disconnecting controller %p. Code=%r\n",
        __FUNCTION__,
        SimpleFileSystemBuffer[Index],
        Status
        );
    }
  } // end for loop

  FreePool (SimpleFileSystemBuffer);
  SimpleFileSystemBuffer = NULL;

  SimpleFileSystemHandleCount = 0;
  // locate all handles with Simple File System protocol - there should be none.
  Status = gBS->LocateHandleBuffer (ByProtocol, &gEfiSimpleFileSystemProtocolGuid, NULL, &SimpleFileSystemHandleCount, &SimpleFileSystemBuffer);
  if ((Status != EFI_NOT_FOUND) || (SimpleFileSystemHandleCount != 0) || (SimpleFileSystemBuffer != NULL)) {
    //
    // If there was an error or there are no Simple File System handles, return.
    //
    Print (
      L"%a: There should be no Simple File System protocols at this time. Count=%d, Buffer=%p, Code=%r\n",
      __FUNCTION__,
      SimpleFileSystemHandleCount,
      SimpleFileSystemBuffer,
      Status
      );

    if (SimpleFileSystemBuffer != NULL) {
      FreePool (SimpleFileSystemBuffer);
    }

    return EFI_SUCCESS;
  }

  //
  //  Reconnect all devices, and Simple File Handles should be the same.
  //
  Print (
    L"%a: Starting ConnectAll()\n",
    __FUNCTION__
    );

  Start = GetPerformanceCounter ();
  EfiBootManagerConnectAll ();
  End = GetPerformanceCounter ();

  Print (
    L"%a: Time to perform Connect All is ",
    __FUNCTION__
    );

  PrintTimeFromNs (GetTimeInNanoSecond (End-Start));

  //
  // Now, look for the SimpleFileSystem protocols again after ensuring the caches have been discarded.
  //
  Status = gBS->LocateHandleBuffer (ByProtocol, &gEfiSimpleFileSystemProtocolGuid, NULL, &SimpleFileSystemHandleCount, &SimpleFileSystemBuffer);
  if (EFI_ERROR (Status) || (SimpleFileSystemHandleCount == 0) || (SimpleFileSystemBuffer == NULL)) {
    //
    // If there was an error or there are no Simple File System handles, return
    //
    Print (
      L"%a: No Simple File System protocols in this system\n"
      __FUNCTION__
      );

    return EFI_SUCCESS;
  }

  if (PreviousSimpleFileSystemHandleCount != SimpleFileSystemHandleCount) {
    Print (
      L"%a: Incorrect number of File System Protocols between before and after DisconnectControllers\n"
      __FUNCTION__
      );

    Print (
      L"%a: First time = %d, second time = %d\n",
      __FUNCTION__,
      PreviousSimpleFileSystemHandleCount,
      SimpleFileSystemHandleCount
      );
  }

  Print (
    L"%a: Continuing with %d SimpleFileSystem protocols\n",
    __FUNCTION__,
    SimpleFileSystemHandleCount
    );

  //
  // Loop through all the device handles that support the SIMPLE_FILE_SYSTEM Protocol
  //
  for (Index = 0; Index < SimpleFileSystemHandleCount; Index++) {
    Status = gBS->HandleProtocol (SimpleFileSystemBuffer[Index], &gEfiDevicePathProtocolGuid, (VOID *)&SimpleFileSystemDevicePath);
    if (EFI_ERROR (Status) || (SimpleFileSystemDevicePath == NULL)) {
      Print (
        L"%a: No Device Path Protocol for this SimpleFileSystem Protocol\n"
        __FUNCTION__
        );
    } else {
      DevicePathString = ConvertDevicePathToText (SimpleFileSystemDevicePath, TRUE, FALSE);
      if (DevicePathString != NULL) {
        Print (
          L"%a: DevicePath is %s\n",
          __FUNCTION__,
          DevicePathString
          );

        FreePool (DevicePathString);
        DevicePathString = NULL;
      } else {
        Print (
          L"%a: DevicePath to text was NULL\n"
          __FUNCTION__
          );
      }
    }

    Status = gBS->HandleProtocol (SimpleFileSystemBuffer[Index], &gEfiSimpleFileSystemProtocolGuid, (VOID *)&SimpleFileSystemProtocol);
    if (EFI_ERROR (Status) || (SimpleFileSystemProtocol == NULL)) {
      Print (
        L"%a: Getting SimpleFileSystemProtocol failed.  Code=%r.  Can't test this one\n"
        __FUNCTION__
        );
      continue;
    }

    TestSimpleFileSystem (SimpleFileSystemBuffer[Index], SimpleFileSystemProtocol);
    Print (L"\n");  // Add a blank line to the test output
  } // end for loop

  if (SimpleFileSystemBuffer != NULL) {
    gBS->FreePool (SimpleFileSystemBuffer);
  }

  return Status;
}
