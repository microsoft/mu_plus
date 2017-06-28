/** @file
 *
  Master header file for the GuidedSectionExtract PEIM. All source files in this module should
  include this file for common definitions.

Copyright (c) 2006 - 2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __PEI_GUIDEDSECTIONEXTRACT_H__
#define __PEI_GUIDEDSECTIONEXTRACT_H__

#include <PiPei.h>
#include <Ppi/Decompress.h>
#include <Ppi/FirmwareVolumeInfo.h>
#include <Ppi/GuidedSectionExtraction.h>

#include <Library/DebugLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/BaseLib.h>
#include <Library/HobLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/UefiDecompressLib.h>
#include <Library/ExtractGuidedSectionLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/S3Lib.h>
#include <Library/RecoveryLib.h>
#include <Library/DebugAgentLib.h>
#include <Library/PeiServicesTablePointerLib.h>


/**
  The ExtractSection() function processes the input section and
  returns a pointer to the section contents. If the section being
  extracted does not require processing (if the section
  GuidedSectionHeader.Attributes has the
  EFI_GUIDED_SECTION_PROCESSING_REQUIRED field cleared), then
  OutputBuffer is just updated to point to the start of the
  section's contents. Otherwise, *Buffer must be allocated
  from PEI permanent memory.

  @param This                   Indicates the
                                EFI_PEI_GUIDED_SECTION_EXTRACTION_PPI instance.
                                Buffer containing the input GUIDed section to be
                                processed. OutputBuffer OutputBuffer is
                                allocated from PEI permanent memory and contains
                                the new section stream.
  @param InputSection           A pointer to the input buffer, which contains
                                the input section to be processed.
  @param OutputBuffer           A pointer to a caller-allocated buffer, whose
                                size is specified by the contents of OutputSize.
  @param OutputSize             A pointer to a caller-allocated
                                UINTN in which the size of *OutputBuffer
                                allocation is stored. If the function
                                returns anything other than EFI_SUCCESS,
                                the value of OutputSize is undefined.
  @param AuthenticationStatus   A pointer to a caller-allocated
                                UINT32 that indicates the
                                authentication status of the
                                output buffer. If the input
                                section's GuidedSectionHeader.
                                Attributes field has the
                                EFI_GUIDED_SECTION_AUTH_STATUS_VALID
                                bit as clear,
                                AuthenticationStatus must return
                                zero. These bits reflect the
                                status of the extraction
                                operation. If the function
                                returns anything other than
                                EFI_SUCCESS, the value of
                                AuthenticationStatus is
                                undefined.

  @retval EFI_SUCCESS           The InputSection was
                                successfully processed and the
                                section contents were returned.

  @retval EFI_OUT_OF_RESOURCES  The system has insufficient
                                resources to process the request.

  @retval EFI_INVALID_PARAMETER The GUID in InputSection does
                                not match this instance of the
                                GUIDed Section Extraction PPI.

**/
EFI_STATUS
EFIAPI
CustomGuidedSectionExtract (
  IN CONST  EFI_PEI_GUIDED_SECTION_EXTRACTION_PPI *This,
  IN CONST  VOID                                  *InputSection,
  OUT       VOID                                  **OutputBuffer,
  OUT       UINTN                                 *OutputSize,
  OUT       UINT32                                *AuthenticationStatus
  );


/**
   Decompresses a section to the output buffer.

   This function looks up the compression type field in the input section and
   applies the appropriate compression algorithm to compress the section to a
   callee allocated buffer.

   @param  This                  Points to this instance of the
                                 EFI_PEI_DECOMPRESS_PEI PPI.
   @param  CompressionSection    Points to the compressed section.
   @param  OutputBuffer          Holds the returned pointer to the decompressed
                                 sections.
   @param  OutputSize            Holds the returned size of the decompress
                                 section streams.

   @retval EFI_SUCCESS           The section was decompressed successfully.
                                 OutputBuffer contains the resulting data and
                                 OutputSize contains the resulting size.

**/
EFI_STATUS
EFIAPI
Decompress (
  IN CONST  EFI_PEI_DECOMPRESS_PPI  *This,
  IN CONST  EFI_COMPRESSION_SECTION *CompressionSection,
  OUT       VOID                    **OutputBuffer,
  OUT       UINTN                   *OutputSize
  );

#endif // __PEI_GUIDEDSECTIONEXTRACT_H__
