/** @file
 *PlatformBootManager  - Ms Extensions to BdsDxe.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Guid/GlobalVariable.h>

#include <Protocol/VariableLock.h>

#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DeviceBootManagerLib.h>
#include <Library/DevicePathLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PlatformBootManagerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

static BOOLEAN   mSecViolation         = FALSE;

/**
  OnDemandConInCOnnect
 */
VOID
EFIAPI
PlatformBootManagerOnDemandConInConnect (
  VOID
  )
{
    EFI_HANDLE               DeviceHandle;
    EFI_HANDLE              *HandleBuffer;
    UINTN                    HandleCount;
    UINTN                    Index;
    EFI_DEVICE_PATH_PROTOCOL **PlatformConnectDeviceList;
    EFI_STATUS               Status;
    CHAR16                  *TmpStr;

    /* if no demand consoles then we return success by default */
    Status = EFI_SUCCESS;

    PlatformConnectDeviceList = DeviceBootManagerOnDemandConInConnect ();
    DEBUG((DEBUG_INFO,"Connect List = %p\n",PlatformConnectDeviceList));
    if (PlatformConnectDeviceList != NULL) {
        while (*PlatformConnectDeviceList != NULL) {
            TmpStr = ConvertDevicePathToText (*PlatformConnectDeviceList, FALSE, FALSE);
            DEBUG((DEBUG_INFO,"Connecting %s\n",TmpStr));
            if (TmpStr != NULL) {
                FreePool (TmpStr);
            }
            Status = EfiBootManagerConnectDevicePath(*PlatformConnectDeviceList, &DeviceHandle);
            PlatformConnectDeviceList++;
        }
    }

    gBS->LocateHandleBuffer (
           ByProtocol,
           &gEfiAbsolutePointerProtocolGuid,
           NULL,
           &HandleCount,
           &HandleBuffer
           );
    DEBUG((DEBUG_INFO,"AbsPtr handle count = %d\n",HandleCount));

    for (Index = 0; Index < HandleCount; Index++) {
        DEBUG((DEBUG_INFO,"Connecting AbsPtr = %p\n",HandleBuffer[Index]));
        gBS->ConnectController (HandleBuffer[Index], NULL, NULL, TRUE);
    }

    return;
}

/**
 * Constructor
 *
 * @return EFI_STATUS
 */
EFI_STATUS
EFIAPI
PlatformBootManagerEntry (
    IN EFI_HANDLE ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable
) {

    return EFI_SUCCESS;
}

/**
BDS Entry  - DXE phase complete, BDS Entered.
*/
VOID
EFIAPI
PlatformBootManagerBdsEntry (
  VOID
) {
    EFI_STATUS                    Status;
    EDKII_VARIABLE_LOCK_PROTOCOL *VarLockProtocol = NULL;


    // Delete errant boot option that was accidentally introduced
    // This code can be removed after 12/31/2017
    Status = gRT->SetVariable (
                    L"PlatformRecovery0000",
                    &gEfiGlobalVariableGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS,
                    0,
                    NULL
                    );
    if (EFI_NOT_FOUND != Status) {
        DEBUG((DEBUG_ERROR,"%a leftover PlatformRecovery0000 was deleted\n", __FUNCTION__));
    }

    // Delete DriverOrder before locking it.  We do not support DriverOrder
    Status = gRT->SetVariable (
                    EFI_DRIVER_ORDER_VARIABLE_NAME,
                    &gEfiGlobalVariableGuid,
                    0,
                    0,
                    NULL
                    );
    DEBUG((DEBUG_INFO,"Status from deleting DriverOrder prior to lock. Code=%r\n",Status));

    Status = gBS->LocateProtocol(&gEdkiiVariableLockProtocolGuid, NULL, (VOID**)&VarLockProtocol);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "%a - Failed to locate var lock protocol (%r).  Can't lock driver order variable\n", __FUNCTION__, Status));
    } else {
        Status = VarLockProtocol->RequestToLock(VarLockProtocol, EFI_DRIVER_ORDER_VARIABLE_NAME, &gEfiGlobalVariableGuid);
        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR,"Unable to lock DriverOrder. Code=%r\n",Status));
        } else {
            DEBUG((DEBUG_INFO,"Variable DriverOrder locked\n"));
        }
    }

    DeviceBootManagerBdsEntry ();

    return;
}

/**
ProcessBootCompletion
*/
VOID
EFIAPI
PlatformBootManagerProcessBootCompletion (
  IN EFI_BOOT_MANAGER_LOAD_OPTION *BootOption
) {

    if ((EFI_SUCCESS == BootOption->Status) && (mSecViolation)) {
        mSecViolation = FALSE;
        BootOption->Status = OEM_PREVIOUS_SECURITY_VIOLATION;
    }

    DeviceBootManagerProcessBootCompletion (BootOption);
}


/**
 HardKeyBoot
*/
VOID
EFIAPI
PlatformBootManagerPriorityBoot (
    UINT16 **BootNext
    )
{
    EFI_BOOT_MANAGER_LOAD_OPTION    BootOption;
    EFI_STATUS                      Status = EFI_SUCCESS;

    Status = DeviceBootManagerPriorityBoot (&BootOption);

    //
    // Exit if nothing to process
    //
    if (EFI_NOT_FOUND == Status) {
        DEBUG((DEBUG_INFO,"No Priority Boot option selected.\n"));
        if (*BootNext != NULL ) {
            DEBUG((DEBUG_INFO, "Boot Next is %04X\n", **BootNext));
        }
        return;
    }

    //
    // There is a priority boot.  Clear BootNext
    //
    if (*BootNext != NULL) {
        FreePool (*BootNext);
        *BootNext = NULL;
    }

    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR,"[Bds] VOL/+ or VOL/- detected, and unable to boot. Code=%r\n",Status));
    } else {
        EfiBootManagerBoot (&BootOption);
        EfiBootManagerFreeLoadOption (&BootOption);
    }

    return;
}

/**
 This is called from BDS right before going into front page 
 when no bootable devices/options found
*/
VOID
EFIAPI
PlatformBootManagerUnableToBoot (
    VOID
    )
{

    DeviceBootManagerUnableToBoot();
}

