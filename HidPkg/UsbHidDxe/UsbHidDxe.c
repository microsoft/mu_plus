/** @file
  USB HID Driver that manages USB HID devices and produces the HidIo protocol.

  USB HID Driver consumes USB I/O Protocol and Device Path Protocol, and produces
  the HidIo protocol on USB HID devices.

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Protocol/HidIo.h>

#include <Protocol/DevicePath.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/UsbIo.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiUsbLib.h>

#include <IndustryStandard/Usb.h>

#define CLASS_HID      3
#define SUBCLASS_BOOT  1

// Refer to USB HID 1.11, section 7.2.6.
#define BOOT_PROTOCOL    0
#define REPORT_PROTOCOL  1

//
// A common header for usb standard descriptor.
// Each stand descriptor has a length and type.
//
#pragma pack(1)
typedef struct {
  UINT8    Len;
  UINT8    Type;
} USB_DESC_HEAD;
#pragma pack()

#define USB_HID_DEV_SIGNATURE  SIGNATURE_32('U','H','I','D')

typedef struct {
  UINT32                          Signature;
  HID_IO_PROTOCOL                 HidIo;
  EFI_USB_IO_PROTOCOL             *UsbIo;
  EFI_USB_INTERFACE_DESCRIPTOR    InterfaceDescriptor;
  EFI_USB_ENDPOINT_DESCRIPTOR     IntInEndpointDescriptor;
  EFI_USB_HID_DESCRIPTOR          *HidDescriptor;
  UINTN                           ReportDescriptorLength;
  VOID                            *ReportDescriptor;
  HID_IO_REPORT_CALLBACK          ReportCallback;
  VOID                            *CallbackContext;
  EFI_EVENT                       DelayedRecoveryEvent;
} USB_HID_DEV;

#define USB_HID_DEV_FROM_HID_IO_PROTOCOL(a) \
    CR(a, USB_HID_DEV, HidIo, USB_HID_DEV_SIGNATURE)

EFI_STATUS
InitiateAsyncInterruptInputTransfers (
  USB_HID_DEV  *Device
  );

EFI_STATUS
ShutdownAsyncInterruptInputTransfers (
  USB_HID_DEV  *Device
  );

/**
  Retrieve the HID Report Descriptor from the device.

  @param  This                    A pointer to the HidIo Instance
  @param  ReportDescriptorSize    On input, the size of the buffer allocated to hold the descriptor.
                                  On output, the actual size of the descriptor.
                                  May be set to zero to query the required size for the descriptor.
  @param  ReportDescriptorBuffer  A pointer to the buffer to hold the descriptor. May be NULL if ReportDescriptorSize is
                                  zero.

  @retval EFI_SUCCESS           Report descriptor successfully returned.
  @retval EFI_BUFFER_TOO_SMALL  The provided buffer is not large enough to hold the descriptor.
  @retval EFI_INVALID_PARAMETER Invalid input parameters.
  @retval EFI_NOT_FOUND         The device does not have a report descriptor.
  @retval Other                 Unexpected error reading descriptor.
**/
EFI_STATUS
EFIAPI
HidGetReportDescriptor (
  IN HID_IO_PROTOCOL  *This,
  IN OUT UINTN        *ReportDescriptorSize,
  IN OUT VOID         *ReportDescriptorBuffer
  )
{
  EFI_STATUS   Status;
  USB_HID_DEV  *UsbHidDevice;
  UINTN        Index;
  UINTN        DescriptorLength;

  if ((This == NULL) || (ReportDescriptorSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  UsbHidDevice = USB_HID_DEV_FROM_HID_IO_PROTOCOL (This);

  if (UsbHidDevice->ReportDescriptorLength == 0) {
    // Report Descriptor not yet read

    // Get report descriptor length from Hid Descriptor.
    if (UsbHidDevice->HidDescriptor == NULL) {
      ASSERT (UsbHidDevice->HidDescriptor != NULL);
      return EFI_NOT_FOUND;
    }

    for (Index = 0; Index < UsbHidDevice->HidDescriptor->NumDescriptors; Index++) {
      if (UsbHidDevice->HidDescriptor->HidClassDesc[Index].DescriptorType == USB_DESC_TYPE_REPORT) {
        break;
      }
    }

    if (Index == UsbHidDevice->HidDescriptor->NumDescriptors) {
      return EFI_NOT_FOUND;
    }

    // Index is set to the Report Descriptor index.
    DescriptorLength               = UsbHidDevice->HidDescriptor->HidClassDesc[Index].DescriptorLength;
    UsbHidDevice->ReportDescriptor = AllocateZeroPool (DescriptorLength);
    if (UsbHidDevice->ReportDescriptor == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Status = UsbGetReportDescriptor (
               UsbHidDevice->UsbIo,
               UsbHidDevice->InterfaceDescriptor.InterfaceNumber,
               (UINT16)DescriptorLength,
               (UINT8 *)UsbHidDevice->ReportDescriptor
               );
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }

    UsbHidDevice->ReportDescriptorLength = DescriptorLength;
  }

  if (*ReportDescriptorSize < UsbHidDevice->ReportDescriptorLength) {
    *ReportDescriptorSize = UsbHidDevice->ReportDescriptorLength;
    return EFI_BUFFER_TOO_SMALL;
  }

  if (ReportDescriptorBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  CopyMem (ReportDescriptorBuffer, UsbHidDevice->ReportDescriptor, UsbHidDevice->ReportDescriptorLength);

  return EFI_SUCCESS;
}

/**
  Retrieves a single report from the device.

  @param  This                  A pointer to the HidIo Instance
  @param  ReportId              Specifies which report to return if the device supports multiple input reports.
                                Set to zero if ReportId is not present.
  @param  ReportType            Indicates the type of report type to retrieve. 1-Input, 3-Feature.
  @param  ReportBufferSize      Indicates the size of the provided buffer to receive the report. The max size is MAX_UINT16.
  @param  ReportBuffer          Pointer to the buffer to receive the report.

  @retval EFI_SUCCESS           Report successfully returned.
  @retval EFI_OUT_OF_RESOURCES  The provided buffer is not large enough to hold the report.
  @retval EFI_INVALID_PARAMETER Invalid input parameters.
  @retval Other                 Unexpected error reading report.
**/
EFI_STATUS
EFIAPI
HidGetReport (
  IN HID_IO_PROTOCOL  *This,
  IN UINT8            ReportId,
  IN HID_REPORT_TYPE  ReportType,
  IN UINTN            ReportBufferSize,
  OUT VOID            *ReportBuffer
  )
{
  USB_HID_DEV  *UsbHidDevice;

  if ((This == NULL) || (ReportBufferSize == 0) || (ReportBufferSize > MAX_UINT16) || (ReportBuffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  // Only support Get_Report for Input or Feature reports.
  if (!((ReportType == HID_INPUT_REPORT) || (ReportType == HID_FEATURE_REPORT))) {
    return EFI_INVALID_PARAMETER;
  }

  UsbHidDevice = USB_HID_DEV_FROM_HID_IO_PROTOCOL (This);

  return UsbGetReportRequest (
           UsbHidDevice->UsbIo,
           UsbHidDevice->InterfaceDescriptor.InterfaceNumber,
           ReportId,
           (UINT8)ReportType,
           (UINT16)ReportBufferSize,
           (UINT8 *)ReportBuffer
           );
}

/**
  Sends a single report to the device.

  @param  This                  A pointer to the HidIo Instance
  @param  ReportId              Specifies which report to return if the device supports multiple input reports.
                                Set to zero if ReportId is not present.
  @param  ReportType            Indicates the type of report type to retrieve. 2-Output, 3-Feature.
  @param  ReportBufferSize      Indicates the size of the provided buffer holding the report to send.
  @param  ReportBuffer          Pointer to the buffer holding the report to send.

  @retval EFI_SUCCESS           Report successfully transmitted.
  @retval EFI_INVALID_PARAMETER Invalid input parameters.
  @retval Other                 Unexpected error transmitting report.
**/
EFI_STATUS
EFIAPI
HidSetReport (
  IN HID_IO_PROTOCOL  *This,
  IN UINT8            ReportId,
  IN HID_REPORT_TYPE  ReportType,
  IN UINTN            ReportBufferSize,
  IN VOID             *ReportBuffer
  )
{
  USB_HID_DEV  *UsbHidDevice;

  if ((This == NULL) || (ReportBufferSize == 0) || (ReportBufferSize > MAX_UINT16) || (ReportBuffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  // Only support Set_Report for Output or Feature reports.
  if (!((ReportType == HID_OUTPUT_REPORT) || (ReportType == HID_FEATURE_REPORT))) {
    return EFI_INVALID_PARAMETER;
  }

  UsbHidDevice = USB_HID_DEV_FROM_HID_IO_PROTOCOL (This);

  // Note: This implementation does not support Set_Report via interrupt out pipes.
  // Per HID 1.1, "operating systems that do not support HID Interrupt Out endpoints will route all Output reports
  // through the control endpoint."
  return UsbSetReportRequest (
           UsbHidDevice->UsbIo,
           UsbHidDevice->InterfaceDescriptor.InterfaceNumber,
           ReportId,
           (UINT8)ReportType,
           (UINT16)ReportBufferSize,
           (UINT8 *)ReportBuffer
           );
}

/**
  Registers a callback function to receive asynchronous input reports from the device.
  The device driver will do any necessary initialization to configure the device to send reports.

  @param  This                  A pointer to the HidIo Instance
  @param  Callback              Callback function to handle reports as they are received.
  @param  Context               Context that will be provided to the callback function.

  @retval EFI_SUCCESS           Callback successfully registered.
  @retval EFI_INVALID_PARAMETER Invalid input parameters.
  @retval EFI_ALREADY_STARTED   Callback function is already registered.
  @retval Other                 Unexpected error registering callback or initiating report generation from device.
**/
EFI_STATUS
EFIAPI
HidRegisterReportCallback (
  IN HID_IO_PROTOCOL         *This,
  IN HID_IO_REPORT_CALLBACK  Callback,
  IN VOID                    *Context OPTIONAL
  )
{
  EFI_STATUS   Status;
  USB_HID_DEV  *UsbHidDevice;

  if ((This == NULL) || (Callback == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  UsbHidDevice = USB_HID_DEV_FROM_HID_IO_PROTOCOL (This);

  if (UsbHidDevice->ReportCallback != NULL) {
    return EFI_ALREADY_STARTED;
  }

  UsbHidDevice->ReportCallback  = Callback;
  UsbHidDevice->CallbackContext = Context;

  Status = InitiateAsyncInterruptInputTransfers (UsbHidDevice);
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  Unregisters a previously registered callback function.
  The device driver will do any necessary initialization to configure the device to stop sending reports.

  @param  This                  A pointer to the HidIo Instance
  @param  Callback              Callback function to unregister.

  @retval EFI_SUCCESS           Callback successfully unregistered.
  @retval EFI_INVALID_PARAMETER Invalid input parameters.
  @retval EFI_NOT_STARTED       Callback function was not previously registered.
  @retval Other                 Unexpected error unregistering report or disabling report generation from device.
**/
EFI_STATUS
EFIAPI
HidUnregisterReportCallback (
  IN HID_IO_PROTOCOL         *This,
  IN HID_IO_REPORT_CALLBACK  Callback
  )
{
  EFI_STATUS   Status;
  USB_HID_DEV  *UsbHidDevice;

  if ((This == NULL) || (Callback == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  UsbHidDevice = USB_HID_DEV_FROM_HID_IO_PROTOCOL (This);

  if (UsbHidDevice->ReportCallback != Callback) {
    return EFI_NOT_STARTED;
  }

  Status = ShutdownAsyncInterruptInputTransfers (UsbHidDevice);
  ASSERT_EFI_ERROR (Status);

  UsbHidDevice->ReportCallback = NULL;

  return Status;
}

/**
  Handles interrupt completion on USB with new HID report.

  @param  Data                  Pointer to buffer containing data returned by USB interrupt completion.
  @param  DataLength            Length of data buffer.
  @param  Context               Context associated with transfer (points to UsbHidDevice that scheduled the transfer)
  @param  Result                Indicates if there was an error on USB.

  @retval EFI_SUCCESS           Interrupt handled and passed up the stack.
  @retval EFI_DEVICE_ERROR      There as an error with the transaction. The interrupt will be re-submitted after a
                                delay.
  @retval Other                 Unexpected error handling the interrupt completion.
**/
EFI_STATUS
EFIAPI
OnReportInterruptComplete (
  IN  VOID    *Data,
  IN  UINTN   DataLength,
  IN  VOID    *Context,
  IN  UINT32  Result
  )
{
  EFI_STATUS   Status;
  USB_HID_DEV  *UsbHidDevice;
  UINT32       UsbResult;

  UsbHidDevice = (USB_HID_DEV *)Context;

  if (Result != EFI_USB_NOERROR) {
    if ((Result & EFI_USB_ERR_STALL) == EFI_USB_ERR_STALL) {
      Status = UsbClearEndpointHalt (
                 UsbHidDevice->UsbIo,
                 UsbHidDevice->IntInEndpointDescriptor.EndpointAddress,
                 &UsbResult
                 );
      ASSERT_EFI_ERROR (Status);
    }

    // Delete & Submit this interrupt again
    // Handler of DelayedRecoveryEvent triggered by timer will re-submit the interrupt.
    Status = UsbHidDevice->UsbIo->UsbAsyncInterruptTransfer (
                                    UsbHidDevice->UsbIo,
                                    UsbHidDevice->IntInEndpointDescriptor.EndpointAddress,
                                    FALSE,
                                    0,
                                    0,
                                    NULL,
                                    NULL
                                    );
    ASSERT_EFI_ERROR (Status);

    // Queue delayed recovery event. EFI_USB_INTERRUPT_DELAY is defined in USB standard for error handling.
    Status = gBS->SetTimer (
                    UsbHidDevice->DelayedRecoveryEvent,
                    TimerRelative,
                    EFI_USB_INTERRUPT_DELAY
                    );
    ASSERT_EFI_ERROR (Status);

    return EFI_DEVICE_ERROR;
  }

  if (DataLength > MAX_UINT16) {
    return EFI_DEVICE_ERROR;
  }

  if ((DataLength == 0) || (Data == NULL)) {
    return EFI_SUCCESS;
  }

  if (UsbHidDevice->ReportCallback != NULL) {
    UsbHidDevice->ReportCallback (
                    (UINT16)DataLength,
                    Data,
                    UsbHidDevice->CallbackContext
                    );
  }

  return EFI_SUCCESS;
}

/**
  Delayed recovery handler. Invoked when there is a USB error with the async interrupt transfer that reads reports from
  the endpoint device. Re-schedules the interrupt.

  @param  Event                 Event associated with this callback.
  @param  Context               Context associated with transfer (points to UsbHidDevice that scheduled the transfer)

  @retval None
**/
VOID
EFIAPI
DelayedRecoveryHandler (
  IN    EFI_EVENT  Event,
  IN    VOID       *Context
  )
{
  EFI_STATUS   Status;
  USB_HID_DEV  *UsbHidDev;

  UsbHidDev = (USB_HID_DEV *)Context;

  // Re-submit Asynchronous Interrupt Transfer for recovery.
  Status = UsbHidDev->UsbIo->UsbAsyncInterruptTransfer (
                               UsbHidDev->UsbIo,
                               UsbHidDev->IntInEndpointDescriptor.EndpointAddress,
                               TRUE,
                               UsbHidDev->IntInEndpointDescriptor.Interval,
                               UsbHidDev->IntInEndpointDescriptor.MaxPacketSize,
                               OnReportInterruptComplete,
                               UsbHidDev
                               );
  ASSERT_EFI_ERROR (Status);
}

/**
  Initiates input reports from the endpoint by scheduling an async interrupt transaction to poll the device.

  @param  UsbHidDevice        The UsbHidDevice instance that is initiating transfers.

  @retval EFI_SUCCESS         Async Interrupt transfer to read input reports has been successfully initiated.
  @retval Other               Unexpected error initiating the transfer.
**/
EFI_STATUS
InitiateAsyncInterruptInputTransfers (
  USB_HID_DEV  *UsbHidDevice
  )
{
  EFI_STATUS  Status;

  // Configure event for delayed recovery handler.
  if (UsbHidDevice->DelayedRecoveryEvent != NULL) {
    Status = gBS->CloseEvent (UsbHidDevice->DelayedRecoveryEvent);
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }

    UsbHidDevice->DelayedRecoveryEvent = NULL;
  }

  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  DelayedRecoveryHandler,
                  UsbHidDevice,
                  &UsbHidDevice->DelayedRecoveryEvent
                  );

  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  // Start the async interrupt transfers for input reports.
  Status = UsbHidDevice->UsbIo->UsbAsyncInterruptTransfer (
                                  UsbHidDevice->UsbIo,
                                  UsbHidDevice->IntInEndpointDescriptor.EndpointAddress,
                                  TRUE,
                                  UsbHidDevice->IntInEndpointDescriptor.Interval,
                                  UsbHidDevice->IntInEndpointDescriptor.MaxPacketSize,
                                  OnReportInterruptComplete,
                                  UsbHidDevice
                                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  Shuts down input reports from the endpoint by deleting the async interrupt transaction to poll the device.

  @param  UsbHidDevice        The UsbHidDevice instance that is initiating transfers.

  @retval EFI_SUCCESS         Async Interrupt transfer to read input reports has been successfully deleted.
  @retval Other               Unexpected error deleting the transfer.
**/
EFI_STATUS
ShutdownAsyncInterruptInputTransfers (
  USB_HID_DEV  *UsbHidDevice
  )
{
  EFI_STATUS  Status;

  // Stop the async transfers for input reports.
  Status = UsbHidDevice->UsbIo->UsbAsyncInterruptTransfer (
                                  UsbHidDevice->UsbIo,
                                  UsbHidDevice->IntInEndpointDescriptor.EndpointAddress,
                                  FALSE,
                                  0,
                                  0,
                                  NULL,
                                  NULL
                                  );

  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    DEBUG ((DEBUG_WARN, "[%a] unexpected error shutting down async interrupt transfer: %r\n", __FUNCTION__, Status));
  }

  // Close the Delayed recovery event.
  if (UsbHidDevice->DelayedRecoveryEvent != NULL) {
    Status = gBS->CloseEvent (UsbHidDevice->DelayedRecoveryEvent);
    if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
      DEBUG ((DEBUG_WARN, "[%a] unexpected error closing delayed recovery event: %r\n", __FUNCTION__, Status));
    }

    UsbHidDevice->DelayedRecoveryEvent = NULL;
  }

  return Status;
}

/**
  Indicates whether this is a USB hid device that this driver should manage.

  @param  UsbIio              UsbIo instance that can be used to perform the test.

  @retval TRUE                This driver can support this device.
  @retval FALSE               This driver cannot support this device.
**/
BOOLEAN
IsUsbHid (
  IN  EFI_USB_IO_PROTOCOL  *UsbIo
  )
{
  EFI_STATUS                    Status;
  EFI_USB_INTERFACE_DESCRIPTOR  InterfaceDescriptor;
  UINTN                         ExcludeListSize;
  UINTN                         ExcludeListIndex;
  UINT8                         *ExcludeList;

  //
  // Get the default interface descriptor
  //
  Status = UsbIo->UsbGetInterfaceDescriptor (
                    UsbIo,
                    &InterfaceDescriptor
                    );

  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  if (InterfaceDescriptor.InterfaceClass != CLASS_HID) {
    return FALSE;
  }

  ExcludeListSize  = PcdGetSize (PcdExcludedHidDevices);
  ExcludeList      = (UINT8 *)PcdGetPtr (PcdExcludedHidDevices);
  ExcludeListIndex = 0;

  for (ExcludeListIndex = 0; ExcludeListIndex + 2 < ExcludeListSize; ExcludeListIndex += 3) {
    if ((ExcludeList[ExcludeListIndex] == 0) &&
        (ExcludeList[ExcludeListIndex + 1] == 0) &&
        (ExcludeList[ExcludeListIndex + 2] == 0))
    {
      // end of exclude list, and haven't found exclusion - device is supported.
      break;
    }

    if ((ExcludeList[ExcludeListIndex] == InterfaceDescriptor.InterfaceClass) &&
        (ExcludeList[ExcludeListIndex + 1] == InterfaceDescriptor.InterfaceSubClass) &&
        (ExcludeList[ExcludeListIndex + 2] == InterfaceDescriptor.InterfaceProtocol))
    {
      // current entry matches an exclude list element - device not supported.
      return FALSE;
    }
  }

  return TRUE;
}

/**
  Retrieves the full HID descriptor for the given interface.

  @param  UsbIo                 UsbIo interface used to read the HID descriptor
  @param  InterfaceDescriptor   InterfaceDescriptor to retrieve HID descriptor for.
  @param  HidDescriptor         Receives a pointer to a freshly allocated buffer containing the HID descriptor. Caller
                                is required to free.

  @retval EFI_SUCCESS           Full HID descriptor returned in UsbGetFullHidDescriptor
  @retval Other                 Unexpected error retrieving the full HID descriptor.
**/
EFI_STATUS
UsbGetFullHidDescriptor (
  IN  EFI_USB_IO_PROTOCOL           *UsbIo,
  IN  EFI_USB_INTERFACE_DESCRIPTOR  *InterfaceDescriptor,
  OUT EFI_USB_HID_DESCRIPTOR        **HidDescriptor
  )
{
  EFI_STATUS                 Status;
  EFI_USB_CONFIG_DESCRIPTOR  ConfigDesc;
  VOID                       *DescriptorBuffer;
  UINT32                     TransferResult;
  USB_DESC_HEAD              *DescriptorHeader;
  UINT16                     DescriptorCursor;
  EFI_USB_HID_DESCRIPTOR     *DiscoveredHidDescriptor;

  if ((UsbIo == NULL) || (InterfaceDescriptor == NULL) || (HidDescriptor == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  // Note: while the USB Device Class Definition for HID spec 1.11 specifies support for retrieving the HID descriptor
  // using a class-specifc Get_Descriptor request, not all devices support this. So read the entire configuration
  // descriptor instead, and parse it for the HID descriptor.

  Status = UsbIo->UsbGetConfigDescriptor (UsbIo, &ConfigDesc);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DescriptorBuffer = AllocateZeroPool (ConfigDesc.TotalLength);
  if (DescriptorBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  // Issuing Get_Descriptor(Configuration) request with total length returns the Configuration descriptor, all Interface
  // descriptors, all Endpoint descriptors, and the HID descriptor for each interface.
  Status = UsbGetDescriptor (
             UsbIo,
             (UINT16)((USB_DESC_TYPE_CONFIG << 8) | (ConfigDesc.ConfigurationValue - 1)),
             0,
             ConfigDesc.TotalLength,
             DescriptorBuffer,
             &TransferResult
             );
  if (EFI_ERROR (Status)) {
    FreePool (DescriptorBuffer);
    return Status;
  }

  // Locate the HID descriptor within the overall configuration descriptor buffer. The HID descriptor will immediately
  // follow the interface descriptor. Refer to USB HID 1.11 section 7.1.
  DiscoveredHidDescriptor = NULL;
  DescriptorCursor        = 0;
  DescriptorHeader        = (USB_DESC_HEAD *)DescriptorBuffer;
  while (DescriptorCursor < ConfigDesc.TotalLength) {
    if (DescriptorHeader->Type == USB_DESC_TYPE_INTERFACE) {
      if ((((USB_INTERFACE_DESCRIPTOR *)DescriptorHeader)->InterfaceNumber == InterfaceDescriptor->InterfaceNumber) &&
          (((USB_INTERFACE_DESCRIPTOR *)DescriptorHeader)->AlternateSetting == InterfaceDescriptor->AlternateSetting))
      {
        // The HID descriptor must immediately follow the interface descriptor.
        DescriptorHeader = (USB_DESC_HEAD *)((UINT8 *)DescriptorBuffer + DescriptorCursor + DescriptorHeader->Len);
        if (DescriptorHeader->Type == USB_DESC_TYPE_HID) {
          // found the HID descriptor.
          DiscoveredHidDescriptor = (EFI_USB_HID_DESCRIPTOR *)DescriptorHeader;
        }

        // If the HID descriptor exists, it must be at this point in the structure. So whether it was found or not,
        // further searching is not required and the loop can be broken here.
        break;
      }
    }

    // Check if the descriptor length is 0
    if (DescriptorHeader->Len == 0) {
      DEBUG ((DEBUG_ERROR, "[%a] Descriptor length is 0\n", __func__));
      break;
    }

    // move to next descriptor
    DescriptorCursor += DescriptorHeader->Len;
    DescriptorHeader  = (USB_DESC_HEAD *)((UINT8 *)DescriptorBuffer + DescriptorCursor);
  }

  // if HID descriptor not found, free and return.
  if (DiscoveredHidDescriptor == NULL) {
    FreePool (DescriptorBuffer);
    return EFI_UNSUPPORTED;
  }

  // if found, copy it and return.
  *HidDescriptor = AllocateCopyPool (DiscoveredHidDescriptor->Length, DiscoveredHidDescriptor);
  FreePool (DescriptorBuffer);

  if (*HidDescriptor == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}

/**
  Retrieves the descriptors for the given device.

  @param  UsbHidDevice          Pointer to the UsbHidDevice instance that will be updated with the descriptors.

  @retval EFI_SUCCESS           UsbHidDevice context has been updated with the descriptor information.
  @retval Other                 Unexpected error retrieving the descriptors.
**/
EFI_STATUS
ReadDescriptors (
  IN USB_HID_DEV  *UsbHidDevice
  )
{
  EFI_STATUS                   Status;
  UINT8                        Index;
  EFI_USB_ENDPOINT_DESCRIPTOR  EndpointDescriptor;

  DEBUG ((DEBUG_VERBOSE, "[%a:%d] getting descriptors.\n", __FUNCTION__, __LINE__));

  ZeroMem (&UsbHidDevice->IntInEndpointDescriptor, sizeof (UsbHidDevice->IntInEndpointDescriptor));

  Status = UsbHidDevice->UsbIo->UsbGetInterfaceDescriptor (UsbHidDevice->UsbIo, &UsbHidDevice->InterfaceDescriptor);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    goto ErrorExit;
  }

  DEBUG ((
    DEBUG_VERBOSE,
    "[%a:%d] interface class: 0x%x, subclass: 0x%x, protocol: 0x%x.\n",
    __FUNCTION__,
    __LINE__,
    UsbHidDevice->InterfaceDescriptor.InterfaceClass,
    UsbHidDevice->InterfaceDescriptor.InterfaceSubClass,
    UsbHidDevice->InterfaceDescriptor.InterfaceProtocol
    ));

  for (Index = 0; Index < UsbHidDevice->InterfaceDescriptor.NumEndpoints; Index++) {
    Status = UsbHidDevice->UsbIo->UsbGetEndpointDescriptor (UsbHidDevice->UsbIo, Index, &EndpointDescriptor);
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      goto ErrorExit;
    }

    if ((EndpointDescriptor.Attributes & 0x03) == USB_ENDPOINT_INTERRUPT) {
      if ((EndpointDescriptor.EndpointAddress & USB_ENDPOINT_DIR_IN) != 0) {
        CopyMem (&UsbHidDevice->IntInEndpointDescriptor, &EndpointDescriptor, sizeof (EndpointDescriptor));
        break;
      }
    }
  }

  //  Interrupt In end point must be found.
  if (UsbHidDevice->IntInEndpointDescriptor.Length == 0) {
    Status = EFI_DEVICE_ERROR;
    goto ErrorExit;
  }

  Status = UsbGetFullHidDescriptor (UsbHidDevice->UsbIo, &UsbHidDevice->InterfaceDescriptor, &UsbHidDevice->HidDescriptor);
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

ErrorExit:
  return Status;
}

/**
  Tests to see if this driver supports a given controller. If a child device is provided,
  it further tests to see if this driver supports creating a handle for the specified child device.

  This function checks to see if the driver specified by This supports the device specified by
  ControllerHandle. Drivers will typically use the device path attached to
  ControllerHandle and/or the services from the bus I/O abstraction attached to
  ControllerHandle to determine if the driver supports ControllerHandle. This function
  may be called many times during platform initialization. In order to reduce boot times, the tests
  performed by this function must be very small, and take as little time as possible to execute. This
  function must not change the state of any hardware devices, and this function must be aware that the
  device specified by ControllerHandle may already be managed by the same driver or a
  different driver. This function must match its calls to AllocatePages() with FreePages(),
  AllocatePool() with FreePool(), and OpenProtocol() with CloseProtocol().
  Because ControllerHandle may have been previously started by the same driver, if a protocol is
  already in the opened state, then it must not be closed with CloseProtocol(). This is required
  to guarantee the state of ControllerHandle is not modified by this function.

  @param[in]  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle     The handle of the controller to test. This handle
                                   must support a protocol interface that supplies
                                   an I/O abstraction to the driver.
  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a device path.  This
                                   parameter is ignored by device drivers, and is optional for bus
                                   drivers. For bus drivers, if this parameter is not NULL, then
                                   the bus driver must determine if the bus controller specified
                                   by ControllerHandle and the child controller specified
                                   by RemainingDevicePath are both supported by this
                                   bus driver.

  @retval EFI_SUCCESS              The device specified by ControllerHandle and
                                   RemainingDevicePath is supported by the driver specified by This.
  @retval EFI_ALREADY_STARTED      The device specified by ControllerHandle and
                                   RemainingDevicePath is already being managed by the driver
                                   specified by This.
  @retval EFI_ACCESS_DENIED        The device specified by ControllerHandle and
                                   RemainingDevicePath is already being managed by a different
                                   driver or an application that requires exclusive access.
                                   Currently not implemented.
  @retval EFI_UNSUPPORTED          The device specified by ControllerHandle and
                                   RemainingDevicePath is not supported by the driver specified by This.
**/
EFI_STATUS
EFIAPI
UsbHidDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS           Status;
  EFI_USB_IO_PROTOCOL  *UsbIo;

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiUsbIoProtocolGuid,
                  (VOID **)&UsbIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Use the USB I/O Protocol interface to check whether Controller is
  // a hid device that can be managed by this driver.
  //
  Status = EFI_SUCCESS;
  if (!IsUsbHid (UsbIo)) {
    Status = EFI_UNSUPPORTED;
  }

  gBS->CloseProtocol (
         Controller,
         &gEfiUsbIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  return Status;
}

/**
  Starts a device controller or a bus controller.

  The Start() function is designed to be invoked from the EFI boot service ConnectController().
  As a result, much of the error checking on the parameters to Start() has been moved into this
  common boot service. It is legal to call Start() from other locations,
  but the following calling restrictions must be followed, or the system behavior will not be deterministic.
  1. ControllerHandle must be a valid EFI_HANDLE.
  2. If RemainingDevicePath is not NULL, then it must be a pointer to a naturally aligned
     EFI_DEVICE_PATH_PROTOCOL.
  3. Prior to calling Start(), the Supported() function for the driver specified by This must
     have been called with the same calling parameters, and Supported() must have returned EFI_SUCCESS.

  @param[in]  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle     The handle of the controller to start. This handle
                                   must support a protocol interface that supplies
                                   an I/O abstraction to the driver.
  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a device path.  This
                                   parameter is ignored by device drivers, and is optional for bus
                                   drivers. For a bus driver, if this parameter is NULL, then handles
                                   for all the children of Controller are created by this driver.
                                   If this parameter is not NULL and the first Device Path Node is
                                   not the End of Device Path Node, then only the handle for the
                                   child device specified by the first Device Path Node of
                                   RemainingDevicePath is created by this driver.
                                   If the first Device Path Node of RemainingDevicePath is
                                   the End of Device Path Node, no child handle is created by this
                                   driver.

  @retval EFI_SUCCESS              The device was started.
  @retval EFI_DEVICE_ERROR         The device could not be started due to a device error.Currently not implemented.
  @retval EFI_OUT_OF_RESOURCES     The request could not be completed due to a lack of resources.
  @retval Others                   The driver failed to start the device.

**/
EFI_STATUS
EFIAPI
UsbHidDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS           Status;
  EFI_TPL              OldTpl;
  EFI_USB_IO_PROTOCOL  *UsbIo;
  USB_HID_DEV          *UsbHidDevice = NULL;

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiUsbIoProtocolGuid,
                  (VOID **)&UsbIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto ExitTpl;
  }

  UsbHidDevice = AllocateZeroPool (sizeof (USB_HID_DEV));
  if (UsbHidDevice == NULL) {
    ASSERT (UsbHidDevice != NULL);
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorExit;
  }

  UsbHidDevice->Signature = USB_HID_DEV_SIGNATURE;
  UsbHidDevice->UsbIo     = UsbIo;

  UsbHidDevice->HidIo.GetReportDescriptor      = HidGetReportDescriptor;
  UsbHidDevice->HidIo.GetReport                = HidGetReport;
  UsbHidDevice->HidIo.SetReport                = HidSetReport;
  UsbHidDevice->HidIo.RegisterReportCallback   = HidRegisterReportCallback;
  UsbHidDevice->HidIo.UnregisterReportCallback = HidUnregisterReportCallback;

  Status = ReadDescriptors (UsbHidDevice);
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  // Some boot devices send a report descriptor for the "non-boot" reports they support
  // but then send boot reports unless they are explicitly configured for report mode.
  // So explicitly set the device to report mode if it is a boot device.
  if (UsbHidDevice->InterfaceDescriptor.InterfaceSubClass == SUBCLASS_BOOT) {
    Status = UsbSetProtocolRequest (
               UsbHidDevice->UsbIo,
               UsbHidDevice->InterfaceDescriptor.InterfaceNumber,
               REPORT_PROTOCOL
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "[%a] failed to set report protocol on device: %r\n", __FUNCTION__, Status));
    }
  }

  Status = gBS->InstallProtocolInterface (
                  &Controller,
                  &gHidIoProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &UsbHidDevice->HidIo
                  );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    goto ErrorExit;
  }

  // Success, but we still need to restore TPL.
  goto ExitTpl;

ErrorExit:
  // best effort - nothing to do if this fails.
  gBS->CloseProtocol (
         Controller,
         &gEfiUsbIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );
  if (UsbHidDevice != NULL) {
    if (UsbHidDevice->HidDescriptor != NULL) {
      FreePool (UsbHidDevice->HidDescriptor);
    }

    if (UsbHidDevice->ReportDescriptor != NULL) {
      FreePool (UsbHidDevice->ReportDescriptor);
    }

    FreePool (UsbHidDevice);
  }

ExitTpl:
  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Stops a device controller or a bus controller.

  The Stop() function is designed to be invoked from the EFI boot service DisconnectController().
  As a result, much of the error checking on the parameters to Stop() has been moved
  into this common boot service. It is legal to call Stop() from other locations,
  but the following calling restrictions must be followed, or the system behavior will not be deterministic.
  1. ControllerHandle must be a valid EFI_HANDLE that was used on a previous call to this
     same driver's Start() function.
  2. The first NumberOfChildren handles of ChildHandleBuffer must all be a valid
     EFI_HANDLE. In addition, all of these handles must have been created in this driver's
     Start() function, and the Start() function must have called OpenProtocol() on
     ControllerHandle with an Attribute of EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER.

  @param[in]  This              A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle  A handle to the device being stopped. The handle must
                                support a bus specific I/O protocol for the driver
                                to use to stop the device.
  @param[in]  NumberOfChildren  The number of child device handles in ChildHandleBuffer.
  @param[in]  ChildHandleBuffer An array of child handles to be freed. May be NULL
                                if NumberOfChildren is 0.

  @retval EFI_SUCCESS           The device was stopped.
  @retval EFI_DEVICE_ERROR      The device could not be stopped due to a device error.

**/
EFI_STATUS
EFIAPI
UsbHidDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Controller,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
{
  EFI_STATUS       Status;
  EFI_TPL          OldTpl;
  HID_IO_PROTOCOL  *HidIo;
  USB_HID_DEV      *UsbHidDevice = NULL;

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  Status = gBS->OpenProtocol (
                  Controller,
                  &gHidIoProtocolGuid,
                  (VOID **)&HidIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    goto ExitTpl;
  }

  UsbHidDevice = USB_HID_DEV_FROM_HID_IO_PROTOCOL (HidIo);

  Status = gBS->UninstallProtocolInterface (
                  Controller,
                  &gHidIoProtocolGuid,
                  HidIo
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "[%a] Failed to uninstall HidIo: %r\n", __FUNCTION__, Status));
    ASSERT_EFI_ERROR (Status);
  }

  ShutdownAsyncInterruptInputTransfers (UsbHidDevice);

  gBS->CloseProtocol (
         Controller,
         &gEfiUsbIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  if (UsbHidDevice->HidDescriptor != NULL) {
    FreePool (UsbHidDevice->HidDescriptor);
  }

  if (UsbHidDevice->ReportDescriptor != NULL) {
    FreePool (UsbHidDevice->ReportDescriptor);
  }

  FreePool (UsbHidDevice);

ExitTpl:
  gBS->RestoreTPL (OldTpl);
  return EFI_SUCCESS;
}

EFI_DRIVER_BINDING_PROTOCOL  gUsbHidDriverBinding = {
  UsbHidDriverBindingSupported,
  UsbHidDriverBindingStart,
  UsbHidDriverBindingStop,
  1,
  NULL,
  NULL
};

/**
  Entrypoint of USB HID Driver.

  This function is the entrypoint of USB Mouse Driver. It installs the Driver Binding
  Protocol for managing USB HID devices.

  @param  ImageHandle       The firmware allocated handle for the EFI image.
  @param  SystemTable       A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.

**/
EFI_STATUS
EFIAPI
UsbHidEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = EfiLibInstallDriverBinding (
             ImageHandle,
             SystemTable,
             &gUsbHidDriverBinding,
             ImageHandle
             );

  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
