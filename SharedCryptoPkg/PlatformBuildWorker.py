##
# Script to Build Shared Crypto Driver
##
##
# Copyright Microsoft Corporation, 2018
##

import os
import sys
import stat
from optparse import OptionParser
import logging
import subprocess
import shutil
import struct
import uuid
from datetime import datetime
from datetime import date
import time
from MuEnvironment.UefiBuild import UefiBuilder

from MuPythonLibrary.UtilityFunctions import RunCmd
from MuPythonLibrary.UtilityFunctions import RunPythonScript

# --------------------------------------------------------------------------------------------------------
# Subclass the UEFI builder and add platform specific functionality.
#


class PlatformBuilder(UefiBuilder):

    def __init__(self, WorkSpace, PackagesPath, PInManager, PInHelper, args):
        super(PlatformBuilder, self).__init__(
            WorkSpace, PackagesPath, PInManager, PInHelper, args)

    def SetPlatformEnv(self):

        self.env.SetValue(
            "ACTIVE_PLATFORM", "SharedCryptoPkg/SharedCryptoDriver.dsc", "Platform Hardcoded")
        self.env.SetValue("PRODUCT_NAME", "SharedCrypto", "Platform Hardcoded")
        self.env.SetValue(
            "TARGET_ARCH", "IA32 X64 AARCH64 ARM", "Platform Hardcoded")
        self.env.SetValue("TARGET", "RELEASE", "Platform Hardcoded")

        self.env.SetValue("LaunchBuildLogProgram", "Notepad",
                          "default - will fail if already set", True)
        self.env.SetValue("LaunchLogOnSuccess", "True",
                          "default - will fail if already set", True)
        self.env.SetValue("LaunchLogOnError", "True",
                          "default - will fail if already set", True)

        return 0

    def __SetEsrtGuidVars(self, var_name, guid_str, desc_string):
        cur_guid = uuid.UUID(guid_str)
        self.env.SetValue("BLD_*_%s_REGISTRY" %
                          var_name, guid_str, desc_string)
        self.env.SetValue("BLD_*_%s_BYTES" % var_name, "{" + (
            ",".join(("0x%X" % byte) for byte in cur_guid.bytes_le)) + "}", desc_string)
        return

    def SetPlatformEnvAfterTarget(self):
        logging.debug("PlatformBuilder SetPlatformEnvAfterTarget")
        rc = 0

        #
        # if private/developer local build we must compute version id value.
        # If server build (public/shared build) the ID will be passed in and the
        # compute step will be skipped.
        #
        try:
            rc = self.ComputeVersionValue()

        # value error thrown when build id is outside valid range.  Should never happen unless logic is flawed
        except ValueError as arg:
            logging.critical(
                "ComputeVersionValue created invalid value.  " + str(arg))
            return -5

        if(rc != 0):
            logging.critical("ComputeVersionValue failed")
            return rc

        #
        # Validate that the value is correct for this product.
        #
        rc = self.ValidateVersionValue()
        if(rc != 0):
            logging.critical("ValidateVersionValue failed")
            return rc

        return rc

    def PlatformPostBuild(self):
        # TODO: Upload to nuget

        return 0

    # ------------------------------------------------------------------
    #
    # Method to do stuff pre build.
    # This is part of the build flow.
    # Currently do nothing.
    #
    # ------------------------------------------------------------------

    def PlatformPreBuild(self):
        return 0

    # ------------------------------------------------------------------
    #
    # Method for the platform to check if a gated build is needed
    # This is part of the build flow.
    # return:
    #  True -  Gated build is needed (default)
    #  False - Gated build is not needed for this platform
    # ------------------------------------------------------------------
    def PlatformGatedBuildShouldHappen(self):
        # for this product we use GIT.
        # When a gated build is requested we check the diff between
        # HEAD and requested branch (usually master)
        # if there are changes in this platformpkg or other changes outside the
        # that might impact this platform then a build should be done for this products
        directoriesICareAbout = [
            'MU_BASECORE', 'Common/MU_TIANO', 'Common/MU'
        ]
        # To optimize add directories within the cared about directories that should be ignored. . Be sure to use / instead of \
        directoriesToIgnore = []
        comparebranch = self.env.GetValue("GATEDBUILD_COMPARE_BRANCH")
        if comparebranch is None:
            logging.error("GATEDBUILD_COMPARE_BRANCH is not set")
            return True

        logging.debug("Return value from GitDiffGatedBuild %d" % ret)
        return True

    # ------------------------------------------------------------------
    #
    # Method to build capsule and package as a windows driver package
    #
    # ------------------------------------------------------------------
    def BuildCapsule(self):
        return 0

    def ComputeVersionValue(self):
        BuildType = self.env.GetValue("TARGET")
        BuildId = self.env.GetBuildValue("BUILDID")
        if(BuildId == None):
            # must compute
            buildint = 0
            now = datetime.now()

            #1 - Generation
            BuildGen = 1
            if((BuildGen & 0xFF) != BuildGen):
                logging.error(
                    "BUILD ID generation too large.  Max Value: %d.  Attempted Value: %d" % (0xFF, BuildGen))
                return -1
            buildint |= BuildGen

            # Date and time definition follows https://docs.python.org/3/library/datetime.html#datetime.date.isocalendar
            # 2 - Year since 2000, in BCD
            now_tuple = now.isocalendar()
            y = now_tuple[0] - 2000

            # Stackoverflow reference for https://stackoverflow.com/questions/209513/convert-hex-string-to-int-in-python
            if((y & 0xFF) != y):
                logging.error(
                    "BUILD ID year too large.  Max Value: %d.  Attempted Value: %d" % (0xFF, y))
                return -1
            y_BCD = int(str(y), 16)
            buildint = (buildint << 8) | y_BCD

            # 3 - Number of the Week, in BCD
            w = now_tuple[1]

            if((w & 0xFF) != w):
                logging.error(
                    "BUILD ID week too large.  Max Value: %d.  Attempted Value: %d" % (0xFF, ww))
                return -1
            w_BCD = int(str(w), 16)
            buildint = (buildint << 8) | w_BCD

            # 4 - Revision number
            BuildRev = self.env.GetValue("REVISION")
            buildrev = 0
            if (BuildRev == None):
                buildrev = 0
            elif (((int(BuildRev)) & 0xFF) != int(BuildRev)):
                logging.error(
                    "BUILD ID revision out of spec.  Max Value: %d.  Attempted Value: %s" % (0xFF, BuildRev))
                return -1
            else:
                buildrev = int(BuildRev)
            buildint = (buildint << 8) | buildrev

            # 5 - Generate build date value
            y = (now.year - 2000)
            m = now.month
            d = now.day

            if((m & 0xF) != m):
                raise ValueError(
                    "BUILD ID month too large.  Max Value: %d.  Attempted Value: %d" % (0xF, m))
            if((d & 0x1F) != d):
                raise ValueError(
                    "BUILD ID day too large.  Max Value: %d.  Attempted Value: %d" % (0x1F, d))

            dateint = (y << 9) | (m << 5) | d

            # set build id value
            self.env.SetValue("BLD_*_BUILDID", str(buildint),
                              "BuildId Computed")
            self.env.SetValue("BLD_*_BUILDDATE", dateint,
                              "Computed during Build Process")
        return 0

    def ValidateVersionValue(self):
        #
        # Validate the version value (32 bit encoded value)
        # Convert to string following the rules of the platform version string format (platform specific)
        # Print out the info.
        #

        #
        # This functions logic follows Eric Lee's defined Build ID definition
        # used on Project H, C, P.  To change product/milestone NO logic changes are needed as
        # they are set in global vars at the top of this file and  date is computed on the fly.
        #
        BuildId = self.env.GetBuildValue("BUILDID")
        if(BuildId == None):
            logging.debug("Failed to find a BuildId")
            return -1
        # Currently only check for generation in boundary and week number
        buildint = int(BuildId)
        #string = gg.yy.ww.rr in decimal
        rr = buildint & 0xFF  # 8bits
        buildint >>= 8

        ww = buildint & 0xFF  # 8bits
        ww = int(hex(ww).lstrip('0x'), 10)
        buildint >>= 8
        if ww > 53:
            logging.critical(
                "Week Number out of boundary. Calculated Value %d" % (ww))
            return -1

        yy = buildint & 0xFF  # 8bits
        yy = int(hex(yy).lstrip('0x'), 10) + 2000
        buildint >>= 8

        gg = ((buildint & 0xFF))  # 8bits

        buildstring = str(gg).zfill(3) + "." + str(yy).zfill(4) + \
            "." + str(ww).zfill(2) + "." + str(rr).zfill(3)

        logging.info("\tBuildVer:       " + BuildId)
        logging.info("\tBuildVer (hex): " + hex(int(BuildId)))
        logging.info("\tBuild String:   " + buildstring)
        logging.info("\tYear:           " + hex(int(yy)))
        logging.info("\tWeek:           " + hex(int(ww)))

        self.env.SetValue("BLD_*_BUILDID_STRING", buildstring,
                          "Computed during Build Process")

        return 0

    #
    # Platform defined flash method
    #
    def PlatformFlashImage(self):
        raise Exception("Flashing not supported")
