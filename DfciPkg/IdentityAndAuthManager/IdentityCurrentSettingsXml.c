/** @file
IdentityCurrentSettingsXml.c

This library supports the setting the Identity Current XML

Copyright (c) 2018, Microsoft Corporation

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

#include "IdentityAndAuthManager.h"
#include <XmlTypes.h>
#include <Library/XmlTreeLib.h>
#include <Library/XmlTreeQueryLib.h>
#include <Library/DfciXmlIdentitySchemaSupportLib.h>
#include <Library/PrintLib.h>
#include <Guid/DfciPacketHeader.h>
#include <Guid/DfciIdentityAndAuthManagerVariables.h>

/**
Create an XML string from all the current Identities

**/
static
EFI_STATUS
CreateXmlStringFromCurrentIdentities(
    OUT CHAR8** XmlString,
    OUT UINTN*  StringSize
    )
{
    EFI_STATUS     Status;
    XmlNode       *List = NULL;
    XmlNode       *CurrentIdentitiesNode = NULL;
    XmlNode       *CurrentIdentitiesListNode = NULL;
    CHAR16        *Thumbprint16;
    CHAR8         *Thumbprint;
    CONST CHAR8   *Id;
    BOOLEAN        FreeThumbprint;
    UINTN          ThumbprintLen;


    if ((XmlString == NULL) || (StringSize == NULL))
    {
        return EFI_INVALID_PARAMETER;
    }

    //create basic xml

    List = New_IdentityCurrentPacketNodeList();
    if (List == NULL)
    {
        DEBUG((DEBUG_ERROR, "%a - Failed to create new Current Identities Packet List Node\n", __FUNCTION__));
        Status = EFI_ABORTED;
        goto EXIT;
    }

    //Get IdentitiesPacket Node
    CurrentIdentitiesNode = GetIdentityCurrentPacketNode(List);
    if (CurrentIdentitiesNode == NULL)
    {
        DEBUG((DEBUG_INFO, "Failed to Get GetCurrentIdentitiesPacketNode Node\n"));
        Status = EFI_NO_MAPPING;
        goto EXIT;
    }

    //
    //Get the Identities Node List Node
    //
    CurrentIdentitiesListNode = GetIdentityCurrentListNodeFromPacketNode(CurrentIdentitiesNode);
    if (CurrentIdentitiesListNode == NULL)
    {
        DEBUG((DEBUG_INFO, "Failed to Get Current Identity List Node from Packet Node\n"));
        Status = EFI_NO_MAPPING;
        goto EXIT;
    }

    for (INT8 i = 0; i < MAX_NUMBER_OF_CERTS; i++)
    {
        Thumbprint = NULL;
        Thumbprint16 = NULL;
        FreeThumbprint = FALSE;
        if (mInternalCertStore.Certs[i].Cert != NULL)
        {
            Thumbprint16 =  GetSha1Thumbprint(mInternalCertStore.Certs[i].Cert, mInternalCertStore.Certs[i].CertSize);
            if (NULL == Thumbprint16)
            {
                Thumbprint = IDENTITY_CURRENT_NO_CERTIFICATE_VALUE;
                ThumbprintLen = sizeof(IDENTITY_CURRENT_NO_CERTIFICATE_VALUE);
            } else {
                ThumbprintLen = StrnLenS(Thumbprint16, CERT_STRING_SIZE);
                Thumbprint = (CHAR8 *) AllocatePool (ThumbprintLen + sizeof(CHAR8));
                Status = UnicodeStrToAsciiStrS (Thumbprint16, Thumbprint, ThumbprintLen + sizeof(CHAR8));
                if (EFI_ERROR(Status)) {
                    FreePool (Thumbprint);
                    DEBUG((DEBUG_ERROR,"%a - Error converting thumbprint to ASCII. Code=%r\n",__FUNCTION__, Status));
                    Thumbprint = NULL;
                } else {
                    FreeThumbprint = TRUE;
                }
                FreePool (Thumbprint16);
            }
        }
        if (NULL == Thumbprint)
        {
            Thumbprint = IDENTITY_CURRENT_NO_CERTIFICATE_VALUE;
            ThumbprintLen = sizeof(IDENTITY_CURRENT_NO_CERTIFICATE_VALUE);
        }
        switch (CertIndexToDfciIdentity(i))
        {
            case DFCI_IDENTITY_SIGNER_ZTD :
                Id = IDENTITY_CURRENT_ZTD_CERT_NAME;
                break;
            case DFCI_IDENTITY_SIGNER_OWNER:
                Id = IDENTITY_CURRENT_OWNER_CERT_NAME;
                break;
            case DFCI_IDENTITY_SIGNER_USER:
                Id = IDENTITY_CURRENT_USER_CERT_NAME;
                break;
            case DFCI_IDENTITY_SIGNER_USER1:
                Id = IDENTITY_CURRENT_USER1_CERT_NAME;
                break;
            case DFCI_IDENTITY_SIGNER_USER2:
                Id = IDENTITY_CURRENT_USER2_CERT_NAME;
                break;
            default:
                if (TRUE == FreeThumbprint)
                {
                    FreePool (Thumbprint);
                    FreeThumbprint = FALSE;
                }
                continue;
        }
        Status = SetIdentityCurrentCertificate (CurrentIdentitiesListNode,
                                                Id,
                                                Thumbprint);
        if (EFI_ERROR(Status))
        {
            DEBUG((DEBUG_ERROR,"%a - Unable to populate XML. Code=%r. Certificate:\n",Status));
            DEBUG((DEBUG_ERROR,"     for %s with certificate: %s\n", Id, Thumbprint));
        }
        if (TRUE == FreeThumbprint)
        {
            FreePool (Thumbprint);
            FreeThumbprint = FALSE;
        }

    } //end for loop

    //print the list
    DEBUG((DEBUG_INFO, "PRINTING CURRENT IDENTITY XML - Start\n"));
    DebugPrintXmlTree(List, 0);
    DEBUG((DEBUG_INFO, "PRINTING CURRENT IDENTITY XML - End\n"));

    //now output as xml string

    Status = XmlTreeToString(List, TRUE, StringSize, XmlString);
    if (EFI_ERROR(Status))
    {
        DEBUG((DEBUG_ERROR, "%a - XmlTreeToString failed.  %r\n", __FUNCTION__, Status));
    }

EXIT:
    if (List != NULL)
    {
        FreeXmlTree(&List);
    }

    if (EFI_ERROR(Status))
    {
        //free memory since it was an error
        if (*XmlString != NULL)
        {
            FreePool(*XmlString);
           *XmlString = NULL;
           *StringSize = 0;
        }
    }
    return Status;
}

