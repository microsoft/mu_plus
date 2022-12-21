import sys
import argparse
import base64
sys.path.append(r'..\..\Support\Python')
from CertSupportLib import CertSupportLib

delimiter = ''


def set_delimiter(new_delimiter):
    global delimiter
    delimiter = new_delimiter


def add_section(outfile, section_name, section_file_name):
    if (not section_file_name):
        return

    try:
        with open(section_file_name, "rb") as binfile:
            bindata = binfile.read()

    except FileNotFoundError:
        print("File %s not found" % section_file_name)
        return

    if bindata is None:
        raise Exception(f"Invalid binary data from {section_name}")

    b64data = base64.b64encode(bindata)
    if b64data is None:
        raise Exception(f"Unable to convert bindata to b64data data from {section_name}")

    outfile.write(delimiter)
    outfile.write('"')
    outfile.write(section_name)
    outfile.write('":"')
    outfile.write(b64data.decode("utf-8"))
    outfile.write('"')
    set_delimiter(',\n')


def add_section_hash(outfile, section_name, section_file_name):
    if (not section_file_name):
        return

    support = CertSupportLib()

    thumbprint = support.get_thumbprint_from_pfx(section_file_name)

    outfile.write(delimiter)
    outfile.write('"')
    outfile.write(section_name)
    outfile.write('":"')
    outfile.write(thumbprint)
    outfile.write('"')
    set_delimiter(',\n')


def add_section_text(outfile, section_name, text):

    outfile.write(delimiter)
    outfile.write('"')
    outfile.write(section_name)
    outfile.write('":')
    if text is None:
        outfile.write('null')
    else:
        outfile.write('"')
        outfile.write(text)
        outfile.write('"')

    set_delimiter(',\n')


#
# main script function
#
def main():
    global delimeter
    parser = argparse.ArgumentParser(description='Create USB Json packet file')

    parser.add_argument("-o",  "--OutputFilePath", dest="OutputFilePath", help="Path to output file (requires)", default=None, required=True)

    parser.add_argument("-i",  "--Identity", dest="IdFilePath", help="Path to Identity packet", default=None)
    parser.add_argument("-i2", "--Identity2", dest="Id2FilePath", help="Path to Identity2 packet", default=None)
    parser.add_argument("-p",  "--Permission", dest="PermFilePath", help="Path to Permission packet", default=None)
    parser.add_argument("-p2", "--Permission2", dest="Perm2FilePath", help="Path to Permission2 packet", default=None)
    parser.add_argument("-s",  "--Settings", dest="SettingsFilePath", help="Path to Settings packet", default=None)
    parser.add_argument("-s2", "--Settings2", dest="Settings2FilePath", help="Path to Settings2 packet", default=None)
    parser.add_argument("-t",  "--Transition1", dest="Transition1FilePath", help="Path to Transition1 packet", default=None)
    parser.add_argument("-t2", "--Transition2", dest="Transition2FilePath", help="Path to Transition2 packet", default=None)

    parser.add_argument("-l",  "--HttpsPfxPath", dest="HttpsPfxFilePath", help="Path to HTTPS.pfx", default=None)
    parser.add_argument("-w",  "--OwnerPfxPath", dest="OwnerPfxFilePath", help="Path to Owner.pfx", default=None)

    parser.add_argument("-null", action="store_true", dest="null", help="Build Null Response", default=False)

    options = parser.parse_args()

    if options.null:
        if ((options.IdFilePath is not None) or
            (options.Id2FilePath is not None) or
            (options.PermFilePath is not None) or
            (options.Perm2FilePath is not None) or
            (options.SettingsFilePath is not None) or
            (options.Settings2FilePath is not None) or
            (options.Transition1FilePath is not None) or
            (options.Transition2FilePath is not None) or
            (options.HttpsPfxFilePath is not None) or
            (options.HttpsPfxFilePath is not None)):
            raise Exception("Specifying --null conflicts with all input file options")

    outfile = open(options.OutputFilePath, "w")
    set_delimiter('{')

    if options.null:
        add_section_text(outfile, "TransitionPacket1", None)
        add_section_text(outfile, "TransitionPacket2", None)
        add_section_text(outfile, "SettingsPacket", None)
        add_section_text(outfile, "ResultCode", "0")
        add_section_text(outfile, "ResultMessage", "No Certs Needed")
    else:
        add_section(outfile, 'ProvisioningPacket', options.IdFilePath)
        add_section(outfile, 'ProvisioningPacket2', options.Id2FilePath)
        add_section(outfile, 'PermissionsPacket', options.PermFilePath)
        add_section(outfile, 'PermissionsPacket2', options.Perm2FilePath)
        add_section(outfile, 'TransitionPacket1', options.Transition1FilePath)
        add_section(outfile, 'TransitionPacket2', options.Transition2FilePath)
        add_section(outfile, 'SettingsPacket', options.SettingsFilePath)
        add_section(outfile, 'SettingsPacket2', options.Settings2FilePath)

        if options.Transition1FilePath is not None:
            add_section_text(outfile, "ResultCode", "0")
            add_section_text(outfile, "ResultMessage", "Bootstrap Packets transferred")

        add_section_hash(outfile, "DdsWildcardCertificateThumbprint", options.OwnerPfxFilePath)
        add_section_hash(outfile, "DdsEncryptionCertificateThumbprint", options.HttpsPfxFilePath)

    if (delimiter == '{'):
        raise Exception("No package written")

    outfile.write("}")
    outfile.close()


if __name__ == '__main__':

    main()
