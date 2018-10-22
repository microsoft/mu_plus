/** @file
Implementation for handling user input from the User Interfaces.

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2015 - 2018, Microsoft Corporation.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "FormDisplay.h"

/**
  Get maximum and minimum info from this opcode.

  @param  OpCode            Pointer to the current input opcode.
  @param  Minimum           The minimum size info for this opcode.
  @param  Maximum           The maximum size info for this opcode.

**/
VOID
GetFieldFromOp (
  IN   EFI_IFR_OP_HEADER       *OpCode,
  OUT  UINTN                   *Minimum,
  OUT  UINTN                   *Maximum
  )
{
  EFI_IFR_STRING    *StringOp;
  EFI_IFR_PASSWORD  *PasswordOp;
  if (OpCode->OpCode == EFI_IFR_STRING_OP) {
    StringOp = (EFI_IFR_STRING *) OpCode;
    *Minimum = StringOp->MinSize;
    *Maximum = StringOp->MaxSize;
  } else if (OpCode->OpCode == EFI_IFR_PASSWORD_OP) {
    PasswordOp = (EFI_IFR_PASSWORD *) OpCode;
    *Minimum = PasswordOp->MinSize;
    *Maximum = PasswordOp->MaxSize;
  } else {
    *Minimum = 0;
    *Maximum = 0;
  }
}

/**
  Adjust the value to the correct one. Rules follow the sample:
  like:  Year change:  2012.02.29 -> 2013.02.29 -> 2013.02.01
         Month change: 2013.03.29 -> 2013.02.29 -> 2013.02.28

  @param  QuestionValue     Pointer to current question.
  @param  Sequence          The sequence of the field in the question.
**/
VOID
AdjustQuestionValue (
  IN  EFI_HII_VALUE           *QuestionValue,
  IN  UINT8                   Sequence
  )
{
  UINT8     Month;
  UINT16    Year;
  UINT8     Maximum;
  UINT8     Minimum;

  Month   = QuestionValue->Value.date.Month;
  Year    = QuestionValue->Value.date.Year;
  Minimum = 1;

  switch (Month) {
  case 2:
    if ((Year % 4) == 0 && ((Year % 100) != 0 || (Year % 400) == 0)) {
      Maximum = 29;
    } else {
      Maximum = 28;
    }
    break;
  case 4:
  case 6:
  case 9:
  case 11:
    Maximum = 30;
    break;
  default:
    Maximum = 31;
    break;
  }

  //
  // Change the month area.
  //
  if (Sequence == 0) {
    if (QuestionValue->Value.date.Day > Maximum) {
      QuestionValue->Value.date.Day = Maximum;
    }
  }

  //
  // Change the Year area.
  //
  if (Sequence == 2) {
    if (QuestionValue->Value.date.Day > Maximum) {
      QuestionValue->Value.date.Day = Minimum;
    }
  }
}

