/** @file
XmlTreeQueryLib

This library supports generic XML queries based on the XmlTreeLib.

Copyright (C) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <XmlTypes.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <XmlTypes.h>
#include <Library/XmlTreeLib.h>
#include <Library/XmlTreeQueryLib.h>

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
  )
{
  LIST_ENTRY  *Link = NULL;

  if (ParentNode == NULL) {
    DEBUG ((DEBUG_ERROR, "%a - Parent Node is NULL\n", __FUNCTION__));
    ASSERT (ParentNode != NULL);
    return NULL;
  }

  if (ElementName == NULL) {
    DEBUG ((DEBUG_ERROR, "%a - Element Name is NULL\n", __FUNCTION__));
    ASSERT (ElementName != NULL);
    return NULL;
  }

  DEBUG ((DEBUG_INFO, "%a - Looking for '%a;\n", __FUNCTION__, ElementName));
  DEBUG ((DEBUG_INFO, "%a - Looking in children of '%a\n", __FUNCTION__, ParentNode->Name));

  for (Link = GetFirstNode (&ParentNode->ChildrenListHead);
       !IsNull (&ParentNode->ChildrenListHead, Link);
       Link = GetNextNode (&ParentNode->ChildrenListHead, Link))
  {
    XmlNode  *NodeThis = (XmlNode *)Link;
    DEBUG ((DEBUG_INFO, "Checking Node: ElementName = '%a'\n", NodeThis->Name));
    if (AsciiStrnCmp (ElementName, NodeThis->Name, MAX_ELEMENT_NAME_LENGTH) == 0) {
      // Found it
      DEBUG ((DEBUG_INFO, "Found element\n"));
      return NodeThis;
    }
  }

  DEBUG ((DEBUG_INFO, "Didn't find element named %a\n", ElementName));
  return NULL;
}

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
  )
{
  LIST_ENTRY  *Link = NULL;

  if (Node == NULL) {
    DEBUG ((DEBUG_ERROR, "%a - Node is NULL\n", __FUNCTION__));
    ASSERT (Node != NULL);
    return NULL;
  }

  if (AttributeName == NULL) {
    DEBUG ((DEBUG_ERROR, "%a - Attribute Name is NULL\n", __FUNCTION__));
    ASSERT (AttributeName != NULL);
    return NULL;
  }

  DEBUG ((DEBUG_INFO, "%a - Looking for attribute with name '%a'\n", __FUNCTION__, AttributeName));
  DEBUG ((DEBUG_INFO, "%a - Looking in attributes of node '%a'\n", __FUNCTION__, Node->Name));

  for (Link = GetFirstNode (&Node->AttributesListHead);
       !IsNull (&Node->AttributesListHead, Link);
       Link = GetNextNode (&Node->AttributesListHead, Link))
  {
    XmlAttribute  *AttrThis = (XmlAttribute *)Link;
    DEBUG ((DEBUG_INFO, "Checking Attribute: Name = '%a'\n", AttrThis->Name));
    if (AsciiStrnCmp (AttributeName, AttrThis->Name, MAX_ATTRIBUTE_NAME_LENGTH) == 0) {
      // Found it
      DEBUG ((DEBUG_INFO, "Found Attribute\n"));
      return AttrThis;
    }
  }

  DEBUG ((DEBUG_INFO, "Didn't find Attribute named '%a'\n", AttributeName));
  return NULL;
}
