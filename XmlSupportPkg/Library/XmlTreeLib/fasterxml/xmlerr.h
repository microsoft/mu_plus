
/**
XML error code definitions.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

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
