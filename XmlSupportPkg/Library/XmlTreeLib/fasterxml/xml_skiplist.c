/**
XML list implemenation for parsing.

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


#include <Uefi.h>                   // UEFI base types
#include <Library/BaseMemoryLib.h>  // SetMem()
#include "fasterxml.h"              // XML Engine
#include "xmlerr.h"                 // XML Errors.
#include "xmlstructure.h"           // XML structures

EFI_STATUS
RtlpFindChunkForElementIndex(
    PRTL_GROWING_LIST        pList,
    UINT32                    ulIndex,
    PRTL_GROWING_LIST_CHUNK *ppListChunk,
    UINT32                  *pulChunkOffset
    )
/*++


  Purpose:

    Finds the chunk for the given index.  This could probably be made faster if
    and when we start using skiplists.  As it stands, we just have to walk through
    the list until the index looked for is inside one of the lists.

  Parameters:

    pList - Growing list management structure

    ulIndex - Index requested by the caller

    ppListChunk - Pointer to a pointer to a list chunk.  On return, points to
        the list chunk containing the index.

    pulChunkOffset - Offset into the chunk (in elements) that was requested

  Returns:

    EFI_SUCCESS - Chunk was found, ppListChunk and pulChunkOffset point to
        the values listed in the 'parameters' section.

    STATUS_NOT_FOUND - The index was beyond the end of the chunk sections.

--*/
{
    PRTL_GROWING_LIST_CHUNK pHere = NULL;

    if (ppListChunk) {
        *ppListChunk = NULL;
    }

    if (pulChunkOffset) {
        *pulChunkOffset = 0;
    }

    if (!ppListChunk || !pList || (ulIndex < pList->cInternalElements)) {
        return RtlpReportXmlError(EFI_INVALID_PARAMETER);
    }

    //
    // Chop off the number of elements in the internal list
    //
    ulIndex -= pList->cInternalElements;


    //
    // Move through list chunks until the index is inside one
    // of them.  A smarter bear would have made all the chunks the
    // same size and could then have just skipped ahead the right
    // number, avoiding comparisons.
    //
    pHere = pList->pFirstChunk;

    while ((ulIndex >= pList->cElementsPerChunk) && pHere) {
        pHere = pHere->pNextChunk;
        ulIndex -= pList->cElementsPerChunk;
    }

    //
    // Set pointer over
    //
    if (ulIndex < pList->cElementsPerChunk) {
        *ppListChunk = pHere;
    }

    //
    // And if the caller cared what chunk this was in, then tell them.
    //
    if (pulChunkOffset && *ppListChunk) {
        *pulChunkOffset = ulIndex;
    }

    return pHere ? EFI_SUCCESS : STATUS_NOT_FOUND;
}




EFI_STATUS
RtlInitializeGrowingList(
    PRTL_GROWING_LIST       pList,
    UINT32                  cbElementSize,
    UINT32                   cElementsPerChunk,
    VOID*                   pvInitialListBuffer,
    UINT32                  cbInitialListBuffer,
    PRTL_ALLOCATOR          Allocation
    )
{

    if ((pList == NULL) ||
        (cElementsPerChunk == 0) ||
        (cbElementSize == 0))
    {
        return RtlpReportXmlError(EFI_INVALID_PARAMETER);
    }


    ZeroMem(pList, sizeof(*pList));

    pList->cbElementSize        = cbElementSize;
    pList->cElementsPerChunk    = cElementsPerChunk;
    pList->Allocator            = *Allocation;

    //
    // Set up  the initial list pointer
    //
    if (pvInitialListBuffer != NULL) {

        pList->pvInternalList = pvInitialListBuffer;

        // Conversion downwards to a ulong, but it's still valid, right?
        pList->cInternalElements = (UINT32)(cbInitialListBuffer / cbElementSize);

        pList->cTotalElements = pList->cInternalElements;

    }

    return EFI_SUCCESS;
}





EFI_STATUS
RtlpExpandGrowingList(
    PRTL_GROWING_LIST       pList,
    UINT32                   ulMinimalIndexCount
    )
