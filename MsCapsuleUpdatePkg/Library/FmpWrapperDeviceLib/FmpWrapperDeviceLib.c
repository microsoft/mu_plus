/**

Copyright (c) 2016, Microsoft Corporation

All rights reserved.
Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

**/


#include <PiDxe.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Protocol/FirmwareManagement.h>
#include <Guid/SystemResourceTable.h>
#include <Library/BaseLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/FmpDeviceLib.h>
#include <Library/MsFmpPayloadHeaderLib.h>
#include <Library/CapsuleUpdatePolicyLib.h>
#include <Library/FmpHelperLib.h>
#include <Library/CapsuleKeyLib.h>
#include <Guid/EventGroup.h>
#include <Library/HobLib.h>
#include <Library/FmpAuthenticationLib.h>
#include <Library/FmpPolicyLib.h>

#include "VariableSupport.h"



EFI_FIRMWARE_IMAGE_DESCRIPTOR mDesc;

BOOLEAN mDescriptorPopulated = FALSE;
BOOLEAN mRuntimeVersionSupported = TRUE;
BOOLEAN mFmpInstalled = FALSE;

//
//Function pointer to progress function
//
EFI_FIRMWARE_MANAGEMENT_UPDATE_IMAGE_PROGRESS    mProgressFunc = NULL;
BOOLEAN mProgressSupported = FALSE;

CHAR16* mImageIdName = (CHAR16*) PcdGetPtr(PcdDeviceLibWrapperDeviceImageName);
EFI_GUID* mImageTypeId = (EFI_GUID*) PcdGetPtr(PcdDeviceLibWrapperDeviceGuid);
UINT64 mImageId = 0x1;
CHAR16* mVersionName = NULL;

EFI_EVENT mDeviceLibLockFwEvent;
BOOLEAN mFmpDeviceLocked = FALSE;


/*

Routine Description:

Helper function to convert the version number into string value.
This implementation uses 1 byte per version number.

Example:   Convert 0x01020304 to 1.2.3.4

Arguments:
Version -- Input UINT32 UEFI version representation.
string  -- String representation.

Return Value:

EFI_STATUS code.
--*/
EFI_STATUS 
ComputeVersionName(IN UINT32 Version, OUT CHAR16** string)
{
    EFI_STATUS  status = EFI_SUCCESS;
    UINT8*      Byte   = (UINT8*)&Version;

    if (string == NULL)
    {
        DEBUG((DEBUG_ERROR, "Error: string must not be NULL\n"));
        status = EFI_INVALID_PARAMETER;
        goto cleanup;
    }

    if (*string != NULL)
    {
        DEBUG((DEBUG_ERROR, "Error: string must be pointer to NULL\n"));
        status = EFI_INVALID_PARAMETER;
        goto cleanup;
    }
    
    //
    //allocate memory for 000.000.000.000\n
    //
    *string = AllocatePool(16 * 2); //16 chars x 2 for unicode
    if (*string == NULL)
    {
        DEBUG((DEBUG_ERROR, "Insufficient memory.\n"));
        status = EFI_OUT_OF_RESOURCES;
        goto cleanup;
    }

    UnicodeSPrint(*string, 32, L"%d.%d.%d.%d", *(Byte + 3), *(Byte + 2), *(Byte + 1), *(Byte));
    DEBUG((DEBUG_INFO, "INFO: Converted 0x%x to %s\n", Version, *string));

cleanup:

    return status;
}//ComputeVersionName()


/**
Callback funtion to report the process of the firmware updating.

Wrap the caller's version in this so that progress from the device lib is within the expected range. 
Convert device lib 0%-100% to 25%-98%.


Device lib wrapper (this driver) 0% - 5% for validation
FmpDeviceLib 6% - 98%  for flashing/update
Device lib wrapper (this driver) 99% - 100% 
 

@param[in]  Completion    A value between 1 and 100 indicating the current completion
progress of the firmware update. Completion progress is
reported as from 1 to 100 percent. A value of 0 is used by
the driver to indicate that progress reporting is not supported.

@retval EFI_SUCCESS       SetImage() continues to do the callback if supported.
@retval other             SetImage() discontinues the callback and completes
the update and returns.

**/
EFI_STATUS
EFIAPI
DeviceLibWrapperProgress(
IN  UINTN                          Completion
)
{
  EFI_STATUS Status = EFI_UNSUPPORTED;

  if (!mProgressSupported)
  {
    return Status;
  }

  if (mProgressFunc == NULL)
  {
    return Status;
  }

  //reserve 6 - 98 for Lib
  //Call the real progress function
  Status = mProgressFunc(((Completion * 92) / 100) + 6);

  if (Status == EFI_UNSUPPORTED)
  {
    mProgressSupported = FALSE;
    mProgressFunc = NULL;
  }
  
  return Status;
}



