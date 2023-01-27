## @file -- DxeMainDependencyLoggingParser.py
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
import json
import argparse
from datetime import datetime

# PIP Modules
from edk2toollib.uefi.edk2.parsers.dec_parser import DecParser

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

# Names of output files provided in the output directory
RESULTS_FILE_NAME        = "DxeMainDependency - Parsing Results.txt"
PARSING_JSON_FILE_NAME   = "DxeMainDependency - Parsed Data.json"
DEC_GUID_JSON_FILE_NAME  = "DxeMainDependency - GUID Names.json"

##
# Class to stream evaluation messages to a file and the console
# 
class EvaluationReporting:

  ##
  # Initializes the class
  # 
  # @param   ResultsFile - Name/path of file to log all result data
  # 
  def __init__ (self, ResultsFile):
    self.WarningCount = 0

    # Open results file
    print("Saving evaluation data to results file:")
    print("    {}\n".format(ResultsFile))

    # Write header information
    self.FileHandle = open(ResultsFile, 'w', encoding='utf-8')
    self.FileHandle.write("DxeMain Dependency Logging Parser - Evaluation Data\n")
    self.FileHandle.write(datetime.now().strftime("%b %d, %Y - %I:%M:%S %p\n\n"))
  
  ##
  # Report an evaluation warning
  # 
  # @param  Messages -       Single string or list of strings indicating the warning
  #                          Written to both the file and console
  # @param  DetailFileName - Name of file to examine for more details
  # @param  InfoList -       List of messages to describe why the warning is issued
  #                          Written only to the file
  #                          Each InfoList entry can be a string or list of strings
  #                          Can be None if no extra information is necessary
  # 
  def Warning(self, Messages, DetailFileName, InfoList):
    if type(Messages) == str:
      Messages = [Messages]
    if DetailFileName != None:
      Messages.append('See file "{}" for details'.format(DetailFileName))
  
    # Console Messages: First line starts with "WARNING:  " and all subsequent indent 10 spaces
    print("WARNING:  " + Messages[0])
    for m in Messages[1:]:
      print("          " + m)
    print("")
  
    # File Messages: First line starts with ">>> WARNING:  ", all subsequent indent 14 spaces
    self.FileHandle.write(">>> WARNING:  " + Messages[0] + "\n")
    for m in Messages[1:]:
      self.FileHandle.write("              " + m + "\n")
    self.FileHandle.write("\n")

    # File InfoList: Write all data to the file with no indent formatting
    if InfoList == None:
      self.WarningCount += 1
    else:
      self.WarningCount += len(InfoList)
      for Messages in InfoList:
        if type(Messages) == str:
          Messages = [Messages]
        for m in Messages:
          self.FileHandle.write(m + "\n")
        self.FileHandle.write("\n")

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
  print('')
  print('>>> ERROR:  Failed to parse the input file')
  print('            ' + Message)
  if ErrorLine != None:
    print('')
    print('          Line Read = "{}"'.format(ErrorLine))
    print('          Expected  = <name>|<depex>|<guid>.<guid>.< ... >.<guid>')
    print('                      <name>  : Name of the driver in ASCII format')
    print('                      <depex> : Dependency expression data, hex byte array using 2 chars per byte ("AABBCC...")')
    print('                      <guid>  : Protocol(s) used by this driver ("AAAAAAAA-BBBB-CCCC-DDDD-EEEEEEEEEEEE")')
  print('')
  sys.exit(2)

