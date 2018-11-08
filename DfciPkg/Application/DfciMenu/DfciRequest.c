/** @file
DfciRequest.c

This module will request new DFCI configuration data from server.

Copyright (c) 2018, Microsoft Corporation

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

#include <Uefi.h>

#include <IndustryStandard/Http11.h>

#include <Guid/DfciPacketHeader.h>
#include <Guid/DfciIdentityAndAuthManagerVariables.h>
#include <Guid/DfciPermissionManagerVariables.h>
#include <Guid/DfciSettingsManagerVariables.h>

#include <Protocol/BootManagerPolicy.h>
#include <Protocol/Http.h>
#include <Protocol/Ip4Config2.h>
#include <Protocol/ServiceBinding.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DfciDeviceIdSupportLib.h>
#include <Library/HttpLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/NetLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include "DfciMenu.h"
#include "DfciRequest.h"

#define URL_STR_MAX_SIZE      255
#define TIMER_PERIOD_1ms     (1000 * 10)    // 1ms value for relative timer.
#define TIMER_PERIOD_1s      (1000 * TIMER_PERIOD_1ms)
#define DHCP_TIMEOUT         (120 * TIMER_PERIOD_1s)
#define HTTP_TIMEOUT         (60 * TIMER_PERIOD_1s)

#define DFCI_REQUEST         L"DfciRequest/"
#define DFCI_IDENTITY        L"/Identity"
#define DFCI_IDENTITY2       L"/Identity2"
#define DFCI_PERMISSIONS     L"/Permissions"
#define DFCI_PERMISSIONS2    L"/Permissions2"
#define DFCI_SETTINGS        L"/Settings"
#define DFCI_SETTINGS2       L"/Settings2"
#define DFCI_CURRENT         L"/Current"

#define HEADER_AGENT_VALUE   "DFCI-Agent"
#define HEADER_ACCEPT_VALUE  "*/*"
#define HEADER_CONTENT_BIN   "application/octet-stream"
#define HEADER_CONTENT_XML   "application/xml"

typedef struct {
    CHAR16     *RequestType;
    UINT32      Signature;
    UINT32      VariableAttributes;
    CHAR16     *VariableName;
    CHAR16     *ResultsVariableName;
    EFI_GUID   *VariableNameSpace;
    CHAR8      *ContentType;
} PROCESS_REQUEST_ENTRY;

STATIC UINT64                mUserStatus = USER_STATUS_SUCCESS;
STATIC PROCESS_REQUEST_ENTRY mRequestTable[] = {

    {DFCI_IDENTITY,
     DFCI_IDENTITY_APPLY_VAR_SIGNATURE,
     DFCI_IDENTITY_VAR_ATTRIBUTES,
     DFCI_IDENTITY_APPLY_VAR_NAME,
     DFCI_IDENTITY_RESULT_VAR_NAME,
   &gDfciAuthProvisionVarNamespace,
     HEADER_CONTENT_BIN},

    {DFCI_IDENTITY2,
     DFCI_IDENTITY_APPLY_VAR_SIGNATURE,
     DFCI_IDENTITY_VAR_ATTRIBUTES,
     DFCI_IDENTITY2_APPLY_VAR_NAME,
     DFCI_IDENTITY2_RESULT_VAR_NAME,
    &gDfciAuthProvisionVarNamespace,
     HEADER_CONTENT_BIN},

    {DFCI_PERMISSIONS,
     DFCI_PERMISSION_POLICY_APPLY_VAR_SIGNATURE,
     DFCI_PERMISSION_POLICY_APPLY_VAR_ATTRIBUTES,
     DFCI_PERMISSION_POLICY_APPLY_VAR_NAME,
     DFCI_PERMISSION_POLICY_RESULT_VAR_NAME,
    &gDfciPermissionManagerVarNamespace,
     HEADER_CONTENT_BIN},

    {DFCI_PERMISSIONS2,
     DFCI_PERMISSION_POLICY_APPLY_VAR_SIGNATURE,
     DFCI_PERMISSION_POLICY_APPLY_VAR_ATTRIBUTES,
     DFCI_PERMISSION2_POLICY_APPLY_VAR_NAME,
     DFCI_PERMISSION2_POLICY_RESULT_VAR_NAME,
    &gDfciPermissionManagerVarNamespace,
     HEADER_CONTENT_BIN},

    {DFCI_SETTINGS,
     DFCI_SECURED_SETTINGS_APPLY_VAR_SIGNATURE,
     DFCI_SECURED_SETTINGS_VAR_ATTRIBUTES,
     DFCI_SETTINGS_APPLY_INPUT_VAR_NAME,
     DFCI_SETTINGS_APPLY_OUTPUT_VAR_NAME,
    &gDfciSettingsManagerVarNamespace,
     HEADER_CONTENT_BIN},

    {DFCI_SETTINGS2,
     DFCI_SECURED_SETTINGS_APPLY_VAR_SIGNATURE,
     DFCI_SECURED_SETTINGS_VAR_ATTRIBUTES,
     DFCI_SETTINGS2_APPLY_INPUT_VAR_NAME,
     DFCI_SETTINGS2_APPLY_OUTPUT_VAR_NAME,
    &gDfciSettingsManagerVarNamespace,
     HEADER_CONTENT_BIN},

    {DFCI_CURRENT,
     0,
     0,
     NULL,
     DFCI_SETTINGS_CURRENT_OUTPUT_VAR_NAME,
    &gDfciSettingsManagerVarNamespace,
     HEADER_CONTENT_XML}
};

#define REQUEST_TABLE_COUNT (sizeof(mRequestTable)/sizeof(mRequestTable[0]))

/**
 * DFCI Private Data
 **/
typedef struct {
    //
    // Parameters
    //
    CHAR8                          *Url;
    UINTN                           UrlSize;
    //
    // Device Id
    //
    CHAR8                           *Manufacturer;
    UINTN                            ManufacturerSize;
    CHAR8                           *ProductName;
    UINTN                            ProductNameSize;
    CHAR8                           *SerialNumber;
    UINTN                            SerialNumberSize;
    CHAR8                           *Uuid;
    UINTN                            UuidSize;

    //
    // Common section  -- From here to the end cleared before each NIC attempt
    //
    EFI_HANDLE                      NicHandle;
    EFI_SERVICE_BINDING_PROTOCOL   *HttpSbProtocol;
    EFI_HTTP_CONFIG_DATA            ConfigData;
    EFI_HTTP_PROTOCOL              *HttpProtocol;
    EFI_HANDLE                      HttpChildHandle;
    BOOLEAN                         DhcpRequested;
    EFI_IP4_CONFIG2_PROTOCOL       *Ip4Config2;

    //
    // Fields valid during DHCP delay
    //
    EFI_EVENT                       WaitEvent;

    //
    // IPv4 Sspecific section
    //
    EFI_HTTPv4_ACCESS_POINT         IPv4Node;

    //
    // IPv6 Specific section
    //
    EFI_HTTPv6_ACCESS_POINT         IPv6Node;

} DFCI_PRIVATE_DATA;

/**
 * Dump the HTTP Headers
 *
 * @param[in] Count
 * @param[out] Headers
 *
 * @return VOID
 *
 */
