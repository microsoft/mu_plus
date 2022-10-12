# @file
#
# DFCI_SupportLib- DFCI basic functions
#
# Copyright (c), Microsoft Corporation
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

#
# DFCI_SupportLib
#
import os, sys
import xml.etree.ElementTree as ET
import binascii
import traceback
import xml.dom.minidom
import subprocess

try:
    from StringIO import StringIO
except ImportError:
    from io import StringIO, BytesIO

from builtins import int

from edk2toollib.uefi.status_codes import UefiStatusCode
from edk2toollib.windows.locate_tools import FindToolInWinSdk

from Data.CertProvisioningVariable import CertProvisioningApplyVariable
from Data.CertProvisioningVariable import CertProvisioningResultVariable
from Data.PermissionPacketVariable import PermissionApplyVariable
from Data.PermissionPacketVariable import PermissionResultVariable
from Data.SecureSettingVariable    import SecureSettingsApplyVariable
from Data.SecureSettingVariable    import SecureSettingsResultVariable

SignToolPath = None
CertMgrPath = None

class DFCI_SupportLib(object):

    def _ReturnSessionIdValue(self, InputString):
        #Session Id:       0xF08A4
        return int(InputString.partition(":")[2].strip(), base=0)


    def compare_session_id_match(self, resultfile, applyfile):
        resultsid = 0
        applysid = 1

        r = open(resultfile, "r")
        for l in r.readlines():
            if("SessionId:" in l):
                resultsid = self._ReturnSessionIdValue(l)
                break
        r.close()

        a = open(applyfile, "r")
        for l in a.readlines():
            if("SessionId:" in l):
                applysid = self._ReturnSessionIdValue(l)
                break
        a.close()

        print ("Apply Session Id: 0x%x" % applysid)
        print ("Result Session Id: 0x%x" % resultsid)
        return resultsid == applysid


    def check_status(self, resultfile, code):
        t = -1
        a = open(resultfile, "r")
        for l in a.readlines():
            if("Status:" in l):
                b = l.partition(":")[2]
                b = b.partition("(")[2]
                b = b.strip()
                b = b.rstrip(')')
                t = int(b, base=0)
                break
        a.close()
        print ("Result Status: %s" % l)
        return t == int(code, base=0)


    def check_setting_status(self, resultfile, id, statuscode):
        t = -1
        xmlstring = ""
        found = False
        a = open(resultfile, "r")

        #find the start of the xml string and then copy all lines to xmlstring variable
        for l in a.readlines():
            if(found):
                xmlstring += l
            else:
                if l.lstrip().startswith("<?xml"):
                    xmlstring = l
                    found = True
        a.close()

        if (len(xmlstring) == 0) or (not found):
            print ("Result XML not found")
            return False

        #make an element tree from xml string
        r = None
        root = ET.fromstring(xmlstring)
        for e in root.findall("./Settings/SettingResult"):
            i = e.find("Id")
            if(i.text == str(id)):
                r = e.find("Result")
                break
        if(r is None):
            print ("Failed to find ID (%s) in the Xml results" % str(id))
            print (xmlstring)
            return False

        print ("Result Status for Id (%s): %s" % (str(id), r.text))
        return int(r.text.strip(), base=0) == int(statuscode, base=0)

    def check_current_setting_value(self, resultfile, id, valuestring):
        xmlstring = ""
        found = False
        a = open(resultfile, "r")

        #find the start of the xml string and then copy all lines to xmlstring variable
        for l in a.readlines():
            if(found):
                xmlstring += l
            else:
                if l.lstrip().startswith("<?xml"):
                    xmlstring = l
                    found = True
        a.close()

        if (len(xmlstring) == 0) or (not found):
            print ("Result XML not found")
            return False

        #make an element tree from xml string
        r = None
        root = ET.fromstring(xmlstring)
        for e in root.findall("./Settings/SettingCurrent"):
            i = e.find("Id")
            if(i.text == str(id)):
                r = e.find("Value")
                break
        if(r is None):
            print ("Failed to find ID (%s) in the Xml results" % str(id))
            print (xmlstring)
            return False

        print ("Result Value for Id (%s): %s" % (str(id), r.text))

        if (r.text is None):
            if valuestring == '':
                return True
            else:
                return False
        return r.text.strip() == valuestring.strip()

    def get_current_permission_value(self, resultfile, id):
        xmlstring = ""
        found = False
        a = open(resultfile, "r")

        #find the start of the xml string and then copy all lines to xmlstring variable
        for l in a.readlines():
            if(found):
                xmlstring += l
            else:
                if l.lstrip().startswith("<?xml"):
                    xmlstring = l
                    found = True
        a.close()

        if (len(xmlstring) == 0) or (not found):
            print ("Result XML not found")
            return False

        #make an element tree from xml string
        r1 = None
        r2 = None
        root = ET.fromstring(xmlstring)

        for e in root.findall("./Permissions/PermissionCurrent"):
            i = e.find("Id")
            if(i.text == str(id)):
                j = e.find("PMask")
                if (j != None):
                    r1 = j.text
                j = e.find("DMask")
                if (j != None):
                    r2 = j.text
                break
        print ("Result Value for Id (%s): PMask=%s, DMask=%s" % (str(id), r1, r2))
        return r1, r2

    def get_current_permission_defaults(self, resultfile):
        xmlstring = ""
        found = False
        a = open(resultfile, "r")

        #find the start of the xml string and then copy all lines to xmlstring variable
        for l in a.readlines():
            if(found):
                xmlstring += l
            else:
                if l.lstrip().startswith("<?xml"):
                    xmlstring = l
                    found = True
        a.close()

        if (len(xmlstring) == 0) or (not found):
            print ("Result XML not found")
            return False

        #make an element tree from xml string
        root = ET.fromstring(xmlstring)

        #Collect the root attributes
        try:
            r1 = root.attrib["Default"]
        except:
            r1 = None

        try:
            r2 = root.attrib["Delegated"]
        except:
            r2 = None

        print ("Result Default Values for : Default=%s, Delegated=%s" % (r1, r2))
        return r1, r2

    #
    # Check all individual status codes for each setting and confirm it matches the input status code
    #
    def check_all_permission_status(self, resultfile, status):
        t = -1
        found = False
        xmlstring = ""
        statuscode = int(status, base=0)
        a = open(resultfile, "r")

        #find the start of the xml string and then copy all lines to xmlstring variable
        for l in a.readlines():
            if(found):
                xmlstring += l
            else:
                if l.lstrip().startswith("<?xml"):
                    xmlstring = l
                    found = True
        a.close()

        if (len(xmlstring) == 0) or (not found):
            print ("Result XML not found")
            return False

        #make an element tree from xml string
        r = None
        root = ET.fromstring(xmlstring)
        rc = True
        for e in root.findall("./Permissions/PermissionResult"):
            i = e.find("Id")
            r = e.find("Result")

            if(r is None):
                print("Failed to find a result node for id (%s)" % i.text.strip())
                return False

            result = int(r.text.strip(), base=0)
            print ("Result Status for Id (%s): %s" % (str(i.text.strip()), r.text))
            if(result != statuscode):
                print("Error.  Status Code for id (%s) didn't match expected" % i.text.strip())
                rc = False
        #done with loop
        return rc

    #
    # Check all individual status codes for each setting and confirm it matches the input status code
    #
    def check_all_setting_status(self, resultfile, status):
        t = -1
        found = False
        xmlstring = ""
        statuscode = int(status, base=0)
        a = open(resultfile, "r")

        #find the start of the xml string and then copy all lines to xmlstring variable
        for l in a.readlines():
            if(found):
                xmlstring += l
            else:
                if l.lstrip().startswith("<?xml"):
                    xmlstring = l
                    found = True
        a.close()

        if (len(xmlstring) == 0) or (not found):
            print ("Result XML not found")
            return False

        #make an element tree from xml string
        r = None
        root = ET.fromstring(xmlstring)
        rc = True
        for e in root.findall("./Settings/SettingResult"):
            i = e.find("Id")
            r = e.find("Result")

            if(r is None):
                print("Failed to find a result node for id (%s)" % i.text.strip())
                return False

            result = int(r.text.strip(), base=0)
            print ("Result Status for Id (%s): %s" % (str(i.text.strip()), r.text))
            if(result != statuscode):
                print("Error.  Status Code for id (%s) didn't match expected" % i.text.strip())
                rc = False
        #done with loop
        return rc

    #
    # Check list list of settings results
    #
    def check_setting_status_by_dictionary(self, resultfile, settingdict):
        t = -1
        found = False
        xmlstring = ""
        a = open(resultfile, "r")

        #find the start of the xml string and then copy all lines to xmlstring variable
        for l in a.readlines():
            if(found):
                xmlstring += l
            else:
                if l.lstrip().startswith("<?xml"):
                    xmlstring = l
                    found = True
        a.close()

        if (len(xmlstring) == 0) or (not found):
            print ("Result XML not found")
            return False

        #make an element tree from xml string
        r = None
        root = ET.fromstring(xmlstring)
        rc = True
        for e in root.findall("./Settings/SettingResult"):
            i = e.find("Id")
            r = e.find("Result")

            if(r is None):
                print("Failed to find a result node for id (%s)" % i.text.strip())
                return False

            index = str(i.text.strip())
            result = int(r.text.strip(), base=0)
            print ("Result Status for Id (%s): %s" % (str(i.text.strip()), r.text))
            if index in settingdict:
                if result != int(settingdict[index], base=0):
                    print("Error.  Status Code for id (%s) didn't match expected" % i.text.strip())
                    rc = False;
            else:
                print("Error.  Index %s not in dictionary" % i.text.strip())
                rc - False;
        #done with loop
        return rc

    def extract_payload_from_current(self, resultfile, payloadfile):
        try:
            tree = ET.parse(resultfile)
            elem = tree.find('./SyncBody/Results/Item/Data')
        except:
            elem = None

        if elem is None:
            return  0x8000000000000007 # EFI_DEVICE_ERROR

        tree = xml.dom.minidom.parseString(elem.text.encode())
        f = open(payloadfile,"wb")
        f.write(tree.toprettyxml().encode())
        f.close
        return 0

    def extract_results_packet(self, resultfile, resultpktfile):
        try:
            tree = ET.parse(resultfile)
            elem = tree.find('./SyncBody/Results/Item/Data')
        except:
            elem = None

        if elem is None:
            return  0x8000000000000007, 0  # EFI_DEVICE_ERROR

        bindata = binascii.a2b_base64(elem.text)   #.decode('utf-16')
        f = BytesIO(bindata)

        g = open(resultpktfile,"wb")
        g.write(f.read())
        g.close()
        f.close()
        return 0

    def get_status_and_sessionid_from_identity_results(self, resultfile):
        f = open(resultfile, 'rb')
        rslt = CertProvisioningResultVariable(f)
        f.close()
        rslt.Print()
        return rslt.Status, rslt.SessionId

    def get_sessionid_from_identity_packet(self, identityfile):
        f = open(identityfile, 'rb')
        rslt = CertProvisioningApplyVariable(f)
        f.close()
        rslt.Print()
        return rslt.SessionId

    def get_status_and_sessionid_from_permission_results(self, resultfile):
        f = open(resultfile, 'rb')
        rslt = PermissionResultVariable(f)
        f.close()
        rslt.Print()
        return rslt.Status, rslt.SessionId

    def get_sessionid_from_permission_packet(self, identityfile):
        f = open(identityfile, 'rb')
        rslt = PermissionApplyVariable(f)
        f.close()
        rslt.Print()
        return rslt.SessionId

    def get_status_and_sessionid_from_settings_results(self, resultfile, checktype):
        f = open(resultfile, 'rb')
        rslt = SecureSettingsResultVariable(f)
        rslt.Print()
        f.close()
        if (checktype != "FULL") and (checktype != "BASIC"):
            print('checktype invalid')
            return  0x8000000000000007, 0  # EFI_DEVICE_ERROR
        RsltRc = rslt.Status
        if RsltRc == 0 and checktype == "FULL":
            try:
                tree = ET.fromstring(rslt.Payload)
                for elem in tree.findall('./Settings/SettingResult'):
                    rc = int (elem.find('Result').text,0)
                    if rc != 0:
                        RsltRc = rc
                    print('Setting %s - Code %s' % (elem.find('Id').text,elem.find('Result').text))
            except:
                traceback.print_exc()
        return RsltRc, rslt.SessionId

    def get_payload_from_permissions_results(self, resultfile, payloadfile):
        f = open(resultfile, 'rb')
        rslt = PermissionResultVariable(f)
        rslt.Print()
        f.close()
        if rslt.Payload != None:
            f = open(payloadfile,"w")
            f.write(rslt.Payload)
            f.close
        return 0

    def get_payload_from_settings_results(self, resultfile, payloadfile):
        f = open(resultfile, 'rb')
        rslt = SecureSettingsResultVariable(f)
        rslt.Print()
        f.close()
        if rslt.Payload != None:
            f = open(payloadfile,"w")
            f.write(rslt.Payload)
            f.close
        return 0

    def get_sessionid_from_settings_packet(self, settingsfile):
        f = open(settingsfile, 'rb')
        rslt = SecureSettingsApplyVariable(f)
        f.close()
        rslt.Print()
        return rslt.SessionId

    def get_status_from_dmtools_results(self, resultsfile):
        result = 0x8000000000000007  # EFI_DEVICE_ERROR
        try:
            tree = ET.parse(resultsfile)
            root = tree.getroot()
            result = 0
            for e in root.findall("./SyncBody/Status"):
                i = e.find("Data")
                if (i.text != "200"):
                    result = 0x8000000000000007  # EFI_DEVICE_ERROR
                    break
                result = 0
        except:
            traceback.print_exc()

        return result

    def get_uefistatus_string (self, StatusCode):
        print(type(StatusCode))

        isint = False
        if isinstance(StatusCode, int):
            isint = True

        if isint == True:
            Ret = UefiStatusCode().Convert64BitToString(StatusCode)
        else:
            Ret = UefiStatusCode().ConvertHexString64ToString(StatusCode)

        if Ret == '':
            Ret = '%x' % StatusCode
        return Ret

    def print_xml_payload(self, XmlFileName):
        try:
            tree = xml.dom.minidom.parse(XmlFileName)
            print('%s' % tree.toprettyxml())
        except:
            traceback.print_exc()
            print('Unable to print settings XML')

    def build_target_parameters(self, Version, SerialNumber = '', Mfg = '', ProdName = ''):
        rslt = []

        if Version == 'V1':
            rslt.append('--HdrVersion')
            rslt.append('1')
            if SerialNumber != '':
                rslt.append('--SnTarget')
                rslt.append(SerialNumber)

        elif Version == 'V2':
            rslt.append('--HdrVersion')
            rslt.append('2')
            if Mfg != '':
                rslt.append('--SMBIOSMfg')
                rslt.append(Mfg)
            if ProdName != '':
                rslt.append('--SMBIOSProd')
                rslt.append(ProdName)
            if SerialNumber != '':
                rslt.append('--SMBIOSSerial')
                rslt.append(SerialNumber)

        else:
            raise ValueError ('Invalid version {}'.format(Version))

        return rslt

    def get_device_ids(self, XmlFileName):
        d = {}
        try:
            tree = ET.parse(XmlFileName)
            root = tree.getroot()

            elem = root.findall('./Identifiers/Identifier')
            for e in elem:
                xid = e.find('Id')
                pn = e.find('Value')
         #       print(' {0} = {1}'.format(xid.text, pn.text)
                d[xid.text] = pn.text

            for key in d:
                print (' Key {0} has the value of {1}'.format(key,  d[key]))

        except:
            traceback.print_exc()
            print('Unable to extract DeviceIdElements.')
            d = {}

        return d

    def get_device_id_element(self, idXmlFile, id):

        d = self.get_device_ids(idXmlFile)
        return d[id]

    def verify_device_id(self, XmlFileName, Mfg, ProdName, SN):

        d = self.get_device_ids(XmlFileName)

        Manufacturer =  d['Manufacturer']
        ProductName  =  d['Product Name']
        SerialNumber =  d['Serial Number']

        rc = 0;
        if Mfg != Manufacturer:
            rc += 4
        if ProdName != ProductName:
            rc += 8
        if SN != SerialNumber:
            rc += 16

        return rc

    def get_dfci_version(self, XmlFileName):
        try:
            tree = ET.parse(XmlFileName)
            root = tree.getroot()

            elem = root.find('./DfciVersion')
            d = elem.text
            print ('Dfci Version detected as %s' % d)

        except:
            traceback.print_exc()
            print('Unable to extract DfciVersion from %s' % XmlFileName)
            d = None

        return d

    def verify_dfci_version(self, XmlFileName, Version):

        d = self.get_dfci_version(XmlFileName)

        rc = True;
        if d != Version:
            rc = False

        return rc

    def get_thumbprints(self, XmlFileName):
        d = {}
        try:
            tree = ET.parse(XmlFileName)
            root = tree.getroot()

            elem = root.findall('./Certificates/Certificate')
            for e in elem:
                xid = e.find('Id')
                pn = e.find('Value')
         #       print(' {0} = {1}'.format(xid.text, pn.text)
                d[xid.text] = pn.text

            for key in d:
                print (' Key {0} has the value of {1}'.format(key,  d[key]))

        except:
            traceback.print_exc()
            print('Unable to extract DeviceIdElements.')
            d = {}

        return d

    def get_thumbprint_element(self, xmlFile, id):
        d = self.get_thumbprints(xmlFile)
        return d[id]

    #
    # Determine if the DUT is online by pinging it
    #
    def is_device_online(self, ipaddress):
        output = subprocess.Popen(["ping.exe", "-n", "1", ipaddress],stdout = subprocess.PIPE).communicate()[0]

        if (b'TTL' in output):
            return True
        else:
            return False

    def get_signtool_path(self):
        global SignToolPath
        if SignToolPath == None:
            SignToolPath = FindToolInWinSdk ("signtool.exe")

            # check if exists
            if SignToolPath is None or not os.path.exists(SignToolPath):
                raise Exception("Can't find signtool.exe on this machine.  Please install the Windows 10 WDK - "
                                "https://developer.microsoft.com/en-us/windows/hardware/windows-driver-kit")

        return SignToolPath

    def get_certmgr_path(self):
        global CertMgrPath
        if CertMgrPath == None:
            CertMgrPath = FindToolInWinSdk ("certmgr.exe")

            # check if exists
            if CertMgrPath is None or not os.path.exists(CertMgrPath):
                raise Exception("Can't find certmgr.exe on this machine.  Please install the Windows 10 WDK - "
                                "https://developer.microsoft.com/en-us/windows/hardware/windows-driver-kit")

        return CertMgrPath