/*
Function used to Get the GUID for the Image Type

@returns Always returns a guid.
*/
EFI_GUID*
GetImageTypeIdGuid()
{
  //use function so this can be changed if needed. 

  EFI_GUID* FmpDeviceLibGuid = NULL;
  EFI_STATUS Status;

  Status = FmpDeviceGetImageTypeIdGuidPtr(&FmpDeviceLibGuid);
  if (EFI_ERROR(Status))
  {
    if (Status != EFI_UNSUPPORTED)
    {
      DEBUG((DEBUG_ERROR, "Error: Fmp Device Lib returned invalid error from GetImageTypeIdGuidPtr(). %r\n", Status));
    }
    //return PCD value
    return mImageTypeId;
  }
  ASSERT(FmpDeviceLibGuid != NULL);
  return FmpDeviceLibGuid;
}

/*
 Function used to Get a string describing the Image Type

 @returns Always returns a string.  
*/
CHAR16*
GetImageTypeNameString()
{
  return mImageIdName;
}

/**
Lowest supported version is a combo of three parts. 
1. Check if the device lib has a lowest supported version
2. Check if we have a variable for lowest supported version (this will be updated with each capsule applied)
3. Check Fixed at build PCD

Take the largest value

**/
UINT32
GetLowestSupportedVersion()
{
  UINT32  DeviceLibLowestSupportedVersion = DEFAULT_LOWESTSUPPORTEDVERSION;
  UINT32  VariableLowestSupportedVersion;
  UINT32  ReturnLsv = PcdGet32(PcdBuildTimeLowestSupportedVersion);
  EFI_STATUS Status; 
  //
  // Get the LowestSupportedVersion.  
  //

  if (CheckLowestSupportedVersion() == FALSE)
  {
    return 1;
  }

  //1st - check the device lib
  Status = FmpDeviceGetLowestSupportedVersion(&DeviceLibLowestSupportedVersion);
  if (EFI_ERROR(Status))
  {
    DeviceLibLowestSupportedVersion = DEFAULT_LOWESTSUPPORTEDVERSION;
  }

  if (DeviceLibLowestSupportedVersion > ReturnLsv)
  {
    ReturnLsv = DeviceLibLowestSupportedVersion;
  }
  
  //2nd - Check the lsv variable for this device
  VariableLowestSupportedVersion = GetLowestSupportedVersionFromVariable();
  if (VariableLowestSupportedVersion > ReturnLsv)
  {
    ReturnLsv = VariableLowestSupportedVersion;
  }

  //return the largest value
  return ReturnLsv;
}

/*
Function used to populate the descriptor once it is 
requested.  
*/
VOID
PopulateDescriptor()
{
  EFI_STATUS status; 

  mDesc.ImageIndex = 1;
  CopyGuid(&mDesc.ImageTypeId, GetImageTypeIdGuid());
  mDesc.ImageId = mImageId;
  mDesc.ImageIdName = GetImageTypeNameString();

  //
  // Get the version.  
  // Some Devices don't support getting the firmware
  // version at runtime.  To support will store in variable.
  //
  status = FmpDeviceGetVersion(&mDesc.Version);
  if (status == EFI_UNSUPPORTED)
  {
    mRuntimeVersionSupported = FALSE;
    mDesc.Version = GetVersionFromVariable();
  }
  else if (EFI_ERROR(status))
  {
    //other error
    DEBUG((DEBUG_ERROR, "GetVersion from FMP device lib (%s) returned %r\n", GetImageTypeNameString(), status));
    mDesc.Version = DEFAULT_VERSION;
  }

  //
  // free the existing version name.  
  //  Shouldn't really happen
  // but this populate function could be called multiple times (to refresh).
  //
  if (mVersionName != NULL)
  {
    FreePool(mVersionName);
    mVersionName = NULL;
  }
  //convert uint32 to unicode...
  mVersionName = FmpDeviceGetVersionString();
  
  //if library doesnt' support 
  //then use our built in conversion function.  
  if (mVersionName == NULL)
  {
    DEBUG((DEBUG_INFO, "GetVersionString unsupported in FmpDeviceLib.  Using default version to version string converter.\n"));
    ComputeVersionName(mDesc.Version, &mVersionName);
  }
  
  mDesc.VersionName = mVersionName;


  mDesc.LowestSupportedImageVersion = GetLowestSupportedVersion();

  FmpDeviceGetAttributes(&mDesc.AttributesSupported, &mDesc.AttributesSetting);

  //
  // LIB should report that image is updatable
  //
  if ((mDesc.AttributesSetting & IMAGE_ATTRIBUTE_IMAGE_UPDATABLE) != IMAGE_ATTRIBUTE_IMAGE_UPDATABLE)
  {
    DEBUG((DEBUG_ERROR, "FMP DEVICE LIB returned invalid attributes.  Image must be updatable\n"));
    mDesc.AttributesSupported |= IMAGE_ATTRIBUTE_IMAGE_UPDATABLE;
    mDesc.AttributesSetting |= IMAGE_ATTRIBUTE_IMAGE_UPDATABLE;
  }

  //Force set the authentication bits in the attributes;
  mDesc.AttributesSupported |= (IMAGE_ATTRIBUTE_AUTHENTICATION_REQUIRED);
  mDesc.AttributesSetting |= (IMAGE_ATTRIBUTE_AUTHENTICATION_REQUIRED);

  mDesc.Compatibilities = 0;
  mDesc.Size = FmpDeviceGetSize();

  mDesc.LastAttemptVersion = GetLastAttemptVersionFromVariable();
  mDesc.LastAttemptStatus = GetLastAttemptStatusFromVariable();

  mDescriptorPopulated = TRUE;
}

