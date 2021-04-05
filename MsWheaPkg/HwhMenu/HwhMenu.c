/** @file
Hwhmenu.c

Hardware Health - Menu to display HwErrRecs

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>

#include <Protocol/HiiConfigAccess.h>

#include <Guid/HwhMenuGuid.h>
#include <Guid/MdeModuleHii.h>
#include <Guid/Cper.h>
#include <Guid/MsWheaReportDataType.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/HiiLib.h>
#include <Library/UefiHiiServicesLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/CheckHwErrRecHeaderLib.h>
#include <Library/ParserRegistryLib.h>

#include <MsWheaErrorStatus.h>
#include <MsWheaReport/MsWheaReportCommon.h>

#include "HwhMenu.h"
#include "HwhMenuVfr.h"
#include "CreatorIDParser.h"
#include "PlatformIDParser.h"

#pragma pack(1)

// Struct Containing a HWErrRec
typedef struct ErrorRecord
{
  LIST_ENTRY                      entry; // Necessary for linked list structure
  EFI_COMMON_ERROR_RECORD_HEADER *error; // Pointer to the HWErrRec
  UINT32                          val;   // Page number
} ErrorRecord;

#pragma pack()

//*---------------------------------------------------------------------------------------*
//* Global Variables                                                                      *
//*---------------------------------------------------------------------------------------*
STATIC  HWH_MENU_CONFIG mHwhMenuConfiguration = {LOGS_TRUE};                              // Configuration for VFR
        LIST_ENTRY      mListHead             = INITIALIZE_LIST_HEAD_VARIABLE(mListHead); // Head of the linked list
        UINT32          NumErrorEntries       = 0;                                        // Number of HwErrRec(s)
        ErrorRecord     *currentPage          = NULL;                                     // Current record displayed on the page
        CHAR16          UnicodeString[MAX_DISPLAY_STRING_LENGTH + 1];                     // Unicode buffer for printing

#define HWH_MENU_SIGNATURE SIGNATURE_32('H', 'w', 'h', 'm')
#define NUM_SEC_DATA_ROWS 15
#define NUM_SEC_DATA_COLUMNS 3

// Writable UNI strings. NOTE: We are using row-column addressing
CONST EFI_STRING_ID DisplayLines[NUM_SEC_DATA_ROWS][NUM_SEC_DATA_COLUMNS] =
{
  {STRING_TOKEN(STR_HWH_LOG_LINE_0_0),  STRING_TOKEN(STR_HWH_LOG_LINE_0_1),  STRING_TOKEN(STR_HWH_LOG_LINE_0_2)},
  {STRING_TOKEN(STR_HWH_LOG_LINE_1_0),  STRING_TOKEN(STR_HWH_LOG_LINE_1_1),  STRING_TOKEN(STR_HWH_LOG_LINE_1_2)},
  {STRING_TOKEN(STR_HWH_LOG_LINE_2_0),  STRING_TOKEN(STR_HWH_LOG_LINE_2_1),  STRING_TOKEN(STR_HWH_LOG_LINE_2_2)},
  {STRING_TOKEN(STR_HWH_LOG_LINE_3_0),  STRING_TOKEN(STR_HWH_LOG_LINE_3_1),  STRING_TOKEN(STR_HWH_LOG_LINE_3_2)},
  {STRING_TOKEN(STR_HWH_LOG_LINE_4_0),  STRING_TOKEN(STR_HWH_LOG_LINE_4_1),  STRING_TOKEN(STR_HWH_LOG_LINE_4_2)},
  {STRING_TOKEN(STR_HWH_LOG_LINE_5_0),  STRING_TOKEN(STR_HWH_LOG_LINE_5_1),  STRING_TOKEN(STR_HWH_LOG_LINE_5_2)},
  {STRING_TOKEN(STR_HWH_LOG_LINE_6_0),  STRING_TOKEN(STR_HWH_LOG_LINE_6_1),  STRING_TOKEN(STR_HWH_LOG_LINE_6_2)},
  {STRING_TOKEN(STR_HWH_LOG_LINE_7_0),  STRING_TOKEN(STR_HWH_LOG_LINE_7_1),  STRING_TOKEN(STR_HWH_LOG_LINE_7_2)},
  {STRING_TOKEN(STR_HWH_LOG_LINE_8_0),  STRING_TOKEN(STR_HWH_LOG_LINE_8_1),  STRING_TOKEN(STR_HWH_LOG_LINE_8_2)},
  {STRING_TOKEN(STR_HWH_LOG_LINE_9_0),  STRING_TOKEN(STR_HWH_LOG_LINE_9_1),  STRING_TOKEN(STR_HWH_LOG_LINE_9_2)},
  {STRING_TOKEN(STR_HWH_LOG_LINE_10_0), STRING_TOKEN(STR_HWH_LOG_LINE_10_1), STRING_TOKEN(STR_HWH_LOG_LINE_10_2)},
  {STRING_TOKEN(STR_HWH_LOG_LINE_11_0), STRING_TOKEN(STR_HWH_LOG_LINE_11_1), STRING_TOKEN(STR_HWH_LOG_LINE_11_2)},
  {STRING_TOKEN(STR_HWH_LOG_LINE_12_0), STRING_TOKEN(STR_HWH_LOG_LINE_12_1), STRING_TOKEN(STR_HWH_LOG_LINE_12_2)},
  {STRING_TOKEN(STR_HWH_LOG_LINE_13_0), STRING_TOKEN(STR_HWH_LOG_LINE_13_1), STRING_TOKEN(STR_HWH_LOG_LINE_13_2)},
  {STRING_TOKEN(STR_HWH_LOG_LINE_14_0), STRING_TOKEN(STR_HWH_LOG_LINE_14_1), STRING_TOKEN(STR_HWH_LOG_LINE_14_2)}
};

//*---------------------------------------------------------------------------------------*
//* HII specific Vendor Device Path definition.                                           *
//*---------------------------------------------------------------------------------------*
struct
{
  VENDOR_DEVICE_PATH        VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  End;
} mHiiVendorDevicePath = {
    {
      {
        HARDWARE_DEVICE_PATH,
        HW_VENDOR_DP,
        {
          (UINT8)(sizeof(VENDOR_DEVICE_PATH)),
          (UINT8)((sizeof(VENDOR_DEVICE_PATH)) >> 8)
        }
      },
      EFI_CALLER_ID_GUID
    },
    {
      END_DEVICE_PATH_TYPE,
      END_ENTIRE_DEVICE_PATH_SUBTYPE,
      {
        (UINT8)(END_DEVICE_PATH_LENGTH),
        (UINT8)((END_DEVICE_PATH_LENGTH) >> 8)
      }
    }
};

//*---------------------------------------------------------------------------------------*
//* Doubly Linked List Methods                                                            *
//*---------------------------------------------------------------------------------------*

/**
 *  Deletes the list structure containing WHEA Errors
 *
 *  @retval     VOID
**/
VOID
DeleteList(VOID)
{

  while (!IsListEmpty(&mListHead)) {
    
    currentPage = (ErrorRecord *)GetFirstNode(&mListHead); //get ErrorRecord being deleted
    RemoveEntryList((LIST_ENTRY *)currentPage);            //remove ErrorRecord from list
    FreePool(currentPage->error);                          //free HwErrRecXXXX in record
    FreePool(currentPage);                                 //free ErrorRecord
  }

  currentPage = NULL;
}

