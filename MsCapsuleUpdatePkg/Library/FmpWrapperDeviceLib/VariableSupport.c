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


#include <Uefi.h>
#include <Protocol/VariableLock.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include "VariableSupport.h"


/*
  Function used to Get the FMP version from a UEFI variable.  
  This will return a default value if variable doesn't exist. 
*/
UINT32
GetVersionFromVariable()
{
  UINT32* Value = NULL;
  UINTN Size = 0;
  EFI_STATUS status;
  UINT32 Version = DEFAULT_VERSION;

  status = GetVariable2(VARNAME_VERSION, &gEfiCallerIdGuid, &Value, &Size);
  if (EFI_ERROR(status))
  {
    DEBUG((DEBUG_ERROR, "Failed to get the Version from variable.  Status = %r\n", status));
    return Version;
  }

  //No error from call
  if (Size == sizeof(*Value))
  {
    //successful read
    Version = *Value;
  }
  else
  {
    //return default since size was unknown
    DEBUG((DEBUG_ERROR, "Getting version Variable returned a size different than expected. Size = 0x%x\n", Size));
  }

  FreePool(Value);
  return Version;
}


/*
Function used to Get the FMP Lowest supported version from a UEFI variable.
This will return a default value if variable doesn't exist.
*/
UINT32
GetLowestSupportedVersionFromVariable()
{
  UINT32* Value = NULL;
  UINTN Size = 0;
  EFI_STATUS status;
  UINT32 Version = DEFAULT_LOWESTSUPPORTEDVERSION;

  status = GetVariable2(VARNAME_LSV, &gEfiCallerIdGuid, &Value, &Size);
  if (EFI_ERROR(status))
  {
    DEBUG((DEBUG_WARN, "Warning: Failed to get the Lowest Supported Version from variable.  Status = %r\n", status));
    return Version;
  }

  //No error from call
  if (Size == sizeof(*Value))
  {
    //successful read
    Version = *Value;
  }
  else
  {
    //return default since size was unknown
    DEBUG((DEBUG_ERROR, "Getting LSV Variable returned a size different than expected. Size = 0x%x\n", Size));
  }

  FreePool(Value);
  return Version;
}

/*
Function used to Get the FMP capsule Last Attempt Status from a UEFI variable.
This will return a default value if variable doesn't exist.
*/
UINT32
GetLastAttemptStatusFromVariable()
{
  UINT32* Value = NULL;
  UINTN Size = 0;
  EFI_STATUS status;
  UINT32 s = DEFAULT_LASTATTEMPT;

  status = GetVariable2(VARNAME_LASTATTEMPTSTATUS, &gEfiCallerIdGuid, &Value, &Size);
  if (EFI_ERROR(status))
  {
    DEBUG((DEBUG_WARN, "Warning: Failed to get the Last Attempt Status from variable.  Status = %r\n", status));
    return s;
  }

  //No error from call
  if (Size == sizeof(*Value))
  {
    //successful read
    s = *Value;
  }
  else
  {
    //return default since size was unknown
    DEBUG((DEBUG_ERROR, "Getting Last Attempt Status Variable returned a size different than expected. Size = 0x%x\n", Size));
  }

  FreePool(Value);
  return s;
}

/*
Function used to Get the FMP capsule Last Attempt Version from a UEFI variable.
This will return a default value if variable doesn't exist.
*/
UINT32
GetLastAttemptVersionFromVariable()
{
  UINT32* Value = NULL;
  UINTN Size = 0;
  EFI_STATUS status;
  UINT32 Version = DEFAULT_LASTATTEMPT;

  status = GetVariable2(VARNAME_LASTATTEMPTVERSION, &gEfiCallerIdGuid, &Value, &Size);
  if (EFI_ERROR(status))
  {
    DEBUG((DEBUG_WARN, "Warning: Failed to get the Last Attempt Version from variable.  Status = %r\n", status));
    return Version;
  }

  //No error from call
  if (Size == sizeof(*Value))
  {
    //successful read
    Version = *Value;
  }
  else
  {
    //return default since size was unknown
    DEBUG((DEBUG_ERROR, "Getting Last Attempt Version variable returned a size different than expected. Size = 0x%x\n", Size));
  }

  FreePool(Value);
  return Version;
}


