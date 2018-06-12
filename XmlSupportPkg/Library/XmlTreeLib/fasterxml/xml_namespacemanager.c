/**
XML namespace manager implementation.

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

EFI_STATUS
EFIAPI
RtlNsInitialize(
    PNS_MANAGER             pManager,
    PFNCOMPAREEXTENTS       pCompare,
    VOID*                   pCompareContext,
    PRTL_ALLOCATOR          Allocation
    )
{
    EFI_STATUS status = EFI_SUCCESS;

    if (pManager == NULL)
    {
      return EFI_INVALID_PARAMETER;
    }

    ZeroMem(pManager, sizeof(pManager));


    status = RtlInitializeGrowingList(
        &pManager->DefaultNamespaces,
        sizeof(NS_NAME_DEPTH),
        50,
        pManager->InlineDefaultNamespaces,
        sizeof(pManager->InlineDefaultNamespaces),
        Allocation);

    if (EFI_ERROR(status)) {
        return status;
    }

    status = RtlInitializeGrowingList(
        &pManager->Aliases,
        sizeof(NS_ALIAS),
        50,
        pManager->InlineAliases,
        sizeof(pManager->InlineAliases),
        Allocation);

    if (EFI_ERROR(status)) {
        return status;
    }

    pManager->pvCompareContext = pCompareContext;
    pManager->pfnCompare = pCompare;
    pManager->ulAliasCount = 0;

    //
    // Should be golden at this point, everything else is zero-initialized, so that's
    // just dandy.
    //
    return status;
}



EFI_STATUS
EFIAPI
RtlNsDestroy(
    PNS_MANAGER pManager
    )
{
    EFI_STATUS status = EFI_SUCCESS;

    if (!ARGUMENT_PRESENT(pManager))
    {
        return RtlpReportXmlError(EFI_INVALID_PARAMETER);
    }

    status = RtlDestroyGrowingList(&pManager->DefaultNamespaces);

    if (!EFI_ERROR(status))
    {
        status = RtlDestroyGrowingList(&pManager->Aliases);
    }

    return status;
}



EFI_STATUS
EFIAPI
RtlNsInsertNamespaceAlias(
    PNS_MANAGER     pManager,
    UINT32           ulDepth,
    PXML_EXTENT     Namespace,
    PXML_EXTENT     Alias
    )
{
    EFI_STATUS status = EFI_SUCCESS;
    PNS_NAME_DEPTH pNameDepth = NULL;
    PNS_ALIAS pNsAliasSlot = NULL;
    PNS_ALIAS pNsFreeSlot = NULL;
    UINT32 ul = 0;
    XML_STRING_COMPARE Equals = XML_STRING_COMPARE_EQUALS;


    //
    // Run through all the aliases we currently have, see if any of them are
    // the one we've got - in which case, we push-down a new namespace on that
    // alias.  As we're going, track to find a free slot, in case we don't find
    // it in the list
    //
    for (ul = 0; ul < pManager->ulAliasCount; ul++) {

        status = RtlIndexIntoGrowingList(
            &pManager->Aliases,
            ul,
            (VOID**)&pNsAliasSlot,
            FALSE);

        if (EFI_ERROR(status)) {
            goto Exit;
        }

        //
        // If we found a hole, stash it
        //
        if (!pNsAliasSlot->fInUse) {

            if (pNsFreeSlot == NULL)
                pNsFreeSlot = pNsAliasSlot;

            pNsAliasSlot = NULL;

        }
        //
        // Does this alias match?
        //
        else {

            status = pManager->pfnCompare(
                pManager->pvCompareContext,
                Alias,
                &pNsAliasSlot->AliasName,
                &Equals);

            if (EFI_ERROR(status)) {
                goto Exit;
            }

            //
            // Not equals, continue
            //
            if (Equals != XML_STRING_COMPARE_EQUALS) {
                pNsAliasSlot = NULL;
            }
            //
            // Otherwise, stop
            //
            else {
                break;
            }
        };
    }


    //
    // We didn't find the alias slot that this fits in to, so see if we can
    // find a free one and initialize it.
    //
    if (pNsAliasSlot == NULL) {

        //
        // Didn't find a free slot, either - add a new entry to the list
        // and go from there
        //
        if (pNsFreeSlot == NULL) {

            status = RtlIndexIntoGrowingList(
                &pManager->Aliases,
                pManager->ulAliasCount++,
                (VOID**)&pNsFreeSlot,
                TRUE);

            if (EFI_ERROR(status)) {
                goto Exit;
            }

            //
            // Init this, it just came out of the 'really free' list
            //
            ZeroMem(pNsFreeSlot, sizeof(*pNsFreeSlot));

            status = RtlInitializeGrowingList(
                &pNsFreeSlot->NamespaceMaps,
                sizeof(NS_NAME_DEPTH),
                20,
                pNsFreeSlot->InlineNamespaceMaps,
                sizeof(pNsFreeSlot->InlineNamespaceMaps),
                &pManager->Aliases.Allocator);

            if (EFI_ERROR(status)) {
                goto Exit;
            }
        }

        ASSERT(pNsFreeSlot != NULL);

        pNsAliasSlot = pNsFreeSlot;

        //
        // Zero init this one
        //
        pNsAliasSlot->fInUse = TRUE;
        pNsAliasSlot->ulNamespaceCount = 0;
        pNsAliasSlot->AliasName = *Alias;
    }

    //
    // Look at the current count in the slot - the top of the stack.
    // If its document depth matches the current document depth,
    // that's a redefinition of an alias <x xmlns:x="y" xmlns:x="y"/>
    // and is a document error.
    //
    if (pNsAliasSlot->ulNamespaceCount > 0) {

        status = RtlIndexIntoGrowingList(
            &pNsAliasSlot->NamespaceMaps,
            (pNsAliasSlot->ulNamespaceCount - 1),
            (VOID**)&pNameDepth,
            FALSE);

        if (EFI_ERROR(status)) {
            goto Exit;
        }

        if (pNsAliasSlot->fInUse && (pNameDepth->Depth == ulDepth)) {
            status = RtlpReportXmlError(STATUS_DUPLICATE_NAME);
            goto Exit;
        }
    }


    //
    // At this point, pNsAliasSlot points at the alias slot for which
    // we want to add a new depth thing.
    //
    status = RtlIndexIntoGrowingList(
        &pNsAliasSlot->NamespaceMaps,
        pNsAliasSlot->ulNamespaceCount++,
        (VOID**)&pNameDepth,
        TRUE);

    if (EFI_ERROR(status)) {
        goto Exit;
    }

    pNsAliasSlot->fInUse = TRUE;

    //
    // Good - now write the depth and the name into the
    // name-at-depth thing
    //
    pNameDepth->Depth = ulDepth;
    pNameDepth->Name = *Namespace;

Exit:
    return status;
}



EFI_STATUS
EFIAPI
RtlNsInsertDefaultNamespace(
    PNS_MANAGER     pManager,
    UINT32           ulDepth,
    PXML_EXTENT     pNamespace
    )
/*++

  Purpose:

    Adds the namespace mentioned in Namespace as the 'default' namespace
    for the depth given.  If a namespace already exists for the depth,
    it replaces it with this one.

  Parameters:

    pManager - Namespace management object to be updated.

    ulDepth - Depth at which this namespace should be active

    Namespace - Extent of the namespace name in the original XML document

  Returns:

    EFI_SUCCESS - Namespace was correctly made active at the depth in
        question.

    STATUS_NO_MEMORY - Unable to access the stack at that depth, possibly
        unable to extend the pseudostack of elements.

    STATUS_XML_PARSE_ERROR - Something else went wrong

    EFI_INVALID_PARAMETER - pManager was NULL.

--*/
{
    EFI_STATUS        status = EFI_SUCCESS;
    UINT32           ulStackTop;
    PNS_NAME_DEPTH  pCurrentStackTop;

    if ((pManager == NULL) || (ulDepth == 0)) {
        return RtlpReportXmlError(EFI_INVALID_PARAMETER);
    }

    ulStackTop = pManager->ulDefaultNamespaceDepth;

    if (ulStackTop == 0) {
        //
        // Simple push.
        //
        status = RtlIndexIntoGrowingList(
            &pManager->DefaultNamespaces,
            0,
            (VOID**)&pCurrentStackTop,
            TRUE);

        if (EFI_ERROR(status)) {
            return status;
        }

        //
        // Great, we now have an entry on the stack
        //
        pManager->ulDefaultNamespaceDepth++;
    }
    else {

        //
        // Find the current stack top in the list of namespaces.
        //
        status = RtlIndexIntoGrowingList(
            &pManager->DefaultNamespaces,
            ulStackTop - 1,
            (VOID**)&pCurrentStackTop,
            FALSE);

        if (EFI_ERROR(status)) {
            return status;
        }

        //
        // Potential coding error?
        //
        ASSERT(pCurrentStackTop->Depth <= ulDepth);

        //
        // If the depth at the top of the stack is more shallow than the new
        // depth requested, then insert a new stack item instead.
        //
        if (pCurrentStackTop->Depth < ulDepth) {

            status = RtlIndexIntoGrowingList(
                &pManager->DefaultNamespaces,
                ulStackTop,
                (VOID**)&pCurrentStackTop,
                TRUE);

            if (EFI_ERROR(status)) {
                return status;
            }

            pManager->ulDefaultNamespaceDepth++;;
        }
    }

    //
    // At this point, pCurrentStackTop should be non-null, and we
    // should be ready to write the new namespace element into the
    // stack just fine.
    //
    ASSERT(pCurrentStackTop != NULL);

    //
    // If the current depth matches the depth of the stack, then that's
    // a document error.
    //
    if (pCurrentStackTop->Depth == ulDepth) {
        return RtlpReportXmlError(STATUS_DUPLICATE_NAME);
    }

    pCurrentStackTop->Depth = ulDepth;
    pCurrentStackTop->Name = *pNamespace;

    return status;
}



