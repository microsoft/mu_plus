/** @file
Contains global macros for DFCI


Copyright (c) 2018, Microsoft Corporation. All rights reserved.

**/


#ifndef DFCI_GLOBAL_PRIVATE_H
#define DFCI_GLOBAL_PRIVATE_H

//
//  PKT_FIELD_FROM_OFFSET (Packet Base Address, Offset to field)
//
//  This macro returns a UINT8 * to the field at offet in the packet.
//

#define PKT_FIELD_FROM_OFFSET(Data, Offset) ((UINT8 *) (((UINT8 *) Data ) + Offset))


#endif // DFCI_GLOBAL_PRIVATE_H