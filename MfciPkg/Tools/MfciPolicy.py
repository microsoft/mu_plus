# @file
#
# Write MFCI packet into the MFCI mailbox
#
# Copyright (c), Microsoft Corporation
# SPDX-License-Identifier: BSD-2-Clause-Patent

import os
import sys
import logging
import argparse
import ctypes
from SettingSupport.UefiVariablesSupportLib import UefiVariable

gEfiGlobalVariableGuid = "8BE4DF61-93CA-11D2-AA0D-00E098032B8C"
gMfciVendorGuid = "EBA1A9D2-BF4D-4736-B680-B36AFB4DD65B"

CurrentPolicyBlob = "CurrentMfciPolicyBlob"
NextPolicyBlob = "NextMfciPolicyBlob"
CurrentMfciPolicy = "CurrentMfciPolicy"

MfciPolicyInfo = ["CurrentMfciPolicyNonce", "Target\\Manufacturer", "Target\\Product", "Target\\SerialNumber", "Target\\OEM_01", "Target\\OEM_02"]


def option_parser():
    parser = argparse.ArgumentParser()

    parser.add_argument(
        "-a",
        "--apply",
        dest="policyblob",
        required=False,
        type=str,
        default='',
        help="""Apply a signed Mfci Policy Blob to be processed on next reboot"""
    )

    parser.add_argument(
        "-d",
        "--delete",
        action='store_true',
        dest="deletepolicy",
        required=False,
        help="""Delete the current Mfci Policy"""
    )

    parser.add_argument(
        "-r",
        "--retrieve",
        action='store_true',
        dest="retrievepolicy",
        required=False,
        help="""Retrieve Current Mfci Policy"""
    )

    parser.add_argument(
        "-dump",
        "--dump",
        action='store_true',
        dest="getsysteminfo",
        required=False,
        help="""Get System Info required for policy generation""",
    )

    parser.add_argument(
        "-u",
        "--unittest",
        action='store_true',
        dest="unit_test",
        required=False,
        help="""prepare unit test by creating policy""",
    )
    arguments = parser.parse_args()

    return arguments


def get_system_info():
    UefiVar = UefiVariable()

    for Variable in MfciPolicyInfo:
        (errorcode, data, errorstring) = UefiVar.GetUefiVar(Variable, gMfciVendorGuid)
        if errorcode != 0:
            logging.critical(f"Failed to get policy variable {Variable}")
        else:
            print(f" {Variable} = {data}")

    return


def get_current_mfci_policy():
    UefiVar = UefiVariable()

    (errorcode, data, errorstring) = UefiVar.GetUefiVar(CurrentMfciPolicy, gMfciVendorGuid)
    if errorcode == 0:
        result = hex(int.from_bytes(data, byteorder="little", signed=False))
        print(f" Current MFCI Policy is {result}")
    else:
        logging.critical(f"Failed to get {CurrentMfciPolicy}")
    return


def delete_current_mfci_policy():
    UefiVar = UefiVariable()

    (errorcode, data, errorstring) = UefiVar.SetUefiVar(CurrentMfciPolicy, gMfciVendorGuid, None, 3)
    print(errorcode, data, errorstring)
    if errorcode == 0:
        logging.critical(f"Failed to Delete {CurrentMfciPolicy}\n{errorstring}")
    else:
        print(f" Current Mfci Policy deleted")
    return


def set_next_mfci_policy(policy):
    # read the entire file
    with open(arguments.setting_file, "rb") as file:
        var = file.read()
        UefiVar = UefiVariable()

        (errorcode, data, errorstring) = UefiVar.SetUefiVar(NextPolicyBlob, gMfciVendorGuid, var, 3)
    return


#
# main script function
#
def main():
    arguments = option_parser()

    if arguments.getsysteminfo:
        get_system_info()
        return

    elif arguments.retrievepolicy:
        get_current_mfci_policy()
        return

    elif arguments.deletepolicy:
        delete_current_mfci_policy()
        return

    elif not arguments.policyblob == "":
        if not os.path.isfile(arguments.policyblob):
            logging.critical(f"File {arguments.policyblob} does not exist")
            return

        set_next_mfci_policy(arguments.policyblob)
        return
    elif arguments.unit_test:
        UefiVar = UefiVariable()

        value = int("0x123456780A0B0C0D", 16).to_bytes(8, "little")

        (errorcode, data, errorstring) = UefiVar.SetUefiVar(CurrentMfciPolicy, gMfciVendorGuid, value, 3)
        if errorcode == 0:
            logging.critical(f"Set {CurrentMfciPolicy} returned {errorcode} {data} {errorstring}")
        return


    return 0


if __name__ == "__main__":
    # setup main console as logger
    logger = logging.getLogger("")
    logger.setLevel(logging.CRITICAL)
    formatter = logging.Formatter("%(levelname)s - %(message)s")
    console = logging.StreamHandler()
    console.setLevel(logging.CRITICAL)

    # check the privilege level and report error
    if not ctypes.windll.shell32.IsUserAnAdmin():
        print("Administrator privilege required. Please launch from an Administrator privilege level.")
        sys.exit(1)

    # call main worker function
    retcode = main()

    logging.shutdown()
    sys.exit(retcode)
