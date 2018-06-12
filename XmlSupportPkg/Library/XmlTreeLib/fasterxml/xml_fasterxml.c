/**
XML parsing engine implementation.

Copyright (c) 2016, Microsoft Corporation

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

#include <Uefi.h>                                // UEFI base types
#include <Library/MemoryAllocationLib.h>         // Memory allocation
#include <Library/BaseLib.h>                     // Safe string functions
#include <Library/DebugLib.h>                    // DEBUG tracing
#include <Library/BaseMemoryLib.h>               // SetMem()
#include "fasterxml.h"                           // XML Engine
#include "xmlerr.h"                              // XML Errors.
#include "xmlstructure.h"                        // XML structures

typedef const void *PCVOID;

extern const XML_SIMPLE_STRING xss_CDATA = CONSTANT_XML_SIMPLE_STRING(L"CDATA");
extern const XML_SIMPLE_STRING xss_xml = CONSTANT_XML_SIMPLE_STRING(L"xml");
extern const XML_SIMPLE_STRING xss_encoding = CONSTANT_XML_SIMPLE_STRING(L"encoding");
extern const XML_SIMPLE_STRING xss_standalone = CONSTANT_XML_SIMPLE_STRING(L"standalone");
extern const XML_SIMPLE_STRING xss_version = CONSTANT_XML_SIMPLE_STRING(L"version");
extern const XML_SIMPLE_STRING xss_xmlns = CONSTANT_XML_SIMPLE_STRING(L"xmlns");

extern const XML_SIMPLE_STRING xss_DOCTYPE = CONSTANT_XML_SIMPLE_STRING(L"DOCTYPE");
extern const XML_SIMPLE_STRING xss_ELEMENT = CONSTANT_XML_SIMPLE_STRING(L"ELEMENT");
extern const XML_SIMPLE_STRING xss_ATTLIST = CONSTANT_XML_SIMPLE_STRING(L"ATTLIST");
extern const XML_SIMPLE_STRING xss_NOTATION = CONSTANT_XML_SIMPLE_STRING(L"NOTATION");
extern const XML_SIMPLE_STRING xss_ENTITY = CONSTANT_XML_SIMPLE_STRING(L"ENTITY");
extern const XML_SIMPLE_STRING xss_NDATA = CONSTANT_XML_SIMPLE_STRING(L"NDATA");
extern const XML_SIMPLE_STRING xss_PUBLIC = CONSTANT_XML_SIMPLE_STRING(L"PUBLIC");
extern const XML_SIMPLE_STRING xss_SYSTEM = CONSTANT_XML_SIMPLE_STRING(L"SYSTEM");
extern const XML_SIMPLE_STRING xss_ID = CONSTANT_XML_SIMPLE_STRING(L"ID");
extern const XML_SIMPLE_STRING xss_IDREF = CONSTANT_XML_SIMPLE_STRING(L"IDREF");
extern const XML_SIMPLE_STRING xss_IDREFS = CONSTANT_XML_SIMPLE_STRING(L"IDREFS");
extern const XML_SIMPLE_STRING xss_ENTITIES = CONSTANT_XML_SIMPLE_STRING(L"ENTITIES");
extern const XML_SIMPLE_STRING xss_NMTOKEN = CONSTANT_XML_SIMPLE_STRING(L"NMTOKEN");
extern const XML_SIMPLE_STRING xss_NMTOKENS = CONSTANT_XML_SIMPLE_STRING(L"NMTOKENS");
extern const XML_SIMPLE_STRING xss_REQUIRED = CONSTANT_XML_SIMPLE_STRING(L"REQUIRED");
extern const XML_SIMPLE_STRING xss_IMPLIED = CONSTANT_XML_SIMPLE_STRING(L"IMPLIED");
extern const XML_SIMPLE_STRING xss_FIXED = CONSTANT_XML_SIMPLE_STRING(L"FIXED");


EFI_STATUS
EFIAPI
RtlXmlDefaultCompareStrings(
  PXML_TOKENIZATION_STATE pState,
  PCXML_EXTENT pLeft,
  PCXML_EXTENT pRight,
  XML_STRING_COMPARE *pfEqual
  )
{
  EFI_STATUS status = EFI_SUCCESS;
  BOOLEAN    fCanDoBinaryCompare = FALSE;

  switch (pState->RawTokenState.EncodingFamily)
  {
  case XMLEF_UTF_8_OR_ASCII:
  case XMLEF_UCS_4_LE:
  case XMLEF_UCS_4_BE:
  case XMLEF_UTF_16_LE:
  case XMLEF_UTF_16_BE:
    fCanDoBinaryCompare = 1;
    break;
  default:
    status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  if (fCanDoBinaryCompare)
  {
    *pfEqual = (pLeft->cbData == pRight->cbData) ?
      XML_STRING_COMPARE_EQUALS : ((pLeft->cbData < pRight->cbData) ? XML_STRING_COMPARE_LT : XML_STRING_COMPARE_GT);
    if (*pfEqual == XML_STRING_COMPARE_EQUALS)
    {
      INTN Comparison = CompareMem(pLeft->pvData, pRight->pvData, (UINTN)pLeft->cbData);
      if (Comparison == 0)
        *pfEqual = XML_STRING_COMPARE_EQUALS;
      else if (Comparison < 0)
        *pfEqual = XML_STRING_COMPARE_LT;
      else
        *pfEqual = XML_STRING_COMPARE_GT;
    }
    goto Exit;
  }
  else
  {
    NTXMLRAWNEXTCHARACTER pfnDecoder;
    VOID* LeftCursor;
    VOID* LeftEnd;
    VOID* RightCursor;
    VOID* RightEnd;

    if (!ARGUMENT_PRESENT(pLeft) || !ARGUMENT_PRESENT(pRight) || !ARGUMENT_PRESENT(pfEqual)) {
      return RtlpReportXmlError(EFI_INVALID_PARAMETER);
    }

    *pfEqual = XML_STRING_COMPARE_EQUALS;

    pfnDecoder = pState->RawTokenState.pfnNextChar;

    LeftCursor = pLeft->pvData;
    LeftEnd = (VOID*)(((UINTN)pLeft->pvData) + (UINTN)pLeft->cbData);

    RightCursor = pRight->pvData;
    RightEnd = (VOID*)(((UINTN)pRight->pvData) + (UINTN)pRight->cbData);

    //
    // Loop through the data until we run out
    //
    while ((LeftCursor < LeftEnd) && (RightCursor < RightEnd))
    {
      XML_RAWTOKENIZATION_RESULT DecodeResult;
      UINT32 LeftCharacter, RightCharacter;

      //
      // Gather the two next characters
      //
      DecodeResult = (*pfnDecoder)(LeftCursor, LeftEnd);
      if (DecodeResult.Character == XML_RAWTOKENIZATION_INVALID_CHARACTER) {
        status = DecodeResult.Result.ErrorCode;
        goto Exit;
      }
      else 
      {
        LeftCharacter = DecodeResult.Character;
        LeftCursor = DecodeResult.Result.NextCursor;
      }

      DecodeResult = (*pfnDecoder)(RightCursor, RightEnd);
      if (DecodeResult.Character == XML_RAWTOKENIZATION_INVALID_CHARACTER) {
        status = DecodeResult.Result.ErrorCode;
        goto Exit;
      }
      else 
      {
        RightCharacter = DecodeResult.Character;
        RightCursor = DecodeResult.Result.NextCursor;
      }

      //
      // Are they equal?
      //
      if (RightCharacter == LeftCharacter) {
        continue;
      }
      //
      // Nope, left is larger
      //
      else if (LeftCharacter > RightCharacter) {
        *pfEqual = XML_STRING_COMPARE_GT;
        goto Exit;
      }
      //
      // Right is larger
      //
      else {
        *pfEqual = XML_STRING_COMPARE_LT;
        goto Exit;
      }
    }

    //
    // There was data left in the right thing
    //
    if (RightCursor < RightEnd) {
      *pfEqual = XML_STRING_COMPARE_LT;
    }
    //
    // There was data left in the left thing
    //
    else if (LeftCursor < LeftEnd) {
      *pfEqual = XML_STRING_COMPARE_GT;
    }
    //
    // Otherwise, it's still equal
    //
  }
Exit:
  return status;
}






EFI_STATUS
EFIAPI
RtlXmlDefaultSpecialStringCompare(
  PXML_TOKENIZATION_STATE pState,
  PCXML_EXTENT            pToken,
  PCXML_SIMPLE_STRING        pSpecialString,
  XML_STRING_COMPARE      *pfMatches,
  NTXMLTRANSFORMCHARACTER Transformation
  )
{
  NTXMLRAWNEXTCHARACTER pfnDecoder;
  VOID* RawCursor;
  VOID* RawEnd;

  CHAR16* StringCursor;
  CHAR16* StringEnd;

  if (!ARGUMENT_PRESENT(pState) || !ARGUMENT_PRESENT(pToken) || !ARGUMENT_PRESENT(pSpecialString) ||
    !ARGUMENT_PRESENT(pfMatches)) {

    return RtlpReportXmlError(EFI_INVALID_PARAMETER);
  }

  pfnDecoder = pState->RawTokenState.pfnNextChar;
  RawCursor = pToken->pvData;
  RawEnd = (VOID*)(((UINTN)pToken->pvData) + (UINTN)pToken->cbData);
  StringCursor = pSpecialString->Buffer;
  StringEnd = (CHAR16*)(((UINTN)StringCursor) + pSpecialString->Length);

  //
  // Rewire the input cursor
  //
  while ((RawCursor < RawEnd) && (StringCursor < StringEnd))
  {
    XML_RAWTOKENIZATION_RESULT Result;
    UINT32 ulStringCharacter;

    ulStringCharacter = *StringCursor++;

    Result = (*pfnDecoder)(RawCursor, RawEnd);
    if (Result.Character == XML_RAWTOKENIZATION_INVALID_CHARACTER) {
      return Result.Result.ErrorCode;
    }

    if (Transformation) {
      ulStringCharacter = (*Transformation)(ulStringCharacter);
      Result.Character = (*Transformation)(Result.Character);
    }

    RawCursor = Result.Result.NextCursor;

    //
    // Out of our range, ick
    //
    if (ulStringCharacter > 0xFFFF) {

      return RtlpReportXmlError(EFI_INVALID_PARAMETER);

    }

    //
    // Not matching characters?
    //
    if (ulStringCharacter != Result.Character) {

      if (ulStringCharacter > Result.Character) {

        *pfMatches = XML_STRING_COMPARE_LT;
        goto Exit;

      }
      else {

        *pfMatches = XML_STRING_COMPARE_GT;
        goto Exit;

      }
    }

  }

  if (RawCursor < RawEnd) {
    *pfMatches = XML_STRING_COMPARE_LT;
  }
  else if (StringCursor < StringEnd) {
    *pfMatches = XML_STRING_COMPARE_GT;
  }
  else {
    *pfMatches = XML_STRING_COMPARE_EQUALS;
  }

Exit:
  return EFI_SUCCESS;
}



#define VALIDATE_XML_STATE(pState) \
    (((pState == NULL) || \
     ((pState->pvCursor == NULL) && (pState->pvXmlData == NULL)) || \
     (((UINT32)((UINT8*)pState->pvCursor - (UINT8*)pState->pvXmlData)) >= pState->Run.cbData)) ? EFI_INVALID_PARAMETER : EFI_SUCCESS)

#define CHECK_VALID_CHARACTER(Test) do { \
    if (!(Test)) { \
        goto IllegalCharacter; \
    } } while (0)

XML_RAWTOKENIZATION_RESULT
EFIAPI
RtlXmlDefaultNextCharacter_UTF8(
  PCVOID pvCursor,
  IN PCVOID pvEnd
  )
{
  XML_RAWTOKENIZATION_RESULT Result;
  UINT8 b1;
  const UINT8 *bCursor;

  const UINT64 InputUINT8sLeft = ((UINT64)pvEnd) - ((UINT64)pvCursor);

  if (pvCursor >= pvEnd)
  {
    Result.Character = XML_RAWTOKENIZATION_INVALID_CHARACTER;
    Result.Result.ErrorCode = STATUS_END_OF_FILE;
    return Result;
  }

  bCursor = pvCursor;
  b1 = *bCursor++;

  if ((b1 & 0x80) == 0x00)
  {
    Result.Character = b1;

    CHECK_VALID_CHARACTER(Result.Character != 0);
  }
  else if ((b1 & 0xe0) == 0xc0)
  {
    CHECK_VALID_CHARACTER(InputUINT8sLeft >= 2);

    {
      const UINT8 b2 = bCursor[0];
      bCursor += 1;

      CHECK_VALID_CHARACTER((b2 & 0xc0) == 0x80);

      Result.Character = ((b1 & 0x1f) << 6) | (b2 & 0x3f);

      CHECK_VALID_CHARACTER(Result.Character >= 0x00000080);
    }

  }
  else if ((b1 & 0xf0) == 0xe0)
  {
    CHECK_VALID_CHARACTER(InputUINT8sLeft >= 3);

    {
      const UINT8 b2 = bCursor[0];
      const UINT8 b3 = bCursor[1];
      bCursor += 2;

      CHECK_VALID_CHARACTER((b2 & 0xc0) == 0x80);
      CHECK_VALID_CHARACTER((b3 & 0xc0) == 0x80);

      Result.Character = ((((b1 & 0x0f) << 6) | (b2 & 0x3f)) << 6) | (b3 & 0x3f);

      CHECK_VALID_CHARACTER(Result.Character >= 0x00000800);
    }

  }
  else if ((b1 & 0xf8) == 0xf0)
  {
    CHECK_VALID_CHARACTER(InputUINT8sLeft >= 4);

    {
      const UINT8 b2 = bCursor[0];
      const UINT8 b3 = bCursor[1];
      const UINT8 b4 = bCursor[2];
      bCursor += 3;

      CHECK_VALID_CHARACTER((b2 & 0xc0) == 0x80);
      CHECK_VALID_CHARACTER((b3 & 0xc0) == 0x80);
      CHECK_VALID_CHARACTER((b4 & 0xc0) == 0x80);

      Result.Character = ((((((b1 & 0x07) << 6) | (b2 & 0x3f)) << 6) | (b3 & 0x3f)) << 6) | (b4 & 0x3f);

      CHECK_VALID_CHARACTER(Result.Character >= 0x00010000);
    }
  }
  else if ((b1 & 0xfc) == 0xf8)
  {
    CHECK_VALID_CHARACTER(InputUINT8sLeft >= 5);

    {
      const UINT8 b2 = bCursor[0];
      const UINT8 b3 = bCursor[1];
      const UINT8 b4 = bCursor[2];
      const UINT8 b5 = bCursor[3];
      bCursor += 4;

      CHECK_VALID_CHARACTER((b2 & 0xc0) == 0x80);
      CHECK_VALID_CHARACTER((b3 & 0xc0) == 0x80);
      CHECK_VALID_CHARACTER((b4 & 0xc0) == 0x80);
      CHECK_VALID_CHARACTER((b5 & 0xc0) == 0x80);

      Result.Character = ((((((((b1 & 0x03) << 6) | (b2 & 0x3f)) << 6) | (b3 & 0x3f)) << 6) | (b4 & 0x3f)) << 6);

      CHECK_VALID_CHARACTER(Result.Character >= 0x00010000);
    }
  }
  else if ((b1 & 0xfe) == 0xfc)
  {
    CHECK_VALID_CHARACTER(InputUINT8sLeft >= 6);

    {
      const UINT8 b2 = bCursor[0];
      const UINT8 b3 = bCursor[1];
      const UINT8 b4 = bCursor[2];
      const UINT8 b5 = bCursor[3];
      const UINT8 b6 = bCursor[4];
      bCursor += 5;

      CHECK_VALID_CHARACTER((b2 & 0xc0) == 0x80);
      CHECK_VALID_CHARACTER((b3 & 0xc0) == 0x80);
      CHECK_VALID_CHARACTER((b4 & 0xc0) == 0x80);
      CHECK_VALID_CHARACTER((b5 & 0xc0) == 0x80);
      CHECK_VALID_CHARACTER((b6 & 0xc0) == 0x80);

      Result.Character = ((((((((((b1 & 0x01) << 6) | (b2 & 0x3f)) << 6) | (b3 & 0x3f)) << 6) | (b4 & 0x3f)) << 6)) << 6) | (b6 & 0x3f);

      CHECK_VALID_CHARACTER(Result.Character >= 0x04000000);
    }
  }
  else
  {
    goto IllegalCharacter;
  }

  Result.Result.NextCursor = (VOID*)bCursor;
  return Result;

IllegalCharacter:
  Result.Result.ErrorCode = STATUS_ILLEGAL_CHARACTER;
  Result.Character = XML_RAWTOKENIZATION_INVALID_CHARACTER;
  return Result;

}


XML_RAWTOKENIZATION_RESULT
EFIAPI
RtlXmlDefaultNextCharacter_UCS4LE(
  PCVOID pvCursor,
  IN PCVOID pvEnd
  )
{
  XML_RAWTOKENIZATION_RESULT Result;

  const UINT64 cLeft = ((UINT64)pvEnd) - ((UINT64)pvCursor);

  if (cLeft < sizeof(UINT32))
  {
    Result.Character = XML_RAWTOKENIZATION_INVALID_CHARACTER;
    Result.Result.ErrorCode = STATUS_END_OF_FILE;
    return Result;
  }
  else
  {
    Result.Character = *((UINT32*)pvCursor);
    Result.Result.NextCursor = (VOID*)(((UINTN)pvCursor) + sizeof(UINT32));
    return Result;
  }
}

XML_RAWTOKENIZATION_RESULT
EFIAPI
RtlXmlDefaultNextCharacter_UCS4BE(
  PCVOID pvCursor,
  IN PCVOID pvEnd
  )
{
  XML_RAWTOKENIZATION_RESULT Result;
  const UINT8* pb = (UINT8*)pvCursor;
  const UINT64 cLeft = ((UINT64)pvEnd) - ((UINT64)pvCursor);

  if (cLeft < sizeof(UINT32))
  {
    Result.Character = XML_RAWTOKENIZATION_INVALID_CHARACTER;
    Result.Result.ErrorCode = STATUS_END_OF_FILE;
  }
  else
  {
    Result.Character =
      (((UINT32)pb[0]) << 24)
      | (((UINT32)pb[1]) << 16)
      | (((UINT32)pb[2]) << 8)
      | pb[3];
    Result.Result.NextCursor = (VOID*)(((UINTN)pvCursor) + sizeof(UINT32));
  }

  return Result;
}


XML_RAWTOKENIZATION_RESULT
EFIAPI
RtlXmlDefaultNextCharacter_UTF16BE(
  PCVOID pvCursor,
  IN PCVOID pvEnd
  )
{
  XML_RAWTOKENIZATION_RESULT Result;

  const UINT8* pb = (UINT8*)pvCursor;
  const UINT64 cLeft = ((UINT64)pvEnd) - ((UINT64)pvCursor);

  if (cLeft < sizeof(UINT16))
  {
    Result.Character = XML_RAWTOKENIZATION_INVALID_CHARACTER;
    Result.Result.ErrorCode = STATUS_END_OF_FILE;
    return Result;
  }
  else
  {
    const UINT16 usFirst = (pb[0] << 8) | pb[1];

    if ((usFirst >= 0xd800) && (usFirst < 0xdc00))
    {
      UINT16 usSecond;

      if (cLeft < (sizeof(UINT16) + sizeof(UINT16)))
        goto InvalidCharacter;

      usSecond = (pb[2] << 8) | pb[3];
      Result.Character = (((usFirst - 0xd800) * 1024) + (usSecond - 0xdc00)) + 0x10000;
      Result.Result.NextCursor = (VOID*)(((UINTN)pvCursor) + (sizeof(UINT16) * 2));
      return Result;
    }
    else
    {
      if ((usFirst >= 0xdc00) && (usFirst <= 0xdfff))
        goto InvalidCharacter;

      Result.Character = usFirst;
      Result.Result.NextCursor = (VOID*)(((UINTN)pvCursor) + sizeof(UINT16));
      return Result;
    }
  }

InvalidCharacter:
  Result.Character = XML_RAWTOKENIZATION_INVALID_CHARACTER;
  Result.Result.ErrorCode = STATUS_ILLEGAL_CHARACTER;
  return Result;
}


XML_RAWTOKENIZATION_RESULT
EFIAPI
RtlXmlDefaultNextCharacter_UTF16LE(
  PCVOID pvCursor,
  IN PCVOID pvEnd
  )
{
  XML_RAWTOKENIZATION_RESULT Result;
  UINT16* pb = (UINT16*)pvCursor;
  const UINT64 cLeft = ((UINT64)pvEnd) - ((UINT64)pvCursor);

  if (cLeft < sizeof(UINT16))
  {
    Result.Character = XML_RAWTOKENIZATION_INVALID_CHARACTER;
    Result.Result.ErrorCode = STATUS_END_OF_FILE;
    return Result;
  }
  else
  {
    const UINT16 usFirst = pb[0];

    if ((usFirst >= 0xd800) && (usFirst < 0xdc00))
    {
      if (cLeft < (sizeof(UINT16) + sizeof(UINT16)))
        goto InvalidCharacter;

      Result.Character = (((usFirst - 0xd800) * 1024) + (pb[1] - 0xdc00)) + 0x10000;
      Result.Result.NextCursor = (VOID*)(((UINTN)pvCursor) + (sizeof(UINT16) * 2));
      return Result;
    }
    else
    {
      //
      // Leading surrogates are bad
      //
      if ((usFirst >= 0xdc00) && (usFirst <= 0xdfff))
        goto InvalidCharacter;

      Result.Character = usFirst;
      Result.Result.NextCursor = (VOID*)(((UINTN)pvCursor) + sizeof(UINT16));
      return Result;
    }
  }

InvalidCharacter:
  Result.Character = XML_RAWTOKENIZATION_INVALID_CHARACTER;
  Result.Result.ErrorCode = STATUS_ILLEGAL_CHARACTER;
  return Result;
}



//
// For now, we're dumb.
//
#define _RtlIsCharacterText(x) (TRUE)

BOOLEAN FORCEINLINE
RtlpIsCharacterLetter(UINT32 ulCharacter)
{
  //
  // BUGBUG: For now, we only care about the US english alphabet
  //
  return (BOOLEAN)(((ulCharacter >= L'a') && (ulCharacter <= L'z')) || ((ulCharacter >= L'A') && (ulCharacter <= L'Z')));
}

BOOLEAN FORCEINLINE
RtlpIsCharacterDigit(UINT32 ulCharacter)
{
  return (BOOLEAN)((ulCharacter >= L'0') && (ulCharacter <= L'9')) == TRUE;
}

BOOLEAN FORCEINLINE
RtlpIsCharacterCombiner(UINT32 ulCharacter)
{
  return FALSE;
}

BOOLEAN FORCEINLINE
RtlpIsCharacterExtender(UINT32 ulCharacter)
{
  return FALSE;
}

NTXML_RAW_TOKEN FORCEINLINE EFIAPI
_RtlpDecodeCharacter(UINT32 ulCharacter) {

  NTXML_RAW_TOKEN RetVal;

  switch (ulCharacter) {
  case L'-':  RetVal = NTXML_RAWTOKEN_DASH;            break;
  case L'.':  RetVal = NTXML_RAWTOKEN_DOT;             break;
  case L'=':  RetVal = NTXML_RAWTOKEN_EQUALS;          break;
  case L'/':  RetVal = NTXML_RAWTOKEN_FORWARDSLASH;    break;
  case L'>':  RetVal = NTXML_RAWTOKEN_GT;              break;
  case L'<':  RetVal = NTXML_RAWTOKEN_LT;              break;
  case L'?':  RetVal = NTXML_RAWTOKEN_QUESTIONMARK;    break;
  case L'\"': RetVal = NTXML_RAWTOKEN_DOUBLEQUOTE;     break;
  case L'\'': RetVal = NTXML_RAWTOKEN_QUOTE;           break;
  case L'[':  RetVal = NTXML_RAWTOKEN_OPENBRACKET;     break;
  case L']':  RetVal = NTXML_RAWTOKEN_CLOSEBRACKET;    break;
  case L'!':  RetVal = NTXML_RAWTOKEN_BANG;            break;
  case L'(':  RetVal = NTXML_RAWTOKEN_OPENPAREN;       break;
  case L')':  RetVal = NTXML_RAWTOKEN_CLOSEPAREN;      break;
  case L'{':  RetVal = NTXML_RAWTOKEN_OPENCURLY;       break;
  case L'}':  RetVal = NTXML_RAWTOKEN_CLOSECURLY;      break;
  case L':':  RetVal = NTXML_RAWTOKEN_COLON;           break;
  case L';':  RetVal = NTXML_RAWTOKEN_SEMICOLON;       break;
  case L'_':  RetVal = NTXML_RAWTOKEN_UNDERSCORE;      break;
  case L'&':  RetVal = NTXML_RAWTOKEN_AMPERSAND;       break;
  case L'#':  RetVal = NTXML_RAWTOKEN_POUNDSIGN;       break;
  case L'%':  RetVal = NTXML_RAWTOKEN_PERCENT;         break;
  case 0x09:
  case 0x0a:
  case 0x0d:
  case 0x20:  RetVal = NTXML_RAWTOKEN_WHITESPACE;      break;

  default:
    if (_RtlIsCharacterText(ulCharacter))
      RetVal = NTXML_RAWTOKEN_TEXT;
    else
      RetVal = NTXML_RAWTOKEN_ERROR;
  }

  return RetVal;
}

VOID
FORCEINLINE
RtlpXmlSetEndOfStream(
  IN PXML_RAWTOKENIZATION_STATE pState,
  OUT PXML_RAW_TOKEN pToken
  )
{
  pToken->Run.cbData = 0;
  pToken->Run.pvData = pState->pvDocumentEnd;
  pToken->Run.Encoding = pState->EncodingFamily;
  pToken->Run.ulCharacters = 0;
  pToken->TokenName = NTXML_RAWTOKEN_END_OF_STREAM;
}

EFI_STATUS
EFIAPI
RtlRawXmlTokenizer_SingleToken(
  PXML_RAWTOKENIZATION_STATE pState,
  PXML_RAW_TOKEN pToken
  )
{
  XML_RAWTOKENIZATION_RESULT Token;

  //
  // Determine if this hits the single-item cache, or we're at the end
  // of the document
  //
  if (pState->pvCursor >= pState->pvDocumentEnd) {
    RtlpXmlSetEndOfStream(pState, pToken);
    return EFI_SUCCESS;
  }

  //
  // Look at the single next input token
  //
  Token = (*pState->pfnNextChar)(pState->pvCursor, pState->pvDocumentEnd);
  if (Token.Character == XML_RAWTOKENIZATION_INVALID_CHARACTER) {
    return Token.Result.ErrorCode;
  }

  //
  // Set up returns
  //
  pToken->Run.pvData = pState->pvCursor;
  pToken->Run.cbData = ((UINT64)Token.Result.NextCursor) - ((UINT64)pState->pvCursor);
  pToken->Run.Encoding = pState->EncodingFamily;
  pToken->Run.ulCharacters = 1;
  pToken->TokenName = _RtlpDecodeCharacter(Token.Character);

  //
  // Update cache
  //
  pState->pvLastCursor = pState->pvCursor;
  pState->LastTokenCache = *pToken;

  return EFI_SUCCESS;
}






EFI_STATUS
EFIAPI
RtlRawXmlTokenizer_GatherWhitespace(
  PXML_RAWTOKENIZATION_STATE pState,
  PXML_RAW_TOKEN pWhitespace,
  PXML_RAW_TOKEN pTerminator
  )
{
  UINT64 ulCharCount = 0;

  VOID* pvCursor = pState->pvCursor;
  VOID* pvEnd = pState->pvDocumentEnd;

  if (pvCursor >= pvEnd) {

    RtlpXmlSetEndOfStream(pState, pWhitespace);

    if (pTerminator != NULL)
    {
      ZeroMem(pTerminator, sizeof(*pTerminator));
    }

    return EFI_SUCCESS;
  }

  //
  // Record starting point
  //
  do
  {
    //
    // Gather a character
    //
    const XML_RAWTOKENIZATION_RESULT Result = (*pState->pfnNextChar)(pvCursor, pvEnd);

    //
    // If this is tab, space, cr or lf, then continue.  Otherwise,
    // quit.
    //
    switch (Result.Character) {
    case XML_RAWTOKENIZATION_INVALID_CHARACTER:
      return Result.Result.ErrorCode;

    case 0x9:
    case 0xa:
    case 0xd:
    case 0x20:
      ulCharCount++;
      break;

    default:
      if (pTerminator) {
        pTerminator->Run.pvData = pvCursor;
        pTerminator->Run.cbData = ((UINT64)Result.Result.NextCursor) - ((UINT64)pvCursor);
        pTerminator->Run.Encoding = pState->EncodingFamily;
        pTerminator->Run.ulCharacters = 1;
        pTerminator->TokenName = _RtlpDecodeCharacter(Result.Character);
      }
      goto Done;
      break;
    }

    //
    // Advance cursor
    //
    pvCursor = Result.Result.NextCursor;
  } while (pvCursor < pvEnd);

  //
  // Hit the end of the document during whitespace?
  //
  if (pTerminator && (pvCursor == pvEnd)) {
    RtlpXmlSetEndOfStream(pState, pTerminator);
  }

  //
  // This label is here b/c if we terminated b/c of not-a-whitespace-thing,
  // then don't bother to compare against the end of the document.
  //
Done:

  //
  // Set up the other stuff in the output.
  //
  pWhitespace->Run.pvData = pState->pvCursor;
  pWhitespace->Run.cbData = ((UINT64)pvCursor) - ((UINT64)pState->pvCursor);
  pWhitespace->Run.ulCharacters = ulCharCount;
  pWhitespace->Run.Encoding = pState->EncodingFamily;
  pWhitespace->TokenName = NTXML_RAWTOKEN_WHITESPACE;

  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
RtlRawXmlTokenizer_GatherPCData(
  PXML_RAWTOKENIZATION_STATE pState,
  PXML_RAW_TOKEN pPcData,
  PXML_RAW_TOKEN pNextRawToken
  )
  /*++

    Purpose:

      Gathers PCDATA (anything that's not a <, &, ]]>, or end of stream) until there's
      no more.

  --*/
{
  VOID* pvCursor;
  VOID* pvEnd;
  UINT64 ulCharCount = 0;

  ZeroMem(pPcData, sizeof(*pPcData));
  pPcData->Run.pvData = pState->pvCursor;
  pPcData->Run.Encoding = pState->EncodingFamily;

  if (pState->pvCursor >= pState->pvDocumentEnd) {
    RtlpXmlSetEndOfStream(pState, pPcData);
    return EFI_SUCCESS;
  }

  pvCursor = pState->pvCursor;
  pvEnd = pState->pvDocumentEnd;

  do {

    const XML_RAWTOKENIZATION_RESULT Result = (*pState->pfnNextChar)(pvCursor, pvEnd);

    switch (Result.Character) {

    case XML_RAWTOKENIZATION_INVALID_CHARACTER:
      return Result.Result.ErrorCode;

      //
      // < terminates PCData, as it's probably the start of
      // a new element.
      //
    case L'<':

      if (pNextRawToken != NULL) {
        pNextRawToken->Run.cbData = ((UINT64)Result.Result.NextCursor) - ((UINT64)pvCursor);
        pNextRawToken->Run.pvData = pvCursor;
        pNextRawToken->Run.Encoding = pState->EncodingFamily;
        pNextRawToken->Run.ulCharacters = 1;
        pNextRawToken->TokenName = NTXML_RAWTOKEN_LT;
      }
      goto NoMore;

      //
      // Everything else is just normal pcdata to use
      //
    default:
      ulCharCount++;
      break;

    }

    //
    // Move the cursor along
    //
    pvCursor = Result.Result.NextCursor;

  } while (pvCursor < pvEnd);

NoMore:
  if (pNextRawToken && (pvCursor >= pvEnd)) {
    RtlpXmlSetEndOfStream(pState, pNextRawToken);
  }

  pPcData->Run.cbData = (UINT64)pvCursor - (UINT64)pState->pvCursor;
  pPcData->Run.ulCharacters = ulCharCount;

  return EFI_SUCCESS;
}



