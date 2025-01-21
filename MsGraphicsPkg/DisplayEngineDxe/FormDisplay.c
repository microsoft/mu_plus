/** @file

Entry and initialization module for the Display Engine

This is a derivitive of the Intel Display Engine on TianoCore.

Since the Intel Display Engine gets custom colors and common functionality from
the CustomizedDisplayLib, and the Microsoft version gets colors and graphic information
from the MsThemeLib, there is no need for the CustomizedDisplayLib.



Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2014, Hewlett-Packard Development Company, L.P.
Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "FormDisplay.h"

#include <Library/BmpSupportLib.h>

// Protocols.
//
MS_SIMPLE_WINDOW_MANAGER_PROTOCOL  *mSWMProtocol;
EFI_GRAPHICS_OUTPUT_PROTOCOL       *mGop;
EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *mSimpleTextInEx;
EFI_EVENT                          mReadyToBootEvent = NULL;

// Master Frame configuration and events.
//
EFI_ABSOLUTE_POINTER_PROTOCOL         *mPointerProtocol;
EFI_EVENT                             mMasterFrameNotifyEvent;
UINT32                                mMasterFrameWidth;
UINT32                                mMasterFrameHeight;
UINT32                                mTitleBarHeight;
UINTN                                 mResidualTimeout = 0;
extern EFI_GUID                       gMsEventMasterFrameNotifyGroupGuid;
EFI_HANDLE                            mImageHandle;
static MS_ONSCREEN_KEYBOARD_PROTOCOL  *mOSKProtocol = NULL;

//
// Search table for UiDisplayMenu()
//
SCAN_CODE_TO_SCREEN_OPERATION  gScanCodeToOperation[] = {
  {
    SCAN_UP,
    UiUp,
  },
  {
    SCAN_DOWN,
    UiDown,
  },
  {
    SCAN_PAGE_UP,
    UiPageUp,
  },
  {
    SCAN_PAGE_DOWN,
    UiPageDown,
  },
  {
    SCAN_ESC,
    UiReset,
  },
  {
    SCAN_LEFT,
    UiLeft,
  },
  {
    SCAN_RIGHT,
    UiRight,
  }
};

UINTN  mScanCodeNumber = ARRAY_SIZE (gScanCodeToOperation);

SCREEN_OPERATION_T0_CONTROL_FLAG  gScreenOperationToControlFlag[] = {
  {
    UiNoOperation,
    CfUiNoOperation,
  },
  {
    UiSelect,
    CfUiSelect,
  },
  {
    UiUp,
    CfUiUp,
  },
  {
    UiDown,
    CfUiDown,
  },
  {
    UiLeft,
    CfUiLeft,
  },
  {
    UiRight,
    CfUiRight,
  },
  {
    UiReset,
    CfUiReset,
  },
  {
    UiPageUp,
    CfUiPageUp,
  },
  {
    UiPageDown,
    CfUiPageDown
  },
  {
    UiHotKey,
    CfUiHotKey
  }
};

EFI_GUID  gDisplayEngineGuid = {
  0xE38C1029, 0xE38F, 0x45b9, { 0x8F, 0x0D, 0xE2, 0xE6, 0x0B, 0xC9, 0xB2, 0x62 }
};

BOOLEAN                      gMisMatch;
EFI_SCREEN_DESCRIPTOR        gStatementDimensions;
BOOLEAN                      mStatementLayoutIsChanged = TRUE;
USER_INPUT                   *gUserInput;
FORM_DISPLAY_ENGINE_FORM     *gFormData;
EFI_HII_HANDLE               gHiiHandle;
UINT16                       gDirection;
LIST_ENTRY                   gMenuOption;
DISPLAY_HIGHLIGHT_MENU_INFO  gHighlightMenuInfo      = { 0 };
BOOLEAN                      mIsFirstForm            = TRUE;
BOOLEAN                      mControlsRequireRefresh = FALSE;
BOOLEAN                      mRefreshOnEvent         = FALSE;
FORM_ENTRY_INFO              gOldFormEntry           = { 0 };
UINT32                       mLastOpCrc              = 0;
EFI_HII_DATABASE_PROTOCOL    *mHiiDatabase;
EFI_HANDLE                   mNotifyHandle;

// Form Reuse check
#define CHUNK_SIZE  240 // Set this to 32 to test filling the buffer and reallocating it.

static UINT8  *mBuffer    = NULL;
static UINTN  mIndex      = 0;
static UINTN  mBufferSize = 0;

//
// Browser Global Strings
//
CHAR16  *gFormNotFound;
CHAR16  *gBrowserError;
CHAR16  *gSaveFailed;
CHAR16  *gNoSubmitIfFailed;
CHAR16  *gSaveProcess;
CHAR16  *gSaveNoSubmitProcess;
CHAR16  *gFormSuppress;
CHAR16  *gProtocolNotFound;

CHAR16  gModalSkipColumn;
CHAR16  gPromptBlockWidth;
CHAR16  gOptionBlockWidth;
CHAR16  gHelpBlockWidth;
CHAR16  *mUnknownString;

FORM_DISPLAY_DRIVER_PRIVATE_DATA  mPrivateData = {
  FORM_DISPLAY_DRIVER_SIGNATURE,
  NULL,
  {
    FormDisplay,
    DriverClearDisplayPage,
    ConfirmDataChange
  },
  NULL
};

/**
  Track the form being published.  The the form being cached is updated.
**/
EFI_STATUS
EFIAPI
FormUpdateNotify (
  IN UINT8                         PackageType,
  IN CONST EFI_GUID                *PackageGuid,
  IN CONST EFI_HII_PACKAGE_HEADER  *Package,
  IN EFI_HII_HANDLE                Handle,
  IN EFI_HII_DATABASE_NOTIFY_TYPE  NotifyType
  )
{
  //
  // Force reparsing the form display data
  //
  mLastOpCrc = 0;

  return EFI_SUCCESS;
}

/**
  Get the string based on the StringId and HII Package List Handle.

  @param  Token                  The String's ID.
  @param  HiiHandle              The package list in the HII database to search for
                                 the specified string.

  @return The output string.

**/
CHAR16 *
GetToken (
  IN EFI_STRING_ID   Token,
  IN EFI_HII_HANDLE  HiiHandle
  )
{
  EFI_STRING  String;

  String = HiiGetString (HiiHandle, Token, NULL);
  if (String == NULL) {
    String = AllocateCopyPool (StrSize (mUnknownString), mUnknownString);
    ASSERT (String != NULL);
  }

  return (CHAR16 *)String;
}

/**
  Initialize the HII String Token to the correct values.

**/
VOID
InitializeDisplayStrings (
  VOID
  )
{
  mUnknownString       = GetToken (STRING_TOKEN (UNKNOWN_STRING), gHiiHandle);
  gSaveProcess         = GetToken (STRING_TOKEN (DISCARD_OR_JUMP), gHiiHandle);
  gSaveFailed          = GetToken (STRING_TOKEN (SAVE_FAILED), gHiiHandle);
  gNoSubmitIfFailed    = GetToken (STRING_TOKEN (NO_SUBMIT_IF_CHECK_FAILED), gHiiHandle);
  gSaveNoSubmitProcess = GetToken (STRING_TOKEN (DISCARD_OR_CHECK), gHiiHandle);
  gFormSuppress        = GetToken (STRING_TOKEN (FORM_SUPPRESSED), gHiiHandle);
  gProtocolNotFound    = GetToken (STRING_TOKEN (PROTOCOL_NOT_FOUND), gHiiHandle);
  gFormNotFound        = GetToken (STRING_TOKEN (STATUS_BROWSER_FORM_NOT_FOUND), gHiiHandle);
  gBrowserError        = GetToken (STRING_TOKEN (STATUS_BROWSER_ERROR), gHiiHandle);
}

/**
  Free up the resource allocated for all strings required
  by Setup Browser.

**/
VOID
FreeDisplayStrings (
  VOID
  )
{
  FreePool (mUnknownString);
  FreePool (gSaveFailed);
  FreePool (gNoSubmitIfFailed);
  FreePool (gSaveProcess);
  FreePool (gSaveNoSubmitProcess);
  FreePool (gFormSuppress);
  FreePool (gProtocolNotFound);
  FreePool (gBrowserError);
  FreePool (gFormNotFound);
}

/**
  Get prompt string id from the opcode data buffer.

  @param  OpCode                 The input opcode buffer.

  @return The prompt string id.

**/
EFI_STRING_ID
GetPrompt (
  IN EFI_IFR_OP_HEADER  *OpCode
  )
{
  EFI_IFR_STATEMENT_HEADER  *Header;

  if (OpCode->Length <= sizeof (EFI_IFR_OP_HEADER)) {
    return 0;
  }

  Header = (EFI_IFR_STATEMENT_HEADER *)(OpCode + 1);

  return Header->Prompt;
}

/**
  Get the supported width for a particular op-code

  @param  MenuOption             The menu option.
  @param  AdjustWidth            The width which is saved for the space.

  @return Returns the number of CHAR16 characters that is support.

**/
UINT16
GetWidth (
  IN UI_MENU_OPTION  *MenuOption,
  OUT UINT16         *AdjustWidth
  )
{
  CHAR16                         *String;
  UINTN                          Size;
  EFI_IFR_TEXT                   *TextOp;
  UINT16                         ReturnWidth;
  FORM_DISPLAY_ENGINE_STATEMENT  *Statement;

  Statement = MenuOption->ThisTag;
  Size      = 0;

  //
  // See if the second text parameter is really NULL
  //
  if (Statement->OpCode->OpCode == EFI_IFR_TEXT_OP) {
    TextOp = (EFI_IFR_TEXT *)Statement->OpCode;
    if (TextOp->TextTwo != 0) {
      String = GetToken (TextOp->TextTwo, gFormData->HiiHandle);
      // MU_CHANGE [BEGIN] - CodeQL change
      if (String != NULL) {
        Size = StrLen (String);
        FreePool (String);
      }

      // MU_CHANGE [END] - CodeQL change
    }
  }

  if ((Statement->OpCode->OpCode == EFI_IFR_SUBTITLE_OP) ||
      (Statement->OpCode->OpCode == EFI_IFR_REF_OP) ||
      (Statement->OpCode->OpCode == EFI_IFR_PASSWORD_OP) ||
      (Statement->OpCode->OpCode == EFI_IFR_ACTION_OP) ||
      (Statement->OpCode->OpCode == EFI_IFR_RESET_BUTTON_OP) ||
      //
      // Allow a wide display if text op-code and no secondary text op-code
      //
      ((Statement->OpCode->OpCode == EFI_IFR_TEXT_OP) && (Size == 0))
      )
  {
    //
    // Return the space width.
    //
    if (AdjustWidth != NULL) {
      *AdjustWidth = 2;
    }

    //
    // Keep consistent with current behavior.
    //
    ReturnWidth = (UINT16)(gPromptBlockWidth + gOptionBlockWidth - 2);
  } else {
    if (AdjustWidth != NULL) {
      *AdjustWidth = 1;
    }

    ReturnWidth = (UINT16)(gPromptBlockWidth - 1);
  }

  //
  // For nest in statement, should the subtitle indent.
  //
  if (MenuOption->NestInStatement) {
    ReturnWidth -= SUBTITLE_INDENT;
  }

  return ReturnWidth;
}

