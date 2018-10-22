/** @file
Functions used to support Getting and Setting Device States.
This library uses the PcdDeviceStateBitmask dynamic PCD to store the device state

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

#include <PiPei.h>
#include <Library/PcdLib.h>
#include <Library/DeviceStateLib.h>
#include <Library/DebugLib.h>

/**
Function to Get current device state
@retval the current DEVICE_STATE
**/
DEVICE_STATE
EFIAPI
GetDeviceState()
{
  DEVICE_STATE DevState = PcdGet32 (PcdDeviceStateBitmask);
  return DevState;
}

/**
Function to Add additional bits to the device state

@param AdditionalState - additional state to set active
@retval Status of operation.  EFI_SUCCESS on successful update.
**/
RETURN_STATUS
EFIAPI
AddDeviceState(DEVICE_STATE AdditionalState)
{
  DEVICE_STATE DevState;
  DEVICE_STATE SetValue;
  EFI_STATUS Status;

  DevState = PcdGet32 (PcdDeviceStateBitmask);

  DEBUG((DEBUG_INFO, "Adding Device State.  0x%X\n", AdditionalState));

  DevState |= AdditionalState;
  Status = PcdSet32S(PcdDeviceStateBitmask, DevState);
  if(EFI_ERROR(Status)) {
    DEBUG ((DEBUG_ERROR, "Error setting device state\n", __FUNCTION__, Status));
    return RETURN_DEVICE_ERROR;
  }

  SetValue = PcdGet32 (PcdDeviceStateBitmask);
  
  return ((SetValue == DevState) ? RETURN_SUCCESS : RETURN_OUT_OF_RESOURCES);
}

