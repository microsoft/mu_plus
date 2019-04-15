##
# Script to Build Shared Crypto Driver
##
##
# Copyright Microsoft Corporation, 2018
##
##
## Script to Build the SharedCryptoPkg binaries for NuGet publishing
##
##
## Copyright Microsoft Corporation, 2018
##
import os, sys
import logging
from MuEnvironment.UefiBuild import UefiBuilder
#
#==========================================================================
# PLATFORM BUILD ENVIRONMENT CONFIGURATION
#
SCRIPT_PATH = os.path.dirname(os.path.abspath(__file__))
WORKSPACE_PATH = os.path.dirname(os.path.dirname(SCRIPT_PATH))
REQUIRED_REPOS = ('MU_BASECORE', 'Common/MU', 'Common/MU_TIANO')
PROJECT_SCOPE = ("corebuild", "")

MODULE_PKGS = ('MU_BASECORE', 'Common/MU', 'Common/MU_TIANO')
MODULE_PKG_PATHS = ";".join(os.path.join(WORKSPACE_PATH, pkg_name) for pkg_name in MODULE_PKGS)

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

    def PlatformPostBuild(self):
        # TODO: Upload to nuget

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
#
#==========================================================================
#

# Smallest 'main' possible. Please don't add unnecessary code.
if __name__ == '__main__':
  # If CommonBuildEntry is not found, the mu_environment pip module has not been installed correctly
  try:
    from MuEnvironment import CommonBuildEntry
  except ImportError:
    print("Running Python version {0} from {1}".format(sys.version, sys.executable))
    raise RuntimeError("Please run \"python -m pip install --upgrade mu_build\".\nContact Microsoft Project Mu team if you run into any problems.")

  # Now that we have access to the entry, hand off to the common code.
  CommonBuildEntry.build_entry(SCRIPT_PATH, WORKSPACE_PATH, REQUIRED_REPOS, PROJECT_SCOPE, MODULE_PKGS, MODULE_PKG_PATHS)
