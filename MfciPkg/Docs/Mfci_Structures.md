# MFCI Structures

## Policy

MFCI provides 64 bits of configuration policy, 32 bits to be defined by Microsoft, 32 bits reserved
for OEM definition. Each 32-bit range is split into two 16-bit ranges, one for state that
persists across reboots for the duration a policy is installed, and one for 1-time actions that are
triggered once at policy installation. Example actions include Microsoft-defined Secure Boot clear
and TPM clear. An example state might be "OEM Manufacturing Mode foo", which enables modification of
secure configuration.  

[Structure Definition](../Include/MfciPolicyType.h)

## Policy Blob

An MFCI policy blob is a structure that contains the aforementioned 64-bit configuration policy,
along with device targeting information including OEM, model, serial number, 2 OEM-defined fields,
& a security nonce.  

To prevent tampering and provide authentication & authorization, MFCI policy blobs are digitally
signed using the attached, embedded PKCS7 format (not the detached format as is used elsewhere in UEFI).
Authentication of MFCI blobs leverages both a trust anchor public certificate that must be present
in the blob's signing chain, and an Enhanced Key Usage (EKU) that must be present on the leaf
certificate signer. By default, MfciPkg trusts a provided test certificate chain when
```SHIP_MODE==FALSE```, otherwise the certificate and EKU specified by the Microsoft MFCI cloud
signing service.

At runtime, a UEFI implementor may consider the policy blobs to be opaque, as MfciPkg's
[private parsing lib](../Private/Library/MfciPolicyParsingLib) handles
the task of authenticating, verifying targeting, and parsing the blobs down to the 64-bit policies.

For the purpose of testing, a python library and commandline wrappers are provided to facilitate
creation of signed MFCI policies.

* <https://github.com/tianocore/edk2-pytool-library/blob/master/docs/features/windows_firmware_policy.md>
* <https://github.com/tianocore/edk2-pytool-extensions/blob/master/docs/usability/using_firmware_policy_tool.md>

## Targeting Information

Policies are targeted at specific devices via Make (OEM), Model, Serial Number, Nonce, and 2
optional, OEM-defined fields.  The Nonce is a randomly-generated 64-bit integer, and the
remaining fields are unescaped, WIDE NULL-terminated UTF-16LE strings.

## Digital Signature

The PKCS7 digital signature format familiar to the UEFI ecosystem is used with 2 key differences.
First, a full PKCS7 is used, not the SignedData subset used by authenticated variables.  Second,
the P7 is embedded, not detached.  The policy targeting and flavor information are embedded in a
PKCS 7 Data inside the full PKCS 7 Signed object.

### Signing Key

To digitally sign a policy blob, a public/private key combination is required.  The scripts
used to generate test signing keys are included in this repo for reference. Please note that the EKU
needs to include OID `1.3.6.1.5.5.7.3.3` as well as the user designed leaf EKU that is used as an
additional test.  This second EKU must be set in pcd `PcdMfciPkcs7RequiredLeafEKU`.

(../UnitTests/MfciPolicyParsingUnitTest/data/certs/CreateCertificates.ps1)
(../UnitTests/MfciPolicyParsingUnitTest/data/certs/MakeChainingCerts.bat)

### Example signing

