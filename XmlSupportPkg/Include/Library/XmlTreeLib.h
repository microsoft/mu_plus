/**
XmlTreeLib.h

Library to support creating, deleting, reading, and writing XML.  

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
#ifndef __XML_TREE_LIB_H__
#define __XML_TREE_LIB_H__


#define XML_MAX_ATTRIBUTE_VALUE_LENGTH	(1024)
#define XML_MAX_ELEMENT_VALUE_LENGTH	(0xFFFF)

/**
This function will create a xml tree given an XML document as a ascii string.

@param   XmlDocument     -- XML document to create the node list for.
@param   SizeXmlDocument -- Length of the document.
@param   RootNode        -- The root node that contains the node list.

@return  EFI_SUCCESS or underlying failure code.

**/
EFI_STATUS
EFIAPI
CreateXmlTree(
  IN  CONST CHAR8*      XmlDocument,
  IN        UINTN       SizeXmlDocument,
  OUT       XmlNode**   RootNode
);

/**
  This function creates a new XML tree.

  @param[in]   Parent   -- Optional parent for this node.
  @param[in]   Name     -- Name for this node.
  @param[in]   Value    -- Optional value for this node.
  @param[out]  Node     -- Optional return pointer for this node.

  @return  EFI_SUCCESS or underlying failure code.

**/
EFI_STATUS
EFIAPI
AddNode(
  IN        XmlNode*     Parent OPTIONAL,
  IN  CONST CHAR8*       Name,
  IN  CONST CHAR8*       Value OPTIONAL,
  OUT       XmlNode**    Node OPTIONAL
);


/**
  This function adds an existing tree to a parent node.

  @param   Parent   -- Parent to add to.
  @param   Tree     -- Existing Tree to to add to the parent as a child.

  @return  EFI_SUCCESS or underlying failure code.

**/

EFI_STATUS
EFIAPI
AddChildTree(
  IN XmlNode* Parent,
  IN XmlNode* Tree
);


/**
  This adds an attribute to an existing XmlNode.

  @param   Parent   -- Parent for this node.
  @param   Name     -- Name for this node.
  @param   Value    -- Optional value for this node.

  @return  EFI_SUCCESS or underlying failure code.

**/
EFI_STATUS
EFIAPI
AddAttributeToNode(
  IN       XmlNode*   Parent,
  IN CONST CHAR8*     Name,
  IN CONST CHAR8*     Value
);

/**
  This function frees the string resources associated with the node,
  and removes it from it's parent node list if it has one.

  @param   Node  -- Node to free.

  @return  EFI_SUCCESS or underlying failure code.

**/
EFI_STATUS
EFIAPI
DeleteNode(
  IN XmlNode* Node
);


/**
  This function frees the string resources associated with the attribute,
  and removes it from it's parent attribute list.

  @param   Attribute  -- Attribute to free.

  @return  EFI_SUCCESS or underlying failure code.

**/
EFI_STATUS
EFIAPI
DeleteAttribute(
  IN XmlAttribute* Attribute
);

/**
  This function will free all of the resources allocated for an XML Tree.
  
  @param   RootNode -- The root node that contains the node list.

  @return  EFI_SUCCESS or underlying failure code.

**/
EFI_STATUS
EFIAPI
FreeXmlTree(
  IN XmlNode** RootNode
);



/**
  This function uses DEBUG to print the Xml tree
  
  @param[in] Node  - Pointer to head xml element to print
  @param[in] Level - IndentLevel to make printing pretty.  
                     Each child element will be indented.
					 When calling externally this parameter should be 0
**/
VOID
EFIAPI
DebugPrintXmlTree(
  IN CONST XmlNode *Node,
  IN UINTN Level
); 

/**
Public function to create an ascii string from an xml tree.
This will use shortened XML notation and no whitespace and xml escaped strings.  (ideal for data transfer) 

@param[in]  Node - Root node or first node to start printing.
@param[out] BufferSize - Number of bytes that the string needed. Includes Null terminator
@param[out] String - Pointer to pointer of ASCII buffer.  Will be set for return to caller.  C
                     Caller needs to free using FreePool when finished
**/
EFI_STATUS
EFIAPI
XmlTreeToString(
  IN  CONST XmlNode    *Node,
  IN        BOOLEAN    Escaped,
  OUT       UINTN      *BufferSize,
  OUT       CHAR8      **String
);

/**
Function to calculate the size of the Ascii string needed
to print this XmlNode and its children.  Generally assumed it will
be the root node of XML Tree.

This uses a shortened XML format (for empty elements), minimal whitespace, and xml escaped strings

@param Node - The root node of the xml tree to start printing from
@param Size - the number of Ascii characters in the string.  This does not include the NULL terminator to end

**/
EFI_STATUS
EFIAPI
CalculateXmlDocSize(
  IN  CONST XmlNode* Node,
  IN        BOOLEAN    Escaped,
  OUT       UINTN*   Size
);

/**
Escape the string so any XML invalid chars are
converted into valid chars.

@param String - Ascii string to escape
@param MaxStringLength - Max length of the Ascii string "String"
@param EscapedString - resulting escaped string if return value is success.  Memory must be freed by caller

@return Status of escape process.  ON success EscapedString * will point to escaped string.
**/
EFI_STATUS
EFIAPI
XmlEscape(
  IN CONST CHAR8* String,
  IN UINTN        MaxStringLength,
  OUT CHAR8**     EscapedString
);

/**
Remove XML escape sequences in the string so strings are valid for string operations

@param EscapedString - Xml Escaped Ascii string
@param MaxStringLength - Max length of the Ascii string "EscapedString"
@param String - resulting non escaped string if return value is success.  Memory must be freed by caller

@return Status of un escape process.  ON success String * will point to string that contains no XML escape sequences.
**/
EFI_STATUS
EFIAPI
XmlUnEscape(
	IN CONST CHAR8* EscapedString,
	IN UINTN        MaxEscapedStringLength,
	OUT CHAR8**     String
);


/**
Function to go thru a tree and count the nodes
**/
EFI_STATUS
EFIAPI
XmlTreeNumberOfNodes(
  IN  CONST XmlNode* Node,
  OUT       UINTN* Count
);

/**
Function to go thru a tree and count the attributes
**/
EFI_STATUS
EFIAPI
XmlTreeNumberOfAttributes(
  IN  CONST XmlNode* Node,
  OUT       UINTN* Count
);

/**
Function to go thru a tree and report the max depth

**/
EFI_STATUS
EFIAPI
XmlTreeMaxDepth(
  IN  CONST XmlNode* Node,
  IN OUT    UINTN* Depth
);

/**
Function to go thru a tree and report the max number of attributes on any element

**/
EFI_STATUS
EFIAPI
XmlTreeMaxAttributes(
  IN  CONST XmlNode* Node,
  IN OUT    UINTN* MaxAttributes
);



#endif // __MS_XML_NODE_INC__