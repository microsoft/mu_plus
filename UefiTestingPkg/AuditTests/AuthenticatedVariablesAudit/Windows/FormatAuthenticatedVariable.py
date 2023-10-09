#!python

"""
Signs a variable in accordance with EFI_AUTHENTICATION_2

Relevant RFC's
    * (PKCS #7: Cryptographic Message Syntax)[https://www.rfc-editor.org/rfc/rfc2315]
    * (Internet X.509 Public Key Infrastructure Certificate and Certificate Revocation List (CRL) Profile (In particular To-be-signed Certificate))[https://www.rfc-editor.org/rfc/rfc5280#section-4.1.2]
    * https://www.itu.int/ITU-T/formal-language/itu-t/x/x420/1999/PKCS7.html

# TODO:
    * Implement Certificate Verification (https://stackoverflow.com/questions/70654598/python-pkcs7-x509-chain-of-trust-with-cryptography)

pip requirements:
    pyasn1
    pyasn1_modules
    edk2toollib
    cryptography # Depends on having openssl installed
"""

import struct
import time
import tempfile
import argparse
import logging
import sys
import uuid
import os
import shutil
import hexdump

import edk2toollib.uefi.uefi_multi_phase as UEFI_MULTI_PHASE

from cryptography import x509
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.serialization import pkcs7
from cryptography.hazmat.primitives.serialization import pkcs12
from pyasn1.codec.der.decoder import decode as der_decode
from pyasn1.codec.der.encoder import encode as der_encode
from pyasn1_modules import rfc2315


# Puts the script into debug mode, may be enabled via argparse
ENABLE_DEBUG = False

# Index into the certificate argument
CERTIFICATE_FILE_PATH = 0
CERTIFICATE_PASSWORD = 1

WIN_CERT_TYPE_EFI_GUID = 0x0ef1
WIN_CERT_REVISION_2_0 = 0x0200
EFI_CERT_TYPE_PKCS7_GUID = '4aafd29d-68df-49ee-8aa9-347d375665a7'

OUTPUT_DIR = ""

ATTRIBUTE_MAP = {
    "NV": UEFI_MULTI_PHASE.EFI_VARIABLE_NON_VOLATILE,
    "BS": UEFI_MULTI_PHASE.EFI_VARIABLE_BOOTSERVICE_ACCESS,
    "RT": UEFI_MULTI_PHASE.EFI_VARIABLE_RUNTIME_ACCESS,
    # Disabling the following two, because they are unsupported (by this script) and deprecated (in UEFI)
    # "HW": UEFI_MULTI_PHASE.EFI_VARIABLE_HARDWARE_ERROR_RECORD,
    # "AW": UEFI_MULTI_PHASE.EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS,
    "AT": UEFI_MULTI_PHASE.EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS,
}

TEMP_FOLDER = tempfile.gettempdir()

WINCERT_FMT = '<L2H16s'
WINCERT_FMT_SIZE = struct.calcsize(WINCERT_FMT)

EFI_TIME_FMT = '<H6BLh2B'
EFI_TIME_FMT_SIZE = struct.calcsize(EFI_TIME_FMT)

logging.basicConfig()
logger = logging.getLogger()
logger.setLevel(logging.INFO)


def export_c_array(bin_file: str, output_dir: str, name: str, c_name: str) -> None:
    """
    Converts a given binary file to a UEFI typed C array

    :param args: argparse arguments passes to the script
    :bin file: binary file to convert
    """

    source_file = bin_file + ".c"
    header_file = bin_file + ".h"
    if output_dir:
        filename = os.path.split(source_file)[-1]
        source_file = os.path.join(output_dir, filename)

    variable_name = name
    if c_name:
        variable_name = c_name

    length = 0

    with open(bin_file, 'rb') as f:

        start = f.tell()
        f.seek(0, 2)
        end = f.tell()
        f.seek(start)
        length = end - start
        c_array = f"UINT8 {variable_name}[{length}] = {{"

        ascii_string = ""
        i = 0
        bytes_per_row = 16
        byte = b""

        for i, byte in enumerate(f.read()):
            if i % bytes_per_row == 0:
                if i != 0:
                    c_array += f"  // {ascii_string} "
                    ascii_string = ""
                c_array += "\n    "

            if byte < 0x20 or byte > 0x7E:
                ascii_string += "."
            else:
                ascii_string += f"{chr(byte)}"

            c_array += f"{byte:#04x}"

            if i != length-1:
                c_array += ", "
 
        # bytes_per_row - 1 because indexes at 0
        # subtract the number of bytes we printed
        # now we know how many bytes we could have printed
        potential_bytes = (bytes_per_row - 1) - (i % bytes_per_row)

        # pad out the number of bytes by our byte length
        # use whatever was left over in byte
        byte_length = len(f" {byte:#04x},")

        # pad out the the line
        c_array += " " * (potential_bytes * byte_length)

        # make up for the trailing ',' and print a comment
        c_array += f"    // {ascii_string}"

    c_array += "\n};"
    # TODO determine what to do with this
    # c_array += f"\n\nUINTN {variable_name}Size = sizeof {variable_name};\n\n"

    with open(source_file, 'w') as f:
        f.write(c_array)

    logger.info("Created %s", source_file)

    with open(header_file, 'w') as f:
        f.write(f"extern UINT8 {variable_name}[{length}];\n")

    logger.info("Created %s", header_file)


