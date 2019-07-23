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
from MuEnvironment.NugetDependency import NugetDependency
from MuPythonLibrary.UtilityFunctions import RunCmd
import shutil
import tempfile
import argparse
import glob
from io import StringIO
#from setuptools_scm import get_version
import re

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
PROJECT_SCOPE = ("corebuild", "sharedcrypto_build")

MODULE_PKGS = ('SharedCryptoPkg/MU_BASECORE_extdep/MU_BASECORE', "SharedCryptoPkg/MU_ARM_TIANO_extdep/MU_ARM_TIANO", "SharedCryptoPkg/MU_TIANO_extdep/MU_TIANO")
MODULE_PKG_PATHS = ";".join(os.path.join(WORKSPACE_PATH, pkg_name) for pkg_name in MODULE_PKGS)
ACTIVE_TARGET = None
SHOULD_DUMP_VERSION = False
RELEASE_NOTES_FILENAME ="release_notes.md"
PACKAGE_NAME="Mu-SharedCrypto"


# Because we reimport this module we need to grab the other version when we are in PlatformBuildworker namespace as opposed to main
def GetAPIKey():
    if __name__ == "__main__":
        global API_KEY
        return API_KEY
    else:
        pbw = __import__("__main__")
        return pbw.API_KEYv  # import the API KEY


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


def ShouldDumpVersion():
    if __name__ == "__main__":
        global SHOULD_DUMP_VERSION
        return SHOULD_DUMP_VERSION
    else:
        pbw = __import__("__main__")
        return pbw.SHOULD_DUMP_VERSION


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


def GetLatestNugetVersion(package_name, source=None):
    cmd = NugetDependency.GetNugetCmd()
    cmd += ["list"]
    cmd += [package_name]
    if source is not None:
        cmd += ["-Source", source]
    return_buffer = StringIO()
    if (RunCmd(cmd[0], " ".join(cmd[1:]), outstream=return_buffer) == 0):
        # Seek to the beginning of the output buffer and capture the output.
        return_buffer.seek(0)
        return_string = return_buffer.read()
        return_buffer.close()
        return return_string.strip().strip(package_name).strip()
    else:
        return "0.0.0.0"


def GetReleaseNote():
    cmd = "log --format=%B -n 1 HEAD"
    return_buffer = StringIO()
    if (RunCmd("git", cmd, outstream=return_buffer) == 0):
        # Seek to the beginning of the output buffer and capture the output.
        return_buffer.seek(0)
        return_string = return_buffer.read(155).replace("\n"," ")  # read the first 155 characters and replace the
        return_buffer.close()
        # TODO: figure out if there was more input and append a ... if needed
        return return_string.strip()
    else:
        raise RuntimeError("Unable to read release notes")


def CreateReleaseNotes(note:str, new_version:str, old_release_notes:list, hashes:dict):
    scriptDir = SCRIPT_PATH

    release_notes_path = os.path.join(scriptDir, RELEASE_NOTES_FILENAME)
    current_notes_file  = open(release_notes_path, "r")
    notes = current_notes_file.readlines()
    current_notes_file.close()

    notes += ["\n"]
    notes += ["## {}- {}\n".format(new_version, note), "\n"]
    for repo in hashes:
        repo_hash = hashes[repo]
        notes += ["{}@{}\n".format(repo, repo_hash)]
    notes += ["\n"]

    # write our notes out to the file
    current_notes_file  = open(release_notes_path, "w")
    current_notes_file.writelines(notes)
    current_notes_file.writelines(old_release_notes)
    current_notes_file.close()


def GetCommitHashes(root_dir:os.PathLike):
    # Recursively looks at every .git and gets the commit from there
    search_path = os.path.join(root_dir, "**", ".git")
    search = glob.iglob(search_path, recursive=True)
    found_repos = {}
    cmd_args = "rev-parse HEAD"
    for git_path in search:
        git_path_dir = os.path.dirname(git_path)
        if git_path_dir == root_dir:
            git_repo_name = "MU_PLUS"
        else:
            _, git_repo_name = os.path.split(git_path_dir)
        git_repo_name = git_repo_name.upper()
        if git_repo_name in found_repos:
            raise RuntimeError("we've already found this repo before "+git_repo_name)
        # read the git hash for this repo
        return_buffer = StringIO()
        RunCmd("git", cmd_args, workingdir=git_path_dir, outstream=return_buffer)
        commit_hash = return_buffer.getvalue().strip()
        return_buffer.close()
        found_repos[git_repo_name] = commit_hash
    return found_repos


