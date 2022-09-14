/** @file
  This module tests HID Mouse Driver logic for conversion between
  HID report and absolute pointer protocol format for mouse movement

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/UnitTestLib.h>
#include "../HidMouseAbsolutePointer.h"

#define UNIT_TEST_NAME     "HID Mouse Host Test"
#define UNIT_TEST_VERSION  "0.1"

#define HID_MOUSE_ABSOLUTE_POINTER_DEV_BAD_SIGNATURE  SIGNATURE_32 ('B', 'A', 'D', 'S')

/**
 * @brief Really simple test to
 * confirm device can be initialized in the test framework.
 *
 * @param Context
 * @return UNIT_TEST_STATUS
 */
UNIT_TEST_STATUS
EFIAPI
TestInitializeDevFunc (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  HID_MOUSE_ABSOLUTE_POINTER_DEV  device;
  EFI_STATUS                      Status;

  Status = InitializeMouseDevice (&device);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);

  return UNIT_TEST_PASSED;
}

/**
 * @brief Test a valid SingleTouch HID report and its
 * translation into the absolute pointer state to be correct.
 *
 * @param Context
 * @return UNIT_TEST_STATUS
 */
UNIT_TEST_STATUS
EFIAPI
TestOnMouseReportFuncForSingleTouchValid (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  HID_MOUSE_ABSOLUTE_POINTER_DEV  device;
  SINGLETOUCH_HID_INPUT_BUFFER    SingleTouchInput;
  EFI_STATUS                      Status;

  ZeroMem (&device, sizeof (device));
  device.Signature = HID_MOUSE_ABSOLUTE_POINTER_DEV_SIGNATURE;
  Status           = InitializeMouseDevice (&device);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);

  ZeroMem (&SingleTouchInput, sizeof (SingleTouchInput));
  SingleTouchInput.CurrentX = 12;    // user selected safe values
  SingleTouchInput.CurrentY = 15;
  SingleTouchInput.Touch    = 1;

  // Make sure the hardcoded test data is valid.
  // If any of these fail then the test needs to be adjusted
  UT_ASSERT_TRUE (SingleTouchInput.CurrentX < device.Mode.AbsoluteMaxX);
  UT_ASSERT_TRUE (SingleTouchInput.CurrentX > device.Mode.AbsoluteMinX);
  UT_ASSERT_TRUE (SingleTouchInput.CurrentY < device.Mode.AbsoluteMaxY);
  UT_ASSERT_TRUE (SingleTouchInput.CurrentY > device.Mode.AbsoluteMinY);

  OnMouseReport (SingleTouch, (UINT8 *)&SingleTouchInput, sizeof (SingleTouchInput), &device);

  // Get result of single touch event and compare
  UT_ASSERT_EQUAL (device.State.CurrentX, 12);
  UT_ASSERT_EQUAL (device.State.CurrentY, 15);
  UT_ASSERT_EQUAL (device.State.CurrentZ, 0);
  UT_ASSERT_EQUAL (device.State.ActiveButtons, 1);

  return UNIT_TEST_PASSED;
}

/**
 * @brief Test an invalid SingleTouch HID report and its
 * translation into the absolute pointer state to be correct.
 *
 * X coordinate is larger than max
 * Expect that report was ignored and state is not changed.
 *
 *
 * @param Context
 * @return UNIT_TEST_STATUS
 */
UNIT_TEST_STATUS
EFIAPI
TestOnMouseReportFuncForSingleTouchToLarge (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  HID_MOUSE_ABSOLUTE_POINTER_DEV  device;
  EFI_STATUS                      Status;
  EFI_ABSOLUTE_POINTER_STATE      CachedState;
  SINGLETOUCH_HID_INPUT_BUFFER    SingleTouchInput;

  ZeroMem (&device, sizeof (device));
  device.Signature = HID_MOUSE_ABSOLUTE_POINTER_DEV_SIGNATURE;
  Status           = InitializeMouseDevice (&device);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);

  // copy the state so future comparison can be done.
  CopyMem (&CachedState, &device.State, sizeof (CachedState));

  ZeroMem (&SingleTouchInput, sizeof (SingleTouchInput));

  /////
  // TEST X coordinate
  //////
  SingleTouchInput.CurrentX = 1025;    // set this larger than MaxX thru inspection.
  SingleTouchInput.CurrentY = 15;
  SingleTouchInput.Touch    = 1;

  // Make sure the hardcoded test data is as expected.
  // If any of these fail then the test needs to be adjusted
  UT_ASSERT_TRUE (SingleTouchInput.CurrentX > device.Mode.AbsoluteMaxX);
  UT_ASSERT_TRUE (SingleTouchInput.CurrentY < device.Mode.AbsoluteMaxY);
  UT_ASSERT_TRUE (SingleTouchInput.CurrentY > device.Mode.AbsoluteMinY);

  OnMouseReport (SingleTouch, (UINT8 *)&SingleTouchInput, sizeof (SingleTouchInput), &device);

  // See that state didn't change.
  UT_ASSERT_MEM_EQUAL (&CachedState, &device.State, sizeof (CachedState));

  /////
  // TEST Y coordinate
  //////
  SingleTouchInput.CurrentX = 10;
  SingleTouchInput.CurrentY = 1025;
  SingleTouchInput.Touch    = 1;

  // Make sure the hardcoded test data is as expected.
  // If any of these fail then the test needs to be adjusted
  UT_ASSERT_TRUE (SingleTouchInput.CurrentX > device.Mode.AbsoluteMinX);
  UT_ASSERT_TRUE (SingleTouchInput.CurrentX < device.Mode.AbsoluteMaxX);
  UT_ASSERT_TRUE (SingleTouchInput.CurrentY > device.Mode.AbsoluteMaxY);

  OnMouseReport (SingleTouch, (UINT8 *)&SingleTouchInput, sizeof (SingleTouchInput), &device);

  // See that state didn't change.
  UT_ASSERT_MEM_EQUAL (&CachedState, &device.State, sizeof (CachedState));

  return UNIT_TEST_PASSED;
}

