##
# Copyright (c) 2018, Microsoft Corporation
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
from MuPythonLibrary.ACPI.DMARParser import *

DMARParserVersion = '1.01'

class SystemFirmwareTable(object):

    def __init__(self):
        # enable required SeSystemEnvironmentPrivilege privilege
        privilege = win32security.LookupPrivilegeValue( None, 'SeSystemEnvironmentPrivilege' )
        token = win32security.OpenProcessToken( win32process.GetCurrentProcess(), win32security.TOKEN_READ|win32security.TOKEN_ADJUST_PRIVILEGES )
        win32security.AdjustTokenPrivileges( token, False, [(privilege, win32security.SE_PRIVILEGE_ENABLED)] )
        win32api.CloseHandle( token )

        kernel32 = windll.kernel32
        ntdll    = windll.ntdll

        # import firmware variable APIs
        try:
            self._GetSystemFirmwareTable = kernel32.GetSystemFirmwareTable
            self._GetSystemFirmwareTable.restype = c_int
            self._GetSystemFirmwareTable.argtypes = [c_int, c_int, c_void_p, c_int]

            self._NtQuerySystemInformation = ntdll.NtQuerySystemInformation

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
                    err = kernel32.GetLastError()
                    logging.error( 'GetSystemFirmwareTable failed (GetLastError = 0x%x)' %  err)
                    logging.error(WinError())
                    return (err, None, WinError(err))
            elif length != 0:
                return (err, Table[:length], None)
            err = kernel32.GetLastError()
            logging.error( 'GetSystemFirmwareTable failed (GetLastError = 0x%x)' %  err)
            logging.error(WinError())
            return (err, None, "Length = 0")
        return (-20, None, "GetSystemFirmwareTable is None")


if __name__ == "__main__":

    #
    # 1. Setup: command line args, logger, cleanup before we start, create the text file log, create XML tree with UEFI version and model
    #
    parser = argparse.ArgumentParser(description='DMAR Test Tool')
    parser.add_argument('-i', '--InputXml', dest='input_xml_file', help='Name of the inpit XML file which will contain allowed RMRRs with PCI path', default=None)
    parser.add_argument('-t', '--OutputText', dest='output_text_file', help='Name of the output text file which will contain the DMAR info', default=None)
    parser.add_argument('-x', '--OutputXml', dest='output_xml_file', help='Name of the output XML file which will contain the DMAR info', default=None)
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
    # 2. Inspect DMAR
    #
    table = SystemFirmwareTable()
    (errorcode, data, errorstring) = table.GetAcpiTable(b'DMAR')
    dmar_table = DMAR_TABLE(data)

    DMARTest = dmar_table.DMARBitEnabled()
    ANDDCount = dmar_table.ANDDCount()
    RMRRTest = dmar_table.CheckRMRRCount(options.input_xml_file)

    print("\n")
    print("DMAR bit should be set:\t\tPASS" if DMARTest else  "DMAR bit should be set:\t\tFAIL")
    print("No ANDD structs in DMAR:\tPASS" if  ANDDCount == 0 else  "No ANDD structs in DMAR:\tFAIL")
    print("No RMRR besides exception list:\tPASS" if  RMRRTest else "No RMRR besides exception list:\tFAIL")

    if(options.output_text_file is not None):
        text_log = open(options.output_text_file, 'w')
        text_log.write(str(dmar_table))
        text_log.close()

    if(options.output_xml_file is not None):
        xml_file = open(options.output_xml_file, 'wb')
        xml_file.write(ET.tostring(dmar_table.xml))
        xml_file.close()