//----------------------------------
// FMP implementation.
//----------------------------------
EFI_STATUS
GetTheImageInfo
(
IN EFI_FIRMWARE_MANAGEMENT_PROTOCOL       *This,
IN OUT    UINTN                           *ImageInfoSize,
IN OUT    EFI_FIRMWARE_IMAGE_DESCRIPTOR   *ImageInfo,
OUT       UINT32                          *DescriptorVersion,
OUT       UINT8                           *DescriptorCount,
OUT       UINTN                           *DescriptorSize,
OUT       UINT32                          *PackageVersion,
OUT       CHAR16                          **PackageVersionName
)
/*++

Routine Description:

    FMP's GetImageInfo implementation.

Arguments:
    This                -- This instance of the protocol.
    ImageInfoSize       -- A pointer to the size in bytes of the ImageInfo buffer.
    ImageInfo           -- A pointer to the buffer in which the firmware places current images(s).
    DescriptorVersion   -- Version number of the descriptor.
    DescriptorCount     -- Number of descriptors.
    DescriptorSize      -- Size of the image descriptor.
    PackageVersion      -- Pakage version. 
    PackageVersionName  -- Name fo the package.  Caller is responsible for 
                           freeing the memory with FreePool().

Return Value:

    EFI_STATUS code.  
--*/
{
    EFI_STATUS status = EFI_SUCCESS;
    
    //
    // Check for valid pointer
    // 
    if (ImageInfoSize == NULL) 
    {
        DEBUG((DEBUG_ERROR, "GetImageInfo - ImageInfoSize is NULL.\n"));
        status = EFI_INVALID_PARAMETER;
        goto cleanup;
    }

    //
    // Check the buffer size
    // NOTE: Check this first so caller can get the necessary memory size it must allocate.  
    //
    if (*ImageInfoSize < (sizeof(EFI_FIRMWARE_IMAGE_DESCRIPTOR))) 
    {
        *ImageInfoSize = sizeof(EFI_FIRMWARE_IMAGE_DESCRIPTOR);
        DEBUG((DEBUG_VERBOSE, "GetImageInfo - ImageInfoSize is to small.\n"));
        status = EFI_BUFFER_TOO_SMALL;
        goto cleanup;
    }
    
    //
    // Confirm that buffer isn't null
    // 
    if ( (ImageInfo == NULL) || (DescriptorVersion == NULL) || (DescriptorCount == NULL) || (DescriptorSize == NULL) || (PackageVersion == NULL))
    {
        DEBUG((DEBUG_ERROR, "GetImageInfo - Pointer Parameter is NULL.\n"));
        status = EFI_INVALID_PARAMETER;
        goto cleanup;
    }

    //
    // Set the size to whatever we need
    // 
    *ImageInfoSize = sizeof(EFI_FIRMWARE_IMAGE_DESCRIPTOR);


    if (!mDescriptorPopulated)
    {
      PopulateDescriptor();
    }

    //
    // Copy the image descriptor
    // 
    CopyMem(ImageInfo, &mDesc, sizeof(EFI_FIRMWARE_IMAGE_DESCRIPTOR));

    *DescriptorVersion = EFI_FIRMWARE_IMAGE_DESCRIPTOR_VERSION;
    *DescriptorCount = 1;
    *DescriptorSize = sizeof(EFI_FIRMWARE_IMAGE_DESCRIPTOR);
    *PackageVersion = 0xFFFFFFFF;  //means unsupported

    //
    //Leave PackageVersionName pointer alone since we don't support in this instance.  
    // 

cleanup:

    return status;
}// GetImageInfo()

EFI_STATUS
GetTheImage(
IN  EFI_FIRMWARE_MANAGEMENT_PROTOCOL  *This,
IN  UINT8                             ImageIndex,
IN  OUT  VOID                         *Image,
IN  OUT  UINTN                        *ImageSize
)
/*++

Routine Description:

    This is a function used to read the current firmware from the device into memory.
    This is an optional function and can return EFI_UNSUPPORTED.  This is useful for
    test and diagnostics. 

Arguments:

    This                -- This instance of the protocol.
    ImageIndex          -- A unique number identifying the firmware image(s).
                           This number is between 1 and DescriptorCount.
    Image               -- Buffer to place the image into.
    ImageSize           -- Size of the Image buffer.

Return Value:

    EFI_STATUS code.  
    If not possible or not practical return EFI_UNSUPPORTED.  

--*/
{
    EFI_STATUS status = EFI_SUCCESS;

    if ((ImageSize == NULL))
    {
        DEBUG((DEBUG_ERROR, "GetImage - ImageSize Pointer Parameter is NULL.\n"));
        status = EFI_INVALID_PARAMETER;
        goto cleanup;
    }

    //
    // Check the buffer size
    //
    if (*ImageSize < FmpDeviceGetSize()) 
    {
      *ImageSize = FmpDeviceGetSize();
      DEBUG((DEBUG_VERBOSE, "GetImage - ImageSize is to small.\n"));
      status = EFI_BUFFER_TOO_SMALL;
      goto cleanup;
    }

    if (Image == NULL)
    {
      DEBUG((DEBUG_ERROR, "GetImage - Image Pointer Parameter is NULL.\n"));
      status = EFI_INVALID_PARAMETER;
      goto cleanup;
    }

    //
    // Check to make sure index is 1 (only 1 image for this device)
    //
    if (ImageIndex != 1)
    {
        DEBUG((DEBUG_ERROR, "GetImage - Image Index Invalid.\n"));
        status = EFI_INVALID_PARAMETER;
        goto cleanup;
    }


    status = FmpDeviceGetImage(Image, ImageSize);
cleanup:    
    
    return status;
}//GetImage()


