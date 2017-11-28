## @file InterruptedVarstoreReclaim.py
# This is a command-line script that will take in a UEFI ROM file
# and modify the NVRAM to corrupt a BootOrder variable. This is only to
# be used as a script to create a ROM image for an NVRAM corruption test.
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

import Uefi.VariableStoreManipulation as VarStore
import Uefi.FtwWorkingBlockFormat as FTW
import os
import argparse
import struct
import sys
import uuid

def GatherArguments():
    parser = argparse.ArgumentParser(description='Corrupt a variable in NV Varstore.')
    parser.add_argument('rom_file', type=str, help='the rom file that will be read and/or modified')
    parser.add_argument('var_store_base', type=str, help='the hex offset of the var store FV within the ROM image')
    parser.add_argument('var_store_size', type=str, help='the hex size of the var store FV within the ROM image')
    parser.add_argument('spare_block_base', type=str, help='the hex offset of FTW spare block within the ROM image')
    return parser.parse_args()

def main():
  args = GatherArguments()

  # 1. Load the variable store from the file.
  var_store = VarStore.VariableStore(args.rom_file, store_base=int(args.var_store_base, 16), store_size=int(args.var_store_size, 16))

  # debug code:
  # print ("varstore:")

  # 2. Get rid of all the invalid variables
  valid_vars = []
  for var in var_store.variables:

    # debug code:
    # print("Var Found: '%s:%s:%s, Attr: %s, NameSize:%s, DataSize:%s'" % (var.VendorGuid, var.Name, hex(var.State), var.Attributes, var.NameSize, var.DataSize))

    if (var.State == 0x3f):
      valid_vars.append(var)

  # 3. Copy over the FV and varstore headers to the spare block base
  spare_base_offset = int(args.spare_block_base, 16)
  var_store.rom_file_map[spare_base_offset:(spare_base_offset + var_store.fv_header.HeaderLength)] = var_store.fv_header.serialize()

  spare_base_vs_header_offset = spare_base_offset + var_store.fv_header.HeaderLength
  var_store.rom_file_map[spare_base_vs_header_offset:(spare_base_vs_header_offset + var_store.var_store_header.StructSize)] = var_store.var_store_header.serialize()
  var_store.rom_file_map.flush()

  # 4. Flush the varstore to the spare block
  var_store.store_base = int(args.spare_block_base, 16)
  var_store.variables = valid_vars
  var_store.flush_to_file()

  # debug code:
  # print("spare block:")
  # var_store = VarStore.VariableStore(args.rom_file, store_base=int(args.spare_block_base, 16), store_size=int(args.var_store_size, 16))
  # for var in var_store.variables:
    # print("Var Found: '%s:%s:%s, Attr: %s, NameSize:%s, DataSize:%s'" % (var.VendorGuid, var.Name, hex(var.State), var.Attributes, var.NameSize, var.DataSize))

  # 4. Write FFs instead of the varstore
  var_store_erased = []
  for i in range(0x20000):
      var_store_erased.append(0xff)
  ErasedVarStoreByteArray = bytearray(var_store_erased)
  rom = open(args.rom_file, 'r+b')
  rom.seek(int(args.var_store_base, 16))
  rom.write(ErasedVarStoreByteArray)
  rom.close()

  # 5. Fill out the working block with a header, a record header, and a record, then flush it to file
  WorkingBlockHeader = FTW.EfiFtwWorkingBlockHeader()
  # EdkiiWorkingBlockSignatureGuid , (0x9E58292B, 0x7C68, 0x497D, 0xA0, 0xCE, 0x6500FD9F1B95))
  WorkingBlockHeader.Signature = uuid.UUID(fields=(0x9E58292B, 0x7C68, 0x497D, 0xA0, 0xCE, 0x6500FD9F1B95))
  WorkingBlockHeader.Crc = 66204642 # pre-calculated
  WorkingBlockHeader.WorkingBlockValidFields = 254 # 0xFE: working block valid, thus bit 0 clear (means valid), bit 1 set
  WorkingBlockHeader.Reserved1 = 255 # padding
  WorkingBlockHeader.Reserved2 = 255 # padding
  WorkingBlockHeader.Reserved3 = 255 # padding
  WorkingBlockHeader.WriteQueueSize = 8160 # 8K reserved for working block minus 32 bytes of header size

  WriteHeader = FTW.EfiFtwWriteHeader()
  WriteHeader.StatusBits = 252 # 0xFC: 0 in bit0 for "HeaderAllocated", 0 in bit1 for "WritesAllocated"
  WriteHeader.ReservedByte1 = 255 # padding
  WriteHeader.ReservedByte2 = 255 # padding
  WriteHeader.ReservedByte3 = 255 # padding
  # SmmFaultTolerantWriteDxe: 470CB248-E8AC-473c-BB4F-81069A1FE6FD
  WriteHeader.CallerId = '48b20c47ace83c47bb4f81069a1fe6fd'.decode('hex')
  WriteHeader.ReservedUint32 = 4294967295 # padding
  WriteHeader.NumberOfWrites = 1
  WriteHeader.PrivateDataSize = 0

  WriteRecord = FTW.EfiFtwWriteRecord()
  WriteRecord.StatusBits = 253 # 0xFD: bit1 cleared for "SpareComplete", others set since not complete
  WriteRecord.ReservedByte1 = 255 # padding
  WriteRecord.ReservedByte2 = 255 # padding
  WriteRecord.ReservedByte3 = 255 # padding
  WriteRecord.ReservedUint32 = 4294967295 # padding
  WriteRecord.Lba = 0
  WriteRecord.Offset = 72 # FV Header doesn't need to be overwritten
  WriteRecord.Length = 122808 # 0x1e000 minus the header
  WriteRecord.RelativeOffset = 18446744073709420544 # 0xfffffffffffe0000
  
  rom = open(args.rom_file, 'r+b')
  rom.seek(int(args.spare_block_base, 16) + int('1E000', 16))
  rom.write(WorkingBlockHeader.serialize())
  rom.write(WriteHeader.serialize())
  rom.write(WriteRecord.serialize())
  rom.close()

if __name__ == '__main__':
  main()
