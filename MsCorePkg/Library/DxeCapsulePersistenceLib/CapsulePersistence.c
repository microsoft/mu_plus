/**
  Implementation of persisting Capsules across reset.

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi/UefiBaseType.h>
#include <Uefi/UefiSpec.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiLib.h>
#include <Library/PrintLib.h>
#include <Library/BaseCryptLib.h>
#include <Library/DevicePathLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CapsulePersistenceLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Guid/FileInfo.h>
#include <Guid/FileSystemInfo.h>

#include "CapsulePersistence.h"

#define CAPSULE_DIR               L"Capsules"
#define CAPSULE_DEFAULT_FILENAME  L"capsule00000.bin"
#define CAPSULE_ID_MODULO         100000// because we have five digits

/**
  Gets the SFS protocol handle for the the first disk that has a GPT partition.

  @param[in]  SfsProtocol            A pointer to the protocol

  @retval     EFI_SUCCESS            Everything worked great and protocol is valid
  @retval     EFI_INVALID_PARAMETER  SfsProtocol is NULL
  @retval     EFI_NOT_FOUND          We didn't find a proper protocol handle to use
  @retval     Others                 Something went wrong.

*/
EFI_STATUS
UefiGetSfsProtocolHandle (
  IN  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  **SfsProtocol
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  BOOLEAN                   Found;
  EFI_HANDLE                Handle;
  EFI_HANDLE                *HandleBuffer;
  UINTN                     Index;
  UINTN                     NumHandles;
  EFI_STRING                PathNameStr;
  EFI_GUID                  *DummyInterface;
  EFI_STATUS                Status;

  if (SfsProtocol == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status       = EFI_SUCCESS;
  NumHandles   = 0;
  HandleBuffer = NULL;

  // Locate all handles that are using the SFS protocol.
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiSimpleFileSystemProtocolGuid,
                  NULL,
                  &NumHandles,
                  &HandleBuffer
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: failed to locate all handles using the Simple FS protocol (%r)\n", __FUNCTION__, Status));
    goto CleanUp;
  }

  //
  // Search the handles to find one that is on a GPT partition on a hard drive.
  //
  Found = FALSE;
  for (Index = 0; (Index < NumHandles) && (Found == FALSE); Index += 1) {
    Handle = HandleBuffer[Index];

    Status = gBS->HandleProtocol (Handle, &gEfiPartTypeSystemPartGuid, (VOID **)&DummyInterface);
    if (!EFI_ERROR (Status)) {
      Found = TRUE;

      DevicePath  = DevicePathFromHandle (Handle);
      PathNameStr = ConvertDevicePathToText (DevicePath, TRUE, TRUE);
      DEBUG ((DEBUG_VERBOSE, "%a: found ESP device path %d -> %s\n", __FUNCTION__, Index, PathNameStr));

      break;
    }
  }

  // If a suitable handle was not found, return error.
  if (Found == FALSE) {
    Status = EFI_NOT_FOUND;
    DEBUG ((DEBUG_ERROR, "%a: failed to locate a handle with a GPT handle out of %d handles from the SFS protocol\n", __FUNCTION__, NumHandles));
    goto CleanUp;
  }

  Status = gBS->HandleProtocol (
                  HandleBuffer[Index],
                  &gEfiSimpleFileSystemProtocolGuid,
                  (VOID **)SfsProtocol
                  );

  if (EFI_ERROR (Status) != FALSE) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to locate Simple FS protocol using the handle to fs0: %r \n", __FUNCTION__, Status));
    goto CleanUp;
  }

CleanUp:
  DEBUG ((DEBUG_VERBOSE, "%a: status %r\n", __FUNCTION__, Status));
  if (HandleBuffer != NULL) {
    FreePool (HandleBuffer);
  }

  return Status;
}

/**
  Opens an SFS volume and if successful, returns a FS handle to the opened volume.

  @param[out] FileSystemHandle  Handle to the opened volume.

  @retval     EFI_SUCCESS       The FS volume was opened successfully.
  @retval     Others            The operation failed.

**/
EFI_STATUS
STATIC
OpenVolumeSFS (
  OUT EFI_FILE  **FileSystemHandle
  )
{
  EFI_STATUS                       Status;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *SfsProtocol;

  if (FileSystemHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Find the handle
  Status = UefiGetSfsProtocolHandle (&SfsProtocol);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to find Simple Filesystem Protocol: %r \n", __FUNCTION__, Status));
  }

  // Open the volume/partition.
  Status = SfsProtocol->OpenVolume (SfsProtocol, FileSystemHandle);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to open Simple FS volume fs0: %r \n", __FUNCTION__, Status));
  }

  return Status;
}

