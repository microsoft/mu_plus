##
# Copyright (c) 2016, Microsoft Corporation

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

##
# This is a sample tool and should not be used in production environments.
#  
# This tool takes in sample certificates (.cer files) and outputs a .h file containing the
# certificates.
###

import re
import sys

print "This is a sample tool and should not be used in production environments\n"

raw_input('Press any key to continue . . .\n')


### Parse input parameters ###

if len(sys.argv) == 1 or sys.argv[1] == "-h" or sys.argv[1] == "-H" or sys.argv[1] == "-?":
  print "This tool creates Certs.h with one or more certificates\n"
  print "usage: ConvertCerToH.py <CertFiles...>"
  print "example: ConvertCerToH.py SAMPLE_DEVELOPMENT.cer SAMPLE_PRODUCTION.cer"
  print "example: ConvertCerToH.py SAMPLE_DEVELOPMENT1.cer SAMPLE_DEVELOPMENT2.cer SAMPLE_PRODUCTION.cer"
  print "example: ConvertCerToH.py SAMPLE_PRODUCTION.cer"
  sys.exit(-1)

if len(sys.argv) > 11:
  print "Error: Currently limiting number of certificates to 10"
  print "usage: ConvertCerToH.py <CertFiles...>"
  sys.exit(-1)


### Process Certificates ###

Certs = []
sys.argv.remove(sys.argv[0])

for fileName in sys.argv:
  print "Processing", fileName

  # Open cert file
  file = open(fileName, "rb")

  # Read binary file
  Cert = file.read()

  # Close cert file
  file.close()

  CertHex = map(hex,map(ord,Cert))
  Cert = re.sub(r'\'|\[|\]', "", str(CertHex))
  Certs.append(Cert)


### Write certificates to Certs.h ###

# Open header file
HeaderFile = open("Certs.h", "w")

HeaderFile.write("//\n")
HeaderFile.write("// Certs.h\n")
HeaderFile.write("//\n\n")
HeaderFile.write("//\n")
HeaderFile.write("// These are the binary DER encoded Product Key certificates \n")
HeaderFile.write("// used to sign the UEFI capsule payload.\n")
HeaderFile.write("//\n\n")

index = 1
for Cert in Certs:
  HeaderFile.write("CONST UINT8 CapsulePublicKeyCert"+str(index)+"[] =\n")
  HeaderFile.write("{\n")
  HeaderFile.write(Cert)
  HeaderFile.write("\n};\n\n")
  index = index + 1

HeaderFile.write("CONST CAPSULE_VERIFICATION_CERTIFICATE CapsuleVerifyCertificates[] = {\n")

index = 1
for Cert in Certs:
  HeaderFile.write("  {CapsulePublicKeyCert"+str(index)+", sizeof(CapsulePublicKeyCert"+str(index)+")},\n")
  index = index + 1

HeaderFile.write("};\n\n")

HeaderFile.write("CONST CAPSULE_VERIFICATION_CERTIFICATE_LIST CapsuleVerifyCertificateList = {\n")
HeaderFile.write("  sizeof(CapsuleVerifyCertificates)/sizeof(CAPSULE_VERIFICATION_CERTIFICATE),\n")
HeaderFile.write("  CapsuleVerifyCertificates\n")
HeaderFile.write("};\n\n")


# Close header file
HeaderFile.close()

print "\nCopy the output file Certs.h to folder MsCapsuleUpdatePkg\Library\CapsuleKeyBaseLib"
