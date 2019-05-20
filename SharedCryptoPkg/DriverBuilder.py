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
from MuEnvironment import CommonBuildEntry
import shutil
import argparse
import glob
try:
    from importlib import reload  # Python 3.4+ only.
except ImportError:
    print("You need at least Python 3.4+. Please upgrade!")
    raise

# ==========================================================================
# PLATFORM BUILD ENVIRONMENT CONFIGURATION
#
SCRIPT_PATH = os.path.dirname(os.path.abspath(__file__))
WORKSPACE_PATH = os.path.dirname(SCRIPT_PATH)
REQUIRED_REPOS = ('Common/MU_TIANO', "MU_BASECORE", "Silicon/Arm/MU_TIANO")
PROJECT_SCOPE = ("corebuild", "")

MODULE_PKGS = ('Common/MU_TIANO', "Silicon/Arm/MU_TIANO", "MU_BASECORE")
MODULE_PKG_PATHS = ";".join(os.path.join(WORKSPACE_PATH, pkg_name) for pkg_name in MODULE_PKGS)
ACTIVE_TARGET = None
VERSION = "2019.03.04.01"

# Because we reimport this module we need to grab the other version when we are in PlatformBuildworker namespace as opposed to main
def GetAPIKey():
    if __name__ == "__main__":
        global API_KEY
        return API_KEY
    else:
        pbw = __import__("__main__")
        return pbw.API_KEY # import the API KEY

def SetAPIKey(key):
    global API_KEY
    API_KEY = key

def SetBuildTarget(key):
    global ACTIVE_TARGET
    ACTIVE_TARGET = key

def GetBuildTarget():
    if __name__ == "__main__":
        global ACTIVE_TARGET
        return ACTIVE_TARGET
    else:
        pbw = __import__("__main__")
        return pbw.ACTIVE_TARGET

def CopyFile(srcDir, destDir, file_name=None):
    if file_name is None:
        file_name = os.path.basename(srcDir)
        srcDir = os.path.dirname(srcDir)
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
            shutil.rmtree(output_dir, ignore_errors=False)
        os.makedirs(output_dir)
    except:
        logging.error("Ran into trouble getting Nuget Output Path setup")
        return False

    # copy the md file
    CopyFile(scriptDir, output_dir, "feature_sharedcrypto.md")
    CopyFile(scriptDir, output_dir, "release_notes.md")
    CopyFile(os.path.join(scriptDir,"Driver"), output_dir, "Mu-SharedCrypto.md")

    sharedcrypto_build_dir = os.path.realpath(os.path.join(rootDir, "Build", "SharedCryptoPkg_Driver"))
    sharedcrypto_build_dir_offset = len(sharedcrypto_build_dir) + 1
    build_dir_efi_search = os.path.join(sharedcrypto_build_dir, "**", "SharedCrypto*.efi")
    build_dir_dpx_search = os.path.join(sharedcrypto_build_dir, "**", "SharedCrypto*.depex")
    build_dir_pdb_search = os.path.join(sharedcrypto_build_dir, "**", "SharedCrypto*.pdb")
    build_dir_txt_search = os.path.join(sharedcrypto_build_dir, "**", "BUILD_REPORT.txt")
    logging.info("Searching {0}".format(build_dir_efi_search))
    for binary in glob.iglob(build_dir_efi_search, recursive=True):
        MoveArchTargetSpecificFile(binary, sharedcrypto_build_dir_offset, output_dir)

    for depex in glob.iglob(build_dir_dpx_search, recursive=True):
        MoveArchTargetSpecificFile(depex, sharedcrypto_build_dir_offset, output_dir)

    for pdb in glob.iglob(build_dir_pdb_search, recursive=True):
        MoveArchTargetSpecificFile(pdb, sharedcrypto_build_dir_offset, output_dir)

    for txt in glob.iglob(build_dir_txt_search, recursive=True):
        CopyFile(txt, output_dir)

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
        # logging.warning("Skipping {0}: {1} from {2}".format(binary_name, dest_filepath, binary))
        return

    if not os.path.exists(dest_path):
        os.makedirs(dest_path)
    logging.critical("Copying {0}: {1}".format(binary_name,binary))
    CopyFile(binary_folder, dest_path, binary_name)


