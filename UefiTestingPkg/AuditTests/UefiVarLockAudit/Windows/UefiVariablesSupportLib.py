##
## Python lib to support Reading and writing UEFI variables from windows 
##
#
#
# Copyright (c) 2016, Microsoft Corporation
#
# All rights reserved.
# Redistribution and use in source and binary forms, with or without 
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
##

import os, sys
from ctypes import *
import logging
import pywintypes
import win32api, win32process, win32security, win32file
import winerror

kernel32 = windll.kernel32
EFI_VAR_MAX_BUFFER_SIZE = 1024*1024

class UefiVariable(object):

    def __init__(self):
        # enable required SeSystemEnvironmentPrivilege privilege
        privilege = win32security.LookupPrivilegeValue( None, 'SeSystemEnvironmentPrivilege' )
        token = win32security.OpenProcessToken( win32process.GetCurrentProcess(), win32security.TOKEN_READ|win32security.TOKEN_ADJUST_PRIVILEGES )
        win32security.AdjustTokenPrivileges( token, False, [(privilege, win32security.SE_PRIVILEGE_ENABLED)] )
        win32api.CloseHandle( token )

        # get windows firmware variable API
        try:
            self._GetFirmwareEnvironmentVariable = kernel32.GetFirmwareEnvironmentVariableW
            self._GetFirmwareEnvironmentVariable.restype = c_int
            self._GetFirmwareEnvironmentVariable.argtypes = [c_wchar_p, c_wchar_p, c_void_p, c_int]
            self._SetFirmwareEnvironmentVariable = kernel32.SetFirmwareEnvironmentVariableW
            self._SetFirmwareEnvironmentVariable.restype = c_int
            self._SetFirmwareEnvironmentVariable.argtypes = [c_wchar_p, c_wchar_p, c_void_p, c_int]
            self._SetFirmwareEnvironmentVariableEx = kernel32.SetFirmwareEnvironmentVariableExW
            self._SetFirmwareEnvironmentVariableEx.restype = c_int
            self._SetFirmwareEnvironmentVariableEx.argtypes = [c_wchar_p, c_wchar_p, c_void_p, c_int, c_int]
        except AttributeError:
            logging.warn( "Some get/set functions don't't seem to exist" )

    #
    #Function to get variable
    # return a tuple of error code and variable data as string
    #
    def GetUefiVar(self, name, guid ):
        err = 0 #success
        efi_var = create_string_buffer( EFI_VAR_MAX_BUFFER_SIZE )
        if self._GetFirmwareEnvironmentVariable is not None:
            logging.info("calling GetFirmwareEnvironmentVariable( name='%s', GUID='%s' ).." % (name, "{%s}" % guid) )
            length = self._GetFirmwareEnvironmentVariable( name, "{%s}" % guid, efi_var, EFI_VAR_MAX_BUFFER_SIZE )
        if (0 == length) or (efi_var is None):
            err = kernel32.GetLastError()
            logging.error( 'GetFirmwareEnvironmentVariable[Ex] failed (GetLastError = 0x%x)' %  err)
            logging.error(WinError())
            return (err, None, WinError(err))
        return (err, efi_var[:length], None)
    #
    #Function to set variable
    # return a tuple of boolean status, errorcode, errorstring (None if not error)
    #
    def SetUefiVar(self, name, guid, var=None, attrs=None):
            var_len = 0
            err = 0
            errorstring = None
            if var is None: 
                var = bytes(0)
            else: 
                var_len = len(var)
            if(attrs == None):
                if self._SetFirmwareEnvironmentVariable is not None:
                    logging.info("Calling SetFirmwareEnvironmentVariable (name='%s', Guid='%s')..." % (name, "{%s}" % guid, ))
                    success = self._SetFirmwareEnvironmentVariable(name, "{%s}" % guid, var, var_len)
            else:
                if self._SetFirmwareEnvironmentVariableEx is not None:
                    logging.info(" calling SetFirmwareEnvironmentVariableEx( name='%s', GUID='%s', length=0x%X, attributes=0x%X ).." % (name, "{%s}" % guid, var_len, attrs) )
                    success = self._SetFirmwareEnvironmentVariableEx( name, "{%s}" % guid, var, var_len, attrs )
                
            if 0 == success:
                err = kernel32.GetLastError()
                logging.error('SetFirmwareEnvironmentVariable failed (GetLastError = 0x%x)' % err )
                logging.error(WinError())
                errorstring = WinError(err)
            return (success,err, errorstring)


    
#Test code
#UefiVar = UefiVariable()
#(errorcode, data, errorstring) = UefiVar.GetUefiVar('PK', '8BE4DF61-93CA-11D2-AA0D-00E098032B8C')
#(status, errorcode, errorstring) = UefiVar.SetUefiVar('PK','8BE4DF61-93CA-11D2-AA0D-00E098032B8C',None, None) 