/**@file

Library registers an interrupt handler which catches exceptions related to memory
protections and logs them in the platform's persistent storage.

Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
Copyright (c) 2012 - 2021, Arm Ltd. All rights reserved.<BR>
Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi/UefiSpec.h>
#include <Uefi/UefiBaseType.h>

#include <Pi/PiStatusCode.h>

#include <Protocol/DebugSupport.h>
#include <Protocol/MemoryProtectionNonstopMode.h>

#include <Guid/DebugImageInfoTable.h>

#include <Library/CpuExceptionHandlerLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DxeMemoryProtectionHobLib.h>
#include <Library/ExceptionPersistenceLib.h>
#include <Library/ResetSystemLib.h>
#include <Library/MsWheaEarlyStorageLib.h>
#include <Library/PeCoffGetEntryPointLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiLib.h>
#include <Library/SerialPortLib.h>
#include <Library/PrintLib.h>

#define IS_TRANSLATION_FAULT(Cause)  ((Cause == 0x4) || (Cause == 0x5) || (Cause == 0x6) || (Cause == 0x7))
#define IS_ACCESS_FLAG_FAULT(Cause)  ((Cause == 0x9) || (Cause == 0xa) || (Cause == 0xb))

/**
  Common constructor for this library.

  @param ImageHandle          Image handle this library.
  @param SystemTable          Pointer to SystemTable.
  @param MemProtExVector      Memory Protection Exception Vector.
  @param StackCookieExVector  Stack Cookie Exception Vector.

  @retval EFI_SUCCESS         Successfully registered CpuArchRegisterMemoryProtectionExceptionHandlers
  @retval EFI_ABORTED         Failed to register CpuArchRegisterMemoryProtectionExceptionHandlers

**/
EFI_STATUS
EFIAPI
MemoryProtectionExceptionHandlerCommonConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable,
  IN  UINTN            MemProtExVector,
  IN  UINTN            StackCookieExVector
  );

/**
  Use the EFI Debug Image Table to lookup the FaultAddress and find which PE/COFF image
  it came from. As long as the PE/COFF image contains a debug directory entry a
  string can be returned. For ELF and Mach-O images the string points to the Mach-O or ELF
  image. Microsoft tools contain a pointer to the PDB file that contains the debug information.

  @param  FaultAddress         Address to find PE/COFF image for.
  @param  ImageBase            Return load address of found image
  @param  PeCoffSizeOfHeaders  Return the size of the PE/COFF header for the image that was found

  @retval NULL                 FaultAddress not in a loaded PE/COFF image.
  @retval                      Path and file name of PE/COFF image.

**/
CHAR8 *
GetImageName (
  IN  UINTN  FaultAddress,
  OUT UINTN  *ImageBase,
  OUT UINTN  *PeCoffSizeOfHeaders
  )
{
  EFI_STATUS                         Status;
  EFI_DEBUG_IMAGE_INFO_TABLE_HEADER  *DebugTableHeader;
  EFI_DEBUG_IMAGE_INFO               *DebugTable;
  UINTN                              Entry;
  CHAR8                              *Address;

  Status = EfiGetSystemConfigurationTable (&gEfiDebugImageInfoTableGuid, (VOID **)&DebugTableHeader);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  DebugTable = DebugTableHeader->EfiDebugImageInfoTable;
  if (DebugTable == NULL) {
    return NULL;
  }

  Address = (CHAR8 *)(UINTN)FaultAddress;
  for (Entry = 0; Entry < DebugTableHeader->TableSize; Entry++, DebugTable++) {
    if (DebugTable->NormalImage != NULL) {
      if ((DebugTable->NormalImage->ImageInfoType == EFI_DEBUG_IMAGE_INFO_TYPE_NORMAL) &&
          (DebugTable->NormalImage->LoadedImageProtocolInstance != NULL))
      {
        if ((Address >= (CHAR8 *)DebugTable->NormalImage->LoadedImageProtocolInstance->ImageBase) &&
            (Address <= ((CHAR8 *)DebugTable->NormalImage->LoadedImageProtocolInstance->ImageBase + DebugTable->NormalImage->LoadedImageProtocolInstance->ImageSize)))
        {
          *ImageBase           = (UINTN)DebugTable->NormalImage->LoadedImageProtocolInstance->ImageBase;
          *PeCoffSizeOfHeaders = PeCoffGetSizeOfHeaders ((VOID *)(UINTN)*ImageBase);
          return PeCoffLoaderGetPdbPointer (DebugTable->NormalImage->LoadedImageProtocolInstance->ImageBase);
        }
      }
    }
  }

  return NULL;
}

