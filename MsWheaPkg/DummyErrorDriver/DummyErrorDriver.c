/** @file
    DummyErrorDriver.c

Raises various ReportStatusCode calls found throughout project MU.
Can be used to test parsing functions and telemetry functionality.

Uncomment calls to ReportStatusCode to create HwErrRecs. Use HwhMenu
frontpage extension to easily view records.

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Pi/PiStatusCode.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DebugLib.h>

/**
  The driver's entry point. Just calls report status code a bunch of times...

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point executed successfully.
  @retval other           Some error occurred when executing this entry point.

**/
EFI_STATUS
EFIAPI
DummyErrorDriverEntryPoint (
  IN    EFI_HANDLE                  ImageHandle,
  IN    EFI_SYSTEM_TABLE            *SystemTable
  )
{

  UINTN EnableDisable = 0;
  UINTN SetValueEnableDisable = 0;
  UINTN Size = sizeof(UINTN);
  EFI_STATUS Status;

  if(!EFI_ERROR(gRT->GetVariable(L"EnableDisableErrors", &gRaiseTelemetryErrorsAtBoot, NULL, &Size, &EnableDisable))) {

    switch(EnableDisable) {

      case 2:
        Status =  gRT->SetVariable(L"EnableDisableErrors",
                        &gRaiseTelemetryErrorsAtBoot,
                        EFI_VARIABLE_NON_VOLATILE |
                        EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                        sizeof(UINTN),
                        &SetValueEnableDisable);

        if(EFI_ERROR(Status)) {
            DEBUG((DEBUG_INFO, "%a Could not set the enable/disable variable!\n", __FUNCTION__));
          }
        break;

      default:
        break;
    }
  }

  if(EnableDisable) {

    // //NvmExpressHci.c
    // ReportStatusCode((EFI_ERROR_MAJOR | EFI_ERROR_CODE), (EFI_IO_BUS_SCSI | EFI_IOB_EC_INTERFACE_ERROR));

    // //NvmExpressPassthru.c
    // ReportStatusCode((EFI_ERROR_MAJOR | EFI_ERROR_CODE), (EFI_IO_BUS_SCSI | EFI_IOB_EC_INTERFACE_ERROR));

    // //Dispatcher.c
    // ReportStatusCode((EFI_ERROR_MAJOR | EFI_ERROR_CODE), (EFI_SOFTWARE_PEI_CORE | EFI_SW_EC_ABORTED));

    // //MpService.c & CpuInitPeim.c & MpService.c
    // ReportStatusCode (EFI_ERROR_MAJOR | EFI_ERROR_CODE, (EFI_COMPUTING_UNIT_HOST_PROCESSOR | EFI_CU_HP_EC_SELF_TEST));

    // //DxeMain.c
    // REPORT_STATUS_CODE (EFI_ERROR_CODE | EFI_ERROR_MAJOR,(EFI_SOFTWARE_DXE_CORE | EFI_SW_DXE_CORE_EC_NO_ARCH));

    // //DxeLoad.c
    REPORT_STATUS_CODE (EFI_ERROR_CODE | EFI_ERROR_MAJOR,(EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEI_EC_S3_RESUME_PPI_NOT_FOUND));
    REPORT_STATUS_CODE (EFI_ERROR_CODE | EFI_ERROR_MAJOR,(EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEI_EC_NO_RECOVERY_CAPSULE));

    // //PeiMain.c
    // REPORT_STATUS_CODE (EFI_ERROR_CODE | EFI_ERROR_MAJOR, (EFI_SOFTWARE_PEI_CORE | EFI_SW_PEI_CORE_EC_DXEIPL_NOT_FOUND));

    // //UefiCapsule.c
    // REPORT_STATUS_CODE (EFI_ERROR_CODE | EFI_ERROR_MAJOR,(EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEI_EC_INVALID_CAPSULE_DESCRIPTOR));

    // //CpuMp.c & CpuBist.c
    // REPORT_STATUS_CODE (EFI_ERROR_CODE | EFI_ERROR_MAJOR,(EFI_COMPUTING_UNIT_HOST_PROCESSOR | EFI_CU_HP_EC_SELF_TEST));

    // //S3Resume.c
    // REPORT_STATUS_CODE (EFI_ERROR_CODE | EFI_ERROR_MAJOR,(EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEI_EC_S3_OS_WAKE_ERROR));
    // REPORT_STATUS_CODE (EFI_ERROR_CODE | EFI_ERROR_MAJOR,(EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEI_EC_S3_RESUME_FAILED));

  }



  return EFI_SUCCESS;
}
