/** @file
  Capsule Library instance to process capsule images.

  Copyright (c) 2016, Microsoft Corporation
  Copyright (c) 2007 - 2013, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "DxeCapsuleLibInternal.h"


typedef struct {
  EFI_CAPSULE_HEADER *Capsule;
  BOOLEAN Processed;
} CAP_ENTRY;


/**
  Validate capsule header.

  @param  CapsuleHeader    Points to a capsule header.

  @retval EFI_SUCESS             Input capsule has a valid header.
  @retval EFI_INVALID_PARAMETER  Input capsule has an invalid header.
**/
EFI_STATUS
ValidateCapsuleHeader(
  IN EFI_CAPSULE_HEADER *CapsuleHeader
  )
{
  if (CapsuleHeader == NULL)
  {
    return EFI_INVALID_PARAMETER;
  }

  if (CapsuleHeader->HeaderSize < sizeof(EFI_CAPSULE_HEADER))
  {
    return EFI_INVALID_PARAMETER;
  }

  if (CapsuleHeader->HeaderSize >= CapsuleHeader->CapsuleImageSize)
  {
    return EFI_INVALID_PARAMETER;
  }

  if ((UINTN)CapsuleHeader + CapsuleHeader->CapsuleImageSize < (UINTN)CapsuleHeader)
  {
    return EFI_INVALID_PARAMETER;  // pointer overflow
  }

  return EFI_SUCCESS;
}

