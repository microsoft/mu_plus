/** @file
This library supports generic XML queries based on the Xml Tree lib.


Copyright (C) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __XML_TREE_QUERY_LIB__
#define __XML_TREE_QUERY_LIB__

// Max length of an Element Name
#define MAX_ELEMENT_NAME_LENGTH    (50)
#define MAX_ATTRIBUTE_NAME_LENGTH  (50)

/**
Find the first 1st generation child that has a matching ElementName

@param[in]  ParentNode to search under
@param[in]  ElementName to search for

@retval
**/
XmlNode *
EFIAPI
FindFirstChildNodeByName (
  IN CONST XmlNode  *ParentNode,
  IN CONST CHAR8    *ElementName
  );

/**
Find the first 1st attribute of the node that has a matching name

@param[in]  Node to search under
@param[in]  AttributeName to search for

@retval XmlAttribute that matches or NULL if not found
**/
XmlAttribute *
EFIAPI
FindFirstAttributeByName (
  IN CONST XmlNode  *Node,
  IN CONST CHAR8    *AttributeName
  );

#endif