def pkcs7_get_signed_data_structure(signature: rfc2315.ContentInfo) -> rfc2315.SignedData:

    # lets decode the der encoded structure as an asn1Spec ContentInfo
    content_info, _ = der_decode(signature, asn1Spec=rfc2315.ContentInfo())

    # Check that this is a signed data structure (doesn't really matter since this should be a rfc2315.signedData structure)
    content_type = content_info.getComponentByName('contentType')
    if content_type != rfc2315.signedData:
        raise ValueError("This wasn't a signed data structure?")

    # Remove the contentInfo section and just return the sign data
    signed_data, _ = der_decode(content_info.getComponentByName(
        'content'), asn1Spec=rfc2315.SignedData())

    return der_encode(signed_data)


def pkcs7_sign(buffer, signers: list, top_level_certificate_file=None, additional_cerificates=[], hash_algorithm="SHA256") -> rfc2315.SignedData:
    """
    :return: this returns a der encoded pkcs7 signed data structure. This is not a pkcs7 message.
        This is the sign data structure encompassed by the outer content info structure of the pkcs7 message
    """
    top_level_certifcate_found = False

    # Set the options for the pkcs7 signature:
    #   - The signature is detached
    #   - Do not convert LF to CRLF in the file (windows format)
    #   - Remove the attributes section from the pkcs7 structure
    options = [pkcs7.PKCS7Options.DetachedSignature,
               pkcs7.PKCS7Options.Binary, pkcs7.PKCS7Options.NoAttributes]

    # Start setting up the builder
    signature_builder = pkcs7.PKCS7SignatureBuilder()

    # Set up the databuffer that will be signed
    signature_builder = signature_builder.set_data(buffer)

    # TODO find a cleaner way to handle this, if the argument is passed in as a quote string we will have nested signers
    if len(signers) == 1:
        signers = signers[0]

    # In UEFI you may technically have multiple signers.. this seems problematic if the 'top level certificate' is the trust anchor
    # So in order to pass `PKCS7_signatureVerify` all the signature's need to pass - no cheating allowed (strictly private key needs to match public key)
    for i, signer in enumerate(signers):

        signers_certificate = signer[CERTIFICATE_FILE_PATH]
        signers_password = signer[CERTIFICATE_PASSWORD]
        pkcs12_signer_blob = b""

        # read from the pfx file the pkcs12_blob
        with open(signers_certificate, 'rb') as f:
            pkcs12_signer_blob = f.read()

        # Grab the certificate key, certificate, and the additional certificates (public keys)
        pkcs12_store = pkcs12.load_pkcs12(
            pkcs12_signer_blob, signers_password.encode('utf-8'))

        tab_indent = i + 1
        logger.info("")
        logger.info("%sSigning with Certificate: ", '\t'*tab_indent)
        logger.info("%s\tIssuer: %s", '\t'*tab_indent,
                    pkcs12_store.cert.certificate.issuer)
        logger.info("%s\tSubject: %s", '\t'*tab_indent,
                    pkcs12_store.cert.certificate.subject)
        # add the signer certificate
        
        hash_algorithm = hash_algorithm.upper()
        hash_algo = None
        if hash_algorithm == "SHA256":
            hash_algo = hashes.SHA256()
        elif hash_algorithm == "SHA384":
            hash_algo = hashes.SHA384()
        elif hash_algorithm == "SHA512":
            hash_algo = hashes.SHA512()

        signature_builder = signature_builder.add_signer(
            pkcs12_store.cert.certificate, pkcs12_store.key, hash_algo)

        # Additional certificates are used during chain validation `pkcs7_verify`
        if top_level_certificate_file and pkcs12_store.additional_certs:

            # load the certificate
            top_level_certificate = None
            with open(top_level_certificate_file, 'rb') as f:
                top_level_certificate = x509.load_der_x509_certificate(
                    f.read())

            # Start adding the certificates - the list is in reverse order
            for i, cert in enumerate(reversed(pkcs12_store.additional_certs)):
                tab_indent = i + 1
                logger.info("")
                logger.info("%sAdding Additional Certificate: ",
                            '\t'*tab_indent)
                logger.info("%s\tIssuer: %s", '\t'*tab_indent,
                            cert.certificate.issuer)
                logger.info("%s\tSubject: %s", '\t'*tab_indent,
                            cert.certificate.subject)

                # Add the certificate for verifcation
                signature_builder = signature_builder.add_certificate(
                    cert.certificate)

                # If we have found the certifiate
                if top_level_certificate.serial_number == cert.certificate.serial_number:
                    logger.info("Found the top level certificate")
                    top_level_certifcate_found = True
                    break

    if top_level_certificate_file and not top_level_certifcate_found:
        logger.error("Top level certificate file provided but never found in the PFX file (%s)",
                     top_level_certificate_file)
        return None

    # Add any additional certificates provided
    for i, cert in enumerate(additional_cerificates):
        # load the certificate
        x509_certificate = None
        with open(cert, 'rb') as f:
            x509_certificate = x509.load_der_x509_certificate(f.read())

        tab_indent = i + 1
        logger.info("")
        logger.info("%sAdding Additional Individiual Certificate: ",
                    '\t'*tab_indent)
        logger.info("%s\tIssuer: %s", '\t'*tab_indent,
                    x509_certificate.issuer)
        logger.info("%s\tSubject: %s", '\t'*tab_indent,
                    x509_certificate.subject)

        # Add the certificate for verifcation
        signature_builder = signature_builder.add_certificate(
            x509_certificate)

    # The signature is enclosed in a asn1 content info structure
    signature = signature_builder.sign(serialization.Encoding.DER, options)

    # So this was fixed in EDK2 as of 
    # return only the SignedData structure from the asn.1 ContentInfo Pkcs#7 message
    return pkcs7_get_signed_data_structure(signature)


