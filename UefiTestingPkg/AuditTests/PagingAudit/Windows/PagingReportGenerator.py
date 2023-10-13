# Collects files from UEFI app, parses them, and generates a HTML report
#
# Copyright (C) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

import logging
import operator
import glob
import json
import datetime
import os
import sys
import argparse
import Globals

#Add script dir to path for import
sp = os.path.dirname(os.path.realpath(sys.argv[0]))
sys.path.append(sp)

from MemoryRangeObjects import *
from BinaryParsing import *


VERSION = "0.90"


class ParsingTool(object):

    def __init__(self, DatFolderPath, PlatformName, PlatformVersion):
        self.Logger = logging.getLogger("ParsingTool")
        self.MemoryAttributesTable = []
        self.MemoryRangeInfo = []
        self.PageDirectoryInfo = []
        self.DatFolderPath = DatFolderPath
        self.ErrorMsg = []
        self.PlatformName = PlatformName
        self.PlatformVersion = PlatformVersion
        self.AddressBits = 0

    def Parse(self):
        #Get Info Files
        InfoFileList =  glob.glob(os.path.join(self.DatFolderPath, "*MemoryInfo*.dat"))
        Page1gbFileList =  glob.glob(os.path.join(self.DatFolderPath, "*1G*.dat"))
        Page2mbFileList =  glob.glob(os.path.join(self.DatFolderPath, "*2M*.dat"))
        Page4kbFileList =  glob.glob(os.path.join(self.DatFolderPath, "*4K*.dat"))
        MatFileList =  glob.glob(os.path.join(self.DatFolderPath, "*MAT*.dat"))
        GuardPageFileList =  glob.glob(os.path.join(self.DatFolderPath, "*GuardPage*.dat"))
        PlatformInfoFileList = glob.glob(os.path.join(self.DatFolderPath, "*PlatformInfo*.dat"))

        logging.debug("Found %d Info Files" % len(InfoFileList))
        logging.debug("Found %d 1gb Page Files" % len(Page1gbFileList))
        logging.debug("Found %d 2mb Page Files" % len(Page2mbFileList))
        logging.debug("Found %d 4kb Page Files" % len(Page4kbFileList))
        logging.debug("Found %d MAT Files" % len(MatFileList))
        logging.debug("Found %d GuardPage Files" % len(GuardPageFileList))
        logging.debug("Found %d Platform Info Files" % len(PlatformInfoFileList))

        for info in PlatformInfoFileList:
            ParsePlatforminfofile(info)

        if Globals.ParsingArchitecture == "":
            raise Exception("Architecture not Identified in PlatformInfo.dat")
        elif Globals.ParsingArchitecture == "AARCH64" and Globals.ExecutionLevel == "":
            logging.error("Architecture is AARCH64 but Execution Level wasn't specified. Assuming EL2")
            Globals.ExecutionLevel = "EL2"
        
        if Globals.Phase == "":
            logging.error("Phase wasn't specified in PlatformInfo.dat. Assuming DXE")
            Globals.Phase = "DXE"
        
        if Globals.Bitwidth <= 0 or Globals.Bitwidth >= 64:
            raise Exception("Bitwidth wasn't specified in PlatformInfo.dat or was invalid.")

        # Parse each file, keeping pages and "Memory Ranges" separate
        # Memory ranges are either "memory descriptions" for memory map types and TSEG
        # or "memory contents" for loaded image information or IDT/GDT
        for info in InfoFileList:
            self.MemoryRangeInfo.extend(ParseInfoFile(info))

        for page1g in Page1gbFileList:
            self.PageDirectoryInfo.extend(Parse1gPages(page1g))

        for page2m in Page2mbFileList:
            self.PageDirectoryInfo.extend(Parse2mPages(page2m))

        for page4k in Page4kbFileList:
            self.PageDirectoryInfo.extend(Parse4kPages(page4k))

        for guardpage in GuardPageFileList:
            self.PageDirectoryInfo.extend(ParseInfoFile(guardpage))

        for mat in MatFileList:
            self.MemoryAttributesTable.extend(ParseInfoFile(mat))

        if len(self.PageDirectoryInfo) == 0:
            self.ErrorMsg.append("No Memory Range info found in page files")
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
                    logging.error("Page overlap index %d and %d.  Base Address = 0x%x", index, index+1, self.PageDirectoryInfo[index].PhysicalStart)
                index += 1

        if len(self.MemoryRangeInfo) == 0:
            self.ErrorMsg.append("No Memory Range info found in Info files")

        # Matching memory ranges up to page table entries
        # use index based iteration so that page splitting
        # is supported.
        index = 0
        while index < len(self.PageDirectoryInfo):
            page = self.PageDirectoryInfo[index]
            for mr in self.MemoryRangeInfo:
                if page.overlap(mr):
                    if (mr.MemoryType is not None) or (mr.GcdType is not None) or (mr.ImageName is not None) or (mr.SystemMemoryType is not None):
                        if (page.PhysicalStart < mr.PhysicalStart):
                            next = page.split(mr.PhysicalStart-1)
                            self.PageDirectoryInfo.insert(index+1, next)
                            # decrement the index so that we process this partial page again
                            # because we are breaking from the MemoryRange Loop
                            index -= 1
                            break

                        if (page.PhysicalEnd > mr.PhysicalEnd):
                            next = page.split(mr.PhysicalEnd)
                            self.PageDirectoryInfo.insert(index +1, next)

                        if mr.MemoryType is not None:
                            if page.MemoryType is None or page.MemoryType == mr.MemoryType:
                                page.MemoryType = mr.MemoryType
                            else:
                                logging.error("Multiple memory types found for one region " + page.pageDebugStr() +" " + mr.MemoryRangeToString())
                                self.ErrorMsg.append("Multiple memory types found for one region.  Base: 0x%X.  EFI Memory Type: %d and %d"% (page.PhysicalStart, page.MemoryType,mr.MemoryType))

                        if mr.GcdType is not None:
                            if page.GcdType is None or page.GcdType == mr.GcdType:
                                page.GcdType = mr.GcdType
                            else:
                                logging.error("Multiple memory types found for one region " + page.pageDebugStr() +" " + mr.MemoryRangeToString())
                                self.ErrorMsg.append("Multiple memory types found for one region.  Base: 0x%X.  GCD Memory Type: %d and %d"% (page.PhysicalStart, page.GcdType,mr.GcdType))

                        if mr.ImageName is not None:
                            if page.ImageName is None:
                                page.ImageName = mr.ImageName
                            else:
                                self.ErrorMsg.append("Multiple memory contents found for one region.  Base: 0x%X.  Memory Contents: %s and %s" % (page.PhysicalStart, page.ImageName, mr.ImageName ))
                                logging.error("Multiple memory contents found for one region " + page.pageDebugStr() + " " +  mr.LoadedImageEntryToString())

                        if mr.SystemMemoryType is not None:
                            if page.SystemMemoryType is None:
                                page.SystemMemoryType = mr.SystemMemoryType
                            else:
                                self.ErrorMsg.append("Multiple System Memory types found for one region.  Base: 0x%X.  System Memory Type: %s and %s."% (page.PhysicalStart,page.GetSystemMemoryType(), mr.GetSystemMemoryType()))
                                logging.error("Multiple system memory types found for one region " + page.pageDebugStr() + " " +  mr.pageDebugStr())
                    
                    # A CPU Number present in a memory range object implies it represents an AP stack. Capture the CPU
                    # number for printing a CPU identifier for each AP stack.
                    if mr.CpuNumber is not None:
                        if page.CpuNumber is None:
                            page.CpuNumber = mr.CpuNumber
                        else:
                            self.ErrorMsg.append("Multiple Cpu Numbers found for one region.  Base: 0x%X.  Cpu Numbers: %s and %s."% (page.PhysicalStart,page.CpuNumber, mr.CpuNumber))
                            logging.error("Multiple Cpu Numbers found for one region " + page.pageDebugStr() + " " +  mr.pageDebugStr())

            for MatEntry in self.MemoryAttributesTable:
                if page.overlap(MatEntry):
                    page.Attribute = MatEntry.Attribute
            index += 1

        # Combining adjacent pages that have the same attributes.
        index = 0
        while index < (len(self.PageDirectoryInfo) - 1):
            currentPage = self.PageDirectoryInfo[index]
            nextPage = self.PageDirectoryInfo[index + 1]
            # Skip guard pages
            if currentPage.SystemMemoryType == SystemMemoryTypes.GuardPage or nextPage.SystemMemoryType == SystemMemoryTypes.GuardPage:
                index += 1
                continue
            if currentPage.sameAttributes(nextPage):
                currentPage.grow(nextPage)
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
        page_infos = []
        for page in self.PageDirectoryInfo:
            info_dict = page.toDictionary()
            # Check for errors.
            if info_dict['Section Type'] == "ERROR":
                self.AddErrorMsg("Page Descriptor at %s has an error parsing the Section Type." % info_dict['Start'])
            page_infos.append(info_dict)
        json_dict['MemoryRanges'] = page_infos

        # Finally, add any errors and produce the JSON string.
        json_dict['errors'] = self.ErrorMsg
        js = json.dumps(json_dict)

        #
        # Open template and replace placeholder with json
        #
        f = open(OutputFilePath, "w")
        if Globals.Phase == 'DXE':
            if Globals.ParsingArchitecture == "X64":
                template = open(os.path.join(sp, "DxePaging_template_X64.html"), "r")
            elif Globals.ParsingArchitecture == "AARCH64":
                template = open(os.path.join(sp, "DxePaging_template_AArch64.html"), "r")
        else:
            template = open(os.path.join(sp, "SmmPaging_template.html"), "r")

        for line in template.readlines():
            if "% TO_BE_FILLED_IN_BY_PYTHON_SCRIPT %" in line:
                line = line.replace("% TO_BE_FILLED_IN_BY_PYTHON_SCRIPT %", js)
            if "% IS_MEMORY_ATTRIBUTE_PROTOCOL_PRESENT %" in line:
                if Globals.MemoryAttributeProtocolPresent != "":
                    line = line.replace("% IS_MEMORY_ATTRIBUTE_PROTOCOL_PRESENT %", "\"" + Globals.MemoryAttributeProtocolPresent + "\"")
                else:
                    line = line.replace("% IS_MEMORY_ATTRIBUTE_PROTOCOL_PRESENT %", "\"FALSE\"")
            f.write(line)
        template.close()
        f.close()
        return 0

#
# Parse and Validate Args.  Then run the tool
#
def main():

    parser = argparse.ArgumentParser(description='Parse Paging information and generate HTML report')
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

    spt = ParsingTool(options.InputFolder, options.PlatformName, options.PlatformVersion)
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
