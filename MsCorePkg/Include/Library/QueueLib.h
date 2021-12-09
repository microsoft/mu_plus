/**
  An interface for a queue in UEFI. It works on a first in, first out principle.

  What the queue is backed by (for example, the variable services) or on disk
  is up to the implementation.

  Queues have a specific GUID that they are referenced by. The implementation
  decides how this GUID will be used.

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _QUEUE_LIB_H_
#define _QUEUE_LIB_H_

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
  );

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
  );

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
  );

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
  );

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
  OUT VOID     **ItemData OPTIONAL,
  OUT UINTN    *ItemDataSize OPTIONAL
  );

#endif