/**
 * @brief Test an invalid SingleTouch HID report and its
 * translation into the absolute pointer state to be correct.
 *
 * X coordinate is smaller than minX
 * Expect that report was ignored and state is not changed.
 *
 * @param Context
 * @return UNIT_TEST_STATUS
 */
UNIT_TEST_STATUS
EFIAPI
TestOnMouseReportFuncForSingleTouchToSmall (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  HID_MOUSE_ABSOLUTE_POINTER_DEV  device;
  EFI_STATUS                      Status;
  EFI_ABSOLUTE_POINTER_STATE      CachedState;
  SINGLETOUCH_HID_INPUT_BUFFER    SingleTouchInput;

  ZeroMem (&device, sizeof (device));
  device.Signature = HID_MOUSE_ABSOLUTE_POINTER_DEV_SIGNATURE;
  Status           = InitializeMouseDevice (&device);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);

  // copy the state so future comparison can be done.
  CopyMem (&CachedState, &device.State, sizeof (CachedState));

  ZeroMem (&SingleTouchInput, sizeof (SingleTouchInput));

  /////////
  // Test the X coordinate
  //
  device.Mode.AbsoluteMinX  = 1;   // change MinX
  SingleTouchInput.CurrentX = 0;   // set this smaller than the MinX.  Previously forced MinX to 1 so 0 should be safe.
  SingleTouchInput.CurrentY = 15;
  SingleTouchInput.Touch    = 1;

  // Make sure the hardcoded test data is as expected.
  // If any of these fail then the test needs to be adjusted
  UT_ASSERT_TRUE (SingleTouchInput.CurrentX < device.Mode.AbsoluteMinX);
  UT_ASSERT_TRUE (SingleTouchInput.CurrentY < device.Mode.AbsoluteMaxY);
  UT_ASSERT_TRUE (SingleTouchInput.CurrentY > device.Mode.AbsoluteMinY);

  OnMouseReport (SingleTouch, (UINT8 *)&SingleTouchInput, sizeof (SingleTouchInput), &device);

  // See that state didn't change.
  UT_ASSERT_MEM_EQUAL (&CachedState, &device.State, sizeof (CachedState));

  //////////
  // Test the Y coordinate
  ///////////
  device.Mode.AbsoluteMinY  = 1;   // change MinY
  SingleTouchInput.CurrentX = 10;
  SingleTouchInput.CurrentY = 0;
  SingleTouchInput.Touch    = 1;

  // Make sure the hardcoded test data is as expected.
  // If any of these fail then the test needs to be adjusted
  UT_ASSERT_TRUE (SingleTouchInput.CurrentX < device.Mode.AbsoluteMaxX);
  UT_ASSERT_TRUE (SingleTouchInput.CurrentX > device.Mode.AbsoluteMinX);
  UT_ASSERT_TRUE (SingleTouchInput.CurrentY < device.Mode.AbsoluteMinY);

  OnMouseReport (SingleTouch, (UINT8 *)&SingleTouchInput, sizeof (SingleTouchInput), &device);

  // See that state didn't change.
  UT_ASSERT_MEM_EQUAL (&CachedState, &device.State, sizeof (CachedState));

  return UNIT_TEST_PASSED;
}

/**
 * @brief Test an invalid SingleTouch HID report and its
 * translation into the absolute pointer state to be correct.
 *
 * The hid report is the incorrect length
 * Expect that report was ignored and state is not changed.
 *
 *
 * @param Context
 * @return UNIT_TEST_STATUS
 */
