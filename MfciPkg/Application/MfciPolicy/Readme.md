# MfciPkg - MfciPolicy

MfciPolicy.py is a simple testing script for interacting with Mfci on windows.

## About

MfciPolicy can be used to interact with the Mfci mailboxes.

After a policy is generated, the policy can be written into the NextMfciPolicyBlob mailbox
through this utility. Additionally, this utility will display the information necessary
to generate a policy.

## Usage

Copy the two files, DecodeUefiLog.py and UefiVaiableSupport folder to the Mfci enabled
system.

The simplest case:

```.sh
  py -3 MfciPolicy.py -h
usage: MfciPolicy.py [-h] [-a POLICYBLOB] [-del] [-r] [-rn] [-d]

options:
  -h, --help            show this help message and exit
  -a POLICYBLOB, --apply POLICYBLOB
                        Apply a signed Mfci Policy Blob to be processed on next reboot
  -del, --delete        Delete the current Mfci Policy and the next Mfci Policy
  -r, --retrieve        Retrieve Current Mfci Policy
  -rn, --retrievenext   Retrieve Next Mfci Policy
  -d, --dump            Get System Info required for policy generation
```

Retrieve the information necessary to generate a Mfci Policy:

```.sh
  py -3 MfciPolicy.py -d

  CurrentMfciPolicyNonce = 0x0
  NextMfciPolicyNonce = 0xD5CFCBC5C1BBB6A7
  Target\Manufacturer = "Palindrome"
  Target\Product = "QEMU Q35"
  Target\SerialNumber = "42-42-42-42"
  Target\OEM_01 = ""
  Target\OEM_02 = ""
```

Apply a new Mfci Policy to the sytem, to be processed on reboot.

```.sh
  py -3 MfciPolicy.py -a PolicyBlob.bin.p7

  NextMfciPolicyBlob was set
```

Delete the current and next Mfci Policy (restore policy back to original state)

```.sh
  py -3 MfciPolicy.py -del

  CurrentMfciPolicyBlob was deleted
  Failed to delete NextMfciPolicyBlob
```

---

## Copyright

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
