# @file
#
# Write MFCI packet into the MFCI mailbox
#
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

import os
import sys
import logging
import argparse
import ctypes
from edk2toollib.os.uefivariablesupport import UefiVariable

MFCI_VENDOR_GUID = "EBA1A9D2-BF4D-4736-B680-B36AFB4DD65B"
CURRENT_POLICY_BLOB = "CurrentMfciPolicyBlob"
CURRENT_MFCI_POLICY = "CurrentMfciPolicy"
NEXT_MFCI_POLICY_BLOB = "NextMfciPolicyBlob"
MFCI_POLICY_INFO_VARIABLES = ["CurrentMfciPolicyNonce", "NextMfciPolicyNonce", "Target\\Manufacturer", "Target\\Product", "Target\\SerialNumber", "Target\\OEM_01", "Target\\OEM_02"]

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
        "-del",
        "--delete",
        action='store_true',
        dest="deletepolicy",
        required=False,
        help="""Delete the current Mfci Policy and the next Mfci Policy"""
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
        "-rn",
        "--retrievenext",
        action='store_true',
        dest="retrievenextpolicy",
        required=False,
        help="""Retrieve Next Mfci Policy"""
    )

    parser.add_argument(
        "-d",
        "--dump",
        action='store_true',
        dest="getsysteminfo",
        required=False,
        help="""Get System Info required for policy generation""",
    )

    arguments = parser.parse_args()

    return arguments


def get_system_info():
    UefiVar = UefiVariable()

    for Variable in MFCI_POLICY_INFO_VARIABLES:
        (errorcode, data) = UefiVar.GetUefiVar(Variable, MFCI_VENDOR_GUID)
        if errorcode != 0:
            logging.critical(f"Failed to get policy variable {Variable}")
        else:
            if Variable == "NextMfciPolicyNonce" or Variable == "CurrentMfciPolicyNonce":
                print(f" {Variable} = {hex(int.from_bytes(data, byteorder='little', signed=False))}")
            else:
                print(f" {Variable} = \"{data.decode("utf16")[0:-1]}\"")
    return


def get_current_mfci_policy():
    UefiVar = UefiVariable()

    (errorcode, data) = UefiVar.GetUefiVar(CURRENT_MFCI_POLICY, MFCI_VENDOR_GUID)
    if errorcode == 0:
        result = hex(int.from_bytes(data, byteorder="little", signed=False))
        logging.info(f" Current MFCI Policy is {result}")
        print(f"{CURRENT_MFCI_POLICY} is {result}")
    else:
        logging.critical(f"Failed to get {CURRENT_MFCI_POLICY}")
    return


def delete_current_mfci_policy():
    UefiVar = UefiVariable()

    errorcode = UefiVar.SetUefiVar(CURRENT_POLICY_BLOB, MFCI_VENDOR_GUID, None, 3)
    if errorcode == 0:
        logging.info(f"Failed to Delete {CURRENT_POLICY_BLOB}\n {errorcode}")
        print(f"Failed to delete {CURRENT_POLICY_BLOB}")
    else:
        logging.info(f"{CURRENT_POLICY_BLOB} was deleted")
        print(f"{CURRENT_POLICY_BLOB} was deleted")
        
    errorcode = UefiVar.SetUefiVar(NEXT_MFCI_POLICY_BLOB, MFCI_VENDOR_GUID, None, 3)
    if errorcode == 0:
        logging.info(f"Failed to Delete {NEXT_MFCI_POLICY_BLOB}\n {errorcode}")
        print(f"Failed to delete {NEXT_MFCI_POLICY_BLOB}")
    else:
        logging.info(f"{NEXT_MFCI_POLICY_BLOB} was deleted")
        print(f"{NEXT_MFCI_POLICY_BLOB} was deleted")
    return


def set_next_mfci_policy(policy):
    # read the entire file
    with open(policy, "rb") as file:
        var = file.read()
        UefiVar = UefiVariable()

        errorcode = UefiVar.SetUefiVar(NEXT_MFCI_POLICY_BLOB, MFCI_VENDOR_GUID, var, 3)
        if errorcode == 0:
            logging.info("Next Policy failed: {errorcode}")
        else:
            logging.info("Next Policy was set")
            print(f"{NEXT_MFCI_POLICY_BLOB} was set")
    return

def get_next_mfci_policy():
    UefiVar = UefiVariable()

    (errorcode, data) = UefiVar.GetUefiVar(NEXT_MFCI_POLICY_BLOB, MFCI_VENDOR_GUID)
    if errorcode != 0:
        logging.info("No Next Mfci Policy Set")
        print(f"No variable {NEXT_MFCI_POLICY_BLOB} found")
    else:
        logging.info(f"{data}")
        print(f"{data}")
    return

#
# main script function
#
def main():
    arguments = option_parser()

    if arguments.getsysteminfo:
        get_system_info()
        return 0

    elif arguments.retrievepolicy:
        get_current_mfci_policy()
        return 0

    elif arguments.deletepolicy:
        delete_current_mfci_policy()
        return 0

    elif arguments.policyblob != "":
        if not os.path.isfile(arguments.policyblob):
            logging.critical(f"File {arguments.policyblob} does not exist")
            return 0

        set_next_mfci_policy(arguments.policyblob)
        return 0

    elif arguments.retrievenextpolicy:
        get_next_mfci_policy()
        return 0
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
