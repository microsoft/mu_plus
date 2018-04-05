/** @file
 *DfciRequest

Copyright (c) 2018, Microsoft Corporation.

**/

#ifndef __DFCI_REQUEST_H__
#define __DFCI_REQUEST_H__

#define USER_STATUS_SUCCESS       0x0000000000000000L
#define USER_STATUS_NO_NIC        0x0000000000000001L
#define USER_STATUS_NO_MEDIA      0x0000000000000002L
#define USER_STATUS_NO_SETTINGS   0x0000000000000003L

/**
  Process the Dfci request

  Url                       Uniform Resource Locator
  HttpsCert                 Used to authenticate HTTPS servers
  UserStatus                Used when Status == NOT_FOUND to indicate
                            a specific message should be displayed

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
DfciRequestProcess(
    IN  CHAR8    *Url,
    IN  UINTN     UrlSize,
    IN  VOID     *HttpsCert  OPTIONAL,
    IN  UINTN     HttpsCertSize,

    // Ip Config Info: TBD
    // IPv4 or IPv6
    // Local IP is DHCP, or fixed etc

    OUT UINT64   *UserStatus
    );

#endif // __DFCI_REQUEST_H__