/**
  Checks whether the required amount of space exists on the filesystem.

  @param[in]  FileSystemHandle  Root handle to SFS.
  @param[in]  SpaceRequired     Amount of space needed

  @retval     EFI_SUCCESS       Enough space exists.
  @retval     EFI_VOLUME_FULL   Not enough space exists.
  @retval     Other             Something went wrong during check for space.

**/
EFI_STATUS
STATIC
IsThereEnoughFreeSpaceOnDisk (
  IN EFI_FILE  *FileSystemHandle,
  IN UINTN     SpaceRequired
  )
{
  EFI_STATUS            Status;
  EFI_FILE_SYSTEM_INFO  *FsInfo = NULL;
  UINTN                 FsInfoSize;

  if (FileSystemHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  FsInfoSize = 0;
  Status     = FileSystemHandle->GetInfo (FileSystemHandle, &gEfiFileSystemInfoGuid, &FsInfoSize, FsInfo);
  if (Status != EFI_BUFFER_TOO_SMALL) {
    ASSERT (Status == EFI_BUFFER_TOO_SMALL);
    Status = EFI_DEVICE_ERROR;
    goto Cleanup;
  }

  FsInfo = AllocatePool (FsInfoSize);
  if (FsInfo == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Cleanup;
  }

  Status = FileSystemHandle->GetInfo (FileSystemHandle, &gEfiFileSystemInfoGuid, &FsInfoSize, FsInfo);
  if (EFI_ERROR (Status)) {
    goto Cleanup;
  }

  DEBUG ((DEBUG_VERBOSE, "%a: Free Space 0x%x bytes. Required 0x%x bytes\n", __FUNCTION__, FsInfo->FreeSpace, SpaceRequired));
  if (FsInfo->FreeSpace < SpaceRequired) {
    DEBUG ((DEBUG_WARN, "[%a] Attempting to persist a capsule, but not enough space on EFI system partition.\n", __FUNCTION__));
    Status = EFI_VOLUME_FULL;
  }

Cleanup:
  if (FsInfo != NULL) {
    FreePool (FsInfo);
  }

  return Status;
}

/**
  Checks whether the required amount of space exists on the filesystem.

  @param[in]  FileSystemHandle  Root handle to SFS.
  @param[in]  SpaceRequired     Amount of space needed
  @param[in]  CreateIfNotExists If true, directory will be created if it isn't present

  @retval     EFI_SUCCESS       Enough space exists.
  @retval     EFI_VOLUME_FULL   Not enough space exists.
  @retval     Other             Something went wrong during check for space.

**/
EFI_STATUS
STATIC
OpenCapsulesDirectory (
  IN EFI_FILE  *FileSystemHandle,
  IN EFI_FILE  **DirHandle,
  IN BOOLEAN   CreateIfNotExists
  )
{
  EFI_STATUS     Status;
  CHAR16         CapsuleDir[] = CAPSULE_DIR;
  UINT64         OpenMode;
  EFI_FILE_INFO  *FileInfo = NULL;
  UINTN          FileInfoSize;

  FileInfo = NULL;
  if ((FileSystemHandle == NULL) || (DirHandle == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  OpenMode = EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE;

  // otherwise continue on
  if (CreateIfNotExists) {
    OpenMode |= EFI_FILE_MODE_CREATE;
  }

  // Open Capsule directory, if it exists.
  Status = FileSystemHandle->Open (
                               FileSystemHandle,
                               DirHandle,
                               CapsuleDir,
                               OpenMode,
                               EFI_FILE_DIRECTORY
                               );
  if (EFI_ERROR (Status)) {
    goto Cleanup;
  }

  // Allocate space to hold the file info for the directory.
  FileInfoSize = SIZE_OF_EFI_FILE_INFO + sizeof (CapsuleDir);
  FileInfo     = AllocatePool (FileInfoSize);
  if (FileInfo == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Cleanup;
  }

  Status = (*DirHandle)->GetInfo (*DirHandle, &gEfiFileInfoGuid, &FileInfoSize, FileInfo);
  if (EFI_ERROR (Status)) {
    goto Cleanup;
  }

  // Validate that it is actually a directory.
  if ((FileInfo->Attribute & EFI_FILE_DIRECTORY) == 0) {
    // file opened is not a directory as expected.
    DEBUG ((DEBUG_INFO, "%a: Capsules is a file, not a directory, deleting\n", __FUNCTION__));
    Status = (*DirHandle)->Delete (*DirHandle); // delete implies close
    if (EFI_ERROR (Status)) {
      goto Cleanup;
    }

    // Call the Open Capsule Directory again once the file is deleted
    Status = OpenCapsulesDirectory (FileSystemHandle, DirHandle, CreateIfNotExists);
  }

Cleanup:

  if (FileInfo != NULL) {
    FreePool (FileInfo);
  }

  return Status;
}

/**
  Removes the Capsules folder and its contents.

  @param[in]  FileSystemHandle  Root handle to SFS.

  @retval     EFI_SUCCESS       Stale capsules removed.
  @retval     Other             Something went wrong during capsule removal.

**/
EFI_STATUS
STATIC
RemoveStaleCapsulesOnFileSystem (
  IN EFI_FILE  *FileSystemHandle
  )
{
  EFI_STATUS     Status;
  EFI_FILE       *DirHandle = NULL;
  EFI_FILE_INFO  *FileInfo  = NULL;
  EFI_FILE       *File;
  UINTN          FileInfoSize;
  UINTN          AllocatedFileInfoSize;

  if (FileSystemHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = OpenCapsulesDirectory (FileSystemHandle, &DirHandle, FALSE);
  if (Status == EFI_NOT_FOUND) {
    DEBUG ((DEBUG_INFO, "%a:%d - Capsules directory doesn't exist\n", __FUNCTION__, __LINE__));
    Status = EFI_SUCCESS; // Great - it already doesn't exist. Success.
    goto Cleanup;
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[%a] - Failed to Open the Capsules Directory.\n", __FUNCTION__));
    goto Cleanup;
  }

  AllocatedFileInfoSize = 0;
  // Iterate through the FileInfos in the capsule directory handle and delete them.
  do {
    FileInfoSize = AllocatedFileInfoSize;
    Status       = DirHandle->Read (DirHandle, &FileInfoSize, FileInfo);
    if (EFI_ERROR (Status)) {
      if (Status == EFI_BUFFER_TOO_SMALL) {
        // Re-allocate FileInfo to be be big enough and try again.
        if (FileInfo != NULL) {
          FreePool (FileInfo);
        }

        FileInfo = AllocatePool (FileInfoSize);
        if (FileInfo == NULL) {
          Status = EFI_OUT_OF_RESOURCES;
          DEBUG ((DEBUG_ERROR, "[%a] - Failed to allocate memory.\n", __FUNCTION__));
          goto Cleanup;
        }

        AllocatedFileInfoSize = FileInfoSize;
        continue;
      }

      // other errors, bail.
      goto Cleanup;
    }

    if ((FileInfo->Attribute & EFI_FILE_DIRECTORY) != 0) {
      continue; // Skip directories.
    }

    if (FileInfoSize != 0) {
      // Open the file so we can delete it.
      Status = DirHandle->Open (
                            DirHandle,
                            &File,
                            FileInfo->FileName,
                            EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE,
                            0
                            );
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "[%a] - Failed to open the dir handle.\n", __FUNCTION__));
        goto Cleanup;
      }

      Status = File->Delete (File);
      DEBUG ((DEBUG_WARN, "[%a] - deleting stale capsule: %s\n", __FUNCTION__, FileInfo->FileName));
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "[%a] - Failed to delete the directory.\n", __FUNCTION__));
        goto Cleanup;
      }
    }
  } while (FileInfoSize != 0);

  // Now all the files are gone, delete the directory itself
  Status = DirHandle->Delete (DirHandle);
  DEBUG ((DEBUG_WARN, "[%a] - deleting capsule directory\n", __FUNCTION__));

