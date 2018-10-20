/** @file
 *MsBootOptionsLib  - Ms Extensions to BdsDxe.

Copyright (c) 2017 - 2018, Microsoft Corporation

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

#include <Protocol/FirmwareVolume2.h>
#include <Protocol/LoadedImage.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootManagerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>


#define INTERNAL_UEFI_SHELL_NAME      L"Internal UEFI Shell 2.0"

#define MS_SDD_BOOT                   L"Internal Storage"
#define MS_SDD_BOOT_PARM               "SDD"
#define MS_USB_BOOT                   L"USB Storage"
#define MS_USB_BOOT_PARM               "USB"
#define MS_PXE_BOOT                   L"PXE Network"
#define MS_PXE_BOOT_PARM               "PXE"

/**
 * Constructor
 *
 * @return EFI_STATUS
 */
EFI_STATUS
EFIAPI
MsBootOptionsLibEntry (
    IN EFI_HANDLE ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable
) {

    return EFI_SUCCESS;
}

/**
 * Build a firmware load option using file and parameter
 *
 * @param BootOption
 * @param FwFile
 * @param Parameter
 *
 * @return EFI_STATUS
 */
static
EFI_STATUS
BuildFwLoadOption (
    EFI_BOOT_MANAGER_LOAD_OPTION *BootOption,
    EFI_GUID                     *FwFile,
    CHAR8                        *Parameter
) {
    EFI_STATUS                         Status;
    CHAR16                             *Description;
    UINTN                              DescriptionLength;
    EFI_DEVICE_PATH_PROTOCOL           *DevicePath;
    EFI_LOADED_IMAGE_PROTOCOL          *LoadedImage;
    MEDIA_FW_VOL_FILEPATH_DEVICE_PATH  FileNode;

    Status = GetSectionFromFv (
               FwFile,
               EFI_SECTION_USER_INTERFACE,
               0,
               (VOID **) &Description,
               &DescriptionLength
               );
    if (EFI_ERROR (Status)) {
        Description = NULL;
    }

    EfiInitializeFwVolDevicepathNode (&FileNode, FwFile);
    Status = gBS->HandleProtocol (
                    gImageHandle,
                    &gEfiLoadedImageProtocolGuid,
                    (VOID **) &LoadedImage
                    );
    ASSERT_EFI_ERROR (Status);
    DevicePath = AppendDevicePathNode (
                   DevicePathFromHandle (LoadedImage->DeviceHandle),
                   (EFI_DEVICE_PATH_PROTOCOL *) &FileNode
                   );
    ASSERT (DevicePath != NULL);

    Status = EfiBootManagerInitializeLoadOption (
               BootOption,
               LoadOptionNumberUnassigned,
               LoadOptionTypeBoot,
               LOAD_OPTION_CATEGORY_APP | LOAD_OPTION_ACTIVE | LOAD_OPTION_HIDDEN,
               (Description != NULL) ? Description : L"Boot Manager Menu",
               DevicePath,
               (UINT8 *)Parameter,
               (Parameter != NULL)  ? (UINT32) AsciiStrSize (Parameter) : 0
               );
    ASSERT_EFI_ERROR (Status);
    FreePool (DevicePath);
    if (Description != NULL) {
        FreePool (Description);
    }
    return Status;
}

/**
 * GetDefaultBootApp - the application that implements the default boot order
 *
 * @return EFI_STATUS
 */
EFI_STATUS
EFIAPI
MsBootOptionsLibGetDefaultBootApp (
    IN OUT EFI_BOOT_MANAGER_LOAD_OPTION *BootOption,
    IN     CHAR8                        *Parameter
) {

    return BuildFwLoadOption (BootOption, &gMsBootPolicyFileGuid, Parameter);
}

/**
  Return the boot option corresponding to the Boot Manager Menu.

  @param BootOption    Return a created Boot Manager Menu with the parameter passed
  @param Parameter     The parameter to add to the BootOption

  @retval EFI_SUCCESS   The Boot Manager Menu is successfully returned.
  @retval Status        Return status of gRT->SetVariable (). BootOption still points
                        to the Boot Manager Menu even the Status is not EFI_SUCCESS.
**/
EFI_STATUS
EFIAPI
MsBootOptionsLibGetBootManagerMenu (
    IN OUT EFI_BOOT_MANAGER_LOAD_OPTION *BootOption,
    IN     CHAR8                        *Parameter
) {

    return BuildFwLoadOption (BootOption, PcdGetPtr(PcdBootManagerMenuFile), Parameter);
}

