/*--

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.


Module Name:

    PrmFuncSample.c

Abstract:

    This module implements a sample that utilizes the
    Windows PRM direct call interface.

Environment:

    Kernel mode only.

--*/

#include "prmfuncsample.h"

HANDLE  FileHandle = NULL;

VOID
EvtDriverUnload (
  WDFDRIVER  Driver
  )
{
  UNREFERENCED_PARAMETER (Driver);
  return;
}

NTSTATUS
DriverEntry (
  _In_ PDRIVER_OBJECT   DriverObject,
  _In_ PUNICODE_STRING  RegistryPath
  )

/*++

Routine Description:
    DriverEntry initializes the driver and is the first routine called by the
    system after the driver is loaded. DriverEntry configures and creates a WDF driver
    object.
    .
Parameters Description:

    DriverObject - represents the instance of the function driver that is loaded
        into memory. DriverObject is allocated by the system before the
        driver is loaded, and it is released by the system after the system unloads
        the function driver from memory.

    RegistryPath - represents the driver specific path in the Registry.
        The function driver can use the path to store driver related data between
        reboots. The path does not store hardware instance specific data.

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise.

--*/
{
  WDF_DRIVER_CONFIG                     Config;
  WDFDRIVER                             Driver;
  NTSTATUS                              Status;
  PRM_DIRECT_CALL_PARAMETERS            TestParameters;
  PRM_INTERFACE                         PrmInterface;
  ULONG64                               EfiStatus;
  BOOLEAN                               Found;
  ADVANCED_LOGGER_PRM_PARAMETER_BUFFER  *ParamBuf;
  VOID                                  *KernelBuf = NULL;
  UNICODE_STRING                        filePath;
  OBJECT_ATTRIBUTES                     objAttr;
  HANDLE                                fileHandle;
  IO_STATUS_BLOCK                       ioStatus;
  UINT32                                BufSize = 0;

  KdPrint (("PRM Function Test Driver - Driver Framework Edition.\n"));

  // Initialize the driver configuration structure
  WDF_DRIVER_CONFIG_INIT (&Config, WDF_NO_EVENT_CALLBACK);

  // Register the EvtDriverUnload callback
  Config.EvtDriverUnload = EvtDriverUnload;
  Config.DriverInitFlags = WdfDriverInitNonPnpDriver;

  //
  // Create a framework driver object to represent our driver.
  //

  Status = WdfDriverCreate (
             DriverObject,
             RegistryPath,
             WDF_NO_OBJECT_ATTRIBUTES, // Driver Attributes
             &Config,                  // Driver Config Info
             &Driver
             );

  if (!NT_SUCCESS (Status)) {
    KdPrint (("WdfDriverCreate failed with status 0x%x\n", Status));
    return Status;
  }

  TestParameters.Guid        = GUID_ADVLOGGER_PRM_HANDLER;
  ParamBuf                   = (ADVANCED_LOGGER_PRM_PARAMETER_BUFFER *)&(TestParameters.ParameterBuffer);
  ParamBuf->OutputBuffer     = NULL;
  ParamBuf->OutputBufferSize = &BufSize;

  //
  // Acquire the direct-call PRM interface, which is defined in
  // prminterface.h.
  //
  //     typedef struct _PRM_INTERFACE {
  //        ULONG Version;
  //        PPRM_UNLOCK_MODULE UnlockModule;
  //        PPRM_LOCK_MODULE LockModule;
  //        PPRM_INVOKE_HANDLER InvokeHandler;
  //        PPRM_QUERY_HANDLER  QueryHandler;
  //     } PRM_INTERFACE, *PPRM_INTERFACE;
  //

  Status = ExGetPrmInterface (1, &PrmInterface);
  if (!NT_SUCCESS (Status)) {
    return Status;
  }

  //
  // Lock the handler's PRM module to synchronize against any potential
  // runtime update to the PRM module.
  //
  // N.B. Note that technically this is only needed if a series of PRM
  // handlers need to be called transactionally (thus preventing
  // interleaving of PRM module updates). However, we will do it here
  // as an example of how it could be done.
  //

  Status = PrmInterface.LockModule (
                          (LPGUID)&TestParameters.Guid
                          );

  if (!NT_SUCCESS (Status)) {
    return Status;
  }

  //
  // Query for the presence of the PRM handler.
  //

  Status = PrmInterface.QueryHandler (
                          (LPGUID)&TestParameters.Guid,
                          &Found
                          );

  if ((!NT_SUCCESS (Status)) || (Found == FALSE)) {
    return Status;
  }

  //
  // Invoke the PRM handler
  //

  Status = PrmInterface.InvokeHandler (
                          (LPGUID)&TestParameters.Guid,
                          TestParameters.ParameterBuffer,
                          0,
                          &EfiStatus
                          );

  if (!NT_SUCCESS (Status)) {
    return Status;
  }

  if ((EfiStatus == EFI_BUFFER_TOO_SMALL) || (EfiStatus == EFI_BUFFER_TOO_SMALL_TRUNCATED)) {
    // ParamBuf now has the right size to alloc
    KernelBuf = ExAllocatePool2 (POOL_FLAG_NON_PAGED, BufSize, 'prmf');
    if (KernelBuf == NULL) {
      return STATUS_INSUFFICIENT_RESOURCES;
    }

    ParamBuf->OutputBuffer = KernelBuf;

    Status = PrmInterface.InvokeHandler (
                            (LPGUID)&TestParameters.Guid,
                            TestParameters.ParameterBuffer,
                            0,
                            &EfiStatus
                            );

    if (!NT_SUCCESS (Status)) {
      return Status;
    }

    if (EfiStatus == 0) {
      RtlInitUnicodeString (&filePath, L"\\??\\C:\\AdvLogger.log");
      InitializeObjectAttributes (&objAttr, &filePath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
      Status = ZwCreateFile (
                 &fileHandle,
                 FILE_GENERIC_WRITE,
                 &objAttr,
                 &ioStatus,
                 NULL,
                 FILE_ATTRIBUTE_NORMAL,
                 0,
                 FILE_OVERWRITE_IF,
                 FILE_SYNCHRONOUS_IO_NONALERT,
                 NULL,
                 0
                 );

      if (NT_SUCCESS (Status)) {
        // Write the buffer to the file
        Status = ZwWriteFile (fileHandle, NULL, NULL, NULL, &ioStatus, KernelBuf, BufSize, NULL, NULL);
        ZwClose (fileHandle);
      }
    }
  }

  //
  // Unlock the PRM module
  //
  // N.B. Note that technically this is only needed if a series of PRM
  // handlers need to be called as part of transaction. However, we will
  // do it here as an example of how it could be done.
  //

  Status = PrmInterface.UnlockModule (
                          (LPGUID)&TestParameters.Guid
                          );

  return Status;
}
