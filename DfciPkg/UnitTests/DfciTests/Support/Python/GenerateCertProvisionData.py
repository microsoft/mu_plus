# @file
#
# Script to Generate a Device Firmware Configuration Interface Provisiong Blob
#
# Copyright (c), Microsoft Corporation
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

##
## Script to Generate a Device Firmware Configuration Interface Provisiong Blob
## This tool takes in a CER file in binary encoding, packages it in a
## DFCI_SIGNER_PROVISION_APPLY_VAR structure, signs it with the
## requested key, and then attaches the signature data in WIN_CERTIFICATE_UEFI_GUID format.
##
## This binary file can then be written to variable store:
## GUID: gDfciAuthProvisionVarNamespace
## NAME: DFCI_IDENTITY_APPLY_VAR_NAME    L"DfciIdentityApply"
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
import hashlib

#get script path
sp = os.path.dirname(os.path.realpath(sys.argv[0]))

#setup python path for build modules
sys.path.append(sp)

from DFCI_SupportLib import DFCI_SupportLib

from Data.CertProvisioningVariable import CertProvisioningApplyVariable
from Data.CertProvisioningVariable import CertProvisioningResultVariable
from edk2toollib.uefi.wincert import *
from edk2toollib.windows.locate_tools import FindToolInWinSdk
from edk2toollib.utility_functions import DetachedSignWithSignTool

#PKCS7 Signed Data OID
gOid = "1.2.840.113549.1.7.2"
gPath2SignTool = None

def PrintSEM(filepath):
    if(filepath and os.path.isfile(filepath)):
        s = open(filepath, "rb")
        SEM = CertProvisioningApplyVariable(s)
        s.close()

        #now print it out.
        SEM.Print()

def ExtractCert(filepath):
    if(filepath and os.path.isfile(filepath)):
        s = open(filepath, "rb")
        SEM = CertProvisioningApplyVariable(s)
        s.close()

        #now write the certificate out.
        (certtype, a, b) = SEM.GetCertType().partition(' ')

        certfilename = os.path.basename(filepath) + "_" + certtype + ".cer"
        s = open(certfilename, "wb")
        SEM.WriteCert(s)
        s.close()
        s = open(certfilename, "rb")
        m = hashlib.new("sha1",s.read())
        s.close()

        # return the sha1 (Thumbprint) of the certificate.
        return m.digest().hex()

def PrintSEMCurrent(filepath):
    if(filepath and os.path.isfile(filepath)):
        outfilename = os.path.basename(filepath) + "_Current" + ".xml"
        a = DFCI_SupportLib ()
        a.extract_payload_from_current(filepath, outfilename)

def PrintSEMResults(filepath):
    if(filepath and os.path.isfile(filepath)):
        s = open(filepath, "rb")
        SEM = CertProvisioningResultVariable(s)
        s.close()

        #now print it out.
        SEM.Print()

def SignSEMProvisionData(options):
    global gPath2SignTool
    if gPath2SignTool == None:
        a = DFCI_SupportLib ()
        gPath2SignTool = a.get_signtool_path ()

    logging.critical("Signing Started")
    logging.critical(options.SigningInputFile)
    logging.critical(options.SigningOutputFile)
    logging.critical(options.SigningPfxFile)

    return DetachedSignWithSignTool (gPath2SignTool, options.SigningInputFile, options.SigningOutputFile,  options.SigningPfxFile, options.SigningPfxPw, gOid)

def TestSignSemTrustedCert(options):
    global gPath2SignTool
    if gPath2SignTool == None:
        a = DFCI_SupportLib ()
        gPath2SignTool = a.get_signtool_path ()

    logging.critical("Signing Started")
    logging.critical(gPath2SignTool)
    logging.critical(options.CertFilePath)
    logging.critical(options.Signing2AOutputFile)
    logging.critical(options.Signing2APfxFile)

    return DetachedSignWithSignTool (gPath2SignTool, options.CertFilePath, options.Signing2AOutputFile,  options.Signing2APfxFile, options.Signing2APfxPw, gOid)

def is_32bit_number(s):
    try:
        float(s)
        if s < 4294967296:
            return True
        else:
            return False
    except ValueError:
        return False


