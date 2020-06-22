# **DRAFT:** Manufacturer Firmware Configuration Interface (MFCI)

## **NOTE**

The following is prerelease documentation for an upcoming feature.  Details are subject to change.

## Overview

### What Is It

Manufacturer Firmware Configuration Interface (MFCI) provides a mechanism for an authenticated
agent, such as an OEM or ODM, to modify firmware security properties of a specific device, typically
for the purposes of remanufacturing or refurbishment.

### Background

Manufacturing and remanufacturing (refurbishment) of devices requires configuration of settings
related to safety (batteries, thermals), compliance (radio calibration, anti-theft), licensing
(OA3, serial numbers), & security.  These sensitive settings are typically secured by the
OEM / ODM at end of manufacturing to ensure that they are not modified with malicious intent.
During initial manufacturing, a device is typically not secured until the final provisioning
station, after calibration and other secure data have been provisioned.  After a device is secured,
product demand may deviate from forecasts and a device may need to be reprovisioned for a different
region, requiring modification of secured settings.  Remanufacturing of devices requires executing
tools and workflows that bypass this security, allowing modification of these sensitive settings,
and performing potentially destructive diagnostics.  The Windows 10 X (a.k.a. WCOS) Compatibility
Requirements (formerly known as Logo) prohibit unauthorized execution of these dangerous tools.
The MFCI feature provides a secure path to enable remanufacturing while maintaining the Windows
security promise.

## Workflows

### Generic MFCI Policy Installation Workflow

1. Read targeting information from a target device (UEFI variables)
1. Determine the flavor of MFCI policy to be applied to the device based upon the type of
remanufacturing to be performed
1. Assemble the targeting information and policy flavor into the unsigned MFCI Policy packet format
1. Digitally sign the packet using the specified digital signing format and trusted signing keys
1. Write the signed MFCI Policy packet to the "next" policy mailbox (a UEFI variable) on the target device
1. Reboot the target device to trigger an installation attempt
1. Prior to OS launch, the firmware attempts to verify the digital signature and targeting information
    1. If verification fails, the policy is deleted from the next policy mailbox, and
    the device proceeds with boot to the OS
    1. If verification succeeds...
        1. Registered firmware handlers are notified
        1. Bits representing 1-shot actions are cleared from the policy, as they should have been
        handled in the prior notifications
        1. The next policy and nonce become "active"
        1. The device reboots so that early boot code can observe the new active policy

### One "Real" Installation Workflow

1. Boot device to FFU mode
1. Use Microsoft-supplied tool _foo_ to read the targeting information from the device
1. Use Microsoft Hardware Dev Center (web portal or REST APIs) to generate the signed policy
    1. Authenticate from an authorized account
    1. Supply the targeting and flavor parameters
    1. Receive the signed MFCI Policy for your device
1. Use the Microsoft-supplied tool _foo_ to set the signed policy to the next device policy mailbox
1. Reboot, and the new policy should take effect
1. OEM performs customer service or re-manufacturing as needed. Note that if the device make, model, serial
number, or related UEFI variables change or are deleted, the policy may no longer be valid on subsequent boots.
1. After re-manufacturing, remove the policy using MS-supplied tool _foo_

### Boot Workflow

1. PEI
    1. This is not the place to perform MFCI policy verification.  Simply get a pre-verified policy
    from prior boot for augmenting PEI security properties.
1. DXE prior to StartOfBds
    1. Drivers that need security policy augmentation should both get the current pre-verified
    policy and register for notification of policy changes
    1. OEM-supplied code populates UEFI variables that communicate the Make, Model, & SN targeting
    information to MFCI policy driver
1. StartOfBds event
    1. The MFCI Policy Driver will...
        1. Check for the presence of a policy.  If present...
            1. Verify its signature, delete the policy on failure
            1. Verify its structure, delete the policy on failure
            1. If the resulting policy is different from the current policy, notify registered handlers,
            make the new policy active, reboot
        1. If a next policy is not present, and a pre-verified policy was used for this boot, roll
        nonces, delete the pre-verified policy, & notify registered handlers

## TODO

* Links to drill-down documentation, more on the APIs, usage, & data format
* Diagrams of the structure

## Integrating MfciPkg

* TODO: OEM/IBV must author code that populates the UEFI variables that this module needs for make/model/sn
* TODO: document DSC & FDF changes
* TODO: certificate PCD and/or FDF changes
* TODO: static versus shared crypto
* TODO: source versus nuget versions
* TODO: builds with production versus test certificates & EKUs

## Data Format

The MFCI policy consists of device targeting information, a policy flavor, & a digital signature.

### Targeting Information

Policies are targeted at specific devices via Make (OEM), Model, Serial Number, Nonce, and 2
optional, OEM-defined fields.  The Nonce is a randomly-generated 64-bit integer, and the
remaining fields are unescaped, WIDE NULL-terminated UTF-16LE strings.

### Policy Flavor Bitfield

A 64-bit bitfield representing both persistent states and 1-time actions to be performed upon
successful authentication and installation of a policy.  The bitfield is split into a 32-bit
Microsoft-defined region, and 32-bit OEM-defined region.  Each of those regions are split again
into a 16-bit region for persistent states and 16-bit region for 1-time actions.

### Digital Signature

The PKCS7 digital signature format familiar to the UEFI ecosystem is used with 2 key differences.
First, a full PKCS7 is used, not the SignedData subset used by authenticated variables.  Second,
the P7 is embedded, not detatched.  The policy targeting and flavor information are embedded in a
PKCS 7 Data inside the full PKCS 7 Signed object.

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
