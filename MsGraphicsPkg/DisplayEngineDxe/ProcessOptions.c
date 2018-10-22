/** @file
Implementation for handling the User Interface option processing.


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

#define MAX_TIME_OUT_LEN  0x10

/**
  Concatenate a narrow string to another string.

  @param Destination The destination string.
  @param DestMax     The Max length of destination string.
  @param Source      The source string. The string to be concatenated.
                     to the end of Destination.

**/
VOID
NewStrCat (
  IN OUT CHAR16               *Destination,
  IN     UINTN                DestMax,
  IN     CHAR16               *Source
  )
{
  UINTN Length;

  for (Length = 0; Destination[Length] != 0; Length++)
    ;

  //
  // We now have the length of the original string
  // We can safely assume for now that we are concatenating a narrow value to this string.
  // For instance, the string is "XYZ" and cat'ing ">"
  // If this assumption changes, we need to make this routine a bit more complex
  //
  Destination[Length] = NARROW_CHAR;
  Length++;

  StrCpyS (Destination + Length, DestMax - Length, Source);
}

/**
  Get UINT64 type value.

  @param  Value                  Input Hii value.

  @retval UINT64                 Return the UINT64 type value.

**/
UINT64
HiiValueToUINT64 (
  IN EFI_HII_VALUE      *Value
  )
{
  UINT64  RetVal;

  RetVal = 0;

  switch (Value->Type) {
  case EFI_IFR_TYPE_NUM_SIZE_8:
    RetVal = Value->Value.u8;
    break;

  case EFI_IFR_TYPE_NUM_SIZE_16:
    RetVal = Value->Value.u16;
    break;

  case EFI_IFR_TYPE_NUM_SIZE_32:
    RetVal = Value->Value.u32;
    break;

  case EFI_IFR_TYPE_BOOLEAN:
    RetVal = Value->Value.b;
    break;

  case EFI_IFR_TYPE_DATE:
    RetVal = *(UINT64*) &Value->Value.date;
    break;

  case EFI_IFR_TYPE_TIME:
    RetVal = (*(UINT64*) &Value->Value.time) & 0xffffff;
    break;

  default:
    RetVal = Value->Value.u64;
    break;
  }

  return RetVal;
}

/**
  Check whether this value type can be transfer to EFI_IFR_TYPE_BUFFER type.

  EFI_IFR_TYPE_REF, EFI_IFR_TYPE_DATE and EFI_IFR_TYPE_TIME are converted to
  EFI_IFR_TYPE_BUFFER when do the value compare.

  @param  Value                  Expression value to compare on.

  @retval TRUE                   This value type can be transter to EFI_IFR_TYPE_BUFFER type.
  @retval FALSE                  This value type can't be transter to EFI_IFR_TYPE_BUFFER type.

**/
BOOLEAN
IsTypeInBuffer (
  IN  EFI_HII_VALUE   *Value
  )
{
  switch (Value->Type) {
  case EFI_IFR_TYPE_BUFFER:
  case EFI_IFR_TYPE_DATE:
  case EFI_IFR_TYPE_TIME:
  case EFI_IFR_TYPE_REF:
    return TRUE;

  default:
    return FALSE;
  }
}

/**
  Check whether this value type can be transfer to EFI_IFR_TYPE_UINT64

  @param  Value                  Expression value to compare on.

  @retval TRUE                   This value type can be transter to EFI_IFR_TYPE_BUFFER type.
  @retval FALSE                  This value type can't be transter to EFI_IFR_TYPE_BUFFER type.

**/
BOOLEAN
IsTypeInUINT64 (
  IN  EFI_HII_VALUE   *Value
  )
{
  switch (Value->Type) {
  case EFI_IFR_TYPE_NUM_SIZE_8:
  case EFI_IFR_TYPE_NUM_SIZE_16:
  case EFI_IFR_TYPE_NUM_SIZE_32:
  case EFI_IFR_TYPE_NUM_SIZE_64:
  case EFI_IFR_TYPE_BOOLEAN:
    return TRUE;

  default:
    return FALSE;
  }
}

