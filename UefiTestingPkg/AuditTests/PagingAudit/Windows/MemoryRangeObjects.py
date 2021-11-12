import copy

# Memory range is either a page table entry, memory map entry,
# or a description of the memory contents.
#
# Copyright (C) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: BSD-2-Clause-Patent
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
        "TSEG",
        "GuardPage"
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
        self.AddressBitwidth = None
        self.PageSplit = False

        # Check to see whether we're a type that we recognize.
        if self.RecordType not in ("TSEG", "MemoryMap", "LoadedImage", "SmmLoadedImage", "PDE", "GDT", "IDT", "PTEntry", "MAT", "GuardPage", "Bitwidth"):
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
        elif self.RecordType in ("GuardPage"):
            self.GuardPageInit(*args)
        elif self.RecordType in ("Bitwidth"):
            self.BitwidthInit(int(args[0], 16))

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

    def MemoryRangeToString(self):
        return """\n  Memory Map Entry
------------------------------------------------------------------
    Type                    : %s
    PhysicalStart           : 0x%010X
    PhysicalEnd             : 0x%010X
    VirtualStart            : 0x%010X
    NumberOfPages           : 0x%010X
    Attribute               : 0x%010X
    PhysicalSize            : 0x%010X
""" % (self.GetMemoryTypeDescription(), self.PhysicalStart, self.PhysicalEnd, self.VirtualStart, self.NumberOfPages, self.Attribute, self.PhysicalSize)

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
        self.SystemMemoryType = 1
        self.PhysicalStart = int(VA, 16)
        self.PageSize = "4k"
        self.PhysicalSize = self.getPageSize()
        self.ReadWrite = 0
        self.UserPrivilege = 1
        self.Nx = 0

    def BitwidthInit(self, Bitwidth):
        self.AddressBitwidth = Bitwidth
        self.PhysicalStart = 0
        self.PhysicalSize = (1 << self.AddressBitwidth)

    #
    # Initializes page table entries
    #
    def PteInit(self, PageSize, ReadWrite, Nx, MustBe1, User, VA):
        self.MustBe1 = MustBe1
        self.PageSize = PageSize if self.MustBe1 == 1 else "pde"
        self.PhysicalStart = VA
        self.ReadWrite = ReadWrite
        self.Nx = Nx
        self.UserPrivilege = User
        self.PhysicalSize = self.getPageSize()
        if (self.PageSize == "4k") and (self.MustBe1 == 0):
            raise Exception("Data error: 4K pages must have MustBe1 be set to 1")
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
    LoadedImage             : %s
""" % (self.getPageSizeStr(), self.MustBe1, self.ReadWrite, self.Nx,  self.PhysicalStart, self.PhysicalEnd, self.PhysicalSize, self.NumberOfEntries, self.GetMemoryTypeDescription(), self.ImageName )

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
            "Number of Entries" : self.NumberOfEntries if (not self.PageSplit) else str(self.NumberOfEntries) + " (p)" ,
            "Memory Type" : self.GetMemoryTypeDescription(),
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

        if (self.PageSplit or compare.PageSplit):
            return False

        return True