/** @file
This file is the header file for the Power Services Lib.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __MS_POWER_SERVICES_LIBRARY_H__
#define __MS_POWER_SERVICES_LIBRARY_H__

/** 
  These gives information to the SystemPowerCheck and SystemPowerMitigate
  functions for the situation that is currently ocurring when the test or
  mitigation action is done.
**/
typedef enum {
    PowerCaseBoot,
    PowerCaseUpdate,
    PowerCaseMax
} POWER_CASE;


/**
  This function tries to determine if the power state of the system is compatible
  with one of the cases enumerated above

  @param[in]   Case     Situation currently ocurring/type of test to perform
  @param[out]  Good     Output address for value of test. Only valid if EFI_SUCCESS returned.

  @retval  EFI_SUCCESS  Power suitability for case was able to be determined
  @retval  !EFI_SUCCESS Power suitability for case was not able to be determined
**/
EFI_STATUS
EFIAPI
SystemPowerCheck (
  IN  POWER_CASE  Case,
  OUT BOOLEAN*    Good
);


/**
  This function is called in response to a successful call to SystemPowerCheck that 
  returns success (power suitability was able to be determined), but has returned a 
  test value that indicates the case has failed.

  @param[in]  Case      Situation currently ocurring

  @retval  EFI_SUCCESS  Power suitability for case was able to be mitigated
  @retval  !EFI_SUCCESS Power suitability for case was not able to be mitigated
**/
EFI_STATUS
EFIAPI
SystemPowerMitigate (
  IN  POWER_CASE  Case
);

#endif

