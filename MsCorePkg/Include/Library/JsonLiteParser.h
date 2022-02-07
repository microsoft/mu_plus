/** @file
JsonLiteParser.h

Library for limited parsing of JSON files

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __JSON_LITE_H__
#define __JSON_LITE_H__

//
// The limited format Json string is in the format:
//
//    { "ASCII-Identifier"  : "ASCII-Value",
//      "ASCII-Identifier1" : "ASCII-Value",
//      "ASCII-Identifier3" : "ASCII-Value",
//      "ASCII-Identifier4" : "ASCII-Value" }
//
// where the ASCII fields include all characters 0x01-0xFF excluding 0x22 (the double quote "). 0x00
// is the NULL terminator.  ASCII is not validated, and may include some UTF-8 special characters, but
// should not have an effect on parsing.  If there is a leading quote, parsing ends at an ending quote
// that only has whitespace, comma, or close brace.
//
// An embedded NULL in the string will only stop parsing the string.
//
// A value (data to the right of the ':') may only be a quoted string, decimal number, or the word null.
//
// JSON_REQUEST_ELEMENT notes:
//
// The FieldName and Value are NOT NULL terminated strings. The FieldLen and ValueLen
// are character counts, without the quotes.
//
typedef struct {
  CONST CHAR8    *FieldName;
  UINTN          FieldLen;
  CONST CHAR8    *Value;
  UINTN          ValueLen;
} JSON_REQUEST_ELEMENT;

#define JSON_NULL  "null"

/**
 *  Function to process a Json Element
 *
 * @param[in]  Json Request Element   Element Being Processed
 * @param[in]  Context for the Process Function
 *
 * @retval EFI_SUCCESS -       Packet processed normally
 * @retval Error -             Error processing packet
 */
typedef
EFI_STATUS
(EFIAPI *JSON_PROCESS_ELEMENT)(
  IN  JSON_REQUEST_ELEMENT *JsonElement,
  IN  VOID                 *Context
  );

/**
 * EncodeJson
 *
 * @param[in]  Request Array
 * @param[in]  Request Count    - Number of entries in the array
 * @param[out] Json String      - Where to store pointer to Json String
 * @param[out] Json String Size - Where to store Json String Size
 *
 * Don't confuse this routine for a real Json Encoder.  This code is for the
 * expected Dfci request packets. Strict formatting is required, and
 * comments are not allowed.
 *
 * The caller is responsible for freeing the returned Json String;
 *
 **/
EFI_STATUS
EFIAPI
JsonLibEncode (
  IN  JSON_REQUEST_ELEMENT  *Request,
  IN  UINTN                 RequestCount,
  OUT CHAR8                 **JsonString,
  OUT UINTN                 *JsonStringSize
  );

/**
 * ParseJson
 *
 * @param[in]      JsonString
 * @param[in]      JsonStringLength   Number of characters in the string
 * @param[in]      Function to process an element
 * @param[in]      Context for the process function
 *
 * Don't confuse this routine for a real Json Parser.  This code is for the
 * expected Dfci request blobs, and consist of a maximum of 6 name value pairs.
 *
 * JsonString will be modified by the parse action.
 * JsonElementArray will be initialized to all zeros before processing
 *
 * returns    EFI_STATUS    EFI_SUCCESS   - Processed at least one JSON element
 *                          EFI_NOT_FOUND - All json elements had null values.
 *                          other         - internal errors
 **/
EFI_STATUS
EFIAPI
JsonLibParse (
  IN  CHAR8                 *JsonString,
  IN  UINTN                 JsonStringSize,
  IN  JSON_PROCESS_ELEMENT  ApplyFunction,
  IN  VOID                  *Context
  );

#endif // __JSON_LITE_H__
