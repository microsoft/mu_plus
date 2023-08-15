/** @file
  TPM Replay DXE-Specific UEFI Variable Logic

  Copyright (c) Microsoft Corporation.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Uefi.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include "TpmReplayInputChannelInternal.h"

/**
  Enumerates the current variable names.

  This function serves as the module's DXE implementation of this function.

  This abstraction allows phase independent code to invoke the GetNextVariableName () API in shared code files.

  @param[in, out]  VariableNameSize The size of the VariableName buffer. The size must be large
                                    enough to fit input string supplied in VariableName buffer.
  @param[in, out]  VariableName     On input, supplies the last VariableName that was returned
                                    by GetNextVariableName(). On output, returns the Nullterminated
                                    string of the current variable.
  @param[in, out]  VendorGuid       On input, supplies the last VendorGuid that was returned by
                                    GetNextVariableName(). On output, returns the
                                    VendorGuid of the current variable.

  @retval EFI_SUCCESS           The function completed successfully.
  @retval EFI_NOT_FOUND         The next variable was not found.
  @retval EFI_BUFFER_TOO_SMALL  The VariableNameSize is too small for the result.
                                VariableNameSize has been updated with the size needed to complete the request.
  @retval EFI_INVALID_PARAMETER VariableNameSize is NULL.
  @retval EFI_INVALID_PARAMETER VariableName is NULL.
  @retval EFI_INVALID_PARAMETER VendorGuid is NULL.
  @retval EFI_INVALID_PARAMETER The input values of VariableName and VendorGuid are not a name and
                                GUID of an existing variable.
  @retval EFI_INVALID_PARAMETER Null-terminator is not found in the first VariableNameSize bytes of
                                the input VariableName buffer.
  @retval EFI_DEVICE_ERROR      The variable could not be retrieved due to a hardware error.

**/
EFI_STATUS
InternalGetNextVariableName (
  IN OUT UINTN     *VariableNameSize,
  IN OUT CHAR16    *VariableName,
  IN OUT EFI_GUID  *VendorGuid
  )
{
  return gRT->GetNextVariableName (
                VariableNameSize,
                VariableName,
                VendorGuid
                );
}

/**
  Returns the value of a variable.

  This function serves as the module's DXE implementation of this function.

  This abstraction allows phase independent code to invoke the GetVariable () API in shared code files.

  @param[in]       VariableName  A Null-terminated string that is the name of the vendor's
                                 variable.
  @param[in]       VendorGuid    A unique identifier for the vendor.
  @param[out]      Attributes    If not NULL, a pointer to the memory location to return the
                                 attributes bitmask for the variable.
  @param[in, out]  DataSize      On input, the size in bytes of the return Data buffer.
                                 On output the size of data returned in Data.
  @param[out]      Data          The buffer to return the contents of the variable. May be NULL
                                 with a zero DataSize in order to determine the size buffer needed.

  @retval EFI_SUCCESS            The function completed successfully.
  @retval EFI_NOT_FOUND          The variable was not found.
  @retval EFI_BUFFER_TOO_SMALL   The DataSize is too small for the result.
  @retval EFI_INVALID_PARAMETER  VariableName is NULL.
  @retval EFI_INVALID_PARAMETER  VendorGuid is NULL.
  @retval EFI_INVALID_PARAMETER  DataSize is NULL.
  @retval EFI_INVALID_PARAMETER  The DataSize is not too small and Data is NULL.
  @retval EFI_DEVICE_ERROR       The variable could not be retrieved due to a hardware error.
  @retval EFI_SECURITY_VIOLATION The variable could not be retrieved due to an authentication failure.

**/
EFI_STATUS
InternalGetVariable (
  IN     CHAR16 *VariableName,
  IN     EFI_GUID *VendorGuid,
  OUT    UINT32 *Attributes, OPTIONAL
  IN OUT UINTN                       *DataSize,
  OUT    VOID                        *Data           OPTIONAL
  )
{
  return gRT->GetVariable (
                VariableName,
                VendorGuid,
                Attributes,
                DataSize,
                Data
                );
}
