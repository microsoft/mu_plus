# @file
#
# Simple UEFI Serial Log capture from an FTDI Serial Port
#
# Copyright (c), Microsoft Corporation
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

import os, sys
import argparse
import logging
import datetime
import time
import serial
import serial.tools.list_ports

class UefiSerialLogging(object):

    # FTDI allows multiple ports.  One OEM  has the System Under Test
    # communicates over the FTDI on the 3rd COM port.

    INSTANCE_FOR_OTHER_1 = 1
    INSTANCE_FOR_OTHER_2 = 2
    INSTANCE_FOR_UEFI = 3
    INSTANCE_FOR_OTHER_4= 4
    TOTAL_PORTS = 4

    @staticmethod
    def FindUefiSerialPort(skip=0):
        systemports = []
        for a in serial.tools.list_ports.comports():
            if((a.pid == 0x6011) and (a.vid==0x403)):
                logging.debug("Found a Serial Port on FTDI BUS %s" % a.device)
                systemports.append(a)
            else:
                continue
        #now sort
        systemports.sort()

        #get the index based on skip parameter
        index = (skip * UefiSerialLogging.TOTAL_PORTS) + (UefiSerialLogging.INSTANCE_FOR_UEFI - 1)
        if(index < len(systemports)):
            return systemports[index].device
        else:
            return None

    def __init__(self, skip=0, baud=6000000):  #buad rate default is 6mbps
        self.s = serial.Serial()
        portname = UefiSerialLogging.FindUefiSerialPort(skip)
        self.s.port = portname
        self.s.baudrate = baud
        self.s.bytesize = serial.EIGHTBITS #8
        self.s.parity = serial.PARITY_NONE #N
        self.s.stopbits = serial.STOPBITS_ONE #1
        self.s.timeout = 5  #5 second timeout

        if(int(serial.VERSION.partition(".")[0]) < 3):
            logging.critical("Old pyserial (%s).  Please update as this only supports the newer 3.0 pyserial syntax." % serial.VERSION)


    def Start_Logging(self, LogFile):
        self.LogFile = open(LogFile, "wb")  #do as binary because serial read function returns byte string
        self.s.open()
        try:
            while( True):
                self.LogFile.write(self.s.read(1000))
        except (KeyboardInterrupt, SystemExit):
            self.LogFile.write(self.s.read())  #do one more read to make sure we got everything
            self.s.close()
            self.LogFile.close()
            pass

        finally:
            self.LogFile.write(self.s.read())  #do one more read to make sure we got everything
            self.s.close()
            self.LogFile.close()


#
#main script function
#
def main():
    parser = argparse.ArgumentParser(description='Uefi Serial Logger')

    #Output debug log
    parser.add_argument("-l", dest="OutputLog", help="Create an output log file (script output): ie -l out.txt", default=None)

    #Turn on debug level logging
    parser.add_argument("--debug", action="store_true", dest="debug", help="turn on debug logging level for file log",  default=False)
    parser.add_argument("--SerialLogOutput", dest="SerialLogOutput", help="Output file to Log all Serial Output to", default="UefiSerialLog.log")
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

    logging.critical("Logging all Serial output to: %s" % options.SerialLogOutput)

    a = UefiSerialLogging()
    a.Start_Logging(options.SerialLogOutput)
    logging.critical("Finished")
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