EFI_STATUS
EFIAPI
RtlRawXmlTokenizer_GatherNTokens(
  PXML_RAWTOKENIZATION_STATE pState,
  PXML_RAW_TOKEN pTokens,
  UINT64 ulTokenCount
  )
{
  VOID* pvCursor = pState->pvCursor;
  const VOID* pvDocumentEnd = pState->pvDocumentEnd;

  //
  // If we're at the document end, set all the tokens to the "end" state
  // and return immediately.
  //
  if ((ulTokenCount == 0) || (pvCursor >= pvDocumentEnd)) {
    goto FillEndOfDocumentTokens;
  }

  //
  // While we've got tokens left, and we're not at the end of the
  // document, start grabbing chunklets
  //
  do {

    const XML_RAWTOKENIZATION_RESULT Result = (*pState->pfnNextChar)(pvCursor, pvDocumentEnd);

    //
    // If this was a zero character, then there might have been an error -
    // see if the status was set, and if so, return
    //
    if (Result.Character == XML_RAWTOKENIZATION_INVALID_CHARACTER) {
      return Result.Result.ErrorCode;
    }

    //
    // Decode the name
    //
    pTokens->TokenName = _RtlpDecodeCharacter(Result.Character);
    pTokens->Run.cbData = ((UINT64)Result.Result.NextCursor) - ((UINT64)pvCursor);
    pTokens->Run.pvData = pvCursor;
    pTokens->Run.ulCharacters = 1;
    pTokens->Run.Encoding = pState->EncodingFamily;

    pvCursor = Result.Result.NextCursor;
    pTokens++;

  } while (ulTokenCount-- && (pvCursor < pvDocumentEnd));

  if (ulTokenCount == -1) {
    ulTokenCount = 0;
  }

  //
  // Did we find the end of the document before we ran out of tokens from the
  // input?  Then fill the remainder with the "end of document" token
  //
FillEndOfDocumentTokens:
  if (ulTokenCount) 
  {
    while (ulTokenCount--) 
    {
      RtlpXmlSetEndOfStream(pState, pTokens++);
    }
  }

  return EFI_SUCCESS;
}




EFI_STATUS
EFIAPI
RtlRawXmlTokenizer_GatherIdentifier(
  PXML_RAWTOKENIZATION_STATE pState,
  PXML_RAW_TOKEN pIdentifier,
  PXML_RAW_TOKEN pStoppedOn
  )
{
  XML_RAWTOKENIZATION_RESULT Result;
  VOID* pvCursor = pState->pvCursor;
  const VOID* pvDocumentEnd = pState->pvDocumentEnd;
  UINT64 ulCharCount = 0;


  pIdentifier->Run.cbData = 0;
  pIdentifier->Run.ulCharacters = 0;
  pIdentifier->Run.Encoding = pState->EncodingFamily;

  if (pvCursor >= pvDocumentEnd) {
    RtlpXmlSetEndOfStream(pState, pIdentifier);
    return EFI_SUCCESS;
  }

  //
  // Start up
  //
  pIdentifier->Run.pvData = pvCursor;
  pIdentifier->TokenName = NTXML_RAWTOKEN_ERROR;

  //
  // Start with the first character at the cursor
  Result = (*pState->pfnNextChar)(pvCursor, pvDocumentEnd);

  //
  // Badly formatted name - stop before we get too far.
  //
  if (Result.Character == XML_RAWTOKENIZATION_INVALID_CHARACTER) {
    return Result.Result.ErrorCode;
  }
  //
  // Not a _ or a character is a bad identifier
  //
  else if ((Result.Character != L'_') && !RtlpIsCharacterLetter(Result.Character)) {

    if (pStoppedOn) {
      pStoppedOn->Run.cbData = ((UINT64)Result.Result.NextCursor) - ((UINT64)pvCursor);
      pStoppedOn->Run.pvData = pvCursor;
      pStoppedOn->Run.ulCharacters = 1;
      pStoppedOn->Run.Encoding = pState->EncodingFamily;
      pStoppedOn->TokenName = _RtlpDecodeCharacter(Result.Character);
    }

    return EFI_SUCCESS;
  }

  ulCharCount = 1;

  //
  // Advance cursor, now just look for name characters
  //
  pvCursor = Result.Result.NextCursor;

  //
  // Was that the last character in the input?
  //
  if (pStoppedOn && (pvCursor >= pvDocumentEnd)) {

    RtlpXmlSetEndOfStream(pState, pStoppedOn);
    goto DoneLooking;
  }


  do {

    Result = (*pState->pfnNextChar)(pvCursor, pvDocumentEnd);

    //
    // dots, dashes, and underscores are fine
    //
    switch (Result.Character) {

    case XML_RAWTOKENIZATION_INVALID_CHARACTER:
      return Result.Result.ErrorCode;

    case '.':
    case '_':
    case '-':
      break;

      //
      // If this wasn't a letter, digit, combiner, or extender, stop looking
      //
    default:
      if (!RtlpIsCharacterLetter(Result.Character) && !RtlpIsCharacterDigit(Result.Character) &&
        !RtlpIsCharacterCombiner(Result.Character) && !RtlpIsCharacterExtender(Result.Character)) {

        if (pStoppedOn) {
          pStoppedOn->Run.cbData = ((UINT64)Result.Result.NextCursor) - ((UINT64)pvCursor);
          pStoppedOn->Run.pvData = pvCursor;
          pStoppedOn->Run.ulCharacters = 1;
          pStoppedOn->Run.Encoding = pState->EncodingFamily;
          pStoppedOn->TokenName = _RtlpDecodeCharacter(Result.Character);
        }
        goto DoneLooking;
      }
      break;
    }

    ulCharCount++;
    pvCursor = Result.Result.NextCursor;

  } while (pvCursor < pvDocumentEnd);



DoneLooking:

  if (pStoppedOn && (pvCursor >= pvDocumentEnd)) {
    RtlpXmlSetEndOfStream(pState, pStoppedOn);
  }

  pIdentifier->Run.cbData = ((UINT64)pvCursor) - ((UINT64)pState->pvCursor);
  pIdentifier->Run.pvData = pState->pvCursor;
  pIdentifier->Run.ulCharacters = ulCharCount;
  pIdentifier->Run.Encoding = pState->EncodingFamily;
  pIdentifier->TokenName = NTXML_RAWTOKEN_TEXT;

  return EFI_SUCCESS;
}




EFI_STATUS
EFIAPI
RtlRawXmlTokenizer_GatherUntilOneOrOther(
  PXML_RAWTOKENIZATION_STATE pState,
  PXML_RAW_TOKEN pGathered,
  NTXML_RAW_TOKEN StopOn1,
  NTXML_RAW_TOKEN StopOn2,
  PXML_RAW_TOKEN pTokenFound
  )
{
  VOID* pvCursor = pState->pvCursor;
  VOID* pvDocumentEnd = pState->pvDocumentEnd;
  //UINT64 cbChunk = 0;
  UINT64 ulCharCount = 0;

  pGathered->Run.cbData = 0;
  pGathered->Run.pvData = pvCursor;
  pGathered->Run.Encoding = pState->EncodingFamily;
  pGathered->Run.ulCharacters = 0;

  if (pTokenFound) {
    ZeroMem(pTokenFound, sizeof(*pTokenFound));
  }

  if (pvCursor >= pvDocumentEnd) {
    RtlpXmlSetEndOfStream(pState, pGathered);
    return EFI_SUCCESS;
  }

  do
  {
    const XML_RAWTOKENIZATION_RESULT Result = (*pState->pfnNextChar)(pvCursor, pvDocumentEnd);
    const NTXML_RAW_TOKEN ulDecoded = _RtlpDecodeCharacter(Result.Character);

    //
    // Zero character, and error?  Oops.
    //
    if (Result.Character == XML_RAWTOKENIZATION_INVALID_CHARACTER) {
      return Result.Result.ErrorCode;
    }

    if ((ulDecoded == StopOn1) || (ulDecoded == StopOn2))
    {
      if (pTokenFound) {
        pTokenFound->Run.cbData = ((UINT64)Result.Result.NextCursor) - ((UINT64)pvCursor);
        pTokenFound->Run.pvData = pvCursor;
        pTokenFound->TokenName = ulDecoded;
      }

      break;
    }

    ulCharCount++;
    pvCursor = Result.Result.NextCursor;

  } while (pvCursor < pvDocumentEnd);

  //
  // If we fell off the document, say we did so.
  //
  if ((pvCursor >= pvDocumentEnd) && pTokenFound) {
    pTokenFound->Run.cbData = 0;
    pTokenFound->Run.pvData = pvDocumentEnd;
    pTokenFound->Run.ulCharacters = 0;
    pTokenFound->Run.Encoding = pState->EncodingFamily;
    pTokenFound->TokenName = NTXML_RAWTOKEN_ERROR;
  }

  //
  // Indicate we're done
  //
  pGathered->Run.cbData = ((UINT64)pvCursor) - ((UINT64)pState->pvCursor);
  pGathered->Run.ulCharacters = ulCharCount;

  return EFI_SUCCESS;
}


#define GATHER_ITEM_SETUP(State, Gathered) do { \
    (Gathered)->Run.cbData = 0; \
    (Gathered)->Run.pvData = (State)->pvCursor; \
    (Gathered)->Run.Encoding = (State)->EncodingFamily; \
    (Gathered)->Run.ulCharacters = 0; \
    if ((State)->pvCursor >= (State)->pvDocumentEnd) { \
        (Gathered)->Run.pvData = (State)->pvDocumentEnd; \
        (Gathered)->TokenName = NTXML_RAWTOKEN_END_OF_STREAM; \
        return EFI_SUCCESS; \
    } \
} while (0);


FORCEINLINE
BOOLEAN RtlRawXmlTokenizer_IsValidPubIdCharacter(
  IN UINT32 ulChar,
  IN BOOLEAN fAllowSingleQuote
  )
{
  if (RtlpIsCharacterLetter(ulChar) || RtlpIsCharacterDigit(ulChar))
  {
    return TRUE;
  }
  else
  {
    switch (ulChar)
    {
    case ' ':
    case '\r':
    case '\n':
    case '-':
    case '(':
    case ')':
    case '+':
    case ',':
    case '.':
    case '/':
    case ':':
    case '=':
    case '?':
    case ';':
    case '!':
    case '*':
    case '#':
    case '@':
    case '$':
    case '_':
    case '%':
      return TRUE;

    case '\'':
      return fAllowSingleQuote;
    }
  }

  return FALSE;
}

