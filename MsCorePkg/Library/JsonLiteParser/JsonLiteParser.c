/** @file
DfciJson.c

This module will encode and decode Dfci JSON like packets.

Copyright (c) 2018, Microsoft Corporation

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

**/

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/JsonLiteParser.h>
#include <Library/MemoryAllocationLib.h>

//
//  MACRO to Skip all characters up to the next ". Stop
//  on a NULL character.
//
#define SKIP_TO_NEXT_QUOTE(a) \
    while (('\"' != *a) &&    \
           ('\0' != *a)) {    \
        a++;                  \
    }

//
//  MACRO to Skip blanks, tabs, \r, and \n characters.
//
#define SKIP_WHITE_SPACE(a) \
    while ((' '  == *a) ||  \
           ('\t' == *a) ||  \
           ('\n' == *a) ||  \
           ('\r' == *a)) {  \
        a++;                \
    }

/**
  LocalAsciiStrCat

  Similar to AsciiStrCatS, but the source string is a count of characters with no NULL
**/
STATIC
EFI_STATUS
LocalAsciiStrCatS (
          CHAR8 *Dest,
          UINTN  DestMax,
    CONST CHAR8 *Src,
          UINTN  Count
    ) {

    while ((*Dest != '\0') && (DestMax > 0)){
        Dest++;
        DestMax--;
    }

    if (Count > DestMax) {
        return EFI_BUFFER_TOO_SMALL;
    }

    while (Count > 0) {
        if (*Src == '\0') {
            *Dest = '\0';
            return EFI_INVALID_PARAMETER;
        }

        *Dest++ = *Src++;
        Count--;
    }

    *Dest = '\0';
    return EFI_SUCCESS;
}
/**
 * EncodeJson
 *
 * @param[in]  Request Array
 * @param[in]  Request Count    - Number of entries in the array
 * @param[out] Json String      - Where to store pointer to Json String
 * @param[out] Json String Size - Where to store Json String Size
 *
 * Don't confuse this routine for a real Json Encoder.  This code is for the
 * expected Dfci request blobs. Comments are not allowed.
 *
 * The caller is responsible for freeing the returned Json String;
 *
 **/