/**
  Helper function to safely retrieve the FMP header from
  within an EFI_FIRMWARE_IMAGE_AUTHENTICATION structure.

  @param[in]  Image        Pointer to the image.
  @param[in]  ImageSize    Size of the image.
  @param[out] PayloadSize  

  @retval     !NULL     Valid pointer to the header.
  @retval     NULL      Structure is bad and pointer cannot be found.

**/
VOID*
GetFmpHeader (
  IN  CONST EFI_FIRMWARE_IMAGE_AUTHENTICATION    *Image,
  IN  CONST UINTN                                ImageSize,
  OUT       UINTN                                *PayloadSize
  )
{
  // Check to make sure that operation can be safely performed.
  if (((UINTN)Image + sizeof(Image->MonotonicCount) + Image->AuthInfo.Hdr.dwLength) < (UINTN)Image || \
      ((UINTN)Image + sizeof(Image->MonotonicCount) + Image->AuthInfo.Hdr.dwLength) >= (UINTN)Image + ImageSize)
  {
    return NULL;    // Pointer overflow. Invalid image.
  }

  *PayloadSize = ImageSize - (sizeof(Image->MonotonicCount) + Image->AuthInfo.Hdr.dwLength);
  return (VOID*)((UINT8*)Image + sizeof(Image->MonotonicCount) + Image->AuthInfo.Hdr.dwLength);
} // GetFmpHeader()


/**
  Helper function to safely calculate the size of all headers
  within an EFI_FIRMWARE_IMAGE_AUTHENTICATION structure.

  @param[in]  Image                   Pointer to the image.
  @param[in]  AdditionalHeaderSize    Size of any headers that cannot be calculated by this function.

  @retval     UINT32>0   Valid size of all the headers.
  @retval     0         Structure is bad and size cannot be found.

**/
UINT32
GetAllHeaderSize (
  IN  CONST EFI_FIRMWARE_IMAGE_AUTHENTICATION     *Image,
  IN  UINT32                                      AdditionalHeaderSize
  )
{
  UINT32 CalculatedSize = sizeof(Image->MonotonicCount) + AdditionalHeaderSize + Image->AuthInfo.Hdr.dwLength;

  // Check to make sure that operation can be safely performed.
  if (CalculatedSize < sizeof(Image->MonotonicCount) ||
      CalculatedSize < AdditionalHeaderSize ||
      CalculatedSize < Image->AuthInfo.Hdr.dwLength)
  {
    return 0;    // Integer overflow. Invalid image.
  }

  return CalculatedSize;
} // GetAllHeaderSize()


EFI_STATUS
CheckTheImage(
IN  EFI_FIRMWARE_MANAGEMENT_PROTOCOL  *This,
IN  UINT8                             ImageIndex,
IN  CONST VOID                        *Image,
IN  UINTN                             ImageSize,
OUT UINT32                            *ImageUpdateable
)
/*++

Routine Description:

Checks to see if the firmware image is valid for the device.

Arguments:

This            -- This instance of the protocol.
ImageIndex      -- A unique number identifying the firmware image(s).
This number is between 1 and DescriptorCount.
Image           -- Points to the new image.
ImageSize       -- Size of the Image buffer.
ImageUpdateable -- Valid?  One of:
IMAGE_UPDATABLE_VALID
IMAGE_UPDATABLE_INVALID
IMAGE_UPDATABLE_INVALID_TYPE
IMAGE_UPDATABLE_INVALID_OLD

Return Value:

EFI_STATUS code.

--*/