STATIC
VOID
DumpHeaders (
    IN  UINTN            Count,
    IN  EFI_HTTP_HEADER *Headers
  ) {

    DEBUG_CODE (
        UINTN   Index;


        for (Index = 0; Index < Count; Index++) {
            DEBUG((DEBUG_INFO,"  %d - %a = %a\n", Index + 1,
                               Headers[Index].FieldName,
                               Headers[Index].FieldValue));
        }
    );
}

/**
 * Free the HTTP Headers.  Each element is a separate allocation.
 *
 * @param Count      Count of entries in the array
 * @param Headers  - Array of HTTP Headers
 *
 * @return VOID
 *
 **/
STATIC
VOID
FreeHeaders (
    IN  UINTN            Count,
    IN  EFI_HTTP_HEADER *Headers
  ) {

    UINTN   Index;

    if (Headers != NULL) {
        for (Index = 0; Index < Count; Index++) {
          FreePool (Headers[Index].FieldName);
          FreePool (Headers[Index].FieldValue);
        }
        FreePool (Headers);
    }
}

/**
 * Wait for an event to be signaled.  While waiting, Poll the HTTP operation
 *
 * @param[in]  Dfci       - Private data
 * @param[in]  MainEvent  - The event to wait on
 * @param[in]  Timeout    - Timeout in SetTimer units
 *
 * @return EFI_STATUS  - EFI_SUCCESS - Wait completed normally
 *                       EFI_TIMEOUT - Event Timed out
 *                       Other       - Wait failed for other reasons
 *
 **/
STATIC
EFI_STATUS
EventWait (
    IN  DFCI_PRIVATE_DATA   *Dfci,
    IN  EFI_EVENT            MainEvent,
    IN  UINTN                Timeout
  ) {

    BOOLEAN     Failed;
    EFI_STATUS  Status;
    UINTN       Step;
    EFI_EVENT   TimeOutEvent;


    Failed = TRUE;
    do {
        Step = 1;
        TimeOutEvent = NULL;
        Status = gBS->CreateEvent (EVT_TIMER, 0, NULL, NULL, &TimeOutEvent);
        if (EFI_ERROR(Status)) {
            break;
        }

        Step = 2;
        //
        // Set the timeout event
        //
        Status = gBS->SetTimer (
            TimeOutEvent,
            TimerRelative,
            Timeout
            );
        if (EFI_ERROR(Status)){
            break;
        }

        Step = 3;
        do {
            if (NULL != Dfci->HttpProtocol) {
                Dfci->HttpProtocol->Poll(Dfci->HttpProtocol);
            }

            if (EFI_SUCCESS == gBS->CheckEvent(MainEvent)) {
                Status = EFI_SUCCESS;
                break;
            }
            if (EFI_SUCCESS == gBS->CheckEvent(TimeOutEvent)) {
                Status = EFI_TIMEOUT;
                break;
            }
        } while (TRUE);
        Failed = FALSE;    // Once we get here, the processed is complete

        Step = 4;
        gBS->SetTimer(TimeOutEvent, TimerCancel, 0);
        gBS->CloseEvent (TimeOutEvent);

    } while (FALSE);

    if (Failed){
        DEBUG((DEBUG_ERROR, "Wait error at step %d - code=%r\n", Step, Status));
    }

    return Status;
}

/**
 *  Timer Tick handler  - Update the list of devices
 *
 *  @param[in] Event      Event that signaled the callback.
 *  @param[in] Context    Pointer to an optional event context.
 *
 *  @retval None.
 *
 **/
STATIC
VOID
EFIAPI TimerTick (
    IN EFI_EVENT  Event,
    IN VOID      *Context
  ) {

    EFI_IP4_CONFIG2_INTERFACE_INFO  *Info;
    UINTN                            DataSize;
    DFCI_PRIVATE_DATA               *Dfci;
    EFI_STATUS                       Status;


    Dfci = (DFCI_PRIVATE_DATA *) Context;
    DataSize = 0;
    Status = Dfci->Ip4Config2->GetData (Dfci->Ip4Config2,
                                        Ip4Config2DataTypeInterfaceInfo,
                                       &DataSize,
                                       &Info);
    if (EFI_BUFFER_TOO_SMALL != Status) {
        DEBUG((DEBUG_ERROR, "Error obtaining IP4 Interface Info size. Code=%r\n", Status));
    } else {
        Info = AllocatePool (DataSize);
        if (NULL == Info) {
            DEBUG((DEBUG_ERROR, "Error allocating %d bytes for Info\n", DataSize));
        } else {
            Info->StationAddress.Addr[0] = 0;
            Status = Dfci->Ip4Config2->GetData (Dfci->Ip4Config2,
                                                Ip4Config2DataTypeInterfaceInfo,
                                               &DataSize,
                                                Info);
            if (EFI_ERROR(Status)) {
                DEBUG((DEBUG_ERROR, "Error obtaining IP4 Interface Info. Code=%r\n", Status));
                DEBUG((DEBUG_ERROR, " DataSize=%d, StructSize=%d\n",
                                     DataSize,
                                     sizeof(EFI_IP4_CONFIG2_INTERFACE_INFO) ));
            } else {
                if (Info->StationAddress.Addr[0] != 0) {
                    gBS->SignalEvent (Dfci->WaitEvent);
                }
            }
            FreePool (Info);
        }
    }
}

/**
 * Configure Static address of IP=0.0.0.0 SubNet=0.0.0.0 GateWay=0.0.0.0
 *
 * @param Dfci  - Internal data
 *
 * @retval EFI_STATUS
 *
 **/
STATIC
EFI_STATUS
ConfigureSTATIC (
    IN DFCI_PRIVATE_DATA *Dfci
  ) {

    EFI_IP4_CONFIG2_MANUAL_ADDRESS   Address;
    EFI_EVENT                        AddressEvent;
    EFI_IPv4_ADDRESS                 Gateway;
    EFI_IP4_CONFIG2_PROTOCOL        *Ip4Config2;
    EFI_IP4_CONFIG2_POLICY           Policy;
    EFI_STATUS                       Status2;
    EFI_STATUS                       Status;


    Ip4Config2 = Dfci->Ip4Config2;

    //
    // 1. Set Policy to Static IP
    //
    Policy = Ip4Config2PolicyStatic;
    Status = Ip4Config2->SetData (Ip4Config2,
                                  Ip4Config2DataTypePolicy,
                                  sizeof(EFI_IP4_CONFIG2_POLICY),
                                  &Policy);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "Unable to set policy to static.. Code=%r\n", Status));
        return Status;
    }

    //
    // 2. Set IP Addr and Subnet to 0.0.0.0 0.0.0.0
    //
    Status = gBS->CreateEvent(0,0,NULL,NULL,&AddressEvent);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "Unable to create wait event. Code=%r\n", Status));
        return Status;
    }

    Status = Ip4Config2->RegisterDataNotify (Ip4Config2,
                                             Ip4Config2DataTypeManualAddress,
                                             AddressEvent);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "Unable to register for Dhcp Events. Code=%r\n", Status));
        gBS->CloseEvent (AddressEvent);
        return Status;
    }

    ZeroMem(&Address,sizeof(EFI_IP4_CONFIG2_MANUAL_ADDRESS));
    Status = Ip4Config2->SetData (Ip4Config2,
                                  Ip4Config2DataTypeManualAddress,
                                  sizeof(EFI_IP4_CONFIG2_MANUAL_ADDRESS),
                                  &Address);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "Unable to set manual address. Code=%r\n", Status));
    } else {
        Status = EventWait (Dfci, AddressEvent, DHCP_TIMEOUT);
        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "Error from wait for SetData->Static. Code=%r\n", Status));
        }
    }

    Status2 = Ip4Config2->UnregisterDataNotify (Ip4Config2,
                                                Ip4Config2DataTypeManualAddress,
                                                AddressEvent);
    if (EFI_ERROR(Status2)) {
        DEBUG((DEBUG_ERROR, "Error from Unregister. Code=%r\n", Status));
    }

    gBS->CloseEvent (AddressEvent);

    //
    // 3. Set Gateway address to 0.0.0.0
    //
    ZeroMem(&Gateway,sizeof(EFI_IPv4_ADDRESS));
    Status = Ip4Config2->SetData (Ip4Config2,
                                  Ip4Config2DataTypeGateway,
                                  sizeof(EFI_IPv4_ADDRESS),
                                  &Gateway);
    if (EFI_ERROR(Status2)) {
        DEBUG((DEBUG_ERROR, "Error setting GateWay address. Code=%r\n", Status));
    }

    return Status;
}