##
# Collects all pertinent UEFI boot data from the input file
# 
# Pertinent lines are between begin/end tags, or all of the file in a begin tag is not found.  Each line has the
# expected format of "<name>|<depex>|<guid_1>.<guid_2>.< ... >.<guid_n>" and if text is found to the left of the
# begin tag, it is assumed to be a logger tag such as a time stamp and the same number of bytes for every subsequent
# line will be removed.
#
# @param   InputFile - Name of file containing data provided by UEFI
# @param   GuidNames - Dictionary list of GUID names collected by CollectGuidNames()
# 
# @return  Json object containing all pertinent data properly verified and formatted
#          Any parsing error is printed on the console and the script exits
#          {
#            "Name": "...",                                      <<< Name of driver in ASCII text
#            "RawDepex": "AA BB CC...",                          <<< Binary dependency data
#            "DepexStack": [                                     <<< Dependency data decoded into a list of commands
#              {
#                "Command": "AA",                                <<< Hex value of dependency command
#                "CommandName": "...",                           <<< String representation of command
#                "Guid": "AAAAAAAA-BBBB-CCCC-DDDD-EEEEEEEEEEEE", <<< GUID associated with command
#                "GuidName": "..."                               <<< Name assigned to GUID from the .DEC file scan
#              },
#              ...
#            ],
#            "UsedProtocols": [                                  <<< List of protocols located by the driver
#              {
#                "Guid": "AAAAAAAA-BBBB-CCCC-DDDD-EEEEEEEEEEEE", <<< GUID used when LocateProtocol was called
#                "GuidName": "..."                               <<< Name assigned to GUID from the .DEC file scan
#              },
#              ...
#            ]
#          }
#
def ReadPertinentData(InputFile, GuidNames):

  # User messaging
  print("Reading UEFI runtime dispatch data:")
  print("    {}".format(InputFile))

  # To start with, assume no tags and record all lines
  IsPertinent = True
  BeginTagCount = 0
  EndTagCount = 0
  HeadLen = 0
  Lines = []

  # Collect pertinent lines first
  FileHandle = open(InputFile, 'r', encoding='utf-8')
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
    print("    Note: Multiple data regions found in the input file, proceeding with data from the last region")

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
    RawDepex = Parts[1].strip().upper()
    if not IsHex(RawDepex):
      PertinentDataReadError("Line depex is not a two-char per byte hex value", Line)
    i = 2
    while i < len(RawDepex):
      RawDepex = RawDepex[:i] + ' ' + RawDepex[i:]
      i += 3

    # Parts[2] is a list of used guids used by LocateProtocol(), verify format and convert to a UsedProtocols list
    UsedProtocols = []
    Parts = Parts[2].split('.')
    for Guid in Parts:
      Guid = Guid.strip().upper()
      if Guid != "":
        if not IsGuid(Guid):
          PertinentDataReadError("Guid ({}) is not properly formatted".format(Guid), Line)
        UsedProtocols.append({ "Guid": Guid,
                               "Name": NameFromGuidNameList(GuidNames, Guid) })

    # Add to dictionary list
    DictionaryList.append({ "Name":          Name,
                            "RawDepex":      RawDepex,
                            "DepexStack":    CreateDepexStack(RawDepex, GuidNames),
                            "UsedProtocols": UsedProtocols })

  # Return requested data
  print("")
  return DictionaryList

##
# Examines all UEFI .dec files found in a directory tree to create a list of dictionary entries indicating
# all GUIDs with related names.
# 
# @param   UefiTreeDirectory - Name/path of UEFI tree to scan or None if scanning is not requested.
# 
# @return  List of dictionary entries containing correlating a GUID to all names found
#          {
#            "Guid": 'AAAAAAAA-BBBB-CCCC-DDDD-EEEEEEEEEEEE',
#            "Info": [
#                      {
#                        "FileName": "<file name>",
#                        "GuidName": "<guid name>"
#                      },
#                      ...
#                    ]
#          }
#
def CollectGuidNames(UefiTreeDirectory):
  DataList = []

  # Return an empty list of entries if the input file name is None
  if UefiTreeDirectory == None:
    print("Bypassing UEFI tree scan\n")
    return []

  # Walk the UEFI tree building the JsonObject first
  print("Scanning UEFI tree:")
  print("    {}\n".format(UefiTreeDirectory))
  for (Root, DirList, FileList) in os.walk(UefiTreeDirectory):

    # If this is the root of the UEFI tree, remove the 'build' folder and any folder starting with '.' such as '.vs' or '.git' from the directory list
    if Root == UefiTreeDirectory:
      NewList = []
      for Item in DirList:
        if Item.lower() == 'build':
          continue
        if Item[0] == '.':
          continue
        NewList.append(Item)
      DirList = NewList

    # Process only the .DEC files in this directory
    for FileName in FileList:
      if os.path.splitext(FileName)[1].lower() != ".dec":
        continue

      # Use the edk2toollib PIP module to collect all protocol, guid, and PPI values
      dec = DecParser()
      dec.ParseFile(os.path.join(Root, FileName))
      for e in (dec.Protocols + dec.PPIs + dec.Guids):
        Guid = str(e.guid).upper()
        GuidName = e.name

        # DataList is sorted, find the index of where the new entry should reside
        Begin = 0
        End = len(DataList)
        while Begin != End:
          Idx = int((Begin + End) / 2)
    
          # If the GUID is already logged, update this entry's info list and return
          if Guid == DataList[Idx]['Guid']:
    
            # If GuidName is already present, no updates are necessary
            InfoList = DataList[Idx]['Info']
            for Entry in InfoList:
              if Entry['GuidName'] == GuidName:
                break
        
            # Not present, so append a new Info entry to the DataList entry
            InfoList.append({ "FileName": FileName, "GuidName": GuidName })
            DataList[Idx]['Info'] = InfoList
            break
    
          # Keep searching
          if Guid < DataList[Idx]['Guid']:
            End = Idx
          else:
            Begin = Idx + 1
    
        # If Begin == End, GUID was not yet logged and both point to the index where the new entry should reside
        if Begin == End:
          DataList.insert(End, { "Guid":Guid, "Info": [ { "FileName": FileName, "GuidName": GuidName } ] } )

  # Return requested data
  return DataList