{
  EFI_STATUS status = EFI_SUCCESS;
  UINTN rawsize = 0;
  VOID* FmpPayloadHeader = NULL;
  UINTN FmpPayloadSize = 0;
  UINT32 Version = 0;
  UINT32 MsFmpHeaderSize = 0;
  UINTN AllHeaderSize = 0;
  UINT32 Index;

  //
  // make sure the descriptor has already been loaded
  //
  if (!mDescriptorPopulated)
  {
    PopulateDescriptor();
  }


  if (ImageUpdateable == NULL)
  {
    DEBUG((DEBUG_ERROR, "CheckImage - ImageUpdateable Pointer Parameter is NULL.\n"));
    status = EFI_INVALID_PARAMETER;
    goto cleanup;
  }

  //
  //Set to valid and then if any tests fail it will update this flag. 
  //
  *ImageUpdateable = IMAGE_UPDATABLE_VALID;

  if (Image == NULL)
  {
    DEBUG((DEBUG_ERROR, "CheckImage - Image Pointer Parameter is NULL.\n"));
    *ImageUpdateable = IMAGE_UPDATABLE_INVALID; //not sure if this is needed
    return EFI_INVALID_PARAMETER;
  }

  status = EFI_ABORTED;

  if (CapsuleVerifyCertificateList.CapsuleVerifyCertificates == NULL) 
  {
    DEBUG((DEBUG_ERROR, "Certificates not found.\n"));
    goto cleanup;
  }

  //
  //Verify Auth Data
  //
  for (Index = 0; Index < CapsuleVerifyCertificateList.NumberOfCertificates; Index++)
  {
    DEBUG((DEBUG_ERROR, "Certificate #%d.\n", (Index + 1)));

    if (CapsuleVerifyCertificateList.CapsuleVerifyCertificates[Index].Key == NULL || \
        CapsuleVerifyCertificateList.CapsuleVerifyCertificates[Index].KeySize == 0) 
    {
      DEBUG((DEBUG_ERROR, "Invalid cerificate, skipping it.\n"));
      continue;
    }

    status = AuthenticateFmpImage(
  	  (EFI_FIRMWARE_IMAGE_AUTHENTICATION*)Image,
  	  ImageSize,
      CapsuleVerifyCertificateList.CapsuleVerifyCertificates[Index].Key, 
      CapsuleVerifyCertificateList.CapsuleVerifyCertificates[Index].KeySize
    );
    if (!EFI_ERROR(status))
    {
      break;
    }
  }

  if (EFI_ERROR(status))
  {
    DEBUG((DEBUG_ERROR, "AuthenticateFmpImage Failed %r.\n", status));
    goto cleanup;
  }

  //
  // Check to make sure index is 1
  //
  if (ImageIndex != 1)
  {
    DEBUG((DEBUG_ERROR, "CheckImage - Image Index Invalid.\n"));
    *ImageUpdateable = IMAGE_UPDATABLE_INVALID_TYPE;
    status = EFI_SUCCESS;
    goto cleanup;
  }


  //Check the MsFmpPayloadHeader 
  FmpPayloadHeader = GetFmpHeader( (EFI_FIRMWARE_IMAGE_AUTHENTICATION*)Image, ImageSize, &FmpPayloadSize );
  if (FmpPayloadHeader == NULL)
  {
    DEBUG((DEBUG_ERROR, "CheckTheImage - GetFmpHeader failed.\n"));
    status = EFI_ABORTED;
    goto cleanup;
  }
  status = GetMsFmpVersion(FmpPayloadHeader, FmpPayloadSize, &Version);
  if (EFI_ERROR(status))
  {
    DEBUG((DEBUG_ERROR, "CheckTheImage - GetMsFmpVersion failed %r.\n", status));
    *ImageUpdateable = IMAGE_UPDATABLE_INVALID;
    status = EFI_SUCCESS;
    goto cleanup;
  }

  //
  // Check the lowest supported version
  //
  if (Version < mDesc.LowestSupportedImageVersion)
  {
    DEBUG((DEBUG_ERROR, "CheckTheImage - Version Lower than lowest supported version. 0x%08X < 0x%08X\n",
            Version, mDesc.LowestSupportedImageVersion));
    *ImageUpdateable = IMAGE_UPDATABLE_INVALID_OLD;
    status = EFI_SUCCESS;
    goto cleanup;
  }

  //
  // Get the MsFmpHeaderSize so we can determine the real payload size
  //
  status = GetMsFmpHeaderSize(FmpPayloadHeader, FmpPayloadSize, &MsFmpHeaderSize);
  if (EFI_ERROR(status))
  {
    DEBUG((DEBUG_ERROR, "CheckTheImage - GetMsFmpHeaderSize failed %r.\n", status));
    *ImageUpdateable = IMAGE_UPDATABLE_INVALID;
    status = EFI_SUCCESS;
    goto cleanup;
  }

  //
  // Call FmpDevice Lib Check Image on the 
  // Raw payload.  So all headers need stripped off
  //
  AllHeaderSize = GetAllHeaderSize( (EFI_FIRMWARE_IMAGE_AUTHENTICATION*)Image, MsFmpHeaderSize );
  if (AllHeaderSize == 0)
  {
    DEBUG((DEBUG_ERROR, "CheckTheImage - GetAllHeaderSize failed.\n"));
    status = EFI_ABORTED;
    goto cleanup;
  }
  rawsize = ImageSize - AllHeaderSize;

  //
  // FmpDeviceLib CheckImage function to do any specific checks
  //
  status = FmpDeviceCheckImage((((UINT8*)Image) + AllHeaderSize), rawsize, ImageUpdateable);
  if (EFI_ERROR(status))
  {
    DEBUG((DEBUG_ERROR, "CheckTheImage - FmpDeviceLib CheckImage failed. status = %r\n", status));
  }

cleanup:
  return status;
}// CheckImage()


