# @file
#
# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
import os
import logging
from edk2toolext.environment import shell_environment
from edk2toolext.invocables.edk2_ci_build import CiBuildSettingsManager
from edk2toolext.invocables.edk2_ci_setup import CiSetupSettingsManager
from edk2toolext.invocables.edk2_setup import SetupSettingsManager, RequiredSubmodule
from edk2toolext.invocables.edk2_update import UpdateSettingsManager
from edk2toolext.invocables.edk2_pr_eval import PrEvalSettingsManager
from edk2toollib.utility_functions import GetHostInfo


class Settings(CiSetupSettingsManager, CiBuildSettingsManager, UpdateSettingsManager, SetupSettingsManager, PrEvalSettingsManager):

    def __init__(self):
        self.ActualPackages = []
        self.ActualTargets = []
        self.ActualArchitectures = []
        self.ActualToolChainTag = ""
        self.UseBuiltInBaseTools = None
        self.ActualScopes = None

    # ####################################################################################### #
    #                             Extra CmdLine configuration                                 #
    # ####################################################################################### #

    def AddCommandLineOptions(self, parserObj):
        group = parserObj.add_mutually_exclusive_group()
        group.add_argument("-force_piptools", "--fpt", dest="force_piptools", action="store_true", default=False, help="Force the system to use pip tools")
        group.add_argument("-no_piptools", "--npt", dest="no_piptools", action="store_true", default=False, help="Force the system to not use pip tools")

    def RetrieveCommandLineOptions(self, args):
        super().RetrieveCommandLineOptions(args)
        if args.force_piptools:
            self.UseBuiltInBaseTools = True
        if args.no_piptools:
            self.UseBuiltInBaseTools = False

    # ####################################################################################### #
    #                        Default Support for this Ci Build                                #
    # ####################################################################################### #

    def GetPackagesSupported(self):
        ''' return iterable of edk2 packages supported by this build.
        These should be edk2 workspace relative paths '''

        return (
            "AdvLoggerPkg",
            "DfciPkg",
            "MfciPkg",
            "HidPkg",
            "MsCorePkg",
            "MsGraphicsPkg",
            "MsWheaPkg",
            "PcBdsPkg",
            "UefiTestingPkg",
            "XmlSupportPkg",
            "ZeroTouchPkg",
            "FhrPkg"
        )

    def GetArchitecturesSupported(self):
        ''' return iterable of edk2 architectures supported by this build '''
        return ("IA32",
                "X64",
                "ARM",
                "AARCH64")

    def GetTargetsSupported(self):
        ''' return iterable of edk2 target tags supported by this build '''
        return ("DEBUG", "RELEASE", "NO-TARGET", "NOOPT")

    # ####################################################################################### #
    #                     Verify and Save requested Ci Build Config                           #
    # ####################################################################################### #

    def SetPackages(self, list_of_requested_packages):
        ''' Confirm the requested package list is valid and configure SettingsManager
        to build the requested packages.

        Raise UnsupportedException if a requested_package is not supported
        '''
        unsupported = set(list_of_requested_packages) - \
            set(self.GetPackagesSupported())
        if(len(unsupported) > 0):
            logging.critical(
                "Unsupported Package Requested: " + " ".join(unsupported))
            raise Exception("Unsupported Package Requested: " +
                            " ".join(unsupported))
        self.ActualPackages = list_of_requested_packages

    def SetArchitectures(self, list_of_requested_architectures):
        ''' Confirm the requests architecture list is valid and configure SettingsManager
        to run only the requested architectures.

        Raise Exception if a list_of_requested_architectures is not supported
        '''
        unsupported = set(list_of_requested_architectures) - \
            set(self.GetArchitecturesSupported())
        if(len(unsupported) > 0):
            logging.critical(
                "Unsupported Architecture Requested: " + " ".join(unsupported))
            raise Exception(
                "Unsupported Architecture Requested: " + " ".join(unsupported))
        self.ActualArchitectures = list_of_requested_architectures

    def SetTargets(self, list_of_requested_target):
        ''' Confirm the request target list is valid and configure SettingsManager
        to run only the requested targets.

        Raise UnsupportedException if a requested_target is not supported
        '''
        unsupported = set(list_of_requested_target) - \
            set(self.GetTargetsSupported())
        if(len(unsupported) > 0):
            logging.critical(
                "Unsupported Targets Requested: " + " ".join(unsupported))
            raise Exception("Unsupported Targets Requested: " +
                            " ".join(unsupported))
        self.ActualTargets = list_of_requested_target

    # ####################################################################################### #
    #                         Actual Configuration for Ci Build                               #
    # ####################################################################################### #

    def GetActiveScopes(self):
        ''' return tuple containing scopes that should be active for this process '''
        if self.ActualScopes is None:
            scopes = ("cibuild", "edk2-build", "host-based-test")

            self.ActualToolChainTag = shell_environment.GetBuildVars().GetValue("TOOL_CHAIN_TAG", "")

            is_linux = GetHostInfo().os.upper() == "LINUX"

            if self.UseBuiltInBaseTools is None:
                is_linux = GetHostInfo().os.upper() == "LINUX"
                # try and import the pip module for basetools
                try:
                    import edk2basetools
                    self.UseBuiltInBaseTools = True
                except ImportError:
                    self.UseBuiltInBaseTools = False
                    pass

            if self.UseBuiltInBaseTools == True:
                scopes += ('pipbuild-unix',) if is_linux else ('pipbuild-win',)
                logging.warning("Using Pip Tools based BaseTools")
            else:
                logging.warning("Falling back to using in-tree BaseTools")

            if is_linux and self.ActualToolChainTag.upper().startswith("GCC"):
                if "AARCH64" in self.ActualArchitectures:
                    scopes += ("gcc_aarch64_linux",)
                if "ARM" in self.ActualArchitectures:
                    scopes += ("gcc_arm_linux",)
                if "RISCV64" in self.ActualArchitectures:
                    scopes += ("gcc_riscv64_unknown",)
            self.ActualScopes = scopes
        return self.ActualScopes

    def GetRequiredSubmodules(self):
        ''' return iterable containing RequiredSubmodule objects.
        If no RequiredSubmodules return an empty iterable
        '''
        rs = []
        return rs

    def GetName(self):
        return "MuPlus"

    def GetDependencies(self):
        ''' Return Git Repository Dependencies

        Return an iterable of dictionary objects with the following fields
        {
            Path: <required> Workspace relative path
            Url: <required> Url of git repo
            Commit: <optional> Commit to checkout of repo
            Branch: <optional> Branch to checkout (will checkout most recent commit in branch)
            Full: <optional> Boolean to do shallow or Full checkout.  (default is False)
            ReferencePath: <optional> Workspace relative path to git repo to use as "reference"
        }
        '''
        return [
            {
                "Path": "MU_BASECORE",
                "Url": "https://github.com/microsoft/mu_basecore.git",
                "Branch": "release/202208"
            },
            {
                "Path": "Silicon/Arm/MU_TIANO",
                "Url": "https://github.com/Microsoft/mu_silicon_arm_tiano.git",
                "Branch": "release/202208"
            },
            {
                "Path": "Silicon/Intel/MU_TIANO",
                "Url": "https://github.com/Microsoft/mu_silicon_intel_tiano.git",
                "Branch": "release/202208"
            },
            {
                "Path": "Common/MU_TIANO",
                "Url": "https://github.com/Microsoft/mu_tiano_plus.git",
                "Branch": "release/202208"
            }
        ]

    def GetPackagesPath(self):
        ''' Return a list of workspace relative paths that should be mapped as edk2 PackagesPath '''
        result = []
        for a in self.GetDependencies():
            result.append(a["Path"])
        return result

    def GetWorkspaceRoot(self):
        ''' get WorkspacePath '''
        return os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

    def FilterPackagesToTest(self, changedFilesList: list, potentialPackagesList: list) -> list:
        ''' Filter potential packages to test based on changed files. '''
        return []
