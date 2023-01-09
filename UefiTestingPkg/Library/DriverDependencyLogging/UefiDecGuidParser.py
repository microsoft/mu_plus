## @file -- UefiDecGuidParser.py
#
# This library and toolset are used with the Core DXE dispatcher to log all DXE drivers' protocol usage and
# dependency expression implementation during boot.
#
# See the readme.md file for full information
#
# Copyright (C) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
import sys
import os
import string
import argparse
from datetime import datetime
from threading import Thread
import time
import json


##
# Class to scan a directory tree for UEFI .DEC files and extract all GUID definitions
# 
class DecGuidParser:

  ##
  # Performs the entire scan and produces the class variables with parsed data
  # 
  # @param    TreeDirectory - Path to the tree to scan for .DEC files
  # @param    SkipListFile - Path to the list of directories to skip in the scan process
  #
  # @class variables:
  #   self.DirectoriesParsed - Number of directories found in the scan process
  #   self.FilesParsed - Number of files found in the scan process
  #   self.WarningList - List of dictionary entries showing the lines that caused an error in parsing
  #                      {
  #                        "FileName": " < Name of file, path relative to UEFI tree > ",
  #                        "Line": " < Free form line string read from DEC file > "
  #                      }
  #   self.DataList    - List of dictionary entries containing all data parsed from DEC files
  #                      {
  #                        "Guid": 'AAAAAAAA-BBBB-CCCC-DDDD-EEEEEEEEEEEE',
  #                        "Info": [
  #                                  {
  #                                    "FileName": " < Name of file, path relative to UEFI tree > ",
  #                                    "GuidName": " < Free form string name from DEC file > "
  #                                  },
  #                                  ...
  #                                ]
  #                      }
  # 
  def __init__(self, TreeDirectory, SkipListFile = None):
    self.DirectoriesParsed = 0
    self.FilesParsed = 0
    self.WarningList = []
    self.DataList = []
    self.WorkingDirectory = os.path.abspath(TreeDirectory)
    SkipList = []

    # If skip list file is provided, read data
    if SkipListFile != None:
      FileHandle = open(os.path.abspath(SkipListFile), 'r', encoding='utf-8')

      # Record all lines removing empty and comments
      Line = FileHandle.readline()
      while Line != "":
        Line = Line.split('#')[0].strip()
        if Line != "":
          SkipList.append(Line.lower())
        Line = FileHandle.readline()

      # Close file
      FileHandle.close()

    # Walk the tree provided
    self.pvt_ParseFolder("", SkipList)

  ## PRIVATE
  # Walks the input directory calling this function recursively on child directories or pvt_ParseFile on DEC files
  #
  # @param   RelativePath - Relative path from self.WorkingDirectory to scan
  # @param   SkipList - List of relative directory paths to skip
  # 
  def pvt_ParseFolder(self, RelativePath, SkipList):
  
    # Check if this folder should be skipped
    TestPath = RelativePath.lower()
    if SkipList == []:
      if TestPath == "build":
        return
      if TestPath[0:1] == ".":
        return
    elif TestPath in SkipList:
      return

    # Loop through all items in this folder
    for Item in os.listdir(os.path.join(self.WorkingDirectory, RelativePath)):
      ItemRelativePath = os.path.join(RelativePath, Item)
      ItemAbsolutePath = os.path.join(self.WorkingDirectory, ItemRelativePath)

      # If a directory, call recursively
      if os.path.isdir(ItemAbsolutePath):
        self.pvt_ParseFolder(ItemRelativePath, SkipList)
  
      # If a .DEC file, parse the data
      elif Item[-4:].lower() == ".dec":
        self.pvt_ParseFile(ItemRelativePath)

    # Update count
    self.DirectoriesParsed += 1

  ## PRIVATE
  # Opens the input file, searches for lines in a [Protocols] or [Guids] section, and calls pvt_ParseFileLine
  #
  # @param   RelativePath - Relative path to self.WorkingDirectory of the file to parse
  # 
  def pvt_ParseFile(self, RelativePath):
    InPertinentSection = False
  
    # Open file and walk through all lines
    FileHandle = open(os.path.join(self.WorkingDirectory, RelativePath), 'r', encoding = 'utf-8')
    RawLine = FileHandle.readline()
    while RawLine != "":
  
      # Remove comments and trim empty space
      Line = RawLine.split('#')[0].strip()
  
      # Check if line is in a [guids] or [protocols] section
      SectionTest = Line[0:10].lower()
      if SectionTest == "[protocols":
        InPertinentSection = True
      elif SectionTest[:6] == "[guids":
        InPertinentSection = True
      elif SectionTest[:1] == "[":
        InPertinentSection = False
      elif InPertinentSection:
        self.pvt_ParseFileLine(RelativePath, Line)
  
      # Next line and loop
      RawLine = FileHandle.readline()
  
    # Close file and increment parse count
    FileHandle.close()
    self.FilesParsed += 1

  ## PRIVATE
  # If the input line is a GUID assignment line, adds the GUID information to self.DataList
  #
  # @param   RelativePath - Path to the file being parsed
  # @param   Line - String line to parse
  #
  def pvt_ParseFileLine(self, RelativePath, Line):
  
    # Setup the new values, returning quietly if this is not a GUID assignment line
    Parts = Line.split('=')
    if len(Parts) != 2:
      return
    NewFile = RelativePath
    NewName = Parts[0].strip()
    NewGuid = self.pvt_DecGuidToUefiGuid(Parts[1].strip())
    if NewGuid == None:
      self.WarningList.append({ "FileName": RelativePath, "Line": Line })
      return

    # JSON entry list is sorted, find where to insert/update data
    TestGuid = "~ not found ~"
    EntryIdx = 0
    while EntryIdx < len(self.DataList):
      TestGuid = self.DataList[EntryIdx]['Guid']
      if (NewGuid == TestGuid) or (NewGuid < TestGuid):
        break
      EntryIdx += 1

    # If the new GUIDs are not equal, the new info list is a single entry list
    if NewGuid != TestGuid:
      NewInfoList = [{ "FileName": NewFile, "GuidName": NewName }]

    # If the GUIDs are equal, the new info list should include both the old and new entries
    else:
      NewInfoList = self.DataList[EntryIdx]['Info']

      # The Info list is sorted, find where to add the new entry
      for InfoIdx in range(0, len(NewInfoList)):
        if NewName < NewInfoList[InfoIdx]['GuidName']:
          break

        # If the GUID name is already present, no update is necessary, return quietly
        if NewName == NewInfoList[InfoIdx]['GuidName']:
          return

      # Update the new info list with the new data
      NewInfoList.insert(InfoIdx, { "FileName": NewFile, "GuidName": NewName })

      # Remove the old JSON entry and replace with the new entry
      self.DataList.pop(EntryIdx)

    # Add the new JSON entry to the list at the proper index
    self.DataList.insert(EntryIdx, { "Guid": NewGuid, "Info": NewInfoList })

  ## PRIVATE
  # Converts a DEC guid format string to a UEFI guid format string
  #
  # @param   DecGuidStr - Input format expected:  '{ 0xaaaaaaaa, 0xbbbb, 0xcccc, { 0xdd, 0xdd, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee }}'
  # 
  # @return  Output format: 'AAAAAAAA-BBBB-CCCC-DDDD-EEEEEEEEEEEE' or None on error
  #
  def pvt_DecGuidToUefiGuid(self, DecGuidStr):

    # Remove space and brackets
    DecGuidStr = DecGuidStr.replace(" ", "")
    DecGuidStr = DecGuidStr.replace("{", "")
    DecGuidStr = DecGuidStr.replace("}", "")

    # Split on comma and force each part to upper hex with expected number of chars
    DecGuidParts = DecGuidStr.split(',')
    UefiGuidParts = []
    try:
      UefiGuidParts.append("{:08X}".format(int(DecGuidParts[0], 16)))
      UefiGuidParts.append("{:04X}".format(int(DecGuidParts[1], 16)))
      UefiGuidParts.append("{:04X}".format(int(DecGuidParts[2], 16)))
      UefiGuidParts.append("{:02X}{:02X}".format(int(DecGuidParts[3], 16),
                                                 int(DecGuidParts[4], 16)))
      UefiGuidParts.append("{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}".format(int(DecGuidParts[5], 16),
                                                                         int(DecGuidParts[6], 16),
                                                                         int(DecGuidParts[7], 16),
                                                                         int(DecGuidParts[8], 16),
                                                                         int(DecGuidParts[9], 16),
                                                                         int(DecGuidParts[10], 16)))

    # Catch any exception trying to access a list part not present or conversion of non hex values
    except:
      return None

    # Return the formatted string
    return '-'.join(UefiGuidParts)

