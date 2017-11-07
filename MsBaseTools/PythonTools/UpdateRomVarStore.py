## @file UpdateRomVarStore.py
# This is a command-line script that will take in a UEFI ROM file
# and the path to an XML data file and update all the variables in the
# UEFI ROM var store to match the contents of the XML file.
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
import uuid
import argparse
import xml.etree.ElementTree as ET
import binascii

def GatherArguments():
  parser = argparse.ArgumentParser(description='Process a command script to alter a ROM image varstore.')
  parser.add_argument('rom_file', type=str, help='the rom file that will be read and/or modified')
  parser.add_argument('var_store_base', type=str, help='the hex offset of the var store FV within the ROM image')
  parser.add_argument('var_store_size', type=str, help='the hex size of the var store FV within the ROM image')
  parser.add_argument('-s', '--set', metavar='set_script', dest='set_file', type=str, help='a script file containing variables that should be updated or created in the variable store')

  return parser.parse_args()

def load_variable_xml(xml_file_path):
  loaded_xml_tree = ET.parse(xml_file_path)
  loaded_xml = loaded_xml_tree.getroot()

  loaded_vars = []
  for node in loaded_xml.findall('Variable'):
    new_var = {
      'name': node.find('Name').text,
      'guid': uuid.UUID(node.find('GUID').text),
      'attributes': int(node.find('Attributes').text, 16)
    }

    # Make sure we can process the data.
    if node.find('Data').get('type') != 'hex':
      raise Exception("Unknown data type '%s' found!" % node.find('Data').get('type'))

    # Update the node data.
    if node.find('Data').get('type') == 'hex':
      # NOTE: Switched to using binascii for Python2/3 code flexibility
      # bytes.fromhex() works for Python3 and "".decode('hex') works for Python2
      # new_var['data'] = bytes.fromhex(node.find('Data').text)
      new_var['data'] = binascii.unhexlify(node.find('Data').text)

    # Add the var to the list.
    loaded_vars.append(new_var)

  return loaded_vars

def main():
  args = GatherArguments()

  if args.set_file is not None and not os.path.isfile(args.set_file):
    raise Exception("Set Script path '%s' does not point to a valid file!" % args.set_file)

  # Load the variable store from the file.
  var_store = VarStore.VariableStore(args.rom_file, store_base=int(args.var_store_base, 16), store_size=int(args.var_store_size, 16))

  # Print information about the current variables.
  for var in var_store.variables:
    if var.State == VF.VAR_ADDED:
      print("Var Found: '%s:%s'" % (var.VendorGuid, var.Name))

  # Attempt to load the set script file.
  set_vars = load_variable_xml(args.set_file)
  create_vars = []
  # Walk through all the script variables...
  # Then walk through all existing variables in the var store...
  # If we find a match, we can update it in place.
  # Otherwise, add it to a list to create later.
  for set_var in set_vars:
    match_found = False

    for var in var_store.variables:
      if var.Name == set_var['name'] and var.VendorGuid == set_var['guid'] and var.State == VF.VAR_ADDED:
        print("Updating Var '%s:%s'..." % (var.VendorGuid, var.Name))

        match_found = True
        var.Attributes = set_var['attributes']
        var.set_data(set_var['data'])

    if not match_found:
      create_vars.append(set_var)

  # If we had variables we were unable to update, let's create them now.
  for create_var in create_vars:
    print("Creating Var '%s:%s'..." % (create_var['guid'], create_var['name']))

    new_var = var_store.get_new_var_class()
    new_var.Attributes = create_var['attributes']
    new_var.VendorGuid = create_var['guid']
    new_var.set_name(create_var['name'])
    new_var.set_data(create_var['data'])

    var_store.variables.append(new_var)

  #
  # Finally, once we have the var store looking the way we want it,
  # let's commit it to file.
  #
  var_store.flush_to_file()

if __name__ == '__main__':
  main()
