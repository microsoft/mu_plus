# Functions used to parse memory range information from files.
#
# Copyright (C) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

import struct
from ctypes import *
from collections import namedtuple
from MemoryRangeObjects import *
import Globals
import logging
import csv

def ParseFileToBytes(fileName):
    file = open(fileName, "rb")
    d = memoryview(file.read())
    file.close()
    return d.tolist()


def BytesToBinaryString(byte_array):
  binary_strings = [format(b, '08b') for b in byte_array]
  return ' '.join(binary_strings)


def BytesToHexString(byte_array):
  hex_strings = [format(b, '02x') for b in byte_array]
  return ''.join(hex_strings)


def ParseInfoFile(fileName):
    logging.debug("-- Processing file '%s'..." % fileName)
    MemoryRanges = []
    with open(fileName, "r") as file:
        database_reader = csv.reader(file)
        for row in database_reader:
            MemoryRanges.append(MemoryRange(row[0], *row[1:]))
    logging.debug("%d entries found in file %s" % (len(MemoryRanges), fileName))
    return MemoryRanges

def ParsePlatforminfofile(fileName):
    logging.debug("-- Processing file '%s'..." % fileName)
    byte_list = ParseFileToBytes(fileName)
    byte_content = bytes(byte_list)
    data = byte_content.decode('ascii')
    database_reader = csv.reader(data.splitlines())
    for row in database_reader:
        if row[0] == "Architecture":
            Globals.ParsingArchitecture = row[1]
        elif row[0] == "ExecutionLevel":
            Globals.ExecutionLevel = row[1]
        elif row[0] == "Bitwidth":
            Globals.Bitwidth = int(row[1])
        elif row[0] == "Phase":
            Globals.Phase = row[1]
        else:
            logging.error("Unknown key found in PlatformInfo.dat: %s" % row[0])

def Parse4kPages(fileName):
    num = 0
    pages = []
    addressbits = (1 << Globals.Bitwidth) - 1
    logging.debug("-- Processing file '%s'..." % fileName)
    ByteArray = ParseFileToBytes(fileName)
    byteZeroIndex = 0
    if (Globals.ParsingArchitecture == "X64"):
        while (byteZeroIndex + 7) < len(ByteArray):
            if 0 == (ByteArray[byteZeroIndex + 0] + ByteArray[byteZeroIndex + 1] + ByteArray[byteZeroIndex + 2] + ByteArray[byteZeroIndex + 3] + ByteArray[byteZeroIndex + 4] + ByteArray[byteZeroIndex + 5] + ByteArray[byteZeroIndex + 6] + ByteArray[byteZeroIndex + 7]):
                byteZeroIndex += 8
                continue
            Present = ((ByteArray[byteZeroIndex + 0] & 0x1))
            ReadWrite = ((ByteArray[byteZeroIndex + 0] & 0x2) >> 1)
            User = ((ByteArray[byteZeroIndex + 0] & 0x4) >> 2)
            PageTableBaseAddress = (((((ByteArray[byteZeroIndex + 1] & 0xF0) >> 4)) + (ByteArray[byteZeroIndex + 2] << 4) + (ByteArray[byteZeroIndex + 3] << 12) + (ByteArray[byteZeroIndex + 4] << 20) + (ByteArray[byteZeroIndex + 5] << 28) + ((ByteArray[byteZeroIndex + 6] & 0xF) << 36) << 12) & addressbits)
            Nx = ((ByteArray[byteZeroIndex + 7] & 0x80) >> 7)

            byteZeroIndex += 8
            num += 1
            pages.append(MemoryRange("PTEntry", "4k", Present, ReadWrite, Nx, 1, User, (PageTableBaseAddress)))
    elif (Globals.ParsingArchitecture == "AARCH64"):
        while (byteZeroIndex + 7) < len(ByteArray):
            # check if this page is zero
            if 0 == (ByteArray[byteZeroIndex + 0] + ByteArray[byteZeroIndex + 1] + ByteArray[byteZeroIndex + 2] + ByteArray[byteZeroIndex + 3] + ByteArray[byteZeroIndex + 4] + ByteArray[byteZeroIndex + 5] + ByteArray[byteZeroIndex + 6] + ByteArray[byteZeroIndex + 7]):
                byteZeroIndex += 8
                continue
            AccessFlag = ((ByteArray[byteZeroIndex + 1] & 0x4) >> 2)
            IsTable = ((ByteArray[byteZeroIndex + 0] & 0x2) >> 1)
            AccessPermisions = (((ByteArray[byteZeroIndex + 0] & 0xC0) >> 6))
            Shareability = ((ByteArray[byteZeroIndex + 1] & 0x3))
            Pxn = ((ByteArray[byteZeroIndex + 6] & 0x20) >> 5)
            Uxn = ((ByteArray[byteZeroIndex + 6] & 0x40) >> 6)
            PageTableBaseAddress = (int.from_bytes(ByteArray[byteZeroIndex: byteZeroIndex + 8], 'little')) & (0xFFFFFFFFF << 12)
            logging.debug("4KB Page: 0x%s. Access Flag: %d. AccessPermissions: %d. Shareability: %d. Pxn: %d. Uxn: %d. PageTableBaseAddress: %s" % (BytesToHexString(ByteArray[byteZeroIndex : byteZeroIndex + 8]), AccessFlag, AccessPermisions, Shareability, Pxn, Uxn, hex(PageTableBaseAddress)))
            byteZeroIndex += 8
            num += 1
            pages.append(MemoryRange("TTEntry", "4k", AccessFlag, (AccessPermisions & 0x2) >> 1, Shareability, Pxn, Uxn, PageTableBaseAddress, IsTable))

    logging.debug("%d entries found in file %s" % (num, fileName))
    return pages


