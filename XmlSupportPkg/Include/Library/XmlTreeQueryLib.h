/** @file
This library supports generic XML queries based on the Xml Tree lib.


Copyright (c) 2017, Microsoft Corporation

All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

**/

#ifndef __XML_TREE_QUERY_LIB__
#define __XML_TREE_QUERY_LIB__

//Max length of an Element Name
#define MAX_ELEMENT_NAME_LENGTH   (50)  
#define MAX_ATTRIBUTE_NAME_LENGTH (50)

/**
Find the first 1st generation child that has a matching ElementName

@param[in]  ParentNode to search under
@param[in]  ElementName to search for

@retval 
**/
XmlNode*
EFIAPI
FindFirstChildNodeByName(
  IN CONST XmlNode *ParentNode,
  IN CONST CHAR8     *ElementName
  );


/**
Find the first 1st attribute of the node that has a matching name

@param[in]  Node to search under
@param[in]  AttributeName to search for

@retval XmlAttribute that matches or NULL if not found
**/
XmlAttribute*
EFIAPI
FindFirstAttributeByName(
  IN CONST XmlNode *Node,
  IN CONST CHAR8     *AttributeName
  );

#endif
