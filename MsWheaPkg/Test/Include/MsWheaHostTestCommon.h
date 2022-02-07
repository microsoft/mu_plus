/** @file -- MsWheaHostTestCommon.h
Common defintions shared (for convenience) by the host tests.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _MS_WHEA_HOST_TEST_COMMON_H_
#define _MS_WHEA_HOST_TEST_COMMON_H_

#define TEST_RSC_PARENT_CLASS       0xE0000000
#define TEST_RSC_SUBCLASS_CRITICAL  0xA0000
#define TEST_RSC_SUBCLASS_MISC      0xC0000
#define TEST_RSC_CRITICAL_5         (TEST_RSC_PARENT_CLASS | TEST_RSC_SUBCLASS_CRITICAL | 0x00000005)
#define TEST_RSC_CRITICAL_B         (TEST_RSC_PARENT_CLASS | TEST_RSC_SUBCLASS_CRITICAL | 0x0000000B)
#define TEST_RSC_MISC_A             (TEST_RSC_PARENT_CLASS | TEST_RSC_SUBCLASS_CRITICAL | 0x0000000A)
#define TEST_RSC_MISC_C             (TEST_RSC_PARENT_CLASS | TEST_RSC_SUBCLASS_CRITICAL | 0x0000000C)

EFI_GUID  mTestGuid1 = { 0x3b389299, 0xabaf, 0x433b, { 0xa4, 0xa9, 0x23, 0xc8, 0x44, 0x02, 0xfc, 0xad }
};
EFI_GUID  mTestGuid2 = { 0x4c49a3aa, 0xbcb0, 0x544c, { 0xb5, 0xba, 0x34, 0xd9, 0x55, 0x13, 0x0d, 0xbe }
};
EFI_GUID  mTestGuid3 = { 0x5d5ab4bb, 0xcdc1, 0x655d, { 0xc6, 0xcb, 0x45, 0xea, 0x66, 0x24, 0x1e, 0xcf }
};

#endif // _MS_WHEA_HOST_TEST_COMMON_H_