/*++

  Purpose:

    Given a growing list, expand it to be able to contain at least
    ulMinimalIndexCount elements.  Does this by allocating chunks via the
    allocator in the list structure and adding them to the growing list
    chunk set.

  Parameters:

    pList - Growing list structure to be expanded

    ulMinimalIndexCount - On return, the pList will have at least enough
        slots to contain this many elements.

  Return codes:

    EFI_SUCCESS - Enough list chunks were allocated to hold the
        requested number of elements.

    STATUS_NO_MEMORY - Ran out of memory during allocation.  Any allocated
        chunks were left allocated and remain owned by the growing list
        until destruction.

    EFI_INVALID_PARAMETER - pList was NULL or invalid.

--*/
{
    EFI_STATUS status = EFI_SUCCESS;
    UINT32 ulNecessaryChunks = 0;
    UINT32 ulExtraElements = ulMinimalIndexCount;
    UINT32 UINT8sInChunk;

    if ((pList == NULL) || (pList->Allocator.pfnAlloc == NULL)) {
        return RtlpReportXmlError(EFI_INVALID_PARAMETER);
    }

    //
    // Already got enough elements in the list?  Great.  The caller
    // was a bit overactive.
    //
    if (pList->cTotalElements > ulMinimalIndexCount) {
        return EFI_SUCCESS;
    }

    //
    // Whack off the number of elements already on the list.
    //
    ulExtraElements -= pList->cTotalElements;

    //
    // How many chunks is that?  Remember to round up.
    //
    ulNecessaryChunks = ulExtraElements / pList->cElementsPerChunk;
    ulNecessaryChunks++;

    //
    // Let's go allocate them, one by one
    //
    if (pList->cbElementSize != 0)
    {
        //if (((MAXUINT32 - sizeof(RTL_GROWING_LIST_CHUNK)) / pList->cbElementSize) < pList->cElementsPerChunk)
        //    return RtlpReportXmlError(EFI_INVALID_PARAMETER);
    }

    UINT8sInChunk = (pList->cbElementSize * pList->cElementsPerChunk) + sizeof(RTL_GROWING_LIST_CHUNK);

    while (ulNecessaryChunks--) {

        PRTL_GROWING_LIST_CHUNK pNewChunk = NULL;

        //
        // Allocate some memory for the chunk
        //
        status = pList->Allocator.pfnAlloc(UINT8sInChunk, (VOID**)&pNewChunk, pList->Allocator.pvContext);
        if (EFI_ERROR(status)) {
            return RtlpReportXmlError(EFI_OUT_OF_RESOURCES);
        }

        //
        // Set up the new chunk
        //
        pNewChunk->pGrowingListParent = pList;
        pNewChunk->pNextChunk = NULL;

        if (pList->pLastChunk) {
            //
            // Swizzle the list of chunks to include this one
            //
            pList->pLastChunk->pNextChunk = pNewChunk;
        }

        pList->pLastChunk = pNewChunk;
        pList->cTotalElements += pList->cElementsPerChunk;

        //
        // If there wasn't a first chunk, this one is.
        //
        if (pList->pFirstChunk == NULL) {
            pList->pFirstChunk = pNewChunk;
        }
    }

    return EFI_SUCCESS;

}







