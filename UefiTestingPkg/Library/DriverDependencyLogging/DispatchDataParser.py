## @file -- DispatchDataParser.py
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
import shutil
import json
import string
import logging
import argparse
from datetime import datetime
from UefiDecGuidParser import DecGuidParser
from UefiDecGuidParser import InProgress

# Correlates Depex command codes to their respective hex values
DEPEX_CMD_ENCODING = { "00": "BEFORE",
                       "01": "AFTER",
                       "02": "PUSH",
                       "03": "AND",
                       "04": "OR",
                       "05": "NOT",
                       "06": "TRUE",
                       "07": "FALSE",
                       "08": "END",
                       "09": "SOR",
                       "FF": "REPLACE_TRUE" }

# List of commands in the depex that will have an associated GUID
DEPEX_CMDS_WITH_GUIDS = [ "BEFORE", "AFTER", "PUSH", "REPLACE_TRUE" ]

# Tags to search for in the input data file
DEPEX_LOG_BEGIN = "DEPEX_LOG_v1_BEGIN"
DEPEX_LOG_END   = "DEPEX_LOG_v1_END"


##
# Class to stream messages to the log file and to the console
# 
class DispatchDataLogger:

  ##
  # Initializes the class
  # 
  # @param  FileName - Name of log file or None if a log file is not required
  # 
  def __init__ (self, FileName):
    self.FileHandle = None
    if FileName != None:
      Name = os.path.abspath(FileName)
      os.makedirs(os.path.split(Name)[0], exist_ok = True)
      self.FileHandle = open(Name, 'w', encoding='utf-8')

  ##
  # Streams messages to the log file and console
  # 
  # @param  Message - String message to display.  If FileName was None at init, only prints to the console
  # @param  LogFileOnly - If True, only prints to the file.  If FileName was None at init, the flag is ignored.
  # 
  def Write(self, Message, LogFileOnly = False):
    if (not LogFileOnly) or (self.FileHandle == None):
      print (Message)
    if self.FileHandle != None:
      self.FileHandle.write(Message + "\n")
      self.FileHandle.flush()

##
# Determine if any length string contains only hex values
# 
# @param   Str - Input string
# 
# @return  True if string len is a multiple of 2 and all chars are hex
# 
def IsHex(Str):
  if (len(Str) % 2) != 0:
    return False
  for c in Str:
    if (c not in "0123456789ABCDEFabcdef"):
      return False
  return True

##
# Determine if the input string is a GUID in UEFI format
# 
# @param   Str - Input string
# 
# @return  True if the string is hex and a UEFI format 'aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee'
# 
def IsGuid(Str):
  GuidPartSizes = [ 8, 4, 4, 4, 12 ]
  GuidParts = Str.split('-')

  if len(GuidParts) != 5:
    return False
  for i in range(0, 5):
    if not IsHex(GuidParts[i]):
      return False
    if len(GuidParts[i]) != GuidPartSizes[i]:
      return False
  return True

##
# Error helper for ReadPertinentData to format detailed messages and exit the script
# 
# @param  Message     - Pertinent message string to provide to user
# @param  ErrorLine   - The line string causing the error or None if the error is not based on a mal-formed line
#
def PertinentDataReadError(Message, ErrorLine = None):
  Log.Write(">> ERROR: Failed to parse the input file")
  Log.Write("          " + Message)
  if ErrorLine != None:
    Log.Write("")
    Log.Write('          Line Read = "{}"'.format(ErrorLine))
    Log.Write("          Expected  = <name>|<depex>|<guid>.<guid>.< ... >.<guid>")
    Log.Write("                      <name>  : Name of the driver in ASCII format")
    Log.Write("                      <depex> : Dependency expression data, hex byte array using 2 chars per byte ('AABBCC...')")
    Log.Write("                      <guid>  : Protocol(s) used by this driver ('AAAAAAAA-BBBB-CCCC-DDDD-EEEEEEEEEEEE')")
  Log.Write("")

  sys.exit()

