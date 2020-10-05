# @file
#
# Script to support the binary structure of a provisioning variable.
#
# Copyright (c), Microsoft Corporation
# SPDX-License-Identifier: BSD-2-Clause-Patent

##
## Data Structure support for Cert Provisioning Variable
##
##

import sys
import struct
from edk2toollib.uefi.wincert import *
from edk2toollib.uefi.status_codes import UefiStatusCode
from edk2toollib.utility_functions import PrintByteList

##
## SEM Cert Provisioning Apply Variable Data
##
class CertProvisioningApplyVariable(object):
    STATIC_STRUCT_SIZE_V1=20
    STATIC_STRUCT_SIZE_V2=32
    HEADER_SIG_VALUE = "MSPA"
    VERSION_V1 = 1
    VERSION_V2 = 2
    IDENTITY_MAP = ["NONE", "OWNER CERT", "USER CERT", "USER1 CERT", "USER2 CERT", "ZTC CERT"]

    def __init__(self, filestream=None, HdrVersion=1):
        if (HdrVersion != self.VERSION_V1 and
            HdrVersion != self.VERSION_V2):
            raise Exception("Invalid version specified")

        self.TestSignature = None
        self.Signature = None
        self.TrustedCertSize = 0
        self.TrustedCert = None
        self.HeaderSignature = self.HEADER_SIG_VALUE
        self.HeaderVersion = HdrVersion
        self.Identity = 0
        self.SessionId = 0

        # V1 unique members
        self.SNTarget = 0

        # V2 unique members
        self.Version = 0
        self.Lsv = 0
        self.Manufacturer = None
        self.ProductName = None
        self.SerialNumber = None
        self.MfgOffset = self.STATIC_STRUCT_SIZE_V2
        self.ProductOffset = 0
        self.SerialOffset = 0
        self.TrustedCertOffset = 0

        if(filestream != None):
            self.PopulateFromFileStream(filestream)
    #
    # Method to un-serialize from a filestream
    #
    def PopulateFromFileStream(self, fs):
        if(fs == None):
            raise Exception("Invalid File stream")

        #only populate from file stream those parts that are complete in the file stream
        offset = fs.tell()
        fs.seek(0,2)
        end = fs.tell()
        fs.seek(offset)

        if((end - offset) < self.STATIC_STRUCT_SIZE_V1): #size of the static header data
            raise Exception("Invalid file stream size")

        self.HeaderSignature = fs.read(4).decode()
        if self.HeaderSignature != self.HEADER_SIG_VALUE:
            raise Exception("Incorrect Header Signature")
        self.HeaderVersion = struct.unpack("=B", fs.read(1))[0]

        if (self.HeaderVersion == self.VERSION_V1):
            self.Identity = struct.unpack("=B", fs.read(1))[0]
            self.SNTarget = struct.unpack("=Q", fs.read(8))[0]
            self.SessionId = struct.unpack("=I", fs.read(4))[0]
            self.TrustedCertSize = struct.unpack("=H", fs.read(2))[0]
            self.TrustedCert = None
        elif (self.HeaderVersion == self.VERSION_V2):
            self.Identity = struct.unpack("=B", fs.read(1))[0]
            Temp = struct.unpack("=B", fs.read(1))[0]
            if Temp != 0:
                raise Exception("Invalid Reserved Field 1")
            Temp = struct.unpack("=B", fs.read(1))[0]
            if Temp != 0:
                raise Exception("Invalid Reserved Field 2")
            self.SessionId = struct.unpack("=I", fs.read(4))[0]
            self.MfgOffset = struct.unpack("=H", fs.read(2))[0]
            self.ProductOffset = struct.unpack("=H", fs.read(2))[0]
            self.SerialOffset = struct.unpack("=H", fs.read(2))[0]
            self.TrustedCertSize = struct.unpack("=H", fs.read(2))[0]
            self.TrustedCertOffset = struct.unpack("=H", fs.read(2))[0]
            self.TrustedCert = None

            Temp = struct.unpack("=H", fs.read(2))[0]
            if Temp != 0:
                raise Exception("Invalid Reserved Field 1")
            self.Version = struct.unpack("=I", fs.read(4))[0]
            self.Lsv = struct.unpack("=I", fs.read(4))[0]

            if self.Version < self.Lsv:
                raise Exception("Invalid Lsv - must not be > Version")

            if ((self.MfgOffset >= self.ProductOffset) or
                (self.ProductOffset >= self.SerialOffset) or
                (self.SerialOffset >= self.TrustedCertOffset)):
                raise Exception("Invalid Offset Structure")

            Temp =  fs.tell()
            if Temp != self.MfgOffset:
                raise Exception("Invalid Mfg Offset")
            self.Manufacturer = fs.read(self.ProductOffset - self.MfgOffset - 1).decode()
            Temp = struct.unpack("=B", fs.read(1))[0]
            if Temp != 0:
                raise Exception("Invalid NULL in Mfg")

            Temp =  fs.tell()
            if Temp != self.ProductOffset:
                raise Exception("Invalid Product Offset")
            self.ProductName = fs.read(self.SerialOffset - self.ProductOffset - 1).decode()
            Temp = struct.unpack("=B", fs.read(1))[0]
            if Temp != 0:
                raise Exception("Invalid NULL in ProductName")

            Temp =  fs.tell()
            if Temp != self.SerialOffset:
                raise Exception("Invalid SerialOffset Offset")
            self.SerialNumber = fs.read(self.TrustedCertOffset - self.SerialOffset - 1).decode()
            Temp = struct.unpack("=B", fs.read(1))[0]
            if Temp != 0:
                raise Exception("Invalid NULL in SerialNumber")

            if self.TrustedCertSize != 0:
                Temp =  fs.tell()
                if Temp != self.TrustedCertOffset:
                    raise Exception("Invalid TrustedCertOffset Offset")
        else:
            raise Exception("Invalid header version")

        if((end - fs.tell()) < self.TrustedCertSize):
            raise Exception("Invalid file stream size (Trusted Cert Size)")

        if(self.TrustedCertSize > 0):
            self.TrustedCert = memoryview(fs.read(self.TrustedCertSize))

        if((end - fs.tell()) > 0):
            if(self.TrustedCertSize > 0):
                self.TestSignature = WinCert.Factory(fs)

        if((end - fs.tell()) > 0):
            self.Signature = WinCert.Factory(fs)

    #
    # Method to Print CertProvisioningApplyVariable to stdout
    #
    def Print(self):
        print ("CertProvisioningVariable")
        print ("  HeaderSignature:  %s" % self.HeaderSignature)
        print ("  HeaderVersion:    0x%X" % self.HeaderVersion)
        print ("  Identity:         0x%X (%s)" % (self.Identity, self.IDENTITY_MAP[self.Identity]))
        print ("  SessionId:        0x%X" % self.SessionId)
        if (self.HeaderVersion == self.VERSION_V1):
            print ("  SN Target:        %d" % self.SNTarget)
        elif (self.HeaderVersion == self.VERSION_V2):
            print ("  Version:          %s" % self.Version)
            print ("  Lsv:              %s" % self.Lsv)
            print ("  Manufacturer:     %s" % self.Manufacturer)
            print ("  Product Name:     %s" % self.ProductName)
            print ("  SerialNumber:     %s" % self.SerialNumber)
        else:
            raise Exception("Invalid header version")

        print ("  TrustedCertSize:  0x%X" % self.TrustedCertSize)
        print ("  TrustedCert:    ")
        if(self.TrustedCert != None):
            ndbl = self.TrustedCert.tolist()
            PrintByteList(ndbl)

        if(self.TrustedCertSize > 0) and (self.TestSignature != None):
            print ("  TestSignature:   ")
            self.TestSignature.Print()

        if(self.Signature != None):
            print ("  Signature:   ")
            self.Signature.Print()


    def Write(self, fs):
        fs.write(self.HeaderSignature.encode('utf-8'))
        fs.write(struct.pack("=B", self.HeaderVersion))
        fs.write(struct.pack("=B", self.Identity))
        if (self.HeaderVersion == self.VERSION_V1):
            fs.write(struct.pack("=Q", self.SNTarget))
            fs.write(struct.pack("=I", self.SessionId))
            fs.write(struct.pack("=H", self.TrustedCertSize))
        elif (self.HeaderVersion == self.VERSION_V2):
            fs.write(struct.pack("=B", 0))
            fs.write(struct.pack("=B", 0))
            fs.write(struct.pack("=I", self.SessionId))
            fs.write(struct.pack("=H", self.MfgOffset))
            self.ProductOffset = self.MfgOffset + len(self.Manufacturer) + 1
            self.SerialOffset = self.ProductOffset + len(self.ProductName) + 1
            self.TrustedCertOffset = self.SerialOffset + len(self.SerialNumber) + 1
            fs.write(struct.pack("=H", self.ProductOffset))
            fs.write(struct.pack("=H", self.SerialOffset))
            fs.write(struct.pack("=H", self.TrustedCertSize))
            fs.write(struct.pack("=H", self.TrustedCertOffset))
            fs.write(struct.pack("=H", 0))  # Alignment UINT16
            fs.write(struct.pack("=I", self.Version))
            fs.write(struct.pack("=I", self.Lsv))
            fs.write(self.Manufacturer.encode('utf-8'))
            fs.write(struct.pack("=B", 0))  # NULL Terminator
            fs.write(self.ProductName.encode('utf-8'))
            fs.write(struct.pack("=B", 0))  # NULL Terminator
            fs.write(self.SerialNumber.encode('utf-8'))
            fs.write(struct.pack("=B", 0))  # NULL Terminator

        else:
            raise Exception("Invalid header version")

        if(self.TrustedCertSize != 0):
            fs.write(self.TrustedCert)
        if(self.TestSignature != None):
            self.TestSignature.Write(fs)
        if(self.Signature != None):
            self.Signature.Write(fs)


    def VerifyComplete(self):
        if(self.TrustedCertSize > 0):
            if(not self.TestSignature):
                return False
        if(not self.Signature):
            return False

        return True

    def GetCertType(self):
        return self.IDENTITY_MAP[self.Identity]

    def WriteCert(self, fs):
        if(self.TrustedCertSize != 0):
            fs.write(self.TrustedCert)