/**
  This function will create a SHELL BootOption to boot.
*/
static
EFI_DEVICE_PATH_PROTOCOL *
CreateShellDevicePath (
    VOID
) {
    UINTN                             FvHandleCount;
    EFI_HANDLE                        *FvHandleBuffer;
    UINTN                             Index;
    EFI_STATUS                        Status;
    EFI_FIRMWARE_VOLUME2_PROTOCOL     *Fv;
    UINTN                             Size;
    UINT32                            AuthenticationStatus;
    EFI_DEVICE_PATH_PROTOCOL          *DevicePath;
    VOID                              *Buffer;

    DevicePath  = NULL;
    Status      = EFI_SUCCESS;

    DEBUG ((DEBUG_INFO, "CreateShellDevicePath\n"));
    gBS->LocateHandleBuffer (
           ByProtocol,
           &gEfiFirmwareVolume2ProtocolGuid,
           NULL,
           &FvHandleCount,
           &FvHandleBuffer
           );

    for (Index = 0; Index < FvHandleCount; Index++) {
        gBS->HandleProtocol (
               FvHandleBuffer[Index],
               &gEfiFirmwareVolume2ProtocolGuid,
               (VOID **) &Fv
               );

        Buffer  = NULL;
        Size    = 0;
        Status  = Fv->ReadSection (
                        Fv,
                        PcdGetPtr(PcdShellFile),
                        EFI_SECTION_PE32,
                        0,
                        &Buffer,
                        &Size,
                        &AuthenticationStatus
                        );
        DEBUG((DEBUG_INFO,"Fv->Read of Internal Shell - Code=%r\n", Status));
        if (EFI_ERROR (Status)) {
            //
            // Skip if no shell file in the FV
            //
            continue;
        } else {
            //
            // Found the shell
            //
            break;
        }
    }

    if (EFI_ERROR (Status)) {
        //
        // No shell present
        //
        if (FvHandleCount) {
            FreePool (FvHandleBuffer);
        }
        return NULL;
    }
    //
    // Build the shell boot option
    //
    DevicePath = DevicePathFromHandle (FvHandleBuffer[Index]);

    if (FvHandleCount) {
        FreePool (FvHandleBuffer);
    }

    return DevicePath;
}

/**
 * Create a boot option
 *
 * @return EFI_STATUS
 */
static
EFI_STATUS
CreateFvBootOption (
    EFI_GUID                     *FileGuid,
    CHAR16                       *Description,
    EFI_BOOT_MANAGER_LOAD_OPTION *BootOption,
    UINT32                       Attributes,
    UINT8                        *OptionalData,    OPTIONAL
    UINT32                       OptionalDataSize
) {
    EFI_STATUS                         Status;
    EFI_DEVICE_PATH_PROTOCOL           *DevicePath;
    EFI_LOADED_IMAGE_PROTOCOL          *LoadedImage;
    MEDIA_FW_VOL_FILEPATH_DEVICE_PATH  FileNode;
    EFI_FIRMWARE_VOLUME2_PROTOCOL      *Fv;
    UINT32                             AuthenticationStatus;
    VOID                               *Buffer;
    UINTN                              Size;

    if ((BootOption == NULL) || (FileGuid == NULL) || (Description == NULL)) {
        return EFI_INVALID_PARAMETER;
    }

    EfiInitializeFwVolDevicepathNode (&FileNode, FileGuid);

    if (!CompareGuid (PcdGetPtr (PcdShellFile), FileGuid)) {
        Status = gBS->HandleProtocol (
                        gImageHandle,
                        &gEfiLoadedImageProtocolGuid,
                        (VOID **) &LoadedImage
                        );
        if (!EFI_ERROR (Status)) {
            Status = gBS->HandleProtocol (
                             LoadedImage->DeviceHandle,
                             &gEfiFirmwareVolume2ProtocolGuid,
                             (VOID **) &Fv
                             );
            if (!EFI_ERROR (Status)) {
                Buffer  = NULL;
                Size    = 0;
                Status  = Fv->ReadSection (
                                Fv,
                                FileGuid,
                                EFI_SECTION_PE32,
                                0,
                                &Buffer,
                                &Size,
                                &AuthenticationStatus
                                );
                if (Buffer != NULL) {
                    FreePool (Buffer);
                }
            }
        }
        if (EFI_ERROR (Status)) {
            return EFI_NOT_FOUND;
        }

        DevicePath = AppendDevicePathNode (
                         DevicePathFromHandle (LoadedImage->DeviceHandle),
                         (EFI_DEVICE_PATH_PROTOCOL *) &FileNode
                         );
    } else {
        DevicePath = CreateShellDevicePath ();
        if (DevicePath == NULL) {
            return EFI_NOT_FOUND;
        }
        DevicePath = AppendDevicePathNode(
                       DevicePath,
                       (EFI_DEVICE_PATH_PROTOCOL *) &FileNode
                       );
    }

    Status = EfiBootManagerInitializeLoadOption (
               BootOption,
               LoadOptionNumberUnassigned,
               LoadOptionTypeBoot,
               Attributes,
               Description,
               DevicePath,
               OptionalData,
               OptionalDataSize
               );
    FreePool (DevicePath);
    return Status;
}

