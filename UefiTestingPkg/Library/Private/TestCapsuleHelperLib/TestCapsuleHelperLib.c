/** @file
This module implements routines that support test capsules.

Copyright (c) 2017, Microsoft Corporation

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

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Private/Guid/TestCapsule.h>
#include <Private/Library/TestCapsuleHelperLib.h>


/**
 Function to get a capsule from the system table.
 Since there can be more than a single capsule with the same
 guid.  Use the Index parameter to iterate thru the
 capsules.

 @param  Index - capsule index to return (0 based)
 @param  Head (out) - Capsule header pointer returned on successful

 @return EFI_SUCCESS if capsule found
         EFI_NOT_FOUND - Index parameter out of bounds
     EFI_VOLUME_CORRUPTED - Capsule doesn't have valid signature
     EFI_INCOMPATIBLE_VERSION - Capsule version not expected
**/
EFI_STATUS
EFIAPI
GetTestCapsuleFromSystemTable(
  IN  UINTN Index,
  OUT EFI_CAPSULE_HEADER **Head
)
{
  EFI_STATUS Status;
  EFI_CAPSULE_TABLE *CapsuleTablePtr;
  EFI_CAPSULE_HEADER* Header;
  TEST_CAPSULE_PAYLOAD* Payload;

  Status = EfiGetSystemConfigurationTable(&gTestCapsuleGuid, &CapsuleTablePtr);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_INFO, __FUNCTION__ " System table entry for test capsule not found\n"));
    return Status;
  }

  if (CapsuleTablePtr->CapsuleArrayNumber <= (UINT32)Index)
  {
    DEBUG((DEBUG_INFO, __FUNCTION__ " Index beyond Capsule Array Number. %d\n", Index));
    return EFI_NOT_FOUND;
  }

  Header = (EFI_CAPSULE_HEADER*)(CapsuleTablePtr->CapsulePtr[Index]);
  Payload = (TEST_CAPSULE_PAYLOAD*)(((UINT8*)Header) + Header->HeaderSize);

  if (Payload->Signature != TEST_CAPSULE_SIGNATURE)
  {
    DEBUG((DEBUG_ERROR, __FUNCTION__ " found test capsule but signature invalid!\n"));
    return EFI_VOLUME_CORRUPTED;
  }

  if (Payload->Version != TEST_CAPSULE_VERSION)
  {
    DEBUG((DEBUG_ERROR, __FUNCTION__ " found test capsule but version invalid!  Payload Version: %d\n", Payload->Version));
    return EFI_INCOMPATIBLE_VERSION;
  }
  *Head = Header;
  return EFI_SUCCESS;
}

/**
Function to get the number of test capsules in the system table

@return the count of capsules.
**/
UINTN
EFIAPI
GetTestCapsuleCountFromSystemTable()
{
  EFI_STATUS Status;
  EFI_CAPSULE_TABLE *CapsuleTablePtr;

  Status = EfiGetSystemConfigurationTable(&gTestCapsuleGuid, &CapsuleTablePtr);
  if (!EFI_ERROR(Status))
  {
    return (UINTN)(CapsuleTablePtr->CapsuleArrayNumber);
  }

  DEBUG((DEBUG_INFO, __FUNCTION__ " System table entry for test capsule not found\n"));
  return 0;
}


