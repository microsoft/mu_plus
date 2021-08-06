/** @file -- SpinnerTest.c

  Copyright (C) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  SpinnerTest Unit Test

**/

#include <Uefi.h>

#include <Protocol/OnScreenKeyboard.h>
#include <Protocol/SimpleTextIn.h>
#include <Protocol/SimpleTextOut.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

//
// Define time in units of one microsecond for MicrosecondDelay ()
//
#define DELAY_ONE_MILLISECOND  (1000)
#define DELAY_ONE_SECOND       (1000 * DELAY_ONE_MILLISECOND)

//
// Delay times for the messages with a delay time.
//
#define DELAY_NVME         5
#define DELAY_BETWEEN_TEST 2
#define DELAY_SPINNER_ON   5
#define DELAY_TO_STOP      1

//
// All test status messages printed on row 0 are assumed to be the same length.  This way,
// the message line looks correct regardless of the actual text length.
//
#define MSG_INIT           L"Initializing the display                                              "
#define MSG_NVME_DELAY     L"The NVMe spinner should start in %d Seconds                           "
#define MSG_NVME_STARTED   L"The NVMe spinner will display for %d seconds                          "
#define MSG_NVME_DISMISSED L"The NVMe spinner should have been dismissed. Continuing in %d seconds "
#define MSG_DFCI_START     L"The Dfci spinner should be displayed for %d seconds                   "
#define MSG_DFCI_DISMISSED L"The Dfci spinner should have been dismissed. Continuing in %d seconds "
#define MSG_GENERAL_START  L"The NVMe spinner should be displayed in 3 corners for %d seconds      "
#define MSG_GENERAL_MID    L"Adding the 4th and 5th spinner for %d seconds                         "
#define MSG_GENERAL_STOP   L"Removing each spinner, one at a time one second apart                 "
#define MSG_FINISHED       L"The Spinner Test has completed\n"

#define MSG_HELP L"\
\r\n\
*******************************************************************************************\r\n\
* Spinner Test - The spinner test will:                                                   *\r\n\
*                                                                                         *\r\n\
*    1. Fill the display with '-' characters.  It does this to enable visually checking   *\r\n\
*       that the display is properly restored when a spinner is dismissed.                *\r\n\
*                                                                                         *\r\n\
*       NOTE:  The last character on the last line is not written as this will cause UEFI *\r\n\
*              to scroll the display.                                                     *\r\n\
*       NOTE2: This test should be run from the Internal Shell as well as from a shell    *\r\n\
*              booted from a USB device. This will test two different display resolutions.*\r\n\
*                                                                                         *\r\n\
*    2. Will start the NVMe spinner.  This will draw the spinner in the lower right       *\r\n\
*       corner of the display.                                                            *\r\n\
*                                                                                         *\r\n\
*    3. The NVMe spinner will be dismissed.  Verify the '-' characters reappear.          *\r\n\
*                                                                                         *\r\n\
*    4. The Dfci spinner will be displayed in the center of the display.                  *\r\n\
*                                                                                         *\r\n\
*    5. Again, the spinner will be dismissed.  Verify the '-' characters reappear.        *\r\n\
*                                                                                         *\r\n\
*    6. The spinners 2, 3, and 4 will be drawn in the corners, and then removed on at     *\r\n\
*       at a time.                                                                        *\r\n\
*                                                                                         *\r\n\
*    7. The test application will clear the screen and terminate.                         *\r\n\
*                                                                                         *\r\n\
*    Press any key to start the test                                                      *\r\n\
*                                                                                         *\r\n\
*******************************************************************************************\r\n"

//
// Global variables
//
STATIC  EFI_SIMPLE_TEXT_INPUT_PROTOCOL    *gConIn;
STATIC  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL   *gConOut;

/**
  WaitForKey    - Wait for a keyboard press

  @param    NONE

  @retval   NONE
**/
VOID
WaitForKey (
  VOID
  )
{
  UINTN           EventIndex;
  EFI_INPUT_KEY   Key;
  EFI_STATUS      Status;

  //
  // Pause - wait for a keystroke
  //
  while (TRUE) {
    Status = gConIn->ReadKeyStroke (gConIn, &Key);
    if (!EFI_ERROR (Status)) {
      break;
    }

    if (Status != EFI_NOT_READY) {
      continue;
    }

    // Wait for another key press
    gBS->WaitForEvent (1, &gConIn->WaitForKey, &EventIndex);
  }
}