STATIC CHAR8  *gExceptionTypeString[] = {
  "Synchronous",
  "IRQ",
  "FIQ",
  "SError"
};

STATIC BOOLEAN  mRecursiveException;

STATIC
VOID
DescribeInstructionOrDataAbort (
  IN CHAR8  *AbortType,
  IN UINTN  Iss
  )
{
  CHAR8  *AbortCause;

  switch (Iss & 0x3f) {
    case 0x0: AbortCause = "Address size fault, zeroth level of translation or translation table base register";
      break;
    case 0x1: AbortCause = "Address size fault, first level";
      break;
    case 0x2: AbortCause = "Address size fault, second level";
      break;
    case 0x3: AbortCause = "Address size fault, third level";
      break;
    case 0x4: AbortCause = "Translation fault, zeroth level";
      break;
    case 0x5: AbortCause = "Translation fault, first level";
      break;
    case 0x6: AbortCause = "Translation fault, second level";
      break;
    case 0x7: AbortCause = "Translation fault, third level";
      break;
    case 0x9: AbortCause = "Access flag fault, first level";
      break;
    case 0xa: AbortCause = "Access flag fault, second level";
      break;
    case 0xb: AbortCause = "Access flag fault, third level";
      break;
    case 0xd: AbortCause = "Permission fault, first level";
      break;
    case 0xe: AbortCause = "Permission fault, second level";
      break;
    case 0xf: AbortCause = "Permission fault, third level";
      break;
    case 0x10: AbortCause = "Synchronous external abort";
      break;
    case 0x18: AbortCause = "Synchronous parity error on memory access";
      break;
    case 0x11: AbortCause = "Asynchronous external abort";
      break;
    case 0x19: AbortCause = "Asynchronous parity error on memory access";
      break;
    case 0x14: AbortCause = "Synchronous external abort on translation table walk, zeroth level";
      break;
    case 0x15: AbortCause = "Synchronous external abort on translation table walk, first level";
      break;
    case 0x16: AbortCause = "Synchronous external abort on translation table walk, second level";
      break;
    case 0x17: AbortCause = "Synchronous external abort on translation table walk, third level";
      break;
    case 0x1c: AbortCause = "Synchronous parity error on memory access on translation table walk, zeroth level";
      break;
    case 0x1d: AbortCause = "Synchronous parity error on memory access on translation table walk, first level";
      break;
    case 0x1e: AbortCause = "Synchronous parity error on memory access on translation table walk, second level";
      break;
    case 0x1f: AbortCause = "Synchronous parity error on memory access on translation table walk, third level";
      break;
    case 0x21: AbortCause = "Alignment fault";
      break;
    case 0x22: AbortCause = "Debug event";
      break;
    case 0x30: AbortCause = "TLB conflict abort";
      break;
    case 0x33:
    case 0x34: AbortCause = "IMPLEMENTATION DEFINED";
      break;
    case 0x35:
    case 0x36: AbortCause = "Domain fault";
      break;
    default: AbortCause = "";
      break;
  }

  DEBUG ((DEBUG_ERROR, "\n%a: %a\n", AbortType, AbortCause));
}

STATIC
VOID
DescribeExceptionSyndrome (
  IN UINT32  Esr
  )
{
  CHAR8  *Message;
  UINTN  Ec;
  UINTN  Iss;

  Ec  = Esr >> 26;
  Iss = Esr & 0x00ffffff;

  switch (Ec) {
    case 0x15: Message = "SVC executed in AArch64";
      break;
    case 0x20:
    case 0x21: DescribeInstructionOrDataAbort ("Instruction abort", Iss);
      return;
    case 0x22: Message = "PC alignment fault";
      break;
    case 0x23: Message = "SP alignment fault";
      break;
    case 0x24:
    case 0x25: DescribeInstructionOrDataAbort ("Data abort", Iss);
      return;
    default: return;
  }

  DEBUG ((DEBUG_ERROR, "\n %a \n", Message));
}