##
# Finds the requested GUID in the list provided by CollectGuidNames() and returns the names associated.  If not
# found, "< unknown >" is returned.
# 
# @param   GuidNames - Dictionary list of GUID names collected by CollectGuidNames()
# @param   Guid - Specific GUID to search for
# 
# @return  Name string
# 
def NameFromGuidNameList(GuidNames, Guid):
  for Entry in GuidNames:
    if Entry['Guid'] == Guid:
      Names = []
      for Info in Entry['Info']:
        Names.append(Info['GuidName'])
      return " | ".join(Names)
  return "< unknown >"

##
# Converts the input dependency data to a stack of dictionary entries.  Any error in decoding will result in
# the object's 'Name' string starting with "ERROR" followed by why the Command or Guid value are not allowed.
#
# @param   RawDepex - String containing the JSON object's RawDepex data
# @param   GuidNames - Dictionary list of GUID names collected by CollectGuidNames()
#
# @return  The JSON object's DepexStack entry
# 
def CreateDepexStack(RawDepex, GuidNames):
  Stack = []

  # Convert the depex to a list of byte values and walk the list
  DepexList = RawDepex.split(' ')
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
        GuidName = NameFromGuidNameList(GuidNames, Guid)

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
# Evaluate if the input GUID was NOT used in a LocateProtocol call
#
# @param   UsedProtocols - List of protocols used by a specific driver
# @param   Guid - Guid to evaluate
# 
# @return  True/False if the GUID was used in a LocateProtocol call
#
def GuidNotUsedInLocateProtocol(UsedProtocols, Guid):

  # If GUID is empty or not assigned, return False
  if (Guid == None) or (Guid == ""):
    return False

  # Search the used protocols list
  for Entry in UsedProtocols:
    if Entry['Guid'] == Guid:
      return False

  # Guid is valid and was not used
  return True

##
# Evaluate the dependency stack to determine if the GUID not being present would block the driver from loading
#
# @param   Stack - Stack from a specific driver's output JSON data
# @param   Guid - Guid to evaluate
# 
# @return  True/False if the GUID would NOT block the driver from loading
#
def GuidNotCoveredByDepex(Stack, Guid):

  # Find where in the stack this GUID resides
  Idx = 0
  while Idx < len(Stack):
    if Guid == Stack[Idx]['Guid']:
      break
    Idx += 1

  # If the GUID is not in the stack, it is not covered
  if Idx == len(Stack):
    return True

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
        return True

    Idx += 1

  # Exit the loop means it was decoded properly
  return False

##
# Main() Code
# 

# Print header and parse command line
print("\nUEFI Runtime Dispatch Data Parser\n")
Parser = argparse.ArgumentParser(description = 'Script to collect and report UEFI runtime dependency expression usage')
Parser.add_argument("--Input", required = True, action = 'store', metavar="<file>", help = 'Name of input file to be scanned for dispatch data.  Can be a UEFI boot log or a file containing a dump of the data from variable services.')
Parser.add_argument("--Output", required = False, action = 'store', metavar="<dir>", help = 'Path of a directory to receive all output files. (optional)')
Parser.add_argument("--UefiTree", required = False, action = 'store', metavar="<dir>", help = 'Path to a directory containing UEFI code to scan and collect GUID names from DEC files (optional)')
CmdLine = Parser.parse_args()

# Convert input paths to absolute paths
CmdLine.Input = os.path.abspath(CmdLine.Input)
if CmdLine.Output == None:
  CmdLine.Output = os.getcwd()
else:
  CmdLine.Output = os.path.abspath(CmdLine.Output)
  os.makedirs(CmdLine.Output, exist_ok = True)
if CmdLine.UefiTree != None:
  CmdLine.UefiTree = os.path.abspath(CmdLine.UefiTree)

