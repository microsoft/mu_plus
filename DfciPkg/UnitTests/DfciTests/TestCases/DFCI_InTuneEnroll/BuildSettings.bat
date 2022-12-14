@echo off
rem @file
rem
rem Script to insert a cert into an XML settings packet
rem
rem Copyright (c), Microsoft Corporation
rem SPDX-License-Identifier: BSD-2-Clause-Patent
rem

..\..\Support\Python\InsertCertIntoXML.py --BinFilePath ..\..\certs\DFCI_HTTPS.cer --OutputFilePath DfciSettings.xml --PatternFilePath DfciSettingsPattern.xml