Cleanup:

  if (FileInfo != NULL) {
    FreePool (FileInfo);
  }

  return Status;
}

/**
  Writes a filename for a given capsule to the pointer

  @param[in]    CapsuleId       Capsule ID number
  @param[out]   Filename        Name of file created
  @param[in]    FilenameSize    Size of buffer

  @retval       EFI_SUCCESS     Filename was generated
  @retval       Others          Filename was not generated
*/
EFI_STATUS
STATIC
GenerateFileName (
  IN UINTN    CapsuleId,
  OUT CHAR16  *Filename,
  IN UINTN    FilenameSize
  )
{
  if (Filename == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (FilenameSize < sizeof (CAPSULE_DEFAULT_FILENAME)) {
    return EFI_BUFFER_TOO_SMALL;
  }

  if (CapsuleId >= CAPSULE_ID_MODULO) {
    return EFI_INVALID_PARAMETER;
  }

  UnicodeSPrint (Filename, FilenameSize, L"capsule%d.bin", CapsuleId);
  return EFI_SUCCESS;
}

/**
  Generate Capsule ID to be used on the disk.

  @param[in]  FileSystemHandle        Root handle to SFS.
  @param[out] CapsuleId               The next free capsule ID

  @retval     EFI_SUCCESS             CapsuleId is valid
  @retval     EFI_INVALID_PARAMETER   FileSystemHandle or CapsuleId are NULL
  @retval     EFI_OUT_OF_RESOURCES    Not enough space in the capsules directory
*/
EFI_STATUS
STATIC
FindNextFreeCapsuleID (
  IN  EFI_FILE  *FileSystemHandle,
  OUT UINT32    *CapsuleId
  )
{
  STATIC UINT32  CapsuleNum = 1;
  EFI_STATUS     Status;
  UINT32         CapsuleNumberModulo = CAPSULE_ID_MODULO;
  CHAR16         Filename[]          = CAPSULE_DEFAULT_FILENAME;
  UINTN          FilenameSize;
  EFI_FILE       *DirHandle;
  EFI_FILE       *File;
  UINTN          Attempts;

  if ((FileSystemHandle == NULL) || (CapsuleId == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  FilenameSize = sizeof (Filename);
  File         = NULL;
  DirHandle    = NULL;

  // Open Capsule directory
  Status = OpenCapsulesDirectory (FileSystemHandle, &DirHandle, FALSE);
  if (Status == EFI_NOT_FOUND) {
    // if we didn't find the capsule directory, we can just the number currently in CapsuleNum
    *CapsuleId = (CapsuleNum) % CapsuleNumberModulo;
    return EFI_SUCCESS;
  }

  if (EFI_ERROR (Status)) {
    goto Cleanup;
  }

  Attempts = 0;
  // create the number that our capsule will be
  while (TRUE) {
    Attempts += 1; // increment the number of attempts
    // generate a new capsule id and filename
    CapsuleNum = (CapsuleNum + Attempts) % CapsuleNumberModulo;
    Status     = GenerateFileName (CapsuleNum, Filename, FilenameSize);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "[%a] - failed to generate a filename: %r\n", __FUNCTION__, Status));
      goto Cleanup;
    }

    // check if this particular file already exists
    Status = DirHandle->Open (DirHandle, &File, Filename, EFI_FILE_MODE_READ, 0);
    if (Status == EFI_NOT_FOUND) {
      // if we didn't find it, success
      Status = EFI_SUCCESS;
      break;
    }

    // otherwise close it
    File->Close (File);
    File = NULL;
    // make sure we don't loop forever
    if (Attempts > 500) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Cleanup;
    }
  }

  *CapsuleId = CapsuleNum;

