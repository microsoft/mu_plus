/**
@file 
Define structure and test data for xml testing.  


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

#ifndef TEST_DATA_H
#define TEST_DATA_H

typedef struct {
  UINTN  EscapedLength;
  UINTN  NotEscapedLength;
  CHAR8* StringEscaped;         //Static xml escaped string
  CHAR8* StringNotEscaped;		//Static string raw
  CHAR8* String;				//dynamically allocated string during the test
}XmlStringParseContext;


XmlStringParseContext Context1          = { 50, 50,  "Hello There Are No Escape Sequences In This String",                                              "Hello There Are No Escape Sequences In This String", NULL };
XmlStringParseContext Context7Esc       = { 95, 66,  "Hello &lt;There&gt; Are &quot;7&quot; Escape Sequence&apos;s In This &amp;lt;  &amp;1234 String", "Hello <There> Are \"7\" Escape Sequence's In This &lt;  &1234 String", NULL };
XmlStringParseContext ContextLT         = { 72, 63,  "Hello &lt;There Are&lt; 3 Less Than Escape &lt; Sequences In This String",                        "Hello <There Are< 3 Less Than Escape < Sequences In This String", NULL };
XmlStringParseContext ContextGT         = { 75, 66,  "Hello &gt;There Are&gt; 3 Greater Than Escape &gt; Sequences In This String",                     "Hello >There Are> 3 Greater Than Escape > Sequences In This String", NULL };
XmlStringParseContext ContextQuote      = { 74, 59,  "Hello &quot;There Are&quot; 3 Quote Escape &quot; Sequences In This String",                      "Hello \"There Are\" 3 Quote Escape \" Sequences In This String", NULL };
XmlStringParseContext ContextApostrophe = { 79, 64,  "Hello &apos;There Are&apos; 3 Apostrophe Escape &apos; Sequences In This String",                 "Hello 'There Are' 3 Apostrophe Escape ' Sequences In This String", NULL };
XmlStringParseContext ContextAmp        = { 75, 63,  "Hello &amp;There Are&amp; 3 Ampersand Escape &amp; Sequences In This String",                     "Hello &There Are& 3 Ampersand Escape & Sequences In This String", NULL };



//
// Test XML parsing
//

typedef struct {
  UINTN TotalElements;
  UINTN TotalAttributes;
  UINTN MaxDepth;
  UINTN MaxAttributes;
  CONST CHAR8 *InputXmlString;
  CHAR8 *ToFreeXmlString;
  XmlNode *Node;
}XmlTestContext;

CONST CHAR8 SimpleElementsOnly[] =
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<RootNode>"
"  <Gen1Node>Gen1Node1 contents</Gen1Node>"
"  <Gen1Node>Gen1Node2 contents "
"    <Gen2Node>Gen2Node1 contents</Gen2Node>"
"  </Gen1Node>"
"  <Gen1Node>Gen1Node3 contents "
"    <Gen2Node>Gen2Node1 contents"
"      <Gen3Node>Gen3Node1 contents</Gen3Node>"
"      <Gen3Node>Gen2Node2 contents</Gen3Node>"
"    </Gen2Node>"
"    <Gen2Node>Gen2Node2 Long Contents Here Long Contents Here Long Contents Here</Gen2Node>"
"  </Gen1Node>"
"</RootNode>";

XmlTestContext SimpleElementsOnlyContext = { 9, 0, 4, 0, SimpleElementsOnly, NULL, NULL };


CONST CHAR8 SimpleElementsAttributes[] =
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<RootNode>"
"  <Gen1Node attribte1='value1' attribute2='value2'>Gen1Node1 contents</Gen1Node>"
"</RootNode>";
XmlTestContext SimpleElementsAttributesContext = {2, 2, 2, 2, SimpleElementsAttributes, NULL, NULL };


CONST CHAR8 NonEncodedXmlAttribute1[] = 
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<RootNode>"
"  <Gen1Node attribte1='value1 < value2' attribute2='value2'>Gen1Node1 contents</Gen1Node>"
"</RootNode>";

XmlTestContext NonEncodedXmlAttribute1Context = { 2, 2, 2, 2, NonEncodedXmlAttribute1, NULL, NULL };


CONST CHAR8 NonEncodedXmlContent1[] =
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<RootNode>"
"  <Gen1Node attribte1='value1 < value2' attribute2='value2'>Gen1Node1 contents < test</Gen1Node>"
"</RootNode>";

XmlTestContext NonEncodedXmlContent1Context = { 2, 2, 2, 2, NonEncodedXmlContent1, NULL, NULL };

CONST CHAR8 EncodedXmlAttribute1[] =
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<RootNode>"
"  <Gen1Node attribte1='value1 &lt; value2' attribute2='value2'>Gen1Node1 contents</Gen1Node>"
"</RootNode>";

XmlTestContext EncodedXmlAttribute1Context = { 2, 2, 2, 2, EncodedXmlAttribute1, NULL, NULL };


CONST CHAR8 EncodedXmlContent1[] =
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<RootNode>"
"  <Gen1Node attribte1='value1 < value2' attribute2='value2'>Gen1Node1 contents &alt; test</Gen1Node>"
"</RootNode>";

XmlTestContext EncodedXmlContent1Context = { 2, 2, 2, 2, EncodedXmlContent1, NULL, NULL };

#endif