/**
  Return the buffer length and buffer pointer for this value.

  EFI_IFR_TYPE_REF, EFI_IFR_TYPE_DATE and EFI_IFR_TYPE_TIME are converted to
  EFI_IFR_TYPE_BUFFER when do the value compare.

  @param  Value                  Expression value to compare on.
  @param  Buf                    Return the buffer pointer.
  @param  BufLen                 Return the buffer length.

**/
VOID
GetBufAndLenForValue (
  IN  EFI_HII_VALUE   *Value,
  OUT UINT8           **Buf,
  OUT UINT16          *BufLen
  )
{
  switch (Value->Type) {
  case EFI_IFR_TYPE_BUFFER:
    *Buf    = Value->Buffer;
    *BufLen = Value->BufferLen;
    break;

  case EFI_IFR_TYPE_DATE:
    *Buf    = (UINT8 *) (&Value->Value.date);
    *BufLen = (UINT16) sizeof (EFI_HII_DATE);
    break;

  case EFI_IFR_TYPE_TIME:
    *Buf    = (UINT8 *) (&Value->Value.time);
    *BufLen = (UINT16) sizeof (EFI_HII_TIME);
    break;

  case EFI_IFR_TYPE_REF:
    *Buf    = (UINT8 *) (&Value->Value.ref);
    *BufLen = (UINT16) sizeof (EFI_HII_REF);
    break;

  default:
    *Buf    = NULL;
    *BufLen = 0;
  }
}

/**
  Compare two Hii value.

  @param  Value1                 Expression value to compare on left-hand.
  @param  Value2                 Expression value to compare on right-hand.
  @param  Result                 Return value after compare.
                                 retval 0                      Two operators equal.
                                 return Positive value if Value1 is greater than Value2.
                                 retval Negative value if Value1 is less than Value2.
  @param  HiiHandle              Only required for string compare.

  @retval other                  Could not perform compare on two values.
  @retval EFI_SUCCESS            Compare the value success.

**/
EFI_STATUS
CompareHiiValue (
  IN  EFI_HII_VALUE   *Value1,
  IN  EFI_HII_VALUE   *Value2,
  OUT INTN            *Result,
  IN  EFI_HII_HANDLE  HiiHandle OPTIONAL
  )
{
  INT64   Temp64;
  CHAR16  *Str1;
  CHAR16  *Str2;
  UINTN   Len;
  UINT8   *Buf1;
  UINT16  Buf1Len;
  UINT8   *Buf2;
  UINT16  Buf2Len;

  if (Value1->Type == EFI_IFR_TYPE_STRING && Value2->Type == EFI_IFR_TYPE_STRING) {
    if (Value1->Value.string == 0 || Value2->Value.string == 0) {
      //
      // StringId 0 is reserved
      //
      return EFI_INVALID_PARAMETER;
    }

    if (Value1->Value.string == Value2->Value.string) {
      *Result = 0;
      return EFI_SUCCESS;
    }

    Str1 = GetToken (Value1->Value.string, HiiHandle);
    if (Str1 == NULL) {
      //
      // String not found
      //
      return EFI_NOT_FOUND;
    }

    Str2 = GetToken (Value2->Value.string, HiiHandle);
    if (Str2 == NULL) {
      FreePool (Str1);
      return EFI_NOT_FOUND;
    }

    *Result = StrCmp (Str1, Str2);

    FreePool (Str1);
    FreePool (Str2);

    return EFI_SUCCESS;
  }

  //
  // Take types(date, time, ref, buffer) as buffer
  //
  if (IsTypeInBuffer(Value1) && IsTypeInBuffer(Value2)) {
    GetBufAndLenForValue(Value1, &Buf1, &Buf1Len);
    GetBufAndLenForValue(Value2, &Buf2, &Buf2Len);

    Len = Buf1Len > Buf2Len ? Buf2Len : Buf1Len;
    *Result = CompareMem (Buf1, Buf2, Len);
    if ((*Result == 0) && (Buf1Len != Buf2Len)) {
      //
      // In this case, means base on samll number buffer, the data is same
      // So which value has more data, which value is bigger.
      //
      *Result = Buf1Len > Buf2Len ? 1 : -1;
    }
    return EFI_SUCCESS;
  }

  //
  // Take remain types(integer, boolean, date/time) as integer
  //
  if (IsTypeInUINT64(Value1) && IsTypeInUINT64(Value2)) {
    Temp64 = HiiValueToUINT64(Value1) - HiiValueToUINT64(Value2);
    if (Temp64 > 0) {
      *Result = 1;
    } else if (Temp64 < 0) {
      *Result = -1;
    } else {
      *Result = 0;
    }
    return EFI_SUCCESS;
  }

  return EFI_UNSUPPORTED;
}