EFI_STATUS
RtlIndexIntoGrowingList(
    PRTL_GROWING_LIST       pList,
    UINT32                   ulIndex,
    VOID*                  *ppvPointerToSpace,
    BOOLEAN                 fGrowingAllowed
    )
{
    EFI_STATUS status = EFI_SUCCESS;

    if ((pList == NULL) || (ppvPointerToSpace == NULL)) {
        return RtlpReportXmlError(EFI_INVALID_PARAMETER);
    }

    *ppvPointerToSpace = NULL;

    //
    // If the index is beyond the current total number of elements, but we're
    // not allowing growing, then say it wasn't found.  Otherwise, we'll always
    // grow the array as necessary to contain the index passed.
    //
    if ((ulIndex >= pList->cTotalElements) && !fGrowingAllowed) {
        return RtlpReportXmlError(STATUS_NOT_FOUND);
    }

    //
    // This element is in the internal list, so just figure out where
    // and point at it.  Do this only if there's an internal element
    // list.
    //
    if ((ulIndex < pList->cInternalElements) && pList->cInternalElements) {

        //
        // The pointer to the space they want is ulIndex*pList->cbElementSize
        // UINT8s down the pointer pList->pvInternalList
        //
        *ppvPointerToSpace = ((UINT8*)(pList->pvInternalList)) + (ulIndex * pList->cbElementSize);
        return EFI_SUCCESS;
    }
    //
    // Otherwise, the index is outside the internal list, find out which one
    // it was supposed to be in.
    //
    else {

        PRTL_GROWING_LIST_CHUNK pThisChunk = NULL;
        UINT32 ulNewOffset = 0;
        UINT8* pbData = NULL;

        status = RtlpFindChunkForElementIndex(pList, ulIndex, &pThisChunk, &ulNewOffset);

        //
        // Success! Go move the chunk pointer past the header of the growing list
        // chunk, and then index off it to find the right place.
        //
        if (NT_SUCCESS(status)) {

            pbData = ((UINT8*)(pThisChunk + 1)) + (pList->cbElementSize * ulNewOffset);

        }
        //
        // Otherwise, the chunk wasn't found, so we have to go allocate some new
        // chunks to hold it, then try again.
        //
        else if (status == STATUS_NOT_FOUND) {

            //
            // Expand the list
            //
            if (EFI_ERROR(status = RtlpExpandGrowingList(pList, ulIndex))) {
                goto Exit;
            }

            //
            // Look again
            //
            status = RtlpFindChunkForElementIndex(pList, ulIndex, &pThisChunk, &ulNewOffset);
            if (EFI_ERROR(status)) {
                goto Exit;
            }

            //
            // Adjust pointers
            //
            pbData = ((UINT8*)(pThisChunk + 1)) + (pList->cbElementSize * ulNewOffset);


        }
        else {
            goto Exit;
        }

        //
        // One of the above should have set the pbData pointer to point at the requested
        // grown-list space.
        //
        *ppvPointerToSpace = pbData;


    }


Exit:
    return status;
}







EFI_STATUS
RtlDestroyGrowingList(
    PRTL_GROWING_LIST       pList
    )
/*++

  Purpose:

    Destroys (deallocates) all the chunks that had been allocated to this
    growing list structure.  Returns the list to the "fresh" state of having
    only the 'internal' element count.

  Parameters:

    pList - List structure to be destroyed

  Returns:

    EFI_SUCCESS - Structure was completely cleaned out

--*/
{
    EFI_STATUS status = EFI_SUCCESS;

    //
    // Fails if the list is null, or there's things to free but there's no freer.
    //
    if ((pList == NULL) || ((pList->pFirstChunk != NULL) && (pList->Allocator.pfnFree == NULL))) {
        return RtlpReportXmlError(EFI_INVALID_PARAMETER);
    }

    //
    // Zing through and kill all the list bits
    //
    while (pList->pFirstChunk != NULL) {

        PRTL_GROWING_LIST_CHUNK pHere;

        pHere = pList->pFirstChunk;
        pList->pFirstChunk = pList->pFirstChunk->pNextChunk;

        if (EFI_ERROR(status = pList->Allocator.pfnFree(pHere, pList->Allocator.pvContext))) {
            return status;
        }

        pList->cTotalElements -= pList->cElementsPerChunk;
    }

    ASSERT(pList->pFirstChunk == NULL);

    //
    // Reset the things that change as we expand the list
    //
    pList->pLastChunk = pList->pFirstChunk = NULL;
    pList->cTotalElements = pList->cInternalElements;

    return status;
}


EFI_STATUS
RtlCloneGrowingList(
    UINT32                   ulFlags,
    PRTL_GROWING_LIST       pDestination,
    PRTL_GROWING_LIST       pSource,
    UINT32                   ulSourceCount
    )
{
    EFI_STATUS status = EFI_SUCCESS;
    UINT32 ul;
    VOID* pvSourceCursor;
    VOID* pvDestCursor;
    UINT32 cbUINT8s;

    //
    // No flags, no null values, element UINT8 size has to match,
    // and the source/dest can't be the same.
    //
    if (((ulFlags != 0) || !pDestination || !pSource) ||
        (pDestination->cbElementSize != pSource->cbElementSize) ||
        (pDestination == pSource))
        return RtlpReportXmlError(EFI_INVALID_PARAMETER);

    cbUINT8s = pDestination->cbElementSize;

    //
    // Now copy UINT8s around
    //
    for (ul = 0; ul < ulSourceCount; ul++) {
        status = RtlIndexIntoGrowingList(pSource, ul, &pvSourceCursor, FALSE);
        if (EFI_ERROR(status))
            goto Exit;

        status = RtlIndexIntoGrowingList(pDestination, ul, &pvDestCursor, TRUE);
        if (EFI_ERROR(status))
            goto Exit;

        CopyMem(pvDestCursor, pvSourceCursor, cbUINT8s);
    }

    status = EFI_SUCCESS;
Exit:
    return status;
}




