/** @file
  This file include all platform action which can be customized by IBV/OEM.

Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under
the terms and conditions of the BSD License that accompanies this distribution.
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "BdsPlatform.h"

static EFI_BOOT_MODE                  mBootMode;
static EFI_DEVICE_PATH_PROTOCOL     **mPlatformConnectSequence;
static USB_CLASS_FORMAT_DEVICE_PATH   mUsbClassKeyboardDevicePath = {
  {
    {
      MESSAGING_DEVICE_PATH,
      MSG_USB_CLASS_DP,
      {
        (UINT8) (sizeof (USB_CLASS_DEVICE_PATH)),
        (UINT8) ((sizeof (USB_CLASS_DEVICE_PATH)) >> 8)
      }
    },
    0xffff,           // VendorId
    0xffff,           // ProductId
    CLASS_HID,        // DeviceClass
    SUBCLASS_BOOT,    // DeviceSubClass
    PROTOCOL_KEYBOARD // DeviceProtocol
  },
  gEndEntire
};

VOID
ExitPmAuth (
  VOID
  )
{
  EFI_HANDLE                  Handle;
  EFI_STATUS                  Status;


  PERF_FUNCTION_BEGIN (); // MS_CHANGE

  DEBUG((DEBUG_INFO,"ExitPmAuth ()- Start\n"));

  //
  // Since PI1.2.1, we need signal EndOfDxe as ExitPmAuth
  //
  EfiEventGroupSignal (&gEfiEndOfDxeEventGroupGuid);

  DEBUG((DEBUG_INFO,"All EndOfDxe callbacks have returned successfully\n"));

  //
  // NOTE: We need install DxeSmmReadyToLock directly here because many boot script is added via ExitPmAuth/EndOfDxe callback.
  // If we install them at same callback, these boot script will be rejected because BootScript Driver runs first to lock them done.
  // So we seperate them to be 2 different events, ExitPmAuth is last chance to let platform add boot script. DxeSmmReadyToLock will
  // make boot script save driver lock down the interface.
  //
  Handle = NULL;
  Status = gBS->InstallProtocolInterface (
                  &Handle,
                  &gEfiDxeSmmReadyToLockProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);
  DEBUG((DEBUG_INFO,"ExitPmAuth ()- End\n"));

  PERF_FUNCTION_END (); // MS_CHANGE
}

VOID
ConnectRootBridge (
  BOOLEAN Recursive
  )
{
  UINTN                            RootBridgeHandleCount;
  EFI_HANDLE                       *RootBridgeHandleBuffer;
  UINTN                            RootBridgeIndex;

  PERF_FUNCTION_BEGIN (); // MS_CHANGE

  RootBridgeHandleCount = 0;
  gBS->LocateHandleBuffer (
         ByProtocol,
         &gEfiPciRootBridgeIoProtocolGuid,
         NULL,
         &RootBridgeHandleCount,
         &RootBridgeHandleBuffer
         );
  for (RootBridgeIndex = 0; RootBridgeIndex < RootBridgeHandleCount; RootBridgeIndex++) {
    gBS->ConnectController (RootBridgeHandleBuffer[RootBridgeIndex], NULL, NULL, Recursive);
  }

  PERF_FUNCTION_END (); // MS_CHANGE
}


BOOLEAN
IsGopDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  while (!IsDevicePathEndType (DevicePath)) {
    if (DevicePathType (DevicePath) == ACPI_DEVICE_PATH &&
        DevicePathSubType (DevicePath) == ACPI_ADR_DP) {
      return TRUE;
    }
    DevicePath = NextDevicePathNode (DevicePath);
  }
  return FALSE;
}

/**
  Remove all GOP device path instance from DevicePath and add the Gop to the DevicePath.
**/
EFI_DEVICE_PATH_PROTOCOL *
UpdateGopDevicePath (
  EFI_DEVICE_PATH_PROTOCOL *DevicePath,
  EFI_DEVICE_PATH_PROTOCOL *Gop
  )
{
  UINTN                    Size;
  UINTN                    GopSize;
  EFI_DEVICE_PATH_PROTOCOL *Temp;
  EFI_DEVICE_PATH_PROTOCOL *Return;
  EFI_DEVICE_PATH_PROTOCOL *Instance;
  BOOLEAN                  Exist;

  Exist = FALSE;
  Return = NULL;
  GopSize = GetDevicePathSize (Gop);
  do {
    Instance = GetNextDevicePathInstance (&DevicePath, &Size);
    if (Instance == NULL) {
      break;
    }
    if (!IsGopDevicePath (Instance) ||
        (Size == GopSize && CompareMem (Instance, Gop, GopSize) == 0)
       ) {
      if (Size == GopSize && CompareMem (Instance, Gop, GopSize) == 0) {
        Exist = TRUE;
      }
      Temp = Return;
      Return = AppendDevicePathInstance (Return, Instance);
      if (Temp != NULL) {
        FreePool (Temp);
      }
    }
    FreePool (Instance);
  } while (DevicePath != NULL);

  if (!Exist) {
    Temp = Return;
    Return = AppendDevicePathInstance (Return, Gop);
    if (Temp != NULL) {
      FreePool (Temp);
    }
  }
  return Return;
}

