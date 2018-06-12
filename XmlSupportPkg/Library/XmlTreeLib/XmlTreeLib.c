/**
@file
Implementation of XmlTreeLib using the faster Xml lib modified to work in UEFI. 

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
#include <Uefi.h>                        // UEFI base types
#include <Library/MemoryAllocationLib.h> // Memory allocation
#include <Library/BaseLib.h>             // Safe string functions
#include <Library/DebugLib.h>            // DEBUG tracing
#include <Library/BaseMemoryLib.h>       // SetMem()
#include <XmlTypes.h>     
#include <Library/XmlTreeLib.h>          // Our header
#include "fasterxml/fasterxml.h"         // XML Engine
#include "fasterxml/xmlerr.h"            // XML errors


#ifndef fDoOnce
#define fDoOnce FALSE
#endif // fDoOnce

#ifndef MAX_PATH
#define MAX_PATH (260)
#endif // MAX_PATH

#ifndef ARRAYSIZE
#define ARRAYSIZE(A) sizeof(A)/sizeof(A[0])
#endif // ARRAYSIZE


//DEFINE the max number of nodes deep the parser will support
#define MAX_RECURSIVE_LEVEL (25)  


//
// Private function prototypes
//

UINTN
_GetXmlUnEscapedLength(
	IN CONST CHAR8* String,
	IN UINTN MaxStringLength
);

UINTN
_GetXmlEscapedLength(
	IN CONST CHAR8* String,
	IN UINTN MaxStringLength
);



/**
Given a character, determine if it is white space.
ch -- Character to test.
TRUE if ch == whitespace.
**/
BOOLEAN
EFIAPI
IsWhiteSpace(IN CHAR8 ch)
{
  BOOLEAN fIsWhiteSpace = FALSE;

  switch (ch)
  {
	  case '\0':
	  case '\r':
	  case ' ':
	  case '\t':
	  case '\n':
	  {
		fIsWhiteSpace = TRUE;
		break;
	  }
  }

  return fIsWhiteSpace;
}// IsWhiteSpace()

 /**
 Function to safely free a buffer.  If
 the buffer is NULL then just return.
 **/
VOID SafeFreeBuffer(CHAR8** ppBuff)
{
  if (ppBuff && *ppBuff)
  {
    FreePool(*ppBuff);
    *ppBuff = NULL;
  }

}// SafeFreeBuffer()


//
// Public functions
//

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
)
{
  EFI_STATUS Status = EFI_SUCCESS;
  CHAR8*     NodeValue = NULL;
  CHAR8*     NodeName = NULL;
  XmlNode*   NodeTemp = NULL;

  do
  {
    if (Name == NULL || AsciiStrLen(Name) == 0)
    {
      DEBUG((EFI_D_ERROR, "ERROR:  AddNode(), pszName or length was NULL\n"));
      Status = EFI_INVALID_PARAMETER;
      break;
    }

    if (Node)
    {
      *Node = NULL;
    }

    NodeTemp = (XmlNode*)AllocateZeroPool(sizeof(XmlNode));
    if (NodeTemp == NULL)
    {
      Status = EFI_OUT_OF_RESOURCES;
      break;
    }

    NodeName = (CHAR8*)AllocateZeroPool(AsciiStrLen(Name) + 1);
    if (NodeName == NULL)
    {
      Status = EFI_OUT_OF_RESOURCES;
      break;
    }
    Status = AsciiStrCpyS(NodeName, AsciiStrLen(Name) + 1, Name);
    if (EFI_ERROR(Status))
    {
      break;
    }

    if (Value && *Value != '\0')
    {
	  Status = XmlUnEscape(Value, XML_MAX_ELEMENT_VALUE_LENGTH, &NodeValue);
      //NodeValue = AllocateZeroPool(AsciiStrLen(Value) + 1);
      if (EFI_ERROR(Status))
      {
        break;
      }
    }

    //
    // Fill in the node names and values...
    //
    NodeTemp->ParentNode = Parent;
    NodeTemp->Name = NodeName;
    NodeTemp->Value = NodeValue;

    //
    // Initailize our list head entries.
    //
    InitializeListHead(&(NodeTemp->ChildrenListHead));
    InitializeListHead(&(NodeTemp->AttributesListHead));

    // 
    // If we have a parent, we need to add this node to the Parent's child list.
    //
    if (Parent)
    {

      //
      // Add the node to the parent's child list.
      //
      InsertTailList(&(Parent->ChildrenListHead), &NodeTemp->Link);

      //
      // Increase the number of child nodes that the parent owns.
      //
      Parent->NumChildren++;

    }

    //
    // Let the caller have the node pointer now.
    //
    if (Node)
    {
      *Node = NodeTemp;
    }

  } while (fDoOnce);

  //
  // Cleanup on error...
  //
  if (EFI_ERROR(Status))
  {
    //
    // SafeFreeBuffer() only frees the memory if the pointer is not NULL.
    // It then sets the pointer to null.
    //
    SafeFreeBuffer((CHAR8**)&NodeTemp);
    SafeFreeBuffer(&NodeName);
    SafeFreeBuffer(&NodeValue);
  }

  return Status;
}// AddNode()

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
)
{
  EFI_STATUS Status = EFI_SUCCESS;

  do
  {
    if (Parent == NULL || Tree == NULL)
    {
      Status = EFI_INVALID_PARAMETER;
      break;
    }

    //
    // Add the node to the parent's child list.
    //
    InsertTailList(&(Parent->ChildrenListHead), &Tree->Link);

    //
    // Increase the number of child nodes that the parent owns.
    //
    Parent->NumChildren++;

    //
    // Set the node's new parent...
    //
    Tree->ParentNode = Parent;

  } while (fDoOnce);

  return Status;
}//AddChildNodeList()


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
)
{
  EFI_STATUS      Status = EFI_SUCCESS;
  CHAR8*          AsciiString = NULL;
  XmlAttribute*   Attribute = NULL;

  do
  {
    if (Parent == NULL || Name == NULL || AsciiStrLen(Name) == 0 || Value == NULL || AsciiStrLen(Value) == 0)
    {
      DEBUG((EFI_D_ERROR, "ERROR:  AddAttributeToNode(), invalid parameter\n"));
      Status = EFI_INVALID_PARAMETER;
      break;
    }

    //
    // Allocate the attribute structure
    //
    Attribute = (XmlAttribute*)AllocateZeroPool(sizeof(XmlAttribute));
    if (Attribute == NULL)
    {
      Status = EFI_OUT_OF_RESOURCES;
      break;
    }

    //
    // Allocate and store the name...
    //
    AsciiString = (CHAR8*)AllocateZeroPool(AsciiStrLen(Name) + 1);
    if (AsciiString == NULL)
    {
      Status = EFI_OUT_OF_RESOURCES;
      break;
    }
    Status = AsciiStrCpyS(AsciiString, AsciiStrLen(Name) + 1, Name);
    if (EFI_ERROR(Status))
    {
      break;
    }
    Attribute->Name = AsciiString;

    //
    // Allocate and store the value...
    //
    AsciiString = NULL;
	Status = XmlUnEscape(Value, XML_MAX_ATTRIBUTE_VALUE_LENGTH, &AsciiString);
    if (EFI_ERROR(Status))
    {
      break;
    }
    Attribute->Value = AsciiString;

    //
    // Add the node to the parent's child list and increase the number of 
    // attributes within this node.
    //
    InsertTailList(&(Parent->AttributesListHead), &(Attribute->Link));
    Parent->NumAttributes++;
    Attribute->Parent = Parent;

  } while (fDoOnce);

  //
  // Cleanup on error...
  //
  if (EFI_ERROR(Status))
  {
    //
    // SafeFreeBuffer() only frees the memory if the pointer is not NULL.
    // It then sets the pointer to null.
    //
    if (Attribute)
    {
      SafeFreeBuffer((CHAR8**)&Attribute->Name);
      SafeFreeBuffer((CHAR8**)&Attribute->Value);
      SafeFreeBuffer((CHAR8**)&Attribute);
    }
  }

  return Status;
}// AddAttributeToNode()


 /**
 This function frees the string resources associated with the node,
 and removes it from it's parent node list if it has one.

 @param   Node  -- Node to free.

 @return  EFI_SUCCESS or underlying failure code.

 **/