/**
 * Configure DHCP address
 *
 * @param[in]  Dfci
 *
 * @return EFI_STATUS
 *
 **/
STATIC
EFI_STATUS
ConfigureDHCP (
    IN DFCI_PRIVATE_DATA *Dfci
  ) {

    EFI_IP4_CONFIG2_PROTOCOL        *Ip4Config2;
    EFI_IP4_CONFIG2_POLICY           Policy;
    EFI_STATUS                       Status;
    EFI_EVENT                        TimerEvent;


    if (NULL == Dfci) {
        return EFI_INVALID_PARAMETER;
    }

    //
    // 1. Set state back to Static
    //
    Status = gBS->CreateEvent(0,0,NULL,NULL,&Dfci->WaitEvent);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "Unable to create wait event. Code=%r\n", Status));
        return Status;
    }

    do {
        Status = ConfigureSTATIC (Dfci);

        if (EFI_ERROR(Status)) {
            break;
        }

        //
        // 2. Set policy to DHCP - this should start a DHCP DORA
        //
        Ip4Config2 = Dfci->Ip4Config2;

        Policy = Ip4Config2PolicyDhcp;
        Status = Ip4Config2->SetData (Ip4Config2,
                                      Ip4Config2DataTypePolicy,
                                      sizeof(EFI_IP4_CONFIG2_POLICY),
                                      &Policy);
        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "Error from SetData->Dhcp. Code=%r\n", Status));
            break;
        }
        Dfci->DhcpRequested = TRUE;    // Remember to set back to STATIC

        //
        // 3. Poll the IP4 Address until valid
        //
        Status = gBS->CreateEvent (EVT_TIMER | EVT_NOTIFY_SIGNAL,
                                   TPL_CALLBACK,
                                   TimerTick,
                                   (VOID *) Dfci,
                                   &TimerEvent);
        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "Unable to create event DHCP Completion. Code=%r\n", Status));
            break;
        }

        //
        // Set the timer event to poll for an IP4 address every 1 second
        //
        Status = gBS->SetTimer (TimerEvent,
                                TimerPeriodic,
                                TIMER_PERIOD_1s
                                );
        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "Unable to set timer for DHCP Completion. Code=%r\n", Status));
            break;
        }
        Status = EventWait (Dfci, Dfci->WaitEvent, DHCP_TIMEOUT);
        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "Error from wait on DHCP address. Code=%r\n", Status));
        } else {
            DEBUG((DEBUG_INFO, "DHCP Address satisfied.\n"));
        }

        gBS->SetTimer (TimerEvent,
                       TimerCancel,
                       0);
        gBS->CloseEvent (TimerEvent);

    } while (FALSE);

    gBS->CloseEvent (Dfci->WaitEvent);
    Dfci->WaitEvent = NULL;

    return Status;
}

/**
 * Configure the NIC for an IPv4 or IPv6 address
 *
 *
 * @param Dfci
 *
 * @return EFI_STATUS
 *
 */
STATIC
EFI_STATUS
ConfigureHTTP (
    IN DFCI_PRIVATE_DATA *Dfci
  ) {

    EFI_IP4_CONFIG2_MANUAL_ADDRESS   Address;
    UINTN                            DataSize;
    EFI_STATUS                       Status;


    if (NULL == Dfci) {
        return EFI_INVALID_PARAMETER;
    }

    if (Dfci->ConfigData.LocalAddressIsIPv6) {
        DEBUG((DEBUG_ERROR, "IPv6 is not supported yet\n"));
        Status = EFI_UNSUPPORTED;
    } else {
        //
        // Initialize HTTP Config Data
        //
        Dfci->ConfigData.HttpVersion = HttpVersion11;
        Dfci->ConfigData.TimeOutMillisec = 0;             // Indicates default timeout period
        Dfci->ConfigData.LocalAddressIsIPv6 = FALSE;

        ZeroMem(&Dfci->IPv4Node, sizeof(Dfci->IPv4Node));
        Dfci->IPv4Node.UseDefaultAddress = TRUE;          // Use address configured already
        Dfci->ConfigData.AccessPoint.IPv4Node = &Dfci->IPv4Node;

        //
        // Check the current IP Address.  If an IP address not present, ConfigureDHCP
        //
        Status = gBS->HandleProtocol(Dfci->NicHandle, &gEfiIp4Config2ProtocolGuid, &Dfci->Ip4Config2);
        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "Error locating IPv4 Config2 protocol. Code=%r\n", Status));
            return Status;
        }

        DataSize = sizeof(EFI_IP4_CONFIG2_MANUAL_ADDRESS);
        Status = Dfci->Ip4Config2->GetData (Dfci->Ip4Config2,
                                            Ip4Config2DataTypeManualAddress,
                                           &DataSize,
                                           &Address);
        if (EFI_ERROR(Status) || (Address.Address.Addr[0] == 0)) {
            DEBUG((DEBUG_ERROR, "Configuring DHCP for DFCI. Code=%r\n", Status));
            Status = ConfigureDHCP(Dfci);
        }
    }

    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "Error configuring HTTP. Code=%r\n", Status));
        return Status;
    }

    //
    // Continue with Common Configuration.
    //

    Status = Dfci->HttpSbProtocol->CreateChild(Dfci->HttpSbProtocol, &Dfci->HttpChildHandle);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "Error creating worker child. Code=%r\n", Status));
        return Status;
    }

    Dfci->HttpProtocol = NULL;
    Status = gBS->HandleProtocol(Dfci->HttpChildHandle, &gEfiHttpProtocolGuid, &Dfci->HttpProtocol);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "Unable to locate HTTP protocol. Code=%r\n", Status));
        return Status;
    }


    Status = Dfci->HttpProtocol->Configure(Dfci->HttpProtocol, &Dfci->ConfigData);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "Unable to configure HTTP Protocol. Code=%r\n", Status));
        return Status;
    }
    return Status;
}