EFI_STATUS
EFIAPI
RtlpRemoveDefaultNamespacesAboveDepth(
    PNS_MANAGER pManager,
    UINT32       ulDepth
    )
/*++

  Purpose:

    Cleans out all default namespaces that are above a certain depth in the
    namespace manager.  It does this iteratively, deleteing each one at the top
    of the stack until it finds one that's below the stack top.

  Parameters:

    pManager - Manager object to be cleaned out

    ulDepth - Depth at which and above the namespaces should be cleaned out.

  Returns:

    EFI_SUCCESS - Default namespace stack has been cleared out.

    * - Unknown failures in RtlIndexIntoGrowingList

--*/
{
    EFI_STATUS        status;
    PNS_NAME_DEPTH  pNsAtDepth = NULL;

    do
    {
        status = RtlIndexIntoGrowingList(
            &pManager->DefaultNamespaces,
            pManager->ulDefaultNamespaceDepth - 1,
            (VOID**)&pNsAtDepth,
            FALSE);

        if (EFI_ERROR(status)) {
            break;
        }

        //
        // Ok, found one that has to be toasted.  Delete it from the stack.
        //
        ASSERT(pNsAtDepth->Depth != NS_NAME_DEPTH_AVAILABLE);

        if (pNsAtDepth->Depth >= ulDepth) {
            pNsAtDepth->Depth = NS_NAME_DEPTH_AVAILABLE;
            pManager->ulDefaultNamespaceDepth--;
        }
        //
        // Otherwise, we're out of the deep water, so stop looking.
        //
        else {
            break;
        }
    }
    while (pManager->ulDefaultNamespaceDepth > 0);

    return status;
}




