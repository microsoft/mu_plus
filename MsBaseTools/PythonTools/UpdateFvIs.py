## @file UpdateFvIs.py
# This is a command-line script that will take in a UEFI ROM file
# and the offset to FV_IS and either adds a driver to it or updates the drivers in it.
#
##
# Copyright (c) 2017, Microsoft Corporation
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

import Uefi.EdkII.PiFirmwareVolume as PiFV
import Uefi.EdkII.PiFirmwareFile as PiFF
import argparse
import glob
import os
import struct
import uuid


def GatherArguments():
    parser = argparse.ArgumentParser(description='Process a command script to alter a ROM image varstore.')
    parser.add_argument('rom_file', type=str, help='the rom file that will be updated')
    parser.add_argument('fv_offset', type=str, help='hex offset of FV in rom') #0x00C80000
    parser.add_argument('ffs_update_file', type=str, help='the updated driver to be applied to rom')

    return parser.parse_args()


ZeroGuid = uuid.UUID('FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF')

def main():
    args = GatherArguments()

    fv_offset = int(args.fv_offset, 16)
    rom_file = open(args.rom_file, 'r+b')
    ffs_update_file = open(args.ffs_update_file, 'r+b')

    # seek to FV base address
    rom_file.seek(fv_offset)
    fv_header = PiFV.EfiFirmwareVolumeHeader().load_from_file(rom_file)
    fv_size = fv_header.FvLength
    fv_ext_header_offset = fv_header.ExtHeaderOffset

    if fv_ext_header_offset != 0:

        # seek to the beginning of the ext header
        rom_file.seek(fv_ext_header_offset, 1)

        # seek to the end of the ext header
        ext_header_length = PiFV.EfiFirmwareVolumeExtHeader().load_from_file(rom_file).ExtHeaderSize
        rom_file.seek(ext_header_length, 1)

    # Get 8 byte aligned
    rom_file.seek(8 - (rom_file.tell() % 8), 1)

    orig = rom_file.tell()
    curr = rom_file.tell()

    update_ffs_header = PiFF.EfiFirmwareFileSystemHeader().load_from_file(ffs_update_file)
    print "File GUID to update:", update_ffs_header.FileSystemGuid

    while curr < (orig + fv_size):
        rom_ffs_header = PiFF.EfiFirmwareFileSystemHeader().load_from_file(rom_file)
        print "Located GUID", rom_ffs_header.FileSystemGuid

        # Check if the GUID matches or if the space in FV_IS is open
        if ((rom_ffs_header.FileSystemGuid == update_ffs_header.FileSystemGuid) or (rom_ffs_header.FileSystemGuid == ZeroGuid)) and ((update_ffs_header.get_size() + curr) < (orig + fv_size)):

            # If we're updating an FFS in place, delete the old one first
            if rom_ffs_header.FileSystemGuid != ZeroGuid:
                for i in range (0, rom_ffs_header.get_size()):
                    rom_file.write(b'\xff')

            # Write update
            rom_file.seek(curr)
            rom_file.write(ffs_update_file.read())

            # Mark FFS header as completed so it will dispatch
            rom_file.seek(curr)
            rom_ffs_header = PiFF.EfiFirmwareFileSystemHeader().load_from_file(rom_file)
            rom_ffs_header.State = 0xF8
            rom_file.write(rom_ffs_header.serialize())

            print "Written!"
            break

        else:
            # jump forward by the length of the ffs
            rom_file.seek(rom_ffs_header.get_size(), 1)
            # the start of the next FFS is going to be 8 bytes aligned
            rom_file.seek(8 - (rom_file.tell() % 8), 1)

        curr = rom_file.tell()

    ffs_update_file.close()
    rom_file.close()

if __name__ == '__main__':
  main()