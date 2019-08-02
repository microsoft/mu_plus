/** @file
  Define local function interfaces

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/



//------------------------------------------------------------------
// PKCS 7
//------------------------------------------------------------------

/**
Function to install Pkcs7 Protocol

**/
EFI_STATUS
EFIAPI
InstallPkcs7Support(
IN EFI_HANDLE          ImageHandle
);

/**
Function to uninstall Pkcs7 Protocol

**/
EFI_STATUS
EFIAPI
UninstallPkcs7Support(
IN EFI_HANDLE          ImageHandle
);


//------------------------------------------------------------------
// PKCS 5
//------------------------------------------------------------------
/**
Function to install Pkcs5 Protocol

**/
EFI_STATUS
EFIAPI
InstallPkcs5Support(
IN EFI_HANDLE          ImageHandle
);

/**
Function to uninstall Pkcs5 Protocol

**/
EFI_STATUS
EFIAPI
UninstallPkcs5Support(
IN EFI_HANDLE          ImageHandle
);



