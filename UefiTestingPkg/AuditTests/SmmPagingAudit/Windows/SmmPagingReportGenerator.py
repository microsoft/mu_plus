# Collects files from UEFI app, parses them, and generates a HTML report
#
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

import logging
import operator
import glob
import json
import datetime
import os
import sys
import argparse

#Add script dir to path for import
sp = os.path.dirname(os.path.realpath(sys.argv[0]))
sys.path.append(sp)

from MemoryRangeObjects import *
from BinaryParsing import *


VERSION = "0.80"


class SmmParsingTool(object):

    def __init__(self, DatFolderPath, PlatformName, PlatformVersion):
        self.Logger = logging.getLogger("SmmParsingTool")
        self.MemoryAttributesTable = []
        self.MemoryRangeInfo = []
        self.PageDirectoryInfo = []
        self.DatFolderPath = DatFolderPath
        self.ErrorMsg = []
        self.PlatformName = PlatformName
        self.PlatformVersion = PlatformVersion

    def Parse(self):
        #Get Info Files
        InfoFileList =  glob.glob(os.path.join(self.DatFolderPath, "*MemoryInfo*.dat"))
        Pte1gbFileList =  glob.glob(os.path.join(self.DatFolderPath, "*1G*.dat"))
        Pte2mbFileList =  glob.glob(os.path.join(self.DatFolderPath, "*2M*.dat"))
        Pte4kbFileList =  glob.glob(os.path.join(self.DatFolderPath, "*4K*.dat"))
        MatFileList =  glob.glob(os.path.join(self.DatFolderPath, "*MAT*.dat"))

        logging.debug("Found %d Info Files" % len(InfoFileList))
        logging.debug("Found %d 1gb Page Files" % len(Pte1gbFileList))
        logging.debug("Found %d 2mb Page Files" % len(Pte2mbFileList))
        logging.debug("Found %d 4kb Page Files" % len(Pte4kbFileList))
        logging.debug("Found %d MAT Files" % len(MatFileList))


        # Parse each file, keeping PTEs and "Memory Ranges" seperate
        # Memory ranges are either "memory descriptions" for memory map types and TSEG
        # or "memory contents" for loaded image information or IDT/GDT
        for info in InfoFileList:
            self.MemoryRangeInfo.extend(ParseInfoFile(info))

        for pte1g in Pte1gbFileList:
            self.PageDirectoryInfo.extend(Parse1gPages(pte1g))

        for pte2m in Pte2mbFileList:
            self.PageDirectoryInfo.extend(Parse2mPages(pte2m))

        for pte4k in Pte4kbFileList:
            self.PageDirectoryInfo.extend(Parse4kPages(pte4k))

        for mat in MatFileList:
            self.MemoryAttributesTable.extend(ParseInfoFile(mat))

        if len(self.PageDirectoryInfo) == 0:
            self.ErrorMsg.append("No Memory Range info found in PTE files")
        else:
            # Sort in descending order
            self.PageDirectoryInfo.sort(key=operator.attrgetter('PhysicalStart'))
            #check for Page Table Overlap - this is an error
            index =0
            maxindex = len(self.PageDirectoryInfo) -1
            while index < maxindex:  #this will allow all comparisions to work
                if(self.PageDirectoryInfo[index].overlap(self.PageDirectoryInfo[index+1])):
                    self.ErrorMsg.append("Page Table Entry Overlap.  Index %d Overlapping %d at StartAddress 0x%X" % 
                    (index, index+1, self.PageDirectoryInfo[index].PhysicalStart))
                    logging.error("PTE overlap index %d and %d.  Base Address = 0x%x", index, index+1, self.PageDirectoryInfo[index].PhysicalStart)
                index += 1
        
        if len(self.MemoryRangeInfo) == 0:
            self.ErrorMsg.append("No Memory Range info found in Info files")

        # Matching memory ranges up to page table entries
        for pte in self.PageDirectoryInfo:
            for mr in self.MemoryRangeInfo:
                if pte.overlap(mr):
                    if mr.MemoryType is not None:
                        if pte.MemoryType is None:
                            pte.MemoryType = mr.MemoryType
                        else:
                            logging.error("Multiple memory types found for one region " + pte.pteDebugStr() +" " + mr.MemoryRangeToString())
                            self.ErrorMsg.append("Multiple memory types found for one region.  Base: 0x%X.  EFI Memory Type: %d and %d"% (pte.PhysicalStart, pte.MemoryType,mr.MemoryType))
                    if mr.ImageName is not None:
                        if pte.ImageName is None:
                            pte.ImageName = mr.ImageName
                        else:
                            self.ErrorMsg.append("Multiple memory contents found for one region.  Base: 0x%X.  Memory Contents: %s and %s" % (pte.PhysicalStart, pte.ImageName, mr.ImageName ))
                            logging.error("Multiple memory contents found for one region " +pte.pteDebugStr() + " " +  mr.LoadedImageEntryToString())

                    if(mr.SystemMemoryType is not None):
                        if(pte.SystemMemoryType is None):
                            pte.SystemMemoryType = mr.SystemMemoryType
                        else:
                            self.ErrorMsg.append("Multiple System Memory types found for one region.  Base: 0x%X.  EFI Memory Type: %s and %s."% (pte.PhysicalStart,pte.SystemMemoryType, mr.SystemMemoryType))
                            logging.error("Multiple system memory types found for one region " +pte.pteDebugStr() + " " +  mr.LoadedImageEntryToString())

            for MatEntry in self.MemoryAttributesTable:
                if pte.overlap(MatEntry):
                    pte.Attribute = MatEntry.Attribute

        # Combining adjacent PTEs that have the same attributes.
        index = 0
        while index < (len(self.PageDirectoryInfo) - 1):
            currentPte = self.PageDirectoryInfo[index]
            nextPte = self.PageDirectoryInfo[index + 1]
            if currentPte.sameAttributes(nextPte):
                currentPte.grow(nextPte)
                del self.PageDirectoryInfo[index + 1]
            else:
                index += 1

        return 0

    def AddErrorMsg(self, msg):
        self.ErrorMsg.append(msg)

    def OutputHtmlReport(self, ToolVersion, OutputFilePath):
        # Create the dictionary to produce a JSON string.
        json_dict = {
            'ToolVersion': ToolVersion,
            'PlatformVersion': self.PlatformVersion,
            'PlatformName': self.PlatformName,
            'DateCollected': datetime.datetime.strftime(datetime.datetime.now(), "%A, %B %d, %Y %I:%M%p" ),
        }

        # Process all of the Page Infos and add them to the JSON.
        pde_infos = []
        for pde in self.PageDirectoryInfo:
            info_dict = pde.toDictionary()
            # Check for errors.
            if info_dict['Section Type'] == "ERROR":
                self.AddErrorMsg("Page Descriptor at %s has an error parsing the Section Type." % info_dict['Start'])
            pde_infos.append(info_dict)
        json_dict['MemoryRanges'] = pde_infos

        # Finally, add any errors and produce the JSON string.
        json_dict['errors'] = self.ErrorMsg
        js = json.dumps(json_dict)

        #
        # Open template and replace placeholder with json
        #
        f = open(OutputFilePath, "w")
        template = open(os.path.join(sp, "SmmPaging_template.html"), "r")
        for line in template.readlines():
            if "%TO_BE_FILLED_IN_BY_PYTHON_SCRIPT%" in line:
                line = line.replace("%TO_BE_FILLED_IN_BY_PYTHON_SCRIPT%", js)
            f.write(line)
        template.close()
        f.close()
        return 0

