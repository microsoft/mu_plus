## @file PiFirmwareVolume.py
# Module contains helper classes and functions to work with UEFI FVs.
#
##
# Copyright (c) 2017, Microsoft Corporation
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

import uuid
import struct
import sys

#
# UEFI GUIDs
#
EfiSystemNvDataFvGuid = uuid.UUID(fields=(0xFFF12B8D, 0x7696, 0x4C8B, 0xA9, 0x85, 0x2747075B4F50))

#
# UEFI #Defines
#
EFI_FVH_SIGNATURE     = b"_FVH"

#
# EFI_FIRMWARE_VOLUME_HEADER
# Can parse or produce an EFI_FIRMWARE_VOLUME_HEADER structure/byte buffer.
#
# typedef struct {
#   UINT8                     ZeroVector[16];
#   EFI_GUID                  FileSystemGuid;
#   UINT64                    FvLength;
#   UINT32                    Signature;
#   EFI_FVB_ATTRIBUTES_2      Attributes;
#   UINT16                    HeaderLength;
#   UINT16                    Checksum;
#   UINT16                    ExtHeaderOffset;
#   UINT8                     Reserved[1];
#   UINT8                     Revision;
#   EFI_FV_BLOCK_MAP_ENTRY    BlockMap[1];
# } EFI_FIRMWARE_VOLUME_HEADER;
class EfiFirmwareVolumeHeader(object):
  def __init__(self):
    self.StructString = "=16s16sQ4sLHHHBB"
    self.FileSystemGuid = None
    self.FvLength = None
    self.Attributes = None
    self.HeaderLength = None
    self.ExtHeaderOffset = None
    self.Revision = None

  def load_from_file(self, file):
    # This function assumes that the file has been seeked
    # to the correct starting location.
    orig_seek = file.tell()
    struct_bytes = file.read(struct.calcsize(self.StructString))
    file.seek(orig_seek)

    # Load this object with the contents of the data.
    (zero_vector, self.FileSystemGuid, self.FvLength, self.Signature, self.Attributes, self.HeaderLength,
      checksum, self.ExtHeaderOffset, reserved, self.Revision) = struct.unpack(self.StructString, struct_bytes)

    # Make sure that this structure is what we think it is.
    if self.Signature != EFI_FVH_SIGNATURE:
      raise Exception("File does not appear to point to a valid EfiFirmwareVolumeHeader!")

    # Update the GUID to be a UUID object.
    if sys.byteorder == 'big':
      self.FileSystemGuid = uuid.UUID(bytes=self.FileSystemGuid)
    else:
      self.FileSystemGuid = uuid.UUID(bytes_le=self.FileSystemGuid)

    return self

if __name__ == '__main__':
    pass
