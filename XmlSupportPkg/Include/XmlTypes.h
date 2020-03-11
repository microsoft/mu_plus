/**
XmlTypes.h

XML structure defintions

Copyright (C) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __XML_TYPES_H__
#define __XML_TYPES_H__


typedef struct _XmlDeclaration
{
  CHAR8* Declaration;
} XmlDeclaration;

// Dev Note:  Keep the LIST_ENTRY item as the first element in all
//            of these structures, so that we can cast them to
//            the structure types.
//

typedef struct _XmlNode
{
  LIST_ENTRY         Link;              // List entry for this structure.
  LIST_ENTRY         ChildrenListHead;  // List head for the children of this node.
  LIST_ENTRY         AttributesListHead;// Optional list of attributes for this node.
  struct _XmlNode*   ParentNode;       // Optional parent for this node.
  UINTN              NumChildren;     // Number of children within childrenListHead.
  UINTN              NumAttributes;   // Number of attributes within this node.
  CHAR8*             Name;           // Name of this node.
  CHAR8*             Value;          // Optional value.
  XmlDeclaration     XmlDeclaration;    // Optional XML declaration for the node.

} XmlNode;

typedef struct _XmlAttribute
{
    LIST_ENTRY         Link;     // List entry for this structure.
    CHAR8*             Name;     // Name of the attribute.
    CHAR8*             Value;    // Value of the attribute.
    struct _XmlNode*   Parent;   // Parent node that this belongs to.

} XmlAttribute;

#endif // __XML_TYPES_H__