def Parse2mPages(fileName):
    num = 0
    pages = []
    addressbits = (1 << Globals.Bitwidth) - 1
    logging.debug("-- Processing file '%s'..." % fileName)
    ByteArray = ParseFileToBytes(fileName)
    byteZeroIndex = 0
    if (Globals.ParsingArchitecture == "X64"):
        while (byteZeroIndex + 7) < len(ByteArray):
            if 0 == (ByteArray[byteZeroIndex + 0] + ByteArray[byteZeroIndex + 1] + ByteArray[byteZeroIndex + 2] + ByteArray[byteZeroIndex + 3] + ByteArray[byteZeroIndex + 4] + ByteArray[byteZeroIndex + 5] + ByteArray[byteZeroIndex + 6] + ByteArray[byteZeroIndex + 7]):
                byteZeroIndex += 8
                continue
            Present = ((ByteArray[byteZeroIndex + 0] & 0x1) >> 0)
            ReadWrite = ((ByteArray[byteZeroIndex + 0] & 0x2) >> 1)
            MustBe1 = ((ByteArray[byteZeroIndex + 0] & 0x80) >> 7)
            User = ((ByteArray[byteZeroIndex + 0] & 0x4) >> 2)
            PageTableBaseAddress = (((((ByteArray[byteZeroIndex + 2] & 0xE0) >> 5)) + (ByteArray[byteZeroIndex + 3] << 3) + (ByteArray[byteZeroIndex + 4] << 11) + (ByteArray[byteZeroIndex + 5] << 19) + ((ByteArray[byteZeroIndex + 6] & 0xF) << 27) << 21) & addressbits)
            Nx = ((ByteArray[byteZeroIndex + 7] & 0x80) >> 7)

            byteZeroIndex += 8
            num += 1
            pages.append(MemoryRange("PTEntry", "2m", Present, ReadWrite, Nx, MustBe1, User, (PageTableBaseAddress)))
    elif (Globals.ParsingArchitecture == "AARCH64"):
        while (byteZeroIndex + 7) < len(ByteArray):
            # check if this page is zero
            if 0 == (ByteArray[byteZeroIndex + 0] + ByteArray[byteZeroIndex + 1] + ByteArray[byteZeroIndex + 2] + ByteArray[byteZeroIndex + 3] + ByteArray[byteZeroIndex + 4] + ByteArray[byteZeroIndex + 5] + ByteArray[byteZeroIndex + 6] + ByteArray[byteZeroIndex + 7]):
                byteZeroIndex += 8
                continue
            AccessFlag = ((ByteArray[byteZeroIndex + 1] & 0x4) >> 2)
            IsTable = ((ByteArray[byteZeroIndex + 0] & 0x2) >> 1)
            AccessPermisions = (((ByteArray[byteZeroIndex + 0] & 0xC0) >> 6))
            Shareability = ((ByteArray[byteZeroIndex + 1] & 0x3))
            Pxn = ((ByteArray[byteZeroIndex + 6] & 0x20) >> 5)
            Uxn = ((ByteArray[byteZeroIndex + 6] & 0x40) >> 6)
            PageTableBaseAddress = (int.from_bytes(ByteArray[byteZeroIndex: byteZeroIndex + 8], 'little')) & (0xFFFFFFFFF << 12)
            logging.debug("2MB Page: 0x%s. Access Flag: %d. IsTable: %d AccessPermissions: %d. Shareability: %d. Pxn: %d. Uxn: %d. PageTableBaseAddress: %s" % (BytesToHexString(ByteArray[byteZeroIndex : byteZeroIndex + 8]), AccessFlag, IsTable, AccessPermisions, Shareability, Pxn, Uxn, hex(PageTableBaseAddress)))
            byteZeroIndex += 8
            num += 1
            pages.append(MemoryRange("TTEntry", "2m", AccessFlag, (AccessPermisions & 0x2) >> 1, Shareability, Pxn, Uxn, PageTableBaseAddress, IsTable))

    logging.debug("%d entries found in file %s" % (num, fileName))
    return pages


