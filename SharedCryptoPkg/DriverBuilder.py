##
# Script to Build Shared Crypto Driver
# Copyright Microsoft Corporation, 2019
#
# This is to build the SharedTls binaries for NuGet publishing
##
import os
import sys
import logging
from MuEnvironment.UefiBuild import UefiBuilder
from MuPythonLibrary import UtilityFunctions
import shutil
import glob

# ==========================================================================
# PLATFORM BUILD ENVIRONMENT CONFIGURATION
#
SCRIPT_PATH = os.path.dirname(os.path.abspath(__file__))
WORKSPACE_PATH = os.path.dirname(SCRIPT_PATH)
REQUIRED_REPOS = ('Common/MU_TIANO', "MU_BASECORE", "Silicon/Arm/MU_TIANO")
PROJECT_SCOPE = ("corebuild", "")

MODULE_PKGS = ('Common/MU_TIANO', "Silicon/Arm/MU_TIANO","MU_BASECORE")
MODULE_PKG_PATHS = ";".join(os.path.join(WORKSPACE_PATH, pkg_name) for pkg_name in MODULE_PKGS)
VERSION = "2019.03.01.01"

# Because we reimport this module we need to grab the other version when we are in PlatformBuildworker namespace as opposed to main
if __name__ == "__main__":
    API_KEY = ""
else:
    pbw = __import__("__main__")
    API_KEY = pbw.API_KEY


def GetAPIKey():
    global API_KEY
    return API_KEY


def SetAPIKey(key):
    global API_KEY
    API_KEY = key


def CopyFile(srcDir, destDir, file_name):
    shutil.copyfile(os.path.join(srcDir, file_name), os.path.join(destDir, file_name))


def GetArchitecture(path: str):
    path = os.path.normpath(path)
    path_parts = path.split(os.path.sep)
    return path_parts[1]


def GetTarget(path: str):
    path = os.path.normpath(path)
    path_parts = path.split(os.path.sep)
    target = path_parts[0]

    if "_" in target:
        return target.split("_")[0]
    return None


def PublishNuget():
    # get the root directory of mu_basecore
    scriptDir = SCRIPT_PATH
    rootDir = WORKSPACE_PATH
    # move the EFI's we generated to a folder to upload
    NugetPath = os.path.join(rootDir,"MU_BASECORE", "BaseTools", "NugetPublishing")
    NugetFilePath = os.path.join(NugetPath, "NugetPublishing.py")
    if not os.path.exists(NugetFilePath):
        raise FileNotFoundError(NugetFilePath)

    logging.info("Running NugetPackager")
    output_dir = os.path.join(rootDir, "Build", ".NugetOutput")

    try:
        if os.path.exists(output_dir):
            shutil.rmtree(output_dir, ignore_errors=True)
        os.makedirs(output_dir)
    except:
        logging.error("Ran into trouble getting Nuget Output Path setup")
        return False

    # copy the md file
    CopyFile(scriptDir, output_dir, "feature_sharedcrypto.md")
    CopyFile(os.path.join(scriptDir,"Driver"), output_dir, "Mu-SharedCrypto.md")

    sharedcrypto_build_dir = os.path.realpath(os.path.join(rootDir, "Build", "NetworkPkg"))
    sharedcrypto_build_dir_offset = len(sharedcrypto_build_dir) + 1
    build_dir_efi_search = os.path.join(sharedcrypto_build_dir, "**", "SharedCrypto*.efi")
    build_dir_dpx_search = os.path.join(sharedcrypto_build_dir, "**", "SharedCrypto*.depex")
    build_dir_pdb_search = os.path.join(sharedcrypto_build_dir, "**", "SharedCrypto*.pdb")
    logging.info("Searching {0}".format(build_dir_efi_search))
    for binary in glob.iglob(build_dir_efi_search, recursive=True):
        MoveArchTargetSpecificFile(binary, sharedcrypto_build_dir_offset, output_dir)

    for depex in glob.iglob(build_dir_dpx_search, recursive=True):
        MoveArchTargetSpecificFile(depex, sharedcrypto_build_dir_offset, output_dir)

    for pdb in glob.iglob(build_dir_pdb_search, recursive=True):
        MoveArchTargetSpecificFile(pdb, sharedcrypto_build_dir_offset, output_dir)

    API_KEY = GetAPIKey()
    config_file = os.path.join("Driver", "Mu-SharedCrypto.config.json")
    params = "--Operation PackAndPush --ConfigFilePath {0} --Version {1} --InputFolderPath {2}  --ApiKey {3}".format(config_file,VERSION, output_dir, API_KEY)
    ret = UtilityFunctions.RunPythonScript(NugetFilePath, params, capture=True, workingdir=scriptDir)
    if ret == 0:
        logging.critical("Finished publishing Nuget version {0}".format(VERSION))
    else:
        logging.error("Unable to publish nuget package")
        return False
    return True


