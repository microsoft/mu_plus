# @file
#
# Script to Generate the test certs required for testing DFCI.
#
# Copyright (c), Microsoft Corporation
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

import glob
import logging
import os
import subprocess
import sys
import time

from ctypes import windll, c_int, c_void_p, c_wchar_p
from threading import Thread
from edk2toollib.windows.locate_tools import FindToolInWinSdk

user32 = windll.user32
makecert_path = None
pvk2pvx_path = None
dpp = None

Password_Dialog_None_Button_Id = 0x1f9    # Discovered using SPYXX from Visual Studio.
BM_CLICK = 0xf5  # from Windows WinUser.h


class DismissPasswordPrompt(object):
    TaskTerminated = False

    def __init__(self):
        # import firmware variable API
        self._FindWindowW = user32.FindWindowW
        self._FindWindowW.restype = c_int
        self._FindWindowW.argtypes = [c_wchar_p, c_wchar_p]

        self._SendDlgItemMessageW = user32.SendDlgItemMessageW
        self._SendDlgItemMessageW.restype = c_int
        self._SendDlgItemMessageW.argtypes = [c_void_p, c_int, c_int, c_void_p, c_void_p]

    def find_prompt(self):
        hwnd = self._FindWindowW(None, 'Create Private Key Password')
        return hwnd

    def send_dlg_item_message(self, hwnd, dlg_item, msg, wparam, lparam):
        self._SendDlgItemMessageW(hwnd, dlg_item, msg, wparam, lparam)

    def terminate_task(self):
        self.TaskTerminated = True

    def is_terminated(self):
        return self.TaskTerminated


def check_for_files(filemask):
    file_list = glob.glob(filemask)
    return len(file_list) != 0


def delete_cert_files(filemask):
    file_list = glob.glob(filemask)
    # Iterate over the list of file paths & remove each file.
    for file_path in file_list:
        os.remove(file_path)


def run_makecert(name, level):
    global makecert_path
    global pvk2pvx_path

    common_parameters = [
        makecert_path,
        '-len', '4096',
        '-m', '120',
        '-a', 'sha256',
        '-pe',
        '-ss', 'DfciCerts',
        '-n', f'CN={name}_{level}, O=Dfci_Test_shop, C=US',
        '-sv', f'{name}_{level}.pvk',
        ]

    if 'Root' in level:
        extended_parameters = [
            '-r',
            '-cy', 'authority',
            ]

    elif 'CA' in level:
        extended_parameters = [
            '-cy', 'authority',
            '-ic', f'{name}_Root.cer',
            '-iv', f'{name}_Root.pvk'
            ]
    elif 'Leaf' in level:
        extended_parameters = [
            '-ic', f'{name}_CA.cer',
            '-iv', f'{name}_CA.pvk',
            '-sky', 'exchange'
            ]
    else:
        raise Exception('Incorrect level({level}) specified.')

    parameters = common_parameters.copy()
    parameters.extend(extended_parameters)
    parameters.append(f'{name}_{level}.cer')

    print(f'Making cert for -----{name}_{level}')

    output = subprocess.run(parameters,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT)
    if output.returncode != 0:
        raise Exception(f'Error running makecert:{output.stdout}')

    parameters = [
        pvk2pvx_path,
        '-pvk', f'{name}_{level}.pvk',
        '-spc', f'{name}_{level}.cer',
        '-pfx', f'{name}_{level}.pfx'
        ]

    output = subprocess.run(parameters,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT)
    if output.returncode != 0:
        raise Exception(f'Error running pvk2pvx:{output.stdout}')

    return 0