EFI_STATUS
EFIAPI
RtlRawXmlTokenizer_GatherPubIdLiteral(
  PXML_RAWTOKENIZATION_STATE pState,
  PXML_RAW_TOKEN pGathered
  )
{
  VOID* pvCursor = pState->pvCursor;
  const VOID* pvDocumentEnd = pState->pvDocumentEnd;

  UINT32 ulCharCount = 0;

  GATHER_ITEM_SETUP(pState, pGathered);

  do
  {
    const XML_RAWTOKENIZATION_RESULT Result = (*pState->pfnNextChar)(pvCursor, pvDocumentEnd);
    BOOLEAN fValidCharacter;

    if (Result.Character == XML_RAWTOKENIZATION_INVALID_CHARACTER) {
      return Result.Result.ErrorCode;
    }

    //
    // Check if this is a valid character, remembering that single quotes
    // are only valid if hte pubid literal is enclosed in double quotes
    //
    fValidCharacter = RtlRawXmlTokenizer_IsValidPubIdCharacter(
      Result.Character,
      pState->LastTokenCache.TokenName == NTXML_RAWTOKEN_DOUBLEQUOTE
      );

    if (!fValidCharacter)
      break;

    ulCharCount++;
    pvCursor = Result.Result.NextCursor;

  } while (TRUE);

  pGathered->TokenName = NTXML_RAWTOKEN_TEXT;
  pGathered->Run.cbData = ((UINT64)pvCursor) - ((UINT64)pState->pvCursor);
  pGathered->Run.ulCharacters = ulCharCount;

  return EFI_SUCCESS;

}





EFI_STATUS
EFIAPI
RtlRawXmlTokenizer_GatherUntil(
  PXML_RAWTOKENIZATION_STATE pState,
  PXML_RAW_TOKEN pGathered,
  NTXML_RAW_TOKEN StopOn,
  PXML_RAW_TOKEN pTokenFound
  )
{
  //UINT32 cbChunk = 0;
  NTXML_RAW_TOKEN ulDecoded;
  UINT32 ulCharCount = 0;

  VOID* pvCursor = pState->pvCursor;
  VOID* pvDocumentEnd = pState->pvDocumentEnd;

  if (pTokenFound) {
    ZeroMem(&pTokenFound->Run, sizeof pTokenFound->Run);

    if (pvCursor >= pvDocumentEnd) {
      pTokenFound->TokenName = NTXML_RAWTOKEN_END_OF_STREAM;
    }
    else {
      pTokenFound->TokenName = NTXML_RAWTOKEN_ERROR;
    }
  }

  GATHER_ITEM_SETUP(pState, pGathered);

  do
  {
    const XML_RAWTOKENIZATION_RESULT Result = (*pState->pfnNextChar)(pvCursor, pvDocumentEnd);

    //
    // Zero character, and error?  Oops.
    //
    if (Result.Character == XML_RAWTOKENIZATION_INVALID_CHARACTER) {
      return Result.Result.ErrorCode;
    }

    //
    // Found the character we were looking for? Neat.
    //
    if ((ulDecoded = _RtlpDecodeCharacter(Result.Character)) == StopOn) {

      if (pTokenFound) {
        pTokenFound->Run.cbData = ((UINT64)Result.Result.NextCursor) - ((UINT64)pvCursor);
        pTokenFound->Run.pvData = pvCursor;
        pTokenFound->Run.ulCharacters = 1;
        pTokenFound->Run.Encoding = pState->EncodingFamily;
        pTokenFound->TokenName = ulDecoded;
      }

      break;
    }

    //
    // Otherwise, add on the UINT8s in token
    //
    ulCharCount++;
    pvCursor = Result.Result.NextCursor;
  } while (pvCursor < pvDocumentEnd);

  //
  // If we fell off the document, say we did so.
  //
  if ((pvCursor >= pvDocumentEnd) && pTokenFound) {
    pTokenFound->Run.cbData = 0;
    pTokenFound->Run.pvData = pvDocumentEnd;
    pTokenFound->Run.ulCharacters = 0;
    pTokenFound->Run.Encoding = pState->EncodingFamily;
    pTokenFound->TokenName = NTXML_RAWTOKEN_ERROR;
  }

  //
  // Indicate we're done
  //
  pGathered->Run.cbData = ((UINT64)pvCursor) - ((UINT64)pState->pvCursor);
  pGathered->Run.ulCharacters = ulCharCount;

  return EFI_SUCCESS;
}


//
// TODO: Naive implementation gathers an identifier and compares it
// TODO: to the string.  Smarter implementations gather a character
// TODO: at a time and compare as they go along.
//
EFI_STATUS
EFIAPI
RtlXmlTokenizer_ExpectIdentifier(
  IN_OUT PXML_TOKENIZATION_STATE pState,
  IN PCXML_SIMPLE_STRING pExpectedString,
  OUT PXML_RAW_TOKEN pGatheredString,
  OUT XML_STRING_COMPARE *pfStringMatched
  )
{
  EFI_STATUS status = RtlRawXmlTokenizer_GatherIdentifier(&pState->RawTokenState, pGatheredString, NULL);

  if (pfStringMatched)
    *pfStringMatched = XML_STRING_COMPARE_LT;

  if (EFI_ERROR(status) || (pGatheredString->TokenName != NTXML_RAWTOKEN_TEXT)) {
    return status;
  }
  else {
    status = pState->pfnCompareSpecialString(
      pState,
      &pGatheredString->Run,
      pExpectedString,
      pfStringMatched,
      NULL
      );

    if (EFI_ERROR(status)) {
      return status;
    }
  }

  return EFI_SUCCESS;

}

#define GET_SINGLE_TOKEN(state, token) do { \
    const EFI_STATUS __status = RtlRawXmlTokenizer_SingleToken(state, token); \
    if (EFI_ERROR(__status)) return __status; \
    cbTotalTokenLength += (token)->Run.cbData; \
} while (0)


