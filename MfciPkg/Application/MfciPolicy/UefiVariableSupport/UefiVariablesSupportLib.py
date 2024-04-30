# @file
#
# Python lib to support Reading and writing UEFI variables from windows
#
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent

from ctypes import (
    windll,
    c_wchar_p,
    c_void_p,
    c_int,
    c_char,
    create_string_buffer,
    WinError,
)
import logging
from win32 import win32api
from win32 import win32process
from win32 import win32security

kernel32 = windll.kernel32
EFI_VAR_MAX_BUFFER_SIZE = 1024 * 1024


class UefiVariable(object):
    def __init__(self):
        # enable required SeSystemEnvironmentPrivilege privilege
        privilege = win32security.LookupPrivilegeValue(
            None, "SeSystemEnvironmentPrivilege"
        )
        token = win32security.OpenProcessToken(
            win32process.GetCurrentProcess(),
            win32security.TOKEN_READ | win32security.TOKEN_ADJUST_PRIVILEGES,
        )
        win32security.AdjustTokenPrivileges(
            token, False, [(privilege, win32security.SE_PRIVILEGE_ENABLED)]
        )
        win32api.CloseHandle(token)

        # import firmware variable API
        try:
            self._GetFirmwareEnvironmentVariable = (
                kernel32.GetFirmwareEnvironmentVariableW
            )
            self._GetFirmwareEnvironmentVariable.restype = c_int
            self._GetFirmwareEnvironmentVariable.argtypes = [
                c_wchar_p,
                c_wchar_p,
                c_void_p,
                c_int,
            ]
            self._SetFirmwareEnvironmentVariable = (
                kernel32.SetFirmwareEnvironmentVariableW
            )
            self._SetFirmwareEnvironmentVariable.restype = c_int
            self._SetFirmwareEnvironmentVariable.argtypes = [
                c_wchar_p,
                c_wchar_p,
                c_void_p,
                c_int,
            ]
            self._SetFirmwareEnvironmentVariableEx = (
                kernel32.SetFirmwareEnvironmentVariableExW
            )
            self._SetFirmwareEnvironmentVariableEx.restype = c_int
            self._SetFirmwareEnvironmentVariableEx.argtypes = [
                c_wchar_p,
                c_wchar_p,
                c_void_p,
                c_int,
                c_int,
            ]
        except:
            logging.warn(
                "G[S]etFirmwareEnvironmentVariableW function doesn't seem to exist"
            )
            pass

    #
    # Helper function to create buffer for var read/write
    #
    def CreateBuffer(self, init, size=None):
        """CreateBuffer(aString) -> character array
        CreateBuffer(anInteger) -> character array
        CreateBuffer(aString, anInteger) -> character array
        """
        if isinstance(init, str):
            if size is None:
                size = len(init) + 1
            buftype = c_char * size
            buf = buftype()
            buf.value = init
            return buf
        elif isinstance(init, int):
            buftype = c_char * init
            buf = buftype()
            return buf
        raise TypeError(init)

    #
    # Function to get variable
    # return a tuple of error code and variable data as string
    #
    def GetUefiVar(self, name, guid):
        # success
        err = 0
        efi_var = create_string_buffer(EFI_VAR_MAX_BUFFER_SIZE)
        if self._GetFirmwareEnvironmentVariable is not None:
            logging.info(
                "calling GetFirmwareEnvironmentVariable( name='%s', GUID='%s' ).."
                % (name, "{%s}" % guid)
            )
            length = self._GetFirmwareEnvironmentVariable(
                name, "{%s}" % guid, efi_var, EFI_VAR_MAX_BUFFER_SIZE
            )
        if (0 == length) or (efi_var is None):
            err = kernel32.GetLastError()
            logging.error(
                "GetFirmwareEnvironmentVariable[Ex] failed (GetLastError = 0x%x)" % err
            )
            logging.error(WinError())
            return (err, None, WinError(err))
        return (err, efi_var[:length], None)

    #
    # Function to set variable
    # return a tuple of boolean status, error_code, error_string (None if not error)
    #
    def SetUefiVar(self, name, guid, var=None, attrs=None):
        var_len = 0
        err = 0
        error_string = None
        if var is None:
            var = bytes(0)
        else:
            var_len = len(var)
        success = 0  # Fail
        if attrs is None:
            if self._SetFirmwareEnvironmentVariable is not None:
                logging.info(
                    "Calling SetFirmwareEnvironmentVariable (name='%s', Guid='%s')..."
                    % (
                        name,
                        "{%s}" % guid,
                    )
                )
                success = self._SetFirmwareEnvironmentVariable(
                    name, "{%s}" % guid, var, var_len
                )
        else:
            attrs = int(attrs)
            if self._SetFirmwareEnvironmentVariableEx is not None:
                logging.info(
                    "Calling SetFirmwareEnvironmentVariableEx( name='%s', GUID='%s', length=0x%X, attributes=0x%X ).."
                    % (name, "{%s}" % guid, var_len, attrs)
                )
                success = self._SetFirmwareEnvironmentVariableEx(
                    name, "{%s}" % guid, var, var_len, attrs
                )

        if 0 == success:
            err = kernel32.GetLastError()
            logging.error(
                "SetFirmwareEnvironmentVariable failed (GetLastError = 0x%x)" % err
            )
            logging.error(WinError())
            error_string = WinError(err)
        return (success, err, error_string)