/**
 * DfciBuildRequestHeaders
 *
 * @param[in]  Url
 * @param[in]  BodyLength
 * @param[in]  ContentType
 * @param[out] Headers
 * @param[out] Count
 *
 *
 * @return EFI_STATUS
 *
 **/
STATIC
EFI_STATUS
DfciBuildRequestHeaders (
    IN  CHAR16           *Url,
    IN  UINTN             BodyLength,
    IN  CONST CHAR8      *ContentType,
    OUT EFI_HTTP_HEADER **Headers,
    OUT UINTN            *Count
  ) {

    EFI_STATUS            Status;
    VOID                 *UrlParser;
    EFI_HTTP_HEADER      *RequestHeaders;
    CHAR8                *AsciiUrl;
    UINTN                 AsciiUrlLen;
    UINTN                 HeaderCount;
    CHAR8                 ContentLengthString[21];    //2**64 is 1.8E19, or 20 digits. +1 for a NULL.


    if ((NULL == Url) ||
        (NULL == ContentType) ||
        (NULL == Headers) ||
        (NULL == Count)) {
        return EFI_INVALID_PARAMETER;
    }

    AsciiUrlLen = StrLen (Url);
    AsciiUrl = (CHAR8 *) AllocatePool (AsciiUrlLen + 1);
    if (NULL == AsciiUrl) {
        return EFI_OUT_OF_RESOURCES;
    }

    Status = UnicodeStrToAsciiStrS (Url, AsciiUrl, AsciiUrlLen + 1);
    if (EFI_ERROR(Status)) {
        FreePool (AsciiUrl);
        return Status;
    }

    if (0 != BodyLength){
        HeaderCount = 5;
    } else {
        HeaderCount = 3;
    }

    RequestHeaders = AllocateZeroPool (sizeof(EFI_HTTP_HEADER) * HeaderCount);  // Allocate headers
    if (NULL == RequestHeaders) {
        FreePool (AsciiUrl);
        return EFI_OUT_OF_RESOURCES;
    }

    UrlParser = NULL;

    Status = HttpParseUrl (AsciiUrl, (UINT32) AsciiUrlLen, FALSE, &UrlParser);
    if (EFI_ERROR (Status)) {
        FreePool (AsciiUrl);
        return Status;
    }

    //
    // Don't check each AllocateCopyPool.  All of the pointers are initialized as NULL, and those that work
    // will be freed by FreeHeaders.  Leave the failure detection to the HTTP operation failing.
    //
    RequestHeaders[0].FieldName  = AllocateCopyPool (AsciiStrSize (HTTP_HEADER_HOST), HTTP_HEADER_HOST);
    RequestHeaders[1].FieldName  = AllocateCopyPool (AsciiStrSize (HTTP_HEADER_USER_AGENT), HTTP_HEADER_USER_AGENT);
    RequestHeaders[1].FieldValue = AllocateCopyPool (AsciiStrSize (HEADER_AGENT_VALUE), HEADER_AGENT_VALUE);
    RequestHeaders[2].FieldName  = AllocateCopyPool (AsciiStrSize (HTTP_HEADER_ACCEPT), HTTP_HEADER_ACCEPT);
    RequestHeaders[2].FieldValue = AllocateCopyPool (AsciiStrSize (HEADER_ACCEPT_VALUE), HEADER_ACCEPT_VALUE);

    Status = HttpUrlGetHostName (AsciiUrl, UrlParser, &RequestHeaders[0].FieldValue);
    if (EFI_ERROR (Status)) {
        DEBUG((DEBUG_ERROR, "Unable to get Host Name from URL\n"));
    }

    if (0 != BodyLength){
        RequestHeaders[3].FieldName = AllocateCopyPool (AsciiStrSize (HTTP_HEADER_CONTENT_LENGTH), HTTP_HEADER_CONTENT_LENGTH);
        AsciiSPrint (ContentLengthString, sizeof(ContentLengthString),"%ld",BodyLength);
        RequestHeaders[3].FieldValue = AllocateCopyPool (AsciiStrSize (ContentLengthString), ContentLengthString);
        RequestHeaders[4].FieldName = AllocateCopyPool (AsciiStrSize (HTTP_HEADER_CONTENT_TYPE), HTTP_HEADER_CONTENT_TYPE);
        RequestHeaders[4].FieldValue = AllocateCopyPool (AsciiStrSize (ContentType), ContentType);
    }

    FreePool (AsciiUrl);
    *Headers = RequestHeaders;
    *Count = HeaderCount;
    HttpUrlFreeParser(UrlParser);

    return Status;
}

/**
 * DfciIssueRequest
 *
 *
 * @param[in]  Dfci
 * @param[in]  RequestToken
 *
 * @return EFI_STATUS
 *
 **/
STATIC
EFI_STATUS
DfciIssueRequest (
    IN  DFCI_PRIVATE_DATA  *Dfci,
    IN  EFI_HTTP_TOKEN     *RequestToken
  ) {

    EFI_HTTP_REQUEST_DATA  *RequestData;
    EFI_HTTP_MESSAGE       *RequestMessage;
    EFI_STATUS              Status;
    EFI_STATUS              Status2;


    if ((NULL == Dfci) ||
        (NULL == RequestToken)) {
        return EFI_INVALID_PARAMETER;
    }

    Status = gBS->CreateEvent(0, 0, NULL, NULL, &RequestToken->Event);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "Unable to create callback event. Code=%r\n", Status));
        return Status;
    }

    RequestMessage = RequestToken->Message;
    RequestData = RequestMessage->Data.Request;

    DEBUG((DEBUG_INFO, "Making Request - Headers:\n"));
    DumpHeaders (RequestMessage->HeaderCount, RequestMessage->Headers);
    DEBUG((DEBUG_INFO, "HttpRequestToken:\n"));
    DEBUG_BUFFER(DEBUG_INFO, RequestToken, sizeof(EFI_HTTP_TOKEN), DEBUG_DM_PRINT_ADDRESS | DEBUG_DM_PRINT_ASCII);
    DEBUG_BUFFER(DEBUG_INFO, RequestMessage, sizeof(EFI_HTTP_MESSAGE), DEBUG_DM_PRINT_ADDRESS | DEBUG_DM_PRINT_ASCII);
    DEBUG_BUFFER(DEBUG_INFO, RequestData, sizeof(EFI_HTTP_REQUEST_DATA), DEBUG_DM_PRINT_ADDRESS | DEBUG_DM_PRINT_ASCII);
    DEBUG((DEBUG_INFO, "%p Url=%s\n", RequestData->Url,RequestData->Url));

    Status = Dfci->HttpProtocol->Request(Dfci->HttpProtocol, RequestToken);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "Http Request failed. Code=%r\n", Status));
        gBS->CloseEvent(RequestToken->Event);
        return Status;
    }

    Status = EventWait (Dfci, RequestToken->Event, HTTP_TIMEOUT);
    gBS->CloseEvent(RequestToken->Event);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "Http request timed out\n"));
        Status2 = Dfci->HttpProtocol->Cancel(Dfci->HttpProtocol, RequestToken);
        if (EFI_ERROR(Status2)) {
            DEBUG((DEBUG_ERROR, "Http Cancel failed. Code=%r\n", Status));
        }
    }
    DEBUG((DEBUG_INFO, "Request Token status = %r\n", RequestToken->Status));
    DEBUG((DEBUG_INFO, "DfciIssueRequest status = %r\n", Status));

    return Status;
}

