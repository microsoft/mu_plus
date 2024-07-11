# @file
# Setup script to publish binaries for DecodeUefiLog.py.
#
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

import os
import sys
import argparse
import logging
import PyInstaller.__main__

#
# Define the tool name
#
tool_name = "DecodeUefiLog.py"

def build_executable(script_name, script_path, output_file_path):
    print(
        f"Building executable for script: {script_name} at path: {script_path}"
    )  # Debugging line
    args = [script_path, "--clean", "--onefile"]

    if output_file_path is not None:
        if os.path.isdir(output_file_path):
            args.append(f"--distpath={output_file_path}")
        else:
            print(
                "The specified output file path does not exist or is not a directory."
            )
            return -1

    PyInstaller.__main__.run(args)


#
# main script function
#
def main():
    parser = argparse.ArgumentParser(
        description="Create executables for {tool_name}"
    )

    base_dir = os.path.dirname(os.path.abspath(__file__))

    build_executable(
        tool_name,
        os.path.join(base_dir, tool_name),
        os.getcwd(),
    )

    return 0


if __name__ == "__main__":
    # setup main console as logger
    logger = logging.getLogger("")
    logger.setLevel(logging.DEBUG)
    formatter = logging.Formatter("%(levelname)s - %(message)s")
    console = logging.StreamHandler()
    console.setLevel(logging.CRITICAL)
    console.setFormatter(formatter)
    logger.addHandler(console)

    # call main worker function
    retcode = main()

    if retcode != 0:
        logging.critical("Failed.  Return Code: %i" % retcode)
    # end logging
    logging.shutdown()
    sys.exit(retcode)