##
# Helper class for DecGuidParser.  The scan process can take 10's of seconds so this class can wrap the parser
# init to initiate a new thread to print a dot without line return every 750mS to show progress.
# 
# __init__() - Starts self.Run() as a new thread
# Run()      - Prints '.' every 0.75 seconds without a new line char
# Halt()     - Halts the dot printing thread
# 
class InProgress:

  ##
  # Starts self.Run() as a new thread
  # 
  def __init__(self):
    self.AllowRun = True
    self.ThreadHandle = Thread(target = self.pvt_Run)
    self.ThreadHandle.start() 

  ## PRIVATE
  # Prints '.' every 0.75 seconds without a new line char
  # 
  def pvt_Run(self):
    while self.AllowRun:
      print('.', end = "", flush = True)
      time.sleep(0.75)

  ##
  # Halts the dot printing thread
  # 
  def Halt(self):
    self.AllowRun = False
    self.ThreadHandle.join()


##
# The following items support running this file as a stand-alone script
# 

##
# Streams a message to a log file (if requested) and the console
#
# @param  Message - String to status to user
# @param  LogFileHandle - If not None, writes the message to the file
# @param  LogFileOnly - Only send this message to the log file, not the console.  If the log file handle is None, this param is ignored.
#
def WriteMessage(Message, LogFileHandle, LogFileOnly = False):
  if (not LogFileOnly) or (LogFileHandle == None):
    print (Message)
  if LogFileHandle != None:
    LogFileHandle.write(Message + "\n")
    LogFileHandle.flush()