Cleanup:

  if (DirHandle != NULL) {
    DirHandle->Close (DirHandle);
  }

  if (File != NULL) {
    File->Close (File);
  }

  return Status;
}

/**
  Create a file to be used to persist a capsule.

  @param[in]        FileSystemHandle  Root handle to SFS.
  @param[out]       File              Pointer to file handle for newly created file.
  @param[in]        CapsuleId         The ID of the capsule

  @retval           EFI_SUCCESS       Successfully determined filename.
  @retval           Other             Failed to create new file

**/
EFI_STATUS
STATIC
CreateCapsuleFileOnFileSystem (
  IN EFI_FILE   *FileSystemHandle,
  OUT EFI_FILE  **File,
  IN UINT32     CapsuleId
  )
{
  EFI_STATUS  Status;
  CHAR16      Filename[sizeof (CAPSULE_DEFAULT_FILENAME)];
  UINTN       FilenameSize;
  EFI_FILE    *DirHandle;

  if ((FileSystemHandle == NULL) || (File == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  FilenameSize = sizeof (Filename);
  DirHandle    = NULL;

  // Open Capsule directory, creating it if it doesn't exist.
  Status = OpenCapsulesDirectory (FileSystemHandle, &DirHandle, TRUE);
  if (EFI_ERROR (Status)) {
    goto Cleanup;
  }

  Status = GenerateFileName (CapsuleId, Filename, FilenameSize);
  if (EFI_ERROR (Status)) {
    goto Cleanup;
  }

  DEBUG ((DEBUG_VERBOSE, "[%a] - Saving capsule to %s\n", __FUNCTION__, Filename));

  Status = DirHandle->Open (
                        DirHandle,
                        File,
                        Filename,
                        EFI_FILE_MODE_READ,
                        0
                        );
  if (!EFI_ERROR (Status)) {
    // if the file already exists, something has gone wrong
    DEBUG ((DEBUG_ERROR, "[%a] - The capsule file already exists: %a\n", __FUNCTION__, Filename));
    (*File)->Close (*File);
    ASSERT_EFI_ERROR (Status);
    return EFI_ALREADY_STARTED;
  }

  // create the file we want to use
  Status = DirHandle->Open (
                        DirHandle,
                        File,
                        Filename,
                        EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE,
                        0
                        );
  if (EFI_ERROR (Status)) {
    goto Cleanup;
  }

Cleanup:

  if (DirHandle != NULL) {
    DirHandle->Close (DirHandle);
  }

  return Status;
}

/**
  Given a capsule, calculate a hash of it

  @param[in]    CapsuleHeader         Points to the capsule
  @param[out]   CapsuleHash           The hash of the capsule passed in

  @retval       EFI_SUCCESS           Hash was calculated successfully
  @retval       EFI_INVALID_PARAMETER CapsuleHash or CapsuleHeader was null
  @retval       EFI_OUT_OF_RESOURCES  Couldn't allocate memory for the hash
  @retval       EFI_DEVICE_ERROR      Something went wrong hashing the capsule
*/
EFI_STATUS
STATIC
CalculateCapsuleHash (
  IN  EFI_CAPSULE_HEADER  *CapsuleHeader,
  OUT UINT64              *CapsuleHash
  )
{
  VOID        *HashContext;
  BOOLEAN     HashStatus;
  EFI_STATUS  Status;
  UINT8       Digest[SHA256_DIGEST_SIZE];

  if (CapsuleHeader == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (CapsuleHash == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // use SHA256 to hash the context
  HashContext = AllocateZeroPool (Sha256GetContextSize ());
  if (HashContext == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = EFI_SUCCESS;
  // start by initializing the context
  ZeroMem (Digest, SHA256_DIGEST_SIZE);
  HashStatus = Sha256Init (HashContext);
  if (!HashStatus) {
    Status = EFI_DEVICE_ERROR;
    goto Cleanup;
  }

  HashStatus = Sha256Update (HashContext, (VOID *)CapsuleHeader, CapsuleHeader->CapsuleImageSize);
  if (!HashStatus) {
    Status = EFI_DEVICE_ERROR;
    goto Cleanup;
  }

  HashStatus = Sha256Final (HashContext, Digest);
  if (!HashStatus) {
    Status = EFI_DEVICE_ERROR;
    goto Cleanup;
  }

  *CapsuleHash = ((UINT64 *)Digest)[0]; // grab the top 64 bits of the digest

Cleanup:
  if (HashContext != NULL) {
    FreePool (HashContext);
    HashContext = NULL;
  }

  return Status;
}

/**
  Open a capsule file on the disk and get the file info
  Caller will need to free OutFileInfo if a non-null pointer is passed in.
  If a null is passed for OutFileInfo, the file info will not be read.

  @param[in]   FileSystemHandle   Root handle to SFS.
  @param[out]  File               Pointer to file handle for newly created file.
  @param[in]   CapsuleId          The ID of the capsule being read in.
  @param[in]   OutFileInfo        Pointer to the info for the capsule file (OPTIONAL)

  @retval      EFI_SUCCESS        Successfully opened file.
  @retval      Other              Failed to open file

**/
EFI_STATUS
OpenCapsuleFileOnFileSystem (
  IN  EFI_FILE       *FileSystemHandle,
  OUT EFI_FILE       **FileHandle,
  IN  UINT32         CapsuleId,
  IN  EFI_FILE_INFO  **OutFileInfo OPTIONAL
  )
{
  EFI_STATUS     Status;
  CHAR16         Filename[] = CAPSULE_DEFAULT_FILENAME;
  UINTN          FilenameSize;
  EFI_FILE       *DirHandle;
  EFI_FILE_INFO  *FileInfo;
  UINTN          FileInfoSize;

  DEBUG ((DEBUG_INFO, "%a: Start\n", __FUNCTION__));

  if ((FileSystemHandle == NULL) || (FileHandle == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  FilenameSize = sizeof (Filename);
  DirHandle    = NULL;
  FileInfo     = NULL;
  FileInfoSize = 0;

  // Open Capsule directory, creating it if it doesn't exist.
  Status = OpenCapsulesDirectory (FileSystemHandle, &DirHandle, FALSE);
  if (EFI_ERROR (Status)) {
    goto Cleanup;
  }

  // get the file name
  Status = GenerateFileName (CapsuleId, Filename, FilenameSize);
  if (EFI_ERROR (Status)) {
    goto Cleanup;
  }

  // open the file
  Status = DirHandle->Open (
                        DirHandle,
                        FileHandle,
                        Filename,
                        EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE,
                        0
                        );
  if (EFI_ERROR (Status)) {
    goto Cleanup;
  }

  // if user didn't give us a fileinfo pointer, don't request it
  if (OutFileInfo == NULL) {
    DEBUG ((DEBUG_INFO, "%a - skipping reading in file info\n", __FUNCTION__));
    goto Cleanup;
  }

  // Attempt to read in the file info
  Status = (*FileHandle)->GetInfo (*FileHandle, &gEfiFileInfoGuid, &FileInfoSize, FileInfo);
  // we expect our buffer to be too small
  if (Status == EFI_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "%a We had an unexpected status while getting the info: %r.\n", __FUNCTION__, Status));
    Status = EFI_DEVICE_ERROR;
    goto Cleanup;
  } else if (Status != EFI_BUFFER_TOO_SMALL) {
    DEBUG ((DEBUG_ERROR, "%a We had an unexpected status while getting the info: %r.\n", __FUNCTION__, Status));
    goto Cleanup;
  }

  // Check to make sure we actually got something
  if (FileInfoSize == 0) {
    Status = EFI_NOT_FOUND;
    goto Cleanup;
  }

  // reallocate and try again
  FileInfo = AllocatePool (FileInfoSize);
  if (FileInfo == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Cleanup;
  }

  Status = (*FileHandle)->GetInfo (*FileHandle, &gEfiFileInfoGuid, &FileInfoSize, FileInfo);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a failed to read file info: %r.\n", __FUNCTION__, Status));
    goto Cleanup;
  }

  *OutFileInfo = FileInfo; // make sure to set the file info to go out

Cleanup:
  DEBUG ((DEBUG_INFO, "%a: exit status %r\n", __FUNCTION__, Status));

  if (DirHandle != NULL) {
    DirHandle->Close (DirHandle);
  }

  return Status;
}

/**
  This persists a capsule across reset but does not add it to the queue
  It is saved to the disk.

  @param[in]  CapsuleHeader         Points to a capsule header.
  @param[out] CapsuleIdentifier     The ID and hash of the capsule (optional)

  @retval     EFI_SUCCESS           Capsule successfully persisted to disk
  @retval     EFI_UNSUPPORTED       Capsule image is not supported by the firmware.
  @retval     EFI_INVALID_PARAMETER CapsuleHeader was null
  @retval     EFI_NOT_FOUND         Disk was not found or other reason
  @retval     EFI_DEVICE_ERROR      Something went wrong persisting the capsule
  @retval     Other                 Something else went wrong persisting the capsule
**/
EFI_STATUS
InternalPersistCapsuleImageAcrossReset (
  IN  EFI_CAPSULE_HEADER            *CapsuleHeader,
  OUT CAPSULE_PERSISTED_IDENTIFIER  *CapsuleIdentifier OPTIONAL
  )
{
  EFI_STATUS  Status;
  EFI_FILE    *FileSystemHandle;
  EFI_FILE    *FileHandle;
  UINTN       CapsuleSize;
  UINT64      CapsuleHash;
  UINT32      CapsuleId;

  if (CapsuleHeader == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  FileSystemHandle = NULL;
  FileHandle       = NULL;
  DEBUG ((DEBUG_INFO, "%a:%d - \n", __FUNCTION__, __LINE__));
  // Open the file system if it isn't already opened
  Status = OpenVolumeSFS (&FileSystemHandle);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[%a] - Failed to open the filesystem to persist the capsule\n", __FUNCTION__));
    goto Cleanup;
  }

  DEBUG ((DEBUG_INFO, "%a:%d - \n", __FUNCTION__, __LINE__));
  // Check to make sure we have enough free space
  CapsuleSize = CapsuleHeader->CapsuleImageSize;
  Status      = IsThereEnoughFreeSpaceOnDisk (FileSystemHandle, CapsuleSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[%a] - Not enough free space on the target partition\n", __FUNCTION__));
    goto Cleanup;
  }

  DEBUG ((DEBUG_INFO, "%a:%d - \n", __FUNCTION__, __LINE__));
  // get the id for the capsule
  Status = FindNextFreeCapsuleID (FileSystemHandle, &CapsuleId);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[%a] - Couldn't find next ready ID\n", __FUNCTION__));
    goto Cleanup;
  }

  DEBUG ((DEBUG_INFO, "%a:%d - \n", __FUNCTION__, __LINE__));
  // Get the hash of the Capsule to save into the system
  Status = CalculateCapsuleHash (CapsuleHeader, &CapsuleHash);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[%a] - Failed to calculate hash of capsule\n", __FUNCTION__));
    goto Cleanup;
  }

  DEBUG ((DEBUG_INFO, "%a:%d - \n", __FUNCTION__, __LINE__));
  // Create the file on the disk to store the capsule
  Status = CreateCapsuleFileOnFileSystem (FileSystemHandle, &FileHandle, CapsuleId);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[%a] - Failed to create a capsule file\n", __FUNCTION__));
    goto Cleanup;
  }

  DEBUG ((DEBUG_INFO, "%a:%d - \n", __FUNCTION__, __LINE__));
  // Write the capsule to the disk with the file handle we created
  Status = FileHandle->Write (FileHandle, &CapsuleSize, CapsuleHeader);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[%a] - Failed to write the capsule file to disk\n", __FUNCTION__));
    goto Cleanup;
  }

  DEBUG ((DEBUG_INFO, "%a:%d - \n", __FUNCTION__, __LINE__));
  if (CapsuleIdentifier != NULL) {
    CapsuleIdentifier->CapsuleHash = CapsuleHash;
    CapsuleIdentifier->CapsuleId   = CapsuleId;
  }

Cleanup:
  DEBUG ((DEBUG_INFO, "%a:%d - \n", __FUNCTION__, __LINE__));
  if (FileHandle != NULL) {
    FileHandle->Close (FileHandle);
  }

  if (FileSystemHandle != NULL) {
    FileSystemHandle->Close (FileSystemHandle);
  }

  return Status;
}

