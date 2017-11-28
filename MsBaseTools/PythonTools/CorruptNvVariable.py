## @file UpdateRomVarStore.py
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
import Uefi.EdkII.VariableFormat as VF
import os
import argparse
import struct
import sys

class CorruptableVariableHeader(VF.VariableHeader):
  def __init__(self):
    super(CorruptableVariableHeader, self).__init__()
    self.NameSizeOverride = None
    self.DataSizeOverride = None

  def corrupt_name_size(self, NewNameSize):
    self.NameSizeOverride = NewNameSize

  def corrupt_data_size(self, NewDataSize):
    self.DataSizeOverride = NewDataSize

  def pack_struct(self, with_padding=False):
    vendor_guid = self.VendorGuid.bytes if sys.byteorder == 'big' else self.VendorGuid.bytes_le
    return struct.pack(self.StructString, self.StartId, self.State, 0, self.Attributes,
                        self.NameSize if (self.NameSizeOverride == None) else self.NameSizeOverride, self.DataSize if (self.DataSizeOverride == None) else self.DataSizeOverride, vendor_guid)

class CorruptableAuthenticatedVariableHeader(VF.AuthenticatedVariableHeader):
  def __init__(self):
    super(CorruptableAuthenticatedVariableHeader, self).__init__()
    self.NameSizeOverride = None
    self.DataSizeOverride = None

  def corrupt_name_size(self, NewNameSize):
    self.NameSizeOverride = NewNameSize

  def corrupt_data_size(self, NewDataSize):
    self.DataSizeOverride = NewDataSize

  def pack_struct(self, with_padding=False):
    vendor_guid = self.VendorGuid.bytes if sys.byteorder == 'big' else self.VendorGuid.bytes_le
    return struct.pack(self.StructString, self.StartId, self.State, 0, self.Attributes, self.MonotonicCount,
                        self.TimeStamp, self.PubKeyIndex, self.NameSize if (self.NameSizeOverride == None) else self.NameSizeOverride, self.DataSize if (self.DataSizeOverride == None) else self.DataSizeOverride, vendor_guid)

def GatherArguments():
  parser = argparse.ArgumentParser(description='Corrupt a variable in NV Varstore.')
  parser.add_argument('rom_file', type=str, help='the rom file that will be read and/or modified')
  parser.add_argument('var_store_base', type=str, help='the hex offset of the var store FV within the ROM image')
  parser.add_argument('var_store_size', type=str, help='the hex size of the var store FV within the ROM image')
  parser.add_argument("-d", "--duplicate", help="mark a deleted variable valid, thus creating a duplicate", action="store_true")
  parser.add_argument("-z", "--zerosize", help="mark variable to be size 0, thus breaking a linked list", action="store_true")
  parser.add_argument("-i", "--invalidsize", help="mark variable to be half its original size, thus breaking a linked list", action="store_true")
  parser.add_argument("-l", "--largesize", help="mark variable to have a very large size, large enough to point outside of varstore, thus breaking a linked list", action="store_true")

  return parser.parse_args()

def main():
  args = GatherArguments()

  # Load the variable store from the file.
  var_store = VarStore.VariableStore(args.rom_file, store_base=int(args.var_store_base, 16), store_size=int(args.var_store_size, 16))

  for i, var in enumerate(var_store.variables):

    # Print information about the current variables if debugging
    # print("Var Found: '%s:%s:%s, Attr: %s, NameSize:%s, DataSize:%s'" % (var.VendorGuid, var.Name, hex(var.State), var.Attributes, var.NameSize, var.DataSize))

    if (args.duplicate == True and var.Name == 'BootOrder' and var.State != 0x3f):
      var.State = 0x3f
      break

    if (args.zerosize == True and var.Name == 'BootOrder'):

      var_serialized = var.serialize()
      if var_store.var_store_header.Type == 'Var':
        new_var = CorruptableVariableHeader().load_from_bytes(var_serialized)
      else:
        new_var = CorruptableAuthenticatedVariableHeader().load_from_bytes(var_serialized)
      new_var.corrupt_name_size(0)
      new_var.corrupt_data_size(0)
      var_store.variables[i] = new_var
      break

    if (args.invalidsize == True and var.Name == 'BootOrder' and var.State != 0x3f):
      var_serialized = var.serialize()
      if var_store.var_store_header.Type == 'Var':
        new_var = CorruptableVariableHeader().load_from_bytes(var_serialized)
      else:
        new_var = CorruptableAuthenticatedVariableHeader().load_from_bytes(var_serialized)
      new_var.corrupt_name_size(var.NameSize / 2)
      new_var.corrupt_data_size(var.DataSize / 2)
      var_store.variables[i] = new_var
      break

    if (args.largesize == True and var.Name == 'BootOrder' and var.State != 0x3f):
      var_serialized = var.serialize()
      if var_store.var_store_header.Type == 'Var':
        new_var = CorruptableVariableHeader().load_from_bytes(var_serialized)
      else:
        new_var = CorruptableAuthenticatedVariableHeader().load_from_bytes(var_serialized)
      new_var.corrupt_data_size(0x1f123)
      var_store.variables[i] = new_var
      break

  #
  # Finally, once we have the var store looking the way we want it,
  # let's commit it to file.
  #
  var_store.flush_to_file()

if __name__ == '__main__':
  main()