static
EFI_STATUS
EFIAPI
_HandleDocTypeDeclStuff(
  PXML_TOKENIZATION_STATE pState,
  PXML_TOKEN pToken,
  UINT64 *pcbTotalTokenLength,
  XML_TOKENIZATION_SPECIFIC_STATE *pResultState
  )
{
  EFI_STATUS status = EFI_SUCCESS;
  PXML_RAW_TOKEN pRawToken = &pState->RawTokenScratch[0];
  PXML_RAW_TOKEN pNextRawToken = &pState->RawTokenScratch[1];
  UINT64 cbTotalTokenLength = 0;
  XML_TOKENIZATION_SPECIFIC_STATE NextState = *pResultState;
  PXML_RAWTOKENIZATION_STATE pRawState = &pState->RawTokenState;

  const XML_TOKENIZATION_SPECIFIC_STATE StartState = pToken->State;

  switch (StartState)
  {
  default:
    ASSERT(FALSE);
    break;

    //
    // Parsing entity declarations is hard.
    //

    //
    // The open of an entity might be followed by "S % S", or it might be "S name S"
    //
  case XTSS_DOCTYPE_ENTITYDECL_OPEN:

    //
    // The reader for <!ENTITY may have swallowed trailing whitespace,
    // but just in case re-scan whitespace and whatever follow.
    //
    status = RtlRawXmlTokenizer_GatherWhitespace(pRawState, pRawToken, pNextRawToken);
    if (EFI_ERROR(status)) {
      return status;
    }

    if (pRawToken->Run.cbData != 0) {
      cbTotalTokenLength = pNextRawToken->Run.cbData;
    }

    if (pNextRawToken->TokenName == NTXML_RAWTOKEN_PERCENT) {

      cbTotalTokenLength += pNextRawToken->Run.cbData;
      NextState = XTSS_DOCTYPE_ENTITYDECL_PARAMETERMARKER;

      //
      // Snarf whitespace after the percent to avoid a wierd state...
      //
      ADVANCE_PVOID(pRawState->pvCursor, (UINTN)cbTotalTokenLength);

      status = RtlRawXmlTokenizer_GatherWhitespace(pRawState, pRawToken, NULL);
      if (EFI_ERROR(status)) {
        return status;
      }

      cbTotalTokenLength += pRawToken->Run.cbData;
    }
    //
    // If there's text next, then we found the "general entity" marker
    // (ie: the lack of a percent sign), so report that back.  The length
    // of this token is the length of the whitespace only.  Since we found
    // text, there's no whitespace after this either.
    //
    else if (pNextRawToken->TokenName == NTXML_RAWTOKEN_TEXT) {

      NextState = XTSS_DOCTYPE_ENTITYDECL_GENERALMARKER;

    }
    else {
      pToken->fError = TRUE;
    }

    break;

    //
    // After both the 'general' and the 'parameter' markers should
    // come an identifier.
    //
  case XTSS_DOCTYPE_ENTITYDECL_GENERALMARKER:
  case XTSS_DOCTYPE_ENTITYDECL_PARAMETERMARKER:

    status = RtlRawXmlTokenizer_GatherIdentifier(pRawState, pRawToken, NULL);
    if (EFI_ERROR(status)) {
      return status;
    }

    cbTotalTokenLength = pRawToken->Run.cbData;
    if ((pRawToken->TokenName == NTXML_RAWTOKEN_TEXT) &&
      (pRawToken->Run.cbData != 0))
    {
      NextState = XTSS_DOCTYPE_ENTITYDECL_NAME;
    }
    else {
      pToken->fError = TRUE;
    }

    break;



    //
    // Post a name comes a set of weirdnesses that requires some
    // hefty work to figure out what the next state is.
    //
    // " or ' -> VALUE_OPEN
    // SYSTEM -> SYSTEM
    // PUBLIC -> PUBLIC
    //
  case XTSS_DOCTYPE_ENTITYDECL_NAME:

    status = RtlRawXmlTokenizer_GatherWhitespace(pRawState, pRawToken, pNextRawToken);
    if (EFI_ERROR(status)) {
      return status;
    }

    cbTotalTokenLength = pRawToken->Run.cbData;

    //
    // Quote or doublequote begins the value part of the name
    //
    if ((pNextRawToken->TokenName == NTXML_RAWTOKEN_QUOTE) ||
      (pNextRawToken->TokenName == NTXML_RAWTOKEN_DOUBLEQUOTE))
    {
      cbTotalTokenLength += pNextRawToken->Run.cbData;
      pState->QuoteTemp = pNextRawToken->TokenName;
      NextState = XTSS_DOCTYPE_ENTITYDECL_VALUE_OPEN;
    }
    //
    // Text? We expect either SYSTEM or PUBLIC here.
    //
    else if (pNextRawToken->TokenName == NTXML_RAWTOKEN_TEXT)
    {
      XML_STRING_COMPARE fCompare;

      ADVANCE_PVOID(pRawState->pvCursor, pRawToken->Run.cbData);

      status = RtlXmlTokenizer_ExpectIdentifier(pState, &xss_PUBLIC, pRawToken, &fCompare);
      if (EFI_ERROR(status)) {
        return status;
      }

      if (fCompare == XML_STRING_COMPARE_EQUALS) {
        NextState = XTSS_DOCTYPE_ENTITYDECL_PUBLIC;
      }
      else {
        status = RtlXmlTokenizer_ExpectIdentifier(pState, &xss_SYSTEM, pRawToken, &fCompare);
        if (EFI_ERROR(status)) {
          return status;
        }

        if (fCompare == XML_STRING_COMPARE_EQUALS) {
          NextState = XTSS_DOCTYPE_ENTITYDECL_SYSTEM;
        }
        else {
          pToken->fError = TRUE;
        }
      }

      //
      // Gather any whitespace after the quote as well.
      //
      cbTotalTokenLength += pRawToken->Run.cbData;
      if (!pToken->fError) {

        ADVANCE_PVOID(pRawState->pvCursor, pRawToken->Run.cbData);

        status = RtlRawXmlTokenizer_GatherWhitespace(pRawState, pRawToken, NULL);
        if (EFI_ERROR(status)) {
          return status;
        }

        cbTotalTokenLength += pRawToken->Run.cbData;
      }

    }

    break;

    //
    // After SYSTEM comes a quote of some sort, mayhaps prefaced
    // by some whitespace.
    //
  case XTSS_DOCTYPE_ENTITYDECL_SYSTEM:

    status = RtlRawXmlTokenizer_GatherWhitespace(pRawState, pRawToken, pNextRawToken);
    if (EFI_ERROR(status)) {
      return status;
    }

    cbTotalTokenLength = pRawToken->Run.cbData + pNextRawToken->Run.cbData;

    if ((pNextRawToken->TokenName == NTXML_RAWTOKEN_QUOTE) ||
      (pNextRawToken->TokenName == NTXML_RAWTOKEN_DOUBLEQUOTE))
    {
      pState->QuoteTemp = pNextRawToken->TokenName;
      NextState = XTSS_DOCTYPE_ENTITYDECL_SYSTEM_TEXT_OPEN;
    }
    else {
      pToken->fError = TRUE;
    }

    break;

    //
    // After an open comes whatever they want until a close
    // of the same quote type as the open.
    //
  case XTSS_DOCTYPE_ENTITYDECL_VALUE_OPEN:
  case XTSS_DOCTYPE_ENTITYDECL_SYSTEM_TEXT_OPEN:

    status = RtlRawXmlTokenizer_GatherUntil(pRawState, pRawToken, pState->QuoteTemp, NULL);
    if (EFI_ERROR(status)) {
      return status;
    }

    cbTotalTokenLength = pRawToken->Run.cbData;

    switch (StartState) {
    case XTSS_DOCTYPE_ENTITYDECL_VALUE_OPEN:
      NextState = XTSS_DOCTYPE_ENTITYDECL_VALUE_VALUE;
      break;
    case XTSS_DOCTYPE_ENTITYDECL_SYSTEM_TEXT_OPEN:
      NextState = XTSS_DOCTYPE_ENTITYDECL_SYSTEM_TEXT_VALUE;
      break;
    }

    break;

    //
    // From a system text value must follow a close quote.
    //
  case XTSS_DOCTYPE_ENTITYDECL_SYSTEM_TEXT_VALUE:

    status = RtlRawXmlTokenizer_SingleToken(pRawState, pRawToken);
    if (EFI_ERROR(status)) {
      return status;
    }

    cbTotalTokenLength = pRawToken->Run.cbData;

    if (pRawToken->TokenName != pState->QuoteTemp) {
      pToken->fError = TRUE;
    }
    //
    // Oh, and grab some more of the whitespace after the quote as the
    // remainder of the close
    //
    else {
      NextState = XTSS_DOCTYPE_ENTITYDECL_SYSTEM_TEXT_CLOSE;

      ADVANCE_PVOID(pRawState->pvCursor, pRawToken->Run.cbData);
      status = RtlRawXmlTokenizer_GatherWhitespace(pRawState, pRawToken, NULL);
      if (EFI_ERROR(status)) {
        return status;
      }

      cbTotalTokenLength += pRawToken->Run.cbData;
    }

    break;

    //
    // After a text close might be the NDATA declaration, or it might be the
    // end of the entity - >.  We require that the cursor be positioned at the
    // next thing past whitespace.
    //
  case XTSS_DOCTYPE_ENTITYDECL_SYSTEM_TEXT_CLOSE:

    status = RtlRawXmlTokenizer_SingleToken(pRawState, pRawToken);
    if (EFI_ERROR(status)) {
      return status;
    }

    cbTotalTokenLength = pRawToken->Run.cbData;

    if (pRawToken->TokenName == NTXML_RAWTOKEN_GT) {

      NextState = XTSS_DOCTYPE_ENTITYDECL_CLOSE;

    }
    //
    // If we found text, it better be 'ndata'
    //
    else if (pRawToken->TokenName == NTXML_RAWTOKEN_TEXT) {

      XML_STRING_COMPARE Compare;

      status = RtlXmlTokenizer_ExpectIdentifier(pState, &xss_NDATA, pRawToken, &Compare);
      if (EFI_ERROR(status)) {
        return status;
      }

      cbTotalTokenLength = pRawToken->Run.cbData;

      if (Compare == XML_STRING_COMPARE_EQUALS) {
        ADVANCE_PVOID(pRawState->pvCursor, cbTotalTokenLength);

        status = RtlRawXmlTokenizer_GatherWhitespace(pRawState, pRawToken, NULL);
        if (EFI_ERROR(status)) {
          return status;
        }

        cbTotalTokenLength += pRawToken->Run.cbData;
        NextState = XTSS_DOCTYPE_ENTITYDECL_NDATA;
      }
      else {
        pToken->fError = TRUE;
      }
    }

    break;


    //
    // From a PUBLIC, we look for a quote.  We should be positioned after
    // any whitespace in the PUBLIC declaration.
    //
  case XTSS_DOCTYPE_ENTITYDECL_PUBLIC:

    status = RtlRawXmlTokenizer_SingleToken(pRawState, pRawToken);
    if (EFI_ERROR(status)) {
      return status;
    }

    cbTotalTokenLength = pRawToken->Run.cbData;
    if ((pRawToken->TokenName == NTXML_RAWTOKEN_QUOTE) ||
      (pRawToken->TokenName == NTXML_RAWTOKEN_DOUBLEQUOTE))
    {
      NextState = XTSS_DOCTYPE_ENTITYDECL_PUBLIC_TEXT_OPEN;
      pState->QuoteTemp = pRawToken->TokenName;
    }
    else {
      pToken->fError = TRUE;
    }

    break;


    //
    // After the 'open' of a public comes a wacky small set of valid characters
    // that make up the PubidLiteral
    //
  case XTSS_DOCTYPE_ENTITYDECL_PUBLIC_TEXT_OPEN:
    status = RtlRawXmlTokenizer_GatherPubIdLiteral(pRawState, pRawToken);
    if (EFI_ERROR(status)) {
      return status;
    }

    cbTotalTokenLength = pRawToken->Run.cbData;
    NextState = XTSS_DOCTYPE_ENTITYDECL_PUBLIC_TEXT_VALUE;
    break;

    //
    // After the value must come a matching close quote.  We'll gather up
    // trailing whitespace afterwards like a good citizen.
    //
  case XTSS_DOCTYPE_ENTITYDECL_PUBLIC_TEXT_VALUE:

    GET_SINGLE_TOKEN(pRawState, pRawToken);

    if (pRawToken->TokenName == pState->QuoteTemp) {
      NextState = XTSS_DOCTYPE_ENTITYDECL_PUBLIC_TEXT_CLOSE;

      ADVANCE_PVOID(pRawState->pvCursor, cbTotalTokenLength);
      status = RtlRawXmlTokenizer_GatherWhitespace(pRawState, pRawToken, NULL);
      if (EFI_ERROR(status)) {
        return status;
      }

      cbTotalTokenLength += pRawToken->Run.cbData;
    }
    else {
      pToken->fError = TRUE;
    }

    break;

    //
    // At the 'close' of a public text part, we should find another quoteish thing
    // that is the start of the system literal id.  Note that we already zipped past
    // the end of any whitespace.
    //
  case XTSS_DOCTYPE_ENTITYDECL_PUBLIC_TEXT_CLOSE:

    GET_SINGLE_TOKEN(pRawState, pRawToken);

    if ((pRawToken->TokenName == NTXML_RAWTOKEN_QUOTE) ||
      (pRawToken->TokenName == NTXML_RAWTOKEN_DOUBLEQUOTE))
    {
      NextState = XTSS_DOCTYPE_ENTITYDECL_SYSTEM_TEXT_OPEN;
      pState->QuoteTemp = pRawToken->TokenName;
    }
    else {
      pToken->fError = TRUE;
    }

    break;


    //
    // At the ndata state, we should get only name
    //
  case XTSS_DOCTYPE_ENTITYDECL_NDATA:

    status = RtlRawXmlTokenizer_GatherIdentifier(pRawState, pRawToken, NULL);
    if (EFI_ERROR(status)) {
      return status;
    }

    cbTotalTokenLength = pRawToken->Run.cbData;
    if (pRawToken->TokenName != NTXML_RAWTOKEN_TEXT) {
      pToken->fError = TRUE;
    }
    else {
      NextState = XTSS_DOCTYPE_ENTITYDECL_NDATA_TEXT;
    }

    break;

    //
    // In <!ENTITY foo "bar"> or <!ENTITY foo SYSTEM "Foob" NDATA gif>, we're positioned
    // after the last " in "bar", or after the f in gif.  We expect some whitespace in
    // either case, followed by a close >.
    //
  case XTSS_DOCTYPE_ENTITYDECL_VALUE_CLOSE:
  case XTSS_DOCTYPE_ENTITYDECL_NDATA_TEXT:

    status = RtlRawXmlTokenizer_GatherWhitespace(pRawState, pRawToken, pNextRawToken);
    if (EFI_ERROR(status)) {
      return status;
    }

    cbTotalTokenLength = pRawToken->Run.cbData + pNextRawToken->Run.cbData;
    if (pNextRawToken->TokenName == NTXML_RAWTOKEN_GT) {
      NextState = XTSS_DOCTYPE_ENTITYDECL_CLOSE;
    }
    else {
      pToken->fError = TRUE;
    }

    break;


    //
    // The value part must terminate in a closing quote.
    //
  case XTSS_DOCTYPE_ENTITYDECL_VALUE_VALUE:

    GET_SINGLE_TOKEN(pRawState, pRawToken);

    if (pRawToken->TokenName == pState->QuoteTemp) {
      NextState = XTSS_DOCTYPE_ENTITYDECL_VALUE_CLOSE;
    }
    else {
      pToken->fError = TRUE;
    }

    break;

  case XTSS_DOCTYPE_ATTLISTDECL_OPEN:

    status = RtlRawXmlTokenizer_GatherWhitespace(pRawState, pRawToken, pNextRawToken);
    if (EFI_ERROR(status))
      return status;

    // remove any whitespace
    cbTotalTokenLength = pRawToken->Run.cbData;
    if (cbTotalTokenLength > 0)
    {
      NextState = XTSS_DOCTYPE_ATTLISTDECL_OPEN;
      break;
    }

    ADVANCE_PVOID(pRawState->pvCursor, pRawToken->Run.cbData);

    // The <!ATTLIST must be immediately followed by a name
    status = RtlRawXmlTokenizer_GatherIdentifier(pRawState, pRawToken, pNextRawToken);
    if (EFI_ERROR(status))
      return status;

    cbTotalTokenLength += pRawToken->Run.cbData;
    if ((pRawToken->TokenName == NTXML_RAWTOKEN_TEXT) &&
      (pRawToken->Run.cbData != 0))
    {
      if (pNextRawToken->TokenName == NTXML_RAWTOKEN_COLON)
        NextState = XTSS_DOCTYPE_ATTLISTDECL_ELEMENT_PREFIX;
      else
        NextState = XTSS_DOCTYPE_ATTLISTDECL_ELEMENT_NAME;
    }
    else
    {
      pToken->fError = TRUE;
    }

    break;

  case XTSS_DOCTYPE_ATTLISTDECL_ELEMENT_COLON:

    // The colon is followed by the local name
    status = RtlRawXmlTokenizer_GatherIdentifier(pRawState, pRawToken, pNextRawToken);
    if (EFI_ERROR(status))
      return status;

    cbTotalTokenLength = pRawToken->Run.cbData;
    if ((pRawToken->TokenName == NTXML_RAWTOKEN_TEXT) &&
      (pRawToken->Run.cbData != 0))
    {
      NextState = XTSS_DOCTYPE_ATTLISTDECL_ELEMENT_NAME;
    }
    else
    {
      pToken->fError = TRUE;
    }

    break;

  case XTSS_DOCTYPE_ATTLISTDECL_ELEMENT_PREFIX:

    status = RtlRawXmlTokenizer_SingleToken(pRawState, pRawToken);
    if (EFI_ERROR(status))
      return status;

    if (pRawToken->TokenName != NTXML_RAWTOKEN_COLON)
      pToken->fError = TRUE;

    cbTotalTokenLength = pRawToken->Run.cbData;
    NextState = XTSS_DOCTYPE_ATTLISTDECL_ELEMENT_COLON;
    break;

  case XTSS_DOCTYPE_ATTLISTDECL_DEFAULT_TEXT_CLOSE:
  case XTSS_DOCTYPE_ATTLISTDECL_DEFAULT_IMPLIED:
  case XTSS_DOCTYPE_ATTLISTDECL_DEFAULT_REQUIRED:
  case XTSS_DOCTYPE_ATTLISTDECL_WHITESPACE:
  case XTSS_DOCTYPE_ATTLISTDECL_ELEMENT_NAME:

    status = RtlRawXmlTokenizer_GatherWhitespace(&pState->RawTokenState, pRawToken, pNextRawToken);
    if (EFI_ERROR(status))
      return status;

    if (pRawToken->Run.cbData > 0)
    {
      cbTotalTokenLength = pRawToken->Run.cbData;
      NextState = XTSS_DOCTYPE_ATTLISTDECL_WHITESPACE;
    }
    else if (pNextRawToken->TokenName == NTXML_RAWTOKEN_GT)
    {
      // We've reached the end of the ATTLIST
      cbTotalTokenLength += pNextRawToken->Run.cbData;
      NextState = XTSS_DOCTYPE_ATTLISTDECL_CLOSE;
    }
    else if (pNextRawToken->TokenName == NTXML_RAWTOKEN_TEXT)
    {
      // The next identifier is an attribute name or prefix
      status = RtlRawXmlTokenizer_GatherIdentifier(pRawState, pRawToken, pNextRawToken);
      if (EFI_ERROR(status))
        return status;

      cbTotalTokenLength = pRawToken->Run.cbData;

      if (pNextRawToken->TokenName == NTXML_RAWTOKEN_COLON)
        NextState = XTSS_DOCTYPE_ATTLISTDECL_ATTPREFIX;
      else
        NextState = XTSS_DOCTYPE_ATTLISTDECL_ATTNAME;
    }
    else
    {
      pToken->fError = TRUE;
    }

    break;

  case XTSS_DOCTYPE_ATTLISTDECL_ATTCOLON:

    // The colon is followed by the local name
    status = RtlRawXmlTokenizer_GatherIdentifier(pRawState, pRawToken, pNextRawToken);
    if (EFI_ERROR(status))
      return status;

    cbTotalTokenLength = pRawToken->Run.cbData;
    if ((pRawToken->TokenName == NTXML_RAWTOKEN_TEXT) &&
      (pRawToken->Run.cbData != 0))
    {
      NextState = XTSS_DOCTYPE_ATTLISTDECL_ATTNAME;
    }
    else
    {
      pToken->fError = TRUE;
    }

    break;

  case XTSS_DOCTYPE_ATTLISTDECL_ATTPREFIX:

    status = RtlRawXmlTokenizer_SingleToken(pRawState, pRawToken);
    if (EFI_ERROR(status))
      return status;

    if (pRawToken->TokenName != NTXML_RAWTOKEN_COLON)
      pToken->fError = TRUE;

    cbTotalTokenLength = pRawToken->Run.cbData;
    NextState = XTSS_DOCTYPE_ATTLISTDECL_ATTCOLON;
    break;


  case XTSS_DOCTYPE_ATTLISTDECL_ATTNAME:

    status = RtlRawXmlTokenizer_GatherWhitespace(pRawState, pRawToken, pNextRawToken);
    if (EFI_ERROR(status))
      return status;

    cbTotalTokenLength = pRawToken->Run.cbData;

    // Open paren begins an enumerated type
    if (pNextRawToken->TokenName == NTXML_RAWTOKEN_OPENPAREN)
    {
      cbTotalTokenLength += pNextRawToken->Run.cbData;
      NextState = XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_ENUMERATED_OPEN;
    }
    else if (pNextRawToken->TokenName == NTXML_RAWTOKEN_TEXT)
    {
      // There are several different types that could appear here
      XML_STRING_COMPARE fCompare;

      ADVANCE_PVOID(pRawState->pvCursor, pRawToken->Run.cbData);

      status = RtlXmlTokenizer_ExpectIdentifier(pState, &xss_CDATA, pRawToken, &fCompare);
      if (EFI_ERROR(status))
        return status;

      if (fCompare == XML_STRING_COMPARE_EQUALS)
      {
        cbTotalTokenLength += pRawToken->Run.cbData;
        NextState = XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_CDATA;
        break;
      }

      status = RtlXmlTokenizer_ExpectIdentifier(pState, &xss_ID, pRawToken, &fCompare);
      if (EFI_ERROR(status)) {
        return status;
      }

      if (fCompare == XML_STRING_COMPARE_EQUALS)
      {
        cbTotalTokenLength += pRawToken->Run.cbData;
        NextState = XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_ID;
        break;
      }

      status = RtlXmlTokenizer_ExpectIdentifier(pState, &xss_IDREF, pRawToken, &fCompare);
      if (EFI_ERROR(status)) {
        return status;
      }

      if (fCompare == XML_STRING_COMPARE_EQUALS)
      {
        cbTotalTokenLength += pRawToken->Run.cbData;
        NextState = XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_IDREF;
        break;
      }

      status = RtlXmlTokenizer_ExpectIdentifier(pState, &xss_IDREFS, pRawToken, &fCompare);
      if (EFI_ERROR(status)) {
        return status;
      }

      if (fCompare == XML_STRING_COMPARE_EQUALS)
      {
        cbTotalTokenLength += pRawToken->Run.cbData;
        NextState = XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_IDREFS;
        break;
      }

      status = RtlXmlTokenizer_ExpectIdentifier(pState, &xss_ENTITY, pRawToken, &fCompare);
      if (EFI_ERROR(status)) {
        return status;
      }

      if (fCompare == XML_STRING_COMPARE_EQUALS)
      {
        cbTotalTokenLength += pRawToken->Run.cbData;
        NextState = XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_ENTITY;
        break;
      }

      status = RtlXmlTokenizer_ExpectIdentifier(pState, &xss_ENTITIES, pRawToken, &fCompare);
      if (EFI_ERROR(status)) {
        return status;
      }

      if (fCompare == XML_STRING_COMPARE_EQUALS)
      {
        cbTotalTokenLength += pRawToken->Run.cbData;
        NextState = XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_ENTITIES;
        break;
      }

      status = RtlXmlTokenizer_ExpectIdentifier(pState, &xss_NMTOKEN, pRawToken, &fCompare);
      if (EFI_ERROR(status)) {
        return status;
      }

      if (fCompare == XML_STRING_COMPARE_EQUALS)
      {
        cbTotalTokenLength += pRawToken->Run.cbData;
        NextState = XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_NMTOKEN;
        break;
      }

      status = RtlXmlTokenizer_ExpectIdentifier(pState, &xss_NMTOKENS, pRawToken, &fCompare);
      if (EFI_ERROR(status)) {
        return status;
      }

      if (fCompare == XML_STRING_COMPARE_EQUALS)
      {
        cbTotalTokenLength += pRawToken->Run.cbData;
        NextState = XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_NMTOKENS;
        break;
      }

      status = RtlXmlTokenizer_ExpectIdentifier(pState, &xss_NOTATION, pRawToken, &fCompare);
      if (EFI_ERROR(status)) {
        return status;
      }

      if (fCompare == XML_STRING_COMPARE_EQUALS)
      {
        cbTotalTokenLength += pRawToken->Run.cbData;
        NextState = XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_NOTATION;
        break;
      }

      //
      // Gather any whitespace after the type as well.
      //
      cbTotalTokenLength += pRawToken->Run.cbData;
      if (!pToken->fError)
      {
        ADVANCE_PVOID(pRawState->pvCursor, cbTotalTokenLength);

        status = RtlRawXmlTokenizer_GatherWhitespace(pRawState, pRawToken, NULL);
        if (EFI_ERROR(status))
          return status;

        cbTotalTokenLength += pRawToken->Run.cbData;
      }
    }

    break;

  case XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_NOTATION:

    status = RtlRawXmlTokenizer_GatherWhitespace(&pState->RawTokenState, pRawToken, pNextRawToken);
    if (EFI_ERROR(status))
      return status;

    if (pNextRawToken->TokenName == NTXML_RAWTOKEN_OPENPAREN)
    {
      cbTotalTokenLength = 1 + pRawToken->Run.cbData;
      NextState = XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_ENUMERATED_OPEN;
    }
    else
    {
      pToken->fError = TRUE;
    }

    break;

  case XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_ENUMERATED_OPEN:

    status = RtlRawXmlTokenizer_GatherUntil(pRawState,
      pRawToken, NTXML_RAWTOKEN_CLOSEPAREN, NULL);

    if (EFI_ERROR(status))
      return status;

    cbTotalTokenLength = pRawToken->Run.cbData;
    NextState = XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_ENUMERATED_VALUE;
    break;

  case XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_ENUMERATED_VALUE:

    status = RtlRawXmlTokenizer_SingleToken(pRawState, pRawToken);
    if (EFI_ERROR(status))
      return status;

    if (pRawToken->TokenName == NTXML_RAWTOKEN_CLOSEPAREN)
    {
      cbTotalTokenLength = pRawToken->Run.cbData;
      NextState = XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_ENUMERATED_CLOSE;
    }
    else
    {
      pToken->fError = TRUE;
    }

    break;

  case XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_CDATA:
  case XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_ID:
  case XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_IDREF:
  case XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_IDREFS:
  case XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_ENTITY:
  case XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_ENTITIES:
  case XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_NMTOKEN:
  case XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_NMTOKENS:
  case XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_ENUMERATED_CLOSE:

    status = RtlRawXmlTokenizer_GatherWhitespace(&pState->RawTokenState, pRawToken, pNextRawToken);
    if (EFI_ERROR(status))
      return status;

    cbTotalTokenLength = pRawToken->Run.cbData;
    ADVANCE_PVOID(pRawState->pvCursor, cbTotalTokenLength);

    // After the type, we should see #REQUIRED, #IMPLIED, #FIXED, or
    // a quoted string
    if (pNextRawToken->TokenName == NTXML_RAWTOKEN_POUNDSIGN)
    {
      XML_STRING_COMPARE fCompare;

      // Grab the pound sign
      status = RtlRawXmlTokenizer_SingleToken(pRawState, pRawToken);
      if (EFI_ERROR(status))
        return status;

      cbTotalTokenLength += pRawToken->Run.cbData;
      ADVANCE_PVOID(pRawState->pvCursor, pRawToken->Run.cbData);

      status = RtlXmlTokenizer_ExpectIdentifier(pState, &xss_IMPLIED, pRawToken, &fCompare);
      if (EFI_ERROR(status))
        return status;

      if (fCompare == XML_STRING_COMPARE_EQUALS)
      {
        cbTotalTokenLength += pRawToken->Run.cbData;
        NextState = XTSS_DOCTYPE_ATTLISTDECL_DEFAULT_IMPLIED;
        break;
      }

      status = RtlXmlTokenizer_ExpectIdentifier(pState, &xss_REQUIRED, pRawToken, &fCompare);
      if (EFI_ERROR(status))
        return status;

      if (fCompare == XML_STRING_COMPARE_EQUALS)
      {
        cbTotalTokenLength += pRawToken->Run.cbData;
        NextState = XTSS_DOCTYPE_ATTLISTDECL_DEFAULT_REQUIRED;
        break;
      }

      status = RtlXmlTokenizer_ExpectIdentifier(pState, &xss_FIXED, pRawToken, &fCompare);
      if (EFI_ERROR(status))
        return status;

      if (fCompare == XML_STRING_COMPARE_EQUALS)
      {
        cbTotalTokenLength += pRawToken->Run.cbData;
        NextState = XTSS_DOCTYPE_ATTLISTDECL_DEFAULT_FIXED;
        break;
      }

      pToken->fError = TRUE;
    }
    else if (pNextRawToken->TokenName == NTXML_RAWTOKEN_QUOTE ||
      pNextRawToken->TokenName == NTXML_RAWTOKEN_DOUBLEQUOTE)
    {
      pState->QuoteTemp = pNextRawToken->TokenName;
      cbTotalTokenLength += pNextRawToken->Run.cbData;
      NextState = XTSS_DOCTYPE_ATTLISTDECL_DEFAULT_TEXT_OPEN;
    }
    else
    {
      pToken->fError = TRUE;
    }

    break;

  case XTSS_DOCTYPE_ATTLISTDECL_DEFAULT_FIXED:

    // After #FIXED must come a default value
    status = RtlRawXmlTokenizer_GatherWhitespace(&pState->RawTokenState, pRawToken, pNextRawToken);
    if (EFI_ERROR(status))
      return status;

    cbTotalTokenLength = pRawToken->Run.cbData;
    ADVANCE_PVOID(pRawState->pvCursor, cbTotalTokenLength);

    if (pNextRawToken->TokenName == NTXML_RAWTOKEN_QUOTE ||
      pNextRawToken->TokenName == NTXML_RAWTOKEN_DOUBLEQUOTE)
    {
      pState->QuoteTemp = pNextRawToken->TokenName;
      cbTotalTokenLength += pNextRawToken->Run.cbData;
      NextState = XTSS_DOCTYPE_ATTLISTDECL_DEFAULT_TEXT_OPEN;
    }
    else
    {
      pToken->fError = TRUE;
    }

    break;

  case XTSS_DOCTYPE_ATTLISTDECL_DEFAULT_TEXT_OPEN:

    // Read everything up to the closing quote
    status = RtlRawXmlTokenizer_GatherUntil(
      pRawState,
      pRawToken,
      pState->QuoteTemp,
      NULL
      );

    if (EFI_ERROR(status))
      return status;

    cbTotalTokenLength = pRawToken->Run.cbData;
    NextState = XTSS_DOCTYPE_ATTLISTDECL_DEFAULT_TEXT_VALUE;
    break;

  case XTSS_DOCTYPE_ATTLISTDECL_DEFAULT_TEXT_VALUE:

    status = RtlRawXmlTokenizer_SingleToken(pRawState, pRawToken);
    if (EFI_ERROR(status))
      return status;

    if (pRawToken->TokenName != pState->QuoteTemp)
      return RtlpReportXmlError(STATUS_XML_PARSE_ERROR);

    cbTotalTokenLength = pRawToken->Run.cbData;
    NextState = XTSS_DOCTYPE_ATTLISTDECL_DEFAULT_TEXT_CLOSE;
    break;

    //
    // After the 'open', gather everything until >.
    //
  case XTSS_DOCTYPE_ELEMENTDECL_OPEN:
  case XTSS_DOCTYPE_NOTATIONDECL_OPEN:

    status = RtlRawXmlTokenizer_GatherUntil(
      pRawState,
      pRawToken,
      NTXML_RAWTOKEN_GT,
      NULL
      );

    if (EFI_ERROR(status)) {
      return status;
    }

    cbTotalTokenLength = pRawToken->Run.cbData;

    switch (StartState) {
    case XTSS_DOCTYPE_ELEMENTDECL_OPEN:
      NextState = XTSS_DOCTYPE_ELEMENTDECL_CONTENT;
      break;
    case XTSS_DOCTYPE_NOTATIONDECL_OPEN:
      NextState = XTSS_DOCTYPE_NOTATIONDECL_CONTENT;
      break;
    }

    break;

    //
    // After content and a > comes whitespace in the doctype decl.
    //
  case XTSS_DOCTYPE_ELEMENTDECL_CONTENT:
  case XTSS_DOCTYPE_NOTATIONDECL_CONTENT:

    status = RtlRawXmlTokenizer_SingleToken(pRawState, pRawToken);
    if (EFI_ERROR(status)) {
      return status;
    }

    cbTotalTokenLength = pRawToken->Run.cbData;
    if (pRawToken->TokenName != NTXML_RAWTOKEN_GT) {
      pToken->fError = TRUE;
    }
    else
    {
      switch (StartState)
      {
      case XTSS_DOCTYPE_ELEMENTDECL_CONTENT:
        NextState = XTSS_DOCTYPE_ELEMENTDECL_CLOSE;
        break;
      case XTSS_DOCTYPE_NOTATIONDECL_CONTENT:
        NextState = XTSS_DOCTYPE_NOTATIONDECL_CLOSE;
        break;
      }
    }

    break;

    //
    // The close of markup is followed by whitespace and a lt.
    //
  case XTSS_DOCTYPE_MARKUP_CLOSE:

    status = RtlRawXmlTokenizer_GatherWhitespace(pRawState, pRawToken, pNextRawToken);
    if (EFI_ERROR(status)) {
      return status;
    }

    if (pRawToken->TokenName == NTXML_RAWTOKEN_ERROR) {
      pToken->fError = TRUE;
      break;
    }

    //
    // The nice thing about gathering whitespace is that 'pRawToken' will
    // always be NTXML_RAWTOKEN_ERROR or NTXML_RAWTOKEN_WHITESPACE, even if
    // it's zero-UINT8s long.
    //
    cbTotalTokenLength = pRawToken->Run.cbData + pNextRawToken->Run.cbData;
    ASSERT(pRawToken->TokenName == NTXML_RAWTOKEN_WHITESPACE);

    if (pNextRawToken->TokenName == NTXML_RAWTOKEN_GT) {
      NextState = XTSS_DOCTYPE_CLOSE;
    }
    else {
      pToken->fError = TRUE;
    }

    break;

    //
    // Document type declarations must be followed by whitespace.
    //
  case XTSS_DOCTYPE_OPEN:
    status = RtlRawXmlTokenizer_GatherWhitespace(pRawState, pRawToken, NULL);

    if (EFI_ERROR(status)) {
      return status;
    }

    cbTotalTokenLength = pRawToken->Run.cbData;
    if ((cbTotalTokenLength > 0) && (pRawToken->TokenName == NTXML_RAWTOKEN_WHITESPACE)) {
      NextState = XTSS_DOCTYPE_WHITESPACE;
    }
    else {
      pToken->fError = TRUE;
    }

    break;

    //
    // If we're at the 'whitespace' of a doctypedecl, then the next thing
    // must be an identifier - whether it's the name of the document or the
    // PUBLIC or SYSTEM types.
    //
  case XTSS_DOCTYPE_WHITESPACE:

    status = RtlRawXmlTokenizer_GatherWhitespace(&pState->RawTokenState, pRawToken, pNextRawToken);
    if (EFI_ERROR(status)) {
      return status;
    }

    //
    // Whitespace leads to more whitespace
    //
    if ((pRawToken->Run.cbData > 0) && (pRawToken->TokenName == NTXML_RAWTOKEN_WHITESPACE)) {

      cbTotalTokenLength = pRawToken->Run.cbData;
      NextState = XTSS_DOCTYPE_WHITESPACE;

    }
    //
    // The next thing that's not whitespace must be an identifier.  Gather it up.
    //
    else if (pNextRawToken->TokenName == NTXML_RAWTOKEN_TEXT) {

      ADVANCE_PVOID(pState->RawTokenState.pvCursor, pRawToken->Run.cbData);

      status = RtlRawXmlTokenizer_GatherIdentifier(
        &pState->RawTokenState,
        pRawToken,
        pNextRawToken
        );

      if (EFI_ERROR(status)) {
        return status;
      }
      else if ((pRawToken->Run.cbData != 0) && (pRawToken->TokenName == NTXML_RAWTOKEN_TEXT)) {
        //
        // Nifty - our new token length is the length of the identifier found, so
        // turn around and emit it out as the name of this doctype.
        //
        NextState = XTSS_DOCTYPE_DOCNAME;
        cbTotalTokenLength = pRawToken->Run.cbData;
      }
      else {
        pToken->fError = TRUE;
        cbTotalTokenLength = pNextRawToken->Run.cbData;
      }

    }
    //
    // Oops, neither an identifier nor whitespace?  Bogus.
    //
    else {

      cbTotalTokenLength = pRawToken->Run.cbData;
      pToken->fError = TRUE;

    }

    break;

    //
    // After the document name, we just gather everything we can until we find either
    // the [ or the >.  If we do find something, then it becomes the externalid of the
    // document type, which we don't want to have to worry about parsing at this point.
    //
  case XTSS_DOCTYPE_DOCNAME:

    status = RtlRawXmlTokenizer_GatherUntilOneOrOther(
      &pState->RawTokenState,
      pRawToken,
      NTXML_RAWTOKEN_OPENBRACKET,
      NTXML_RAWTOKEN_GT,
      NULL
      );

    if (EFI_ERROR(status)) {
      return status;
    }

    //
    // If there was stuff in what we gathered, then that's the external ID (even if
    // it includes whitespace...)  We have the side-effect here of always indicating
    // "external id found" even if there was no external ID really found.  While
    // sleazy, it makes our logic easier.
    //
    cbTotalTokenLength = pRawToken->Run.cbData;
    NextState = XTSS_DOCTYPE_EXTERNALID;
    break;

    //
    // After the external ID, one should expect either > or [.
    //
  case XTSS_DOCTYPE_EXTERNALID:

    status = RtlRawXmlTokenizer_SingleToken(
      &pState->RawTokenState,
      pRawToken
      );

    if (EFI_ERROR(status)) {
      return status;
    }

    cbTotalTokenLength = pRawToken->Run.cbData;
    if (pRawToken->TokenName == NTXML_RAWTOKEN_GT) {
      NextState = XTSS_DOCTYPE_CLOSE;
    }
    else if (pRawToken->TokenName == NTXML_RAWTOKEN_OPENBRACKET) {
      NextState = XTSS_DOCTYPE_MARKUP_OPEN;
    }
    else {
      pToken->fError = TRUE;
    }
    break;

    //
    // After the open bracket for a doctype, we expect either whitespace or a close
    // bracket, or another <!something ... >
    //
  case XTSS_DOCTYPE_ATTLISTDECL_CLOSE:
  case XTSS_DOCTYPE_ENTITYDECL_CLOSE:
  case XTSS_DOCTYPE_NOTATIONDECL_CLOSE:
  case XTSS_DOCTYPE_ELEMENTDECL_CLOSE:
  case XTSS_DOCTYPE_MARKUP_OPEN:
  case XTSS_DOCTYPE_MARKUP_WHITESPACE:

    GET_SINGLE_TOKEN(pRawState, pRawToken);

    //
    // More whitespace?  Fine, keep going, but gather as much whitespace
    // as possible first.
    //
    if (pRawToken->TokenName == NTXML_RAWTOKEN_WHITESPACE) {

      status = RtlRawXmlTokenizer_GatherWhitespace(pRawState, pNextRawToken, NULL);
      if (EFI_ERROR(status)) {
        return status;
      }

      cbTotalTokenLength = pNextRawToken->Run.cbData;
      NextState = XTSS_DOCTYPE_MARKUP_WHITESPACE;

    }
    //
    // We don't want to see end of stream here.
    //
    else if (pRawToken->TokenName == NTXML_RAWTOKEN_END_OF_STREAM) {

      pToken->fError = TRUE;
    }
    //
    // Close bracket is easy.
    //
    else if (pRawToken->TokenName == NTXML_RAWTOKEN_CLOSEBRACKET) {

      NextState = XTSS_DOCTYPE_MARKUP_CLOSE;

    }
    //
    // We don't support parameter entities
    //
    else if (pRawToken->TokenName == NTXML_RAWTOKEN_PERCENT) {

      return RtlpReportXmlError(STATUS_NOT_IMPLEMENTED);
    }
    //
    // If it's a combination of <!, it might be something we care about.
    //
    else if (pRawToken->TokenName == NTXML_RAWTOKEN_LT) {

      ADVANCE_PVOID(pState->RawTokenState.pvCursor, pRawToken->Run.cbData);
      GET_SINGLE_TOKEN(pRawState, pNextRawToken);

      if (pNextRawToken->TokenName != NTXML_RAWTOKEN_BANG) {

        pToken->fError = TRUE;

      }
      //
      // Ok, we got <!, now see if we can find yet another token from
      // the input stream as an identifier...
      //
      else {

        UINT32 u;

        //
        // This list matches object types to their opening state
        //
        struct {
          PCXML_SIMPLE_STRING ObjectName;
          XML_TOKENIZATION_SPECIFIC_STATE NewState;
        }
        static const DocTypeObjectNaming[] = {
            { &xss_ENTITY, XTSS_DOCTYPE_ENTITYDECL_OPEN },
            { &xss_ELEMENT, XTSS_DOCTYPE_ELEMENTDECL_OPEN },
            { &xss_ATTLIST, XTSS_DOCTYPE_ATTLISTDECL_OPEN },
            { &xss_NOTATION, XTSS_DOCTYPE_NOTATIONDECL_OPEN },
        };

        ADVANCE_PVOID(pState->RawTokenState.pvCursor, pNextRawToken->Run.cbData);

        for (u = 0; u != RTL_NUMBER_OF(DocTypeObjectNaming); u++)
        {
          XML_STRING_COMPARE Compare;

          status = RtlXmlTokenizer_ExpectIdentifier(
            pState,
            DocTypeObjectNaming[u].ObjectName,
            pRawToken,
            &Compare
            );

          if (EFI_ERROR(status)) {
            return status;
          }

          if (Compare == XML_STRING_COMPARE_EQUALS) {
            NextState = DocTypeObjectNaming[u].NewState;
            cbTotalTokenLength += pRawToken->Run.cbData;
            break;
          }
        }

        if (u == RTL_NUMBER_OF(DocTypeObjectNaming)) {
          cbTotalTokenLength += pRawToken->Run.cbData;
          pToken->fError = TRUE;
        }

      }


    }
    //
    // Some other token that shouldn't be here
    //
    else {

      pToken->fError = TRUE;
    }


    break;
  }

  *pcbTotalTokenLength = cbTotalTokenLength;

  if (*pResultState != NextState) {
    *pResultState = NextState;
  }

  return status;
}