/**
  Will copy LineWidth amount of a string in the OutputString buffer and return the
  number of CHAR16 characters that were copied into the OutputString buffer.
  The output string format is:
    Glyph Info + String info + '\0'.

  In the code, it deals \r,\n,\r\n same as \n\r, also it not process the \r or \g.

  @param  InputString            String description for this option.
  @param  LineWidth              Width of the desired string to extract in CHAR16
                                 characters
  @param  GlyphWidth             The glyph width of the begin of the char in the string.
  @param  Index                  Where in InputString to start the copy process
  @param  OutputString           Buffer to copy the string into

  @return Returns the number of CHAR16 characters that were copied into the OutputString
  buffer, include extra glyph info and '\0' info.

**/
UINT16
GetLineByWidth (
  IN CHAR16      *InputString,
  IN UINT16      LineWidth,
  IN OUT UINT16  *GlyphWidth,
  IN OUT UINTN   *Index,
  OUT CHAR16     **OutputString
  )
{
  UINT16   StrOffset;
  UINT16   GlyphOffset;
  UINT16   OriginalGlyphWidth;
  BOOLEAN  ReturnFlag;
  UINT16   LastSpaceOffset;
  UINT16   LastGlyphWidth;

  if ((InputString == NULL) || (Index == NULL) || (OutputString == NULL)) {
    return 0;
  }

  if ((LineWidth == 0) || (*GlyphWidth == 0)) {
    return 0;
  }

  //
  // Save original glyph width.
  //
  OriginalGlyphWidth = *GlyphWidth;
  LastGlyphWidth     = OriginalGlyphWidth;
  ReturnFlag         = FALSE;
  LastSpaceOffset    = 0;

  //
  // NARROW_CHAR can not be printed in screen, so if a line only contain  the two CHARs: 'NARROW_CHAR + CHAR_CARRIAGE_RETURN' , it is a empty line  in Screen.
  // To avoid displaying this  empty line in screen,  just skip  the two CHARs here.
  //
  if ((InputString[*Index] == NARROW_CHAR) && (InputString[*Index + 1] == CHAR_CARRIAGE_RETURN)) {
    *Index = *Index + 2;
  }

  //
  // Fast-forward the string and see if there is a carriage-return in the string
  //
  for (StrOffset = 0, GlyphOffset = 0; GlyphOffset <= LineWidth; StrOffset++) {
    switch (InputString[*Index + StrOffset]) {
      case NARROW_CHAR:
        *GlyphWidth = 1;
        break;

      case WIDE_CHAR:
        *GlyphWidth = 2;
        break;

      case CHAR_CARRIAGE_RETURN:
      case CHAR_LINEFEED:
      case CHAR_NULL:
        ReturnFlag = TRUE;
        break;

      default:
        GlyphOffset = GlyphOffset + *GlyphWidth;

        //
        // Record the last space info in this line. Will be used in rewind.
        //
        if ((InputString[*Index + StrOffset] == CHAR_SPACE) && (GlyphOffset <= LineWidth)) {
          LastSpaceOffset = StrOffset;
          LastGlyphWidth  = *GlyphWidth;
        }

        break;
    }

    if (ReturnFlag) {
      break;
    }
  }

  //
  // Rewind the string from the maximum size until we see a space to break the line
  //
  if (GlyphOffset > LineWidth) {
    //
    // Rewind the string to last space char in this line.
    //
    if (LastSpaceOffset != 0) {
      StrOffset   = LastSpaceOffset;
      *GlyphWidth = LastGlyphWidth;
    } else {
      //
      // Roll back to last char in the line width.
      //
      StrOffset--;
    }
  }

  //
  // The CHAR_NULL has process last time, this time just return 0 to stand for the end.
  //
  if ((StrOffset == 0) && (InputString[*Index + StrOffset] == CHAR_NULL)) {
    return 0;
  }

  //
  // Need extra glyph info and '\0' info, so +2.
  //
  *OutputString = AllocateZeroPool ((StrOffset + 2) * sizeof (CHAR16));
  if (*OutputString == NULL) {
    return 0;
  }

  //
  // Save the glyph info at the begin of the string, will used by Print function.
  //
  if (OriginalGlyphWidth == 1) {
    *(*OutputString) = NARROW_CHAR;
  } else {
    *(*OutputString) = WIDE_CHAR;
  }

  CopyMem ((*OutputString) + 1, &InputString[*Index], StrOffset * sizeof (CHAR16));

  if (InputString[*Index + StrOffset] == CHAR_SPACE) {
    //
    // Skip the space info at the begin of next line.
    //
    *Index = (UINT16)(*Index + StrOffset + 1);
  } else if (InputString[*Index + StrOffset] == CHAR_LINEFEED) {
    //
    // Skip the /n or /n/r info.
    //
    if (InputString[*Index + StrOffset + 1] == CHAR_CARRIAGE_RETURN) {
      *Index = (UINT16)(*Index + StrOffset + 2);
    } else {
      *Index = (UINT16)(*Index + StrOffset + 1);
    }
  } else if (InputString[*Index + StrOffset] == CHAR_CARRIAGE_RETURN) {
    //
    // Skip the /r or /r/n info.
    //
    if (InputString[*Index + StrOffset + 1] == CHAR_LINEFEED) {
      *Index = (UINT16)(*Index + StrOffset + 2);
    } else {
      *Index = (UINT16)(*Index + StrOffset + 1);
    }
  } else {
    *Index = (UINT16)(*Index + StrOffset);
  }

  //
  // Include extra glyph info and '\0' info, so +2.
  //
  return StrOffset + 2;
}

// Opcode CRC mechanism
VOID
MeasureStart (
  )
{
  if (mBuffer != NULL) {
    FreePool (mBuffer);
  }

  mBuffer     = NULL;
  mIndex      = 0;
  mBufferSize = 0;
}

// Measure the VFR Opcode sequence.  Store the opcode into a buffer, then crc the buffer.
void
Measure (
  UINT8  Data
  )
{
  if (mIndex == mBufferSize) {
    if (mBuffer == NULL) {
      mBuffer     = AllocatePool (CHUNK_SIZE);
      mBufferSize = CHUNK_SIZE;
    } else {
      mBuffer      = ReallocatePool (mBufferSize, mBufferSize + CHUNK_SIZE, mBuffer);
      mBufferSize += CHUNK_SIZE;
    }
  }

  mBuffer[mIndex++] = Data;
}

UINT32
MeasureEnd (
  )
{
  UINT32      Crc = 0;
  EFI_STATUS  Status;

  if (mBuffer != NULL) {
    Status = gBS->CalculateCrc32 (mBuffer, mIndex, &Crc);
    if (EFI_ERROR (Status)) {
      Crc = 0;
    }

    FreePool (mBuffer);
    mBuffer     = NULL;
    mIndex      = 0;
    mBufferSize = 0;
  }

  return Crc;
}

/**
  Add one menu option by specified description and context.

  @param  Statement              Statement of this Menu Option.
  @param  MenuItemCount          The index for this Option in the Menu.
  @param  NestIn                 Whether this statement is nest in another statement.

**/
VOID
UiAddMenuOption (
  IN FORM_DISPLAY_ENGINE_STATEMENT  *Statement,
  IN UINT16                         *MenuItemCount,
  IN BOOLEAN                        NestIn
  )
{
  UI_MENU_OPTION  *MenuOption;
  UINTN           Index;
  UINTN           Count;
  UINT16          NumberOfLines;
  UINT16          GlyphWidth;
  UINT16          Width;
  UINTN           ArrayEntry;
  CHAR16          *OutputString;
  EFI_STRING_ID   PromptId;
  UINT32          OptionCount;
  LIST_ENTRY      *OLLink;

  NumberOfLines = 1;
  ArrayEntry    = 0;
  GlyphWidth    = 1;
  Count         = 1;
  MenuOption    = NULL;

  PromptId = GetPrompt (Statement->OpCode);
  ASSERT (PromptId != 0);

  if ((Statement->OpCode->OpCode == EFI_IFR_DATE_OP) || (Statement->OpCode->OpCode == EFI_IFR_TIME_OP)) {
    Count = 3;
  }

  for (Index = 0; Index < Count; Index++) {
    MenuOption = AllocateZeroPool (sizeof (UI_MENU_OPTION));
    // MU_CHANGE [BEGIN] - CodeQL change
    if (MenuOption == NULL) {
      ASSERT (MenuOption != NULL);
      return;
    }

    // MU_CHANGE [END] - CodeQL change
    MenuOption->Signature       = UI_MENU_OPTION_SIGNATURE;
    MenuOption->Description     = GetToken (PromptId, gFormData->HiiHandle);
    MenuOption->Handle          = gFormData->HiiHandle;
    MenuOption->ThisTag         = Statement;
    MenuOption->NestInStatement = NestIn;
    MenuOption->EntryNumber     = *MenuItemCount;
    // MenuOption->TouchSelection.Selectable = FALSE;

    MenuOption->Sequence = Index;

    if ((Statement->Attribute & HII_DISPLAY_GRAYOUT) != 0) {
      MenuOption->GrayOut = TRUE;
    } else {
      MenuOption->GrayOut = FALSE;
    }

    if (((Statement->Attribute & HII_DISPLAY_LOCK) != 0) || ((gFormData->Attribute & HII_DISPLAY_LOCK) != 0)) {
      MenuOption->GrayOut = TRUE;
    }

    //
    // If the form or the question has the lock attribute, deal same as grayout.
    //
    if (((gFormData->Attribute & HII_DISPLAY_LOCK) != 0) || ((Statement->Attribute & HII_DISPLAY_LOCK) != 0)) {
      MenuOption->GrayOut = TRUE;
    }

    Measure (Statement->OpCode->OpCode);

    switch (Statement->OpCode->OpCode) {
      case EFI_IFR_ORDERED_LIST_OP:
      case EFI_IFR_ONE_OF_OP:

        // Count the number of option entries.
        //
        OptionCount = 0;
        OLLink      = GetFirstNode (&Statement->OptionListHead);
        while (!IsNull (&Statement->OptionListHead, OLLink)) {
          OptionCount++;
          OLLink = GetNextNode (&Statement->OptionListHead, OLLink);
        }

        Measure ((UINT8)OptionCount);
        Measure ((UINT8)(OptionCount >> 8));

      // NO BREAK HERE, Fall through.

      case EFI_IFR_NUMERIC_OP:
      case EFI_IFR_TIME_OP:
      case EFI_IFR_DATE_OP:
      case EFI_IFR_CHECKBOX_OP:
      case EFI_IFR_PASSWORD_OP:
      case EFI_IFR_STRING_OP:
        //
        // User could change the value of these items
        //
        MenuOption->IsQuestion = TRUE;
        break;
      case EFI_IFR_TEXT_OP:
        if (FeaturePcdGet (PcdBrowserGrayOutTextStatement)) {
          //
          // Initializing GrayOut option as TRUE for Text setup options
          // so that those options will be Gray in colour and un selectable.
          //
          MenuOption->ReadOnly = TRUE;
        }

        break;
      default:
        MenuOption->IsQuestion = FALSE;
        break;
    }

    if ((Statement->Attribute & HII_DISPLAY_READONLY) != 0) {
      MenuOption->ReadOnly = TRUE;
      if (FeaturePcdGet (PcdBrowerGrayOutReadOnlyMenu)) {
        MenuOption->GrayOut = TRUE;
      }
    }

    if ((Index == 0) &&
        (Statement->OpCode->OpCode != EFI_IFR_DATE_OP) &&
        (Statement->OpCode->OpCode != EFI_IFR_TIME_OP))
    {
      Width = GetWidth (MenuOption, NULL);
      for ( ; GetLineByWidth (MenuOption->Description, Width, &GlyphWidth, &ArrayEntry, &OutputString) != 0x0000;) {
        //
        // If there is more string to process print on the next row and increment the Skip value
        //
        if (StrLen (&MenuOption->Description[ArrayEntry]) != 0) {
          NumberOfLines++;
        }

        FreePool (OutputString);
      }
    } else {
      //
      // Add three MenuOptions for Date/Time
      // Data format :      [01/02/2004]      [11:22:33]
      // Line number :        0  0    1         0  0  1
      //
      NumberOfLines = 0;
    }

    if (Index == 2) {
      //
      // Override LineNumber for the MenuOption in Date/Time sequence
      //
      MenuOption->Skip = 1;
    } else {
      MenuOption->Skip = NumberOfLines;
    }

    InsertTailList (&gMenuOption, &MenuOption->Link);
  }

  (*MenuItemCount)++;
}

/**
  Create the menu list base on the form data info.

**/
VOID
ConvertStatementToMenu (
  VOID
  )
{
  UINT16                         MenuItemCount;
  LIST_ENTRY                     *Link;
  LIST_ENTRY                     *NestLink;
  FORM_DISPLAY_ENGINE_STATEMENT  *Statement;
  FORM_DISPLAY_ENGINE_STATEMENT  *NestStatement;

  MenuItemCount = 0;
  InitializeListHead (&gMenuOption);

  Link = GetFirstNode (&gFormData->StatementListHead);
  while (!IsNull (&gFormData->StatementListHead, Link)) {
    Statement = FORM_DISPLAY_ENGINE_STATEMENT_FROM_LINK (Link);
    Link      = GetNextNode (&gFormData->StatementListHead, Link);

    UiAddMenuOption (Statement, &MenuItemCount, FALSE);

    //
    // Check the statement nest in this host statement.
    //
    NestLink = GetFirstNode (&Statement->NestStatementList);
    while (!IsNull (&Statement->NestStatementList, NestLink)) {
      NestStatement = FORM_DISPLAY_ENGINE_STATEMENT_FROM_LINK (NestLink);
      NestLink      = GetNextNode (&Statement->NestStatementList, NestLink);

      UiAddMenuOption (NestStatement, &MenuItemCount, TRUE);
    }
  }
}

static
EFI_STATUS
CalculateGridSize (
  IN LIST_ENTRY  *Link,
  OUT UINT32     *Rows,
  OUT UINT32     *Columns
  )
{
  EFI_STATUS  Status = EFI_SUCCESS;
  UINT32      MaxRows,
              MaxColumns,
              Column;
  UI_MENU_OPTION                 *MenuOption;
  FORM_DISPLAY_ENGINE_STATEMENT  *Statement;

  // Determine the size of the grid required.  Walk through all the statements until the Grid End
  // OpCode is found.  The Subtile OpCode is used to group elements horizontally - keep track of the
  // maximum number of nested elements under a Subtitle OpCode.  This will be the number of columns
  // required.  The total number of Subtitle OpCodes defines the number of rows.
  //
  MaxRows    = 0;
  MaxColumns = 0;
  Column     = 0;

  // Use a temporary link pointer so we don't lose track of the original position, move to the next link node.
  //
  BOOLEAN  FoundEndOpCode = FALSE;

  while (!IsNull (&gMenuOption, Link) && FALSE == FoundEndOpCode) {
    MenuOption = MENU_OPTION_FROM_LINK (Link);
    Statement  = MenuOption->ThisTag;

    switch (Statement->OpCode->OpCode) {
      case EFI_IFR_CHECKBOX_OP:     // Button.
      case EFI_IFR_STRING_OP:       // Edit Box.
        Column++;                   // *** Fall through to the next case group (checkboxes and edit boxes two columns).
      case EFI_IFR_ACTION_OP:       // Button.
      case EFI_IFR_REF_OP:          // Form Goto.
      case EFI_IFR_TEXT_OP:         // Text.
        Column++;
        if (Column > MaxColumns) {
          MaxColumns = Column;
        }

        break;
      case EFI_IFR_SUBTITLE_OP:     // Subtitle.
        Column = 0;
        MaxRows++;
        break;
      case EFI_IFR_GUID_OP:
      {
        EFI_GUID  GridEndGuid        = GRID_END_OPCODE_GUID;
        EFI_GUID  GridSelectCellGuid = GRID_SELECT_CELL_OPCODE_GUID;

        if (CompareGuid (&GridEndGuid, (EFI_GUID *)((CHAR8 *)Statement->OpCode + sizeof (EFI_IFR_OP_HEADER)))) {
          FoundEndOpCode = TRUE;
        } else if (CompareGuid (&GridSelectCellGuid, (EFI_GUID *)((CHAR8 *)Statement->OpCode + sizeof (EFI_IFR_OP_HEADER)))) {
          // Allow for sparsely populated grids by location.  Account for the specific cell requested.
          UINT32  GridRowColumn = *(UINT32 *)(&Statement->OpCode->OpCode + sizeof (EFI_IFR_GUID));
          UINT32  Row;

          Row    = GridRowColumn >> 16;
          Column = GridRowColumn & 0xFFFF;
          DEBUG ((DEBUG_INFO, "INFO [DE]: Found Grid GUID SelectCell OpCode for row %d column %d.\r\n", Row, Column));
          if (Row > MaxRows) {
            MaxRows = Row;
          }

          if (Column > MaxColumns) {
            MaxColumns = Column;
          }
        }

        break;
      }
      default:
        break;
    }

    // Move to the next statement.
    //
    Link = GetNextNode (&gMenuOption, Link);
  }

  // Return information to the caller - note that row and/or column size may be 0.
  //
  *Rows    = MaxRows;
  *Columns = MaxColumns;

  return Status;
}

