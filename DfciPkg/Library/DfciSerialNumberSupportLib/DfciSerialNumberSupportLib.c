
#include <PiDxe.h>
#include <Library/DfciSerialNumberSupportLib.h>
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
GetSerialNumber (
  OUT UINTN*  SerialNumber
  )
{

  CHAR8* AsciiValue = "DFCIDEVICE10";

  //
  // Validate the parameters
  //
  if (SerialNumber == NULL) {
    DEBUG((DEBUG_ERROR, "GetSerialNumber(), invalid param.\n"));
    return EFI_INVALID_PARAMETER;
  }

  *SerialNumber = AsciiStrDecimalToUintn(AsciiValue);
  DEBUG((DEBUG_INFO, "System Serial Number is %d\n", *SerialNumber));
  return EFI_SUCCESS;
}