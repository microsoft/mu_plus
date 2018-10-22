# DeviceStateLib

## About

The MsCorePkg provides the necessary functions to store platform specific device states.  These device states can then be quired by any element within the boot environment to enable special code paths.  In this library implementation a bitmask is stored in a PCD to signify what modes are active.

The default bits in the bitmask are set in DeviceStateLib.h - but each platform is expected to implement its own header to define the platform specific device states or to define any of the unused bits:

* BIT 0:  DEVICE_STATE_SECUREBOOT_OFF - UEFI Secure Boot disabled
* BIT 1:  DEVICE_STATE_MANUFACTURING_MODE - Device is in an OEM defined manufacturing mode
* BIT 2:  DEVICE_STATE_DEVELOPMENT_BUILD_ENABLED - Device is a development build.  Non-production features might be enabled
* BIT 3:  DEVICE_STATE_SOURCE_DEBUG_ENABLED - Source debug mode is enabled allowing a user to connect and control the device
* BIT 4:  DEVICE_STATE_UNDEFINED - Set by the platform
* BIT 24: DEVICE_STATE_PLATFORM_MODE_0
* BIT 25: DEVICE_STATE_PLATFORM_MODE_1
* BIT 26: DEVICE_STATE_PLATFORM_MODE_2
* BIT 27: DEVICE_STATE_PLATFORM_MODE_3

## Copyright

Copyright (c) 2018, Microsoft Corporation

All rights reserved. Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
