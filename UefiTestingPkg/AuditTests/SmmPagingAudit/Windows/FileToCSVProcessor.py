# Collects files from UEFI app, parses them, writes them to a CSV file.
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

from MemoryRangeObjects import *
from UtilityFunctions import *
from BinaryParsing import *
import operator
import glob
import csv

# Looking for .dat files from the UEFI app
infos =  glob.glob(".\*MemoryInfo*.dat")
pte1gs =  glob.glob(".\*1G*.dat")
pte2ms =  glob.glob(".\*2M*.dat")
pte4ks =  glob.glob(".\*4K*.dat")

debugFile = open("debug.txt", "w")

MemoryRanges = []
ptes = []

# Parse each file, keeping PTEs and "Memory Ranges" seperate
# Memory ranges are either "memory descriptions" for memory map types and TSEG
# or "memory contents" for loaded image information or IDT/GDT
for info in infos:
    MemoryRanges.extend(ParseInfoFile(info))

for pte1g in pte1gs:
    ptes.extend(Parse1gPages(pte1g))

for pte2m in pte2ms:
    ptes.extend(Parse2mPages(pte2m))

for pte4k in pte4ks:
    ptes.extend(Parse4kPages(pte4k))

if len(ptes) == 0 or len(MemoryRanges) == 0:
    raise Exception("Need to be in same directory as .dat files outputted from the UEFI app")


# Sort in descending order
ptes.sort(key=operator.attrgetter('PhysicalStart'))

# Matching memory ranges up to page table entries
for pte in ptes:
    for mr in MemoryRanges:
        if overlap(pte, mr):
            if mr.MemoryType is not None:
                if pte.MemoryType is None:
                    pte.MemoryType = mr.MemoryType
                else:
                    debugFile.write("Multiple memory types found for one region")
                    debugFile.write(pte.pteDebugStr())
                    debugFile.write(mr.MemoryRangeToString())
            elif mr.ImageName is not None:
                if pte.ImageName is None:
                    pte.ImageName = mr.ImageName
                else:
                    debugFile.write("Multiple memory contents found for one region")
                    debugFile.write(pte.pteDebugStr())
                    debugFile.write(mr.LoadedImageEntryToString())


# Combining adjacent PTEs that have the same attributes.
index = 0
while index < (len(ptes) - 1):
    currentPte = ptes[index]
    nextPte = ptes[index + 1]
    if sameAttributes(currentPte, nextPte):
        currentPte.grow(nextPte)
        del ptes[index + 1]
    else:
        index += 1

# Write PTEs to output file
ptecount = 0
with open('output.csv', 'wb') as csvfile:
    writer = csv.DictWriter(csvfile, fieldnames=MemoryRange.pteFieldNames)
    writer.writeheader()
    for pte in ptes:
        ptecount += pte.NumberOfEntries
        writer.writerow(pte.toCsvRow())

print "Number of unique ptes {0:d}".format(len(ptes))
print "Number of ptes total {0:d}".format(ptecount)

debugFile.close()

print "debug at debug.txt"
print "output at output.csv"