def GetOldReleaseNotesAndHashes(notes_path:os.PathLike):
    # Read in the text file
    if not os.path.isfile(notes_path):
        raise FileNotFoundError("Unable to find the old release notes")
    old_notes_file  = open(notes_path, "r")
    notes = old_notes_file.readlines()
    old_notes_file.close()
    version_re = re.compile(r'##\s*\d+\.\d+\.\d+\.\d+')
    while len(notes) > 0:  # go through the notes until we find a version
        if not version_re.match(notes[0]):
            del notes[0]
        else:
            break
    old_hashes = {}
    hash_re = re.compile(r'([\w_]+)@([\w\d]+)')
    for note_line in notes:
        hash_match = hash_re.match(note_line)
        if hash_match:
            repo, commit_hash = hash_match.groups()
            repo = str(repo).upper()
            if repo not in old_hashes:
                old_hashes[repo] = commit_hash

    return (notes, old_hashes)


def DownloadNugetPackageVersion(package_name:str, version:str, destination:os.PathLike, source=None):
    cmd = NugetDependency.GetNugetCmd()
    cmd += ["install", package_name]
    if source is not None:
        cmd += ["-Source", source]
    cmd += ["-ExcludeVersion"]
    cmd += ["-Version", version]
    cmd += ["-Verbosity", "detailed"]
    cmd += ["-OutputDirectory", '"' + destination + '"']
    ret = RunCmd(cmd[0], " ".join(cmd[1:]))
    if ret != 0:
        return False
    else:
        return True

def GetReleaseForCommit(commit_hash:str):
    git_dir = os.path.dirname(SCRIPT_PATH)
    cmd_args = ["log", '--format="%h %D"', "--all", "-n 100"]
    return_buffer = StringIO()
    RunCmd("git", " ".join(cmd_args), workingdir=git_dir, outstream=return_buffer)
    return_buffer.seek(0)
    results = return_buffer.readlines()
    return_buffer.close()
    log_re = re.compile(r'release/(\d{6})')
    for log_item in results:
        commit = log_item[:11]
        branch = log_item[11:].strip()
        if len(branch) == 0:
            continue
        match = log_re.search(branch)
        if match:
            return match.group(1)

    raise RuntimeError("We couldn't find the release branch that we correspond to")


def GetSubVersions(old_version:str, current_release:str, curr_hashes:dict, old_hashes:dict):
    year, month, major, minor = old_version.split(".")
    curr_year = int(current_release[0:4])
    curr_mon = int(current_release[4:])
    differences = []
    for repo in curr_hashes:
        if repo not in old_hashes:
            logging.warning("Skipping comparing "+repo)
            continue
        if curr_hashes[repo] != old_hashes[repo]:
            differences.append(repo)
    if curr_year != int(year) or curr_mon != int(month):
        major = 1
        minor = 1
    elif "OPENSSL" in differences or "MU_TIANO" in differences:
        major = int(major) + 1
        minor = 1
    elif len(differences) > 0:
        minor = int(minor) + 1
    return "{}.{}".format(major, minor)


def GetNextVersion():
    # first get the last version
    old_version = GetLatestNugetVersion(PACKAGE_NAME)  # TODO: get the source from the JSON file that configures this
    # Get a temporary folder to download the nuget package into
    temp_nuget_path = tempfile.mkdtemp()
    # Download the Nuget Package
    DownloadNugetPackageVersion(PACKAGE_NAME, old_version, temp_nuget_path)
    # Unpack and read the previous release notes, skipping the header, also get hashes
    old_notes, old_hashes = GetOldReleaseNotesAndHashes(os.path.join(temp_nuget_path, PACKAGE_NAME, PACKAGE_NAME, RELEASE_NOTES_FILENAME))
    # Get the current hashes of open ssl and ourself
    curr_hashes = GetCommitHashes(WORKSPACE_PATH)
    # Figure out what release branch we are in
    current_release = GetReleaseForCommit(curr_hashes["MU_PLUS"])
    # Put that as the first two pieces of our version
    new_version = current_release[0:4]+"."+current_release[4:]+"."
    # Calculate the newest version
    new_version += GetSubVersions(old_version, current_release, curr_hashes, old_hashes)
    if new_version == old_version:
        raise RuntimeError("We are unable to republish the same version that was published last")
    # Create the release note from this branch, currently the commit message on the head?
    release_note = GetReleaseNote()
    # Create our release notes, appending the old ones
    CreateReleaseNotes(release_note, new_version, old_notes, curr_hashes)
    # Clean up the temporary nuget file?
    logging.critical("Creating new version:"+new_version)
    return new_version


