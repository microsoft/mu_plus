# @file
#
# Script to Generate a Settings XML payload.
#
# Copyright (c), Microsoft Corporation
# SPDX-License-Identifier: BSD-2-Clause-Patent

import os, sys
import traceback
import argparse
import datetime
from DFCI_SupportLib import DFCI_SupportLib

class SettingsXMLLib(object):

    #
    # Create Settings XML
    #
    # Given a list of settings and values, create an XML settings payload
    #
    def create_settings_xml(self, filename, version, lsv, settingslist):

        f = open(filename, "w")
        f.write('<?xml version="1.0" encoding="utf-8"?>\n')
        f.write('<SettingsPacket xmlns="urn:UefiSettings-Schema">\n')
        f.write('    <CreatedBy>Dfci Testcase Libraries</CreatedBy>\n')
        f.write('    <CreatedOn>')

        print(datetime.datetime.now().strftime("%Y-%m-%d %H:%M"), end='', file=f)

        f.write('</CreatedOn>\n')
        f.write('    <Version>')
        print (version, end='', file=f)
        f.write('</Version>\n')
        f.write('    <LowestSupportedVersion>2</LowestSupportedVersion>\n')
        f.write('    <Settings>\n')

        #
        # The settings list is a list of a list.  The lowest level list is really a tuple of
        # setting id and value
        #
        for setting in settingslist:
            f.write('        <Setting>\n')
            f.write('            <Id>')
            print (setting[0], end='', file=f)
            f.write('</Id>\n')
            f.write('            <Value>')
            print (setting[1], end='', file=f)
            f.write('</Value>\n')
            f.write('        </Setting>\n')

        f.write('    </Settings>\n')
        f.write('</SettingsPacket>\n')

        f.close

        return True

    #
    # Validate Current Settings
    #
    # Input is the current settings and a list of setting/value pairs (list of lists)
    #
    # Ensure the settings in the checklist have the proper value
    #
    def validate_current_settings(self, testname, currentSettingXmlFile, checklist):

        for item in checklist:
            a = DFCI_SupportLib()
            rc = a.check_current_setting_value(currentSettingXmlFile, item[0], item[1])
            if not rc:
                return rc

        return True
