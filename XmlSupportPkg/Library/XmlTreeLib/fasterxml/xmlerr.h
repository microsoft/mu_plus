
/**
XML error code definitions.

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


#ifndef __XML_ERROR_INC__
#define __XML_ERROR_INC__

#ifndef IN_OPT
#define IN_OPT
#endif


EFI_STATUS
EFIAPI
RtlXmlReportErrorFunction(
    IN EFI_STATUS Status,
    IN UINT32 Line,
    IN UINT32 Indicator,
    IN_OPT VOID* Context
    );

#define RtlpReportXmlError(Status) RtlXmlReportErrorFunction(Status, __LINE__, 0, NULL)

//
// Define some NT_STATUS codes.
//
#ifndef STATUS_NOT_IMPLEMENTED
#define STATUS_NOT_IMPLEMENTED          (0xC0000002)
#endif // STATUS_NOT_IMPLEMENTED

#ifndef EFI_INVALID_PARAMETER
#define EFI_INVALID_PARAMETER        (0xC000000D)
#endif 

#ifndef STATUS_END_OF_FILE
#define STATUS_END_OF_FILE              (0xC0000011)
#endif 

#ifndef STATUS_DUPLICATE_NAME
#define STATUS_DUPLICATE_NAME           (0xC00000BD)
#endif 

#ifndef STATUS_INTERNAL_ERROR
#define STATUS_INTERNAL_ERROR           (0xC00000E5)
#endif 

#ifndef STATUS_ILLEGAL_CHARACTER
#define STATUS_ILLEGAL_CHARACTER        (0xC0000161)
#endif 

#ifndef STATUS_NOT_FOUND
#define STATUS_NOT_FOUND                (0xC0000225)
#endif 

#ifndef STATUS_XML_PARSE_ERROR
#define STATUS_XML_PARSE_ERROR          (0xC000A083)
#endif 

#ifndef STATUS_XML_ENCODING_MISMATCH
#define STATUS_XML_ENCODING_MISMATCH    (0xC0150021)
#endif 

#ifndef STATUS_BUFFER_TOO_SMALL
#define STATUS_BUFFER_TOO_SMALL (0xC0000023)
#endif

#ifndef NT_SUCCESS
#define NT_SUCCESS(x)           ((x) >= 0)
#endif

#ifndef ARGUMENT_PRESENT
#define ARGUMENT_PRESENT(x)     ((x) != 0)
#endif

#ifndef MAXULONG
#define MAXULONG        ((UINT32)-1)
#endif

#ifndef EFI_INVALID_PARAMETER_1
#define EFI_INVALID_PARAMETER_1 (0xC00000EF)
#endif 

#ifndef EFI_INVALID_PARAMETER_2
#define EFI_INVALID_PARAMETER_2 (0xC00000F0)
#endif

#ifndef EFI_INVALID_PARAMETER_3
#define EFI_INVALID_PARAMETER_3 (0xC00000F1)
#endif


#endif // __XML_ERROR_INC__