def PublishNuget():
    logging.info("Running NugetPackager")
    # get the root directory of mu_basecore
    scriptDir = SCRIPT_PATH
    rootDir = WORKSPACE_PATH
    API_KEY = GetAPIKey()
    DUMP_VERSION = ShouldDumpVersion()
    VERSION = GetNextVersion()

    if (DUMP_VERSION):
        print("##vso[task.setvariable variable=NugetPackageVersion;isOutput=true]"+VERSION)

    # move the EFI's we generated to a folder to upload
    output_dir = os.path.join(rootDir, "Build", "SharedCrypto_NugetOutput")
    build_dir = os.path.join(rootDir, "Build")
    try:
        if os.path.exists(output_dir):
            shutil.rmtree(output_dir, ignore_errors=False)
        os.makedirs(output_dir)
    except Exception:
        logging.error("Ran into trouble getting Nuget Output Path setup")
        return False

    # copy the release notes and the license
    CopyFile(scriptDir, output_dir, "feature_sharedcrypto.md")
    CopyFile(scriptDir, output_dir, "LICENSE.txt")
    CopyFile(os.path.join(scriptDir, "Driver"), output_dir, "Mu-SharedCrypto.md")
    CopyFile(scriptDir, output_dir, "release_notes.md")  # we will generate new release notes when we Get the next version

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
        file_name = os.path.basename(txt)
        srcDir = os.path.dirname(txt)
        target = os.path.basename(srcDir)
        shutil.copyfile(os.path.join(srcDir, file_name), os.path.join(output_dir, target + file_name))

    logging.info("Attempting to package the Nuget package")
    config_file = os.path.join("Driver", "Mu-SharedCrypto.config.json")
    if API_KEY is not None:
        logging.info("Will attempt to publish as well")
        params = "--Operation PackAndPush --ConfigFilePath {0} --Version {1} --InputFolderPath {2}  --ApiKey {3}".format(config_file, VERSION, output_dir, API_KEY)
    else:
        params = "--Operation Pack --ConfigFilePath {0} --Version {1} --InputFolderPath {2} --OutputFolderPath {3}".format(config_file, VERSION, output_dir, build_dir)
    # TODO: change this from a runcmd to directly invoking nuget publishing
    ret = UtilityFunctions.RunCmd("nuget-publish", params, capture=True, workingdir=scriptDir)
    if ret == 0:
        logging.critical("Finished packaging/publishing Nuget version {0}".format(VERSION))
    else:
        logging.error("Unable to pack/publish nuget package")
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
    logging.info("Copying {0}: {1}".format(binary_name, binary))
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
        # We need to use flash image as it runs after post Build to restore the shell environment
        self.FlashImage = True

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
        # the shell_env is a singleton so it carries over between builds and we need to checkpoint it
        self.env.internal_shell_env.restore_checkpoint(self.shell_checkpoint)  # revert the last checkpoint
        return 0


# ==========================================================================
def reload_logging():
    logging.disable(logging.NOTSET)
    logging.shutdown()
    reload(logging)  # otherwise we get errors trying to talk to closed handlers


# Smallest 'main' possible. Please don't add unnecessary code.
if __name__ == '__main__':

    args = [sys.argv[0]]
    parser = argparse.ArgumentParser(description='Grab API Key')
    parser.add_argument('--api-key', dest="api_key", help='API key for NuGet')
    parser.add_argument('--dump-version', '--dumpversion', '--dump_version', dest="dump_version", action='store_true', default=False, help='whether or not to dump the version onto the command-line')
    parsed_args, remaining_args = parser.parse_known_args()  # this strips the first argument off
    if remaining_args is not None:  # any arguments that remain need to be tacked on
        args.extend(remaining_args)  # tack them on after the first argument
    sys.argv = args
    SetAPIKey(parsed_args.api_key)  # Set the API ley
    SHOULD_DUMP_VERSION = parsed_args.dump_version  # set the dump versions

    build_targets = ["DEBUG", "RELEASE"]
    doing_an_update = False
    if "--update" in (" ".join(sys.argv)).lower():
        doing_an_update = True

    for target in build_targets:
        # set the build target
        SetBuildTarget(target)
        sys.argv = args  # restore the args since they were consumed by the previous build entry
        try:
            CommonBuildEntry.build_entry(SCRIPT_PATH, WORKSPACE_PATH, REQUIRED_REPOS, PROJECT_SCOPE, MODULE_PKGS, MODULE_PKG_PATHS, worker_module='DriverBuilder')
        except SystemExit as e:  # catch the sys.exit call from uefibuild
            if e.code != 0:
                logging.warning("Stopping the build")
                break
        finally:
            if doing_an_update:
                print("Exiting because we're updating")
                break
        # this is hacky to close logging and restart it back to a default state
        reload_logging()
    # we've finished building
    # if we want to be verbose?
    logging.getLogger("").setLevel(logging.INFO)
    newHandler = logging.StreamHandler(sys.stdout)
    newHandler.setLevel(logging.NOTSET)
    logging.getLogger("").addHandler(newHandler)

    if not doing_an_update:
        logging.critical("--Creating NUGET package--")
        if not PublishNuget():
            logging.error("Failed to publish Nuget")
