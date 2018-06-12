/**
Implementation of the XML structures/fuctions used in parsing.

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
#include <Library/BaseMemoryLib.h>               // SetMem()
#include "fasterxml.h"                           // XML Engine
#include "xmlerr.h"                              // XML Errors.
#include "xmlstructure.h"                        // XML structures

EFI_STATUS
EFIAPI
RtlXmlDestroyNextLogicalThing(
    PXML_LOGICAL_STATE pState
    )
{
    EFI_STATUS status;

    status = RtlDestroyGrowingList(&pState->ElementStack);

    return status;

}


typedef struct _XMLSTRUCTURE_LOOKAHEAD_DATA
{
    XML_TOKENIZATION_SPECIFIC_STATE WantedState;
    LOGICAL_XML_ERROR ErrorIfMissing;
    BOOLEAN fCanSkip;
}
XMLSTRUCTURE_LOOKAHEAD_DATA, *PXMLSTRUCTURE_LOOKAHEAD_DATA;
typedef const XMLSTRUCTURE_LOOKAHEAD_DATA *PCXMLSTRUCTURE_LOOKAHEAD_DATA;

static
EFI_STATUS
EFIAPI
RtlpXmlExpectStates(
    PXML_TOKENIZATION_STATE pParseState,
    UINTN cLookaheads,
    PCXMLSTRUCTURE_LOOKAHEAD_DATA pLookaheads,
    OUT PXML_TOKEN FoundTokens,
    OUT UINTN* cConsumed
    )
{
    EFI_STATUS status = EFI_SUCCESS;
    UINTN c;

    *cConsumed = 0;

    if (FoundTokens != NULL)
    {
        for (c = 0; c != cLookaheads; c++) {

            #pragma prefast(suppress:394, "Prefast doesn't understand about != vs. < in loop invariants")
            status = RtlXmlNextToken(pParseState, &FoundTokens[c], TRUE);
            if (EFI_ERROR(status) || FoundTokens[c].fError) 
            {
                if (FoundTokens[c].fError) {
                    status = EFI_INVALID_PARAMETER;
                }
                goto Exit;
            }

            if (FoundTokens[c].State != pLookaheads[c].WantedState) {
                break;
            }
        }
    }
    else
    {
        XML_TOKEN TempToken;
        for (c = 0; c != cLookaheads; c++) {

            status = RtlXmlNextToken(pParseState, &TempToken, TRUE);
            if (EFI_ERROR(status) || TempToken.fError) {
                if (TempToken.fError) {
                    status = EFI_INVALID_PARAMETER;
                }
                goto Exit;
            }

            if (TempToken.State != pLookaheads[c].WantedState) {
                break;
            }
        }
    }

    *cConsumed = c;

Exit:
    return status;
}

EFI_STATUS
EFIAPI
RtlXmlInitializeNextLogicalThing(
    OUT PXML_LOGICAL_STATE pParseState,
    IN PCXML_INIT_LOGICAL_LAYER Init
    )
{
    EFI_STATUS status;
    UINTN cbEncodingBOM;

    if (pParseState != NULL) {
        ZeroMem(pParseState, sizeof(*pParseState));
    }

    if (Init == NULL)
        return RtlpReportXmlError(EFI_INVALID_PARAMETER);

    if (pParseState == NULL)
        return RtlpReportXmlError(EFI_INVALID_PARAMETER);

    //
    // Reuse a previous state if we must.
    //
    if (Init->PreviousState) {
        status = RtlXmlCloneTokenizationState(
            Init->PreviousState,
            &pParseState->ParseState);
    }
    //
    // Otherwise, initialize from the ground up
    //
    else {
        status = RtlXmlInitializeTokenization(
            &pParseState->ParseState,
            &Init->TokenizationInit);
    }

    if (EFI_ERROR(status)) {
        return status;
    }

    //
    // Set up the rest of our stuff
    //
    status = RtlInitializeGrowingList(
        &pParseState->ElementStack,
        sizeof(XMLDOC_THING),
        40,
        pParseState->InlineElements,
        sizeof(pParseState->InlineElements),
        Init->Allocator);

    if (EFI_ERROR(status)) {
        return status;
    }

    status = RtlXmlDetermineStreamEncoding(&pParseState->ParseState, &cbEncodingBOM);
    if (EFI_ERROR(status))
        return status;

    pParseState->ParseState.RawTokenState.pvCursor =
        (UINT8*)pParseState->ParseState.RawTokenState.pvCursor + cbEncodingBOM;

    return status;
}



static
EFI_STATUS
EFIAPI
_RtlpFixUpNamespaces(
    XML_LOGICAL_STATE   *pState,
    PNS_MANAGER             pNsManager,
    PRTL_GROWING_LIST       pAttributes,
    PXMLDOC_THING           pThing,
    UINT32                   ulDocumentDepth,
    LOGICAL_XML_ERROR      *pLogicalError,
    XML_EXTENT             *pFailingExtent
    )
{
    EFI_STATUS status = EFI_SUCCESS;
    UINT32 ul = 0;
    PXMLDOC_ATTRIBUTE pAttribute = NULL;
    XML_EXTENT FoundNamespace;
    PXML_EXTENT ExtentToTest;

    *pLogicalError = XMLERROR_SUCCESS;
    ZeroMem(pFailingExtent, sizeof(*pFailingExtent));

    //
    // The element itself and the attributes may have namespace prefixes.  If
    // they do, then we should find the matching namespace and set that into the
    // element/attributes presented.
    //
    if (pNsManager == NULL) {
        goto Exit;
    }

    //
    // We can only deal with elements and end-elements
    //
    if ((pThing->ulThingType != XMLDOC_THING_ELEMENT) &&
        (pThing->ulThingType != XMLDOC_THING_END_ELEMENT)) {
        return RtlpReportXmlError(EFI_INVALID_PARAMETER);
    }

    //
    // Check the element itself.  The prefix might be NULL, in which case
    // it should pick up any default namespace active.
    //
    ExtentToTest = (pThing->ulThingType == XMLDOC_THING_ELEMENT)
        ? &(pThing->item.Element.NsPrefix)
        : &(pThing->item.EndElement.NsPrefix);

    status = RtlNsGetNamespaceForAlias(
        pNsManager,
        ulDocumentDepth,
        ExtentToTest,
        &FoundNamespace);

    if (NT_SUCCESS(status)) {
        *ExtentToTest = FoundNamespace;
    }
    else if (status == STATUS_NOT_FOUND) {
        *pLogicalError = XMLERROR_NS_UNKNOWN_PREFIX;
        *pFailingExtent = *ExtentToTest;
        status = RtlpReportXmlError(STATUS_XML_PARSE_ERROR);
        goto Exit;
    }
    else {
        goto Exit;
    }

    if (pAttributes && (pThing->ulThingType == XMLDOC_THING_ELEMENT)) {

        //
        // Now for each element, find the namespace it lives in
        //
        for (ul = 0; ul < pThing->item.Element.ulAttributeCount; ul++) {

            status = RtlIndexIntoGrowingList(
                pAttributes,
                ul,
                (VOID**)&pAttribute,
                FALSE);

            if (EFI_ERROR(status)) {
                goto Exit;
            }

            //
            // No namespace?  Don't look it up, don't look it up, don't look it up ...
            //
            if ((pAttribute->NsPrefix.cbData != 0) &&
                !pAttribute->WasNamespaceDeclaration &&
                !pAttribute->HasXmlPrefix) {

                status = RtlNsGetNamespaceForAlias(
                    pNsManager,
                    ulDocumentDepth,
                    &pAttribute->NsPrefix,
                    &FoundNamespace);

                //
                // Good, mark as the namespace
                //
                if (!EFI_ERROR(status)) {
                    pAttribute->NsPrefix = FoundNamespace;
                }
                else if (status == EFI_NOT_FOUND) {
                    *pLogicalError = XMLERROR_NS_UNKNOWN_PREFIX;
                    *pFailingExtent = pAttribute->NsPrefix;
                    status = RtlpReportXmlError(STATUS_XML_PARSE_ERROR);
                    goto Exit;
                }
                else {
                    goto Exit;
                }
            }
        }
    }

    status = EFI_SUCCESS;

Exit:
    return status;
}


#define ATTEMPT_NEXT(state, worker) do { \
    const EFI_STATUS __status = RtlXmlNextToken((state), (worker), TRUE); \
    if (EFI_ERROR(__status) || (worker)->fError) goto ErrorAndExit; \
} while (0)

//
// Parsing an entity declaration is hard.
//
static
EFI_STATUS
EFIAPI
_ParseAndPackageEntityDecl(
    PXML_LOGICAL_STATE pParseState,
    PXML_TOKEN TokenWorker,
    PXMLDOC_THING DocThing
    )
{

    PXMLDOC_ENTITYDECL pEntityDecl = &DocThing->item.EntityDecl;
    XML_LINE_AND_COLUMN CurrentLocation;
    EFI_STATUS status = EFI_SUCCESS;

    status = RtlXmlGetCurrentLocation(&pParseState->ParseState, &CurrentLocation);
    if (EFI_ERROR(status)) {
        return status;
    }

    //
    // Must have a name, but might have a parameter marker first.
    //
    ATTEMPT_NEXT(&pParseState->ParseState, TokenWorker);

    if (TokenWorker->State == XTSS_DOCTYPE_ENTITYDECL_PARAMETERMARKER) {

        pEntityDecl->EntityType = DOCUMENT_ENTITY_TYPE_PARAMETER;
    }
    else if (TokenWorker->State == XTSS_DOCTYPE_ENTITYDECL_GENERALMARKER) {
        pEntityDecl->EntityType = DOCUMENT_ENTITY_TYPE_GENERAL;
    }
    else {
        DocThing->item.Error.Code = XMLERROR_ENTITYDECL_MISSING_TYPE_INDICATOR;
        goto ErrorAndExit;
    }

    ATTEMPT_NEXT(&pParseState->ParseState, TokenWorker);

    if (TokenWorker->State != XTSS_DOCTYPE_ENTITYDECL_NAME) {
        DocThing->item.Error.Code = XMLERROR_ENTITYDECL_NAME_MALFORMED;
        goto ErrorAndExit;
    }

    pEntityDecl->Name = TokenWorker->Run;


    /*
        Both parameter and general identities follow the same declaration grammar,
        except that only general entities may have NData declarations after an external ID.

        [71] GEDecl ::=  '<!ENTITY' S Name S EntityDef S? '>'
        [72] PEDecl ::=  '<!ENTITY' S '%' S Name S PEDef S? '>'
        [74] PEDef ::=  EntityValue | ExternalID
        [73] EntityDef ::=  EntityValue | (ExternalID NDataDecl?)
        [9]  EntityValue ::=  '"' ([^%&"] | PEReference | Reference)* '"' |  "'" ([^%&'] | PEReference | Reference)* "'"
        [75] ExternalID ::= 'SYSTEM' S SystemLiteral | 'PUBLIC' S PubidLiteral S SystemLiteral

    */
    ATTEMPT_NEXT(&pParseState->ParseState, TokenWorker);

    if (TokenWorker->State == XTSS_DOCTYPE_ENTITYDECL_SYSTEM) {

        static const XMLSTRUCTURE_LOOKAHEAD_DATA SystemLookaheads[] =
        {
            { XTSS_DOCTYPE_ENTITYDECL_SYSTEM_TEXT_OPEN },
            { XTSS_DOCTYPE_ENTITYDECL_SYSTEM_TEXT_VALUE },
            { XTSS_DOCTYPE_ENTITYDECL_SYSTEM_TEXT_CLOSE },
        };

        XML_TOKEN Tokens[RTL_NUMBER_OF(SystemLookaheads)];
        UINTN cTokensMatched;

        status = RtlpXmlExpectStates(&pParseState->ParseState, RTL_NUMBER_OF(SystemLookaheads), SystemLookaheads, Tokens, &cTokensMatched);
        if (EFI_ERROR(status) || (cTokensMatched != RTL_NUMBER_OF(SystemLookaheads))) {
            DocThing->ulThingType = XMLDOC_THING_ERROR;
            DocThing->item.Error.BadExtent = Tokens[cTokensMatched].Run;
            DocThing->item.Error.Code = XMLERROR_ENTITYDECL_SYSTEM_ID_INVALID;
            DocThing->item.Error.Location = CurrentLocation;
            goto Exit;
        }

        pEntityDecl->ValueType = DOCUMENT_ENTITY_VALUE_TYPE_SYSTEM;
        pEntityDecl->SystemId = Tokens[1].Run;

    }
    else if (TokenWorker->State == XTSS_DOCTYPE_ENTITYDECL_PUBLIC) {

        static const XMLSTRUCTURE_LOOKAHEAD_DATA PublicLookaheadTokens[] =
        {
            { XTSS_DOCTYPE_ENTITYDECL_PUBLIC_TEXT_OPEN },
            { XTSS_DOCTYPE_ENTITYDECL_PUBLIC_TEXT_VALUE },
            { XTSS_DOCTYPE_ENTITYDECL_PUBLIC_TEXT_CLOSE },
            { XTSS_DOCTYPE_ENTITYDECL_SYSTEM_TEXT_OPEN },
            { XTSS_DOCTYPE_ENTITYDECL_SYSTEM_TEXT_VALUE },
            { XTSS_DOCTYPE_ENTITYDECL_SYSTEM_TEXT_CLOSE },
        };

        XML_TOKEN Tokens[RTL_NUMBER_OF(PublicLookaheadTokens)];
        UINTN cTokensMatched;

        status = RtlpXmlExpectStates(&pParseState->ParseState, RTL_NUMBER_OF(PublicLookaheadTokens), PublicLookaheadTokens, Tokens, &cTokensMatched);

        if (EFI_ERROR(status) || (cTokensMatched != RTL_NUMBER_OF(PublicLookaheadTokens))) {
            DocThing->ulThingType = XMLDOC_THING_ERROR;
            DocThing->item.Error.BadExtent = Tokens[cTokensMatched].Run;
            DocThing->item.Error.Code = XMLERROR_ENTITYDECL_PUBLIC_ID_INVALID;
            goto Exit;
        }

        pEntityDecl->ValueType = DOCUMENT_ENTITY_VALUE_TYPE_PUBLIC;
        pEntityDecl->PublicId = Tokens[1].Run;
        pEntityDecl->SystemId = Tokens[4].Run;

    }
    else if (TokenWorker->State == XTSS_DOCTYPE_ENTITYDECL_VALUE_OPEN) {

        static const XMLSTRUCTURE_LOOKAHEAD_DATA ValueLookaheads[] =
        {
            { XTSS_DOCTYPE_ENTITYDECL_VALUE_VALUE },
            { XTSS_DOCTYPE_ENTITYDECL_VALUE_CLOSE },
        };

        XML_TOKEN Tokens[RTL_NUMBER_OF(ValueLookaheads)];
        UINTN cTokensMatched;

        status = RtlpXmlExpectStates(&pParseState->ParseState, RTL_NUMBER_OF(ValueLookaheads), ValueLookaheads, Tokens, &cTokensMatched);
        if (EFI_ERROR(status) || (cTokensMatched != RTL_NUMBER_OF(ValueLookaheads))) {
            DocThing->ulThingType = XMLDOC_THING_ERROR;
            DocThing->item.Error.BadExtent = Tokens[cTokensMatched].Run;
            DocThing->item.Error.Code = XMLERROR_ENTITYDECL_VALUE_INVALID;
            DocThing->item.Error.Location = CurrentLocation;
            goto Exit;
        }

        pEntityDecl->ValueType = DOCUMENT_ENTITY_VALUE_TYPE_NORMAL;
        pEntityDecl->NormalValue = Tokens[0].Run;

    }

    //
    // Advance once more - if the state is 'close entity', then that's fine -
    // stop now.  Otherwise, if it's NDATA, then it might be followed by an
    // ndata name, which we need.
    //
    ATTEMPT_NEXT(&pParseState->ParseState, TokenWorker);

    if (TokenWorker->State == XTSS_DOCTYPE_ENTITYDECL_NDATA) {

        static const XMLSTRUCTURE_LOOKAHEAD_DATA NDataLookaheads[] =
        {
            { XTSS_DOCTYPE_ENTITYDECL_NDATA_TEXT},
        };

        XML_TOKEN Tokens[RTL_NUMBER_OF(NDataLookaheads)];
        UINTN cTokensMatched;

        status = RtlpXmlExpectStates(&pParseState->ParseState, RTL_NUMBER_OF(NDataLookaheads), NDataLookaheads, Tokens, &cTokensMatched);
        if (EFI_ERROR(status) || (cTokensMatched != RTL_NUMBER_OF(NDataLookaheads))) {
            DocThing->ulThingType = XMLDOC_THING_ERROR;
            DocThing->item.Error.BadExtent = Tokens[cTokensMatched].Run;
            DocThing->item.Error.Code = XMLERROR_ENTITYDECL_NDATA_INVALID;
            DocThing->item.Error.Location = CurrentLocation;
            goto Exit;
        }

        pEntityDecl->NDataType = Tokens[0].Run;

        ATTEMPT_NEXT(&pParseState->ParseState, TokenWorker);
    }

    //
    // Ok, we either advanced past the ndata, or we were at the close already - make sure
    // this is a "close"
    //
    if (TokenWorker->State != XTSS_DOCTYPE_ENTITYDECL_CLOSE) {
        DocThing->item.Error.Code = XMLERROR_ENTITYDECL_MISSING_CLOSE;
        goto ErrorAndExit;
    }