/**
 *  Changes the current page to be the current error record to be the next in the list structure
 *
 *  @retval     BOOLEAN       TRUE if currentPage was changed to next
 *                            FALSE otherwise
**/
BOOLEAN
PageForward(VOID)
{

  LIST_ENTRY *tmp;
  
  tmp = GetNextNode(&mListHead, (LIST_ENTRY *)currentPage);

  if (tmp != &mListHead) {
    currentPage = (ErrorRecord *)tmp;
    return TRUE;
  }
  return FALSE;
}

/**
 *  Changes the current page to be the current error record to be the previous in the list structure
 *
 *  @retval     BOOLEAN     TRUE if currentPage was changed to previous
 *                          FALSE otherwise
**/
BOOLEAN
PageBackward(VOID)
{

  LIST_ENTRY *tmp;
  
  tmp = GetPreviousNode(&mListHead, (LIST_ENTRY *)currentPage);

  if (tmp != &mListHead) {
    currentPage = (ErrorRecord *)tmp;
    return TRUE;
  }
  return FALSE;
}

//*---------------------------------------------------------------------------------------*
//* Hii Config Access functions                                                           *
//*---------------------------------------------------------------------------------------*
EFI_STATUS
EFIAPI
ExtractConfig(
  IN     CONST  EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN     CONST  EFI_STRING                      Request,
     OUT        EFI_STRING                      *Progress,
     OUT        EFI_STRING                      *Results
  );

EFI_STATUS
EFIAPI
RouteConfig(
  IN     CONST  EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN     CONST  EFI_STRING                      Configuration,
     OUT        EFI_STRING                      *Progress
  );

EFI_STATUS
EFIAPI
DriverCallback(
  IN     CONST  EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN            EFI_BROWSER_ACTION              Action,
  IN            EFI_QUESTION_ID                 QuestionId,
  IN            UINT8                           Type,
  IN            EFI_IFR_TYPE_VALUE              *Value,
     OUT        EFI_BROWSER_ACTION_REQUEST      *ActionRequest
  );

