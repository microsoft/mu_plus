# @file GenerateUnsignedSettingsList.py
#
#
# Generate an XML Permissions packet from a UEFI LOGthe SecuredCorePcds PEI and DXE .inf file, and an include file for the platform
#
#
import argparse
import datetime
import logging
import sys
import xml.etree.cElementTree as ET
import xml.dom.minidom
import re


def main():

    parser = argparse.ArgumentParser(description="Generate a permission " +
                                     "XML from list of settings providers listed in a UEFI log.")

    #
    # Common arguments
    #
    parser.add_argument("-v",
                        "--Verbose",
                        dest="Verbose",
                        action="store_true",
                        default=False,
                        help="set debug logging level")

    parser.add_argument("-l",
                        dest="OutputLog",
                        default=None,
                        help="Create an output log file: ie: -l out.txt")

    parser.add_argument("-i",
                        "--InputFileName",
                        dest="InputFileName",
                        default=None,
                        help="UEFI Log file name")

    parser.add_argument("-o",
                        dest="OutputFileName",
                        default="UnsignedPermissions.xml",
                        help="Generated unsigned permissions file")

    options = parser.parse_args()

    # setup file based logging if OutputLog specified
    if options.OutputLog:
        if options.Verbose:
            debug_level = logging.DEBUG
        else:
            debug_level = logging.INFO

        if options.OutputLog is None or len(options.OutputLog) < 2:
            logging.basicConfig(level=debug_level,
                                format='%(name)s - %(levelname)s - %(message)s')
        else:
            # setup file based logging
            logging.basicConfig(filename=options.OutputLog,
                                level=debug_level,
                                filemode='w',
                                format='%(name)s - %(levelname)s - %(message)s')

    logging.info(f"Log Started: {datetime.datetime.now():%A, %B %d, %Y %I:%M%p}")

    #
    # Initialize the XML tree
    #
    UnsignedPermissionsPacket = ET.Element("PermissionsPacket")
    UnsignedPermissionsPacket.set('xmlns', 'urn:UefiSettings-Schema')

    Comment = ET.Comment('\t@file\n' +
                         '\n'
                         '\tSample Unsigned Permission file.  The following settings will be allowed to be updated\n' +
                         '\tusing unsigned settings packets.  This file must be customized for every platform, and\n' +
                         '\tincluded in the platform .fdf file like:\n' +
                         '\n' +
                         '\tFILE FREEFORM = PCD(gDfciPkgTokenSpaceGuid.PcdUnsignedPermissionsFile) {\n' +
                         '\t\tSECTION RAW = YourPlatform/Somewhere/UnsignedPermissions.xml\n' +
                         '\t}\n' +
                         '\n' +
                         '\tCopyright (c), Microsoft Corporation\n' +
                         '\n' +
                         '\tNOTE:\n' +
                         '\t\tNone of the attribute values for Default, Delegated, Append, PMask, and DMask are used.\n' +
                         '\t\tHowever, they must be present for the parser that reads this xml file.  While this .xml file\n' +
                         '\t\tshows values for these attributes, the values from this file are ignored.\n' +
                         '\t')

    UnsignedPermissionsPacket.insert(1, Comment)  # 1 is the index where comment is inserted

    Permissions = ET.SubElement(UnsignedPermissionsPacket, 'Permissions')
    Permissions.set('Default', '3')   # It it were to be used, 3 == Local User + Unsigned user
    Permissions.set('Delegated', '0')
    Permissions.set('Append', 'False')

    if options.InputFileName is None:
        print("You must specify the file name of a UEFI log file")
        return 0

    try:
        with open(options.InputFileName, "r") as UefiLog:
            Lines = UefiLog.readlines()
    except FileNotFoundError:
        print(f"File {options.InputFileName} not found.")
        return 0

    xmlFile = None
    try:
        xmlFile = open(options.OutputFileName, "w")
        lineIterator = iter([i for i in Lines])
        Search = "START PRINTING ALL REGISTERED SETTING PROVIDERS"
        while True:
            try:
                line = next(lineIterator)  # Load the next element
                if line.__contains__(Search):
                    break

            except StopIteration:
                print("Cannot find start of the settings providers list")

        Search = "END PRINTING ALL REGISTERED SETTING PROVIDERS"
        while True:
            try:
                line = next(lineIterator)  # Load the next element
                if line.__contains__(Search):
                    break

                x = re.sub(' +', ' ', line).split()
                for Idx in range(len(x)):
                    if x[Idx] == "Id:":
                        Permission = ET.SubElement(Permissions, 'Permission')
                        ET.SubElement(Permission, 'Id').text = x[Idx+1]
                        ET.SubElement(Permission, 'PMask').text = '3'  # It it were to be used, 3 == Local User + Unsigned user
                        ET.SubElement(Permission, 'DMask').text = '0'

            except StopIteration:
                print("Cannot find end of the settings providers list")
                raise

        dom = xml.dom.minidom.parseString(ET.tostring(UnsignedPermissionsPacket))
        part1, part2 = dom.toprettyxml().split("?>")

        xmlFile.write(part1 + 'encoding="UTF-8"?>\n' + part2)

    except Exception:
        print(f"Unable to create {options.OutputFileName}.")

    finally:
        if xmlFile is not None:
            xmlFile.close()

    return 0


# --------------------------------------------------------------------------- #
#
#   Entry point
#
# --------------------------------------------------------------------------- #
if __name__ == '__main__':
    # setup main console as logger

    retcode = 16
    try:
        # Run main application
        retcode = main()

    except Exception:
        logging.exception("Exception occurred")

    if retcode != 0:
        logging.critical(f"Failed.  Return Code: {retcode}")

    # end logging
    logging.shutdown()
    sys.exit(retcode)