/**
  Get field info from numeric opcode.

  @param  OpCode            Pointer to the current input opcode.
  @param  IntInput          Whether question shows with EFI_IFR_DISPLAY_INT_DEC type.
  @param  QuestionValue     Input question value, with EFI_HII_VALUE type.
  @param  Value             Return question value, always return UINT64 type.
  @param  Minimum           The minimum size info for this opcode.
  @param  Maximum           The maximum size info for this opcode.
  @param  Step              The step size info for this opcode.
  @param  StorageWidth      The storage width info for this opcode.

**/
VOID
GetValueFromNum (
  IN  EFI_IFR_OP_HEADER     *OpCode,
  IN  BOOLEAN               IntInput,
  IN  EFI_HII_VALUE         *QuestionValue,
  OUT UINT64                *Value,
  OUT UINT64                *Minimum,
  OUT UINT64                *Maximum,
  OUT UINT64                *Step,
  OUT UINT16                *StorageWidth
)
{
  EFI_IFR_NUMERIC       *NumericOp;

  NumericOp = (EFI_IFR_NUMERIC *) OpCode;

  switch (NumericOp->Flags & EFI_IFR_NUMERIC_SIZE) {
  case EFI_IFR_NUMERIC_SIZE_1:
    if (IntInput) {
      *Minimum = (INT64) (INT8) NumericOp->data.u8.MinValue;
      *Maximum = (INT64) (INT8) NumericOp->data.u8.MaxValue;
      *Value   = (INT64) (INT8) QuestionValue->Value.u8;
    } else {
      *Minimum = NumericOp->data.u8.MinValue;
      *Maximum = NumericOp->data.u8.MaxValue;
      *Value   = QuestionValue->Value.u8;
    }
    *Step    = NumericOp->data.u8.Step;
    *StorageWidth = (UINT16) sizeof (UINT8);
    break;

  case EFI_IFR_NUMERIC_SIZE_2:
    if (IntInput) {
      *Minimum = (INT64) (INT16) NumericOp->data.u16.MinValue;
      *Maximum = (INT64) (INT16) NumericOp->data.u16.MaxValue;
      *Value   = (INT64) (INT16) QuestionValue->Value.u16;
    } else {
      *Minimum = NumericOp->data.u16.MinValue;
      *Maximum = NumericOp->data.u16.MaxValue;
      *Value   = QuestionValue->Value.u16;
    }
    *Step    = NumericOp->data.u16.Step;
    *StorageWidth = (UINT16) sizeof (UINT16);
    break;

  case EFI_IFR_NUMERIC_SIZE_4:
    if (IntInput) {
      *Minimum = (INT64) (INT32) NumericOp->data.u32.MinValue;
      *Maximum = (INT64) (INT32) NumericOp->data.u32.MaxValue;
      *Value   = (INT64) (INT32) QuestionValue->Value.u32;
    } else {
      *Minimum = NumericOp->data.u32.MinValue;
      *Maximum = NumericOp->data.u32.MaxValue;
      *Value   = QuestionValue->Value.u32;
    }
    *Step    = NumericOp->data.u32.Step;
    *StorageWidth = (UINT16) sizeof (UINT32);
    break;

  case EFI_IFR_NUMERIC_SIZE_8:
    if (IntInput) {
      *Minimum = (INT64) NumericOp->data.u64.MinValue;
      *Maximum = (INT64) NumericOp->data.u64.MaxValue;
      *Value   = (INT64) QuestionValue->Value.u64;
    } else {
      *Minimum = NumericOp->data.u64.MinValue;
      *Maximum = NumericOp->data.u64.MaxValue;
      *Value   = QuestionValue->Value.u64;
    }
    *Step    = NumericOp->data.u64.Step;
    *StorageWidth = (UINT16) sizeof (UINT64);
    break;

  default:
    break;
  }

  if (*Maximum == 0) {
    *Maximum = (UINT64) -1;
  }
}

/**
  This routine reads a numeric value from the user input.

  @param  MenuOption        Pointer to the current input menu.

  @retval EFI_SUCCESS       If numerical input is read successfully
  @retval EFI_DEVICE_ERROR  If operation fails

**/
EFI_STATUS
GetNumericInput (
  IN  UI_MENU_OPTION              *MenuOption
  )
{
    ASSERT(FALSE );
    DEBUG((DEBUG_ERROR," GetNumericInput is not supproted\n"));
    return EFI_UNSUPPORTED;
}
/**
  Adjust option order base on the question value.

  @param  Question           Pointer to current question.
  @param  PopUpMenuLines     The line number of the pop up menu.

  @retval EFI_SUCCESS       If Option input is processed successfully
  @retval EFI_DEVICE_ERROR  If operation fails

**/
EFI_STATUS
AdjustOptionOrder (
  IN  FORM_DISPLAY_ENGINE_STATEMENT  *Question,
  OUT UINTN                          *PopUpMenuLines
  )
{
  UINTN                   Index;
  EFI_IFR_ORDERED_LIST    *OrderList;
  UINT8                   *ValueArray;
  UINT8                   ValueType;
  LIST_ENTRY              *Link;
  DISPLAY_QUESTION_OPTION *OneOfOption;
  EFI_HII_VALUE           *HiiValueArray;

  Link        = GetFirstNode (&Question->OptionListHead);
  OneOfOption = DISPLAY_QUESTION_OPTION_FROM_LINK (Link);
  ValueArray  = Question->CurrentValue.Buffer;
  ValueType   =  OneOfOption->OptionOpCode->Type;
  OrderList   = (EFI_IFR_ORDERED_LIST *) Question->OpCode;

  for (Index = 0; Index < OrderList->MaxContainers; Index++) {
    if (GetArrayData (ValueArray, ValueType, Index) == 0) {
      break;
    }
  }

  *PopUpMenuLines = Index;

  //
  // Prepare HiiValue array
  //
  HiiValueArray = AllocateZeroPool (*PopUpMenuLines * sizeof (EFI_HII_VALUE));
  ASSERT (HiiValueArray != NULL);

  for (Index = 0; Index < *PopUpMenuLines; Index++) {
    HiiValueArray[Index].Type = ValueType;
    HiiValueArray[Index].Value.u64 = GetArrayData (ValueArray, ValueType, Index);
  }

  for (Index = 0; Index < *PopUpMenuLines; Index++) {
    OneOfOption = ValueToOption (Question, &HiiValueArray[*PopUpMenuLines - Index - 1]);
    if (OneOfOption == NULL) {
      return EFI_NOT_FOUND;
    }

    RemoveEntryList (&OneOfOption->Link);

    //
    // Insert to head.
    //
    InsertHeadList (&Question->OptionListHead, &OneOfOption->Link);
  }

  FreePool (HiiValueArray);

  return EFI_SUCCESS;
}