/**
  Validate Fmp capsules layout.

  @param  CapsuleHeader    Points to a capsule header.

  @retval EFI_SUCESS                     Input capsule is a correct FMP capsule.
  @retval EFI_INVALID_PARAMETER  Input capsule is not a correct FMP capsule.
**/
EFI_STATUS
ValidateFmpCapsule(
  IN EFI_CAPSULE_HEADER *CapsuleHeader
  )
{
  EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER       *FmpCapsuleHeader;
  UINT8                                        *EndOfCapsule;
  EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER *ImageHeader;
  UINT8                                        *EndOfPayload = NULL;
  UINT64                                       *ItemOffsetList;
  UINT32                                       ItemNum;
  UINTN                                        Index;
  UINT64                                       PreviousItemOffset = 0;

  if(ValidateCapsuleHeader(CapsuleHeader) != EFI_SUCCESS) {
    return EFI_INVALID_PARAMETER;
  }

  FmpCapsuleHeader = (EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER *)((UINT8 *)CapsuleHeader + CapsuleHeader->HeaderSize);
  EndOfCapsule = (UINT8 *)CapsuleHeader + CapsuleHeader->CapsuleImageSize;

  if (FmpCapsuleHeader->Version > EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER_INIT_VERSION) {
    return EFI_INVALID_PARAMETER;
  }
  ItemOffsetList = (UINT64 *)(FmpCapsuleHeader + 1);

  ItemNum = FmpCapsuleHeader->EmbeddedDriverCount + FmpCapsuleHeader->PayloadItemCount;
  if (ItemNum == 0) {
    return EFI_SUCCESS;
  }

  //Currently we do not support Embedded Drivers.  
  //This opens up concerns about validating the driver as we can't trust secure boot chain (pk)
  if (FmpCapsuleHeader->EmbeddedDriverCount != 0)
  {
    DEBUG((DEBUG_ERROR, "%a - FMP Capsule contains an embedded driver.  This is not supported by this implementation\n", __FUNCTION__));
    return EFI_UNSUPPORTED;
  }

  for (Index = 0; Index < ItemNum; Index++)
  {
    if (ItemOffsetList[Index] >= (CapsuleHeader->CapsuleImageSize - CapsuleHeader->HeaderSize) || \
        ItemOffsetList[Index] < (sizeof(EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER) + sizeof(UINT64)*ItemNum) || \
        ((UINTN)FmpCapsuleHeader + ItemOffsetList[Index]) < (UINTN)FmpCapsuleHeader || \
        ((UINT8 *)FmpCapsuleHeader + ItemOffsetList[Index]) >= EndOfCapsule)
    {
      //this Item in list is not valid so its not good. 
      return EFI_INVALID_PARAMETER;
    }

    if (ItemOffsetList[Index] <= PreviousItemOffset)
    {
      // all items in list must be sorted in ascending order
      return EFI_INVALID_PARAMETER;
    }

    PreviousItemOffset = ItemOffsetList[Index];
  }

  if (FmpCapsuleHeader->PayloadItemCount != 0) {
    //
    // Check if the last payload is within capsule image range
    //
    ImageHeader = (EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER *)((UINT8 *)FmpCapsuleHeader + ItemOffsetList[ItemNum - 1]);
    if (((UINT8 *)ImageHeader + sizeof(EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER) - sizeof(ImageHeader->UpdateHardwareInstance)) >= EndOfCapsule)
    {
      return EFI_INVALID_PARAMETER;
    }

    //check ImageHeader version
    //Anything other than Current Version
    if (ImageHeader->Version != EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER_INIT_VERSION)
    {
      //special case to support V1
      if (ImageHeader->Version == 0x1)
      {
        DEBUG((DEBUG_WARN, "%a - FMP Capsule Image Header is V1.  Supported but you should move to V2 ASAP.\n", __FUNCTION__));
        EndOfPayload = (UINT8 *)ImageHeader;
        EndOfPayload += sizeof(EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER) - sizeof(ImageHeader->UpdateHardwareInstance);
        EndOfPayload += ImageHeader->UpdateImageSize + ImageHeader->UpdateVendorCodeSize;
      }
      else
      {
        DEBUG((DEBUG_ERROR, "%a - FMP Capsule Image Header is not a supported Version.\n", __FUNCTION__));
        DEBUG((DEBUG_INFO, "Supported Version 0x%X\nInput Capsule Version 0x%X\n", EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER_INIT_VERSION, ImageHeader->Version));
        return EFI_UNSUPPORTED;
      }
    }
    else //Standard Path Header == Current Header (v2)
    {
      EndOfPayload = (UINT8 *)(ImageHeader + 1) + ImageHeader->UpdateImageSize + ImageHeader->UpdateVendorCodeSize;
    }
  } //End PayloadItemCount != 0 

  // If embedded drivers supported in future and no payloads present, then EndOfPayload will need to be calculated for the last driver also

  if (EndOfPayload != EndOfCapsule) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/**
  Process Firmware management protocol data capsule.

  @param  CapsuleHeader         Points to a capsule header.

  @retval EFI_SUCESS            Process Capsule Image successfully.
  @retval EFI_UNSUPPORTED       Capsule image is not supported by the firmware.
  @retval EFI_VOLUME_CORRUPTED  FV volume in the capsule is corrupted.
  @retval EFI_OUT_OF_RESOURCES  Not enough memory.
**/
EFI_STATUS
ProcessFmpCapsuleImage(
  IN EFI_CAPSULE_HEADER *CapsuleHeader
  )
{
  EFI_STATUS                                    Status;
  EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER        *FmpCapsuleHeader;
  UINT8                                         *EndOfCapsule;
  EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER  *ImageHeader;
  EFI_HANDLE                                    ImageHandle;
  UINT64                                        *ItemOffsetList;
  UINT32                                        ItemNum;
  UINTN                                         Index;
  UINTN                                         ExitDataSize;
  EFI_HANDLE                                    *HandleBuffer;
  EFI_FIRMWARE_MANAGEMENT_PROTOCOL              *Fmp;
  UINTN                                         NumberOfHandles;
  UINTN                                         DescriptorSize;
  UINT8                                         FmpImageInfoCount;
  UINT32                                        FmpImageInfoDescriptorVer;
  UINTN                                         ImageInfoSize;
  UINT32                                        PackageVersion;
  CHAR16                                        *PackageVersionName;
  CHAR16                                        *AbortReason;
  EFI_FIRMWARE_IMAGE_DESCRIPTOR                 *FmpImageInfoBuf;
  EFI_FIRMWARE_IMAGE_DESCRIPTOR                 *TempFmpImageInfo;
  UINTN                                         DriverLen;
  UINTN                                         Index1;
  UINTN                                         Index2;
  MEMMAP_DEVICE_PATH                            MemMapNode;
  EFI_DEVICE_PATH_PROTOCOL                      *DriverDevicePath;

  //
  // Validate the capsule (perhaps again) before processing in case some one calls
  // ProcessFmpCapsuleImage() before or without calling ValidateFmpCapsule()
  //
  Status = ValidateFmpCapsule(CapsuleHeader);
  if (EFI_ERROR(Status))
  {
    return Status;
  }

  Status = EFI_SUCCESS;
  HandleBuffer = NULL;
  ExitDataSize = 0;
  DriverDevicePath = NULL;

  FmpCapsuleHeader = (EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER *)((UINT8 *)CapsuleHeader + CapsuleHeader->HeaderSize);
  EndOfCapsule = (UINT8 *)CapsuleHeader + CapsuleHeader->CapsuleImageSize;

  if (FmpCapsuleHeader->Version > EFI_FIRMWARE_MANAGEMENT_CAPSULE_HEADER_INIT_VERSION) 
  {
    return EFI_INVALID_PARAMETER;
  }
  ItemOffsetList = (UINT64 *)(FmpCapsuleHeader + 1);

  ItemNum = FmpCapsuleHeader->EmbeddedDriverCount + FmpCapsuleHeader->PayloadItemCount;

  //
  // capsule in which driver count and payload count are both zero is not processed.
  //
  if (ItemNum == 0) 
  {
    return EFI_SUCCESS;
  }

  //
  // 1. ConnectAll to ensure 
  //    All the communication protocol required by driver in capsule installed 
  //    All FMP protocols are installed
  //
  EfiBootManagerConnectAll();


  //
  // 2. Try to load & start all the drivers within capsule 
  //
  SetDevicePathNodeLength(&MemMapNode.Header, sizeof(MemMapNode));
  MemMapNode.Header.Type = HARDWARE_DEVICE_PATH;
  MemMapNode.Header.SubType = HW_MEMMAP_DP;
  MemMapNode.MemoryType = EfiBootServicesCode;
  MemMapNode.StartingAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)CapsuleHeader;
  MemMapNode.EndingAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)((UINT8 *)CapsuleHeader + CapsuleHeader->CapsuleImageSize - 1);

  DriverDevicePath = AppendDevicePathNode(NULL, &MemMapNode.Header);
  if (DriverDevicePath == NULL) 
  {
    return EFI_OUT_OF_RESOURCES;
  }

  for (Index = 0; Index < FmpCapsuleHeader->EmbeddedDriverCount; Index++) 
  {
    if ((FmpCapsuleHeader->PayloadItemCount == 0) && (Index == (UINTN)FmpCapsuleHeader->EmbeddedDriverCount - 1)) 
    {
      //
      // When driver is last element in the ItemOffsetList array, the driver size is calculated by reference CapsuleImageSize in EFI_CAPSULE_HEADER
      //
      DriverLen = CapsuleHeader->CapsuleImageSize - CapsuleHeader->HeaderSize - (UINTN)ItemOffsetList[Index];
    }
    else 
    {
      DriverLen = (UINTN)ItemOffsetList[Index + 1] - (UINTN)ItemOffsetList[Index];
    }

    Status = gBS->LoadImage(
      FALSE,
      gImageHandle,
      DriverDevicePath,
      (UINT8 *)FmpCapsuleHeader + ItemOffsetList[Index],
      DriverLen,
      &ImageHandle
      );
    if (EFI_ERROR(Status)) 
    {
      goto EXIT;
    }

    Status = gBS->StartImage(
      ImageHandle,
      &ExitDataSize,
      NULL
      );
    if (EFI_ERROR(Status)) 
    {
      DEBUG((DEBUG_ERROR, "Driver Return Status = %r\n", Status));
      goto EXIT;
    }
  }

  //
  // Connnect all again to connect drivers within capsule 
  //
  if (FmpCapsuleHeader->EmbeddedDriverCount > 0) 
  {
    EfiBootManagerConnectAll();
  }

  //
  // 3. Route payload to right FMP instance
  //
  Status = gBS->LocateHandleBuffer(
    ByProtocol,
    &gEfiFirmwareManagementProtocolGuid,
    NULL,
    &NumberOfHandles,
    &HandleBuffer
    );

  if (!EFI_ERROR(Status)) 
  {
    for (Index1 = 0; Index1 < NumberOfHandles; Index1++) 
    {
      Status = gBS->HandleProtocol(
        HandleBuffer[Index1],
        &gEfiFirmwareManagementProtocolGuid,
        (VOID **)&Fmp
        );
      if (EFI_ERROR(Status)) 
      {
        continue;
      }

      ImageInfoSize = 0;
      Status = Fmp->GetImageInfo(
        Fmp,
        &ImageInfoSize,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
        );
      if (Status != EFI_BUFFER_TOO_SMALL) 
      {
        continue;
      }

      FmpImageInfoBuf = NULL;
      FmpImageInfoBuf = AllocateZeroPool(ImageInfoSize);
      if (FmpImageInfoBuf == NULL) 
      {
        Status = EFI_OUT_OF_RESOURCES;
        goto EXIT;
      }

      PackageVersionName = NULL;
      Status = Fmp->GetImageInfo(
        Fmp,
        &ImageInfoSize,               // ImageInfoSize
        FmpImageInfoBuf,              // ImageInfo
        &FmpImageInfoDescriptorVer,   // DescriptorVersion
        &FmpImageInfoCount,           // DescriptorCount
        &DescriptorSize,              // DescriptorSize
        &PackageVersion,              // PackageVersion
        &PackageVersionName           // PackageVersionName
        );

      //
      // If FMP GetInformation interface failed, skip this resource
      //
      if (EFI_ERROR(Status)) 
      {
        FreePool(FmpImageInfoBuf);
        continue;
      }

      if (PackageVersionName != NULL) 
      {
        FreePool(PackageVersionName);
      }

      TempFmpImageInfo = FmpImageInfoBuf;
      for (Index2 = 0; Index2 < FmpImageInfoCount; Index2++) 
      {
        DEBUG(( DEBUG_VERBOSE, __FUNCTION__" - Checking '%s', type %g, index %d...\n",
                TempFmpImageInfo->ImageIdName, &TempFmpImageInfo->ImageTypeId, TempFmpImageInfo->ImageIndex ));
        //
        // Check all the payload entry in capsule payload list.
        // Start counting at end of embedded drivers and continue until you're out of items.
        //
        for (Index = FmpCapsuleHeader->EmbeddedDriverCount; Index < ItemNum; Index++) 
        {
          // Point to the image header for the current payload.
          UINTN HeaderSize = sizeof(EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER);
          ImageHeader = (EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER *)((UINT8 *)FmpCapsuleHeader + ItemOffsetList[Index]);

          DEBUG(( DEBUG_VERBOSE, __FUNCTION__" - Checking payload type %g, index %d...\n",
                  &ImageHeader->UpdateImageTypeId, TempFmpImageInfo->ImageIndex ));

          if (CompareGuid(&ImageHeader->UpdateImageTypeId, &TempFmpImageInfo->ImageTypeId) &&
              ImageHeader->UpdateImageIndex == TempFmpImageInfo->ImageIndex)
          {
            //check version
            if (ImageHeader->Version != EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER_INIT_VERSION)
            {
              //
              //NOT current version 
              // We support version 1 headers while current is version 2
              //
              if ((ImageHeader->Version == 0x1) && (EFI_FIRMWARE_MANAGEMENT_CAPSULE_IMAGE_HEADER_INIT_VERSION == 2))
              {
                HeaderSize -= sizeof(UINT64);
              }
              else
              {
                //unsupported version  this payload object is not supported
                DEBUG(( DEBUG_WARN, __FUNCTION__" - Payload %g version bad! %d\n", &ImageHeader->UpdateImageTypeId, ImageHeader->Version ));
                continue;
              }
            }


            AbortReason = NULL;

            if (ImageHeader->UpdateVendorCodeSize == 0) 
            {
              Status = Fmp->SetImage(
                Fmp,
                TempFmpImageInfo->ImageIndex,           // ImageIndex
                ((UINT8 *)ImageHeader + HeaderSize),  // Image
                ImageHeader->UpdateImageSize,           // ImageSize
                NULL,                                   // VendorCode
                Update_Image_Progress,                  // Progress
                &AbortReason                            // AbortReason
                );
            }
            else 
            {
              Status = Fmp->SetImage(
                Fmp,
                TempFmpImageInfo->ImageIndex,                                          // ImageIndex
                (((UINT8 *)ImageHeader) + HeaderSize),                                  // Image
                ImageHeader->UpdateImageSize,                                          // ImageSize
                (UINT8 *)(((UINT8 *)ImageHeader + HeaderSize) + ImageHeader->UpdateImageSize), // VendorCode
                Update_Image_Progress,                                                 // Progress
                &AbortReason                                                           // AbortReason
                );
            }
            if (AbortReason != NULL) 
            {
              DEBUG((EFI_D_ERROR, "ABORT REASON: %s\n", AbortReason));
              FreePool(AbortReason);
            }
          }
          else
          {
            DEBUG(( DEBUG_VERBOSE, __FUNCTION__" - Id or index did not match.\n" ));
          }
        }

        //
        // Use DescriptorSize to move ImageInfo Pointer to stay compatible with different ImageInfo version
        //
        TempFmpImageInfo = (EFI_FIRMWARE_IMAGE_DESCRIPTOR *)((UINT8 *)TempFmpImageInfo + DescriptorSize);
      }
      FreePool(FmpImageInfoBuf);
    }
  }

