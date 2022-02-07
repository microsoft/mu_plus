/** @file
DfciGlobalPrivate.h

Contains global macros and structures for DFCI

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef DFCI_GLOBAL_PRIVATE_H
#define DFCI_GLOBAL_PRIVATE_H

//
//  PKT_FIELD_FROM_OFFSET (Packet Base Address, Offset to field)
//
//  This macro returns a UINT8 * to the field at offset in the packet.
//

#define PKT_FIELD_FROM_OFFSET(Data, Offset)  ((UINT8 *) (((UINT8 *) Data ) + Offset))

#endif // DFCI_GLOBAL_PRIVATE_H
