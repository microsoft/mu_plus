/** @file
  The PEI phase implementation of the public interface to 
  the MFCI Policy

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiPei.h>
#include <MfciPolicyType.h>
#include <MfciVariables.h>

#include <Library/PeiServicesLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/HobLib.h>

#include <Ppi/MfciPolicyPpi.h>
#include <Ppi/ReadOnlyVariable2.h>

/**
  This function handles PlatformInit task after PeiReadOnlyVariable2 PPI produced

  @param[in]  PeiServices  Pointer to PEI Services Table.
  @param[in]  NotifyDesc   Pointer to the descriptor for the Notification event that
                           caused this function to execute.
  @param[in]  Ppi          Pointer to the PPI data associated with this function.

  @retval     EFI_SUCCESS  The function completes successfully
  @retval     others
**/
EFI_STATUS
EFIAPI
PeiVariableNotify (
  IN CONST EFI_PEI_SERVICES     **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  );


MFCI_POLICY_TYPE
EFIAPI
InternalGetMfciPolicy (
  IN CONST MFCI_POLICY_PPI   *This
  );

MFCI_POLICY_PPI MfciProtocol = {
  .GetMfciPolicy = InternalGetMfciPolicy
};

STATIC EFI_PEI_NOTIFY_DESCRIPTOR mPeiVariableNotifyList = {
  (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiReadOnlyVariable2PpiGuid,
  (EFI_PEIM_NOTIFY_ENTRY_POINT) PeiVariableNotify
};

STATIC EFI_PEI_PPI_DESCRIPTOR mMfciProtocolList = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gMfciPpiGuid,
  &MfciProtocol
};

/**
  GetMfciPolicy()

  This function returns the MFCI Policy in effect for the current boot

  @param[in] This                - Current MFCI policy ppi installed.

  @retval Other                  - Bits definition can be found in MfciPolicyType.h.

**/
MFCI_POLICY_TYPE
EFIAPI
InternalGetMfciPolicy (
  IN CONST MFCI_POLICY_PPI   *This
  )
{
  MFCI_POLICY_TYPE                  *PolicyPtr;
  MFCI_POLICY_TYPE                  Policy;
  UINTN                             EntrySize;
  VOID                              *GuidHob;

  Policy = CUSTOMER_STATE;

  if (This == NULL) {
    // Do not give out any if input parameter is insane.
    goto Cleanup;
  }

  GuidHob = GetFirstGuidHob(&gMfciHobGuid);
  if (GuidHob == NULL) {
    // This means no reported errors during Pei phase, job done!
    goto Cleanup;
  }

  PolicyPtr = GET_GUID_HOB_DATA(GuidHob);
  EntrySize = GET_GUID_HOB_DATA_SIZE(GuidHob);

  if ((PolicyPtr == NULL) || (EntrySize != sizeof(MFCI_POLICY_TYPE))) {
    goto Cleanup;
  }

  Policy = *PolicyPtr;

Cleanup:
  return Policy;
}

/**
  This function handles MFCI policy task when/after PeiReadOnlyVariable2 PPI produces:

  @param[in]  PeiServices  Pointer to PEI Services Table.
  @param[in]  NotifyDesc   Pointer to the descriptor for the Notification event that
                           caused this function to execute.
  @param[in]  Ppi          Pointer to the PPI data associated with this function.

  @retval     EFI_SUCCESS  The function completes successfully
  @retval     others
**/
EFI_STATUS
EFIAPI
PeiVariableNotify (
  IN CONST EFI_PEI_SERVICES     **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  EFI_STATUS                        Status;
  EFI_PEI_READ_ONLY_VARIABLE2_PPI   *PeiVariablePpi;
  MFCI_POLICY_TYPE                  Policy;
  MFCI_POLICY_TYPE                  *PolicyHobPtr;
  UINTN                             DataSize;
  UINT32                            Attributes;

  if (Ppi == NULL || PeiServices == NULL) {
    ASSERT (FALSE);
    Status = EFI_INVALID_PARAMETER;
    goto Cleanup;
  }

  PeiVariablePpi = (EFI_PEI_READ_ONLY_VARIABLE2_PPI *) Ppi;

  DataSize = sizeof (Policy);
  Status = PeiVariablePpi->GetVariable (
                            PeiVariablePpi,
                            CURRENT_MFCI_POLICY_VARIABLE_NAME,
                            &MFCI_VAR_VENDOR_GUID,
                            &Attributes,
                            &DataSize,
                            &Policy);

  //
  // Finally, sanitize the data and account for any errors.
  // The only way we'll return non CUSTOMER_STATE is if that
  // is the current policy *and* everything else checks out.
  if (EFI_ERROR( Status ) || Attributes != MFCI_POLICY_VARIABLE_ATTR ||
      DataSize != sizeof( Policy )) {
    Policy = CUSTOMER_STATE;
  }

  PolicyHobPtr = BuildGuidHob(&gMfciHobGuid, DataSize);
  if (PolicyHobPtr == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Cleanup;
  }

  *PolicyHobPtr = Policy;

  Status = (*PeiServices)->InstallPpi (
                              PeiServices,
                              &mMfciProtocolList
                              );
  ASSERT_EFI_ERROR (Status);

Cleanup:
  return Status;
}

/**
Entry to MfciPei.

@param FileHandle                     The image handle.
@param PeiServices                    The PEI services table.

@retval Status                        From internal routine or boot object, should not fail
**/
EFI_STATUS
EFIAPI
MfciPeiEntry (
  IN EFI_PEI_FILE_HANDLE              FileHandle,
  IN CONST EFI_PEI_SERVICES           **PeiServices
  )
{

  EFI_STATUS                        Status = EFI_SUCCESS;
  EFI_PEI_READ_ONLY_VARIABLE2_PPI   *PeiVariablePpi;

  DEBUG((DEBUG_INFO, "%a: enter...\n", __FUNCTION__));

  Status = PeiServicesLocatePpi(&gEfiPeiReadOnlyVariable2PpiGuid,
                                0,
                                NULL,
                                (VOID**)&PeiVariablePpi);

  if (EFI_ERROR(Status) != FALSE) {
    // If variable ppi is not available, set a callback for when it is ready.
    // TODO BUG
    DEBUG((DEBUG_WARN, "%a: failed to locate PEI Variable PPI (%r)\n", __FUNCTION__, Status));
    goto Cleanup;
  }
  else {
    // Otherwise, try to get the variable indicating MFCI policy applicable for this device.
    Status = PeiVariableNotify(PeiServices, NULL, PeiVariablePpi);
    if (EFI_ERROR(Status) != FALSE) {
      DEBUG((DEBUG_ERROR, "%a: Status failure from PeiVariableNotify(%r)\n", __FUNCTION__, Status));
    }
  }

Cleanup:
  DEBUG((DEBUG_INFO, "%a: exit (%r)\n", __FUNCTION__, Status));
  return Status;
}
