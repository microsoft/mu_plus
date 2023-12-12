/**@file CheckHwErrRecHeaderLib.h

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef CHECK_HW_ERR_REC_HEADER_LIB_H_
#define CHECK_HW_ERR_REC_HEADER_LIB_H_

/**
 *  Checks that all length and offset fields within the HWErrRec fall within the bounds of
 *  the buffer, all section data is accounted for in their respective section headers, and that
 *  all section data is contiguous
 *
 *  @param[in]  Err              -    Pointer to HWErr record being checked
 *  @param[in]  Size             -    Size obtained from calling GetVariable() to obtain the record
 *
 *  @retval     BOOLEAN          -    TRUE if all length and offsets are safe
 *                                    FALSE otherwise
**/
BOOLEAN
EFIAPI
ValidateCperHeader (
  IN CONST EFI_COMMON_ERROR_RECORD_HEADER  *Err,
  IN CONST UINTN                           Size
  );

#endif