##
##  SEM Cert Provision Result Variable Data
##
class CertProvisioningResultVariable(object):
    STATIC_STRUCT_SIZE=18
    HEADER_SIG_VALUE = "MSPR"
    VERSION = 1

    def __init__(self, filestream=None):
        if(filestream == None):
            self.HeaderSignature = self.HEADER_SIG_VALUE
            self.HeaderVersion = self.VERSION
            self.Status = 0
            self.Identity = 0
            self.SessionId = 0
        else:
            self.PopulateFromFileStream(filestream)
    #
    # Method to un-serialize from a filestream
    #
    def PopulateFromFileStream(self, fs):
        if(fs == None):
            raise Exception("Invalid File stream")

        #only populate from file stream those parts that are complete in the file stream
        offset = fs.tell()
        fs.seek(0,2)
        end = fs.tell()
        fs.seek(offset)

        if((end - offset) < self.STATIC_STRUCT_SIZE): #size of the static header data
            raise Exception("Invalid file stream size")

        self.HeaderSignature = fs.read(4).decode()
        if self.HeaderSignature != self.HEADER_SIG_VALUE:
            raise Exception("Incorrect Header Signature")
        self.HeaderVersion = struct.unpack("=B", fs.read(1))[0]
        if (self.HeaderVersion != self.VERSION):
            raise Exception("Incorrect Header Version")
        self.Identity = struct.unpack("=B", fs.read(1))[0]
        self.SessionId = struct.unpack("=I", fs.read(4))[0]
        self.Status = struct.unpack("=Q", fs.read(8))[0]

    #
    # Method to Print SEM var to stdout
    #
    def Print(self):
        print ("CertProvisioningResultVariable")
        print ("  HeaderSignature:  %s" % self.HeaderSignature)
        print ("  HeaderVersion:    0x%X" % self.HeaderVersion)
        print ("  Identity:         0x%X (%s)" % (self.Identity, CertProvisioningApplyVariable.IDENTITY_MAP[self.Identity]))
        print ("  SessionId:        0x%X" % self.SessionId)
        print ("  Status:           %s (0x%X)" % (UefiStatusCode().Convert64BitToString(self.Status), self.Status))


    def Write(self, fs):
        fs.write(self.HeaderSignature.encode('utf-8'))
        fs.write(struct.pack("=B", self.HeaderVersion))
        fs.write(struct.pack("=B", self.Identity))
        fs.write(struct.pack("=I", self.SessionId))
        fs.write(struct.pack("=Q", self.Status))