#
#main script function
#
def main():
    parser = argparse.ArgumentParser(description='Create SEM Provisioning Cert')

    #Output debug log
    parser.add_argument("-l", dest="OutputLog", help="Create an output log file: ie -l out.txt", default=None)
    parser.add_argument("-p", dest="PrintFile", help="Print File as Provisioning Blob", default= None)
    parser.add_argument("-pr", dest="PrintResultsFile", help="Print Result File as Identity Blob", default= None)
    parser.add_argument("-pc", dest="PrintCurrentFile", help="Print Current File as {basename}_Current.xml", default= None)
    parser.add_argument("-xc", dest="ExtractCertFile", help="Extract the certificate to {basename}_{certtype}.cer", default=None)
    parser.add_argument("--dirty", action="store_true", dest="dirty", help="Leave around the temp files after finished", default=False)

    Step1Group = parser.add_argument_group(title="Step1", description="Signed Data Prep.  Build data structure.")
    Step1Group.add_argument("--Step1Enable", dest="Step1Enable", help="Do Step 1 - Signed Data Prep", default=False, action="store_true")
    Step1Group.add_argument("--Identity", dest="Identity", help="Identity (Owner=1, User=2, User1=3, User2=4, Ztc=5).  Default is Owner", default=1)
    Step1Group.add_argument("--SnTarget", dest="SnTarget", help="Target to only a device with given Serial Number in decimal. Zero means all devices", default=0)
    Step1Group.add_argument("--CertFilePath", dest="CertFilePath", help="Path to binary DER Cert", default=None)
    Step1Group.add_argument("--PrepResultFile", dest="PrepResultFile", help="Optional File for output from Step1. Required if not doing step2 or step2A", default=None)
    Step1Group.add_argument("--HdrVersion", dest="HdrVersion", help="Specify packet version",  default=CertProvisioningApplyVariable.VERSION_V1)
    Step1Group.add_argument("--SMBIOSMfg", dest="SMBIOSMfg", help="Specify SMBIOS Manufacturer",  default=None)
    Step1Group.add_argument("--SMBIOSProd", dest="SMBIOSProd", help="Specify SMBIOS Product Name",  default=None)
    Step1Group.add_argument("--SMBIOSSerial", dest="SMBIOSSerial", help="Specify SMBIOS Serial Number",  default=None)
    Step1Group.add_argument("--Version", dest="Version", help="Specify Identity version",  default=0)
    Step1Group.add_argument("--Lsv", dest="Lsv", help="Specify the lowest supported version",  default=0)

    Step2AGroup = parser.add_argument_group(title="Step2A", description="Test Signature Generation Step.")
    Step2AGroup.add_argument("--Step2AEnable", dest="Step2AEnable", help="Do Step 2A - Local Signing for Test Signature", default=False, action="store_true")
    #need to add arguments here for signing.
    Step2AGroup.add_argument("--Signing2AResultFile", dest="Signing2AResultFile", help="Optional File for output from Step2A. Required if not doing step2B", default=None)
    Step2AGroup.add_argument("--Signing2APfxFile", dest="Signing2APfxFile", help="Path to PFX file for signing Test Signature ", default=None)
    Step2AGroup.add_argument("--Signing2APfxPw", dest="Signing2APfxPw", help="Optional Password for PFX file for signing Test Signature", default=None)

    Step2BGroup = parser.add_argument_group(title="Step2B", description="Package Cert Provision With Test Signature.")
    Step2BGroup.add_argument("--Step2BEnable", dest="Step2BEnable", help="Do Step 2B - Package Test Signature with Cert Provision Data", default=False, action="store_true")
    Step2BGroup.add_argument("--TestSignatureInputFile", dest="TestSignatureInputFile", help="Detached Signature file for Test Signature", default=None)
    Step2BGroup.add_argument("--CertProvisionBlobInputFile", dest="CertProvisionBlobAfterStep1File", help="Step1 Output File to use as Input to combine with Test Signature", default=None)
    Step2BGroup.add_argument("--Prep2BResultFile", dest="Prep2BResultFile", help="Optional File for output from Step2B. Required if not doing step2", default=None)


    Step2Group = parser.add_argument_group(title="Step2", description="Signature Generation Step.")
    Step2Group.add_argument("--Step2Enable", dest="Step2Enable", help="Do Step 2 - Local Signing", default=False, action="store_true")
    #need to add arguments here for signing.
    Step2Group.add_argument("--SigningInputFile", dest="SigningInputFile", help="Optional File for intput for Step2.  Required if not doing step1", default=None)
    Step2Group.add_argument("--SigningResultFile", dest="SigningResultFile", help="Optional File for output from Step2. Required if not doing step3", default=None)
    Step2Group.add_argument("--SigningPfxFile", dest="SigningPfxFile", help="Path to PFX file for signing", default=None)
    Step2Group.add_argument("--SigningPfxPw", dest="SigningPfxPw", help="Optional Password for PFX file for signing", default=None)

    Step3Group = parser.add_argument_group(title="Step3", description="Final Provisioning Var Construction.")
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
        if(not options.CertFilePath) or (not os.path.isfile(options.CertFilePath)):
            logging.critical("Not CertFilePath.  That means we are un-enrolling")

        if(not options.Step2Enable) and (not options.Step2AEnable):
            #must have output file
            if(not options.PrepResultFile):
                logging.critical("Since Step2A/2 is not enabled there must be a PrepResultFile for the result")
                return -3

        if(options.PrepResultFile):
            logging.debug("Step 1 Result will be written to: " + options.PrepResultFile)

        if(options.SigningInputFile):
            logging.critical("Since Step1 is enabled an Input File for Step2 is not allowed")
            return -11

    #Step 2A Test Signature Generation
    if(options.Step2AEnable):
        logging.debug("Step 2A Enabled")
        if(not options.CertFilePath) or (not os.path.isfile(options.CertFilePath)):
            logging.debug("Not CertFilePath.  That means we are un-enrolling")
            logging.critical("Step 2A should not be enabled if un-enrolling")
            return -847

        if(not options.Signing2APfxFile):
            logging.critical("Since Step2A is enabled you must supply a path to a PFX file for test signing")
            return -848

        if(not options.Step2BEnable):
            #must have output file
            if(not options.Signing2AResultFile):
                logging.critical("Since Step2B is not enabled there must be a Signing2AResultFile for the result")
                return -5

            if(options.Signing2AResultFile):
                logging.debug("Step2A Result will be written to: " + options.Signing2AResultFile)

    #Step2B Combine Step1 and Step2A into Single File in prep for Step2
    if(options.Step2BEnable):
        logging.debug("Step 2B Enabled")
        if(not options.Step2AEnable):
            #must have Test Signature Input File
            if(not options.TestSignatureInputFile) or (not os.path.isfile(options.TestSignatureInputFile)):
                logging.critical("Step2B Must have an Test Signature Input File when 2A is not enabled")
                return -8487
        else:
            #Step 2A enabled
            if(options.TestSignatureInputFile):
                logging.critical("Step2B can not have a Test Signature Input File when 2A is enabled")
                return -8489

        if(not options.Step1Enable):
            #must have Step1s data file
            if(not options.CertProvisionBlobAfterStep1File) or (not os.path.isfile(options.CertProvisionBlobAfterStep1File)):
                logging.critical("Step2B must have a Cert Priovision Blob when Step 1 is not enabled")
                return -8490

        if(not options.Step2Enable):
            #must have an output file
            if(not options.Prep2BResultFile):
                logging.critical("Step2B must have an output file (Prep2BResultFile)  when Step2 is not enabled")
                return -8491


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
        SEM = CertProvisioningApplyVariable(None, int(options.HdrVersion))
        SEM.Identity = int(options.Identity);

        if (int(options.HdrVersion) ==  CertProvisioningApplyVariable.VERSION_V1):
            SEM.SNTarget = int(options.SnTarget);
        elif (int(options.HdrVersion) ==  CertProvisioningApplyVariable.VERSION_V2):
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

            if is_32bit_number (options.Version ):
                SEM.Version = options.Version

            if is_32bit_number (options.Lsv):
                SEM.Lsv = options.Lsv
        else:
            logging.critical("Invalid Header Version specified")
            return -31

        if(options.CertFilePath != None):
            a = open(options.CertFilePath, "rb")
            SEM.TrustedCert = a.read()
            a.close()
            SEM.TrustedCertSize = os.path.getsize(options.CertFilePath)

        of = open(Step1OutFile, "wb")
        SEM.Write(of)
        of.close()

        #if user requested a step1 output file copy the temp file
        if(options.PrepResultFile):
            shutil.copy(Step1OutFile, options.PrepResultFile)

        #setup input for Step2
        options.SigningInputFile = Step1OutFile
        #setup input for Step2B
        options.CertProvisionBlobAfterStep1File = Step1OutFile

    #STEP 2A - Local Test Signature of Cert
    if(options.Step2AEnable):
        logging.critical("Step2A Started")
        Step2AFileToSign = os.path.join(tempdir, "Step2AIn.bin")
        Step2AOutFile = os.path.join(tempdir, "Step2AOut.bin")
        shutil.copy(options.CertFilePath, Step2AFileToSign)
        options.Signing2AOutputFile = Step2AOutFile
        ret = TestSignSemTrustedCert(options)
        if(ret != 0):
            logging.critical("TestSignSemTrustedCert (Step2A) Failed: " + str(ret))
            return ret

        if(options.Signing2AResultFile):
            shutil.copy(Step2AOutFile, options.Signing2AResultFile)

        #setup for step 2B
        options.TestSignatureInputFile = Step2AOutFile

    #STEP 2B - Combine Cert Provision blob with Test signature
    if(options.Step2BEnable):
        logging.critical("Step2B Started")
        Step2BOutFile = os.path.join(tempdir, "Step2BOut.bin")
        fi = open(options.CertProvisionBlobAfterStep1File, "rb")
        SEM = CertProvisioningApplyVariable(fi)
        fi.close()
        SEM.TestSignature = WinCertUefiGuid()
        TestDetached = open(options.TestSignatureInputFile, "rb")
        SEM.TestSignature.AddCertData(TestDetached)
        TestDetached.close()
        SemOut = open(Step2BOutFile, "wb")
        SEM.Write(SemOut)
        SemOut.close()

        if(options.Prep2BResultFile):
            shutil.copy(Step2BOutFile, options.Prep2BResultFile)

        #Setup for Step2
        options.SigningInputFile = Step2BOutFile

    #STEP 2 - Local sign
    if(options.Step2Enable):
        logging.critical("Step2 Started")
        #copy signinginputfile into temp dir
        FileToSign = os.path.join(tempdir, "Step2In.bin")
        shutil.copy(options.SigningInputFile, FileToSign)
        options.SigningInputFile = FileToSign
        options.SigningOutputFile = os.path.join(tempdir, "Step2Signature.bin")

        #do local signature
        ret = SignSEMProvisionData(options)
        if(ret != 0):
            logging.critical("SignSEMProvisionData (Step2) Failed: " + str(ret))
            return ret

        if(options.SigningResultFile):
            shutil.copy(options.SigningOutputFile, options.SigningResultFile)

        #setup input for Step3
        options.FinalizeInputFile = options.SigningInputFile
        options.FinalizeInputDetachedSignatureFile = options.SigningOutputFile


    #STEP 3 - Write Signature Structure and complete the KeyManifiest
    if(options.Step3Enable):
        logging.critical("Step3 Started")
        sstep1file = open(options.FinalizeInputFile, "rb")
        SEM = CertProvisioningApplyVariable(sstep1file)
        sstep1file.close()
        SEM.SessionId = random.randint(0, 4294967295) #generate a random session id
        SEM.Signature = WinCertUefiGuid()
        detached = open(options.FinalizeInputDetachedSignatureFile, "rb")
        SEM.Signature.AddCertData(detached)
        detached.close()

        if(not options.FinalizeResultFile):
            options.FinalizeResultFile = os.path.join(tempdir, "Step3Out.bin")

        of = open(options.FinalizeResultFile, "wb")
        SEM.Write(of)
        of.close()

        if(not options.CertFilePath) or (not os.path.isfile(options.CertFilePath)):
            # If not unenrolling, verify completeness.  Unenroll does not have a trusted cert
            # or a self signature.
            if(not SEM.VerifyComplete()):
                logging.critical("SEM Package Not complete")
                return -84

    #
    # Function to print SEM
    #
    if(options.PrintFile) and (os.path.isfile(options.PrintFile)):
        PrintSEM(options.PrintFile)

    if(options.PrintResultsFile) and (os.path.isfile(options.PrintResultsFile)):
        PrintSEMResults(options.PrintResultsFile)

    if(options.PrintCurrentFile) and (os.path.isfile(options.PrintCurrentFile)):
        PrintSEMCurrent(options.PrintCurrentFile)

    if(options.ExtractCertFile) and (os.path.isfile(options.ExtractCertFile)):
        Thumbprint = ExtractCert(options.ExtractCertFile)
        logging.critical(f"Extracted cert with thumbprint {Thumbprint}")

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