EFI_STATUS
EFIAPI
DeleteNode(IN XmlNode* Node)
{
  EFI_STATUS      Status = EFI_SUCCESS;
  LIST_ENTRY*     Link = NULL;

  //check for null
  if (Node == NULL)
  {
    return EFI_INVALID_PARAMETER;
  }


  
  //delete any children - can't use for loop because removal breaks iterator
  while (!IsListEmpty(&Node->ChildrenListHead))
  {
    Link = GetFirstNode(&Node->ChildrenListHead);
    Status = DeleteNode((XmlNode*)Link);
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, __FUNCTION__ " Failed to delete node.  Status = %r\n", Status));
      ASSERT_EFI_ERROR(Status);
    }

    //Now remove it from our children list
    RemoveEntryList(Link);
    Node->NumChildren--;
    FreePool(Link);
  }

  // all children gone....
  //now remove all attributes - can't use for loop because removal breaks iterator
  while (!IsListEmpty(&Node->AttributesListHead))
  {
    Link = GetFirstNode(&Node->AttributesListHead);
  
    Status = DeleteAttribute((XmlAttribute*)Link);
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, __FUNCTION__ " Failed to delete attribute.  Status = %r\n", Status));
      ASSERT_EFI_ERROR(Status);
    }

    //now remove from Attribute list
    RemoveEntryList(Link);
    Node->NumAttributes--;
    FreePool(Link);
  }//go to next attribute

  //now free our node memory
  SafeFreeBuffer(&(Node->XmlDeclaration.Declaration));
  SafeFreeBuffer(&(Node->Name));
  SafeFreeBuffer(&(Node->Value));
  Node->ParentNode = NULL;

  return Status;
}// DeleteNode()


/**
 This function frees the string resources associated with the attribute,
 and removes it from it's parent attribute list.

 @param   Attribute  -- Attribute to free.

 @return  EFI_SUCCESS or underlying failure code.

**/
EFI_STATUS
EFIAPI
DeleteAttribute(
  IN XmlAttribute* Attribute)
{
  EFI_STATUS Status = EFI_SUCCESS;

  if (Attribute == NULL)
  {
    return EFI_INVALID_PARAMETER;
  }

  SafeFreeBuffer(&(Attribute->Name));
  SafeFreeBuffer(&(Attribute->Value));
  Attribute->Parent = NULL;
  return Status;
}// DeleteAttribute()