UNIT_TEST_STATUS
EFIAPI
TestOnMouseReportFuncForIncorrectSingleTouchReportLength (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  HID_MOUSE_ABSOLUTE_POINTER_DEV  device;
  EFI_STATUS                      Status;
  EFI_ABSOLUTE_POINTER_STATE      CachedState;
  SINGLETOUCH_HID_INPUT_BUFFER    SingleTouchInput;

  ZeroMem (&device, sizeof (device));
  device.Signature = HID_MOUSE_ABSOLUTE_POINTER_DEV_SIGNATURE;
  Status           = InitializeMouseDevice (&device);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);

  // copy the state so future comparison can be done.
  CopyMem (&CachedState, &device.State, sizeof (CachedState));

  ZeroMem (&SingleTouchInput, sizeof (SingleTouchInput));

  // configure valid report for test
  SingleTouchInput.CurrentX = 12;
  SingleTouchInput.CurrentY = 15;
  SingleTouchInput.Touch    = 1;

  // Make sure the hardcoded test data is as expected.
  // If any of these fail then the test needs to be adjusted
  UT_ASSERT_TRUE (SingleTouchInput.CurrentX < device.Mode.AbsoluteMaxX);
  UT_ASSERT_TRUE (SingleTouchInput.CurrentX > device.Mode.AbsoluteMinX);
  UT_ASSERT_TRUE (SingleTouchInput.CurrentY < device.Mode.AbsoluteMaxY);
  UT_ASSERT_TRUE (SingleTouchInput.CurrentY > device.Mode.AbsoluteMinY);

  // Test 1 - Report length too small
  OnMouseReport (SingleTouch, (UINT8 *)&SingleTouchInput, sizeof (SingleTouchInput) -1, &device);

  // See that state didn't change.
  UT_ASSERT_MEM_EQUAL (&CachedState, &device.State, sizeof (CachedState));

  // Test 2 - Report length too large
  OnMouseReport (SingleTouch, (UINT8 *)&SingleTouchInput, sizeof (SingleTouchInput) +1, &device);

  // See that state didn't change.
  UT_ASSERT_MEM_EQUAL (&CachedState, &device.State, sizeof (CachedState));

  return UNIT_TEST_PASSED;
}

/**
 * @brief Test invalid parameter of HidInputReportBuffer to OnMouseReport
 *
 * Expect that report was ignored and state is not changed.
 *
 *
 * @param Context
 * @return UNIT_TEST_STATUS
 */
UNIT_TEST_STATUS
EFIAPI
TestOnMouseReportFuncForInvalidParameterHidInputReportBuffer (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  HID_MOUSE_ABSOLUTE_POINTER_DEV  device;
  EFI_STATUS                      Status = EFI_SUCCESS;
  EFI_ABSOLUTE_POINTER_STATE      CachedState;

  ZeroMem (&device, sizeof (device));
  device.Signature = HID_MOUSE_ABSOLUTE_POINTER_DEV_SIGNATURE;
  Status           = InitializeMouseDevice (&device);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);

  // copy the state so future comparison can be done.
  CopyMem (&CachedState, &device.State, sizeof (CachedState));

  OnMouseReport (SingleTouch, NULL, sizeof (SINGLETOUCH_HID_INPUT_BUFFER), &device);

  // See that state didn't change.
  UT_ASSERT_MEM_EQUAL (&CachedState, &device.State, sizeof (CachedState));

  return UNIT_TEST_PASSED;
}

/**
 * @brief Test an invalid parameter of context to onMouseReport
 *
 * Since OnMouseReport is called by external driver, the Context parameter should
 * be confirmed to be:
 *   Not NULL
 *   A valid HID_MOUSE_ABSOLUTE_POINTER_DEV which will be checked by checking "signature" field.
 *
 * @param Context
 * @return UNIT_TEST_STATUS
 */
UNIT_TEST_STATUS
EFIAPI
TestOnMouseReportFuncForInvalidParameterContext (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  HID_MOUSE_ABSOLUTE_POINTER_DEV  device;
  EFI_STATUS                      Status;
  EFI_ABSOLUTE_POINTER_STATE      CachedState;
  SINGLETOUCH_HID_INPUT_BUFFER    SingleTouchInput;

  ZeroMem (&device, sizeof (device));
  device.Signature = HID_MOUSE_ABSOLUTE_POINTER_DEV_BAD_SIGNATURE;           // Use the bad signature here as it should be checked.
  Status           = InitializeMouseDevice (&device);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);

  // copy the state so future comparison can be done.
  CopyMem (&CachedState, &device.State, sizeof (CachedState));

  ZeroMem (&SingleTouchInput, sizeof (SingleTouchInput));

  SingleTouchInput.CurrentX = 12;
  SingleTouchInput.CurrentY = 15;
  SingleTouchInput.Touch    = 1;

  // Make sure the hardcoded test data is as expected.
  // If any of these fail then the test needs to be adjusted
  UT_ASSERT_TRUE (SingleTouchInput.CurrentX < device.Mode.AbsoluteMaxX);
  UT_ASSERT_TRUE (SingleTouchInput.CurrentX > device.Mode.AbsoluteMinX);
  UT_ASSERT_TRUE (SingleTouchInput.CurrentY < device.Mode.AbsoluteMaxY);
  UT_ASSERT_TRUE (SingleTouchInput.CurrentY > device.Mode.AbsoluteMinY);

  OnMouseReport (SingleTouch, (UINT8 *)&SingleTouchInput, sizeof (SingleTouchInput), NULL);  // Null context

  // See that state didn't change.
  UT_ASSERT_MEM_EQUAL (&CachedState, &device.State, sizeof (CachedState));

  OnMouseReport (SingleTouch, (UINT8 *)&SingleTouchInput, sizeof (SingleTouchInput), &device);  // device has invalid signature

  // See that state didn't change.
  UT_ASSERT_MEM_EQUAL (&CachedState, &device.State, sizeof (CachedState));

  return UNIT_TEST_PASSED;
}