EFI_STATUS
EFIAPI
JsonLibEncode (
    IN  JSON_REQUEST_ELEMENT *Request,
    IN  UINTN                 RequestCount,
    OUT CHAR8               **JsonString,
    OUT UINTN                *JsonStringSize) {

    UINTN      i;
    UINTN      j;
    UINTN      ValueLen;
    BOOLEAN    NeedQuotes;
    CHAR8     *RequestBuffer;
    UINTN      RequestSize;
    EFI_STATUS Status;

    if ((NULL == Request) || (0 == RequestCount) || (NULL == JsonString) | (NULL == JsonStringSize)) {
        return EFI_INVALID_PARAMETER;
    }

    //
    // Account for
    //    2                  the enclosing braces {}
    //    3*RequestCount     " characters for the name, and the : separator
    //    1*RequestCount     for the , separators and the terminating NULL
    //
    RequestSize = 2 + (4 * RequestCount);
    Status = EFI_SUCCESS;

    for (i = 0; i < RequestCount; i++) {
        if (NULL != Request[i].Value) {
            ValueLen = Request[i].ValueLen;
            NeedQuotes = FALSE;
            for (j=0; (j < Request[i].ValueLen) && !NeedQuotes; j++) {
                if ((Request[i].Value[j] < '0') ||
                    (Request[i].Value[j] > '9')) {
                    NeedQuotes = TRUE;
                }
            }
            if (NeedQuotes) {
                RequestSize += (2 * sizeof(CHAR8));
            }
        } else {
            ValueLen = AsciiStrLen(JSON_NULL);
        }
        RequestSize += Request[i].FieldLen + ValueLen;
    }

    RequestBuffer = AllocatePool (RequestSize);
    if (NULL == RequestBuffer) {
        return EFI_OUT_OF_RESOURCES;
    }

    RequestBuffer[0] = '{';
    RequestBuffer[1] = '\0';
    for (i = 0; i < RequestCount; i++) {
        if (0 != i) {
            Status |= AsciiStrCatS (RequestBuffer, RequestSize, ",");
        }

        Status |= AsciiStrCatS (RequestBuffer, RequestSize, "\"");
        Status |= LocalAsciiStrCatS (RequestBuffer, RequestSize, Request[i].FieldName, Request[i].FieldLen);
        Status |= AsciiStrCatS (RequestBuffer, RequestSize, "\":");
        if (NULL != Request[i].Value) {
            NeedQuotes = FALSE;
            for (j=0; (j < Request[i].ValueLen) && !NeedQuotes; j++) {
                if ((Request[i].Value[j] < '0') ||
                    (Request[i].Value[j] > '9')) {
                    NeedQuotes = TRUE;
                }
            }

            if (NeedQuotes) {
                Status |= AsciiStrCatS (RequestBuffer, RequestSize, "\"");
            }

            Status |= LocalAsciiStrCatS (RequestBuffer, RequestSize, Request[i].Value, Request[i].ValueLen);
            if (NeedQuotes) {
                Status |= AsciiStrCatS (RequestBuffer, RequestSize, "\"");
            }
        } else {
            Status |= AsciiStrCatS (RequestBuffer, RequestSize, JSON_NULL);
            RequestSize = RequestSize - (2 * sizeof(CHAR8));
        }

        if (EFI_ERROR(Status)) {
            break;
        }
    }

    Status |= AsciiStrCatS (RequestBuffer, RequestSize, "}");

    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR,"Error parsing encode request.  Code may be =%r\n", Status));
        FreePool (RequestBuffer);
    } else {
        DEBUG((DEBUG_INFO,"Request Buffer:\n"));
        DEBUG_BUFFER(DEBUG_INFO, RequestBuffer, RequestSize, (DEBUG_DM_PRINT_OFFSET | DEBUG_DM_PRINT_ASCII));
        *JsonString = RequestBuffer;
        *JsonStringSize = RequestSize;
    }

    return Status;
}

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
    IN  CHAR8               *JsonString,
    IN  UINTN                JsonStringSize,
    IN  JSON_PROCESS_ELEMENT ProcessFunction,
    IN  VOID                *Context
  ) {

    CHAR8               *JsonChar;
    UINTN                Length;
    JSON_REQUEST_ELEMENT Rqst;
    EFI_STATUS           Status;
    BOOLEAN              Processed;
    BOOLEAN              Changed;
    BOOLEAN              SkipNULL;


    if ((NULL == JsonString) || (NULL == ProcessFunction) || (0 == JsonStringSize)) {
            DEBUG((DEBUG_INFO,"Parse buffer received NULL buffer or NULL function\n"));
            return EFI_INVALID_PARAMETER;
    }

    Processed = FALSE;
    Changed = FALSE;
    DEBUG((DEBUG_INFO,"Parse buffer @ %p, Size = %d:\n", JsonString, JsonStringSize));
    DEBUG_BUFFER(DEBUG_INFO, JsonString, JsonStringSize, (DEBUG_DM_PRINT_OFFSET | DEBUG_DM_PRINT_ASCII));

    Length = AsciiStrnLenS (JsonString, JsonStringSize);
    if (Length == JsonStringSize) {
        DEBUG((DEBUG_ERROR,"No NULL in JsonString\n"));
        return EFI_INVALID_PARAMETER;
    }

    JsonChar = JsonString;

    SKIP_WHITE_SPACE (JsonChar);

    // Consume start character
    if ('{' != *JsonChar) {
        DEBUG((DEBUG_INFO,"Invalid Json Start character\n"));
        return EFI_INVALID_PARAMETER;
    }

    JsonChar++;
    while (TRUE) {

        SKIP_WHITE_SPACE (JsonChar);
        DEBUG((DEBUG_ERROR,"Parsing at %p\n", JsonChar));
        // Expect a quoted name
        if ('\"' != *JsonChar) {
            DEBUG((DEBUG_INFO,"Name did not start with a quote\n"));
            return EFI_INVALID_PARAMETER;
        }

        JsonChar++;
        Rqst.FieldName = JsonChar;

        SKIP_TO_NEXT_QUOTE (JsonChar);

        if ('\"' != *JsonChar) {
            DEBUG((DEBUG_INFO,"Name did not end with a quote\n"));
            return EFI_INVALID_PARAMETER;
        }

        Rqst.FieldLen = JsonChar - Rqst.FieldName;   // Len does not include quote
        JsonChar++;

        SKIP_WHITE_SPACE (JsonChar);

        if (':' != *JsonChar) {
            DEBUG((DEBUG_INFO,"Value separator incorrect\n"));
            return EFI_INVALID_PARAMETER;
        }

        JsonChar++;
        SKIP_WHITE_SPACE (JsonChar);

        // The value may be quoted, the word null, or a sequence of decimal digits.
        SkipNULL = FALSE;
        switch (*JsonChar) {
            case '\"':          // Handle quoted value
                JsonChar++;     // Skip quote
                Rqst.Value = JsonChar;

                SKIP_TO_NEXT_QUOTE (JsonChar);

                if ('\"' != *JsonChar) {
                    DEBUG((DEBUG_ERROR,"Value did not end with a quote\n"));
                    return EFI_INVALID_PARAMETER;
                }
                Rqst.ValueLen = JsonChar - Rqst.Value;  // Len does not include trailing quote
                JsonChar++;  // Skip trailing quote
            break;

            case 'n':           // Handle the word null
                JsonChar++;
                if ('u' != *JsonChar++) {
                    goto INVALID;
                }

                if ('l' != *JsonChar++) {
                    goto INVALID;
                }

                if ('l' != *JsonChar++) {
INVALID:
                    DEBUG((DEBUG_ERROR, "Invalid value\n"));
                    return EFI_INVALID_PARAMETER;
                }

                SkipNULL = TRUE;
            break;

            default:
                Rqst.Value = JsonChar;
                if ((*JsonChar < '0') || (*JsonChar > '9')) {
                    goto INVALID;
                }

                JsonChar++;
                while ((*JsonChar >= '0') && (*JsonChar <= '9')) {
                    JsonChar++;
                }
                                                           // JsonChar now points to next item
                Rqst.ValueLen = JsonChar - Rqst.Value;
            break;
        }

        if (!SkipNULL) {
            Status = (ProcessFunction) (&Rqst, Context);
            if (EFI_MEDIA_CHANGED == Status) {
                Status = EFI_SUCCESS;
                Changed = TRUE;
                DEBUG((DEBUG_INFO, "Media Changed from Process Function\n"));
            }

            if (EFI_ERROR(Status)) {
                DEBUG((DEBUG_ERROR, "Error from Element Apply. Code = %r\n",Status));
                return Status;
            }
        }

        Processed = TRUE;

        SKIP_WHITE_SPACE (JsonChar);

        if (',' == *JsonChar) {
            JsonChar++;
            if ('\0' == *JsonChar) {
                DEBUG((DEBUG_ERROR, "End of string without terminator\n"));
                return EFI_INVALID_PARAMETER;
            }

            continue;
        }

        if ('}' == *JsonChar) {
            if (Changed) {
                Status = EFI_MEDIA_CHANGED;
            } else if (Processed) {
                Status = EFI_SUCCESS;
            } else {
                Status = EFI_NOT_FOUND;
            }
            return Status;
        }

        DEBUG((DEBUG_ERROR,"Malformed JsonString\n"));
        return EFI_INVALID_PARAMETER;
    }

    ASSERT(FALSE);  // Cannot get here
    return EFI_INVALID_PARAMETER;
}