class AuthenticatedVariable2(object):

    def __init__(self, decodefs=None):

        # The signature buffer is what is is used for "signing the payload"
        self.signature_buffer = None
        self.variable_data = b""
        self.efi_time = b""
        self.signature = b""
        self.wincert = b""
        self.serialized_buffer = b""

        # the file stream to decode
        if decodefs:
            self._unpack(decodefs)

    def new(self, name, guid, attributes, tm=time.localtime(), variablefs=None):
        """
        :param variablefs: must be opened as 'rb'
        """

        self.efi_time = struct.pack(
            EFI_TIME_FMT,
            tm.tm_year,
            tm.tm_mon,
            tm.tm_mday,
            tm.tm_hour,
            tm.tm_min,
            tm.tm_sec,
            0,
            0,
            0,
            0,
            0)

        # Generate the hash data to be digested
        self.signature_buffer = name.encode('utf_16_le') + guid.bytes_le + \
            struct.pack('<I', attributes) + self.efi_time

        # Filestream may be empty (delete variable)
        if variablefs:
            self.variable_data = variablefs.read()

        # Append the variable data to the buffer
        self.signature_buffer += self.variable_data

    def sign_payload(self, signers, top_level_certificate, additional_cerificates, hash_algorithm="SHA256"):
        """
        :param certificate: signing certificate
        :param password: password for the signing certificate
        """

        if not self.signature_buffer:
            logger.warning("Signature Buffer was empty")
            return b""

        self.signature = pkcs7_sign(
            self.signature_buffer, signers, top_level_certificate, additional_cerificates, hash_algorithm)

        return self.signature

    def serialize(self):
        """
        returns byte array of serialized buffer
        """

        if not self.signature:
            logger.error("Can't serialize without a signature")
            return b""

        # Set the wincert and authinfo
        self.wincert = struct.pack(WINCERT_FMT,
                                   WINCERT_FMT_SIZE + len(self.signature),
                                   WIN_CERT_REVISION_2_0,
                                   WIN_CERT_TYPE_EFI_GUID,
                                   uuid.UUID(EFI_CERT_TYPE_PKCS7_GUID).bytes_le)

        self.serialized_buffer = self.efi_time + \
            self.wincert + self.signature + self.variable_data

        return self.serialized_buffer

    def _unpack(self, decodefs):

        # Get the length of the file
        decodefs.seek(0)
        start = decodefs.tell()
        decodefs.seek(0, 2)
        bytes_left_unread = decodefs.tell() - start

        # reset the pointer
        decodefs.seek(0)

        if (EFI_TIME_FMT_SIZE + WINCERT_FMT_SIZE) > bytes_left_unread:
            logger.error("Not enough bytes for EFI_TIME and WINCERT")
            return -1

        self.efi_time = decodefs.read(EFI_TIME_FMT_SIZE)
        bytes_left_unread -= EFI_TIME_FMT_SIZE
        self.wincert = decodefs.read(WINCERT_FMT_SIZE)
        bytes_left_unread -= WINCERT_FMT_SIZE

        (
            wincert_size,
            _,
            wincert_guid,
            certificate_guid
        ) = struct.unpack(WINCERT_FMT, self.wincert)

        if wincert_guid != WIN_CERT_TYPE_EFI_GUID or certificate_guid != uuid.UUID(EFI_CERT_TYPE_PKCS7_GUID).bytes_le:
            logger.error("WINCERT appears to be invalid:")
            return -1

        # wincert was valid
        signature_size = wincert_size - WINCERT_FMT_SIZE
        self.signature = decodefs.read(signature_size)
        bytes_left_unread -= signature_size

        self.variable_data = decodefs.read(bytes_left_unread)

        return 0

    def describe_efi_time(self, outputfs=sys.stdout):
        """
        :param outputfs: output stream to write to

        :return: None
        """

        logger.info("Describing EFI Time")

        efi_time = struct.unpack(EFI_TIME_FMT, self.efi_time)
        timestamp = time.strftime(
            "%Y-%m-%d %H:%M:%S", time.struct_time(efi_time))

        outputfs.write("EFI Time:\n")
        outputfs.write(f"\t{timestamp}\n")

    def describe_wincert(self, outputfs=sys.stdout):
        """
        :param outputfs: output stream to write to

        :return: None
        """

        logger.info("Describing WIN_CERTIFICATE")

        (
            wincert_size,
            wincert_revision,
            wincert_guid,
            certificate_guid
        ) = struct.unpack(WINCERT_FMT, self.wincert)

        guid = uuid.UUID(int=int.from_bytes(certificate_guid, 'little'))
        outputfs.write("WIN_CERTIFICATE:\n")
        outputfs.write(f"\tdwLength: 0x{wincert_size:0x} ({wincert_size})\n")
        outputfs.write(
            f"\twRevision: 0x{wincert_revision:0x} ({wincert_revision})\n")
        outputfs.write(
            f"\twCertificateType: 0x{wincert_guid:0x} ({wincert_guid})\n")
        outputfs.write(f"\tbCertificate: {guid}\n")

    def describe_signature(self, outputfs=sys.stdout):
        """
        :param outputfs: output stream to write to

        :return: None
        """

        logger.info("Describing Signature")

        if not self.signature:
            raise Exception("self.signature was empty")

        signed_data, _ = der_decode(
            self.signature, asn1Spec=rfc2315.SignedData())

        outputfs.write(str(signed_data))

    def describe_variable(self, outputfs=sys.stdout, split_content=False):
        """
        :param outputfs: output stream to write to

        :return: None
        """
        logger.info("Describing Variable Data")

        outputfs.write("Variable Data:\n")
        variable_hexdump = hexdump.hexdump(self.variable_data, result='return')

        for line in variable_hexdump.split('\n'):
            outputfs.write(f"\t{line}\n")

        if split_content:
            logger.info("splitting content data")
            with open('content.bin', 'wb') as f:
                f.write(self.variable_data)

    def describe_all(self, outputfs=sys.stdout, bar_length=120, split_content=False):
        """
        Describes all fields in the payload

        :param outputfs: output stream to write to
        :param bar_length: length of the bar in between sections

        :return: None
        """

        self.describe_efi_time(outputfs=outputfs)
        outputfs.write(f"\n{'='*bar_length}\n")
        self.describe_wincert(outputfs=outputfs)
        outputfs.write(f"\n{'='*bar_length}\n")
        self.describe_signature(outputfs=outputfs)
        outputfs.write(f"\n{'='*bar_length}\n")
        self.describe_variable(outputfs=outputfs, split_content=split_content)

    def get_signature(self):
        """
        returns the byte stream signature generated by `sign_payload`
        """
        if not self.signature:
            raise Exception("self.signature was empty")

        return self.signature