/**
 * Register a boot option
 *
 * @param FileGuid
 * @param Description
 * @param Position
 * @param Attributes
 * @param OptionalData
 * @param OptionalDataSize
 *
 * @return UINTN
 */
static
UINTN
RegisterFvBootOption (
  EFI_GUID                         *FileGuid,
  CHAR16                           *Description,
  UINTN                            Position,
  UINT32                           Attributes,
  UINT8                            *OptionalData,    OPTIONAL
  UINT32                           OptionalDataSize
) {
    EFI_STATUS                     Status;
    UINTN                          OptionIndex;
    EFI_BOOT_MANAGER_LOAD_OPTION   NewOption;
    EFI_BOOT_MANAGER_LOAD_OPTION   *BootOptions;
    UINTN                          BootOptionCount;
    UINTN                          i;

    NewOption.OptionNumber = LoadOptionNumberUnassigned;
    Status = CreateFvBootOption (FileGuid, Description, &NewOption, Attributes, OptionalData, OptionalDataSize);
    if (!EFI_ERROR (Status)) {
        BootOptions = EfiBootManagerGetLoadOptions (&BootOptionCount, LoadOptionTypeBoot);

        OptionIndex = EfiBootManagerFindLoadOption (&NewOption, BootOptions, BootOptionCount);
        if (OptionIndex == -1) {
            NewOption.Attributes ^= LOAD_OPTION_ACTIVE;
            OptionIndex = EfiBootManagerFindLoadOption(&NewOption, BootOptions, BootOptionCount);
            NewOption.Attributes ^= LOAD_OPTION_ACTIVE;
        }

        if (OptionIndex == -1) {
            Status = EfiBootManagerAddLoadOptionVariable (&NewOption, Position);
            DEBUG((DEBUG_INFO,"Added   Boot option as Boot%04x - %s\n",NewOption.OptionNumber,Description));
            ASSERT_EFI_ERROR (Status);
        } else {
            NewOption.OptionNumber = BootOptions[OptionIndex].OptionNumber;
            DEBUG((DEBUG_INFO,"Reusing Boot option as Boot%04x - %s\n",NewOption.OptionNumber,Description));
        }
        EfiBootManagerFreeLoadOption (&NewOption);
        EfiBootManagerFreeLoadOptions (BootOptions, BootOptionCount);
    } else {
        // The Shell is optional.  If the shell cannot be created (due to not in image), then
        // ensure the boot option for INTERNAL SHELL is deleted.
        if (0 == StrCmp (INTERNAL_UEFI_SHELL_NAME ,Description)) {
            BootOptions = EfiBootManagerGetLoadOptions (&BootOptionCount, LoadOptionTypeBoot);
            for (i=0; i<BootOptionCount; i++) {
                if (0 == StrCmp(INTERNAL_UEFI_SHELL_NAME ,BootOptions[i].Description)) {
                    EfiBootManagerDeleteLoadOptionVariable (BootOptions[i].OptionNumber, LoadOptionTypeBoot);
                    DEBUG((DEBUG_INFO,"Deleting Boot option as Boot%04x - %s\n",BootOptions[i].OptionNumber,BootOptions[i].Description));
                }
            }
            EfiBootManagerFreeLoadOptions (BootOptions, BootOptionCount);
        }
    }

    return NewOption.OptionNumber;
}

