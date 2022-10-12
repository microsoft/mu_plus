# @file
#
# Script to Generate a a Permissions XML payload.
#
# Copyright (c), Microsoft Corporation
# SPDX-License-Identifier: BSD-2-Clause-Patent

import os, sys
import traceback
import argparse
import datetime
from DFCI_SupportLib import DFCI_SupportLib

class PermissionsXMLLib(object):

    #
    # Create Permissions XML
    #
    # Given a list of permissions, PMASK, and DMASK, create an XML permissions payload
    #
    def create_permissions_xml(self, filename, version, lsv, def_pmask, def_dmask, permissionslist):

        f = open(filename, "w")
        f.write('<?xml version="1.0" encoding="utf-8"?>\n')
        f.write('<PermissionsPacket xmlns="urn:UefiSettings-Schema">\n')
        f.write('    <CreatedBy>Dfci Testcase Libraries</CreatedBy>\n')
        f.write('    <CreatedOn>')

        print(datetime.datetime.now().strftime("%Y-%m-%d %H:%M"), end='', file=f)

        f.write('</CreatedOn>\n')
        f.write('    <Version>')
        print (version, end='', file=f)
        f.write('</Version>\n')
        f.write('    <LowestSupportedVersion>2</LowestSupportedVersion>\n')
        f.write('    <Permissions')
        f.write(' Default="')
        print(def_pmask, end='', file=f)
        f.write('" Delegated="')
        print(def_dmask, end='', file=f)
        f.write('" Append="False">')

        #
        # The permissions list is a list of a list.  The lowest level list is really a tuple of
        # permission id, PMASK, and DMASK.  DMASK may be None.
        #
        for permission in permissionslist:
            f.write('        <Permission>\n')
            f.write('            <Id>')
            print (permission[0], end='', file=f)
            f.write('</Id>\n')
            f.write('            <PMask>')
            print (permission[1], end='', file=f)
            f.write('</PMask>\n')
            if (permission[2] is not None):
                f.write('            <DMask>')
                print (permission[2], end='', file=f)
                f.write('</DMask>\n')
            f.write('        </Permission>\n')

        f.write('    </Permissions>\n')
        f.write('</PermissionsPacket>\n')

        f.close

        return True

    #
    # Validate Current Permissions
    #
    # Input is the current permissions and a list of permission/PMASK/DMASK tuples (list of lists)
    #
    # Ensure the settings in the checklist have the proper value
    #
    def validate_current_permissions(self, testname, currentPermissionsXmlFile, checklist):

        for item in checklist:
            a = DFCI_SupportLib()
            PMask, DMask = a.get_current_permission_value(currentPermissionsXmlFile, item[0])

            if (PMask != item[1]):
                print ('PMask Mismatch for %s, was=%s, Should be=%s' % (item[0], PMask, item[1]))
                return False

            if (PMask is not None):
                if (DMask != item[2]):
                    print ('DMask Mismatch for %s, was=%s, Should be=%s' % (item[0], DMask, item[2]))
                    return False;

        return True

    #
    # Validate Current Permission Defaults
    #
    # Input is the current permissions and the default PMASK and DMASK
    #
    # Ensure the settings in the checklist have the proper value
    #
    def validate_current_permission_defaults(self, testname, currentPermissionsXmlFile, CheckDefault, CheckDelegated):

        a = DFCI_SupportLib()
        Default, Delegated = a.get_current_permission_defaults(currentPermissionsXmlFile)

        if (Default != CheckDefault):
            print ('PMask Mismatch was %s should be %s' % (Default, CheckDefault))
            return False

        if (Delegated != CheckDelegated):
            print ('DMask Mismatch was %s should be %s' % (Delegated, CheckDelegated))
            return False;

        return True
