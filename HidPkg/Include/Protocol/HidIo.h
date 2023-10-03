/** @file
  HID I/O

  This protocol is used by code, typically drivers, running in the EFI
  boot services environment to access HID devices.

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef HID_IO_H__
#define HID_IO_H__

typedef struct _HID_IO_PROTOCOL HID_IO_PROTOCOL;

typedef enum {
  InputReport  = 1,
  OutputReport = 2,
  Feature      = 3
} HID_REPORT_TYPE;

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
typedef
EFI_STATUS
(EFIAPI *HID_IO_GET_REPORT_DESCRIPTOR)(
  IN HID_IO_PROTOCOL *This,
  IN OUT UINTN *ReportDescriptorSize,
  IN OUT VOID *ReportDescriptorBuffer
  );

/**
  Retrieves a single report from the device.

  @param  This                  A pointer to the HidIo Instance
  @param  ReportId              Specifies which report to return if the device supports multiple input reports.
                                Set to zero if ReportId is not present.
  @param  ReportType            Indicates the type of report type to retrieve. 1-Input, 3-Feature.
  @param  ReportBufferSize      Indicates the size of the provided buffer to receive the report.
  @param  ReportBuffer          Pointer to the buffer to receive the report.

  @retval EFI_SUCCESS           Report successfully returned.
  @retval EFI_OUT_OF_RESOURCES  The provided buffer is not large enough to hold the report.
  @retval EFI_INVALID_PARAMETER Invalid input parameters.
  @retval Other                 Unexpected error reading report.
**/
typedef
EFI_STATUS
(EFIAPI *HID_IO_GET_REPORT)(
  IN HID_IO_PROTOCOL *This,
  IN UINT8 ReportId,
  IN HID_REPORT_TYPE ReportType,
  IN UINTN ReportBufferSize,
  OUT VOID *ReportBuffer
  );

/**
  Sends a single report to the device.

  @param  This                  A pointer to the HidIo Instance
  @param  ReportId              Specifies which report to send if the device supports multiple input reports.
                                Set to zero if ReportId is not present.
  @param  ReportType            Indicates the type of report type to retrieve. 2-Output, 3-Feature.
  @param  ReportBufferSize      Indicates the size of the provided buffer holding the report to send.
  @param  ReportBuffer          Pointer to the buffer holding the report to send.

  @retval EFI_SUCCESS           Report successfully transmitted.
  @retval EFI_INVALID_PARAMETER Invalid input parameters.
  @retval Other                 Unexpected error transmitting report.
**/
typedef
EFI_STATUS
(EFIAPI *HID_IO_SET_REPORT)(
  IN HID_IO_PROTOCOL *This,
  IN UINT8 ReportId,
  IN HID_REPORT_TYPE ReportType,
  IN UINTN ReportBufferSize,
  IN VOID *ReportBuffer
  );

/**
  Report received callback function.

  @param  ReportBufferSize      Indicates the size of the provided buffer holding the received report.
  @param  ReportBuffer          Pointer to the buffer holding the report.
  @param  Context               Context provided when the callback was registered.

  @retval None
**/
typedef
VOID
(EFIAPI *HID_IO_REPORT_CALLBACK)(
  IN UINT16 ReportBufferSize,
  IN VOID *ReportBuffer,
  IN VOID *Context OPTIONAL
  );

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
typedef
EFI_STATUS
(EFIAPI *HID_IO_REGISTER_REPORT_CALLBACK)(
  IN HID_IO_PROTOCOL *This,
  IN HID_IO_REPORT_CALLBACK Callback,
  IN VOID *Context OPTIONAL
  );

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
typedef
EFI_STATUS
(EFIAPI *HID_IO_UNREGISTER_REPORT_CALLBACK)(
  IN HID_IO_PROTOCOL *This,
  IN HID_IO_REPORT_CALLBACK Callback
  );

struct _HID_IO_PROTOCOL {
  HID_IO_GET_REPORT_DESCRIPTOR         GetReportDescriptor;
  HID_IO_GET_REPORT                    GetReport;
  HID_IO_SET_REPORT                    SetReport;
  HID_IO_REGISTER_REPORT_CALLBACK      RegisterReportCallback;
  HID_IO_UNREGISTER_REPORT_CALLBACK    UnregisterReportCallback;
};

#endif