def delete_dfci_certs_from_store():
    parameters = [
        'powershell',
        '-Command',
        'get-childitem',
        'Cert:\\CurrentUser\\DfciCerts'
        ]

    output = subprocess.run(parameters,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT)
    if output.returncode != 0:
        raise Exception(f'Error running powershell:{output.stdout}')

    for line in output.stdout.splitlines():
        line_segments = line.decode().split()
        if not isinstance(line_segments, list):
            continue
        if len(line_segments) < 2:
            continue
        if len(line_segments[0]) < 40:
            continue

        hash = line_segments[0]

        cert_name = 'Unknown'
        if line_segments[1].startswith('CN='):
            (cert_name, *others) = line_segments[1].split(',')
            cert_name = cert_name[3:]

        print(f'Deleting {cert_name} with hash {hash} from the system certificate store')

        cert_location = 'Cert:\\CurrentUser\\DfciCerts\\' + hash

        parameters = [
            'powershell',
            '-Command',
            'Remove-Item',
            cert_location,
            '-DeleteKey'
            ]

        output = subprocess.run(parameters,
                                stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT)
        if output.returncode != 0:
            raise Exception(f'Error running powershell:{output.stdout}')


def dismiss_password_prompt_task():
    global dpp

    while True:
        hwnd = 0
        # wait for the dialog box to pop up
        while hwnd == 0 and not dpp.is_terminated():
            hwnd = dpp.find_prompt()
            time.sleep(0.25)
        if dpp.TaskTerminated:
            break

        # click the None button (Dialog Id == 0x1f9)
        dpp.send_dlg_item_message(hwnd, Password_Dialog_None_Button_Id, BM_CLICK, 0, 0)

        # Wait for the dialog box to go away
        while hwnd != 0 and not dpp.is_terminated():
            hwnd = dpp.find_prompt()


def main():
    global makecert_path
    global pvk2pvx_path
    global dpp

    print('MakeCert will prompt for passwords.  For the test certs, a password')
    print('is not required.  This script will dismiss the dialogs by pressing')
    print('the No Password button automatically.')

    dpp = DismissPasswordPrompt()

    rc = 0
    makecert_path = FindToolInWinSdk('MakeCert.exe')
    pvk2pvx_path = makecert_path.replace('MakeCert.exe', 'pvk2pfx.exe')

    if (check_for_files('ZTD*') or
        check_for_files('DDS*') or
        check_for_files('MDM*')):
        answer = 'x'

        while (True):
            print('\nWARNING:')

            print('There are existing certs.  Are you sure you want to erase these')
            print('certs and create new ones?')
            answer = sys.stdin.readline().strip()
            if answer in ['y', 'Y']:
                break;
            if answer in ['n', 'N']:
                print('Makecerts aborted')
                return 8

    # Delete any current test certificates
    delete_cert_files('ZTD*')
    delete_cert_files('DDS*')
    delete_cert_files('MDM*')

    t1 = Thread(target=dismiss_password_prompt_task)
    t1.start()

    run_makecert('ZTD', 'Root')
    run_makecert('ZTD', 'CA')
    run_makecert('ZTD', 'Leaf')

    run_makecert('DDS', 'Root')
    run_makecert('DDS', 'CA')
    run_makecert('DDS', 'Leaf')

    run_makecert('DDS', 'CA2')
    run_makecert('DDS', 'Leaf2')

    run_makecert('MDM', 'Root')
    run_makecert('MDM', 'CA')
    run_makecert('MDM', 'Leaf')

    run_makecert('MDM', 'CA2')
    run_makecert('MDM', 'Leaf2')

    dpp.terminate_task()

    t1.join()

    # MakeCert places the certs into the Windows CurrentUser store.  Because MakeCert
    # was supplied with -ss DfciCerts, the certificates are stored under DfciCerts
    # Delete the just made certs from the certificate store as they are not needed there.
    delete_dfci_certs_from_store()

    return rc


if __name__ == '__main__':
    # setup main console as logger
    logger = logging.getLogger('')
    logger.setLevel(logging.DEBUG)
    formatter = logging.Formatter("%(levelname)s - %(message)s")
    console = logging.StreamHandler()
    console.setLevel(logging.CRITICAL)
    console.setFormatter(formatter)
    logger.addHandler(console)

    # call main worker function
    retcode = main()

    if retcode != 0:
        logging.critical("Failed.  Return Code: %i" % retcode)

    # end logging
    logging.shutdown()
    sys.exit(retcode)
