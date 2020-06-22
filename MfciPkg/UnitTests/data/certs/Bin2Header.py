## @file
# Converts binaries to EFI-style C structs
#
# Copyright (c) Microsoft Corporation
# SPDX-License-Identifier: BSD-2-Clause-Patent
##


import argparse


def Bin2Header(BinFilePath, HeaderFilePath, ArrayName):
    inFile = open(BinFilePath, "rb")
    outFile = open(HeaderFilePath, "w")

    outFile.write('  CONST  UINT8    ')
    outFile.write(ArrayName)
    outFile.write('[] = {')

    binData = inFile.read()
    inFile.close()
    binDataSize = len(binData)
    for i in range(0, binDataSize):
        if i != 0:
            outFile.write(',')
        if (i % 16) == 0:
            outFile.write('\n          ')
        else:
            outFile.write(' ')
        outFile.write("".join("0x{:02x}".format(binData[i])))

    outFile.write("};\n")
    outFile.close()


def main():
    parser = argparse.ArgumentParser(
        description='Create Header file from Binary file')

    parser.add_argument("-b", "--BinaryFile", dest="BinaryFilePath",
                        help="Path to Binary File", default=None)
    parser.add_argument("-q", "--HeaderFile", dest="HeaderFilePath",
                        help="Path to created header file", default=None)
    parser.add_argument("-a", "--ArrayName", dest="ArrayName",
                        help="Name for the binary array", default=None)

    options = parser.parse_args()

    Bin2Header(options.BinaryFilePath, options.HeaderFilePath, options.ArrayName)


if __name__ == '__main__':
    main()
