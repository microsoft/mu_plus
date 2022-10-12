# HwhMenu

## About

![HWH Menu](hwh_menu_mu.gif)

A UEFI front page application which displays various fields of hardware error records to the user.
The application attempts to parse fields to provide more useful information and prints the raw
data if it cannot.

## Main File

### **HwhMenu.c**

The main application file.

### Loading Logs

The HwErrRecs are loaded when the Hardware Health tab is first opened using GetVariable().
The records are verified using CheckHwErrRecHeaderLib within MsWheaPkg before being added to a
linked list structure. The linked list is not being deleted because it will simply be reclaimed
when the OS boots or another allocation call is made which needs that memory. The config struct
used by the vfr holds a single UINT8 which if equal to LOGS_TRUE means there are errors to
display. If it is equal to LOGS_FALSE, the page will be suppressed and a string saying that
there are no logs present will be displayed at the top.

### Paging between logs and updating the form

There is a simple paging interface between records.
Whenever Next or Previous is pressed by the user, the page attempts to perform the desired action.
If it is successful, UpdateForm() is called which simply inserts nothing into the label section
of the VFR. What this does is make the form think it has been updated and tricks it into
refreshing, thus allowing all error specific strings which have been edited to be displayed to
the user.

### Parsing the logs

Each field which we display is parsed using the structs defined in Cper.h.
Parsing most fields is straightforward, but parsing section data requires a bit more work.
When preparing section data for the user, a call is made to ParserRegistryLib using the Section
Type guid within the section header of the hardware error record. The parser registry holds a
table which associates guids with function pointers, and any entity which employs its own section
guid should register a section data parser with the library. During parsing, if the section type
matches one in the register, the function pointer is called with the parameters specified in
ParserRegistryLib.h (SECTIONFUNCTIONPTR). The parser parses the data and populates an array of
strings with their desired display. HwhMenu.c holds a 2D array of EFI_STRING_IDs which are the
lines/columns where section data can be placed. The populated string array is written to these
strings until there is no more data to display or there are no more strings to put the data.
The user can place '\n' within their strings to separate a line into columns. See the
GenericSectionParserLib for a parsing example.

### Adding to the Section Parser

If you, the platform developer, recognize a section guid and know how to parse the bytes, you can provide a
function to do so by calling ParserLibRegisterSectionParser() with the guid you're able to parse and a function
pointer to parse it. This can be done within a new driver or by simply extending an existing driver. The HWH menu
will pass to your function (if the guid matches) a pointer to the section data and a pointer to an array of strings
to fill out with your completed parse.

## Secondary files

### **CreatorIDParser.c and PlatformIDParser.c**

Called from within the HwhMenu.c file to parse the Creator and Platform IDs.

### **HwhMenuVfr.h**

Holds configuration information and guid opcodes used in the VFR file

## Including the HWH Menu

To include, paste the following in the platform DSC flie:

``` { .md }

[LibraryClasses]
    ParserRegistryLib     |MsWheaPkg/Library/ParserRegistryLib/ParserRegistryLib.inf
    CheckHwErrRecHeaderLib|MsWheaPkg/Library/CheckHwErrRecHeaderLib/CheckHwErrRecHeaderLib.inf
[Components.X64]
    MsWheaPkg/HwhMenu/HwhMenu.inf

```

and the following in the platform FDF file

``` { .md }

[FV.FVDXE]
    INF  MsWheaPkg/HwhMenu/HwhMenu.inf

```

## Copyright

Copyright (C) Microsoft Corporation. All rights reserved.

SPDX-License-Identifier: BSD-2-Clause-Patent