Exit:
    return status;

ErrorAndExit:
    DocThing->ulThingType = XMLDOC_THING_ERROR;
    DocThing->item.Error.BadExtent = TokenWorker->Run;
    DocThing->item.Error.Location = CurrentLocation;
    goto Exit;

}


EFI_STATUS
EFIAPI
RtlXmlNextLogicalThing(
    PXML_LOGICAL_STATE pParseState,
    PNS_MANAGER pNamespaceManager,
    PXMLDOC_THING pDocumentPiece,
    PRTL_GROWING_LIST pAttributeList
    )
{
    XML_TOKEN TokenWorker;
    EFI_STATUS status;
    XML_LINE_AND_COLUMN CurrentLocation, PrevLocation;

    if (!ARGUMENT_PRESENT(pParseState) ||
        !ARGUMENT_PRESENT(pDocumentPiece)) {
        return RtlpReportXmlError(EFI_INVALID_PARAMETER);
    }

    //
    // If the attribute list is there, it better have slots that are at least this big
    //
    if ((pAttributeList != NULL) && (pAttributeList->cbElementSize < sizeof(XMLDOC_ATTRIBUTE))) {
        return RtlpReportXmlError(EFI_INVALID_PARAMETER);
    }

TryAgain:

    ZeroMem(pDocumentPiece, sizeof(*pDocumentPiece));

    RtlXmlGetCurrentLocation(&pParseState->ParseState, &CurrentLocation);

    status = RtlXmlNextToken(&pParseState->ParseState, &TokenWorker, TRUE);
    if (EFI_ERROR(status) || TokenWorker.fError) {
        if (EFI_ERROR(status))
            status = RtlpReportXmlError(STATUS_XML_PARSE_ERROR);
        return status;
    }

    pDocumentPiece->TotalExtent.pvData = TokenWorker.Run.pvData;
    pDocumentPiece->ulDocumentDepth = pParseState->ulElementStackDepth;

    //
    // The cursor should only be at a few certain points when we're called here.
    //
    switch (TokenWorker.State) {

        //
        // Everything in a document type declaration is mostly discarded, save for
        // entity declarations.  After an entitydecl, the state is "whitespace", so
        // this will properly handle the post-entitydecl operations of eating more of the
        // doctype goop.
        //
    case XTSS_DOCTYPE_WHITESPACE:
    case XTSS_DOCTYPE_OPEN:
    case XTSS_DOCTYPE_MARKUP_WHITESPACE:
    case XTSS_DOCTYPE_MARKUP_CLOSE:
        do {
            status = RtlXmlNextToken(&pParseState->ParseState, &TokenWorker, TRUE);

            if (EFI_ERROR(status) || TokenWorker.fError)
                goto Exit;

            //
            // Package up an entity declaration (eat the name and the value)
            //
            if (TokenWorker.State == XTSS_DOCTYPE_ENTITYDECL_OPEN)
            {
                pDocumentPiece->ulThingType = XMLDOC_THING_ENTITYDECL;
                status = _ParseAndPackageEntityDecl(pParseState, &TokenWorker, pDocumentPiece);
                if (EFI_ERROR(status) || TokenWorker.fError)
                    goto Exit;

                break;
            }

            if (TokenWorker.State == XTSS_DOCTYPE_ATTLISTDECL_OPEN)
            {
                pDocumentPiece->ulThingType = XMLDOC_THING_ATTLIST;

                do
                {
                    status = RtlXmlNextToken(&pParseState->ParseState, &TokenWorker, TRUE);
                    if (EFI_ERROR(status) || TokenWorker.fError)
                        goto Exit;
                }
                while (TokenWorker.State == XTSS_DOCTYPE_ATTLISTDECL_OPEN);

                // Expect to see the element name after the <!ATTLIST
                if (TokenWorker.State == XTSS_DOCTYPE_ATTLISTDECL_ELEMENT_NAME)
                {
                    pDocumentPiece->item.Attlist.NamespacePrefix.cbData = 0;
                    pDocumentPiece->item.Attlist.NamespacePrefix.ulCharacters = 0;
                    pDocumentPiece->item.Attlist.ElementName = TokenWorker.Run;
                }
                else if (TokenWorker.State == XTSS_DOCTYPE_ATTLISTDECL_ELEMENT_PREFIX)
                {
                    pDocumentPiece->item.Attlist.NamespacePrefix = TokenWorker.Run;

                    // Get the colon
                    status = RtlXmlNextToken(&pParseState->ParseState, &TokenWorker, TRUE);
                    if (EFI_ERROR(status) || TokenWorker.fError)
                        goto Exit;

                    if (TokenWorker.State != XTSS_DOCTYPE_ATTLISTDECL_ELEMENT_COLON)
                    {
                        status = RtlpReportXmlError(STATUS_XML_PARSE_ERROR);
                        goto Exit;
                    }

                    // Get the local name
                    status = RtlXmlNextToken(&pParseState->ParseState, &TokenWorker, TRUE);
                    if (EFI_ERROR(status) || TokenWorker.fError)
                        goto Exit;

                    if (TokenWorker.State != XTSS_DOCTYPE_ATTLISTDECL_ELEMENT_NAME)
                    {
                        status = RtlpReportXmlError(STATUS_XML_PARSE_ERROR);
                        goto Exit;
                    }

                    pDocumentPiece->item.Attlist.ElementName = TokenWorker.Run;
                }
                else
                {
                    status = RtlpReportXmlError(STATUS_XML_PARSE_ERROR);
                    goto Exit;
                }

                break;
            }
        }
        while (TokenWorker.State != XTSS_DOCTYPE_CLOSE);

        if (pDocumentPiece->ulThingType != XMLDOC_THING_ENTITYDECL &&
            pDocumentPiece->ulThingType != XMLDOC_THING_ATTLIST)
        {
            goto TryAgain;
        }

        break;

    case XTSS_DOCTYPE_ATTLISTDECL_ELEMENT_NAME:
    case XTSS_DOCTYPE_ATTLISTDECL_WHITESPACE:

        // Expect to see either an attribute definition (possibly
        // preceded by whitespace) or the end of the attribute list
        do
        {
            status = RtlXmlNextToken(&pParseState->ParseState, &TokenWorker, TRUE);
            if (EFI_ERROR(status) || TokenWorker.fError)
                goto Exit;
        }
        while (TokenWorker.State == XTSS_DOCTYPE_ATTLISTDECL_WHITESPACE);

        // Are we at the end of the ATTLIST?
        if (TokenWorker.State == XTSS_DOCTYPE_ATTLISTDECL_CLOSE)
        {
            pDocumentPiece->ulThingType = XMLDOC_THING_END_ATTLIST;
            break;
        }

        pDocumentPiece->item.Attdef.NamespacePrefix.ulCharacters = 0;
        pDocumentPiece->item.Attdef.NamespacePrefix.cbData = 0;

        if (TokenWorker.State == XTSS_DOCTYPE_ATTLISTDECL_ATTPREFIX)
        {
            pDocumentPiece->item.Attdef.NamespacePrefix = TokenWorker.Run;

            // Get the colon
            status = RtlXmlNextToken(&pParseState->ParseState, &TokenWorker, TRUE);
            if (EFI_ERROR(status) || TokenWorker.fError)
                goto Exit;

            if (TokenWorker.State != XTSS_DOCTYPE_ATTLISTDECL_ATTCOLON)
            {
                status = RtlpReportXmlError(STATUS_XML_PARSE_ERROR);
                goto Exit;
            }

            // Get the local name
            status = RtlXmlNextToken(&pParseState->ParseState, &TokenWorker, TRUE);
            if (EFI_ERROR(status) || TokenWorker.fError)
                goto Exit;
        }

        if (TokenWorker.State != XTSS_DOCTYPE_ATTLISTDECL_ATTNAME)
        {
            status = RtlpReportXmlError(STATUS_XML_PARSE_ERROR);
            goto Exit;
        }

        pDocumentPiece->ulThingType = XMLDOC_THING_ATTDEF;
        pDocumentPiece->item.Attdef.AttributeName = TokenWorker.Run;

        // Now get the attribute type
        status = RtlXmlNextToken(&pParseState->ParseState, &TokenWorker, TRUE);
        if (EFI_ERROR(status) || TokenWorker.fError)
            goto Exit;

        switch (TokenWorker.State)
        {
        case XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_CDATA:
            pDocumentPiece->item.Attdef.AttributeType = DOCUMENT_ATTDEF_TYPE_CDATA;
            break;

        case XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_ID:
            pDocumentPiece->item.Attdef.AttributeType = DOCUMENT_ATTDEF_TYPE_ID;
            break;

        case XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_IDREF:
            pDocumentPiece->item.Attdef.AttributeType = DOCUMENT_ATTDEF_TYPE_IDREF;
            break;

        case XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_IDREFS:
            pDocumentPiece->item.Attdef.AttributeType = DOCUMENT_ATTDEF_TYPE_IDREFS;
            break;

        case XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_ENTITY:
            pDocumentPiece->item.Attdef.AttributeType = DOCUMENT_ATTDEF_TYPE_ENTITY;
            break;

        case XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_ENTITIES:
            pDocumentPiece->item.Attdef.AttributeType = DOCUMENT_ATTDEF_TYPE_ENTITIES;
            break;

        case XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_NMTOKEN:
            pDocumentPiece->item.Attdef.AttributeType = DOCUMENT_ATTDEF_TYPE_NMTOKEN;
            break;

        case XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_NMTOKENS:
            pDocumentPiece->item.Attdef.AttributeType = DOCUMENT_ATTDEF_TYPE_NMTOKENS;
            break;

        case XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_NOTATION:
            do
            {
                status = RtlXmlNextToken(&pParseState->ParseState, &TokenWorker, TRUE);
                if (EFI_ERROR(status) || TokenWorker.fError)
                    goto Exit;
            }
            while (TokenWorker.State != XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_ENUMERATED_VALUE);

            pDocumentPiece->item.Attdef.AttributeType = DOCUMENT_ATTDEF_TYPE_ENUMERATED_NOTATION;
            pDocumentPiece->item.Attdef.EnumeratedType = TokenWorker.Run;

            // Get the close paren
            status = RtlXmlNextToken(&pParseState->ParseState, &TokenWorker, TRUE);
            if (EFI_ERROR(status) || TokenWorker.fError)
                goto Exit;

            break;

        case XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_ENUMERATED_OPEN:
            // Get the enumeration
            status = RtlXmlNextToken(&pParseState->ParseState, &TokenWorker, TRUE);
            if (EFI_ERROR(status) || TokenWorker.fError)
                goto Exit;

            if (TokenWorker.State != XTSS_DOCTYPE_ATTLISTDECL_ATTTYPE_ENUMERATED_VALUE)
            {
                status = RtlpReportXmlError(STATUS_XML_PARSE_ERROR);
                goto Exit;
            }

            pDocumentPiece->item.Attdef.AttributeType = DOCUMENT_ATTDEF_TYPE_ENUMERATED;
            pDocumentPiece->item.Attdef.EnumeratedType = TokenWorker.Run;

            // Get the close paren
            status = RtlXmlNextToken(&pParseState->ParseState, &TokenWorker, TRUE);
            if (EFI_ERROR(status) || TokenWorker.fError)
                goto Exit;

            break;

        default:
            status = RtlpReportXmlError(STATUS_XML_PARSE_ERROR);
            goto Exit;
        }

        // Now get the default value declaration
        status = RtlXmlNextToken(&pParseState->ParseState, &TokenWorker, TRUE);
        if (EFI_ERROR(status) || TokenWorker.fError)
            goto Exit;

        pDocumentPiece->item.Attdef.DefaultDeclType = DOCUMENT_ATTDEF_DEFAULTDECL_TYPE_NONE;
        switch (TokenWorker.State)
        {
        case XTSS_DOCTYPE_ATTLISTDECL_DEFAULT_REQUIRED:
            pDocumentPiece->item.Attdef.DefaultDeclType = DOCUMENT_ATTDEF_DEFAULTDECL_TYPE_REQUIRED;
            break;

        case XTSS_DOCTYPE_ATTLISTDECL_DEFAULT_IMPLIED:
            pDocumentPiece->item.Attdef.DefaultDeclType = DOCUMENT_ATTDEF_DEFAULTDECL_TYPE_IMPLIED;
            break;

        case XTSS_DOCTYPE_ATTLISTDECL_DEFAULT_FIXED:
            pDocumentPiece->item.Attdef.DefaultDeclType = DOCUMENT_ATTDEF_DEFAULTDECL_TYPE_FIXED;

            // Skip forward to the default value
            status = RtlXmlNextToken(&pParseState->ParseState, &TokenWorker, TRUE);
            if (EFI_ERROR(status) || TokenWorker.fError)
                goto Exit;

            // Fallthrough

        case XTSS_DOCTYPE_ATTLISTDECL_DEFAULT_TEXT_OPEN:
            status = RtlXmlNextToken(&pParseState->ParseState, &TokenWorker, TRUE);
            if (EFI_ERROR(status) || TokenWorker.fError)
                goto Exit;

            if (TokenWorker.State != XTSS_DOCTYPE_ATTLISTDECL_DEFAULT_TEXT_VALUE)
            {
                status = RtlpReportXmlError(STATUS_XML_PARSE_ERROR);
                goto Exit;
            }

            pDocumentPiece->item.Attdef.DefaultValue = TokenWorker.Run;

            // Get the close quote
            status = RtlXmlNextToken(&pParseState->ParseState, &TokenWorker, TRUE);
            if (EFI_ERROR(status) || TokenWorker.fError)
                goto Exit;

            break;

        default:
            status = RtlpReportXmlError(STATUS_XML_PARSE_ERROR);
            goto Exit;
        }

        break;

    case XTSS_DOCTYPE_ATTLISTDECL_CLOSE:

        pDocumentPiece->ulThingType = XMLDOC_THING_END_ATTLIST;
        break;


        //
        // Package up a comment.  These can appear before the root element, no need
        // to check for the document element being opened.
        //
    case XTSS_COMMENT_OPEN:
        {

            static const XMLSTRUCTURE_LOOKAHEAD_DATA Lookaheads[] =
            {
                { XTSS_COMMENT_COMMENTARY, XMLERROR_COMMENT_MALFORMED },
                { XTSS_COMMENT_CLOSE, XMLERROR_COMMENT_MALFORMED },
            };

            XML_TOKEN ParsedBits[RTL_NUMBER_OF(Lookaheads)];
            UINTN cParsed;

            status = RtlpXmlExpectStates(
                &pParseState->ParseState,
                RTL_NUMBER_OF(Lookaheads),
                Lookaheads,
                ParsedBits,
                &cParsed
                );

            if (EFI_ERROR(status) || (cParsed != RTL_NUMBER_OF(Lookaheads)))
            {
                pDocumentPiece->ulThingType = XMLDOC_THING_ERROR;
                pDocumentPiece->item.Error.BadExtent = ParsedBits[cParsed].Run;
                pDocumentPiece->item.Error.Code = Lookaheads[cParsed].ErrorIfMissing;
                pDocumentPiece->item.Error.Location = CurrentLocation;
                goto Exit;
            }
            else
            {
                pDocumentPiece->ulThingType = XMLDOC_THING_COMMENT;
                pDocumentPiece->item.Comment.Content = ParsedBits[0].Run;
                pDocumentPiece->item.Comment.Location = CurrentLocation;
            }
        }
        break;


    case XTSS_STREAM_HYPERSPACE:

        //
        // Can't have non-whitespace after the close of a root element according to [1] and [27].
        //
        if (pParseState->fFirstElementFound && (pParseState->ulElementStackDepth == 0))
        {
            BOOLEAN fIsWhitespace;
            status = RtlXmlIsExtentWhitespace(&pParseState->ParseState.RawTokenState, &TokenWorker.Run, &fIsWhitespace);
            if (EFI_ERROR(status))
                goto Exit;

            if (!fIsWhitespace) {
                pDocumentPiece->item.Error.Location = CurrentLocation;
                pDocumentPiece->ulThingType = XMLDOC_THING_ERROR;
                pDocumentPiece->item.Error.BadExtent = TokenWorker.Run;
                pDocumentPiece->item.Error.Code = XMLERROR_INVALID_POST_ROOT_ELEMENT_CONTENT;
                status = EFI_SUCCESS;
                goto Exit;
            }
        }

        pDocumentPiece->ulThingType = XMLDOC_THING_HYPERSPACE;
        pDocumentPiece->item.PCDATA.Content = TokenWorker.Run;
        pDocumentPiece->item.PCDATA.Location = CurrentLocation;

        break;

        //
        // CDATA is just returned as-is
        //
    case XTSS_CDATA_OPEN:

        if (pParseState->fFirstElementFound && (pParseState->ulElementStackDepth == 0)) {
            pDocumentPiece->item.Error.Location = CurrentLocation;
            pDocumentPiece->ulThingType = XMLDOC_THING_ERROR;
            pDocumentPiece->item.Error.BadExtent = TokenWorker.Run;
            pDocumentPiece->item.Error.Code = XMLERROR_INVALID_POST_ROOT_ELEMENT_CONTENT;
            status = EFI_SUCCESS;
            goto Exit;
        }
        else  {

            static const XMLSTRUCTURE_LOOKAHEAD_DATA Lookaheads[] =
            {
                { XTSS_CDATA_CDATA, XMLERROR_CDATA_MALFORMED },
                { XTSS_CDATA_CLOSE, XMLERROR_CDATA_MALFORMED },
            };

            XML_TOKEN ParsedBits[RTL_NUMBER_OF(Lookaheads)];
            UINTN cParsed;

            status = RtlpXmlExpectStates(
                &pParseState->ParseState,
                RTL_NUMBER_OF(Lookaheads),
                Lookaheads,
                ParsedBits,
                &cParsed
                );

            if (EFI_ERROR(status) || (cParsed != RTL_NUMBER_OF(Lookaheads)))
            {
                pDocumentPiece->ulThingType = XMLDOC_THING_ERROR;
                pDocumentPiece->item.Error.BadExtent = ParsedBits[cParsed].Run;
                pDocumentPiece->item.Error.Code = Lookaheads[cParsed].ErrorIfMissing;
                pDocumentPiece->item.Error.Location = CurrentLocation;
                goto Exit;
            }
            else
            {
                pDocumentPiece->ulThingType = XMLDOC_THING_CDATA;
                pDocumentPiece->item.CDATA.Content = ParsedBits[0].Run;
                pDocumentPiece->item.CDATA.Location = CurrentLocation;
            }

        }
        break;



        //
        // Starting an xmldecl
        //
    case XTSS_XMLDECL_OPEN:
        {
            PXML_EXTENT pTargetExtent = NULL;

            if (pParseState->fFirstElementFound) {
                pDocumentPiece->ulThingType = XMLDOC_THING_ERROR;
                pDocumentPiece->item.Error.Code = XMLERROR_XMLDECL_NOT_FIRST_THING;
                pDocumentPiece->item.Error.BadExtent = TokenWorker.Run;
                pDocumentPiece->item.Error.Location = CurrentLocation;
                goto Exit;
            }

            pDocumentPiece->ulThingType = XMLDOC_THING_XMLDECL;
            pDocumentPiece->item.XmlDecl.Location = CurrentLocation;

            do {
                status = RtlXmlNextToken(&pParseState->ParseState, &TokenWorker, TRUE);
                if (EFI_ERROR(status)) {
                    return status;
                }

                if (TokenWorker.fError ||
                    (TokenWorker.State == XTSS_STREAM_END) ||
                    (TokenWorker.State == XTSS_XMLDECL_CLOSE) ||
                    (TokenWorker.State == XTSS_ERRONEOUS)) {

                    break;
                }

                switch (TokenWorker.State) {
                case XTSS_XMLDECL_VERSION:
                    pTargetExtent = &pDocumentPiece->item.XmlDecl.Version;
                    break;

                case XTSS_XMLDECL_STANDALONE:
                    pTargetExtent = &pDocumentPiece->item.XmlDecl.Standalone;
                    break;

                case XTSS_XMLDECL_ENCODING:
                    pTargetExtent = &pDocumentPiece->item.XmlDecl.Encoding;
                    break;

                    //
                    // Put the value where it's supposed to go.  Don't do this
                    // if we don't have a target extent known.  Silently ignore
                    // (this maybe should be an error?  I think the lower-level
                    // tokenizer knows about this) unknown xmldecl instructions.
                    //
                case XTSS_XMLDECL_VALUE:
                    if (pTargetExtent) {
                        *pTargetExtent = TokenWorker.Run;
                        pTargetExtent = NULL;
                    }
                    break;
                }
            }
            while (TRUE);

            //
            // We stopped for some other reason
            //
            if (TokenWorker.State != XTSS_XMLDECL_CLOSE) {
                pDocumentPiece->ulThingType = XMLDOC_THING_ERROR;
                pDocumentPiece->item.Error.BadExtent = TokenWorker.Run;
                pDocumentPiece->item.Error.Code = XMLERROR_XMLDECL_INVALID_FORMAT;
                pDocumentPiece->item.Error.Location = CurrentLocation;
            }

        }
        break;



        //
        // A processing instruction was found.  Record it in the returned blibbet
        //
    case XTSS_PI_OPEN:
        {
            //
            // Acquire the following token
            //
            status = RtlXmlNextToken(&pParseState->ParseState, &TokenWorker, TRUE);
            if (EFI_ERROR(status) || TokenWorker.fError || (TokenWorker.State != XTSS_PI_TARGET)) {
                pDocumentPiece->ulThingType = XMLDOC_THING_ERROR;
                pDocumentPiece->item.Error.Code = XMLERROR_PI_TARGET_NOT_FOUND;
                pDocumentPiece->item.Error.BadExtent = TokenWorker.Run;
                pDocumentPiece->item.Error.Location = CurrentLocation;
                goto Exit;
            }

            //
            // At this point, it's a processing instruction.  Record the target name
            // and mark up the return structure
            //
            pDocumentPiece->ulThingType = XMLDOC_THING_PROCESSINGINSTRUCTION;
            pDocumentPiece->item.ProcessingInstruction.Target = TokenWorker.Run;
            pDocumentPiece->item.ProcessingInstruction.Location = CurrentLocation;

            //
            // Look for all the PI stuff ... if you find the end before finding the
            // value, that's fine.  Otherwise, mark the value as being 'the value'
            //
            do {

                status = RtlXmlNextToken(&pParseState->ParseState, &TokenWorker, TRUE);
                if (EFI_ERROR(status) || TokenWorker.fError) {
                    pDocumentPiece->ulThingType = XMLDOC_THING_ERROR;
                    pDocumentPiece->item.Error.Code = XMLERROR_PI_CONTENT_ERROR;
                    pDocumentPiece->item.Error.BadExtent = TokenWorker.Run;
                    pDocumentPiece->item.Error.Location = CurrentLocation;
                    goto Exit;
                }

                if (TokenWorker.State == XTSS_PI_VALUE) {
                    pDocumentPiece->item.ProcessingInstruction.Instruction = TokenWorker.Run;
                }
                //
                // Found the end of the PI
                //
                else if (TokenWorker.State == XTSS_PI_CLOSE) {
                    break;
                }
                //
                // Found end of stream instead?
                //
                else if (TokenWorker.State == XTSS_STREAM_END) {
                    pDocumentPiece->ulThingType = XMLDOC_THING_ERROR;
                    pDocumentPiece->item.Error.Code = XMLERROR_PI_EOF_BEFORE_CLOSE;
                    pDocumentPiece->item.Error.BadExtent = TokenWorker.Run;
                    pDocumentPiece->item.Error.Location = CurrentLocation;
                    break;
                }
            }
            while (TRUE);

        }
        break;

        //
        // We're starting an element.  Gather together all the attributes for the
        // element.
        //
    case XTSS_ELEMENT_OPEN:

        if (pParseState->fFirstElementFound && (pParseState->ulElementStackDepth == 0)) {
            pDocumentPiece->ulThingType = XMLDOC_THING_ERROR;
            pDocumentPiece->item.Error.BadExtent = TokenWorker.Run;
            pDocumentPiece->item.Error.Code = XMLERROR_INVALID_POST_ROOT_ELEMENT_CONTENT;
            pDocumentPiece->item.Error.Location = CurrentLocation;
            status = EFI_SUCCESS;
            goto Exit;
        }
        else {

            PXMLDOC_ATTRIBUTE pElementAttribute = NULL;
            PXMLDOC_THING pStackElement = NULL;

            if (!pParseState->fFirstElementFound)
                pParseState->fFirstElementFound = TRUE;

            //
            // See what the first token after the open part is.  If it's a namespace
            // prefix, or a name, then we can deal with it.  Otherwise, it's a problem
            // for us.
            //
            status = RtlXmlNextToken(&pParseState->ParseState, &TokenWorker, TRUE);
            if (EFI_ERROR(status) || TokenWorker.fError ||
                ((TokenWorker.State != XTSS_ELEMENT_NAME) &&
                (TokenWorker.State != XTSS_ELEMENT_NAME_NS_PREFIX ))) {

                pDocumentPiece->ulThingType = XMLDOC_THING_ERROR;
                pDocumentPiece->item.Error.Code = XMLERROR_ELEMENT_NAME_NOT_FOUND;
                pDocumentPiece->item.Error.BadExtent = TokenWorker.Run;
                pDocumentPiece->item.Error.Location = CurrentLocation;
                goto Exit;
            }

            pDocumentPiece->ulThingType = XMLDOC_THING_ELEMENT;
            pDocumentPiece->item.Element.Location = CurrentLocation;

            //
            // If this was a namespace prefix, save it off and skip the colon afterwards
            // to position TokenWorker at the name of the element itself
            //
            if (TokenWorker.State == XTSS_ELEMENT_NAME_NS_PREFIX) {

                pDocumentPiece->item.Element.NsPrefix = TokenWorker.Run;
                pDocumentPiece->item.Element.OriginalNsPrefix = TokenWorker.Run;

                //
                // Consume the colon
                //
                status = RtlXmlNextToken(&pParseState->ParseState, &TokenWorker, TRUE);
                if (EFI_ERROR(status) ||
                    TokenWorker.fError ||
                    (TokenWorker.State != XTSS_ELEMENT_NAME_NS_COLON)) {

                    pDocumentPiece->ulThingType = XMLDOC_THING_ERROR;
                    pDocumentPiece->item.Error.Code = XMLERROR_ELEMENT_NS_PREFIX_MISSING_COLON;
                    pDocumentPiece->item.Error.BadExtent = TokenWorker.Run;
                    pDocumentPiece->item.Error.Location = CurrentLocation;
                    goto Exit;
                }

                //
                // Fill TokenWorker with the name part
                //
                status = RtlXmlNextToken(&pParseState->ParseState, &TokenWorker, TRUE);
                if (EFI_ERROR(status) ||
                    TokenWorker.fError ||
                    (TokenWorker.State != XTSS_ELEMENT_NAME)) {

                    pDocumentPiece->ulThingType = XMLDOC_THING_ERROR;
                    pDocumentPiece->item.Error.Code = XMLERROR_ELEMENT_NAME_NOT_FOUND;
                    pDocumentPiece->item.Error.BadExtent = TokenWorker.Run;
                    pDocumentPiece->item.Error.Location = CurrentLocation;
                    goto Exit;
                }
            }

            //
            // Great, we found the name of this element.
            //
            pDocumentPiece->item.Element.Name = TokenWorker.Run;
            pDocumentPiece->item.Element.ulAttributeCount = 0;

            //
            // Now let's go finding name/value pairs (yippee!)
            //

            //
            // Temporarily store the current location because it's gonna get changed in the loop below.
            //
            PrevLocation = CurrentLocation;
            do {
                if (NT_SUCCESS(status))
                {
                    if (NT_SUCCESS(status = RtlXmlNextToken(&pParseState->ParseState, &TokenWorker, FALSE)))
                    {
                        RtlXmlGetCurrentLocation(&pParseState->ParseState, &CurrentLocation);
                        status = RtlXmlAdvanceTokenization(&pParseState->ParseState, &TokenWorker);
                    }
                }

                //
                // If we found a close of this element tag, then quit.
                //
                if ((TokenWorker.State == XTSS_ELEMENT_CLOSE) ||
                    (TokenWorker.State == XTSS_ELEMENT_CLOSE_EMPTY) ||
                    (TokenWorker.State == XTSS_STREAM_END) ||
                    TokenWorker.fError ||
                    EFI_ERROR(status)) {
                    break;
                }

                switch (TokenWorker.State) {

                    //
                    // Found just <foo xmlns="..."> - gather the equals and the value.
                    //
                case XTSS_ELEMENT_XMLNS_DEFAULT:
                    {
                        static const XMLSTRUCTURE_LOOKAHEAD_DATA Lookaheads[] =
                        {
                            { XTSS_ELEMENT_XMLNS_EQUALS, XMLERROR_NS_DECL_MISSING_EQUALS },
                            { XTSS_ELEMENT_XMLNS_VALUE_OPEN, XMLERROR_NS_DECL_MISSING_QUOTE },
                            { XTSS_ELEMENT_XMLNS_VALUE, XMLERROR_NS_DECL_MISSING_VALUE },
                            { XTSS_ELEMENT_XMLNS_VALUE_CLOSE, XMLERROR_NS_DECL_MISSING_QUOTE },
                        };

                        UINTN cLookaheadsFound;
                        XML_TOKEN TempTokens[RTL_NUMBER_OF(Lookaheads)];

                        //
                        // Expect these four states in a row - even if the value is empty, we'll
                        // get an empty value back.
                        //
                        status = RtlpXmlExpectStates(
                            &pParseState->ParseState,
                            RTL_NUMBER_OF(Lookaheads),
                            Lookaheads,
                            TempTokens,
                            &cLookaheadsFound
                            );

                        if (EFI_ERROR(status))
                        {
                            pDocumentPiece->ulThingType = XMLDOC_THING_ERROR;
                            pDocumentPiece->item.Error.Location = CurrentLocation;
                            pDocumentPiece->item.Error.BadExtent = TempTokens[cLookaheadsFound].Run;
                            pDocumentPiece->item.Error.Code = XMLERROR_NS_DECL_GENERAL_FAILURE;
                            goto Exit;
                        }
                        else if (cLookaheadsFound != RTL_NUMBER_OF(Lookaheads))
                        {
                            pDocumentPiece->ulThingType = XMLDOC_THING_ERROR;
                            pDocumentPiece->item.Error.Location = CurrentLocation;
                            pDocumentPiece->item.Error.BadExtent = TempTokens[cLookaheadsFound].Run;
                            pDocumentPiece->item.Error.Code = Lookaheads[cLookaheadsFound].ErrorIfMissing;
                            goto Exit;
                        }

                        //
                        // Ok, the second token gathered is the value.
                        //
                        if (pNamespaceManager != NULL) {

                            status = RtlNsInsertDefaultNamespace(
                                pNamespaceManager,
                                pDocumentPiece->ulDocumentDepth + 1,
                                &TempTokens[2].Run);

                            if (status == STATUS_DUPLICATE_NAME) {

                                pDocumentPiece->ulThingType = XMLDOC_THING_ERROR;
                                pDocumentPiece->item.Error.Location = CurrentLocation;
                                pDocumentPiece->item.Error.BadExtent = TempTokens[2].Run;
                                pDocumentPiece->item.Error.Code = XMLERROR_NS_DECL_DEFAULT_REDEFINITION;
                                status = EFI_SUCCESS;
                                goto Exit;

                            }
                            else if (EFI_ERROR(status)) {
                                return status;
                            }
                        }

                        //
                        // And add it as an attribute in the output list of
                        // attributes.
                        //
                        if (pAttributeList != NULL) {

                            status = RtlIndexIntoGrowingList(
                                pAttributeList,
                                pDocumentPiece->item.Element.ulAttributeCount,
                                (VOID**)&pElementAttribute,
                                TRUE);

                            if (EFI_ERROR(status)) {
                                return status;
                            }

                            //
                            // The There's no namespace and no value, just the
                            // prefix name and the namespace value.
                            //
                            ZeroMem(pElementAttribute, sizeof(*pElementAttribute));
                            pElementAttribute->WasNamespaceDeclaration = TRUE;
                            pElementAttribute->HasXmlPrefix = FALSE;
                            pElementAttribute->Name = TokenWorker.Run;
                            pElementAttribute->Value = TempTokens[2].Run;
                            pElementAttribute->Location = CurrentLocation;
                        }

                        pDocumentPiece->item.Element.ulAttributeCount++;
                    }
                    break;

                    //
                    // Found the xmlns part of xmlns:foo='bar', create a new
                    // xml namespace declaration entry
                    //
                case XTSS_ELEMENT_XMLNS:
                    {
                        const XML_EXTENT XmlNsPrefix = TokenWorker.Run;

                        static const XMLSTRUCTURE_LOOKAHEAD_DATA Lookaheads[] =
                        {
                            { XTSS_ELEMENT_XMLNS_COLON },
                            { XTSS_ELEMENT_XMLNS_ALIAS },
                            { XTSS_ELEMENT_XMLNS_EQUALS },
                            { XTSS_ELEMENT_XMLNS_VALUE_OPEN },
                            { XTSS_ELEMENT_XMLNS_VALUE },
                            { XTSS_ELEMENT_XMLNS_VALUE_CLOSE },
                        };

                        UINTN cGathered;
                        XML_TOKEN TempTokens[RTL_NUMBER_OF(Lookaheads)];

                        status = RtlpXmlExpectStates(
                            &pParseState->ParseState,
                            RTL_NUMBER_OF(Lookaheads),
                            Lookaheads,
                            TempTokens,
                            &cGathered
                            );

                        if (EFI_ERROR(status))
                        {
                            pDocumentPiece->ulThingType = XMLDOC_THING_ERROR;
                            pDocumentPiece->item.Error.Code = XMLERROR_NS_DECL_GENERAL_FAILURE;
                            goto Exit;
                        }
                        else if (cGathered != RTL_NUMBER_OF(Lookaheads))
                        {
                            pDocumentPiece->ulThingType = XMLDOC_THING_ERROR;
                            pDocumentPiece->item.Error.Location = CurrentLocation;
                            pDocumentPiece->item.Error.BadExtent = TempTokens[cGathered].Run;
                            pDocumentPiece->item.Error.Code = Lookaheads[cGathered].ErrorIfMissing;
                            goto Exit;
                        }

                        //
                        // If we had a namespace manager, insert this new declaration into it.
                        //
                        if (pNamespaceManager != NULL) {

                            status = RtlNsInsertNamespaceAlias(
                                pNamespaceManager,
                                pDocumentPiece->ulDocumentDepth + 1,
                                &TempTokens[4].Run,
                                &TempTokens[1].Run
                                );

                            if (status == STATUS_DUPLICATE_NAME) {

                                pDocumentPiece->ulThingType = XMLDOC_THING_ERROR;
                                pDocumentPiece->item.Error.Code = XMLERROR_NS_DECL_PREFIX_REDEFINITION;
                                pDocumentPiece->item.Error.BadExtent = TempTokens[1].Run;
                                pDocumentPiece->item.Error.Location = CurrentLocation;
                                status = EFI_SUCCESS;
                                goto Exit;
                            }
                            else if (EFI_ERROR(status)) {

                                return status;
                            }
                        }

                        //
                        // Now add this to the list of attributes, because it really is
                        // an attribute.
                        //
                        if (pAttributeList != NULL) {

                            status = RtlIndexIntoGrowingList(
                                pAttributeList,
                                pDocumentPiece->item.Element.ulAttributeCount,
                                (VOID**)&pElementAttribute,
                                TRUE
                                );

                            if (EFI_ERROR(status)) {
                                return status;
                            }

                            ZeroMem(pElementAttribute, sizeof(*pElementAttribute));
                            pElementAttribute->Name = TempTokens[1].Run;
                            pElementAttribute->OriginalNsPrefix = XmlNsPrefix;
                            pElementAttribute->Value = TempTokens[4].Run;
                            pElementAttribute->WasNamespaceDeclaration = TRUE;
                            pElementAttribute->HasXmlPrefix = FALSE;
                            pElementAttribute->Location = CurrentLocation;
                        }

                        pDocumentPiece->item.Element.ulAttributeCount++;
                    }
                    break;

                    //
                    // A new namespace prefix on an attribute should be easily
                    // recognizable
                    //
                case XTSS_ELEMENT_ATTRIBUTE_NAME_NS_PREFIX:
                    {
                        static const XMLSTRUCTURE_LOOKAHEAD_DATA Lookaheads[] =
                        {
                            { XTSS_ELEMENT_ATTRIBUTE_NAME_NS_COLON },
                            { XTSS_ELEMENT_ATTRIBUTE_NAME },
                            { XTSS_ELEMENT_ATTRIBUTE_EQUALS },
                            { XTSS_ELEMENT_ATTRIBUTE_OPEN },
                            { XTSS_ELEMENT_ATTRIBUTE_VALUE },
                            { XTSS_ELEMENT_ATTRIBUTE_CLOSE }
                        };

                        UINTN cGathered;
                        XML_TOKEN TempTokens[RTL_NUMBER_OF(Lookaheads)];

                        status = RtlpXmlExpectStates(
                            &pParseState->ParseState,
                            RTL_NUMBER_OF(Lookaheads),
                            Lookaheads,
                            TempTokens,
                            &cGathered
                            );

                        if (EFI_ERROR(status))
                        {
                            pDocumentPiece->ulThingType = XMLDOC_THING_ERROR;
                            pDocumentPiece->item.Error.Location = CurrentLocation;
                            pDocumentPiece->item.Error.BadExtent = TempTokens[cGathered].Run;
                            pDocumentPiece->item.Error.Code = XMLERROR_NS_DECL_GENERAL_FAILURE;
                            goto Exit;
                        }
                        else if (cGathered != RTL_NUMBER_OF(Lookaheads))
                        {
                            pDocumentPiece->ulThingType = XMLDOC_THING_ERROR;
                            pDocumentPiece->item.Error.Location = CurrentLocation;
                            pDocumentPiece->item.Error.BadExtent = TempTokens[cGathered].Run;
                            pDocumentPiece->item.Error.Code = Lookaheads[cGathered].ErrorIfMissing;
                            goto Exit;
                        }

                        if (pAttributeList != NULL) {

                            status = RtlIndexIntoGrowingList(
                                pAttributeList,
                                pDocumentPiece->item.Element.ulAttributeCount,
                                (VOID**)&pElementAttribute,
                                TRUE
                                );

                            if (EFI_ERROR(status)) {
                                return status;
                            }

                            pElementAttribute->Name = TempTokens[1].Run;
                            pElementAttribute->NsPrefix = TokenWorker.Run;
                            pElementAttribute->OriginalNsPrefix = TokenWorker.Run;
                            pElementAttribute->Value = TempTokens[4].Run;
                            pElementAttribute->WasNamespaceDeclaration = FALSE;
                            pElementAttribute->HasXmlPrefix = FALSE;
                            pElementAttribute->Location = CurrentLocation;
                        }

                        pDocumentPiece->item.Element.ulAttributeCount++;
                    }
                    break;

                    //
                    // The beginning of an attribute whose prefix is "xml"
                    //
                case XTSS_ELEMENT_XML:
                    {
                        static const XMLSTRUCTURE_LOOKAHEAD_DATA Lookaheads[] =
                        {
                            { XTSS_ELEMENT_XML_COLON },
                            { XTSS_ELEMENT_XML_NAME },
                            { XTSS_ELEMENT_XML_EQUALS },
                            { XTSS_ELEMENT_XML_VALUE_OPEN },
                            { XTSS_ELEMENT_XML_VALUE },
                            { XTSS_ELEMENT_XML_VALUE_CLOSE }
                        };

                        UINTN cGathered;
                        XML_TOKEN TempTokens[RTL_NUMBER_OF(Lookaheads)];

                        status = RtlpXmlExpectStates(
                            &pParseState->ParseState,
                            RTL_NUMBER_OF(Lookaheads),
                            Lookaheads,
                            TempTokens,
                            &cGathered
                            );

                        if (EFI_ERROR(status))
                        {
                            pDocumentPiece->ulThingType = XMLDOC_THING_ERROR;
                            pDocumentPiece->item.Error.Location = CurrentLocation;
                            pDocumentPiece->item.Error.BadExtent = TempTokens[cGathered].Run;
                            pDocumentPiece->item.Error.Code = XMLERROR_NS_DECL_GENERAL_FAILURE;
                            goto Exit;
                        }
                        else if (cGathered != RTL_NUMBER_OF(Lookaheads))
                        {
                            pDocumentPiece->ulThingType = XMLDOC_THING_ERROR;
                            pDocumentPiece->item.Error.Location = CurrentLocation;
                            pDocumentPiece->item.Error.BadExtent = TempTokens[cGathered].Run;
                            pDocumentPiece->item.Error.Code = Lookaheads[cGathered].ErrorIfMissing;
                            goto Exit;
                        }

                        if (pAttributeList != NULL) {

                            status = RtlIndexIntoGrowingList(
                                pAttributeList,
                                pDocumentPiece->item.Element.ulAttributeCount,
                                (VOID**)&pElementAttribute,
                                TRUE
                                );

                            if (EFI_ERROR(status)) {
                                return status;
                            }

                            pElementAttribute->Name = TempTokens[1].Run;
                            pElementAttribute->NsPrefix = TokenWorker.Run;
                            pElementAttribute->OriginalNsPrefix = TokenWorker.Run;
                            pElementAttribute->Value = TempTokens[4].Run;
                            pElementAttribute->WasNamespaceDeclaration = FALSE;
                            pElementAttribute->HasXmlPrefix = TRUE;
                            pElementAttribute->Location = CurrentLocation;
                        }

                        pDocumentPiece->item.Element.ulAttributeCount++;
                    }
                    break;

                    //
                    // We found an attribute name, or a namespace prefix.  Allocate a new
                    // attribute off the list and set it up
                    //
                case XTSS_ELEMENT_ATTRIBUTE_NAME:
                    {
                        static const XMLSTRUCTURE_LOOKAHEAD_DATA Lookaheads[] =
                        {
                            { XTSS_ELEMENT_ATTRIBUTE_EQUALS },
                            { XTSS_ELEMENT_ATTRIBUTE_OPEN },
                            { XTSS_ELEMENT_ATTRIBUTE_VALUE },
                            { XTSS_ELEMENT_ATTRIBUTE_CLOSE }
                        };

                        UINTN cGathered;
                        XML_TOKEN TempTokens[RTL_NUMBER_OF(Lookaheads)];

                        status = RtlpXmlExpectStates(
                            &pParseState->ParseState,
                            RTL_NUMBER_OF(Lookaheads),
                            Lookaheads,
                            TempTokens,
                            &cGathered
                            );

                        if (EFI_ERROR(status))
                        {
                            pDocumentPiece->ulThingType = XMLDOC_THING_ERROR;
                            pDocumentPiece->item.Error.Location = CurrentLocation;
                            pDocumentPiece->item.Error.BadExtent = TempTokens[cGathered].Run;
                            pDocumentPiece->item.Error.Code = XMLERROR_NS_DECL_GENERAL_FAILURE;
                            goto Exit;
                        }
                        else if (cGathered != RTL_NUMBER_OF(Lookaheads))
                        {
                            pDocumentPiece->ulThingType = XMLDOC_THING_ERROR;
                            pDocumentPiece->item.Error.Location = CurrentLocation;
                            pDocumentPiece->item.Error.BadExtent = TempTokens[cGathered].Run;
                            pDocumentPiece->item.Error.Code = Lookaheads[cGathered].ErrorIfMissing;
                            goto Exit;
                        }
                        else
                        {
                            if (pAttributeList != NULL) {

                                status = RtlIndexIntoGrowingList(
                                    pAttributeList,
                                    pDocumentPiece->item.Element.ulAttributeCount,
                                    (VOID**)&pElementAttribute,
                                    TRUE
                                    );

                                if (EFI_ERROR(status)) {
                                    return status;
                                }

                                ZeroMem(pElementAttribute, sizeof(*pElementAttribute));
                                pElementAttribute->Name = TokenWorker.Run;
                                pElementAttribute->Value = TempTokens[2].Run;
                                pElementAttribute->Location = CurrentLocation;
                            }

                            pDocumentPiece->item.Element.ulAttributeCount++;
                        }
                    }
                    break;

                }
            }
            while (TRUE);

            // Pop the previous currentlocation.
            CurrentLocation = PrevLocation;

            //
            // Now that we're all done, go put this element on the stack
            //
            if (!TokenWorker.fError && NT_SUCCESS(status)) {
                LOGICAL_XML_ERROR ErrorInLookup = XMLERROR_SUCCESS;
                XML_EXTENT ErroneousPrefix;
                ZeroMem(&ErroneousPrefix, sizeof(ErroneousPrefix));

                //
                // Fix up namespaces first
                //
                if (pNamespaceManager) {
                    status = _RtlpFixUpNamespaces(
                        pParseState,
                        pNamespaceManager,
                        pAttributeList,
                        pDocumentPiece,
                        pDocumentPiece->ulDocumentDepth + 1,
                        &ErrorInLookup,
                        &ErroneousPrefix
                        );
                }

                //
                // Returning STATUS_XML_PARSE_ERROR is the way we know that
                // something happened in fixing up the namespaces. Set up
                // an error context and be done.
                //
                if (status == STATUS_XML_PARSE_ERROR) {
                    pDocumentPiece->ulThingType = XMLDOC_THING_ERROR;
                    pDocumentPiece->item.Error.Code = ErrorInLookup;
                    pDocumentPiece->item.Error.BadExtent = ErroneousPrefix;
                    pDocumentPiece->item.Error.Location = CurrentLocation;
                    status = EFI_SUCCESS;
                    goto Exit;
                }
                else if (EFI_ERROR(status)) {
                    return status;
                }

                //
                // This is an empty element (no children), and the namespace depth frame
                // has to be left as well
                //
                if (TokenWorker.State == XTSS_ELEMENT_CLOSE_EMPTY) {
                    pDocumentPiece->item.Element.fElementEmpty = TRUE;

                    if (pNamespaceManager) {

                        status = RtlNsLeaveDepth(
                            pNamespaceManager,
                            pDocumentPiece->ulDocumentDepth + 1
                            );

                        if (EFI_ERROR(status)) {
                            return status;
                        }
                    }
                }
                else {
                    status = RtlIndexIntoGrowingList(
                        &pParseState->ElementStack,
                        pDocumentPiece->ulDocumentDepth,
                        (VOID**)&pStackElement,
                        TRUE);

                    if (EFI_ERROR(status)) {
                        return status;
                    }

                    //
                    // Open tag, increment depth
                    //
                    pParseState->ulElementStackDepth++;

                    *pStackElement = *pDocumentPiece;
                }
            }


        }
        break;






        //
        // We're ending an element run, so we have to pop an item off the stack.
        //
    case XTSS_ENDELEMENT_OPEN:
        {
            PXMLDOC_THING pLastElement = NULL;

            status = RtlIndexIntoGrowingList(
                &pParseState->ElementStack,
                --pParseState->ulElementStackDepth,
                (VOID**)&pLastElement,
                FALSE);

            if (EFI_ERROR(status)) {
                return status;
            }

            //
            // Now get the current element in the stream
            //
            status = RtlXmlNextToken(&pParseState->ParseState, &TokenWorker, TRUE);
            if (EFI_ERROR(status) || TokenWorker.fError ||
                ((TokenWorker.State != XTSS_ENDELEMENT_NAME) && (TokenWorker.State != XTSS_ENDELEMENT_NS_PREFIX))) {
                pDocumentPiece->ulThingType = XMLDOC_THING_ERROR;
                pDocumentPiece->item.Error.BadExtent = TokenWorker.Run;
                pDocumentPiece->item.Error.Code = XMLERROR_ENDELEMENT_NAME_NOT_FOUND;
                pDocumentPiece->item.Error.Location = CurrentLocation;
            }
            else {

                //
                // A namespace prefix must get recorded, and then the colon has to be skipped
                //
                if (TokenWorker.State == XTSS_ENDELEMENT_NS_PREFIX) {

                    pDocumentPiece->item.EndElement.NsPrefix = TokenWorker.Run;
                    pDocumentPiece->item.EndElement.OriginalNsPrefix = TokenWorker.Run;

                    //
                    // Ensure that a colon was found
                    //
                    status = RtlXmlNextToken(&pParseState->ParseState, &TokenWorker, TRUE);
                    if (EFI_ERROR(status) || TokenWorker.fError || (TokenWorker.State != XTSS_ENDELEMENT_NS_COLON)) {
                        pDocumentPiece->ulThingType = XMLDOC_THING_ERROR;
                        pDocumentPiece->item.Error.BadExtent = TokenWorker.Run;
                        pDocumentPiece->item.Error.Code = XMLERROR_ENDELEMENT_MALFORMED_NAME;
                        pDocumentPiece->item.Error.Location = CurrentLocation;
                        goto Exit;
                    }

                    //
                    // We must get an element name
                    //
                    status = RtlXmlNextToken(&pParseState->ParseState, &TokenWorker, TRUE);
                    if (EFI_ERROR(status) || TokenWorker.fError || (TokenWorker.State != XTSS_ENDELEMENT_NAME)) {
                        pDocumentPiece->ulThingType = XMLDOC_THING_ERROR;
                        pDocumentPiece->item.Error.BadExtent = TokenWorker.Run;
                        pDocumentPiece->item.Error.Code = XMLERROR_ENDELEMENT_MALFORMED_NAME;
                        pDocumentPiece->item.Error.Location = CurrentLocation;
                        goto Exit;
                    }
                }

                //
                // Save the name, and the opening element (which we found on the stack)
                //
                pDocumentPiece->item.EndElement.Name = TokenWorker.Run;
                pDocumentPiece->item.EndElement.OpeningElement = pLastElement->item.Element;
                pDocumentPiece->item.EndElement.Location = CurrentLocation;
                pDocumentPiece->ulThingType = XMLDOC_THING_END_ELEMENT;
                pDocumentPiece->ulDocumentDepth--;

                //
                // And consume elements until we hit the close of an element
                //
                do {
                    status = RtlXmlNextToken(&pParseState->ParseState, &TokenWorker, TRUE);

                    if (EFI_ERROR(status) || TokenWorker.fError || (TokenWorker.State == XTSS_STREAM_END)) {
                        pDocumentPiece->ulThingType = XMLDOC_THING_ERROR;
                        pDocumentPiece->item.Error.BadExtent = TokenWorker.Run;
                        pDocumentPiece->item.Error.Code = XMLERROR_ENDELEMENT_MALFORMED;
                        pDocumentPiece->item.Error.Location = CurrentLocation;
                        goto Exit;
                    }
                    else if (TokenWorker.State == XTSS_ENDELEMENT_CLOSE) {
                        break;
                    }

                }
                while (TRUE);

                //
                // Fix up namespaces before returning
                //
                if (pNamespaceManager != NULL)
                {
                    LOGICAL_XML_ERROR Failure;
                    XML_EXTENT FailingExtent;

                    status = _RtlpFixUpNamespaces(
                        pParseState,
                        pNamespaceManager,
                        NULL,
                        pDocumentPiece,
                        pLastElement->ulDocumentDepth + 1,
                        &Failure,
                        &FailingExtent
                        );

                    if (status == STATUS_XML_PARSE_ERROR) {
                        pDocumentPiece->ulThingType = XMLDOC_THING_ERROR;
                        pDocumentPiece->item.Error.BadExtent = FailingExtent;
                        pDocumentPiece->item.Error.Code = Failure;
                        pDocumentPiece->item.Error.Location = CurrentLocation;
                        status = EFI_SUCCESS;
                        goto Exit;
                    }
                    else if (EFI_ERROR(status))
                        goto Exit;

                    status = RtlNsLeaveDepth(pNamespaceManager, pLastElement->ulDocumentDepth + 1);
                    if (EFI_ERROR(status))
                        goto Exit;
                }

                //
                // One last thing - we need to make sure that the element being closed
                // matches the element that was opened.  Do this by comparing the namespace
                // and name of the open/close elements.
                //
                {
                    XML_STRING_COMPARE Comparison;

                    status = (*pParseState->ParseState.pfnCompareStrings)(
                        &pParseState->ParseState,
                        &pDocumentPiece->item.EndElement.OpeningElement.Name,
                        &pDocumentPiece->item.EndElement.Name,
                        &Comparison);

                    if ((Comparison == XML_STRING_COMPARE_EQUALS) && NT_SUCCESS(status))
                    {
                        status = (*pParseState->ParseState.pfnCompareStrings)(
                            &pParseState->ParseState,
                            &pDocumentPiece->item.EndElement.OpeningElement.OriginalNsPrefix,
                            &pDocumentPiece->item.EndElement.OriginalNsPrefix,
                            &Comparison);
                    }

                    if (EFI_ERROR(status))
                        goto Exit;

                    if (Comparison != XML_STRING_COMPARE_EQUALS) {
                        XML_EXTENT ex = pDocumentPiece->TotalExtent;
                        pDocumentPiece->ulThingType = XMLDOC_THING_ERROR;
                        pDocumentPiece->item.Error.BadExtent = ex;
                        pDocumentPiece->item.Error.Code = XMLERROR_ENDELEMENT_MISMATCHED_CLOSE_TAG;
                        pDocumentPiece->item.Error.Location = CurrentLocation;
                        status = EFI_SUCCESS;
                        goto Exit;
                    }

                }

            }

        }
        break;








        //
        // Oo, the end of the stream!
        //
    case XTSS_STREAM_END:
        if (pParseState->ulElementStackDepth == 0) {
            pDocumentPiece->ulThingType = XMLDOC_THING_END_OF_STREAM;
        }
        else {
            pDocumentPiece->ulThingType = XMLDOC_THING_ERROR;
            pDocumentPiece->item.Error.BadExtent = TokenWorker.Run;
            pDocumentPiece->item.Error.Code = XMLERROR_EOF_BEFORE_CLOSE;
            pDocumentPiece->item.Error.Location = CurrentLocation;
            goto Exit;
        }
        break;
    }

Exit:
    pDocumentPiece->TotalExtent.cbData = (UINT8*) pParseState->ParseState.RawTokenState.pvCursor -
        (UINT8*)pDocumentPiece->TotalExtent.pvData;

    if (TokenWorker.fError && NT_SUCCESS(status))
    {
        status = RtlpReportXmlError(STATUS_XML_PARSE_ERROR);
    }

    return status;
}

