/** @file
LogDumper.c

This application will dump the AdvancedLog to a file.

Copyright (C) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <LogDumper.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/ShellDynamicCommand.h>

EFI_HII_HANDLE  gAdvLogHiiHandle;

CHAR16 *
EFIAPI
AdvLogDumpGetHelp (
  IN EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL  *This,
  IN CONST CHAR8                         *Language
  );

SHELL_STATUS
EFIAPI
AdvLogDumpCommandHandler (
  IN EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL  *This,
  IN EFI_SYSTEM_TABLE                    *SystemTable,
  IN EFI_SHELL_PARAMETERS_PROTOCOL       *ShellParameters,
  IN EFI_SHELL_PROTOCOL                  *Shell
  );

STATIC EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL  mAdvLogDumperDynamicCommand = {
  L"Advlogdump",
  AdvLogDumpCommandHandler,
  AdvLogDumpGetHelp
};

/**
  This functions displays help message for AdvLogDump command.

  @param[in] This                   The instance of the EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL.
  @param[in] Language               The pointer to the language string to use.

  @return string                    Pool allocated help string, must be freed by caller
**/
CHAR16 *
EFIAPI
AdvLogDumpGetHelp (
  IN EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL  *This,
  IN CONST CHAR8                         *Language
  )
{
  return HiiGetString (gAdvLogHiiHandle, STRING_TOKEN (STR_ADV_LOG_HELP), Language);
}

/**
  This function handles the AdvLogDump command.

  @param[in] This                   The instance of the EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL.
  @param[in] Language               The pointer to the language string to use.

  @return string                    Pool allocated help string, must be freed by caller
**/
SHELL_STATUS
EFIAPI
AdvLogDumpCommandHandler (
  IN EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL  *This,
  IN EFI_SYSTEM_TABLE                    *SystemTable,
  IN EFI_SHELL_PARAMETERS_PROTOCOL       *ShellParameters,
  IN EFI_SHELL_PROTOCOL                  *Shell
  )
{
  EFI_STATUS  Status;

  gEfiShellParametersProtocol = ShellParameters;
  gEfiShellProtocol           = Shell;

  Status = ShellInitialize ();
  ASSERT_EFI_ERROR (Status);

  return AdvLogDumperInternalWorker (gImageHandle, SystemTable);
}

/**
  The user Entry Point for LogDumper driver.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
AdvancedLogDumperEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  gAdvLogHiiHandle = InitializeHiiPackage (ImageHandle);
  if (gAdvLogHiiHandle == NULL) {
    return EFI_ABORTED;
  }

  Status = gBS->InstallProtocolInterface (
                  &ImageHandle,
                  &gEfiShellDynamicCommandProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mAdvLogDumperDynamicCommand
                  );
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/**
  AdvancedLogDump unload handler.

  @param ImageHandle            The image handle of the process.

  @retval EFI_SUCCESS           The image is unloaded.
  @retval Others                Failed to unload the image.
**/
EFI_STATUS
EFIAPI
AdvancedLogDumperUnload (
  IN EFI_HANDLE  ImageHandle
  )
{
  EFI_STATUS  Status;

  Status = gBS->UninstallProtocolInterface (
                  ImageHandle,
                  &gEfiShellDynamicCommandProtocolGuid,
                  &mAdvLogDumperDynamicCommand
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  HiiRemovePackages (gAdvLogHiiHandle);
  return EFI_SUCCESS;
}