/*++


    At a high level in tokenization, we have a series of base states
    and places we can go next based on what kind of input we start
    getting.

--*/
EFI_STATUS
EFIAPI
RtlXmlNextToken(
  PXML_TOKENIZATION_STATE pState,
  PXML_TOKEN              pToken,
  BOOLEAN                 fAdvanceState
  )
{
  VOID*               pvStarterCursor;
  UINT64              cbTotalTokenLength = 0;
  EFI_STATUS            success;
  PXML_RAW_TOKEN      pNextRawToken;
  PXML_RAW_TOKEN      pRawToken;
  XML_STRING_COMPARE  fCompare = XML_STRING_COMPARE_LT;

  XML_TOKENIZATION_SPECIFIC_STATE PreviousState;
  XML_TOKENIZATION_SPECIFIC_STATE NextState = XTSS_NOTHING;


  if (!ARGUMENT_PRESENT(pState)) {
    return RtlpReportXmlError(EFI_INVALID_PARAMETER_1);
  }

  if (!ARGUMENT_PRESENT(pToken)) {
    return RtlpReportXmlError(EFI_INVALID_PARAMETER_2);
  }

  //
  // Set this up
  //
  pRawToken = &pState->RawTokenScratch[0];
  pNextRawToken = &pState->RawTokenScratch[1];

  pToken->Run.cbData = 0;
  pToken->Run.pvData = pState->RawTokenState.pvCursor;
  pToken->Run.ulCharacters = 0;
  pToken->Run.Encoding = pState->RawTokenState.EncodingFamily;
  pToken->fError = FALSE;


  if (pState->PreviousState == XTSS_STREAM_END) {
    //
    // A little short circuiting - if we're in the "end of stream" logical
    // state, then we can't do anything else - just return status.
    //
    pToken->State = XTSS_STREAM_END;
    return EFI_SUCCESS;
  }


  //
  // Stash this for later diffs
  //
  pvStarterCursor = pState->RawTokenState.pvCursor;



  //
  // Copy these onto the stack for faster lookup during token
  // processing and state detection.
  //
  PreviousState = pState->PreviousState;

  //
  // Set the outbound thing
  //
  pToken->State = PreviousState;

  switch (PreviousState)
  {

    //
    // If we just closed a state, or we're at the start of a stream, or
    // we're in hyperspace, we have to figure out what the next state
    // should be based on the raw token.
    //
  case XTSS_DOCTYPE_CLOSE:
  case XTSS_XMLDECL_CLOSE:
  case XTSS_ELEMENT_CLOSE:
  case XTSS_ELEMENT_CLOSE_EMPTY:
  case XTSS_ENDELEMENT_CLOSE:
  case XTSS_CDATA_CLOSE:
  case XTSS_PI_CLOSE:
  case XTSS_COMMENT_CLOSE:
  case XTSS_STREAM_START:
  case XTSS_STREAM_HYPERSPACE:

    //
    // We always need a token here to see what our next state is
    //
    success = RtlRawXmlTokenizer_SingleToken(&pState->RawTokenState, pRawToken);
    if (EFI_ERROR(success)) {
      pToken->fError = TRUE;
      return success;
    }

    cbTotalTokenLength = pRawToken->Run.cbData;


    //
    // Oh, end of stream.  Goody.
    //
    if (pRawToken->TokenName == NTXML_RAWTOKEN_END_OF_STREAM) {

      if (PreviousState == XTSS_DOCTYPE_CLOSE ||
        PreviousState == XTSS_XMLDECL_CLOSE ||
        PreviousState == XTSS_STREAM_START) {

        //
        // Uh-oh, we never saw the root element
        //
        pToken->fError = TRUE;
      }

      NextState = XTSS_STREAM_END;
    }
    //
    // The < starts a gross bunch of detection code
    //
    else if (pRawToken->TokenName == NTXML_RAWTOKEN_LT) {

      //
      // Acquire the next thing from the input stream, see what it claims to be
      //
      ADVANCE_PVOID(pState->RawTokenState.pvCursor, pRawToken->Run.cbData);
      if (EFI_ERROR(success = RtlRawXmlTokenizer_SingleToken(&pState->RawTokenState, pRawToken))) {
        return success;
      }



      switch (pRawToken->TokenName) {

        //
        // </ is the start of an end-element
        //
      case NTXML_RAWTOKEN_FORWARDSLASH:
        cbTotalTokenLength += pRawToken->Run.cbData;
        NextState = XTSS_ENDELEMENT_OPEN;
        break;




        //
        // Potentially, this could either be "xml" or just another
        // name token.  Let's see what the next token is, just to
        // be sure.
        //
      case NTXML_RAWTOKEN_QUESTIONMARK:
      {
        cbTotalTokenLength += pRawToken->Run.cbData;

        //
        // Defaultwise, this is just a PI opening
        //
        NextState = XTSS_PI_OPEN;

        //
        // Find the identifier out of the input
        //
        ADVANCE_PVOID(pState->RawTokenState.pvCursor, pRawToken->Run.cbData);
        success = RtlRawXmlTokenizer_GatherIdentifier(&pState->RawTokenState, pRawToken, NULL);
        if (EFI_ERROR(success)) {
          return success;
        }

        //
        // If we got data from the identifier lookup, and the thing found was text, then maybe
        // it's the 'xml' special PI
        //
        if ((pRawToken->Run.cbData != 0) && (pRawToken->TokenName == NTXML_RAWTOKEN_TEXT)) {

          success = pState->pfnCompareSpecialString(
            pState,
            &pRawToken->Run,
            &xss_xml,
            &fCompare,
            NULL);

          if (EFI_ERROR(success)) {
            return success;
          }

          //
          // If these two match, then we're really in the XMLDECL
          // element
          //
          if (fCompare == XML_STRING_COMPARE_EQUALS) {
            NextState = XTSS_XMLDECL_OPEN;
            cbTotalTokenLength += pRawToken->Run.cbData;
          }
        }
      }
      break;



      //
      // <! from the base state can be either:
      //
      // <!--             Comment
      // <!DOCTYPE        Internal subset
      //
      case NTXML_RAWTOKEN_BANG:
      {

        PXML_RAW_TOKEN pNextTwoTokens = &pState->RawTokenScratch[2];

        ADVANCE_PVOID(pState->RawTokenState.pvCursor, pRawToken->Run.cbData);
        cbTotalTokenLength += pRawToken->Run.cbData;

        success = RtlRawXmlTokenizer_GatherNTokens(
          &pState->RawTokenState,
          pNextTwoTokens,
          2
          );

        if (EFI_ERROR(success)) {
          return success;
        }

        //
        // Dash-dash is the start of a comment.
        //
        if ((pNextTwoTokens[0].TokenName == NTXML_RAWTOKEN_DASH) &&
          (pNextTwoTokens[1].TokenName == NTXML_RAWTOKEN_DASH))
        {
          cbTotalTokenLength +=
            pNextTwoTokens[0].Run.cbData +
            pNextTwoTokens[1].Run.cbData;
          NextState = XTSS_COMMENT_OPEN;
        }
        //
        // Open-squarebracket might start a CDATA section
        //
        else if ((pNextTwoTokens[0].TokenName == NTXML_RAWTOKEN_OPENBRACKET) &&
          (pNextTwoTokens[1].TokenName == NTXML_RAWTOKEN_TEXT))
        {
          ADVANCE_PVOID(pState->RawTokenState.pvCursor, pNextTwoTokens[0].Run.cbData);

          success = RtlXmlTokenizer_ExpectIdentifier(
            pState,
            &xss_CDATA,
            &pNextTwoTokens[1],
            &fCompare
            );

          if (EFI_ERROR(success)) {
            return success;
          }

          cbTotalTokenLength += pNextTwoTokens[0].Run.cbData + pNextTwoTokens[1].Run.cbData;

          //
          // If it was <![CDATA so far, make sure the next thing is another
          // open square bracket.
          //
          if (fCompare == XML_STRING_COMPARE_EQUALS) {

            ADVANCE_PVOID(pState->RawTokenState.pvCursor, pNextTwoTokens[1].Run.cbData);

            success = RtlRawXmlTokenizer_SingleToken(&pState->RawTokenState, &pNextTwoTokens[2]);
            if (EFI_ERROR(success)) {
              return success;
            }

            cbTotalTokenLength += pNextTwoTokens[2].Run.cbData;
            if (pNextTwoTokens[2].TokenName == NTXML_RAWTOKEN_OPENBRACKET) {
              NextState = XTSS_CDATA_OPEN;
            }
            else {
              pToken->fError = TRUE;
            }
          }
          else {
            pToken->fError = TRUE;
          }


        }
        //
        // DOCTYPE means the internal subset starts
        //
        else if (pNextTwoTokens[0].TokenName == NTXML_RAWTOKEN_TEXT)
        {
          //
          // The cursor should be at the start of the identifier,
          // gather it and see what it comes out to be
          //
          success = RtlRawXmlTokenizer_GatherIdentifier(
            &pState->RawTokenState,
            &pNextTwoTokens[0],
            NULL
            );

          if (EFI_ERROR(success)) {
            return success;
          }

          success = pState->pfnCompareSpecialString(
            pState,
            &pNextTwoTokens[0].Run,
            &xss_DOCTYPE,
            &fCompare,
            NULL);

          if (EFI_ERROR(success)) {
            return success;
          }

          ASSERT(pNextTwoTokens[0].TokenName == NTXML_RAWTOKEN_TEXT);
          cbTotalTokenLength += pNextTwoTokens[0].Run.cbData;

          //
          // Anything other than 'DOCTYPE' is bogus here.
          //
          if (fCompare == XML_STRING_COMPARE_EQUALS) {
            NextState = XTSS_DOCTYPE_OPEN;
          }
          else {
            pToken->fError = TRUE;
          }

        }
        //
        // Otherwise, we found something not a dash and not a doctype
        //
        else {
          cbTotalTokenLength += pNextTwoTokens[0].Run.cbData;
          pToken->fError = TRUE;
        }

      }
      break;


      //
      // Everything else starts an element section.  The next pass will decide
      // if it's valid.  Adjust the size backwards a little.
      //
      default:
        cbTotalTokenLength = pRawToken->Run.cbData;
        NextState = XTSS_ELEMENT_OPEN;
        break;
      }
    }
    //
    // Otherwise, we're back in hyperspace, gather some more tokens until we find something
    // interesting - a <,
    //
    else {
      success = RtlRawXmlTokenizer_GatherPCData(
        &pState->RawTokenState,
        pRawToken,
        pNextRawToken);

      cbTotalTokenLength = pRawToken->Run.cbData;

      NextState = XTSS_STREAM_HYPERSPACE;
    }
    break;


    //
    // This function was getting unreasonably large.  Delegate
    // processing of all doctype states off to a helper function.
    // With any luck, the optimizer will just inline it all and
    // be done.
    //
  case XTSS_DOCTYPE_OPEN:
  case XTSS_DOCTYPE_WHITESPACE:
  case XTSS_DOCTYPE_DOCNAME:
  case XTSS_DOCTYPE_EXTERNALID:
  case XTSS_DOCTYPE_MARKUP_OPEN:
  case XTSS_DOCTYPE_MARKUP_WHITESPACE:
  case XTSS_DOCTYPE_MARKUP_CLOSE:
  case XTSS_DOCTYPE_ELEMENTDECL_OPEN:
  case XTSS_DOCTYPE_ELEMENTDECL_CONTENT:
  case XTSS_DOCTYPE_ELEMENTDECL_CLOSE:
  case XTSS_DOCTYPE_ATTLISTDECL_OPEN:
  case XTSS_DOCTYPE_ATTLISTDECL_ELEMENT_NAME:
  case XTSS_DOCTYPE_ATTLISTDECL_ELEMENT_PREFIX:
  case XTSS_DOCTYPE_ATTLISTDECL_ELEMENT_COLON:
  case XTSS_DOCTYPE_ATTLISTDECL_WHITESPACE:
  case XTSS_DOCTYPE_ATTLISTDECL_ATTNAME:
  case XTSS_DOCTYPE_ATTLISTDECL_ATTPREFIX:
  case XTSS_DOCTYPE_ATTLISTDECL_ATTCOLON:
  case XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_CDATA:
  case XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_ID:
  case XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_IDREF:
  case XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_IDREFS:
  case XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_ENTITY:
  case XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_ENTITIES:
  case XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_NMTOKEN:
  case XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_NMTOKENS:
  case XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_ENUMERATED_OPEN:
  case XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_ENUMERATED_VALUE:
  case XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_ENUMERATED_CLOSE:
  case XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_NOTATION:
  case XTSS_DOCTYPE_ATTLISTDECL_DEFAULT_REQUIRED:
  case XTSS_DOCTYPE_ATTLISTDECL_DEFAULT_IMPLIED:
  case XTSS_DOCTYPE_ATTLISTDECL_DEFAULT_FIXED:
  case XTSS_DOCTYPE_ATTLISTDECL_DEFAULT_TEXT_OPEN:
  case XTSS_DOCTYPE_ATTLISTDECL_DEFAULT_TEXT_VALUE:
  case XTSS_DOCTYPE_ATTLISTDECL_DEFAULT_TEXT_CLOSE:
  case XTSS_DOCTYPE_ATTLISTDECL_CLOSE:
  case XTSS_DOCTYPE_ENTITYDECL_OPEN:
  case XTSS_DOCTYPE_ENTITYDECL_NAME:
  case XTSS_DOCTYPE_ENTITYDECL_PARAMETERMARKER:
  case XTSS_DOCTYPE_ENTITYDECL_GENERALMARKER:
  case XTSS_DOCTYPE_ENTITYDECL_SYSTEM:
  case XTSS_DOCTYPE_ENTITYDECL_SYSTEM_TEXT_OPEN:
  case XTSS_DOCTYPE_ENTITYDECL_SYSTEM_TEXT_VALUE:
  case XTSS_DOCTYPE_ENTITYDECL_SYSTEM_TEXT_CLOSE:
  case XTSS_DOCTYPE_ENTITYDECL_PUBLIC:
  case XTSS_DOCTYPE_ENTITYDECL_PUBLIC_TEXT_OPEN:
  case XTSS_DOCTYPE_ENTITYDECL_PUBLIC_TEXT_VALUE:
  case XTSS_DOCTYPE_ENTITYDECL_PUBLIC_TEXT_CLOSE:
  case XTSS_DOCTYPE_ENTITYDECL_NDATA:
  case XTSS_DOCTYPE_ENTITYDECL_NDATA_TEXT:
  case XTSS_DOCTYPE_ENTITYDECL_VALUE_OPEN:
  case XTSS_DOCTYPE_ENTITYDECL_VALUE_VALUE:
  case XTSS_DOCTYPE_ENTITYDECL_VALUE_CLOSE:
  case XTSS_DOCTYPE_ENTITYDECL_CLOSE:
  case XTSS_DOCTYPE_NOTATIONDECL_OPEN:
  case XTSS_DOCTYPE_NOTATIONDECL_CONTENT:
  case XTSS_DOCTYPE_NOTATIONDECL_CLOSE:
    success = _HandleDocTypeDeclStuff(
      pState,
      pToken,
      &cbTotalTokenLength,
      &NextState
      );

    if (EFI_ERROR(success)) {
      return success;
    }
    break;

    //
    // The open-tag can only be followed by whitespace.  Gather it up, but error out if
    // there wasn't any.
    //
  case XTSS_XMLDECL_OPEN:
    success = RtlRawXmlTokenizer_GatherWhitespace(&pState->RawTokenState, pRawToken, NULL);
    if (EFI_ERROR(success)) {
      return success;
    }

    cbTotalTokenLength = pRawToken->Run.cbData;
    if ((pRawToken->Run.cbData > 0) && (pRawToken->TokenName == NTXML_RAWTOKEN_WHITESPACE)) {
      NextState = XTSS_XMLDECL_WHITESPACE;
    }
    else {
      pToken->fError = TRUE;
    }
    break;


    //
    // Each of these has to be followed by optional whitespace
    // followed by an equals sign
    //
  case XTSS_XMLDECL_ENCODING:
  case XTSS_XMLDECL_STANDALONE:
  case XTSS_XMLDECL_VERSION:
    success = RtlRawXmlTokenizer_GatherWhitespace(
      &pState->RawTokenState,
      pRawToken,
      pNextRawToken
      );

    if (EFI_ERROR(success)) {
      return success;
    }

    cbTotalTokenLength += pRawToken->Run.cbData;
    ADVANCE_PVOID(pState->RawTokenState.pvCursor, pRawToken->Run.cbData);

    success = RtlRawXmlTokenizer_SingleToken(&pState->RawTokenState, pRawToken);
    if (EFI_ERROR(success)) {
      return success;
    }

    cbTotalTokenLength = pRawToken->Run.cbData;
    if (pRawToken->TokenName == NTXML_RAWTOKEN_EQUALS) {
      NextState = XTSS_XMLDECL_EQUALS;
    }
    else {
      pToken->fError = TRUE;
    }
    break;







    //
    // If the next thing is a quote (after some optional whitespace),
    // then record it, otherwise error out.
    //
  case XTSS_XMLDECL_EQUALS:

    success = RtlRawXmlTokenizer_GatherWhitespace(
      &pState->RawTokenState,
      pRawToken,
      pNextRawToken
      );

    if (EFI_ERROR(success)) {
      return success;
    }

    cbTotalTokenLength += pRawToken->Run.cbData;
    ADVANCE_PVOID(pState->RawTokenState.pvCursor, pRawToken->Run.cbData);

    success = RtlRawXmlTokenizer_SingleToken(&pState->RawTokenState, pRawToken);
    if (EFI_ERROR(success)) {
      return success;
    }

    cbTotalTokenLength = pRawToken->Run.cbData;

    if ((pRawToken->TokenName == NTXML_RAWTOKEN_QUOTE) || (pRawToken->TokenName == NTXML_RAWTOKEN_DOUBLEQUOTE)) {

      pState->QuoteTemp = pRawToken->TokenName;
      NextState = XTSS_XMLDECL_VALUE_OPEN;

    }
    else {
      pToken->fError = TRUE;
    }
    break;






    //
    // Values can only be followed by another quote
    //
  case XTSS_XMLDECL_VALUE:
    success = RtlRawXmlTokenizer_SingleToken(&pState->RawTokenState, pRawToken);
    if (EFI_ERROR(success)) {
      return success;
    }

    cbTotalTokenLength = pRawToken->Run.cbData;

    if (pRawToken->TokenName == pState->QuoteTemp) {
      NextState = XTSS_XMLDECL_VALUE_CLOSE;
    }
    //
    // Otherwise, something odd was present in the input stream...
    //
    else {
      pToken->fError = TRUE;
    }

    break;





    //
    // Value-open is followed by N tokens until a close is found
    //
  case XTSS_XMLDECL_VALUE_OPEN:

    success = RtlRawXmlTokenizer_GatherUntil(&pState->RawTokenState, pRawToken, pState->QuoteTemp, pNextRawToken);
    if (EFI_ERROR(success)) {
      return success;
    }

    //
    // With luck, we'll always hit this state.  Found the closing quote value
    //
    if (pNextRawToken->TokenName == pState->QuoteTemp) {
      cbTotalTokenLength = pRawToken->Run.cbData;
      NextState = XTSS_XMLDECL_VALUE;
    }
    //
    // Otherwise, we found something odd (end of stream, maybe)
    else {
      pToken->fError = TRUE;
    }

    break;





    //
    // Whitespace and value-close can only be followed by more whitespace
    // or the close-PI tag
    //
  case XTSS_XMLDECL_VALUE_CLOSE:
  case XTSS_XMLDECL_WHITESPACE:

    success = RtlRawXmlTokenizer_GatherWhitespace(&pState->RawTokenState, pRawToken, pNextRawToken);
    if (EFI_ERROR(success)) {
      return success;
    }

    if ((pRawToken->Run.cbData > 0) && (pRawToken->TokenName == NTXML_RAWTOKEN_WHITESPACE)) {
      cbTotalTokenLength = pRawToken->Run.cbData;
      NextState = XTSS_XMLDECL_WHITESPACE;
    }
    //
    // Maybe there wasn't whitespace, but the next thing was a questionmark
    //
    else if (pNextRawToken->TokenName == NTXML_RAWTOKEN_QUESTIONMARK) {

      cbTotalTokenLength = pNextRawToken->Run.cbData;

      ADVANCE_PVOID(pState->RawTokenState.pvCursor, pNextRawToken->Run.cbData);

      success = RtlRawXmlTokenizer_SingleToken(&pState->RawTokenState, pRawToken);
      if (EFI_ERROR(success)) {
        return success;
      }

      cbTotalTokenLength += pRawToken->Run.cbData;

      //
      // ? must be followed by > in an xmldecl.
      //
      if (pRawToken->TokenName == NTXML_RAWTOKEN_GT) {
        NextState = XTSS_XMLDECL_CLOSE;
      }
      else {
        pToken->fError = TRUE;
      }
    }
    //
    // If we're on whitespace, and the next raw token is a textual thing, then we can
    // probably gather up an attribute from the input.
    //
    else if ((pNextRawToken->TokenName == NTXML_RAWTOKEN_TEXT) && (PreviousState == XTSS_XMLDECL_WHITESPACE)) {

      UINT32 u;

      static const struct {
        PCXML_SIMPLE_STRING                 ss;
        XML_TOKENIZATION_SPECIFIC_STATE state;
      } ComparisonStates[] = {
          { &xss_encoding,    XTSS_XMLDECL_ENCODING },
          { &xss_version,     XTSS_XMLDECL_VERSION },
          { &xss_standalone,  XTSS_XMLDECL_STANDALONE }
      };

      //
      // Snif the actual full identifier from the input stream
      //
      success = RtlRawXmlTokenizer_GatherIdentifier(&pState->RawTokenState, pRawToken, NULL);
      if (EFI_ERROR(success)) {
        return success;
      }

      //
      // This had better be text again
      //
      ASSERT(pRawToken->TokenName == NTXML_RAWTOKEN_TEXT);
      cbTotalTokenLength = pRawToken->Run.cbData;

      //
      // Look to see if it's any of the known XMLDECL attributes
      //
      for (u = 0; u < RTL_NUMBER_OF(ComparisonStates); u++) {

        success = pState->pfnCompareSpecialString(
          pState,
          &pRawToken->Run,
          ComparisonStates[u].ss,
          &fCompare,
          NULL);

        if (EFI_ERROR(success)) {
          return success;
        }

        if (fCompare == XML_STRING_COMPARE_EQUALS) {
          NextState = ComparisonStates[u].state;
          break;
        }

      }

      //
      // No match found means unknown xmldecl attribute name.
      //
      if (fCompare != XML_STRING_COMPARE_EQUALS) {
        pToken->fError = TRUE;
      }
    }
    else {
      pToken->fError = TRUE;
    }
    break;






    //
    // After a PI opening <?, there should come a name
    //
  case XTSS_PI_OPEN:

    success = RtlRawXmlTokenizer_GatherIdentifier(&pState->RawTokenState, pRawToken, NULL);
    if (EFI_ERROR(success)) {
      return success;
    }

    cbTotalTokenLength = pRawToken->Run.cbData;

    //
    // Found an identifier
    //
    if ((pRawToken->Run.cbData > 0) && (pRawToken->TokenName == NTXML_RAWTOKEN_TEXT)) {
      NextState = XTSS_PI_TARGET;
    }
    else {
      pToken->fError = TRUE;
    }

    break;






    //
    // After a value should only come a ?> combo.
    //
  case XTSS_PI_VALUE:
  {
    PXML_RAW_TOKEN NewTokens = pState->RawTokenScratch + 2;

    success = RtlRawXmlTokenizer_GatherNTokens(
      &pState->RawTokenState,
      NewTokens,
      2
      );

    if (EFI_ERROR(success)) {
      return success;
    }

    //
    // Set these up to start with
    //
    cbTotalTokenLength = NewTokens[0].Run.cbData + NewTokens[1].Run.cbData;

    //
    // After a PI must come a ?> pair
    //
    if ((NewTokens[0].TokenName == NTXML_RAWTOKEN_QUESTIONMARK) &&
      (NewTokens[1].TokenName == NTXML_RAWTOKEN_GT)) {

      NextState = XTSS_PI_CLOSE;
    }
    //
    // Otherwise, error out
    //
    else {
      pToken->fError = TRUE;
    }
  }

  break;


  //
  // After a target must come either whitespace or a ?> pair.
  //
  case XTSS_PI_TARGET:

    success = RtlRawXmlTokenizer_GatherWhitespace(&pState->RawTokenState, pRawToken, pNextRawToken);
    if (EFI_ERROR(success)) {
      return success;
    }

    cbTotalTokenLength = pRawToken->Run.cbData;

    //
    // Whitespace present?  Dandy
    //
    if ((pRawToken->Run.cbData != 0) && (pRawToken->TokenName == NTXML_RAWTOKEN_WHITESPACE)) {
      NextState = XTSS_PI_WHITESPACE;
    }
    //
    // If this was a questionmark, then gather the next two items.
    //
    else if (pNextRawToken->TokenName == NTXML_RAWTOKEN_QUESTIONMARK) {

      PXML_RAW_TOKEN pTokens = pState->RawTokenScratch + 2;

      success = RtlRawXmlTokenizer_GatherNTokens(&pState->RawTokenState, pTokens, 2);
      if (EFI_ERROR(success)) {
        return success;
      }

      cbTotalTokenLength = pTokens[0].Run.cbData + pTokens[1].Run.cbData;

      //
      // ?> -> PI close
      //
      if ((pTokens[0].TokenName == NTXML_RAWTOKEN_QUESTIONMARK) &&
        (pTokens[1].TokenName == NTXML_RAWTOKEN_GT)) {

        NextState = XTSS_PI_CLOSE;
      }
      //
      // ? just hanging out there is an error
      //
      else {
        pToken->fError = TRUE;
      }
    }
    //
    // Not starting with whitespace or a questionmark after a value name is illegal.
    //
    else {
      pToken->fError = TRUE;
    }
    break;



    //
    // After the whitespace following a PI target comes random junk until a ?> is found.
    //
  case XTSS_PI_WHITESPACE:

    cbTotalTokenLength = 0;
    NextState = XTSS_NOTHING;

    do
    {
      UINT64 cbThisChunklet = 0;

      success = RtlRawXmlTokenizer_GatherUntil(
        &pState->RawTokenState,
        pRawToken,
        NTXML_RAWTOKEN_QUESTIONMARK,
        pNextRawToken);

      if (EFI_ERROR(success))
        return success;

      cbThisChunklet = pRawToken->Run.cbData;
      ADVANCE_PVOID(pState->RawTokenState.pvCursor, pRawToken->Run.cbData);

      //
      // Found a questionmark, see if this is really ?>
      //
      if (pNextRawToken->TokenName == NTXML_RAWTOKEN_QUESTIONMARK) {

        ADVANCE_PVOID(pState->RawTokenState.pvCursor, pNextRawToken->Run.cbData);

        success = RtlRawXmlTokenizer_SingleToken(&pState->RawTokenState, pRawToken);
        if (EFI_ERROR(success)) {
          return success;
        }

        //
        // Wasn't ?> - simply forward the cursor past the two and continue
        //
        if (pRawToken->TokenName != NTXML_RAWTOKEN_GT) {
          cbTotalTokenLength += cbThisChunklet;
          cbTotalTokenLength += pNextRawToken->Run.cbData + pRawToken->Run.cbData;
          ADVANCE_PVOID(pState->RawTokenState.pvCursor, pRawToken->Run.cbData);
          continue;
        }
        else {
          NextState = XTSS_PI_VALUE;
        }
      }
      //
      // Otherwise, was this maybe end of stream?  We'll just stop looking
      //
      else if (pNextRawToken->TokenName == NTXML_RAWTOKEN_END_OF_STREAM) {
        NextState = XTSS_ERRONEOUS;
        pToken->fError = TRUE;
      }

      //
      // Advance the cursor and append the data to the current chunklet
      //
      cbTotalTokenLength += cbThisChunklet;
    } while (NextState == XTSS_NOTHING);

    break;




    //
    // We gather data here until we find -- in the input stream.
    //
  case XTSS_COMMENT_OPEN:

    NextState = XTSS_NOTHING;

    do
    {
      UINT64 cbChunk = 0;

      success = RtlRawXmlTokenizer_GatherUntil(&pState->RawTokenState, pRawToken, NTXML_RAWTOKEN_DASH, pNextRawToken);
      if (EFI_ERROR(success)) {
        return success;
      }

      cbChunk = pRawToken->Run.cbData;

      if (pNextRawToken->TokenName == NTXML_RAWTOKEN_DASH) {
        //
        // Go past the text and the dash
        //
        ADVANCE_PVOID(pState->RawTokenState.pvCursor, cbChunk + pNextRawToken->Run.cbData);

        success = RtlRawXmlTokenizer_SingleToken(&pState->RawTokenState, pRawToken);
        if (EFI_ERROR(success)) {
          return success;
        }

        //
        // That was a dash as well - we don't want to add that to the run, but we
        // should stop looking.  Skip backwards the length of the "next" we found
        // above.
        //
        if (pRawToken->TokenName == NTXML_RAWTOKEN_DASH) {
          NextState = XTSS_COMMENT_COMMENTARY;
          REWIND_PVOID(pState->RawTokenState.pvCursor, pNextRawToken->Run.cbData);
        }
        //
        // Add the dash and the non-dash as well
        //
        else {
          ADVANCE_PVOID(pState->RawTokenState.pvCursor, pRawToken->Run.cbData);
          cbChunk += pNextRawToken->Run.cbData + pRawToken->Run.cbData;
        }
      }
      //
      // End of stream found means "end of commentary" - next call through
      // here will detect the badness and return
      //
      else if (pNextRawToken->TokenName == NTXML_RAWTOKEN_END_OF_STREAM ||
        pNextRawToken->TokenName == NTXML_RAWTOKEN_ERROR) {
        NextState = XTSS_COMMENT_COMMENTARY;
      }

      cbTotalTokenLength += cbChunk;
    } while (NextState == XTSS_NOTHING);

    break;





    //
    // After commentary can only come -->, so gather three tokens
    // and see if they're all there
    //
  case XTSS_COMMENT_COMMENTARY:
  {
    PXML_RAW_TOKEN pTokens = pState->RawTokenScratch + 2;

    //
    // Grab three tokens
    //
    success = RtlRawXmlTokenizer_GatherNTokens(
      &pState->RawTokenState,
      pTokens,
      3);

    if (EFI_ERROR(success)) {
      return success;
    }

    //
    // Store their size
    //
    cbTotalTokenLength =
      pTokens[0].Run.cbData +
      pTokens[1].Run.cbData +
      pTokens[2].Run.cbData;

    //
    // If this is -->, then great.
    //
    if ((pTokens[0].TokenName == NTXML_RAWTOKEN_DASH) &&
      (pTokens[1].TokenName == NTXML_RAWTOKEN_DASH) &&
      (pTokens[2].TokenName == NTXML_RAWTOKEN_GT)) {

      NextState = XTSS_COMMENT_CLOSE;
    }
    //
    // Otherwise, bad format.
    //
    else {
      pToken->fError = TRUE;
    }
  }

  break;





  //
  // We had found the opening of an "end" element.  Find out
  // what it's supposed to be.
  //
  case XTSS_ENDELEMENT_OPEN:

    success = RtlRawXmlTokenizer_GatherIdentifier(
      &pState->RawTokenState,
      pRawToken,
      pNextRawToken);

    if (EFI_ERROR(success)) {
      return success;
    }

    cbTotalTokenLength = pRawToken->Run.cbData;

    //
    // No data in the token?  Malformed identifier
    //
    if (pRawToken->Run.cbData == 0) {
      pToken->fError = TRUE;
    }
    //
    // Is the next thing a colon?  Then we got a prefix.  Otherwise,
    // we got a name.
    //
    else {
      NextState = (pNextRawToken->TokenName == NTXML_RAWTOKEN_COLON)
        ? XTSS_ENDELEMENT_NS_PREFIX
        : XTSS_ENDELEMENT_NAME;
    }

    break;

    //
    // Followed only by a colon here
    //
  case XTSS_ENDELEMENT_NS_PREFIX:
    success = RtlRawXmlTokenizer_SingleToken(&pState->RawTokenState, pRawToken);
    if (EFI_ERROR(success)) {
      return success;
    }

    cbTotalTokenLength = pRawToken->Run.cbData;

    if (pRawToken->TokenName == NTXML_RAWTOKEN_COLON) {
      NextState = XTSS_ENDELEMENT_NS_COLON;
    }
    else {
      pToken->fError = TRUE;
    }

    break;

    //
    // The colon part of an endelement is followed by the name
    //
  case XTSS_ENDELEMENT_NS_COLON:
    success = RtlRawXmlTokenizer_GatherIdentifier(&pState->RawTokenState, pRawToken, NULL);
    if (EFI_ERROR(success)) {
      return success;
    }

    cbTotalTokenLength = pRawToken->Run.cbData;

    if (pRawToken->Run.cbData > 0) {
      NextState = XTSS_ENDELEMENT_NAME;
    }
    else {
      pToken->fError = TRUE;
    }

    break;


    //
    // The production states:
    //
    // '</' name S? '>'
    //
    // At this point, we're past the name, so we're looking for optional whitespace and
    // a closing >.
    //
  case XTSS_ENDELEMENT_NAME:
  case XTSS_ENDELEMENT_WHITESPACE:

    success = RtlRawXmlTokenizer_GatherWhitespace(&pState->RawTokenState, pRawToken, pNextRawToken);
    if (EFI_ERROR(success)) {
      return success;
    }

    cbTotalTokenLength = pRawToken->Run.cbData + pNextRawToken->Run.cbData;

    if (pNextRawToken->TokenName == NTXML_RAWTOKEN_GT) {
      NextState = XTSS_ENDELEMENT_CLOSE;
    }
    else {
      pToken->fError = TRUE;
    }
    break;







    //
    // We're in an element, so look at the next thing
    //
  case XTSS_ELEMENT_OPEN:

    success = RtlRawXmlTokenizer_GatherIdentifier(&pState->RawTokenState, pRawToken, pNextRawToken);
    if (EFI_ERROR(success)) {
      return success;
    }

    cbTotalTokenLength = pRawToken->Run.cbData;

    //
    // Was there data in the identifier?
    //
    if (pRawToken->Run.cbData > 0) {
      NextState = (pNextRawToken->TokenName == NTXML_RAWTOKEN_COLON)
        ? XTSS_ELEMENT_NAME_NS_PREFIX
        : XTSS_ELEMENT_NAME;
    }
    //
    // Otherwise, there was erroneous data there
    //
    else {
      pToken->fError = TRUE;
    }

    break;





    //
    // After a prefix should only come a colon
    //
  case XTSS_ELEMENT_NAME_NS_PREFIX:
    success = RtlRawXmlTokenizer_SingleToken(&pState->RawTokenState, pRawToken);
    if (EFI_ERROR(success)) {
      return success;
    }

    cbTotalTokenLength = pRawToken->Run.cbData;

    if (pRawToken->TokenName == NTXML_RAWTOKEN_COLON) {
      NextState = XTSS_ELEMENT_NAME_NS_COLON;
    }
    else {
      pToken->fError = TRUE;
    }
    break;





    //
    // After a colon can only come a name piece
    //
  case XTSS_ELEMENT_NAME_NS_COLON:


    success = RtlRawXmlTokenizer_GatherIdentifier(&pState->RawTokenState, pRawToken, NULL);
    if (EFI_ERROR(success)) {
      return success;
    }

    cbTotalTokenLength = pRawToken->Run.cbData;

    //
    // If there was data in the name
    //
    if (pRawToken->Run.cbData > 0) {
      NextState = XTSS_ELEMENT_NAME;
    }
    //
    // Otherwise, we found something else, error
    //
    else {
      pToken->fError = TRUE;
    }

    break;




    //
    // We're in the name portion of an element  Here, we should get either
    // whitespace, /> or >.  Let's gather whitespace and see what the next token
    // after it is.
    //
  case XTSS_ELEMENT_NAME:

    success = RtlRawXmlTokenizer_GatherWhitespace(&pState->RawTokenState, pRawToken, pNextRawToken);
    if (EFI_ERROR(success)) {
      return success;
    }

    cbTotalTokenLength = pRawToken->Run.cbData;

    if (pRawToken->Run.cbData > 0) {
      NextState = XTSS_ELEMENT_WHITESPACE;
    }
    else {

      //
      // If the next raw token is a gt symbol, then gather it (again... ick)
      // and say we're at the end of an element
      //
      if (pNextRawToken->TokenName == NTXML_RAWTOKEN_GT) {

        cbTotalTokenLength += pNextRawToken->Run.cbData;
        NextState = XTSS_ELEMENT_CLOSE;
      }
      //
      // A forwardslash has to be followed by a >
      //
      else if (pNextRawToken->TokenName == NTXML_RAWTOKEN_FORWARDSLASH) {

        PXML_RAW_TOKEN pNextTokens = pState->RawTokenScratch + 2;

        success = RtlRawXmlTokenizer_GatherNTokens(&pState->RawTokenState, pNextTokens, 2);
        if (EFI_ERROR(success)) {
          return success;
        }

        ASSERT(pNextTokens[0].TokenName == NTXML_RAWTOKEN_FORWARDSLASH);

        cbTotalTokenLength =
          pNextTokens[0].Run.cbData +
          pNextTokens[1].Run.cbData;

        //
        // /> -> close-empty
        //
        if ((pNextTokens[1].TokenName == NTXML_RAWTOKEN_GT) &&
          (pNextTokens[0].TokenName == NTXML_RAWTOKEN_FORWARDSLASH)) {

          NextState = XTSS_ELEMENT_CLOSE_EMPTY;
        }
        //
        // /* -> Oops.
        //
        else {
          pToken->fError = TRUE;
        }
      }
      //
      // Otherwise, we got something after a name that wasn't whitespace, or
      // part of a clsoe, so that's an error
      //
      else {
        pToken->fError = TRUE;
      }
    }

    break;





    //
    // After an attribute name, the only legal thing is an equals
    // sign.
    //
  case XTSS_ELEMENT_ATTRIBUTE_NAME:
    success = RtlRawXmlTokenizer_GatherWhitespace(
      &pState->RawTokenState,
      pRawToken,
      pNextRawToken
      );

    if (EFI_ERROR(success)) {
      return success;
    }

    cbTotalTokenLength += pRawToken->Run.cbData;
    ADVANCE_PVOID(pState->RawTokenState.pvCursor, pRawToken->Run.cbData);

    success = RtlRawXmlTokenizer_SingleToken(&pState->RawTokenState, pRawToken);;
    if (EFI_ERROR(success)) {
      return success;
    }

    cbTotalTokenLength += pRawToken->Run.cbData;

    if (pRawToken->TokenName == NTXML_RAWTOKEN_EQUALS) {
      NextState = XTSS_ELEMENT_ATTRIBUTE_EQUALS;
    }
    else 
    {
      pToken->fError = TRUE;
    }
    break;






    //
    // After an equals can only come a quote and a set of value data.  We
    // record the opening quote and gather data until the closing quote.
    //
  case XTSS_ELEMENT_ATTRIBUTE_EQUALS:

    success = RtlRawXmlTokenizer_GatherWhitespace(
      &pState->RawTokenState,
      pRawToken,
      pNextRawToken
      );

    if (EFI_ERROR(success)) {
      return success;
    }

    cbTotalTokenLength += pRawToken->Run.cbData;
    ADVANCE_PVOID(pState->RawTokenState.pvCursor, pRawToken->Run.cbData);

    success = RtlRawXmlTokenizer_SingleToken(&pState->RawTokenState, pRawToken);
    if (EFI_ERROR(success)) {
      return success;
    }

    cbTotalTokenLength += pRawToken->Run.cbData;

    //
    // Quote or doublequote starts an attribute value
    //
    if ((pRawToken->TokenName == NTXML_RAWTOKEN_QUOTE) ||
      (pRawToken->TokenName == NTXML_RAWTOKEN_DOUBLEQUOTE)) {

      pState->QuoteTemp = pRawToken->TokenName;
      NextState = XTSS_ELEMENT_ATTRIBUTE_OPEN;
    }
    else {
      pToken->fError = TRUE;
    }
    break;




    //
    // We gather stuff until we find the close-quote
    //
  case XTSS_ELEMENT_ATTRIBUTE_OPEN:

    ASSERT((pState->QuoteTemp == NTXML_RAWTOKEN_QUOTE) || (pState->QuoteTemp == NTXML_RAWTOKEN_DOUBLEQUOTE));

    success = RtlRawXmlTokenizer_GatherUntil(&pState->RawTokenState, pRawToken, pState->QuoteTemp, NULL);
    if (EFI_ERROR(success)) {
      return success;
    }

    cbTotalTokenLength = pRawToken->Run.cbData;
    NextState = XTSS_ELEMENT_ATTRIBUTE_VALUE;

    break;






    //
    // Only followed by the same quote that opened it
    //
  case XTSS_ELEMENT_ATTRIBUTE_VALUE:

    ASSERT((pState->QuoteTemp == NTXML_RAWTOKEN_QUOTE) || (pState->QuoteTemp == NTXML_RAWTOKEN_DOUBLEQUOTE));

    success = RtlRawXmlTokenizer_SingleToken(&pState->RawTokenState, pRawToken);
    if (EFI_ERROR(success)) {
      return success;
    }

    cbTotalTokenLength = pRawToken->Run.cbData;

    if (pRawToken->TokenName == pState->QuoteTemp) {
      NextState = XTSS_ELEMENT_ATTRIBUTE_CLOSE;
    }
    else {
      pToken->fError = TRUE;
    }

    break;





    //
    // After an attribute namespace prefix should only come a colon.
    //
  case XTSS_ELEMENT_ATTRIBUTE_NAME_NS_PREFIX:

    success = RtlRawXmlTokenizer_SingleToken(&pState->RawTokenState, pRawToken);
    if (EFI_ERROR(success)) {
      return success;
    }

    cbTotalTokenLength = pRawToken->Run.cbData;

    if (pRawToken->TokenName == NTXML_RAWTOKEN_COLON) {
      NextState = XTSS_ELEMENT_ATTRIBUTE_NAME_NS_COLON;
    }
    else {
      pToken->fError = TRUE;
    }
    break;




    //
    // After a colon should come only more name bits
    //
  case XTSS_ELEMENT_ATTRIBUTE_NAME_NS_COLON:

    success = RtlRawXmlTokenizer_GatherIdentifier(&pState->RawTokenState, pRawToken, NULL);
    if (EFI_ERROR(success)) {
      return success;
    }

    cbTotalTokenLength = pRawToken->Run.cbData;

    if (pRawToken->Run.cbData > 0) {
      NextState = XTSS_ELEMENT_ATTRIBUTE_NAME;
    }
    else {
      pToken->fError = TRUE;
    }

    break;



    //
    // Attribute end-of-value and whitespace both have the same transitions to
    // the next state.
    //
  case XTSS_ELEMENT_ATTRIBUTE_CLOSE:
  case XTSS_ELEMENT_XMLNS_VALUE_CLOSE:
  case XTSS_ELEMENT_XML_VALUE_CLOSE:
  case XTSS_ELEMENT_WHITESPACE:

    success = RtlRawXmlTokenizer_GatherWhitespace(&pState->RawTokenState, pRawToken, pNextRawToken);
    if (EFI_ERROR(success)) {
      return success;
    }

    if (pRawToken->Run.cbData > 0) {
      cbTotalTokenLength = pRawToken->Run.cbData;
      NextState = XTSS_ELEMENT_WHITESPACE;
    }
    //
    // Just a >? Then we're at "element close"
    //
    else if (pNextRawToken->TokenName == NTXML_RAWTOKEN_GT) {

      cbTotalTokenLength += pNextRawToken->Run.cbData;
      NextState = XTSS_ELEMENT_CLOSE;

    }
    //
    // Forwardslash?  See if there's a > after it
    //
    else if (pNextRawToken->TokenName == NTXML_RAWTOKEN_FORWARDSLASH) {

      PXML_RAW_TOKEN pTokens = pState->RawTokenScratch + 2;

      success = RtlRawXmlTokenizer_GatherNTokens(&pState->RawTokenState, pTokens, 2);
      if (EFI_ERROR(success)) {
        return success;
      }

      cbTotalTokenLength = pTokens[0].Run.cbData + pTokens[1].Run.cbData;

      ASSERT(pTokens[0].TokenName == NTXML_RAWTOKEN_FORWARDSLASH);

      if ((pTokens[0].TokenName == NTXML_RAWTOKEN_FORWARDSLASH) &&
        (pTokens[1].TokenName == NTXML_RAWTOKEN_GT)) {

        NextState = XTSS_ELEMENT_CLOSE_EMPTY;
      }
      else {
        pToken->fError = TRUE;
      }
    }
    //
    // Otherwise try to gather an identifier (attribute name) from the stream
    //
    else {
      success = RtlRawXmlTokenizer_GatherIdentifier(&pState->RawTokenState, pRawToken, pNextRawToken);
      if (EFI_ERROR(success)) {
        return success;
      }

      cbTotalTokenLength = pRawToken->Run.cbData;

      //
      // Found an identifier.  Is it 'xmlns'?
      //
      if (pRawToken->Run.cbData > 0) {

        success = pState->pfnCompareSpecialString(
          pState,
          &pRawToken->Run,
          &xss_xmlns,
          &fCompare,
          NULL);

        if (EFI_ERROR(success)) {
          return success;
        }

        if (fCompare == XML_STRING_COMPARE_EQUALS) {
          switch (pNextRawToken->TokenName) {
          case NTXML_RAWTOKEN_COLON:
            NextState = XTSS_ELEMENT_XMLNS;
            break;
          case NTXML_RAWTOKEN_EQUALS:
          case NTXML_RAWTOKEN_WHITESPACE:
            NextState = XTSS_ELEMENT_XMLNS_DEFAULT;
            break;
          default:
            NextState = XTSS_ERRONEOUS;
            pToken->fError = TRUE;
          }
        }
        else {
          // How about "xml"?
          success = pState->pfnCompareSpecialString(
            pState,
            &pRawToken->Run,
            &xss_xml,
            &fCompare,
            NULL
            );

          if (EFI_ERROR(success)) {
            return success;
          }

          if (fCompare == XML_STRING_COMPARE_EQUALS)
          {
            switch (pNextRawToken->TokenName) {
            case NTXML_RAWTOKEN_COLON:
              NextState = XTSS_ELEMENT_XML;
              break;
            case NTXML_RAWTOKEN_EQUALS:
            case NTXML_RAWTOKEN_WHITESPACE:
              NextState = XTSS_ELEMENT_ATTRIBUTE_NAME;
              break;
            default:
              NextState = XTSS_ERRONEOUS;
              pToken->fError = TRUE;
            }
          }
          else
          {
            switch (pNextRawToken->TokenName) {
            case NTXML_RAWTOKEN_COLON:
              NextState = XTSS_ELEMENT_ATTRIBUTE_NAME_NS_PREFIX;
              break;
            case NTXML_RAWTOKEN_EQUALS:
            case NTXML_RAWTOKEN_WHITESPACE:
              NextState = XTSS_ELEMENT_ATTRIBUTE_NAME;
              break;
            default:
              NextState = XTSS_ERRONEOUS;
              pToken->fError = TRUE;
            }
          }
        }
      }
      else {
        pToken->fError = TRUE;
      }
    }
    break;


    //
    // Followed by a colon only
    //
  case XTSS_ELEMENT_XMLNS:
  case XTSS_ELEMENT_XML:
    success = RtlRawXmlTokenizer_SingleToken(&pState->RawTokenState, pRawToken);
    if (EFI_ERROR(success)) {
      return success;
    }

    cbTotalTokenLength = pRawToken->Run.cbData;

    if (pRawToken->TokenName == NTXML_RAWTOKEN_COLON)
    {
      if (PreviousState == XTSS_ELEMENT_XMLNS) {
        NextState = XTSS_ELEMENT_XMLNS_COLON;
      }
      else {
        NextState = XTSS_ELEMENT_XML_COLON;
      }
    }
    else {
      pToken->fError = TRUE;
    }
    break;

    //
    // Followed only by an identifier
    //
  case XTSS_ELEMENT_XMLNS_COLON:
  case XTSS_ELEMENT_XML_COLON:
    success = RtlRawXmlTokenizer_GatherIdentifier(
      &pState->RawTokenState,
      pRawToken,
      pNextRawToken);

    if (EFI_ERROR(success)) {
      return success;
    }

    cbTotalTokenLength = pRawToken->Run.cbData;

    if (pRawToken->Run.cbData > 0)
    {
      if (PreviousState == XTSS_ELEMENT_XMLNS_COLON) {
        NextState = XTSS_ELEMENT_XMLNS_ALIAS;
      }
      else {
        NextState = XTSS_ELEMENT_XML_NAME;
      }
    }
    else {
      pToken->fError = TRUE;
    }
    break;

    //
    // Alias followed by equals
    //
  case XTSS_ELEMENT_XMLNS_ALIAS:
  case XTSS_ELEMENT_XML_NAME:

    success = RtlRawXmlTokenizer_GatherWhitespace(
      &pState->RawTokenState,
      pRawToken,
      pNextRawToken
      );

    if (EFI_ERROR(success)) {
      return success;
    }

    cbTotalTokenLength += pRawToken->Run.cbData;
    ADVANCE_PVOID(pState->RawTokenState.pvCursor, pRawToken->Run.cbData);

    success = RtlRawXmlTokenizer_SingleToken(&pState->RawTokenState, pRawToken);
    if (EFI_ERROR(success)) {
      return success;
    }

    cbTotalTokenLength += pRawToken->Run.cbData;

    if (pRawToken->TokenName == NTXML_RAWTOKEN_EQUALS)
    {
      if (PreviousState == XTSS_ELEMENT_XMLNS_ALIAS) {
        NextState = XTSS_ELEMENT_XMLNS_EQUALS;
      }
      else {
        NextState = XTSS_ELEMENT_XML_EQUALS;
      }
    }
    else {
      pToken->fError = TRUE;
    }
    break;


    //
    // Equals followed by quote
    //
  case XTSS_ELEMENT_XMLNS_EQUALS:
  case XTSS_ELEMENT_XML_EQUALS:

    success = RtlRawXmlTokenizer_GatherWhitespace(
      &pState->RawTokenState,
      pRawToken,
      pNextRawToken
      );

    if (EFI_ERROR(success)) {
      return success;
    }

    cbTotalTokenLength += pRawToken->Run.cbData;
    ADVANCE_PVOID(pState->RawTokenState.pvCursor, pRawToken->Run.cbData);

    success = RtlRawXmlTokenizer_SingleToken(&pState->RawTokenState, pRawToken);
    if (EFI_ERROR(success)) {
      return success;
    }

    cbTotalTokenLength += pRawToken->Run.cbData;

    if ((pRawToken->TokenName == NTXML_RAWTOKEN_QUOTE) ||
      (pRawToken->TokenName == NTXML_RAWTOKEN_DOUBLEQUOTE)) {

      pState->QuoteTemp = pRawToken->TokenName;

      if (PreviousState == XTSS_ELEMENT_XMLNS_EQUALS) {
        NextState = XTSS_ELEMENT_XMLNS_VALUE_OPEN;
      }
      else {
        NextState = XTSS_ELEMENT_XML_VALUE_OPEN;
      }
    }
    else {
      pToken->fError = TRUE;
    }
    break;


    //
    // Value open starts the value, which continues until either the
    // end of the document or the close quote is found.  We just return
    // all the data we found, and assume the pass for XTSS_ELEMENT_XMLNS_VALUE
    // will detect the 'end of file looking for quote' error.
    //
  case XTSS_ELEMENT_XMLNS_VALUE_OPEN:
  case XTSS_ELEMENT_XML_VALUE_OPEN:
    success = RtlRawXmlTokenizer_GatherUntil(
      &pState->RawTokenState,
      pRawToken,
      pState->QuoteTemp,
      pNextRawToken);

    if (EFI_ERROR(success)) {
      return success;
    }

    cbTotalTokenLength = pRawToken->Run.cbData;

    if (PreviousState == XTSS_ELEMENT_XMLNS_VALUE_OPEN) {
      NextState = XTSS_ELEMENT_XMLNS_VALUE;
    }
    else {
      NextState = XTSS_ELEMENT_XML_VALUE;
    }

    break;



    //
    // Must find a quote that matches the quote we found before
    //
  case XTSS_ELEMENT_XMLNS_VALUE:
  case XTSS_ELEMENT_XML_VALUE:
    success = RtlRawXmlTokenizer_SingleToken(&pState->RawTokenState, pRawToken);
    if (EFI_ERROR(success)) {
      return success;
    }

    cbTotalTokenLength = pRawToken->Run.cbData;

    if (pRawToken->TokenName == pState->QuoteTemp) {
      if (PreviousState == XTSS_ELEMENT_XMLNS_VALUE) {
        NextState = XTSS_ELEMENT_XMLNS_VALUE_CLOSE;
      }
      else {
        NextState = XTSS_ELEMENT_XML_VALUE_CLOSE;
      }
    }
    else {
      pToken->fError = TRUE;
    }
    break;



    //
    // Must be followed by an equals, possibly after some whitespace
    //
  case XTSS_ELEMENT_XMLNS_DEFAULT:
    success = RtlRawXmlTokenizer_GatherWhitespace(
      &pState->RawTokenState,
      pRawToken,
      pNextRawToken
      );

    if (EFI_ERROR(success)) {
      return success;
    }

    cbTotalTokenLength += pRawToken->Run.cbData;
    ADVANCE_PVOID(pState->RawTokenState.pvCursor, pRawToken->Run.cbData);

    success = RtlRawXmlTokenizer_SingleToken(&pState->RawTokenState, pRawToken);
    if (EFI_ERROR(success)) {
      return success;
    }

    cbTotalTokenLength += pRawToken->Run.cbData;

    if (pRawToken->TokenName == NTXML_RAWTOKEN_EQUALS) {
      NextState = XTSS_ELEMENT_XMLNS_EQUALS;
    }
    else {
      pToken->fError = TRUE;
    }
    break;



    //
    // <![CDATA[ is followed by 'whatever' until a ]]> is found.
    //
  case XTSS_CDATA_OPEN:

    NextState = XTSS_NOTHING;

    do
    {
      UINT64 cbChunk = 0;
      //BOOLEAN fAdvanced = FALSE;

      success = RtlRawXmlTokenizer_GatherUntil(
        &pState->RawTokenState,
        pRawToken,
        NTXML_RAWTOKEN_CLOSEBRACKET,
        pNextRawToken
        );

      if (EFI_ERROR(success)) {
        return success;
      }

      //
      // Found ], might be the start of ]]> - advance past this current piece
      // and see what the next three are.
      //
      if (pNextRawToken->TokenName == NTXML_RAWTOKEN_CLOSEBRACKET) {

        ASSERT(RTL_NUMBER_OF(pState->RawTokenScratch) >= 4);
        PXML_RAW_TOKEN pTokens = pState->RawTokenScratch + 1;

        ADVANCE_PVOID(pState->RawTokenState.pvCursor, pRawToken->Run.cbData);

        //
        // Read the next three single tokens from the stream, see if they are ]]>.
        // If so, don't add this chunk
        //
        success = RtlRawXmlTokenizer_GatherNTokens(&pState->RawTokenState, pTokens, 3);
        if (EFI_ERROR(success)) {
          return success;
        }

        if ((pTokens[0].TokenName == NTXML_RAWTOKEN_CLOSEBRACKET) &&
          (pTokens[1].TokenName == NTXML_RAWTOKEN_CLOSEBRACKET) &&
          (pTokens[2].TokenName == NTXML_RAWTOKEN_GT))
        {
          NextState = XTSS_CDATA_CDATA;
          cbChunk = pRawToken->Run.cbData;
        }
        //
        // Did we hit the EOS?  Stop, someone else will deal with it.
        //
        else if ((pTokens[0].TokenName == NTXML_RAWTOKEN_END_OF_STREAM) ||
          (pTokens[1].TokenName == NTXML_RAWTOKEN_END_OF_STREAM) ||
          (pTokens[2].TokenName == NTXML_RAWTOKEN_END_OF_STREAM))
        {
          NextState = XTSS_CDATA_CDATA;
          cbChunk = pRawToken->Run.cbData;
        }
        //
        // Otherwise, accept the first token plus the previous run and
        // continue.  Back the pointer up a little bit then continue on
        //
        else {
          cbChunk = pTokens[0].Run.cbData;
          ADVANCE_PVOID(pState->RawTokenState.pvCursor, cbChunk);
          cbChunk += pRawToken->Run.cbData;
          pToken->Run.ulCharacters +=
            pRawToken->Run.ulCharacters + pNextRawToken->Run.ulCharacters;
        }
      }
      //
      // End of stream found means "end of cdata" - next call through
      // here will detect the badness and return
      //
      else if (pNextRawToken->TokenName == NTXML_RAWTOKEN_END_OF_STREAM) {
        NextState = XTSS_CDATA_CDATA;
      }
      else if (pNextRawToken->TokenName == NTXML_RAWTOKEN_ERROR) {
        NextState = XTSS_ERRONEOUS;
      }
      //
      // Otherwise, just a random thing, add the token length to the chunk
      // and continue
      //
      else {
        ADVANCE_PVOID(pState->RawTokenState.pvCursor, cbChunk);
        cbChunk = pNextRawToken->Run.cbData;
      }

      cbTotalTokenLength += cbChunk;
    } while (NextState == XTSS_NOTHING);

    break;


    //
    // After the cdata, we expect exactly three things
    //
  case XTSS_CDATA_CDATA:

  {
    PXML_RAW_TOKEN Tokens = pState->RawTokenScratch;

    success = RtlRawXmlTokenizer_GatherNTokens(&pState->RawTokenState, Tokens, 3);
    if (EFI_ERROR(success)) {
      return success;
    }

    cbTotalTokenLength = Tokens[0].Run.cbData + Tokens[1].Run.cbData + Tokens[2].Run.cbData;

    if ((Tokens[0].TokenName == NTXML_RAWTOKEN_CLOSEBRACKET) &&
      (Tokens[1].TokenName == NTXML_RAWTOKEN_CLOSEBRACKET) &&
      (Tokens[2].TokenName == NTXML_RAWTOKEN_GT))
    {
      NextState = XTSS_CDATA_CLOSE;
    }
    else
    {
      pToken->fError = TRUE;
    }
  }
  break;






  //
  // Wierd, some unhandled state.
  //
  default:
    NextState = XTSS_ERRONEOUS;
    success = STATUS_INTERNAL_ERROR;
    pToken->fError = TRUE;
    break;
  }

  /*
  if (pToken->fError)
      NextState = XTSS_ERRONEOUS;
  */

  //
  // Reset the state of the raw tokenizer back to the original incoming state,
  // as the caller is the one that has to do the "advance"
  //
  pState->RawTokenState.pvCursor = pvStarterCursor;

  pToken->Run.cbData = cbTotalTokenLength;
  pToken->Run.pvData = pvStarterCursor;
  pToken->State = NextState;
  pToken->Run.ulCharacters += pRawToken->Run.ulCharacters;
  pToken->Run.Encoding = pRawToken->Run.Encoding;

  if (NT_SUCCESS(success) && fAdvanceState) {
    success = RtlXmlAdvanceTokenization(pState, pToken);
  }

  return success;
}