/**
  Search an Option of a Question by its value.

  @param  Question               The Question
  @param  OptionValue            Value for Option to be searched.

  @retval Pointer                Pointer to the found Option.
  @retval NULL                   Option not found.

**/
DISPLAY_QUESTION_OPTION *
ValueToOption (
  IN FORM_DISPLAY_ENGINE_STATEMENT   *Question,
  IN EFI_HII_VALUE                   *OptionValue
  )
{
  LIST_ENTRY               *Link;
  DISPLAY_QUESTION_OPTION  *Option;
  INTN                     Result;
  EFI_HII_VALUE            Value;

  Link = GetFirstNode (&Question->OptionListHead);
  while (!IsNull (&Question->OptionListHead, Link)) {
    Option = DISPLAY_QUESTION_OPTION_FROM_LINK (Link);

    ZeroMem (&Value, sizeof (EFI_HII_VALUE));
    Value.Type = Option->OptionOpCode->Type;
    CopyMem (&Value.Value, &Option->OptionOpCode->Value, Option->OptionOpCode->Header.Length - OFFSET_OF (EFI_IFR_ONE_OF_OPTION, Value));

    if ((CompareHiiValue (&Value, OptionValue, &Result, NULL) == EFI_SUCCESS) && (Result == 0)) {
      return Option;
    }

    Link = GetNextNode (&Question->OptionListHead, Link);
  }

  return NULL;
}


/**
  Return data element in an Array by its Index.

  @param  Array                  The data array.
  @param  Type                   Type of the data in this array.
  @param  Index                  Zero based index for data in this array.

  @retval Value                  The data to be returned

**/
UINT64
GetArrayData (
  IN VOID                     *Array,
  IN UINT8                    Type,
  IN UINTN                    Index
  )
{
  UINT64 Data;

  ASSERT (Array != NULL);

  Data = 0;
  switch (Type) {
  case EFI_IFR_TYPE_NUM_SIZE_8:
    Data = (UINT64) *(((UINT8 *) Array) + Index);
    break;

  case EFI_IFR_TYPE_NUM_SIZE_16:
    Data = (UINT64) *(((UINT16 *) Array) + Index);
    break;

  case EFI_IFR_TYPE_NUM_SIZE_32:
    Data = (UINT64) *(((UINT32 *) Array) + Index);
    break;

  case EFI_IFR_TYPE_NUM_SIZE_64:
    Data = (UINT64) *(((UINT64 *) Array) + Index);
    break;

  default:
    break;
  }

  return Data;
}


/**
  Set value of a data element in an Array by its Index.

  @param  Array                  The data array.
  @param  Type                   Type of the data in this array.
  @param  Index                  Zero based index for data in this array.
  @param  Value                  The value to be set.

**/
VOID
SetArrayData (
  IN VOID                     *Array,
  IN UINT8                    Type,
  IN UINTN                    Index,
  IN UINT64                   Value
  )
{

  ASSERT (Array != NULL);

  switch (Type) {
  case EFI_IFR_TYPE_NUM_SIZE_8:
    *(((UINT8 *) Array) + Index) = (UINT8) Value;
    break;

  case EFI_IFR_TYPE_NUM_SIZE_16:
    *(((UINT16 *) Array) + Index) = (UINT16) Value;
    break;

  case EFI_IFR_TYPE_NUM_SIZE_32:
    *(((UINT32 *) Array) + Index) = (UINT32) Value;
    break;

  case EFI_IFR_TYPE_NUM_SIZE_64:
    *(((UINT64 *) Array) + Index) = (UINT64) Value;
    break;

  default:
    break;
  }
}

