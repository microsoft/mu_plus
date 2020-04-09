/** @file PlatformSmmProtectionsTestLibNull.c

Null Implementation of PlatformSmmProtectionsTestLib

Copyright (C) Microsoft Corporation. All rights reserved.

**/

#include <Uefi.h>

/**
  Execute unauthorized IO read.
  The Smm paging protections test invokes this routine to request platform attempt
  to execute an IoRead that violates SMM security.
  @retval     EFI_UNSUPPORTED   The platform does not support IoRead protections.
  @retval     EFI_SECURITY_VIOLATION  Since this test is supposed to produce
                                      a system crash, any sort of return value
                                      should be considered a security violation.
**/
EFI_STATUS
TestUnauthorizedIoRead (
  VOID
) {
  return EFI_UNSUPPORTED;
}

/**
  Execute unauthorized IO write.
  The Smm paging protections test invokes this routine to request platform attempt
  to execute an IoWrite that violates SMM security.
  @retval     EFI_UNSUPPORTED   The platform does not support IoWrite protections.
  @retval     EFI_SECURITY_VIOLATION  Since this test is supposed to produce
                                      a system crash, any sort of return value
                                      should be considered a security violation.
**/
EFI_STATUS
TestUnauthorizedIoWrite (
  VOID
) {
  return EFI_UNSUPPORTED;
}

/**
  Execute unauthorized MSR read.
  The Smm paging protections test invokes this routine to request platform attempt
  to execute a MSR read that violates SMM security.
  @retval     EFI_UNSUPPORTED   The platform does not support MSR read protections.
  @retval     EFI_SECURITY_VIOLATION  Since this test is supposed to produce
                                      a system crash, any sort of return value
                                      should be considered a security violation.
**/
EFI_STATUS
TestUnauthorizedMsrRead (
  VOID
) {
  return EFI_UNSUPPORTED;
}

/**
  Execute unauthorized MSR write.
  The Smm paging protections test invokes this routine to request platform attempt
  to execute a MSR write that violates SMM security.
  @retval     EFI_UNSUPPORTED   The platform does not support MSR write protections.
  @retval     EFI_SECURITY_VIOLATION  Since this test is supposed to produce
                                      a system crash, any sort of return value
                                      should be considered a security violation.
**/
EFI_STATUS
TestUnauthorizedMsrWrite (
  VOID
) {
  return EFI_UNSUPPORTED;
}

/**
  Execute unauthorized privileged instruction.
  The Smm paging protections test invokes this routine to request platform attempt to
   execute a privileged instruction that violates SMM security.
  @retval     EFI_UNSUPPORTED         The platform does not support privileged
                                      instruction protections.
  @retval     EFI_SECURITY_VIOLATION  Since this test is supposed to produce
                                      a system crash, any sort of return value
                                      should be considered a security violation.
**/
EFI_STATUS
TestPrivilegedInstruction (
  VOID
) {
  return EFI_UNSUPPORTED;
}

/**
  Attempt to access the SMM entry point.
  The Smm paging protections test invokes this routine to request that the platform attempt to
  access the protected SMM entry point.
  @retval     EFI_UNSUPPORTED         The platform does not support privileged
                                      instruction protections.
  @retval     EFI_SECURITY_VIOLATION  Since this test is supposed to produce
                                      a system crash, any sort of return value
                                      should be considered a security violation.
**/
EFI_STATUS
TestEntryPointAccess (
  VOID
) {
  return EFI_UNSUPPORTED;
}