/**
  DisplayMessageWithTimeout   - The message is assumed to have a %d in it to be replaced
                                with a time out left value

  @param    Msg       - The message to be displayed
  @param    Timeout   - Number of seconds to display message

  NOTE:
        The maximum timeout supported is 20 seconds.  Anything longer will be ignored.

**/
VOID
DisplayMessageWithTimeout (
  IN CHAR16 *Msg,
  IN UINTN  Timeout
  )
{
  CHAR16      LocalMessage[sizeof (MSG_INIT) / sizeof (CHAR16)];
  EFI_STATUS  Status;

  if (Timeout > 20) {
    Timeout = 20;
  }

  for (; Timeout > 0; Timeout--) {
    UnicodeSPrint (LocalMessage, sizeof(LocalMessage), Msg, Timeout);

    Status = gConOut->SetCursorPosition (gConOut, 0, 0);
    ASSERT_EFI_ERROR (Status);

    Status = gConOut->OutputString (gConOut, LocalMessage);
    ASSERT_EFI_ERROR (Status);

    MicroSecondDelay (DELAY_ONE_SECOND);
  }
}

/**
  SpinnerTestEntry    - Entry point for the SpinnerTest

  @param      ImageHandle  - Loaded Image handle
  @param      SystemTable  - Pointer to EFI System Table

  @retval     EFI_SUCCESS           - Test encountered no code errors
  @retval     EFI_INVALID_PARAMETER - ConIn or ConOut missing

**/
EFI_STATUS
EFIAPI
SpinnerTestEntry (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  UINTN                              Column;
  UINTN                              Columns;
  EFI_GUID                           *IconGuid;
  UINTN                              IconGuidSize;
  MS_ONSCREEN_KEYBOARD_PROTOCOL     *OSKProtocol = NULL;
  UINTN                              Row;
  UINTN                              Rows;
  EFI_STATUS                         Status;

  gConIn = SystemTable->ConIn;
  gConOut = SystemTable->ConOut;

  if ((gConIn == NULL) || (gConOut == NULL)) {
    DEBUG ((DEBUG_ERROR, "Test requires both ConIn and ConOut\n"));
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  //
  // This test case is a visual test, and prints information about the test before it is run.
  //
  // Step 1.  Clear the display and display the help message.  Wait for a key press to continue.
  //
  Status = gConOut->QueryMode (gConOut, gConOut->Mode->Mode, &Columns, &Rows);
  ASSERT_EFI_ERROR (Status);

  Status = gConOut->ClearScreen (gConOut);
  ASSERT_EFI_ERROR (Status);

  Status = gConOut->OutputString (gConOut, MSG_HELP);
  ASSERT_EFI_ERROR (Status);

  WaitForKey ();

  //
  // Step 2.  If the system has an OnScreen keyboard, turn off the KeyboardIcon as the test
  //          interferes with the KeyboardIcon
  //
  // Disable the OSK Keyboard after WaitForKey as Reading a character will cause the Icon to display
  // again. If there are any errors, just ignore them, ans assume no On Screen Keyboard
  //
  Status = gBS->LocateProtocol (
                  &gMsOSKProtocolGuid,
                  NULL,
                  (VOID **)&OSKProtocol
                  );
  if (EFI_ERROR (Status)) {
    OSKProtocol = NULL;
  } else {
    Status = OSKProtocol->ShowKeyboardIcon (OSKProtocol, FALSE);
    if (EFI_ERROR (Status)) {
      OSKProtocol = NULL;
    }
  }

  //
  // Step 3.  Clear the screen then fill screen with a single character,
  //          reserving the top line for the test case status messages
  //

  Status = gConOut->ClearScreen (gConOut);
  ASSERT_EFI_ERROR (Status);

  Status = gConOut->OutputString (gConOut, MSG_INIT);
  ASSERT_EFI_ERROR (Status);

  for (Row = 1; Row < Rows; Row++) {
    for (Column = 0; Column < Columns; Column++) {
        //
        // Skip writing the last character on the last line - this causes the page to scroll.
        //
        if ((Row != (Rows - 1)) || (Column != (Columns - 1))) {
          Status = gConOut->SetCursorPosition (gConOut, Column, Row);
          ASSERT_EFI_ERROR (Status);

          Status = gConOut->OutputString (gConOut, L"-");
          ASSERT_EFI_ERROR (Status);
        }
    }
  }

  //
  // The spinner only knows about the NVMe spinner.  All other spinners are general purpose.
  //
  // For the purposes of this test, set the unused general purpose spinners to display the NVMe
  // spinner icon.  Currently, only General1 is assigned, and it is assigned to Dfci.
  //

  IconGuid = PcdGetExPtr (&gMsGraphicsPkgTokenSpaceGuid, PcdGeneral5File);

  IconGuidSize = sizeof (EFI_GUID);

  PcdSetExPtrS (&gMsGraphicsPkgTokenSpaceGuid, PcdGeneral2File, &IconGuidSize, IconGuid);
  PcdSetExPtrS (&gMsGraphicsPkgTokenSpaceGuid, PcdGeneral3File, &IconGuidSize, IconGuid);
  PcdSetExPtrS (&gMsGraphicsPkgTokenSpaceGuid, PcdGeneral4File, &IconGuidSize, IconGuid);

  // Set Spinner 2 location to upper left as the NVMe spinner already tested the lower right
  PcdSetEx8S (&gMsGraphicsPkgTokenSpaceGuid, PcdGeneral2Location, 4);
  PcdSetEx8S (&gMsGraphicsPkgTokenSpaceGuid, PcdGeneral3Location, 2);
  PcdSetEx8S (&gMsGraphicsPkgTokenSpaceGuid, PcdGeneral4Location, 3);

  //
  // Step 4.  Signal the NVMe spinner.  The NVMe spinner has a built in 5 second delay,
  //          so indicate to the tester to wait 5 seconds.
  EfiEventGroupSignal (&gNVMeEnableStartEventGroupGuid);
  DisplayMessageWithTimeout (MSG_NVME_DELAY, DELAY_NVME);

  //
  // Step 5.  Update the status message to NVMe Spinner displayed
  //
  DisplayMessageWithTimeout (MSG_NVME_STARTED, DELAY_SPINNER_ON);

  //
  // Step 6.  Dismiss the NVMe spinner
  //
  EfiEventGroupSignal (&gNVMeEnableCompleteEventGroupGuid);
  DisplayMessageWithTimeout (MSG_NVME_DISMISSED, DELAY_BETWEEN_TEST);

  //
  // Step 7.  Start the Dfci Spinner (Uses General Spinner 1)
  //
  EfiEventGroupSignal (&gGeneralSpinner1StartEventGroupGuid);
  DisplayMessageWithTimeout (MSG_DFCI_START, DELAY_SPINNER_ON);

  //
  // Step 8.  Dismiss the General Purpose Spinner 1
  //
  EfiEventGroupSignal (&gGeneralSpinner1CompleteEventGroupGuid);
  DisplayMessageWithTimeout (MSG_DFCI_DISMISSED, DELAY_BETWEEN_TEST);

  //
  // Step 9. Start the four corners, delay a bit, and the start a center spinner
  //
  EfiEventGroupSignal (&gGeneralSpinner2StartEventGroupGuid);
  EfiEventGroupSignal (&gGeneralSpinner3StartEventGroupGuid);
  EfiEventGroupSignal (&gGeneralSpinner4StartEventGroupGuid);
  EfiEventGroupSignal (&gNVMeEnableStartEventGroupGuid);
  DisplayMessageWithTimeout (MSG_GENERAL_START, DELAY_NVME);

  EfiEventGroupSignal (&gGeneralSpinner1StartEventGroupGuid);
  DisplayMessageWithTimeout (MSG_GENERAL_MID, DELAY_BETWEEN_TEST);

  //
  // Step 10. Start stopping the GP spinners one at a time
  //
  DisplayMessageWithTimeout (MSG_GENERAL_STOP, DELAY_TO_STOP);
  EfiEventGroupSignal (&gNVMeEnableCompleteEventGroupGuid);
  MicroSecondDelay (DELAY_ONE_SECOND);

  EfiEventGroupSignal (&gGeneralSpinner4CompleteEventGroupGuid);
  MicroSecondDelay (DELAY_ONE_SECOND);

  EfiEventGroupSignal (&gGeneralSpinner3CompleteEventGroupGuid);
  MicroSecondDelay (DELAY_ONE_SECOND);

  EfiEventGroupSignal (&gGeneralSpinner2CompleteEventGroupGuid);
  MicroSecondDelay (DELAY_ONE_SECOND);

  EfiEventGroupSignal (&gGeneralSpinner1CompleteEventGroupGuid);
  MicroSecondDelay (DELAY_ONE_SECOND);

  //
  // Step 11.  Clear the screen, display complete message, and restore the OSK Icon
  //
  Status = gConOut->ClearScreen (gConOut);
  ASSERT_EFI_ERROR (Status);

  Status = gConOut->OutputString (gConOut, MSG_FINISHED);
  ASSERT_EFI_ERROR (Status);

  if (OSKProtocol != NULL) {
    OSKProtocol->ShowKeyboardIcon (OSKProtocol, TRUE);
  }

  return EFI_SUCCESS;
}
