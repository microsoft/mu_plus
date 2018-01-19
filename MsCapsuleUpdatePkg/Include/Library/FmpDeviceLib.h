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


#ifndef __FMP_DEVICE_LIB__
#define __FMP_DEVICE_LIB__

/** 
  Callback function to install FMP instance.

  @param[in]  Handle    			The device handle to install a FMP instance on.

  @retval EFI_SUCCESS       		FMP Installed
  @retval EFI_INVALID_PARAMETER     Handle was invalid
  @retval other             		Error installing FMP

**/
typedef
EFI_STATUS
(EFIAPI *FMP_DEVICE_LIB_REGISTER_FMP_INSTALLER)(
  IN  EFI_HANDLE      Handle
  );


/**
  Used to pass the FMP install function to this lib.  
  This allows the library to have control of the handle
  that the FMP instance is installed on.  This allows the library 
  to use DriverBinding protocol model to locate its device(s) in the
  system. 

  @param[in] 				Function pointer to FMP install function. 

  @retval EFI_SUCCESS     	Library has saved function pointer and will call function pointer on each DriverBinding Start.
  @retval EFI_UNSUPPORTED 	Library doesn't use driver binding and only supports a single instance. 
  @retval other error     	Error occurred.  Don't install FMP
  
**/
EFI_STATUS
EFIAPI
RegisterFmpInstaller( 
  IN FMP_DEVICE_LIB_REGISTER_FMP_INSTALLER Func 
  );


  
/**
Used to get the size of the image in bytes.  
NOTE - Do not return zero as that will identify the device as 
not updatable.  

@retval UINTN that represents the size of the firmware.  

**/
UINTN
EFIAPI
FmpDeviceGetSize();

/**
Used to return a library supplied guid that will be the ImageTypeId guid of the FMP descriptor.  
This is optional but can be used if at runtime the guid needs to be determined.  

@param  Guid:  Double Guid Ptr that will be updated to point to guid.  This should be from static memory
               and will not be freed.  
@return EFI_UNSUPPORTED: if you library instance doesn't need dynamic guid return this.
@return Error: Any error will cause the wrapper to use the GUID defined by PCD
@return EFI_SUCCESS:  Guid ptr should be updated to point to static memeory which contains a valid guid
**/
EFI_STATUS
EFIAPI
FmpDeviceGetImageTypeIdGuidPtr(
  OUT EFI_GUID** Guid);

/**
Used to get the FMP attributes for this device.
Do not use Authentication.  

@param[out] Supported     Attributes Supported on this platform (See FirmwareManagementProtocol).
@param[out] Setting       Attributes Set for the current running image (See FirmwareManagementProtocol).
**/
VOID
EFIAPI
FmpDeviceGetAttributes(
IN OUT  UINT64* Supported,
IN OUT UINT64* Setting
);

/**
Gets the current Lowest Supported Version.
This is a protection mechanism so that a previous version with known issue is not
applied.

ONLY implement this if your running firmware has a method to return this at runtime.

@param[out] Version           On return this value represents the 
current Lowest Supported Version (in same format as GetVersion).  

@retval EFI_SUCCESS           The Lowest Supported Version was correctly retrieved
@retval EFI_UNSUPPORTED       Device firmware doesn't support reporting LSV
@retval EFI_DEVICE_ERROR      Error occurred when trying to get the LSV
**/
EFI_STATUS
EFIAPI
FmpDeviceGetLowestSupportedVersion(
  OUT UINT32* LowestSupportedVersion
);

/**
Gets the current running version in Unicode string format.  
ONLY implement this if your running firmware has a method to return this at boot time.
Memory must be allocated in BS memory.  String must be NULL terminated. 
Return NULL if error or not implemented.  

@retval Valid Pointer         A function allocated memory buffer in BS memory containing a NULL terminated unicode string representing the version. 
@retval NULL                  An error or unsupported.  
**/
CHAR16*
EFIAPI
FmpDeviceGetVersionString();


/**
Gets the current running version.  
ONLY implement this if your running firmware has a method to return this at runtime.

@param[out] Version           On return this value represents the current running version

@retval EFI_SUCCESS           The version was correctly retrieved
@retval EFI_UNSUPPORTED       Device firmware doesn't support reporting current version
@retval EFI_DEVICE_ERROR      Error occurred when trying to get the version
**/
EFI_STATUS
EFIAPI
FmpDeviceGetVersion(
  OUT UINT32* Version
);

