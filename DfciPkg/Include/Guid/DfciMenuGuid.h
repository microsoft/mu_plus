/** @file
DfciMenuGuid.h

This file defines the Device Firmware Configuration Interface formset guid and page ID.

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

#ifndef __DFCI_MENU_FORMSET_GUID_H__
#define __DFCI_MENU_FORMSET_GUID_H__


#define DFCI_MENU_FORMSET_GUID  \
  { \
  0x3b82283d, 0x7add, 0x4c6a, {0xad, 0x2b, 0x71, 0x9b, 0x8d, 0x7b, 0x77, 0xc9} \
  }

#define DFCI_MENU_FORM_ID           0x2000

extern EFI_GUID gDfciMenuFormsetGuid;

//
// Recovery Support - Optionally provided by platform.  If the platform supports
// an offline recovery mechanism, produce a DFCI_RECOVERY_FORMSET that can be
// entered from a long form goto.  The following is in DfciMenuVfr.Vfr:
//
//           goto                                    // External reference to recovery menu
//               formsetguid = DFCI_RECOVERY_FORMSET_GUID,
//               formid      = DFCI_RECOVERY_FORM_ID,
//               question    = DFCI_RECOVERY_QUESTION_ID,
//               prompt      = STRING_TOKEN(STR_DFCI_MENU_RECOVERY_NOW),
//               help        = STRING_TOKEN(STR_NULL_STRING);
//
#define DFCI_RECOVERY_FORMSET_GUID  \
  { \
  0xcf9873b2, 0xce63, 0x4f63, {0x9a, 0x5f, 0x16, 0x54, 0xe4, 0x01, 0x61, 0xc6 } \
  }

#define DFCI_RECOVERY_FORM_ID       0x1000

#define DFCI_RECOVERY_QUESTION_ID   0x1200



extern EFI_GUID gDfciRecoveryFormsetGuid;

#endif