/**
 * Populate current identities.  Due to this being new, every boot
 * needs check if CurrentIdentities needs to be published.
 *
 * @param Force        TRUE - A change may have occurred. Rebuild current XML
 *
 * @return EFI_STATUS EFIAPI
 */
EFI_STATUS
EFIAPI
PopulateCurrentIdentities(BOOLEAN   Force)
{
    EFI_STATUS Status;
    UINT32     Attributes;
    CHAR8     *Var = NULL;
    UINTN      VarSize;


    VarSize = 0;
    Status = gRT->GetVariable(DFCI_IDENTITY_CURRENT_VAR_NAME,
                             &gDfciAuthProvisionVarNamespace,
                             &Attributes,
                             &VarSize,
                              NULL);
    if ((EFI_BUFFER_TOO_SMALL == Status) &&
        (DFCI_IDENTITY_VAR_ATTRIBUTES  == Attributes))
    {
        DEBUG((DEBUG_INFO, "%a - Current Identity Xml already set\n", __FUNCTION__));
        if (!Force)
        {
            return EFI_SUCCESS;
        }
    }
    if ((EFI_SUCCESS == Status) &&
        (DFCI_IDENTITY_VAR_ATTRIBUTES  != Attributes))
    {
        // Delete the current variable if it has incorrect attributes
        Status = gRT->SetVariable(DFCI_IDENTITY_CURRENT_VAR_NAME,
                                 &gDfciAuthProvisionVarNamespace,
                                 0,
                                 0,
                                 NULL);
    }

    //Create string of Xml
    Status = CreateXmlStringFromCurrentIdentities(&Var, &VarSize);
    if (EFI_ERROR(Status))
    {
        DEBUG((DEBUG_ERROR, "%a - Failed to create xml string from current identities %r\n", __FUNCTION__, Status));
        goto EXIT;
    }

    //Save variable
    Status = gRT->SetVariable(DFCI_IDENTITY_CURRENT_VAR_NAME, &gDfciAuthProvisionVarNamespace, DFCI_IDENTITY_VAR_ATTRIBUTES , VarSize, Var);
    if (EFI_ERROR(Status))
    {
        DEBUG((DEBUG_ERROR, "%a - Failed to write current identities Xml variable %r\n", __FUNCTION__, Status));
        goto EXIT;
    }
    //Success
    DEBUG((DEBUG_INFO, "%a - Current Identities Xml Var Set with data size: 0x%X\n", __FUNCTION__, VarSize));
    Status = EFI_SUCCESS;

EXIT:
    if (Var != NULL)
    {
        FreePool(Var);
        Var = NULL;
    }
    return Status;
}
