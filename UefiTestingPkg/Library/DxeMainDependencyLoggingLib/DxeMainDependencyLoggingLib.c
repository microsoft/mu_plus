/** @file -- DxeMainDependencyLoggingLib.c

This library and toolset are used with the Core DXE dispatcher to log all DXE drivers' protocol usage and
dependency expression implementation during boot.

See the readme.md file for full information

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <PiDxe.h>

#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiLib.h>
#include "DxeMainDependencyLoggingLib.h"

//
// WARNING - This library is tightly coupled to the MdeModulePkg Core DXE driver
//
#include "../Core/Dxe/DxeMain.h"
extern EFI_BOOT_SERVICES  mBootServices;
extern LIST_ENTRY         mDiscoveredList;

// Flag to halt logging of protocol usage once ready-to-boot is called
BOOLEAN  gLoggingEnabled = FALSE;

// Function pointer for the hooked EFI_BOOT_SERVICES::LocateProtocol function
EFI_LOCATE_PROTOCOL  HookedLocateProtocol = NULL;

// Linked list of used protocols and their originating memory address found during boot
LIST_ENTRY  mProtocolUsageLL = INITIALIZE_LIST_HEAD_VARIABLE (mProtocolUsageLL);

// Variable service name and namespace GUID for publishing the logging data
EFI_GUID  mVsNamespaceGuid = {
  0x4d2A2AEB, 0x9138, 0x44FB, { 0xB6, 0x44, 0x22, 0x17, 0x5F, 0xBB, 0xB0, 0x85 }
};
CHAR16    *mVsNameString = L"DEPEX_LOG_v1";

/**
  Builds a string based on the AsciiSPrint style input, prints it to the UEFI log, and concatenates
  it to the string in MsgBuffer->String, reallocating the buffer if needed.

  @param  MsgBuffer      Context information about the allocated buffer for storing all concatinated strings
  @param  FormatString   See MdePkg PrintLib::AsciiSPrint() for exact usage
**/
VOID
MessageAscii (
  MESSAGE_BUFFER  *MsgBuffer,
  CONST CHAR8     *FormatString,
  ...
  )
{
  static BOOLEAN  NewLine = TRUE;
  VA_LIST         Marker;
  UINTN           UsedSize;
  UINTN           OutSize;
  UINTN           FreeSize;

  // Check the amount of free space still left in the buffer
  UsedSize = (UINTN)(MsgBuffer->CatPtr) - (UINTN)(MsgBuffer->String);
  FreeSize = MsgBuffer->BufferSize - UsedSize;

  // Realloc the buffer if insufficient room for MESSAGE_ASCII_MAX_STRING_SIZE
  if (FreeSize < MESSAGE_ASCII_MAX_STRING_SIZE) {
    MsgBuffer->String      = (CHAR8 *)ReallocatePool (MsgBuffer->BufferSize, MsgBuffer->BufferSize + MESSAGE_BUFFER_REALLOC_CHUNK_SZ, MsgBuffer->String);
    MsgBuffer->CatPtr      = &(MsgBuffer->String[UsedSize]);
    MsgBuffer->BufferSize += MESSAGE_BUFFER_REALLOC_CHUNK_SZ;
    FreeSize              += MESSAGE_BUFFER_REALLOC_CHUNK_SZ;
  }

  // Format the string into the output buffer contatinating the previous string.
  VA_START (Marker, FormatString);
  OutSize = AsciiVSPrint (MsgBuffer->CatPtr, FreeSize, FormatString, Marker);
  VA_END (Marker);

  // Debug print the string, inserting a tab if it is a new line to help distinguish it from normal UEFI logs
  if (NewLine) {
    DEBUG ((LOGGING_DEBUG_LEVEL, "    "));
    NewLine = FALSE;
  }

  DEBUG ((LOGGING_DEBUG_LEVEL, MsgBuffer->CatPtr));

  // If this line ends in '\n', the next call will be a new line
  if ((OutSize > 0) && (MsgBuffer->CatPtr[OutSize - 1] == '\n')) {
    NewLine = TRUE;
  }

  // Reposition the CatPtr to the end NULL
  MsgBuffer->CatPtr = &(MsgBuffer->CatPtr[OutSize]);
}