##
# Main function to run if executed as a script
# 
def main():

  # Print header and parse command line
  print("\nUEFI Code Tree GUID Definition Parser\n")
  Parser = argparse.ArgumentParser(description = 'Script to walk a UEFI code tree, scan all .DEC files, and report on the GUID usage')
  Parser.add_argument("--Input", required = True, action = 'store', metavar="<dir>", help = 'Name/path of UEFI code tree directory')
  Parser.add_argument("--Output", required = False, action = 'store', metavar="<file>", help = 'Name/path of JSON file to receive all parsing data (optional)')
  Parser.add_argument("--LogFile", required = False, action = 'store', metavar="<file>", help = 'Name/path of log file to receive results (optional)')
  Parser.add_argument("--SkipListFile", required = False, action = 'store', metavar="<file>", help = 'Name/path of file containing folder paths relative to root to skip.  If unused, "/build" and "/.*" are assumed.')
  CmdLine = Parser.parse_args()

  # Setup the log file if requested
  if CmdLine.LogFile != None:
    Name = os.path.abspath(CmdLine.LogFile)
    os.makedirs(os.path.split(Name)[0], exist_ok = True)
    LogFileHandle = open(Name, 'w', encoding='utf-8')
  else:
    LogFileHandle = None

  # Print input parameters
  WriteMessage(datetime.now().strftime("Evaluation Data\n%b %d, %Y - %I:%M:%S %p\n"), LogFileHandle)
  WriteMessage("Input Directory:  " + str(CmdLine.Input), LogFileHandle)
  WriteMessage("Output JSON File: " + str(CmdLine.Output), LogFileHandle)
  WriteMessage("Log File:         " + str(CmdLine.LogFile), LogFileHandle)
  WriteMessage("Skip List File:   " + str(CmdLine.SkipListFile) + "\n", LogFileHandle)

  # Verify input is a directory
  CmdLine.Input = os.path.abspath(CmdLine.Input)
  if not os.path.isdir(CmdLine.Input):
    WriteMessage("ERROR:  Input directory not found", LogFileHandle)
    sys.exit()

  # Loop through all folders creating a list of { GUID, GUID Name, File Name } dictionaries
  print("Scanning for DEC files", end = "")
  Progress = InProgress()
  try:
    Parser = DecGuidParser(CmdLine.Input, CmdLine.SkipListFile)
    Progress.Halt()
  except Exception as e:
    Progress.Halt()
    raise e
  print("\n")

  # If out file is requested, write the new JSON file
  if CmdLine.Output != None:
    JsonFile = open(CmdLine.Output, 'w', encoding = 'utf-8')
    JsonFile.write(json.dumps(Parser.DataList, indent = 2))
    JsonFile.close()
  
  # Check for duplicate GUID usage
  GuidsWithDups = 0
  for e in Parser.DataList:
    if len(e['Info']) > 1:
      GuidsWithDups += 1
      Msg = ">>  WARNING: DEC file GUID ({}) used with multiple names\n".format(e['Guid'])
      for i in e['Info']:
        Msg += "      {} - {}\n".format(i['GuidName'], i['FileName'])
      WriteMessage(Msg, LogFileHandle, LogFileOnly = True)

  # Check for warnings found during the scan
  for w in Parser.WarningList:
      WriteMessage(">>  WARNING: DEC file line could not be parsed", LogFileHandle)
      WriteMessage("      File: " + w['FileName'], LogFileHandle)
      WriteMessage("      Line: " + w['Line'], LogFileHandle)
      WriteMessage("", LogFileHandle)
  
  # Status and results
  WriteMessage("Results:", LogFileHandle)
  WriteMessage("  Directories searched: {}".format(Parser.DirectoriesParsed), LogFileHandle)
  WriteMessage("  DEC files searched:   {}".format(Parser.FilesParsed), LogFileHandle)
  WriteMessage("  DEC GUIDs found:      {}\n".format(len(Parser.DataList)), LogFileHandle)
  if len(Parser.WarningList) > 0:
    WriteMessage("## WARNING: Found {} invalid lines in DEC files\n".format(gWarningsFound), LogFileHandle)
  if GuidsWithDups > 1:
    WriteMessage("## WARNING: Found {} GUIDs in UEFI DEC files that had multiple names\n".format(GuidsWithDups), LogFileHandle)

# If run as an individual file, execute the main function
if __name__ == "__main__":
  main()