EFI_STATUS
SetTheImage(
IN  EFI_FIRMWARE_MANAGEMENT_PROTOCOL                 *This,
IN  UINT8                                            ImageIndex,
IN  CONST VOID                                       *Image,
IN  UINTN                                            ImageSize,
IN  CONST VOID                                       *VendorCode,
IN  EFI_FIRMWARE_MANAGEMENT_UPDATE_IMAGE_PROGRESS    Progress,
OUT CHAR16                                           **AbortReason
)
/*++

Routine Description:

    Function to update the firmware.

Arguments:

    This          -- This instance of the protocol.
    ImageIndex    -- A unique number identifying the firmware image(s).
                     This number is between 1 and DescriptorCount.
    Image         -- Points to the new image.
    ImageSize     -- Size of the Image buffer.
    VendorCode    -- This enables vendor to implement vendor-specific
                     image update policies. NULL == use default.
    Progress      -- Callback for letting client know the progress.
    AbortReason   -- String that specifies an error.  Caller is responsible
                     to free the memory with FreePool().

Return Value:

    EFI_STATUS code.  

--*/
{
    EFI_STATUS status                         = EFI_SUCCESS;
    UINT32 Updateable                         = 0;
    BOOLEAN BooleanValue                      = FALSE;
    UINT32 MsFmpHeaderSize                    = 0;
    VOID*  MsFmpHeader                        = NULL;
    UINTN  FmpPayloadSize                     = 0;
    UINT32 AllHeaderSize                      = 0;
    UINT32 IncommingFwVersion                 = 0;
    UINT32 LastAttemptStatus = LAST_ATTEMPT_STATUS_ERROR_UNSUCCESSFUL;

    SetLastAttemptVersionInVariable(IncommingFwVersion);  //set to 0 to clear any previous results. 

    //
    // if we have locked the device - don't pass thru.  
    // it should be blocked by hardware too but we can catch here even faster
    //
    //
    if (mFmpDeviceLocked)
    {
      DEBUG((DEBUG_ERROR, "SetTheImage - Device is already locked.  Can't update.\n"));
      status = EFI_ACCESS_DENIED;
      goto cleanup;
    }

    //
    //call check image to verify the image
    //
    status = CheckTheImage(This, ImageIndex, Image, ImageSize, &Updateable);
    if (EFI_ERROR(status))
    {
        DEBUG((DEBUG_ERROR, "SetTheImage - Check The Image failed with %r.\n", status));
        if (status == EFI_SECURITY_VIOLATION)
        {
          LastAttemptStatus = LAST_ATTEMPT_STATUS_ERROR_AUTH_ERROR;
        }
        goto cleanup;
    }

    //
    //since no functional error in CheckTheImage we can try to get the Version to support better error reporting. 
    //
    MsFmpHeader = GetFmpHeader( (EFI_FIRMWARE_IMAGE_AUTHENTICATION*)Image, ImageSize, &FmpPayloadSize );
    if (MsFmpHeader == NULL)
    {
      DEBUG((DEBUG_ERROR, "SetTheImage - GetFmpHeader failed.\n"));
      status = EFI_ABORTED;
      goto cleanup;
    }
    status = GetMsFmpVersion(MsFmpHeader, FmpPayloadSize, &IncommingFwVersion);
    if (!EFI_ERROR(status))
    {
      SetLastAttemptVersionInVariable(IncommingFwVersion);  //set to actual value
    }

    
    if (Updateable != IMAGE_UPDATABLE_VALID)
    {
        DEBUG((DEBUG_ERROR, "SetTheImage - Check The Image returned that the Image was not valid for update.  Updatable value = 0x%X.\n", Updateable));
        status = EFI_ABORTED;
        goto cleanup;
    }

    if (Progress == NULL)
    {
        DEBUG((DEBUG_ERROR, "SetTheImage - Invalid progress callback\n"));
        status = EFI_INVALID_PARAMETER;
        goto cleanup;
    }

    mProgressFunc = Progress;
    mProgressSupported = TRUE;
    
    //Unique to Implemenation: the lowest 8bits for percentage...the next 24bits for color.  Only matters on first call.  
    //ignore the upper 8 bits of the color...those are not used anyway. 
    status = Progress((PcdGet32(PcdProgressColor) << 8) + 1);  //checking the image is at least 1%  
    if (EFI_ERROR(status))
    {
        DEBUG((DEBUG_ERROR, "SetTheImage - Progress Callback failed with status %r.\n", status));
    }

    //
    //Check System Power
    //
    status = CheckSystemPower(&BooleanValue);
    if (EFI_ERROR(status))
    {
      DEBUG((DEBUG_ERROR, "CheckSystemPower - API call failed %r.\n", status));
      goto cleanup;
    }
    if (!BooleanValue)
    {
      status = EFI_ABORTED;
      DEBUG((DEBUG_ERROR, "CheckSystemPower - returned False.  Update not allowed due to System Power.\n"));
      LastAttemptStatus = LAST_ATTEMPT_STATUS_ERROR_PWR_EVT_BATT;
      goto cleanup;
    }

    Progress(2);

    //
    //Check System Thermal
    //
    status = CheckSystemThermal(&BooleanValue);
    if (EFI_ERROR(status))
    {
      DEBUG((DEBUG_ERROR, "CheckSystemThermal - API call failed %r.\n", status));
      goto cleanup;
    }
    if (!BooleanValue)
    {
      status = EFI_ABORTED;
      DEBUG((DEBUG_ERROR, "CheckSystemThermal - returned False.  Update not allowed due to System Thermal.\n"));
      goto cleanup;
    }

    Progress(3);

    //
    //Check System Env
    //
    status = CheckSystemEnvironment(&BooleanValue);
    if (EFI_ERROR(status))
    {
      DEBUG((DEBUG_ERROR, "CheckSystemEnvironment - API call failed %r.\n", status));
      goto cleanup;
    }
    if (!BooleanValue)
    {
      status = EFI_ABORTED;
      DEBUG((DEBUG_ERROR, "CheckSystemEnvironment - returned False.  Update not allowed due to System Env.\n"));
      goto cleanup;
    }

    Progress(4);

    // Save LastAttemptStatus as error so that if SetImage never returns we have error state recorded. 
    SetLastAttemptStatusInVariable(LastAttemptStatus);

    //
    //strip off all the headers so the device can process its firmware
    //
    status = GetMsFmpHeaderSize(MsFmpHeader, FmpPayloadSize, &MsFmpHeaderSize);
    if (EFI_ERROR(status))
    {
      DEBUG((DEBUG_ERROR, "GetMsFmpHeaderSize failed %r.\n", status));
      goto cleanup;
    }

    AllHeaderSize = GetAllHeaderSize( (EFI_FIRMWARE_IMAGE_AUTHENTICATION*)Image, MsFmpHeaderSize );
    if (AllHeaderSize == 0)
    {
      DEBUG((DEBUG_ERROR, "GetAllHeaderSize failed.\n"));
      status = EFI_ABORTED;
      goto cleanup;
    }

    Progress(5);  //indicate we are done and handing off to FmpDeviceLib

    //
    //Copy the requested image to the firmware using the FmpDeviceLib
    //
    status = FmpDeviceSetImage((((UINT8*)Image) + AllHeaderSize), ImageSize - AllHeaderSize, VendorCode, DeviceLibWrapperProgress,IncommingFwVersion, AbortReason);
    if (EFI_ERROR(status))
    {
      DEBUG((DEBUG_ERROR, "SetImage from FmpDeviceLib failed. status =  %r.\n", status));
      goto cleanup;
    }


    //finished the update without error
    Progress(99);  //indicate we are back from FMP Device Lib

    //
    //Update the version stored in variable
    //
    if (!mRuntimeVersionSupported)
    {
      UINT32 Version = DEFAULT_VERSION;
      GetMsFmpVersion(MsFmpHeader, FmpPayloadSize, &Version);
      SetVersionInVariable(Version);
    }

    //
    //update lowest supported variable
    //
    {
      UINT32 Version = DEFAULT_LOWESTSUPPORTEDVERSION;
      GetMsFmpLowestSupportedVersion(MsFmpHeader, FmpPayloadSize, &Version);
      SetLowestSupportedVersionInVariable(Version);
    }

    LastAttemptStatus = LAST_ATTEMPT_STATUS_SUCCESS;

    //
    //set flag so we repopulate 
    //--only for devices that don't require system reboot
    if (! PcdGetBool(PcdDeviceLibWrapperSystemResetRequired)) {
      mDescriptorPopulated = FALSE;
    }
    
  cleanup:
    mProgressFunc = NULL;
    mProgressSupported = FALSE;
    SetLastAttemptStatusInVariable(LastAttemptStatus);
    //set progress to 100 after everything is done including recording status. 
    Progress(100);
    return status;
}// SetImage()