/**
  Platform Bds init. Incude the platform firmware vendor, revision
  and so crc check.
**/
VOID
EFIAPI
PlatformBootManagerBeforeConsole (
  VOID
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL            *TempDevicePath;
  EFI_DEVICE_PATH_PROTOCOL            *ConsoleOut;
  EFI_DEVICE_PATH_PROTOCOL            *Temp;
  EFI_HANDLE                          Handle;
  BDS_CONSOLE_CONNECT_ENTRY          *PlatformConsoles;

  UINTN                     TempSize;
  BOOLEAN                   IsConsoleConfigured;

  mBootMode = GetBootModeHob();  // BeforeConsole has to be called before AfterConsole.

  IsConsoleConfigured = TRUE;
  // Try to find ConIn and ConOut from variable services, either variable is not configured will set flag to FALSE
  TempSize = 0;
  Status = gRT->GetVariable (EFI_CON_IN_VARIABLE_NAME, &gEfiGlobalVariableGuid, NULL, &TempSize, NULL);
  if (Status != EFI_BUFFER_TOO_SMALL) {
      IsConsoleConfigured = FALSE;
  }

  TempSize = 0;
  Status = gRT->GetVariable (EFI_CON_OUT_VARIABLE_NAME, &gEfiGlobalVariableGuid, NULL, &TempSize, NULL);
  if (Status != EFI_BUFFER_TOO_SMALL) {
      IsConsoleConfigured = FALSE;
  }

  //
  // Append Usb Keyboard short form DevicePath into "ConIn"
  //
  EfiBootManagerUpdateConsoleVariable (
    ConIn,
    (EFI_DEVICE_PATH_PROTOCOL *) &mUsbClassKeyboardDevicePath,
    NULL
    );

  //
  // Connect Root Bridge to make PCI BAR resource allocated and all PciIo created
  //
  ConnectRootBridge (FALSE);

  TempDevicePath = NULL;
  Handle = DeviceBootManagerBeforeConsole (&TempDevicePath, &PlatformConsoles);

  //
  // Update ConOut variable accordign to the Console Handle
  //
  ConsoleOut = NULL;
  GetEfiGlobalVariable2 (L"ConOut", (VOID **) &ConsoleOut,NULL);

  if (Handle != NULL) {
    if (TempDevicePath != NULL) {
      Temp = ConsoleOut;
      ConsoleOut = UpdateGopDevicePath (ConsoleOut, TempDevicePath);
      if (Temp != NULL) {
        FreePool (Temp);
      }
      FreePool (TempDevicePath);
      Status = gRT->SetVariable (
                      L"ConOut",
                      &gEfiGlobalVariableGuid,
                      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                      GetDevicePathSize (ConsoleOut),
                      ConsoleOut
                      );
    }
  }

  if (ConsoleOut != NULL) {
    FreePool(ConsoleOut);
  }

  //
  // Fill ConIn/ConOut in Full Configuration boot mode
  //
  DEBUG ((DEBUG_INFO, "%a - %x\n", __FUNCTION__, mBootMode));
  if (mBootMode == BOOT_WITH_FULL_CONFIGURATION ||
      mBootMode == BOOT_WITH_DEFAULT_SETTINGS ||
      mBootMode == BOOT_WITH_FULL_CONFIGURATION_PLUS_DIAGNOSTICS ||
      mBootMode == BOOT_IN_RECOVERY_MODE) {


    if (IsConsoleConfigured == FALSE) {
      //
      // Only fill ConIn/ConOut when ConIn/ConOut is empty because we may drop to Full Configuration boot mode in non-first boot
      //

      if (PlatformConsoles != NULL) {
          while ((*PlatformConsoles).DevicePath != NULL) {
            //
            // Update the console variable with the connect type
            //
            if (((*PlatformConsoles).ConnectType & CONSOLE_IN) == CONSOLE_IN) {
              EfiBootManagerUpdateConsoleVariable (ConIn, (*PlatformConsoles).DevicePath, NULL);
            }
            if (((*PlatformConsoles).ConnectType & CONSOLE_OUT) == CONSOLE_OUT) {
              EfiBootManagerUpdateConsoleVariable (ConOut, (*PlatformConsoles).DevicePath, NULL);
            }
            if (((*PlatformConsoles).ConnectType & STD_ERROR) == STD_ERROR) {
              EfiBootManagerUpdateConsoleVariable (ErrOut, (*PlatformConsoles).DevicePath, NULL);
            }
            PlatformConsoles++;
          }
      }
    }
  }

  //
  // Exit PM auth before Legacy OPROM run.
  //

  ExitPmAuth ();

  //
  // Dispatch the deferred 3rd party images.
  //
  EfiBootManagerDispatchDeferredImages ();
}