/**
Function to calculate the size of the Ascii string needed
to print this XmlNode and its children.
This uses a shortened XML format, minimal whitespace, and xml encoded strings

This is an internal function.  Public function is CalculateXmlDocSize

**/
EFI_STATUS
EFIAPI
_CaclSizeRecursively(
  IN  CONST XmlNode*   Node,
  IN        BOOLEAN    Escaped,
  OUT       UINTN*     Size,
  IN        UINTN      Level)
{
  XmlAttribute *Att = NULL;
  LIST_ENTRY *Link = NULL;
  UINTN      NameSize = 0;
  EFI_STATUS Status = EFI_SUCCESS;

  if ((Node == NULL) || (Size == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  if (Level > MAX_RECURSIVE_LEVEL)
  {
    DEBUG((DEBUG_ERROR, "!!!ERROR: BAD XML.  Allowable recursive depth exceeded.\n"));
    return EFI_BAD_BUFFER_SIZE;
  }

  /* handle the xml decl*/
  if (Node->XmlDeclaration.Declaration != NULL)
  {
    if (Node->ParentNode != NULL)
    {
      DEBUG((DEBUG_ERROR, "!!!ERROR: BAD XML.  Should not have XmlDeclaration for a non-root node\n"));
    }
    *Size = (*Size) + AsciiStrLen(Node->XmlDeclaration.Declaration);
  }

  /* Handle start tag*/
  NameSize = AsciiStrLen(Node->Name);
  *Size = (*Size) + NameSize + 1;  //'<'

  //Loop attributes
  for (Link = Node->AttributesListHead.ForwardLink; Link != &(Node->AttributesListHead); Link = Link->ForwardLink)
  {
    Att = (XmlAttribute*)Link;
    *Size = (*Size) + AsciiStrLen(Att->Name) + 4; // '=', ' ','"','"'
	if (Escaped)
	{
		*Size = (*Size) + _GetXmlEscapedLength(Att->Value, XML_MAX_ATTRIBUTE_VALUE_LENGTH);
	}
	else
	{
		*Size = (*Size) + AsciiStrLen(Att->Value);
	}
  }

  //handle children and ending
  if ((Node->Value == NULL) && (Node->NumChildren == 0))
  {
    //Special short cut on the node  - Use empty node notation  />
    *Size = (*Size) + 3; // ' ', '/', '>'
  }
  else  //longer notation
  {
    *Size = (*Size) + 1; // '>'

    //Show Value if value
    if (Node->Value != NULL)
    {
	  if (Escaped)
	  {
		  *Size = (*Size) + _GetXmlEscapedLength(Node->Value, XML_MAX_ELEMENT_VALUE_LENGTH);
	  }
	  else
	  {
		  *Size = (*Size) + AsciiStrLen(Node->Value);
	  }
    }

    // Process all children
    if (Node->NumChildren > 0)
    {
      UINTN child = 0;  //use for debugging only
      //loop children
      for (Link = Node->ChildrenListHead.ForwardLink; Link != &(Node->ChildrenListHead); Link = Link->ForwardLink, child++)
      {
        Status = _CaclSizeRecursively((CONST XmlNode*)Link, Escaped, Size, Level+1);
        if (EFI_ERROR(Status))
        {
          DEBUG((DEBUG_ERROR, "%a - Error Status from child index %d of element: %a\n", __FUNCTION__, child, Node->Name));
          return Status;
        }
      }
    } //end children loop

    *Size = (*Size) + NameSize + 3; // '<', '/', '>/
  } //end of node
  return EFI_SUCCESS;
}

/**
Function to calculate the size of the Ascii string needed
to print this XmlNode and its children.  Generally assumed it will
be the root node.

This uses a shortened XML format and uses minimal whitespace.

@param Node - The root node of the xml tree to start printing from
@param Size - the number of Ascii characters in the string.  This does not include the NULL terminator

**/
EFI_STATUS
EFIAPI
CalculateXmlDocSize(
  IN  CONST XmlNode* Node,
  IN        BOOLEAN    Escaped,
  OUT       UINTN*   Size)
{

  if ((Node == NULL) || (Size == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  if (Node->ParentNode != NULL)
  {
    DEBUG((DEBUG_WARN, "%a - Called with node other than root node.  Siblings will not be traversed.\n", __FUNCTION__));
  }

  *Size = 0;

  return _CaclSizeRecursively(Node, Escaped, Size, 0);
}

/**
Internal function to convert an Xml Node and its children into an Ascii string
using shortened Xml Notation and no whitespace.

Public function is XmlTreeToString
**/
EFI_STATUS
EFIAPI
_ToStringRecursively(
  IN  CONST XmlNode*   Node,
  IN        UINTN      BufferSize,
  IN OUT    CHAR8      *String,
  IN        UINTN      Level,
  IN        BOOLEAN    Escaped)
{
  XmlAttribute *Att = NULL;
  LIST_ENTRY *Link = NULL;
  EFI_STATUS Status = EFI_SUCCESS;

  if ((Node == NULL) || (String == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  if (Level > MAX_RECURSIVE_LEVEL)
  {
    DEBUG((DEBUG_ERROR, "!!!ERROR: BAD XML.  Allowable recursive depth exceeded.\n"));
    return EFI_BAD_BUFFER_SIZE;
  }

  /* handle the xml decl*/
  if (Node->XmlDeclaration.Declaration != NULL)
  {
    if (Node->ParentNode != NULL)
    {
      DEBUG((DEBUG_ERROR, "!!!ERROR: BAD XML.  Should not have XmlDeclaration for a non-root node\n"));
    }
    Status = AsciiStrCatS(String, BufferSize, Node->XmlDeclaration.Declaration);
    if (EFI_ERROR(Status))
    {
      goto EXIT;
    }
  }

  /* Handle start tag*/
  Status = AsciiStrCatS(String, BufferSize, "<");
  if (EFI_ERROR(Status))
  {
    goto EXIT;
  }

  Status = AsciiStrCatS(String, BufferSize, Node->Name);
  if (EFI_ERROR(Status))
  {
    goto EXIT;
  }

  //Loop attributes
  for (Link = Node->AttributesListHead.ForwardLink; Link != &(Node->AttributesListHead); Link = Link->ForwardLink)
  {
    Att = (XmlAttribute*)Link;
    Status = AsciiStrCatS(String, BufferSize, " ");
    if (EFI_ERROR(Status))
    {
      goto EXIT;
    }
    Status = AsciiStrCatS(String, BufferSize, Att->Name);
    if (EFI_ERROR(Status))
    {
      goto EXIT;
    }

    Status = AsciiStrCatS(String, BufferSize, "=\"");
    if (EFI_ERROR(Status))
    {
      goto EXIT;
    }
	if (Escaped)
	{
		CHAR8 *EscapedString = NULL;
		Status = XmlEscape(Att->Value, XML_MAX_ATTRIBUTE_VALUE_LENGTH, &EscapedString);
		if (EFI_ERROR(Status))
		{
			goto EXIT;
		}
		Status = AsciiStrCatS(String, BufferSize, EscapedString);
		SafeFreeBuffer(&EscapedString);
	}
	else
	{
		Status = AsciiStrCatS(String, BufferSize, Att->Value);
	}
    if (EFI_ERROR(Status))
    {
      goto EXIT;
    }
    Status = AsciiStrCatS(String, BufferSize, "\"");
    if (EFI_ERROR(Status))
    {
      goto EXIT;
    }
  }

  //handle children and ending
  if ((Node->Value == NULL) && (Node->NumChildren == 0))
  {
    //Special short cut on the node  - Use empty node notation  />
    Status = AsciiStrCatS(String, BufferSize, " />");
    if (EFI_ERROR(Status))
    {
      goto EXIT;
    }
  }
  else  //longer notation
  {
    Status = AsciiStrCatS(String, BufferSize, ">");
    if (EFI_ERROR(Status))
    {
      goto EXIT;
    }

    //Show Value if value
    if (Node->Value != NULL)
    {
		if (Escaped)
		{
			CHAR8 *EscapedString = NULL;
			Status = XmlEscape(Node->Value, XML_MAX_ELEMENT_VALUE_LENGTH, &EscapedString);
			if (EFI_ERROR(Status))
			{
				goto EXIT;
			}
			Status = AsciiStrCatS(String, BufferSize, EscapedString);
			SafeFreeBuffer(&EscapedString);
		}
		else
		{
			Status = AsciiStrCatS(String, BufferSize, Node->Value);
		}
      if (EFI_ERROR(Status))
      {
        goto EXIT;
      }
    }

    // Process all children
    if (Node->NumChildren > 0)
    {
      UINTN child = 0;  //use for debugging only
      //loop children
      for (Link = Node->ChildrenListHead.ForwardLink; Link != &(Node->ChildrenListHead); Link = Link->ForwardLink, child++)
      {
        Status = _ToStringRecursively((CONST XmlNode*)Link, BufferSize, String, Level+1, Escaped);
        if (EFI_ERROR(Status))
        {
          DEBUG((DEBUG_ERROR, "%a - Error Status from child index %d of element: %a\n", __FUNCTION__, child, Node->Name));
          goto EXIT;
        }
      }
    } //end children loop

    Status = AsciiStrCatS(String, BufferSize, "</");
    if (EFI_ERROR(Status))
    {
      goto EXIT;
    }
    Status = AsciiStrCatS(String, BufferSize, Node->Name);
    if (EFI_ERROR(Status))
    {
      goto EXIT;
    }
    Status = AsciiStrCatS(String, BufferSize, ">");
    if (EFI_ERROR(Status))
    {
      goto EXIT;
    }
  } //end of node

EXIT:
  return Status;
}

/**
Public function to create an ascii string from an xml node tree.
This will use shortened XML notation and no whitespace.  (ideal for data transfer)

@param[in]  Node - Root node or first node to start printing.
@param      Escaped - Should the Xml be escaped.  Generally this should be true
@param[out] BufferSize - Number of bytes that the string needed. Includes Null terminator
@param[out] String - Pointer to pointer of ASCII buffer.  Will be set for return to caller.  Needs to be freed using FreePool when finished
**/
EFI_STATUS
EFIAPI
XmlTreeToString(
  IN  CONST XmlNode    *Node,
  IN        BOOLEAN    Escaped,
  OUT       UINTN      *BufferSize,
  OUT       CHAR8      **String
  )
{
  EFI_STATUS Status;
  UINTN Size = 0;
  CHAR8* XmlString = NULL;
  if ((Node == NULL) || (BufferSize == NULL) || (String == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  if (Node->ParentNode != NULL)
  {
    DEBUG((DEBUG_WARN, "%a - Called with node other than root node.  Siblings will not be traversed.\n", __FUNCTION__));
  }

  Status = CalculateXmlDocSize(Node, Escaped, &Size);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Error in CalculateXmlDocSize %r\n", __FUNCTION__, Status));
    return Status;
  }
  Size++;  //for the NULL char
  DEBUG((DEBUG_INFO, "%a - Pre Calculated Size of string is 0x%X\n", __FUNCTION__, Size));

  XmlString = (CHAR8*)AllocatePool(Size);
  if (XmlString == NULL)
  {
    DEBUG((DEBUG_ERROR, "Failed to allocate string for XML"));
    return EFI_OUT_OF_RESOURCES;
  }
  *XmlString = '\0';  //set to null for ascii str cat
  Status = _ToStringRecursively(Node, Size, XmlString, 0, Escaped);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to convert xml node tree into string. %r\n", Status));
    return Status;
  }

  DEBUG((DEBUG_INFO, "%a - Pre Calculated Length of string is 0x%X. AsciiStrLen is 0x%X\n", __FUNCTION__, (Size - 1), AsciiStrLen(XmlString)));

  *String = XmlString;
  *BufferSize = Size;
  return EFI_SUCCESS;
}


/**
Engine parsing code which will build a XmlNode for the XmlTree

*Internal Function

@param[in] XmlDocument      -- XML document.
@param[in] XmlDocumentSize  -- Size of the XML document.
@param[in out] Root         -- Pointer to recieve the node list.

Return Value:

EFI_SUCCESS or underlying failure code.

**/
EFI_STATUS
EFIAPI
BuildNodeList(
  IN CONST CHAR8*      XmlDocument,
  IN       UINTN       XmlDocumentSize,
  IN OUT   XmlNode**   Root
)
{
  EFI_STATUS             Status = EFI_INVALID_PARAMETER;
  UINTN                  EncodingLength = 0;
  XmlNode*               CurrentNode = NULL;
  CHAR8*                 XmlDeclaration = NULL;
  CHAR8*                 StartDoc = NULL;
  UINT64                 ProcessedCharacters = 0;
  BOOLEAN                ProcessedNode = FALSE;

  XML_TOKENIZATION_STATE State;
  XML_TOKENIZATION_INIT  Init;
  XML_LINE_AND_COLUMN    Location;
  CHAR8                  Element[MAX_PATH];
  CHAR8                  HyperSpace[MAX_PATH];
  CHAR8                  EndElement[MAX_PATH];
  CHAR8                  AttributeName[MAX_PATH];
  CHAR8                  AttributeValue[MAX_PATH];

  //
  // Zero everthing out to start.
  //   
  ZeroMem(&State, sizeof(State));
  ZeroMem(&Init, sizeof(Init));
  ZeroMem(&Location, sizeof(Location));
  ZeroMem(Element, ARRAYSIZE(Element));
  ZeroMem(HyperSpace, ARRAYSIZE(HyperSpace));
  ZeroMem(EndElement, ARRAYSIZE(EndElement));
  ZeroMem(AttributeName, ARRAYSIZE(AttributeName));
  ZeroMem(AttributeValue, ARRAYSIZE(AttributeValue));

  //
  // Initialize the XML engine.
  //
  Init.Size = sizeof(Init);
  Init.XmlData = (VOID*)XmlDocument;
  Init.XmlDataSize = (UINT32)XmlDocumentSize;
  Init.SupportPosition = TRUE;

  if (XmlDocument == NULL || XmlDocumentSize == 0 || Root == NULL)
  {
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }
  *Root = NULL;

  //
  // Start by initializing the tokenizer with our data.  Note that we don't
  // pass along the optional "special string" and normal comparison functions,
  // as the XML parser has its own implementation.
  //
  Status = RtlXmlInitializeTokenization(&State, &Init);
  if (EFI_ERROR(Status))
  {
    DEBUG((EFI_D_ERROR, "Failed to initialize tokenization\n"));
    goto Exit;
  }

  //
  // Always determine the encoding of an xml stream.  This should be done before
  // the first "next" call on the tokenizer to make sure the correct character
  // decoder is selected.
  //
  Status = RtlXmlDetermineStreamEncoding(&State, &EncodingLength);
  if (EFI_ERROR(Status))
  {
    DEBUG((EFI_D_ERROR, "Failed to determine encoding type\n"));
    goto Exit;
  }

  //
  // Finding the encoding may have adjusted the real pointer for the top of the
  // document to skip past the BOM
  //
  State.RawTokenState.pvCursor = (VOID*)(((UINTN)State.RawTokenState.pvCursor) + EncodingLength);

  do
  {
    XML_TOKEN Next;

    //
    // Acquire the next token from the parser.
    //
    Status = RtlXmlNextToken(&State, &Next, FALSE);
    if (EFI_ERROR(Status))
    {
      DEBUG((EFI_D_ERROR, "Failed to get the next token, Status = 0x%x\n", Status));
      goto Exit;
    }
    else if (Next.fError)
    {
      //
      // Errors in parse, such as bad characters, come out here in the
      // fError member
      //
      DEBUG((EFI_D_ERROR, "Error during tokenization, Status = 0x%x\n", Next.fError));
      goto Exit;
    }


    if (StartDoc == NULL)
    {
      StartDoc = (CHAR8*)Next.Run.pvData;
    }

    ProcessedCharacters += Next.Run.cbData;
    if (ProcessedCharacters >= XmlDocumentSize)
    {
      DEBUG((DEBUG_VERBOSE, "Reached the specified number of characters, ending...\n"));
      if (ProcessedNode == FALSE)
      {
        DEBUG((EFI_D_ERROR, "ERROR:  We reached the end, and no nodes were created.\n"));
        Status = EFI_INVALID_PARAMETER;
      }
      else
      {
        Status = EFI_SUCCESS;
      }
      goto Exit;
    }

    //
    // Get the current location.
    //
    Status = RtlXmlGetCurrentLocation(&State, &Location);
    if (EFI_ERROR(Status))
    {
      DEBUG((EFI_D_ERROR, "Failed to get location information.\n"));
    }

    if (Next.Run.pvData == NULL)
    {
      DEBUG((EFI_D_ERROR, "ERROR:  Next.Run.pvData == NULL\n"));
      Status = EFI_INVALID_PARAMETER;
      goto Exit;
    }
    //
    // Pick off the XML declaration if there is one.
    //
    if (Next.State == XTSS_XMLDECL_CLOSE)
    {
      const UINTN EndlineSize = 2;
      XmlDeclaration = AllocateZeroPool(State.Location.Column + EndlineSize);
      if (XmlDeclaration == NULL)
      {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
      }

      Status = AsciiStrnCpyS(XmlDeclaration, State.Location.Column + EndlineSize, StartDoc, State.Location.Column + 1);
      if (EFI_ERROR(Status))
      {
        goto Exit;
      }
    }
    else if (Next.State == XTSS_ELEMENT_NAME)
    {
      //
      // We found an element name, so create a new node for it.
      //
      Status = AsciiStrnCpyS(Element,  ARRAYSIZE(Element), (CHAR8*)Next.Run.pvData,  (UINTN)Next.Run.ulCharacters);
      if (EFI_ERROR(Status))
      {
        DEBUG((EFI_D_ERROR, "ERROR, AsciiStrnCpyS() failed\n"));
        goto Exit;
      }
      DEBUG((DEBUG_VERBOSE, "New, adding node: '%a'\n", Element));

      if (*Root == NULL)
      {
        //
        // This is the root node.
        //
        Status = AddNode(NULL, Element, NULL, Root);
        if (EFI_ERROR(Status))
        {
          goto Exit;
        }

        //
        // Add the declaration if we had one.
        //
        (*Root)->XmlDeclaration.Declaration = XmlDeclaration;

        CurrentNode = *Root;
      }
      else
      {
        Status = AddNode(CurrentNode, Element, NULL, &CurrentNode);
        if (EFI_ERROR(Status))
        {
          goto Exit;
        }
      }

      //
      // Mark that we have successfully added a new node, so we can check
      // for valid XML when the end of the document is reached.
      //
      ProcessedNode = TRUE;
    }
    else if (Next.State == XTSS_STREAM_HYPERSPACE)
    {
      BOOLEAN NotWhiteSpace = FALSE;
      Status = AsciiStrnCpyS(HyperSpace, ARRAYSIZE(HyperSpace), (CHAR8*)Next.Run.pvData, (UINTN)Next.Run.ulCharacters);
      if (EFI_ERROR(Status))
      {
        goto Exit;
      }

      //
      // See if we have a value.
      //
      for (UINT32 i = 0; i < Next.Run.ulCharacters; i++)
      {
        if (!IsWhiteSpace(HyperSpace[i]))
        {
          NotWhiteSpace = TRUE;
          break;
        }
      }

      if (NotWhiteSpace)
      {
        //
        // Allocate memory for the value.  This will be cleaned up when the
        // node is deleted when FreeXmlTree() is called.
        //
        CHAR8* Value = AllocateZeroPool((UINTN)Next.Run.ulCharacters + 1);
        if (Value == NULL)
        {
          Status = EFI_OUT_OF_RESOURCES;
          goto Exit;
        }

        DEBUG((DEBUG_VERBOSE, "Found value %a\n", HyperSpace));
        Status = AsciiStrCpyS(Value, (UINTN)Next.Run.ulCharacters + 1, HyperSpace);
        if (EFI_ERROR(Status))
        {
          goto Exit;
        }

        if (CurrentNode)
        {
          CurrentNode->Value = Value;
        }
      }
    }
    else if (Next.State == XTSS_ELEMENT_ATTRIBUTE_NAME)
    {
      //
      // We found an attribute name, so buffer it so that it is available
      // once we get the attribute value.
      //
      Status = AsciiStrnCpyS(AttributeName, ARRAYSIZE(AttributeName), (CHAR8*)Next.Run.pvData, (UINTN)Next.Run.ulCharacters);
      if (EFI_ERROR(Status))
      {
        goto Exit;
      }
      DEBUG((DEBUG_VERBOSE, "Found attribute name: '%a'\n", AttributeName));

    }
    else if (Next.State == XTSS_ELEMENT_ATTRIBUTE_VALUE)
    {
      //
      // We received the attribute value, so we can now add the name 
      // and value to the current node.
      //
      Status = AsciiStrnCpyS(AttributeValue, ARRAYSIZE(AttributeValue), (CHAR8*)Next.Run.pvData, (UINTN)Next.Run.ulCharacters);
      if (EFI_ERROR(Status))
      {
        goto Exit;
      }
      DEBUG((DEBUG_VERBOSE, "Found attribute Value: '%a'\n", AttributeValue));

      //
      // Now add the attribute to the current node...
      //
      Status = AddAttributeToNode(CurrentNode, AttributeName, AttributeValue);
      if (EFI_ERROR(Status))
      {
        DEBUG((EFI_D_ERROR, "ERROR:  AddAttributeToNode() failed, Status = 0x%x\n", Status));
        goto Exit;
      }
    }
    else if (Next.State == XTSS_ENDELEMENT_NAME)
    {
      Status = AsciiStrnCpyS(EndElement, ARRAYSIZE(EndElement), (CHAR8*)Next.Run.pvData, (UINTN)Next.Run.ulCharacters);
      if (EFI_ERROR(Status))
      {
        goto Exit;
      }

      DEBUG((DEBUG_VERBOSE, "XTSS_ENDELEMENT_NAME, %a\n", EndElement));

      //
      // If szEndElement is not equal to pCurrentNode->pszName,
      // we were given invalid XML, so we should fail.
      //
      if (CurrentNode)
      {
        if (AsciiStrCmp(EndElement, CurrentNode->Name) != 0)
        {
          DEBUG((EFI_D_ERROR, "ERROR:  Ending element does not match current node CurrentElement: '%a', CurrentNode: '%a'\n",
            EndElement, CurrentNode->Name));
          Status = EFI_INVALID_PARAMETER;
          goto Exit;
        }
      }


      //
      // We have reached the end of an element, so move the current
      // node up to the parent.
      //
      if (CurrentNode)
      {
        CurrentNode = CurrentNode->ParentNode;
      }

    }
    else if (Next.State == XTSS_ELEMENT_CLOSE_EMPTY)
    {
      DEBUG((DEBUG_VERBOSE,"XTSS_ELEMENT_CLOSE_EMPTY, empty close\n"));

      //
      // We have reached the end of an element, so move the current
      // node up to the parent.
      //
      if (CurrentNode)
      {
        CurrentNode = CurrentNode->ParentNode;
      }
    }

    //
    // Advance to the next token within the document...
    //
    Status = RtlXmlAdvanceTokenization(&State, &Next);
    if (EFI_ERROR(Status))
    {
      DEBUG((EFI_D_ERROR, "Failed to advance tokenization\n"));
      goto Exit;
    }

    //
    // Parsing is finished when you recieve the stream-end state.
    //
    if (Next.State == XTSS_STREAM_END)
    {
      DEBUG((DEBUG_VERBOSE, "At the end of the document\n"));
      break;
    }

  } while (TRUE);

Exit:
  if (EFI_ERROR(Status))
  {
    //In error state clean up after ourselves
    // the api makes it clear if parsing fails
    // no xml tree is to be returned
    if (*Root != NULL)
    {
      FreeXmlTree(Root);
    }
  }

  return Status;
}//BuildNodeList()


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
)
{
  EFI_STATUS  Status = EFI_SUCCESS;

  if (XmlDocument == NULL || SizeXmlDocument == 0 || RootNode == NULL)
  {
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  Status = BuildNodeList(XmlDocument, SizeXmlDocument, RootNode);
  if (EFI_ERROR(Status))
  {
    goto Exit;
  }
Exit:
  return Status;
}



 /**
 This function will free all of the resources allocated for an XML Tree.

 @param   RootNode -- The root node that contains the node list.

 @return  EFI_SUCCESS or underlying failure code.  
 On successfuly return RootNode Ptr will be NULL

 **/
EFI_STATUS
EFIAPI
FreeXmlTree(
  IN XmlNode** RootNode
)
{
  EFI_STATUS  Status = EFI_SUCCESS;
  if (RootNode == NULL)
  {
    return EFI_INVALID_PARAMETER;
  }

  if (*RootNode == NULL)
  {
    return EFI_INVALID_PARAMETER;
  }

  Status = DeleteNode(*RootNode);
  SafeFreeBuffer((CHAR8**)RootNode);

  return Status;
}// FreeXmlTree()




/**
This function uses DEBUG to print the Xml Tree

  @param[in] Node  - Pointer to head xml element to print
  @param[in] Level - IndentLevel to make printing pretty.
  Each child element will be indented.
**/
VOID
EFIAPI
DebugPrintXmlTree(
  IN CONST XmlNode *Node,
  IN UINTN Level)
{
  XmlAttribute *Att = NULL;
  LIST_ENTRY *Link = NULL;
  CHAR8 Indent[11];
  UINTN Index = Level;


  if (Node == NULL)
  {
    return;
  }

  SetMem(Indent, sizeof(Indent) - 1, ' ');
  Indent[0] = '_';
  if (Index > 19)
  {
    Index = 20;
  }
  Indent[Index] = '\0';

  if (Node->XmlDeclaration.Declaration != NULL)
  {
    if (Node->ParentNode != NULL)
    {
      DEBUG((DEBUG_ERROR, "!!!ERROR: BAD XML.  Should not have xmlDeclaration for a non-root node\n"));
    }
    DEBUG((DEBUG_INFO, "%a\n", Node->XmlDeclaration.Declaration));
  }

  DEBUG((DEBUG_INFO, "%a", Indent));
  //start tag
  DEBUG((DEBUG_INFO, "<%a", Node->Name));

  //Loop attributes
  for (Link = Node->AttributesListHead.ForwardLink; Link != &(Node->AttributesListHead); Link = Link->ForwardLink)
  {
    Att = (XmlAttribute*)Link;
    DEBUG((DEBUG_INFO, " %a=\"%a\"", Att->Name, Att->Value));
  }

  if ((Node->Value == NULL) && (Node->NumChildren == 0))
  {
    //Special short cut on the node  - Use empty node notation
    DEBUG((DEBUG_INFO, " />\n"));
  }
  else
  {
    DEBUG((DEBUG_INFO, ">"));

    //Show Value if value
    if (Node->Value != NULL)
    {
      DEBUG((DEBUG_INFO, "%a", Node->Value));
    }
    if (Node->NumChildren > 0)
    {
      DEBUG((DEBUG_INFO, "\n"));  //Start children on new line
      //loop children
      for (Link = Node->ChildrenListHead.ForwardLink; Link != &(Node->ChildrenListHead); Link = Link->ForwardLink)
      {
        DebugPrintXmlTree((XmlNode*)Link, Level + 1);
      }
      DEBUG((DEBUG_INFO, "%a", Indent));
    }

    //end tag
    DEBUG((DEBUG_INFO, "</%a>\n", Node->Name));
  }
  return;
}

UINTN
_GetXmlUnEscapedLength(
  IN CONST CHAR8* String,
  IN UINTN MaxStringLength
)
{
  UINTN Len = 0;
  UINTN i=0;

  Len = AsciiStrnLenS(String, MaxStringLength + 1);

  if (Len > MaxStringLength)
  {
    DEBUG((DEBUG_ERROR, __FUNCTION__ " String is too big or not NULL terminated.  MaxLen = 0x%LX\n", (UINT64)MaxStringLength));
    ASSERT(Len <= MaxStringLength);
    return 0;
  }

  //String is good null terminated string. 
  // loop thru all chars looking for chars that are escaped.  
  // When found subtract the size of escape sequence
  while (String[i] != '\0')
  {
    if (String[i] == '&')
    {
      //Must use a string compare with length because
      // string is not null terminated because we are 
      // in the middle of the string.  
      if (AsciiStrnCmp(&String[i + 1], "lt;", 3) == 0)
      {
        Len -= 3;
        i += 3;
      }
      else if (AsciiStrnCmp(&String[i + 1], "gt;",3) == 0)
      {
        Len -= 3;
        i += 3;
      }
      else if (AsciiStrnCmp(&String[i + 1], "quot;", 5) == 0)
      {
        Len -= 5;
        i += 5;
      }
      else if (AsciiStrnCmp(&String[i + 1], "apos;", 5) == 0)
      {
        Len -= 5;
        i += 5;
      }
      else if (AsciiStrnCmp(&String[i + 1], "amp;", 4) == 0)
      {
        Len -= 4;
        i += 4;
      }
      else
      {
        DEBUG((DEBUG_INFO, __FUNCTION__ " found an & char that is not valid xml escape sequence\n"));
      }
    }
    i++; 
  }
  return Len;
}


/**
return Length after escaping.  This length does not include null terminator
**/
UINTN
_GetXmlEscapedLength(
  IN CONST CHAR8* String,
  IN UINTN MaxStringLength
)
{
  UINTN Len = 0;
  UINTN i=0;

  Len = AsciiStrnLenS(String, MaxStringLength + 1);

  if (Len > MaxStringLength)
  {
    DEBUG((DEBUG_ERROR, __FUNCTION__ " String is too big or not NULL terminated\n"));
    ASSERT(Len <= MaxStringLength);
    return 0;
  }

  //String is good null terminated string. 
  // loop thru all chars looking for chars that need
  // to be escaped.  When found add the additional length
  // of the escape sequence
  while(String[i] != '\0')
  { 
    switch (String[i++])
    {
      case '<':  // &lt;
      case '>':  // &gt;
        Len += 3;
        break;

      case '\"': //&quot;
      case '\'': //&apos;
        Len += 5;
        break;

      case '&': //&amp;
        Len += 4;
        break;

      default:
        break;
    }
  }
  return Len;
}


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
  OUT CHAR8**     EscapedString)
{
  UINTN EscapedLength = 0;
  CHAR8* EString = NULL;  //local copy of the escaped string
  UINTN i = 0;
  UINTN j = 0;

  if (EscapedString == NULL)
  {
    return EFI_INVALID_PARAMETER;
  }

  EscapedLength = _GetXmlEscapedLength(String, MaxStringLength);
  if (EscapedLength == 0)
  {
    DEBUG((DEBUG_ERROR, __FUNCTION__ " failed to get valid escaped length\n"));
    return EFI_INVALID_PARAMETER;
  }

  EString = AllocateZeroPool(EscapedLength +1 ); //add one for NULL termination
  if (EString == NULL)
  {
    return EFI_OUT_OF_RESOURCES;
  }

  //Now traverse the String and escape chars
  while ((String[i] != '\0') && (j < EscapedLength))
  {
    switch (String[i]) {
      case '<': //&lt;
        if (j + 4 > EscapedLength)
        {
          DEBUG((DEBUG_ERROR, __FUNCTION__ " invalid parsing <.  J+4 will cause overflow\n"));
          j = EscapedLength; //cause error to force break without corrupting memory 
          break;
        }
        EString[j++] = '&';
        EString[j++] = 'l';
        EString[j++] = 't';
        EString[j++] = ';';
        break;

      case '>': //&gt;
        if (j + 4 > EscapedLength)
        {
          DEBUG((DEBUG_ERROR, __FUNCTION__ " invalid parsing >.  J + 4 will cause overflow\n"));
          j = EscapedLength + 1; //cause error to force break without corrupting memory 
          break;
        }
        EString[j++] = '&';
        EString[j++] = 'g';
        EString[j++] = 't';
        EString[j++] = ';';
        break;

      case '\"': //&quot;
        if (j + 6 > EscapedLength)
        {
          DEBUG((DEBUG_ERROR, __FUNCTION__ " invalid parsing \".  J+6 will cause overflow\n"));
          j = EscapedLength +1 ; //cause error to force break without corrupting memory 
          break;
        }
        EString[j++] = '&';
        EString[j++] = 'q';
        EString[j++] = 'u';
        EString[j++] = 'o';
        EString[j++] = 't';
        EString[j++] = ';';
        break;
      
      case '\'': //&apos;
        if (j + 6 > EscapedLength)
        {
          DEBUG((DEBUG_ERROR, __FUNCTION__ " invalid parsing '.  J+6 will cause overflow\n"));
          j = EscapedLength; //cause error to force break without corrupting memory 
          break;
        }
        EString[j++] = '&';
        EString[j++] = 'a';
        EString[j++] = 'p';
        EString[j++] = 'o';
        EString[j++] = 's';
        EString[j++] = ';';
        break;

      case '&': //&amp;
        if (j + 5 > EscapedLength)
        {
          DEBUG((DEBUG_ERROR, __FUNCTION__ " invalid parsing &.  J+5 will cause overflow\n"));
          j = EscapedLength +1 ; //cause error to force break without corrupting memory 
          break;
        }
        EString[j++] = '&';
        EString[j++] = 'a';
        EString[j++] = 'm';
        EString[j++] = 'p';
        EString[j++] = ';';
        break;

      default:  //char that doesn't require escaping
        EString[j++] = String[i];
    }  //switch
    i++;
  } //while

  //check for errors:
  if ((j != EscapedLength) || (String[i] != '\0') )
  {
    //report unique messages for easier debug
    if (j != EscapedLength) 
    {
      DEBUG((DEBUG_ERROR, __FUNCTION__ " escape string process failed.  New String index counter (j = %d EscapedLength = %d) not at end point\n", j, EscapedLength));
      ASSERT(j == EscapedLength);
    }
    else
    {
      DEBUG((DEBUG_ERROR, __FUNCTION__ " escape string process failed.  Input String index counter (i = %d String[i] = %c) not at NULL terminator\n", i, (CHAR16)String[i]));
      ASSERT(String[i] == '\0');
    }

    FreePool(EString);
    return EFI_DEVICE_ERROR;
  }


  EString[j] = '\0';  //null terminate

  *EscapedString = EString;
  return EFI_SUCCESS;
}

/**
Remove XML escape sequences in the string so strings are valid for string operations

@param EscapedString - Xml Escaped Ascii string
@param MaxStringLength - Max length of the Ascii string "EscapedString"
@param String - resulting escaped string if return value is success.  Memory must be freed by caller

@return Status of un escape process.  ON success String * will point to string that contains no XML escape sequences.
**/
EFI_STATUS
EFIAPI
XmlUnEscape(
  IN CONST CHAR8* EscapedString,
  IN UINTN        MaxEscapedStringLength,
  OUT CHAR8**     String
)
{
  UINTN Length = 0;
  CHAR8* RawString = NULL;  //local copy of the raw string
  UINTN i, j;

  i = 0;
  j = 0;


  if (String == NULL)
  {
    return EFI_INVALID_PARAMETER;
  }

  Length = _GetXmlUnEscapedLength(EscapedString, MaxEscapedStringLength);
  if (Length == 0)
  {
    DEBUG((DEBUG_ERROR, __FUNCTION__ " failed to get valid unescaped length\n"));
    return EFI_INVALID_PARAMETER;
  }

  RawString = AllocateZeroPool(Length + 1); //add one for NULL termination
  if (RawString == NULL)
  {
    return EFI_OUT_OF_RESOURCES;
  }

  //Now traverse the String and Unescape chars
  while ((EscapedString[i] != '\0') && (j < Length))
  {
    if (EscapedString[i] == '&')
    {
      if (AsciiStrnCmp(&EscapedString[i + 1], "lt;", 3) == 0)
      {
		RawString[j++] = '<';
        i += 4;
      }
      else if (AsciiStrnCmp(&EscapedString[i + 1], "gt;", 3) == 0)
      {
		RawString[j++] = '>';
        i += 4;
      }
      else if (AsciiStrnCmp(&EscapedString[i + 1], "quot;", 5) == 0)
      {
		RawString[j++] = '"';
        i += 6;
      }
      else if (AsciiStrnCmp(&EscapedString[i + 1], "apos;", 5) == 0)
      {
		RawString[j++] = '\'';
        i += 6;
      }
      else if (AsciiStrnCmp(&EscapedString[i + 1], "amp;", 4) == 0)
      {
		RawString[j++] = '&';
        i += 5;
      }
      else
      {
        DEBUG((DEBUG_INFO, __FUNCTION__ " found an & char that is not valid xml escape sequence\n"));
      }
    }
	else  //not an escape character
	{
		RawString[j++] = EscapedString[i++];
	}
  } //while

    //check for errors:
  if ((j != Length) || (EscapedString[i] != '\0'))
  {
    //report unique messages for easier debug
    if (j != Length)
    {
      DEBUG((DEBUG_ERROR, __FUNCTION__ " unescape string process failed.  New String index counter (j = %d Length = %d) not at end point\n", j, Length));
      ASSERT(j == Length);
    }
    else
    {
      DEBUG((DEBUG_ERROR, __FUNCTION__ " unescape string process failed.  Input String index counter (i = %d EscapedString[i] = %c) not at NULL terminator\n", i, (CHAR16)EscapedString[i]));
      ASSERT(EscapedString[i] == '\0');
    }

    FreePool(RawString);
    return EFI_DEVICE_ERROR;
  }


  RawString[j] = '\0';  //null terminate

  *String = RawString;
  return EFI_SUCCESS;
}

/**
Function to go thru a xml tree and count the nodes
**/
EFI_STATUS
EFIAPI
XmlTreeNumberOfNodes(
  IN  CONST XmlNode* Node,
  IN OUT    UINTN* Count)
{
  LIST_ENTRY *Link = NULL;
  EFI_STATUS Status = EFI_SUCCESS;
  UINTN Child = 0;

  if ((Node == NULL) || (Count == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  for (Link = GetFirstNode(&Node->ChildrenListHead);
    !IsNull(&Node->ChildrenListHead, Link);
    Link = GetNextNode(&Node->ChildrenListHead, Link), Child++)
  {
    Status = XmlTreeNumberOfNodes((CONST XmlNode*)Link, Count);
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "%a - Error Status (%r) from child element index %d of %a\n", __FUNCTION__, Status, Child, Node->Name));
      return Status;
    }
  }
  (*Count)++;

  return EFI_SUCCESS;
}

/**
Function to go thru a xml tree and report the max depth

**/
EFI_STATUS
EFIAPI
XmlTreeMaxDepth(
  IN  CONST XmlNode* Node,
  OUT       UINTN* Depth)
{
  LIST_ENTRY *Link = NULL;
  EFI_STATUS Status = EFI_SUCCESS;
  UINTN MDepth = 0;
  UINTN TestDepth = 0;
  UINTN Child = 0;

  if ((Node == NULL) || (Depth == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  for (Link = GetFirstNode(&Node->ChildrenListHead);
    !IsNull(&Node->ChildrenListHead, Link);
    Link = GetNextNode(&Node->ChildrenListHead, Link), Child++)
  {
    TestDepth = 0;
    Status = XmlTreeMaxDepth((CONST XmlNode*)Link, &TestDepth);
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "%a - Error Status (%r) from child element index %d of %a\n", __FUNCTION__, Status, Child, Node->Name));
      return Status;
    }
    //check if this childs depth is the largest of the children
    if (TestDepth > MDepth)
    {
      MDepth = TestDepth;
    }
  }
  *Depth = (*Depth) + 1 + MDepth;

  return EFI_SUCCESS;
}

/**
Function to go thru a tree and count the attributes
**/
EFI_STATUS
EFIAPI
XmlTreeNumberOfAttributes(
  IN  CONST XmlNode* Node,
  OUT       UINTN* Count
)
{
  LIST_ENTRY *Link = NULL;
  EFI_STATUS Status = EFI_SUCCESS;
  UINTN Child = 0;

  if ((Node == NULL) || (Count == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  //count attributes
  for (Link = GetFirstNode(&Node->AttributesListHead);
    !IsNull(&Node->AttributesListHead, Link);
    Link = GetNextNode(&Node->AttributesListHead, Link))
  {
    (*Count)++;
  }

  //call for children
  for (Link = GetFirstNode(&Node->ChildrenListHead);
    !IsNull(&Node->ChildrenListHead, Link);
    Link = GetNextNode(&Node->ChildrenListHead, Link), Child++)
  {
    Status = XmlTreeNumberOfAttributes((CONST XmlNode*)Link, Count);
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "%a - Error Status (%r) from child element index %d of %a\n", __FUNCTION__, Status, Child, Node->Name));
      return Status;
    }
  }

  return EFI_SUCCESS;
}


/**
Function to go thru a tree and report the max number of attributes on any element

**/
EFI_STATUS
EFIAPI
XmlTreeMaxAttributes(
  IN  CONST XmlNode* Node,
  IN OUT    UINTN* MaxAttributes
)
{
  LIST_ENTRY *Link = NULL;
  EFI_STATUS Status = EFI_SUCCESS;
  UINTN LocalAttributeCount = 0;
  UINTN Child = 0;

  if ((Node == NULL) || (MaxAttributes == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  //count attributes of this node
  for (Link = GetFirstNode(&Node->AttributesListHead);
    !IsNull(&Node->AttributesListHead, Link);
    Link = GetNextNode(&Node->AttributesListHead, Link))
  {
    LocalAttributeCount++;
  }

  //
  // Check if our attribute count is larger than current max
  //
  if (LocalAttributeCount > *MaxAttributes)
  {
    *MaxAttributes = LocalAttributeCount;
  }

  //
  //Loop thru children recursively 
  //
  for (Link = GetFirstNode(&Node->ChildrenListHead);
    !IsNull(&Node->ChildrenListHead, Link);
    Link = GetNextNode(&Node->ChildrenListHead, Link), Child++)
  {
    Status = XmlTreeMaxAttributes((CONST XmlNode*)Link, MaxAttributes);
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "%a - Error Status (%r) from child element index %d of %a\n", __FUNCTION__, Status, Child, Node->Name));
      return Status;
    }
  }

  return EFI_SUCCESS;
}
