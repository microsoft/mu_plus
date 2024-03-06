/** @file
  This file contains the information about the certificate that we will search for in the DB
  If the certificate is found in the DB, the system will be considered up to date and
  no recovery attempt will be performed.

  Copyright (C) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef TARGET_CERTIFICATE_H_
#define TARGET_CERTIFICATE_H_

#include <Uefi.h>

//
// Version:          3 (0x02)
// Serial number:    1137338005262830966003041513284644495440216090 (0x330000001a888b9800562284c100000000001a)
// Algorithm ID:     SHA256withRSA
// Validity
//   Not Before:     13/06/2023 18:58:29 (dd-mm-yyyy hh:mm:ss) (230613185829Z)
//   Not After:      13/06/2035 19:08:29 (dd-mm-yyyy hh:mm:ss) (350613190829Z)
// Issuer
//   C  = US
//   ST = Washington
//   L  = Redmond
//   O  = Microsoft Corporation
//   CN = Microsoft Root Certificate Authority 2010
// Subject
//   C  = US
//   O  = Microsoft Corporation
//   CN = Windows UEFI CA 2023
// Public Key
//   Algorithm:      RSA
//   Length:         2048 bits
//   Modulus:        bc:b2:35:d1:54:79:b4:8f:cc:81:2a:6e:b3:12:d6:93:
//                   97:30:7c:38:5c:bf:79:92:19:0a:0f:2d:0a:fe:bf:e0:
//                   a8:d8:32:3f:d2:ab:6f:6f:81:c1:4d:17:69:45:cf:85:
//                   80:27:a3:7c:b3:31:cc:a5:a7:4d:f9:43:d0:5a:2f:d7:
//                   18:1b:d2:58:96:05:39:a3:95:b7:bc:dd:79:c1:a0:cf:
//                   8f:e2:53:1e:2b:26:62:a8:1c:ae:36:1e:4f:a1:df:b9:
//                   13:ba:0c:25:bb:24:65:67:01:aa:1d:41:10:b7:36:c1:
//                   6b:2e:b5:6c:10:d3:4e:96:d0:9f:2a:a1:f1:ed:a1:15:
//                   0b:82:95:c5:ff:63:8a:13:b5:92:34:1e:31:5e:61:11:
//                   ae:5d:cc:f1:10:e6:4c:79:c9:72:b2:34:8a:82:56:2d:
//                   ab:0f:7c:c0:4f:93:8e:59:75:41:86:ac:09:10:09:f2:
//                   51:65:50:b5:f5:21:b3:26:39:8d:aa:c4:91:b3:dc:ac:
//                   64:23:06:cd:35:5f:0d:42:49:9c:4f:0d:ce:80:83:82:
//                   59:fe:df:4b:44:e1:40:c8:3d:63:b6:cf:b4:42:0d:39:
//                   5c:d2:42:10:0c:08:c2:74:eb:1c:dc:6e:bc:0a:ac:98:
//                   bb:cc:fa:1e:3c:a7:83:16:c5:db:02:da:d9:96:df:6b
//   Exponent:       65537 (0x10001)
// Certificate Signature
//   Algorithm:      SHA256withRSA
//   Signature:      9f:c9:b6:ff:6e:e1:9c:3b:55:f6:fe:8b:39:dd:61:04:
//                   6f:d0:ad:63:cd:17:76:4a:a8:43:89:8d:f8:c6:f2:8c:
//                   5e:90:e1:e4:68:a5:15:ec:b8:d3:60:0c:40:57:1f:fb:
//                   5e:35:72:61:de:97:31:6c:79:a0:f5:16:ae:4b:1c:ed:
//                   01:0c:ef:f7:57:0f:42:30:18:69:f8:a1:a3:2e:97:92:
//                   b8:be:1b:fe:2b:86:5e:42:42:11:8f:8e:70:4d:90:a7:
//                   fd:01:63:f2:64:bf:9b:e2:7b:08:81:cf:49:f2:37:17:
//                   df:f1:f9:72:d3:c3:1d:c3:90:45:4d:e6:80:06:bd:fd:
//                   e5:6a:69:ce:b3:7e:4e:31:5b:84:73:a8:e8:72:3f:27:
//                   35:c9:7c:20:ce:00:9b:4f:e0:4c:b4:36:69:cb:f7:34:
//                   11:11:74:12:7a:a8:8c:2e:81:6c:a6:50:ad:19:fa:a8:
//                   46:45:6f:b1:67:73:c3:6b:e3:40:e8:2a:69:8f:24:10:
//                   e1:29:6e:8d:16:88:ee:8e:7f:66:93:02:6f:5b:9e:04:
//                   8c:cc:81:1c:ad:97:54:f1:18:2e:7e:52:90:bc:51:de:
//                   2a:0e:ae:66:ea:bc:64:6e:a0:91:64:e4:2f:12:a8:bc:
//                   e7:6b:ba:c7:1b:9b:79:1a:64:66:f1:43:b4:d1:c3:46:
//                   21:38:81:79:4c:fa:f0:31:0d:d3:79:ff:7a:12:a5:1d:
//                   d9:dd:ac:a2:0f:71:82:f7:93:ff:5c:a1:61:ae:65:f2:
//                   14:81:ed:79:5a:9a:87:ea:60:7b:cb:b3:4f:75:34:ca:
//                   ba:a1:ef:a2:f6:a2:80:45:a1:8b:27:81:cd:d5:77:38:
//                   3e:ca:4e:dd:28:ea:58:ba:c5:a0:29:de:86:8c:88:fc:
//                   95:27:51:dd:ab:d3:d0:5b:0d:77:c7:6c:8f:55:d7:d4:
//                   a2:0e:5b:e4:34:46:14:16:1d:e3:1c:d6:6d:99:ad:4c:
//                   ec:71:73:2f:ab:ce:b2:b4:29:de:55:30:53:39:3a:32:
//                   8b:f0:ea:9c:88:12:3b:05:68:19:bf:cf:87:52:10:fb:
//                   d6:13:60:f3:41:64:f4:08:57:81:cb:9d:11:a5:8e:f4:
//                   e5:27:f5:a3:3a:ec:e4:3d:4a:b7:ce:f9:88:0d:9f:bd:
//                   ca:6d:d2:4a:bc:58:76:8e:32:04:94:6e:dd:f4:cf:6d:
//                   47:6d:c2:d7:6a:dc:87:71:ea:a4:bf:ef:67:97:9c:b8:
//                   c7:80:36:2a:2a:59:c9:c0:0c:a7:44:a0:73:b5:8c:cf:
//                   38:5a:ae:f8:bb:86:95:f0:44:ad:66:7a:33:ed:71:e4:
//                   45:87:83:e5:a7:ce:a2:40:d0:72:d2:48:00:fa:f9:1a
//
// Extensions
//   keyUsage CRITICAL:
//     digitalSignature,keyCertSign,cRLSign
//   1.3.6.1.4.1.311.21.1 :
//   subjectKeyIdentifier :
//     aefc5fbbbe055d8f8daa585473499417ab5a5272
//   1.3.6.1.4.1.311.20.2 :
//   basicConstraints CRITICAL:
//     cA=true
//   authorityKeyIdentifier :
//     kid=d5f656cb8fe8a25c6268d13d94905bd7ce9a18c4
//   cRLDistributionPoints :
//     http://crl.microsoft.com/pki/crl/products/MicRooCerAut_2010-06-23.crl
//   authorityInfoAccess :
//     caissuer: http://www.microsoft.com/pki/certs/MicRooCerAut_2010-06-23.crt
//

//
// The certificate we are looking for
//
#define CERT_ORGANIZATION         "Microsoft Corporation"
#define CERT_ORGANIZATION_OFFSET  0xF6
#define CERT_COMMON_NAME          "Windows UEFI CA 2023"
#define CERT_COMMON_NAME_OFFSET   0x116
#define CERT_SIGNATURE            { 0x9f, 0xc9, 0xb6, 0xff, 0x6e, 0xe1, 0x9c, 0x3b, 0x55, 0xf6, 0xfe, 0x8b, 0x39, 0xdd, 0x61, 0x04 }
#define CERT_SIGNATURE_OFFSET     0x3AE

//
// This is the certificate entry structure
// It is used to check if the certificate is already in the DB
//
typedef struct {
  CHAR8    *Organization;      // The organization name in the certificate
  UINTN    OrganizationLength; // The length of the organization name in the certificate
  UINTN    OrganizationOffset; // The offset of the organization name in the certificate
  CHAR8    *CommonName;        // The common name in the certificate
  UINTN    CommonNameLength;   // The length of the common name in the certificate
  UINTN    CommonNameOffset;   // The offset of the common name in the certificate
  UINT8    Signature[16];      // The signature of the certificate
  UINTN    SignatureOffset;    // The offset of the signature in the certificate
} CERTIFICATE_ENTRY;

//
// This is Certificate entry for the Windows UEFI CA 2023 certificate
// This will be used to check if the certificate is already in the DB
//
CERTIFICATE_ENTRY  mTargetCertificate = {
  .Organization       = CERT_ORGANIZATION,
  .OrganizationLength = sizeof (CERT_ORGANIZATION) - 1,
  .OrganizationOffset = CERT_ORGANIZATION_OFFSET,
  .CommonName         = CERT_COMMON_NAME,
  .CommonNameLength   = sizeof (CERT_COMMON_NAME) - 1,
  .CommonNameOffset   = CERT_COMMON_NAME_OFFSET,
  .Signature          = CERT_SIGNATURE,
  .SignatureOffset    = CERT_SIGNATURE_OFFSET
};

#endif // TARGET_CERTIFICATE_H_