#ifndef MDEPKG_NDEBUG
STATIC
CONST CHAR8 *
BaseName (
  IN  CONST CHAR8  *FullName
  )
{
  CONST CHAR8  *Str;

  Str = FullName + AsciiStrLen (FullName);

  while (--Str > FullName) {
    if ((*Str == '/') || (*Str == '\\')) {
      return Str + 1;
    }
  }

  return Str;
}

#endif

/**
  This is the default action to take on an unexpected exception

  @param  ExceptionType    Type of the exception
  @param  SystemContext    Register state at the time of the Exception

**/
VOID
DefaultExceptionHandler (
  IN     EFI_EXCEPTION_TYPE  ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  CHAR8  Buffer[100];
  UINTN  CharCount;
  INT32  Offset;

  if (mRecursiveException) {
    DEBUG ((DEBUG_INFO, "\nRecursive exception occurred while dumping the CPU state\n"));

    ResetWarm ();
  }

  mRecursiveException = TRUE;

  DEBUG ((DEBUG_INFO, "\n\n%a Exception at 0x%016lx\n", gExceptionTypeString[ExceptionType], SystemContext.SystemContextAArch64->ELR));

  DEBUG_CODE_BEGIN ();
  CHAR8   *Pdb, *PrevPdb;
  UINTN   ImageBase;
  UINTN   PeCoffSizeOfHeader;
  UINT64  *Fp;
  UINT64  RootFp[2];
  UINTN   Idx;

  PrevPdb = Pdb = GetImageName (SystemContext.SystemContextAArch64->ELR, &ImageBase, &PeCoffSizeOfHeader);
  if (Pdb != NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "PC 0x%012lx (0x%012lx+0x%08x) [ 0] %a\n",
      SystemContext.SystemContextAArch64->ELR,
      ImageBase,
      SystemContext.SystemContextAArch64->ELR - ImageBase,
      BaseName (Pdb)
      ));
  } else {
    DEBUG ((DEBUG_ERROR, "PC 0x%012lx\n", SystemContext.SystemContextAArch64->ELR));
  }

  if ((UINT64 *)SystemContext.SystemContextAArch64->FP != 0) {
    Idx = 0;

    RootFp[0] = ((UINT64 *)SystemContext.SystemContextAArch64->FP)[0];
    RootFp[1] = ((UINT64 *)SystemContext.SystemContextAArch64->FP)[1];
    if (RootFp[1] != SystemContext.SystemContextAArch64->LR) {
      RootFp[0] = SystemContext.SystemContextAArch64->FP;
      RootFp[1] = SystemContext.SystemContextAArch64->LR;
    }

    for (Fp = RootFp; Fp[0] != 0; Fp = (UINT64 *)Fp[0]) {
      Pdb = GetImageName (Fp[1], &ImageBase, &PeCoffSizeOfHeader);
      if (Pdb != NULL) {
        if (Pdb != PrevPdb) {
          Idx++;
          PrevPdb = Pdb;
        }

        DEBUG ((
          DEBUG_ERROR,
          "PC 0x%012lx (0x%012lx+0x%08x) [% 2d] %a\n",
          Fp[1],
          ImageBase,
          Fp[1] - ImageBase,
          Idx,
          BaseName (Pdb)
          ));
      } else {
        DEBUG ((DEBUG_ERROR, "PC 0x%012lx\n", Fp[1]));
      }
    }

    PrevPdb = Pdb = GetImageName (SystemContext.SystemContextAArch64->ELR, &ImageBase, &PeCoffSizeOfHeader);
    if (Pdb != NULL) {
      DEBUG ((DEBUG_ERROR, "\n[ 0] %a\n", Pdb));
    }

    Idx = 0;
    for (Fp = RootFp; Fp[0] != 0; Fp = (UINT64 *)Fp[0]) {
      Pdb = GetImageName (Fp[1], &ImageBase, &PeCoffSizeOfHeader);
      if ((Pdb != NULL) && (Pdb != PrevPdb)) {
        DEBUG ((DEBUG_ERROR, "[% 2d] %a\n", ++Idx, Pdb));
        PrevPdb = Pdb;
      }
    }
  }

  DEBUG_CODE_END ();

  DEBUG ((DEBUG_ERROR, "\n  X0 0x%016lx   X1 0x%016lx   X2 0x%016lx   X3 0x%016lx\n", SystemContext.SystemContextAArch64->X0, SystemContext.SystemContextAArch64->X1, SystemContext.SystemContextAArch64->X2, SystemContext.SystemContextAArch64->X3));
  DEBUG ((DEBUG_ERROR, "  X4 0x%016lx   X5 0x%016lx   X6 0x%016lx   X7 0x%016lx\n", SystemContext.SystemContextAArch64->X4, SystemContext.SystemContextAArch64->X5, SystemContext.SystemContextAArch64->X6, SystemContext.SystemContextAArch64->X7));
  DEBUG ((DEBUG_ERROR, "  X8 0x%016lx   X9 0x%016lx  X10 0x%016lx  X11 0x%016lx\n", SystemContext.SystemContextAArch64->X8, SystemContext.SystemContextAArch64->X9, SystemContext.SystemContextAArch64->X10, SystemContext.SystemContextAArch64->X11));
  DEBUG ((DEBUG_ERROR, " X12 0x%016lx  X13 0x%016lx  X14 0x%016lx  X15 0x%016lx\n", SystemContext.SystemContextAArch64->X12, SystemContext.SystemContextAArch64->X13, SystemContext.SystemContextAArch64->X14, SystemContext.SystemContextAArch64->X15));
  DEBUG ((DEBUG_ERROR, " X16 0x%016lx  X17 0x%016lx  X18 0x%016lx  X19 0x%016lx\n", SystemContext.SystemContextAArch64->X16, SystemContext.SystemContextAArch64->X17, SystemContext.SystemContextAArch64->X18, SystemContext.SystemContextAArch64->X19));
  DEBUG ((DEBUG_ERROR, " X20 0x%016lx  X21 0x%016lx  X22 0x%016lx  X23 0x%016lx\n", SystemContext.SystemContextAArch64->X20, SystemContext.SystemContextAArch64->X21, SystemContext.SystemContextAArch64->X22, SystemContext.SystemContextAArch64->X23));
  DEBUG ((DEBUG_ERROR, " X24 0x%016lx  X25 0x%016lx  X26 0x%016lx  X27 0x%016lx\n", SystemContext.SystemContextAArch64->X24, SystemContext.SystemContextAArch64->X25, SystemContext.SystemContextAArch64->X26, SystemContext.SystemContextAArch64->X27));
  DEBUG ((DEBUG_ERROR, " X28 0x%016lx   FP 0x%016lx   LR 0x%016lx  \n", SystemContext.SystemContextAArch64->X28, SystemContext.SystemContextAArch64->FP, SystemContext.SystemContextAArch64->LR));

  /* We save these as 128bit numbers, but have to print them as two 64bit numbers,
     so swap the 64bit words to correctly represent a 128bit number.  */
  DEBUG ((DEBUG_ERROR, "\n  V0 0x%016lx %016lx   V1 0x%016lx %016lx\n", SystemContext.SystemContextAArch64->V0[1], SystemContext.SystemContextAArch64->V0[0], SystemContext.SystemContextAArch64->V1[1], SystemContext.SystemContextAArch64->V1[0]));
  DEBUG ((DEBUG_ERROR, "  V2 0x%016lx %016lx   V3 0x%016lx %016lx\n", SystemContext.SystemContextAArch64->V2[1], SystemContext.SystemContextAArch64->V2[0], SystemContext.SystemContextAArch64->V3[1], SystemContext.SystemContextAArch64->V3[0]));
  DEBUG ((DEBUG_ERROR, "  V4 0x%016lx %016lx   V5 0x%016lx %016lx\n", SystemContext.SystemContextAArch64->V4[1], SystemContext.SystemContextAArch64->V4[0], SystemContext.SystemContextAArch64->V5[1], SystemContext.SystemContextAArch64->V5[0]));
  DEBUG ((DEBUG_ERROR, "  V6 0x%016lx %016lx   V7 0x%016lx %016lx\n", SystemContext.SystemContextAArch64->V6[1], SystemContext.SystemContextAArch64->V6[0], SystemContext.SystemContextAArch64->V7[1], SystemContext.SystemContextAArch64->V7[0]));
  DEBUG ((DEBUG_ERROR, "  V8 0x%016lx %016lx   V9 0x%016lx %016lx\n", SystemContext.SystemContextAArch64->V8[1], SystemContext.SystemContextAArch64->V8[0], SystemContext.SystemContextAArch64->V9[1], SystemContext.SystemContextAArch64->V9[0]));
  DEBUG ((DEBUG_ERROR, " V10 0x%016lx %016lx  V11 0x%016lx %016lx\n", SystemContext.SystemContextAArch64->V10[1], SystemContext.SystemContextAArch64->V10[0], SystemContext.SystemContextAArch64->V11[1], SystemContext.SystemContextAArch64->V11[0]));
  DEBUG ((DEBUG_ERROR, " V12 0x%016lx %016lx  V13 0x%016lx %016lx\n", SystemContext.SystemContextAArch64->V12[1], SystemContext.SystemContextAArch64->V12[0], SystemContext.SystemContextAArch64->V13[1], SystemContext.SystemContextAArch64->V13[0]));
  DEBUG ((DEBUG_ERROR, " V14 0x%016lx %016lx  V15 0x%016lx %016lx\n", SystemContext.SystemContextAArch64->V14[1], SystemContext.SystemContextAArch64->V14[0], SystemContext.SystemContextAArch64->V15[1], SystemContext.SystemContextAArch64->V15[0]));
  DEBUG ((DEBUG_ERROR, " V16 0x%016lx %016lx  V17 0x%016lx %016lx\n", SystemContext.SystemContextAArch64->V16[1], SystemContext.SystemContextAArch64->V16[0], SystemContext.SystemContextAArch64->V17[1], SystemContext.SystemContextAArch64->V17[0]));
  DEBUG ((DEBUG_ERROR, " V18 0x%016lx %016lx  V19 0x%016lx %016lx\n", SystemContext.SystemContextAArch64->V18[1], SystemContext.SystemContextAArch64->V18[0], SystemContext.SystemContextAArch64->V19[1], SystemContext.SystemContextAArch64->V19[0]));
  DEBUG ((DEBUG_ERROR, " V20 0x%016lx %016lx  V21 0x%016lx %016lx\n", SystemContext.SystemContextAArch64->V20[1], SystemContext.SystemContextAArch64->V20[0], SystemContext.SystemContextAArch64->V21[1], SystemContext.SystemContextAArch64->V21[0]));
  DEBUG ((DEBUG_ERROR, " V22 0x%016lx %016lx  V23 0x%016lx %016lx\n", SystemContext.SystemContextAArch64->V22[1], SystemContext.SystemContextAArch64->V22[0], SystemContext.SystemContextAArch64->V23[1], SystemContext.SystemContextAArch64->V23[0]));
  DEBUG ((DEBUG_ERROR, " V24 0x%016lx %016lx  V25 0x%016lx %016lx\n", SystemContext.SystemContextAArch64->V24[1], SystemContext.SystemContextAArch64->V24[0], SystemContext.SystemContextAArch64->V25[1], SystemContext.SystemContextAArch64->V25[0]));
  DEBUG ((DEBUG_ERROR, " V26 0x%016lx %016lx  V27 0x%016lx %016lx\n", SystemContext.SystemContextAArch64->V26[1], SystemContext.SystemContextAArch64->V26[0], SystemContext.SystemContextAArch64->V27[1], SystemContext.SystemContextAArch64->V27[0]));
  DEBUG ((DEBUG_ERROR, " V28 0x%016lx %016lx  V29 0x%016lx %016lx\n", SystemContext.SystemContextAArch64->V28[1], SystemContext.SystemContextAArch64->V28[0], SystemContext.SystemContextAArch64->V29[1], SystemContext.SystemContextAArch64->V29[0]));
  DEBUG ((DEBUG_ERROR, " V30 0x%016lx %016lx  V31 0x%016lx %016lx\n", SystemContext.SystemContextAArch64->V30[1], SystemContext.SystemContextAArch64->V30[0], SystemContext.SystemContextAArch64->V31[1], SystemContext.SystemContextAArch64->V31[0]));

  DEBUG ((DEBUG_ERROR, "\n  SP 0x%016lx  ELR 0x%016lx  SPSR 0x%08lx  FPSR 0x%08lx\n ESR 0x%08lx          FAR 0x%016lx\n", SystemContext.SystemContextAArch64->SP, SystemContext.SystemContextAArch64->ELR, SystemContext.SystemContextAArch64->SPSR, SystemContext.SystemContextAArch64->FPSR, SystemContext.SystemContextAArch64->ESR, SystemContext.SystemContextAArch64->FAR));

  DEBUG ((DEBUG_ERROR, "\n ESR : EC 0x%02x  IL 0x%x  ISS 0x%08x\n", (SystemContext.SystemContextAArch64->ESR & 0xFC000000) >> 26, (SystemContext.SystemContextAArch64->ESR >> 25) & 0x1, SystemContext.SystemContextAArch64->ESR & 0x1FFFFFF));

  DescribeExceptionSyndrome ((UINT32)SystemContext.SystemContextAArch64->ESR);

  DEBUG ((DEBUG_ERROR, "\nStack dump:\n"));
  for (Offset = -256; Offset < 256; Offset += 32) {
    DEBUG ((
      DEBUG_ERROR,
      "%c %013lx: %016lx %016lx %016lx %016lx\n",
      Offset == 0 ? '>' : ' ',
      SystemContext.SystemContextAArch64->SP + Offset,
      *(UINT64 *)(SystemContext.SystemContextAArch64->SP + Offset),
      *(UINT64 *)(SystemContext.SystemContextAArch64->SP + Offset + 8),
      *(UINT64 *)(SystemContext.SystemContextAArch64->SP + Offset + 16),
      *(UINT64 *)(SystemContext.SystemContextAArch64->SP + Offset + 24)
      ));
  }
}