/**
  Pulls the driver's name from the core DXE driver private data and sends it to the AsciiMessage function.  If an
  error occurs attempting to get the name, the driver's GUID name is used.

  @param  MsgBuffer         Structure context of the allocated buffer for storing all concatinated strings
  @param  ImagePrivateData  Pointer to the DXE core driver private information structure
  @param  DriverGuidName    Pointer to the driver's GUID name to use if an error occurs
**/
VOID
EFIAPI
MessageName (
  MESSAGE_BUFFER             *MsgBuffer,
  LOADED_IMAGE_PRIVATE_DATA  *ImagePrivateData,
  EFI_GUID                   *GuidName
  )
{
  CHAR8  *Name;
  CHAR8  *Ptr;

  // If NULL, use the driver entry GUID name
  if ((ImagePrivateData == NULL) || (ImagePrivateData->ImageContext.PdbPointer == NULL)) {
    MessageAscii (MsgBuffer, "%g", GuidName);
    return;
  }

  // The Pdb string is a path, move the pointer to the file name
  Name = (CHAR8 *)ImagePrivateData->ImageContext.PdbPointer;
  Ptr  = Name;
  while (*Ptr != 0) {
    if ((*Ptr == '/') || (*Ptr == '\\')) {
      Name = &(Ptr[1]);
    }

    Ptr++;
  }

  // If name length is zero, use the driver entry GUID name
  if (*Name == 0) {
    MessageAscii (MsgBuffer, "%g", GuidName);
  } else {
    MessageAscii (MsgBuffer, Name);
  }
}

/**
  Reads the dependency expression using the firmware volume protocol associated with the driver and sends it as an
  ASCII string to the AsciiMessage function

  @param  MsgBuffer    Structure context of the allocated buffer for storing all concatinated strings
  @param  DriverEntry  Pointer to the DXE core driver information structure
**/
VOID
EFIAPI
MessageDepex (
  MESSAGE_BUFFER         *MsgBuffer,
  EFI_CORE_DRIVER_ENTRY  *DriverEntry
  )
{
  EFI_STATUS  Status;
  UINT32      AuthStatus;
  UINT8       *Depex    = NULL;
  UINTN       DepexSize = 0;
  UINTN       x;

  // Call the FV protocol with NULL pointer to force it to allocate
  Status = DriverEntry->Fv->ReadSection (
                              DriverEntry->Fv,
                              &(DriverEntry->FileName),
                              EFI_SECTION_DXE_DEPEX,
                              0,
                              (VOID **)&Depex,
                              &DepexSize,
                              &AuthStatus
                              );

  // On error, skip writing the data
  if (!EFI_ERROR (Status)) {
    // Write depex as a series of hex bytes
    for (x = 0; x < DepexSize; x++) {
      MessageAscii (MsgBuffer, "%02X", Depex[x]);
    }

    // Free memory provided by the ReadSection call
    FreePool (Depex);
  }
}

/**
  Searches the protocol usage linked list for any LocateProtocol calls that originated from the driver represented by
  the private image data.  Any found are sent to the AsciiMessage function and removed from the linked list.

  @param  MsgBuffer         Structure context of the allocated buffer for storing all concatinated strings
  @param  ImagePrivateData  Pointer to the DXE core driver private information structure
**/
VOID
EFIAPI
MessageProtocols (
  MESSAGE_BUFFER             *MsgBuffer,
  LOADED_IMAGE_PRIVATE_DATA  *ImagePrivateData
  )
{
  LIST_ENTRY               *ListPtr;
  DL_PROTOCOL_USAGE_ENTRY  *Entry;
  BOOLEAN                  FirstEntry = TRUE;

  UINTN  StartAddress = (UINTN)ImagePrivateData->ImageBasePage;
  UINTN  EndAddress   = (ImagePrivateData->NumberOfPages * EFI_PAGE_SIZE) + StartAddress;

  // Loop through protocol usage linked list
  ListPtr = GetFirstNode (&mProtocolUsageLL);
  while (ListPtr != &mProtocolUsageLL) {
    Entry = LIST_ENTRY_PTR_TO_USAGE_ENTRY_PTR (ListPtr);

    // If not in this driver's memory range, move to next entry and loop
    if ((Entry->GuidAddress < StartAddress) || (Entry->GuidAddress >= EndAddress)) {
      ListPtr = GetNextNode (&mProtocolUsageLL, ListPtr);
      continue;
    }

    // Save this entry's GUID to the log
    if (FirstEntry) {
      MessageAscii (MsgBuffer, "%g", &(Entry->GuidName));
      FirstEntry = FALSE;
    } else {
      MessageAscii (MsgBuffer, ".%g", &(Entry->GuidName));
    }

    // Remove and free this entry, set EntryPtr to the next one to examine
    ListPtr = GetNextNode (&mProtocolUsageLL, ListPtr);
    RemoveEntryList (&(Entry->ListEntry));
    FreePool (Entry);
  }
}

