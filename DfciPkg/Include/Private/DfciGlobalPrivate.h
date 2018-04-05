/** @file
Contains global macros for DFCI


Copyright (c) 2018, Microsoft Corporation. All rights reserved.

**/


#ifndef DFCI_GLOBAL_PRIVATE_H
#define DFCI_GLOBAL_PRIVATE_H

//
//  PKT_OFFSET (Structure Base Address, Offset to field)
//
#define PKT_FIELD_FROM_OFFSET(Data, Field) ((UINT8 *) (((UINT8 *) Data ) + Field))
#define PKT_FIELD_OFFSET(Field, Base) ((UINT16) ((UINT8 *) Field - (UINT8 *) Base))


#endif // DFCI_GLOBAL_PRIVATE_H