def Parse1gPages(fileName):
    num = 0
    pages = []
    addressbits = (1 << Globals.Bitwidth) - 1
    logging.debug("-- Processing file '%s'..." % fileName)
    ByteArray = ParseFileToBytes(fileName)
    byteZeroIndex = 0
    if (Globals.ParsingArchitecture == "X64"):
        while (byteZeroIndex + 7) < len(ByteArray):
            if 0 == (ByteArray[byteZeroIndex + 0] + ByteArray[byteZeroIndex + 1] + ByteArray[byteZeroIndex + 2] + ByteArray[byteZeroIndex + 3] + ByteArray[byteZeroIndex + 4] + ByteArray[byteZeroIndex + 5] + ByteArray[byteZeroIndex + 6] + ByteArray[byteZeroIndex + 7]):
                byteZeroIndex += 8
                continue
            Present = ((ByteArray[byteZeroIndex + 0] & 0x1))
            ReadWrite = ((ByteArray[byteZeroIndex + 0] & 0x2) >> 1)
            MustBe1 = ((ByteArray[byteZeroIndex + 0] & 0x80) >> 7)
            User = ((ByteArray[byteZeroIndex + 0] & 0x4) >> 2)
            PageTableBaseAddress = (((((ByteArray[byteZeroIndex + 3] & 0xC0) >> 6)) + (ByteArray[byteZeroIndex + 4] << 2) + (ByteArray[byteZeroIndex + 5] << 10) + ((ByteArray[byteZeroIndex + 6] & 0xF) << 18) << 30) & addressbits) # shift and address bits
            Nx = ((ByteArray[byteZeroIndex + 7] & 0x80) >> 7)

            byteZeroIndex += 8
            pages.append(MemoryRange("PTEntry", "1g", Present, ReadWrite, Nx, MustBe1, User, PageTableBaseAddress))
            num += 1
    elif (Globals.ParsingArchitecture == "AARCH64"):
        while (byteZeroIndex + 7) < len(ByteArray):
            # check if this page is zero
            if 0 == (ByteArray[byteZeroIndex + 0] + ByteArray[byteZeroIndex + 1] + ByteArray[byteZeroIndex + 2] + ByteArray[byteZeroIndex + 3] + ByteArray[byteZeroIndex + 4] + ByteArray[byteZeroIndex + 5] + ByteArray[byteZeroIndex + 6] + ByteArray[byteZeroIndex + 7]):
                byteZeroIndex += 8
                continue
            AccessFlag = ((ByteArray[byteZeroIndex + 1] & 0x4) >> 2)
            IsTable = ((ByteArray[byteZeroIndex + 0] & 0x2) >> 1)
            AccessPermisions = (((ByteArray[byteZeroIndex + 0] & 0xC0) >> 6))
            Shareability = ((ByteArray[byteZeroIndex + 1] & 0x3))
            Pxn = ((ByteArray[byteZeroIndex + 6] & 0x20) >> 5)
            Uxn = ((ByteArray[byteZeroIndex + 6] & 0x40) >> 6)
            PageTableBaseAddress = (int.from_bytes(ByteArray[byteZeroIndex: byteZeroIndex + 8], 'little')) & (0xFFFFFFFFF << 12)
            logging.debug("1GB Page: 0x%s. Access Flag: %d. IsTable: %d AccessPermissions: %d. Shareability: %d. Pxn: %d. Uxn: %d. PageTableBaseAddress: %s" % (BytesToHexString(ByteArray[byteZeroIndex : byteZeroIndex + 8]), AccessFlag, IsTable, AccessPermisions, Shareability, Pxn, Uxn, hex(PageTableBaseAddress)))
            byteZeroIndex += 8
            num += 1
            pages.append(MemoryRange("TTEntry", "1g", AccessFlag, (AccessPermisions & 0x2) >> 1, Shareability, Pxn, Uxn, PageTableBaseAddress, IsTable))

    logging.debug("%d entries found in file %s" % (num, fileName))
    return pages