/**
 * @brief Test the absolute pointer functionality for GetState and Reset
 *
 * @param Context
 * @return UNIT_TEST_STATUS
 */
UNIT_TEST_STATUS
EFIAPI
TestAbsolutePointerGetStateFunctionality (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  HID_MOUSE_ABSOLUTE_POINTER_DEV  device;
  EFI_STATUS                      Status = EFI_SUCCESS;

  // Initialize the device
  ZeroMem (&device, sizeof (device));

  device.Signature        = HID_MOUSE_ABSOLUTE_POINTER_DEV_SIGNATURE;
  device.HidMouseProtocol = NULL;    // hopefully no usage otherwise need mocking

  device.AbsolutePointerProtocol.GetState = GetMouseAbsolutePointerState;
  device.AbsolutePointerProtocol.Reset    = HidMouseAbsolutePointerReset;
  device.AbsolutePointerProtocol.Mode     = &device.Mode;

  Status = InitializeMouseDevice (&device);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);

  // Add a good HID report so the absolute pointer will have new state

  SINGLETOUCH_HID_INPUT_BUFFER  SingleTouchInput;

  ZeroMem (&SingleTouchInput, sizeof (SingleTouchInput));
  SingleTouchInput.CurrentX = 12;
  SingleTouchInput.CurrentY = 15;
  SingleTouchInput.Touch    = 1;

  OnMouseReport (SingleTouch, (UINT8 *)&SingleTouchInput, sizeof (SingleTouchInput), &device);

  // Should be able to get the state and it should be non-zero

  EFI_ABSOLUTE_POINTER_STATE  State;

  ZeroMem (&State, sizeof (State));
  Status = device.AbsolutePointerProtocol.GetState (&device.AbsolutePointerProtocol, &State);

  /////
  // Test that GetState worked and has expected data
  //////
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);

  UT_ASSERT_EQUAL (State.CurrentX, 12);
  UT_ASSERT_EQUAL (State.CurrentY, 15);
  UT_ASSERT_EQUAL (State.CurrentZ, 0);
  UT_ASSERT_EQUAL (State.ActiveButtons, 1);

  ////////////
  // (ABSPTR) TEST that if no additional single touch events occur and another get state is called
  //      it returns not ready and does not copy any state data
  ///////////

  // Clear state and expect that state will not have old data copied
  // and EFI_not_ready will be returned

  ZeroMem (&State, sizeof (State));
  Status = device.AbsolutePointerProtocol.GetState (&device.AbsolutePointerProtocol, &State);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_NOT_READY);

  UT_ASSERT_EQUAL (State.CurrentX, 0);
  UT_ASSERT_EQUAL (State.CurrentY, 0);
  UT_ASSERT_EQUAL (State.CurrentZ, 0);
  UT_ASSERT_EQUAL (State.ActiveButtons, 0);

  ///////
  // (ABSPTR) TEST that if valid data is set and then a reset is called before reading it the
  //     data is cleared and no valid data is available for get state
  ////////

  OnMouseReport (SingleTouch, (UINT8 *)&SingleTouchInput, sizeof (SingleTouchInput), &device);
  Status = device.AbsolutePointerProtocol.Reset (&device.AbsolutePointerProtocol, FALSE);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);

  Status = device.AbsolutePointerProtocol.GetState (&device.AbsolutePointerProtocol, &State);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_NOT_READY);

  UT_ASSERT_EQUAL (State.CurrentX, 0);
  UT_ASSERT_EQUAL (State.CurrentY, 0);
  UT_ASSERT_EQUAL (State.CurrentZ, 0);
  UT_ASSERT_EQUAL (State.ActiveButtons, 0);

  ///////
  /// Pass in Null for State and confirm invalid parameter returned
  ///////
  OnMouseReport (SingleTouch, (UINT8 *)&SingleTouchInput, sizeof (SingleTouchInput), &device);
  Status = device.AbsolutePointerProtocol.GetState (&device.AbsolutePointerProtocol, NULL);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);

  return UNIT_TEST_PASSED;
}

///////////////////////////////////////////////////////////////////////////////
// BOOT MOUSE TESTS
///////////////////////////////////////////////////////////////////////////////

/**
 * @brief Test a valid BootMouse HID report and its
 * translation into the absolute pointer state to be correct.
 *
 * Report has No Z value
 *
 * @param Context
 * @return UNIT_TEST_STATUS
 */