EXIT:

  if (HandleBuffer != NULL) 
  {
    FreePool(HandleBuffer);
  }

  if (DriverDevicePath != NULL) 
  {
    FreePool(DriverDevicePath);
  }

  return Status;
}


/**
Process Windows Firmware Update Display Capsule

@param CapsuleHeader      Points to a capsule header

**/
EFI_STATUS
EFIAPI
ProcessWindowsFwUpdateDisplayCapsule(
  IN EFI_CAPSULE_HEADER *CapsuleHeader
  )
{
  DISPLAY_DISPLAY_PAYLOAD* pload = NULL;
  EFI_GRAPHICS_OUTPUT_PROTOCOL* gop = NULL;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Blt = NULL;
  EFI_STATUS Status = EFI_SUCCESS;
  UINT8*  Image = NULL;
  UINT8   Checksum = 0;
  UINTN   ImageSize = 0;

  UINTN  BmpWidth = 0;
  UINTN  BmpHeight = 0;
  UINTN   BltSize = 0;

  if(ValidateCapsuleHeader(CapsuleHeader) != EFI_SUCCESS) {
    return EFI_INVALID_PARAMETER;
  }

  //lets check known values
  if (!CompareGuid(&gWindowsUxCapsuleGuid, &CapsuleHeader->CapsuleGuid)) {
    DEBUG((DEBUG_ERROR, "ProcessWindowsFwUpdateDisplayCapsule - Wrong Capsule Header Guid\n"));
    return EFI_UNSUPPORTED;
  }

  if ((CapsuleHeader->Flags & CAPSULE_FLAGS_PERSIST_ACROSS_RESET) != CAPSULE_FLAGS_PERSIST_ACROSS_RESET)
  {
    DEBUG((DEBUG_ERROR, "ProcessWindowsFwUpdateDisplayCapsule - Unexpected flags. 0x%x\n", CapsuleHeader->Flags));
    return EFI_UNSUPPORTED;
  }

  //Get pointer to display capsule payload
  pload = (DISPLAY_DISPLAY_PAYLOAD*)((UINT8*)CapsuleHeader + CapsuleHeader->HeaderSize);
  if (((UINT8 *)pload + sizeof(DISPLAY_DISPLAY_PAYLOAD)) >= ((UINT8 *)CapsuleHeader + CapsuleHeader->CapsuleImageSize))
  {
    return EFI_INVALID_PARAMETER;
  }

  //version
  if (pload->Version != 1)
  {
    DEBUG((DEBUG_ERROR, "ProcessWindowsFwUpdateDisplayCapsule - Payload Not expected version.  0x%x\n", pload->Version));
    return EFI_UNSUPPORTED;
  }

  //imagetype
  if (pload->ImageType != 0)
  {
    DEBUG((DEBUG_ERROR, "ProcessWindowsFwUpdateDisplayCapsule - Payload has unsupported ImageType.  0x%x\n", pload->ImageType));
    return EFI_UNSUPPORTED;
  }

  //
  // Sanity check - check ImageSize to make sure its reasonable (8mb)
  //
  if (CapsuleHeader->CapsuleImageSize > (8 * 1024 * 1024))
  {
    DEBUG((DEBUG_ERROR, "ProcessWindowsFwUpdateDisplayCapsule - CapsuleImageSize is too big.  0x%x\n", CapsuleHeader->CapsuleImageSize));
    return EFI_ABORTED;
  }

  //Checksum
  Checksum = CalculateSum8((CONST UINT8*) CapsuleHeader, (UINTN)CapsuleHeader->CapsuleImageSize);
  if (Checksum != 0)
  {
    DEBUG((DEBUG_ERROR, "ProcessWindowsFwUpdateDisplayCapsule - Checksum doesn't equal zero.  0x%x\n", Checksum));
    return EFI_ABORTED;
  }

  //now get the image
  Image = (UINT8*)(pload + 1);
  ImageSize = CapsuleHeader->CapsuleImageSize - CapsuleHeader->HeaderSize - sizeof(DISPLAY_DISPLAY_PAYLOAD);

  if ((ImageSize < 1) || (ImageSize > CapsuleHeader->CapsuleImageSize))
  {
    DEBUG((DEBUG_ERROR, "ProcessWindowsFwUpdateDisplayCapsule - Image size is invalid\n", Status));
    return EFI_ABORTED;
  }

  //locate GOP
  Status = gBS->HandleProtocol(gST->ConsoleOutHandle, &gEfiGraphicsOutputProtocolGuid, (VOID **)&gop);

  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "ProcessWindowsFwUpdateDisplayCapsule - could not locate GOP.  Status = %r\n", Status));
    return EFI_ABORTED;
  }

  //
  //SET mode if not already in the correct mode
  //
  if (gop->Mode->Mode != pload->Mode && pload->Mode <= gop->Mode->MaxMode) 
  {
    DEBUG((DEBUG_INFO, "ProcessWindowsFwUpdateDisplayCapsule - GOP Mode not correctly set.  Current Mode: 0x%x  Capsule Defined Mode = %r\n", gop->Mode->Mode, pload->Mode));
    Status = gop->SetMode(gop, pload->Mode);
    if (EFI_ERROR(Status)) {
      DEBUG((DEBUG_ERROR, "ProcessWindowsFwUpdateDisplayCapsule - Failed to set GOP mode.  Attempted Mode: 0x%x  Status = %r\n", pload->Mode, Status));
      return EFI_ABORTED;
    }
  }

  //
  //convert bmp to blt buffer.  Need to free buffer when done
  //
  Status = TranslateBmpToGopBlt(Image, ImageSize, &Blt, &BltSize, &BmpHeight, &BmpWidth);

  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "TranslateBmpToGopBlt returned error. Status = %r\n", Status));
    goto Cleanup;
  }

  DEBUG((DEBUG_INFO, "FwUpdateDisplayBmp - \n\t Destination X 0x%X\n\t Destination Y 0x%X\n\t Width 0x%X\n\t Height 0x%X\n", pload->OffsetX, pload->OffsetY, BmpWidth, BmpHeight));

  //BLT operation
  Status = gop->Blt(
    gop,
    Blt,
    EfiBltBufferToVideo,
    (UINTN)0,
    (UINTN)0,
    (UINTN)pload->OffsetX,
    (UINTN)pload->OffsetY,
    BmpWidth,
    BmpHeight,
    (BmpWidth * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL))
    );

  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "ProcessWindowsFwUpdateDisplayCapsule - Failed to Blt Buffer To Video. Status %r\n", Status));
  }
