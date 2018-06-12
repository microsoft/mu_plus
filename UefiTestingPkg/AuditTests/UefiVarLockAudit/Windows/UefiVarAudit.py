#
# Script to iterate thru an xml file and 
# check the UEFI variable read/write properties of a given variable  
#
# Copyright (c) 2016, Microsoft Corporation
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
##


import os, sys
import argparse
import logging
import datetime
import struct
import hashlib
import shutil
import time
import xml.etree.ElementTree as ET
from xml.etree.ElementTree import Element
from UefiVariablesSupportLib import UefiVariable

#
#main script function
#
def main():

    parser = argparse.ArgumentParser(description='Variable Audit Tool')

    #Output debug log
    parser.add_argument("-l", dest="OutputLog", help="Create an output log file: ie -l out.txt", default=None)
    parser.add_argument("--OutputXml", dest="OutputXml", help="Output Xml file that contains final results", default=None)
    parser.add_argument("--InputXml", dest="InputXml", help="Input Xml file", default=None)
    
    #Turn on dubug level logging
    parser.add_argument("--debug", action="store_true", dest="debug", help="turn on debug logging level for file log",  default=False)
    options = parser.parse_args()

    #setup file based logging if outputReport specified
    if(options.OutputLog):
        if(len(options.OutputLog) < 2):
            logging.critical("the output log file parameter is invalid")
            return -2
        else:
            #setup file based logging
            filelogger = logging.FileHandler(filename=options.OutputLog, mode='w')
            if(options.debug):
                filelogger.setLevel(logging.DEBUG)
            else:
                filelogger.setLevel(logging.INFO)

            filelogger.setFormatter(formatter)
            logging.getLogger('').addHandler(filelogger)

    logging.info("Log Started: " + datetime.datetime.strftime(datetime.datetime.now(), "%A, %B %d, %Y %I:%M%p" ))

    #Check for required input parameters
    if(not options.InputXml) or (not os.path.isfile(options.InputXml)):  
        logging.critical("No Input Xml file specified")
        return -1

    if(not options.OutputXml):
        logging.critical("Output Xml file path not specified")
        return -2
    
    Uefi = UefiVariable()

    #read in XML file as doc    
    XmlFile = ET.parse(options.InputXml)
    XmlRoot = XmlFile.getroot()

    for var in XmlRoot.findall("Variable"):
        name = var.get("Name")
        guid = var.get("Guid")
        (ReadStatus, Data, ReadErrorString) = Uefi.GetUefiVar(name, guid)
        (WriteSuccess, ErrorCode, WriteErrorString)= Uefi.SetUefiVar(name, guid)
        if(WriteSuccess != 0):
            logging.info("Must Restore Var %s:%s" % (name, guid))
            (RestoreSuccess, RestoreEC, RestoreErrorString) = Uefi.SetUefiVar(name, guid, Data)
            if (RestoreSuccess == 0):
                logging.critical("Restoring failed for Var %s:%s  0x%X  ErrorCode: 0x%X %s" % (name, guid, RestoreSuccess, RestoreEC, RestoreErrorString))
        #append
        #<FromOs>
        #<ReadStatus>0x0 Success</ReadStatus>
        #<WriteStatus>0x8000000000000002 Invalid Parameter</WriteStatus>  
        ele = Element("FromOs")
        rs = Element("ReadStatus")
        ws = Element("WriteStatus")
        rs.text = "0x%lX" % (ReadStatus)
        if(ReadErrorString is not None):
            rs.text = rs.text + " %s" % ReadErrorString
        ws.text = "0x%lX" % ErrorCode
        if(WriteErrorString is not None):
            ws.text = ws.text + " %s" % WriteErrorString
        ele.append(rs)
        ele.append(ws)
        var.append(ele)

    XmlFile.write(options.OutputXml)
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
