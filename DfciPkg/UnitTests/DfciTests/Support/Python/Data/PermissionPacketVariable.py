# @file
#
# Script to support the binary format of a Permission variable
#
# Copyright (c), Microsoft Corporation
# SPDX-License-Identifier: BSD-2-Clause-Patent

##
## Data Structure support for Permission Packet Provisioning Variable
##
##

import struct
import xml.dom.minidom
from edk2toollib.uefi.wincert import WinCert
from edk2toollib.uefi.status_codes import UefiStatusCode
from edk2toollib.utility_functions import PrintByteList


##
## SEM Permission Apply Variable Data
##
class PermissionApplyVariable(object):
    STATIC_STRUCT_SIZE_V1=22
    STATIC_STRUCT_SIZE_V2=22
    HEADER_SIG_VALUE = "MPPA"
    VERSION_V1 = 1
    VERSION_V2 = 2

    def __init__(self, filestream=None, HdrVersion=1):
        if (HdrVersion != self.VERSION_V1 and
            HdrVersion != self.VERSION_V2):
            raise Exception("Invalid version specified")

        # Common members
        self.HeaderSignature = None
        self.HeaderVersion = 0
        self.Payload = None
        self._XmlTree = None  #private XML structure
        self.Signature = None
        self.SessionId = 0
        self.PayloadSize = 0
        self.Rsvd1 = 0
        self.Rsvd2 = 0
        self.Rsvd3 = 0

        # V1 unique members
        self.SNTarget = 0

        # V2 unique members
        self.Manufacturer = None
        self.ProductName = None
        self.SerialNumber = None
        self.MfgOffset = self.STATIC_STRUCT_SIZE_V2
        self.ProductOffset = 0
        self.SerialOffset = 0
        self.PayloadOffset = 0

        if(filestream == None):
            self.HeaderSignature = self.HEADER_SIG_VALUE
            self.HeaderVersion = HdrVersion
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

        if((end - offset) < self.STATIC_STRUCT_SIZE_V1): # minimum size of the static header data
            raise Exception("Invalid file stream size")

        self.HeaderSignature = fs.read(4).decode()
        if self.HeaderSignature != self.HEADER_SIG_VALUE:
            print ("  HeaderSignature:  %s" % self.HeaderSignature)
            raise Exception("Incorrect Header Signature")
        self.HeaderVersion = struct.unpack("=B", fs.read(1))[0]
        self.Rsvd1 = struct.unpack("=B", fs.read(1))[0]
        self.Rsvd2 = struct.unpack("=B", fs.read(1))[0]
        self.Rsvd3 = struct.unpack("=B", fs.read(1))[0]
        if ((self.Rsvd1 != 0) or
            (self.Rsvd2 != 0) or
            (self.Rsvd3 != 0)):
            raise Exception("Reserved bytes must be zero")

        self.Payload = None

        if (self.HeaderVersion == self.VERSION_V1):
            self.SNTarget = struct.unpack("=Q", fs.read(8))[0]
            self.SessionId = struct.unpack("=I", fs.read(4))[0]
            self.PayloadSize = struct.unpack("=H", fs.read(2))[0]

        elif (self.HeaderVersion == self.VERSION_V2):
            if((end - offset) < self.STATIC_STRUCT_SIZE_V2): # minimum size of the static header data
                raise Exception("Invalid V2 file stream size")
            self.SessionId = struct.unpack("=I", fs.read(4))[0]
            self.MfgOffset = struct.unpack("=H", fs.read(2))[0]
            self.ProductOffset = struct.unpack("=H", fs.read(2))[0]
            self.SerialOffset = struct.unpack("=H", fs.read(2))[0]
            self.PayloadSize = struct.unpack("=H", fs.read(2))[0]
            self.PayloadOffset = struct.unpack("=H", fs.read(2))[0]

            if ((self.MfgOffset >= self.ProductOffset) or
                (self.ProductOffset >= self.SerialOffset) or
                (self.SerialOffset >= self.PayloadOffset)):
                raise Exception("Invalid Offset Structure")

            if (end - fs.tell() < self.PayloadOffset):
                raise Exception("Packet too small for SmBiosString")

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
            self.SerialNumber = fs.read(self.PayloadOffset - self.SerialOffset - 1).decode()
            Temp = struct.unpack("=B", fs.read(1))[0]
            if Temp != 0:
                raise Exception("Invalid NULL in ProductName")
        else:
            raise Exception("Invalid header version")

        if((end - fs.tell()) < self.PayloadSize):
            raise Exception("Invalid file stream size (PayloadSize)")

        self.Payload = fs.read(self.PayloadSize).decode()
        self._XmlTree = xml.dom.minidom.parseString(self.Payload)


        if((end - fs.tell()) > 0):
            self.Signature = WinCert.Factory(fs)


    def AddXmlPayload(self, xmlstring):
        if(self.Payload):
            raise Exception("Can't Add an XML payload to an object already containing payload")

        self.Payload = xmlstring
        self._XmlTree = xml.dom.minidom.parseString(self.Payload)
        self.PayloadSize = len(xmlstring)
    #
    # Method to Print PermissionApplyVariable to stdout
    #
    def Print(self, ShowRawXmlAsBytes=False):
        print ("PermissionApplyVariable")
        print ("  HeaderSignature:  %s" % self.HeaderSignature)
        print ("  HeaderVersion:    0x%X" % self.HeaderVersion)
        print ("  SessionId:        0x%X" % self.SessionId)
        print ("  PayloadSize:      0x%X" % self.PayloadSize)
        if (self.HeaderVersion == self.VERSION_V1):
            print ("  SN Target:        %d" % self.SNTarget)
        elif (self.HeaderVersion == self.VERSION_V2):
            print ("  Manufacturer:     %s" % self.Manufacturer)
            print ("  Product Name:     %s" % self.ProductName)
            print ("  SerialNumber:     %s" % self.SerialNumber)
        else:
            raise Exception("Invalid header version")

        if(self._XmlTree is not None):
            print ("%s" % self._XmlTree.toprettyxml())
        else:
            print ("XML TREE DOESN'T EXIST")

        if(ShowRawXmlAsBytes and (self.Payload != None)):
            print ("  Payload Bytes:    ")
            ndbl = list(bytearray(self.Payload.encode()))
            print(type(ndbl ) )
            PrintByteList(ndbl)

        if(self.Signature != None):
            self.Signature.Print()


    def Write(self, fs):
        fs.write(self.HeaderSignature.encode('utf-8'))
        fs.write(struct.pack("=B", self.HeaderVersion))
        fs.write(struct.pack("=B", self.Rsvd1))
        fs.write(struct.pack("=B", self.Rsvd2))
        fs.write(struct.pack("=B", self.Rsvd3))

        if (self.HeaderVersion == self.VERSION_V1):
            fs.write(struct.pack("=Q", self.SNTarget))
            fs.write(struct.pack("=I", self.SessionId))
            fs.write(struct.pack("=H", self.PayloadSize))
        elif (self.HeaderVersion == self.VERSION_V2):
            fs.write(struct.pack("=I", self.SessionId))
            fs.write(struct.pack("=H", self.MfgOffset))
            self.ProductOffset = self.MfgOffset + len(self.Manufacturer) + 1
            self.SerialOffset = self.ProductOffset + len(self.ProductName) + 1
            self.PayloadOffset = self.SerialOffset + len(self.SerialNumber) + 1
            fs.write(struct.pack("=H", self.ProductOffset))
            fs.write(struct.pack("=H", self.SerialOffset))
            fs.write(struct.pack("=H", self.PayloadSize))
            fs.write(struct.pack("=H", self.PayloadOffset))
            fs.write(self.Manufacturer.encode('utf-8'))
            fs.write(struct.pack("=B", 0))  # NULL Terminator
            fs.write(self.ProductName.encode('utf-8'))
            fs.write(struct.pack("=B", 0))  # NULL Terminator
            fs.write(self.SerialNumber.encode('utf-8'))
            fs.write(struct.pack("=B", 0))  # NULL Terminator
        else:
            raise Exception("Invalid header version")

        fs.write(self.Payload.encode('utf-8'))
        if(self.Signature != None):
            self.Signature.Write(fs)