EFI_CAPSULE_BLOCK_DESCRIPTOR*
EFIAPI
AllocateAndPopulateDescriptorBlock(
  IN  EFI_PHYSICAL_ADDRESS NextBlockAddress,  //if no more blocks the address can be 0
  IN  INTN  Count,
  IN  UINTN Sizes[]
  )
{
  EFI_CAPSULE_BLOCK_DESCRIPTOR* Group = NULL;
  INTN Index = 0;

  if (Count < 1)
  {
    DEBUG((DEBUG_ERROR, __FUNCTION__ " Invalid Count Parameter\n"));
    return NULL;
  }

  //make sure not continuation pointers in the set until the end
  for (Index = 0; Index < (Count - 1); Index++)
  {
    if (Sizes[Index] == 0)
    {
      DEBUG((DEBUG_ERROR, __FUNCTION__ " Invalid Sizes.  Can't have zero element in array except at end.\n"));
      return NULL;
    }
  }

  if (Sizes[Count - 1] != 0)
  {
    DEBUG((DEBUG_ERROR, __FUNCTION__ " Invalid Sizes.  Must end with zero\n"));
    return NULL;
  }

  //
  // Should be good data now allocate and setup the block(s)
  //
  Group = AllocateRuntimeZeroPool(Count * sizeof(EFI_CAPSULE_BLOCK_DESCRIPTOR));
  if (Group == NULL)
  {
    DEBUG((DEBUG_ERROR, __FUNCTION__ " failed to allocate memory for capsule descriptors\n"));
    return NULL;
  }
  Index = 0;
  //Now allocate each block and set the continuation pointer to the next group
  while (Index < Count)
  {
    Group[Index].Length = (UINT64)Sizes[Index];

    if (Sizes[Index] == 0)
    { //continuation
      Group[Index].Union.ContinuationPointer = NextBlockAddress;
    }
    else
    { //data
      Group[Index].Union.DataBlock = (EFI_PHYSICAL_ADDRESS)(UINTN)AllocateRuntimeZeroPool(Sizes[Index]);
      if (Group[Index].Union.DataBlock == 0)
      {
        DEBUG((DEBUG_ERROR, __FUNCTION__ " failed to allocate data block\n"));
        //Error - free up memory allocated and return null
        //goto Cleanup;
        FreeSgList(Group);
        return NULL;  
      }
    }
    Index++;
  }
  return Group;
}