VOID
ConnectSequence (
  VOID
  )
{
  EFI_HANDLE               DeviceHandle;
  EFI_STATUS               Status;
  EFI_DEVICE_PATH_PROTOCOL **PlatformConnectSequence;

  PERF_FUNCTION_BEGIN (); // MS_CHANGE

  //
  // Here we can get the customized platform connect sequence
  // Notes: we can connect with new variable which record the
  // last time boots connect device path sequence
  //
  PlatformConnectSequence = mPlatformConnectSequence;
  if (PlatformConnectSequence != NULL) {
      while (*PlatformConnectSequence != NULL) {
        //
        // Build the platform boot option
        //
        Status = EfiBootManagerConnectDevicePath (*PlatformConnectSequence, &DeviceHandle);
        if (!EFI_ERROR (Status)) {
            gBS->ConnectController (DeviceHandle, NULL, NULL, TRUE);
        }
        PlatformConnectSequence++;
      }
  }

  //
  // Dispatch again since Switchable Graphics driver depends on PCI_IO protocol
  //
  gDS->Dispatch ();

  PERF_FUNCTION_END (); // MS_CHANGE
}

STATIC
EFI_STATUS
SetMorControl (
  VOID
  )
{
  UINT8                        MorControl;
  UINTN                        VariableSize;
  EFI_STATUS                   Status;

  VariableSize = sizeof (MorControl);
  MorControl = 1;

  Status = gRT->SetVariable(
                  MEMORY_OVERWRITE_REQUEST_VARIABLE_NAME,
                  &gEfiMemoryOverwriteControlDataGuid,
                  EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                  VariableSize,
                  &MorControl
                  );

  return Status;
}