##
# Opens the input data file, extracts/validates pertinent lines, and returns a list of dictionaries representing
# the pertinent data.
# 
# Pertinent lines are between begin/end tags, or all of the file in a begin tag is not found.  The dictionary is
# created based on the expected line format of "<name>|<depex>|<guid_1>.<guid_2>.< ... >.<guid_n>" and if text is
# found to the left of the begin tag, it is assumed to be a logger tag such as a time stamp and the same number
# of bytes for every subsequent line will be removed.
#
# @param   InFile - Name of input file
# 
# @return  Dictionary List - [ { "Name": "...",
#                                "Depex": "AA BB CC ...",
#                                "Guids": [ "AAAAAAAA-BBBB-CCCC-DDDD-EEEEEEEEEEEE", ... ]
#                              },
#                              ...
#                            ]
# 
def ReadPertinentData(InFile):
  IsPertinent = True
  BeginTagCount = 0
  EndTagCount = 0
  HeadLen = 0
  Lines = []

  # Collect pertinent lines first
  FileHandle = open(os.path.abspath(InFile), 'r', encoding='utf-8')
  Line = FileHandle.readline()
  while Line != "":
    Line = Line.rstrip('\n')

    # If a begin tag, reset Line list, determine head length, and set pertinent flag
    if DEPEX_LOG_BEGIN in Line:
      BeginTagCount += 1
      HeadLen = Line.find(DEPEX_LOG_BEGIN)
      IsPertinent = True
      Lines = []

    # If end tag, set the pertinent flag
    elif DEPEX_LOG_END in Line:
      EndTagCount += 1
      IsPertinent = False

    # If a pertinent line, record the data
    elif IsPertinent:
      Lines.append(Line[HeadLen:])

    # Next line and loop
    Line = FileHandle.readline()

  # Loop finished
  FileHandle.close()

  # Return error if begin or end tag counts don't match
  if BeginTagCount > EndTagCount:
    PertinentDataReadError("Begin tag '{}' is missing a matching end tag '{}'".format(DEPEX_LOG_BEGIN, DEPEX_LOG_END))
  if EndTagCount > BeginTagCount:
    PertinentDataReadError("End tag '{}' is missing a matching begin tag '{}'".format(DEPEX_LOG_END, DEPEX_LOG_BEGIN))

  # Warn if multiple regions found
  if BeginTagCount > 1:
    Log.Write("Note: Found multiple data regions in the input file, proceeding with data from the last region\n")

  # Build dictionary list from line list
  DictionaryList = []
  for Line in Lines:

    # Split on '|' char
    Parts = Line.split('|')
    if len(Parts) != 3:
      PertinentDataReadError("Missing 3 sections separated by '|'", Line)

    # Parts[0] is the name
    Name = Parts[0].strip()

    # Parts[1] is the raw depex, verify hex and convert to all upper case with spaces between bytes
    Depex = Parts[1].strip().upper()
    if not IsHex(Depex):
      PertinentDataReadError("Line depex is not a two-char per byte hex value", Line)
    i = 2
    while i < len(Depex):
      Depex = Depex[:i] + ' ' + Depex[i:]
      i += 3

    # Parts[2] is a list of guids, verify guid format and convert to upper case
    Guids = []
    Parts = Parts[2].split('.')
    for Guid in Parts:
      Guid = Guid.strip().upper()
      if Guid != "":
        if not IsGuid(Guid):
          PertinentDataReadError("Guid ({}) is not properly formatted".format(Guid), Line)
        Guids.append(Guid)
  
    # Add to dictionary list
    DictionaryList.append({ "Name": Name, "Depex": Depex, "Guids": Guids })

  # Return requested data
  return DictionaryList
  
##
# Uses the DecGuidParser class to collect a list of GUIDs and assigned names
# 
# @param   TreeDirName - Name of UEFI tree directory to scan
# 
# @return  DecGuidParser::DataList
# 
def CollectGuidNames(TreeDirName):
  if TreeDirName == None:
    return []

  # Print status and start the InProgress status dots before scanning the DEC files
  print("Scanning for DEC files", end = "")
  Progress = InProgress()
  try:
    Parser = DecGuidParser(TreeDirName)
    Progress.Halt()
  except Exception as e:
    Progress.Halt()
    raise e
  print("\n")

  # Exit on any warning found during the scan
  if len(Parser.WarningList) > 0:
    Log.Write(">> ERROR: Failed to scan the UEFI tree")
    Log.Write("          DEC file line(s) could not be parsed\n")
    for w in Parser.WarningList:
      Log.Write("          File: {}".format(w['FileName']))
      Log.Write('          Line: "{}"\n'.format(w['Line']))
    sys.exit()

  return Parser.DataList