/**
  Callback executed at ready-to-boot to collect boot data and provide to user in the UEFI debug log and through a
  volatile variable service value.

  Start tag:    "DEPEX_LOG_v1_BEGIN\n"
  End tag:      "DEPEX_LOG_v1_END\n"
  Line format:  "<name>|<depex>|<guid_1>.<guid_2>.< ... >.<guid_n>\n"
    <name>  = Name of the driver in ASCII format
    <depex> = Dependency expression data in hex "AABBCC..." format
    <guid>  = Protocols used by this driver in UEFI format "aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee"

  @param   Event    Used to close the callback event once called
  @param   Context  Unused

  @return  None
**/
VOID
EFIAPI
DepexDataRtbCallback (
  EFI_EVENT  Event,
  VOID       *Context
  )
{
  DL_PROTOCOL_USAGE_ENTRY    *Entry;
  LOADED_IMAGE_PRIVATE_DATA  *ImagePrivateData;
  EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage;
  EFI_CORE_DRIVER_ENTRY      *DriverEntry;
  MESSAGE_BUFFER             MsgBuffer;
  LIST_ENTRY                 *Link;
  EFI_STATUS                 Status;

  // Close event and halt logging through the hooked LocateProtocol call
  gBS->CloseEvent (Event);
  gLoggingEnabled = FALSE;

  // Setup a buffer to store all output data
  MsgBuffer.String     = (CHAR8 *)AllocatePool (MESSAGE_BUFFER_REALLOC_CHUNK_SZ);
  MsgBuffer.String[0]  = 0x00;
  MsgBuffer.CatPtr     = MsgBuffer.String;
  MsgBuffer.BufferSize = MESSAGE_BUFFER_REALLOC_CHUNK_SZ;

  // Start the output logging
  DEBUG ((DEBUG_INFO, "[%a] Dependency logging data:\n", DEBUG_TAG));
  MessageAscii (&MsgBuffer, DEPEX_LOG_BEGIN);

  // Loop through the discovered driver list
  for (Link = mDiscoveredList.ForwardLink; Link != &mDiscoveredList; Link = Link->ForwardLink) {
    // Pointer to the driver entry structure
    DriverEntry = CR (Link, EFI_CORE_DRIVER_ENTRY, Link, EFI_CORE_DRIVER_ENTRY_SIGNATURE);
    if (DriverEntry == NULL) {
      continue;
    }

    // Pointer to the private data structures
    Status = CoreHandleProtocol (DriverEntry->ImageHandle, &gEfiLoadedImageProtocolGuid, (VOID **)&LoadedImage);
    if (EFI_ERROR (Status) || (LoadedImage == NULL)) {
      continue;
    }

    ImagePrivateData = LOADED_IMAGE_PRIVATE_DATA_FROM_THIS (LoadedImage);

    // Create the log line "<name>|<depex>|<guid>.<guid>.<...>.<guid(n)>"
    MessageName (&MsgBuffer, ImagePrivateData, &(DriverEntry->FileName));
    MessageAscii (&MsgBuffer, "|");
    MessageDepex (&MsgBuffer, DriverEntry);
    MessageAscii (&MsgBuffer, "|");
    MessageProtocols (&MsgBuffer, ImagePrivateData);
    MessageAscii (&MsgBuffer, "\n");
  }

  // End loging
  MessageAscii (&MsgBuffer, DEPEX_LOG_END);

  // Save the collected messages to Variable Services
  Status = gRT->SetVariable (
                  mVsNameString,
                  &mVsNamespaceGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                  (UINTN)MsgBuffer.CatPtr - (UINTN)MsgBuffer.String,
                  MsgBuffer.String
                  );
  DEBUG ((DEBUG_INFO, "[%a] Saving to Variable Services:\n", DEBUG_TAG));
  DEBUG ((DEBUG_INFO, "    Name:         \"%S\"\n", mVsNameString));
  DEBUG ((DEBUG_INFO, "    Namespace:    %g\n", &mVsNamespaceGuid));
  DEBUG ((DEBUG_INFO, "    Attributes:   EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS\n"));
  DEBUG ((DEBUG_INFO, "    Size:         %d Bytes\n", (UINTN)(MsgBuffer.CatPtr) - (UINTN)(MsgBuffer.String)));
  DEBUG ((DEBUG_INFO, "    Write Status: %r\n", Status));

  // Free the message buffer
  FreePool (MsgBuffer.String);
  ZeroMem (&MsgBuffer, sizeof (MsgBuffer));

  // Free linked list of protocols and warn user of any not used in the above logging
  if (!IsListEmpty (&mProtocolUsageLL)) {
    DEBUG ((DEBUG_INFO, "[%a] NOTE:  These protocols were used by a driver hosting the GUID at an unrecognized memory address:\n", DEBUG_TAG));
    while (!IsListEmpty (&mProtocolUsageLL)) {
      Entry = LIST_ENTRY_PTR_TO_USAGE_ENTRY_PTR (GetFirstNode (&mProtocolUsageLL));
      DEBUG ((DEBUG_INFO, "    (%g) @ [%p]\n", &(Entry->GuidName), (VOID *)(Entry->GuidAddress)));
      RemoveEntryList (&(Entry->ListEntry));
      FreePool (Entry);
    }
  }
}

