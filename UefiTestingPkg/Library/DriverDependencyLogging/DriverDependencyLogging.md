**Driver Dependency Logging Toolset**

  Copyright (C) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  This test module is intended to be added to the Core DXE dispatcher to provide an evaluation of each DXE
  driver loaded during boot for dependency expression declaration and protocol usage.  Notification will be
  provided for any driver using a protocol not covered by the depex and any depex GUID not used in a
  locate protocol call.


**Enabling data logging:**

  This library is tightly coupled to the MdeModulePkg core DXE main driver, but support is handled by the
  constructor with no code modification necessary.  To enable logging, add the library INF file to the
  platform DSC file's DxeMain driver declaration as follows:
    ```console
    MdeModulePkg/Core/Dxe/DxeMain.inf {
      <LibraryClasses>
        NULL|UefiTestingPkg/Library/DriverDependencyLogging/DriverDependencyLogging.inf
    }
    ```


**Data logging process:**

  The data logging is performed by the DispatchDataLogging library in 2 parts:

  **Part 1:** Logging LocateProtocol() function call data:
    The library constructor will hook the boot services table LocateProtocol function and record each calls'
    memory location of the GUID, and what GUID is being used.  The location of the GUID will later be used to
    determine which driver made the call, and the GUID value will be used during the evaluation process to
    compare to the driver's dependency expression.

  **Part 2:** Logging driver usage data
    The library constuctor will also register for a callback at ready-to-boot that walks the core DXE driver's
    linked list of all drivers dispatched during boot.  For each driver that sucessfully loaded, it publishes
    the driver's name, its dependency expression, and a list of protocols it located through the LocateProtocol
    function.  This data is written to the UEFI boot log and saved as volatile variable in variable services
    using name "DEPEX_LOG_v1" and namespace GUID { 0x4D2A2AEB, 0x9138, 0x44FB, { 0xB6, 0x44, 0x22, 0x17, 0x5F,
    0xBB, 0xB0, 0x85 }}.

  The data logged is ASCII text using a new-line char to designate driver entries in the following format:
    ```console
    Start tag:    "DEPEX_LOG_v1_BEGIN\n"
    End tag:      "DEPEX_LOG_v1_END\n"
    Line format:  "<name>|<depex>|<guid_1>.<guid_2>.< ... >.<guid_n>\n"
      <name>  = Name of the driver in ASCII format
      <depex> = Dependency expression data in hex with 2 chars per byte ("AABBCC...")
      <guid>  = Protocols used by this driver in standard GUID format ("AAAAAAAA-BBBB-CCCC-DDDD-EEEEEEEEEEEE")
    ```

**Data evaluation process:**

  The data logged is evaluated by using the tool "DispatchDataParser.py" to determine which drivers could
  potentially cause loading issues.

  * Step 1: Pull/validate all driver entries (lines) from the input file
  * Step 2: Walk a UEFI tree collecting all GUIDs defined in .DEC files as [guids] and [protocols] along with
            the assigned name.
  * Step 3: Create a manageable JSON object for each driver:
      ```json
      {
        "Name": "...",                                      <<< Name of driver in ASCII text
        "RawDepex": "AA BB CC...",                          <<< Binary dependency data
        "DepexStack": [                                     <<< Dependency data decoded into a list of commands
          {
            "Command": "AA",                                <<< Hex value of dependency command
            "CommandName": "...",                           <<< String representation of command
            "Guid": "AAAAAAAA-BBBB-CCCC-DDDD-EEEEEEEEEEEE", <<< GUID associated with command
            "GuidName": "..."                               <<< Name assigned to GUID from the .DEC file scan
          },
          ...
        ],
        "UsedProtocols": [                                  <<< List of protocols located by the driver
          "AAAAAAAA-BBBB-CCCC-DDDD-EEEEEEEEEEEE",
          "AAAAAAAA-BBBB-CCCC-DDDD-EEEEEEEEEEEE",
          ...
        ]
      }
      ```
  * Step 4: Create evaluation output file listing the following items:
      * Errors from parsing the input file and code tree
      * GUIDs found in DEC files that had multiple assigned names
      * Warnings for drivers using a protocol without declaring it in the depex
      * Warnings for drivers declaring a prtocol in the depex but not using it
    Note:  The warnings are not definitive errors since GUIDs can be used safely without declaration in the depex
           and the depex can be used to force load ordering even if the protcol is not used in code.

  The tool DispatchDataParser.py tool is the primary tool to perform all evaluation and using the '-h' parameter
  will provide command line help.

  The tool UefiDecGuidParser.py is ingested by the DispatchDataParser.py tool to walk the UEFI tree looking for
  GUID names, but it can also be run stand-alone to get more information on GUID duplicates.  It too can be run
  with the '-h' parameter to get command line help.