Cleanup:
  if (Blt == NULL) {
    DEBUG((DEBUG_ERROR, "ProcessWindowsFwUpdateDisplayCapsule - Failed to Blt Buffer To Video.  Blt is NULL\n"));
  }
  else
  {
    FreePool(Blt);
  }
  return Status;
}


UINT32
EFIAPI
ClearCapsuleVars()
{
  CHAR16                               CapsuleVarName[30];
  CHAR16                               *TempVarName;
  EFI_PHYSICAL_ADDRESS                 CapsuleDataPtr64;
  EFI_STATUS                           Status;
  UINTN                                Size;
  UINTN                                Index;
  UINT32                               Cleared;

  Cleared = 0;
  Size = sizeof(CapsuleDataPtr64);
  StrCpy(CapsuleVarName, EFI_CAPSULE_VARIABLE_NAME);
  TempVarName = CapsuleVarName + StrLen(CapsuleVarName);
  for (Index = 0; Index < PcdGet8(PcdMaxCapsules); Index++)
  {
    if (Index > 0) {
      //for all index values greater than zero 
      //the number must be appended to the var name
      UnicodeValueToString(TempVarName, 0, Index, 0);
    }

    Status = gRT->GetVariable(CapsuleVarName, &gEfiCapsuleVendorGuid, NULL, &Size, &CapsuleDataPtr64);
    if (Status == EFI_NOT_FOUND)
    {
      DEBUG((DEBUG_VERBOSE, "Capsule variable Index = %d NOT FOUND.\n", Index));
      continue;
    }
    else if (EFI_ERROR(Status)) 
    {
      DEBUG((DEBUG_ERROR, "Capsule variable Index = %d returned unexpected error %r.  Will delete anyway.  \n", Index, Status));
    }
    else 
    {
      DEBUG((DEBUG_VERBOSE, "Capsule variable Index = %d FOUND.  Delete it now.\n", Index));
    }
   Status = gRT->SetVariable(CapsuleVarName, &gEfiCapsuleVendorGuid, EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS, 0, NULL);

   if (EFI_ERROR(Status))
   {
     DEBUG((DEBUG_ERROR, "Failed to delete Capsule Variable for Index %i Status = %r.\n", Index, Status));
   }
   else
   {
     Cleared += 1;
   }
  }

  Size = sizeof(CapsuleDataPtr64);
  StrCpy(CapsuleVarName, EFI_SYSTEM_TABLE_CAPSULE_VARIABLE_NAME);
  TempVarName = CapsuleVarName + StrLen(CapsuleVarName);
  for (Index = 0; Index < PcdGet8(PcdMaxCapsules); Index++)
  {
    if (Index > 0) {
      //for all index values greater than zero 
      //the number must be appended to the var name
      UnicodeValueToString(TempVarName, 0, Index, 0);
    }

    Status = gRT->GetVariable(CapsuleVarName, &gEfiCapsuleVendorGuid, NULL, &Size, &CapsuleDataPtr64);
    if (Status == EFI_NOT_FOUND)
    {
      DEBUG((DEBUG_VERBOSE, "System Table capsule variable Index = %d NOT FOUND.\n", Index));
      continue;
    }
    else if (EFI_ERROR(Status)) 
    {
      DEBUG((DEBUG_ERROR, "System Table capsule variable Index = %d returned unexpected error %r.  Will delete anyway.  \n", Index, Status));
    }
    else 
    {
      DEBUG((DEBUG_VERBOSE, "System Table capsule variable Index = %d FOUND.  Delete it now.\n", Index));
    }
   Status = gRT->SetVariable(CapsuleVarName, &gEfiCapsuleVendorGuid, EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS, 0, NULL);

   if (EFI_ERROR(Status))
   {
     DEBUG((DEBUG_ERROR, "Failed to delete System Table Capsule Variable for Index %i Status = %r.\n", Index, Status));
   }
  }

  return Cleared;
}