##
# Returns a string from DecGuidParser::DataList indicating the name(s) assigned to a specific GUID.  If the GUID
# is not found, "< unknown >" is returned.  If multiple names are found, all are returned separated by ' | '.
# 
# @param   GuidNameDataList - DecGuidParser::DataList
# @param   Guid - Specific GUID to search for
# 
# @return  Name string
# 
def NameFromGuidNameList(GuidNameDataList, Guid):
  for Entry in GuidNameDataList:
    if Entry['Guid'] == Guid:
      List = []
      for Info in Entry['Info']:
        List.append(Info['GuidName'])
      return " | ".join(List)
  return "< unknown >"

##
# Converts the input item's depex data to a stack of dictionary entries
#
# @param   PertinentItem - {
#                            "Name": "...",
#                            "Depex": "AA BB CC ...",
#                            "Guids": [ "AAAAAAAA-BBBB-CCCC-DDDD-EEEEEEEEEEEE", ... ]
#                          }
# @param   GuidNameDataList - DecGuidParser::DataList
#
# @return  [                                                     <- List of entries
#            { "Command": "AA",                                  <- Byte value pulled from Item['Depex']
#              "CommandName": "...",                             <- Human readable command name or "ERROR: ..." describing why the command could not be decoded
#              "Guid": "AAAAAAAA-BBBB-CCCC-DDDD-EEEEEEEEEEEE",   <- UEFI format GUID pulled from Item['Depex']
#              "GuidName": "..."                                 <- Guid name found in code tree scan or "ERROR: ..." describing why the GUID could not be decoded
#            },
#            ...
#          ]
def CreateDepexStack(PertinentItem, GuidNameDataList):
  Stack = []

  # Convert the depex to a list of byte values and walk the list
  DepexList = PertinentItem['Depex'].split(' ')
  if (len(DepexList) == 1) and (DepexList[0] == ""):
    DepexList = []
  while len(DepexList) > 0:

    # Command and Command Name
    Command = DepexList.pop(0)
    CommandName = DEPEX_CMD_ENCODING.get(Command, "ERROR: Invalid command byte")
    Guid = ""
    GuidName = ""

    # Guid and GuidName
    if CommandName in DEPEX_CMDS_WITH_GUIDS:
      GuidByteList = DepexList[:16]
      DepexList = DepexList[16:]
      Guid = "{}-{}-{}-{}-{}".format("".join(GuidByteList[3::-1]),  # Part A - UINT32  - bytes [0:3] are reverse ordered
                                     "".join(GuidByteList[5:3:-1]), # Part B - UINT16  - bytes [4:5] are reverse ordered
                                     "".join(GuidByteList[7:5:-1]), # Part C - UINT16  - bytes [6:7] are reverse ordered
                                     "".join(GuidByteList[8:10]),   # Part D - UINT8[] - bytes [8:9] are in correct order
                                     "".join(GuidByteList[10:16]))  # Part E - UINT8[] - bytes [10:15] are in correct order
      if len(GuidByteList) < 16:
        GuidName = "ERROR: Insufficient data in Depex to support a GUID"
      else:
        GuidName = NameFromGuidNameList(GuidNameDataList, Guid)

    # Add stack entry and break on error
    Stack.append({ "Command": Command,
                   "CommandName": CommandName,
                   "Guid": Guid,
                   "GuidName": GuidName })
    if (CommandName[:5] == "ERROR") or (GuidName[:5] == "ERROR"):
      break

  # Return stack data
  return Stack