def create_authenticated_variable_v2(tm, name, guid, attributes, data_file, signers, top_level_certificate=None,
                                     additional_cerificates=[], output_dir='./', hash_algorithm="SHA256"):
    """
    :param tm: Time object representing the time at which this variable was created
    :param name: UEFI variable name
    :param guid: The GUID namespace that the variable belongs to
    :param attributes: The attributes the variable 
    :param data_file: The filename containing the binary data to be serialized, hashed and converted into an
         authenticated variable (May be an empty file)
    :param certificate: the certificate to sign the binary data with (May be PKCS7 or PFX)
    :param cert_password: the password for the certificate
    :param output_dir: directory to drop the signed authenticated variable data
    """

    # create a authenticated variable object
    auth_var = AuthenticatedVariable2()

    with open(data_file, 'rb') as f:
        # create a new authenticated variable with the data
        auth_var.new(name, guid, attributes, tm=tm, variablefs=f)

    # Sign the payload with the signers list
    signature = auth_var.sign_payload(
        signers, top_level_certificate, additional_cerificates, hash_algorithm)
    if not signature:
        logger.error("Signature Failed")
        return

    if ENABLE_DEBUG:
        # write the signature to a file for manual inspection

        output_file = data_file + ".signature"
        if output_dir:
            filename = os.path.split(output_file)[-1]
            output_file = os.path.join(output_dir, filename)

        with open(output_file, 'wb') as f:
            f.write(signature)

    # Serialize the signed data into a byte array
    serialized_variable = auth_var.serialize()
    if not serialized_variable:
        logger.error("Invalid serialized variable")
        return None

    # output the signed payload with a ".signed"
    output_file = data_file + ".signed"
    if output_dir:
        filename = os.path.split(output_file)[-1]
        output_file = os.path.join(output_dir, filename)

    with open(output_file, 'wb') as f:
        f.write(serialized_variable)

    logger.info("Created %s", output_file)

    return output_file


