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


#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/FirmwareManagement.h>
#include <Library/DebugLib.h>
#include <Library/FmpHelperLib.h>
#include <Guid/ImageAuthentication.h>

/**
Locate all FMP instances installed in the system and return them in
a null terminated list.  Caller must free the list when finished.

@param[out] EFI_FIRMWARE_MANAGEMENT_PROTOCOL*** List of protocol instances

@retval Status
**/
EFI_STATUS
EFIAPI
GetAllFmp(
OUT EFI_FIRMWARE_MANAGEMENT_PROTOCOL*** FmpList
)
{
	EFI_STATUS Status;
	EFI_HANDLE                                    *HandleBuffer;
	EFI_FIRMWARE_MANAGEMENT_PROTOCOL              *Fmp;
	UINTN                                         NumberOfHandles;
	UINTN										  Index1;
	EFI_FIRMWARE_MANAGEMENT_PROTOCOL			  **List;


	List = NULL;

	if (FmpList == NULL)
	{
		return EFI_INVALID_PARAMETER;
	}


	//Get all handles that have FMP
	Status = gBS->LocateHandleBuffer(
		ByProtocol,
		&gEfiFirmwareManagementProtocolGuid,
		NULL,
		&NumberOfHandles,
		&HandleBuffer
		);

	if (!EFI_ERROR(Status) && (NumberOfHandles > 0)) {

		*FmpList = AllocateZeroPool((NumberOfHandles + 1) * sizeof(EFI_FIRMWARE_MANAGEMENT_PROTOCOL*));
		if (*FmpList == NULL)
		{
			if (HandleBuffer != NULL) {
				FreePool(HandleBuffer);
			}
			return EFI_OUT_OF_RESOURCES;
		}
		//loop thru all handles getting the FMP instance
		List = *FmpList;  //use temp pointer so when we return *FmpList is pointing to head of list.

		for (Index1 = 0; Index1 < NumberOfHandles; Index1++) {
			Status = gBS->HandleProtocol(
				HandleBuffer[Index1],
				&gEfiFirmwareManagementProtocolGuid,
				(VOID **)&Fmp
				);
			if (EFI_ERROR(Status)) {
				DEBUG((DEBUG_ERROR, "Failed to get FMP for a handle 0x%x\n", HandleBuffer[Index1]));
				continue;
			}

			*List = Fmp;
			List += 1; //increment fmpList
		}

		Status = EFI_SUCCESS;
	}

	if (HandleBuffer != NULL)
	{
		FreePool(HandleBuffer);
	}

	return Status;
}