EFI_STATUS
EFIAPI
RtlXmlGetCurrentLocation(
  PXML_TOKENIZATION_STATE State,
  PXML_LINE_AND_COLUMN Location
  )
{
  if (ARGUMENT_PRESENT(Location)) {
    Location->Line = 0;
    Location->Column = 0;
  }

  if (!ARGUMENT_PRESENT(State)) {
    return RtlpReportXmlError(EFI_INVALID_PARAMETER_1);
  }

  if (!ARGUMENT_PRESENT(Location)) {
    return RtlpReportXmlError(EFI_INVALID_PARAMETER_2);
  }

  *Location = State->Location;

  return EFI_SUCCESS;

}




EFI_STATUS
EFIAPI
RtlXmlAdvanceTokenization(
  IN_OUT PXML_TOKENIZATION_STATE pState,
  IN PXML_TOKEN pToken
  )
{
  VOID* pvTarget;
  VOID* pvCurrent;
  XML_LINE_AND_COLUMN NewLocation;
  NTXMLRAWNEXTCHARACTER Next;
  XML_RAWTOKENIZATION_RESULT res;

  if (!ARGUMENT_PRESENT(pState)) {
    return RtlpReportXmlError(EFI_INVALID_PARAMETER_1);
  }

  if (!ARGUMENT_PRESENT(pToken)) {
    return RtlpReportXmlError(EFI_INVALID_PARAMETER_2);
  }

  if (pState->SupportsLocations)
  {

    Next = pState->RawTokenState.pfnNextChar;
    NewLocation = pState->Location;
    pvCurrent = pState->RawTokenState.pvCursor;
    pvTarget = (VOID*)(((UINTN)pvCurrent) + (UINTN)pToken->Run.cbData);

    while (pvCurrent < pvTarget)
    {
      res = (*Next)(pvCurrent, pvTarget);

      if (res.Character == XML_RAWTOKENIZATION_INVALID_CHARACTER)
        return res.Result.ErrorCode;

      if (res.Character == '\n') {
        NewLocation.Line++;
        NewLocation.Column = 1;
      }
      else {
        NewLocation.Column++;
      }

      pvCurrent = res.Result.NextCursor;
    }

    pState->Location = NewLocation;
    pState->RawTokenState.pvCursor = pvCurrent;
  }
  else
  {
    ADVANCE_PVOID(pState->RawTokenState.pvCursor, pToken->Run.cbData);
  }

  pState->PreviousState = pToken->State;

  return EFI_SUCCESS;
}



