/**
@file
Define structure and test data for xml testing.


Copyright (C) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

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
XmlStringParseContext ContextAmpNoEsc   = { 15, 15,  "Hello & Goodbye", "Hello & Goodbye", NULL };



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

CONST CHAR8 LongElement[] =
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<RootNode>"
"  <Gen1Node>Gen1Node1 contents</Gen1Node>"
"  <Gen1Node>Gen1Node2 contents "
"    <Gen2Node>Gen2Node1 contents</Gen2Node>"
"    <LongNodeData>"
"       MIIDrjCCApqgAwIBAgIQc0nOztwB5qNLayWxmLzFhTAJBgUrDgMCHQUAMEwxCzAJBgNVBAYTAlVTMR4wHAYDVQQKExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xHTAbBgNVBAMeFABEAEYAQwBJAF8ASABUAFQAUABTMB4XDTE4MDUwMjE1NDczMVoXDTM5MTIzMTIzNTk1OVowTDELMAkGA1UEBhMCVVMxHjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlvbjEdMBsGA1UEAx4UAEQARgBDAEkAXwBIAFQAVABQAFMwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQC50eVITBEA5akSD1yode1hAA77q8FGQcpAhc3yCD5TwmPJdFd0H/51zWAYLqOgP2cu+GhZQn0sZNT1YRZS5HXnTxMBd1GYI6fPEYY9pu4PdD+Olc1z1D2qk+ItFyBXsXDWRMYUbeHeY++cUni2815OacC055pTJrLpVbqsoavPjswT6UxHmTFZ9PJVXiYdlcSVb4r8xLxfreDhl00vG6QPU/hE16cFLpCIzsZDZ+o4YqAfeTu0W9TMxMfFGeYKAG56DqY15Q5nSo04LOY3Z8OjnaeekohCN1gRV5QlIM6hGs09pRnNC5Qb54bpsAdnNJJBM7H0pB/FIr5dH2n1XM/ZAgMBAAGjgZMwgZAwDwYDVR0TAQH/BAUwAwIBADB9BgNVHQEEdjB0gBB08GR9c43yJ6xq+3luFHgpoU4wTDELMAkGA1UEBhMCVVMxHjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlvbjEdMBsGA1UEAx4UAEQARgBDAEkAXwBIAFQAVABQAFOCEHNJzs7cAeajS2slsZi8xYUwCQYFKw4DAh0FAAOCAQEABeIS7s+wYZaWfMOOuPcOSWyTGyKBjFgm6EI6F+/JoKlUth1uSyjJb2UM6n8ZkEnTnm5crm/txHdRbG/q7ccmRhN9+LDukWq9gm9F3ciFodXDwRhDq9rDWGyXkXV4mz/rrlckBWpM4iYCrYoJsg6FL7wQLbpiFdbGbmVWIaN3Q2jsOJ7xcJtt56xYZZ1sAn4PMcX8KkoUnpqH+/+c97bEUqC8414ljng1yC2+Ja+/SHJAKFj9TefN2v0k3dW7X1woP0xG9wZy9G8CtTJSPyKbD9S0Ps+/nxUPHHbyEdfdO0wct50eN/GNzYEyMjLeMc/klsW3V+0S0j895uKjU+CPgQ=="
"    </LongNodeData>"
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
XmlTestContext LongElementContext = { 10, 0, 4, 0, LongElement, NULL, NULL };


CONST CHAR8 SimpleElementsAttributes[] =
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<RootNode>"
"  <Gen1Node attribute1='value1' attribute2='value2'>Gen1Node1 contents</Gen1Node>"
"</RootNode>";
XmlTestContext SimpleElementsAttributesContext = {2, 2, 2, 2, SimpleElementsAttributes, NULL, NULL };


CONST CHAR8 NonEncodedXmlAttribute1[] =
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<RootNode>"
"  <Gen1Node attribute1='value1 < value2' attribute2='value2'>Gen1Node1 contents</Gen1Node>"
"</RootNode>";

XmlTestContext NonEncodedXmlAttribute1Context = { 2, 2, 2, 2, NonEncodedXmlAttribute1, NULL, NULL };


CONST CHAR8 NonEncodedXmlContent1[] =
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<RootNode>"
"  <Gen1Node attribute1='value1 < value2' attribute2='value2'>Gen1Node1 contents < test</Gen1Node>"
"</RootNode>";

XmlTestContext NonEncodedXmlContent1Context = { 2, 2, 2, 2, NonEncodedXmlContent1, NULL, NULL };

CONST CHAR8 EncodedXmlAttribute1[] =
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<RootNode>"
"  <Gen1Node attribute1='value1 &lt; value2' attribute2='value2'>Gen1Node1 contents</Gen1Node>"
"</RootNode>";

XmlTestContext EncodedXmlAttribute1Context = { 2, 2, 2, 2, EncodedXmlAttribute1, NULL, NULL };


CONST CHAR8 EncodedXmlContent1[] =
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<RootNode>"
"  <Gen1Node attribute1='value1 < value2' attribute2='value2'>Gen1Node1 contents &alt; test</Gen1Node>"
"</RootNode>";

XmlTestContext EncodedXmlContent1Context = { 2, 2, 2, 2, EncodedXmlContent1, NULL, NULL };

#endif
