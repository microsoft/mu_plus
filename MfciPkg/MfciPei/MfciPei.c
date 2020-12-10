/** @file
  The PEI phase implementation of the public interface to the MFCI Policy
  Constructs a HOB so that DXE knows what policy was used during PEI

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


MFCI_POLICY_TYPE
EFIAPI
InternalGetMfciPolicy (
  IN CONST MFCI_POLICY_PPI   *This
  );

MFCI_POLICY_PPI MfciPpi = {
  .GetMfciPolicy = InternalGetMfciPolicy
};

STATIC EFI_PEI_PPI_DESCRIPTOR mMfciPpiList = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gMfciPpiGuid,
  &MfciPpi
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
    DEBUG((DEBUG_ERROR, "%a: Input pointer should NOT be NULL", __FUNCTION__));
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
Entry to MfciPei
Read the PEI phase MFCI policy from a variable and publish to a HOB for consumption by both
this driver's PPI as well as DXE phase.  Register a PPI so that PEI drivers can determine the
MFCI policy and take action accordingly

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
  EFI_PEI_READ_ONLY_VARIABLE2_PPI   *PeiVariablePpi = NULL;
  MFCI_POLICY_TYPE                  Policy;
  MFCI_POLICY_TYPE                  *PolicyHobPtr;
  UINTN                             DataSize;
  UINT32                            Attributes;

  DEBUG((DEBUG_INFO, "%a: enter...\n", __FUNCTION__));

  Status = PeiServicesLocatePpi(&gEfiPeiReadOnlyVariable2PpiGuid,
                                0,
                                NULL,
                                (VOID**)&PeiVariablePpi);

  if (EFI_ERROR(Status) || PeiVariablePpi == NULL) {
    DEBUG((DEBUG_ERROR, "%a: failed to locate PEI Variable PPI (%r) PeiVariablePpi(%p)\n", __FUNCTION__, Status, PeiVariablePpi));  // Depex failed
    goto Cleanup;
  }

  DataSize = sizeof (Policy);
  Status = PeiVariablePpi->GetVariable (
                            PeiVariablePpi,
                            CURRENT_MFCI_POLICY_VARIABLE_NAME,
                            &MFCI_VAR_VENDOR_GUID,
                            &Attributes,
                            &DataSize,
                            &Policy);

  if (EFI_ERROR( Status )
      || Attributes != MFCI_POLICY_VARIABLE_ATTR
      || DataSize != sizeof( Policy )) {
    DEBUG((DEBUG_ERROR,
      "%a: GetVariable(CURRENT_MFCI_POLICY_VARIABLE_NAME) failed to return " \
      "well-formed data Status(%r) Attributes(0x%x) DataSize(%d)\n" \
      "note that this is expected on first boot after flashing\n",
      __FUNCTION__, Status, Attributes, DataSize));

    // Fail secure
    Policy = CUSTOMER_STATE;
    DataSize = sizeof (Policy);
  }

  PolicyHobPtr = BuildGuidHob(&gMfciHobGuid, DataSize);
  if (PolicyHobPtr == NULL) {
    DEBUG((DEBUG_ERROR, "%a: BuildGuidHob() returned NULL", __FUNCTION__));
    ASSERT(PolicyHobPtr != NULL);
    Status = EFI_OUT_OF_RESOURCES;
    goto Cleanup;  // without a HOB, the PPI cannot return accurate state
  }
  DEBUG((DEBUG_INFO, "%a: Published MFCI HOB with policy(0x%lx)\n", __FUNCTION__, Policy));

  // publish the Policy into a HOB for consumption by both the PPI as well
  // as the DXE phase
  *PolicyHobPtr = Policy;

  Status = (*PeiServices)->InstallPpi (
                              PeiServices,
                              &mMfciPpiList
                              );
  ASSERT_EFI_ERROR (Status);

Cleanup:
  DEBUG((DEBUG_INFO, "%a: exit (%r)\n", __FUNCTION__, Status));
  return Status;
}