EFI_STATUS
EFIAPI
RtlpRemoveNamespaceAliasesAboveDepth(
    PNS_MANAGER pManager,
    UINT32       ulDepth
    )
/*++

  Purpose:

    Looks through the list of namespace aliases in this manager and removes the
    ones that are at or above a given depth.

  Parameters:

    pManager - Manager object from which the extra namespaces should be deleted

    ulDepth - Depth above which namespace aliases should be removed.

  Returns:

    EFI_SUCCESS - Correctly removed aliases above the specified depth

    * - Something else happened, potentially in RtlpIndexIntoGrowingList

--*/
{
    EFI_STATUS status = EFI_SUCCESS;
    UINT32 idx = 0;

    //
    // Note that the alias list is constructed such that it continually
    // grows, but deleteing namespace aliases can leave holes in the
    // list that can be filled in.  The ulAliasCount member of the namespace
    // manager is there to know what the high water mark of namespaces is,
    // above which we don't need to go to find valid aliases.  This value
    // is maintained in RtlNsInsertNamespaceAlias, but never cleared up.
    // A "potentially bad" situation could arise when a document with a lot of
    // namespace aliases at the second-level appears.
    //
    for (idx = 0; idx < pManager->ulAliasCount; idx++) {
        PNS_ALIAS pAlias;

        /* Get the namespace alias entry */
        status = RtlIndexIntoGrowingList(
            &pManager->Aliases,
            idx,
            (VOID* *)&pAlias,
            FALSE);

        if (EFI_ERROR(status))
            return status;

        if (pAlias->fInUse)
        {
            UINT32 jdx;

            for (jdx = 0; jdx < pAlias->ulNamespaceCount; jdx++)
            {
                PNS_NAME_DEPTH pNameDepth;

                /* Get the name and depth for each mapping of this alias */
                status = RtlIndexIntoGrowingList(
                    &pAlias->NamespaceMaps,
                    jdx,
                    (VOID* *)&pNameDepth,
                    FALSE);

                if (EFI_ERROR(status))
                    return status;

                /*
                 * Stop when we hit a depth greater than the depth that was
                 * passed in.  We assume that the namespace list is in order
                 * of increasing depth, so everything after this in the list
                 * should be lopped off.
                 */
                if (pNameDepth->Depth >= ulDepth)
                    break;
            }

            pAlias->ulNamespaceCount = jdx;

            /* No point in keeping around an alias with no mapping */
            if (pAlias->ulNamespaceCount == 0)
                pAlias->fInUse = FALSE;
        }
    }

    return status;
}




