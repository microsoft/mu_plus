/** @file
  UEFI Component Name(2) protocol implementation for HID Mouse Absolute Pointer Driver.

Copyright (c) Microsoft Corporation.

Derived from USB Mouse Absolute Pointer Driver, which is:
Copyright (c) 2004 - 2011, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "HidMouseAbsolutePointer.h"

//
// EFI Component Name Protocol
//
GLOBAL_REMOVE_IF_UNREFERENCED EFI_COMPONENT_NAME_PROTOCOL  mHidMouseAbsolutePointerComponentName = {
  HidMouseAbsolutePointerComponentNameGetDriverName,
  HidMouseAbsolutePointerComponentNameGetControllerName,
  "eng"
};

//
// EFI Component Name 2 Protocol
//
GLOBAL_REMOVE_IF_UNREFERENCED EFI_COMPONENT_NAME2_PROTOCOL  mHidMouseAbsolutePointerComponentName2 = {
  (EFI_COMPONENT_NAME2_GET_DRIVER_NAME)HidMouseAbsolutePointerComponentNameGetDriverName,
  (EFI_COMPONENT_NAME2_GET_CONTROLLER_NAME)HidMouseAbsolutePointerComponentNameGetControllerName,
  "en"
};

GLOBAL_REMOVE_IF_UNREFERENCED EFI_UNICODE_STRING_TABLE  mHidMouseAbsolutePointerDriverNameTable[] = {
  { "eng;en", L"HID Mouse Absolute Pointer Driver" },
  { NULL,     NULL                                 }
};

/**
  Retrieves a Unicode string that is the user readable name of the driver.

  This function retrieves the user readable name of a driver in the form of a
  Unicode string. If the driver specified by This has a user readable name in
  the language specified by Language, then a pointer to the driver name is
  returned in DriverName, and EFI_SUCCESS is returned. If the driver specified
  by This does not support the language specified by Language,
  then EFI_UNSUPPORTED is returned.

  @param  This                  A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.
  @param  Language              A pointer to a Null-terminated ASCII string
                                array indicating the language. This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified
                                in RFC 4646 or ISO 639-2 language code format.
  @param  DriverName            A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                driver specified by This in the language
                                specified by Language.

  @retval EFI_SUCCESS           The Unicode string for the Driver specified by
                                This and the language specified by Language was
                                returned in DriverName.
  @retval EFI_INVALID_PARAMETER Language is NULL.
  @retval EFI_INVALID_PARAMETER DriverName is NULL.
  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
HidMouseAbsolutePointerComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  )
{
  return LookupUnicodeString2 (
           Language,
           This->SupportedLanguages,
           mHidMouseAbsolutePointerDriverNameTable,
           DriverName,
           (BOOLEAN)(This == &mHidMouseAbsolutePointerComponentName)
           );
}

/**
  Retrieves a Unicode string that is the user readable name of the controller
  that is being managed by a driver.

  This function retrieves the user readable name of the controller specified by
  ControllerHandle and ChildHandle in the form of a Unicode string. If the
  driver specified by This has a user readable name in the language specified by
  Language, then a pointer to the controller name is returned in ControllerName,
  and EFI_SUCCESS is returned.  If the driver specified by This is not currently
  managing the controller specified by ControllerHandle and ChildHandle,
  then EFI_UNSUPPORTED is returned.  If the driver specified by This does not
  support the language specified by Language, then EFI_UNSUPPORTED is returned.

  @param  This                  A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.
  @param  ControllerHandle      The handle of a controller that the driver
                                specified by This is managing.  This handle
                                specifies the controller whose name is to be
                                returned.
  @param  ChildHandle           The handle of the child controller to retrieve
                                the name of.  This is an optional parameter that
                                may be NULL.  It will be NULL for device
                                drivers.  It will also be NULL for a bus drivers
                                that wish to retrieve the name of the bus
                                controller.  It will not be NULL for a bus
                                driver that wishes to retrieve the name of a
                                child controller.
  @param  Language              A pointer to a Null-terminated ASCII string
                                array indicating the language.  This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified in
                                RFC 4646 or ISO 639-2 language code format.
  @param  ControllerName        A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                controller specified by ControllerHandle and
                                ChildHandle in the language specified by
                                Language from the point of view of the driver
                                specified by This.

  @retval EFI_SUCCESS           The Unicode string for the user readable name in
                                the language specified by Language for the
                                driver specified by This was returned in
                                DriverName.
  @retval EFI_INVALID_PARAMETER ControllerHandle is NULL.
  @retval EFI_INVALID_PARAMETER ChildHandle is not NULL and it is not a valid
                                EFI_HANDLE.
  @retval EFI_INVALID_PARAMETER Language is NULL.
  @retval EFI_INVALID_PARAMETER ControllerName is NULL.
  @retval EFI_UNSUPPORTED       The driver specified by This is not currently
                                managing the controller specified by
                                ControllerHandle and ChildHandle.
  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
HidMouseAbsolutePointerComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  EFI_HANDLE                   ChildHandle        OPTIONAL,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **ControllerName
  )
{
  EFI_STATUS                      Status;
  HID_MOUSE_ABSOLUTE_POINTER_DEV  *HidMouseDev;
  HID_POINTER_PROTOCOL            *HidMouseProtocol;
  EFI_ABSOLUTE_POINTER_PROTOCOL   *AbsolutePointerProtocol;

  //
  // This is a device driver, so ChildHandle must be NULL.
  //
  if (ChildHandle != NULL) {
    return EFI_UNSUPPORTED;
  }

  //
  // Check Controller's handle
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gHidPointerProtocolGuid,
                  (VOID **)&HidMouseProtocol,
                  mHidMouseAbsolutePointerDriverBinding.DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (!EFI_ERROR (Status)) {
    gBS->CloseProtocol (
           ControllerHandle,
           &gHidPointerProtocolGuid,
           mHidMouseAbsolutePointerDriverBinding.DriverBindingHandle,
           ControllerHandle
           );

    return EFI_UNSUPPORTED;
  }

  if (Status != EFI_ALREADY_STARTED) {
    return EFI_UNSUPPORTED;
  }

  //
  // Get the device context
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiAbsolutePointerProtocolGuid,
                  (VOID **)&AbsolutePointerProtocol,
                  mHidMouseAbsolutePointerDriverBinding.DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  HidMouseDev = HID_MOUSE_ABSOLUTE_POINTER_DEV_FROM_MOUSE_PROTOCOL (AbsolutePointerProtocol);

  return LookupUnicodeString2 (
           Language,
           This->SupportedLanguages,
           HidMouseDev->ControllerNameTable,
           ControllerName,
           (BOOLEAN)(This == &mHidMouseAbsolutePointerComponentName)
           );
}