# --------------------------------------------------------------------------------------------------------
# Subclass the UEFI builder and add platform specific functionality.
#
class PlatformBuilder(UefiBuilder):

    def __init__(self, WorkSpace, PackagesPath, PInManager, PInHelper, args):
        super(PlatformBuilder, self).__init__(
            WorkSpace, PackagesPath, PInManager, PInHelper, args)

    def SetPlatformEnv(self):
        self.shell_checkpoint = self.env.internal_shell_env.checkpoint()
        if GetBuildTarget() is None:
            return 1
        self.env.SetValue("ACTIVE_PLATFORM", "SharedCryptoPkg/SharedCryptoPkgDriver.dsc", "Platform Hardcoded")
        self.env.SetValue("TARGET_ARCH", "IA32 X64 AARCH64 ARM", "Platform Hardcoded")
        self.env.SetValue("TARGET", GetBuildTarget(), "Platform Hardcoded")
        self.env.SetValue("BUILDREPORTING", "TRUE", "Platform Hardcoded")
        self.env.SetValue("BUILDREPORT_TYPES", 'PCD DEPEX LIBRARY BUILD_FLAGS', "Platform Hardcoded")

        #self.env.SetValue("CONF_TEMPLATE_DIR", "NetworkPkg", "Conf template directory hardcoded - temporary and should go away")

        self.env.SetValue("LaunchBuildLogProgram", "Notepad", "default - will fail if already set", True)
        self.env.SetValue("LaunchLogOnSuccess", "False", "default - will fail if already set", True)
        self.env.SetValue("LaunchLogOnError", "False", "default - will fail if already set", True)
        self.FlashImage = True

        return 0

    def PlatformPostBuild(self):
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

    def PlatformFlashImage(self):
        # we have to restore checkpoint in flash as there are certain steps that run inbetween PostBuild and Flash (build report, etc)
        self.env.internal_shell_env.restore_checkpoint(self.shell_checkpoint) # revert the last checkpoint
        return 0

# ==========================================================================
#

def reload_logging():
    logging.disable(logging.NOTSET)
    logging.shutdown()
    reload(logging)  # otherwise we get errors trying to talk to closed handlers


# Smallest 'main' possible. Please don't add unnecessary code.
if __name__ == '__main__':

    args = [sys.argv[0]]
    parser = argparse.ArgumentParser(description='Grab API Key')
    parser.add_argument('--api-key', dest="api_key", help='API key for NuGet')
    parsed_args, remaining_args = parser.parse_known_args() # this strips the first argument off
    if remaining_args is not None: # any arguments that remain need to be tacked on
        args.extend(remaining_args) # tack them on after the first argument
    sys.argv = args
    SetAPIKey(parsed_args.api_key)  # Set the API ley
    # set the build target
    SetBuildTarget("DEBUG")
    try:
        CommonBuildEntry.build_entry(SCRIPT_PATH, WORKSPACE_PATH, REQUIRED_REPOS, PROJECT_SCOPE, MODULE_PKGS, MODULE_PKG_PATHS, worker_module='DriverBuilder')
    except SystemExit as e:  # catch the sys.exit call from uefibuild
        if e.code != 0:
            raise
        print("Success" + str(e.code))

    # this is hacky to close logging and restart it back to a default state
    reload_logging()
    SetBuildTarget("RELEASE")
    sys.argv = args  # restore the args since they were consumed by the previous build entry
    try:
        CommonBuildEntry.build_entry(SCRIPT_PATH, WORKSPACE_PATH, REQUIRED_REPOS, PROJECT_SCOPE, MODULE_PKGS, MODULE_PKG_PATHS, worker_module='DriverBuilder')
    except SystemExit as e:
        if e.code != 0:
            raise
        print("Success" + str(e.code))

    if GetAPIKey() is not None:
        reload_logging()  # reload logging for publishing the nuget package
        logging.critical("--Publishing NUGET package--")
        if not PublishNuget():
            logging.error("Failed to publish Nuget")
    else:
        print("Skipping nuget publish step") # logging is trashed so we have to print
