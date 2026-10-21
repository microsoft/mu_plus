/** @file
  Public test data for SecureBootKeyAuditApp.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef SECURE_BOOT_KEY_AUDIT_TEST_DATA_H_
#define SECURE_BOOT_KEY_AUDIT_TEST_DATA_H_

#include <Uefi.h>

extern CONST UINT8  gSecureBootKeyAuditPkA[];
extern CONST UINTN  gSecureBootKeyAuditPkASize;
extern CONST UINT8  gSecureBootKeyAuditPkB[];
extern CONST UINTN  gSecureBootKeyAuditPkBSize;
extern CONST UINT8  gSecureBootKeyAuditPkMldsa[];
extern CONST UINTN  gSecureBootKeyAuditPkMldsaSize;
extern CONST UINT8  gSecureBootKeyAuditSetupPkA[];
extern CONST UINTN  gSecureBootKeyAuditSetupPkASize;
extern CONST UINT8  gSecureBootKeyAuditSetupPkMldsa[];
extern CONST UINTN  gSecureBootKeyAuditSetupPkMldsaSize;
extern CONST UINT8  gSecureBootKeyAuditSetupPkAForMldsa[];
extern CONST UINTN  gSecureBootKeyAuditSetupPkAForMldsaSize;
extern CONST UINT8  gSecureBootKeyAuditPkAToB[];
extern CONST UINTN  gSecureBootKeyAuditPkAToBSize;
extern CONST UINT8  gSecureBootKeyAuditPkBToA[];
extern CONST UINTN  gSecureBootKeyAuditPkBToASize;
extern CONST UINT8  gSecureBootKeyAuditPkAToMldsa[];
extern CONST UINTN  gSecureBootKeyAuditPkAToMldsaSize;
extern CONST UINT8  gSecureBootKeyAuditPkMldsaToA[];
extern CONST UINTN  gSecureBootKeyAuditPkMldsaToASize;
extern CONST UINT8  gSecureBootKeyAuditDeleteByA[];
extern CONST UINTN  gSecureBootKeyAuditDeleteByASize;
extern CONST UINT8  gSecureBootKeyAuditDeleteByMldsa[];
extern CONST UINTN  gSecureBootKeyAuditDeleteByMldsaSize;
extern CONST UINT8  gSecureBootKeyAuditDeleteByAAfterMldsa[];
extern CONST UINTN  gSecureBootKeyAuditDeleteByAAfterMldsaSize;
extern CONST UINT8  gSecureBootKeyAuditRecoveryDeleteByA[];
extern CONST UINTN  gSecureBootKeyAuditRecoveryDeleteByASize;
extern CONST UINT8  gSecureBootKeyAuditRecoveryDeleteByB[];
extern CONST UINTN  gSecureBootKeyAuditRecoveryDeleteByBSize;
extern CONST UINT8  gSecureBootKeyAuditRecoveryDeleteByMldsa[];
extern CONST UINTN  gSecureBootKeyAuditRecoveryDeleteByMldsaSize;
extern CONST UINT8  gSecureBootKeyAuditDbTbsHashV1[];
extern CONST UINTN  gSecureBootKeyAuditDbTbsHashV1Size;
extern CONST UINT8  gSecureBootKeyAuditDbTbsHashV2[];
extern CONST UINTN  gSecureBootKeyAuditDbTbsHashV2Size;
extern CONST UINT8  gSecureBootKeyAuditEnrollDbTbsHashV1[];
extern CONST UINTN  gSecureBootKeyAuditEnrollDbTbsHashV1Size;
extern CONST UINT8  gSecureBootKeyAuditEnrollDbTbsHashV2[];
extern CONST UINTN  gSecureBootKeyAuditEnrollDbTbsHashV2Size;
extern CONST UINT8  gSecureBootKeyAuditDeleteDbV1ByMldsa[];
extern CONST UINTN  gSecureBootKeyAuditDeleteDbV1ByMldsaSize;
extern CONST UINT8  gSecureBootKeyAuditDeleteDbV2ByMldsa[];
extern CONST UINTN  gSecureBootKeyAuditDeleteDbV2ByMldsaSize;
extern CONST UINT8  gSecureBootKeyAuditHelloMldsa[];
extern CONST UINTN  gSecureBootKeyAuditHelloMldsaSize;
extern CONST UINT8  gSecureBootKeyAuditFixtureId[];
extern CONST UINTN  gSecureBootKeyAuditFixtureIdSize;
extern CONST UINT8  gSecureBootKeyAuditManifestSha256[];
extern CONST UINTN  gSecureBootKeyAuditManifestSha256Size;

#endif