static
EFI_STATUS
CreateFormControls (
  IN FORM_DISPLAY_ENGINE_FORM  *FormData,
  OUT Canvas                   **FormCanvas
  )
{
  EFI_STATUS                     Status = EFI_SUCCESS;
  EFI_FONT_INFO                  FontInfo;
  SWM_RECT                       FormRect;
  SWM_RECT                       ControlRect;
  Canvas                         *LocalCanvas;
  LIST_ENTRY                     *Link;
  UI_MENU_OPTION                 *MenuOption;
  CHAR16                         *Description;
  FORM_DISPLAY_ENGINE_STATEMENT  *Statement;
  BOOLEAN                        GridScope              = FALSE;
  Grid                           *LocalGrid             = NULL;
  UINT16                         CurrentColumn          = 0;
  UINT16                         CurrentRow             = 0;
  BOOLEAN                        FoundFirstGridSubtitle = FALSE;

  // Define a canvas bounding rectangle that fills the form window.
  //
  SWM_RECT_INIT2 (
    FormRect,
    mMasterFrameWidth,
    mTitleBarHeight,
    mGop->Mode->Info->HorizontalResolution - mMasterFrameWidth,
    mGop->Mode->Info->VerticalResolution - mTitleBarHeight
    );

  // Create a canvas for rendering the HII form.
  //
  LocalCanvas = new_Canvas (
                  FormRect,
                  &gMsColorTable.FormCanvasBackgroundColor
                  );

  ASSERT (NULL != LocalCanvas);
  if (NULL == LocalCanvas) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  // Configure a default font size and style.
  //
  FontInfo.FontStyle   = EFI_HII_FONT_STYLE_NORMAL;
  FontInfo.FontSize    = MsUiGetSmallFontHeight ();    // Cell height for small font.
  FontInfo.FontName[0] = L'\0';

  // Set a starting position within the canvas for rendering UI controls.
  //
  UINT32  OrigX             = (FormRect.Left + ((mMasterFrameWidth * FP_FCANVAS_BORDER_PAD_WIDTH_PERCENT) / 100));
  UINT32  OrigY             = (FormRect.Top + ((mMasterFrameHeight * FP_FCANVAS_BORDER_PAD_HEIGHT_PERCENT) / 100));
  UINT32  CanvasRightLimit  = (FormRect.Right - ((mMasterFrameWidth * FP_FCANVAS_BORDER_PAD_WIDTH_PERCENT) / 100));
  UINT32  CanvasBottomLimit = (FormRect.Bottom - ((mMasterFrameHeight * FP_FCANVAS_BORDER_PAD_HEIGHT_PERCENT) / 100));

  // Walk through the list of processed HII opcodes and create custom UI controls for each element.
  //
  for (Link = GetFirstNode (&gMenuOption); (EFI_SUCCESS == Status) && !IsNull (&gMenuOption, Link); Link = GetNextNode (&gMenuOption, Link)) {
    MenuOption      = MENU_OPTION_FROM_LINK (Link);
    MenuOption->Row = OrigY;
    MenuOption->Col = OrigX;

    Description = MenuOption->Description;
    Statement   = MenuOption->ThisTag;

    // Debug
 #if 0
    if (NULL != Statement) {
      if (EFI_IFR_GUID_OP == Statement->OpCode->OpCode) {
        EFI_IFR_GUID  *GUIDOpCode = (EFI_IFR_GUID *)(&Statement->OpCode->OpCode);
        DEBUG ((DEBUG_ERROR, "INFO [DE]: OpCode=0x%x, GUID=%g\r\n", Statement->OpCode->OpCode, GUIDOpCode->Guid));
      } else {
        DEBUG ((DEBUG_ERROR, "INFO [DE]: OpCode=0x%x, Description=\"%s\"\r\n", Statement->OpCode->OpCode, Description));
      }
    }

 #endif

    // Skip a line if the string is blank (VFR-formatted spacing).  Use the last font height selected.
    //
    if ((FALSE == GridScope) && ((NULL == Description) || (L'\0' == *Description))) {
      OrigY += FontInfo.FontSize;
      continue;
    }

    // Select an appropriate font size based on the font height escape sequence encoded in the UNI string.
    //
    // HACK - Our UNI file compiler doesn't seem to support the Sep. 2014 Revision 1.2 UNI file format specification for encoding font information.
    //
    if (0 == StrnCmp (Description, L"\\fh!48!", 7)) {
      Description      += StrLen (L"\\fh!48!");
      FontInfo.FontSize = MsUiGetLargeFontHeight ();           // Cell height for large font.
    } else if (0 == StrnCmp (Description, L"\\fh!36!", 7)) {
      Description      += StrLen (L"\\fh!36!");
      FontInfo.FontSize = MsUiGetMediumFontHeight ();           // Cell height for medium font.
    } else if (0 == StrnCmp (Description, L"\\fh!28!", 7)) {
      Description      += StrLen (L"\\fh!28!");
      FontInfo.FontSize = MsUiGetStandardFontHeight ();         // Cell height for standard font.
    } else if (0 == StrnCmp (Description, L"\\fh!24!", 7)) {
      Description      += StrLen (L"\\fh!24!");
      FontInfo.FontSize = MsUiGetSmallFontHeight ();            // Cell height for small font.
    } else if (0 == StrnCmp (Description, L"\\f!Fixed!", 9)) {
      Description      += StrLen (L"\\f!Fixed!");
      FontInfo.FontSize = MsUiGetFixedFontHeight ();            // Cell height for fixed font.
    } else {
      FontInfo.FontSize = MsUiGetStandardFontHeight ();         // Cell height for standard font.
    }

    // Process the current statement.
    //
    switch (Statement->OpCode->OpCode) {
      case EFI_IFR_SUBTITLE_OP:     // Subtitle.
      {
        if (TRUE == GridScope) {
          if (TRUE == FoundFirstGridSubtitle) {
            CurrentColumn = 0;
            CurrentRow++;
          }

          FoundFirstGridSubtitle = TRUE;
        }

        break;
      }
      case EFI_IFR_GUID_OP:
      {
        EFI_GUID  GridStartOpcodeGuid      = GRID_START_OPCODE_GUID;
        EFI_GUID  GridEndOpcodeGuid        = GRID_END_OPCODE_GUID;
        EFI_GUID  GridSelectCellOpcodeGuid = GRID_SELECT_CELL_OPCODE_GUID;
        EFI_GUID  BitmapOpcodeGuid         = BITMAP_OPCODE_GUID;

        // Check whether this is a UI Grid Start opcode.
        //
        if (CompareGuid (&GridStartOpcodeGuid, (EFI_GUID *)((CHAR8 *)Statement->OpCode + sizeof (EFI_IFR_OP_HEADER)))) {
          SWM_RECT  GridRect;
          UINT32    MaxRows, MaxColumns;
          UINT32    GridCellHeight = MsUiScaleByTheme (*(UINT32 *)(&Statement->OpCode->OpCode + sizeof (EFI_IFR_GUID)));

          // Calculate the required grid size.
          //
          Status = CalculateGridSize (
                     Link,
                     &MaxRows,
                     &MaxColumns
                     );

          if (EFI_ERROR (Status) || (0 == MaxRows) || (0 == MaxColumns)) {
            DEBUG ((DEBUG_ERROR, "ERROR [DE]: Calculated grid size (Rows=%d, Columns=%d, CellHeight=%d) failed.  Code=%r.\n", MaxRows, MaxColumns, GridCellHeight, Status));
            break;
          }

          // DEBUG ((DEBUG_INFO, "INFO [DE]: Calculated grid size (Rows=%d, Columns=%d, CellHeight=%d).\r\n", MaxRows, MaxColumns, GridCellHeight));

          // Calculate the correct size for the grid.
          //
          SWM_RECT_INIT (
            GridRect,
            OrigX,
            OrigY,
            CanvasRightLimit,
            OrigY + (MaxRows * GridCellHeight) - 1
            );

          // Create a grid for aligning UI controls.
          //
          LocalGrid = new_Grid (
                        LocalCanvas,
                        GridRect,
                        MaxRows,                       // Number of Rows.
                        MaxColumns,                    // Number of Columns.
                        TRUE                           // Clip child controls to fit their respective grid cells.
                        );

          ASSERT (NULL != LocalGrid);
          if (NULL == LocalGrid) {
            Status = EFI_OUT_OF_RESOURCES;
            break;
          }

          // Add the control to the canvas.
          //
          Status = LocalCanvas->AddControl (
                                  LocalCanvas,
                                  FALSE,                                   // Grid doesn't support highlighting.
                                  TRUE,                                    // Grids are invisible.
                                  (VOID *)LocalGrid
                                  );

          if (EFI_ERROR (Status)) {
            break;
          }

          // Increment the vertical position by the size of the grid.
          //
          LocalGrid->Base.GetControlBounds (
                            LocalGrid,
                            &ControlRect
                            );

          OrigY += SWM_RECT_HEIGHT (ControlRect);

          // Indicate that the grid now has scope.  Until the Grid End OpCode is encountered, all
          // controls added from this point forward will be added to the grid.
          //
          CurrentColumn          = 0;
          CurrentRow             = 0;
          FoundFirstGridSubtitle = FALSE;
          GridScope              = TRUE;
        } else if (CompareGuid (&GridEndOpcodeGuid, (EFI_GUID *)((CHAR8 *)Statement->OpCode + sizeof (EFI_IFR_OP_HEADER)))) {
          // Signal that the current grid no longer has scope. Controls added from this point forward will be added
          // directly to the canvas (unless another grid is created).
          //
          if (LocalGrid == NULL) {
            DEBUG ((DEBUG_ERROR, "ERROR [DE]: GridEndOp without valid StartGridOp\n"));
          }

          if ((LocalGrid != NULL) && (LocalGrid->m_GridInitialHeight != LocalGrid->m_GridCellHeight)) {
            DEBUG ((DEBUG_ERROR, "ERROR [DE]: Grid elements larger than initial grid height.  Correct VFR StartGridOp value.\r\n"));
          }

          GridScope = FALSE;
          LocalGrid = NULL;
        } else if (CompareGuid (&GridSelectCellOpcodeGuid, (EFI_GUID *)((CHAR8 *)Statement->OpCode + sizeof (EFI_IFR_OP_HEADER)))) {
          // Select a specific grid cell for the next UI control to be placed.
          //
          UINT32  DataPayload = *(UINT32 *)(&Statement->OpCode->OpCode + sizeof (EFI_IFR_GUID));

          CurrentRow    = (UINT16)((DataPayload & 0xFFFF0000) >> 16);
          CurrentColumn = (UINT16)(DataPayload & 0x0000FFFF);
        } else if (CompareGuid (&BitmapOpcodeGuid, (EFI_GUID *)((CHAR8 *)Statement->OpCode + sizeof (EFI_IFR_OP_HEADER)))) {
          UINT8                          *BMPData    = NULL;
          UINTN                          BMPDataSize = 0;
          EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *BltBuffer  = NULL;
          UINTN                          BltBufferSize;
          UINTN                          BitmapHeight;
          UINTN                          BitmapWidth;

          // Extract the bitmap file GUID from the IFR opcode payload.
          //
          EFI_GUID  *FileGuid = (EFI_GUID *)(&Statement->OpCode->OpCode + sizeof (EFI_IFR_GUID));

          DEBUG ((DEBUG_INFO, "INFO [DE]: Found bitmap opcode (GUID=%g).\r\n", FileGuid));

          // Get the specified image from FV.
          //
          Status = GetSectionFromAnyFv (
                     FileGuid,
                     EFI_SECTION_RAW,
                     0,
                     (VOID **)&BMPData,
                     &BMPDataSize
                     );

          if (EFI_ERROR (Status)) {
            DEBUG ((DEBUG_ERROR, "ERROR [DE]: Failed to find bitmap file (GUID=%g) (%r).\r\n", FileGuid, Status));
            break;
          }

          // Convert the bitmap from BMP format to a GOP framebuffer-compatible form.
          //
          Status = TranslateBmpToGopBlt (
                     BMPData,
                     BMPDataSize,
                     &BltBuffer,
                     &BltBufferSize,
                     &BitmapHeight,
                     &BitmapWidth
                     );
          if (EFI_ERROR (Status)) {
            FreePool (BMPData);
            DEBUG ((DEBUG_ERROR, "ERROR [DE]: Failed to convert bitmap file to GOP format (%r).\r\n", Status));
            break;
          }

          Bitmap  *B = new_Bitmap (
                         (GridScope ? 0 : (UINT32)MenuOption->Col),
                         (GridScope ? 0 : (UINT32)MenuOption->Row),
                         (UINT32)BitmapWidth,
                         (UINT32)BitmapHeight,
                         BltBuffer
                         );

          // Clean-up memory before we go on.
          //
          FreePool (BMPData);
          FreePool (BltBuffer);

          ASSERT (NULL != B);
          if (NULL == B) {
            Status = EFI_OUT_OF_RESOURCES;
            break;
          }

          MenuOption->BaseControl = &B->Base;

          // Add the control to either the current grid or the canvas, depending on scope.
          //

          if (TRUE == GridScope) {
            // Add the control to the grid.
            //
            Status = LocalGrid->AddControl (
                                  LocalGrid,
                                  FALSE,                        // Not highlightable.
                                  FALSE,                        // Not invisible.
                                  CurrentRow,
                                  CurrentColumn,
                                  (VOID *)B
                                  );

            // Move to the next grid column.
            //
            CurrentColumn++;
          } else {
            // Add the control to the canvas.
            //
            Status = LocalCanvas->AddControl (
                                    LocalCanvas,
                                    FALSE,                       // Not highlightable.
                                    FALSE,                       // Not invisible.
                                    (VOID *)B
                                    );

            // Increment the vertical position by the size of the button.
            //
            B->Base.GetControlBounds (
                      B,
                      &ControlRect
                      );

            // Move the next control's origin down the page by the height of the current control.
            //
            OrigY += SWM_RECT_HEIGHT (ControlRect);
          }
        }

        break;
      }

      case EFI_IFR_ACTION_OP:       // Button.
      case EFI_IFR_REF_OP:          // Form Goto.
      {
        EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *NormalColor, *HoverColor, *SelectColor, *RingColor, *TextColor, *SelectTextColor;
        UINT32                         ButtonWidth, ButtonHeight;

        // Set up the colors based off the attributes.
        //
        if ((Statement->Attribute & HII_DISPLAY_READONLY) == HII_DISPLAY_READONLY) {
          // If this is a link-style button...
          // The background should always be white.
          NormalColor = HoverColor = SelectColor = RingColor = &gMsColorTable.ButtonNormalColor;
          // The text color should be blue and grey when clicked.
          TextColor       = &gMsColorTable.ButtonTextNormalColor;            // As per UI doc
          SelectTextColor = &gMsColorTable.ButtonTextSelectColor;            // As per UI doc

          ButtonWidth = ButtonHeight = SUI_BUTTON_AUTO_SIZE;
        } else {
          // If this is a standard button...
          // Button background should be grey.
          NormalColor = HoverColor = RingColor = &gMsColorTable.ButtonLinkNormalColor;
          // Clicking the button should darken it.
          SelectColor = &gMsColorTable.ButtonLinkSelectColor;
          // Standard text is black for the light background.
          TextColor = &gMsColorTable.ButtonLinkTextNormalColor;
          // Clicking the button changes to white text for the darker background.
          SelectTextColor = &gMsColorTable.ButtonLinkTextSelectColor;

          ButtonWidth  = MsUiScaleByTheme (460);            // Width - TODO.
          ButtonHeight = MsUiScaleByTheme (100);            // Height - TODO.
        }

        Button  *B = new_Button (
                       (GridScope ? 0 : (UINT32)MenuOption->Col),
                       (GridScope ? 0 : (UINT32)MenuOption->Row),
                       ButtonWidth,
                       ButtonHeight,
                       &FontInfo,
                       NormalColor,                       // Normal.
                       HoverColor,                        // Hover.
                       SelectColor,                       // Select.
                       &gMsColorTable.ButtonGrayoutColor, // GrayOut
                       RingColor,                         // Button ring.
                       TextColor,                         // Normal text.
                       SelectTextColor,                   // Select text.
                       Description,
                       MenuOption
                       );

        ASSERT (NULL != B);
        if (NULL == B) {
          Status = EFI_OUT_OF_RESOURCES;
          break;
        }

        MenuOption->BaseControl = &B->Base;

        if (MenuOption->GrayOut == TRUE) {
          B->Base.SetControlState (B, GRAYED);
        }

        // Add the control to either the current grid or the canvas, depending on scope.
        //
        if (TRUE == GridScope) {
          // Add the control to the grid.
          //
          Status = LocalGrid->AddControl (
                                LocalGrid,
                                TRUE,                      // Highlightable.
                                FALSE,                     // Not invisible.
                                CurrentRow,
                                CurrentColumn,
                                (VOID *)B
                                );

          // Move to the next grid column.
          //
          CurrentColumn++;
        } else {
          // Add the control to the canvas.
          //
          Status = LocalCanvas->AddControl (
                                  LocalCanvas,
                                  TRUE,                      // Highlightable.
                                  FALSE,                     // Not invisible.
                                  (VOID *)B
                                  );

          // Increment the vertical position by the size of the button.
          //
          B->Base.GetControlBounds (
                    B,
                    &ControlRect
                    );

          // Move the next control's origin down the page by the height of the current control.
          //
          OrigY += SWM_RECT_HEIGHT (ControlRect);
        }

        break;
      }

      case EFI_IFR_ONE_OF_OP:           // Listbox.
      case EFI_IFR_ORDERED_LIST_OP:     // Ordered List.
      {
        UINT32                   OptionCount;
        LIST_ENTRY               *OLLink;
        DISPLAY_QUESTION_OPTION  *OneOfOption;
        UIT_LB_CELLDATA          *OptionList;
        UINT32                   Flags = 0;

        UINT32  ListWidth;
        Label   *L;

        EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *TextColor = &gMsColorTable.LabelTextNormalColor;
        SWM_RECT                       LabelRect;

        SWM_RECT_INIT (
          LabelRect,
          (GridScope ? 0 : (UINT32)MenuOption->Col),
          (GridScope ? 0 : (UINT32)MenuOption->Row),
          CanvasRightLimit,
          CanvasBottomLimit
          );

        if (MenuOption->GrayOut) {
          TextColor = &gMsColorTable.LabelTextGrayoutColor;
        }

        L = new_Label (
              LabelRect.Left,
              LabelRect.Top,
              SWM_RECT_WIDTH (LabelRect),
              SWM_RECT_HEIGHT (LabelRect),
              &FontInfo,
              TextColor,                               // Foreground (text) color.
              &gMsColorTable.LabelTextBackgroundColor, // Background color.
              Description
              );

        ASSERT (NULL != L);
        if (NULL == L) {
          Status = EFI_OUT_OF_RESOURCES;
          break;
        }

        MenuOption->BaseControl = &L->Base;
        // Add the control to either the current grid or the canvas, depending on scope.
        //
        if (TRUE == GridScope) {
          // Add the control to the grid.
          //
          Status = LocalGrid->AddControl (
                                LocalGrid,
                                FALSE,                             // Labels won't be highlighted.
                                FALSE,                             // Not invisible.
                                CurrentRow,
                                CurrentColumn,
                                (VOID *)L
                                );

          // Move to the next grid column.
          //
          CurrentColumn++;
        } else {
          // Add the control to the canvas.
          //
          Status = LocalCanvas->AddControl (
                                  LocalCanvas,
                                  FALSE,                             // Labels won't be highlighted.
                                  FALSE,                             // Not invisible.
                                  (VOID *)L
                                  );

          // Increment the vertical position by the size of the label.
          //
          L->Base.GetControlBounds (
                    L,
                    &ControlRect
                    );

          // Move the next control's origin down the page by the height of the current control.
          //
          OrigY          += SWM_RECT_HEIGHT (ControlRect) + 40;        // TODO - appropriate buffer between listbox label and listbox?
          MenuOption->Row = OrigY;
        }

        // Check whether there was an error or are Options associated with the OrderedList.
        //
        if (EFI_ERROR (Status) || IsListEmpty (&Statement->OptionListHead)) {
          break;
        }

        // Count the number of option entries.
        //
        OptionCount = 0;
        OLLink      = GetFirstNode (&Statement->OptionListHead);
        while (!IsNull (&Statement->OptionListHead, OLLink)) {
          OptionCount++;
          OLLink = GetNextNode (&Statement->OptionListHead, OLLink);
        }

        // Allocate space for an array of pointers to the option strings.
        //
        OptionList = AllocateZeroPool ((OptionCount + 1) * sizeof (UIT_LB_CELLDATA));              // Includes NULL terminator.
        ASSERT (NULL != OptionList);
        if (NULL == OptionList) {
          Status = EFI_OUT_OF_RESOURCES;
          break;
        }

        if (EFI_IFR_ORDERED_LIST_OP == Statement->OpCode->OpCode) {
          Flags |= UIT_LISTBOX_FLAGS_ORDERED_LIST;
          if (((EFI_IFR_ORDERED_LIST *)Statement->OpCode)->Flags & EMBEDDED_CHECKBOX) {
            Flags |= UIT_LISTBOX_FLAGS_CHECKBOX;
          }

          if (((EFI_IFR_ORDERED_LIST *)Statement->OpCode)->Flags & EMBEDDED_DELETE) {
            Flags |= UIT_LISTBOX_FLAGS_ALLOW_DELETE;
          }
        }

        // Build a list of pointers to option strings.
        //
        OptionCount = 0;
        OLLink      = GetFirstNode (&Statement->OptionListHead);
        while (!IsNull (&Statement->OptionListHead, OLLink)) {
          OneOfOption                      = DISPLAY_QUESTION_OPTION_FROM_LINK (OLLink);
          OptionList[OptionCount].CellText = (CHAR16 *)HiiGetString (FormData->HiiHandle, OneOfOption->OptionOpCode->Option, NULL);
          if (Flags & UIT_LISTBOX_FLAGS_CHECKBOX) {
            if (OneOfOption->OptionOpCode->Value.u32 & ORDERED_LIST_CHECKBOX_VALUE_32) {
              OptionList[OptionCount].CheckBoxSelected = TRUE;
            } else {
              OptionList[OptionCount].CheckBoxSelected = FALSE;
            }
          }

          if (Flags & UIT_LISTBOX_FLAGS_ALLOW_DELETE) {
            if (OneOfOption->OptionOpCode->Value.u32 & ORDERED_LIST_ALLOW_DELETE_VALUE_32) {
              OptionList[OptionCount].TrashcanEnabled = TRUE;
            } else {
              OptionList[OptionCount].TrashcanEnabled = FALSE;
            }
          }

          OptionCount++;
          OLLink = GetNextNode (&Statement->OptionListHead, OLLink);
        }

        // TODO - allocated buffer needs to be cleaned up when we exit but not before the listbox is done with it...

        // Create a listbox with all the options.
        //
        FontInfo.FontStyle = EFI_HII_FONT_STYLE_NORMAL;
        FontInfo.FontSize  = MsUiGetStandardFontHeight ();
        ListWidth          = MsUiScaleByTheme ((EFI_IFR_ORDERED_LIST_OP == Statement->OpCode->OpCode) ? 1000 : 800);
        ListBox  *LB = new_ListBox (
                         (GridScope ? 0 : (UINT32)MenuOption->Col),
                         (GridScope ? 0 : (UINT32)MenuOption->Row),
                         ListWidth,
                         MsUiScaleByTheme (100),
                         Flags,
                         &FontInfo,
                         MsUiScaleByTheme (50),                                  // Cell text X offset
                         &gMsColorTable.ListBoxNormalColor,                      // Normal
                         &gMsColorTable.ListBoxHoverColor,                       // Hover
                         &gMsColorTable.ListBoxSelectColor,                      // Select
                         &gMsColorTable.ListBoxGrayoutColor,                     // Grayed
                         OptionList,
                         MenuOption
                         );

        ASSERT (NULL != LB);
        if (NULL == LB) {
          Status = EFI_OUT_OF_RESOURCES;
          break;
        }

        MenuOption->BaseControl = &LB->Base;

        if (MenuOption->GrayOut == TRUE) {
          LB->Base.SetControlState (LB, GRAYED);
        }

        // Add the control to either the current grid or the canvas, depending on scope.
        //
        if (TRUE == GridScope) {
          // Add the control to the grid.
          //
          Status = LocalGrid->AddControl (
                                LocalGrid,
                                TRUE,                                // Listbox can be highlighted (focus).
                                FALSE,                               // Not invisible.
                                CurrentRow,
                                CurrentColumn,
                                (VOID *)LB
                                );

          // Move to the next grid column.
          //
          CurrentColumn++;
        } else {
          // Add the control to the canvas.
          //
          Status = LocalCanvas->AddControl (
                                  LocalCanvas,
                                  TRUE,                                // Listbox can be highlighted (focus).
                                  FALSE,                               // Not invisible.
                                  (VOID *)LB
                                  );

          // Increment the vertical position by the size of the listbox.
          //
          LB->Base.GetControlBounds (
                     LB,
                     &ControlRect
                     );

          OrigY += SWM_RECT_HEIGHT (ControlRect);
        }

        break;
      }

      case EFI_IFR_STRING_OP:       // StringOp
      {
        EFI_IFR_STRING  *String = (EFI_IFR_STRING *)Statement->OpCode;
        SWM_RECT        LabelRect;

        SWM_RECT_INIT (
          LabelRect,
          (GridScope ? 0 : (UINT32)MenuOption->Col),
          (GridScope ? 0 : (UINT32)MenuOption->Row),
          CanvasRightLimit,
          CanvasBottomLimit
          );

        Label  *L = new_Label (
                      LabelRect.Left,
                      LabelRect.Top,
                      SWM_RECT_WIDTH (LabelRect),
                      SWM_RECT_HEIGHT (LabelRect),
                      &FontInfo,
                      ((MsUiGetLargeFontHeight () == FontInfo.FontSize) ? &gMsColorTable.LabelTextLargeColor : &gMsColorTable.LabelTextNormalColor), // TODO - Foreground (text) color.
                      &gMsColorTable.LabelTextBackgroundColor,                                                                                       // Background color.
                      Description
                      );

        ASSERT (NULL != L);
        if (NULL == L) {
          Status = EFI_OUT_OF_RESOURCES;
          break;
        }

        // Add the control to either the current grid or the canvas, depending on scope.
        //
        if (TRUE == GridScope) {
          // Add the control to the grid.
          //
          Status = LocalGrid->AddControl (
                                LocalGrid,
                                FALSE,                                // Labels won't be highlighted.
                                FALSE,                                // Not invisible.
                                CurrentRow,
                                CurrentColumn,
                                (VOID *)L
                                );

          // Move to the next grid column.
          //
          CurrentColumn++;
        } else {
          // Add the control to the canvas.
          //
          Status = LocalCanvas->AddControl (
                                  LocalCanvas,
                                  FALSE,                                // Labels won't be highlighted.
                                  FALSE,                                // Not invisible.
                                  (VOID *)L
                                  );

          // Increment the vertical position by the size of the label.
          //
          L->Base.GetControlBounds (
                    L,
                    &ControlRect
                    );

          OrigY          += SWM_RECT_HEIGHT (ControlRect);
          MenuOption->Row = OrigY;
        }

        // Check for error - exit.
        //
        if (EFI_ERROR (Status)) {
          break;
        }

        // TODO
        FontInfo.FontSize = MsUiGetFixedFontHeight ();             // Cell height for fixed font.

        EditBox  *E = new_EditBox (
                        (GridScope ? 0 : (UINT32)MenuOption->Col),
                        (GridScope ? 0 : (UINT32)MenuOption->Row),
                        String->MaxSize,
                        UIT_EDITBOX_TYPE_SELECTABLE,
                        &FontInfo,
                        &gMsColorTable.EditBoxNormalColor,
                        &gMsColorTable.EditBoxTextColor,
                        &gMsColorTable.EditBoxGrayoutColor,
                        &gMsColorTable.EditBoxTextGrayoutColor,
                        &gMsColorTable.EditBoxSelectColor,
                        //      (String->Question.Flags & EFI_IFR_FLAG_READ_ONLY) ? (CHAR16 *) Statement->CurrentValue.Buffer : NULL,
                        (CHAR16 *)Statement->CurrentValue.Buffer,
                        //      NULL,
                        MenuOption
                        );
        ASSERT (NULL != E);
        if (NULL == E) {
          Status = EFI_OUT_OF_RESOURCES;
          break;
        }

        MenuOption->BaseControl = &E->Base;

        // Add the control to either the current grid or the canvas, depending on scope.
        //
        if (TRUE == GridScope) {
          // Add the control to the grid.
          //
          Status = LocalGrid->AddControl (
                                LocalGrid,
                                TRUE,                              // EditBox will be highlighted.
                                FALSE,                             // Not invisible.
                                CurrentRow,
                                CurrentColumn,
                                (VOID *)E
                                );

          // Move to the next grid column.
          //
          CurrentColumn++;
        } else {
          // Add the control to the canvas.
          //
          Status = LocalCanvas->AddControl (
                                  LocalCanvas,
                                  TRUE,                              // EditBox will be highlighted.
                                  FALSE,                             // Not invisible.
                                  (VOID *)E
                                  );

          // Increment the vertical position by the size of the label.
          //
          E->Base.GetControlBounds (
                    E,
                    &ControlRect
                    );

          OrigY += SWM_RECT_HEIGHT (ControlRect);
        }

        if (String->Question.Flags & EFI_IFR_FLAG_READ_ONLY) {
          E->Base.SetControlState (E, GRAYED);
        }

        break;
      }

      case EFI_IFR_TEXT_OP:         // Text.
      {
        EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *TextColor = &gMsColorTable.LabelTextNormalColor;        // DCR (MsUiGetLargeFontHeight () == FontInfo.FontSize ? &gMsColorTable.LabelTextLargeColor : &gMsColorTable.LabelTextNormalColor;
        SWM_RECT                       LabelRect;

        SWM_RECT_INIT (
          LabelRect,
          (GridScope ? 0 : (UINT32)MenuOption->Col),
          (GridScope ? 0 : (UINT32)MenuOption->Row),
          CanvasRightLimit,
          CanvasBottomLimit
          );

        if (0 == StrnCmp (Description, L"\\fc!Red!", 8)) {
          Description += StrLen (L"\\fc!Red!");
          TextColor    = &gMsColorTable.LabelTextRedColor;
        }

        if (MenuOption->GrayOut) {
          TextColor = &gMsColorTable.LabelTextGrayoutColor;
        }

        Label  *L = new_Label (
                      LabelRect.Left,
                      LabelRect.Top,
                      SWM_RECT_WIDTH (LabelRect),
                      SWM_RECT_HEIGHT (LabelRect),
                      &FontInfo,
                      TextColor,                               // TODO - Foreground (text) color.
                      &gMsColorTable.LabelTextBackgroundColor, // Background color.
                      Description
                      );

        ASSERT (NULL != L);
        if (NULL == L) {
          Status = EFI_OUT_OF_RESOURCES;
          break;
        }

        MenuOption->BaseControl = &L->Base;

        // Add the control to either the current grid or the canvas, depending on scope.
        //
        if (TRUE == GridScope) {
          // Add the control to the grid.
          //
          Status = LocalGrid->AddControl (
                                LocalGrid,
                                FALSE,                             // Labels won't be highlighted.
                                FALSE,                             // Not invisible.
                                CurrentRow,
                                CurrentColumn,
                                (VOID *)L
                                );

          // Move to the next grid column.
          //
          CurrentColumn++;
        } else {
          // Add the control to the canvas.
          //
          Status = LocalCanvas->AddControl (
                                  LocalCanvas,
                                  FALSE,                             // Labels won't be highlighted.
                                  FALSE,                             // Not invisible.
                                  (VOID *)L
                                  );

          // Increment the vertical position by the size of the label.
          //
          L->Base.GetControlBounds (
                    L,
                    &ControlRect
                    );

          OrigY += SWM_RECT_HEIGHT (ControlRect);
        }

        break;
      }

      case EFI_IFR_CHECKBOX_OP:     // Checkbox.
      {
        SWM_RECT  LabelRect;

        SWM_RECT_INIT (
          LabelRect,
          (GridScope ? 0 : (UINT32)MenuOption->Col),
          (GridScope ? 0 : (UINT32)MenuOption->Row),
          CanvasRightLimit,
          CanvasBottomLimit
          );

        Label  *L = new_Label (
                      LabelRect.Left,
                      LabelRect.Top,
                      SWM_RECT_WIDTH (LabelRect),
                      SWM_RECT_HEIGHT (LabelRect),
                      &FontInfo,
                      (MsUiGetLargeFontHeight () == FontInfo.FontSize ? &gMsColorTable.LabelTextLargeColor : &gMsColorTable.LabelTextNormalColor), // TODO - Foreground (text) color.
                      &gMsColorTable.LabelTextBackgroundColor,                                                                                     // Background color.
                      Description
                      );

        ASSERT (NULL != L);
        if (NULL == L) {
          Status = EFI_OUT_OF_RESOURCES;
          break;
        }

        // Add the control to either the current grid or the canvas, depending on scope.
        //
        if (TRUE == GridScope) {
          // Add the control to the grid.
          //
          Status = LocalGrid->AddControl (
                                LocalGrid,
                                FALSE,                                // Labels won't be highlighted.
                                FALSE,                                // Not invisible.
                                CurrentRow,
                                CurrentColumn,
                                (VOID *)L
                                );

          // Move to the next grid column.
          //
          CurrentColumn++;
        } else {
          // Add the control to the canvas.
          //
          Status = LocalCanvas->AddControl (
                                  LocalCanvas,
                                  FALSE,                                // Labels won't be highlighted.
                                  FALSE,                                // Not invisible.
                                  (VOID *)L
                                  );

          // Increment the vertical position by the size of the label.
          //
          L->Base.GetControlBounds (
                    L,
                    &ControlRect
                    );

          OrigY          += SWM_RECT_HEIGHT (ControlRect);
          MenuOption->Row = OrigY;
        }

        // Check for error - exit.
        //
        if (EFI_ERROR (Status)) {
          break;
        }

        // TODO
        FontInfo.FontSize = MsUiGetSmallFontHeight ();             // Cell height for 24pt font.

        // Create a toggle switch to represent the checkbox.
        //
        ToggleSwitch  *S = new_ToggleSwitch (
                             (GridScope ? 0 : (UINT32)MenuOption->Col),
                             (GridScope ? 0 : (UINT32)MenuOption->Row),
                             MsUiScaleByTheme (160),                                             // Width  - TODO.
                             MsUiScaleByTheme (75),                                              // Height - TODO.
                             &FontInfo,
                             gMsColorTable.ToggleSwitchOnColor,                              // On.
                             gMsColorTable.ToggleSwitchOffColor,                             // Off.
                             gMsColorTable.ToggleSwitchHoverColor,                           // Hover.
                             gMsColorTable.ToggleSwitchGrayoutColor,                         // Gray
                             L"On ",
                             L"Off",
                             MenuOption->ThisTag->CurrentValue.Value.b,                         // Initial switch value is based on current setting.
                             MenuOption
                             );

        ASSERT (NULL != S);
        if (NULL == S) {
          Status = EFI_OUT_OF_RESOURCES;
          break;
        }

        MenuOption->BaseControl = &S->Base;

        if (MenuOption->GrayOut == TRUE) {
          S->Base.SetControlState (S, GRAYED);
        }

        // Add the control to either the current grid or the canvas, depending on scope.
        //
        if (TRUE == GridScope) {
          // Add the control to the grid.
          //
          Status = LocalGrid->AddControl (
                                LocalGrid,
                                TRUE,                              // Support highlighting.
                                FALSE,                             // Not invisible.
                                CurrentRow,
                                CurrentColumn,
                                (VOID *)S
                                );

          // Move to the next grid column.
          //
          CurrentColumn++;
        } else {
          // Add the control to the canvas.
          //
          Status = LocalCanvas->AddControl (
                                  LocalCanvas,
                                  TRUE,                              // Support highlighting.
                                  FALSE,                             // Not invisible.
                                  (VOID *)S
                                  );

          // Increment the vertical position by the size of the toggle switch.
          //
          S->Base.GetControlBounds (
                    S,
                    &ControlRect
                    );

          OrigY += SWM_RECT_HEIGHT (ControlRect);
        }

        break;
      }

      default:
        Status = EFI_INVALID_PARAMETER;
        DEBUG ((DEBUG_WARN, "WARN [DE]: Unrecognized menu OpCode (0x%x).\r\n", (UINT8)Statement->OpCode->OpCode));
        break;
    }

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "WARN [DE]: Error processing OpCode (0x%x). Code=%r\r\n", (UINT8)Statement->OpCode->OpCode, Status));
    }
  }

  // If we terminated the loop early due to an error, exit.
  //
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  // If there is a canvas from earlier, see what we can recycle from it then free it.
  // Do this before redrawing the controlls
  if (NULL != mPrivateData.PreviousCanvas) {
    if (FALSE == mStatementLayoutIsChanged) {
      Status = LocalCanvas->Base.CopySettings (
                                   LocalCanvas,
                                   mPrivateData.PreviousCanvas
                                   );
      if (EFI_ERROR (Status)) {
        mStatementLayoutIsChanged = TRUE;
      }
    }

    delete_Canvas (mPrivateData.PreviousCanvas);
    mPrivateData.PreviousCanvas = NULL;
  }

  // Now that all the child controls have been created, paint the canvas to draw them all.
  //
  if ((TRUE == mStatementLayoutIsChanged) || (TRUE == mControlsRequireRefresh)) {
    LocalCanvas->Base.Draw (
                        LocalCanvas,
                        FALSE,
                        NULL,
                        NULL
                        );
    mControlsRequireRefresh = FALSE;
    mRefreshOnEvent         = FALSE;
  }

  if (TRUE == mRefreshOnEvent) {
    for (Link = GetFirstNode (&gMenuOption); (EFI_SUCCESS == Status) && !IsNull (&gMenuOption, Link); Link = GetNextNode (&gMenuOption, Link)) {
      MenuOption = MENU_OPTION_FROM_LINK (Link);
      if (NULL != MenuOption->BaseControl) {
        // We are only refreshing an EDITBOX that is grayed out.  It is the only one with REFRESH INTERVAL 1

        if (MenuOption->BaseControl->ControlType == EDITBOX) {
          EditBox  *E = (EditBox *)MenuOption->BaseControl;
          if (GRAYED == E->Base.GetControlState (E)) {
            E->SetCurrentTextString (E, (CHAR16 *)MenuOption->ThisTag->CurrentValue.Buffer);
          }
        }
      }
    }
  }

  // Return the canvas to the caller.
  //
  *FormCanvas = LocalCanvas;