/**
  Check whether this value already in the array, if yes, return the index.

  @param  Array                  The data array.
  @param  Type                   Type of the data in this array.
  @param  Value                  The value to be find.
  @param  Index                  The index in the array which has same value with Value.

  @retval   TRUE Found the value in the array.
  @retval   FALSE Not found the value.

**/
BOOLEAN
FindArrayData (
  IN VOID                     *Array,
  IN UINT8                    Type,
  IN UINT64                   Value,
  OUT UINTN                   *Index OPTIONAL
  )
{
  UINTN  Count;
  UINT64 TmpValue;
  UINT64 ValueComp;

  ASSERT (Array != NULL);

  Count    = 0;
  TmpValue = 0;

  switch (Type) {
  case EFI_IFR_TYPE_NUM_SIZE_8:
    ValueComp = (UINT8) Value;
    break;

  case EFI_IFR_TYPE_NUM_SIZE_16:
    ValueComp = (UINT16) Value;
    break;

  case EFI_IFR_TYPE_NUM_SIZE_32:
    ValueComp = (UINT32) Value;
    break;

  case EFI_IFR_TYPE_NUM_SIZE_64:
    ValueComp = (UINT64) Value;
    break;

  default:
    ValueComp = 0;
    break;
  }

  while ((TmpValue = GetArrayData (Array, Type, Count)) != 0) {
    if (ValueComp == TmpValue) {
      if (Index != NULL) {
        *Index = Count;
      }
      return TRUE;
    }

    Count ++;
  }

  return FALSE;
}

/**
  Print Question Value according to it's storage width and display attributes.

  @param  Question               The Question to be printed.
  @param  FormattedNumber        Buffer for output string.
  @param  BufferSize             The FormattedNumber buffer size in bytes.

  @retval EFI_SUCCESS            Print success.
  @retval EFI_BUFFER_TOO_SMALL   Buffer size is not enough for formatted number.

**/
EFI_STATUS
PrintFormattedNumber (
  IN FORM_DISPLAY_ENGINE_STATEMENT   *Question,
  IN OUT CHAR16               *FormattedNumber,
  IN UINTN                    BufferSize
  )
{
  INT64          Value;
  CHAR16         *Format;
  EFI_HII_VALUE  *QuestionValue;
  EFI_IFR_NUMERIC *NumericOp;

  if (BufferSize < (21 * sizeof (CHAR16))) {
    return EFI_BUFFER_TOO_SMALL;
  }

  QuestionValue = &Question->CurrentValue;
  NumericOp     = (EFI_IFR_NUMERIC *) Question->OpCode;

  Value = (INT64) QuestionValue->Value.u64;
  switch (NumericOp->Flags & EFI_IFR_DISPLAY) {
  case EFI_IFR_DISPLAY_INT_DEC:
    switch (QuestionValue->Type) {
    case EFI_IFR_NUMERIC_SIZE_1:
      Value = (INT64) ((INT8) QuestionValue->Value.u8);
      break;

    case EFI_IFR_NUMERIC_SIZE_2:
      Value = (INT64) ((INT16) QuestionValue->Value.u16);
      break;

    case EFI_IFR_NUMERIC_SIZE_4:
      Value = (INT64) ((INT32) QuestionValue->Value.u32);
      break;

    case EFI_IFR_NUMERIC_SIZE_8:
    default:
      break;
    }

    if (Value < 0) {
      Value = -Value;
      Format = L"-%ld";
    } else {
      Format = L"%ld";
    }
    break;

  case EFI_IFR_DISPLAY_UINT_DEC:
    Format = L"%ld";
    break;

  case EFI_IFR_DISPLAY_UINT_HEX:
    Format = L"%lx";
    break;

  default:
    return EFI_UNSUPPORTED;
  }

  UnicodeSPrint (FormattedNumber, BufferSize, Format, Value);

  return EFI_SUCCESS;
}


