# @file
#
# Script to Generate a Device Firmware Configuration Interface Permission Provisiong Blob
#
# Copyright (c), Microsoft Corporation
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

##
## Script to Generate a Device Firmware Configuration Interface Permission Provisiong Blob
## This tool takes in a XML file in PermissionPacket format, packages it in a
## DFCI_PERMISSION_POLICY_APPLY_VAR structure, signs it with the
## requested key, and then attaches the signature data in WIN_CERTIFICATE_UEFI_GUID format.
##
## This binary file can then be written to variable store:
## GUID: gDfciPermissionManagerVarNamespace
## NAME: DFCI_PERMISSION_POLICY_APPLY_VAR_NAME    L"DfciPermissionApply"
##
## THIS IS FOR UNIT TEST
##
## General process:
##   Phase 1: Create payload file by combining relevant info
##   Phase 2: Sign it using signtool
##   Phase 3: Parse signature into WIN_CERT and package to create final output
##

import os, sys
import argparse
import logging
import datetime
import struct
import shutil
import time
import random

#get script path
sp = os.path.dirname(os.path.realpath(sys.argv[0]))

#setup python path for build modules
sys.path.append(sp)

from DFCI_SupportLib import DFCI_SupportLib

from edk2toollib.uefi.wincert import *
from edk2toollib.utility_functions import DetachedSignWithSignTool
from edk2toollib.windows.locate_tools import FindToolInWinSdk
from Data.PermissionPacketVariable import PermissionApplyVariable
from Data.PermissionPacketVariable import PermissionResultVariable


#PKCS7 Signed Data OID
gOid = "1.2.840.113549.1.7.2"
gPath2SignTool = None


def PrintSEM(filepath):
    if(filepath and os.path.isfile(filepath)):
        s = open(filepath, "rb")
        SEM = PermissionApplyVariable(s)
        s.close()

        #now print it out.
        SEM.Print()

def PrintSEMResults(filepath):
    if(filepath and os.path.isfile(filepath)):
        s = open(filepath, "rb")
        SEM = PermissionResultVariable(s)
        s.close()

        #now print it out.
        SEM.Print()

def PrintSEMCurrent(filepath):
    if(filepath and os.path.isfile(filepath)):
        outfilename = os.path.basename(filepath) + "_Current" + ".xml"
        a = DFCI_SupportLib ()
        a.extract_payload_from_current(filepath, outfilename)

def SignSEMData(options):
    global gPath2SignTool
    if gPath2SignTool == None:
        a = DFCI_SupportLib ()
        gPath2SignTool = a.get_signtool_path ()

    return DetachedSignWithSignTool (gPath2SignTool, options.SigningInputFile, options.SigningOutputFile,  options.SigningPfxFile, options.SigningPfxPw, gOid)