/**
Retrieves a copy of the current firmware image of the device.

This function allows a copy of the current firmware image to be created and saved.
The saved copy could later been used, for example, in firmware image recovery or rollback.

@param[out] Image              Points to the buffer where the current image is copied to.
@param[out] ImageSize          On entry, points to the size of the buffer pointed to by Image, in bytes.
On return, points to the length of the image, in bytes.

@retval EFI_SUCCESS            The device was successfully updated with the new image.
@retval EFI_BUFFER_TOO_SMALL   The buffer specified by ImageSize is too small to hold the
image. The current buffer size needed to hold the image is returned
in ImageSize.
@retval EFI_INVALID_PARAMETER  The Image was NULL.
@retval EFI_NOT_FOUND          The current image is not copied to the buffer.
@retval EFI_UNSUPPORTED        The operation is not supported.

**/
EFI_STATUS
EFIAPI
FmpDeviceGetImage(
IN OUT VOID*  Image,
IN OUT UINTN* ImageSize
);


/**
Checks if the firmware image is valid for the device.

This function allows firmware update application to validate the firmware image without
invoking the SetImage() first.

@param[in]  Image              Points to the new image.
@param[in]  ImageSize          Size of the new image in bytes.
@param[out] ImageUpdatable     Indicates if the new image is valid for update. It also provides,
if available, additional information if the image is invalid.

@retval EFI_SUCCESS            The image was successfully checked.
@retval EFI_INVALID_PARAMETER  The Image was NULL.

**/
EFI_STATUS
EFIAPI
FmpDeviceCheckImage(
IN  CONST VOID                        *Image,
IN  UINTN                             ImageSize,
OUT UINT32                            *ImageUpdateable
);


/**
Updates the firmware image of the device.

This function updates the hardware with the new firmware image.
This function returns EFI_UNSUPPORTED if the firmware image is not updatable.
If the firmware image is updatable, the function should perform the following minimal validations
before proceeding to do the firmware image update.
- Validate the image is a supported image for this device.  The function returns EFI_ABORTED if
the image is unsupported.  The function can optionally provide more detailed information on
why the image is not a supported image.
- Validate the data from VendorCode if not null.  Image validation must be performed before
VendorCode data validation.  VendorCode data is ignored or considered invalid if image
validation failed.  The function returns EFI_ABORTED if the data is invalid.

VendorCode enables vendor to implement vendor-specific firmware image update policy.  Null if
the caller did not specify the policy or use the default policy.  As an example, vendor can implement
a policy to allow an option to force a firmware image update when the abort reason is due to the new
firmware image version is older than the current firmware image version or bad image checksum.
Sensitive operations such as those wiping the entire firmware image and render the device to be
non-functional should be encoded in the image itself rather than passed with the VendorCode.
AbortReason enables vendor to have the option to provide a more detailed description of the abort
reason to the caller.

@param[in]  Image              Points to the new image.
@param[in]  ImageSize          Size of the new image in bytes.
@param[in]  VendorCode         This enables vendor to implement vendor-specific firmware image update policy.
Null indicates the caller did not specify the policy or use the default policy.
@param[in]  Progress           A function used by the driver to report the progress of the firmware update.
@param[in]  CapsuleFwVersion   MSFMPCapsule version of the image
@param[out] AbortReason        A pointer to a pointer to a null-terminated string providing more
details for the aborted operation. The buffer is allocated by this function
with AllocatePool(), and it is the caller's responsibility to free it with a
call to FreePool().

@retval EFI_SUCCESS            The device was successfully updated with the new image.
@retval EFI_ABORTED            The operation is aborted.
@retval EFI_INVALID_PARAMETER  The Image was NULL.
@retval EFI_UNSUPPORTED        The operation is not supported.

**/
EFI_STATUS
EFIAPI
FmpDeviceSetImage(
IN  CONST VOID                                       *Image,
IN  UINTN                                            ImageSize,
IN  CONST VOID                                       *VendorCode,
IN  EFI_FIRMWARE_MANAGEMENT_UPDATE_IMAGE_PROGRESS    Progress,
IN  UINT32                                           CapsuleFwVersion,
OUT CHAR16                                           **AbortReason
);


/**
Device frimware should trigger lock mechanism so that device fw can not be updated or tampered with.
This lock mechanism is generally only cleared by a full system reset (not just sleep state/low power mode)

@retval EFI_SUCCESS           The device was successfully locked.
@retval EFI_UNSUPPORTED       The hardware device/firmware doesn't support locking

**/
EFI_STATUS
EFIAPI
FmpDeviceLock(
);



#endif