UNIT_TEST_STATUS
EFIAPI
TestOnMouseReportFuncForBootMouseValidNoZ (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  HID_MOUSE_ABSOLUTE_POINTER_DEV  device;
  MOUSE_HID_INPUT_BUFFER          Input;
  EFI_STATUS                      Status;
  EFI_ABSOLUTE_POINTER_STATE      Before;

  ZeroMem (&device, sizeof (device));
  device.Signature = HID_MOUSE_ABSOLUTE_POINTER_DEV_SIGNATURE;
  Status           = InitializeMouseDevice (&device);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);

  // copy the state since boot mouse is displacement based the
  // state is always changing based on current.
  CopyMem (&Before, &device.State, sizeof (Before));

  ZeroMem (&Input, sizeof (Input));
  Input.XDisplacement = 13;
  Input.YDisplacement = 30;
  Input.Button1       = 1; // button pressed

  OnMouseReport (BootMouse, (UINT8 *)&Input, sizeof (Input)-1, &device);  // subtract 1 since we want to have report with no Z displacement

  // Get result of single touch event and compare
  UT_ASSERT_EQUAL (device.State.CurrentX, Before.CurrentX + Input.XDisplacement);
  UT_ASSERT_EQUAL (device.State.CurrentY, Before.CurrentY + Input.YDisplacement);
  UT_ASSERT_EQUAL (device.State.CurrentZ, 0);
  UT_ASSERT_EQUAL (device.State.ActiveButtons, 1);

  // copy the state since boot mouse is displacement based the
  // state is always changing based on current.
  CopyMem (&Before, &device.State, sizeof (Before));

  ZeroMem (&Input, sizeof (Input));
  Input.XDisplacement = -20;
  Input.YDisplacement = -52;

  OnMouseReport (BootMouse, (UINT8 *)&Input, sizeof (Input)-1, &device);  // subtract 1 since we want to have report with no Z displacement

  // Get result of single touch event and compare
  UT_ASSERT_EQUAL (device.State.CurrentX, Before.CurrentX + Input.XDisplacement);
  UT_ASSERT_EQUAL (device.State.CurrentY, Before.CurrentY + Input.YDisplacement);
  UT_ASSERT_EQUAL (device.State.CurrentZ, 0);
  UT_ASSERT_EQUAL (device.State.ActiveButtons, 0);

  return UNIT_TEST_PASSED;
}

/**
 * @brief Test a valid BootMouse HID report and its
 * translation into the absolute pointer state to be correct.
 *
 * Report has a valid Z value
 *
 * @param Context
 * @return UNIT_TEST_STATUS
 */
UNIT_TEST_STATUS
EFIAPI
TestOnMouseReportFuncForBootMouseValidWithZ (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  HID_MOUSE_ABSOLUTE_POINTER_DEV  device;
  MOUSE_HID_INPUT_BUFFER          Input;
  EFI_STATUS                      Status;
  EFI_ABSOLUTE_POINTER_STATE      Before;

  ZeroMem (&device, sizeof (device));
  device.Signature = HID_MOUSE_ABSOLUTE_POINTER_DEV_SIGNATURE;
  Status           = InitializeMouseDevice (&device);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);

  // set the mode Z max to 1024 to allow Z
  // TODO - Look to make this based on HidProtocol Info
  device.Mode.AbsoluteMaxZ = 1024;

  // copy the state since boot mouse is displacement based the
  // state is always changing based on current.
  CopyMem (&Before, &device.State, sizeof (Before));

  ZeroMem (&Input, sizeof (Input));
  Input.XDisplacement = 13;
  Input.YDisplacement = 30;
  Input.ZDisplacement = 4;
  Input.Button1       = 1; // button pressed

  OnMouseReport (BootMouse, (UINT8 *)&Input, sizeof (Input), &device);

  // Get result of single touch event and compare
  UT_ASSERT_EQUAL (device.State.CurrentX, Before.CurrentX + Input.XDisplacement);
  UT_ASSERT_EQUAL (device.State.CurrentY, Before.CurrentY + Input.YDisplacement);
  UT_ASSERT_EQUAL (device.State.CurrentZ, Before.CurrentZ + Input.ZDisplacement);
  UT_ASSERT_EQUAL (device.State.ActiveButtons, 1);

  // copy the state since boot mouse is displacement based the
  // state is always changing based on current.
  CopyMem (&Before, &device.State, sizeof (Before));

  ZeroMem (&Input, sizeof (Input));
  Input.XDisplacement = -20;
  Input.YDisplacement = -52;
  Input.ZDisplacement = -2;
  Input.Button2       = 1;
  Input.Button3       = 1;

  OnMouseReport (BootMouse, (UINT8 *)&Input, sizeof (Input), &device);

  // Get result of single touch event and compare
  UT_ASSERT_EQUAL (device.State.CurrentX, Before.CurrentX + Input.XDisplacement);
  UT_ASSERT_EQUAL (device.State.CurrentY, Before.CurrentY + Input.YDisplacement);
  UT_ASSERT_EQUAL (device.State.CurrentZ, Before.CurrentZ + Input.ZDisplacement);
  UT_ASSERT_EQUAL (device.State.ActiveButtons, 6); // bit 1 | bit 2

  return UNIT_TEST_PASSED;
}

/**
 * @brief Test a valid BootMouse HID report and its
 * translation into the absolute pointer state to be correct.
 *
 * Report has a valid Z value plus carries extra device
 * specific data as allowed by BootMouse report format
 *
 * @param Context
 * @return UNIT_TEST_STATUS
 */