Exit:

  return Status;
}

EFI_STATUS
UiDisplayMenu (
  IN FORM_DISPLAY_ENGINE_FORM  *FormData
  )
{
  EFI_STATUS                     Status                  = EFI_SUCCESS;
  DISPLAY_ENGINE_SHARED_STATE    *MasterFrameSharedState = (DISPLAY_ENGINE_SHARED_STATE *)(UINTN)PcdGet64 (PcdCurrentPointerState);
  SWM_INPUT_STATE                InputState;
  OBJECT_STATE                   ControlState = NORMAL;
  UI_EVENT_TYPE                  EventType;
  CHAR16                         *OptionString = NULL;
  Canvas                         *FormCanvas   = NULL;
  UI_MENU_OPTION                 *MenuOption;
  FORM_DISPLAY_ENGINE_STATEMENT  *Statement;
  static BOOLEAN                 FormHasKeyFocus = FALSE;
  VOID                           *Context;
  LB_RETURN_DATA                 ReturnData;
  UINTN                          i;
  UINTN                          Timeout;
  UINTN                          EventNum;

  ZeroMem (&InputState, sizeof (InputState));

  // Verify that the shared buffer between FrontPage and the Display Engine exists.
  //
  ASSERT (NULL != FormData && NULL != MasterFrameSharedState);
  if ((NULL == FormData) || (NULL == MasterFrameSharedState)) {
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  // Compute the Master Frame width and TitleBar height.
  //
  mTitleBarHeight    = ((mGop->Mode->Info->VerticalResolution * FP_TBAR_HEIGHT_PERCENT) / 100);
  mMasterFrameWidth  = ((mGop->Mode->Info->HorizontalResolution * FP_MFRAME_WIDTH_PERCENT) / 100);
  mMasterFrameHeight = (mGop->Mode->Info->VerticalResolution - mTitleBarHeight);

  // Performance optimization - if there isn't a previous form, it's the first time we're displaying something.  Fill
  // the canvas with the default background color (slow) and hereafter the canvas will clear only the control areas it owns (faster).
  //
  if (TRUE == mStatementLayoutIsChanged) {
    if (NULL == mPrivateData.PreviousCanvas) {
      // For the first time through, fill the canvas with the defined background color (slow).
      //
      mGop->Blt (
              mGop,
              &gMsColorTable.FormCanvasBackgroundColor,
              EfiBltVideoFill,
              0,
              0,
              mMasterFrameWidth,
              mTitleBarHeight,
              (mGop->Mode->Info->HorizontalResolution - mMasterFrameWidth),
              (mGop->Mode->Info->VerticalResolution - mTitleBarHeight),
              0
              );
    } else {
      mPrivateData.PreviousCanvas->ClearCanvas (mPrivateData.PreviousCanvas);
    }
  }

  // Create a new canvas and child controls for the current HII form.
  //
  Status = CreateFormControls (
             FormData,
             &FormCanvas
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR [DE] - Failed to create form UI controls.\r\n"));
    goto Exit;
  }

  // Keep a pointer to the new form canvas for later.
  //
  mPrivateData.PreviousCanvas = FormCanvas;

  // Reset the "close form" request flag.
  //
  MasterFrameSharedState->CloseFormRequest = FALSE;

  // By default we'll show Top Menu highlighting.  This will be cleared when touch/mouse are used.
  //
  MasterFrameSharedState->ShowTopMenuHighlight = TRUE;

  // Wait for user input or a form close request.
  //  UIEventTimeOut
  EFI_EVENT      WaitEvents[3];
  UI_EVENT_TYPE  EventTypes[3];

  // Keyboard.
  WaitEvents[0] = mSimpleTextInEx->WaitForKeyEx;
  EventTypes[0] = UIEventKey;

  // Touch/Mouse.
  WaitEvents[1] = mPointerProtocol->WaitForInput;
  EventTypes[1] = UIEventTouch;

  EventNum = 2;

  // FormRefreshEvent
  if (gFormData->FormRefreshEvent != NULL) {
    WaitEvents[2] = gFormData->FormRefreshEvent;
    EventTypes[2] = UIEventDriver;
    EventNum++;
  }

  // Start user input event loop.  The Master Frame controls when we exit the loop and thus the forms browser
  // returns control to the FrontPage.
  //
  while (FALSE == MasterFrameSharedState->CloseFormRequest) {
    // Wait for an input event from the user.
    //
    Timeout         = FormExitTimeout (gFormData);
    Status          = mSWMProtocol->WaitForEvent (EventNum, WaitEvents, &i, Timeout, mRefreshOnEvent);
    mRefreshOnEvent = FALSE;
    if (!EFI_ERROR (Status)) {
      if (i == EventNum) {
        EventType = UIEventTimeOut;
      } else {
        EventType = EventTypes[i];
      }

      // Process the input event based on type.
      //
      switch (EventType) {
        case UIEventKey:            // Key-press event.
        {
          InputState.InputType = SWM_INPUT_TYPE_KEY;

          Status = mSimpleTextInEx->ReadKeyStrokeEx (
                                      mSimpleTextInEx,
                                      &InputState.State.KeyState
                                      );
          break;
        }
        case UIEventTouch:          // Touch/mouse event.
        {
          static BOOLEAN  WatchForFirstFingerUpEvent = FALSE;
          BOOLEAN         WatchForFirstFingerUpEvent2;

          InputState.InputType = SWM_INPUT_TYPE_TOUCH;

          Status = mPointerProtocol->GetState (
                                       mPointerProtocol,
                                       &InputState.State.TouchState
                                       );

          // TODO:  Remove this filter when implementing hover.
          //        Also requires optimizing the redraw routines.
          // Filter out all extra pointer moves with finger UP.
          WatchForFirstFingerUpEvent2 = WatchForFirstFingerUpEvent;
          WatchForFirstFingerUpEvent  = SWM_IS_FINGER_DOWN (InputState.State.TouchState);
          if (!SWM_IS_FINGER_DOWN (InputState.State.TouchState) && (FALSE == WatchForFirstFingerUpEvent2)) {
            continue;
          }

          break;
        }
        case UIEventDriver:
          gUserInput->Action = BROWSER_ACTION_NONE;
          mRefreshOnEvent    = TRUE;
          goto Exit;

          break;

        default:
          continue;
      }
    }

    // Unable to retrieve key or touch/mouse event state - go back to waiting.
    //
    if (EFI_ERROR (Status)) {
      continue;
    }

    // Check whether the event should be forwarded to the Master Frame instead of the form canvas.
    //
    if (((SWM_INPUT_TYPE_TOUCH == InputState.InputType) && (FALSE == FormHasKeyFocus) && (InputState.State.TouchState.CurrentX < mMasterFrameWidth)) ||
        ((SWM_INPUT_TYPE_KEY == InputState.InputType) && (FALSE == FormHasKeyFocus) && (CHAR_TAB != InputState.State.KeyState.Key.UnicodeChar)))
    {
      // If we're displaying the Top Menu highlight and the TAB or SHIFT+TAB is pressed, toggle the tab order highlight state.
      //
      // If we are processing a touch event, clear the Top Menu highlight since we're navigating by touch and don't need the keyboard highlight.
      //
      if (SWM_INPUT_TYPE_TOUCH == InputState.InputType) {
        MasterFrameSharedState->ShowTopMenuHighlight = FALSE;
      } else if (SWM_INPUT_TYPE_KEY == InputState.InputType) {
        MasterFrameSharedState->ShowTopMenuHighlight = TRUE;
      }

      gUserInput->Action = BROWSER_ACTION_FORM_EXIT;

      // If the event wasn't a key press, go back to waiting.  Since the key press could be a tab/shift-tab that moves
      // highlight from the Master Frame to the form canvas and vice versa, we need to process that next.
      // Copy the input state information into the shared context buffer (shared between FP and display engine).
      //
      MasterFrameSharedState->NotificationType = USERINPUT;
      CopyMem (&MasterFrameSharedState->InputState, &InputState, sizeof (SWM_INPUT_STATE));

      // Signal FP that there's data for it to process.
      //
      gBS->SignalEvent (mMasterFrameNotifyEvent);
      continue;
    }

    // Process special actions based on input event type.
    //
    switch (InputState.InputType) {
      case SWM_INPUT_TYPE_KEY:
      {
        EFI_KEY_DATA  *pKey = &InputState.State.KeyState;

        // If user pressed SHIFT-TAB/TAB, move the highlight to the next control.
        //

        if (CHAR_TAB == pKey->Key.UnicodeChar) {
          MasterFrameSharedState->ShowTopMenuHighlight = FALSE;
          if (0 != (pKey->KeyState.KeyShiftState & (EFI_LEFT_SHIFT_PRESSED | EFI_RIGHT_SHIFT_PRESSED))) {
            // If the Master Frame has focus (form canvas doesn't) and SHIFT-TAB is pressed, focus switches to the form canvas.
            //
            if (FALSE == FormHasKeyFocus) {
              FormHasKeyFocus = TRUE;
            }

            if (TRUE == FormHasKeyFocus) {
              // *** Send to the Form Canvas ***
              //
              Status = FormCanvas->MoveHighlight (
                                     FormCanvas,
                                     FALSE
                                     );

              // If we tab to the end of the form canvas' controls, focus should return to the Master Frame and since we're
              // navigating with the keyboard, it should enable menu listbox highlight.
              //
              if (EFI_NOT_FOUND == Status) {
                FormHasKeyFocus                              = FALSE;
                MasterFrameSharedState->ShowTopMenuHighlight = TRUE;
                gUserInput->Action                           = BROWSER_ACTION_FORM_EXIT;
              }
            }
          } else {
            // If the Master Frame has focus (form canvas doesn't) and TAB is pressed, focus switches to the form canvas.
            //
            if (FALSE == FormHasKeyFocus) {
              FormHasKeyFocus = TRUE;
            }

            if (TRUE == FormHasKeyFocus) {
              // *** Send to the Form Canvas ***
              //
              Status = FormCanvas->MoveHighlight (
                                     FormCanvas,
                                     TRUE
                                     );

              // If we tab to the end of the form canvas' controls, focus should return to the Master Frame and since we're
              // navigating with the keyboard, it should enable menu listbox highlight.
              //
              if (EFI_NOT_FOUND == Status) {
                FormHasKeyFocus                              = FALSE;
                MasterFrameSharedState->ShowTopMenuHighlight = TRUE;
                gUserInput->Action                           = BROWSER_ACTION_FORM_EXIT;
              }
            }
          }

          // Signal to the Top Menu to refresh to show the highlight.
          //
          MasterFrameSharedState->NotificationType = REDRAW;
          CopyMem (&MasterFrameSharedState->InputState, &InputState, sizeof (SWM_INPUT_STATE));

          // Signal FP that there's data for it to process.
          //
          gBS->SignalEvent (mMasterFrameNotifyEvent);
        }

        break;
      }

      case SWM_INPUT_TYPE_TOUCH:
      {
        if (InputState.State.TouchState.CurrentX >= mMasterFrameWidth) {
          FormHasKeyFocus = TRUE;
        } else {
          FormHasKeyFocus = FALSE;
        }

        MasterFrameSharedState->ShowTopMenuHighlight = FALSE;

        gUserInput->Action = BROWSER_ACTION_FORM_EXIT;

        // Signal to the Top Menu to refresh to show the highlight.
        //
        MasterFrameSharedState->NotificationType = REDRAW;
        CopyMem (&MasterFrameSharedState->InputState, &InputState, sizeof (SWM_INPUT_STATE));

        // Signal FP that there's data for it to process.
        //
        gBS->SignalEvent (mMasterFrameNotifyEvent);

        break;
      }

      default:
        break;
    }

    // Refresh the canvas with the input state - this may cause of the child UI controls to report a select state that will
    // need to be processed.
    //
    Context      = NULL;
    ControlState = FormCanvas->Base.Draw (
                                      FormCanvas,
                                      FALSE,                   // Don't highlight the canvas.
                                      &InputState,
                                      &Context
                                      );

    // If one of the child controls was selected and we have selection context, process it.
    //
    if ((SELECT == ControlState) && (NULL != Context)) {
      MenuOption = (UI_MENU_OPTION *)Context;

      Statement = MenuOption->ThisTag;

      switch (Statement->OpCode->OpCode) {
        case EFI_IFR_STRING_OP:
        {
          EditBox         *E = NULL;
          CHAR16          *NewString;
          CHAR16          *ReturnValue;
          UINTN           ReturnSize;
          EFI_IFR_STRING  *String = (EFI_IFR_STRING *)Statement->OpCode;

          Status = FormCanvas->GetSelectedControl (
                                 FormCanvas,
                                 (VOID **)&E
                                 );

          if ((EFI_SUCCESS == Status) && (NULL != E)) {
            NewString = E->GetCurrentTextString (E);
            if (NULL != NewString) {
              ReturnSize = (StrnLenS (NewString, String->MaxSize) + 1) * sizeof (CHAR16);              // +1 for a max string + NULL
              if (ReturnSize > 0) {
                ReturnValue = AllocateCopyPool (ReturnSize, NewString);

                if (NULL != ReturnValue) {
                  //
                  // AllocateCopyPool will copy the NULL from a string shorter than String->MaxSize;
                  // A longer string will just have the extra character.  Setting the last CHAR16
                  // in the buffer will insure there is a NULL terminator.
                  ReturnValue[(ReturnSize/sizeof (CHAR16))-1] = '\0';                    // Set the last character in the buffer to NULL
                  gUserInput->InputValue.Buffer               = (UINT8 *)ReturnValue;
                  gUserInput->InputValue.BufferLen            = (UINT16)ReturnSize;
                  gUserInput->InputValue.Value.string         = HiiSetString (gFormData->HiiHandle, 0, ReturnValue, NULL);
                  gUserInput->Action                          = 0;
                  gUserInput->SelectedStatement               = Statement;
                  E->ClearEditBox (E);

                  // Exit the input event processing loop to process the callback and save state.
                  //
                  MasterFrameSharedState->CloseFormRequest = TRUE;
                }
              }
            }
          }

          break;
        }

        case EFI_IFR_TEXT_OP:
        case EFI_IFR_REF_OP:
        case EFI_IFR_ACTION_OP:
        case EFI_IFR_RESET_BUTTON_OP:
        case EFI_IFR_CHECKBOX_OP:
        {
          // Editable Questions: oneof, ordered list, checkbox, numeric, string, password
          //
          Status = ProcessOptions (MenuOption, TRUE, &OptionString, TRUE);

          if (OptionString != NULL) {
            FreePool (OptionString);
          }

          // Capture button press as the last action so we can restore the normal button color when we return.
          //
          if ((EFI_IFR_CHECKBOX_OP == Statement->OpCode->OpCode) ||
              (EFI_IFR_ACTION_OP == Statement->OpCode->OpCode))
          {
            mControlsRequireRefresh = TRUE;
          }

          // Capture the selected statement.
          //
          gUserInput->Action            = 0;
          gUserInput->SelectedStatement = Statement;

          // Exit the input event processing loop to process the callback and save state.
          //
          MasterFrameSharedState->CloseFormRequest = TRUE;
          break;
        }

        case EFI_IFR_ONE_OF_OP:
        {
          // The user selected a cell in an ordered list.  Since the ordered list is a "tap to promote" style listbox, determine
          // which cell was selected (and thus promoted in order) so we can mirror that in the options list.
          //
          ListBox  *LB = NULL;

          Status = FormCanvas->GetSelectedControl (
                                 FormCanvas,
                                 (VOID **)&LB
                                 );
          // If the canvas gave us the selected control, we know it's a "tap to promote" listbox because of the "ordered list" opcode.
          // Ask the listbox which cell was selected.  This is the cell that was promoted and we need to mirror the promotion in the
          // menu options list.
          //
          if ((EFI_SUCCESS == Status) && (NULL != LB)) {
            Status = LB->GetSelectedCellIndex (
                           LB,
                           &ReturnData
                           );
            // Process the current order.
            //
            if ((EFI_SUCCESS == Status) &&
                (!IsListEmpty (&Statement->OptionListHead)))
            {
              UINT32                   OptionNumber = 0;
              DISPLAY_QUESTION_OPTION  *OneOfOption;
              LIST_ENTRY               *Current;

              Current = GetFirstNode (&Statement->OptionListHead);
              while (!IsNull (&Statement->OptionListHead, Current)) {
                if (OptionNumber == ReturnData.SelectedCell) {
                  break;
                }

                Current = GetNextNode (&Statement->OptionListHead, Current);
                OptionNumber++;
              }

              if (!IsNull (&Statement->OptionListHead, Current)) {
                OneOfOption = DISPLAY_QUESTION_OPTION_FROM_LINK (Current);

                UINT8  ValueType = OneOfOption->OptionOpCode->Type;
                gUserInput->InputValue.Type = ValueType;
                SetValuesByType (&gUserInput->InputValue.Value, &OneOfOption->OptionOpCode->Value, ValueType);

                gUserInput->Action            = 0;
                gUserInput->SelectedStatement = Statement;
                // Exit the input event processing loop to process the callback and save state.
                //
                MasterFrameSharedState->CloseFormRequest = TRUE;
              }
            }
          }

          break;
        }

        case EFI_IFR_ORDERED_LIST_OP:
        {
          // The user selected a cell in an ordered list.  Since the ordered list is a "tap to promote" style listbox, determine
          // which cell was selected (and thus promoted in order) so we can mirror that in the options list.
          //
          ListBox  *LB = NULL;

          Status = FormCanvas->GetSelectedControl (
                                 FormCanvas,
                                 (VOID **)&LB
                                 );

          // If the canvas gave us the selected control, we know it's a "tap to promote" listbox because of the "ordered list" opcode.
          // Ask the listbox which cell was selected.  This is the cell that was promoted and we need to mirror the promotion in the
          // menu options list.
          //
          if ((EFI_SUCCESS == Status) && (NULL != LB)) {
            Status = LB->GetSelectedCellIndex (
                           LB,
                           &ReturnData
                           );
            if (EFI_SUCCESS == Status) {
              // Now that we have the selected cell index, locate the equivalent node in the option list and swap it.
              //
              UINT32  Index;
              UINT32  *ReturnValue;
              UINT32  *ValueArray;
              UINTN   Src;
              UINTN   Tgt;
              UINTN   Jndex;

              DEBUG ((
                DEBUG_INFO,
                "Ordered list Action=%d, Sel=%d, Tgt=%d, Dir=%d\n",
                ReturnData.Action,
                ReturnData.SelectedCell,
                ReturnData.TargetCell,
                ReturnData.Direction
                ));
              // Process the current order.
              //
              ReturnValue = AllocateZeroPool (Statement->CurrentValue.BufferLen);
              // MU_CHANGE [BEGIN] - CodeQL change
              if (ReturnValue == NULL) {
                ASSERT (ReturnValue != NULL);
                return EFI_OUT_OF_RESOURCES;
              }

              // MU_CHANGE [END] - CodeQL change
              ValueArray = (UINT32 *)Statement->CurrentValue.Buffer;

              Src = ReturnData.SelectedCell;
              Tgt = ReturnData.TargetCell;
              #define EXTENDED_DEBUG  0// Set to 1 for extra debug
 #if (EXTENDED_DEBUG)
              DEBUG ((DEBUG_ERROR, "Old Buffer %p\n", ((CHAR8 *)ValueArray) - 0x18));
              DUMP_HEX (DEBUG_ERROR, 0, ((CHAR8 *)ValueArray) - 0x18, Statement->CurrentValue.BufferLen + 0x20, "");

              DEBUG ((DEBUG_ERROR, "New empty Buffer %p\n", ((CHAR8 *)ReturnValue) - 0x18));
              DUMP_HEX (DEBUG_ERROR, 0, ((CHAR8 *)ReturnValue) - 0x18, Statement->CurrentValue.BufferLen + 0x20, "");
 #endif
              // An ordered list always returns all of the data, so
              // move old value to return value honoring the values of
              // Src and Tgt (ie MOVE)
              Jndex = 0;
              for (Index = 0; Index < (UINT32)(Statement->CurrentValue.BufferLen / sizeof (UINT32)); ) {
                if (Index == Tgt) {
                  ReturnValue[Index++] = ValueArray[Src];
                } else if (Jndex == Src) {
                  Jndex++;
                } else {
                  ReturnValue[Index++] = ValueArray[Jndex++];
                }
              }

              switch (ReturnData.Action) {
                case LB_ACTION_TOGGLE:
                  ReturnValue[Src] ^= ORDERED_LIST_CHECKBOX_VALUE_32;
                  break;
                case LB_ACTION_DELETE:
                  Tgt = Statement->CurrentValue.BufferLen / sizeof (UINT32);
                  CopyMem (&ReturnValue[Src], &ReturnValue[Src + 1], (Tgt - Src - 1) * sizeof (UINT32));
                  ReturnValue[Tgt - 1] = 0;
                  break;
                case LB_ACTION_MOVE:
                  // No special action on MOVE -
                  break;
                case LB_ACTION_BOOT:
                  ReturnValue[Src] |= ORDERED_LIST_BOOT_VALUE_32;
                  break;
                case LB_ACTION_SELECT:
                // Select is returned when nothing happens (ie move to self)
                case LB_ACTION_NONE:
                  // Do nothing;
                  break;
              }

 #if (EXTENDED_DEBUG)
              DEBUG ((DEBUG_ERROR, "New filled Buffer %p\n", ((CHAR8 *)ReturnValue) - 0x18));
              DUMP_HEX (DEBUG_ERROR, 0, ((CHAR8 *)ReturnValue) - 0x18, Statement->CurrentValue.BufferLen + 0x20, "");
 #endif
              if (CompareMem (ReturnValue, ValueArray, Statement->CurrentValue.BufferLen) == 0) {
                DEBUG ((DEBUG_ERROR, "%a no change detected\n", __FUNCTION__));
                // ** Error condition ***
                FreePool (ReturnValue);
              } else {
                // Capture the selected statement and input value.
                //
                gUserInput->InputValue.Buffer    = (UINT8 *)ReturnValue;
                gUserInput->InputValue.BufferLen = Statement->CurrentValue.BufferLen;
                gUserInput->Action               = 0;
                gUserInput->SelectedStatement    = Statement;

                // Exit the input event processing loop to process the callback and save state.
                //
                mControlsRequireRefresh                  = TRUE;
                MasterFrameSharedState->CloseFormRequest = TRUE;
              }
            }
          }
          break;
        }

        default:
          break;
      }
    }
  }

Exit:

  // Note that we don't clean-up the canvas here.  Instead, we simply clear the canvas.
  //
  if ((gUserInput->Action == 0) && (gUserInput->SelectedStatement == NULL)) {
    gUserInput->Action = BROWSER_ACTION_NONE;
  }

  return Status;
}

/**
  Free the UI Menu Option structure data.

  @param   MenuOptionList         Point to the menu option list which need to be free.

**/
VOID
FreeMenuOptionData (
  LIST_ENTRY  *MenuOptionList
  )
{
  LIST_ENTRY           *Link;
  UI_MENU_OPTION       *Option;

  //
  // Free menu option list
  //
  while (!IsListEmpty (MenuOptionList)) {
    Link   = GetFirstNode (MenuOptionList);
    Option = MENU_OPTION_FROM_LINK (Link);
    if (Option->Description != NULL) {
      FreePool (Option->Description);
    }

    RemoveEntryList (&Option->Link);
    FreePool (Option);
  }
}

/**

  Base on the browser status info to show an pop up message.

**/
VOID
BrowserStatusProcess (
  VOID
  )
{
  CHAR16             *ErrorInfo;
  EFI_IFR_OP_HEADER  *OpCodeBuf;
  EFI_STRING_ID      StringToken;
  CHAR16             *PrintString;
  SWM_MB_RESULT      SwmResult = 0;

  if (gFormData->BrowserStatus == BROWSER_SUCCESS) {
    return;
  }

  StringToken = 0;
  OpCodeBuf   = NULL;
  if (gFormData->HighLightedStatement != NULL) {
    OpCodeBuf = gFormData->HighLightedStatement->OpCode;
  }

  if (gFormData->BrowserStatus == (BROWSER_WARNING_IF)) {
    ASSERT (OpCodeBuf != NULL && OpCodeBuf->OpCode == EFI_IFR_WARNING_IF_OP);

    StringToken = ((EFI_IFR_WARNING_IF *)OpCodeBuf)->Warning;
  } else {
    if ((gFormData->BrowserStatus == (BROWSER_NO_SUBMIT_IF)) &&
        ((OpCodeBuf != NULL) && (OpCodeBuf->OpCode == EFI_IFR_NO_SUBMIT_IF_OP)))
    {
      StringToken = ((EFI_IFR_NO_SUBMIT_IF *)OpCodeBuf)->Error;
    } else if ((gFormData->BrowserStatus == (BROWSER_INCONSISTENT_IF)) &&
               ((OpCodeBuf != NULL) && (OpCodeBuf->OpCode == EFI_IFR_INCONSISTENT_IF_OP)))
    {
      StringToken = ((EFI_IFR_INCONSISTENT_IF *)OpCodeBuf)->Error;
    }
  }

  if (StringToken != 0) {
    ErrorInfo = GetToken (StringToken, gFormData->HiiHandle);
    // MU_CHANGE [BEGIN] - CodeQL change
    if (ErrorInfo == NULL) {
      ErrorInfo = gBrowserError;
    }

    // MU_CHANGE [END] - CodeQL change
  } else if (gFormData->ErrorString != NULL) {
    //
    // Only used to compatible with old setup browser.
    // Not use this field in new browser core.
    //
    ErrorInfo = gFormData->ErrorString;
  } else {
    switch (gFormData->BrowserStatus) {
      case BROWSER_SUBMIT_FAIL:
        ErrorInfo = gSaveFailed;
        break;

      case BROWSER_FORM_NOT_FOUND:
        ErrorInfo = gFormNotFound;
        break;

      case BROWSER_FORM_SUPPRESS:
        ErrorInfo = gFormSuppress;
        break;

      case BROWSER_PROTOCOL_NOT_FOUND:
        ErrorInfo = gProtocolNotFound;
        break;

      case BROWSER_SUBMIT_FAIL_NO_SUBMIT_IF:
        ErrorInfo = gNoSubmitIfFailed;
        break;

      default:
        ErrorInfo = gBrowserError;
        break;
    }
  }

  switch (gFormData->BrowserStatus) {
    case BROWSER_SUBMIT_FAIL:
    case BROWSER_SUBMIT_FAIL_NO_SUBMIT_IF:
      ASSERT (gUserInput != NULL);
      if (gFormData->BrowserStatus == (BROWSER_SUBMIT_FAIL)) {
        PrintString = gSaveProcess;
      } else {
        PrintString = gSaveNoSubmitProcess;
      }

      SwmDialogsMessageBox (
        L"Internal Error",
        ErrorInfo,                             // Dialog body text.
        PrintString,                           // Dialog Caption text.
        SWM_MB_OK | SWM_MB_STYLE_ALERT2,       // Show OK and CANCEL buttons.
        0,                                     // No timeout
        &SwmResult
        );                                     // Return result.

      gUserInput->Action = BROWSER_ACTION_DISCARD;
      break;

    default:
      SwmDialogsMessageBox (
        L"Requested Pause",
        ErrorInfo,                                // Dialog body text.
        L"Press OK to continue",                  // Dialog Caption text.
        SWM_MB_OK | SWM_MB_STYLE_ALERT2,          // Show OK.
        0,                                        // No timeout
        &SwmResult
        );                                        // Return result.
      break;
  }

  if (StringToken != 0) {
    FreePool (ErrorInfo);
  }
}

/**
  Display one form, and return user input.

  @param FormData                Form Data to be shown.
  @param UserInputData           User input data.

  @retval EFI_SUCCESS            1.Form Data is shown, and user input is got.
                                 2.Error info has show and return.
  @retval EFI_INVALID_PARAMETER  The input screen dimension is not valid
  @retval EFI_NOT_FOUND          New form data has some error.
**/
EFI_STATUS
EFIAPI
FormDisplay (
  IN FORM_DISPLAY_ENGINE_FORM  *FormData,
  OUT USER_INPUT               *UserInputData
  )
{
  EFI_STATUS Status = EFI_SUCCESS;
  UINT32     ThisOpCrc;

  ASSERT (FormData != NULL);
  if (FormData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  gUserInput = UserInputData;
  gFormData  = FormData;

  Status = mSWMProtocol->ActivateWindow (
                           mSWMProtocol,
                           mImageHandle,
                           TRUE
                           );

  //
  // Process the status info first.
  //
  BrowserStatusProcess ();
  if (gFormData->BrowserStatus != BROWSER_SUCCESS) {
    //
    // gFormData->BrowserStatus != BROWSER_SUCCESS, means only need to print the error info, return here.
    //
    return EFI_SUCCESS;
  }

  MeasureStart ();
  ConvertStatementToMenu ();
  ThisOpCrc = MeasureEnd ();

  //
  // Check whether layout is changed.
  //
  if (  mIsFirstForm
     || (gOldFormEntry.HiiHandle != FormData->HiiHandle)
     || (ThisOpCrc != mLastOpCrc)
     || (!CompareGuid (&gOldFormEntry.FormSetGuid, &FormData->FormSetGuid))
     || (gOldFormEntry.FormId != FormData->FormId)
        )
  {
    mStatementLayoutIsChanged = TRUE;
    DEBUG ((DEBUG_INFO, "Layout was changed. Crc=%x LastCrc=%x\n", ThisOpCrc, mLastOpCrc));
  } else {
    DEBUG ((DEBUG_INFO, "Layout was preserved. Crc=%x\n", ThisOpCrc));
    mStatementLayoutIsChanged = FALSE;
  }

  mLastOpCrc = ThisOpCrc;

  // Enable mouse pointer (conditional, based on whether a mouse input device is being used).
  //
  mSWMProtocol->EnableMousePointer (
                  mSWMProtocol,
                  TRUE
                  );
  Status = UiDisplayMenu (FormData);

  if ((mOSKProtocol != NULL) && (FALSE == mRefreshOnEvent)) {
    mOSKProtocol->ShowKeyboard (mOSKProtocol, FALSE);
    mOSKProtocol->ShowKeyboardIcon (mOSKProtocol, FALSE);
    mOSKProtocol->ShowDockAndCloseButtons (mOSKProtocol, FALSE);
  }

  Status = mSWMProtocol->ActivateWindow (
                           mSWMProtocol,
                           mImageHandle,
                           FALSE
                           );

  //
  // Backup last form info.
  //

  mIsFirstForm            = FALSE;
  gOldFormEntry.HiiHandle = FormData->HiiHandle;
  CopyGuid (&gOldFormEntry.FormSetGuid, &FormData->FormSetGuid);
  gOldFormEntry.FormId = FormData->FormId;

  //
  // Free the Ui menu option list.
  //
  FreeMenuOptionData (&gMenuOption);

  return Status;
}

/**
  Clear Screen to the initial state.
**/
VOID
EFIAPI
DriverClearDisplayPage (
  VOID
  )
{
  // This isn't called by anybody except SetupBrowserDxe\Setup.c.  In the initial
  // release, Setup.c was overridden to comment out this call.

  //    ClearDisplayPage ();

  mIsFirstForm = TRUE;
}

/**
  Set Buffer to Value for Size bytes.

  @param  Buffer                 Memory to set.
  @param  Size                   Number of bytes to set
  @param  Value                  Value of the set operation.

**/
VOID
SetUnicodeMem (
  IN VOID    *Buffer,
  IN UINTN   Size,
  IN CHAR16  Value
  )
{
  CHAR16 *Ptr;

  Ptr = Buffer;
  while ((Size--) != 0) {
    *(Ptr++) = Value;
  }
}

VOID
EFIAPI
FormNullCallback (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
}

EFI_STATUS
EFIAPI
RegisterWithSimpleWindowManager (
  IN EFI_HANDLE  ImageHandle
  )
{
  EFI_STATUS Status = EFI_SUCCESS;
  SWM_RECT   FrameRect;

  //
  // Open the Simple Window Manager protocol.
  //
  Status = gBS->LocateProtocol (
                  &gMsSWMProtocolGuid,
                  NULL,
                  (VOID **)&mSWMProtocol
                  );

  if (EFI_ERROR (Status)) {
    mSWMProtocol = NULL;
    DEBUG ((DEBUG_ERROR, "ERROR [DE]: Failed to find Simple Window Manager protocol (%r).\r\n", Status));
    goto Exit;
  }

  // Default to registering full screen support.
  //
  SWM_RECT_INIT2 (
    FrameRect,
    0,
    0,
    mGop->Mode->Info->HorizontalResolution,
    mGop->Mode->Info->VerticalResolution
    );

  //
  // Register with the Simple Window Manager.
  //
  Status = mSWMProtocol->RegisterClient (
                           mSWMProtocol,
                           ImageHandle,
                           SWM_Z_ORDER_CLIENT,
                           &FrameRect,
                           NULL,
                           NULL,
                           &mPointerProtocol,
                           NULL                                  // No paint notifications required.
                           );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR [DE]: Failed to register with Simple Window Manager protocol (%r).\r\n", Status));
    goto Exit;
  }

Exit:

  return Status;
}

/**
  ReadyToBoot callback to force screen clear.

  @param[in]  Event     Event whose notification function is being invoked
  @param[in]  Context   Pointer to the notification function's context

**/
VOID
EFIAPI
FormDisplayOnReadyToBoot (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  DEBUG ((DEBUG_INFO, "%a\n", __FUNCTION__));

  if (NULL != mPrivateData.PreviousCanvas) {
    delete_Canvas (mPrivateData.PreviousCanvas);
    mPrivateData.PreviousCanvas = NULL;
  }
}

/**
  Initialize Setup Browser driver.

  @param ImageHandle     The image handle.
  @param SystemTable     The system table.

  @retval EFI_SUCCESS    The Setup Browser module is initialized correctly..
  @return Other value if failed to initialize the Setup Browser module.

**/
EFI_STATUS
EFIAPI
InitializeDisplayEngine (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS Status = EFI_SUCCESS;

  mImageHandle = ImageHandle;
  // Determine if the GOP Protocol is available
  //
  Status = gBS->HandleProtocol (
                  gST->ConsoleOutHandle,
                  &gEfiGraphicsOutputProtocolGuid,
                  (VOID **)&mGop
                  );
  //
  // Known issue where on some devices gop is not found on console out.
  // Need to root cause
  //
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "INFO [DE]: Failed to find GOP on ConsoleOutHandle.  Try globally.\n"));
    Status = gBS->LocateProtocol (&gEfiGraphicsOutputProtocolGuid, NULL, (VOID **)&mGop);
  }

  if (EFI_ERROR (Status)) {
    mGop = NULL;
    DEBUG ((DEBUG_ERROR, "INFO [DE]: Failed to find GOP protocol (%r).\r\n", Status));
    ASSERT_EFI_ERROR (Status);
    Status = EFI_UNSUPPORTED;
    goto Exit;
  }

  // Open the Simple Text Ex protocol on the Console handle.
  //
  Status = gBS->HandleProtocol (
                  gST->ConsoleInHandle,
                  &gEfiSimpleTextInputExProtocolGuid,
                  (VOID **)&mSimpleTextInEx
                  );

  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    mSimpleTextInEx = NULL;
    DEBUG ((DEBUG_ERROR, "ERROR [DE]: Failed to find Simple Text Input Ex protocol (%r).\r\n", Status));
    goto Exit;
  }

  // Initialize the Simple UI ToolKit.
  //
  Status = InitializeUIToolKit (ImageHandle);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR [DE]: Failed to initialize UI toolkit (%r).\r\n", Status));
    goto Exit;
  }

  //
  // Publish our HII data
  //
  gHiiHandle = HiiAddPackages (
                 &gDisplayEngineGuid,
                 ImageHandle,
                 DisplayEngineStrings,
                 NULL
                 );

  ASSERT (gHiiHandle != NULL);

  //
  // Install Form Display protocol
  //
  Status = gBS->InstallProtocolInterface (
                  &mPrivateData.Handle,
                  &gEdkiiFormDisplayEngineProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mPrivateData.FromDisplayProt
                  );

  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR [DE]: Failed to install Form Display Engine protocol (%r).\r\n", Status));
    goto Exit;
  }

  InitializeDisplayStrings ();

  ZeroMem (&gHighlightMenuInfo, sizeof (gHighlightMenuInfo));
  ZeroMem (&gOldFormEntry, sizeof (gOldFormEntry));

  // Create the master frame notification event.
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  EfiEventEmptyFunction,
                  NULL,
                  &gMsEventMasterFrameNotifyGroupGuid,
                  &mMasterFrameNotifyEvent
                  );

  if (EFI_SUCCESS != Status) {
    DEBUG ((DEBUG_ERROR, "ERROR [DE]: Failed to create master frame notification event (%r).\r\n", Status));
    goto Exit;
  }

  // Register with the Simple Window Manager.
  //
  Status = RegisterWithSimpleWindowManager (ImageHandle);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR [DE]: Failed to register with window manager (%r).\r\n", Status));
    goto Exit;
  }

  // Install ReadyToBoot callback to note when the disply may be corrupted
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  FormDisplayOnReadyToBoot,
                  NULL,
                  &gEfiEventReadyToBootGuid,
                  &mReadyToBootEvent
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR [DE]: Failed to register OnReadyToBoot (%r).\r\n", Status));
    Status = EFI_SUCCESS;      // Don't fail if this fails.
    goto Exit;
  }

  Status = gBS->LocateProtocol (
                  &gMsOSKProtocolGuid,
                  NULL,
                  (VOID **)&mOSKProtocol

                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "WARN [DE]: Failed to locate on-screen keyboard protocol (%r).\r\n", Status));
    mOSKProtocol = NULL;
  }

  Status = gBS->LocateProtocol (
                  &gEfiHiiDatabaseProtocolGuid,
                  NULL,
                  (VOID **)&mHiiDatabase
                  );
  ASSERT_EFI_ERROR (Status);     // Protocol is in dependency list, so shouldn't ASSERT
  //
  // Register notify for Form package update
  //
  Status = mHiiDatabase->RegisterPackageNotify (
                           mHiiDatabase,
                           EFI_HII_PACKAGE_FORMS,
                           NULL,
                           FormUpdateNotify,
                           EFI_HII_DATABASE_NOTIFY_REMOVE_PACK,
                           &mNotifyHandle
                           );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR [DE]: Failed to locate HiiDatabase protocol (%r).\r\n", Status));
  }

