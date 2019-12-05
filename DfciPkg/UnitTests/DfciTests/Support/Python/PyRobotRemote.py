# @file
#
# PyRobotRemote - Runs on the System Under Test (DUT) providing
#                 functionality needed for DFCI testing
#
# Copyright (c), Microsoft Corporation
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

import os
import subprocess
import logging
import shlex

from Lib.UefiVariablesSupportLib import UefiVariable


#update this whenever you make a change
RobotRemoteChangeDate       = "2019-08-14 08:30"
RobotRemoteVersion          = 1.03


class UefiRemoteTesting(object):
    """Library to be used with Robot Framework's remote server.

    This supports Robot Framework Remote Interface.  This lets a remote test system perform local operations using
    remote framework and built in remote "Keywords"
    """
    def __init__(self):
        self.filepath = None
        self.lines = []


    def Run_Command_And_Return_Output(self, cmdline):
        cmd = shlex.split(cmdline)
        p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        p.wait()
        out = p.communicate()[0]
        rc = p.returncode
        lines = out.splitlines()

        if rc != 0 or len(lines) < 3:
            return "Error"
        else:
            return lines[2].decode('utf-8').strip()

    #
    # String variables are deisgned to have a NULL.  This does
    # confuse Python, so get rid of the NULL when it is expected
    #
    def GetUefiVariable(self, name, guid, trim):
        UefiVar = UefiVariable();
        logging.info("Calling GetUefiVar ( name='%s', GUID='%s')" % (name, "{%s}" % guid) )
        (rc,var,errorstring) = UefiVar.GetUefiVar (name, guid)
        var2 = var
        if (var is not None) and (trim == 'trim'):
            varlen = len(var)
            if varlen > 1:
                var2 = var[0:varlen-1]
        return (rc,var2,errorstring)

    def SetUefiVariable(self, name, guid, attrs=None, contents=None):
        UefiVar = UefiVariable();
        (rc,err,errorstring) = UefiVar.SetUefiVar (name, guid, contents, attrs);
        return rc

    def remote_ack(self):
        return True

    def remote_get_version(self):
        return RobotRemoteVersion

    def remote_warm_reboot(self):
        os.system("shutdown -r -t 1")


if __name__ == '__main__':
    from robotremoteserver import RobotRemoteServer
    print ("Version %s - %s" % (str(RobotRemoteVersion), RobotRemoteChangeDate))

    #setup main console as logger
    logger = logging.getLogger('')
    logger.setLevel(logging.DEBUG)
    formatter = logging.Formatter("%(levelname)s - %(message)s")
    console = logging.StreamHandler()
    console.setLevel(logging.CRITICAL)
    console.setFormatter(formatter)
    logger.addHandler(console)

    RobotRemoteServer(UefiRemoteTesting(), host='0.0.0.0', port=8270)

    logging.shutdown()
    sys.exit(retcode)