//*---------------------------------------------------------------------------------------*
//* Private internal data                                                                 *
//*---------------------------------------------------------------------------------------*
struct
{
  UINTN                           Signature;
  EFI_HANDLE                      DriverHandle;
  EFI_HII_HANDLE                  HiiHandle;
  EFI_HII_CONFIG_ACCESS_PROTOCOL  ConfigAccess;
} mHwhMenuPrivate = {
    HWH_MENU_SIGNATURE,
    NULL,
    NULL,
    {
      ExtractConfig,
      RouteConfig,
      DriverCallback
    }
};

//*---------------------------------------------------------------------------------------*
//* Main Functions                                                                        *
//*---------------------------------------------------------------------------------------*

/**
 *  Writes a maximum of MAX_DISPLAY_STRING_LENGTH characters to the
 *  specified EFI_STRING_ID for the VFR
 *
 *  @param[in]  Str           EFI_STRING being written to
 *  @param[in]  Format        Format string
 *  @param[in]  ...           Variables placed into format string
 * 
 *  @retval     0             No characters were written to Str. Note
 *                            that it could be a blank string was written
 *              UINTN         Number of characters written
**/
UINTN
UnicodeDataToVFR(
  IN CONST EFI_STRING_ID  Str,
  IN CONST CHAR16         *Format,
  ...
  )
{

  CHAR16  Buffer[MAX_DISPLAY_STRING_LENGTH + 1];
  VA_LIST Marker;
  UINTN   NumWritten;

  VA_START(Marker, Format);
  NumWritten = UnicodeVSPrint(Buffer, MAX_DISPLAY_STRING_LENGTH * sizeof(CHAR16), Format, Marker);
  VA_END(Marker);

  if (HiiSetString(mHwhMenuPrivate.HiiHandle, Str, Buffer, NULL) == 0) {
    return 0;
  }

  return NumWritten;
}

/**
 * UpdateForm
 *
 * Forces the form to update by inserting nothing into the label section of VFR file
 *
 * @retval      VOID
 */
VOID
UpdateForm(VOID)
{
  EFI_STATUS              Status;
  VOID                    *StartOpCodeHandle;
  VOID                    *EndOpCodeHandle    = NULL;
  EFI_IFR_GUID_LABEL      *StartLabel;
  EFI_IFR_GUID_LABEL      *EndLabel;
  BOOLEAN                 Aborted             = TRUE;

  do {

    // Init OpCode Handle and Allocate space for creation of UpdateData Buffer
    StartOpCodeHandle = HiiAllocateOpCodeHandle();
    ASSERT(StartOpCodeHandle != NULL);
    if (NULL == StartOpCodeHandle) {
      break;
    }

    EndOpCodeHandle = HiiAllocateOpCodeHandle();
    ASSERT(EndOpCodeHandle != NULL);
    if (NULL == EndOpCodeHandle) {
      break;
    }

    // Create Hii Extend Label OpCode as the start opcode
    StartLabel = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode(StartOpCodeHandle,
                                                           &gEfiIfrTianoGuid,
                                                           NULL, 
                                                           sizeof(EFI_IFR_GUID_LABEL));
    if (NULL == StartLabel) {
      break;
    }
    StartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;

    // Create Hii Extend Label OpCode as the end opcode
    EndLabel = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode(EndOpCodeHandle,
                                                         &gEfiIfrTianoGuid,
                                                         NULL,
                                                         sizeof(EFI_IFR_GUID_LABEL));
    if (NULL == EndLabel) {
      break;
    }
    EndLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;

    StartLabel->Number = LABEL_UPDATE_LOCATION;
    EndLabel->Number = LABEL_UPDATE_END;

    Status = HiiUpdateForm(mHwhMenuPrivate.HiiHandle,
                           &gHwhMenuFormsetGuid,
                           HWH_MENU_FORM_ID,
                           StartOpCodeHandle,
                           EndOpCodeHandle);

    if (EFI_ERROR(Status)) {
      DEBUG((DEBUG_ERROR, "%a Error in HiiUpdateform.  Code=%r\n", __FUNCTION__, Status));
    }
    else {
      Aborted = FALSE;
    }
  } while (FALSE);

  if (Aborted) {
    DEBUG((DEBUG_ERROR, "%a Form Update aborted.\n", __FUNCTION__ ));
  }

  if (StartOpCodeHandle != NULL) {
    HiiFreeOpCodeHandle(StartOpCodeHandle);
  }
  if (StartOpCodeHandle != NULL) {
    HiiFreeOpCodeHandle(EndOpCodeHandle);
  }
}