static
EFI_STATUS
EFIAPI
CharacterToDigit(
    IN BOOLEAN fHex,
    IN UINT32 Character,
    OUT UINT32* Digit
    )
{
    *Digit = 0;

    if ((Character >= '0') && (Character <= '9')) {
        *Digit = Character - '0';
        return EFI_SUCCESS;
    }

    if (fHex)
    {
        if ((Character >= 'a') && (Character <= 'f')) {
            *Digit = Character - 'a' + 10;
            return EFI_SUCCESS;
        }

        if ((Character >= 'A') && (Character <= 'F')) {
            *Digit = Character - 'A' + 10;
            return EFI_SUCCESS;
        }
    }

    return RtlpReportXmlError(STATUS_XML_PARSE_ERROR);
}

static
EFI_STATUS
EFIAPI
RtlpXmlCharacterCodeToCharacter(
    IN PXML_RAWTOKENIZATION_STATE pState,
    IN VOID* pvCursor,
    IN VOID* pvDocumentEnd,
    OUT VOID* *ppvReferenceEnd,
    OUT UINT32 *pCharacter
    )
{
    BOOLEAN fHex = FALSE;
    BOOLEAN fAtLeastOneDigit = FALSE;
    UINT32 Character = 0;
    EFI_STATUS Status = EFI_SUCCESS;

    XML_RAWTOKENIZATION_RESULT Result;

    *ppvReferenceEnd = NULL;
    *pCharacter = 0;

    //
    // Get the initial character
    //
    Result = (*pState->pfnNextChar)(pvCursor, pvDocumentEnd);

    if (Result.Character == XML_RAWTOKENIZATION_INVALID_CHARACTER) {
        return Result.Result.ErrorCode;
    }

    if (Result.Character == 'x') {
        fHex = TRUE;
    }
    else {
        Status = CharacterToDigit(fHex, Result.Character, &Character);
        if (EFI_ERROR(Status)) {
            return Status;
        }

        fAtLeastOneDigit = TRUE;
    }

    pvCursor = Result.Result.NextCursor;

    //
    // Parse the rest of the digits up to the terminating semicolon
    //
    do {
        const UINT32 Base = fHex ? 16 : 10;
        UINT32 NextDigit;

        Result = (*pState->pfnNextChar)(pvCursor, pvDocumentEnd);

        if (Result.Character == XML_RAWTOKENIZATION_INVALID_CHARACTER) {
            return Result.Result.ErrorCode;
        }

        if (Result.Character != ';') {

            Status = CharacterToDigit(fHex, Result.Character, &NextDigit);
            if (EFI_ERROR(Status)) {
                return Status;
            }

            fAtLeastOneDigit = TRUE;

            if (Character > (MAXULONG / Base)) {
                return RtlpReportXmlError(EFI_INVALID_PARAMETER);
            }

            Character *= Base;

            if (Character > (MAXULONG - NextDigit)) {
                return RtlpReportXmlError(EFI_INVALID_PARAMETER);
            }

            Character += NextDigit;
        }

        pvCursor = Result.Result.NextCursor;

    } while (Result.Character != ';');

    if (!fAtLeastOneDigit)
        return RtlpReportXmlError(STATUS_XML_PARSE_ERROR);

    *ppvReferenceEnd = pvCursor;
    *pCharacter = Character;

    return EFI_SUCCESS;
}