/**
 * DfciGetResponse
 *
 *
 * @param[in]  Dfci
 * @param[in]  ResponseToken
 *
 * @return EFI_STATUS
 *
 **/
STATIC
EFI_STATUS
DfciGetResponse (
    IN  DFCI_PRIVATE_DATA  *Dfci,
    IN  EFI_HTTP_TOKEN     *ResponseToken
  ) {

    EFI_HTTP_RESPONSE_DATA *ResponseData;
    EFI_HTTP_MESSAGE       *ResponseMessage;
    EFI_STATUS              Status;
    EFI_STATUS              Status2;


    if ((NULL == Dfci) ||
        (NULL == ResponseToken)) {
        return EFI_INVALID_PARAMETER;
    }

    Status = gBS->CreateEvent(0,0,NULL,NULL,&ResponseToken->Event);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "Unable to create callback event. Code=%r\n", Status));
        return Status;
    }

    ResponseMessage = ResponseToken->Message;
    ResponseData = ResponseMessage->Data.Response;

    DEBUG((DEBUG_INFO, "HttpResponseToken:\n"));
    DEBUG_BUFFER(DEBUG_INFO, ResponseToken, sizeof(EFI_HTTP_TOKEN), DEBUG_DM_PRINT_ADDRESS | DEBUG_DM_PRINT_ASCII);
    DEBUG_BUFFER(DEBUG_INFO, ResponseMessage, sizeof(EFI_HTTP_MESSAGE), DEBUG_DM_PRINT_ADDRESS | DEBUG_DM_PRINT_ASCII);
    if (NULL != ResponseData) {
        DEBUG_BUFFER(DEBUG_INFO, ResponseData, sizeof(EFI_HTTP_RESPONSE_DATA), DEBUG_DM_PRINT_ADDRESS | DEBUG_DM_PRINT_ASCII);
    }

    Status = Dfci->HttpProtocol->Response(Dfci->HttpProtocol, ResponseToken);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "Http Response failed. Code=%r\n", Status));
        return Status;
    }

    Status = EventWait (Dfci, ResponseToken->Event, HTTP_TIMEOUT);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "Http Response timeout.\n"));
        Status2 = Dfci->HttpProtocol->Cancel(Dfci->HttpProtocol, ResponseToken);
        if (EFI_ERROR(Status2)) {
            DEBUG((DEBUG_ERROR, "Http HttpCancel failed. Code=%r", Status));
        }
    }

    if (NULL != ResponseData) {
        DEBUG((DEBUG_INFO, "Response status is %d\n", ResponseData->StatusCode));
    }
    DEBUG((DEBUG_INFO, "Received %d headers\n", ResponseMessage->HeaderCount));

    DumpHeaders (ResponseMessage->HeaderCount, ResponseMessage->Headers);

    return Status;
}

/**
 * DfciGetSettingsPacket
 *
 *
 * @param[in]  Dfci
 * @param[in]  Url
 * @param[out] SettingsPkt
 * @param[out] SettingsPktSize
 *
 * @return EFI_STATUS
 *
 **/
STATIC
EFI_STATUS
DfciGetSettingsPacket (
    IN  DFCI_PRIVATE_DATA *Dfci,
    IN  CHAR16            *Url,
    OUT VOID             **SettingsPkt,
    OUT UINTN             *SettingsPktSize
  ) {

    UINTN                           ContentLength;
    EFI_HTTP_HEADER                *ContentLengthHeader;
    UINTN                           CurrentLength;
    CHAR8                          *Packet;
    EFI_HTTP_REQUEST_DATA           RequestData;
    EFI_HTTP_MESSAGE                RequestMessage;
    EFI_HTTP_TOKEN                  RequestToken;
    EFI_HTTP_RESPONSE_DATA          ResponseData;
    EFI_HTTP_MESSAGE                ResponseMessage;
    EFI_HTTP_TOKEN                  ResponseToken;
    EFI_STATUS                      Status;


    if ((NULL == Dfci) ||
        (NULL == Url) ||
        (NULL == SettingsPkt) ||
        (NULL == SettingsPktSize)) {
        return EFI_INVALID_PARAMETER;
    }

    RequestData.Method = HttpMethodGet;     // Only get the headers to determine the body length
    RequestData.Url = Url;

    RequestMessage.BodyLength = 0;
    RequestMessage.Body = NULL;
    RequestMessage.Data.Request = &RequestData;

    RequestToken.Event = NULL;
    RequestToken.Status = EFI_SUCCESS;
    RequestToken.Message = &RequestMessage;

    Status = DfciBuildRequestHeaders (Url, RequestMessage.BodyLength, HEADER_CONTENT_BIN, &RequestMessage.Headers, &RequestMessage.HeaderCount);
    if (EFI_ERROR(Status)) {
        return Status;
    }

    Status = DfciIssueRequest (Dfci, &RequestToken);
    if (EFI_ERROR(Status)) {
        goto S_EXIT1;
    }

    ResponseData.StatusCode = HTTP_STATUS_UNSUPPORTED_STATUS;

    ResponseMessage.Data.Response = &ResponseData;
    ResponseMessage.HeaderCount = 0;
    ResponseMessage.Headers = NULL;

    ResponseMessage.BodyLength = 0;
    ResponseMessage.Body = NULL;

    ResponseToken.Event = NULL;
    ResponseToken.Status = EFI_SUCCESS;
    ResponseToken.Message = &ResponseMessage;

    Status = DfciGetResponse (Dfci, &ResponseToken);
    if (EFI_ERROR(Status)) {
        goto S_EXIT1;
    }

    ContentLengthHeader = HttpFindHeader (ResponseMessage.HeaderCount,
                                          ResponseMessage.Headers,
                                          HTTP_HEADER_CONTENT_LENGTH);
    ContentLength = 0;
    if (NULL != ContentLengthHeader) {
        ContentLength = AsciiStrDecimalToUintn(ContentLengthHeader->FieldValue);
    }

    DEBUG((DEBUG_INFO, "ContentLength=%d,ActualLength=%d\n", ContentLength, ResponseMessage.BodyLength));

    FreeHeaders (ResponseMessage.HeaderCount, ResponseMessage.Headers);

    ResponseMessage.HeaderCount = 0;
    ResponseMessage.Headers = NULL;
    ResponseMessage.Data.Response = NULL;

    if (0 == ContentLength) {
        DEBUG((DEBUG_INFO, "No content available\n"));
        Status = EFI_NOT_FOUND;
        goto S_EXIT1;
    }

    Packet = AllocatePool (ContentLength);
    if (NULL == Packet) {
        DEBUG((DEBUG_ERROR, "Unable to allocate return buffer\n"));
        Status = EFI_OUT_OF_RESOURCES;
        goto S_EXIT1;
    }
    CurrentLength = 0;

    while (CurrentLength < ContentLength) {
        ResponseMessage.Body = &Packet[CurrentLength];
        ResponseMessage.BodyLength = ContentLength - CurrentLength;
        Status = DfciGetResponse (Dfci, &ResponseToken);
        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "Error from additional response data. Code=%r\n", Status));
            FreePool (Packet);
            goto S_EXIT1;
        }
        CurrentLength += ResponseMessage.BodyLength;
    }

    *SettingsPkt = Packet;
    *SettingsPktSize = ContentLength;
    ResponseMessage.Body = NULL;

