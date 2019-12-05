# @file
#
# Utility Functions to support re-use in python scripts.
#
# Copyright (c), Microsoft Corporation
# SPDX-License-Identifier: BSD-2-Clause-Patent

##
## Utility Functions to support re-use in python scripts.
##
## Includes functions for running external commands, etc
##
from __future__ import print_function  #support Python3 and 2 for print
import os
import sys
import logging
import datetime
import shutil
import threading
import subprocess

#
#set signtool path --
#  Requires the windows 8.1 kit.
#  only works on 64bit systems but all dev machines should be 64bit by now.
gSignToolPath = os.path.join(os.getenv("ProgramFiles(x86)"), "Windows Kits", "10", "bin", "10.0.18362.0", "x64", "signtool.exe")

#
# Cert Manager is used for deleting the cert when add/removing certs
#
gCertMgrPath = os.path.join(os.getenv("ProgramFiles(x86)"), "Windows Kits", "10", "bin", "10.0.18363.0", "x64", "certmgr.exe")

#
# Cert Util is used to import PFX into cert store
#
gCertUtilPath = "CertUtil.exe"

#
#check Windows Kit files and try using 8.1 if not in 10
#
#check the tool path and update it
if not os.path.exists(gCertMgrPath):
    gCertMgrPath = gCertMgrPath.replace('10', '8.1')

#check the tool path and update it
if not os.path.exists(gSignToolPath):
    gSignToolPath = gSignToolPath.replace('10', '8.1')


#
# process output stream and write to log.
# part of the threading pattern.
#
#  http://stackoverflow.com/questions/19423008/logged-subprocess-communicate
#
def reader(stream):
    while True:
        s = stream.readline()
        if not s:
            break
        logging.info(s.rstrip())
    stream.close()

def filereader(filepath, stream):
     f = open(filepath, "w")
     while True:
         s = stream.readline()
         if not s:
             break
         f.write(s)
     stream.close()
     f.close()

#
# Run a shell commmand and print the output to the log file
#
def RunCmd(cmd, capture=True, outfile=None):
    starttime = datetime.datetime.now()
    logging.debug("Cmd to run is: " + cmd)
    logging.info("------------------------------------------------")
    logging.info("--------------Cmd Output Starting---------------")
    logging.info("------------------------------------------------")
    c = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if(capture):
        if(outfile):
            outr = threading.Thread(target=filereader, args=(outfile, c.stdout,))
        else:
            outr = threading.Thread(target=reader, args=(c.stdout,))
        outr.start()
        c.wait()
        outr.join()
    else:
        c.wait()

    #replaced communicate method with modified threading solution found here
    # http://stackoverflow.com/questions/19423008/logged-subprocess-communicate
    #

    endtime = datetime.datetime.now()
    delta = endtime - starttime
    logging.info("------------------------------------------------")
    logging.info("--------------Cmd Output Finished---------------")
    logging.info("--------- Running Time (mm:ss): {0[0]:02}:{0[1]:02} ----------".format(divmod(delta.seconds, 60)))
    logging.info("------------------------------------------------")
    return c.returncode


def SignWithSignTool(ToSignFilePath, DetachedSignatureOutputFilePath, PfxFile, PfxPass, Oid):
    OutputDir = os.path.dirname(DetachedSignatureOutputFilePath)
    cmd = gSignToolPath + ' sign /p7ce DetachedSignedData /fd sha256 /p7co ' + Oid + ' /p7 "' + OutputDir + '" /f "' + PfxFile + '"'
    if PfxPass:
        #add password if set
        cmd = cmd + ' /p ' + PfxPass
    cmd = cmd + ' /debug /v "' + ToSignFilePath + '" '
    logging.critical("Command is: %s" % cmd)
    ret = RunCmd(cmd)
    if(ret != 0):
        raise Exception("Signtool error %d" % ret)
    signedfile = os.path.join(OutputDir, os.path.basename(ToSignFilePath) + ".p7")
    if(not os.path.isfile(signedfile)):
        raise Exception("Output file doesn't eixst %s" % signedfile)

    shutil.move(signedfile, DetachedSignatureOutputFilePath)
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