static
EFI_STATUS
EFIAPI
RtlpXmlReferenceToCharacter(
    IN PXML_RAWTOKENIZATION_STATE pState,
    IN VOID* pvCursor,
    IN VOID* pvDocumentEnd,
    OUT VOID* *ppvReferenceEnd,
    OUT UINT32 *pCharacter
    )
{
    //
    // Really entity references can expand to more than a single character, but
    // that requires DTD support that isn't currently implemented
    //
    static const struct _ENTITY_REFERENCE_DATA
    {
        CHAR16* pwszEntityName;
        UINT32 EntityValue;
    } sc_BuiltinEntities[] =
    {
        { L"amp", '&' },
        { L"apos", '\'' },
        { L"quot", '\"' },
        { L"lt", '<' },
        { L"gt", '>' }
    };

    UINT32 i = 0;
    UINT32 iChar = 0;

    XML_RAWTOKENIZATION_RESULT Result;

    BOOLEAN rgfMatches[RTL_NUMBER_OF(sc_BuiltinEntities)];

    for (i = 0; i < RTL_NUMBER_OF(rgfMatches); i++) {
        rgfMatches[i] = TRUE;
    }

    *ppvReferenceEnd = NULL;
    *pCharacter = 0;

    //
    // Get the initial '&'
    //
    Result = (*pState->pfnNextChar)(pvCursor, pvDocumentEnd);

    if (Result.Character == XML_RAWTOKENIZATION_INVALID_CHARACTER) {
        return Result.Result.ErrorCode;
    }

    if (Result.Character != '&') {
        return RtlpReportXmlError(STATUS_INTERNAL_ERROR);
    }

    pvCursor = Result.Result.NextCursor;

    //
    // Check if the next character is a '#' (character reference)
    //
    Result = (*pState->pfnNextChar)(pvCursor, pvDocumentEnd);

    if (Result.Character == XML_RAWTOKENIZATION_INVALID_CHARACTER) {
        return Result.Result.ErrorCode;
    }

    if (Result.Character == '#') {
        pvCursor = Result.Result.NextCursor;

        return RtlpXmlCharacterCodeToCharacter(
            pState,
            pvCursor,
            pvDocumentEnd,
            ppvReferenceEnd,
            pCharacter);
    }

    //
    // Try to match given reference against list of known entities
    //
    do {
        Result = (*pState->pfnNextChar)(pvCursor, pvDocumentEnd);

        if (Result.Character == XML_RAWTOKENIZATION_INVALID_CHARACTER) {
            return Result.Result.ErrorCode;
        }

        if (Result.Character != ';') {

            for (i = 0; i < RTL_NUMBER_OF(sc_BuiltinEntities); i++) {

                if (rgfMatches[i]) {

                    if (Result.Character != sc_BuiltinEntities[i].pwszEntityName[iChar]) {
                        rgfMatches[i] = FALSE;
                    }

                    if (sc_BuiltinEntities[i].pwszEntityName[iChar] == L'\0') {
                        rgfMatches[i] = FALSE;
                    }
                }
            }
        }

        iChar++;
        pvCursor = Result.Result.NextCursor;

    } while (Result.Character != ';');

    //
    // If a match was found, return it; otherwise error
    //
    for (i = 0; i < RTL_NUMBER_OF(rgfMatches); i++) {
        if (rgfMatches[i])
            break;
    }

    if (i < RTL_NUMBER_OF(sc_BuiltinEntities)) {
        *ppvReferenceEnd = pvCursor;
        *pCharacter = sc_BuiltinEntities[i].EntityValue;
    }
    else {
        return RtlpReportXmlError(STATUS_XML_PARSE_ERROR);
    }

    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
RtlXmlExtentToUtf8String(
    IN UINT32                   ConversionFlags,
    IN_OUT PXML_RAWTOKENIZATION_STATE pParseState,
    IN PCXML_EXTENT             pExtent,
    IN_OUT PLUTF8_STRING         pString,
    OUT UINTN*                 pcbRequired
    )
{
    EFI_STATUS status;
    VOID* pvCursor;
    VOID* pvDocumentEnd;
    UINT8* pbOutputCursor;
    UINT8* pbOutputCursorEnd;
    UINT32 Character;
    BOOLEAN fConvertReferences = FALSE;
    UINT32 cbRequired = 0;

    if (pString != NULL)
        pString->Length = 0;

    if (pcbRequired != NULL)
        *pcbRequired = 0;

    if ((ConversionFlags & ~RTL_XML_EXTENT_TO_UTF8_STRING_FLAG_CONVERT_REFERENCES) != 0)
        return RtlpReportXmlError(EFI_INVALID_PARAMETER);

    if (pParseState == NULL)
        return RtlpReportXmlError(EFI_INVALID_PARAMETER);

    if (pExtent == NULL)
        return RtlpReportXmlError(EFI_INVALID_PARAMETER);

    if (pString && (pString->Buffer == NULL) && (pString->MaximumLength != 0))
        return RtlpReportXmlError(EFI_INVALID_PARAMETER);

    pvCursor = pExtent->pvData;
    pvDocumentEnd = (VOID*)(((UINTN)pvCursor) + (UINTN)pExtent->cbData);

    if (pString) {
        pbOutputCursor = pString->Buffer;
        pbOutputCursorEnd = (UINT8*)(((UINTN)pbOutputCursor) + pString->MaximumLength);
    }
    else {
        pbOutputCursor = pbOutputCursorEnd = NULL;
    }

    fConvertReferences = (ConversionFlags & RTL_XML_EXTENT_TO_UTF8_STRING_FLAG_CONVERT_REFERENCES) != 0;

    //
    // Rawtokenize the bits
    //
    while (pvCursor < pvDocumentEnd) {

        const XML_RAWTOKENIZATION_RESULT Result = (*pParseState->pfnNextChar)(pvCursor, pvDocumentEnd);

        if (Result.Character == XML_RAWTOKENIZATION_INVALID_CHARACTER) {
            status = Result.Result.ErrorCode;
            goto Exit;
        }

        if (Result.Character == '&' && fConvertReferences) {

            EFI_STATUS Status = RtlpXmlReferenceToCharacter(
                pParseState,
                pvCursor,
                pvDocumentEnd,
                &pvCursor,
                &Character);

            if (EFI_ERROR(Status))
                return Status;
        }
        else {
            pvCursor = Result.Result.NextCursor;
            Character = Result.Character;
        }

        if (Character < 0x80) {

            if (pbOutputCursor < pbOutputCursorEnd) {
                pbOutputCursor[0] = (UINT8)Character;
                pbOutputCursor += 1;
            }

            cbRequired += 1;
        }
        else if (Character < 0x800) {

            if ((pbOutputCursor + 1) < pbOutputCursorEnd) {
                pbOutputCursor[0] = 0xc0 | (UINT8)((Character >> 6) & 0x1f);
                pbOutputCursor[1] = 0x80 | (UINT8)((Character >> 0) & 0x3f);
                pbOutputCursor += 2;
            }
            else
                pbOutputCursor = pbOutputCursorEnd;

            cbRequired += 2;
        }
        else if (Character < 0x10000) {

            if ((pbOutputCursor + 2) < pbOutputCursorEnd) {
                pbOutputCursor[0] = 0xe0 | (UINT8)((Character >> 12) & 0x0f);
                pbOutputCursor[1] = 0x80 | (UINT8)((Character >> 6) & 0x3f);
                pbOutputCursor[2] = 0x80 | (UINT8)((Character >> 0) & 0x3f);
                pbOutputCursor += 3;
            }
            else
                pbOutputCursor = pbOutputCursorEnd;

            cbRequired += 3;
        }
        else if (Character < 0x200000) {

            if ((pbOutputCursor + 3) < pbOutputCursorEnd) {
                pbOutputCursor[0] = 0xf0 | (UINT8)((Character >> 18) & 0x07);
                pbOutputCursor[1] = 0x80 | (UINT8)((Character >> 12) & 0x3f);
                pbOutputCursor[2] = 0x80 | (UINT8)((Character >> 6) & 0x3f);
                pbOutputCursor[3] = 0x80 | (UINT8)((Character >> 0) & 0x3f);
                pbOutputCursor += 4;
            }
            else
                pbOutputCursor = pbOutputCursorEnd;

            cbRequired += 4;
        }
        else if (Character < 0x4000000) {

            if ((pbOutputCursor + 4) < pbOutputCursorEnd) {
                pbOutputCursor[0] = 0xf8 | (UINT8)((Character >> 24) & 0x03);
                pbOutputCursor[1] = 0x80 | (UINT8)((Character >> 18) & 0x3f);
                pbOutputCursor[2] = 0x80 | (UINT8)((Character >> 12) & 0x3f);
                pbOutputCursor[3] = 0x80 | (UINT8)((Character >> 6) & 0x3f);
                pbOutputCursor[4] = 0x80 | (UINT8)((Character >> 0) & 0x3f);
                pbOutputCursor += 5;
            }
            else
                pbOutputCursor = pbOutputCursorEnd;

            cbRequired += 5;
        }
        else {

            if ((pbOutputCursor + 5) < pbOutputCursorEnd) {
                pbOutputCursor[0] = 0xfc | (UINT8)((Character >> 30) & 0x01);
                pbOutputCursor[1] = 0x80 | (UINT8)((Character >> 24) & 0x3f);
                pbOutputCursor[2] = 0x80 | (UINT8)((Character >> 18) & 0x3f);
                pbOutputCursor[3] = 0x80 | (UINT8)((Character >> 12) & 0x3f);
                pbOutputCursor[4] = 0x80 | (UINT8)((Character >> 6) & 0x3f);
                pbOutputCursor[5] = 0x80 | (UINT8)((Character >> 0) & 0x3f);
                pbOutputCursor += 6;
            }
            else
                pbOutputCursor = pbOutputCursorEnd;

            cbRequired += 6;
        }
    }

    if (pcbRequired)
        *pcbRequired = cbRequired;

    if ((pString == NULL) || (cbRequired > pString->MaximumLength)) {
        status = RtlpReportXmlError(EFI_INVALID_PARAMETER);
    }
    else {
        pString->Length = cbRequired;
        status = EFI_SUCCESS;
    }

Exit:
    return status;
}

EFI_STATUS
EFIAPI
RtlXmlExtentToString(
    IN UINT32                         ConversionFlags,
    IN_OUT PXML_RAWTOKENIZATION_STATE pState,
    IN PCXML_EXTENT                   pExtent,
    IN_OUT PUNICODE_STRING            pString,
    OUT UINTN*                        pcbString
    )
{
    CHAR16* pwszWriteCursor;
    CHAR16* pwszWriteEnd;
    VOID* pvCursor;
    VOID* pvDocumentEnd;
    UINT32 Character;
    BOOLEAN fConvertReferences;
    UINT32 cbRequired = 0;

    if (!pState || !pExtent || !pcbString || !pString)
        return RtlpReportXmlError(EFI_INVALID_PARAMETER);

    pString->Length = 0;

    *pcbString = 0;


    pvCursor = pExtent->pvData;
    pvDocumentEnd = (VOID*)(((UINTN)pExtent->pvData) + (UINTN)pExtent->cbData);
    pwszWriteCursor = pString->Buffer;
    pwszWriteEnd = (CHAR16*)(((UINTN)pwszWriteCursor) + pString->MaximumLength);

    fConvertReferences = (ConversionFlags & RTL_XML_EXTENT_TO_STRING_FLAG_CONVERT_REFERENCES) != 0;

    while (pvCursor < pvDocumentEnd) {

        XML_RAWTOKENIZATION_RESULT Result = (*pState->pfnNextChar)(pvCursor, pvDocumentEnd);

        if (Result.Character == XML_RAWTOKENIZATION_INVALID_CHARACTER) {
            return Result.Result.ErrorCode;
        }

        if (Result.Character == '&' && fConvertReferences) {

            EFI_STATUS Status = RtlpXmlReferenceToCharacter(
                pState,
                pvCursor,
                pvDocumentEnd,
                &pvCursor,
                &Character);

            if (EFI_ERROR(Status))
                return Status;
        }
        else {
            pvCursor = Result.Result.NextCursor;
            Character = Result.Character;
        }

        //
        // Normal character
        //
        if (Character < 0x10000) {

            if (pwszWriteCursor < pwszWriteEnd) {
                pwszWriteCursor[0] = (CHAR16)Character;
                pwszWriteCursor++;
            }

            cbRequired += sizeof(CHAR16);
        }
        //
        // Two chars required
        //
        else if (Character < 0x110000) {

            if ((pwszWriteEnd + 2) <= pwszWriteEnd) {
                pwszWriteCursor[0] = (CHAR16)(((Character - 0x10000) / 0x400) + 0xd800);
                pwszWriteCursor[1] = (CHAR16)(((Character - 0x10000) % 0x400) + 0xdc00);
                pwszWriteCursor += 2;
            }

            cbRequired += sizeof(CHAR16) * 2;
        }
        else {
            return RtlpReportXmlError(STATUS_ILLEGAL_CHARACTER);
        }
    }

    *pcbString = cbRequired;

    if ((pString == NULL) || (cbRequired > pString->MaximumLength)) {
        return EFI_INVALID_PARAMETER;
    }
    else if (cbRequired > 0xffff) {
        return RtlpReportXmlError(EFI_INVALID_PARAMETER);
    }
    else {
        pString->Length = (UINT16)cbRequired;
        return EFI_SUCCESS;
    }

}


EFI_STATUS
EFIAPI
RtlXmlMatchAttribute(
    IN PXML_TOKENIZATION_STATE      State,
    IN PXMLDOC_ATTRIBUTE            Attribute,
    IN PCXML_SIMPLE_STRING         Namespace,
    IN PCXML_SIMPLE_STRING         AttributeName,
    OUT XML_STRING_COMPARE         *CompareResult
    )
{
    EFI_STATUS status = EFI_SUCCESS;

    if (CompareResult)
        *CompareResult = XML_STRING_COMPARE_LT;

    if (!CompareResult || !State || !Attribute || !AttributeName)
        return RtlpReportXmlError(EFI_INVALID_PARAMETER);

    //
    // If they asked for a namespace, then the attribute has to have
    // a namespace, and vice-versa.
    //
    if ((Namespace == NULL) != (Attribute->NsPrefix.cbData == 0)) {
        if (Namespace == NULL) {
            *CompareResult = XML_STRING_COMPARE_LT;
        }
        else {
            *CompareResult = XML_STRING_COMPARE_GT;
        }
    }

    if (Namespace != NULL) {

        status = State->pfnCompareSpecialString(
            State,
            &Attribute->NsPrefix,
            Namespace,
            CompareResult,
            NULL);

        if (EFI_ERROR(status) || (*CompareResult != XML_STRING_COMPARE_EQUALS))
            goto Exit;
    }

    status = State->pfnCompareSpecialString(
        State,
        &Attribute->Name,
        AttributeName,
        CompareResult,
        NULL);

    if (EFI_ERROR(status) || (*CompareResult != XML_STRING_COMPARE_EQUALS))
        goto Exit;

    *CompareResult = XML_STRING_COMPARE_EQUALS;
Exit:
    return status;

}



EFI_STATUS
EFIAPI
RtlXmlMatchLogicalElement(
    IN  PXML_TOKENIZATION_STATE     pState,
    IN  PXMLDOC_ELEMENT             pElement,
    IN  PCXML_SIMPLE_STRING         pNamespace,
    IN  PCXML_SIMPLE_STRING         pElementName,
    OUT BOOLEAN*                    pfMatches
    )
{
    EFI_STATUS status = EFI_SUCCESS;
    XML_STRING_COMPARE Compare;

    if (pfMatches)
        *pfMatches = FALSE;

    if (!pState || !pElement || !pElementName || !pfMatches)
        return RtlpReportXmlError(EFI_INVALID_PARAMETER);

    if ((pNamespace == NULL) != (pElement->NsPrefix.cbData == 0))
        goto Exit;

    if (pNamespace != NULL) {

        status = pState->pfnCompareSpecialString(pState, &pElement->NsPrefix, pNamespace, &Compare, NULL);
        if (EFI_ERROR(status) || (Compare != XML_STRING_COMPARE_EQUALS)) {
            goto Exit;
        }
    }

    status = pState->pfnCompareSpecialString(pState, &pElement->Name, pElementName, &Compare, NULL);
    if (EFI_ERROR(status) || (Compare != XML_STRING_COMPARE_EQUALS))
        goto Exit;

    *pfMatches = TRUE;
Exit:
    return status;
}






//
//  Dev Note:  The compiler is trying to optimize this loop by using
//             a memset, which then does not link.  Turn it off.
//
//for (ul = 0; ul < ulFindCount; ul++)
//{
//    ppAttributes[ul] = NULL;
//}

#pragma optimize("", off)

EFI_STATUS
EFIAPI
RtlXmlFindAttributesInElement(
    IN  PXML_TOKENIZATION_STATE     pState,
    IN  PRTL_GROWING_LIST           pAttributeList,
    IN  UINT32                       ulAttributeCountInElement,
    IN  UINT32                       ulFindCount,
    IN  PCXML_ATTRIBUTE_DEFINITION  pAttributeNames,
    OUT PXMLDOC_ATTRIBUTE          *ppAttributes,
    OUT UINT32*                      pulUnmatchedAttributes
    )
{
    EFI_STATUS            status;
    PXMLDOC_ATTRIBUTE   pAttrib;
    UINT32               ul = 0;
    UINT32               attr = 0;
    XML_STRING_COMPARE  Compare;

    if (pulUnmatchedAttributes)
        *pulUnmatchedAttributes = 0;

    if (!pAttributeNames && (ulFindCount != 0))
        return RtlpReportXmlError(EFI_INVALID_PARAMETER);

    //
    // NULL the outbound array members appropriately
    //
    
    for (ul = 0; ul < ulFindCount; ul++)
    {
        ppAttributes[ul] = NULL;
    }

    //
    // For each attribute in the element...
    //
    for (attr = 0; attr < ulAttributeCountInElement; attr++) {

        //
        // Find this element
        //
        status = RtlIndexIntoGrowingList(pAttributeList, attr, (VOID**)&pAttrib, FALSE);
        if (EFI_ERROR(status))
            goto Exit;

        //
        // Compare it to all the attributes we're looking for
        //
        for (ul = 0; ul < ulFindCount; ul++) {

            //
            // If there was a namespace, then see if it matches first
            //
            if (pAttributeNames[ul].Namespace != NULL) {

                status = pState->pfnCompareSpecialString(
                    pState,
                    &pAttrib->NsPrefix,
                    pAttributeNames[ul].Namespace,
                    &Compare,
                    NULL);

                if (EFI_ERROR(status))
                    goto Exit;

                if (Compare != XML_STRING_COMPARE_EQUALS)
                    continue;
            }

            status = pState->pfnCompareSpecialString(
                pState,
                &pAttrib->Name,
                &pAttributeNames[ul].Name,
                &Compare,
                NULL);

            if (EFI_ERROR(status))
                goto Exit;

            if (Compare == XML_STRING_COMPARE_EQUALS) {
                ppAttributes[ul] = pAttrib;
                break;
            }
        }

        if ((ul == ulFindCount) && pulUnmatchedAttributes) {
            (*pulUnmatchedAttributes)++;
        }
    }

    status = EFI_SUCCESS;
Exit:
    return status;
}

EFI_STATUS
EFIAPI
RtlXmlSkipElement(
    PXML_LOGICAL_STATE pState,
    PXMLDOC_ELEMENT TheElement
    )
{
    XMLDOC_THING TempThing;
    EFI_STATUS status;

    if (!pState || !TheElement)
        return RtlpReportXmlError(EFI_INVALID_PARAMETER);

    if (TheElement->fElementEmpty)
        return EFI_SUCCESS;

    while (TRUE) {

        status = RtlXmlNextLogicalThing(pState, NULL, &TempThing, NULL);
        if (EFI_ERROR(status))
            goto Exit;

        // See if the end element we found is the same as the element we're
        // looking for.
        if (TempThing.ulThingType == XMLDOC_THING_END_ELEMENT) {

            // If these point at the same thing, then this really is the close of this element
            if (TempThing.item.EndElement.OpeningElement.Name.pvData == TheElement->Name.pvData) {
                break;
            }
        }
        // The caller can deal with end of stream on their next call to
        // the logical xml advancement routine...
        else if (TempThing.ulThingType == XMLDOC_THING_END_OF_STREAM) {
            break;
        }
    }

    status = EFI_SUCCESS;
Exit:
    return status;
}
