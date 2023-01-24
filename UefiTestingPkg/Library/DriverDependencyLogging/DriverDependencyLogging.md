# Driver Dependency Logging Toolset

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

This test module is intended to be added to the Core DXE dispatcher to provide an evaluation of each DXE driver loaded
during boot for dependency expression declaration and protocol usage.  Notification will be provided for any driver
using a protocol not covered by the depex and any depex GUID not used in a locate protocol call.

## Enabling data logging

  This library is tightly coupled to the MdeModulePkg core DXE main driver, but support is handled by the constructor
  with no code modification necessary.  To enable logging, add the library INF file to the platform DSC file's DxeMain
  driver declaration as follows:
  
  ```inf
  MdeModulePkg/Core/Dxe/DxeMain.inf {
    <LibraryClasses>
      NULL|UefiTestingPkg/Library/DriverDependencyLogging/DriverDependencyLogging.inf
  }
  ```

## Data logging process

  The data logging is performed by the DispatchDataLogging library in 2 parts:
  
  **Part 1: Logging LocateProtocol() function call data:**
    The library constructor will hook the boot services table LocateProtocol function and record each calls' memory
    location of the GUID, and what GUID is being used.  The location of the GUID will later be used to determine which
    driver made the call, and the GUID value will be used during the evaluation process to compare to the driver's
    dependency expression.
  
  **Part 2: Logging driver usage data**
    The library constructor will register for a callback at ready-to-boot and record pertinent data collected during
    the boot process to both the UEFI boot log and a volatile variable in variable services:
      Variable Name:   "DEPEX_LOG_v1"
      Namespace GUID:  { 0x4D2A2AEB, 0x9138, 0x44FB, { 0xB6, 0x44, 0x22, 0x17, 0x5F, 0xBB, 0xB0, 0x85 }}

  The pertinent data is collected at ready-to-boot by walking the CoreDxe driver's linked list of dispatched drivers
  and for each that is sucessfully loaded, a data line is created and sent to the log.

  ```console
  DEPEX_LOG_v1_BEGIN   Tag indicating begin of pertinent data
  Data_Line_1          Pertinent data from first driver logged
  Data_Line_2          Pertinent data from second driver logged
  ...                  Pertinent data from ...
  Data_Line_(n)        Pertinent data from last driver logged
  DEPEX_LOG_v1_END     Tag indicating end of pertinent data
  ```
  
  Each Data_Line added to the log will focus on a single driver and contains 3 parts separated by a '|' character.
  
  ```console
  "<name>|<depex>|<guid_1>.<guid_2>.< ... >.<guid_n>\n"   Line format
  <name>   = Name of the driver
  <depex>  = The driver's dependency expression in hex using 2 chars per byte ("AABBCC...")
  <guid_n> = All guids used by this driver when calling gBS->LocateProtocol() are listed after the second '|'
             character, are in the format ("AAAAAAAA-BBBB-CCCC-DDDD-EEEEEEEEEEEE"), and are separated by '.'
  ```

## Data evaluation process

  The data logged by the UEFI is evaluated by using the tool "DispatchDataParser.py" to determine which drivers could
  potentially cause loading issues.  Running the script with parameter -h gives command line usage and the script will
  process the data as follows:
  
  1. Collect all GUIDs defined by a UEFI tree .dec file as [protocols], [PPIs], or [guids] then output a JSON file
     showing the values found, all names for each, and which .DEC file they were recorded from.  Any error in this
     collection process will result in a console message and the script halting with a return code of 2.

     ```json
     [                                                    <<< JSON file is a list of entries
       {
         "Guid": 'AAAAAAAA-BBBB-CCCC-DDDD-EEEEEEEEEEEE',  <<< Unique GUID value parsed from the tree
         "Info": [                                        <<< List of name information entries for this GUID
                   {
                     "FileName": "<file name>",           <<< Name of .DEC file declaring this GUID
                     "GuidName": "<guid name>"            <<< Name associated with the GUID value
                   },
                   ...                                    <<< One entry for each name associated with a GUID
                 ]
       },
       ...                                                <<< One entry for each GUID found
     ]
     ```

  2. Read all pertinent data from the input log, validate each Data_Line format, then output a JSON file showing the
     data expanded into a parseable format.  Any error in this collection process will result in a console message and
     the script halting with a return code of 2.

     ```json
     [
       {
         "Name": "...",                                      <<< Name of driver in ASCII text
         "RawDepex": "AA BB CC...",                          <<< Binary dependency data
         "DepexStack": [                                     <<< Dependency data decoded into a command/GUID list
           {
             "Command": "AA",                                <<< Hex value of dependency command
             "CommandName": "...",                           <<< String representation of command
             "Guid": "AAAAAAAA-BBBB-CCCC-DDDD-EEEEEEEEEEEE", <<< GUID associated with command
             "GuidName": "..."                               <<< Name found in the GUID scan from step 1
           },                                                    If multiple names found, they are separated by '|'
           ...
         ],
         "UsedProtocols": [                                  <<< List of protocols located by the driver
           {
             "Guid": "AAAAAAAA-BBBB-CCCC-DDDD-EEEEEEEEEEEE", <<< GUID used when LocateProtocol was called
             "GuidName": "..."                               <<< Name found in the GUID scan from step 1
           },                                                    If multiple names found, they are separated by '|'
           ...
         ]
       },
       ...
     ]
     ```

  3. Create evaluation output file listing the following items:
     - A list of drivers that contained an invalid depex stack
     - A list of GUIDs that had multiple names assigned
     - A list of drivers that used a protocol in a gBS->LocateProtocol() call without declaring it in the depex
       Note: This may not indicate a true error due to using other protections such as a callback when installed
     - A list of drivers that declared a protocol in the depex but never used it in a call to gBS->LocateProtocol()
       Note: This may not indicate a true error due to reasons such as load ordering without using the protocol
     Any error in this evaluation process will not halt the script, but will message the warning to the console and
     results file, with detailed information in the results file, and a return code of 1.
