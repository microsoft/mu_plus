/** @file
Provides helper function for resetting the system after capsule update. 

This instance just uses standard UEFI gRT->ResetSystem with a cold flag.  

Copyright (c) 2015, Microsoft Corporation. All rights reserved.<BR>

**/

#include <Uefi.h>
#include <Library/CapsuleResetLib.h>

/**
Function used to Reset the system after capsule udpate. 

@return - should not return if successful
@return Error - Either not supported or some system failure.  Calling code must handle reset.  
**/
EFI_STATUS
EFIAPI
ResetAfterCapsuleUpdate()
{
  return EFI_UNSUPPORTED;
}