/**
  Process nothing.

  @param Event    The Event need to be process
  @param Context  The context of the event.

**/
VOID
EFIAPI
EmptyEventProcess (
  IN  EFI_EVENT    Event,
  IN  VOID         *Context
  )
{
}
/**
  Process a Question's Option (whether selected or un-selected).

  @param  MenuOption             The MenuOption for this Question.
  @param  Selected               TRUE: if Question is selected.
  @param  OptionString           Pointer of the Option String to be displayed.
  @param  SkipErrorValue         Whether need to return when value without option for it.

  @retval EFI_SUCCESS            Question Option process success.
  @retval Other                  Question Option process fail.

**/
EFI_STATUS
ProcessOptions (
  IN  UI_MENU_OPTION              *MenuOption,
  IN  BOOLEAN                     Selected,
  OUT CHAR16                      **OptionString,
  IN  BOOLEAN                     SkipErrorValue
  )
{
  EFI_STATUS                      Status;
  CHAR16                          *StringPtr;
  FORM_DISPLAY_ENGINE_STATEMENT   *Question;
  CHAR16                          FormattedNumber[21];
  CHAR16                          Character[2];
  UINTN                           BufferSize;
  EFI_HII_VALUE                   *QuestionValue;
  EFI_STRING_ID                   StringId;
  BOOLEAN                         ValueInvalid;

  Status        = EFI_SUCCESS;

  StringPtr     = NULL;
  Character[1]  = L'\0';
  *OptionString = NULL;
  StringId      = 0;
  ValueInvalid  = FALSE;

  ZeroMem (FormattedNumber, 21 * sizeof (CHAR16));
  BufferSize = (gOptionBlockWidth + 1) * 2 * gStatementDimensions.BottomRow;

  Question = MenuOption->ThisTag;
  QuestionValue = &Question->CurrentValue;

  switch (Question->OpCode->OpCode) {
  case EFI_IFR_CHECKBOX_OP:
    if (Selected) {
      //
      // Since this is a BOOLEAN operation, flip it upon selection
      //
      gUserInput->InputValue.Type    = QuestionValue->Type;
      gUserInput->InputValue.Value.b = (BOOLEAN) (QuestionValue->Value.b ? FALSE : TRUE);

      //
      // Perform inconsistent check
      //
      return EFI_SUCCESS;
    } else {
      *OptionString = AllocateZeroPool (BufferSize);
      ASSERT (*OptionString);

      *OptionString[0] = LEFT_CHECKBOX_DELIMITER;

      if (QuestionValue->Value.b) {
        *(OptionString[0] + 1) = CHECK_ON;
      } else {
        *(OptionString[0] + 1) = CHECK_OFF;
      }
      *(OptionString[0] + 2) = RIGHT_CHECKBOX_DELIMITER;
    }
    break;
  default:
    break;
  }

  return Status;
}


/**
  Process the help string: Split StringPtr to several lines of strings stored in
  FormattedString and the glyph width of each line cannot exceed gHelpBlockWidth.

  @param  StringPtr              The entire help string.
  @param  FormattedString        The oupput formatted string.
  @param  EachLineWidth          The max string length of each line in the formatted string.
  @param  RowCount               TRUE: if Question is selected.

**/
UINTN
ProcessHelpString (
  IN  CHAR16  *StringPtr,
  OUT CHAR16  **FormattedString,
  OUT UINT16  *EachLineWidth,
  IN  UINTN   RowCount
  )
{
  UINTN   Index;
  CHAR16  *OutputString;
  UINTN   TotalRowNum;
  UINTN   CheckedNum;
  UINT16  GlyphWidth;
  UINT16  LineWidth;
  UINT16  MaxStringLen;
  UINT16  StringLen;

  TotalRowNum    = 0;
  CheckedNum     = 0;
  GlyphWidth     = 1;
  Index          = 0;
  MaxStringLen   = 0;
  StringLen      = 0;

  //
  // Set default help string width.
  //
  LineWidth      = (UINT16) (gHelpBlockWidth - 1);

  //
  // Get row number of the String.
  //
  while ((StringLen = GetLineByWidth (StringPtr, LineWidth, &GlyphWidth, &Index, &OutputString)) != 0) {
    if (StringLen > MaxStringLen) {
      MaxStringLen = StringLen;
    }

    TotalRowNum ++;
    FreePool (OutputString);
  }
  *EachLineWidth = MaxStringLen;

  *FormattedString = AllocateZeroPool (TotalRowNum * MaxStringLen * sizeof (CHAR16));
  ASSERT (*FormattedString != NULL);

  //
  // Generate formatted help string array.
  //
  GlyphWidth  = 1;
  Index       = 0;
  while((StringLen = GetLineByWidth (StringPtr, LineWidth, &GlyphWidth, &Index, &OutputString)) != 0) {
    CopyMem (*FormattedString + CheckedNum * MaxStringLen, OutputString, StringLen * sizeof (CHAR16));
    CheckedNum ++;
    FreePool (OutputString);
  }

  return TotalRowNum;
}