[Signtool](https://learn.microsoft.com/en-us/windows/win32/seccrypto/signtool) is available in Windows Kits
and can be used to sign the policy blob.

After the policy blob is created, sign tool can be called, as in the below example. The signed policy blob
has a .p7 extension, and this is the file that should be place into the MfciNext variable.

```
    c:\\Program Files (x86)\\Windows Kits\\10\\bin\\10.0.17763.0\\x64\\signtool.exe
    sign 
    /fd SHA256 
    /p7 .
    /p7co 1.2.840.113549.1.7.1 
    /p7ce Embedded
    /f <pfx file of leaf key>.pfx
    /v /debug 
    /p <password used to secure the leaf key pfx file>
    <input policy>.bin
```

## Signed Packet Example

`certutil.exe -asn`  output for an example MFCI Policy packet (signed) is as follows:

```ASN
0000: 30 82 0e 6c                               ; SEQUENCE (e6c Bytes)
0004:    06 09                                  ; OBJECT_ID (9 Bytes)
0006:    |  2a 86 48 86 f7 0d 01 07  02
         |     ; 1.2.840.113549.1.7.2 PKCS 7 Signed
000f:    a0 82 0e 5d                            ; OPTIONAL[0] (e5d Bytes)
0013:       30 82 0e 59                         ; SEQUENCE (e59 Bytes)
0017:          02 01                            ; INTEGER (1 Bytes)
0019:          |  01
001a:          31 0f                            ; SET (f Bytes)
001c:          |  30 0d                         ; SEQUENCE (d Bytes)
001e:          |     06 09                      ; OBJECT_ID (9 Bytes)
0020:          |     |  60 86 48 01 65 03 04 02  01
               |     |     ; 2.16.840.1.101.3.4.2.1 sha256 (sha256NoSign)
0029:          |     05 00                      ; NULL (0 Bytes)
002b:          30 82 02 17                      ; SEQUENCE (217 Bytes)
002f:          |  06 09                         ; OBJECT_ID (9 Bytes)
0031:          |  |  2a 86 48 86 f7 0d 01 07  01
               |  |     ; 1.2.840.113549.1.7.1 PKCS 7 Data
003a:          |  a0 82 02 08                   ; OPTIONAL[0] (208 Bytes)
003e:          |     04 82 02 04                ; OCTET_STRING (204 Bytes)
```

[policy payload starts here]

```ASN
0042:          |        02 00 01 00 00 00 08 f8  e6 5a 84 83 b9 4e a2 3a  ; .........Z...N.:
0052:          |        0c cc 10 93 e3 dd 00 00  00 00 00 00 00 00 07 00  ; ................
0062:          |        00 00 10 ef 00 00 00 00  0e 00 00 00 28 00 00 00  ; ............(...
0072:          |        00 00 10 ef 58 00 00 00  66 00 00 00 76 00 00 00  ; ....X...f...v...
0082:          |        00 00 10 ef 8e 00 00 00  9c 00 00 00 b6 00 00 00  ; ................
0092:          |        00 00 10 ef e0 00 00 00  ee 00 00 00 fc 00 00 00  ; ................
00a2:          |        00 00 10 ef 0e 01 00 00  1c 01 00 00 2a 01 00 00  ; ............*...
00b2:          |        00 00 10 ef 2e 01 00 00  3c 01 00 00 48 01 00 00  ; ........<...H...
00c2:          |        00 00 10 ef 52 01 00 00  5c 01 00 00 6a 01 00 00  ; ....R...\...j...
00d2:          |        0c 00 54 00 61 00 72 00  67 00 65 00 74 00 18 00  ; ..T.a.r.g.e.t...
00e2:          |        4d 00 61 00 6e 00 75 00  66 00 61 00 63 00 74 00  ; M.a.n.u.f.a.c.t.
00f2:          |        75 00 72 00 65 00 72 00  00 00 2c 00 43 00 6f 00  ; u.r.e.r...,.C.o.
0102:          |        6e 00 74 00 6f 00 73 00  6f 00 20 00 43 00 6f 00  ; n.t.o.s.o. .C.o.
0112:          |        6d 00 70 00 75 00 74 00  65 00 72 00 73 00 2c 00  ; m.p.u.t.e.r.s.,.
0122:          |        20 00 4c 00 4c 00 43 00  0c 00 54 00 61 00 72 00  ;  .L.L.C...T.a.r.
0132:          |        67 00 65 00 74 00 0e 00  50 00 72 00 6f 00 64 00  ; g.e.t...P.r.o.d.
0142:          |        75 00 63 00 74 00 00 00  14 00 4c 00 61 00 70 00  ; u.c.t.....L.a.p.
0152:          |        74 00 6f 00 70 00 20 00  46 00 6f 00 6f 00 0c 00  ; t.o.p. .F.o.o...
0162:          |        54 00 61 00 72 00 67 00  65 00 74 00 18 00 53 00  ; T.a.r.g.e.t...S.
0172:          |        65 00 72 00 69 00 61 00  6c 00 4e 00 75 00 6d 00  ; e.r.i.a.l.N.u.m.
0182:          |        62 00 65 00 72 00 00 00  26 00 46 00 30 00 30 00  ; b.e.r...&.F.0.0.
0192:          |        31 00 33 00 2d 00 30 00  30 00 30 00 32 00 34 00  ; 1.3.-.0.0.0.2.4.
01a2:          |        33 00 35 00 34 00 36 00  2d 00 58 00 30 00 32 00  ; 3.5.4.6.-.X.0.2.
01b2:          |        0c 00 54 00 61 00 72 00  67 00 65 00 74 00 0c 00  ; ..T.a.r.g.e.t...
01c2:          |        4f 00 45 00 4d 00 5f 00  30 00 31 00 00 00 0e 00  ; O.E.M._.0.1.....
01d2:          |        4f 00 44 00 4d 00 20 00  46 00 6f 00 6f 00 0c 00  ; O.D.M. .F.o.o...
01e2:          |        54 00 61 00 72 00 67 00  65 00 74 00 0c 00 4f 00  ; T.a.r.g.e.t...O.
01f2:          |        45 00 4d 00 5f 00 30 00  32 00 00 00 00 00 0c 00  ; E.M._.0.2.......
0202:          |        54 00 61 00 72 00 67 00  65 00 74 00 0a 00 4e 00  ; T.a.r.g.e.t...N.
0212:          |        6f 00 6e 00 63 00 65 00  05 00 ef cd ab 89 67 45  ; o.n.c.e.......gE
0222:          |        23 01 08 00 55 00 45 00  46 00 49 00 0c 00 50 00  ; #...U.E.F.I...P.
0232:          |        6f 00 6c 00 69 00 63 00  79 00 05 00 03 00 00 00  ; o.l.i.c.y.......
0242:          |        00 00 00 00                                       ; ....
```

[policy payload ends here]

[digital signature continues but is not shown here]