EFI_STATUS
GetPackageInfo(
IN  EFI_FIRMWARE_MANAGEMENT_PROTOCOL *This,
OUT UINT32                           *PackageVersion,
OUT CHAR16                           **PackageVersionName,
OUT UINT32                           *PackageVersionNameMaxLen,
OUT UINT64                           *AttributesSupported,
OUT UINT64                           *AttributesSetting
)
{

    return EFI_UNSUPPORTED;
}


EFI_STATUS
SetPackageInfo(
IN  EFI_FIRMWARE_MANAGEMENT_PROTOCOL   *This,
IN  CONST VOID                         *Image,
IN  UINTN                              ImageSize,
IN  CONST VOID                         *VendorCode,
IN  UINT32                             PackageVersion,
IN  CONST CHAR16                       *PackageVersionName
)
{

    return EFI_UNSUPPORTED;
}


VOID
EFIAPI
DeviceLibWrapperDeviceLibLockEventNotify(
IN EFI_EVENT        Event,
IN VOID             *Context
)
{
  if (!mFmpDeviceLocked) {
    if (LockFmpDeviceOnReadyToBoot() == TRUE)
    {
      EFI_STATUS Status = FmpDeviceLock();
      if (EFI_ERROR(Status))
      {
        if (Status != EFI_UNSUPPORTED)
        {
          DEBUG((DEBUG_ERROR, "FmpDeviceLib returned error from FmpDeviceLock().  Status = %r\n", Status));
        }
        else 
        {
          DEBUG((DEBUG_WARN, "FmpDeviceLib returned error from FmpDeviceLock().  Status = %r\n", Status));
        }
      }
      mFmpDeviceLocked = TRUE;
    }
    else
    {
      DEBUG((DEBUG_VERBOSE, "FmpDeviceLib - Not calling lib for lock because mfg mode\n"));
    }
  }  
}



