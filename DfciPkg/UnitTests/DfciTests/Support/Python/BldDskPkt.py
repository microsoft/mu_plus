# @file
#
# Build a JSON packet for the USB Refresh option
#
# Copyright (c), Microsoft Corporation
# SPDX-License-Identifier: BSD-2-Clause-Patent

##
##
## Convert each of the bin files to Base64 then add each of them to a JSON element.
##

import os, sys

import argparse
import base64

delimiter = ''

def set_delimiter (new_delimiter):
    global delimiter
    delimiter = new_delimiter

def AddSection (outFile, sectionName, sectionFileName):


    if (not sectionFileName):
        print ("Skipping section %s" % sectionName)
        return

    try:
        with open(sectionFileName, "rb") as binfile:
            bindata = binfile.read()

    except FileNotFoundError:
        print ("File %s not found" % sectionFileName)
        return

    if bindata == None:
        raise Exception ("Invalid binary data from %s" %sectionName)

    b64data = base64.b64encode(bindata)
    if b64data == None:
        raise Exception ("Unable to convert bindata to b64data data from %s" %sectionName)

    outFile.write (delimiter)
    outFile.write (' "')
    outFile.write (sectionName)
    outFile.write ('" : "')
    outFile.write (b64data.decode("utf-8"))
    outFile.write ('"')
    set_delimiter (',\r\n')

#
#main script function
#
def main():
    parser = argparse.ArgumentParser(description='Create USB Json packet file')

    parser.add_argument("-i",  "--Identity", dest="IdFilePath", help="Path to Identity packet", default=None)
    parser.add_argument("-i2", "--Identity2", dest="Id2FilePath", help="Path to Identity2 packet", default=None)
    parser.add_argument("-p",  "--Permission", dest="PermFilePath", help="Path to Permission packet", default=None)
    parser.add_argument("-p2", "--Permission2", dest="Perm2FilePath", help="Path to Permission2 packet", default=None)
    parser.add_argument("-s",  "--Settings", dest="SettingsFilePath", help="Path to Settings packet", default=None)
    parser.add_argument("-s2", "--Settings2", dest="Settings2FilePath", help="Path to Settings2 packet", default=None)
    parser.add_argument("-t",  "--Transition1", dest="Transition1FilePath", help="Path to Transition1 packet", default=None)
    parser.add_argument("-t2", "--Transition2", dest="Transition2FilePath", help="Path to Transition2 packet", default=None)
    parser.add_argument("-o",  "--OutputFilePath", dest="OutputFilePath", help="Path to output file", default=None)

    options = parser.parse_args()

    outFile = open (options.OutputFilePath, "w")

    set_delimiter ('{')
    AddSection (outFile, 'ProvisioningPacket', options.IdFilePath)
    AddSection (outFile, 'ProvisioningPacket2', options.Id2FilePath)
    AddSection (outFile, 'PermissionsPacket', options.PermFilePath)
    AddSection (outFile, 'PermissionsPacket2', options.Perm2FilePath)
    AddSection (outFile, 'TransitionPacket1', options.Transition1FilePath)
    AddSection (outFile, 'TransitionPacket2', options.Transition2FilePath)
    AddSection (outFile, 'SettingsPacket', options.SettingsFilePath)
    AddSection (outFile, 'SettingsPacket2', options.Settings2FilePath)

    if (delimiter == '{'):
        raise exception ("No package written")

    outFile.write ("}");
    outFile.close();

if __name__ == '__main__':

    main()