def sign_variable(args):
    # Generate a  timestamp
    tm = time.localtime()

    # create the ourput directory
    output_dir = args.output_dir
    if args.c_name:
        output_dir = os.path.join(output_dir, args.c_name)

    os.makedirs(output_dir, exist_ok=True)

    output_file = create_authenticated_variable_v2(
        tm, args.name, args.guid, args.attributes, args.data_file, args.signers_info,
        args.top_level_certificate, args.additional_certificates, output_dir, args.hash_algorithm)

    if not output_file:
        logger.error("Failed to create output file")
        return 1

    if args.export_c_array:
        export_c_array(output_file, output_dir, args.name, args.c_name)

    dest_data_file = os.path.split(args.data_file)[-1]
    dest_data_file = os.path.join(output_dir, dest_data_file)
    # Copy the original file
    shutil.copy(args.data_file, dest_data_file)

    # Success
    return 0


def describe_variable(args):
    """
    Describes a private authenticated variable (2) payload and all of the fields and data contained within


    """
    auth_var = None
    with open(args.signed_payload, 'rb') as f:
        auth_var = AuthenticatedVariable2(decodefs=f)

    with open(args.output, 'w') as f:
        auth_var.describe_all(f, split_content=args.split_content)

    logger.info(f"Output: {args.output}")

    return 0


def typecheck_file_exists(filepath):
    """
    Checks if this is a valid filepath for argparse

    :param filepath: filepath to check for existance

    :return: valid filepath
    """
    if not os.path.isfile(filepath):
        raise argparse.ArgumentTypeError(
            f"You sure this is a valid filepath? : {filepath}")

    return filepath


def typecheck_signer_info(signer):
    """
    Converts <certificate-path>;<password> to (certificate-path, password)

    if `sep` is missing, converts the password to ""

    TODO check if certificate-path is empty

    :param: signer - path and password of certificate pfx file seperated by a seperator

    :return: tuple(<certificate>, <password>)
    """
    sep = ';'
    signer_info = []

    signers = signer.split(' ')
    for signer in signers:
        try:
            certificate_password_set = signer.split(sep,  1)

            # if the password is missing, just set the password to None
            if len(certificate_password_set) == 1:
                certificate_password_set.append("")

            if not certificate_password_set[CERTIFICATE_FILE_PATH].lower().endswith('.pfx'):
                raise argparse.ArgumentTypeError(
                    "signing certificate must be pkcs12 .pfx file")

            typecheck_file_exists(certificate_password_set[CERTIFICATE_FILE_PATH])

            signer_info.append(tuple(certificate_password_set))
        except Exception as exc:
            raise ValueError(
                "signers_info must be passed as <certificate-path>{sep}<password>") from exc

    return signer_info