/**
  Those capsules supported by the firmwares.

  @param  CapsuleHeader    Points to a capsule header.

  @retval EFI_SUCESS       Input capsule is supported by firmware.
  @retval EFI_UNSUPPORTED  Input capsule is not supported by the firmware.
  @retval EFI_INVALID_PARAMETER Input capsule layout is not correct
**/
EFI_STATUS
EFIAPI
SupportCapsuleImage(
  IN EFI_CAPSULE_HEADER *CapsuleHeader
  )
{
  if(ValidateCapsuleHeader(CapsuleHeader) != EFI_SUCCESS) {
    return EFI_INVALID_PARAMETER;
  }

  //Windows Firmware Update Display Capsule
  // This capsule is validated later in ProcessWindowsFwUpdateDisplayCapsule()
  if (CompareGuid(&gWindowsUxCapsuleGuid, &CapsuleHeader->CapsuleGuid)) {
    return EFI_SUCCESS;
  }

  //FMP 
  if (CompareGuid(&gEfiFmpCapsuleGuid, &CapsuleHeader->CapsuleGuid)) {
    //
    // Check layout of FMP capsule
    //
    return ValidateFmpCapsule(CapsuleHeader);
  }

  //if its an unknown guid ---lets check to see if there is fmp capsule embedded in the capsule.
  //...this is because Windows ESRT v1 appends a capsule header with device guid as the capsule guid
  // EFI Capsule header
  //   -- Capsule Guid  [Device Guid]
  //   ---cap header members
  // Efi Capsule Header
  //   -- Capsule Guid [FMP]
  //   ---cap header members
  //
  //# MS WINDOWS ESRT V1 SUPPORT  -- REMOVE THIS ONCE WE CHANGE WINDOWS ESRT BEHAVIOR
  //
  {
    EFI_CAPSULE_HEADER* t = (EFI_CAPSULE_HEADER *)(((UINT8*)CapsuleHeader) + CapsuleHeader->HeaderSize);
    //Embedded FMP 
    if (CompareGuid(&gEfiFmpCapsuleGuid, &t->CapsuleGuid)) {
      //
      // Check layout of FMP capsule
      //
      return ValidateFmpCapsule(t);
    }

  }

  return EFI_UNSUPPORTED;
}

