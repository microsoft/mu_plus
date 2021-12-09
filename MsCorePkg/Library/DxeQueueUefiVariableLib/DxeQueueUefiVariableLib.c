/**
An interface for managing a queue.
This can currently hold a max of 100000 items

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Library/QueueLib.h>


// TODO: should this be a PCD? So that it can be modified?
#define DEFAULT_QUEUE_VAR_ATTR (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS)
#define DEFAULT_QUEUE_VAR_NAME   L"00000"
#define DEFAULT_QUEUE_VAR_FORMAT L"%d"
#define DEFAULT_QUEUE_MODULO     100000

/**
  Writes a variable name string for a given ID

  @param[in]    VarId                       Number to append to string
  @param[out]   VarName                     Returned full name of variable
  @param[in]    VarNameSize                 Size of buffer

  @retval       EFI_SUCCESS                 Variable name was generated
  @retval       EFI_INVALID_PARAMETER       VarName is NULL
  @retval       EFI_BUFFER_TOO_SMALL        VarNameSize is too small
*/
EFI_STATUS
STATIC
GenerateVarName (
  IN UINTN   VarId,
  IN CHAR16  *VarName,
  IN UINTN   VarNameSize
  )
{
  if (VarName == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  if (VarNameSize < sizeof(DEFAULT_QUEUE_VAR_NAME)) {
    return EFI_BUFFER_TOO_SMALL;
  }
  UnicodeSPrint (VarName, VarNameSize, DEFAULT_QUEUE_VAR_FORMAT, VarId);
  return EFI_SUCCESS;
}

/**
  Retrieves the variable Id from the variable name

  @param[in]    VarName                     Returned full name of variable
  @param[in]    VarNameSize                 Size of buffer
  @param[out]   VarId                       ID at the end of variable name

  @retval       EFI_SUCCESS                 Variable ID was read
  @retval       EFI_INVALID_PARAMETER       VarName or VarId are NULL
*/
EFI_STATUS
STATIC
GetIdFromVarName (
  CHAR16  *VarName,
  UINTN   VarNameSize,
  UINTN   *VarId
  )
{
  EFI_STATUS Status;
  CHAR16     *EndPointer;

  if (VarName == NULL || VarId == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  Status = StrDecimalToUintnS (
                 VarName,
                 &EndPointer,
                 VarId);
  if (EFI_ERROR(Status)) {
    return EFI_INVALID_PARAMETER;
  }

  if (*EndPointer != L'\0') {
    return EFI_INVALID_PARAMETER;
  }
  return EFI_SUCCESS;
}

/**
  Gets the next queue variable based on what was passed in

  User will need to free VariableNamePtr once done.

  @param[in, out] VariableNamePtr         a double pointer to the variable name, allows for
                                          reallocations as needed
  @param[in]      VariableGuid            a pointer to the EFI_GUID used for iteration
  @param[in]      VariableNameSize        a pointer to the current size of the VariableNamePtr
  @param[in]      VariableGuid            a pointer to the EFI_GUID of the queue

  @retval         EFI_SUCCESS             Next variable was found.
  @retval         EFI_INVALID_PARAMETER   A null pointer was passed in for any of the arguments
  @retval         EFI_NOT_FOUND           The end of the var store has been reached
  @retval         EFI_OUT_OF_RESOURCES    Not enough storage is available to hold the variable name
*/
EFI_STATUS
EFIAPI
GetNextQueueVariableName (
  IN OUT  CHAR16   **VariableNamePtr,
  IN      EFI_GUID *VariableGuid,
  IN      UINTN    *VariableNameSize,
  IN      EFI_GUID *DesiredVariableGuid
  )
{
  EFI_STATUS Status;
  UINTN      RequestedVariableNameSize;
  UINTN      CurrentVariableSize;
  CHAR16     *VariableName;
  DEBUG((DEBUG_INFO, "%a:%d - \n", __FUNCTION__, __LINE__));

  if (VariableNamePtr == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  DEBUG((DEBUG_INFO, "%a:%d - \n", __FUNCTION__, __LINE__));
  VariableName = *VariableNamePtr;
  // if they passed us a pointer to a null, allocate with the default size
  if (VariableName == NULL) {
    CurrentVariableSize = 60;
    VariableName = AllocateZeroPool (CurrentVariableSize);
    DEBUG((DEBUG_INFO, "%a:%d - \n", __FUNCTION__, __LINE__));
  }
  else {
    DEBUG((DEBUG_INFO, "%a:%d - \n", __FUNCTION__, __LINE__));
    CurrentVariableSize = *VariableNameSize;
  }

  if (VariableName == NULL) {
    DEBUG((DEBUG_INFO, "%a:%d - \n", __FUNCTION__, __LINE__));
   return EFI_OUT_OF_RESOURCES;
  }

  Status = EFI_SUCCESS;
  while (Status == EFI_SUCCESS) {
    RequestedVariableNameSize = CurrentVariableSize;
    Status = gRT->GetNextVariableName (&RequestedVariableNameSize, VariableName, VariableGuid);
    // check if it's too small, if so, reallocate and start over.
    if (Status == EFI_BUFFER_TOO_SMALL) {
      // reallocate so that we can resume where we were in the variable store
      VariableName = ReallocatePool (CurrentVariableSize, RequestedVariableNameSize, VariableName);
      CurrentVariableSize = RequestedVariableNameSize;
      // loop back around and try again
      Status = EFI_SUCCESS;
      continue;
    }
    // check if we have a variable
    if (!EFI_ERROR (Status) && CompareGuid (VariableGuid, DesiredVariableGuid)) {
      DEBUG((DEBUG_INFO, "%a:%d - \n", __FUNCTION__, __LINE__));
      break;
    }
  }

  *VariableNameSize = CurrentVariableSize;
  *VariableNamePtr = VariableName;
  DEBUG((DEBUG_INFO, "%a:%d - \n", __FUNCTION__, __LINE__));
  return Status;
}

/**
  Gets the number of items currently in the queue.

  @param[in]  QueueGuid               The Identifier of the queue in question
  @param[out] ItemCount               The number of items that have been queued.

  @retval     EFI_SUCCESS             Everything went as expected.
  @retval     EFI_INVALID_PARAMETER   ItemCount or QueueGuid are NULL
**/
EFI_STATUS
EFIAPI
GetQueueItemCount (
  IN  EFI_GUID *QueueGuid,
  OUT UINTN    *ItemCount
  )
{
  EFI_STATUS  Status;
  CHAR16      *VariableName;
  UINTN       VariableNameSize;
  UINTN       CurrenQueueCount;
  EFI_GUID    VariableGuid;


  if (ItemCount == NULL || QueueGuid == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  VariableName = NULL;
  CurrenQueueCount = 0;
  Status = EFI_SUCCESS;

  // look for all the capsule queue variables
  while (Status == EFI_SUCCESS) {
    Status = GetNextQueueVariableName (&VariableName, &VariableGuid, &VariableNameSize, QueueGuid);
    if (!EFI_ERROR (Status)) {
      CurrenQueueCount += 1;
    }
  }
  // going all the way to the end of the varstore gives us a EFI_NOT_FOUND
  if (Status == EFI_NOT_FOUND) {
    Status = EFI_SUCCESS;
  }

  *ItemCount = CurrenQueueCount;

  if (VariableName != NULL) {
    FreePool (VariableName);
    VariableName = NULL;
  }
  return Status;
}

/**
  Adds an item to the back of the queue.

  @param[in]  QueueGuid     The Identifier of the queue in question
  @param[in]  ItemData      A pointer to the data that should be added to the queue
  @param[in]  ItemDataSize  The size of the data that should be added

  @retval     EFI_SUCCESS   Everything went as expected.
  @retval     Other         Something went horribly horribly wrong
*/
EFI_STATUS
EFIAPI
QueueAddItem (
  IN  EFI_GUID *QueueGuid,
  IN  VOID     *ItemData,
  IN  UINTN    ItemDataSize
  )
{
  EFI_STATUS  Status;
  CHAR16      *VariableName;
  UINTN       VariableNameSize;
  EFI_GUID    VariableGuid;
  UINTN       VarCurrentId;
  UINTN       VarMaxId;
  CHAR16      NewVarName []= DEFAULT_QUEUE_VAR_NAME;


  if (QueueGuid == NULL || ItemData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  VariableName = NULL;
  Status = EFI_SUCCESS;
  VarMaxId = 0;

  // Step 1: get the ID of the last item in the queue
  while (Status == EFI_SUCCESS) {
    Status = GetNextQueueVariableName (&VariableName, &VariableGuid, &VariableNameSize, QueueGuid);
    if (Status == EFI_NOT_FOUND) {
      Status = EFI_SUCCESS;
      break;
    }
    if (EFI_ERROR (Status)) {
      break;
    }
    Status = GetIdFromVarName (VariableName, VariableNameSize, &VarCurrentId);
    VarMaxId = MAX (VarMaxId, VarCurrentId);
  }
  if (EFI_ERROR (Status)) {
    goto Cleanup;
  }
  // step 2: Add one to that and generate a new variable name
  VarMaxId += 1;
  if (VarMaxId >= DEFAULT_QUEUE_MODULO) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Cleanup;
  }
  Status = GenerateVarName (VarMaxId, NewVarName, sizeof (NewVarName));
  if (EFI_ERROR (Status)) {
    return Status;
  }
  // step 3: save data to that variable
  Status = gRT->SetVariable (
          NewVarName,
          QueueGuid,
          DEFAULT_QUEUE_VAR_ATTR,
          ItemDataSize,
          ItemData
          );

Cleanup:
  if (VariableName != NULL) {
    FreePool (VariableName);
    VariableName = NULL;
  }

  return Status;
}

/**
  Pops an item from the queue at a specified index.

  This function allocates a buffer to hold the item from the queue.
  It is expected that the caller will free this allocated memory.

  The ItemData parameters are optional if nulls are supplied, the item will
  be popped but the data will not be returned.

  Once dequeued successfully, the count of the queue should decrease by one.

  @param[in]    QueueGuid     The Identifier of the queue in question
  @param[in]    ItemIndex     The index of the item to peak at
  @param[out]   ItemData      A double pointer to the data that should be added to the queue (OPTIONAL)
  @param[out]   ItemDataSize  A pointer size of the data that was returned (OPTIONAL)

  @retval       EFI_SUCCESS   Everything went as expected.
  @retval       Other         Something went wrong
*/
EFI_STATUS
EFIAPI
QueuePopItemAtIndex (
  IN  EFI_GUID *QueueGuid,
  IN  UINTN    ItemIndex,
  OUT VOID     **ItemData OPTIONAL,
  OUT UINTN    *ItemDataSize OPTIONAL
  )
{
  EFI_STATUS  Status;
  CHAR16      *VariableName;
  UINTN       VariableNameSize;
  EFI_GUID    VariableGuid;
  UINTN       VariableDataSize;
  VOID        *VariableData;
  UINTN       QueueIndex;

  if (QueueGuid == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  VariableName = NULL;
  Status = EFI_SUCCESS;
  VariableData = NULL;
  QueueIndex = 0;
  VariableDataSize = 0;

  // Step 1: find the variable of the first item in the queue.
  do {
    Status = GetNextQueueVariableName (&VariableName, &VariableGuid, &VariableNameSize, QueueGuid);
    if (EFI_ERROR (Status)) {
      DEBUG((DEBUG_ERROR, "[%a] - failed to find next item in queue\n", __FUNCTION__));
      goto Cleanup;
    }
    QueueIndex += 1;
  } while (Status == EFI_SUCCESS && QueueIndex <= ItemIndex);

  if (EFI_ERROR (Status)) {
    DEBUG((DEBUG_ERROR, "[%a] - failed to find the queue item at index %d\n", __FUNCTION__, ItemIndex));
    goto Cleanup;
  }

  // if they passed in non null pointers, we should return the variable data
  if (ItemData != NULL && ItemDataSize != NULL) {
    // Step 2: figure out how big the variable is and allocate the data for it
    Status = gRT->GetVariable (
                    VariableName,
                    QueueGuid,
                    NULL,
                    &VariableDataSize,
                    NULL
                    );
    if (Status != EFI_BUFFER_TOO_SMALL) {
      DEBUG ((DEBUG_ERROR, "[%a] - failed to get size of variable data\n", __FUNCTION__));
      goto Cleanup;
    }
    VariableData = AllocatePool (VariableDataSize);
    if (VariableData == NULL) {
      DEBUG ((DEBUG_ERROR, "[%a] - failed to allocate resources\n", __FUNCTION__));
      Status = EFI_OUT_OF_RESOURCES;
      goto Cleanup;
    }
    // step 3: read in the data to the new allocated buffer
    Status = gRT->GetVariable (
                    VariableName,
                    QueueGuid,
                    NULL,
                    &VariableDataSize,
                    VariableData
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "[%a] - failed to read variable data\n", __FUNCTION__));
      goto Cleanup;
    }

    // step 4: set the return pointer correctly
    *ItemData = VariableData;
    *ItemDataSize = VariableDataSize;
  }

  // step 5: delete the variable
  Status = gRT->SetVariable (
                  VariableName,
                  QueueGuid,
                  DEFAULT_QUEUE_VAR_ATTR,
                  0,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[%a] - failed to delete variable\n", __FUNCTION__));
    goto Cleanup;
  }


Cleanup:
  if (VariableName != NULL) {
    FreePool (VariableName);
    VariableName = NULL;
  }

  return Status;
}

/**
  Pops an item from the front of the queue.

  This function allocates a buffer to hold the item from the queue.
  It is expected that the caller will free this allocated memory.

  Once dequeued successfully, the count of the queue should decrease by one.

  @param[in]  QueueGuid     The Identifier of the queue in question
  @param[out] ItemData      A double pointer to the data that should be added to the queue
  @param[out] ItemDataSize  A pointer size of the data that was returned

  @retval     EFI_SUCCESS   Everything went as expected.
  @retval     Other         Something went wrong
*/
EFI_STATUS
EFIAPI
QueuePopItem (
  IN  EFI_GUID *QueueGuid,
  OUT VOID     **ItemData OPTIONAL,
  OUT UINTN    *ItemDataSize OPTIONAL
  )
{
 return QueuePopItemAtIndex(QueueGuid, 0, ItemData, ItemDataSize);
}

/**
  Peaks at a specific item currently in the queue.

  This function allocates a buffer to hold the item from the queue.
  It is expected that the caller will free this allocated memory.

  Calling this function will not impact the count of the queue.

  @param[in]    QueueGuid     The Identifier of the queue in question
  @param[in]    ItemIndex     The index of the item to peak at
  @param[out]   ItemData      A double pointer to the data that should be added to the queue (OPTIONAL)
  @param[out]   ItemDataSize  A pointer size of the data that was returned (OPTIONAL)

  @retval       EFI_SUCCESS   Everything went as expected.
  @retval       Other         Something went horribly horribly wrong
*/
EFI_STATUS
EFIAPI
QueuePeekAtIndex (
  IN  EFI_GUID *QueueGuid,
  IN  UINTN    ItemIndex,
  OUT VOID     **ItemData,
  OUT UINTN    *ItemDataSize
  )
{
  EFI_STATUS  Status;
  CHAR16      *VariableName;
  UINTN       VariableNameSize;
  EFI_GUID    VariableGuid;
  UINTN       VariableDataSize;
  VOID        *VariableData;
  UINTN       QueueIndex;


  if (QueueGuid == NULL) {
    DEBUG ((DEBUG_ERROR, "[%a] - invalid parameter as QueueGuid is NULL\n", __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }

  if (ItemData == NULL) {
    DEBUG ((DEBUG_ERROR, "[%a] - invalid parameter as ItemData is NULL\n", __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }

  if (ItemDataSize == NULL) {
    DEBUG ((DEBUG_ERROR, "[%a] - invalid parameter as ItemDataSize is NULL\n", __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }

  // Step 1: find the variable name of the item at the specific index (if it exists)
  VariableName = NULL;
  Status = EFI_SUCCESS;
  VariableData = NULL;
  QueueIndex = 0;
  VariableDataSize = 0;

  do {
    Status = GetNextQueueVariableName (&VariableName, &VariableGuid, &VariableNameSize, QueueGuid);
    
    if (EFI_ERROR (Status)) {
      DEBUG((DEBUG_ERROR, "[%a] - failed to find next item in queue\n", __FUNCTION__));
      goto Cleanup;
    }
    QueueIndex += 1;
  } while (Status == EFI_SUCCESS && QueueIndex <= ItemIndex);

  if (EFI_ERROR (Status)) {
    DEBUG((DEBUG_ERROR, "[%a] - failed to find the queue item at index %d\n", __FUNCTION__, ItemIndex));
    goto Cleanup;
  }

  // Step 2: figure out how big the variable is and allocate the data for it
  Status = gRT->GetVariable (
                  VariableName,
                  QueueGuid,
                  NULL,
                  &VariableDataSize,
                  NULL
                  );
  if (EFI_ERROR (Status) && Status != EFI_BUFFER_TOO_SMALL) {
    DEBUG((DEBUG_ERROR, "[%a] - failed to find variable\n", __FUNCTION__));
    goto Cleanup;
  }
  VariableData = AllocatePool (VariableDataSize);
  if (VariableData == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG((DEBUG_ERROR, "[%a] - failed to allocate resources\n", __FUNCTION__));
    goto Cleanup;
  }

  // step 3: read in the data to the new allocated buffer
  Status = gRT->GetVariable (
                  VariableName,
                  QueueGuid,
                  NULL,
                  &VariableDataSize,
                  VariableData
                  );
  if (EFI_ERROR (Status)) {
    FreePool (VariableData);
    DEBUG((DEBUG_ERROR, "[%a] - failed to read in variable\n", __FUNCTION__));
    goto Cleanup;
  }

  // step 4: set the return pointer correctly
  *ItemData = VariableData;
  *ItemDataSize = VariableDataSize;

Cleanup:
  if (VariableName != NULL) {
    FreePool (VariableName);
    VariableName = NULL;
  }

  return Status;
}
