/** @file
ParserRegistryLib.c

Holds a table which associates a guid with a function. When ParserLibRegisterSectionParser
is called, a guid and function are put into the table. And when ParserLibFindSectionParser
is called, the input guid is used to return an associated function (if one exists)

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>

#include <Guid/Cper.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Library/ParserRegistryLib.h>

struct {
  SectionMapType    **Map;
  UINTN             MaxNumber;
  UINTN             CurrentNumber;
} SectionMap = { NULL, 0, 0 };

/**
 *  Inserts a guid and function pointer into the internal table. The function pointer can later be retrieved
 *  by calling ParserLibFindSectionParser with the guid used to register the function. Note that we do not allow one
 *  guid to register more than one function.
 *
 *  @param[in,out]      Map                        Pointer to Function map the entry is being added to
 *  @param[in,out]      MaxNumber                  Max number of pointers which can be registered in current allocate
 *  @param[in,out]      CurrentNumber              Current number of pointers in the map
 *  @param[in]          Entry                      Element being added to map
 *
 *  @retval             EFI_SUCCESS                The entry was successfully added
 *                      EFI_OUT_OF_RESOURCES       Couldn't allocate the space required to store the entry
**/
EFI_STATUS
EFIAPI
AddTableEntry (
  IN OUT VOID   ***Map,           // Keeping this void for generality
  IN OUT UINTN  *MaxNumber,
  IN OUT UINTN  *CurrentNumber,
  IN     VOID   *Entry
  )
{
  VOID  **TempMap;

  // Check whether the table is enough to store new entry.
  if (*CurrentNumber == *MaxNumber) {
    // Reallocate memory for the table.
    TempMap = ReallocatePool (
                *MaxNumber * sizeof (VOID *),
                (*MaxNumber + 5) * sizeof (VOID *),              // just increase 5 spaces for now
                *Map
                );

    // No enough resource to allocate.
    if (TempMap == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    *Map = TempMap;

    // Increase max number.
    *MaxNumber += 5;
  }

  // Add entry to the table.
  (*Map)[*CurrentNumber] = Entry;
  (*CurrentNumber)++;

  return EFI_SUCCESS;
}

/**
 *  Inserts a guid and function pointer into the internal table. The function pointer can later be retrieved
 *  by calling ParserLibFindSectionParser with the guid used to register the function. Note that we do not allow one
 *  guid to register more than one function.
 *
 *  @param[in]     Ptr                        Function pointer being registered
 *  @param[in]     Guid                       Guid being registered
 *
 *  @retval        EFI_SUCCESS                The guid and pointer were successfully registered
 *                 EFI_ABORTED                The guid has already been registered
 *                 EFI_OUT_OF_RESOURCES       Couldn't allocate the space required to store the guid and pointer
**/
EFI_STATUS
EFIAPI
ParserLibRegisterSectionParser (
  IN CONST SECTIONFUNCTIONPTR  Ptr,
  IN CONST GUID                *Guid
  )
{
  for (UINTN i = 0; i < SectionMap.CurrentNumber; i++) {
    if (CompareGuid ((GUID *)&(SectionMap.Map[i]->Guid), Guid)) {
      return EFI_ABORTED;
    }
  }

  SectionMapType  *new =  AllocatePool (sizeof (SectionMapType));

  new->Guid   = *Guid;
  new->Parser =  Ptr;

  return AddTableEntry ((VOID *)(&SectionMap.Map), &SectionMap.MaxNumber, &SectionMap.CurrentNumber, (VOID *)new);
}

/**
 *  Retrieves a function pointer from internal table using the input guid (if it exists)
 *
 *  @param[in]     Guid                       Guid used to find associated function pointer
 *
 *  @retval        NULL                       The guid was not registered with an associated function
 *                 Anything else              Function pointer returned
**/
SECTIONFUNCTIONPTR
EFIAPI
ParserLibFindSectionParser (
  IN CONST GUID  *Guid
  )
{
  for (UINTN i = 0; i < SectionMap.CurrentNumber; i++) {
    if (CompareGuid ((GUID *)&(SectionMap.Map[i]->Guid), Guid)) {
      return SectionMap.Map[i]->Parser;
    }
  }

  return NULL;
}