Exit:

  return Status;
}

/**
  This is the default unload handle for display core drivers.

  @param[in]  ImageHandle       The drivers' driver image.

  @retval EFI_SUCCESS           The image is unloaded.
  @retval Others                Failed to unload the image.

**/
EFI_STATUS
EFIAPI
UnloadDisplayEngine (
  IN EFI_HANDLE  ImageHandle
  )
{
  //
  // Unregister notify for Form package update
  //
  mHiiDatabase->UnregisterPackageNotify (
                  mHiiDatabase,
                  mNotifyHandle
                  );

  HiiRemovePackages (gHiiHandle);

  FreeDisplayStrings ();

  if (gHighlightMenuInfo.OpCode != NULL) {
    FreePool (gHighlightMenuInfo.OpCode);
  }

  if (gHighlightMenuInfo.TOSOpCode != NULL) {
    FreePool (gHighlightMenuInfo.TOSOpCode);
  }

  if (NULL != mReadyToBootEvent) {
    gBS->CloseEvent (mReadyToBootEvent);
  }

  return EFI_SUCCESS;
}

/**
    MS Display Engine doesn't support CustomizedDisplayLib.
*/

/**
  All data is always submitted in the MS Display Engine.

  @return Action BROWSER_ACTION_SUBMIT.
**/
UINTN
EFIAPI
ConfirmDataChange (
  VOID
  )
{
  return BROWSER_ACTION_SUBMIT;
}

/**
  Set Timeout value for a certain Form to get user response.

  This function allows to set timeout value on a certain form if necessary.
  If timeout is not zero, the form will exit if user has no response in timeout.

  @param[in]  FormData   Form Data to be shown in Page

  @return 0     No timeout for this form.
  @return > 0   Timeout value in 100 ns units.
**/
UINT64
EFIAPI
FormExitTimeout (
  IN FORM_DISPLAY_ENGINE_FORM  *FormData
  )
{
  return 0;
}
