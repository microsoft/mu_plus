# @file
#
# Cert Supoport functions
#
# Copyright (c), Microsoft Corporation
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

##
## Obtain the SHA1 value from a .pfx file
##

import os, sys
import traceback
import argparse
import base64
import datetime
import logging
import shutil
import threading
import subprocess
import tempfile

from DFCI_SupportLib import DFCI_SupportLib
from edk2toollib.utility_functions import RunCmd

CertMgrPath = None

class CertSupportLib(object):

    def get_thumbprint_from_pfx(self, pfxfilename=None):
        global CertMgrPath

        if pfxfilename == None:
            raise Exception ("Pfx File Name is required")
        fp = tempfile.NamedTemporaryFile (delete=False);

        tfile = fp.name
        fp.close()

        try:
            #
            # Cert Manager is used for deleting the cert when add/removing certs
            #

            #1 - use Certmgr to get the PFX sha1 thumbprint
            if CertMgrPath is None:
                CertMgrPath = DFCI_SupportLib().get_certmgr_path()

            parameters = " /c " + pfxfilename
            ret = RunCmd (CertMgrPath, parameters, outfile=tfile)
            if(ret != 0):
                logging.critical("Failed to get cert info from Pfx file using CertMgr.exe")
                return ret
            f = open(tfile, "r")

            pfxdetails = f.readlines()
            f.close()
            os.remove(tfile)
            #2 Parse the pfxdetails for the sha1 thumbprint
            thumbprint = ""
            found = False
            for a in pfxdetails:
                a = a.strip()
                if(len(a)):
                    if(found):
                        thumbprint = ''.join(a.split())
                        break
                    else:
                        if(a == "SHA1 Thumbprint::"):
                            found = True

            if(len(thumbprint) != 40) or (found == False):
                return 'No thumbprint'

        except Exception as exp:
            traceback.print_exc()
            return "Unable to read certificate"

        return thumbprint
