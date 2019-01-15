/** @file
  UEFI FormsBrowser Extensions.

  Copyright (c) 2018,  Microsoft Corporation.

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

#ifndef __MS_UEFI_CHECKBOX_H__
#define __MS_UEFI_CHECKBOX_H__

// Ordered List extensions
#define EMBEDDED_CHECKBOX              ((UINT8) 0x80)
#define EMBEDDED_DELETE                ((UINT8) 0x40)

// The input/output value of a UINT32 value array when the EMBEDDED extensions are use does the following:
// -- The low 16 bits represent the boot option value for the menu item
// -- On input, the CHECKBOX_VALUE is the LOAD_OPTION_ACTIVE bit.
// -- On output, the CHECKBOX_VALUE is used to set the LOAD_OPTION_ACTIVE bit.
// -- On input only, ALLOW_DELETE_VALUE is used to tell Listbox that an entry is allowed to be deleted.
// -- On output only, the BOOT_VALUE is used to tell the boot menu code to boot the selected item.
#define ORDERED_LIST_CHECKBOX_VALUE_32     ((UINT32)0x01000000)   // Currently, only value type UINT32 is supported
#define ORDERED_LIST_ALLOW_DELETE_VALUE_32 ((UINT32)0x02000000)   // Currently, only value type UINT32 is supported
#define ORDERED_LIST_BOOT_VALUE_32         ((UINT32)0x80000000)   // Currently, only value type UINT32 is supported

#endif
