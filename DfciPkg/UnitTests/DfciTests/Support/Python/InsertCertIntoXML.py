# @file
#
# Convert Bin cert file to base 64 string, and replace the XYZZY string with the
# base 64 string.
#
# Copyright (c), Microsoft Corporation
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

import os, sys

import argparse
import logging
import binascii


#
#main script function
#
def main():
    parser = argparse.ArgumentParser(description='Create SEM Provisioning Cert CSP XML')

    parser.add_argument("--BinFilePath", dest="BinFilePath", help="Path to binary packet", default=None)
    parser.add_argument("--OutputFilePath", dest="OutputFilePath", help="Path to output file", default=None)
    parser.add_argument("--PatternFilePath", dest="PatternFilePath", help="Path to Xml pattern", default=None)

    options = parser.parse_args()

    with open(options.BinFilePath, "rb") as binfile:
        bindata = binfile.read()

    if bindata == None:
        raise Exception ("Invalid binary data")

    b64data = binascii.b2a_base64(bindata).decode("utf-8")

    FoundXYZZY = False
    with open(options.OutputFilePath, "w") as outfile:
        with  open(options.PatternFilePath, "r") as patternfile:
            for pl in patternfile:
                if pl.strip() == "XYZZY":
                    FoundXYZZY = True
                    outfile.write(b64data)
                else:
                    outfile.write(pl)
            if not FoundXYZZY:
                raise Exception ("Invalid pattern data")
    logging.critical("Successfully created XML pkt")
    return 0



if __name__ == '__main__':
    #setup main console as logger
    logger = logging.getLogger('')
    logger.setLevel(logging.DEBUG)
    formatter = logging.Formatter("%(levelname)s - %(message)s")
    console = logging.StreamHandler()
    console.setLevel(logging.CRITICAL)
    console.setFormatter(formatter)
    logger.addHandler(console)

    #call main worker function
    retcode = main()

    if retcode != 0:
        logging.critical("Failed.  Return Code: %i" % retcode)
    #end logging
    logging.shutdown()
    sys.exit(retcode)