S_EXIT1:
    FreeHeaders(RequestMessage.HeaderCount, RequestMessage.Headers);
    return Status;
}

/**
 * DfciSendSettingsPacket
 *
 *
 * @param[in]  Dfci
 * @param[in]  Url
 * @param[in]  ContentType
 * @param[in]  SettingsResult
 * @param[in]  SettingsResultSize
 *
 * @return EFI_STATUS
 *
 **/
STATIC
EFI_STATUS
DfciSendSettingsPacket (
    IN  DFCI_PRIVATE_DATA *Dfci,
    IN  CHAR16            *Url,
    IN  CONST CHAR8       *ContentType,
    IN  VOID              *SettingsResult,
    IN  UINTN              SettingsResultSize
  ) {

    UINTN                           ContentLength;
    EFI_HTTP_HEADER                *ContentLengthHeader;
    UINTN                           CurrentLength;
    CHAR8                          *Packet;
    EFI_HTTP_REQUEST_DATA           RequestData;
    EFI_HTTP_MESSAGE                RequestMessage;
    EFI_HTTP_TOKEN                  RequestToken;
    EFI_HTTP_RESPONSE_DATA          ResponseData;
    EFI_HTTP_MESSAGE                ResponseMessage;
    EFI_HTTP_TOKEN                  ResponseToken;
    EFI_STATUS                      Status;


    if ((NULL == Dfci) ||
        (NULL == Url) ||
        (NULL == ContentType) ||
        (NULL == SettingsResult)) {
        return EFI_INVALID_PARAMETER;
    }

    RequestData.Method = HttpMethodPut;     // Put the file on the server
    RequestData.Url = Url;

    RequestMessage.BodyLength = SettingsResultSize;
    RequestMessage.Body = SettingsResult;
    RequestMessage.Data.Request = &RequestData;

    DEBUG((DEBUG_INFO, "Content being sent\n"));
    DEBUG_BUFFER(DEBUG_INFO, SettingsResult, MIN (512, SettingsResultSize), DEBUG_DM_PRINT_ADDRESS | DEBUG_DM_PRINT_ASCII);

    RequestToken.Event = NULL;
    RequestToken.Status = EFI_SUCCESS;
    RequestToken.Message = &RequestMessage;

    Status = DfciBuildRequestHeaders (Url, RequestMessage.BodyLength, ContentType, &RequestMessage.Headers, &RequestMessage.HeaderCount);
    if (EFI_ERROR(Status)) {
        return Status;
    }

    Status = DfciIssueRequest (Dfci, &RequestToken);
    if (EFI_ERROR(Status)) {
        goto P_EXIT1;
    }

    ResponseData.StatusCode = HTTP_STATUS_UNSUPPORTED_STATUS;

    ResponseMessage.Data.Response = &ResponseData;
    ResponseMessage.HeaderCount = 0;
    ResponseMessage.Headers = NULL;

    ResponseMessage.BodyLength = 0;
    ResponseMessage.Body = NULL;

    ResponseToken.Event = NULL;
    ResponseToken.Status = EFI_SUCCESS;
    ResponseToken.Message = &ResponseMessage;

    Status = DfciGetResponse (Dfci, &ResponseToken);
    if (EFI_ERROR(Status)) {
        goto P_EXIT1;
    }

    ContentLengthHeader = HttpFindHeader (ResponseMessage.HeaderCount,
                                          ResponseMessage.Headers,
                                          HTTP_HEADER_CONTENT_LENGTH);
    ContentLength = 0;
    if (NULL != ContentLengthHeader) {
        ContentLength = AsciiStrDecimalToUintn(ContentLengthHeader->FieldValue);
    }

    DEBUG((DEBUG_INFO, "ContentLength=%d,ActualLength=%d\n", ContentLength, ResponseMessage.BodyLength));

    FreeHeaders (ResponseMessage.HeaderCount,ResponseMessage.Headers);

    ResponseMessage.HeaderCount = 0;
    ResponseMessage.Headers = NULL;
    ResponseMessage.Data.Response = NULL;

    if (0 == ContentLength) {
        DEBUG((DEBUG_INFO, "No content available\n"));
        Status = EFI_NOT_FOUND;
        goto P_EXIT1;
    }

    Packet = AllocatePool (ContentLength);
    if (NULL == Packet) {
        DEBUG((DEBUG_ERROR, "Unable to allocate return buffer\n"));
        Status = EFI_OUT_OF_RESOURCES;
        goto P_EXIT1;
    }
    CurrentLength = 0;

    while (CurrentLength < ContentLength) {
        ResponseMessage.Body = &Packet[CurrentLength];
        ResponseMessage.BodyLength = ContentLength - CurrentLength;
        Status = DfciGetResponse (Dfci, &ResponseToken);
        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "Error from additional response data. Code=%r\n", Status));
            FreePool (Packet);
            goto P_EXIT1;
        }
        CurrentLength += ResponseMessage.BodyLength;
    }

    DEBUG_BUFFER(DEBUG_INFO, Packet, MIN (1504, CurrentLength), DEBUG_DM_PRINT_ADDRESS | DEBUG_DM_PRINT_ASCII);

    FreePool (Packet);

P_EXIT1:
    FreeHeaders(RequestMessage.HeaderCount, RequestMessage.Headers);
    return Status;
}

/**
 * GetRequestUrl
 *
 * Build a Url from <BaseUrl>/DfciRequest/<MachineId>/<RequestType>
 *
 * @param Url
 * @param Type
 * @param RequestUrl    - New URL (caller must free)
 *
 * @return EFI_STATUS
 *
 **/
