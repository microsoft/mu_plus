/** @file
  Update FACS Hardware Signature library definition. A device can implement
  instances to support device specific behavior.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <DfciSystemSettingTypes.h>

#include <Guid/DfciSettingsManagerVariables.h>

#include <Protocol/AcpiSystemDescriptionTable.h>
#include <Protocol/PciIo.h>

#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/MuUefiVersionLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UpdateFacsHardwareSignatureLib.h>

/**
  This function uses the ACPI SDT protocol to locate an ACPI table.
  It is really only useful for finding tables that only have a single instance,
  e.g. FADT, FACS, MADT, etc.  It is not good for locating SSDT, etc.

  @param[in] Signature           - Pointer to an ASCII string containing the OEM Table ID from the ACPI table header
  @param[in, out] Table          - Updated with a pointer to the table
  @param[in, out] Handle         - AcpiSupport protocol table handle for the table found

  @retval EFI_SUCCESS            - The function completed successfully and the table was found.
  @retval Other                  - Some error occurred or the table was not found.
**/
static
EFI_STATUS
LocateAcpiTableBySignature (
  IN      UINT32                       Signature,
  IN OUT  EFI_ACPI_DESCRIPTION_HEADER  **Table,
  IN OUT  UINTN                        *Handle
  )
{
  EFI_STATUS              Status;
  INTN                    Index;
  EFI_ACPI_TABLE_VERSION  Version;
  EFI_ACPI_SDT_PROTOCOL   *AcpiSdt = NULL;

  Status = gBS->LocateProtocol (&gEfiAcpiSdtProtocolGuid, NULL, (VOID **)&AcpiSdt);

  if (EFI_ERROR (Status) || (AcpiSdt == NULL)) {
    return EFI_NOT_FOUND;
  }

  //
  // Locate table with matching ID
  //
  Version = 0;
  Index   = 0;
  do {
    Status = AcpiSdt->GetAcpiTable (Index, (EFI_ACPI_SDT_HEADER **)Table, &Version, Handle);
    if (EFI_ERROR (Status)) {
      break;
    }

    Index++;
  } while ((*Table)->Signature != Signature);

  //
  // If we found the table, there will be no error.
  //
  return Status;
}

/**
 * UpdateFacsHardwareSignature
 *
 * MAT is computed at ExitBootServices.  FACS.HardwareSignature is used before that,
 * so cannot include MAT in the HardwareSignature.
 *
 * @param[in]   FacsHwSigAlgorithm   FACS HardwareSignature algorithm enumerate.
 *
 * @retval   EFI_SUCCESS       No errors when updating FACS HardwareSignature.
 * @retval   EFI_UNSUPPORTED   The selected HardwareSignature algorithm is not supported.
 * @retval   Other             Unexpected failure when updating FACS HardwareSignature.
 */
