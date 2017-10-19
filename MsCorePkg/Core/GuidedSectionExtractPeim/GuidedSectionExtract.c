/** @file

Copyright (c) 2006 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "GuidedSectionExtract.h"


CONST EFI_PEI_GUIDED_SECTION_EXTRACTION_PPI mCustomGuidedSectionExtractionPpi = {
  CustomGuidedSectionExtract
};

CONST EFI_PEI_DECOMPRESS_PPI mDecompressPpi = {
  Decompress
};

CONST EFI_PEI_PPI_DESCRIPTOR mPpiList[] = {
  {
    (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiPeiDecompressPpiGuid,
    (VOID *) &mDecompressPpi
  }
};

/**
  Entry point of JASTST PEIM.
  
  @param  FileHandle  Handle of the file being invoked.
  @param  PeiServices Describes the list of possible PEI Services.

  @retval EFI_SUCESS  The entry point of DXE IPL PEIM executes successfully.
  @retval Others      Some error occurs during the execution of this function. 

**/
EFI_STATUS
EFIAPI
PeimInitializeGuidedSectionExtract (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{

  EFI_STATUS                                Status;
  EFI_BOOT_MODE                             BootMode;
  EFI_GUID                                  *ExtractHandlerGuidTable;
  UINTN                                     ExtractHandlerNumber;
  EFI_PEI_PPI_DESCRIPTOR                    *GuidPpi;
  
  Status = EFI_SUCCESS;

  BootMode = GetBootModeHob();
  if (BootMode != BOOT_ON_S3_RESUME) {

    //
    // EFI_SUCESS means it is the first time to call register for shadow. 
    // 
    Status = PeiServicesRegisterForShadow(FileHandle);
    if (Status == EFI_SUCCESS) {
      return Status;
    }
    
    //
    // Ensure that PEIM is shadowed to permanent memory.
    //
    ASSERT (Status == EFI_ALREADY_STARTED);
  }
     
  //
  // Get custom extract guided section method guid list 
  //
  ExtractHandlerNumber = ExtractGuidedSectionGetGuidList (&ExtractHandlerGuidTable);
  
  //
  // Install custom extraction guid PPI
  //
  if (ExtractHandlerNumber > 0) {
    GuidPpi = (EFI_PEI_PPI_DESCRIPTOR *) AllocatePool (ExtractHandlerNumber * sizeof (EFI_PEI_PPI_DESCRIPTOR));
    ASSERT (GuidPpi != NULL);
    while (ExtractHandlerNumber-- > 0) {
      GuidPpi->Flags = EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST;
      GuidPpi->Ppi   = (VOID *) &mCustomGuidedSectionExtractionPpi;
      GuidPpi->Guid  = &ExtractHandlerGuidTable[ExtractHandlerNumber];
      Status = PeiServicesInstallPpi (GuidPpi++);
      ASSERT_EFI_ERROR(Status);
    }
  }
  
  //
  // Install DxeIpl and Decompress PPIs.
  //
  Status = PeiServicesInstallPpi (mPpiList);
  ASSERT_EFI_ERROR(Status);

  return Status;
}

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
  )
{
  EFI_STATUS      Status;
  UINT8           *ScratchBuffer;
  UINT32          ScratchBufferSize;
  UINT32          OutputBufferSize;
  UINT16          SectionAttribute;
  
  //
  // Init local variable
  //
  ScratchBuffer = NULL;

  //
  // Call GetInfo to get the size and attribute of input guided section data.
  //
  Status = ExtractGuidedSectionGetInfo (
             InputSection,
             &OutputBufferSize,
             &ScratchBufferSize,
             &SectionAttribute
             );
  
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "GetInfo from guided section Failed - %r\n", Status));
    return Status;
  }
  
  if (ScratchBufferSize != 0) {
    //
    // Allocate scratch buffer
    //
    ScratchBuffer = AllocatePages (EFI_SIZE_TO_PAGES (ScratchBufferSize));
    if (ScratchBuffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  }

  if (((SectionAttribute & EFI_GUIDED_SECTION_PROCESSING_REQUIRED) != 0) && OutputBufferSize > 0) {  
    //
    // Allocate output buffer
    //
    *OutputBuffer = AllocatePages (EFI_SIZE_TO_PAGES (OutputBufferSize) + 1);
    if (*OutputBuffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    DEBUG ((DEBUG_INFO, "Customized Guided section Memory Size required is 0x%x and address is 0x%p\n", OutputBufferSize, *OutputBuffer));
    //
    // *OutputBuffer still is one section. Adjust *OutputBuffer offset, 
    // skip EFI section header to make section data at page alignment.
    //
    *OutputBuffer = (VOID *)((UINT8 *) *OutputBuffer + EFI_PAGE_SIZE - sizeof (EFI_COMMON_SECTION_HEADER));
  }
  
  Status = ExtractGuidedSectionDecode (
             InputSection, 
             OutputBuffer,
             ScratchBuffer,
             AuthenticationStatus
             );
  if (EFI_ERROR (Status)) {
    //
    // Decode failed
    //
    DEBUG ((DEBUG_ERROR, "Extract guided section Failed - %r\n", Status));
    return Status;
  }
  
  *OutputSize = (UINTN) OutputBufferSize;
  
  return EFI_SUCCESS;
}


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
  )
{
  EFI_STATUS                      Status;
  UINT8                           *DstBuffer;
  UINT8                           *ScratchBuffer;
  UINT32                          DstBufferSize;
  UINT32                          ScratchBufferSize;
  VOID                            *CompressionSource;
  UINT32                          CompressionSourceSize;
  UINT32                          UncompressedLength;
  UINT8                           CompressionType;

  if (CompressionSection->CommonHeader.Type != EFI_SECTION_COMPRESSION) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  if (IS_SECTION2 (CompressionSection)) {
    CompressionSource = (VOID *) ((UINT8 *) CompressionSection + sizeof (EFI_COMPRESSION_SECTION2));
    CompressionSourceSize = (UINT32) (SECTION2_SIZE (CompressionSection) - sizeof (EFI_COMPRESSION_SECTION2));
    UncompressedLength = ((EFI_COMPRESSION_SECTION2 *) CompressionSection)->UncompressedLength;
    CompressionType = ((EFI_COMPRESSION_SECTION2 *) CompressionSection)->CompressionType;
  } else {
    CompressionSource = (VOID *) ((UINT8 *) CompressionSection + sizeof (EFI_COMPRESSION_SECTION));
    CompressionSourceSize = (UINT32) (SECTION_SIZE (CompressionSection) - sizeof (EFI_COMPRESSION_SECTION));
    UncompressedLength = CompressionSection->UncompressedLength;
    CompressionType = CompressionSection->CompressionType;
  }
  
  //
  // This is a compression set, expand it
  //
  switch (CompressionType) {
  case EFI_STANDARD_COMPRESSION:
    if (FeaturePcdGet(PcdDxeIplSupportUefiDecompress)) {
      //
      // Load EFI standard compression.
      // For compressed data, decompress them to destination buffer.
      //
      Status = UefiDecompressGetInfo (
                 CompressionSource,
                 CompressionSourceSize,
                 &DstBufferSize,
                 &ScratchBufferSize
                 );
      if (EFI_ERROR (Status)) {
        //
        // GetInfo failed
        //
        DEBUG ((DEBUG_ERROR, "Decompress GetInfo Failed - %r\n", Status));
        return EFI_NOT_FOUND;
      }
      //
      // Allocate scratch buffer
      //
      ScratchBuffer = AllocatePages (EFI_SIZE_TO_PAGES (ScratchBufferSize));
      if (ScratchBuffer == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }
      //
      // Allocate destination buffer, extra one page for adjustment 
      //
      DstBuffer = AllocatePages (EFI_SIZE_TO_PAGES (DstBufferSize) + 1);
      if (DstBuffer == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }
      //
      // DstBuffer still is one section. Adjust DstBuffer offset, skip EFI section header
      // to make section data at page alignment.
      //
      DstBuffer = DstBuffer + EFI_PAGE_SIZE - sizeof (EFI_COMMON_SECTION_HEADER);
      //
      // Call decompress function
      //
      Status = UefiDecompress (
                  CompressionSource,
                  DstBuffer,
                  ScratchBuffer
                  );
      if (EFI_ERROR (Status)) {
        //
        // Decompress failed
        //
        DEBUG ((DEBUG_ERROR, "Decompress Failed - %r\n", Status));
        return EFI_NOT_FOUND;
      }
      break;
    } else {
      //
      // PcdDxeIplSupportUefiDecompress is FALSE
      // Don't support UEFI decompression algorithm.
      //
      ASSERT (FALSE);
      return EFI_NOT_FOUND;
    }

  case EFI_NOT_COMPRESSED:
    //
    // Allocate destination buffer
    //
    DstBufferSize = UncompressedLength;
    DstBuffer     = AllocatePages (EFI_SIZE_TO_PAGES (DstBufferSize) + 1);
    if (DstBuffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    //
    // Adjust DstBuffer offset, skip EFI section header
    // to make section data at page alignment.
    //
    DstBuffer = DstBuffer + EFI_PAGE_SIZE - sizeof (EFI_COMMON_SECTION_HEADER);
    //
    // stream is not actually compressed, just encapsulated.  So just copy it.
    //
    CopyMem (DstBuffer, CompressionSource, DstBufferSize);
    break;

  default:
    //
    // Don't support other unknown compression type.
    //
    ASSERT (FALSE);
    return EFI_NOT_FOUND;
  }

  *OutputSize = DstBufferSize;
  *OutputBuffer = DstBuffer;
  return EFI_SUCCESS;
}