# Collect all GUID names from the input UEFI tree
GuidNames = CollectGuidNames(CmdLine.UefiTree)

# Save GuidNames list to a file as a JSON object
OutFile = os.path.join(CmdLine.Output, DEC_GUID_JSON_FILE_NAME)
print("Saving UEFI tree GUID data:")
print("    {}\n".format(OutFile))
JsonFile = open(OutFile, 'w', encoding='utf-8')
JsonFile.write(json.dumps(GuidNames, indent = 2))
JsonFile.close()

# Pull the pertinent data from the input file
DispatchData = ReadPertinentData(CmdLine.Input, GuidNames)

# Save the pertinent DispatchData list to a file as a JSON object
OutFile = os.path.join(CmdLine.Output, PARSING_JSON_FILE_NAME)
print("Recording dispatch data in JSON format:")
print("    {}".format(OutFile))
print("")
JsonFile = open(OutFile, 'w', encoding='utf-8')
JsonFile.write(json.dumps(DispatchData, indent = 2))
JsonFile.close()

# Open the evaluation reporting class
Report = EvaluationReporting(os.path.join(CmdLine.Output, RESULTS_FILE_NAME))

# Remove DispatchData entries that have a bad depex
Idx = 0
while Idx < len(DispatchData):
  if len(DispatchData[Idx]['DepexStack']) > 0:
    LastStackEntry = DispatchData[Idx]['DepexStack'][-1]
    if (LastStackEntry['CommandName'][:5] == "ERROR") or (LastStackEntry['GuidName'][:5] == "ERROR"):
      DispatchData.pop(Idx)
      Report.Warning("Driver '{}' contained an invalid dependency expression".format(DispatchData[Idx]['Name']),
                     PARSING_JSON_FILE_NAME,
                     None)
      continue
  Idx += 1

# Check for multiple names for a specific GUID value
WarnList = []
for Entry in GuidNames:
  if len(Entry['Info']) > 1:
    Lines = [
      "    Guid: {}".format(Entry['Guid']),
      "    FileName / GuidName:"
    ]
    for Info in Entry['Info']:
      Lines.append("        {} / {}".format(Info['FileName'], Info['GuidName']))
    WarnList.append(Lines)
if len(WarnList) > 0:
  Report.Warning("Found {} GUID(s) in the UEFI tree that had multiple names assigned".format(len(WarnList)),
                 DEC_GUID_JSON_FILE_NAME,
                 WarnList)

# Check for drivers that used protocols not defined in the depex
WarnList = []
for Entry in DispatchData:
  Lines = []
  for ProtocolInfo in Entry["UsedProtocols"]:
    if GuidNotCoveredByDepex(Entry['DepexStack'], ProtocolInfo['Guid']):
      Lines.append("        {} ( {} )".format(ProtocolInfo['Guid'], ProtocolInfo['Name']))
  if len(Lines) > 0:
    WarnList.append([
                      "    FileName: {}".format(Entry['Name']),
                      "    Protocols used in a LocateProtocol() call, but not listed in Depex:"
                    ] + Lines)
if len(WarnList) > 0:
  Report.Warning([
                   "Found {} driver(s) that used a protocol GUID not covered by its dependency expression".format(len(WarnList)),
                   "This may not indicate a true error due to using other protections such as a callback when installed"
                 ],
                 PARSING_JSON_FILE_NAME,
                 WarnList)

# Examine all drivers guids in the depex that were not located and used
WarnList = []
for Entry in DispatchData:
  Lines = []
  for StackEntry in Entry['DepexStack']:
    if GuidNotUsedInLocateProtocol(Entry['UsedProtocols'], StackEntry['Guid']):
      Lines.append("        {}  ( {} )".format(StackEntry['Guid'], StackEntry['GuidName']))
  if len(Lines) > 0:
    WarnList.append([
                      "    FileName: {}".format(Entry['Name']),
                      "    Protocols listed in the depex but not used in a LocateProtocol() call:"
                    ] + Lines)
if len(WarnList) > 0:
  Report.Warning([
                   "Found {} driver(s) that declared a GUID in the depex without using it in a LocateProtocol call".format(len(WarnList)),
                   "This may not indicate a true error due to reasons such as load ordering without a need to use the protocol"
                 ],
                 PARSING_JSON_FILE_NAME,
                 WarnList)

# Exit with a return code of 1 if any warnings found
print(Report.WarningCount, "Warnings found\n")
if Report.WarningCount > 0:
  sys.exit(1)
sys.exit(0)
