import copy
import logging
from enum import Enum

# Memory range is either a page table entry, memory map entry,
# or a description of the memory contents.
#
# Copyright (C) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

ParsingArchitecture = ""

def SetArchitecture(arch):
    global ParsingArchitecture
    ParsingArchitecture = arch

class SystemMemoryTypes(Enum):
    TSEG = "TSEG"
    GuardPage = "GuardPage"
    NullPage = "NULL Page"
    StackGuard = "BSP Stack Guard"
    Stack = "BSP Stack"
    ApStackGuard = "AP {0} Stack Guard"
    ApStack = "AP {0} Stack"
    ApSwitchStack = "AP {0} Switch Stack"

class MemoryRange(object):
    # MemmoryMap type list
    MemoryMapTypes = [
        "EfiReservedMemoryType",
        "EfiLoaderCode",
        "EfiLoaderData",
        "EfiBootServicesCode",
        "EfiBootServicesData",
        "EfiRuntimeServicesCode",
        "EfiRuntimeServicesData",
        "EfiConventionalMemory",
        "EfiUnusableMemory",
        "EfiACPIReclaimMemory",
        "EfiACPIMemoryNVS",
        "EfiMemoryMappedIO",
        "EfiMemoryMappedIOPortSpace",
        "EfiPalCode",
        "EfiPersistentMemory",
        "EfiMaxMemoryType"
        ]

    MemorySpaceTypes = [
        "EfiGcdMemoryTypeNonExistent",
        "EfiGcdMemoryTypeReserved",
        "EfiGcdMemoryTypeSystemMemory",
        "EfiGcdMemoryTypeMemoryMappedIo",
        "EfiGcdMemoryTypePersistent",
        "EfiGcdMemoryTypePersistentMemory",
        "EfiGcdMemoryTypeMoreReliable",
        "EfiGcdMemoryTypeMaximum"
        ]

    # Definitions from Uefi\UefiSpec.h
    MemoryAttributes = {
        0x0000000000000001 : "EFI_MEMORY_UC",
        0x0000000000000002 : "EFI_MEMORY_WC",
        0x0000000000000004 : "EFI_MEMORY_WT",
        0x0000000000000008 : "EFI_MEMORY_WB",
        0x0000000000000010 : "EFI_MEMORY_UCE",
        0x0000000000001000 : "EFI_MEMORY_WP",
        0x0000000000002000 : "EFI_MEMORY_RP",
        0x0000000000004000 : "EFI_MEMORY_XP",
        0x0000000000020000 : "EFI_MEMORY_RO",
        0x0000000000008000 : "EFI_MEMORY_NV",
        0x8000000000000000 : "EFI_MEMORY_RUNTIME",
        0x0000000000010000 : "EFI_MEMORY_MORE_RELIABLE"
    }

    PageSize = {
        "1g" : 1 * 1024 * 1024 * 1024,
        "2m" : 2 * 1024 * 1024,
        "4k" : 4 * 1024,
        }

    TsegEfiMemoryType = 16
    NoneGcdMemoryType = 7

    @staticmethod
    def attributes_to_flags(attributes):
        # Walk through each of the attribute bit masks defined in MemoryAttributes.
        # If one of them is set in the attributes parameter, add the textual representation
        # to a list and return it.
        return [MemoryRange.MemoryAttributes[attr] for attr in MemoryRange.MemoryAttributes if (attributes & attr) > 0]


    def __init__(self, record_type, *args, **kwargs):
        self.RecordType = record_type
        self.MemoryType = None
        self.GcdType = None
        self.SystemMemoryType = None
        self.MustBe1 = None
        self.UserPrivilege = None
        self.ImageName = None
        self.NumberOfEntries = 1
        self.Found = False
        self.Attribute = 0
        self.AddressBitwidth = None
        self.PageSplit = False
        self.CpuNumber = None

        # Continue processing according to the data type.
        if self.RecordType in ("LoadedImage", "SmmLoadedImage"):
            self.LoadedImageEntryInit(int(args[0], 16), int(args[1], 16), args[2])
        elif self.RecordType in ("MemoryMap", "TSEG", "MAT"):
            self.MemoryMapEntryInit(*(int(arg, 16) for arg in args))
        elif self.RecordType in ("PDE", "GDT", "IDT"):
            self.LoadedImageEntryInit(int(args[0], 16), int(args[1], 16), self.RecordType)
        elif self.RecordType in ("PTEntry"):
            self.PteInit(*args)
        elif self.RecordType in ("TTEntry"):
            self.TteInit(*args)
        elif self.RecordType in ("GuardPage"):
            self.GuardPageInit(*args)
        elif self.RecordType in ("Bitwidth"):
            self.BitwidthInit(int(args[0], 16))
        elif self.RecordType in ("Stack", "StackGuard"):
            self.StackInit (self.RecordType, *(int(arg, 16) for arg in args))
        elif self.RecordType in ("ApStack", "ApStackGuard", "ApSwitchStack"):
            self.ApStackInit (self.RecordType, *(int(arg, 16) for arg in args))
        elif self.RecordType in ("Null"):
            self.NullInit (self.RecordType, *(int(arg, 16) for arg in args))
        else:
            raise RuntimeError("Unknown type '%s' found!" % self.RecordType)
        self.CalculateEnd()

    def CalculateEnd(self):
        self.PhysicalEnd = self.PhysicalStart + self.PhysicalSize - 1

    def GetMemoryTypeDescription(self):
        if self.MemoryType is None:
            return "None"
        else:
            try: return MemoryRange.MemoryMapTypes[self.MemoryType]
            except: raise Exception("Memory type is invalid")
    
    def GetGcdTypeDescription(self):
        if self.GcdType is None:
            return "None"
        else:
            try: return MemoryRange.MemorySpaceTypes[self.GcdType]
            except: raise Exception("Gcd type is invalid")

    def GetSystemMemoryType(self):
        if self.SystemMemoryType is None:
            return "None"
        try:
            # Some SystemMemoryType objects correlate with a formatted string in the SystemMemoryTypes list.
            # The formatted string includes a place for the CPU number of the memory range object.
            if (self.SystemMemoryType == SystemMemoryTypes.ApStack or self.SystemMemoryType == SystemMemoryTypes.ApStackGuard or self.SystemMemoryType == SystemMemoryTypes.ApSwitchStack):
                return self.SystemMemoryType.value.format(self.CpuNumber)

            return self.SystemMemoryType.value
        except:
            raise ValueError("System Memory Type is invalid %s" % self.SystemMemoryType.value)

    def StackInit (self, SystemMemoryType, PhysicalStart, Length):
        self.PhysicalStart = PhysicalStart
        self.PhysicalSize = Length

        # Convert the system type to an index in the object list of memory types
        if SystemMemoryType == "StackGuard":
            self.SystemMemoryType = SystemMemoryTypes.StackGuard
        elif SystemMemoryType == "Stack":
            self.SystemMemoryType = SystemMemoryTypes.Stack
        else:
            raise ValueError("System Memory Type is invalid %s" % SystemMemoryType)

    def ApStackInit (self, SystemMemoryType, PhysicalStart, Length, CpuNumber):
        self.PhysicalStart = PhysicalStart
        self.PhysicalSize = Length
        self.CpuNumber = CpuNumber
        
        # Convert the system type to an index in the object list of memory types
        if SystemMemoryType == "ApStackGuard":
            self.SystemMemoryType = SystemMemoryTypes.ApStackGuard
        elif SystemMemoryType == "ApStack":
            self.SystemMemoryType = SystemMemoryTypes.ApStack
        elif SystemMemoryType == "ApSwitchStack":
            self.SystemMemoryType = SystemMemoryTypes.ApSwitchStack
        else:
            raise ValueError("System Memory Type is invalid %s" % SystemMemoryType)
    
    def NullInit (self, SystemMemoryType, PhysicalStart):
        self.PhysicalStart = PhysicalStart
        self.PhysicalSize = MemoryRange.PageSize["4k"]

        # Convert the system type to an index in the object list of memory types
        if SystemMemoryType == "Null":
            self.SystemMemoryType = SystemMemoryTypes.NullPage
        else:
            raise ValueError("System Memory Type is invalid %s" % SystemMemoryType)

    #
    # Initializes memory descriptions
    #
    def MemoryMapEntryInit(self, Type, PhysicalStart, VirtualStart, NumberOfPages, Attribute, GcdType):
        if(Type < 16):
            self.MemoryType = Type
        if (Type == self.TsegEfiMemoryType):
            #set it as tseg
            self.SystemMemoryType = SystemMemoryTypes.TSEG
        self.PhysicalStart = PhysicalStart
        self.VirtualStart = VirtualStart
        self.PhysicalSize = NumberOfPages * 4 * 1024
        self.Attribute = Attribute
        self.NumberOfPages = NumberOfPages
        if (GcdType < self.NoneGcdMemoryType):
            self.GcdType = GcdType

    def MemoryRangeToString(self):
        return """\n  Memory Map Entry
------------------------------------------------------------------
    Type                    : %s
    PhysicalStart           : 0x%010X
    PhysicalEnd             : 0x%010X
    Attribute               : 0x%010X
    PhysicalSize            : 0x%010X
""" % (self.GetMemoryTypeDescription(), self.PhysicalStart, self.PhysicalEnd, self.Attribute, self.PhysicalSize)

    #
    # intitalizes memory contents description
    #
    def LoadedImageEntryInit(self, Base, Size, Name):
        self.PhysicalStart = Base
        self.PhysicalSize = Size
        self.ImageName = Name
    def LoadedImageEntryToString(self):
        return """\n  Loaded Image
------------------------------------------------------------------
    PhysicalStart           : 0x%010X
    PhysicalEnd             : 0x%010X
    PhysicalSize            : 0x%010X
    Name                    : %s
""" % (self.PhysicalStart, (self.PhysicalEnd), self.PhysicalSize, self.ImageName)

    def GuardPageInit(self, VA):
        self.SystemMemoryType = SystemMemoryTypes.GuardPage
        self.PhysicalStart = int(VA, 16)
        self.PageSize = "4k"
        self.PhysicalSize = self.getPageSize()
        if ParsingArchitecture == "AARCH64":
            self.AccessFlag = 0
            self.ReadWrite = 0
            self.Ux = 0
            self.Px = 0
        else:
            self.ReadWrite = 0
            self.UserPrivilege = 1
            self.Nx = 0
            self.Present = 0

    def BitwidthInit(self, Bitwidth):
        self.AddressBitwidth = Bitwidth
        self.PhysicalStart = 0
        self.PhysicalSize = (1 << self.AddressBitwidth)
    
    def TteInit(self, PageSize, AccessFlag, ReadWrite, Sharability, Pxn, Uxn, VA, IsTable):
        self.PageSize = PageSize
        self.AccessFlag = AccessFlag
        self.ReadWrite = 0 if (ReadWrite == 1) else 1
        self.Sharability = Sharability
        self.Px = 0 if (Pxn == 1) else 1
        self.Ux = 0 if (Uxn == 1) else 1
        self.PhysicalStart = VA
        self.IsTable = IsTable
        self.PhysicalSize = self.getPageSize()

    #
    # Initializes page table entries
    #
    def PteInit(self, PageSize, Present, ReadWrite, Nx, MustBe1, User, VA):
        self.MustBe1 = MustBe1
        self.PageSize = PageSize if self.MustBe1 == 1 else "pde"
        self.PhysicalStart = VA
        self.ReadWrite = ReadWrite
        self.Nx = Nx
        self.UserPrivilege = User
        self.PhysicalSize = self.getPageSize()
        self.Present = Present
        if (self.PageSize == "4k") and (self.MustBe1 == 0):
            raise Exception("Data error: 4K pages must have MustBe1 be set to 1")
    def pageDebugStr(self):
        return """\n  %s
------------------------------------------------------------------
    PhysicalStart           : 0x%010X
    PhysicalEnd             : 0x%010X
    PhysicalSize            : 0x%010X
    Number                  : 0x%010X
    Type                    : %s
    LoadedImage             : %s
""" % (self.getPageSizeStr(), self.PhysicalStart, self.PhysicalEnd, self.PhysicalSize, self.NumberOfEntries, self.GetMemoryTypeDescription(), self.ImageName )

    def getPageSize(self):
        return MemoryRange.PageSize[self.PageSize]

    def getPageSizeStr(self):
        return self.PageSize

    # Used to combine two page table entries with the same attributes
    def grow(self, other):
        self.NumberOfEntries += other.NumberOfEntries
        self.PhysicalSize += other.PhysicalSize
        self.CalculateEnd()

    # Returns dict describing this object
    def toDictionary(self, architecture):
        # Pre-process the Section Type
        # Set a reasonable default.
        section_type = "UNEXPECTED VALUE"
        if architecture == "X64":
            # If this range is not associated with an image, it does not have
            # a section type.
            if self.ImageName == None:
                section_type = "Not Tracked"
            else:
                # if an image range can't be read or executed, this is almost certainly
                # an error.
                if self.Nx == 1 and self.ReadWrite == 0:
                    section_type = "ERROR"
                elif self.Nx == 1:
                    section_type = "DATA"
                elif self.ReadWrite == 0:
                    section_type = "CODE"

            return {
                "Page Size" : self.getPageSizeStr(),
                "Present" : "Yes" if (self.Present == 1) else "No",
                "Read/Write" : "Enabled" if (self.ReadWrite == 1) else "Disabled",
                "Execute" : "Disabled" if (self.Nx == 1) else "Enabled",
                "Privilege" : "User" if (self.UserPrivilege == 1) else "Supervisor",
                "Start" : "0x{0:010X}".format(self.PhysicalStart),
                "End" : "0x{0:010X}".format(self.PhysicalEnd),
                "Number of Entries" : self.NumberOfEntries if (not self.PageSplit) else str(self.NumberOfEntries) + " (p)" ,
                "Memory Type" : self.GetMemoryTypeDescription(),
                "GCD Memory Type" : self.GetGcdTypeDescription(),
                "Section Type" : section_type,
                "System Memory": self.GetSystemMemoryType(),
                "Memory Contents" : self.ImageName,
                "Partial Page": self.PageSplit}
        elif architecture == "AARCH64":
            # If this range is not associated with an image, it does not have
            # a section type.
            if self.ImageName == None:
                section_type = "Not Tracked"
            else:
                # if an image range can't be read or executed, this is almost certainly
                # an error.
                if self.Ux == 0 and self.ReadWrite == 0:
                    section_type = "ERROR"
                elif self.Ux == 0:
                    section_type = "DATA"
                elif self.ReadWrite == 0:
                    section_type = "CODE"
                else:
                    section_type = "UNKNOWN"

            # Check the execution setting
            ExecuteString = "Disabled"
            if (self.Ux and self.Px):
                ExecuteString = "UX/PX"
            elif (self.Ux):
                ExecuteString = "UX"
            elif (self.Px):
                ExecuteString = "PX"

            return {
                "Page Size" : self.getPageSizeStr(),
                "Access Flag" : "Yes" if (self.AccessFlag == 1) else "No",
                "Read/Write" : "Enabled" if (self.ReadWrite == 1) else "Disabled",
                "Execute" : ExecuteString,
                "Start" : "0x{0:010X}".format(self.PhysicalStart),
                "End" : "0x{0:010X}".format(self.PhysicalEnd),
                "Number of Entries" : self.NumberOfEntries if (not self.PageSplit) else str(self.NumberOfEntries) + " (p)" ,
                "Memory Type" : self.GetMemoryTypeDescription(),
                "GCD Memory Type" : self.GetGcdTypeDescription(),
                "Section Type" : section_type,
                "System Memory": self.GetSystemMemoryType(),
                "Memory Contents" : self.ImageName,
                "Partial Page": self.PageSplit}

    def overlap(self, compare):
        if(self.PhysicalStart >= compare.PhysicalStart) and (self.PhysicalStart <= compare.PhysicalEnd):
            return True

        if(compare.PhysicalStart >= self.PhysicalStart) and (compare.PhysicalStart <= self.PhysicalEnd):
            return True

        return False

    def eq(self, compare):
        if (self.PhysicalStart == compare.PhysicalStart) and (self.PhysicalEnd == compare.PhysicalEnd):
            return True

        return False

    def split(self, end_of_current):
        ''' split the memory range object into two and copy
        all attributes to the new object.
        end_of_current = physical end of the first object.

        modify self
        return
        '''
        if end_of_current is None:
            raise Exception("Failed Split - Invalid parameter.  end_of_current can not be none")

        if end_of_current <= self.PhysicalStart:
            raise Exception("Failed Split - Invalid parameter.  end_of_current can not be <= start. " +
                            f"self.PhysicalStart = {self.PhysicalStart} " +
                            f"end_of_current = {end_of_current}" )

        if end_of_current >= self.PhysicalEnd:
            raise Exception("Failed Split - Invalid parameter.  end_of_current can not be >= end" +
                            f"self.PhysicalEnd = {self.PhysicalEnd} " +
                            f"end_of_current = {end_of_current}" )

        self.PageSplit = True
        next = copy.deepcopy(self)
        self.PhysicalEnd = end_of_current
        next.PhysicalStart = end_of_current +1
        return next

    def sameAttributes(self, compare, architecture):
        if compare is None:
            return False

        if not (((self.PhysicalEnd + 1) == compare.PhysicalStart) or ((compare.PhysicalEnd + 1) == self.PhysicalStart)):
            return False

        if (self.PageSize != compare.PageSize):
            return False

        if (self.PageSplit or compare.PageSplit):
            return False

        if (self.ImageName != compare.ImageName):
            return False

        if (self.MemoryType != compare.MemoryType):
            return False
        
        if (self.GcdType != compare.GcdType):
            return False

        if (self.SystemMemoryType != compare.SystemMemoryType):
            return False

        if (self.Attribute != compare.Attribute):
            return False

        if (self.ReadWrite != compare.ReadWrite):
            return False

        if architecture == "X64":
            if (self.MustBe1 != compare.MustBe1):
                return False

            if (self.Present != compare.Present):
                return False

            if (self.Nx != compare.Nx):
                return False

            if(self.UserPrivilege != compare.UserPrivilege):
                return False

        elif architecture == "AARCH64":
            if (self.IsTable != compare.IsTable):
                return False

            if (self.AccessFlag != compare.AccessFlag):
                return False

            if (self.Ux != compare.Ux):
                return False

            if (self.Px != compare.Px):
                return False

        else:
            return False

        return True