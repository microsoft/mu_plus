# Memory range is either a page table entry, memory map entry,
# or a description of the memory contents.
#
# Copyright (c) 2016, Microsoft Corporation
#
# All rights reserved.
# Redistribution and use in source and binary forms, with or without 
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
##
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


    SystemMemoryTypes = [
        "TSEG"
    ]

    PageSize = {
        "1g" : 1 * 1024 * 1024 * 1024,
        "2m" : 2 * 1024 * 1024,
        "4k" : 4 * 1024,
        }


    @staticmethod
    def attributes_to_flags(attributes):
        # Walk through each of the attribute bit masks defined in MemoryAttributes.
        # If one of them is set in the attributes parameter, add the textual representation
        # to a list and return it.
        return [MemoryRange.MemoryAttributes[attr] for attr in MemoryRange.MemoryAttributes if (attributes & attr) > 0]


    def __init__(self, record_type, *args, **kwargs):
        self.RecordType = record_type
        self.MemoryType = None
        self.SystemMemoryType = None
        self.MustBe1 = None
        self.UserPrivilege = None
        self.ImageName = None
        self.NumberOfEntries = 1
        self.Found = False
        self.Attribute = 0

        # Check to see whether we're a type that we recognize.
        if self.RecordType not in ("TSEG", "MemoryMap", "LoadedImage", "SmmLoadedImage", "PDE", "GDT", "IDT", "PTEntry", "MAT"):
            print(self.RecordType, args)
            raise RuntimeError("Unknown type '%s' found!" % self.RecordType)

        # Continue processing according to the data type.
        if self.RecordType in ("LoadedImage", "SmmLoadedImage"):
            self.LoadedImageEntryInit(int(args[0], 16), int(args[1], 16), args[2])
        elif self.RecordType in ("MemoryMap", "TSEG", "MAT"):
            self.MemoryMapEntryInit(*(int(arg, 16) for arg in args))
        elif self.RecordType in ("PDE", "GDT", "IDT"):
            self.LoadedImageEntryInit(int(args[0], 16), int(args[1], 16), self.RecordType)
        elif self.RecordType in ("PTEntry"):
            self.PteInit(*args)

        self.CalculateEnd()

    def CalculateEnd(self):
        self.PhysicalEnd = self.PhysicalStart + self.PhysicalSize - 1

    def GetMemoryTypeDescription(self):
        if self.MemoryType is None:
            return "None"
        else:
            try: return MemoryRange.MemoryMapTypes[self.MemoryType]
            except: raise Exception("Memory type is invalid")

    def GetSystemMemoryType(self):
        if self.SystemMemoryType is None:
            return "None"
        try:
            return MemoryRange.SystemMemoryTypes[self.SystemMemoryType]
        except:
            raise Exception("System Memory Type is invalid %d" % self.SystemMemoryType)

    
    # 
    # Initializes memory descriptions
    # 
    def MemoryMapEntryInit(self, Type, PhysicalStart, VirtualStart, NumberOfPages, Attribute):
        if(Type < 16):
            self.MemoryType = Type
        else:
            #set it as tseg
            self.SystemMemoryType = 0
        self.PhysicalStart = PhysicalStart
        self.VirtualStart = VirtualStart
        self.PhysicalSize = NumberOfPages * 4 * 1024
        self.Attribute = Attribute
        self.NumberOfPages = NumberOfPages

    # 
    # Intializes memory contents description
    # 
    def LoadedImageEntryInit(self, Base, Size, Name):
        self.PhysicalStart = Base
        self.PhysicalSize = Size
        self.ImageName = Name

    # 
    # Initializes page table entries
    # 
    def PteInit(self, PageSize, ReadWrite, Nx, MustBe1, User, VA):    
        self.MustBe1 = MustBe1
        self.PageSize = PageSize if self.MustBe1 == 1 else "pde"
        self.PhysicalStart = VA
        self.ReadWrite = ReadWrite
        self.Nx = Nx
        self.UserPrivilege = 1
        self.PhysicalSize = self.getPageSize()
        if (self.PageSize == "4k") and (self.MustBe1 == 0):
            raise Exception("Data error: 4K pages must have MustBe1 be set to 1")
   
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
    def toDictionary(self):
        # Pre-process the Section Type
        # Set a reasonable default.
        section_type = "UNEXPECTED VALUE"
        # If there are no attributes, we're done.
        if not self.Attribute:
            section_type = "Not Tracked"
        else:
            attribute_names = MemoryRange.attributes_to_flags(self.Attribute)
            # If the attributes are both XP and RO, that's not good.
            if "EFI_MEMORY_XP" in attribute_names and "EFI_MEMORY_RO" in attribute_names:
                section_type = "ERROR"
            elif "EFI_MEMORY_XP" in attribute_names:
                section_type = "DATA"
            elif "EFI_MEMORY_RO" in attribute_names:
                section_type = "CODE"

        return {
            "Page Size" : self.getPageSizeStr(),
            "Read/Write" : "Enabled" if (self.ReadWrite == 1) else "Disabled",
            "Execute" : "Disabled" if (self.Nx == 1) else "Enabled",
            "Privilege" : "User" if (self.UserPrivilege == 1) else "Supervisor",
            "Start" : "0x{0:010X}".format(self.PhysicalStart),
            "End" : "0x{0:010X}".format(self.PhysicalEnd),
            "Number of Entries" : self.NumberOfEntries,
            "Memory Type" : self.GetMemoryTypeDescription(),
            "Section Type" : section_type,
            "System Memory": self.GetSystemMemoryType(),
            "Memory Contents" : self.ImageName}

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

    def sameAttributes(self, compare):
        if compare is None:
            return False

        if not (((self.PhysicalEnd + 1) == compare.PhysicalStart) or ((compare.PhysicalEnd + 1) == self.PhysicalStart)):
            return False

        if (self.PageSize != compare.PageSize):
            return False

        if (self.ReadWrite != compare.ReadWrite):
            return False

        if (self.MustBe1 != compare.MustBe1):
            return False

        if (self.Nx != compare.Nx):
            return False

        if(self.UserPrivilege != compare.UserPrivilege):
            return False

        if (self.ImageName != compare.ImageName):
            return False

        if (self.MemoryType != compare.MemoryType): 
            return False
        
        if (self.SystemMemoryType != compare.SystemMemoryType):
            return False

        if (self.Attribute != compare.Attribute):
            return False
        
        return True