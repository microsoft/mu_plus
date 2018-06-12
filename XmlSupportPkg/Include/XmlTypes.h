/**
XmlTypes.h

XML structure defintions

Copyright (c) 2016, Microsoft Corporation

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
  UINTN              NumChildren;     // Number of children within chidrenListHead.
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