##
# Evaluate the dependency stack to determine if the GUID not being present would block the driver from loading
#
# @param   Stack - Depex stack to parse
#                  [
#                    {
#                      "Command": "AA",
#                      "CommandName": "...",
#                      "Guid": "AAAAAAAA-BBBB-CCCC-DDDD-EEEEEEEEEEEE",
#                      "GuidName": "..."
#                    },
#                    ...
#                  ]
# @param   Guid - Guid to evaluate
# 
# @return  True/False if the GUID would block the driver from loading
#
def GuidIsCoveredByDepex(Stack, Guid):

  # Find where in the stack this GUID resides
  Idx = 0
  while Idx < len(Stack):
    if Guid == Stack[Idx]['Guid']:
      break
    Idx += 1

  # Return False if not in the stack
  if Idx == len(Stack):
    return False

  # Walk the rest of the commands to make sure the GUID is honored
  Idx += 1
  PushDepth = 0
  while Idx < len(Stack):

    # If this command has an associated GUID, increment the push depth
    if Stack[Idx]['Guid'] != "":
      PushDepth += 1

    # If this command does not have a GUID and depth is above 0, decrement push depth
    elif PushDepth > 0:
      PushDepth -= 1

    # If no GUID and push depth is 0, this command must be either AND or END
    else:
      if Stack[Idx]['CommandName'] != "END" and Stack[Idx]['CommandName'] != "AND":
        return False

    Idx += 1

  # If we exited the loop, it didn't decode right, return False
  return False

##
# Reports all results from this script
#
# @param   GuidNameDataList - DecGuidParser::DataList
# @param   JsonObject - JSON object built by this script
#
def PerformEvaluation(GuidNameDataList, JsonObject):
  ProtocolErrFileCount = 0
  DepexErrFileCount = 0
  GuidDupCount = 0
  InvalidDepexCount = 0
  TotalWarningCount = 0

  # Warn for all duplicated GUIDs found
  for Entry in GuidNameDataList:
    if len(Entry['Info']) > 1:
      GuidDupCount += 1
      TotalWarningCount += 1
      Msg = ">> WARNING: DEC file GUID ({}) used with multiple names\n".format(Entry['Guid'])
      for Info in Entry['Info']:
        Msg += "            {} - {}\n".format(Info['GuidName'], Info['FileName'])
      Log.Write(Msg, LogFileOnly = True)

  # Individual JSON entry evaluation
  for JsonEntry in JsonObject:

    # Warn if this driver had an invalid stack and skip to next if found
    Stack = JsonEntry['DepexStack']
    if len(Stack) > 0 and ((Stack[-1]['CommandName'][:5] == "ERROR") or (Stack[-1]['GuidName'][:5] == "ERROR")):
      Msg =  ">> WARNING: Invalid depex in file {}\n".format(JsonEntry['Name'])
      Msg += "            Raw Data: {}\n".format(JsonEntry['RawDepex'])
      Msg += "            Stack:    {}\n".format(Stack[0])
      for s in Stack[1:]:
        Msg += "                      {}\n".format(s)
      Log.Write(Msg, LogFileOnly = True)
      InvalidDepexCount += 1
      TotalWarningCount += 1
      continue

    # Check for Protocols used without declaring in the depex
    ProtocolErrMessage = ""
    for Protocol in JsonEntry["UsedProtocols"]:
      if not GuidIsCoveredByDepex(JsonEntry['DepexStack'], Protocol['Guid']):
        if Protocol['Name'][:1] == "<":
          ProtocolErrMessage += "                {}\n".format(Protocol['Guid'])
        else:
          ProtocolErrMessage += "                {}  ( {} )\n".format(Protocol['Guid'], Protocol['Name'])
    if ProtocolErrMessage != "":
      ProtocolErrMessage = "            Protocol GUIDs used without being declared in the depex\n" + ProtocolErrMessage
      ProtocolErrFileCount += 1

    # Check for GUIDs in depex without being used as a protocol
    DepexErrMessage = ""
    for DepexEntry in JsonEntry['DepexStack']:
      if DepexEntry['Guid'] == "":
        continue
      if DepexEntry['GuidName'][:5] == "ERROR":
        continue
      if DepexEntry['Guid'] in JsonEntry["UsedProtocols"]:
        continue
      if Protocol['Name'][:1] == "<":
        DepexErrMessage += "                {}\n".format(Protocol['Guid'])
      else:
        DepexErrMessage += "                {}  ( {} )\n".format(Protocol['Guid'], Protocol['Name'])
    if DepexErrMessage != "":
      DepexErrMessage = "            Protocol GUIDs declared in depex not used in driver\n" + DepexErrMessage
      DepexErrFileCount += 1

    # Message depex/protocol warnings for this driver
    FullErrMessage = ProtocolErrMessage + DepexErrMessage
    if FullErrMessage != "":
      TotalWarningCount += 1
      Log.Write(">> WARNING: {} Evaluation:".format(JsonEntry['Name']), LogFileOnly = True)
      Log.Write(FullErrMessage, LogFileOnly = True)

  # Report final evaluation status
  Log.Write("\nEvaluation finished")
  Log.Write("    {} drivers examined".format(len(JsonObject)))
  Log.Write("    {} total warnings logged\n".format(TotalWarningCount))
  if GuidDupCount > 0:
    Log.Write("    {} warnings for GUID values found in the UEFI tree DEC files with multiple names assigned".format(GuidDupCount))
  if InvalidDepexCount > 0:
    Log.Write("    {} warnings for drivers found with an invalid stack\n".format(InvalidDepexCount))
  if ProtocolErrFileCount > 0:
    Log.Write("    {} warnings for drivers using a protocol without declaring it in the depex\n".format(ProtocolErrFileCount))
  if DepexErrFileCount > 0:
    Log.Write("    {} warnings for drivers declaring a protocol in the depex but not using it\n".format(DepexErrFileCount))
  Log.Write("")


