/** @file

<DESCRIPTION>

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Pi/PiStatusCode.h>

#include <Protocol/ShellParameters.h>
#include <Protocol/Shell.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/PrintLib.h>
#include <Library/MuTelemetryHelperLib.h>

#define DELETE_ERROR_DIGIT            3
#define SET_NO_ERRORS                 0
#define SET_CONT_ERRORS               1
#define SET_ONE_TIME_ERRORS           2
#define GEN_ERROR_DIGIT               4

#define EFI_HW_ERR_REC_VAR_NAME       L"HwErrRec"
#define EFI_HW_ERR_REC_VAR_NAME_LEN   13      // Buffer length covers at least "HwErrRec####\0"
#define MAX_NUM_DIGITS_READ           4
#define MAX_CHARS_EXTRA_DATA          8

/**
 *  Deletes all WHEA Error Records
 *
 *  @retval    void
**/
VOID
DeleteAllWheaErrors(VOID)
{
  EFI_STATUS  Status;                                   // Return status
  UINTN       Size      = 0;                            // Used to store size of HwErrRec found
  UINT32      OuterLoop = 0;                            // Store the current index we're checking
  CHAR16      VarName[EFI_HW_ERR_REC_VAR_NAME_LEN];     // HwRecRecXXXX used to find variable

  for (OuterLoop = 0; OuterLoop <= MAX_UINT32; OuterLoop++) {

    UnicodeSPrint(VarName, sizeof(VarName), L"%s%04X", EFI_HW_ERR_REC_VAR_NAME, (UINT16)(OuterLoop & MAX_UINT16));

    Status = gRT->GetVariable(VarName, &gEfiHardwareErrorVariableGuid, NULL, &Size, NULL);

    if (Status == EFI_NOT_FOUND) {
      break;
    }

    gRT->SetVariable(VarName, &gEfiHardwareErrorVariableGuid, 0, 0, NULL);
  }
}

