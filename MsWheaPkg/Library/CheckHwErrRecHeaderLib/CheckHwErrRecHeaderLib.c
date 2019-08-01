/**@file CheckHwErrRecHeaderLib.c

Validates that size fields within HWErrRecXXXX headers are within bounds. After validation, utilizing size
and offset fields within the common header and section header(s) is safe

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Base.h>

#include <Guid/Cper.h>

#include <Library/DebugLib.h>
#include <Library/CheckHwErrRecHeaderLib.h>
#include <Library/SafeIntLib.h>

/**
 *  Checks that all length and offset fields within the HWErrRec fall within the bounds of
 *  the buffer, all section data is accounted for in their respective section headers, and that
 *  all section data is contiguous
 * 
 *  @param[in]  Err              -    Pointer to HWErr record being checked
 *  @param[in]  Size             -    Size obtained from calling GetVariable() to obtain the record
 *
 *  @retval     BOOLEAN          -    TRUE if all length and offsets are safe
 *                                    FALSE otherwise
**/
BOOLEAN
EFIAPI
ValidateCperHeader(
    IN CONST EFI_COMMON_ERROR_RECORD_HEADER                 *Err, 
    IN CONST UINTN                                           Size
    )
{

    EFI_ERROR_SECTION_DESCRIPTOR      *SectionHeader        = (EFI_ERROR_SECTION_DESCRIPTOR*) (Err + 1);
    UINTN                              SecLenPlusOffset;

    // Make sure Err is safe to access
    if(Err == NULL)
    {
        DEBUG((DEBUG_ERROR,"%a : Pointer passed in to function is Null\n", __FUNCTION__));
        return FALSE;
    }

    // Make sure the size is at least as large as the size of a Record Header
    if(Size < sizeof(EFI_COMMON_ERROR_RECORD_HEADER)) {
        DEBUG((DEBUG_ERROR,"%a : Size of HWErrRec is smaller than the size of a CPER Header\n", __FUNCTION__));
        return FALSE;
    }

    // Check that signature matches CPER
    if (Err->SignatureStart != EFI_ERROR_RECORD_SIGNATURE_START) {
        DEBUG((DEBUG_ERROR, "%a : HWErrRec had an incorrect signature\n",__FUNCTION__));
        return FALSE;
    }

    // Make sure the length of the record matches the record length field
    if(Size != Err->RecordLength) {
        DEBUG((DEBUG_ERROR, "%a : Size of HWErrRec is not equal to Record Length field of CPER Header\n",__FUNCTION__));
        return FALSE;
    }

    // Make sure the size is large enough to contain the specified number of section headers
    // NOTE: Err->SectionCount is a UINT16, so we can be almost certain this will not overflow on 32 or 64 bit architectures
    if(Size < sizeof(EFI_COMMON_ERROR_RECORD_HEADER) + (sizeof(EFI_ERROR_SECTION_DESCRIPTOR) * Err->SectionCount)) {
        DEBUG((DEBUG_ERROR, "%a : Size of HWErrRec is less than the number of section headers specified in SectionCount\n",__FUNCTION__));
        return FALSE;
    }

    SecLenPlusOffset = sizeof(EFI_COMMON_ERROR_RECORD_HEADER) + (sizeof(EFI_ERROR_SECTION_DESCRIPTOR) * Err->SectionCount);

    // For each section header, make sure the offset and length is still within the size of the error record
    for(UINT16 i = 0; i < Err->SectionCount; i++) {

        // Make sure the sections are contiguous
        // NOTE: EFI_ERROR_SECTION_DESCRIPTOR and EFI_COMMON_ERROR_RECORD_HEADER are #pragma pack(1)
        if(SectionHeader->SectionOffset != SecLenPlusOffset) {
            DEBUG((DEBUG_ERROR,"%a : The beginning of Section %d is not at the expected offset. Expected: 0x%X, Actual: 0x%X\n",__FUNCTION__, i + 1,
                    SecLenPlusOffset,SectionHeader->SectionOffset));
            return FALSE;
        }

        // If the length is zero it is possible to read a byte that should not be read
        if(SectionHeader->SectionLength == 0) {
            DEBUG((DEBUG_ERROR,"%a : Section %d has length zero\n",__FUNCTION__, i + 1));
            return FALSE;
        }

        // Check if the offset of this section is not in a reasonable position
        if(((UINTN) Err + (UINTN) SectionHeader->SectionOffset) < ((UINTN) SectionHeader + sizeof(EFI_ERROR_SECTION_DESCRIPTOR))) {
            DEBUG((DEBUG_ERROR,"%a : Section %d data offset is within or before the section header itself\n",__FUNCTION__, i + 1));
            return FALSE;
        }

        // Avoid integer overflow
        if(EFI_ERROR(SafeUintnAdd(SectionHeader->SectionLength, SectionHeader->SectionOffset, &SecLenPlusOffset))) {
            DEBUG((DEBUG_ERROR,"%a : Integer overflow of Section Length + Section Offset field of Section %d\n",__FUNCTION__, i + 1));
            return FALSE;
        }
        
        // Ensure section length and offset addition do not run off the end of the buffer
        if(Size < SecLenPlusOffset) {
            DEBUG((DEBUG_ERROR,"%a : Size of HWErrRec is less than the Section Length + Section Offset field of Section %d\n",__FUNCTION__, i + 1));
            return FALSE;
        }

        SectionHeader += 1;
    }

    // Make sure there is no data after the last section
    if(Size != SecLenPlusOffset) {
        DEBUG((DEBUG_ERROR,"%a : The size of the buffer extends past the end of the section data\n",__FUNCTION__));
        return FALSE;
    }
    
    return TRUE;
}