EFI_STATUS
RtlXmlInitializeTokenization(
  OUT PXML_TOKENIZATION_STATE     pState,
  IN PCXML_TOKENIZATION_INIT     pInit
  )
{
  EFI_STATUS success = EFI_SUCCESS;

  if (!ARGUMENT_PRESENT(pState)) {
    return RtlpReportXmlError(EFI_INVALID_PARAMETER_1);
  }

  if (!ARGUMENT_PRESENT(pInit)) {
    return RtlpReportXmlError(EFI_INVALID_PARAMETER_2);
  }

  ZeroMem(pState, sizeof(*pState));

  pState->RawTokenState.OriginalDocument.pvData = pInit->XmlData;
  pState->RawTokenState.OriginalDocument.cbData = pInit->XmlDataSize;

  pState->RawTokenState.pvCursor = pInit->XmlData;
  pState->RawTokenState.pvDocumentEnd = (VOID*)(((UINTN)pInit->XmlData) + pInit->XmlDataSize);

  pState->RawTokenState.pfnNextChar = NULL;

  if (pInit->SpecialStringCompare) {
    pState->pfnCompareSpecialString = pInit->SpecialStringCompare;
  }
  else {
    pState->pfnCompareSpecialString = RtlXmlDefaultSpecialStringCompare;
  }

  if (pInit->StringComparison) {
    pState->pfnCompareStrings = pInit->StringComparison;
  }
  else {
    pState->pfnCompareStrings = RtlXmlDefaultCompareStrings;
  }

  pState->DecoderSelection = pInit->FetchDecoder;
  pState->pvComparisonContext = pInit->CallbackContext;
  pState->PreviousState = XTSS_STREAM_START;
  pState->Location.Line = 1;
  pState->Location.Column = 1;
  pState->SupportsLocations = pInit->SupportPosition;

  return success;
}