#
#main script function
#
def main():
    parser = argparse.ArgumentParser(description='Create SEM Permission Packet Variable')

    #Output debug log
    parser.add_argument("-l", dest="OutputLog", help="Create an output log file: ie -l out.txt", default=None)
    parser.add_argument("-p", dest="PrintFile", help="Print File as Permission Blob", default= None)
    parser.add_argument("-pr", dest="PrintResultsFile", help="Print Result File as Permission Blob", default= None)
    parser.add_argument("-pc", dest="PrintCurrentFile", help="Print Current File to {basename}_Current.xml", default= None)
    parser.add_argument("--dirty", action="store_true", dest="dirty", help="Leave around the temp files after finished", default=False)

    Step1Group = parser.add_argument_group(title="Step1", description="Signed Data Prep.  Build data structure.")
    Step1Group.add_argument("--Step1Enable", dest="Step1Enable", help="Do Step 1 - Signed Data Prep", default=False, action="store_true")
    Step1Group.add_argument("--SnTarget", dest="SnTarget", help="Target to only a device with given Serial Number in decimal. Zero means all devices", default=0)
    Step1Group.add_argument("--XmlFilePath", dest="XmlFilePath", help="Path to Xml Permission Packet File", default=None)
    Step1Group.add_argument("--PrepResultFile", dest="PrepResultFile", help="Optional File for output from Step1. Required if not doing step2", default=None)
    Step1Group.add_argument("--HdrVersion", dest="HdrVersion", help="Specify packet version",  default= PermissionApplyVariable.VERSION_V1)
    Step1Group.add_argument("--SMBIOSMfg", dest="SMBIOSMfg", help="Specify SMBIOS Manufacturer",  default=None)
    Step1Group.add_argument("--SMBIOSProd", dest="SMBIOSProd", help="Specify SMBIOS Product Name",  default=None)
    Step1Group.add_argument("--SMBIOSSerial", dest="SMBIOSSerial", help="Specify SMBIOS Serial Number",  default=None)

    Step2Group = parser.add_argument_group(title="Step2", description="Signature Generation Step.")
    Step2Group.add_argument("--Step2Enable", dest="Step2Enable", help="Do Step 2 - Local Signing", default=False, action="store_true")
    #need to add arguments here for signing.  signtool path and parameters
    Step2Group.add_argument("--SigningInputFile", dest="SigningInputFile", help="Optional File for intput for Step2.  Required if not doing step1", default=None)
    Step2Group.add_argument("--SigningResultFile", dest="SigningResultFile", help="Optional File for output from Step2. Required if not doing step3", default=None)
    Step2Group.add_argument("--SigningPfxFile", dest="SigningPfxFile", help="Path to PFX file for signing", default=None)
    Step2Group.add_argument("--SigningPfxPw", dest="SigningPfxPw", help="Optional Password for PFX file for signing", default=None)

    Step3Group = parser.add_argument_group(title="Step3", description="Final Var Construction.")
    Step3Group.add_argument("--Step3Enable", dest="Step3Enable", help="Do Step 3 - Final Provisioning Var Construction", default=False, action="store_true")
    Step3Group.add_argument("--FinalizeInputFile", dest="FinalizeInputFile", help="Optional if doing Step2. Generally Step1 Output or Step2 input.  ", default=None)
    Step3Group.add_argument("--FinalizeInputDetachedSignatureFile", dest="FinalizeInputDetachedSignatureFile", help="Signtool Detached Signature File.  Optional if doing Step2", default=None)
    Step3Group.add_argument("--FinalizeResultFile", dest="FinalizeResultFile", help="File for output from Step3.  Complete SEM Provisioning Var File.", default=None)

    #Turn on debug level logging
    parser.add_argument("--debug", action="store_true", dest="debug", help="turn on debug logging level for file log",  default=False)
    options = parser.parse_args()

    #setup file based logging if outputReport specified
    if(options.OutputLog):
        if(len(options.OutputLog) < 2):
            logging.critical("the output log file parameter is invalid")
            return -2
        else:
            #setup file based logging
            filelogger = logging.FileHandler(filename=options.OutputLog, mode='w')
            if(options.debug):
                filelogger.setLevel(logging.DEBUG)
            else:
                filelogger.setLevel(logging.INFO)

            filelogger.setFormatter(formatter)
            logging.getLogger('').addHandler(filelogger)

    logging.info("Log Started: " + datetime.datetime.strftime(datetime.datetime.now(), "%A, %B %d, %Y %I:%M%p" ))

    #Step 1 Prep
    if(options.Step1Enable):
        logging.debug("Step 1 Enabled")
        if(not options.XmlFilePath) or (not os.path.isfile(options.XmlFilePath)):
            logging.critical("For Step1 there must be a valid XML Permission file")
            return -2

        if(not options.Step2Enable):
            #must have output file
            if(not options.PrepResultFile):
                logging.critical("Since Step2 is not enabled there must be a PrepResultFile for the result")
                return -3

        if(options.PrepResultFile):
            logging.debug("Step 1 Result will be written to: " + options.PrepResultFile)

        if(options.SigningInputFile):
            logging.critical("Since Step1 is enabled an Input File for Step2 is not allowed")
            return -11

    #Step 2 signing
    if(options.Step2Enable):
        logging.debug("Step 2 Enabled")
        if(not options.SigningPfxFile):
            logging.critical("Since Step2 is enabled you must supply a path to a PFX file for signing")
            return -10

        if(not options.Step1Enable) and ((not options.SigningInputFile) or (not os.path.isfile(options.SigningInputFile))):
            logging.critical("For Step2 you must do Step1 or have a valid SigningInputFile")
            return -4

        if(not options.Step3Enable):
            #must have output file
            if(not options.SigningResultFile):
                logging.critical("Since Step3 is not enabled there must be a SigningResultFile for the result")
                return -5
            if(options.SigningResultFile):
                logging.debug("Step2 Result will be written to: " + options.SigningResultFile)

        if(options.FinalizeInputDetachedSignatureFile):
            logging.critical("Since Step2 is enabled an Input Detached signature file for Step3 is not allowed")
            return -13

        if(options.FinalizeInputFile):
            logging.critical("Since Step2 is enabled an Input file for Step3 is not allowed")
            return -14

    #Step 3 Finalize
    if(options.Step3Enable):
        logging.debug("Step 3 Enabled")

        if(not options.Step2Enable) and (options.Step1Enable):
            logging.critical("Can't have only Step1 and 3 Enabled")
            return -12

        if(not options.Step2Enable) and ((not options.FinalizeInputFile) or (not os.path.isfile(options.FinalizeInputFile)) or (not options.FinalizeInputDetachedSignatureFile) or (not os.path.isfile(options.FinalizeInputDetachedSignatureFile))):
            logging.critical("For Step3 you must do Step2 or have a valid FinalizeInputFile and FinalizeInputDetachedSignatureFile")
            return -6

        #must have an output file
        if(not options.FinalizeResultFile):
            logging.critical("For Step3 you must have a FinalizeResultFile")
            return -7
        else:
            logging.debug("Step3 Result will be written to: " + options.FinalizeResultFile)


    tempdir = "_temp_" + str(time.time())
    logging.critical("Temp directory is: " + os.path.join(os.getcwd(), tempdir))
    os.makedirs(tempdir)

    #STEP 1 - Prep Var
    if(options.Step1Enable):
        logging.critical("Step1 Started")
        Step1OutFile = os.path.join(tempdir, "Step1Out.bin")
        SEM = PermissionApplyVariable(None, int(options.HdrVersion))

        if (int(options.HdrVersion) ==  PermissionApplyVariable.VERSION_V1):
            SEM.SNTarget = int(options.SnTarget);
        elif (int(options.HdrVersion) ==  PermissionApplyVariable.VERSION_V2):
            if options.SMBIOSMfg == None:
                SEM.Manufacturer = "OEMSH"
            else:
                SEM.Manufacturer = options.SMBIOSMfg

            if options.SMBIOSProd == None:
                SEM.ProductName = "OEMSH Product"
            else:
                SEM.ProductName = options.SMBIOSProd

            if options.SMBIOSSerial == None:
                SEM.SerialNumber = "789789789"
            else:
                SEM.SerialNumber = options.SMBIOSSerial
        else:
            logging.critical("Invalid header version specified")
            return -31

        a = open(options.XmlFilePath, "r")
        SEM.AddXmlPayload(a.read())
        a.close()

        of = open(Step1OutFile, "wb")
        SEM.Write(of)
        of.close()

        #if user requested a step1 output file copy the temp file
        if(options.PrepResultFile):
            shutil.copy(Step1OutFile, options.PrepResultFile)

        #setup input for Step2
        options.SigningInputFile = Step1OutFile


    #STEP 2 - Local sign
    if(options.Step2Enable):
        logging.critical("Step2 Started")
        #copy signinginputfile into temp dir
        FileToSign = os.path.join(tempdir, "Step2In.bin")
        shutil.copy(options.SigningInputFile, FileToSign)
        options.SigningInputFile = FileToSign
        options.SigningOutputFile = os.path.join(tempdir, "Step2Signature.bin")

        #do local signature
        ret = SignSEMData(options)
        if(ret != 0):
            logging.critical("SignSEMData (Step2) Failed: " + str(ret))
            return ret

        if(options.SigningResultFile):
            shutil.copy(options.SigningOutputFile, options.SigningResultFile)

        #setup input for Step3
        options.FinalizeInputFile = options.SigningInputFile
        options.FinalizeInputDetachedSignatureFile = options.SigningOutputFile


    #STEP 3 - Write Signature Structure and complete file
    if(options.Step3Enable):
        logging.critical("Step3 Started")
        sstep1file = open(options.FinalizeInputFile, "rb")
        SEM = PermissionApplyVariable(sstep1file)
        sstep1file.close()
        SEM.Signature = WinCertUefiGuid()
        detached = open(options.FinalizeInputDetachedSignatureFile, "rb")
        SEM.Signature.AddCertData(detached)
        detached.close()
        SEM.SessionId = random.randint(0, 4294967295) #generate a random session id

        if(not options.FinalizeResultFile):
            options.FinalizeResultFile = os.path.join(tempdir, "Step3Out.bin")

        of = open(options.FinalizeResultFile, "wb")
        SEM.Write(of)
        of.close()

    #
    # Function to print SEM
    #
    if(options.PrintFile) and (os.path.isfile(options.PrintFile)):
        PrintSEM(options.PrintFile)

    if(options.PrintResultsFile) and (os.path.isfile(options.PrintResultsFile)):
        PrintSEMResults(options.PrintResultsFile)

    if(options.PrintCurrentFile) and (os.path.isfile(options.PrintCurrentFile)):
        PrintSEMCurrent(options.PrintCurrentFile)

    #clean up if user didn't request to leave around
    if(not options.dirty):
        shutil.rmtree(tempdir)

    return 0


if __name__ == '__main__':
    #setup main console as logger
    logger = logging.getLogger('')
    logger.setLevel(logging.DEBUG)
    formatter = logging.Formatter("%(levelname)s - %(message)s")
    console = logging.StreamHandler()
    console.setLevel(logging.CRITICAL)
    console.setFormatter(formatter)
    logger.addHandler(console)

    #call main worker function
    retcode = main()

    if retcode != 0:
        logging.critical("Failed.  Return Code: %i" % retcode)
    #end logging
    logging.shutdown()
    sys.exit(retcode)