/**
  Allows the system to request a capsule by ID from the persisted medium

  If the persisted capsule by a given ID is not present, the pointer data will not be changed.
  The persisted capsule will not be cleared from the disk until explicitly told to do so.

  IMPORTANT: Caller is responsible for allocating/freeing the returned data (CapsuleData)

  @param[in]    CapsuleId             The ID of the capsule that is going to be loaded
  @param[in]    CapsuleHash           The hash of the capsule this is going to be loaded
                                      This will be verified when the capsule is loaded
  @param[out]   CapsuleData           A double pointer that will point to the capsule
                                      This value will not be changed on non EFI_SUCCESS
                                      return values (OPTIONAL).
  @param[out]   CapsuleDataSize       The size of the capsule data buffer

  @retval       EFI_SUCCESS           Capsules were de-persisted, and output data is valid.
  @retval       EFI_INVALID_PARAMETER CapsuleData was NULL or the hash didn't match
  @retval       EFI_OUT_OF_RESOURCES  Failed to allocate a buffer
  @retval       EFI_BUFFER_TOO_SMALL  Buffer is too small
  @retval       EFI_NOT_FOUND         The capsule file was not found
  @retval       EFI_DEVICE_ERROR      Something went wrong while trying to retrieve the capsule.

**/
EFI_STATUS
InternalGetPersistedCapsuleData (
  IN UINT32               CapsuleId,
  IN UINT64               CapsuleHash,
  OUT EFI_CAPSULE_HEADER  *CapsuleData OPTIONAL,
  OUT UINTN               *CapsuleDataSize
  )
{
  EFI_STATUS     Status;
  UINTN          CapsuleSize;
  UINT64         CalculatedCapsuleHash;
  EFI_FILE       *FileSystemHandle;
  EFI_FILE       *File;
  EFI_FILE_INFO  *FileInfo;

  DEBUG ((DEBUG_INFO, "%a: start\n", __FUNCTION__));

  if (CapsuleDataSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  FileInfo         = NULL;
  File             = NULL;
  FileSystemHandle = NULL;
  CapsuleSize      = 0;

  // Open the file system if it isn't already opened
  Status = OpenVolumeSFS (&FileSystemHandle);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[%a] - Failed to open the filesystem to read the capsule\n", __FUNCTION__));
    goto Cleanup;
  }

  // open the capsule file
  Status = OpenCapsuleFileOnFileSystem (FileSystemHandle, &File, CapsuleId, &FileInfo);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[%a] - Failed to open the capsule file requested\n", __FUNCTION__));
    goto Cleanup;
  }

  if (FileInfo == NULL) {
    DEBUG ((DEBUG_ERROR, "[%a] - Failed to get file info\n", __FUNCTION__));
    Status = EFI_NOT_FOUND;
    goto Cleanup;
  }

  // look at the size of the capsule we need
  CapsuleSize = (UINTN)FileInfo->FileSize;
  if (CapsuleSize > (*CapsuleDataSize)) {
    Status = EFI_BUFFER_TOO_SMALL;
    goto Cleanup;
  }

  // Make sure they passed in a parameter
  if (CapsuleData == NULL) {
    DEBUG ((DEBUG_ERROR, "[%a] - NULL CapsuleData\n", __FUNCTION__));
    Status = EFI_INVALID_PARAMETER;
    goto Cleanup;
  }

  CapsuleSize = *CapsuleDataSize;

  // read in the file from the disk
  Status = File->Read (File, &CapsuleSize, CapsuleData);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[%a] - Failed to read capsule file\n", __FUNCTION__));
    goto Cleanup;
  }

  // Check to make sure the file we read in matches what we expect
  if (CapsuleSize != CapsuleData->CapsuleImageSize) {
    DEBUG ((DEBUG_ERROR, "[%a] - File loaded is not the correct size. Expected %x Got %x\n", __FUNCTION__, CapsuleData->CapsuleImageSize, CapsuleSize));
    return EFI_DEVICE_ERROR;
  }

  // calculate the hash of the capsule we just loaded from disk
  Status = CalculateCapsuleHash (CapsuleData, &CalculatedCapsuleHash);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[%a] - Failed to calculate hash for capsule\n", __FUNCTION__));
    goto Cleanup;
  }

  // make sure the capsule hash we calculated matches what we expect
  if (CalculatedCapsuleHash != CapsuleHash) {
    Status = EFI_INVALID_PARAMETER;
    DEBUG ((DEBUG_ERROR, "[%a] - Capsule hash(0x%lx) did not match what we expected(0x%lx)\n", __FUNCTION__, CalculatedCapsuleHash, CapsuleHash));
    goto Cleanup;
  }

