/** @file
Functions used to support Getting and Setting Device States.

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

#ifndef __DEVICE_STATE_LIB_H__
#define __DEVICE_STATE_LIB_H__

//
//DEFINE the possible device states here.
// Current API is defined as a 31bit bitmask  (bit 32 is reserved for MAX)
//
#define DEVICE_STATE_SECUREBOOT_OFF            (1 << (0))
#define DEVICE_STATE_MANUFACTURING_MODE        (1 << (1))
#define DEVICE_STATE_DEVELOPMENT_BUILD_ENABLED (1 << (2))
#define DEVICE_STATE_SOURCE_DEBUG_ENABLED      (1 << (3))
#define DEVICE_STATE_UNDEFINED                 (1 << (4))

#define DEVICE_STATE_PLATFORM_MODE_0           (1 << (24))
#define DEVICE_STATE_PLATFORM_MODE_1           (1 << (25))
#define DEVICE_STATE_PLATFORM_MODE_2           (1 << (26))
#define DEVICE_STATE_PLATFORM_MODE_3           (1 << (27))

#define DEVICE_STATE_MAX                       (1 << (31))



typedef UINT32  DEVICE_STATE;


/**
Function to Get current device state
@retval the current DEVICE_STATE
**/
DEVICE_STATE
EFIAPI
GetDeviceState();

/**
Function to Add additional bits to the device state

@param AdditionalState - additional state to set active
@retval Status of operation.  EFI_SUCCESS on successful update.
**/
RETURN_STATUS
EFIAPI
AddDeviceState(
DEVICE_STATE AdditionalState
);


#endif
