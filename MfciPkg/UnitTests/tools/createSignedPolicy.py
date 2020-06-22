# @file
# Create MFCI Policy for the current device
#
# Copyright (c) Microsoft Corporation
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

from edk2toollib.windows.policy.firmware_policy import FirmwarePolicy
from edk2toolext.capsule import signing_helper
import struct

SIGNER = 'signtool'
# SIGNER = 'openssl'
PFX = 'Leaf-test.pfx'
OID = '1.2.840.113549.1.7.1'


def CreatePolicyFromParameters(manufacturer: str, product: str,
                               sn: str, nonce: int, oem1: str, oem2: str,
                               devicePolicy: int) -> bytes:
    """
    Populates a MFCI Policy object with provided parameters & returns its serialization as a bytearray
    """
    policy = FirmwarePolicy()
    TargetInfo = {'Manufacturer': manufacturer,
                  'Product': product,
                  'SerialNumber': sn,
                  'OEM_01': oem1,
                  'OEM_02': oem2,
                  'Nonce': nonce}
    policy.SetDeviceTarget(TargetInfo)
    policy.SetDevicePolicy(devicePolicy)
    ba = bytearray()
    policy.Serialize(ba)
    return bytes(ba)


def createSignedPolicy(filename: str, signer_type: str, pkcs12_test: bool = False) -> None:
    manf = bytes.decode(open('Manufacturer.bin', 'rb').read(), encoding='utf_16_le')
    prod = bytes.decode(open('Product.bin', 'rb').read(), encoding='utf_16_le')
    sn = bytes.decode(open('SerialNumber.bin', 'rb').read(), encoding='utf_16_le')
    oem1 = bytes.decode(open('OEM_01.bin', 'rb').read(), encoding='utf_16_le')
    oem2 = bytes.decode(open('OEM_02.bin', 'rb').read(), encoding='utf_16_le')
    nonceInt = struct.unpack('<Q', open('TargetNonce.bin', 'rb').read())[0]
    devicePolicy = 3
    policy_ba = CreatePolicyFromParameters(manf, prod, sn, nonceInt, oem1, oem2, devicePolicy)

    signer_options = {
        'key_file_format': 'pkcs12',
        'key_file': PFX,
        'oid': OID}

    if signer_type == 'signtool':
        signer = signing_helper.get_signer(signing_helper.SIGNTOOL_SIGNER)
        signature_options = {
            'type': 'pkcs7',
            'type_options': {'embedded'},
            'encoding': 'DER',
            'hash_alg': 'sha256'
        }
    elif signer_type == 'openssl':
        signer = signing_helper.get_signer(signing_helper.PYOPENSSL_SIGNER)
        signature_options = {
            'type': 'bare',
            'encoding': 'binary',
            'hash_alg': 'sha256'
        }
    else:
        raise Exception

    if pkcs12_test is True:
        signature_options['sign_alg'] = 'pkcs12'

    print('\nSigning ' + filename)
    signed = signer.sign(policy_ba, signature_options, signer_options)
    print('Signing complete.\n')

    print(signed)
    open(filename + '.bin', 'wb').write(policy_ba)
    open(filename + '.bin.p7', 'wb').write(signed)


def main():
    createSignedPolicy('policySigntoolEmbedded', 'signtool', False)
    createSignedPolicy('policySigntoolCompatDetached', 'signtool', True)
    createSignedPolicy('policyOpenssl', 'openssl', False)
    createSignedPolicy('policyOpensslCompat', 'openssl', True)


if __name__ == '__main__':
    main()