/**
 *  Dump the Section data in byte and ascii form
 * 
 *  @param[in]     Strings          Pointer to unallocated array of strings which will be populated by the parser
 *  @param[in]     Err              Pointer to the HwErrRec bing parsed
 *  @param[in]     SectionHeader    Pointer to Section Header of data being parsed
 * 
 *  @retval        UINTN            Number of CHAR16* allocated in Strings to be written to the form
**/
UINTN
SectionDump(
          IN OUT CHAR16                         ***Strings,
    CONST IN     EFI_COMMON_ERROR_RECORD_HEADER *Err,
    CONST IN     EFI_ERROR_SECTION_DESCRIPTOR   *SecHead)
{
  CONST UINT8       *Ptr2Data;
        UINT32      BytesCurrentLine = 0;
        UINT32      NumLines, OuterLoop, InnerLoop;
        CHAR16      *Ptr2Buffer;
        CHAR8       Ascii[17];

  Ascii[16] = '\0';

  // Quick Null-check
  if (Err == NULL || SecHead == NULL || Strings == NULL) {
    return 0;
  }

  // Get how many lines of bytes we are printing at 16 bytes per line
  NumLines = SecHead->SectionLength % 16 == 0 ? SecHead->SectionLength / 16 : (SecHead->SectionLength / 16) + 1;

  // Allocate a pool to contain all CHAR 16*
  *Strings = AllocatePool(sizeof(CHAR16 *) * NumLines);

  // Get a pointer to the beginning of the data
  Ptr2Data = (CONST UINT8 *)Err + SecHead->SectionOffset;

  // For each line...
  for (OuterLoop = 0; OuterLoop < NumLines; OuterLoop++) {

    // Allocate the line
    (*Strings)[OuterLoop] = AllocatePool(sizeof(CHAR16) * MAX_DISPLAY_STRING_LENGTH);

    // Get a pointer to the beginning of that line
    Ptr2Buffer = (*Strings)[OuterLoop];

    // See how many bytes need to be printed on this line
    BytesCurrentLine = SecHead->SectionLength - (OuterLoop * 16) < 16 ? SecHead->SectionLength - (OuterLoop * 16) : 16;

    // For each byte in the line...
    for (InnerLoop = 0; InnerLoop < BytesCurrentLine; InnerLoop++) {

      // Check if there is an ascii interpretation of the byte. If so, add it or just add a '.'
      Ascii[InnerLoop] = ((*Ptr2Data >= 0x20) && (*Ptr2Data <= 0x7e)) ? *Ptr2Data : '.';

      // Write the byte interpretation and more our buffer pointer forward
      Ptr2Buffer += UnicodeSPrint(Ptr2Buffer, 4 * sizeof(CHAR16), L"%02X ", *Ptr2Data);

      // Increment our data pointer to the next byte
      Ptr2Data++;
    }

    // Finally, print the ascii interpretation of this line
    UnicodeSPrint(Ptr2Buffer, 20 * sizeof(CHAR16), L"\n\n%a", Ascii);
  }

  // Let the caller know how many lines we wrote
  return NumLines;
}

/**
 *  Parses the date and time and writes values to uni strings
 *
 *  @retval     VOID
**/
VOID
ParseDateTime(VOID)
{
  UnicodeDataToVFR((EFI_STRING_ID)STR_HWH_LOG_DATE_VALUE,
                   L"%02X/%02X/%02X",
                   currentPage->error->TimeStamp.Month,
                   currentPage->error->TimeStamp.Day,
                   currentPage->error->TimeStamp.Year);

  UnicodeDataToVFR((EFI_STRING_ID)STR_HWH_LOG_TIME_VALUE,
                   L"%02X:%02X:%02X",
                   currentPage->error->TimeStamp.Hours,
                   currentPage->error->TimeStamp.Minutes,
                   currentPage->error->TimeStamp.Seconds);
}

/**
 *  Parses the number of sections and writes value to uni string
 *
 *  @retval     VOID
**/
VOID
ParseNumberOfSections(VOID)
{
  UnicodeDataToVFR((EFI_STRING_ID)STR_HWH_LOG_NUMSECTIONS_VALUE,
                   L"%d",
                   currentPage->error->SectionCount);
}

/**
 *  Parses the severity which (for now) simply displays the number value
 *
 *  @retval     VOID
**/
VOID
ParseSeverity(VOID)
{

  UnicodeDataToVFR((EFI_STRING_ID)STR_HWH_LOG_SEVERITY_VALUE,
                   L"%d",
                   currentPage->error->ErrorSeverity);
}