UNIT_TEST_STATUS
EFIAPI
TestOnMouseReportFuncForBootMouseValidWithZAndExtra (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  HID_MOUSE_ABSOLUTE_POINTER_DEV  device;
  MOUSE_HID_INPUT_BUFFER          Input;
  EFI_STATUS                      Status;
  EFI_ABSOLUTE_POINTER_STATE      Before;

  ZeroMem (&device, sizeof (device));
  device.Signature = HID_MOUSE_ABSOLUTE_POINTER_DEV_SIGNATURE;
  Status           = InitializeMouseDevice (&device);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);

  // set the mode Z max to 1024 to allow Z
  // TODO - Look to make this based on HidProtocol Info
  device.Mode.AbsoluteMaxZ = 1024;

  // copy the state since boot mouse is displacement based the
  // state is always changing based on current.
  CopyMem (&Before, &device.State, sizeof (Before));

  ZeroMem (&Input, sizeof (Input));
  Input.XDisplacement = 75;
  Input.YDisplacement = 212;
  Input.ZDisplacement = 17;

  OnMouseReport (BootMouse, (UINT8 *)&Input, sizeof (Input) + 3, &device); // just add extra bytes to reported length since it should never be looked at

  // Get result of single touch event and compare
  UT_ASSERT_EQUAL (device.State.CurrentX, Before.CurrentX + Input.XDisplacement);
  UT_ASSERT_EQUAL (device.State.CurrentY, Before.CurrentY + Input.YDisplacement);
  UT_ASSERT_EQUAL (device.State.CurrentZ, Before.CurrentZ + Input.ZDisplacement);
  UT_ASSERT_EQUAL (device.State.ActiveButtons, 0);

  // copy the state since boot mouse is displacement based the
  // state is always changing based on current.
  CopyMem (&Before, &device.State, sizeof (Before));

  ZeroMem (&Input, sizeof (Input));
  Input.XDisplacement = -20;
  Input.YDisplacement = -52;
  Input.ZDisplacement = -2;
  Input.Button3       = 1;

  OnMouseReport (BootMouse, (UINT8 *)&Input, sizeof (Input) + 5, &device);  // just add extra bytes to reported length since it should never be looked at

  // Get result of single touch event and compare
  UT_ASSERT_EQUAL (device.State.CurrentX, Before.CurrentX + Input.XDisplacement);
  UT_ASSERT_EQUAL (device.State.CurrentY, Before.CurrentY + Input.YDisplacement);
  UT_ASSERT_EQUAL (device.State.CurrentZ, Before.CurrentZ + Input.ZDisplacement);
  UT_ASSERT_EQUAL (device.State.ActiveButtons, 4);

  return UNIT_TEST_PASSED;
}

/**
 * @brief Test an invalid BootMouse HID report and its
 * translation into the absolute pointer state to be correct.
 *
 * report length is shorter than allowed. State should not change.
 *
 * @param Context
 * @return UNIT_TEST_STATUS
 */
UNIT_TEST_STATUS
EFIAPI
TestOnMouseReportFuncForBootMouseInvalidLength (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  HID_MOUSE_ABSOLUTE_POINTER_DEV  device;
  MOUSE_HID_INPUT_BUFFER          Input;
  EFI_STATUS                      Status;
  EFI_ABSOLUTE_POINTER_STATE      Before;

  ZeroMem (&device, sizeof (device));
  device.Signature = HID_MOUSE_ABSOLUTE_POINTER_DEV_SIGNATURE;
  Status           = InitializeMouseDevice (&device);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);

  // copy the state since boot mouse is displacement based the
  // state is always changing based on current.
  CopyMem (&Before, &device.State, sizeof (Before));

  ZeroMem (&Input, sizeof (Input));
  Input.XDisplacement = 13;
  Input.YDisplacement = 30;
  Input.ZDisplacement = 4;
  Input.Button1       = 1; // button pressed

  OnMouseReport (BootMouse, (UINT8 *)&Input, 2, &device);  // length 2 is invalid.

  // Get result of single touch event and compare
  UT_ASSERT_EQUAL (device.State.CurrentX, Before.CurrentX);
  UT_ASSERT_EQUAL (device.State.CurrentY, Before.CurrentY);
  UT_ASSERT_EQUAL (device.State.CurrentZ, Before.CurrentZ);
  UT_ASSERT_EQUAL (device.State.ActiveButtons, Before.ActiveButtons);

  OnMouseReport (BootMouse, (UINT8 *)&Input, 1, &device);  // length 1 is invalid.

  // Get result of single touch event and compare
  UT_ASSERT_EQUAL (device.State.CurrentX, Before.CurrentX);
  UT_ASSERT_EQUAL (device.State.CurrentY, Before.CurrentY);
  UT_ASSERT_EQUAL (device.State.CurrentZ, Before.CurrentZ);
  UT_ASSERT_EQUAL (device.State.ActiveButtons, Before.ActiveButtons);

  OnMouseReport (BootMouse, (UINT8 *)&Input, 0, &device);    // length 0 is invalid.

  // Get result of single touch event and compare
  UT_ASSERT_EQUAL (device.State.CurrentX, Before.CurrentX);
  UT_ASSERT_EQUAL (device.State.CurrentY, Before.CurrentY);
  UT_ASSERT_EQUAL (device.State.CurrentZ, Before.CurrentZ);
  UT_ASSERT_EQUAL (device.State.ActiveButtons, Before.ActiveButtons);

  return UNIT_TEST_PASSED;
}