/**
  Fault handler which logs exceptions in the platform specific early store and does a warm reset.

  @param  InterruptType    Defines the type of interrupt or exception that
                           occurred on the processor.This parameter is processor architecture specific.
  @param  SystemContext    A pointer to the processor context when
                           the interrupt occurred on the processor.

**/
VOID
EFIAPI
MemoryProtectionExceptionHandler (
  IN EFI_EXCEPTION_TYPE  InterruptType,
  IN EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  UINT32  InterruptCause = 0;

  DefaultExceptionHandler (InterruptType, SystemContext);

  MsWheaESAddRecordV0 (
    (EFI_COMPUTING_UNIT_MEMORY|EFI_CU_MEMORY_EC_UNCORRECTABLE),
    (UINT64)PeCoffSearchImageBase (SystemContext.SystemContextAArch64->ELR),
    SystemContext.SystemContextAArch64->ELR,
    NULL,
    NULL
    );

  // Isolate the first 8 bits of the ESR to get the interrupt cause
  InterruptCause = (UINT32)SystemContext.SystemContextAArch64->ESR & 0x3f;
  if (IS_TRANSLATION_FAULT (InterruptCause) || IS_ACCESS_FLAG_FAULT (InterruptCause)) {
    if (EFI_ERROR (ExPersistSetException (ExceptionPersistPageFault))) {
      DEBUG ((
        DEBUG_ERROR,
        "%a - Unable to mark exception occurred in platform early store\n",
        __FUNCTION__
        ));
    }
  }

  ResetWarm ();
}

/**
  Main entry for this library.

  @param ImageHandle     Image handle this library.
  @param SystemTable     Pointer to SystemTable.

  @retval EFI_SUCCESS

**/
EFI_STATUS
EFIAPI
MemoryProtectionExceptionHandlerConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  MemoryProtectionExceptionHandlerCommonConstructor (
    ImageHandle,
    SystemTable,
    EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS,
    PcdGet8 (PcdStackCookieExceptionVector)
    );

  return EFI_SUCCESS;
}
