# Xml Support Package
## &#x1F539; Copyright
Copyright (c) 2017, Microsoft Corporation

All rights reserved. Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

## &#x1F539; About
This package adds some limited XML support to the UEFI environment.  Xml brings value in that there are numerous,
robust, readily avilable parsing solutions in nearly every environment, language, and operating system. 
The UEFI support is limited in that it only supports the ASCII strings and does not support XSD, schema, namespaces,
or other extensions to XML.  

### &#x1F538; XmlTreeLib
The XmlTreeLib is the cornerstone of this pacakge.  It provides functions for:
* Reading and parsing XML strings into an XML node/tree stucture
* Creating or altering xml nodes within a tree
* Writing xml nodes/trees to ASCII string
* Escaping and Un-Escaping strings


### &#x1F538; XmlTreeQueryLib
The XmlTreeQueryLib provides very basic and simple query functions allowing code to interact 
with the XmlTree to do things like:
* Find the first child element node with a name equal to the parameter
* Find the first attribute node of a given element with a name equal to the parameter

### &#x1F538; UnitTestResultReportLib
A UnitTestResultReportLib that formats the results in XML using the JUnit defined
schema.  This instance allows the UEFI Unit Test Framework to integrate results with
existing tools and other frameworks.  

## &#x1F539; Testing
There are UEFI shell application based unit tests for each library.  These tests attempt to verify basic functionality of public interfaces.  Check the **UntTests** folder at the root of the package.  

## &#x1F539; Developer Notes
* These libraries have known limitations and have not been fully vetted for untrusted input.  If used in such a 
situation it is suggested to validate the input before leveraging the XML libraries.  With that said the ability to use
xml in UEFI has been invaluable for building features and tests that interact with code running in other environments.
* The parser has been tuned to fail fast and in invalid XML just return NULL.  