/**
  Base on the type to compare the value.

  @param  Value1                The first value need to compare.
  @param  Value2                The second value need to compare.
  @param  Type                  The value type for above two values.

  @retval TRUE                  The two value are same.
  @retval FALSE                 The two value are different.

**/
BOOLEAN
IsValuesEqual (
  IN EFI_IFR_TYPE_VALUE *Value1,
  IN EFI_IFR_TYPE_VALUE *Value2,
  IN UINT8              Type
  )
{
  switch (Type) {
  case EFI_IFR_TYPE_BOOLEAN:
  case EFI_IFR_TYPE_NUM_SIZE_8:
    return (BOOLEAN) (Value1->u8 == Value2->u8);

  case EFI_IFR_TYPE_NUM_SIZE_16:
    return (BOOLEAN) (Value1->u16 == Value2->u16);

  case EFI_IFR_TYPE_NUM_SIZE_32:
    return (BOOLEAN) (Value1->u32 == Value2->u32);

  case EFI_IFR_TYPE_NUM_SIZE_64:
    return (BOOLEAN) (Value1->u64 == Value2->u64);

  default:
    ASSERT (FALSE);
    return FALSE;
  }
}

/**
  Base on the type to set the value.

  @param  Dest                  The dest value.
  @param  Source                The source value.
  @param  Type                  The value type for above two values.

**/
VOID
SetValuesByType (
  OUT EFI_IFR_TYPE_VALUE *Dest,
  IN  EFI_IFR_TYPE_VALUE *Source,
  IN  UINT8              Type
  )
{
  switch (Type) {
  case EFI_IFR_TYPE_BOOLEAN:
    Dest->b = Source->b;
    break;

  case EFI_IFR_TYPE_NUM_SIZE_8:
    Dest->u8 = Source->u8;
    break;

  case EFI_IFR_TYPE_NUM_SIZE_16:
    Dest->u16 = Source->u16;
    break;

  case EFI_IFR_TYPE_NUM_SIZE_32:
    Dest->u32 = Source->u32;
    break;

  case EFI_IFR_TYPE_NUM_SIZE_64:
    Dest->u64 = Source->u64;
    break;

  default:
    ASSERT (FALSE);
    break;
  }
}

/**
  Get selection for OneOf and OrderedList (Left/Right will be ignored).

  @param  MenuOption        Pointer to the current input menu.

  @retval EFI_SUCCESS       If Option input is processed successfully
  @retval EFI_DEVICE_ERROR  If operation fails

**/
EFI_STATUS
GetSelectionInputPopUp (
  IN  UI_MENU_OPTION              *MenuOption
  )
{
    ASSERT(FALSE );
    DEBUG((DEBUG_ERROR,"GetSelectionInputPopUp is not supported\n"));
    return EFI_UNSUPPORTED;
}

