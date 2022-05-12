/** @file
  Update FACS Hardware Signature library definition. A device can implement
  instances to support device specific behavior.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __UPDATE_FACS_HARDWARE_SIGNATURE_LIB_H_
#define __UPDATE_FACS_HARDWARE_SIGNATURE_LIB_H_

// This enumeration defines all the FACS hardware signature algorithms.
typedef enum {
  DefaultFacsHwSigAlgorithm = 0,
  FacsV2CompatibleFacsHwSigAlgorithm,
  FacsV3CompatibleFacsHwSigAlgorithm,
  FacsV2WithoutPciIdsFacsHwSigAlgorithm,
  FacsV2LibraryCustom1FacsHwSigAlgorithm,
  FacsV2LibraryCustom2FacsHwSigAlgorithm,
  FacsHwSigAlgorithmMax
} FACS_HARDWARE_SIGNATURE_ALGORITHM;

/**
 * UpdateFacsHardwareSignature
 *
 * MAT is computed at ExitBootServices.  FACS.HardwareSignature is used before that,
 * so cannot include MAT in the HardwareSignature.
 *
 * @param[in]   FacsHwSigAlgorithm   FACS HardwareSignature algorithm enumerate.
 *
 * @retval   EFI_SUCCESS       No errors when updating FACS HardwareSignature.
 * @retval   EFI_UNSUPPORTED   The selected HardwareSignature algorithm is not supported.
 * @retval   Other             Unexpected failure when updating FACS HardwareSignature.
 */
EFI_STATUS
EFIAPI
UpdateFacsHardwareSignature (
  IN FACS_HARDWARE_SIGNATURE_ALGORITHM  FacsHwSigAlgorithm
  );

#endif // __UPDATE_FACS_HARDWARE_SIGNATURE_LIB_H_
