##
## Utility Functions to support re-use in python scripts.  
##
## Includes functions for running external commands, etc
##
## Copyright Microsoft Corporation, 2017
##
from __future__ import print_function  #support Python3 and 2 for print
import os
import logging
import datetime
import shutil
import threading
import subprocess


# ref: http://stackoverflow.com/questions/36932/how-can-i-represent-an-enum-in-python
class Enum(tuple): __getattr__ = tuple.index



# PropagatingThread copied from sample here:
# https://stackoverflow.com/questions/2829329/catch-a-threads-exception-in-the-caller-thread-in-python
class PropagatingThread(threading.Thread):
    def run(self):
        self.exc = None
        try:
            if hasattr(self, '_Thread__target'):
                # Thread uses name mangling prior to Python 3.
                self.ret = self._Thread__target(*self._Thread__args, **self._Thread__kwargs)
            else:
                self.ret = self._target(*self._args, **self._kwargs)
        except BaseException as e:
            self.exc = e

    def join(self, timeout=None):
        super(PropagatingThread, self).join()
        if self.exc:
            raise self.exc
        return self.ret

##
## helper functions
##

#
# process output stream and write to log.
# part of the threading pattern.
#
#  http://stackoverflow.com/questions/19423008/logged-subprocess-communicate
#
def reader(filepath, outstream, stream):
    f = None
    #open file if caller provided path
    if(filepath):
        f = open(filepath, "w")

    while True:
        s = stream.readline()
        if not s:
            break
        if(f is not None):
            #write to file if caller provided file
            f.write(s)
        if(outstream is not None):
            #write to stream object if caller provided object
            outstream.write(s)
        logging.info(s.rstrip())
    stream.close()
    if(f is not None):
        f.close()

#
# Run a shell commmand and print the output to the log file
#
def RunCmd(cmd, capture=True, workingdir=None, outfile=None, outstream=None):
    starttime = datetime.datetime.now()
    logging.debug("Cmd to run is: " + cmd)
    logging.info("------------------------------------------------")
    logging.info("--------------Cmd Output Starting---------------")
    logging.info("------------------------------------------------")
    c = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, cwd=workingdir)
    if(capture):
        outr = PropagatingThread(target=reader, args=(outfile, outstream, c.stdout,))
        outr.start()
        c.wait()
        outr.join()
    else:
        c.wait()
            
    endtime = datetime.datetime.now()
    delta = endtime - starttime
    logging.info("------------------------------------------------")
    logging.info("--------------Cmd Output Finished---------------")
    logging.info("--------- Running Time (mm:ss): {0[0]:02}:{0[1]:02} ----------".format(divmod(delta.seconds, 60)))
    logging.info("------------------------------------------------")
    return c.returncode
    
    
def SignWithRsaPkcs1(ToSignFilePath, DetachedSignatureOutputFilePath, PfxFile, PfxPass, RsaPkcs1SignToolPath):
    OutDir = os.path.dirname(DetachedSignatureOutputFilePath)
    tfile = os.path.abspath(os.path.join(OutDir, "temp.tmp"))
    logging.debug("Temp file for rsapkcs1 is %s" % tfile)
    
    #
    # Cert Manager is used for deleting the cert when add/removing certs
    #
    CertMgrPath = os.path.join(os.getenv("ProgramFiles(x86)"), "Windows Kits", "10", "bin", "x64", "certmgr.exe")

    #
    # Cert Util is used to import PFX into cert store
    #
    CertUtilPath = "CertUtil.exe"
    
    #check the tool path and update it
    if not os.path.exists(CertMgrPath):
        CertMgrPath = CertMgrPath.replace('10', '8.1')
        
    if not os.path.isfile(RsaPkcs1SignToolPath):
        logging.critical("Invalid RsaPkcs1SignToolPath.  Can't Sign!")
        return -1


    #1 - use Certmgr to get the PFX sha1 thumbprint
    cmd = CertMgrPath + " /c " + PfxFile
    ret = RunCmd(cmd, True, outfile=tfile)
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

    logging.info("Thumbprint for this %s is %s" % (PfxFile, thumbprint))
    if(len(thumbprint) != 40) or (found == False):
        logging.critical("Thumbprint parsing failed.  Should have been 40 characters but instead was %d" % len(thumbprint))
        return -100

    
    #3 - Install cert in cert store
    if(not PfxPass):
        PfxPass = ""
    cmd = CertUtilPath + " -p \"" + PfxPass + "\" -user -importPFX My " + PfxFile + " NoRoot,NoChain"
    ret = RunCmd(cmd)
    if(ret != 0):
        logging.critical("Failed to add cert from Pfx file using CertMgr.exe")
        if(ret == -2147024156):
            logging.critical("\n\nYOU MUST BE RUNNING THIS AS ADMIN\n\n")
        return ret
    
    #4 - Sign with Pcks1SignTool
    signerThumb = ""
    count = 0
    for c in thumbprint:
        signerThumb = signerThumb + c
        if(count % 2):
            signerThumb = signerThumb + " "
        count += 1
    signerThumb = signerThumb.strip()
        

    cmd = RsaPkcs1SignToolPath + ' sha256 /s ' + ToSignFilePath + ' ' + DetachedSignatureOutputFilePath + " /thumb \"" + signerThumb + "\" My u"
    ret = RunCmd(cmd)
    if(ret != 0):
        logging.critical("Failed to sign using RsaPkcs1Signer")
    
    
    #5 - Remove from cert store
    cmd = CertMgrPath + " -del -sha1 " + thumbprint + " -c -s my"
    ret = RunCmd(cmd, True)
    if(ret != 0):
        logging.critical("Failed to remove cert from Pfx file using CertMgr.exe")
    
    return ret


