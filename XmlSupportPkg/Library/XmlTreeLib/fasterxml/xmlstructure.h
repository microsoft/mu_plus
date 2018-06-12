/**
Structure definitions used in the XML parsing engine.


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


#ifndef _FUSION_FASTERXML_INC_XMLSTRUCTURE_H_
#define _FUSION_FASTERXML_INC_XMLSTRUCTURE_H_

#pragma once

#ifdef __cplusplus
extern "C" {
#endif



typedef struct _UNICODE_STRING
{
    UINT16 Length;
    UINT16 MaximumLength;
    CHAR16 *Buffer;
} UNICODE_STRING, *PUNICODE_STRING;
typedef const UNICODE_STRING *PCUNICODE_STRING;

typedef struct _LUTF8_STRING
{
    UINT32 Length;
    UINT32 MaximumLength;
    UINT8 *Buffer;
} LUTF8_STRING, *PLUTF8_STRING;
typedef const LUTF8_STRING *PCLUTF8_STRING;


enum XMLDOC_THING_TYPE {
    XMLDOC_THING_ERROR,
    XMLDOC_THING_END_OF_STREAM,
    XMLDOC_THING_XMLDECL,
    XMLDOC_THING_ELEMENT,
    XMLDOC_THING_END_ELEMENT,
    XMLDOC_THING_PROCESSINGINSTRUCTION,
    XMLDOC_THING_ATTRIBUTE,
    XMLDOC_THING_HYPERSPACE,
    XMLDOC_THING_CDATA,
    XMLDOC_THING_ENTITYDECL,
    XMLDOC_THING_ATTLIST,
    XMLDOC_THING_ATTDEF,
    XMLDOC_THING_END_ATTLIST,
    XMLDOC_THING_COMMENT,
};

typedef enum {
    XMLERROR_SUCCESS = 0,
    XMLERROR_ATTRIBUTE_NAME_NOT_FOUND,              // <binky foo:=""> or <binky =""/> - Attribute name part not found
    XMLERROR_ATTRIBUTE_NS_PREFIX_MISSING_COLON,     // <bingy foo="ham"> - Somehow we got into a state where we thought we had a namespace prefix, but it wasn't followed by a colon
    XMLERROR_CDATA_MALFORMED,                       // CDATA not properly formed?
    XMLERROR_ELEMENT_NAME_NOT_FOUND,                // < binky="bleep"> or <foo: /> - element name not found
    XMLERROR_ELEMENT_NS_PREFIX_MISSING_COLON,
    XMLERROR_ENDELEMENT_MALFORMED,                  // EOF before end of element found, or other problem
    XMLERROR_ENDELEMENT_MALFORMED_NAME,             // The name was malformed .. ns missing or something like that
    XMLERROR_ENDELEMENT_NAME_NOT_FOUND,             // Missing the name part of a </> tag.
    XMLERROR_EOF_BEFORE_CLOSE,                      // EOF was found before the element stack emptied
    XMLERROR_NS_UNKNOWN_PREFIX,                     // The prefix found was undeclared on the element
    XMLERROR_NS_DECL_GENERAL_FAILURE,               // General problem reading xmldecls
    XMLERROR_NS_DECL_RESERVED_PREFIX,               // The declaration attempted to define a reserved prefix
    XMLERROR_NS_DECL_MISSING_EQUALS,                // The namespace declaration was missing an equals sign
    XMLERROR_NS_DECL_MISSING_QUOTE,                 // The namespace declaration was missing a quote (single or double)
    XMLERROR_NS_DECL_MISSING_VALUE,                 // The namespace declaration was missing a value
    XMLERROR_PI_CONTENT_ERROR,                      // There was a problem with the content of the processing instruction
    XMLERROR_PI_EOF_BEFORE_CLOSE,
    XMLERROR_PI_TARGET_NOT_FOUND,
    XMLERROR_XMLDECL_INVALID_FORMAT,                // Something rotten in the <?xml?>
    XMLERROR_XMLDECL_NOT_FIRST_THING,
    XMLERROR_ENTITYDECL_NAME_MALFORMED,
    XMLERROR_ENTITYDECL_SYSTEM_ID_INVALID,
    XMLERROR_ENTITYDECL_PUBLIC_ID_INVALID,
    XMLERROR_ENTITYDECL_VALUE_INVALID,
    XMLERROR_ENTITYDECL_NDATA_INVALID,
    XMLERROR_ENTITYDECL_MISSING_CLOSE,
    XMLERROR_ENTITYDECL_MISSING_TYPE_INDICATOR,
    XMLERROR_ENDELEMENT_MISMATCHED_CLOSE_TAG,       // <foo></bar>
    XMLERROR_INVALID_POST_ROOT_ELEMENT_CONTENT,     // <foo/>bar - According to [1] and [27], "bar" is not allowed there
    XMLERROR_NS_DECL_PREFIX_REDEFINITION,           // <foo xmlns:a="b" xmlns:a="b"/>
    XMLERROR_NS_DECL_DEFAULT_REDEFINITION,          // <foo xmlns="a" xmlns="a"/>
    XMLERROR_COMMENT_MALFORMED,                     // Comment missing commentary or close
} LOGICAL_XML_ERROR;

typedef struct _XMLDOC_ELEMENT {
    //
    // Name of this element tag
    //
    XML_EXTENT Name;

    //
    // Namespace prefix
    //
    XML_EXTENT NsPrefix;

    //
    // Original namespace prefix
    //
    XML_EXTENT OriginalNsPrefix;

    //
    // Location in the original XML document
    //
    XML_LINE_AND_COLUMN Location;

    //
    // How many attributes are there?
    //
    UINT32 ulAttributeCount;

    //
    // Is this element empty?
    //
    BOOLEAN fElementEmpty;

}
XMLDOC_ELEMENT, *PXMLDOC_ELEMENT;

typedef struct _XMLDOC_ERROR {
    //
    // The erroneous extent
    //
    XML_EXTENT  BadExtent;

    //
    // Location in the original XML document
    //
    XML_LINE_AND_COLUMN Location;

    //
    // What was the error?
    //
    LOGICAL_XML_ERROR   Code;
}
XMLDOC_ERROR, *PXMLDOC_ERROR;

typedef struct _XMLDOC_ATTRIBUTE {
    //
    // Name of this attribute
    //
    XML_EXTENT Name;

    //
    // Namespace thereof
    //
    XML_EXTENT NsPrefix;

    //
    // The value of this attribute
    //
    XML_EXTENT Value;

    //
    // The original namespace prefix
    //
    XML_EXTENT OriginalNsPrefix;

    //
    // Location in the original XML document
    //
    XML_LINE_AND_COLUMN Location;

    //
    // If this is 'true', then the attribute was really
    // a namespace declaration
    //
    BOOLEAN WasNamespaceDeclaration;

    //
    // If this is true, then the element's prefix is "xml"
    //
    BOOLEAN HasXmlPrefix;
}
XMLDOC_ATTRIBUTE, *PXMLDOC_ATTRIBUTE;

typedef struct _XMLDOC_ENDELEMENT {
    //
    // End-element namespace prefix
    //
    XML_EXTENT NsPrefix;

    //
    // End-element tag name
    //
    XML_EXTENT Name;

    //
    // End-element prefix
    //
    XML_EXTENT OriginalNsPrefix;

    //
    // Location in the original XML document
    //
    XML_LINE_AND_COLUMN Location;

    //
    // Original element pointer
    //
    XMLDOC_ELEMENT OpeningElement;

}
XMLDOC_ENDELEMENT, *PXMLDOC_ENDELEMENT;

typedef struct _XMLDOC_XMLDECL {
    XML_EXTENT  Encoding;
    XML_EXTENT  Version;
    XML_EXTENT  Standalone;

    //
    // Location in the original XML document
    //
    XML_LINE_AND_COLUMN Location;

}
XMLDOC_XMLDECL, *PXMLDOC_XMLDECL;

typedef struct _XMLDOC_PROCESSING {

    XML_EXTENT Target;

    XML_EXTENT Instruction;

    //
    // Location in the original XML document
    //
    XML_LINE_AND_COLUMN Location;
}
XMLDOC_PROCESSING, *PXMLDOC_PROCESSING;

typedef struct _XMLDOC_ATTLIST
{
    XML_EXTENT NamespacePrefix;
    XML_EXTENT ElementName;
}
XMLDOC_ATTLIST, *PXMLDOC_ATTLIST;

#define DOCUMENT_ATTDEF_TYPE_CDATA                 0
#define DOCUMENT_ATTDEF_TYPE_ID                    1
#define DOCUMENT_ATTDEF_TYPE_IDREF                 2
#define DOCUMENT_ATTDEF_TYPE_IDREFS                3
#define DOCUMENT_ATTDEF_TYPE_ENTITY                4
#define DOCUMENT_ATTDEF_TYPE_ENTITIES              5
#define DOCUMENT_ATTDEF_TYPE_NMTOKEN               6
#define DOCUMENT_ATTDEF_TYPE_NMTOKENS              7
#define DOCUMENT_ATTDEF_TYPE_ENUMERATED            8
#define DOCUMENT_ATTDEF_TYPE_ENUMERATED_NOTATION   9

#define DOCUMENT_ATTDEF_DEFAULTDECL_TYPE_REQUIRED  0
#define DOCUMENT_ATTDEF_DEFAULTDECL_TYPE_IMPLIED   1
#define DOCUMENT_ATTDEF_DEFAULTDECL_TYPE_FIXED     2
#define DOCUMENT_ATTDEF_DEFAULTDECL_TYPE_NONE      3

typedef struct _XMLDOC_ATTDEF
{
    XML_EXTENT NamespacePrefix;
    XML_EXTENT AttributeName;

    UINT32 AttributeType;
    XML_EXTENT EnumeratedType;

    UINT32 DefaultDeclType;
    XML_EXTENT DefaultValue;
}
XMLDOC_ATTDEF, *PXMLDOC_ATTDEF;

typedef struct _XMLDOC_ENDATTLIST
{
    UINT32 ulUnused;
}
XMLDOC_ENDATTLIST, *PXMLDOC_ENDATTLIST;

//
// For entities, there's a pile of interesting things available
//
#define DOCUMENT_ENTITY_TYPE_PARAMETER          (1)
#define DOCUMENT_ENTITY_TYPE_GENERAL            (2)

//
// <!ENTITY Name "Foo">
//
#define DOCUMENT_ENTITY_VALUE_TYPE_NORMAL       (0)

//
// <!ENTITY Name SYSTEM "Foo">
//
#define DOCUMENT_ENTITY_VALUE_TYPE_SYSTEM       (1)

//
// <!ENTITY Name PUBLIC "Foo" SYSTEM "Bar">
//
#define DOCUMENT_ENTITY_VALUE_TYPE_PUBLIC       (2)


//
// Entities are hard.
//
typedef struct _XMLDOC_ENTITYDECL {

    //
    // Entities are either "parameter" (ie: usable later in the dtd),
    // or "general" (ie: not usable later in the dtd, but act as 'constants'
    // when parsing a document
    //
    UINT32       EntityType;
    UINT32       ValueType;

    //
    // This is always the 'name' part of an entitydecl
    //
    XML_EXTENT  Name;

    //
    // These are present/absent bases on the types above.
    //
    // - When the value type is 'normal', then 'normalvalue' is
    //   set
    //
    // - When the value type is 'system', then SystemId is set
    //
    // - When the value type is 'public', then both systemid and
    //   publicid are set
    //
    // - If an ndata type is present, then NDataType is set
    //
    XML_EXTENT  NormalValue;
    XML_EXTENT  SystemId;
    XML_EXTENT  PublicId;
    XML_EXTENT  NDataType;
}
XMLDOC_ENTITYDECL, *PXMLDOC_ENTITYDECL;

typedef struct _XMLDOC_PCDATA
{
    XML_EXTENT  Content;

    //
    // Location in the original XML document
    //
    XML_LINE_AND_COLUMN Location;
}
XMLDOC_PCDATA, *PXMLDOC_PCDATA;
typedef const XMLDOC_PCDATA *PCXMLDOC_PCDATA;

typedef struct _XMLDOC_CDATA
{
    XML_EXTENT  Content;

    //
    // Location in the original XML document
    //
    XML_LINE_AND_COLUMN Location;
}
XMLDOC_CDATA, *PXMLDOC_CDATA;
typedef const XMLDOC_CDATA *PCXMLDOC_CDATA;

typedef struct _XMLDOC_COMMENT
{
    XML_EXTENT Content;

    //
    // Location in the original XML document
    //
    XML_LINE_AND_COLUMN Location;
}
XMLDOC_COMMENT, *PXMLDOC_COMMENT;
typedef const XMLDOC_COMMENT *PCXMLDOC_COMMENT;


typedef struct _XMLDOC_THING {

    //
    // What kind of thing is this?
    //
    enum XMLDOC_THING_TYPE ulThingType;

    //
    // How deep down the document is it?
    //
    UINT32 ulDocumentDepth;


    //
    // Have the namespaces been fixed up yet?
    //
    BOOLEAN fNamespacesExpanded;

    //
    // The caller should be passing in a pointer to an attribute
    // list that they have initialized to contain XMLDOC_ATTRIBUTE
    // objects.
    //
    PRTL_GROWING_LIST AttributeList;

    //
    // The total extent of this thing in the document
    //
    XML_EXTENT TotalExtent;

    
    union 
    {

        XMLDOC_ERROR Error;

        XMLDOC_ELEMENT Element;

        //
        // The </close> tag
        //
        XMLDOC_ENDELEMENT EndElement;

        //
        // The pcdata that was found in this segment of the document
        //
        XMLDOC_CDATA CDATA;

        //
        // The hyperspace found in this section of the document
        //
        XMLDOC_PCDATA PCDATA;

        //
        // Information about the <?xml?> section of the document
        //
        XMLDOC_XMLDECL XmlDecl;

        //
        // A processing instruction has a target and an actual instruction
        //
        XMLDOC_PROCESSING ProcessingInstruction;

        //
        // Entity data
        //
        XMLDOC_ENTITYDECL EntityDecl;

        //
        // <!ATTLIST ...
        //
        XMLDOC_ATTLIST Attlist;

        //
        // An attribute definition within an ATTLIST
        //
        XMLDOC_ATTDEF Attdef;

        //
        // The close tag of an ATTLIST
        //
        XMLDOC_ENDATTLIST EndAttlist;

        //
        // The content between a <!-- and a -->
        //
        XMLDOC_COMMENT Comment;
    } item;

} XMLDOC_THING, *PXMLDOC_THING;

struct _tagXML_LOGICAL_STATE;


typedef EFI_STATUS (*PFN_CALLBACK_PER_LOGICAL_XML)(
    struct _tagXML_LOGICAL_STATE*       pLogicalState,
    PXMLDOC_THING                       pLogicalThing,
    PRTL_GROWING_LIST                   pAttributes,
    VOID*                               pvCallbackContext
    );



typedef struct _tagXML_LOGICAL_STATE{

    //
    // The overall state of parsing
    //
    XML_TOKENIZATION_STATE ParseState;

    //
    // Have we found the first element yet?
    //
    BOOLEAN fFirstElementFound;

    //
    // Depth of the 'element stack' that we're building up.
    //
    UINT32 ulElementStackDepth;

    //
    // Growing list that backs up the stack of elements.
    //
    RTL_GROWING_LIST ElementStack;

    //
    // Inline stuff to save some heap allocations.
    //
    XMLDOC_THING InlineElements[8];


}
XML_LOGICAL_STATE, *PXML_LOGICAL_STATE;


typedef struct _XML_ATTRIBUTE_DEFINITION {
    PCXML_SIMPLE_STRING Namespace;
    XML_SIMPLE_STRING Name;
} XML_ATTRIBUTE_DEFINITION, *PXML_ATTRIBUTE_DEFINITION;

typedef const XML_ATTRIBUTE_DEFINITION *PCXML_ATTRIBUTE_DEFINITION;


//
// The necessary stuff to initialize the logical parsing layer
//
typedef struct _XML_INIT_LOGICAL_LAYER {

    //
    // The size of this structure; it will probably grow over time
    //
    UINT32 Size;

    //
    // Allocator with which to do all allocation work
    //
    PRTL_ALLOCATOR Allocator;

    //
    // Initialize the lower layer as appropriate
    //
    XML_TOKENIZATION_INIT TokenizationInit;

    //
    // If the logical layer is to pick up from a previous state, then
    // this member must be non-NULL.  This is useful for when doing a
    // mini-reparse over a section of the document.
    //
    PXML_TOKENIZATION_STATE PreviousState;

} XML_INIT_LOGICAL_LAYER, *PXML_INIT_LOGICAL_LAYER;
typedef const XML_INIT_LOGICAL_LAYER *PCXML_INIT_LOGICAL_LAYER;


//
// This mini-tokenizer allows you to pick up the logical analysis
// from any arbitrary point in another document (handy for when you
// want to go back and re-read something, like in xmldsig...).  If you
// are cloning an active logical parse, then it's imperative that
// you pass along the same namespace management object.
//
EFI_STATUS
RtlXmlInitializeNextLogicalThing(
    OUT PXML_LOGICAL_STATE pParseState,
    IN PCXML_INIT_LOGICAL_LAYER Init
    );

EFI_STATUS
RtlXmlNextLogicalThing(
    PXML_LOGICAL_STATE pParseState,
    PNS_MANAGER pNamespaceManager,
    PXMLDOC_THING pDocumentPiece,
    PRTL_GROWING_LIST pAttributeList
    );

EFI_STATUS
RtlXmlDestroyNextLogicalThing(
    PXML_LOGICAL_STATE pState
    );

#define RTL_XML_EXTENT_TO_STRING_FLAG_CONVERT_REFERENCES 0x00000001

EFI_STATUS
RtlXmlExtentToString(
    IN UINT32                   ConversionFlags,
    IN_OUT PXML_RAWTOKENIZATION_STATE pParseState,
    IN PCXML_EXTENT           pExtent,
    IN_OUT PUNICODE_STRING       pString,
    OUT UINTN*                 pcbString
    );

#define RTL_XML_EXTENT_TO_UTF8_STRING_FLAG_CONVERT_REFERENCES 0x00000001

EFI_STATUS
RtlXmlExtentToUtf8String(
    IN UINT32 ConversionFlags,
    IN_OUT PXML_RAWTOKENIZATION_STATE pParseState,
    IN PCXML_EXTENT pExtent,
    IN_OUT PLUTF8_STRING pString,
    OUT UINTN* pcbRequired
    );

EFI_STATUS
RtlXmlMatchLogicalElement(
    IN  PXML_TOKENIZATION_STATE     pState,
    IN  PXMLDOC_ELEMENT             pElement,
    IN  PCXML_SIMPLE_STRING         pNamespace,
    IN  PCXML_SIMPLE_STRING         pElementName,
    OUT BOOLEAN*                    pfMatches
    );

EFI_STATUS
RtlXmlFindAttributesInElement(
    IN  PXML_TOKENIZATION_STATE     pState,
    IN  PRTL_GROWING_LIST           pAttributeList,
    IN  UINT32                       ulAttributeCountInElement,
    IN  UINT32                       ulFindCount,
    IN  PCXML_ATTRIBUTE_DEFINITION  pAttributeNames,
    OUT PXMLDOC_ATTRIBUTE          *ppAttributes,
    OUT UINT32*                      pulUnmatchedAttributes
    );

EFI_STATUS
RtlXmlSkipElement(
    PXML_LOGICAL_STATE pState,
    PXMLDOC_ELEMENT TheElement
    );

EFI_STATUS
RtlXmlMatchAttribute(
    IN PXML_TOKENIZATION_STATE      State,
    IN PXMLDOC_ATTRIBUTE            Attribute,
    IN PCXML_SIMPLE_STRING          Namespace,
    IN PCXML_SIMPLE_STRING          AttributeName,
    OUT XML_STRING_COMPARE         *CompareResult
    );

//
// This handy macro exploits a property of the XML parser, that end-elements'
// OpeningElement::Name member contains exactly the contents of the original
// element as parsed, with pointers and other content intact.
//
#define RtlXmlIsEndElementFor(OpenElement, MaybeClose) \
    (RtlEqualMemory(&(MaybeClose)->OpeningElement.Name, &(OpenElement)->Name, sizeof(XML_EXTENT)))


#ifdef __cplusplus
};
#endif

#endif
