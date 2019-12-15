/**@file Empty library to provide platform version information

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/MuUefiVersionLib.h>

/**
  Return Uefi version number defined by platform

  @retval  UINT32   UEFI firmware version
**/
UINT32
EFIAPI
GetUefiVersionNumber (
  VOID
)
{
  return 0;
}

/**
  Return a Null-terminated Uefi version Ascii string defined by platform

  @param[out]       Buffer        The caller allocated buffer to hold the returned version
                                  Ascii string. May be NULL with a zero Length in order
                                  to determine the length buffer needed.
  @param[in, out]   Length        On input, the count of Ascii chars available in Buffer.
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
)
{
  return EFI_UNSUPPORTED;
}

/**
  Return a Null-terminated Uefi version Unicode string defined by platform

  @param[out]       Buffer        The caller allocated buffer to hold the returned version
                                  Unicode string. May be NULL with a zero Length in order
                                  to determine the length buffer needed.
  @param[in, out]   Length        On input, the count of Unicode chars available in Buffer.
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
)
{
  return EFI_UNSUPPORTED;
}

/**
  Return a Null-terminated Uefi build date Ascii string defined by platform

  @param[out]       Buffer        The caller allocated buffer to hold the returned build
                                  date Ascii string. May be NULL with a zero Length in
                                  order to determine the length buffer needed.
  @param[in, out]   Length        On input, the count of Ascii chars available in Buffer.
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
)
{
  return EFI_UNSUPPORTED;
}

/**
  Return a Null-terminated Uefi build date Unicode defined by platform

  @param[out]       Buffer        The caller allocated buffer to hold the returned build
                                  date Unicode string. May be NULL with a zero Length in
                                  order to determine the length buffer needed.
  @param[in, out]   Length        On input, the count of Unicode chars available in Buffer.
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
)
{
  return EFI_UNSUPPORTED;
}
