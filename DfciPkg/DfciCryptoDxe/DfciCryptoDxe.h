/** @file
  Define local function interfaces

  Copyright (c) 2015, Microsoft Corporation. All rights reserved.<BR>

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



