
#include <PiDxe.h>

#include <Library/BaseCryptLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/DfciPkcs5PasswordHash.h>


DFCI_PKCS5_PASSWORD_HASH_PROTOCOL mPkcs5PwHashProtocol;


/**
Pkcs5 wrapper function - This is basically a pass thru to the BaseCryptLib .  


**/
EFI_STATUS
EFIAPI
HashUsingPkcs5(
  IN CONST  DFCI_PKCS5_PASSWORD_HASH_PROTOCOL   *This,
  IN        UINTN                                     PasswordSize,
  IN CONST  CHAR8                                     *Password,
  IN        UINTN                                     SaltSize,
  IN CONST  UINT8                                     *Salt,
  IN        UINTN                                     IterationCount,
  IN        UINTN                                     DigestSize,
  IN        UINTN                                     OutputSize,
  OUT       UINT8                                     *Output
  )
{
  if (This != &mPkcs5PwHashProtocol)
  {
    DEBUG((DEBUG_ERROR, "%a - Invalid This pointer\n", __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }

  return Pkcs5HashPassword(     PasswordSize,       // Size
                                Password,           // Password
                                SaltSize,           // SaltSize
                                Salt,               // Salt
                                IterationCount,     // IterationCount
                                DigestSize,         // DigestSize
                                OutputSize,         // OutputSize
                                Output );     
}




/**
Function to install Pkcs5 Protocol for other drivers to use

**/
EFI_STATUS
EFIAPI
InstallPkcs5Support(
IN EFI_HANDLE          ImageHandle
)
{
  mPkcs5PwHashProtocol.HashPassword = HashUsingPkcs5;

  return gBS->InstallMultipleProtocolInterfaces(
    &ImageHandle,
    &gDfciPKCS5PasswordHashProtocolGuid,
    &mPkcs5PwHashProtocol,
    NULL
    );
}

/**
Function to uninstall Pkcs5 Protocol

**/
EFI_STATUS
EFIAPI
UninstallPkcs5Support(
IN EFI_HANDLE          ImageHandle
)
{
  return EFI_SUCCESS;
}