STATIC
EFI_STATUS
GetRequestUrl (
    IN  DFCI_PRIVATE_DATA  *Dfci,
    IN  CHAR16             *RequestType,
    OUT CHAR16            **RequestUrl
  ) {

    CHAR16     *MachineId;
    UINTN       MachineIdSize;
    EFI_STATUS  Status;
    CHAR16     *WorkUrl;
    UINTN       WorkUrlSize;


    if ((NULL == Dfci) ||
        (NULL == RequestType) ||
        (NULL == RequestUrl) ||
        (Dfci->UrlSize < sizeof(CHAR16))) {
        return EFI_INVALID_PARAMETER;
    }

    //
    //  For now, just use the string serialnumber for the MachineId
    //
    // TBD, the exact string needed to access InTune.  This is set up for a
    // test server mikeytbds3

    MachineIdSize = Dfci->SerialNumberSize * sizeof(CHAR16);
    MachineId = AllocatePool (MachineIdSize);

    if (NULL == MachineId) {
        DEBUG((DEBUG_ERROR, "Unable to allocate memory for MachineId\n"));
        return EFI_OUT_OF_RESOURCES;
    }

    Status = AsciiStrToUnicodeStrS (Dfci->SerialNumber, MachineId, Dfci->SerialNumberSize);
    if (EFI_ERROR(Status)) {
        FreePool (MachineId);
        DEBUG((DEBUG_ERROR, "Unable to convert Ascii SerialNumber to Unicode. Code=%r\n", Status));
        return Status;
    }

    WorkUrlSize = Dfci->UrlSize * sizeof(CHAR16);              // BaseUrl size - includes terminating NULL
    WorkUrlSize += StrLen (DFCI_REQUEST) * sizeof(CHAR16);     // and "DfciRequest/" size
    WorkUrlSize += StrLen (MachineId) * sizeof(CHAR16);        // <MachineId> size
    WorkUrlSize += StrLen (RequestType) * sizeof(CHAR16);      // <RequestType> size
    WorkUrlSize += sizeof(CHAR16);                             // Possible extra "/"

    WorkUrl = AllocatePool (WorkUrlSize);
    if (NULL == WorkUrl) {
        FreePool (MachineId);
        DEBUG((DEBUG_ERROR, "Unable to allocate memory for WorkUrl\n"));
        return EFI_OUT_OF_RESOURCES;
    }

    Status = AsciiStrToUnicodeStrS (Dfci->Url, WorkUrl, Dfci->UrlSize);
    if (EFI_ERROR(Status)) {
        FreePool (MachineId);
        FreePool (WorkUrl);
        DEBUG((DEBUG_ERROR, "Unable to convert Ascii URL to Unicode. Code=%r\n", Status));
        return Status;
    }

    if ('/' != Dfci->Url[Dfci->UrlSize - 2]) {
        StrCatS  (WorkUrl, WorkUrlSize, L"/");         // Add '/' if necessary
    }

    StrCatS  (WorkUrl, WorkUrlSize, DFCI_REQUEST);     // Add DfciRequest/
    StrCatS  (WorkUrl, WorkUrlSize, MachineId);        // Add MachineID
    StrCatS  (WorkUrl, WorkUrlSize, RequestType);      // Add /<RequestType>

    FreePool (MachineId);

    *RequestUrl = WorkUrl;
    DEBUG((DEBUG_INFO, "Url        = %a\n", Dfci->Url));
    DEBUG((DEBUG_INFO, "RequestUrl = %s\n", WorkUrl));

    return EFI_SUCCESS;
}

/**
 * ProcessSendResultItem
 *
 * Send one Dfci Result packet to the server
 *
 * @param[in]  Dfci
 * @param[in]  RequestEntry - Contains request type, variable name, etc
 *
 * @return EFI_STATUS
 *
 **/
STATIC
EFI_STATUS
ProcessSendResultItem (
    IN DFCI_PRIVATE_DATA     *Dfci,
    IN PROCESS_REQUEST_ENTRY *RequestEntry
  ) {

    VOID       *SettingsResult;
    UINTN       SettingsResultSize;
    EFI_STATUS  Status;
    CHAR16     *Url;


    if ((NULL == Dfci) ||
        (NULL == RequestEntry) ||
        (NULL == RequestEntry->RequestType) ||
        (NULL == RequestEntry->ResultsVariableName) ||
        (NULL == RequestEntry->VariableNameSpace)) {
        return EFI_INVALID_PARAMETER;
    }

    Status = GetVariable3(RequestEntry->ResultsVariableName,
                          RequestEntry->VariableNameSpace,
                         &SettingsResult,
                         &SettingsResultSize,
                          NULL);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "GetVariable failed for %s. Code = %r\n", RequestEntry->ResultsVariableName, Status));
        if (EFI_NOT_FOUND == Status) {
            return EFI_SUCCESS;            // No results, SUCCESS.
        }
        return Status;
    }

    Status = GetRequestUrl(Dfci, RequestEntry->RequestType, &Url);
    if (EFI_ERROR(Status)) {
        FreePool (SettingsResult);
        return Status;
    }

    Status = DfciSendSettingsPacket (Dfci,
                                     Url, RequestEntry->ContentType,
                                     SettingsResult,
                                     SettingsResultSize);
    FreePool (Url);
    FreePool (SettingsResult);

    return Status;
}

/**
 * ProcessRequestItem
 *
 * Request one Dfci Settings packet from the server
 *
 * @param[in]  Dfci
 * @param[in]  RequestEntry
 * @param[out] SettingApplied  -- Only set to TRUE if Setting applied, else not set
 *
 * @return EFI_STATUS
 *
 **/
STATIC
EFI_STATUS
ProcessRequestItem (
    IN  DFCI_PRIVATE_DATA     *Dfci,
    IN  PROCESS_REQUEST_ENTRY *RequestEntry,
    OUT BOOLEAN               *SettingApplied
  ) {

    DFCI_PACKET_SIGNATURE   *SettingsInfo;
    VOID                    *SettingsPkt;
    UINTN                    SettingsPktSize;
    EFI_STATUS               Status;
    CHAR16                  *Url;


    if ((NULL == Dfci) ||
        (NULL == RequestEntry) ||
        (NULL == RequestEntry->RequestType) ||
        (NULL == RequestEntry->VariableName) ||
        (NULL == RequestEntry->VariableNameSpace)) {
        return EFI_INVALID_PARAMETER;
    }

    Status = GetRequestUrl(Dfci, RequestEntry->RequestType, &Url);
    if (EFI_ERROR(Status)) {
        return Status;
    }

    Status = DfciGetSettingsPacket (Dfci, Url, &SettingsPkt, &SettingsPktSize);
    if (EFI_ERROR(Status)) {
        FreePool (Url);
        return Status;
    }

    DEBUG_BUFFER(DEBUG_INFO, SettingsPkt, MIN (1504, SettingsPktSize), DEBUG_DM_PRINT_ADDRESS | DEBUG_DM_PRINT_ASCII);

    //
    // All of the structures are the same with respect to the location of the signature field.
    //
    SettingsInfo = (DFCI_PACKET_SIGNATURE *) SettingsPkt;

    //
    // Validate the correct signature is in the packet before setting the variable.
    //
    if (RequestEntry->Signature == SettingsInfo->Signature) {
        Status = gRT->SetVariable(RequestEntry->VariableName,
                                  RequestEntry->VariableNameSpace,
                                  RequestEntry->VariableAttributes,
                                  SettingsPktSize,
                                  SettingsPkt);
        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "Unable to set %s. Code=%r\n", RequestEntry->VariableName, Status));
        } else {
            *SettingApplied = TRUE;
        }
    } else {
        DEBUG((DEBUG_ERROR, "SettingsInfo->Signature not as expected. Expected %x, got %x\n", RequestEntry->Signature, SettingsInfo->Signature));
        Status = EFI_NOT_FOUND;
    }
    FreePool (Url);
    FreePool (SettingsPkt);

    return Status;
}

/**
 * Process Dfci Requests
 *
 * Dfci requests are a sequence of requests - See the mRequestTable for the sequence:
 *
 *
 * The url for these requests are:
 *
 * <hosturl>/DfciRequest/<MachineId>/<request>
 *
 *
 * @param[in]  Dfci
 *
 * @return EFI_STATUS
 *
 **/