/**
 *  Parses the current error out of total number of errors
 *  and writes value to uni strings
 *
 *  @retval     VOID
**/
VOID
ParsePageNumber(VOID)
{
  UnicodeDataToVFR((EFI_STRING_ID)STR_HWH_PAGE_NUM,
                   L"          Error %d of %d",
                   currentPage->val,
                   NumErrorEntries);
}

/**
 *  Finds the number of CHAR16s between source and either '\n' or '\0'
 *
 *  @param[in]     Str      Pointer to string being parsed
 * 
 *  @retval        UINTN    Number of CHAR16s from source address where '\n' or '\0' is encountered
**/
UINTN
FindNewline(
  IN CHAR16 **Source
  )
{
  // Null-check
  if (Source == NULL || *Source == NULL) {
    return 0;
  }

  UINTN Counter = 0;
  CHAR16 *End = *Source;

  // Walk the string until one of the following is true
  while (*End != '\n' && *End != '\0' && Counter < MAX_DISPLAY_STRING_LENGTH + 1) {
    End++;
    Counter++;
  }

  // Return how far we walked
  return Counter;
}

/**
 *  Parses the current page number out of overall number of pages
 *
 *  @param[in]     Err              Pointer to HwErrRec being parsed
 *  @param[in]     SectionHeader    Pointer to Section Header of data being parsed
 *  @param[in]     index            Pointer to to the current line of the section data portion of 
 *                                  front page being written to
 * 
 *  @retval        VOID
**/
VOID 
ParseSectionData(
  IN     CONST EFI_COMMON_ERROR_RECORD_HEADER *Err,
  IN     CONST EFI_ERROR_SECTION_DESCRIPTOR   *SectionHeader,
  IN OUT       UINT8                          *index
  )
{
  if (Err == NULL || SectionHeader == NULL || index == NULL) {
    return;
  }

  CHAR16              **SectionDataStrings  = NULL;      // Array of strings which are written in the section data area of page
  SECTIONFUNCTIONPTR  SectionParser         = NULL;      // Pointer to function which can parse the section data
  CHAR16              *StringParsePtr       = NULL;      // Pointer to current place in string being written
  UINTN               NumberOfStrings       = 0;         // Number of strings within SectionDataStrings
  UINTN               StringParseChars      = 0;         // Number of chars from CHAR16* to '\n' or '\0'
  UINTN               OuterLoop, InnerLoop;
  // Get parser for the section data if available
  SectionParser = ParserLibFindSectionParser(&(SectionHeader->SectionType));

  if (SectionParser == NULL) {
    SectionParser = (SECTIONFUNCTIONPTR)&SectionDump;
  }

  // Call the parser and get how many strings were returned
  NumberOfStrings = SectionParser(&SectionDataStrings, Err, SectionHeader);

  for (OuterLoop = 0; OuterLoop < NumberOfStrings; OuterLoop++) {
    // Set the parse pointer to the start of the string
    StringParsePtr = SectionDataStrings[OuterLoop];

    // If this string was not allocated for some reason, just move on to the next one
    if (StringParsePtr == NULL) {
      continue;
    }

    // If we've run out of writtable lines, free this one and continue
    // so the rest are freed as well
    if ((*index) >= NUM_SEC_DATA_ROWS) {
      FreePool(StringParsePtr);
      continue;
    }

    // For each column in the row being written to
    for (InnerLoop = 0; InnerLoop < NUM_SEC_DATA_COLUMNS; InnerLoop++) {

      // Make sure we still have more chars to parse from the string. If not, just clear the uni string
      if (StringParsePtr >= SectionDataStrings[OuterLoop] + StrnLenS(SectionDataStrings[OuterLoop], MAX_DISPLAY_STRING_LENGTH)) {
        HiiSetString(mHwhMenuPrivate.HiiHandle, DisplayLines[*index][InnerLoop], L"\0", NULL);
      }
      else {

        // Find distance from StringParsePtr to a '\0' or '\n'
        StringParseChars = FindNewline(&StringParsePtr);

        // Create null-terminated string from StringParsePtr to StringParsePtr + StringParseChars
        if (!EFI_ERROR(StrnCpyS(UnicodeString, MAX_DISPLAY_STRING_LENGTH + 1, StringParsePtr, StringParseChars - 1))) {

          // Write the string to .uni string in the InnerLoop-th column
          HiiSetString(mHwhMenuPrivate.HiiHandle, DisplayLines[*index][InnerLoop], UnicodeString, NULL);
        }
        else {

          // Just clear the string if for some reason it could not be copied
          HiiSetString(mHwhMenuPrivate.HiiHandle, DisplayLines[*index][InnerLoop], L"\0", NULL);
        }

        // Put the ptr 1 CHAR16 ahead of whatever character stopped FindNewline()
        StringParsePtr += StringParseChars + 1;
      }
    }

    // Free the string and increment to the next line of the page
    (*index)++;
    FreePool(SectionDataStrings[OuterLoop]);
  }

  // Free the array if it was created
  if (SectionDataStrings != NULL) {
    FreePool(SectionDataStrings);
    SectionDataStrings = NULL;
  }

  // Publish blank line
  for (OuterLoop = 0; OuterLoop < NUM_SEC_DATA_COLUMNS; OuterLoop++) {
    HiiSetString(mHwhMenuPrivate.HiiHandle, DisplayLines[*index][OuterLoop], L"\0", NULL);
  }

  (*index)++;
}