/**
  The firmware implements to process the capsule image.

  @param  CapsuleHeader         Points to a capsule header.

  @retval EFI_SUCESS            Process Capsule Image successfully.
  @retval EFI_UNSUPPORTED       Capsule image is not supported by the firmware.
  @retval EFI_VOLUME_CORRUPTED  FV volume in the capsule is corrupted.
  @retval EFI_OUT_OF_RESOURCES  Not enough memory.
**/
EFI_STATUS
EFIAPI
ProcessCapsuleImage(
  IN EFI_CAPSULE_HEADER *CapsuleHeader
  )
{
  DEBUG((DEBUG_INFO, "Starting %a...\n", __FUNCTION__));

  if (SupportCapsuleImage(CapsuleHeader) != EFI_SUCCESS) {
    return EFI_UNSUPPORTED;
  }

  //
  // Process FMP
  //
  if (CompareGuid(&gEfiFmpCapsuleGuid, &CapsuleHeader->CapsuleGuid)) {
    return ProcessFmpCapsuleImage(CapsuleHeader);
  }

  //
  // Check Windows Firmware Update Display Capsule
  //
  if (CompareGuid(&gWindowsUxCapsuleGuid, &CapsuleHeader->CapsuleGuid)) {
    return ProcessWindowsFwUpdateDisplayCapsule(CapsuleHeader);
  }

  //if its an unknown guid ---lets check to see if there is fmp capsule embedded in the capsule.
  //...this is because Windows ESRT v1 appends a capsule header with device guid as the capsule guid
  // EFI Capsule header
  //   -- Capsule Guid  [Device Guid]
  //   ---cap header members
  // Efi Capsule Header
  //   -- Capsule Guid [FMP]
  //   ---cap header members
  //
  //# MS WINDOWS ESRT V1 SUPPORT -- REMOVE THIS ONCE WE CHANGE WINDOWS ESRT BEHAVIOR
  //
  {
    EFI_CAPSULE_HEADER* t = (EFI_CAPSULE_HEADER *)(((UINT8*)CapsuleHeader) + CapsuleHeader->HeaderSize);
    //Embedded FMP 
    if (CompareGuid(&gEfiFmpCapsuleGuid, &t->CapsuleGuid)) {
      return ProcessFmpCapsuleImage(t);
    }

  }

  return EFI_UNSUPPORTED;
}


