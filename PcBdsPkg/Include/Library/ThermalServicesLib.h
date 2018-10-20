/** @file
This file is the header file for the Thermal Services Lib.

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

#ifndef __MS_THERMAL_SERVICES_LIBRARY_H__
#define __MS_THERMAL_SERVICES_LIBRARY_H__

/**
  These gives information to the SystemThermalCheck and SystemThermalMitigate
  functions for the situation that is currently ocurring when the test or
  mitigation action is done
**/
typedef enum {
  ThermalCaseBoot,
  ThermalCaseUpdate,
  ThermalCaseMax
} THERMAL_CASE;


/**
  This function tries to determine if the thermal state of the system is compatible
  with one of the cases enumerated above

  @param[in]  Case      Situation currently ocurring/type of test to perform
  @param[out] Good      Output address for value of test. Only valid if EFI_SUCCESS returned.

  @retval  EFI_SUCCESS  Thermal suitability for case was able to be determined
  @retval  !EFI_SUCCESS Thermal suitability for case was not able to be determined
**/
EFI_STATUS
EFIAPI
SystemThermalCheck (
  IN  THERMAL_CASE  Case,
  OUT BOOLEAN*      Good
);


/**
  This function is called in response to a successful call to SystemThermalCheck that 
  returns success (thermal state was able to be determined), but has returned a test
  value that indicates the case has failed.

  @param[in]  Case      Situation currently ocurring
  @param[in]  Timeout   Milliseconds to wait before timing out

  @retval  EFI_SUCCESS  Thermal state was able to be mitigated
  @retval  !EFI_SUCCESS Thermal state was not able to be mitigated in timeout period

**/
EFI_STATUS
EFIAPI
SystemThermalMitigate (
  IN  THERMAL_CASE  Case,
  IN  UINT32        TimeoutPeriod
);


#endif

