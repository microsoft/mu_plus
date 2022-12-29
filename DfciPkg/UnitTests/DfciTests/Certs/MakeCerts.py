# @file
#
# Script to Generate the test certs required for testing Dfci.
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

Password_Dialog_None_Button_Id = 0x1f9    # Discovered using spyxx
BM_CLICK = 0xf5  # from Windows WinUser.h


class Dismiss_Password_Prompt(object):
    TaskTerminated = False

    def __init__(self):
        # import firmware variable API
        self._FindWindowW = user32.FindWindowW
        self._FindWindowW.restype = c_int
        self._FindWindowW.argtypes = [c_wchar_p, c_wchar_p]

        self._SendDlgItemMessageW = user32.SendDlgItemMessageW
        self._SendDlgItemMessageW.restype = c_int
        self._SendDlgItemMessageW.argtypes = [c_void_p, c_int, c_int, c_void_p, c_void_p]

    def FindPrompt(self):

        hwnd = self._FindWindowW(None, 'Create Private Key Password')
        return hwnd

    def SendDlgItemMessage(self, hwnd, DlgItem, msg, wparam, lparam):
        self._SendDlgItemMessageW(hwnd, DlgItem, msg, wparam, lparam)

    def terminate_task(self):
        self.TaskTerminated = True

    def is_terminated(self):
        return self.TaskTerminated


def delete_cert_files(filemask):

    fileList = glob.glob(filemask)
    # Iterate over the list of filepaths & remove each file.
    for filePath in fileList:
        print(filePath)
        os.remove(filePath)


def run_makecert(name, level):
    global makecert_path
    global pvk2pvx_path

    common_parameters = [
        makecert_path,
        '-len', '4096',
        '-m', '120',
        '-a', 'sha256',
        '-pe',
        '-ss', 'my',
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

    print(f'-------------{name} - {level}')

    for i in range(0, len(parameters)):
        if i == 0:
            print('makecert', end=' ')
        else:
            print(parameters[i], end=' ')
    print('')

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

    for i in range(0, len(parameters) - 1):
        if i == 0:
            print('pk2pvx', end=' ')
        else:
            print(parameters[i], end=' ')
    print('')

    output = subprocess.run(parameters,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT)
    if output.returncode != 0:
        raise Exception(f'Error running pvk2pvx:{output.stdout}')

    return 0


def dismiss_password_prompt_task():
    global dpp

    while True:
        hwnd = 0
        # wait for the dialog box to pop up
        while hwnd == 0 and not dpp.is_terminated():
            hwnd = dpp.FindPrompt()
            time.sleep(0.25)
        if dpp.TaskTerminated:
            break

        # click the None button (Dlg Id == 0x1f9)
        dpp._SendDlgItemMessageW(hwnd, Password_Dialog_None_Button_Id, BM_CLICK, 0, 0)

        # Wait for the dialog box to go away
        while hwnd != 0 and not dpp.is_terminated():
            hwnd = dpp.FindPrompt()


def main():
    global makecert_path
    global pvk2pvx_path
    global dpp

    dpp = Dismiss_Password_Prompt()

    rc = 0
    makecert_path = FindToolInWinSdk('MakeCert.exe')
    pvk2pvx_path = makecert_path.replace('MakeCert.exe', 'pvk2pfx.exe')

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
