
#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Register\ArchitecturalMsr.h>
#include <UnitTestTypes.h>
#include <Library/UnitTestLib.h>
#include <Library/UnitTestLogLib.h>
#include "../HeapGuardTestCommon.h"

UNIT_TEST_STATUS
EFIAPI
UefiHardwareNxProtectionEnabled (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  MSR_IA32_EFER_REGISTER Efer;

  DEBUG((DEBUG_ERROR, __FUNCTION__"\n"));
  Efer.Uint64 = AsmReadMsr64 (MSR_IA32_EFER);
  if (Efer.Bits.NXE == 1) {
    return UNIT_TEST_PASSED;
  }
  UT_LOG_WARNING("Efer set as 0x%x\n",Efer);
  return UNIT_TEST_ERROR_TEST_FAILED;
}