Cleanup:
  DEBUG ((DEBUG_INFO, "%a- exit status %r\n", __FUNCTION__, Status));

  *CapsuleDataSize = CapsuleSize;

  if (FileInfo != NULL) {
    FreePool (FileInfo);
  }

  if (File != NULL) {
    File->Close (File);
  }

  if (FileSystemHandle != NULL) {
    FileSystemHandle->Close (FileSystemHandle);
  }

  return Status;
}

/**
  Deletes the data for a given capsule.

  @param[in]  CapsuleId        The ID of the capsule that is going to be loaded

  @retval     EFI_SUCCESS      Capsules were de-persisted, and output data is valid.
  @retval     EFI_NOT_FOUND    The capsule file was not found
  @retval     EFI_DEVICE_ERROR Something went wrong while trying to retrieve the capsule.

**/
EFI_STATUS
InternalDeletePersistedCapsuleData (
  IN UINT32  CapsuleId
  )
{
  EFI_STATUS  Status;
  EFI_FILE    *FileSystemHandle = NULL;
  EFI_FILE    *File             = NULL;

  DEBUG ((DEBUG_INFO, "%a: start\n", __FUNCTION__));

  // Open the file system if it isn't already opened
  Status = OpenVolumeSFS (&FileSystemHandle);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[%a] Failed to open the filesystem to read the capsule\n", __FUNCTION__));
    goto Cleanup;
  }

  // open the capsule file
  File   = NULL;
  Status = OpenCapsuleFileOnFileSystem (FileSystemHandle, &File, CapsuleId, NULL);
  if (Status == EFI_NOT_FOUND) {
    // the file has already been deleted?
    Status = EFI_SUCCESS;
    goto Cleanup;
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[%a] Failed to open the capsule file requested\n", __FUNCTION__));
    goto Cleanup;
  }

  // delete the file
  Status = File->Delete (File);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[%a] Failed to delete the capsule file requested\n", __FUNCTION__));
    goto Cleanup;
  }