EFI_STATUS
EFIAPI
BuildTestCapsule(
  IN      UINT32                         CapsuleFlags,
  IN OUT  EFI_CAPSULE_BLOCK_DESCRIPTOR** SgList,
  IN      INTN                          Count,
  IN      UINTN                          Sizes[]
)
{
  EFI_CAPSULE_HEADER* Header = NULL;
  TEST_CAPSULE_PAYLOAD* Payload = NULL;
  EFI_CAPSULE_BLOCK_DESCRIPTOR *Next = NULL;
  EFI_STATUS Status = EFI_SUCCESS;
  INTN Index;
  INTN BlockLen;
  EFI_CAPSULE_BLOCK_DESCRIPTOR* Group = NULL;

  if (SgList == NULL)
  {
    Status = EFI_INVALID_PARAMETER;
    goto Cleanup;
  }

  *SgList = NULL;

  if (Count < 2)
  {
    DEBUG((DEBUG_ERROR, __FUNCTION__ " Count must be at least 2\n"));
    Status = EFI_INVALID_PARAMETER;
    goto Cleanup;
  }

  if (Sizes[Count - 1] != 0)
  {
    DEBUG((DEBUG_ERROR, __FUNCTION__ " Sizes array must end with a zero element\n"));
    Status = EFI_INVALID_PARAMETER;
    goto Cleanup;
  }

  if (Sizes[0] < sizeof(EFI_CAPSULE_HEADER))
  {
    DEBUG((DEBUG_ERROR, __FUNCTION__ " First block must be large enough to hold the entire capsule header\n"));
    Status = EFI_INVALID_PARAMETER;
    goto Cleanup;
  }

  if (Sizes[0] < (sizeof(EFI_CAPSULE_HEADER) + sizeof(TEST_CAPSULE_PAYLOAD)))
  { //special case where capsule header and payload are in different scatter gather list blocks
    //block 0 must be equal to capsule header
    if (Sizes[0] != sizeof(EFI_CAPSULE_HEADER))
    {
      //error - if capsule header and test payload are split it must be on clean boundary
      DEBUG((DEBUG_ERROR, __FUNCTION__ " First datablock must be exactly header sized with header and payload split\n"));
      Status = EFI_INVALID_PARAMETER;
      goto Cleanup;
    }

    for (Index = 1; Index < Count; Index++)
    {  //find the first datablock 
      if (Sizes[Index] != 0)
      {
        if (Sizes[Index] < sizeof(TEST_CAPSULE_PAYLOAD))
        {
          //error - the test payload must fit on a single datablock
          DEBUG((DEBUG_ERROR, __FUNCTION__ " Next data block must be large enough to hold the entire payload structure\n"));
          Status = EFI_INVALID_PARAMETER;
          goto Cleanup;
        } 
        break;
      }
    } //end for loop 
  }//close if dealing with test capsule payload and header in different blocks

  //Loop thru the sizes in reverse order.  Create scatter gather list and allocate
  //datablocks
  Index = Count - 2; //skip the end node
  BlockLen = 1;
  while (Index >= 0)
  {
    BlockLen++;
    if ((Index == 0) || (Sizes[Index] == 0))
    {
      INTN tempindex = Index;
      if (tempindex != 0) {
        tempindex += 1;
        BlockLen -= 1;
      }
      Group = AllocateAndPopulateDescriptorBlock((EFI_PHYSICAL_ADDRESS)(UINTN)Next, BlockLen, &Sizes[tempindex]);
      if (Group == NULL)
      {
        Status = EFI_OUT_OF_RESOURCES;
        DEBUG((DEBUG_ERROR, __FUNCTION__ " failed to allocate memory for capsule descriptors\n"));
        goto Cleanup;
      }
      //Reset for next
      BlockLen = 1;
      Next = Group;  //setup next continuation pointer
      if (Index == 0)
      {
        *SgList = Group;  //special case for head
      }
    } //close if loop
    Index--;
  } //close while loop


  //
  // Initialize the capsule header.
  //
  Header = (EFI_CAPSULE_HEADER*)(UINTN)((*SgList)->Union.DataBlock);
  CopyGuid(&Header->CapsuleGuid, &gTestCapsuleGuid);
  Header->HeaderSize = sizeof(*Header);
  Header->CapsuleImageSize = (UINT32)GetLayoutTotalSize(Count, Sizes);
  Header->Flags = CapsuleFlags;

  //
  // Initialize our capsule payload header.
  //
  if (Sizes[0] < (sizeof(EFI_CAPSULE_HEADER) + sizeof(TEST_CAPSULE_PAYLOAD)))
  { //special case where capsule header and payload are in different scatter gather list blocks
    //go find the 2nd datablock...the Header or payload can not be spread across blocks.  
    //always skip the first block as that is the header
    Next = *SgList;
    do
    {
      if (Next->Length == 0)
      {
        Next = (EFI_CAPSULE_BLOCK_DESCRIPTOR*)(UINTN)(Next->Union.ContinuationPointer);
      }
      else
      {
        Next += 1;
      }
    } while (Next->Length == 0);
    Payload = (TEST_CAPSULE_PAYLOAD*)(UINTN)(Next->Union.DataBlock);
  }
  else
  {
    Payload = (TEST_CAPSULE_PAYLOAD*)(((UINT8*)Header) + Header->HeaderSize);
  }

  Payload->Signature = TEST_CAPSULE_SIGNATURE;
  Payload->Version = TEST_CAPSULE_VERSION;
  Payload->DataSize = Header->CapsuleImageSize - Header->HeaderSize;

  Status = EFI_SUCCESS;
  goto Done;

Cleanup:
  FreeSgList(*SgList);
  *SgList = NULL;
  FreeSgList(Group);
  Group = NULL;

Done:
  return Status;
}

VOID
EFIAPI
FreeSgList(EFI_CAPSULE_BLOCK_DESCRIPTOR* List)
{
  INTN i = 0;
  if (List == NULL)
  {
    return;
  }
  //free all the datablocks in this list
  while (List[i].Length != 0)
  {
    //datablock
    FreePool((VOID*)(UINTN)List[i].Union.DataBlock);
    List[i].Union.DataBlock = 0;
    i++;
  }

  //its a continuation block
  FreeSgList((EFI_CAPSULE_BLOCK_DESCRIPTOR*)(UINTN)(List[i].Union.ContinuationPointer));
  List[i].Union.ContinuationPointer = 0;
  FreePool(List);
  return;
}


UINTN
EFIAPI
GetLayoutTotalSize(
  IN      INTN                          Count,
  IN      UINTN                          Sizes[])
{
  UINTN Result = 0;
  INTN Index;
  for (Index = 0; Index < Count; Index++)
  {
    Result += (UINTN)Sizes[Index];
  }
  return Result;
}


