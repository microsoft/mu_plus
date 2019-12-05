@echo off
rem @file
rem
rem Script to create the HTTPS cert with multiple entires in the Subject Alternative Names field.
rem
rem Copyright (c), Microsoft Corporation
rem SPDX-License-Identifier: BSD-2-Clause-Patent
rem

erase DFCI_HTTPS.pem
erase DFCI_HTTPS.cer
erase DFCI_HTTPS.pfx


openssl.exe req -x509 -nodes -sha256 -days 730 -newkey rsa:2048 -keyout DFCI_HTTPS.key -out DFCI_HTTPS.pem -config httpreq.cnf

openssl.exe pkcs12 -nodes -inkey DFCI_HTTPS.key -in DFCI_HTTPS.pem -export -out DFCI_HTTPS.pfx

openssl.exe x509 -in DFCI_HTTPS.pem -outform der -out DFCI_HTTPS.cer
