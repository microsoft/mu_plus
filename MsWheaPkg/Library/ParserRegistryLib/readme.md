# ParserRegistryLib

## About

Holds a table which associates guids with function pointers used for parsing section data.
If an entity wishes to parse a section type in a specific way, they simply need to call the register
function using a the section type guid and function pointer. When HwhMenu.c tries to parse
section data, it will look through all guids and return a function pointer if one matches.
The functions being registered must adhere to the SECTIONFUNCTIONPTR type located in
ParserRegistryLib.h.

## Copyright

Copyright (C) Microsoft Corporation. All rights reserved.

SPDX-License-Identifier: BSD-2-Clause-Patent
