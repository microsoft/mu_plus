/**

Copyright (c) 2016, Microsoft Corporation

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


#include <PiDxe.h>
#include <Library/CapsuleUpdatePolicyLib.h>


/*
Method to check if the system power is good for capsule update

@param [in out] Good - Boolean will be updated with state of TRUE meaning
                       power is good.  False meaning power is not good.  Value is
                       only valid if status code is Success.

@retval Success - Good parameter updated with result.
@retval EFI_ERROR - error occurred and good parameter is not valid
*/
EFI_STATUS
EFIAPI
CheckSystemPower(IN OUT BOOLEAN* Good)
{
  *Good = TRUE;
  return EFI_SUCCESS;
}

/*
Method to check if the system thermal is good for capsule update

@param [in out] Good - Boolean will be updated with state of TRUE meaning
                       thermal is good.  False meaning thermal is not good.  Value is
                       only valid if status code is Success.

@retval Success - Good parameter updated with result.
@retval EFI_ERROR - error occurred and good parameter is not valid
*/
EFI_STATUS
EFIAPI
CheckSystemThermal(IN OUT BOOLEAN* Good)
{
  *Good = TRUE;
  return EFI_SUCCESS;
}


/*
Method to check if the system env is good for capsule update

@param [in out] Good - Boolean will be updated with state of TRUE meaning
                       System Env is good.  False meaning it is not good.  Value is
                       only valid if status code is Success.

@retval Success - Good parameter updated with result.
@retval EFI_ERROR - error occurred and good parameter is not valid
*/
EFI_STATUS
EFIAPI
CheckSystemEnvironment(IN OUT BOOLEAN* Good)
{
  *Good = TRUE;
  return EFI_SUCCESS;
}