/**
 *  Created to test uefi application which enables/disables errors.
 *
 *  Simply run the application in the shell and add an argument after the
 *  .efi file which is one of the following:
 *
 *  For now, let's say 0 = No errors
 *                     1 = Errors every boot
 *                     2 = Errors only on next boot
 *                     3 = Delete Currently Stored Variables
 *                     4 = Raise an error right now. Can take 2 additional string arguments
 *                         to populate extradata1 and extradata2 in UEFI Generic section data
 *                         struct
 *  Expand this later
 *
 *  @param[in]    ImageHandle
 *  @param[in]    SystemTable
 *
 *  @retval       EFI_SUCCESS           The desired value was written to the variable
 *                EFI_PROTOCOL_ERROR    The shell parameter protocol could not be found
 *                EFI_NOT_FOUND         The variable could not be written to
 *                EFI_INVALID_PARAMETER The shell parameter was not a valid option
 *
**/
EFI_STATUS
EFIAPI
EnableDisableErrorsEntry(
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS                    Status;                             // Return Status
  EFI_SHELL_PARAMETERS_PROTOCOL *ShellParams;                       // Used to get Argv and Argc
  CHAR8                         Argument[MAX_NUM_DIGITS_READ + 1];  // Ascii interpretation of Argv[1]
  UINTN                         Result;                             // Integer interpretation of Argv[1]
  UINTN                         TempNum;                            // An integer needed by UnicodeStrnToAsciiStrS which is currently unused
  CHAR8                         *ExtraData1;                        // If logging an error immediately, can optionally put a string here for ExtraData1
  CHAR8                         *ExtraData2;                        // If logging an error immediately, can optionally put a string here for ExtraData2

  Argument[MAX_NUM_DIGITS_READ] = '\0';

  Status = gBS->HandleProtocol (
                  gImageHandle,
                  &gEfiShellParametersProtocolGuid,
                  (VOID**)&ShellParams
                  );

  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_INFO, "%a Could not retrieve command line args!\n",__FUNCTION__));
    return EFI_PROTOCOL_ERROR;
  }

  // Make sure theres an argument to view (besides the name of this file)
  if(ShellParams->Argc > 1) {

    // Convert the argument into Ascii so it can be converted to a digit
    UnicodeStrnToAsciiStrS(ShellParams->Argv[1],
                          StrnLenS(ShellParams->Argv[1], MAX_NUM_DIGITS_READ),
                          Argument,
                          MAX_NUM_DIGITS_READ + 1,
                          &TempNum
                          );

    // Get decimal version of input
    Result = AsciiStrDecimalToUintn(Argument);

      // Check these individually because I suspect it may be possible to pass in a number like "1.3" but haven't tried
      if(Result == SET_NO_ERRORS || Result == SET_CONT_ERRORS || Result == SET_ONE_TIME_ERRORS) {

        // Set the variable
        Status =  gRT->SetVariable(L"EnableDisableErrors",
                                  &gRaiseTelemetryErrorsAtBoot,
                                  EFI_VARIABLE_NON_VOLATILE |
                                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                                  sizeof(UINTN),
                                  &Result
                                  );

        // Debug print if we couldn't set the value
        if(EFI_ERROR(Status)) {
          DEBUG((DEBUG_INFO, "%a Could not set the enable/disable variable!\n",__FUNCTION__));
          return EFI_NOT_FOUND;
        }

      // Check if the user wants to generate an immediate error
      } else if (Result == GEN_ERROR_DIGIT) {

        // Allocate space for the strings they may have passed in. Zero fill it so the data is empty if they
        // did not supply anything
        ExtraData1 = AllocateZeroPool(MAX_CHARS_EXTRA_DATA + 1);
        ExtraData2 = AllocateZeroPool(MAX_CHARS_EXTRA_DATA + 1);

        // Make sure the strings were allocated successfully
        if (ExtraData1 == NULL || ExtraData2 == NULL) {
          DEBUG((DEBUG_INFO, "%a Failed to allocate strings for ExtraData1 and/or ExtraData2\n",__FUNCTION__));
          return EFI_OUT_OF_RESOURCES;
        }

        // Null terminate the strings
        ExtraData1[MAX_CHARS_EXTRA_DATA] = '\0';
        ExtraData2[MAX_CHARS_EXTRA_DATA] = '\0';

        // If there is a second argument to fill into the extra data 1
        if (ShellParams->Argc > 2) {

            // Fill it in
            UnicodeStrnToAsciiStrS(ShellParams->Argv[2],
                                  StrnLenS(ShellParams->Argv[2], MAX_CHARS_EXTRA_DATA),
                                  ExtraData1,
                                  MAX_CHARS_EXTRA_DATA + 1,
                                  &TempNum
                                  );

            // If there is a third argument to fill into the extradata2
            if (ShellParams->Argc > 3) {

              // Fill it in
              UnicodeStrnToAsciiStrS(ShellParams->Argv[3],
                                    StrnLenS(ShellParams->Argv[3], MAX_CHARS_EXTRA_DATA),
                                    ExtraData2,
                                    MAX_CHARS_EXTRA_DATA + 1,
                                    &TempNum
                                    );
            }
        }

        // Finally, log a telemetry event so a WHEA record is added to varstore
        Status = LogTelemetry (FALSE,NULL, 0, NULL, NULL, *((UINT64*)ExtraData1), *((UINT64*)ExtraData2));
      }

      // If the user wants to delete the errors
      else if (Result == DELETE_ERROR_DIGIT) {
        DEBUG((DEBUG_INFO, "%a Deleting WHEA Errors\n",__FUNCTION__));
        DeleteAllWheaErrors();
      }

      // Debug print that we did not receive a valid digit
      else {
        DEBUG((DEBUG_INFO, "%a Parameter argument was invalid\n",__FUNCTION__));
        return EFI_INVALID_PARAMETER;
      }
  }

  // Debug print that we need the user to input some arguments
  else {
      DEBUG((DEBUG_INFO, "%a Need to provide additional arguments\n",__FUNCTION__));
      return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}