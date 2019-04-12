##
# Script to Build Shared Crypto Driver
##
##
# Copyright Microsoft Corporation, 2018
##

import logging
from MuEnvironment.UefiBuild import UefiBuilder

# --------------------------------------------------------------------------------------------------------
# Subclass the UEFI builder and add platform specific functionality.
#


class PlatformBuilder(UefiBuilder):

    def __init__(self, WorkSpace, PackagesPath, PInManager, PInHelper, args):
        super(PlatformBuilder, self).__init__(
            WorkSpace, PackagesPath, PInManager, PInHelper, args)

    def SetPlatformEnv(self):

        self.env.SetValue("ACTIVE_PLATFORM", "SharedCryptoPkg/SharedCryptoDriver.dsc", "Platform Hardcoded")
        self.env.SetValue("TARGET_ARCH", "IA32 X64 AARCH64 ARM", "Platform Hardcoded")
        self.env.SetValue("TARGET", "RELEASE", "Platform Hardcoded")
        self.env.SetValue("CONF_TEMPLATE_DIR", "SharedCryptoPkg", "Conf template directory hardcoded - temporary and should go away")

        self.env.SetValue("LaunchBuildLogProgram", "Notepad", "default - will fail if already set", True)
        self.env.SetValue("LaunchLogOnSuccess", "True", "default - will fail if already set", True)
        self.env.SetValue("LaunchLogOnError", "True", "default - will fail if already set", True)

        return 0

    def SetPlatformEnvAfterTarget(self):
        logging.debug("PlatformBuilder SetPlatformEnvAfterTarget")
        return 0

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
        return True

    #
    # Platform defined flash method
    #
    def PlatformFlashImage(self):
        raise Exception("Flashing not supported")
