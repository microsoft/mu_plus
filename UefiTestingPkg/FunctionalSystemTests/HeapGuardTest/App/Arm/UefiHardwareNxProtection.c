
#include <Uefi.h>

#include <Library/DebugLib.h>
#include "../HeapGuardTestCommon.h"

UNIT_TEST_STATUS
EFIAPI
UefiHardwareNxProtectionEnabled (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
    //  TODO: Does ARM have an equivelent "EFER BIT" to check if NX protections are on?
    return UNIT_TEST_PASSED;
}