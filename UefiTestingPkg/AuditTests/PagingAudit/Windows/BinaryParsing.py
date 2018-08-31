# Functions used to parse memory range information from files.
#
# Copyright (c) 2016, Microsoft Corporation
#
# All rights reserved.
# Redistribution and use in source and binary forms, with or without 
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
##

import struct
from ctypes import *
from collections import namedtuple
from MemoryRangeObjects import *
import logging
import csv

ADDRESS_BITS = 0x0000007FFFFFF000

def ParseFileToBytes(fileName):
    file = open(fileName, "rb")
    d = memoryview(file.read())
    file.close()
    return d.tolist()


def ParseInfoFile(fileName):
    logging.debug("-- Processing file '%s'..." % fileName)
    MemoryRanges = []
    with open(fileName, "r") as file:
        database_reader = csv.reader(file)
        for row in database_reader:
            MemoryRanges.append(MemoryRange(row[0], *row[1:]))
    logging.debug("%d entries found in file %s" % (len(MemoryRanges), fileName))
    return MemoryRanges


def Parse4kPages(fileName):
    num = 0
    pages = []
    logging.debug("-- Processing file '%s'..." % fileName)
    ByteArray = ParseFileToBytes(fileName)
    byteZeroIndex = 0
    while (byteZeroIndex + 7) < len(ByteArray):
        if 0 == (ByteArray[byteZeroIndex + 0] + ByteArray[byteZeroIndex + 1] + ByteArray[byteZeroIndex + 2] + ByteArray[byteZeroIndex + 3] + ByteArray[byteZeroIndex + 4] + ByteArray[byteZeroIndex + 5] + ByteArray[byteZeroIndex + 6] + ByteArray[byteZeroIndex + 7]):
            byteZeroIndex += 8
            continue
        Present = ((ByteArray[byteZeroIndex + 0] & 0x1))
        ReadWrite = ((ByteArray[byteZeroIndex + 0] & 0x2) >> 1)
        User = ((ByteArray[byteZeroIndex + 0] & 0x4) >> 2)
        # PageTableBaseAddress is 40 bits long
        PageTableBaseAddress = (((((ByteArray[byteZeroIndex + 1] & 0xF0) >> 4)) + (ByteArray[byteZeroIndex + 2] << 4) + (ByteArray[byteZeroIndex + 3] << 12) + (ByteArray[byteZeroIndex + 4] << 20) + (ByteArray[byteZeroIndex + 5] << 28) + ((ByteArray[byteZeroIndex + 6] & 0xF) << 36) << 12) & ADDRESS_BITS)
        Nx = ((ByteArray[byteZeroIndex + 7] & 0x80) >> 7)
        if Present == 0:
            raise Exception ("Data error")

        byteZeroIndex += 8
        num += 1
        pages.append(MemoryRange("PTEntry", "4k", ReadWrite, Nx, 1, User, (PageTableBaseAddress)))
    logging.debug("%d entries found in file %s" % (num, fileName))
    return pages


def Parse2mPages(fileName):
    num = 0
    pages = []
    logging.debug("-- Processing file '%s'..." % fileName)
    ByteArray = ParseFileToBytes(fileName)
    byteZeroIndex = 0
    while (byteZeroIndex + 7) < len(ByteArray):
        if 0 == (ByteArray[byteZeroIndex + 0] + ByteArray[byteZeroIndex + 1] + ByteArray[byteZeroIndex + 2] + ByteArray[byteZeroIndex + 3] + ByteArray[byteZeroIndex + 4] + ByteArray[byteZeroIndex + 5] + ByteArray[byteZeroIndex + 6] + ByteArray[byteZeroIndex + 7]):
            byteZeroIndex += 8
            continue
        Present = ((ByteArray[byteZeroIndex + 0] & 0x1) >> 0)
        ReadWrite = ((ByteArray[byteZeroIndex + 0] & 0x2) >> 1)
        MustBe1 = ((ByteArray[byteZeroIndex + 0] & 0x80) >> 7)
        User = ((ByteArray[byteZeroIndex + 0] & 0x4) >> 2)
        # PageTableBaseAddress is 31 bits long
        PageTableBaseAddress = (((((ByteArray[byteZeroIndex + 2] & 0xE0) >> 5)) + (ByteArray[byteZeroIndex + 3] << 3) + (ByteArray[byteZeroIndex + 4] << 11) + (ByteArray[byteZeroIndex + 5] << 19) + ((ByteArray[byteZeroIndex + 6] & 0xF) << 27) << 21) & ADDRESS_BITS)
        Nx = ((ByteArray[byteZeroIndex + 7] & 0x80) >> 7)

        byteZeroIndex += 8
        num += 1
        pages.append(MemoryRange("PTEntry", "2m", ReadWrite, Nx, MustBe1, User, (PageTableBaseAddress)))
    logging.debug("%d entries found in file %s" % (num, fileName))
    return pages


def Parse1gPages(fileName):
    num = 0
    pages = []
    logging.debug("-- Processing file '%s'..." % fileName)
    ByteArray = ParseFileToBytes(fileName)
    byteZeroIndex = 0
    while (byteZeroIndex + 7) < len(ByteArray):
        if 0 == (ByteArray[byteZeroIndex + 0] + ByteArray[byteZeroIndex + 1] + ByteArray[byteZeroIndex + 2] + ByteArray[byteZeroIndex + 3] + ByteArray[byteZeroIndex + 4] + ByteArray[byteZeroIndex + 5] + ByteArray[byteZeroIndex + 6] + ByteArray[byteZeroIndex + 7]):
            byteZeroIndex += 8
            continue
        Present = ((ByteArray[byteZeroIndex + 0] & 0x1))
        ReadWrite = ((ByteArray[byteZeroIndex + 0] & 0x2) >> 1)
        MustBe1 = ((ByteArray[byteZeroIndex + 0] & 0x80) >> 7)
        User = ((ByteArray[byteZeroIndex + 0] & 0x4) >> 2)
        # PageTableBaseAddress is 22 bits long
        PageTableBaseAddress = (((((ByteArray[byteZeroIndex + 3] & 0xC0) >> 6)) + (ByteArray[byteZeroIndex + 4] << 2) + (ByteArray[byteZeroIndex + 5] << 10) + ((ByteArray[byteZeroIndex + 6] & 0xF) << 18) << 30) & ADDRESS_BITS) # shift and address bits
        Nx = ((ByteArray[byteZeroIndex + 7] & 0x80) >> 7)

        byteZeroIndex += 8
        pages.append(MemoryRange("PTEntry", "1g", ReadWrite, Nx, MustBe1, User, PageTableBaseAddress))
        num += 1
    logging.debug("%d entries found in file %s" % (num, fileName))
    return pages