/**
 * @brief Test a valid BootMouse HID report and its
 * translation into the absolute pointer state to be correct.
 *
 * Reports with displacements that would exceed the bounds.
 * Make sure state stays within the bounds
 *
 * @param Context
 * @return UNIT_TEST_STATUS
 */
UNIT_TEST_STATUS
EFIAPI
TestOnMouseReportFuncForBootMouseValidBoundsCheck (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  HID_MOUSE_ABSOLUTE_POINTER_DEV  device;
  MOUSE_HID_INPUT_BUFFER          Input;
  EFI_STATUS                      Status;

  ZeroMem (&device, sizeof (device));
  device.Signature = HID_MOUSE_ABSOLUTE_POINTER_DEV_SIGNATURE;
  Status           = InitializeMouseDevice (&device);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);

  // set the mode Z max to 1024 to allow Z
  // TODO - Look to make this based on HidProtocol Info
  device.Mode.AbsoluteMaxZ = 1024;

  // Assume all modes have 0 to 1024 range.

  // Set Current to within 127 of edge
  device.State.CurrentX = 1024-127;
  device.State.CurrentY = 1020;  // allow to go past
  device.State.CurrentZ = 1000;

  // Test 1 - Go to max X using max displacement; Go beyond max Y; Go to max Z
  ZeroMem (&Input, sizeof (Input));
  Input.XDisplacement = 127;
  Input.YDisplacement = 120;
  Input.ZDisplacement = 24;

  OnMouseReport (BootMouse, (UINT8 *)&Input, sizeof (Input), &device);

  // Get result of single touch event and compare
  UT_ASSERT_EQUAL (device.State.CurrentX, 1024);
  UT_ASSERT_EQUAL (device.State.CurrentY, 1024);
  UT_ASSERT_EQUAL (device.State.CurrentZ, 1024);
  UT_ASSERT_EQUAL (device.State.ActiveButtons, 0);

  // Test 2 - now that at max make sure more positive displacement doesn't cause increase
  ZeroMem (&Input, sizeof (Input));
  Input.XDisplacement = 20;
  Input.YDisplacement = 1;
  Input.ZDisplacement = 127;
  Input.Button2       = 1;

  OnMouseReport (BootMouse, (UINT8 *)&Input, sizeof (Input), &device);

  // Get result of single touch event and compare
  UT_ASSERT_EQUAL (device.State.CurrentX, 1024);
  UT_ASSERT_EQUAL (device.State.CurrentY, 1024);
  UT_ASSERT_EQUAL (device.State.CurrentZ, 1024);
  UT_ASSERT_EQUAL (device.State.ActiveButtons, 2); // bit 1

  // Test 3 - try to go to or below zero
  device.State.CurrentX = 2;
  device.State.CurrentY = 5;
  device.State.CurrentZ = 127;

  ZeroMem (&Input, sizeof (Input));
  Input.XDisplacement = -127;
  Input.YDisplacement = -6;
  Input.ZDisplacement = -127;

  OnMouseReport (BootMouse, (UINT8 *)&Input, sizeof (Input), &device);

  // Get result of single touch event and compare
  UT_ASSERT_EQUAL (device.State.CurrentX, 0);
  UT_ASSERT_EQUAL (device.State.CurrentY, 0);
  UT_ASSERT_EQUAL (device.State.CurrentZ, 0);
  UT_ASSERT_EQUAL (device.State.ActiveButtons, 0);

  // Test 4 - now that at min try more negative displacement
  ZeroMem (&Input, sizeof (Input));
  Input.XDisplacement = -5;
  Input.YDisplacement = -127;
  Input.ZDisplacement = -1;

  OnMouseReport (BootMouse, (UINT8 *)&Input, sizeof (Input), &device);

  // Get result of single touch event and compare
  UT_ASSERT_EQUAL (device.State.CurrentX, 0);
  UT_ASSERT_EQUAL (device.State.CurrentY, 0);
  UT_ASSERT_EQUAL (device.State.CurrentZ, 0);
  UT_ASSERT_EQUAL (device.State.ActiveButtons, 0);

  return UNIT_TEST_PASSED;
}

