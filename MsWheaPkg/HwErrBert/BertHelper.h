
/** @file -- 


Copyright (c), Microsoft Corporation

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
#ifndef _BERT_H_
#define _BERT_H_

//
// ACPI table information used to initialize tables.
//
#define BOOT_ERROR_REGION_SIZE        0x1000 
#define EFI_HW_ERR_REC_VAR_NAME       L"HwErrRec"
#define EFI_HW_ERR_REC_VAR_NAME_LEN   13      // Buffer length covers at least "HwErrRec####\0"

// A macro to initialise the common header part of EFI ACPI tables as defined by
// EFI_ACPI_DESCRIPTION_HEADER structure.
#define ACPI_HEADER(Signature, Type, Revision) {            \
  Signature,                      /* UINT32  Signature */       \
  sizeof (Type),                  /* UINT32  Length */          \
  Revision,                       /* UINT8   Revision */        \
  0,                              /* UINT8   Checksum */        \
  { EFI_ACPI_OEM_ID },        /* UINT8   OemId[6] */        \
  EFI_ACPI_OEM_TABLE_ID,      /* UINT64  OemTableId */      \
  EFI_ACPI_OEM_REVISION,      /* UINT32  OemRevision */     \
  EFI_ACPI_CREATOR_ID,        /* UINT32  CreatorId */       \
  EFI_ACPI_CREATOR_REVISION   /* UINT32  CreatorRevision */ \
  }

typedef struct _BERT_CONTEXT {
  EFI_ACPI_6_1_BOOT_ERROR_RECORD_TABLE_HEADER   *BertHeader;
  VOID                                          *Block;
  UINT32                                        BlockSize;
} BERT_CONTEXT;


EFI_STATUS
BertSetAcpiTable (
  IN BERT_CONTEXT *Context
);

VOID
BertHeaderCreator (
  BERT_CONTEXT  *Context,
  UINT32        ErrorBlockSize
);

EFI_ACPI_6_1_GENERIC_ERROR_STATUS_STRUCTURE*
BertErrorBlockInitial(
  VOID   *Block,
  UINT32 Severity
);

BOOLEAN
BertErrorBlockAddErrorData (
  IN VOID                  *ErrorBlock,
  IN UINT32                MaxBlockLength,
  IN EFI_GUID              *Guid,
  IN VOID                  *GenericErrorData,
  IN UINT32                SizeOfGenericErrorData,
  IN UINT32                ErrorSeverity,
  IN BOOLEAN               Correctable
);

BOOLEAN
EFIAPI
BertAddCperSection(
  IN EFI_ACPI_6_1_BOOT_ERROR_RECORD_TABLE_HEADER *Bert,
  IN EFI_COMMON_ERROR_RECORD_HEADER              *CperHdr,
  IN EFI_ERROR_SECTION_DESCRIPTOR                *CperErrSecDscp
);

BOOLEAN
EFIAPI
BertAddAllCperSections (
  IN EFI_ACPI_6_1_BOOT_ERROR_RECORD_TABLE_HEADER *Bert,
  IN VOID                                        *ErrorData
);
#endif    // _BERT_H_