/**
 Library function used to look thru the hob list 
 and locate and process all supported capsules.

 return EFI_STATUS
**/
EFI_STATUS
EFIAPI
ProcessCapsules (
  VOID
  )
{
  EFI_STATUS                  Status;
  EFI_PEI_HOB_POINTERS        HobPointer;
  EFI_CAPSULE_HEADER          *CapsuleHeader;
  UINT32                      CapsuleTotalNumber;
  UINT32                      Index;
  CONST UINT32                CapsuleMaxNumber = PcdGet8(PcdMaxCapsules);
  CAP_ENTRY                   *CapArray;
  UINT32                      Cleared;

  CapsuleTotalNumber = 0;
  Status = EFI_SUCCESS;
  CapArray = NULL;

  //
  // Clear all of the capsule variables.
  //
  Cleared = ClearCapsuleVars();

  //
  // If not in flash-update mode, don't do anything else.
  //
  if (GetBootModeHob() != BOOT_ON_FLASH_UPDATE) {

    //
    // If capsule variables were cleared while not in flash-update mode, an
    // an error must have occurred before the variables could be cleared in
    // an earlier session.  A telemetry event for this condition should be
    // added here in the future.
    //
    if (Cleared != 0) {
      DEBUG((DEBUG_INFO, "ProcessCapsules - cleared %d capsule variables while not in flash update mode\n", Cleared));
    }

    goto Cleanup;
  }

  CapArray = AllocateZeroPool(sizeof (CAP_ENTRY) * CapsuleMaxNumber);
  if (CapArray == NULL)
  {
    Status = EFI_OUT_OF_RESOURCES;
    goto Cleanup;
  }

  //
  // Find all capsule images from hob
  //
  HobPointer.Raw = GetHobList();
  while ((HobPointer.Raw = GetNextHob(EFI_HOB_TYPE_UEFI_CAPSULE, HobPointer.Raw)) != NULL) {
    // skip NULL base-addresses
    if (HobPointer.Capsule->BaseAddress != 0) {
      CapArray[CapsuleTotalNumber].Capsule = (EFI_CAPSULE_HEADER *)(UINTN)(HobPointer.Capsule->BaseAddress);
      CapArray[CapsuleTotalNumber].Processed = FALSE;
      CapsuleTotalNumber++;
    }

    if (CapsuleTotalNumber == CapsuleMaxNumber) { 
      DEBUG((DEBUG_INFO, "ProcessCapsules - Reached Max Capsule Supported in a single pass\n"));
      break;  
    }
    HobPointer.Raw = GET_NEXT_HOB(HobPointer);
  }

  DEBUG((DEBUG_INFO, "Total Number of Capsules to process: %d\n", CapsuleTotalNumber));

  if (CapsuleTotalNumber == 0) {
    //
    // We didn't find a hob, so had no errors.
    //
    Status = EFI_SUCCESS;
    goto Cleanup;
  }

  //
  //Loop once to locate the Windows Display Capsule as it should be handled first. 
  //
  for (Index = 0; Index < CapsuleTotalNumber; Index++) {
    CapsuleHeader = (EFI_CAPSULE_HEADER*)(CapArray[Index].Capsule);
    if (ValidateCapsuleHeader(CapsuleHeader) == EFI_SUCCESS && \
        (CapsuleHeader->Flags & CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE) == 0) {
      if (CompareGuid(&gWindowsUxCapsuleGuid, &CapsuleHeader->CapsuleGuid)){
        DEBUG((DEBUG_INFO, "Found Windows Display Capsule!\n"));
        //
        // Call capsule library to process capsule image.
        //
        ProcessCapsuleImage(CapsuleHeader);
        CapArray[Index].Processed = TRUE;
      }
    }
  }


  //
  // Loop and process all capsules that have not already been processed.  
  //
  for (Index = 0; Index < CapsuleTotalNumber; Index++) {
    if (CapArray[Index].Processed == TRUE)
    {
      continue;
    }

    CapsuleHeader = (EFI_CAPSULE_HEADER*)CapArray[Index].Capsule;
    if (ValidateCapsuleHeader(CapsuleHeader) == EFI_SUCCESS && \
        (CapsuleHeader->Flags & CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE) == 0) {
      //
      // Call capsule library to process capsule image.
      //
      Status = ProcessCapsuleImage(CapsuleHeader);
      if (EFI_ERROR(Status))
      {
        DEBUG((DEBUG_ERROR, "ProcessCapsuleImage Failed (%r) for capsule index 0x%X\n", Status, Index));
      }
    }
    else
    {
      //TODO: WE SHOULD FIGURE OUT IF WE WANT TO SUPPORT THESE
      DEBUG((DEBUG_INFO, "We have a capsule with Populate System Table.  Do nothing for now!\n"));
    }
  }

  //
  //once finished processing we will reset.  
  //
  Status = ResetAfterCapsuleUpdate();  
  if (EFI_ERROR(Status))
  {

    DEBUG((DEBUG_WARN, "CapsuleProcessLib: ResetAfterCapsuleUpdate didn't handle reset %r.  Doing UEFI Standard Reset.\n", Status));
    gRT->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, NULL);
  }

Cleanup:
  if (CapArray != NULL) {
    FreePool(CapArray);
  }

  return Status;
}