Cleanup:
  DEBUG ((DEBUG_INFO, "[%a] status %r\n", __FUNCTION__, Status));

  if (FileSystemHandle != NULL) {
    FileSystemHandle->Close (FileSystemHandle);
  }

  return Status;
}

/**
  Removes the Capsules folder and its contents.

  @retval     EFI_SUCCESS       Stale capsules removed.
  @retval     Other             Something went wrong during capsule removal.

**/
EFI_STATUS
InternalDeleteAllCapsulesOnFileSystem (
  VOID
  )
{
  EFI_STATUS  Status;
  EFI_FILE    *FileSystemHandle = NULL;

  // Open the file system if it isn't already opened
  Status = OpenVolumeSFS (&FileSystemHandle);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[%a] Failed to open the filesystem\n", __FUNCTION__));
    goto Cleanup;
  }

  // Delete all the capsules
  Status = RemoveStaleCapsulesOnFileSystem (FileSystemHandle);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[%a] Failed to delete the capsule files\n", __FUNCTION__));
    goto Cleanup;
  }

Cleanup:
  DEBUG ((DEBUG_INFO, "[%a] status %r\n", __FUNCTION__, Status));

  if (FileSystemHandle != NULL) {
    FileSystemHandle->Close (FileSystemHandle);
  }

  return Status;
}
