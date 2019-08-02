# Xml Support Package

## About

This package adds some limited XML support to the UEFI environment.  Xml brings value in that there are numerous,
robust, readily available parsing solutions in nearly every environment, language, and operating system. 
The UEFI support is limited in that it only supports the ASCII strings and does not support XSD, schema, namespaces,
or other extensions to XML.  

### XmlTreeLib

The XmlTreeLib is the cornerstone of this package.  It provides functions for:
* Reading and parsing XML strings into an XML node/tree structure
* Creating or altering xml nodes within a tree
* Writing xml nodes/trees to ASCII string
* Escaping and Un-Escaping strings


### XmlTreeQueryLib

The XmlTreeQueryLib provides very basic and simple query functions allowing code to interact 
with the XmlTree to do things like:
* Find the first child element node with a name equal to the parameter
* Find the first attribute node of a given element with a name equal to the parameter

### UnitTestResultReportLib

A UnitTestResultReportLib that formats the results in XML using the JUnit defined
schema.  This instance allows the UEFI Unit Test Framework to integrate results with
existing tools and other frameworks.  

## Testing

There are UEFI shell application based unit tests for each library.  These tests attempt to verify basic functionality of public interfaces.  Check the **UnitTests** folder at the root of the package.  

## Developer Notes

* These libraries have known limitations and have not been fully vetted for un-trusted input.  If used in such a 
situation it is suggested to validate the input before leveraging the XML libraries.  With that said the ability to use
xml in UEFI has been invaluable for building features and tests that interact with code running in other environments.
* The parser has been tuned to fail fast and in invalid XML just return NULL.  

## Copyright

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent
