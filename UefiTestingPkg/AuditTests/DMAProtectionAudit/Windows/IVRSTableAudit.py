##
# Copyright (C) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
# Python script allowing reading DMAR from Windows, storing result in XML
# or text form.
##

import os, sys
import argparse
from ctypes import *
from collections import namedtuple
import logging
import pywintypes
import win32api, win32process, win32security, win32file
import winerror
import struct
import subprocess
import datetime
import xml.etree.ElementTree as ET
from edk2toollib.acpi.ivrs_parser import *

IVRSParserVersion = '1.00'

class SystemFirmwareTable(object):

    def __init__(self):
        # enable required SeSystemEnvironmentPrivilege privilege
        privilege = win32security.LookupPrivilegeValue( None, 'SeSystemEnvironmentPrivilege' )
        token = win32security.OpenProcessToken( win32process.GetCurrentProcess(), win32security.TOKEN_READ|win32security.TOKEN_ADJUST_PRIVILEGES )
        win32security.AdjustTokenPrivileges( token, False, [(privilege, win32security.SE_PRIVILEGE_ENABLED)] )
        win32api.CloseHandle( token )

        # import firmware variable APIs
        try:
            self._GetSystemFirmwareTable = windll.kernel32.GetSystemFirmwareTable
            self._GetSystemFirmwareTable.restype = c_int
            self._GetSystemFirmwareTable.argtypes = [c_int, c_int, c_void_p, c_int]

            self._NtQuerySystemInformation = windll.ntdll.NtQuerySystemInformation

            # NTSTATUS WINAPI NtQuerySystemInformation(
            #     _In_      SYSTEM_INFORMATION_CLASS SystemInformationClass,
            #     _Inout_   PVOID                    SystemInformation,
            #     _In_      ULONG                    SystemInformationLength,
            #     _Out_opt_ PULONG                   ReturnLength
            # );
            self._NtQuerySystemInformation.restype = c_ulong
            self._NtQuerySystemInformation.argtypes = [c_int, c_void_p, c_ulong, POINTER(c_ulong)]

        except AttributeError:
            logging.error( "GetSystemFirmwareTable function doesn't seem to exist" )
            pass

    def CheckExemptListCount(self, iommu_table, goldenxml=None):
        goldenignores = list()

        if goldenxml is None or not os.path.isfile(goldenxml):
            print("XML File not found")
        else:
            goldenfile = ET.parse(goldenxml)
            goldenroot = goldenfile.getroot()
            for entry in goldenroot:
                if entry.tag == "IVMD":
                    goldenignores.append(entry.attrib)

        for IVMDentry in iommu_table.IVMD_list:
            if not IVMDentry.validateIVMD(goldenignores):
                print("IVMD PCIe Endpoint " + str(IVMDentry) + " found but not in golden XML")
                return False

        return True

    #
    #Function to get an AcpiTable
    # return a tuple of error code, table data, and errorstring (None if not error)
    #
    def GetAcpiTable(self, TableId):
        err = 0 #success
        TableType = struct.unpack(b'>i', b'ACPI')[0]  #big endian
        TableIdAsInt = struct.unpack(b'<i', TableId)[0]  #TableId is Little endian or native
        TableLength = 1000
        Table = create_string_buffer( TableLength )
        if self._GetSystemFirmwareTable is not None:
            logging.info("calling GetSystemFirmwareTable( FwTableProvider=0x%x, FwTableId=0x%X )" % (TableType, TableIdAsInt))
            length = self._GetSystemFirmwareTable(TableType , TableIdAsInt, Table, TableLength)
            if(length > TableLength):
                logging.info("Table Length is: 0x%x" % length)
                Table = create_string_buffer( length )
                length2 = self._GetSystemFirmwareTable( TableType, TableIdAsInt, Table, length)

                if(length2 != length):
                    err = windll.kernel32.GetLastError()
                    logging.error( 'GetSystemFirmwareTable failed (GetLastError = 0x%x)' %  err)
                    logging.error(WinError())
                    return (err, None, WinError(err))
            elif length != 0:
                return (err, Table[:length], None)
            err = windll.kernel32.GetLastError()
            logging.error( 'GetSystemFirmwareTable failed (GetLastError = 0x%x)' %  err)
            logging.error(WinError())
            return (err, None, "Length = 0")
        return (-20, None, "GetSystemFirmwareTable is None")


if __name__ == "__main__":

    #
    # 1. Setup: command line args, logger, cleanup before we start, create the text file log, create XML tree with UEFI version and model
    #
    parser = argparse.ArgumentParser(description='IVRS Test Tool')
    parser.add_argument('-i', '--InputXml', dest='input_xml_file', help='Name of the inpit XML file which will contain allowed exclusions with PCI path', default=None)
    parser.add_argument('-t', '--OutputText', dest='output_text_file', help='Name of the output text file which will contain the IVRS info', default=None)
    parser.add_argument('-x', '--OutputXml', dest='output_xml_file', help='Name of the output XML file which will contain the IVRS info', default=None)
    options = parser.parse_args()

    # Set up logging
    logger = logging.getLogger('')
    logger.setLevel(logging.INFO)
    formatter = logging.Formatter("%(levelname)s - %(message)s")
    console = logging.StreamHandler()
    console.setLevel(logging.CRITICAL)
    console.setFormatter(formatter)
    logger.addHandler(console)

    #
    # 2. Inspect IVRS table
    #
    table = SystemFirmwareTable()
    (errorcode, data, errorstring) = table.GetAcpiTable(b'IVRS')
    if data == None:
        raise Exception ('No IVRS table found, is this a supported platform?')

    iommu_table = IVRS_TABLE(data)

    IVRSTest = iommu_table.IVRSBitEnabled()
    IVMDTest = table.CheckExemptListCount(iommu_table, options.input_xml_file)

    print("\n")
    print("IVRS bit should be set:\t\tPASS" if IVRSTest else  "DMAR bit should be set:\t\tFAIL")
    print("No IVMD besides exception list:\tPASS" if  IVMDTest else "No IVMD besides exception list:\tFAIL")

    if(options.output_text_file is not None):
        sys.stdout = open(options.output_text_file, 'w')
        iommu_table.DumpInfo()
        sys.stdout.close()

    if(options.output_xml_file is not None):
        xml_file = open(options.output_xml_file, 'wb')
        xml_file.write(ET.tostring(iommu_table.ToXmlElementTree()))
        xml_file.close()