static const UINT8 s_rgbUTF16_big_BOM[] = { 0xFE, 0xFF };
static const UINT8 s_rgbUTF16_little_BOM[] = { 0xFF, 0xFE };
static const UINT8 s_rgbUCS4_big[] = { 0x00, 0x00, 0x00, 0x3c };
static const UINT8 s_rgbUCS4_little[] = { 0x3c, 0x00, 0x00, 0x00 };
static const UINT8 s_rgbUTF16_big[] = { 0x00, 0x3C, 0x00, 0x3F };
static const UINT8 s_rgbUTF16_little[] = { 0x3C, 0x00, 0x3F, 0x00 };
static const UINT8 s_rgbUTF8_or_mixed[] = { 0x3C, 0x3F, 0x78, 0x6D };
static const UINT8 s_rgbUTF8_with_bom[] = { 0xEF, 0xBB, 0xBF };

//
// These values for 'presumed' encoding families found at
// http://www.xml.com/axml/testaxml.htm (Appendix F)
//
typedef struct _tagENCODER_CORRELATION
{
  const UINT8 *pbSense;
  UINT32 cbSense;
  XML_ENCODING_FAMILY Family;
  UINT32 cbToDiscard;
  NTXMLRAWNEXTCHARACTER pfnFastDecoder;
  UINT32 EncodingNameCount;
  PCXML_SIMPLE_STRING EncodingNameList;
  UINT32 ulCharSize;
}
ENCODER_CORRELATION, *PENCODER_CORRELATION;
typedef const ENCODER_CORRELATION *PCENCODER_CORRELATION;

static const XML_SIMPLE_STRING sc_Encoding_Utf8 = CONSTANT_XML_SIMPLE_STRING(L"UTF-8");
static const XML_SIMPLE_STRING sc_Encoding_Ucs4 = CONSTANT_XML_SIMPLE_STRING(L"UCS-4");
static const XML_SIMPLE_STRING sc_Encoding_Utf16Ucs2[] =
{
    CONSTANT_XML_SIMPLE_STRING(L"UCS-2"),
    CONSTANT_XML_SIMPLE_STRING(L"UTF-16"),
};

static const ENCODER_CORRELATION EncodingCorrelation[] =
{
    { s_rgbUTF8_or_mixed, RTL_NUMBER_OF(s_rgbUTF8_or_mixed),        XMLEF_UTF_8_OR_ASCII, 0, RtlXmlDefaultNextCharacter_UTF8, 1, &sc_Encoding_Utf8, 1 },
    { s_rgbUTF8_with_bom, RTL_NUMBER_OF(s_rgbUTF8_with_bom),        XMLEF_UTF_8_OR_ASCII, 3, RtlXmlDefaultNextCharacter_UTF8, 1, &sc_Encoding_Utf8, 1 },
    { s_rgbUTF16_big_BOM, RTL_NUMBER_OF(s_rgbUTF16_big_BOM),        XMLEF_UTF_16_BE, 2, RtlXmlDefaultNextCharacter_UTF16BE, 2, sc_Encoding_Utf16Ucs2, sizeof(UINT16) },
    { s_rgbUTF16_little_BOM, RTL_NUMBER_OF(s_rgbUTF16_little_BOM),  XMLEF_UTF_16_LE, 2, RtlXmlDefaultNextCharacter_UTF16LE, 2, sc_Encoding_Utf16Ucs2, sizeof(UINT16) },
    { s_rgbUTF16_big, RTL_NUMBER_OF(s_rgbUTF16_big),                XMLEF_UTF_16_BE, 0, RtlXmlDefaultNextCharacter_UTF16BE, 2, sc_Encoding_Utf16Ucs2, sizeof(UINT16) },
    { s_rgbUTF16_little, RTL_NUMBER_OF(s_rgbUTF16_little),          XMLEF_UTF_16_LE, 0, RtlXmlDefaultNextCharacter_UTF16LE, 2, sc_Encoding_Utf16Ucs2, sizeof(UINT16) },
    { s_rgbUCS4_big, RTL_NUMBER_OF(s_rgbUCS4_big),                  XMLEF_UCS_4_BE, 0, RtlXmlDefaultNextCharacter_UCS4BE, 1, &sc_Encoding_Ucs4, sizeof(UINT32) },
    { s_rgbUCS4_little, RTL_NUMBER_OF(s_rgbUCS4_little),            XMLEF_UCS_4_LE, 0, RtlXmlDefaultNextCharacter_UCS4LE, 1, &sc_Encoding_Ucs4, sizeof(UINT32) },
};

static const PCENCODER_CORRELATION sc_pDefaultDecoder = &EncodingCorrelation[0];

//
// This horribly stupid case conversion function switches latin a-z to A-Z.
//
static
UINT32
EFIAPI
RtlpUpcaseUcsCharacter(
  UINT32 ulCharacter
  )
{
  if ((ulCharacter >= 'a') && (ulCharacter <= 'z'))
  {
    return (ulCharacter - 'a') + 'A';
  }
  else
  {
    return ulCharacter;
  }
}


EFI_STATUS
EFIAPI
RtlXmlDetermineStreamEncoding(
  PXML_TOKENIZATION_STATE pState,
  UINTN* pulUINT8sOfEncoding
  )
  /*++

    Purpose:

      Sniffs the input stream to find a BOM, an '<?xml encoding="', etc. to know
      what the encoding of this stream is.  On return the various members of
      pState that describe the stream's encoding will be set properly.

    Returns:

      EFI_SUCCESS - Correctly determined the encoding of the XML stream.

      EFI_INVALID_PARAMETER - If pState is NULL, or various bits of the
          state structure are set up improperly.

  --*/
{
  VOID* pvCursor = pState ? pState->RawTokenState.pvCursor : NULL;
  const VOID* pvDocumentEnd = pState ? pState->RawTokenState.pvDocumentEnd : NULL;
  const UINT64 cAvailableUINT8s = ((UINT64)pvDocumentEnd) - ((UINT64)pvCursor);
  PCENCODER_CORRELATION pChosenEncoder = NULL;
  XML_TOKENIZATION_STATE PrivateState;
  EFI_STATUS status;
  XML_TOKEN Token;
  UINT32 u;

  if (pState == NULL)
    return RtlpReportXmlError(EFI_INVALID_PARAMETER);

  CopyMem(&PrivateState, pState, sizeof(XML_TOKENIZATION_STATE));

  if (pulUINT8sOfEncoding == NULL)
    return RtlpReportXmlError(EFI_INVALID_PARAMETER);

  //
  // Zip through our set of encoders and do the corellation work
  //
  for (u = 0; u != RTL_NUMBER_OF(EncodingCorrelation); u++) {

#pragma prefast(suppress:394, "Prefast doesn't understand about != vs. < in loop invariants")
    const PCENCODER_CORRELATION pEncoder = &EncodingCorrelation[u];

    if ((cAvailableUINT8s < pEncoder->cbSense) || (pEncoder->cbSense == 0))
      continue;

    //
    // This must be it
    //
    if (CompareMem(pvCursor, pEncoder->pbSense, pEncoder->cbSense) == 0) {
      pChosenEncoder = pEncoder;
      break;
    }
  }

  //
  // If we didn't pick one, default to the first (the UTF8 one)
  //
  if (pChosenEncoder == NULL) {
    pChosenEncoder = sc_pDefaultDecoder;
  }

  //
  // Now use the decoder to remove a bunch of characters until we find the
  // encoding statement.
  //
  *pulUINT8sOfEncoding = pChosenEncoder->cbToDiscard;
  PrivateState.RawTokenState.pfnNextChar = pChosenEncoder->pfnFastDecoder;
  PrivateState.RawTokenState.pvCursor = (VOID*)(((UINTN)PrivateState.RawTokenState.pvCursor) + pChosenEncoder->cbToDiscard);
  pState->RawTokenState.pfnNextChar = pChosenEncoder->pfnFastDecoder;
  pState->RawTokenState.EncodingFamily = pChosenEncoder->Family;

  if (NT_SUCCESS(status = RtlXmlNextToken(&PrivateState, &Token, TRUE))) {

    BOOLEAN fNextValueIsEncoding = FALSE;

    //
    // Didn't find the XMLDECL opening, or we found an error during
    // tokenization?  Stop looking... return success, assume the caller
    // will do the Right Thing when it calls RtlXmlNextToken itself.
    //
    if ((Token.State != XTSS_XMLDECL_OPEN) || Token.fError) {
      goto Exit;
    }

    //
    // Let's look until we find the close of the XMLDECL, the end of
    // the document, or an error, for the encoding value
    //
    do {

      status = RtlXmlNextToken(&PrivateState, &Token, TRUE);
      if (EFI_ERROR(status)) {
        break;
      }

      //
      // Hmm... something odd, quit looking
      //
      if (Token.fError || (Token.State == XTSS_ERRONEOUS) ||
        (Token.State == XTSS_STREAM_END) || (Token.State == XTSS_XMLDECL_CLOSE)) {

        break;

      }
      //
      // Otherwise, is this the 'encoding' marker?
      //
      else if (Token.State == XTSS_XMLDECL_ENCODING) {
        fNextValueIsEncoding = TRUE;
      }
      //
      // See if this is what we really want
      //
      else if ((Token.State == XTSS_XMLDECL_VALUE) && fNextValueIsEncoding) {

        XML_STRING_COMPARE compare = XML_STRING_COMPARE_GT;

        for (u = 0; u != pChosenEncoder->EncodingNameCount; u++) {

          status = PrivateState.pfnCompareSpecialString(
            &PrivateState,
            &Token.Run,
            &pChosenEncoder->EncodingNameList[u],
            &compare,
            &RtlpUpcaseUcsCharacter);

          if (EFI_ERROR(status)) {
            goto Exit;
          }
          else if (compare == XML_STRING_COMPARE_EQUALS) {
            break;
          }
        }

        //
        // If we matched one, then we found an appropriate encoding
        // that we know and love.  We're done.  Otherwise, we have to
        // go ask the user what encoder they suggest.
        //
        if (compare != XML_STRING_COMPARE_EQUALS) {

          NTXMLRAWNEXTCHARACTER SelectedDecoder;

          //
          // No decoder selector?  Oops.
          //
          if (pState->DecoderSelection == NULL) {
            status = RtlpReportXmlError(STATUS_XML_ENCODING_MISMATCH);
            goto Exit;
          }

          SelectedDecoder = (*pState->DecoderSelection)(&Token.Run);

          //
          // The user failed to select a different decoder.
          //
          if (SelectedDecoder == NULL) {
            status = RtlpReportXmlError(STATUS_XML_ENCODING_MISMATCH);
            goto Exit;
          }
          //
          // They handed us one, let's use it in place of the other one
          // that we were using before.
          //
          else {
            PrivateState.RawTokenState.pfnNextChar = SelectedDecoder;
            pState->RawTokenState.pfnNextChar = SelectedDecoder;
          }

        }

        fNextValueIsEncoding = FALSE;

      }

    } while (TRUE);

  }

  status = EFI_SUCCESS;
Exit:
  return status;


}



EFI_STATUS
EFIAPI
RtlXmlCloneRawTokenizationState(
  IN PXML_RAWTOKENIZATION_STATE pStartState,
  OUT PXML_RAWTOKENIZATION_STATE pTargetState
  )
{
  if (!pStartState || !pTargetState) {
    return RtlpReportXmlError(EFI_INVALID_PARAMETER);
  }

  *pTargetState = *pStartState;

  return EFI_SUCCESS;
}


EFI_STATUS EFIAPI
RtlXmlCloneTokenizationState(
  PXML_TOKENIZATION_STATE pStartState,
  PXML_TOKENIZATION_STATE pTargetState
  )
{
  if (!ARGUMENT_PRESENT(pStartState) || !ARGUMENT_PRESENT(pTargetState)) {
    return RtlpReportXmlError(EFI_INVALID_PARAMETER);
  }

  *pTargetState = *pStartState;

  return EFI_SUCCESS;
}



EFI_STATUS
EFIAPI
RtlXmlCopyStringOut(
  IN PXML_RAWTOKENIZATION_STATE pState,
  IN PXML_EXTENT             pExtent,
  IN UINT32                  cbInTarget,
  CHAR16*                    pwszTarget,
  OUT UINT64                 *pCbResult
  )
{
  CHAR16* pwszWriteCursor = pwszTarget;
  CHAR16* pwszWriteEnd = (CHAR16*)(((UINTN)pwszTarget) + cbInTarget);
  VOID* pvCursor;
  VOID* pvDocumentEnd;

  if (pCbResult) {
    *pCbResult = 0;
  }
  if ((pwszTarget != NULL) && (cbInTarget < 2))
  {
    return RtlpReportXmlError(STATUS_BUFFER_TOO_SMALL);
  }


  if (pwszTarget) {
    *pwszTarget = L'\0';
  }


  if ((cbInTarget % sizeof(CHAR16)) != 0)
    return RtlpReportXmlError(EFI_INVALID_PARAMETER);

  if (!pState || !pExtent || !pCbResult)
    return RtlpReportXmlError(EFI_INVALID_PARAMETER);

  if (cbInTarget && (pwszTarget == NULL))
    return RtlpReportXmlError(EFI_INVALID_PARAMETER);

  pvCursor = pExtent->pvData;
  pvDocumentEnd = (VOID*)(((UINTN)pExtent->pvData) + (UINTN)pExtent->cbData);

  while (pvCursor < pvDocumentEnd) {

    XML_RAWTOKENIZATION_RESULT Result = (*pState->pfnNextChar)(pvCursor, pvDocumentEnd);

    if (Result.Character == XML_RAWTOKENIZATION_INVALID_CHARACTER) {
      return Result.Result.ErrorCode;
    }

    //
    // Normal character
    //
    if (Result.Character < 0x10000) {

      if (pwszWriteCursor && pwszWriteCursor < pwszWriteEnd) {
        pwszWriteCursor[0] = (CHAR16)Result.Character;
      }

      pwszWriteCursor++;
    }
    //
    // Two chars required
    //
    else if (Result.Character < 0x110000) {

      if ((pwszWriteEnd + 2) <= pwszWriteEnd && pwszWriteCursor != NULL) {
        pwszWriteCursor[0] = (CHAR16)(((Result.Character - 0x10000) / 0x400) + 0xd800);
        pwszWriteCursor[1] = (CHAR16)(((Result.Character - 0x10000) % 0x400) + 0xdc00);
      }

      pwszWriteCursor += 2;
    }
    else {
      return RtlpReportXmlError(STATUS_ILLEGAL_CHARACTER);
    }

    pvCursor = Result.Result.NextCursor;
  }

  *pCbResult = ((UINT64)pwszWriteCursor) - ((UINT64)pwszTarget);

  if (*pCbResult > cbInTarget) {
    return RtlpReportXmlError(STATUS_BUFFER_TOO_SMALL);
  }

  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
RtlXmlIsExtentWhitespace(
  IN_OUT PXML_RAWTOKENIZATION_STATE pState,
  IN PCXML_EXTENT Run,
  OUT BOOLEAN* pfIsWhitespace
  )
{
  VOID* pvCursor;
  VOID* pvDocumentEnd;

  if (pfIsWhitespace)
    *pfIsWhitespace = FALSE;

  if (!pState || !pfIsWhitespace || !Run)
    return RtlpReportXmlError(EFI_INVALID_PARAMETER);

  pvCursor = Run->pvData;
  pvDocumentEnd = (VOID*)(((UINTN)pvCursor) + (UINTN)Run->cbData);

  while (pvCursor < pvDocumentEnd)
  {
    XML_RAWTOKENIZATION_RESULT Result = (*pState->pfnNextChar)(pvCursor, pvDocumentEnd);

    if (Result.Character == XML_RAWTOKENIZATION_INVALID_CHARACTER) {
      return Result.Result.ErrorCode;
    }
    else if (_RtlpDecodeCharacter(Result.Character) != NTXML_RAWTOKEN_WHITESPACE) {
      return EFI_SUCCESS;
    }

    pvCursor = Result.Result.NextCursor;
  }

  //
  // Yes, it was all whitespace
  //
  *pfIsWhitespace = TRUE;
  return EFI_SUCCESS;
}


