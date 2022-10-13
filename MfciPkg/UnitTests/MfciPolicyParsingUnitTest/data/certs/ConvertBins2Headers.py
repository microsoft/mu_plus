## @file
# Converts all .cer files in a supplied directory
# into C-struct header files
#
# Copyright (c) Microsoft Corporation
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

import argparse
import os

from Bin2Header import Bin2Header


def main():
    parser = argparse.ArgumentParser(
        description='Create Header files from Binary files')

    parser.add_argument("-p", "--BlobsPath", dest="BlobsPath",
                        help="Path to Binary File", default=None)
    options = parser.parse_args()
    with os.scandir(options.BlobsPath) as it:
        for entry in it:
            if  entry.is_file():

                ext2var = {
                    '.cer' : 'mCert_',
                    '.bin' : 'mBin_',
                    '.p7'  : 'mSigned_'
                }

                ext = os.path.splitext(entry)[1]
                if ext not in ext2var:
                    continue
                arrayName = ext2var[ext] + entry.name[:entry.name.find('.')]
                headerFilename = entry.name + ".h"
                #print(entry.name, headerFilename, arrayName)
                Bin2Header(entry.name, headerFilename, arrayName)


if __name__ == '__main__':
    main()
