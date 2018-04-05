
#include "IdentityAndAuthManager.h"

LIST_ENTRY  mAuthIdMapList = INITIALIZE_LIST_HEAD_VARIABLE(mAuthIdMapList);  //linked list for the active Auth Handles to Identity Map


/*
Function to find the DFCI_AUTH_TO_ID_LIST_ENTRY given the auth token.
From the List entry and Identiry Properties and all other fields can be 
easily accessed.  The List Entry also can contain other information about
validity period, access count, etc.  If any data is returned to an external caller
it should be copied so that the enternal entity can not modify the internal values.  

@param Token    Pointer to Auth Token to find
@retval         Will return the DFCI_AUTH List entry structure if found.  Otherwise NULL
*/
DFCI_AUTH_TO_ID_LIST_ENTRY*
FindListEntryByAuthToken(
  IN  CONST DFCI_AUTH_TOKEN   *Token
)
{
  LIST_ENTRY* Link = NULL;
  DFCI_AUTH_TO_ID_LIST_ENTRY *Entry = NULL;

  for (Link = mAuthIdMapList.ForwardLink; Link != &mAuthIdMapList; Link = Link->ForwardLink) {

    //Convert Link Node into object stored
    Entry = CR(Link, DFCI_AUTH_TO_ID_LIST_ENTRY, Link, DFCI_AUTH_TO_ID_LIST_ENTRY_SIGNATURE);
    if (Entry->AuthToken == *Token)
    {
      DEBUG((DEBUG_INFO, "%a - Found (0x%X)\n",__FUNCTION__, *Token));

      //TODO: add any security filtering here.  For example if we add a Auth Timeout or max access count

      return Entry;
    }
  }
  DEBUG((DEBUG_INFO, "%a - Failed to find (0x%X)\n", __FUNCTION__, *Token));
  return NULL;
}



/**
Add an Auth Token to Id Properties Entry

@param  Token                Add new token to id map
@param  Properties           Identity Properties ptr

**/
EFI_STATUS
AddAuthHandleMapping(
  IN DFCI_AUTH_TOKEN          *Token,
  IN DFCI_IDENTITY_PROPERTIES *Properties
)
{
  DFCI_AUTH_TO_ID_LIST_ENTRY *Entry = NULL;

  if ((Token == NULL) || (Properties == NULL))
  {
    DEBUG((DEBUG_ERROR, "%a - Invalid parameter\n", __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }

  DEBUG((DEBUG_INFO, "Adding Auth Token Mapping 0x%X\n", *Token));

  //check to make sure it doesn't already exist. 
  if (FindListEntryByAuthToken(Token) != NULL)
  {
    DEBUG((DEBUG_ERROR, "Error - Can't map the same auth token to more than one id property. 0x%X\n", *Token));
    ASSERT(FALSE);
    return EFI_INVALID_PARAMETER;
  }

  //Allocate memory for 
  Entry = AllocateZeroPool(sizeof(DFCI_AUTH_TO_ID_LIST_ENTRY));
  if (Entry == NULL)
  {
    DEBUG((DEBUG_ERROR, "%a - Couldn't allocate memory for entry\n", __FUNCTION__));
    return EFI_OUT_OF_RESOURCES;
  }

  Entry->Signature = DFCI_AUTH_TO_ID_LIST_ENTRY_SIGNATURE;

  //Copy auth token to new entry
  Entry->AuthToken = *Token;
  Entry->Identity = Properties;

  //insert into list
  InsertTailList(&mAuthIdMapList, &Entry->Link);

  return EFI_SUCCESS;
}


VOID
FreeAuthHandleMapping(
  IN DFCI_AUTH_TO_ID_LIST_ENTRY *Entry
)
{
  if (Entry != NULL)
  {
    RemoveEntryList(&Entry->Link);
    //Any dynamic memory in the Entry should be freed here. 
    //Note: Identity is currently staically allocated so it should not be freed.
    FreePool(Entry);
  }
}

/**
Function to dispose any existing identity mappings that
are for an Id in the Identity Mask.

Success for all disposed
Error for error condition
**/
EFI_STATUS
EFIAPI
DisposeAllIdentityMappings(DFCI_IDENTITY_MASK Mask)
{
  LIST_ENTRY* Link = NULL;
  DFCI_AUTH_TO_ID_LIST_ENTRY *Entry = NULL;

  for (Link = mAuthIdMapList.ForwardLink; Link != &mAuthIdMapList; )
  {
    //Convert Link Node into object stored
    Entry = CR(Link, DFCI_AUTH_TO_ID_LIST_ENTRY, Link, DFCI_AUTH_TO_ID_LIST_ENTRY_SIGNATURE);

    //Move Link up to next node before possibly freeing Current Entry
    Link = Link->ForwardLink;


    if ((Entry->Identity->Identity & Mask) != 0)
    {
      DEBUG((DEBUG_INFO, "%a - Disposed of Entry with Identity 0x%x\n", __FUNCTION__, Entry->Identity->Identity));
      FreeAuthHandleMapping(Entry);
    }
  }
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
DisposeAuthToken(
  IN  CONST DFCI_AUTHENTICATION_PROTOCOL        *This,
  IN OUT DFCI_AUTH_TOKEN                  *IdentityToken
)
{
  DFCI_AUTH_TO_ID_LIST_ENTRY *Entry = NULL;

  if (IdentityToken == NULL)
  {
    return EFI_INVALID_PARAMETER;
  }

  Entry = FindListEntryByAuthToken(IdentityToken);
  if (Entry != NULL)
  {
    //found it
    FreeAuthHandleMapping(Entry);
    DEBUG((DEBUG_INFO, "%a - Disposed of AuthToken 0x%x\n", __FUNCTION__, *IdentityToken));
    *IdentityToken = DFCI_AUTH_TOKEN_INVALID;
    return EFI_SUCCESS;
  }

  DEBUG((DEBUG_ERROR, "%a - AuthToken 0x%x not found\n", __FUNCTION__, *IdentityToken));
  return EFI_NOT_FOUND;
}


/**
Function to Get the Identity Properties of a given Identity Token.

This function will recieve caller input and needs to protect against brute force
attack by rate limiting or some other means as the IdentityToken values are limited.

@param This           Auth Protocol Instance Pointer
@param IdentityToken  Token to get Properties
@param Properties     caller allocated memory for the current Identity properties to be copied to.

@retval EFI_SUCCESS   If the auth token is valid and Properties updated with current values
@retval ERROR         Failed

**/
EFI_STATUS
EFIAPI
GetIdentityProperties(
  IN  CONST DFCI_AUTHENTICATION_PROTOCOL     *This,
  IN  CONST DFCI_AUTH_TOKEN                  *IdentityToken,
  IN OUT DFCI_IDENTITY_PROPERTIES      *Properties
)
{
  DFCI_AUTH_TO_ID_LIST_ENTRY *Entry = NULL;

  if ((IdentityToken == NULL) || (This == NULL) || (Properties == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  Entry = FindListEntryByAuthToken(IdentityToken);
  if (Entry == NULL)
  {
    DEBUG((DEBUG_ERROR, "%a - Auth Token (0x%X) Not found.\n", __FUNCTION__, *IdentityToken));
    return EFI_NOT_FOUND;
  }
  //Must be copied we don't want to give back
  //a ptr to the real entry otherwise user could modify internal data. 
  CopyMem(Properties, Entry->Identity, sizeof(DFCI_IDENTITY_PROPERTIES));
  return EFI_SUCCESS;
}