##
##  SEM Permission Result Variable Data
##
class PermissionResultVariable(object):
    STATIC_STRUCT_SIZE=20
    STATIC_STRUCT_SIZE_V2=22
    HEADER_SIG_VALUE = "MPPR"
    VERSION_V1 = 1
    VERSION_V2 = 2

    def __init__(self, filestream=None, HdrVersion=1):
        if (HdrVersion != PermissionResultVariable.VERSION_V1 and
            HdrVersion != PermissionResultVariable.VERSION_V2):
            raise Exception("Invalid version specified")

        self.Payload = None
        self.PayloadSize = 0
        self._XmlTree = None  #private xml structure

        if(filestream == None):
            self.HeaderSignature = PermissionResultVariable.HEADER_SIG_VALUE
            self.HeaderVersion = HdrVersion
            self.Status = 0
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

        if((end - offset) < PermissionResultVariable.STATIC_STRUCT_SIZE): #size of the static header data
            raise Exception("Invalid file stream size")

        self.HeaderSignature = fs.read(4).decode()
        if self.HeaderSignature != PermissionResultVariable.HEADER_SIG_VALUE:
            print ("  HeaderSignature:  %s" % self.HeaderSignature)
            raise Exception("Incorrect Header Signature")
        self.HeaderVersion = struct.unpack("=B", fs.read(1))[0]
        if (self.HeaderVersion != PermissionResultVariable.VERSION_V1 and
            self.HeaderVersion != PermissionResultVariable.VERSION_V2):
            raise Exception("Incorrect Header Version")
        fs.seek(3,1)  #skip three bytes ahead to avoid the rsvd bytes
        self.Status = struct.unpack("=Q", fs.read(8))[0]
        self.SessionId = struct.unpack("=I", fs.read(4))[0]
        if self.HeaderVersion == PermissionResultVariable.VERSION_V2:
            if((end - offset) < PermissionResultVariable.STATIC_STRUCT_SIZE_V2): #size of the static header data
                raise Exception("Invalid file stream size")
            self.PayloadSize = struct.unpack("=H", fs.read(2))[0]
            self.Payload = None
            self._XmlTree = None

            if((end - fs.tell()) < self.PayloadSize):
                raise Exception("Invalid file stream size (Payload).  %d" % self.PayloadSize)

            #is it possible to have 0 sized
            if(self.PayloadSize > 0):
                self.Payload = fs.read(self.PayloadSize)
                self.Payload = self.Payload.decode("utf-8")
                self.Payload = self.Payload.rstrip('\x00') #remove ending NULL if there.  this only happens in some cases
                self._XmlTree = xml.dom.minidom.parseString(self.Payload)

    #
    # Method to Print SEM var to stdout
    #
    def Print(self, ShowRawXmlAsBytes=False):
        print ("PermissionResultVariable")
        print ("  HeaderSignature:  %s" % self.HeaderSignature)
        print ("  HeaderVersion:    0x%X" % self.HeaderVersion)
        print ("  Status:           %s (0x%X)" % (UefiStatusCode().Convert64BitToString(self.Status), self.Status))
        print ("  SessionId:        0x%X" % self.SessionId)

        if self.HeaderVersion == PermissionResultVariable.VERSION_V2:
            print ("  Payload Size:     0x%X" % self.PayloadSize)
            if(self._XmlTree is not None):
                print ("%s" % self._XmlTree.toprettyxml() )
            else:
                print ("XML TREE DOESN'T EXIST" )

            if(ShowRawXmlAsBytes and (self.Payload != None)):
                print ("  Payload Bytes:    " )
                ndbl = memoryview(self.Payload).tolist()
                PrintByteList(ndbl)


    def Write(self, fs):
        raise Exception("Unsupported/Unnecessary function")

        '''fs.write(self.HeaderSignature.encode('utf-8')
        fs.write(struct.pack("=B", self.HeaderVersion))
        fs.write(struct.pack("=B", self.Identity))
        fs.write(struct.pack("=H", self.NewDataSize))
        fs.write(self.NewDataBuffer)
        if(self.Signature != None):
            self.Signature.Write(fs)
            '''
