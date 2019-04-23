##
# Tool to publish the cryptobin drivers to the nuget feed
#
#
# Copyright (c) 2018, Microsoft Corporation

# All rights reserved.
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
##

import sys
import os
from MuPythonLibrary import UtilityFunctions
from MuEnvironment import MuLogging
import logging
from shutil import copyfile
import shutil
import glob

version = "20190411.0.2"


def CopyFile(srcDir, destDir, file_name):
    copyfile(os.path.join(srcDir, file_name), os.path.join(destDir, file_name))


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


if __name__ == "__main__":
    # get the root directory of mu_plus
    scriptDir = os.path.dirname(os.path.realpath(__file__))
    driverDir = os.path.join(scriptDir, "Driver")
    rootDir = os.path.dirname(scriptDir)
    os.chdir(rootDir)  # set ourselves to there
    rootPath = os.path.realpath(os.path.join(rootDir, "ci.mu.yaml"))  # get our yaml file
    args = sys.argv

    if len(args) != 2:
        raise RuntimeError("You need to include your API key as the first argument")

    api_key = args[1]
    MuLogging.setup_console_logging(use_color=False, logging_level=logging.DEBUG)
    # move the EFI's we generated to a folder to upload
    NugetPath = os.path.join(rootDir, "MU_BASECORE", "BaseTools", "NugetPublishing")
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

    # copy the md file
    CopyFile(driverDir, output_dir, "Mu-SharedCrypto.md")

    sharedcrypto_build_dir = os.path.realpath(os.path.join(rootDir, "Build", "SharedCryptoPkg"))
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

    params = "--Operation PackAndPush --ConfigFilePath Mu-SharedCrypto.config.json --Version {2} --InputFolderPath {0}  --ApiKey {1}".format(output_dir, api_key, version)
    UtilityFunctions.RunPythonScript(NugetFilePath, params, capture=True, workingdir=driverDir)

    logging.critical("Finished")
    # make sure
