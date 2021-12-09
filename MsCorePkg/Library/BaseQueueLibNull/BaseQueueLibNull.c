/**
  An NULL implementation of the library for the generic queue.

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi/UefiBaseType.h>
#include <Uefi/UefiSpec.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

/**
  Gets the number of items currently in the queue.

  @param[in]  QueueGuid     The Identifier of the queue in question
  @param[out] ItemCount     The number of items that have been queued.

  @retval     EFI_SUCCESS   Everything went as expected.
  @retval     Other         Something went horribly horribly wrong
**/
EFI_STATUS
EFIAPI
GetQueueItemCount (
  IN  EFI_GUID *QueueGuid,
  OUT UINTN    *ItemCount
  )
{
  DEBUG ((DEBUG_ERROR, "[%a] This is a NULL lib and should not be used in production.\n", __FUNCTION__));
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
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
  DEBUG ((DEBUG_ERROR, "[%a] This is a NULL lib and should not be used in production.\n", __FUNCTION__));
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}

/**
  Pops an item from the queue at a specified index.

  This function allocates a buffer to hold the item from the queue.
  It is expected that the caller will free this allocated memory.

  The ItemData parameters are optional if NULLs are supplied, the item will
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
  DEBUG ((DEBUG_ERROR, "[%a] This is a NULL lib and should not be used in production.\n", __FUNCTION__));
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
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
  OUT VOID     **ItemData,
  OUT UINTN    *ItemDataSize
  )
{
  DEBUG ((DEBUG_ERROR, "[%a] This is a NULL lib and should not be used in production.\n", __FUNCTION__));
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
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
  DEBUG ((DEBUG_ERROR, "[%a] This is a NULL lib and should not be used in production.\n", __FUNCTION__));
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}