def MoveArchTargetSpecificFile(binary, offset, output_dir):
    binary_path = binary[offset:]
    binary_name = os.path.basename(binary)
    binary_folder = os.path.dirname(binary)
    arch = GetArchitecture(binary_path)
    target = GetTarget(binary_path)
    if target is None:
        raise FileExistsError("Unknown file {0}".format(binary))
    dest_path = os.path.join(output_dir, target, arch)
    dest_filepath = os.path.join(dest_path, binary_name)
    if os.path.exists(dest_filepath):
        logging.warning("Skipping {0}: {1} from {2}".format(binary_name, dest_filepath, binary))
        return

    if not os.path.exists(dest_path):
        os.makedirs(dest_path)
    # logging.warning("Copying {0}: {1}".format(binary_name,binary))
    CopyFile(binary_folder, dest_path, binary_name)


# --------------------------------------------------------------------------------------------------------
# Subclass the UEFI builder and add platform specific functionality.
#
class PlatformBuilder(UefiBuilder):

    def __init__(self, WorkSpace, PackagesPath, PInManager, PInHelper, args):
        super(PlatformBuilder, self).__init__(
            WorkSpace, PackagesPath, PInManager, PInHelper, args)

    def SetPlatformEnv(self):

        self.env.SetValue("ACTIVE_PLATFORM", "SharedCryptoPkg/SharedCryptoPkgDriver.dsc", "Platform Hardcoded")
        self.env.SetValue("TARGET_ARCH", "IA32 X64 AARCH64 ARM", "Platform Hardcoded")
        self.env.SetValue("TARGET", "DEBUG", "Platform Hardcoded")
        self.env.SetValue("CONF_TEMPLATE_DIR", "NetworkPkg", "Conf template directory hardcoded - temporary and should go away")

        self.env.SetValue("LaunchBuildLogProgram", "Notepad", "default - will fail if already set", True)
        self.env.SetValue("LaunchLogOnSuccess", "False", "default - will fail if already set", True)
        self.env.SetValue("LaunchLogOnError", "False", "default - will fail if already set", True)

        return 0

    def PlatformPostBuild(self):
        # TODO: Upload to nuget
        if PublishNuget():
            return 0
        else:
            return 1

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

# ==========================================================================
#


# Smallest 'main' possible. Please don't add unnecessary code.
if __name__ == '__main__':
    # If CommonBuildEntry is not found, the mu_environment pip module has not been installed correctly

    if len(sys.argv) < 2:
        raise RuntimeError("You need to include your API key as the first argument")

    SetAPIKey(sys.argv[1])
    GetAPIKey()

    args = [sys.argv[0]]
    if len(sys.argv) > 2:
        args.extend(sys.argv[2:])
    sys.argv = args

    try:
        from MuEnvironment import CommonBuildEntry
    except ImportError:
        print("Running Python version {0} from {1}".format(sys.version, sys.executable))
        raise RuntimeError("Please run \"python -m pip install --upgrade mu_build\".\nContact Microsoft Project Mu team if you run into any problems.")

    # Now that we have access to the entry, hand off to the common code.
    # TODO: How do we support building release and debug at the same time?
    CommonBuildEntry.build_entry(SCRIPT_PATH, WORKSPACE_PATH, REQUIRED_REPOS, PROJECT_SCOPE, MODULE_PKGS, MODULE_PKG_PATHS, worker_module='DriverBuilder')