/**
 *  Updates all strings on the VFR with the new error record being displayed
 *
 *  @retval     VOID
**/
VOID
UpdateDisplayStrings(VOID)
{
  UINT8 OuterLoop;

  // Make sure there is data to populate the page
  if (currentPage == NULL) {
    return;
  }

  CONST EFI_COMMON_ERROR_RECORD_HEADER *Err         = currentPage->error;   // Pointer to Error being displayed
  UINT8 SecLineIndex                                = 0;                    // Index of section line being written to

  // DebugDumpMemory(DEBUG_INFO, Err, currentPage->error->RecordLength, DEBUG_DM_PRINT_ASCII);

  ParseDateTime();                    // Publish date and time fields
  ParseNumberOfSections();            // Publish section num field
  ParsePageNumber();                  // Publish page number
  ParseSeverity();                    // Publish severity field
  ParseSourceID(&(Err->PlatformID));  // Publish Source ID field
  ParseCreatorID(&(Err->CreatorID));  // Publish Creator ID field

  //display at most 2 Sections.
  for (OuterLoop = 0; OuterLoop < 2; OuterLoop++) {

    //if We have another section to display
    if (OuterLoop < Err->SectionCount) {

      UnicodeDataToVFR(DisplayLines[SecLineIndex++][0],
                       L"Section %d",
                       OuterLoop + 1);

      ParseSectionData(Err, (((EFI_ERROR_SECTION_DESCRIPTOR *)(Err + 1)) + OuterLoop), &SecLineIndex);
    }
  }

  // Set the rest of the lines to blank
  while (SecLineIndex < NUM_SEC_DATA_ROWS) {

    for (OuterLoop = 0; OuterLoop < NUM_SEC_DATA_COLUMNS; OuterLoop++) {
      HiiSetString(mHwhMenuPrivate.HiiHandle, DisplayLines[SecLineIndex][OuterLoop], L"\0", NULL);
    }
    
    SecLineIndex++;
  }
}

/**
 *  Finds the XXXX in the HwRecRecXXXX -> the next index at which a Whea record will be inserted.
 * 
 *  @retval     UINT32     A Whea Index.
 *
**/
UINT32
GetMaxWheaIndex(VOID)
{
  EFI_STATUS  Status;                                   // Return status
  UINTN       Size      = 0;                            // Used to store size of HwErrRec found
  UINT32      OuterLoop = 0;                            // Store the current index we're checking
  CHAR16      VarName[EFI_HW_ERR_REC_VAR_NAME_LEN];     // HwRecRecXXXX used to find variable

  for (OuterLoop = 0; OuterLoop <= MAX_UINT32; OuterLoop++) {

    UnicodeSPrint(VarName, sizeof(VarName), L"%s%04X", EFI_HW_ERR_REC_VAR_NAME, (UINT16)(OuterLoop & MAX_UINT16));

    Status = gRT->GetVariable(VarName, &gEfiHardwareErrorVariableGuid, NULL, &Size, NULL);

    // We've found the next index. Excellent.
    if (Status == EFI_NOT_FOUND) {
      break;
    }
  }

  return OuterLoop;
}