#
# Parse and Validate Args.  Then run the tool
#
def main():

    parser = argparse.ArgumentParser(description='Parse SMM Paging information and generate HTML report')
    parser.add_argument('-i', "--InputFolderPath", dest="InputFolder", help="Path to folder containing the DAT files from the UEFI shell tool (default is CWD)", default=os.getcwd())
    parser.add_argument('-o', "--OutputReport", dest="OutputReport", help="Path to output html report (default is report.html)", default=os.path.join(os.getcwd(), "report.html"))
    parser.add_argument('-p', "--PlatformName", dest="PlatformName", help="Name of Platform.  Will show up on report", default="Test Platform")
    parser.add_argument("--PlatformVersion", dest="PlatformVersion", help="Version of Platform.  Will show up report", default="1.0.0")

    #Turn on dubug level logging
    parser.add_argument("--debug", action="store_true", dest="debug", help="turn on debug logging level for file log",  default=False)
    #Output debug log
    parser.add_argument("-l", dest="OutputLog", help="Create an output log file: ie -l out.txt", default=None)

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

    #Do parameter validation 
    if(options.InputFolder is None or not os.path.isdir(options.InputFolder)):
        logging.critical("Invalid Input Folder Path to folder containing DAT files")
        return -5

    if(options.OutputReport is None):
        logging.critical("No OutputReport Path")
        return -6

    logging.debug("Input Folder Path is: %s" % options.InputFolder)
    logging.debug("Output Report is: %s" % options.OutputReport)

    spt = SmmParsingTool(options.InputFolder, options.PlatformName, options.PlatformVersion)
    spt.Parse()
    return spt.OutputHtmlReport(VERSION, options.OutputReport)

#--------------------------------
# Control starts here
#
#--------------------------------
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