/**
Locate a FMP protocol instance with a descriptor matching
the input parameters.  On success this function allocates
memory for the ImageDescriptor (the caller must free) and
will set the pointer to the FMP.

@param[in] EFI_GUID		 The guid of the FMP instance
@param[in] UINT8         The ImageIndex that matches the descriptor
@param[in, optional] EFI_FIRMWARE_MANAGEMENT_PROTOCOL** NULL terminated list of Fmp Instances to ignore. 
@param[out] EFI_FIRMWARE_IMAGE_DESCRIPTOR**  Descriptor that matches
@param[out optional] EFI_FIRMWARE_MANAGEMENT_PROTOCOL** Protocol instance.  If NULL it will not be returned.


@retval Status			 Success when descriptor and fmp found.
@retval Error		     Not found or other error condition
**/
EFI_STATUS
EFIAPI
GetFmpAndDescriptor(
IN EFI_GUID*								FmpGuid,
IN UINT8									ImageIndex,
IN OPTIONAL EFI_FIRMWARE_MANAGEMENT_PROTOCOL**		ExcludeFmp,
OUT EFI_FIRMWARE_IMAGE_DESCRIPTOR**			Descriptor,
OUT OPTIONAL EFI_FIRMWARE_MANAGEMENT_PROTOCOL**		FmpInstance
)
{
	EFI_STATUS									  Status;
	EFI_FIRMWARE_MANAGEMENT_PROTOCOL**			  FmpList; //List of pointers to fmp
	EFI_FIRMWARE_MANAGEMENT_PROTOCOL**			  Fmp;		//interator for list
	UINTN                                         DescriptorSize;
	EFI_FIRMWARE_IMAGE_DESCRIPTOR                 *FmpImageInfoBuf;
	EFI_FIRMWARE_IMAGE_DESCRIPTOR                 *FmpImageInfoBufOrg;
	UINT8                                         FmpImageInfoCount;
	UINT32                                        FmpImageInfoDescriptorVer;
	UINTN                                         ImageInfoSize;
	UINT32                                        PackageVersion;
	CHAR16                                        *PackageVersionName;
	BOOLEAN										  Found;


	FmpList = NULL;
	FmpImageInfoBufOrg = NULL;
	Found = FALSE;
	Status = GetAllFmp(&FmpList);

	if (EFI_ERROR(Status))
	{
		DEBUG((DEBUG_ERROR, "GetFmpAndDescriptor - Failed to Locate FMP instances.  Status = %R\n", Status));
		return Status;
	}

	//loop thru all the FMP instances
	//Loop thru list of pointers until NULL
	for (Fmp = FmpList; ((*Fmp != NULL) && (!Found)); Fmp++)
	{
		//don't process if this FMP is in the exclude list
		if (ExcludeFmp != NULL) {
			BOOLEAN Exclude = FALSE;
			EFI_FIRMWARE_MANAGEMENT_PROTOCOL** tfmp = ExcludeFmp;
			while (*tfmp != NULL) {
				if (*tfmp == *Fmp) {
					//if they point to the same FMP instance
					Exclude = TRUE;
					DEBUG((DEBUG_INFO, "GetFmpAndDescriptor - Ignoring an instance of FMP.\n"));
					break;
				}
				//go to next in list
				tfmp++;
			}
			if (Exclude)
			{
				//go to next fmp as we should not process this one
				continue;
			}
		}


		//get the GetImageInfo for the FMP
		ImageInfoSize = 0;
		//
		// get necessary descriptor size
		// this should return TOO SMALL
		Status = (*Fmp)->GetImageInfo(
			(*Fmp),						  // FMP Pointer
			&ImageInfoSize,				  // Buffer Size (in this case 0)
			NULL,						  // NULL so we can get size
			&FmpImageInfoDescriptorVer,   // DescriptorVersion
			&FmpImageInfoCount,           // DescriptorCount
			&DescriptorSize,              // DescriptorSize
			&PackageVersion,              // PackageVersion
			&PackageVersionName           // PackageVersionName
			);

		if (Status != EFI_BUFFER_TOO_SMALL) {
			DEBUG((DEBUG_ERROR, "Unexpected Failure in GetImageInfo.  Status = %r\n", Status));
			continue;
		}

		FmpImageInfoBuf = NULL;
		FmpImageInfoBuf = AllocateZeroPool(ImageInfoSize);
		if (FmpImageInfoBuf == NULL) {
			Status = EFI_OUT_OF_RESOURCES;
			DEBUG((DEBUG_ERROR, "Failed to get memory for descriptors.\n"));
			goto GetFmpAndDescriptorCleanUp;
		}

		FmpImageInfoBufOrg = FmpImageInfoBuf;
		PackageVersionName = NULL;
		Status = (*Fmp)->GetImageInfo(
			(*Fmp),
			&ImageInfoSize,               // ImageInfoSize
			FmpImageInfoBuf,              // ImageInfo
			&FmpImageInfoDescriptorVer,   // DescriptorVersion
			&FmpImageInfoCount,           // DescriptorCount
			&DescriptorSize,              // DescriptorSize
			&PackageVersion,              // PackageVersion
			&PackageVersionName           // PackageVersionName
			);

		if (EFI_ERROR(Status)) {
			DEBUG((DEBUG_ERROR, "Failure in GetImageInfo.  Status = %r\n", Status));
			goto GetFmpAndDescriptorCleanUp;
		}

		//check each descriptor and read from the one specified
		while (FmpImageInfoCount > 0) {
			if (CompareGuid(&FmpImageInfoBuf->ImageTypeId, FmpGuid))
			{
				//correct image...now check index
				DEBUG((DEBUG_INFO, "Found FMP for reading.\n"));

				//check index
				if (FmpImageInfoBuf->ImageIndex == ImageIndex)
				{
					//Found our Entry.  
					//Set return values
          if (FmpInstance != NULL) {
            *FmpInstance = *Fmp;
          }

					*Descriptor = (EFI_FIRMWARE_IMAGE_DESCRIPTOR*)AllocatePool(DescriptorSize);
					CopyMem(*Descriptor, FmpImageInfoBuf, DescriptorSize);  //copy this descriptor into memory buffer to return.
					Status = EFI_SUCCESS;
					Found = TRUE;
					break;  //break from the while loop

				} //index doesn't match
			}

			FmpImageInfoCount--;
			FmpImageInfoBuf = (EFI_FIRMWARE_IMAGE_DESCRIPTOR*)(((UINT8 *)FmpImageInfoBuf) + DescriptorSize);  //increment the buffer pointer ahead by the size of the descriptor
		} //close while loop

		//clean up - since we used Buf to move around inside the buffer we must free Org. 
		FreePool(FmpImageInfoBufOrg);
		FmpImageInfoBuf = NULL;
		FmpImageInfoBufOrg = NULL;
		if (PackageVersionName != NULL)
		{
			FreePool(PackageVersionName);
			PackageVersionName = NULL;
		}
	} //for loop for all FMP

GetFmpAndDescriptorCleanUp:
	//Free up the FmpList of pointers
	if (FmpList != NULL) { FreePool(FmpList); }
	if (FmpImageInfoBufOrg != NULL) { FreePool(FmpImageInfoBufOrg); }

  if (!Found && !EFI_ERROR(Status))
  {
    //no other error in function but didn't find fmp instance
    Status = EFI_NOT_FOUND;
  }
	return Status;
} //close func