/**

 Library function used to look thru the hob list 
 and locate and process system table capsules.

**/
VOID
EFIAPI
LocateAndProcessSystemTableCapsules (
  VOID
  )
{

  UINT32                      CacheIndex;
  UINT32                      CacheNumber;
  CONST UINT32                CapsuleMaxNumber = PcdGet8(PcdMaxCapsules);
  CAP_ENTRY                   *CapArray;
  EFI_GUID                    *CapsuleGuidCache;
  EFI_CAPSULE_HEADER*         CapsuleHeader;
  UINT32                      CapsuleNumber;
  VOID                        **CapsulePtrCache;
  EFI_STATUS                  *CapsuleStatusArray;
  EFI_CAPSULE_TABLE           *CapsuleTable;
  UINT32                      CapsuleTotalNumber;
  EFI_PEI_HOB_POINTERS        HobPointer;
  UINT32                      Index;
  UINTN                       Size;
  EFI_STATUS                  Status;

  DEBUG((DEBUG_INFO, __FUNCTION__ ": enter...\n"));

  CacheNumber = 0;
  CapsuleNumber = 0;
  CapsuleTotalNumber = 0;
  CapsuleGuidCache = NULL;
  CapsulePtrCache = NULL;
  CapsuleStatusArray = NULL;
  CapArray = NULL;

  //
  // Clear the capsule variables so they are not present next time system starts.
  //
  ClearCapsuleVars();

  //
  // We don't do anything else if the boot mode is not test_capsule.
  //
  if (GetBootModeHob() != BOOT_ON_SYSTEM_TABLE_CAPSULE) {
    goto Cleanup;
  }

  CapArray = AllocateZeroPool(sizeof (CAP_ENTRY) * CapsuleMaxNumber);
  if (CapArray == NULL)
  {
    Status = EFI_OUT_OF_RESOURCES;
    goto Cleanup;
  }

  //
  // Find all capsule images from hob
  //
  HobPointer.Raw = GetHobList();
  while ((HobPointer.Raw = GetNextHob(EFI_HOB_TYPE_UEFI_CAPSULE, HobPointer.Raw)) != NULL) {
    // skip NULL base-addresses
    if (HobPointer.Capsule->BaseAddress != 0) {
      CapArray[CapsuleTotalNumber].Capsule = (EFI_CAPSULE_HEADER *)(UINTN)(HobPointer.Capsule->BaseAddress);
      CapArray[CapsuleTotalNumber].Processed = FALSE;
      CapsuleTotalNumber++;
    }

    if (CapsuleTotalNumber == CapsuleMaxNumber) { 
      DEBUG((DEBUG_INFO, __FUNCTION__ ": Reached Max Capsule Supported in a single pass\n"));
      break;  
    }

    HobPointer.Raw = GET_NEXT_HOB(HobPointer);
  }

  DEBUG((DEBUG_INFO, __FUNCTION__ ": Number of Capsules to process: %d\n", CapsuleTotalNumber));

  //
  // Return if there are no capsules.
  //
  if (CapsuleTotalNumber == 0) {
    goto Cleanup;
  }

  CapsulePtrCache  = (VOID**)AllocateZeroPool(sizeof (VOID*) * CapsuleTotalNumber);
  if (CapsulePtrCache == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Cleanup;
  }

  CapsuleGuidCache = (EFI_GUID*)AllocateZeroPool(sizeof(EFI_GUID) * CapsuleTotalNumber);
  if (CapsuleGuidCache == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Cleanup;
  }

  CapsuleStatusArray = (EFI_STATUS*)AllocateZeroPool(sizeof(EFI_STATUS) * CapsuleTotalNumber);
  if (CapsuleStatusArray == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Cleanup;
  }

  //
  // Capsules that have the CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE flag set are 
  // used by the operating system to persist information across a system reset.
  // The EFI System Table must point to an array of capsules that contains the 
  // same CapsuleGuid value. Agents searching for this type capsule will look
  // in the EFI System Table and search for the capsule's Guid and associated 
  // pointer to retrieve the data. The two steps below sort the capsules by guid
  // and install the array to EFI System Table. First, loop for all coalesced
  // capsules, record unique CapsuleGuids and cache them in an array for later
  // sorting.
  //
  for (Index = 0; Index < CapsuleTotalNumber; Index += 1) {
    CapsuleStatusArray[Index] = EFI_UNSUPPORTED;
    CapsuleHeader = (EFI_CAPSULE_HEADER*)CapArray[Index].Capsule;
    if (ValidateCapsuleHeader(CapsuleHeader) == EFI_SUCCESS && \
        (CapsuleHeader->Flags & CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE) != 0) {

      //
      // Scan the GUID cache. If an entry is found that matches the GUID of the
      // current capsule header, stop scanning.
      //
      CacheIndex = 0;
      while (CacheIndex < CacheNumber) {
        if (CompareGuid(&CapsuleGuidCache[CacheIndex], &CapsuleHeader->CapsuleGuid)) {
          break;
        }

        CacheIndex += 1;
      }

      //
      // If no entry exists for the GUID of the current capsule header, add it.
      //
      if (CacheIndex == CacheNumber) {
        CopyMem(&CapsuleGuidCache[CacheNumber], &CapsuleHeader->CapsuleGuid, sizeof(EFI_GUID));
        CacheNumber += 1;
      }
    }
  }

  //
  // Now, go through the capsule array again, and for each unique GUID, collect
  // pointers to the capsule headers that share the same GUID in a different
  // array.
  //
  CacheIndex = 0;
  while (CacheIndex < CacheNumber) {
    CapsuleNumber = 0;
    for (Index = 0; Index < CapsuleTotalNumber; Index += 1) {
      CapsuleHeader = (EFI_CAPSULE_HEADER*)CapArray[Index].Capsule;
      if (ValidateCapsuleHeader(CapsuleHeader) == EFI_SUCCESS && \
          (CapsuleHeader->Flags & CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE) != 0) {
        if (CompareGuid(&CapsuleGuidCache[CacheIndex], &CapsuleHeader->CapsuleGuid)) {
          CapsulePtrCache[CapsuleNumber] = (VOID*)CapsuleHeader;
          CapsuleNumber += 1;
          CapsuleStatusArray[Index] = EFI_SUCCESS;
        }
      }
    }

    //
    // Now, for each unique GUID in GuidCacheArray, gather all coalesced capsules
    // with the same guid and alloc memory for an array of SYSTEM_TABLE_CAPSULE_TABLE
    // entries. This array is passed to the InstallConfigurationTable routine.
    //
    if (CapsuleNumber != 0) {
      DEBUG((DEBUG_INFO, __FUNCTION__ ": %d capsules to install in system table\n", CapsuleNumber));
      Size = sizeof(EFI_CAPSULE_TABLE) + (CapsuleNumber - 1) * sizeof(VOID*);
      CapsuleTable = AllocateRuntimePool(Size);
      if (CapsuleTable == NULL) {
        DEBUG((DEBUG_INFO, __FUNCTION__ ": failed to alloc capsule table\n"));
        Status = EFI_OUT_OF_RESOURCES;
        goto Cleanup;
      }

      DEBUG((DEBUG_INFO, __FUNCTION__ ": calling InstallConfigurationTable...\n"));
      CapsuleTable->CapsuleArrayNumber = CapsuleNumber;
      CopyMem(&CapsuleTable->CapsulePtr[0], CapsulePtrCache, CapsuleNumber * sizeof(VOID*));
      Status = gBS->InstallConfigurationTable(&CapsuleGuidCache[CacheIndex], (VOID*)CapsuleTable);
      if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, __FUNCTION__ ": error installing configuration table (%r)\n", Status));
        FreePool(CapsuleTable);
      }
    } else {
      DEBUG((DEBUG_INFO, __FUNCTION__ ": no capsules to install in system table\n"));
    }

    CacheIndex += 1;
  }

Cleanup:
  if (CapArray != NULL) {
    FreePool(CapArray);
  }

  if (CapsuleGuidCache != NULL) {
    FreePool(CapsuleGuidCache);
  }

  if (CapsulePtrCache != NULL) {
    FreePool(CapsulePtrCache);
  }

  if (CapsuleStatusArray != NULL) {
    FreePool(CapsuleStatusArray);
  }

  DEBUG((DEBUG_INFO, __FUNCTION__ ": leave\n"));
  return;
}