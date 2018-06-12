/** @file
  This application will locate all variables and aquire their status as deletable.

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


#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>

#include <Library/ShellLib.h>
#include "LockTestXml.h"

#define MAX_NAME_LEN  1024
#define MAX_NAME_SIZE (MAX_NAME_LEN * sizeof(CHAR16 ))



XmlNode*
EFIAPI
CreateListOfAllVars()
{
  EFI_STATUS                      Status;
  CHAR16                          varName[MAX_NAME_LEN];
  EFI_GUID                        varGuid;
  UINTN                           varNameSize;
  XmlNode                       *List;

  List = New_VariablesNodeList();
  if (List == NULL)
  {
    DEBUG((DEBUG_ERROR, "Failed to allocate an XML list\n"));
    return NULL;
  }
  ZeroMem(&varGuid, sizeof(EFI_GUID));
  varName[0] = L'\0';
  varNameSize = MAX_NAME_SIZE;
  Status = gRT->GetNextVariableName(&varNameSize, &varName[0], &varGuid);
  while (!EFI_ERROR(Status))
  {
    UINT8 *varData = NULL;
    UINTN  varDataSize = 0;
    UINT32 varAttributes = 0;

    Status = GetVariable3(varName, &varGuid, &varData, &varDataSize, &varAttributes);
    if (!EFI_ERROR(Status))
    {
      if (New_VariableNodeInList(List, varName, &varGuid, varAttributes, varDataSize, varData) == NULL)
      {
        DEBUG((DEBUG_ERROR, "Failed to create new Var Node.  Var Name: %s Guid: %g\n", varName, &varGuid));
        Status = EFI_DEVICE_ERROR;

      }
    }
    if (varData) { FreePool(varData); }

    //get next
    varNameSize = MAX_NAME_SIZE;
    Status = gRT->GetNextVariableName(&varNameSize, &varName[0], &varGuid);
  }

  return List;
}

EFI_STATUS
EFIAPI
UpdateListWithReadWriteInfo(XmlNode *List)
{
  EFI_STATUS Status;
  LIST_ENTRY *Link = NULL;

  if (List == NULL)
  {
    return EFI_INVALID_PARAMETER;
  }

  //loop thru all children of the list
  if (List->ParentNode != NULL)
  {
    DEBUG((DEBUG_ERROR, "!!!ERROR: BAD XML.  List should be head node\n"));
    return EFI_INVALID_PARAMETER;
  }

  //Loop thru all children which should be variable nodes
  for (Link = List->ChildrenListHead.ForwardLink; Link != &(List->ChildrenListHead); Link = Link->ForwardLink)
  {
    XmlNode* Current = (XmlNode*)Link;
    CHAR16* varName = NULL;
    EFI_GUID varGuid;
    UINT8 *varData = NULL;
    UINTN varDataSize = 0;
    UINT32 varAttributes = 0;
    EFI_STATUS StatusFromDelete = EFI_SUCCESS;

    //Get current info
    Status = GetNameGuidMembersFromNode(Current, &varName, &varGuid);
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, __FUNCTION__ " Failed in GetNameGuidMembers.  Status = %r\n", Status));
      return Status;
    }

    //Get current data - don't use XML stringized data...easier just to read binary again
    Status = GetVariable3(varName, &varGuid, &varData, &varDataSize, &varAttributes);
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, __FUNCTION__ " Failed in GetVar3.  Status = %r\n", Status));
      FreePool(varName);
      return Status;
    }

    DEBUG((DEBUG_INFO, __FUNCTION__ " testing write properties for var %g", &varGuid));
    DEBUG((DEBUG_INFO," ::%s", varName));
    DEBUG((DEBUG_INFO, "\n"));  //do independent debug print so that we always have newline.  Some names can be long and overrun the debug buffer

    //Delete current var
    StatusFromDelete = gRT->SetVariable(varName, &varGuid, varAttributes, 0, NULL);
    Status = AddReadyToBootStatusToNode(Current, EFI_SUCCESS, StatusFromDelete);
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, __FUNCTION__ " failed in AddReadyToBootStatusToNode.  Status = %r\n", Status));
    }

    //restore if needed
    if (!EFI_ERROR(StatusFromDelete))
    {
      Status = gRT->SetVariable(varName, &varGuid, varAttributes, varDataSize, varData);
      if (EFI_ERROR(Status))
      {
        DEBUG((DEBUG_ERROR, __FUNCTION__ " failed to restore variable data.  Status = %r\n", Status));
      }
    }

    //clean up
    if (varName != NULL) { FreePool(varName); }
    if (varData != NULL) { FreePool(varData); }
  }
  return EFI_SUCCESS;

}


/**
  The user Entry Point for Application. The user code starts with this function
  as the real entry point for the application.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
LockTestEntry(
IN EFI_HANDLE        ImageHandle,
IN EFI_SYSTEM_TABLE  *SystemTable
)
{
    EFI_STATUS                     Status;
    CHAR16                         LogFileName[] = L"UefiVarLockAudit_manifest.xml";
    SHELL_FILE_HANDLE              FileHandle;
    XmlNode*                     MyList = NULL;
    UINTN                          StringSize = 0;
    CHAR8*                         XmlString = NULL;

    
    MyList = CreateListOfAllVars();
    if (MyList == NULL)
    {
      Status = EFI_OUT_OF_RESOURCES;
      DEBUG((DEBUG_ERROR, "Failed to get list of vars Status = %r\n", Status));
      goto Exit;
    }

    //Get R/W properties
    Status = UpdateListWithReadWriteInfo(MyList);
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "Failed to Update List with Read/Write Properties = %r\n", Status));
      goto Exit;
    }

    //Write XML
    Status = XmlTreeToString(MyList, TRUE, &StringSize, &XmlString);
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "XmlTreeToString failed.  %r\n", Status));
      goto Exit;
    }

    if (StringSize == 0)
    {
      Status = EFI_OUT_OF_RESOURCES;
      DEBUG((DEBUG_ERROR, "StringSize equal 0.\n"));
      goto Exit;
    }

    //
    // subtract 1 from string size to avoid writing the NULL terminator
    //
    StringSize--;

    //
    //First lets open the file if it exists so we can delete it...This is the work around for truncation
    //
    Status = ShellOpenFileByName(LogFileName, &FileHandle, EFI_FILE_MODE_WRITE | EFI_FILE_MODE_READ, 0);
    if (!EFI_ERROR(Status))
    {
      //if file handle above was opened it will be closed by the delete.
      Status = ShellDeleteFile(&FileHandle);
      if (EFI_ERROR(Status))
      {
        DEBUG((DEBUG_ERROR, __FUNCTION__ " failed to delete file %r\n", Status));
      }
    }


    Status = ShellOpenFileByName(LogFileName, &FileHandle, EFI_FILE_MODE_CREATE | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(Status)) {
      DEBUG((DEBUG_ERROR, "Failed to open %s file for create. Status = %r\n", LogFileName, Status));
      goto Exit;
    }
    else
    {
      ShellPrintEx(-1, -1, L"Writing XML to file %s\n", LogFileName);
      ShellWriteFile(FileHandle, &StringSize, XmlString);
      ShellCloseFile(&FileHandle);
    }

    //success
    Status = EFI_SUCCESS;

  Exit:
    if (MyList != NULL) { FreeXmlTree(&MyList); }
    if (XmlString != NULL) { FreePool(XmlString); }
    return Status & 0x7FFFFFFFFFFFFF;
}