/**
  Monitors all calls to LocateProtol and records the GUID used along with the memory location where the GUID was
  declared to later determine which driver made the LocateProtocol call

  For parameter and return values, see MdePkg/Include/Uefi/UeifSpec.h
**/
EFI_STATUS
EFIAPI
LocateProtocolHook (
  IN  EFI_GUID  *Protocol,
  IN  VOID      *Registration OPTIONAL,
  OUT VOID      **Interface
  )
{
  DL_PROTOCOL_USAGE_ENTRY  *Entry;
  LIST_ENTRY               *ListPtr;
  BOOLEAN                  Logged;

  // Perform logging if enabled
  if (gLoggingEnabled) {
    // Determine if this GUID/memory location has already been logged
    Logged = FALSE;
    BASE_LIST_FOR_EACH (ListPtr, &mProtocolUsageLL) {
      Entry = LIST_ENTRY_PTR_TO_USAGE_ENTRY_PTR (ListPtr);
      if (CompareGuid (&(Entry->GuidName), Protocol) && (Entry->GuidAddress == (UINTN)Protocol)) {
        Logged = TRUE;
        break;
      }
    }

    // Add a new node if this GUID was not already logged
    if (!Logged) {
      DEBUG ((DEBUG_INFO, "[%a] Logging Protocol %g @ 0x%016lX\n", DEBUG_TAG, Protocol, (UINTN)Protocol));

      Entry = (DL_PROTOCOL_USAGE_ENTRY *)AllocatePool (sizeof (DL_PROTOCOL_USAGE_ENTRY));
      ASSERT (Entry);
      if (Entry != NULL) {
        Entry->GuidAddress = (UINTN)Protocol;
        CopyGuid (&(Entry->GuidName), Protocol);
      }

      InsertTailList (&mProtocolUsageLL, &(Entry->ListEntry));
    }
  }

  // Hand off to boot services LocateProtocol
  return HookedLocateProtocol (Protocol, Registration, Interface);
}

/**
  The constructor hooks into the LocateProtocol function pointer to allow logging and registers for a ready-to-boot
  callback to publish all recorded data.

  @param[in]  ImageHandle   The firmware allocated handle for the EFI image.
  @param[in]  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS
**/
EFI_STATUS
EFIAPI
DxeMainDependencyLoggingLibInit (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_EVENT   DepexDataRtbCallbackEvent = NULL;
  EFI_STATUS  Status;

  DEBUG ((DEBUG_INFO, "[%a] Hooking LocateProtocol and registering a ready-to-boot callback\n", DEBUG_TAG));

  // Register for a callback at ready-to-boot
  Status = EfiCreateEventReadyToBootEx (
             TPL_CALLBACK,
             DepexDataRtbCallback,
             NULL,
             &DepexDataRtbCallbackEvent
             );
  ASSERT_EFI_ERROR (Status);

  // Hook the LocateProtocol call
  HookedLocateProtocol         = mBootServices.LocateProtocol;
  mBootServices.LocateProtocol = LocateProtocolHook;

  // Enable logging and exit
  gLoggingEnabled = TRUE;
  return EFI_SUCCESS;
}