/**
 * Register Default Boot Options
 *
 * @param
 *
 * @return VOID EFIAPI
 */
VOID
EFIAPI
MsBootOptionsLibRegisterDefaultBootOptions (
  VOID
) {
    DEBUG((DEBUG_INFO,"%a\n",__FUNCTION__));

    RegisterFvBootOption(&gMsBootPolicyFileGuid, MS_SDD_BOOT, (UINTN)-1, LOAD_OPTION_ACTIVE, (UINT8*)MS_SDD_BOOT_PARM, sizeof(MS_SDD_BOOT_PARM));
    RegisterFvBootOption(&gMsBootPolicyFileGuid, MS_USB_BOOT, (UINTN)-1, LOAD_OPTION_ACTIVE, (UINT8*)MS_USB_BOOT_PARM, sizeof(MS_USB_BOOT_PARM));
    RegisterFvBootOption(&gMsBootPolicyFileGuid, MS_PXE_BOOT, (UINTN)-1, LOAD_OPTION_ACTIVE, (UINT8*)MS_PXE_BOOT_PARM, sizeof(MS_PXE_BOOT_PARM));
    RegisterFvBootOption(PcdGetPtr(PcdShellFile),  INTERNAL_UEFI_SHELL_NAME, (UINTN)-1, LOAD_OPTION_ACTIVE, NULL, 0);
}

/**
 * Get default boot options
 *
 * @param OptionCount
 *
 * @return EFI_BOOT_MANAGER_LOAD_OPTION*EFIAPI
 */
EFI_BOOT_MANAGER_LOAD_OPTION *
EFIAPI
MsBootOptionsLibGetDefaultOptions (
    OUT UINTN    *OptionCount
) {
    UINTN                         LocalOptionCount = 4;
    EFI_BOOT_MANAGER_LOAD_OPTION *Option;
    EFI_STATUS                    Status;
    EFI_STATUS                    Status2;

    Option = (EFI_BOOT_MANAGER_LOAD_OPTION *)  AllocateZeroPool (sizeof(EFI_BOOT_MANAGER_LOAD_OPTION) * LocalOptionCount);
    ASSERT (Option != NULL);
    if (Option == NULL) {
        *OptionCount = 0;
        return NULL;
    }

    Status = CreateFvBootOption (&gMsBootPolicyFileGuid, MS_SDD_BOOT, &Option[0], LOAD_OPTION_ACTIVE, (UINT8*)MS_SDD_BOOT_PARM, sizeof(MS_SDD_BOOT_PARM));
    Status |= CreateFvBootOption (&gMsBootPolicyFileGuid, MS_USB_BOOT, &Option[1], LOAD_OPTION_ACTIVE, (UINT8*)MS_USB_BOOT_PARM, sizeof(MS_USB_BOOT_PARM));
    Status |= CreateFvBootOption (&gMsBootPolicyFileGuid, MS_PXE_BOOT, &Option[2], LOAD_OPTION_ACTIVE, (UINT8*)MS_PXE_BOOT_PARM, sizeof(MS_PXE_BOOT_PARM));

    Status2 = CreateFvBootOption (PcdGetPtr(PcdShellFile), INTERNAL_UEFI_SHELL_NAME, &Option[3], LOAD_OPTION_ACTIVE, NULL, 0);
    if (EFI_ERROR(Status2)) {     // The shell is optional.  So, ignore that we cannot create it.
        LocalOptionCount--;
    }

    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "%a Error creating defatult boot options\n", __FUNCTION__));
        FreePool(Option);
        Option = NULL;
        LocalOptionCount = 0;
    }
    *OptionCount = LocalOptionCount;

    return Option;
}