/**
  The function will excute with as the platform policy, current policy
  is driven by boot mode. IBV/OEM can customize this code for their specific
  policy action.

  @param DriverOptionList - The header of the driver option link list
  @param BootOptionList   - The header of the boot option link list
  @param ProcessCapsules  - A pointer to ProcessCapsules()
  @param BaseMemoryTest   - A pointer to BaseMemoryTest()
**/
VOID
EFIAPI
PlatformBootManagerAfterConsole (
  VOID
  )
{
  EFI_STATUS                    Status;
//  EFI_INPUT_KEY                 Key;

  if (PcdGetBool(PcdTestKeyUsed) == TRUE) {
    Print(L"WARNING: Capsule Test Key is used.\n");
    DEBUG((DEBUG_INFO, "WARNING: Capsule Test Key is used.\n"));
  }

  mPlatformConnectSequence = DeviceBootManagerAfterConsole ();

  //
  // Boot Mode obtained in BeforeConsole action.
  //
  DEBUG((DEBUG_INFO,"BootMode 0x%x\n", mBootMode));

  //
  // Go the different platform policy with different boot mode
  // Notes: this part code can be change with the table policy
  //
  switch (mBootMode) {
    case BOOT_ON_S4_RESUME:
    case BOOT_WITH_MINIMAL_CONFIGURATION:
      DEBUG((DEBUG_ERROR, "THIS BOOT MODE IS UNSUPPORTED.  0x%X \n", mBootMode));

    case BOOT_ASSUMING_NO_CONFIGURATION_CHANGES:

      // run memory test here to mark all memory good.  This is a hack until we get real BDS.
      Status = MemoryTest(QUICK);  //we use NULL memory test so level doesn't matter.

      //
      // Perform some platform specific connect sequence
      //
      ConnectSequence ();

      break;

    case BOOT_ON_FLASH_UPDATE:
      EfiBootManagerConnectAll();
      Status = ProcessCapsules();

      // If capsule update require reboot
      // this function will not return.
      if (EFI_ERROR(Status))
      {
        SetMorControl();
        DEBUG((DEBUG_INFO, "Locate and Process Capsules returned error (Status=%r). Setting MOR to clear memory and initiating reset.\n", Status));
      }

      //If we get here we need to reboot as we never want to boot in Flash Update mode.
      gRT->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, NULL);
      break;


    case BOOT_IN_RECOVERY_MODE:
      DEBUG((DEBUG_ERROR, "THIS BOOT MODE IS UNSUPPORTED.  0x%X \n", mBootMode));

      //
      // In recovery boot mode, we still enter to the
      // frong page now
      //

      break;

    case BOOT_WITH_FULL_CONFIGURATION:
    case BOOT_WITH_FULL_CONFIGURATION_PLUS_DIAGNOSTICS:
      DEBUG((DEBUG_ERROR, "THIS BOOT MODE IS UNSUPPORTED.  0x%X \n", mBootMode));

    case BOOT_WITH_DEFAULT_SETTINGS:
    default:
      // run memory test here to mark all memory good.  This is a hack until we get real BDS.
      Status = MemoryTest(QUICK);  //we use NULL memory test so level doesn't matter.
      //
      // Perform some platform specific connect sequence
      //
      // PERF_START_EX(NULL,"EventRec", NULL, AsmReadTsc(), 0x7050); // MS_CHANGE
      ConnectSequence ();
      // PERF_END_EX(NULL,"EventRec", NULL, AsmReadTsc(), 0x7051); // MS_CHANGE

      break;
  }

  //
  // For all cases, we need to call ProcessCapsules in order to clear
  // the capsule variables. The BOOT_ON_FLASH_UPDATE case above calls this
  // routine but the system is always reset in that case before reaching this
  // point.
  //
  (void)ProcessCapsules();
}

/**
  This function is called each second during the boot manager waits the timeout.

  @param TimeoutRemain  The remaining timeout.
**/
 VOID
 EFIAPI
PlatformBootManagerWaitCallback (
  UINT16          TimeoutRemain
  )
 {
  return;
 }