#Sign with Signtool
def SignWithSignTool(ToSignFilePath, SignatureOutputFile, PfxFilePath, PfxPass=None, Oid="1.2.840.113549.1.7.2"):
    #Find Signtool
    SignToolPath = os.path.join(os.getenv("ProgramFiles(x86)"), "Windows Kits", "8.1", "bin", "x64", "signtool.exe")
    #check the tool path and update it to try win 10 version if 8.1 doesn't work.  
    # Bug in Win10 version as of 15063 SDK.  Waiting on future sdk to fix.  This is why 8.1 is preferred
    if not os.path.exists(SignToolPath):
        SignToolPath = SignToolPath.replace('8.1', '10')

    OutputDir = os.path.dirname(SignatureOutputFile)
    #Signtool docs https://docs.microsoft.com/en-us/dotnet/framework/tools/signtool-exe
    #Signtool parameters from https://docs.microsoft.com/en-us/windows-hardware/manufacture/desktop/secure-boot-key-generation-and-signing-using-hsm--example
    # Search for "Secure Boot Key Generation and Signing Using HSM"
    cmd = SignToolPath + ' sign /fd sha256 /p7ce DetachedSignedData /p7co ' + Oid + ' /p7 "' + OutputDir + '" /f "' + PfxFilePath + '"'
    if PfxPass is not None:
        #add password if set
        cmd = cmd + ' /p ' + PfxPass
    cmd = cmd + ' /debug /v "' + ToSignFilePath + '" '
    ret = RunCmd(cmd)
    if(ret != 0):
        raise Exception("Signtool error")
    signedfile = os.path.join(OutputDir, os.path.basename(ToSignFilePath) + ".p7")
    if(not os.path.isfile(signedfile)):
        raise Exception("Output file doesn't eixst %s" % signedfile)

    shutil.move(signedfile, SignatureOutputFile)
    return ret

###
# Function to print a byte list as hex and optionally output ascii as well as
# offset within the buffer
###
def PrintByteList(ByteList, IncludeAscii=True, IncludeOffset=True, IncludeHexSep=True, OffsetStart=0):
    Ascii = ""
    for index in range(len(ByteList)):
        #Start of New Line
        if(index % 16 == 0):
            if(IncludeOffset):
                print("0x%04X -" % (index + OffsetStart), end='')

        #Midpoint of a Line
        if(index % 16 == 8):
            if(IncludeHexSep):
                print(" -", end='')

        #Print As Hex Byte
        print(" 0x%02X" % ByteList[index], end='')

        #Prepare to Print As Ascii
        if(ByteList[index] < 0x20) or (ByteList[index] > 0x7E):
            Ascii += "."
        else:
            Ascii += ("%c" % ByteList[index])

        #End of Line
        if(index % 16 == 15):
            if(IncludeAscii):
                print(" %s" % Ascii, end='')
            Ascii = ""
            print("")

    #Done - Lets check if we have partial
    if(index % 16 != 15):
        #Lets print any partial line of ascii
        if(IncludeAscii) and (Ascii != ""):
            #Pad out to the correct spot
            
            while(index % 16 != 15):
                print("     ", end='')
                if(index % 16 == 7):  #acount for the - symbol in the hex dump
                    if(IncludeOffset):
                        print("  ", end='')
                index += 1
            #print the ascii partial line
            print(" %s" % Ascii, end='')
            #print a single newline so that next print will be on new line
        print("")

if __name__ == '__main__':
    pass
    # Test code for printing a byte buffer
    # a = [0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d]
    # index = 0x55
    # while(index < 0x65):
    #     a.append(index)
    #     PrintByteList(a)
    #     index += 1