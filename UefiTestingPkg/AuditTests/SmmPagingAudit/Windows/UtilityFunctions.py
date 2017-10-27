# Functions used to compare memory ranges.
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

import glob

def sameAttributes(i, j):
    if i is None or j is None:
        return False

    iLow = i.PhysicalStart
    iHigh = i.PhysicalEnd

    jLow = j.PhysicalStart
    jHigh = j.PhysicalEnd

    if not (((iHigh + 1) == jLow) or ((jHigh + 1) == iLow)):
        return False
    if (i.PageSize != j.PageSize):
        return False

    elif (i.ReadWrite != j.ReadWrite):
        return False

    elif (i.MustBe1 != j.MustBe1):
        return False

    elif (i.Nx != j.Nx):
        return False

    elif (i.ImageName != j.ImageName):
        return False

    elif (i.MemoryType != j.MemoryType): 
        return False
    else:
        return True


def overlap(i, j):
    iLow = i.PhysicalStart
    iHigh = i.PhysicalEnd

    jLow = j.PhysicalStart
    jHigh = j.PhysicalEnd

    if (iLow >= jLow) and (iLow <= jHigh):
        return True

    if (jLow >= iLow) and (jLow <= iHigh):
        return True

    return False
    
def eq(i, j):
    iLow = i.PhysicalStart
    iHigh = i.PhysicalEnd

    jLow = j.PhysicalStart
    jHigh = j.PhysicalEnd

    if (iLow == jLow) and (iHigh == jHigh):
        return True

    return False