EFI_STATUS
EFIAPI
RtlNsLeaveDepth(
    PNS_MANAGER pManager,
    UINT32       ulDepth
    )
{
    EFI_STATUS status = EFI_SUCCESS;

    if (pManager == NULL) {
        return RtlpReportXmlError(EFI_INVALID_PARAMETER);
    }

    //
    // Meta-question.  Should we try to clean up both the alias list as well
    // as the default namespace list before we return failure to the caller?
    // I suppose we should, but a failure in either of these is bad enough to
    // leave the namespace manager in a bad way.
    //
    if (pManager->ulDefaultNamespaceDepth > 0) {
        status = RtlpRemoveDefaultNamespacesAboveDepth(pManager, ulDepth);
        if (EFI_ERROR(status)) {
            return status;
        }
    }

    if (pManager->ulAliasCount > 0) {
        status = RtlpRemoveNamespaceAliasesAboveDepth(pManager, ulDepth);
        if (EFI_ERROR(status)) {
            return status;
        }
    }

    return status;
}



EFI_STATUS
EFIAPI
RtlpNsFindMatchingAlias(
    PNS_MANAGER     pManager,
    PXML_EXTENT     pAliasName,
    PNS_ALIAS      *pAlias
    )
{
    EFI_STATUS    status = EFI_SUCCESS;
    UINT32       idx = 0;

    *pAlias = NULL;

    for (idx = 0; idx < pManager->ulAliasCount; idx++) {

        XML_STRING_COMPARE Matches = XML_STRING_COMPARE_EQUALS;
        PNS_ALIAS pCandidateAlias = NULL;

        status = RtlIndexIntoGrowingList(
            &pManager->Aliases,
            idx,
            (VOID**)&pCandidateAlias,
            FALSE
            );

        //
        // This shouldn't fail - we are within the boundaries set
        // in the alias count, and we're not attempting to grow
        // the array at all.
        //
        if(EFI_ERROR(status))
            return status;

        //
        // If this slot is in use...
        //
        if (pCandidateAlias->fInUse) {

            status = pManager->pfnCompare(
                pManager->pvCompareContext,
                &pCandidateAlias->AliasName,
                pAliasName,
                &Matches
                );

            if (EFI_ERROR(status)) {
                return status;
            }

            //
            // This alias matches the alias in the list
            //
            if (Matches == XML_STRING_COMPARE_EQUALS) {
                ASSERT(pCandidateAlias->fInUse);
                *pAlias = pCandidateAlias;
                return EFI_SUCCESS;
            }
        }
    }

    ASSERT(*pAlias == NULL);

    return EFI_SUCCESS;
}