def typecheck_attributes(attributes):

    if ',' not in attributes:
        raise argparse.ArgumentTypeError(
            "Must provide at least one of \"NV\", \"BS\" or \"RT\"")

    if 'AT' not in attributes:
        raise argparse.ArgumentTypeError(
            "The time based authenticated variable attribute (\"AT\") must be set")

    # verify the attributes and calculate
    attributes_value = 0
    for a in attributes.split(','):
        if a not in ATTRIBUTE_MAP:
            raise argparse.ArgumentTypeError(f"{a} is not a valid attribute")

        attributes_value |= ATTRIBUTE_MAP[a.upper()]

    return attributes_value


def setup_sign_parser(subparsers):
    """
    Sets up the sign parser

    :param subparsers: - sub parser from argparse to add options to

    :returns: subparser
    """

    sign_parser = subparsers.add_parser(
        "sign", help="Signs variables using the command line"
    )
    sign_parser.set_defaults(function=sign_variable)

    sign_parser.add_argument(
        "name",
        help="UTF16 Formated Name of Variable"
    )

    sign_parser.add_argument(
        "guid", type=uuid.UUID,
        help="UUID of the namespace the variable belongs to. (Ex. 12345678-1234-1234-1234-123456789abc)"
    )

    sign_parser.add_argument(
        "attributes", type=typecheck_attributes,
        help="Variable Attributes, AT is a required (Ex \"NV,BT,RT,AT\")"
    )

    sign_parser.add_argument(
        "data_file", type=typecheck_file_exists,
        help="Binary file of variable data. An empty file is accepted and will be used to clear the authenticated data"
    )

    sign_parser.add_argument(
        "signers_info", nargs='+', type=typecheck_signer_info,
        help="Pkcs12 certificate and password to sign the authenticated data with (Cert.pfx;password)"
    )

    sign_parser.add_argument(
        "--top-level-certificate", default=None, type=typecheck_file_exists,
        help="If included, this is the top level certificate in the pfx (pkcs12) that the signer should chain up to"
    )

    sign_parser.add_argument(
        "--export-c-array", default=False, action="store_true",
        help="Exports a given buffer as a C array"
    )

    sign_parser.add_argument(
        "--c-name", default=None,
        help="Override C variable name on export"
    )

    sign_parser.add_argument(
        "--output-dir", default="./",
        help="Output directory for the signed data"
    )

    sign_parser.add_argument(
        "--additional-certificates", nargs="+", default=[],
        help="Advance operation to add any number of certificates to the certificate section of a signature" +
        " that may have nothing to do with the signature. In most cases --top-level-certificate should be used instead."
    )

    sign_parser.add_argument("--hash-algorithm", default="SHA256", choices=[
                             "SHA256", "SHA384", "SHA512"], help="Hash algorithm to use when signing")

    return subparsers


def setup_describe_parser(subparsers):

    describe_parser = subparsers.add_parser(
        "describe", help="Parses Authenticated Variable 2 structures"
    )
    describe_parser.set_defaults(function=describe_variable)

    describe_parser.add_argument(
        "signed_payload", type=typecheck_file_exists,
        help="Signed payload to parse"
    )

    describe_parser.add_argument(
        "--output", default="./variable.describe",
        help="Output file to write the parse data to"
    )

    describe_parser.add_argument(
        "--split-content", default=False, action="store_true",
        help="splits the content out into 'content.bin'"
    )

    return subparsers


def parse_args():
    """
    Parses arguments from the command line
    """
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers()

    parser.add_argument(
        "--debug", action='store_true', default=False,
        help="enables debug printing for deep inspection"
    )

    subparsers = setup_sign_parser(subparsers)
    subparsers = setup_describe_parser(subparsers)

    args = parser.parse_args()

    if not hasattr(args, "function"):
        parser.print_help(sys.stderr)
        sys.exit(1)

    global ENABLE_DEBUG
    ENABLE_DEBUG = args.debug

    if ENABLE_DEBUG:
        logger.setLevel(logging.DEBUG)

    return args


def main():
    args = parse_args()

    status_code = args.function(args)

    return sys.exit(status_code)


main()