EFI_STATUS
EFIAPI
UpdateFacsHardwareSignature (
  IN FACS_HARDWARE_SIGNATURE_ALGORITHM  FacsHwSigAlgorithm
  )
{
  EFI_STATUS                                           Status = EFI_SUCCESS;
  UINTN                                                Handle;
  EFI_ACPI_3_0_FIXED_ACPI_DESCRIPTION_TABLE            *FadtPtr = NULL;
  static EFI_ACPI_2_0_FIRMWARE_ACPI_CONTROL_STRUCTURE  *FacsPtr = NULL;
  UINT32                                               *Buffer  = NULL;
  UINTN                                                BufferCount;     // Count of UINT32 items in Buffer

  // Variables for Part 1 PCI data
  EFI_PCI_IO_PROTOCOL  *PciIo;
  UINTN                PciHandleCount;
  EFI_HANDLE           *PciHandleBuffer = NULL;
  UINT32               PciId;

  // Variables for Part 2 Firmware Version data
  UINTN   IndexPart2;
  UINT32  UefiFwVersion;

  // Variables for Part 3 System Settings data
  UINTN  IndexPart3;
  UINTN  SettingsDataSize;

  // Variables for Part 4 Memory Map data
  UINTN                  IndexPart4;
  UINTN                  MemoryMapSize;
  EFI_MEMORY_DESCRIPTOR  *MemoryMap = NULL;
  CHAR8                  *Entry;
  UINT64                 *MapState;
  UINTN                  MapKey;
  UINTN                  DescriptorSize;
  UINT32                 DescriptorVersion;
  UINTN                  Count;
  UINTN                  i;
  UINT64                 Temp;

  //
  // This library only supports one algorithm at this time
  //
  if ((FacsHwSigAlgorithm != DefaultFacsHwSigAlgorithm) &&
      (FacsHwSigAlgorithm != FacsV2CompatibleFacsHwSigAlgorithm))
  {
    DEBUG ((DEBUG_ERROR, "%a Unsupported FACS HW Signature Algorithm selected!\n", __FUNCTION__));
    Status = EFI_UNSUPPORTED;
    goto CleanUp;
  }

  // Rebuild the hardware signature. For MS systems, the following parts will make up the hardware signature:
  //
  // Part1. The PCI DeviceID's
  // Part2. The firmware version
  // Part3. The current device settings
  // Part4. The MemoryMap  -- place last as a large block is reserved, but most is not used
  //
  //
  // Step 1. Locate the FACS.
  //
  Handle = 0;
  Status = LocateAcpiTableBySignature (
             EFI_ACPI_1_0_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE,
             (EFI_ACPI_DESCRIPTION_HEADER **)&FadtPtr,
             &Handle
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a Unable to locate FADT\n", __FUNCTION__));
    goto CleanUp;      // Table not found.
  }

  FacsPtr = (EFI_ACPI_2_0_FIRMWARE_ACPI_CONTROL_STRUCTURE *)(UINTN)FadtPtr->FirmwareCtrl;

  if (NULL == FacsPtr) {
    DEBUG ((DEBUG_ERROR, "%a Unable to locate FacsPtr\n", __FUNCTION__));
    Status = EFI_BAD_BUFFER_SIZE;
    goto CleanUp;
  }

  //
  // Step 2. Determine space for PCI IDs (Part 1)
  //
  PciHandleCount  = 0;
  PciHandleBuffer = NULL;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiPciIoProtocolGuid,
                  NULL,
                  &PciHandleCount,
                  &PciHandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a Unable to locate any Pci I/O devices\n", __FUNCTION__));
    goto CleanUp;
  }

  // Index of part 1 is always 0                             // Buffer[0] is the start of the data
  BufferCount  = PciHandleCount;                               // Set BufferCount to the number of UINT32 values to be stored.
  BufferCount += (BufferCount & 0x01);                         // Add 1 to align to 64 bit boundary if necessary
  BufferCount += sizeof (UINT64) / sizeof (UINT32);            // Add contents of XDsdt;

  //
  // Step 3. Determine space for firmware version (Part 2)
  //
  IndexPart2   = BufferCount;                                  // Where, in Buffer, Part 2 starts
  BufferCount += sizeof (UINT32) / sizeof (UINT32);            // Firmware version from UEFI lib will always be sizeof(UINT32)
  BufferCount += (BufferCount & 0x01);                         // Add 1 to align to 64 bit boundary if necessary

  //
  // Step 4. Determine space for system settings (Part 3)
  //
  IndexPart3       = BufferCount;                              // Where, in Buffer, Part 3 starts;
  SettingsDataSize = 0;
  Status           = gRT->GetVariable (
                            DFCI_SETTINGS_CURRENT_OUTPUT_VAR_NAME,
                            &gDfciSettingsManagerVarNamespace,
                            NULL,
                            &SettingsDataSize,
                            NULL
                            );
  if (EFI_BUFFER_TOO_SMALL != Status) {
    DEBUG ((DEBUG_ERROR, "%a Unable to locate the settings variable\n", __FUNCTION__));
    goto CleanUp;
  }

  BufferCount += (SettingsDataSize + sizeof (UINT32) - 1) / sizeof (UINT32); // Round up to UINT32 alignment
  BufferCount += (BufferCount & 0x01);                                       // Add 1 to align to 64 bit boundary if necessary

  //
  // Step 5. Determine space for Memory Map info (Part 4)
  //
  IndexPart4    = BufferCount;                                 // Where, in Buffer, Part 4 starts;
  MemoryMapSize = 0;
  Status        = gBS->GetMemoryMap (&MemoryMapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion);
  if (EFI_BUFFER_TOO_SMALL != Status) {
    DEBUG ((DEBUG_ERROR, "%a Unable to obtain the memory map\n", __FUNCTION__));
    goto CleanUp;
  }

  BufferCount += (MemoryMapSize + sizeof (UINT32) - 1) / sizeof (UINT32);
  BufferCount += (BufferCount & 0x01);                         // Add 1 to align to 64 bit boundary if necessary

  //
  // Step 6. Allocate buffer for all 4 parts
  //
  Buffer = (UINT32 *)AllocateZeroPool (BufferCount * sizeof (UINT32));     // Insure alignment voids are zero
  if (NULL == Buffer) {
    DEBUG ((DEBUG_ERROR, "%a Unable to obtain the memory for FACS HardwareSignature\n", __FUNCTION__));
    Status = EFI_OUT_OF_RESOURCES;
    goto CleanUp;
  }

  //
  // Step 7. Fill buffer with PCI IDs
  //
  for (i = 0; i < PciHandleCount; i++) {
    PciId  = 0;
    Status = gBS->HandleProtocol (PciHandleBuffer[i], &gEfiPciIoProtocolGuid, (VOID **)&PciIo);
    if (!EFI_ERROR (Status)) {
      Status = PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, 0, 1, &PciId);
      if (EFI_ERROR (Status)) {
        continue;
      }

      Buffer[i] = PciId;
    }
  }

  Buffer[i++] = (UINT32)FadtPtr->XDsdt;
  Buffer[i++] = (UINT32)(FadtPtr->XDsdt >> 32);

  if (i > IndexPart2) {
    DEBUG ((DEBUG_ERROR, "%a Buffer overrun computing FACS HardwareSignature\n", __FUNCTION__));
    Status = EFI_BUFFER_TOO_SMALL;
    goto CleanUp;
  }

  //
  // Step 8. Fill buffer with firmware version
  //
  UefiFwVersion      = GetUefiVersionNumber ();
  Buffer[IndexPart2] = UefiFwVersion;

  //
  // Step 9. Fill buffer with device settings
  //
  Status = gRT->GetVariable (
                  DFCI_SETTINGS_CURRENT_OUTPUT_VAR_NAME,
                  &gDfciSettingsManagerVarNamespace,
                  NULL,
                  &SettingsDataSize,
                  &Buffer[IndexPart3]
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a Unable to obtain the settings\n", __FUNCTION__));
    goto CleanUp;
  }

  //
  // Step 10. Fill buffer with memory map entries
  //
  // The MemoryMap buffer is an array of 48 byte entries.  This code is will build an array
  // of 16 byte entries on top of the existing MemoryMap buffer, but only from selected
  // entries (Runtime, Rsvd, ACPI) from the MemoryMap buffer.
  //
  MemoryMap = (EFI_MEMORY_DESCRIPTOR *)&Buffer[IndexPart4];
  MapState  = (UINT64 *)MemoryMap;
  Status    = gBS->GetMemoryMap (&MemoryMapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion);
  Entry     = (CHAR8 *)MemoryMap;
  if (Status == EFI_SUCCESS) {
    // Code relies on the output array element size to be <= the input array element size, as
    // the output array is built on top of the input array.
    //
    // This code allows for the output element size to be up to the size
    // of the input element. This allows the code to only need the one buffer.
    // Since entries could be overlaid, make sure that all the data needed for
    // the output element is obtained before storing the output entry.
    if (DescriptorSize >= (2 * sizeof (UINT64))) {
      Count = MemoryMapSize / DescriptorSize;
      for (i = 0; i < Count; i++) {
        MemoryMap = (EFI_MEMORY_DESCRIPTOR *)Entry;
        if ((MemoryMap->Type == EfiRuntimeServicesCode) ||
            (MemoryMap->Type == EfiRuntimeServicesData) ||
            (MemoryMap->Type == EfiReservedMemoryType)  ||
            (MemoryMap->Type == EfiACPIReclaimMemory)   ||
            (MemoryMap->Type == EfiACPIMemoryNVS))
        {
          Temp        = MemoryMap->NumberOfPages;
          *MapState++ = (UINT64)MemoryMap->PhysicalStart;
          *MapState++ = Temp;
        }

        Entry += DescriptorSize;
      }
    }
  }

  // Compute the count of bytes used from Buffer[0] to current value of MapState.
  Count = (UINT64)MapState - (UINT64)Buffer;

  //
  // Step 11. Crc buffer, and store result in FacsPtr->HardwareSignature.
  //
  Status = gBS->CalculateCrc32 (Buffer, Count, &FacsPtr->HardwareSignature);
  DEBUG ((DEBUG_INFO, "CRC = %x, Facs =%p, and Status = %r\n ", FacsPtr->HardwareSignature, FacsPtr, Status));

CleanUp:
  //
  // Step 12. Free buffers
  //
  if (PciHandleBuffer != NULL) {
    FreePool (PciHandleBuffer);
  }

  if (Buffer != NULL) {
    FreePool (Buffer);
  }

  return Status;
}
