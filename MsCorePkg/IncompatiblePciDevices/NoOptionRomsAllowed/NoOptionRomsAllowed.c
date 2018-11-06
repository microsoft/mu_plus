/**@file
NoOptionRomsAllowed.c

Implements the IncompatiblePciDevices protocol to disable loading *ALL* PCI option ROMS
from the device ROM BAR.  This still allows the PciPlatformProtocol to provide an
option ROM from the UEFI Image.

Copyright (c) 2018, Microsoft Corporation

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

#include <IndustryStandard/Acpi10.h>

#include <Protocol/IncompatiblePciDeviceSupport.h>

#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>

EFI_STATUS
EFIAPI
NoRomsCheckDevice (
    IN  EFI_INCOMPATIBLE_PCI_DEVICE_SUPPORT_PROTOCOL *This,
    IN  UINTN                                         VendorId,
    IN  UINTN                                         DeviceId,
    IN  UINTN                                         RevisionId,
    IN  UINTN                                         SubsystemVendorId,
    IN  UINTN                                         SubsystemDeviceId,
    OUT VOID                                        **Configuration);

static EFI_INCOMPATIBLE_PCI_DEVICE_SUPPORT_PROTOCOL mNoRomsProtocol = {NoRomsCheckDevice};

typedef struct {
    EFI_ACPI_QWORD_ADDRESS_SPACE_DESCRIPTOR AddrDescriptor;
    EFI_ACPI_END_TAG_DESCRIPTOR             AddrEnd;
} NO_ROMS_CONFIGURATION;

//
//  Configuration to ignore the option ROM for a device.
//
static NO_ROMS_CONFIGURATION mNoRomsConfiguration = {
    // EFI_ACPI_QWORD_ADDRESS_SPACE_DESCRIPTOR
    {
       //   ACPI_LARGE_RESOURCE_HEADER
       {ACPI_QWORD_ADDRESS_SPACE_DESCRIPTOR, sizeof(EFI_ACPI_QWORD_ADDRESS_SPACE_DESCRIPTOR)},
       //   rest of EFI_ACPI_QWORD_ADDRESS_SPACE_DESCRIPTOR
        INCOMPATIBLE_ACPI_ADDRESS_SPACE_TYPE_ROM, // ResType;
        0,                                        // GenFlag;
        0,                                        // SpecificFlag;
        0,                                        // AddrSpaceGranularity;
        0,                                        // AddrRangeMin;
        0,                                        // AddrRangeMax;
        0,                                        // AddrTranslationOffset;
        0                                         // AddrLen;
    },
    // ACPI_END_TAG
    { ACPI_END_TAG_DESCRIPTOR , 0 }
};

/**
 * Incompatible Pci Device Support Protocol CheckDevice
 *
 * This function forces the PciBusDxe driver to ignore all
 * PCI devices Option Rom BAR.
 *
 *
 * @param This
 * @param VendorId
 * @param DeviceId
 * @param RevisionId
 * @param SubsystemVendorId
 * @param SubsystemDeviceId
 * @param Configuration
 *
 * @return EFI_STATUS EFIAPI
 */
EFI_STATUS
EFIAPI
NoRomsCheckDevice (
    IN  EFI_INCOMPATIBLE_PCI_DEVICE_SUPPORT_PROTOCOL *This,
    IN  UINTN                                         VendorId,
    IN  UINTN                                         DeviceId,
    IN  UINTN                                         RevisionId,
    IN  UINTN                                         SubsystemVendorId,
    IN  UINTN                                         SubsystemDeviceId,
    OUT VOID                                        **Configuration) {

     NO_ROMS_CONFIGURATION *NoRomsConfiguration;

     NoRomsConfiguration = AllocateCopyPool (sizeof(mNoRomsConfiguration), &mNoRomsConfiguration);

     if (NULL == NoRomsConfiguration) {
         DEBUG((DEBUG_ERROR,"%a unable to allocate memory for configuration\n",__FUNCTION__));
     }

     *Configuration = NoRomsConfiguration;
     return EFI_SUCCESS;
}

/**
  The user Entry Point for Application. Publish the Icompatible Pci Device
  protocol.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
NoOptionRomsAllowedEntry(
    IN EFI_HANDLE        ImageHandle,
    IN EFI_SYSTEM_TABLE  *SystemTable
    ) {

    EFI_STATUS  Status;

    DEBUG((DEBUG_INFO,"%a Protocol installer\n",__FUNCTION__));

    Status = gBS->InstallProtocolInterface (&ImageHandle,
                                            &gEfiIncompatiblePciDeviceSupportProtocolGuid,
                                             EFI_NATIVE_INTERFACE,
                                            &mNoRomsProtocol);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR,"Error installing Incompatible Pci Devices protocol. Code=%r",Status));
        ASSERT(FALSE);
    }

    return EFI_SUCCESS;
}