/**
 *  Populates list structure with Whea Errors.
 *
 *  @retval     VOID  
 *
**/
EFI_STATUS
PopulateWheaErrorList(VOID)
{
  EFI_STATUS                      Status;                               // Return status
  UINTN                           Size                = 0;              // Size of variable being stored
  CHAR16                          VarName[EFI_HW_ERR_REC_VAR_NAME_LEN]; // HwRecRecXXXX name of var being stored
  EFI_COMMON_ERROR_RECORD_HEADER  *ErrorRecordPointer = NULL;           // Pointer to the start of the record
  ErrorRecord                     *new;                                 // New record being added
  UINT32                          OuterLoop;

  NumErrorEntries = GetMaxWheaIndex();

  for (OuterLoop = NumErrorEntries; OuterLoop > 0; --OuterLoop) {

    // Create HwRecRecXXXX string
    UnicodeSPrint(VarName, sizeof(VarName), L"%s%04X", EFI_HW_ERR_REC_VAR_NAME, OuterLoop - 1);

    // Determine size required to allocate
    Status = gRT->GetVariable(VarName, &gEfiHardwareErrorVariableGuid, NULL, &Size, NULL);

    if (Status != EFI_NOT_FOUND) {

      ErrorRecordPointer = AllocatePool(Size);

      // Populate the error record
      Status = gRT->GetVariable(VarName, &gEfiHardwareErrorVariableGuid, NULL, &Size, ErrorRecordPointer);

      if (ValidateCperHeader(ErrorRecordPointer, Size)) {

        new = AllocateZeroPool(sizeof(ErrorRecord));

        new->error = ErrorRecordPointer;
        new->val = OuterLoop;

        InsertHeadList(&mListHead, (LIST_ENTRY *)new);
      }
    }
  }

  if (!IsListEmpty(&mListHead)) {
    currentPage = (ErrorRecord *)GetFirstNode(&mListHead);
    return EFI_SUCCESS;
  }

  else {
    return EFI_ABORTED;
  }
}

/**
 *  This function processes the results of changes in configuration.
 *
 *  @param[in]  This               Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
 *  @param[in]  Action             Specifies the type of action taken by the browser.
 *  @param[in]  QuestionId         A unique value which is sent to the original
 *                                 exporting driver so that it can identify the type
 *                                 of data to expect.
 *  @param[in]  Type               The type of value for the question.
 *  @param[in]  Value              A pointer to the data being sent to the original
 *                                 exporting driver.
 *  @param[out] ActionRequest      On return, points to the action requested by the
 *                                 callback function.
 *
 *  @retval EFI_SUCCESS            The callback successfully handled the action.
 *  @retval EFI_OUT_OF_RESOURCES   Not enough storage is available to hold the
 *                                 variable and its data.
 *  @retval EFI_DEVICE_ERROR       The variable could not be saved.
 *  @retval EFI_UNSUPPORTED        The specified Action is not supported by the
 *                                 callback.
**/
EFI_STATUS
EFIAPI
DriverCallback(
    IN     CONST EFI_HII_CONFIG_ACCESS_PROTOCOL *This,
    IN           EFI_BROWSER_ACTION             Action,
    IN           EFI_QUESTION_ID                QuestionId,
    IN           UINT8                          Type,
    IN           EFI_IFR_TYPE_VALUE             *Value,
       OUT       EFI_BROWSER_ACTION_REQUEST   *ActionRequest
    )
{
  EFI_STATUS Status = EFI_SUCCESS;
  *ActionRequest    = EFI_BROWSER_ACTION_REQUEST_NONE;

  DEBUG((DEBUG_INFO, "*Hii - Hwh* - Question ID=0x%08x Type=0x%04x Action=0x%04x Value=0x%lx\n", QuestionId, Type, Action, Value->u64));

  switch (Action) {

  case EFI_BROWSER_ACTION_FORM_OPEN:

    // Capture form opening
    if (QuestionId == HWH_MENU_LEFT_ID) {

      // Make sure there are errors to display. If not, set the configuration
      // to suppress most of the page and set the "No Logs To Display" string at top
      if (currentPage == NULL && mHwhMenuConfiguration.Logs != LOGS_FALSE) {

        if (EFI_ERROR(PopulateWheaErrorList())) {
          mHwhMenuConfiguration.Logs = LOGS_FALSE;
          UpdateForm();
          *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_APPLY;
        }
      
      }

      UpdateDisplayStrings();
    }
    break;

  case EFI_BROWSER_ACTION_FORM_CLOSE:

    // Capture form closing
    if (QuestionId == HWH_MENU_LEFT_ID) {
      currentPage = (ErrorRecord *)GetFirstNode(&mListHead);
    }
    break;

  case EFI_BROWSER_ACTION_CHANGED:
    //Rely on short-circuiting of && statement to avoid paging when unnecessary
    if ((QuestionId == HWH_MENU_RIGHT_ID && PageForward()) || (QuestionId == HWH_MENU_LEFT_ID && PageBackward())) {
      UpdateDisplayStrings();
      UpdateForm();
      *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_APPLY;
    }
    break;

  default:
    break;
  }

  return Status;
}

/**
 *  This function processes the results of changes in configuration.
 *
 *  @param[in]  This               Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
 *  @param[in]  Configuration      A null-terminated Unicode string in <ConfigResp>
 *                                 format.
 *  @param[out]  Progress          A pointer to a string filled in with the offset of
 *                                 the most recent '&' before the first failing
 *                                 name/value pair (or the beginning of the string if
 *                                 the failure is in the first name/value pair) or
 *                                 the terminating NULL if all was successful.
 *
 *  @retval EFI_SUCCESS            The Results is processed successfully.
 *  @retval EFI_INVALID_PARAMETER  Configuration is NULL.
 *  @retval EFI_NOT_FOUND          Routing data doesn't match any storage in this
 *                                 driver.
 *
 **/
