/** @file -- MsWheaErrorStatus.h

This header file defines MsWheaReport expected/applied invocation components.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __MS_WHEA_ERROR_STATUS__ 
#define __MS_WHEA_ERROR_STATUS__ 

#define MS_WHEA_EARLY_STORAGE_SUBCLASS  \
                                      0x00CA0000

#define MS_WHEA_ERROR_EARLY_STORAGE_STORE_FULL  \
                                      (EFI_SOFTWARE | MS_WHEA_EARLY_STORAGE_SUBCLASS | EFI_SW_EC_EVENT_LOG_FULL)

/**

 Microsoft WHEA accepted error status type, other types will be ignored

**/
#define MS_WHEA_ERROR_STATUS_TYPE_INFO  (EFI_ERROR_MINOR | EFI_ERROR_CODE)
#define MS_WHEA_ERROR_STATUS_TYPE_FATAL (EFI_ERROR_MAJOR | EFI_ERROR_CODE)

#endif // __MS_WHEA_ERROR_STATUS__ 
