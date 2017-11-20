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

    SystemMemoryTypes = [
        "TSEG"
    ]

    PageSize = {
        "1g" : 1 * 1024 * 1024 * 1024,
        "2m" : 2 * 1024 * 1024,
        "4k" : 4 * 1024,
        }

    def __init__(self, *args, **kwargs):
        self.MemoryType = None
        self.SystemMemoryType = None
        self.MustBe1 = None
        self.ImageName = None
        self.NumberOfEntries = 1
        self.Found = False

        # Initializes depending on number and type arguments passed in
        if len(args) == 3:
            self.LoadedImageEntryInit(*args)
        else:
            try:
                int(args[0])
                self.MemoryMapEntryInit(*args)
            except:   
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

    def __str__(self):
        if self.MustBe1 is not None:
            return self.PteToString()
        elif self.MemoryType is not None:
            return self.MemoryRangeToString()
        elif self.ImageName is not None:
            return self.LoadedImageEntryToString()
    
    __repr__ = __str__


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

    def MemoryRangeToString(self):
        return """\n  Memory Map Entry
------------------------------------------------------------------
    Type                    : %s
    PhysicalStart           : 0x%010X
    VirtualStart            : 0x%010X
    NumberOfPages           : 0x%010X
    Attribute               : 0x%010X
    PhysicalSize            : 0x%010X
""" % (self.GetMemoryTypeDescription(), self.PhysicalStart, self.VirtualStart, self.NumberOfPages, self.Attribute, 0)

    # 
    # Intializes memory contents description
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
    # 
    # Initializes page table entries
    # 
    def PteInit(self, PageSize, ReadWrite, Nx, MustBe1, VA):    
        self.MustBe1 = MustBe1
        self.PageSize = PageSize if self.MustBe1 == 1 else "pde"
        self.PhysicalStart = VA
        self.ReadWrite = ReadWrite
        self.Nx = Nx
        self.PhysicalSize = self.getPageSize()
        if (self.PageSize == "4k") and (self.MustBe1 == 0):
            raise Exception("Data error: 4K pages must have MustBe1 be set to 1")


    def PteToString(self):
        return """%s,%X,%X,%X,0x%010X,0x%010X,%d,%s,%s""" % (self.getPageSizeStr(), self.ReadWrite, self.Nx, self.MustBe1, self.PhysicalStart, self.PhysicalEnd, self.NumberOfEntries, self.GetMemoryTypeDescription(), self.ImageName )
    
   
    def getPageSize(self):
        return MemoryRange.PageSize[self.PageSize]

    def getPageSizeStr(self):
        return self.PageSize

    def pteDebugStr(self):
        return """\n  %s
------------------------------------------------------------------
    MustBe1                 : 0x%010X
    ReadWrite               : 0x%010X
    Nx                      : 0x%010X
    PhysicalStart           : 0x%010X
    PhysicalEnd             : 0x%010X
    PhysicalSize            : 0x%010X
    Number                  : 0x%010X
    Type                    : %s
    System Type             : %s
    LoadedImage             : %s
""" % (self.getPageSizeStr(), self.MustBe1, self.ReadWrite, self.Nx,  self.PhysicalStart, self.PhysicalEnd, self.PhysicalSize, self.NumberOfEntries, self.GetMemoryTypeDescription(), self.GetSystemMemoryType() ,self.ImageName )
    
    # Used to combine two page table entries with the same attributes
    def grow(self, other):
        self.NumberOfEntries += other.NumberOfEntries
        self.PhysicalSize += other.PhysicalSize
        self.CalculateEnd()

    # Returns dict describing this object
    def toDictionary(self):
        return {
            "Page Size" : self.getPageSizeStr(),
            "Read/Write" : "Enabled" if (self.ReadWrite == 1) else "Diabled",
            "Execute" : "Disabled" if (self.Nx == 1) else "Enabled",
            "Start" : "0x{0:010X}".format(self.PhysicalStart),
            "End" : "0x{0:010X}".format(self.PhysicalEnd),
            "Number of Entries" : self.NumberOfEntries,
            "Memory Type" : self.GetMemoryTypeDescription(),
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

        if (self.ImageName != compare.ImageName):
            return False

        if (self.MemoryType != compare.MemoryType): 
            return False
        
        if (self.SystemMemoryType != compare.SystemMemoryType):
            return False
        
        return True
