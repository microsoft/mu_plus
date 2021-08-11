/** @file
    Advanced Logger Common function declaration


    Copyright (C) Microsoft Corporation. All rights reserved.
    SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __ADVANCED_LOGGER_INTERNAL_PROTOCOL_H__
#define __ADVANCED_LOGGER_INTERNAL_PROTOCOL_H__

typedef struct {
    ADVANCED_LOGGER_PROTOCOL  AdvLoggerProtocol;
    ADVANCED_LOGGER_INFO     *LoggerInfo;
} ADVANCED_LOGGER_PROTOCOL_CONTAINER;

#define LOGGER_INFO_FROM_PROTOCOL(a) (BASE_CR (a, ADVANCED_LOGGER_PROTOCOL_CONTAINER, AdvLoggerProtocol)->LoggerInfo)


#endif  // __ADVANCED_LOGGER_INTERNAL_PROTOCOL_H__
