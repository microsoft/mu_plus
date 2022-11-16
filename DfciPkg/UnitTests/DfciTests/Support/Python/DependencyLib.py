# @file
#
# Dependency Lib - Limited functionality to for a robot testcase to depende on a successful
#                  completion of a previous testcase.
#
# Copyright (c), Microsoft Corporation
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

from robot.libraries.BuiltIn import BuiltIn

class DependencyLib(object):
    ROBOT_LISTENER_API_VERSION = 2
    ROBOT_LIBRARY_SCOPE = "GLOBAL"

    def __init__(self):
        self.ROBOT_LIBRARY_LISTENER = self
        self.test_status = {}

    def require_test_case(self, name):
        key = name.lower()
        if (key not in self.test_status):
            BuiltIn().fail("required test case can't be found: '%s'" % name)

        if (self.test_status[key] != "PASS"):
            BuiltIn().fail("required test case failed: '%s'" % name)

        return True

    def _end_test(self, name, attrs):
        self.test_status[name.lower()] = attrs["status"]