/**
  Initialize the unit test framework, suite, and unit tests for the
  unit tests and run the unit tests.

  @retval  EFI_SUCCESS           All test cases were dispatched.
  @retval  EFI_OUT_OF_RESOURCES  There are not enough resources available to
                                 initialize the unit tests.
**/
EFI_STATUS
EFIAPI
UefiTestMain (
  VOID
  )
{
  EFI_STATUS                  Status;
  UNIT_TEST_FRAMEWORK_HANDLE  Framework;
  UNIT_TEST_SUITE_HANDLE      HidMouseMiscSuiteHandle; // basic functional tests
  UNIT_TEST_SUITE_HANDLE      AbsPtrSuiteHandle;       // tests related to absolute pointer interface
  UNIT_TEST_SUITE_HANDLE      SimpleTouchSuiteHandle;  // tests using SimpleTouch hid report
  UNIT_TEST_SUITE_HANDLE      BootMouseSuiteHandle;    // tests using BootMouse hid report

  Framework = NULL;

  DEBUG ((DEBUG_INFO, "%a v%a\n", UNIT_TEST_NAME, UNIT_TEST_VERSION));

  //
  // Start setting up the test framework for running the tests.
  //
  Status = InitUnitTestFramework (&Framework, UNIT_TEST_NAME, gEfiCallerBaseName, UNIT_TEST_VERSION);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in InitUnitTestFramework. Status = %r\n", Status));
    goto EXIT;
  }

  //
  // Create a suite
  //
  Status = CreateUnitTestSuite (&HidMouseMiscSuiteHandle, Framework, "HidMouseAbsolutePointerDxe basic tests", "HidMouseAbsolutePointerDxe.Misc", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for HidMouseMiscSuiteHandle\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  //
  // Register Tests
  //
  AddTestCase (HidMouseMiscSuiteHandle, "Initialize the Mouse Dev", "InitMouseDev", TestInitializeDevFunc, NULL, NULL, NULL);
  AddTestCase (HidMouseMiscSuiteHandle, "OnMouseReport Func Invalid Parameter Context", "OnMouseReport.InvalidParameter.Context", TestOnMouseReportFuncForInvalidParameterContext, NULL, NULL, NULL);
  AddTestCase (HidMouseMiscSuiteHandle, "OnMouseReport Func Invalid Parameter HidInputReportBuffer", "OnMouseReport.InvalidParameter.HidInputReportBuffer", TestOnMouseReportFuncForInvalidParameterHidInputReportBuffer, NULL, NULL, NULL);

  //
  // Create a suite
  //
  Status = CreateUnitTestSuite (&AbsPtrSuiteHandle, Framework, "HidMouseAbsolutePointerDxe absolute pointer protocol tests", "HidMouseAbsolutePointerDxe.AbsPtrProtocol", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for AbsPtrSuiteHandle\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  //
  // Register Tests
  //
  AddTestCase (AbsPtrSuiteHandle, "Test Absolute Pointer GetState function", "HidMouse.AbsolutePointer.GetState", TestAbsolutePointerGetStateFunctionality, NULL, NULL, NULL);

  //
  // Create a suite
  //
  Status = CreateUnitTestSuite (&SimpleTouchSuiteHandle, Framework, "HidMouseAbsolutePointerDxe SimpleTouch HID Report", "HidMouseAbsolutePointerDxe.HID.SimpleTouch", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for SimpleTouchSuiteHandle\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  //
  // Register Tests
  //
  AddTestCase (SimpleTouchSuiteHandle, "Process a valid SingleTouch HID Report", "ValidReport", TestOnMouseReportFuncForSingleTouchValid, NULL, NULL, NULL);
  AddTestCase (SimpleTouchSuiteHandle, "Process a SingleTouch HID Report with coordinates larger than max", "CoordinateLargerThanMax", TestOnMouseReportFuncForSingleTouchToLarge, NULL, NULL, NULL);
  AddTestCase (SimpleTouchSuiteHandle, "Process a SingleTouch HID Report with coordinates smaller than min", "CoordinateSmallerThanMin", TestOnMouseReportFuncForSingleTouchToSmall, NULL, NULL, NULL);
  AddTestCase (SimpleTouchSuiteHandle, "Process a SingleTouch HID Report incorrect length", "HidInputReportBufferSizeIncorrect", TestOnMouseReportFuncForIncorrectSingleTouchReportLength, NULL, NULL, NULL);

  //
  // Create a suite
  //
  Status = CreateUnitTestSuite (&BootMouseSuiteHandle, Framework, "HidMouseAbsolutePointerDxe Boot Mouse HID Report", "HidMouseAbsolutePointerDxe.HID.BootMouse", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for BootMouseSuiteHandle\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  //
  // Register Tests
  //
  AddTestCase (BootMouseSuiteHandle, "Process a valid BootMouse HID Report with no z field", "ValidReport.NoZ", TestOnMouseReportFuncForBootMouseValidNoZ, NULL, NULL, NULL);
  AddTestCase (BootMouseSuiteHandle, "Process a valid BootMouse HID Report with z field", "ValidReport.WithZ", TestOnMouseReportFuncForBootMouseValidWithZ, NULL, NULL, NULL);
  AddTestCase (BootMouseSuiteHandle, "Process a valid BootMouse HID Report with additional report data", "ValidReport.WithAdditionalData", TestOnMouseReportFuncForBootMouseValidWithZAndExtra, NULL, NULL, NULL);
  AddTestCase (BootMouseSuiteHandle, "Process a BootMouse HID Report with incorrect length", "HidInputReportBufferSizeIncorrect", TestOnMouseReportFuncForBootMouseInvalidLength, NULL, NULL, NULL);
  AddTestCase (BootMouseSuiteHandle, "Process a set of BootMouse HID Reports that try to exceed min and max", "MinMaxCoordinate", TestOnMouseReportFuncForBootMouseValidBoundsCheck, NULL, NULL, NULL);

  //
  // Execute the tests.
  //
  Status = RunAllTestSuites (Framework);

EXIT:
  if (Framework) {
    FreeUnitTestFramework (Framework);
  }

  return Status;
}

/**
  Standard POSIX C entry point for host based unit test execution.
**/
int
main (
  int   argc,
  char  *argv[]
  )
{
  return UefiTestMain ();
}
