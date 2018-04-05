
#include <PiDxe.h>
#include <Library/DfciDeviceIdSupportLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

/**
Gets the serial number for this device.

@para[out] SerialNumber - UINTN value of serial number

@return EFI_SUCCESS - SerialNumber has been updated to equal the serial number of the device
@return EFI_ERROR   - Error getting number

**/
EFI_STATUS
EFIAPI
GetSerialNumber(
  OUT UINTN*  SerialNumber
  ) {

   return EFI_UNSUPPORTED;
}


/**
 * Get the Device Id elements for the Dfci Packets
 *
 * @param Manufacturer
 * @param ProductName
 * @param SerialNumber
 * @param UUID
 *
 * @return EFI_STATUS EFIAPI
 */
EFI_STATUS
EFIAPI
DfciSupportGetDeviceId (
    OUT DFCI_DEVICE_ID_ELEMENTS **DeviceId
  ) {

    return EFI_UNSUPPORTED;
}