/** 
  Function to install FMP instance. 

  @param[in]  Handle    			The device handle to install a FMP instance on.

  @retval EFI_SUCCESS       		FMP Installed
  @retval EFI_INVALID_PARAMETER     Handle was invalid
  @retval other             		Error installing FMP

**/
EFI_STATUS
EFIAPI
InstallFmpInstance(IN EFI_HANDLE  Handle)
{
  EFI_STATUS status = EFI_SUCCESS;
  EFI_FIRMWARE_MANAGEMENT_PROTOCOL *  Fmp = NULL;
  //Allocate FMP structure

  //This wrapper only supports installing on a single device
  if (mFmpInstalled)
  {
    return EFI_ALREADY_STARTED;
  }


  Fmp = AllocateZeroPool(sizeof(EFI_FIRMWARE_MANAGEMENT_PROTOCOL));
  if (Fmp == NULL) 
  {
      DEBUG((DEBUG_ERROR, "Failed to allocate memory for the Device Lib Wrapper FMP.\n"));
      status = EFI_OUT_OF_RESOURCES;
      goto cleanup;
  }

  //set up the function pointers
  Fmp->GetImageInfo = GetTheImageInfo;
  Fmp->GetImage = GetTheImage;
  Fmp->SetImage = SetTheImage;
  Fmp->CheckImage = CheckTheImage;
  Fmp->GetPackageInfo = GetPackageInfo;
  Fmp->SetPackageInfo = SetPackageInfo;
  
  //install the protocol
  status = gBS->InstallMultipleProtocolInterfaces (
                    &Handle,
                    &gEfiFirmwareManagementProtocolGuid,
                    Fmp,
                    NULL
                    );

  if (EFI_ERROR (status)) 
  {
      DEBUG((DEBUG_ERROR, "Device Lib Wrapper FMP: install protocol error, status = %r.\n", status));
      FreePool(Fmp);
      goto cleanup;
  }

  DEBUG((DEBUG_INFO, "Device Lib Wrapper FMP: FMP Protocol Installed!\n"));
  mFmpInstalled = TRUE;

cleanup:

  return status;
}// InstallFmpInstance()

/**
  Main entry for this library.

  @param ImageHandle     Image handle this driver.
  @param SystemTable     Pointer to SystemTable.

**/
EFI_STATUS
EFIAPI
FmpWrapperDeviceLibInit (
  IN EFI_HANDLE                   ImageHandle,
  IN EFI_SYSTEM_TABLE             *SystemTable
  )
{
  EFI_STATUS status = EFI_SUCCESS;
  
  //
  //Register with library the install function so if the library uses
  //UEFI driver model/driver binding protocol it can install FMP on its device handle
  // If library is simple lib that does not use driver binding then it should return 
  // unsupported and this will install the FMP instance on the ImageHandle
  //
  status = RegisterFmpInstaller(InstallFmpInstance);
  if(status == EFI_UNSUPPORTED){
    DEBUG((DEBUG_INFO, "Fmp Device Lib returned unsupported for Register Function.  Installing single instance of FMP.\n"));
	  status = InstallFmpInstance(ImageHandle);
  }
  else if(EFI_ERROR(status)) {
	  DEBUG((DEBUG_ERROR, "Fmp Device Lib returned error on Register.  No FMP installed.  Status = %r\n", status));
  } 
  else 
  {
    DEBUG((DEBUG_INFO, "Fmp Device Lib Register returned success.  Expect FMP to be installed during the BDS/Device connection phase.\n"));
  }

  //If the boot mode isn't flash update then we must lock all vars.  
  //If it is flash update leave them unlocked as the sytem will not
  //boot all the way in flash update mode
  if (BOOT_ON_FLASH_UPDATE != GetBootModeHob())
  {
    LockAllVars();
  }


  //
  // Register notify function to Lock Device on ReadyToBoot Event.
  //
  status = gBS->CreateEventEx(
    EVT_NOTIFY_SIGNAL,
    TPL_CALLBACK,
    DeviceLibWrapperDeviceLibLockEventNotify,
    NULL,
    &gEfiEventReadyToBootGuid,
    &mDeviceLibLockFwEvent
    );

  ASSERT_EFI_ERROR(status);
  if (EFI_ERROR(status))
  {
    DEBUG((DEBUG_ERROR, "DeviceLibWrapperFMP Failed to register for ready to boot.  Status = %r\n", status));
  }

  return status;
}