EFI_STATUS
EFIAPI
RouteConfig(
    IN     CONST EFI_HII_CONFIG_ACCESS_PROTOCOL *This,
    IN     CONST EFI_STRING                     Configuration,
       OUT       EFI_STRING                     *Progress)
{
  EFI_STATUS Status;

  if (Configuration == NULL || Progress == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  if (Configuration == NULL) {
    return EFI_UNSUPPORTED;
  }
  if (StrStr(Configuration, L"OFFSET") == NULL) {
    return EFI_UNSUPPORTED;
  }
  Status = EFI_SUCCESS;

  DEBUG((DEBUG_INFO, "%a: complete. Code = %r\n", __FUNCTION__, Status));
  return Status;
}

/**
 *  This function allows a caller to extract the current configuration for one
 *  or more named elements from the target driver.
 *
 *  @param[in]   This              Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
 *  @param[in]   Request           A null-terminated Unicode string in
 *                                 <ConfigRequest> format.
 *  @param[out]  Progress          On return, points to a character in the Request
 *                                 string. Points to the string's null terminator if
 *                                 request was successful. Points to the most recent
 *                                 '&' before the first failing name/value pair (or
 *                                 the beginning of the string if the failure is in
 *                                 the first name/value pair) if the request was not
 *                                 successful.
 *  @param[out]  Results           A null-terminated Unicode string in
 *                                 <ConfigAltResp> format which has all values filled
 *                                 in for the names in the Request string. String to
 *                                 be allocated by the called function.
 *
 *  @retval EFI_SUCCESS            The Results is filled with the requested values.
 *  @retval EFI_OUT_OF_RESOURCES   Not enough memory to store the results.
 *  @retval EFI_INVALID_PARAMETER  Request is illegal syntax, or unknown name.
 *  @retval EFI_NOT_FOUND          Routing data doesn't match any storage in this
 *                                 driver.
 *
 **/
EFI_STATUS
EFIAPI
ExtractConfig(
    IN     CONST EFI_HII_CONFIG_ACCESS_PROTOCOL *This,
    IN     CONST EFI_STRING                     Request,
       OUT       EFI_STRING                     *Progress,
       OUT       EFI_STRING                     *Results)
{
  EFI_STATUS Status;

  if (Progress == NULL || Results == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  if (Request == NULL) {
    return EFI_UNSUPPORTED;
  }
  if (StrStr(Request, L"OFFSET") == NULL) {
    return EFI_UNSUPPORTED;
  }

  // The Request string may be truncated as it is long.  Ensure \n gets out
  DEBUG((DEBUG_INFO, "%a: Request=%s\n", __FUNCTION__));
  DEBUG((DEBUG_INFO, "%s", Request));
  DEBUG((DEBUG_INFO, "\n"));

  if (HiiIsConfigHdrMatch(Request, &gHwhMenuFormsetGuid, L"HwhMenuConfig")) {
    Status = gHiiConfigRouting->BlockToConfig(gHiiConfigRouting,
                                              Request,
                                              (UINT8 *)&mHwhMenuConfiguration,
                                              sizeof(mHwhMenuConfiguration),
                                              Results,
                                              Progress);

    DEBUG((DEBUG_INFO, "%a: Size is %d, Code=%r\n", __FUNCTION__, sizeof(mHwhMenuConfiguration), Status));
  }

  Status = EFI_SUCCESS;
  DEBUG((DEBUG_INFO, "%a: complete. Code = %r\n", __FUNCTION__, Status));
  return Status;
}

/**
*  This function is the main entry of the Hardware Health Menu application.
*
*  @param[in]   ImageHandle
*  @param[in]   SystemTable
*
**/
EFI_STATUS
EFIAPI
HwhMenuEntry(
    IN EFI_HANDLE       ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable)
{
  EFI_STATUS Status;

  Status = gBS->InstallMultipleProtocolInterfaces(&mHwhMenuPrivate.DriverHandle,
                                                  &gEfiDevicePathProtocolGuid,
                                                  &mHiiVendorDevicePath,
                                                  &gEfiHiiConfigAccessProtocolGuid,
                                                  &mHwhMenuPrivate.ConfigAccess,
                                                  NULL);

  mHwhMenuPrivate.HiiHandle = HiiAddPackages(&gHwhMenuFormsetGuid,
                                             mHwhMenuPrivate.DriverHandle,
                                             HwhMenuVfrBin,
                                             HwhMenuStrings,
                                             NULL);

  return Status;
}
