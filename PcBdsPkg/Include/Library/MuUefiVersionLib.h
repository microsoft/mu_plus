
/** @file
Library to provide platform version information

Copyright (c) 2016 - 2019, Microsoft Corporation

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

#ifndef __MU_UEFI_VERSION_LIB__
#define __MU_UEFI_VERSION_LIB__

/**
  Return Uefi version number defined by platform

  @retval  UINT32   UEFI firmware version
**/
UINT32
EFIAPI
GetUefiVersionNumber (
  VOID
);

/**
  Return a Null-terminated Uefi version Ascii string defined by platform

  @param[out]       Buffer        The caller allocated buffer to hold the returned version
                                  Ascii string. May be NULL with a zero Length in order
                                  to determine the length buffer needed.
  @param[in, out]   Length        On input, the count of Ascii chars avaiable in Buffer.
                                  On output, the count of Ascii chars of data returned
                                  in Buffer, including Null-terminator.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_BUFFER_TOO_SMALL    The Length is too small for the result.
  @retval EFI_INVALID_PARAMETER   Buffer is NULL.
  @retval EFI_INVALID_PARAMETER   Length is NULL.
  @retval EFI_INVALID_PARAMETER   The Length is not 0 and Buffer is NULL.
  @retval Others                  Other implementation specific errors.
**/
EFI_STATUS
EFIAPI
GetUefiVersionStringAscii (
      OUT CHAR8   *Buffer,            OPTIONAL
  IN  OUT UINTN   *Length
);

/**
  Return a Null-terminated Uefi version Unicode string defined by platform

  @param[out]       Buffer        The caller allocated buffer to hold the returned version
                                  Unicode string. May be NULL with a zero Length in order
                                  to determine the length buffer needed.
  @param[in, out]   Length        On input, the count of Unicode chars avaiable in Buffer.
                                  On output, the count of Unicode chars of data returned
                                  in Buffer, including Null-terminator.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_BUFFER_TOO_SMALL    The Length is too small for the result.
  @retval EFI_INVALID_PARAMETER   Buffer is NULL.
  @retval EFI_INVALID_PARAMETER   Length is NULL.
  @retval EFI_INVALID_PARAMETER   The Length is not 0 and Buffer is NULL.
  @retval Others                  Other implementation specific errors.
**/
EFI_STATUS
EFIAPI
GetUefiVersionStringUnicode (
      OUT CHAR16  *Buffer,            OPTIONAL
  IN  OUT UINTN   *Length
);

/**
  Return a Null-terminated Uefi build date Ascii string defined by platform

  @param[out]       Buffer        The caller allocated buffer to hold the returned build
                                  date Ascii string. May be NULL with a zero Length in
                                  order to determine the length buffer needed.
  @param[in, out]   Length        On input, the count of Ascii chars avaiable in Buffer.
                                  On output, the count of Ascii chars of data returned
                                  in Buffer, including Null-terminator.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_BUFFER_TOO_SMALL    The Length is too small for the result.
  @retval EFI_INVALID_PARAMETER   Buffer is NULL.
  @retval EFI_INVALID_PARAMETER   Length is NULL.
  @retval EFI_INVALID_PARAMETER   The Length is not 0 and Buffer is NULL.
  @retval Others                  Other implementation specific errors.
**/
EFI_STATUS
EFIAPI
GetBuildDateStringAscii (
      OUT CHAR8   *Buffer,            OPTIONAL
  IN  OUT UINTN   *Length
);

/**
  Return a Null-terminated Uefi build date Unicode defined by platform

  @param[out]       Buffer        The caller allocated buffer to hold the returned build
                                  date Unicode string. May be NULL with a zero Length in
                                  order to determine the length buffer needed.
  @param[in, out]   Length        On input, the count of Unicode chars avaiable in Buffer.
                                  On output, the count of Unicode chars of data returned 
                                  in Buffer, including Null-terminator.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_BUFFER_TOO_SMALL    The Length is too small for the result.
  @retval EFI_INVALID_PARAMETER   Buffer is NULL.
  @retval EFI_INVALID_PARAMETER   Length is NULL.
  @retval EFI_INVALID_PARAMETER   The Length is not 0 and Buffer is NULL.
  @retval Others                  Other implementation specific errors.
**/
EFI_STATUS
EFIAPI
GetBuildDateStringUnicode (
      OUT CHAR16  *Buffer,            OPTIONAL
  IN  OUT UINTN   *Length
);

#endif // __MU_UEFI_VERSION_LIB__