EFI_STATUS
RtlNsGetNamespaceForAlias(
    PNS_MANAGER     pManager,
    UINT32           ulDepth,
    PXML_EXTENT     Alias,
    PXML_EXTENT     pNamespace
    )
{
    EFI_STATUS status = EFI_SUCCESS;

    if ((pManager == NULL) || (Alias == NULL) || (pNamespace == NULL)) {
        return RtlpReportXmlError(EFI_INVALID_PARAMETER);
    }

    ZeroMem(pNamespace, sizeof(*pNamespace));

    //
    // No prefix, get the active default namespace
    //
    if (Alias->cbData == 0) {

        PNS_NAME_DEPTH pDefault = NULL;

        //
        // There's default namespaces
        //
        if (pManager->ulDefaultNamespaceDepth != 0) {

            status = RtlIndexIntoGrowingList(
                &pManager->DefaultNamespaces,
                pManager->ulDefaultNamespaceDepth - 1,
                (VOID**)&pDefault,
                FALSE);

            if (EFI_ERROR(status)) {
                goto Exit;
            }

            //
            // Coding error - asking for depths below the depth at the top of
            // the default stack
            //
            ASSERT(pDefault->Depth <= ulDepth);
        }

        //
        // We've found a default namespace that suits us
        //
        if (pDefault != NULL) {
            *pNamespace = pDefault->Name;
        }

        status = EFI_SUCCESS;

    }
    //
    // Otherwise, look through the list of aliases active
    //
    else {

        PNS_ALIAS pThisAlias = NULL;
        PNS_NAME_DEPTH pNamespaceFound = NULL;

        //
        // This can return "status not found", which is fine
        //
        status = RtlpNsFindMatchingAlias(pManager, Alias, &pThisAlias);
        if (EFI_ERROR(status)) {
            goto Exit;
        }

        if (pThisAlias == NULL) {
            status = RtlpReportXmlError(STATUS_NOT_FOUND);
            goto Exit;
        }


        //
        // The one we found must be in use, and may not be empty
        //
        ASSERT(pThisAlias->fInUse && pThisAlias->ulNamespaceCount);

        //
        // Look at the topmost aliased namespace
        //
        status = RtlIndexIntoGrowingList(
            &pThisAlias->NamespaceMaps,
            pThisAlias->ulNamespaceCount - 1,
            (VOID**)&pNamespaceFound,
            FALSE);

        if (EFI_ERROR(status)) {
            goto Exit;
        }

        //
        // Coding error, asking for stuff that's below the depth found
        //
        ASSERT(pNamespaceFound && (pNamespaceFound->Depth <= ulDepth));

        //
        // Outbound
        //
        *pNamespace = pNamespaceFound->Name;
    }

    status = EFI_SUCCESS;

Exit:
    return status;
}