STATIC
EFI_STATUS
ProcessDfciRequests (
    IN  DFCI_PRIVATE_DATA  *Dfci
  )  {

    UINTN       i;
    EFI_STATUS  Status;
    BOOLEAN     SettingApplied;


    if (NULL == Dfci) {
        return EFI_INVALID_PARAMETER;
    }

    SettingApplied = FALSE;
    //
    // Send Results and Current to Settings Manager
    //
    for (i=0; i < REQUEST_TABLE_COUNT; i++) {
        Status = ProcessSendResultItem (Dfci, &mRequestTable[i]);
        if (EFI_ERROR(Status)) {
            return Status;
        }
    }

    //
    // Get new settings from Settings Manager
    //
    for (i=0; i < REQUEST_TABLE_COUNT; i++) {
        if (mRequestTable[i].Signature != 0) {
            Status = ProcessRequestItem (Dfci, &mRequestTable[i], &SettingApplied);
            if (EFI_ERROR(Status) && (Status != EFI_NOT_FOUND)) {
                return Status;
            }
        }
    }

    if (Status == EFI_NOT_FOUND) {
        Status = EFI_SUCCESS;
    }

    if (!SettingApplied) {
        mUserStatus = USER_STATUS_NO_SETTINGS;
    }

    return Status;
}

/**
 *  Dfci Request from Network
 *
 *  @param[in] Url            A pointer to the EFI System Table.
 *  @param[in] UrlSize
 *  @param[out] UserStatus
 *
 *  @retval EFI_SUCCESS       The entry point is executed successfully.
 *  @retval other             Some error occurs when executing this entry point.
 *
 **/
EFI_STATUS
EFIAPI
DfciRequestProcess (
    IN CHAR8      *Url,
    IN UINTN       UrlSize,
    OUT UINT64    *UserStatus
  ) {

    EFI_BOOT_MANAGER_POLICY_PROTOCOL *BootPolicy;
    DFCI_PRIVATE_DATA                *Dfci;
    BOOLEAN                           DoneProcessing;
    EFI_HANDLE                       *HandleBuffer;
    UINTN                             HandleCount;
    BOOLEAN                           MediaPresent;
    UINTN                             NicIndex;
    EFI_STATUS                        Status;


    if ((NULL == Url) ||
        (NULL == UserStatus)) {
        return EFI_INVALID_PARAMETER;
    }

    Status = gBS->LocateProtocol(&gEfiBootManagerPolicyProtocolGuid,
                                 NULL,
                                 (VOID **)&BootPolicy);

    // If the platform chose to publish a Boot Manager Policy, ask it to start the
    // networking stack.  Ignore any errors, and attempt to access the network even if
    // the BootPolicy returns an error.
    if (!EFI_ERROR(Status)) {
        Status = BootPolicy->ConnectDeviceClass (BootPolicy, &gEfiBootManagerPolicyNetworkGuid);
        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR,"Error starting the network. Code = %r\n",Status));
        }
    }

    mUserStatus = USER_STATUS_SUCCESS;
    //
    // Try each connected NIC until a successful settings transfer.
    //
    Dfci = AllocateZeroPool (sizeof(DFCI_PRIVATE_DATA));
    if (NULL == Dfci) {
        DEBUG((DEBUG_ERROR, "Unable to allocate Dfci private data\n"));
        return EFI_OUT_OF_RESOURCES;
    }

    Dfci->Url = Url;
    Dfci->UrlSize = UrlSize;
    HandleBuffer = NULL;
    Status = DfciIdSupportGetManufacturer (&Dfci->Manufacturer, &Dfci->ManufacturerSize);
    Status |= DfciIdSupportGetProductName (&Dfci->ProductName, &Dfci->ProductNameSize);
    Status |= DfciIdSupportGetSerialNumber (&Dfci->SerialNumber, &Dfci->SerialNumberSize);
    Status |= DfciIdSupportGetUuid (&Dfci->Uuid, &Dfci->UuidSize);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "%a: Unable to get SmBios Info. %r\n", __FUNCTION__, Status));
        // Status is mangled - just return EFI_UNSUPPORTED
        Status = EFI_UNSUPPORTED;
        goto CLEANUP;
    }

    DoneProcessing = FALSE;
    //
    // Find NIC's with HTTP ServiceBinding protocol.  These are the available HTTP devices.
    //
    Status = gBS->LocateHandleBuffer(
        ByProtocol,
        &gEfiHttpServiceBindingProtocolGuid,
        NULL,
        &HandleCount,
        &HandleBuffer);
    if (EFI_ERROR(Status) || (0 == HandleCount)) {
        mUserStatus = USER_STATUS_NO_NIC;
        DEBUG((DEBUG_ERROR, "Unable to locate any NIC's for HTTP file access\n"));
        Status = EFI_NOT_FOUND;
        goto CLEANUP;
    }

    for (NicIndex = 0; (NicIndex < HandleCount) && !DoneProcessing; NicIndex++) {
        //
        // Clear the working section of DFCI_PRIVATE_DATA
        //
        ZeroMem (&Dfci->NicHandle,sizeof(DFCI_PRIVATE_DATA) - OFFSET_OF(DFCI_PRIVATE_DATA, NicHandle));

        Dfci->NicHandle = HandleBuffer[NicIndex];

        Status = gBS->HandleProtocol(Dfci->NicHandle, &gEfiHttpServiceBindingProtocolGuid, &Dfci->HttpSbProtocol);
        if (EFI_ERROR(Status)) {
            mUserStatus = USER_STATUS_NO_NIC;
            DEBUG((DEBUG_ERROR, "Error locating HttpServiceBinding protocol. Code=%r\n", Status));
            goto CLEANUP;
        }

        // Verify Media is present before doing any work.  We don't really care about the error cases.  On
        // error cases, assume Media is Present.
        MediaPresent = TRUE;
        Status = NetLibDetectMedia (Dfci->NicHandle,&MediaPresent);
        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_INFO, "NetLibDetectMedi returned %r. Assuming Media Present\n", Status));
        }
        if (!MediaPresent) {
            mUserStatus = USER_STATUS_NO_MEDIA;
            continue;
        }

        Dfci->ConfigData.LocalAddressIsIPv6 = FALSE;
        Dfci->HttpChildHandle = gImageHandle;   //Place HttpChild on our image handle
        Status = ConfigureHTTP (Dfci);
        if (EFI_ERROR(Status)) {
            goto EARLY_EXIT;
        }

        mUserStatus = USER_STATUS_SUCCESS;

        Status = ProcessDfciRequests (Dfci);
        if (EFI_ERROR(Status)) {
            goto EARLY_EXIT;
        }

        DoneProcessing = TRUE;

EARLY_EXIT:
        if (Dfci->DhcpRequested) {
            ConfigureSTATIC (Dfci);
        }
        if (NULL != Dfci->HttpProtocol) {
            Status = Dfci->HttpProtocol->Configure(Dfci->HttpProtocol, NULL);
            if (EFI_ERROR(Status)) {
                DEBUG((DEBUG_ERROR, "Unable to cleanup HTTP Protocol. Code=%r\n", Status));
            }
        }

        Status = Dfci->HttpSbProtocol->DestroyChild(Dfci->HttpSbProtocol, Dfci->HttpChildHandle);
        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "Error destroying worker child. Code=%r\n", Status));
        }
    }

CLEANUP:
    if (NULL != HandleBuffer) {
        FreePool(HandleBuffer);
    }

    FreePool (Dfci);

    *UserStatus = mUserStatus;

    return Status;
}