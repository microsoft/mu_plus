/**
Printing of XML errors.

Copyright (C) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#include <Uefi.h>                 // UEFI base types
#include "fasterxml.h"            // XML Engine
#include "xmlerr.h"               // XML Errors.
#include "xmlstructure.h"         // XML structures

EFI_STATUS
EFIAPI
RtlXmlReportErrorFunction(
    IN EFI_STATUS Status,
    IN UINT32 Line,
    IN UINT32 Indicator,
    IN_OPT VOID* Context
    )
{
    //
    // Don't do anything for now, but don't allow inlining of the call.
    //
    return Status;
}


