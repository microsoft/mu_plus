/** @file
Function hook provided to the platform to reset the system after capsule.  
Some systems require unique reset operations.

Copyright (c) 2014, Microsoft Corporation. All rights reserved.<BR>

**/

#ifndef __CAPSULE_RESET_LIB__
#define __CAPSULE_RESET_LIB__

/**
Function used to Reset the system after capsule udpate.

@return - should not return if successful
@return Error - Either not supported or some system failure.  Calling code must handle reset.
**/
EFI_STATUS 
EFIAPI 
ResetAfterCapsuleUpdate();


#endif