##
# Main() Code
# 

# Print header and parse command line
print("\nUEFI Runtime Dispatch Data Parser\n")
Parser = argparse.ArgumentParser(description = 'Script to collect and report UEFI runtime dependency expression usage')
Parser.add_argument("--Input", required = True, action = 'store', metavar="<file>", help = 'Name of input file to be scanned for dispatch data.  Can be a UEFI boot log or a dump of the data from variable services.')
Parser.add_argument("--Output", required = False, action = 'store', metavar="<file>", help = 'Name/path of a JSON file to receive all parsed data (optional)')
Parser.add_argument("--LogFile", required = False, action = 'store', metavar="<file>", help = 'Name/path of log file to receive results (optional)')
Parser.add_argument("--UefiTree", required = False, action = 'store', metavar="<dir>", help = 'Name/path of a UEFI tree to be scanned to retrieve GUID names from DEC files (optional)')
CmdLine = Parser.parse_args()

# Setup the log file if requested
Log = DispatchDataLogger(CmdLine.LogFile)
Log.Write("Dispatch Data Parser - Evaluation Data")
Log.Write(datetime.now().strftime("%b %d, %Y - %I:%M:%S %p\n"))

# Validate and diaplay input parameters
CmdLine.Input = os.path.abspath(CmdLine.Input)
Log.Write("Input File:   " + str(CmdLine.Input))
if CmdLine.Output != None:
  CmdLine.Output = os.path.abspath(CmdLine.Output)
  os.makedirs(os.path.split(CmdLine.Output)[0], exist_ok = True)
Log.Write("Output File:  " + str(CmdLine.Output))
if CmdLine.LogFile != None:
  CmdLine.LogFile = os.path.abspath(CmdLine.LogFile)
  os.makedirs(os.path.split(CmdLine.LogFile)[0], exist_ok = True)
Log.Write("Log File:     " + str(CmdLine.LogFile))
if CmdLine.UefiTree != None:
  CmdLine.UefiTree = os.path.abspath(CmdLine.UefiTree)
Log.Write("UEFI Tree:    " + str(CmdLine.UefiTree) + "\n")

# Pull the pertinent data from the input file
PertinentData = ReadPertinentData(CmdLine.Input)

# Collect GUID names if requested
GuidNameDataList = CollectGuidNames(CmdLine.UefiTree)

# Create the JSON object by creating an entry from each item in the Pertinent data list
JsonObject = []
for Item in PertinentData:
  UsedProtocols = []
  for g in Item['Guids']:
    UsedProtocols.append({ "Guid": g, "Name": NameFromGuidNameList(GuidNameDataList, g) })
  JsonObject.append({ "Name":          Item['Name'],
                      "RawDepex":      Item['Depex'],
                      "DepexStack":    CreateDepexStack(Item, GuidNameDataList),
                      "UsedProtocols": UsedProtocols })

# Save the new JSON object if requested
if CmdLine.Output != None:
  Name = os.path.abspath(CmdLine.Output)
  os.makedirs(os.path.split(Name)[0], exist_ok = True)
  JsonFile = open(Name, 'w', encoding='utf-8')
  JsonFile.write(json.dumps(JsonObject, indent = 2))
  JsonFile.close()

# Evaluate the data found
PerformEvaluation(GuidNameDataList, JsonObject)