/*
Function used to Set the FMP version to a UEFI variable.

*/
VOID
SetVersionInVariable(UINT32 v)
{
  EFI_STATUS status = EFI_SUCCESS;
  UINT32 Current = GetVersionFromVariable();
  if (Current != v) {
    status = gRT->SetVariable(VARNAME_VERSION, &gEfiCallerIdGuid, EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS, sizeof(v), &v);
    if (EFI_ERROR(status))
    {
      DEBUG((DEBUG_ERROR, "Failed to set the Version into a variable.  Status = %r\n", status));
    }
  }
  else {
    DEBUG((DEBUG_INFO, "Version variable doesn't need to update.  Same value as before.\n"));
  }
  return;
}

/*
Function used to Set the FMP lowest supported version to a UEFI variable.

*/
VOID
SetLowestSupportedVersionInVariable(UINT32 v)
{
  EFI_STATUS status = EFI_SUCCESS;
  UINT32 CurrentLSV = GetLowestSupportedVersionFromVariable();
  if (v > CurrentLSV) {
    status = gRT->SetVariable(VARNAME_LSV, &gEfiCallerIdGuid, EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS, sizeof(v), &v);
    if (EFI_ERROR(status))
    {
      DEBUG((DEBUG_ERROR, "Failed to set the LSV into a variable.  Status = %r\n", status));
    }
  }
  else {
    DEBUG((DEBUG_INFO, "LSV variable doesn't need to update.  Same value as before.\n"));
  }
  return;
}

/*
Function used to Set the FMP Capsule Last Attempt Status to a UEFI variable.

*/
VOID
SetLastAttemptStatusInVariable(UINT32 s)
{
  EFI_STATUS status = EFI_SUCCESS;
  UINT32 Current = GetLastAttemptStatusFromVariable();
  if (Current != s) {
    status = gRT->SetVariable(VARNAME_LASTATTEMPTSTATUS, &gEfiCallerIdGuid, EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS, sizeof(s), &s);
    if (EFI_ERROR(status))
    {
      DEBUG((DEBUG_ERROR, "Failed to set the LastAttemptStatus into a variable.  Status = %r\n", status));
    }
  }
  else {
    DEBUG((DEBUG_INFO, "LastAttemptStatus variable doesn't need to update.  Same value as before.\n"));
  }
  return;
}

/*
Function used to Set the FMP Capsule Last Attempt Version to a UEFI variable.

*/
VOID 
SetLastAttemptVersionInVariable(UINT32 v)
{
  EFI_STATUS status = EFI_SUCCESS;
  UINT32 Current = GetLastAttemptVersionFromVariable();
  if (Current != v) {
    status = gRT->SetVariable(VARNAME_LASTATTEMPTVERSION, &gEfiCallerIdGuid, EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS, sizeof(v), &v);
    if (EFI_ERROR(status))
    {
      DEBUG((DEBUG_ERROR, "Failed to set the LastAttemptVersion into a variable.  Status = %r\n", status));
    }
  }
  else {
    DEBUG((DEBUG_INFO, "LastAttemptVersion variable doesn't need to update.  Same value as before.\n"));
  }
  return;
}

VOID
LockAllVars()
{
  EFI_STATUS Status;
  EDKII_VARIABLE_LOCK_PROTOCOL      *Protocol = NULL;
  Status = gBS->LocateProtocol(&gEdkiiVariableLockProtocolGuid, NULL, (VOID**)&Protocol);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "Failed to locate var lock protocol (%r).  Can't lock variables\n", Status));
    return;
  }
  Status = Protocol->RequestToLock(Protocol, L"*", &gEfiCallerIdGuid);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "Failed to lock.  Status = %r\n", Status));
  } 
  else
  {
    DEBUG((DEBUG_INFO, "All variables are locked\n"));
  }
}
