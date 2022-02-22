/** @file
  This protocol allows the Runtime Capsule service to be abstracted away because it
  only returns valid results before ExitBootServices.

  Copyright (c) Microsoft Corporation. All rights reserved.

**/

#ifndef _CAPSULE_SERVICE_H_
#define _CAPSULE_SERVICE_H_

#include <Uefi/UefiSpec.h>

//
// Forward reference for ANSI compatability
//
typedef struct _CAPSULE_SERVICE_PROTOCOL CAPSULE_SERVICE_PROTOCOL;

struct _CAPSULE_SERVICE_PROTOCOL {
  EFI_UPDATE_CAPSULE                UpdateCapsule;
  EFI_QUERY_CAPSULE_CAPABILITIES    QueryCapsuleCapabilities;
};

extern EFI_GUID  gCapsuleServiceProtocolGuid;

#endif
