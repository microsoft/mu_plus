import os
import sys
import argparse

sys.path.append(r'..\Support\Python')
from DFCI_SupportLib import DFCI_SupportLib


def main(dsl):
    parser = argparse.ArgumentParser(description='Create USB Json packet file')

    parser.add_argument("-c",  "--CertPath", dest="CertPath", help="Path to public cert path)", default=None, required=True)
    parser.add_argument("-i",  "--IdentityPacketFilePath", dest="IdentityFilePath", help="Path to signed data", default=None, required=True)

    options = parser.parse_args()

    dsl.verify_identity_packet(options.CertPath, options.IdentityFilePath)

if __name__ == '__main__':
    dsl = DFCI_SupportLib()
    main(dsl)