EFI_STATUS
RtlAllocateGrowingList(
    PRTL_GROWING_LIST  *ppGrowingList,
    UINT32              cbThingSize,
    PRTL_ALLOCATOR      Allocation
    )
{
    PRTL_GROWING_LIST pvWorkingList = NULL;
    EFI_STATUS status = EFI_SUCCESS;

    if (ppGrowingList != NULL)
        *ppGrowingList = NULL;
    else
        return RtlpReportXmlError(EFI_INVALID_PARAMETER_1);

    if (!Allocation)
        return RtlpReportXmlError(EFI_INVALID_PARAMETER_3);

    //
    // Allocate space
    //
    status = Allocation->pfnAlloc(sizeof(RTL_GROWING_LIST), (VOID**)&pvWorkingList, Allocation->pvContext);
    if (EFI_ERROR(status)) {
        goto Exit;
    }

    //
    // Set up the structure
    //
    status = RtlInitializeGrowingList(
        pvWorkingList,
        cbThingSize,
        8,
        NULL,
        0,
        Allocation);

    if (EFI_ERROR(status)) {
        goto Exit;
    }

    *ppGrowingList = pvWorkingList;
    pvWorkingList = NULL;
    status = EFI_SUCCESS;
Exit:
    if (pvWorkingList) {
        Allocation->pfnFree(pvWorkingList, Allocation->pvContext);
    }

    return status;

}





EFI_STATUS
RtlSearchGrowingList(
    PRTL_GROWING_LIST TheList,
    UINT32 ItemCount,
    PFN_LIST_COMPARISON_CALLBACK SearchCallback,
    VOID* SearchTarget,
    VOID* SearchContext,
    VOID* *pvFoundItem
    )
{
    EFI_STATUS status = EFI_SUCCESS;
    UINT32 ul;
    int CompareResult = 0;

    if (pvFoundItem)
        *pvFoundItem = NULL;

//    if (TheList->ulFlags & GROWING_LIST_FLAG_IS_SORTED) {
    if (0) {
    }
    else {

        UINT32 uOffset = 0;
        PRTL_GROWING_LIST_CHUNK Chunklet;

        ul = 0;

        //
        // Scan the internal item list.
        //
        while ((ul < ItemCount) && (ul < TheList->cInternalElements)) {

            VOID* pvHere = (VOID*)(((UINTN)TheList->pvInternalList) + uOffset);

            status = SearchCallback(TheList, SearchTarget, pvHere, SearchContext, &CompareResult);
            if (EFI_ERROR(status)) {
                goto Exit;
            }

            if (CompareResult == 0) {
                if (pvFoundItem)
                    *pvFoundItem = pvHere;
                status = EFI_SUCCESS;
                goto Exit;
            }

            uOffset += TheList->cbElementSize;
            ul++;
        }

        //
        // Ok, we ran out of internal elements, do the same thing here but on the chunk list
        //
        Chunklet = TheList->pFirstChunk;
        while ((ul < ItemCount) && Chunklet) {

            VOID* Data = (VOID*)(Chunklet + 1);
            UINT32 ulHighOffset = TheList->cElementsPerChunk * TheList->cbElementSize;

            uOffset = 0;

            //
            // Spin through the items in this chunklet
            //
            while (uOffset < ulHighOffset) {

                VOID* pvHere = (VOID*)(((UINTN)Data) + uOffset);

                status = SearchCallback(TheList, SearchTarget, pvHere, SearchContext, &CompareResult);
                if (EFI_ERROR(status)) {
                    goto Exit;
                }

                if (CompareResult == 0) {
                    if (pvFoundItem)
                        *pvFoundItem = pvHere;
                    status = EFI_SUCCESS;
                    goto Exit;
                }

                uOffset += TheList->cbElementSize;
            }

        }

        //
        // If we got here, we didn't find it in either the internal list or the external one.
        //
        status = STATUS_NOT_FOUND;
        if (pvFoundItem)
            *pvFoundItem = NULL;

    }

Exit:
    return status;
}


