/**
Xml Parsing Engine

Copyright(c) 2017, Microsoft Corporation

All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met :
1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and / or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES(INCLUDING,
  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT(INCLUDING NEGLIGENCE
    OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

**/

#ifndef _FASTERXML_H_INCLUDED_
#define _FASTERXML_H_INCLUDED_


#pragma once

#include <Library/DebugLib.h> // ASSERT

#ifndef IN_OUT
#define IN_OUT
#endif// IN_OUT

#ifndef FORCEINLINE
#define FORCEINLINE __inline
#endif

#ifdef __cplusplus
extern "C" {
#endif


typedef struct _XML_SIMPLE_STRING
{
    UINT32 Length;
    CHAR16* Buffer;
} XML_SIMPLE_STRING, *PXML_SIMPLE_STRING;
typedef const XML_SIMPLE_STRING *PCXML_SIMPLE_STRING;

#define CONSTANT_XML_SIMPLE_STRING(x) { sizeof(x) - sizeof(CHAR16), (x) }


//
// These "raw tokens" are the stuff that comes out of the
// base tokenization engine.  Special characters are given
// names, a 'special character' being one that is called out
// anywhere in the XML spec as having a meaning other than
// text.
//
typedef enum
{
    NTXML_RAWTOKEN_ERROR,
    NTXML_RAWTOKEN_DASH,
    NTXML_RAWTOKEN_DOT,
    NTXML_RAWTOKEN_END_OF_STREAM,
    NTXML_RAWTOKEN_EQUALS,
    NTXML_RAWTOKEN_FORWARDSLASH,
    NTXML_RAWTOKEN_GT,
    NTXML_RAWTOKEN_LT,
    NTXML_RAWTOKEN_QUESTIONMARK,
    NTXML_RAWTOKEN_QUOTE,
    NTXML_RAWTOKEN_DOUBLEQUOTE,
    NTXML_RAWTOKEN_START_OF_STREAM,
    NTXML_RAWTOKEN_TEXT,
    NTXML_RAWTOKEN_WHITESPACE,
    NTXML_RAWTOKEN_OPENBRACKET,
    NTXML_RAWTOKEN_CLOSEBRACKET,
    NTXML_RAWTOKEN_BANG,
    NTXML_RAWTOKEN_OPENCURLY,
    NTXML_RAWTOKEN_CLOSECURLY,
    NTXML_RAWTOKEN_OPENPAREN,
    NTXML_RAWTOKEN_CLOSEPAREN,
    NTXML_RAWTOKEN_COLON,
    NTXML_RAWTOKEN_SEMICOLON,
    NTXML_RAWTOKEN_UNDERSCORE,
    NTXML_RAWTOKEN_AMPERSAND,
    NTXML_RAWTOKEN_POUNDSIGN,
    NTXML_RAWTOKEN_PERCENT,
} NTXML_RAW_TOKEN;


typedef enum {

    XMLEF_UNKNOWN = 0,
    XMLEF_UCS_4_LE,
    XMLEF_UCS_4_BE,
    XMLEF_UTF_16_LE,
    XMLEF_UTF_16_BE,
    XMLEF_UTF_8_OR_ASCII

} XML_ENCODING_FAMILY;


typedef struct _XML_LINE_AND_COLUMN
{
    UINT32 Line;
    UINT32 Column;
}
XML_LINE_AND_COLUMN, *PXML_LINE_AND_COLUMN;
typedef const XML_LINE_AND_COLUMN *PCXML_LINE_AND_COLUMN;


typedef struct _XML_EXTENT {
    VOID*   pvData;                 // Pointer into the original XML document
    UINT64  cbData;                 // UINT8 count from the extent base
    XML_ENCODING_FAMILY Encoding;   // Encoding family for faster decoding
    UINT64   ulCharacters;           // Character count in this extent
}
XML_EXTENT, *PXML_EXTENT;

typedef const XML_EXTENT *PCXML_EXTENT;

typedef struct _XML_RAWTOKENIZATION_STATE *PXML_RAWTOKENIZATION_STATE;
typedef struct _XML_TOKENIZATION_STATE *PXML_TOKENIZATION_STATE;

//
// This interface decodes a single UINT8 from the input stream.  On success,
// the Character is any valid character, and Result.NextCursor is the next UINT8
// from the input stream to be analyzed.  On failure, ErrorCode is 0xffffffff
// and ErrorCode provides more specific error information.
//
typedef struct _XML_RAWTOKENIZATION_RESULT {
    UINT32 Character;
    union {
        EFI_STATUS ErrorCode;
        VOID* NextCursor;
    } Result;
} XML_RAWTOKENIZATION_RESULT, *PXML_RAWTOKENIZATION_RESULT;
typedef const XML_RAWTOKENIZATION_RESULT *PCXML_RAWTOKENIZATION_RESULT;

#define XML_RAWTOKENIZATION_INVALID_CHARACTER (0xffffffff)


//
// On input, pvCursor points at the next UINT8 to decoded, and
// pvEnd points at the final UINT8 of input.
//
typedef XML_RAWTOKENIZATION_RESULT (EFIAPI *NTXMLRAWNEXTCHARACTER)(
    const void *pvCursor,
    IN const void *pvEnd
    );

//
// Clients of the raw tokenizer can provide their own decoding
// functions here.  This function will be called with a 7-bit
// string from the XML decl's "encoding" attribute indicating
// what encoder should be returned.  The function returned -
// or NULL if one can't be provided - will be called to decode
// each UINT8 of the input string, sometimes more than once.
//
// Because of the autodetection of encodings, this function will
// only be called for encodings detected other than utf8, utf16,
// and ucs4 (and their LE/BE variants).  The XML spec says that
// this string must only be 7-bit ascii.
//
typedef NTXMLRAWNEXTCHARACTER (EFIAPI *NTXMLFETCHCHARACTERDECODER)(
    PCXML_EXTENT EncodingName
    );


extern const XML_SIMPLE_STRING xss_CDATA;
extern const XML_SIMPLE_STRING xss_xml;
extern const XML_SIMPLE_STRING xss_encoding;
extern const XML_SIMPLE_STRING xss_standalone;
extern const XML_SIMPLE_STRING xss_version;

//
// A 'raw' token is more or less a run of UINT8s in the XML that is given
// a name.  The low-level tokenizer returns these as it runs, and assumes
// that the higher-level tokenizer knows how to turn groups of these into
// productions, and from there the lexer knows how to turn groups of the
// real tokens into meaning.
//
typedef struct _XML_RAW_TOKEN
{
    //
    // This is the 'name' of this token, so that we can easily switch on
    // it in upper-level layers.
    //
    NTXML_RAW_TOKEN     TokenName;

    //
    // Pointer and length of the extent
    //
    XML_EXTENT          Run;
}
XML_RAW_TOKEN, *PXML_RAW_TOKEN;

//
// This is the base tokenization state blob necessary to keep tokenizing
// between calls.  See member descriptions for more details.
//
typedef struct _XML_RAWTOKENIZATION_STATE
{

    //
    // VOID* and length of the original XML document
    //
    XML_EXTENT              OriginalDocument;

    //
    // Pointer to the 'end' of the document.
    //
    VOID* pvDocumentEnd;

    //
    // Pointer into the XML data that represents where we are at the moment
    // in tokenization.  Will not be moved by the raw tokenizer - you must
    // use the NtRawXmlAdvanceCursor (or related) to move the cursor along
    // the data stream.  Hence, calling the tokenizer twice in a row will
    // get you the same token.
    //
    VOID*                   pvCursor;

    //
    // Returns the next character out of the input stream.
    //
    NTXMLRAWNEXTCHARACTER pfnNextChar;

    //
    // The encoding family can be detected from the first UINT8s in the
    // incoming stream.  They are classified according to the XML spec,
    // which defaults to UTF-8.
    //
    XML_ENCODING_FAMILY     EncodingFamily;

    //
    // When the upper-level tokenizer detects the "encoding" statement
    // in the <?xml ...?> declaration, it should set this member to the
    // code page that was found.  Noticably, this will start out as
    // zero on initialization.  A smart "next character" function will
    // do some default operation to continue working even if this is
    // unset.
    //
    UINT32                   DetectedCodePage;

    XML_RAW_TOKEN LastTokenCache;
    VOID* pvLastCursor;

    //
    // Locations in the document.  These help display sane error messages
    // to the user.
    //
    UINT32                   CurrentLineNumber;
    UINT32                   CurrentCharacter;
}
XML_RAWTOKENIZATION_STATE, *PXML_RAWTOKENIZATION_STATE;


//
// Simple interface out to the Real World.  This allocator should be
// replaced (eventually) with calls directly into the proper
// allocator (HeapAlloc/ExAllocatePoolWithTag) in production code.
//
typedef EFI_STATUS (*NTXML_ALLOCATOR)(
    UINT32 ulUINT8s,
    VOID* *ppvAllocated,
    VOID* pvAllocationContext);

//
// Frees memory allocated with the corresponding NTXML_ALLOCATOR
// call.
//
typedef EFI_STATUS (*NTXML_DEALLOCATOR)(VOID* pvAllocated, VOID* pvContext);


/*++

Normal operation would go like this:

  <?xml version="1.0"? encoding="UTF-8" standalone="yes"?>
  <!-- commentary -->
  <?bonk foo?>
  <ham>
    <frooby:cheese hot="yes"/>
  </ham>

  XTLS_STREAM_START
  XTLS_XMLDECL                      {XTSS_XMLDECL_OPEN      "<?xml"         }
  XTLS_XMLDECL                      {XTSS_XMLDECL_VERSION   "version"       }
  XTLS_XMLDECL                      {XTSS_XMLDECL_EQUALS    "="             }
  XTLS_XMLDECL                      {XTSS_XMLDECL_VALUE     "1.0"           }
  XTLS_XMLDECL                      {XTSS_XMLDECL_ENCODING  "encoding"      }
  XTLS_XMLDECL                      {XTSS_XMLDECL_EQUALS    "="             }
  XTLS_XMLDECL                      {XTSS_XMLDECL_VALUE     "UTF-8"         }
  XTLS_XMLDECL                      {XTSS_XMLDECL_STANDALONE "standalone"   }
  XTLS_XMLDECL                      {XTSS_XMLDECL_EQUALS    "="             }
  XTLS_XMLDECL                      {XTSS_XMLDECL_VALUE     "yes"           }
  XTLS_XMLDECL                      {XTSS_XMLDECL_CLOSE     "?>"            }
  XTLS_COMMENT                      {XTSS_COMMENT_OPEN      "<!--"          }
  XTLS_COMMENT                      {XTSS_COMMENT_CONTENT   " commentary "  }
  XTLS_COMMENT                      {XTSS_COMMENT_CLOSE     "-->"           }
  XTLS_PROCESSING_INSTRUCTION       {XTSS_PI_OPEN           "<?"            }
  XTLS_PROCESSING_INSTRUCTION       {XTSS_PI_NAME           "bonk"          }
  XTLS_PROCESSING_INSTRUCTION       {XTSS_PI_CONTENT        "foo"           }
  XTLS_PROCESSING_INSTRUCTION       {XTSS_PI_CLOSE          "?>"            }
  XTLS_FLOATINGDATA                 {XTSS_FD_WHITESPACE     "\n"            }
  XTLS_ELEMENT                      {XTSS_ELEMENT_OPEN      "<"             }
  XTLS_ELEMENT                      {XTSS_ELEMENT_NAME      "ham"           }
  XTLS_ELEMENT                      {XTSS_ELEMENT_CLOSE     ">"             }
  XTLS_FLOATINGDATA                 {XTSS_FLOATINGDATA      "\n  "          }
  XTLS_ELEMENT                      {XTSS_ELEMENT_OPEN      "<"             }
  XTLS_ELEMENT                      {XTSS_ELEMENT_NAMESPACE "frooby"        }
  XTLS_ELEMENT                      {XTSS_ELEMENT_NAME      "cheese"        }
  XTLS_ELEMENT                      {XTSS_ELEMENT_VALUENAME "hot"           }
  XTLS_ELEMENT                      {XTSS_ELEMENT_VALUE     "yes"           }
  XTLS_ELEMENT                      {XTSS_ELEMENT_EMPTYCLOSE   "/>"         }
  XTLS_FLOATINGDATA                 {XTSS_FLOATINGDATA      "\n"            }
  XTLS_ELEMENT                      {XTSS_ELEMENT_CLOSETAG  "</"            }
  XTLS_ELEMENT                      {XTSS_ELEMENT_NAME      "ham"           }
  XTLS_ELEMENT                      {XTSS_ELEMENT_CLOSE     ">"             }
  XTLS_STREAM_END

--*/


typedef enum {

    //
    // No state is assigned yet
    //
    XTSS_NOTHING,

    //
    // The last error member is set if this is indicated
    //
    XTSS_ERRONEOUS,


    //
    // In the middle of "nowhere" - the hyperspace between elements
    //
    XTSS_STREAM_HYPERSPACE,

    //
    // At the start of the input stream
    //
    XTSS_STREAM_START,

    //
    // At the end of the input stream
    //
    XTSS_STREAM_END,


    ////////////////////////////////////////////
    //
    // ELEMENT STATES
    //
    ////////////////////////////////////////////

    //
    // Meaning:     An element tag was found.
    //
    // Rawtoken:    NTXML_RAWTOKEN_LT
    //
    XTSS_ELEMENT_OPEN,

    //
    // Meaning:     A run of text was found that could represent a name.
    //              This is basically all the text found between the opening
    //              element tag and some illegal values.
    //
    // Rawtoken:    A run of any of the following:
    //                  NTXML_RAWTOKEN_TEXT
    //                  NTXML_RAWTOKEN_DOT
    //                  NTXML_RAWTOKEN_COLON
    //                  NTXML_RAWTOKEN_UNDERSCORE
    //                  NTXML_RAWTOKEN_DASH
    //              The name ends when something else appears.
    //
    XTSS_ELEMENT_NAME,


    //
    // Found the xmlns part of <foo xmlns:bar=
    //
    XTSS_ELEMENT_XMLNS,

    //
    // Found <foo xmlns=
    //
    XTSS_ELEMENT_XMLNS_DEFAULT,

    //
    // Found the 'a' in <foo xml:a=
    //
    XTSS_ELEMENT_XMLNS_ALIAS,

    //
    // Found the colon between xmlns and the alias
    //
    XTSS_ELEMENT_XMLNS_COLON,

    //
    // Found the equals sign between xmlns and the value
    //
    XTSS_ELEMENT_XMLNS_EQUALS,

    XTSS_ELEMENT_XMLNS_VALUE_OPEN,
    XTSS_ELEMENT_XMLNS_VALUE_CLOSE,
    XTSS_ELEMENT_XMLNS_VALUE,

    //
    // Found the xml part of <foo xml:bar=
    //
    XTSS_ELEMENT_XML,

    //
    // Found the colon between xml and the local name
    //
    XTSS_ELEMENT_XML_COLON,

    XTSS_ELEMENT_XML_NAME,
    XTSS_ELEMENT_XML_EQUALS,
    XTSS_ELEMENT_XML_VALUE_OPEN,
    XTSS_ELEMENT_XML_VALUE,
    XTSS_ELEMENT_XML_VALUE_CLOSE,

    //
    // This is the prefix for an element name, if present
    //
    XTSS_ELEMENT_NAME_NS_PREFIX,

    //
    // This is the colon after an element name ns prefix
    //
    XTSS_ELEMENT_NAME_NS_COLON,

    //
    // This is the prefix on an attribute name for a namespace
    //
    XTSS_ELEMENT_ATTRIBUTE_NAME_NS_PREFIX,

    //
    // This is the colon after an element attribute name namespace prefix
    //
    XTSS_ELEMENT_ATTRIBUTE_NAME_NS_COLON,

    //
    // Meaning:     A close of a tag (>) was found
    //
    // Rawtoken:    NTXML_RAWTOKEN_GT
    //
    XTSS_ELEMENT_CLOSE,

    //
    // Meaning:     An empty-tag (/>) was found
    //
    // Rawtoken:    NTXML_RAWTOKEN_FORWARDSLASH NTXML_RAWTOKEN_GT
    //
    XTSS_ELEMENT_CLOSE_EMPTY,

    //
    // Meaning:     An attribute name was found
    //
    // Rawtoken:    See rules for XTSS_ELEMENT_NAME
    //
    XTSS_ELEMENT_ATTRIBUTE_NAME,

    //
    // Meaning:     An equals sign was found in an element
    //
    // Rawtoken:    NTXML_RAWTOKEN_EQUALS
    //
    XTSS_ELEMENT_ATTRIBUTE_EQUALS,

    //
    // Meaning:     Element attribute value data was found after a
    //              quote of some variety.
    //
    // Rawtoken:    A run of any thing that's not the following:
    //                  NTXML_RAWTOKEN_LT
    //                  NTXML_RAWTOKEN_QUOTE (unless this quote is not the same
    //                                        as the quote in
    //                                          XTSS_ELEMENT_ATTRIBUTE_QUOTE)
    //
    // N.B.:        See special rules on handling entities in text.
    //
    XTSS_ELEMENT_ATTRIBUTE_VALUE,
    XTSS_ELEMENT_ATTRIBUTE_OPEN,
    XTSS_ELEMENT_ATTRIBUTE_CLOSE,

    //
    // Meaning:     Whitespace was found in the element tag at this point
    //
    // Rawtoken:    NTXML_RAWTOKEN_WHITESPACE
    //
    XTSS_ELEMENT_WHITESPACE,




    ////////////////////////////////////////////
    //
    // END ELEMENT SPECIFIC STATES
    //
    ////////////////////////////////////////////

    //
    // Meaning:     The start of an "end element" was found
    //
    // Rawtoken:    NTXML_RAWTOKEN_LT NTXML_RAWTOKEN_FORWARDSLASH
    //
    XTSS_ENDELEMENT_OPEN,

    //
    // Meaning:     The name of an end element was found
    //
    // Rawtoken:    See rules for XTSS_ELEMENT_NAME
    //
    XTSS_ENDELEMENT_NAME,

    //
    // Meaning:     We're in the whitespace portion of the end element
    //
    // Rawtoken:    NTXML_RAWTOKEN_WHITESPACE
    //
    XTSS_ENDELEMENT_WHITESPACE,

    //
    // Meaning:     The close of an endelement tag was found
    //
    // Rawtoken:    NTXML_RAWTOKEN_GT
    //
    XTSS_ENDELEMENT_CLOSE,

    //
    // Namespace prefix on the endelement name
    //
    XTSS_ENDELEMENT_NS_PREFIX,

    //
    // Colon after the namespace prefix in the endelement tag
    //
    XTSS_ENDELEMENT_NS_COLON,



    ////////////////////////////////////////////
    //
    // XML PROCESSING INSTRUCTION STATES
    //
    ////////////////////////////////////////////

    //
    // Meaning:     The start of an xml processing instruction was found
    //
    // Rawtokens:   NTXML_RAWTOKEN_LT NTXML_RAWTOKEN_QUESTIONMARK
    //
    XTSS_PI_OPEN,

    //
    // Meaning:     The end of an XML processing instruction was found
    //
    // Rawtokens:   NTXML_RAWTOKEN_QUESTIONMARK NTXML_RAWTOKEN_GT
    //
    XTSS_PI_CLOSE,

    //
    // Meaning:     The processing instruction name was found
    //
    // Rawtokens:   A nonempty stream of tokens identifying a name.  See the
    //              rules for XTSS_ELEMENT_NAME for details.
    //
    XTSS_PI_TARGET,

    //
    // Meaning:     Some processing instruction metadata was found.
    //
    // Rawtokens:   Anything except the sequence
    //                  NTXML_RAWTOKEN_QUESTIONMARK NTXML_RAWTOKEN_GT
    //
    XTSS_PI_VALUE,

    //
    // Meaning:     Whitespace between the target and the value was found
    //
    // Rawtokens:   NTXML_RAWTOKEN_WHITESPACE
    //
    XTSS_PI_WHITESPACE,



    ////////////////////////////////////////////
    //
    // XML PROCESSING INSTRUCTION STATES
    //
    ////////////////////////////////////////////

    //
    // Meaning:     Start of a comment block
    //
    // Rawtokens:   NTXML_RAWTOKEN_LT NTXML_RAWTOKEN_BANG NTXML_RAWTOKEN_DASH NTXML_RAWTOKEN_DASH
    //
    XTSS_COMMENT_OPEN,

    //
    // Meaning:     Commentary data, should be ignored by a good processor
    //
    // Rawtokens:   Anything except the sequence:
    //                  NTXML_RAWTOKEN_DASH NTXML_RAWTOKEN_DASH
    //
    XTSS_COMMENT_COMMENTARY,

    //
    // Meaning:     Comment close tag
    //
    // Rawtokens:   NTXML_RAWTOKEN_DASH NTXML_RAWTOKEN_DASH NTXML_RAWTOKEN_GT
    //
    XTSS_COMMENT_CLOSE,


    ////////////////////////////////////////////
    //
    // XML PROCESSING INSTRUCTION STATES
    //
    ////////////////////////////////////////////

    //
    // Meaning:     Opening of a CDATA block
    //
    // Rawtokens:   NTXML_RAWTOKEN_LT
    //              NTXML_RAWTOKEN_BRACE
    //              NTXML_RAWTOKEN_BANG
    //              NTXML_RAWTOKEN_TEXT (CDATA)
    //              NTXML_RAWTOKEN_BRACE
    //
    XTSS_CDATA_OPEN,

    //
    // Meaning:     Unparseable CDATA stuff
    //
    // Rawtokens:   Anything except the sequence
    //                  NTXML_RAWTOKEN_BRACE
    //                  NTXML_RAWTOKEN_BRACE
    //                  NTXML_RAWTOKEN_GT
    //
    XTSS_CDATA_CDATA,

    //
    // Meaning:     End of a CDATA block
    //
    XTSS_CDATA_CLOSE,


    ////////////////////////////////////////////
    //
    // XMLDECL (<?xml) states
    //
    ////////////////////////////////////////////

    XTSS_XMLDECL_OPEN,
    XTSS_XMLDECL_CLOSE,
    XTSS_XMLDECL_WHITESPACE,
    XTSS_XMLDECL_EQUALS,
    XTSS_XMLDECL_ENCODING,
    XTSS_XMLDECL_STANDALONE,
    XTSS_XMLDECL_VERSION,
    XTSS_XMLDECL_VALUE_OPEN,
    XTSS_XMLDECL_VALUE,
    XTSS_XMLDECL_VALUE_CLOSE,

    //
    // Entity states
    //
    // - Open means a & was found in the pcdata hyperspace
    // - Entity means the meat of the entity object was found
    // - Close means the ; of an entity was found.
    //
    XTSS_ENTITYREF_OPEN,
    XTSS_ENTITYREF_ENTITY,
    XTSS_ENTITYREF_CLOSE,

    //
    // Internal subset document type states.  If we're going to parse one,
    // we have to parse them all.
    //
    // doctypedecl ::= '<!DOCTYPE' DocName ExternalId? ('[' MarkupDecl* ']')? >
    // MarkupDecl ::= ElementDecl | AttListDecl | EntityDecl |
    //      NotationDecl | PI | Comment
    //
    XTSS_DOCTYPE_OPEN,          // '<!DOCTYPE'
    XTSS_DOCTYPE_WHITESPACE,    // Whitespace found, ignorable
    XTSS_DOCTYPE_DOCNAME,       // DocName, follows Identifier rules
    XTSS_DOCTYPE_EXTERNALID,    // Everything from the document name through [ or >.
    XTSS_DOCTYPE_MARKUP_OPEN,
    XTSS_DOCTYPE_MARKUP_WHITESPACE,
    XTSS_DOCTYPE_MARKUP_CLOSE,
    XTSS_DOCTYPE_CLOSE,

    //
    // ElementDecl ::= '<!ELEMENT' .*? '>'
    //
    XTSS_DOCTYPE_ELEMENTDECL_OPEN,
    XTSS_DOCTYPE_ELEMENTDECL_CONTENT,
    XTSS_DOCTYPE_ELEMENTDECL_CLOSE,

    //
    // AttListDecl ::= '<!ATTLIST' .*? '>'
    //
    XTSS_DOCTYPE_ATTLISTDECL_OPEN,
    XTSS_DOCTYPE_ATTLISTDECL_ELEMENT_NAME,
    XTSS_DOCTYPE_ATTLISTDECL_ELEMENT_PREFIX,
    XTSS_DOCTYPE_ATTLISTDECL_ELEMENT_COLON,
    XTSS_DOCTYPE_ATTLISTDECL_WHITESPACE,
    XTSS_DOCTYPE_ATTLISTDECL_ATTNAME,
    XTSS_DOCTYPE_ATTLISTDECL_ATTPREFIX,
    XTSS_DOCTYPE_ATTLISTDECL_ATTCOLON,
    XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_CDATA,
    XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_ID,
    XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_IDREF,
    XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_IDREFS,
    XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_ENTITY,
    XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_ENTITIES,
    XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_NMTOKEN,
    XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_NMTOKENS,
    XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_ENUMERATED_OPEN,
    XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_ENUMERATED_VALUE,
    XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_ENUMERATED_CLOSE,
    XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_NOTATION,
    XTSS_DOCTYPE_ATTLISTDECL_DEFAULT_REQUIRED,
    XTSS_DOCTYPE_ATTLISTDECL_DEFAULT_IMPLIED,
    XTSS_DOCTYPE_ATTLISTDECL_DEFAULT_FIXED,
    XTSS_DOCTYPE_ATTLISTDECL_DEFAULT_TEXT_OPEN,
    XTSS_DOCTYPE_ATTLISTDECL_DEFAULT_TEXT_VALUE,
    XTSS_DOCTYPE_ATTLISTDECL_DEFAULT_TEXT_CLOSE,
    XTSS_DOCTYPE_ATTLISTDECL_CLOSE,

    //
    // EntityDecl ::= '<!ENTITY' .*? '>'
    //
    XTSS_DOCTYPE_ENTITYDECL_OPEN,
    XTSS_DOCTYPE_ENTITYDECL_NAME,
    XTSS_DOCTYPE_ENTITYDECL_PARAMETERMARKER,        // % before name
    XTSS_DOCTYPE_ENTITYDECL_GENERALMARKER,          // No % between <!ENTITY and the name
    XTSS_DOCTYPE_ENTITYDECL_SYSTEM,
    XTSS_DOCTYPE_ENTITYDECL_SYSTEM_TEXT_OPEN,
    XTSS_DOCTYPE_ENTITYDECL_SYSTEM_TEXT_VALUE,
    XTSS_DOCTYPE_ENTITYDECL_SYSTEM_TEXT_CLOSE,
    XTSS_DOCTYPE_ENTITYDECL_PUBLIC,
    XTSS_DOCTYPE_ENTITYDECL_PUBLIC_TEXT_OPEN,
    XTSS_DOCTYPE_ENTITYDECL_PUBLIC_TEXT_VALUE,
    XTSS_DOCTYPE_ENTITYDECL_PUBLIC_TEXT_CLOSE,
    XTSS_DOCTYPE_ENTITYDECL_NDATA,
    XTSS_DOCTYPE_ENTITYDECL_NDATA_TEXT,
    XTSS_DOCTYPE_ENTITYDECL_VALUE_OPEN,
    XTSS_DOCTYPE_ENTITYDECL_VALUE_VALUE,
    XTSS_DOCTYPE_ENTITYDECL_VALUE_CLOSE,
    XTSS_DOCTYPE_ENTITYDECL_CLOSE,

    //
    // NotationDecl ::= '<!NOTATION' .*? '>'
    //
    XTSS_DOCTYPE_NOTATIONDECL_OPEN,
    XTSS_DOCTYPE_NOTATIONDECL_CONTENT,
    XTSS_DOCTYPE_NOTATIONDECL_CLOSE,


} XML_TOKENIZATION_SPECIFIC_STATE;


//
// Another, similar XML token structure for the 'cooked' XML bits.
//
typedef struct _XML_TOKEN {

    //
    // Pointer and length of the data in the token
    //
    XML_EXTENT      Run;

    //
    // What state are we in at the moment
    //
    XML_TOKENIZATION_SPECIFIC_STATE State;

    //
    // Was there an error gathering up this state?
    //
    BOOLEAN fError;

}
XML_TOKEN, *PXML_TOKEN;

typedef const XML_TOKEN *PCXML_TOKEN;

typedef enum {
    XML_STRING_COMPARE_EQUALS = 0,
    XML_STRING_COMPARE_GT = 1,
    XML_STRING_COMPARE_LT = -1
}
XML_STRING_COMPARE;


typedef UINT32 (EFIAPI *NTXMLTRANSFORMCHARACTER)(
    UINT32 ulCharacter
    );

//
// This function knows how to compare a pvoid and a length against
// a 7-bit ascii string
//
typedef EFI_STATUS (*NTXMLSPECIALSTRINGCOMPARE)(
    PXML_TOKENIZATION_STATE         pState,
    PCXML_EXTENT                    pRawToken,
    PCXML_SIMPLE_STRING                pSpecialString,
    XML_STRING_COMPARE             *pfResult,
    NTXMLTRANSFORMCHARACTER         pTransformation
    );



//
// Compare two extents
//
typedef EFI_STATUS (*NTXMLCOMPARESTRINGS)(
    PXML_TOKENIZATION_STATE TokenizationState,
    PCXML_EXTENT pLeft,
    PCXML_EXTENT pRight,
    XML_STRING_COMPARE *pfEquivalent);


typedef EFI_STATUS (*RTLXMLCALLBACK)(
    VOID*                           pvCallbackContext,
    PXML_TOKENIZATION_STATE         State,
    PCXML_TOKEN                     Token,
    BOOLEAN*                        StopTokenization
    );


//
// Now let's address the 'cooked' tokenization
// methodology.
//
typedef struct _XML_TOKENIZATION_STATE {

    //
    // Core tokenization state data
    //
    XML_RAWTOKENIZATION_STATE RawTokenState;

    //
    // State values
    //
    XML_TOKENIZATION_SPECIFIC_STATE PreviousState;

    //
    // Scratch pad for holding tokens
    //
    XML_RAW_TOKEN RawTokenScratch[6];

    //
    // Ways to compare two strings
    //
    NTXMLCOMPARESTRINGS pfnCompareStrings;

    //
    // In documents that do not have BOMs, or in documents where the
    // input stream's encoding is not supported by a built-in decoder,
    // we must ask the caller when the encoding switches to know what
    // encoder should be used.
    //
    NTXMLFETCHCHARACTERDECODER DecoderSelection;

    //
    // Compare an extent against a 'magic' string
    //
    NTXMLSPECIALSTRINGCOMPARE pfnCompareSpecialString;

    //
    // User-specified context for doing the above two things
    //
    VOID* pvComparisonContext;

    //
    // Scratch space for the opening quote rawtoken name, if we're in
    // a quoted string (ie: attribute value, etc.)
    //
    NTXML_RAW_TOKEN         QuoteTemp;

    //
    // Current parse location
    //
    BOOLEAN                 SupportsLocations;
    XML_LINE_AND_COLUMN     Location;

}
XML_TOKENIZATION_STATE, *PXML_TOKENIZATION_STATE;



EFI_STATUS
RtlXmlAdvanceTokenization(
    IN_OUT PXML_TOKENIZATION_STATE pState,
    IN PXML_TOKEN pToken
    );



EFI_STATUS
RtlXmlDetermineStreamEncoding(
    PXML_TOKENIZATION_STATE pState,
    UINTN* pulUINT8sOfEncoding
    );


//
//
typedef struct XML_TOKENIZATION_INIT
{
    UINT32 Size;
    VOID* XmlData;
    UINT32 XmlDataSize;

    BOOLEAN SupportPosition;
    VOID* CallbackContext;
    NTXMLCOMPARESTRINGS StringComparison;
    NTXMLSPECIALSTRINGCOMPARE SpecialStringCompare;
    NTXMLFETCHCHARACTERDECODER FetchDecoder;

} XML_TOKENIZATION_INIT, *PXML_TOKENIZATION_INIT;
typedef const XML_TOKENIZATION_INIT *PCXML_TOKENIZATION_INIT;


EFI_STATUS
EFIAPI
RtlXmlInitializeTokenization(
    OUT PXML_TOKENIZATION_STATE     pState,
    IN PCXML_TOKENIZATION_INIT     pInit
    );

EFI_STATUS EFIAPI
RtlXmlCloneRawTokenizationState(
    IN PXML_RAWTOKENIZATION_STATE pStartState,
    OUT PXML_RAWTOKENIZATION_STATE pTargetState
    );


EFI_STATUS EFIAPI
RtlXmlCloneTokenizationState(
    PXML_TOKENIZATION_STATE pStartState,
    PXML_TOKENIZATION_STATE pTargetState
    );


EFI_STATUS
EFIAPI
RtlXmlNextToken(
    PXML_TOKENIZATION_STATE pState,
    PXML_TOKEN              pToken,
    BOOLEAN                 fAdvanceState
    );


EFI_STATUS
EFIAPI
RtlXmlGetCurrentLocation(
    PXML_TOKENIZATION_STATE State,
    PXML_LINE_AND_COLUMN Location
    );

/** Returns a string ordering, not necessarily alphabetical */
EFI_STATUS
EFIAPI
RtlXmlDefaultCompareStrings(
    PXML_TOKENIZATION_STATE pState,
    PCXML_EXTENT pLeft,
    PCXML_EXTENT pRight,
    XML_STRING_COMPARE *pfEqual
    );

EFI_STATUS
EFIAPI
RtlXmlCopyStringOut(
    IN PXML_RAWTOKENIZATION_STATE pState,
    IN PXML_EXTENT             pExtent,
    IN UINT32                  cbInTarget,
    CHAR16*                    pwszTarget,
    OUT UINT64*                pCbResult
    );

EFI_STATUS
EFIAPI
RtlXmlIsExtentWhitespace(
    IN_OUT PXML_RAWTOKENIZATION_STATE pState,
    IN PCXML_EXTENT Run,
    OUT BOOLEAN* pfIsWhitespace
    );


#ifndef RTL_NUMBER_OF
#define RTL_NUMBER_OF(q) (sizeof(q)/sizeof((q)[0]))
#endif

#define ADVANCE_PVOID(pv, offset) ((pv) = (VOID*)(((UINTN)(pv)) + (UINTN)(offset)))
#define REWIND_PVOID(pv, offset) ((pv) = (VOID*)(((UINTN)(pv)) - (UINTN)(offset)))


typedef struct _RTL_ALLOCATOR
{
    EFI_STATUS (EFIAPI *pfnAlloc)(UINT32, VOID**, VOID*);

    EFI_STATUS (EFIAPI *pfnFree)(VOID*, VOID*);

    VOID* pvContext;

} RTL_ALLOCATOR, *PRTL_ALLOCATOR;


#define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)

#ifndef MAX_ULONG
#define MAX_ULONG ((UINT32)-1)
#endif



#ifndef ROUND_UP_COUNT
#define ROUND_UP_COUNT(Count,Pow2) \
        ( ((Count)+(Pow2)-1) & (~(((UINT32)(Pow2))-1)) )
#endif

#ifndef POINTER_IS_ALIGNED
#define POINTER_IS_ALIGNED(Ptr,Pow2) \
        ( ( ( ((UINT64)(Ptr)) & (((Pow2)-1)) ) == 0) ? TRUE : FALSE )
#endif


#ifndef offsetof
#define offsetof(s, m) ((UINT32)(&((s *)NULL)->m))
#endif


/*
    A vector-like structure for storing lists of attributes, namespaces, etc.
 */
typedef struct _RTL_GROWING_LIST_CHUNK {

    //
    // Pointer back to the parent list
    //
    struct _RTL_GROWING_LIST *pGrowingListParent;

    //
    // Pointer to the next chunk in the list
    //
    struct _RTL_GROWING_LIST_CHUNK *pNextChunk;

}
RTL_GROWING_LIST_CHUNK, *PRTL_GROWING_LIST_CHUNK;

#define GROWING_LIST_FLAG_IS_SORTED     (0x00000001)

typedef struct _RTL_GROWING_LIST {

    //
    // Any flags about this list?
    //
    UINT32 Flags;

    //
    // How many total elments are in this growing list?
    //
    UINT32 cTotalElements;

    //
    // How big is each element in this list?
    //
    UINT32 cbElementSize;

    //
    // How many to allocate per list chunk?  As each piece of the growing list
    // fills up, this is the number of elements to allocate to the new chunk
    // of the list.
    //
    UINT32 cElementsPerChunk;

    //
    // How many are in the initial internal list?
    //
    UINT32 cInternalElements;

    //
    // Pointer to the intial "internal" list, if specified by the caller
    //
    VOID* pvInternalList;

    //
    // The allocation-freeing context and function pointers
    //
    RTL_ALLOCATOR Allocator;

    //
    // First chunk
    //
    PRTL_GROWING_LIST_CHUNK pFirstChunk;

    //
    // Last chunk (quick access)
    //
    PRTL_GROWING_LIST_CHUNK pLastChunk;

}
RTL_GROWING_LIST, *PRTL_GROWING_LIST;



EFI_STATUS
RtlInitializeGrowingList(
    PRTL_GROWING_LIST       pList,
    UINT32                  cbElementSize,
    UINT32                   cElementsPerChunk,
    VOID*                   pvInitialListBuffer,
    UINT32                  cbInitialListBuffer,
    PRTL_ALLOCATOR          Allocator
    );

EFI_STATUS
RtlIndexIntoGrowingList(
    PRTL_GROWING_LIST       pList,
    UINT32                   ulIndex,
    VOID*                  *ppvPointerToSpace,
    BOOLEAN                 fGrowingAllowed
    );

EFI_STATUS
RtlDestroyGrowingList(
    PRTL_GROWING_LIST       pList
    );

//
// The growing list control structure can be placed anywhere in the allocation
// that's optimal (on cache boundaries, etc.)
//
#define RTL_INIT_GROWING_LIST_EX_FLAG_LIST_ANYWHERE     (0x00000001)


EFI_STATUS
RtlCloneGrowingList(
    UINT32                   Flags,
    PRTL_GROWING_LIST       pDestination,
    PRTL_GROWING_LIST       pSource,
    UINT32                   ulCount
    );


EFI_STATUS
RtlAllocateGrowingList(
    PRTL_GROWING_LIST          *ppGrowingList,
    UINT32                      cbThingSize,
    PRTL_ALLOCATOR              Allocator
    );

typedef EFI_STATUS (__cdecl *PFN_LIST_COMPARISON_CALLBACK)(
    PRTL_GROWING_LIST HostList,
    VOID* Left,
    VOID* Right,
    VOID* Context,
    int *Result
    );

EFI_STATUS
RtlSortGrowingList(
    PRTL_GROWING_LIST pGrowingList,
    UINT32 ItemCount,
    PFN_LIST_COMPARISON_CALLBACK SortCallback,
    VOID* SortContext
    );

EFI_STATUS
RtlSearchGrowingList(
    PRTL_GROWING_LIST TheList,
    UINT32 ItemCount,
    PFN_LIST_COMPARISON_CALLBACK SearchCallback,
    VOID* SearchTarget,
    VOID* SearchContext,
    VOID* *pvFoundItem
    );



/*++
    This structure contains an extent and a depth, which the namespace
    manager knows how to interpret in the right context.  The NS_NAMESPACE
    structure contains a list of these which represent aliases at various
    depths along the document structure.  The NS_DEFAULT_NAMESPACES contains
    a pseudo-stack which has the current 'default' namespace at the top.


--*/

#define NS_NAME_DEPTH_AVAILABLE ((UINT32)-1)

typedef struct NS_NAME_DEPTH {
    XML_EXTENT      Name;
    UINT32           Depth;
}
NS_NAME_DEPTH, *PNS_NAME_DEPTH;

#define NS_ALIAS_MAP_INLINE_COUNT       (2)
#define NS_ALIAS_MAP_GROWING_COUNT      (20)


typedef struct _NS_ALIAS {

    //
    // Is this in use?
    //
    BOOLEAN fInUse;

    //
    // The name of the alias - "x" or "asm" or the short tag before the : in
    // an element name, like <x:foo>
    //
    XML_EXTENT  AliasName;

    //
    // How many aliased namespaces are there?
    //
    UINT32 ulNamespaceCount;

    //
    // The namespaces that it can map to, with their depths
    //
    RTL_GROWING_LIST    NamespaceMaps;

    //
    // A list of some inline elements, for fun.  This is shallow, as it's
    // the typical case that someone will create a large set of aliases
    // to a small set of namespaces, rather than the other way around.
    //
    NS_NAME_DEPTH InlineNamespaceMaps[NS_ALIAS_MAP_INLINE_COUNT];

}
NS_ALIAS, *PNS_ALIAS;


#define NS_MANAGER_INLINE_ALIAS_COUNT       (5)
#define NS_MANAGER_ALIAS_GROWTH_SIZE        (40)
#define NS_MANAGER_DEFAULT_COUNT            (5)
#define NS_MANAGER_DEFAULT_GROWTH_SIZE      (40)

typedef EFI_STATUS (*PFNCOMPAREEXTENTS)(
    VOID* pvContext,
    PCXML_EXTENT pLeft,
    PCXML_EXTENT pRight,
    XML_STRING_COMPARE *pfMatching);



typedef struct _NS_MANAGER {

    //
    // How deep is the default namespace stack?
    //
    UINT32 ulDefaultNamespaceDepth;

    //
    // The default namespaces go into this list
    //
    RTL_GROWING_LIST    DefaultNamespaces;

    //
    // How many aliases are there?
    //
    UINT32 ulAliasCount;

    //
    // The array of aliases.  N.B. that this list can have holes in it, and
    // the user will have to do some special magic to find empty slots in
    // it to make efficient use of this.  Alternatively, you could have another
    // growing list representing a 'freed slot' stack, but I'm not sure that
    // would really be an optimization.
    //
    RTL_GROWING_LIST  Aliases;

    //
    // Comparison
    //
    PFNCOMPAREEXTENTS pfnCompare;

    //
    // Context
    //
    VOID* pvCompareContext;

    //
    // Inline list of aliases to start with
    //
    NS_ALIAS        InlineAliases[NS_MANAGER_INLINE_ALIAS_COUNT];
    NS_NAME_DEPTH   InlineDefaultNamespaces[NS_MANAGER_DEFAULT_COUNT];
}
NS_MANAGER, *PNS_MANAGER;


EFI_STATUS
RtlNsInitialize(
    PNS_MANAGER             pManager,
    PFNCOMPAREEXTENTS       pCompare,
    VOID*                   pCompareContext,
    PRTL_ALLOCATOR          Allocation
    );

EFI_STATUS
RtlNsDestroy(
    PNS_MANAGER pManager
    );

EFI_STATUS
RtlNsInsertDefaultNamespace(
    PNS_MANAGER     pManager,
    UINT32           ulDepth,
    PXML_EXTENT     pNamespace
    );

EFI_STATUS
RtlNsInsertNamespaceAlias(
    PNS_MANAGER     pManager,
    UINT32           ulDepth,
    PXML_EXTENT     pNamespace,
    PXML_EXTENT     pAlias
    );

EFI_STATUS
RtlNsLeaveDepth(
    PNS_MANAGER pManager,
    UINT32       ulDepth
    );

EFI_STATUS
RtlNsGetNamespaceForAlias(
    PNS_MANAGER     pManager,
    UINT32           ulDepth,
    PXML_EXTENT     Alias,
    PXML_EXTENT     pNamespace
    );



#ifdef __cplusplus
}
#endif

#endif
