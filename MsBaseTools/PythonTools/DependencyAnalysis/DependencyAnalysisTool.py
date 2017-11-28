''' @file

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
    THE POSSIBILITY OF SUCH DAMAGE.

    Copyright (C) 2017 Microsoft Corporation. All Rights Reserved.

'''

#
# Tool builds table of dependencies declared in INF files for a UEFI Code tree
#

import os
import sys
import logging
import argparse
import datetime
import json

#Add PythonLibrary base to path for import
sp = os.path.dirname(os.path.realpath(sys.argv[0]))
sys.path.append(os.path.join(os.path.dirname(os.path.dirname(sp)), "PythonLibrary"))

from Uefi.EdkII.Parsers.DecParser import *
from Uefi.EdkII.Parsers.InfParser import *
from Uefi.EdkII.PathUtilities import *
from Uefi.EdkII.Parsers.BuildReportParser import *

VERSION = "0.92"

#Cache of Parsed DEC objects
PackageObjectList = {}

#List of all parsing errors
Errors = []      

#####
# Helper routine to take key and value pairs and create json
#####
def JS(key, value, comma=True):
    r = json.JSONEncoder().encode(key.strip()) + ": " + json.JSONEncoder().encode(value.strip())
    if comma:
            r = r + ","
    return r

#######
# Given the Global and the InfObj find the package (dec)
# that meets the dependency
#
# If not found return None
######
def FindDepPackage( GlobalString, InfObj):
    global PackageObjectList
    global Errors
    for a in InfObj.PackagesUsed:
        DecObj = None
        if a in PackageObjectList:
            DecObj = PackageObjectList[a]
        else:
            DecObj = DecParser()
            DecObj.SetBaseAbsPath(InfObj.RootPath)
            DecObj.SetPackagePaths(InfObj.PPs)
            try:
                DecObj.ParseFile(a)
                PackageObjectList[a] = DecObj
            except Exception as e:
                Errors.append("Failed to Parse DEC %s.  Exception: %s" % (a, str(e)))
                continue
        
        #now find it
        if(GlobalString in DecObj.ProtocolsUsed):
            return DecObj.Dict["PACKAGE_NAME"].strip().lower()
        if(GlobalString in DecObj.PPIsUsed):
            return DecObj.Dict["PACKAGE_NAME"].strip().lower()
        if(GlobalString in DecObj.GuidsUsed):
            return DecObj.Dict["PACKAGE_NAME"].strip().lower()
        if(GlobalString in DecObj.LibrariesUsed):
            return DecObj.Dict["PACKAGE_NAME"].strip().lower()
        if(GlobalString in DecObj.PcdsUsed):
            return DecObj.Dict["PACKAGE_NAME"].strip().lower()
    
    return None

def FindLibraryClassPkg(LcName):
    global PackageObjectList
    global Errors
    if LcName is None:
        return None

    if LcName.lower() == "null":
        return None
    for dec, decobj in PackageObjectList.items():
        if(LcName in decobj.LibrariesUsed):
            return decobj.Dict["PACKAGE_NAME"].strip().lower()

    Errors.append("Library Class %s not declared in any DEC file" % LcName)
    return None


######
# Main routine with logic to iterate the filesystem, 
# generate all the dependencies and then convert into
# json to inject into the html template file
#
#####
def RunTool(ws, PackagesPath, InputDir, OutputReport, BuildReportFile, IgnoreDirs, Name, Version, ParameterStringForDisplay):
    global Errors
    PathHelper = Edk2Path(ws, PackagesPath)
    ModuleDeps = []  #hold all deps
    ModuleList = []
    AsBuiltMappings = []
    LibraryClassInstanceDict = {}

    #
    # Iterate and find all INFs
    #
    for root, dirs, files in os.walk(InputDir):

        #remove all the ignore directories
        for a in [x for x in dirs if x.lower() in IgnoreDirs]:
            logging.debug("Ignoring directory %s" % a)
            dirs.remove(a)

        for name in files:
            if (name.lower().endswith('.inf')):
                fp = os.path.join(root, name)
                logging.debug("Processing File Path: %s" % fp)
                pkg = PathHelper.GetContainingPackage(fp).strip().lower()
                ip = InfParser()
                ip.SetBaseAbsPath(ws)
                ip.SetPackagePaths(PackagesPath)
                ip.ParseFile(fp)
                path = PathHelper.GetEdk2RelativePathFromAbsolutePath(fp)
                for prot in ip.ProtocolsUsed:
                    # SrcPkg, SrcModule, Dep type, Dep Pkg, Dep, edk2path )
                    DepPkg = FindDepPackage(prot, ip)
                    if(DepPkg is None):
                        logging.critical("Dep (%s) not found in packages." % prot)
                        Errors.append("Protocol Dep %s in INF %s not found in packages.," % (prot, fp))
                        continue
                    logging.debug("Dep (%s) found in package %s" % (prot, DepPkg))
                    ModuleDeps.append({"SrcPkg": pkg, "SrcInf": name,"DepType": "Protocol", "DepPkg": DepPkg, "Dep":prot, "Edk2Path": path})

                for g in ip.GuidsUsed:
                    # SrcPkg, SrcModule, Dep type, Dep Pkg, Dep )
                    DepPkg = FindDepPackage(g, ip)
                    if(DepPkg is None):
                        logging.critical("Dep (%s) not found in packages." % g)
                        Errors.append("Guid Dep %s in INF %s not found in packages." % (g, fp))
                        continue
                    logging.debug("Dep (%s) found in package %s" % (g, DepPkg))
                    ModuleDeps.append({"SrcPkg": pkg, "SrcInf": name,"DepType": "Guid", "DepPkg": DepPkg, "Dep":g, "Edk2Path": path})

                for ppi in ip.PpisUsed:
                    # SrcPkg, SrcModule, Dep type, Dep Pkg, Dep )
                    DepPkg = FindDepPackage(ppi, ip)
                    if(DepPkg is None):
                        logging.critical("Dep (%s) not found in packages." % ppi)
                        Errors.append("Ppi Dep %s in INF %s not found in packages." % (ppi, fp))
                        continue
                    logging.debug("Dep (%s) found in package %s" % (ppi, DepPkg))
                    ModuleDeps.append({"SrcPkg": pkg, "SrcInf": name,"DepType": "Ppi", "DepPkg": DepPkg, "Dep":ppi, "Edk2Path": path})

                for lc in ip.LibrariesUsed:
                    # SrcPkg, SrcModule, Dep type, Dep Pkg, Dep )
                    DepPkg = FindDepPackage(lc, ip)
                    if(DepPkg is None):
                        logging.critical("Dep (%s) not found in packages." % lc)
                        Errors.append("Library Class Dep %s in INF %s not found in packages." % (lc, fp))
                        continue
                    logging.debug("Dep (%s) found in package %s" % (lc, DepPkg))
                    ModuleDeps.append({"SrcPkg": pkg, "SrcInf": name,"DepType": "Library", "DepPkg": DepPkg, "Dep":lc, "Edk2Path": path})

                for pcd in ip.PcdsUsed:
                    # SrcPkg, SrcModule, Dep type, Dep Pkg, Dep )
                    DepPkg = FindDepPackage(pcd, ip)
                    if(DepPkg is None):
                        logging.critical("Dep (%s) not found in packages." % pcd)
                        Errors.append("Pcd Dep %s in INF %s not found in packages." % (pcd, fp))
                        continue
                    logging.debug("Dep (%s) found in package %s" % (pcd, DepPkg))
                    ModuleDeps.append({"SrcPkg": pkg, "SrcInf": name,"DepType": "Pcd", "DepPkg": DepPkg, "Dep":pcd, "Edk2Path": path})

                # Now add to module list
                ModObj = {}
                ModObj["Edk2Path"] = path
                ModObj["Pkg"] = pkg
                try:
                    ModObj["Name"] = ip.Dict["BASE_NAME"]
                except Exception as e:
                    Errors.append("INF has no BASE_NAME %s" % ip.Path)
                    ModObj["Name"] = "NONE"
                ModObj["LibraryClass"] = ""
                ModObj["LibrarySupportedTypes"] = ""
                ModObj["SrcInf"] = name

                try: 
                    ModObj["Type"] = ip.Dict["MODULE_TYPE"]
                except Exception as e:
                    Errors.append("INF has no MODULE_TYPE %s" % ip.Path)
                    ModObj["Type"] = "NONE"
                
                if ip.LibraryClass != "":
                    ModObj["LibraryClass"] = ip.LibraryClass
                    ModObj["LibrarySupportedTypes"] = ip.SupportedPhases

                ModuleList.append(ModObj)

    # Process BuildReport if present
    if(BuildReportFile is not None) and (os.path.isfile(BuildReportFile)):
        ppcsv = ""
        for a in PackagesPath:
            ppcsv += "," + a
        br = BuildReport(BuildReportFile, ws, ppcsv, {})
        br.BasicParse()

        for k in sorted(br.Modules):
            mod = br.Modules[k]
            AsBuildObj = { "ModuleInf": mod.InfPath, "ModuleName": mod.Name, "LibMappings": mod.Libraries}
            AsBuiltMappings.append(AsBuildObj)
    else:
        BuildReportFile = ""

    #Make string for parameters and make sure to escape and quote any argument with spaces
    p = ''
    for a in ParameterStringForDisplay[1:]:
        if(a.strip().count(' ') == 0):
            p += " " + a
        else:
            p += ' \"' + a + '\"'

    #
    # Convert into JSON
    #
    js = "{" + JS("ToolVersion", VERSION) 
    js += JS("DateCollected", datetime.datetime.strftime(datetime.datetime.now(), "%A, %B %d, %Y %I:%M%p" ))
    js += JS("CodebaseName", Name)
    js += JS("CodebaseVersion", Version)
    js += JS("ParserCmd", p)

    js += "errors: " + json.dumps(list(set(Errors)))
    js += ", deps: " + json.dumps(ModuleDeps)
    js += ", ModList: "+ json.dumps(ModuleList)
    js += ", AsBuiltMappings: " + json.dumps(AsBuiltMappings)
    js += ", " + JS("BuildReport", BuildReportFile, False)
    js += "}"


    #
    # Open template and replace placeholder with json
    #
    f = open(OutputReport, "w")
    template = open(os.path.join(sp, "Dep_template.html"), "r")
    for line in template.readlines():
        if "%TO_BE_FILLED_IN_BY_PYTHON_SCRIPT%" in line:
            line = line.replace("%TO_BE_FILLED_IN_BY_PYTHON_SCRIPT%", js)
        f.write(line)
    template.close()
    f.close()

    return 0


#
# Parse and Validate Args.  Then run the tool
#
def main():

    parser = argparse.ArgumentParser(description='Parse Module INF file and show its dependencies')
    parser.add_argument("--Workspace", dest="ws", help="absolute path to Edk2 build workspace", default=None)
    parser.add_argument("--PackagesPathCsvString", dest="ppcsv", help="Packages Path csv (absolute paths or ws relative paths)", default=None)
    parser.add_argument("--OutputReport", dest="OutputReport", help="Path to output report", default=None)
    parser.add_argument("--InputDir", dest="InputDir", help="folder to start scanning.  Default is same as Workspace", default=None)
    parser.add_argument("--BuildReport", dest="BuildReport", help="optinal parameter will include graphs for as built dependencies as detected from build report", default=None)
    parser.add_argument("--IgnoreFolderCsvString", dest="IgnoreFoldersCSV", help="Add folders to ignore list when parsing for InputDir", default=None)
    parser.add_argument("--CodebaseName", help="Name of codebase.  Will be displayed on report", default="Test Codebase")
    parser.add_argument("--CodebaseVersion", help="Version of Codebase.  Will be displayed on report", default="1.0.0")

    #Turn on dubug level logging
    parser.add_argument("--debug", action="store_true", dest="debug", help="turn on debug logging level for file log",  default=False)
    #Output debug log
    parser.add_argument("-l", dest="OutputLog", help="Create an output log file: ie -l out.txt", default=None)

    options = parser.parse_args()

    #setup file based logging if outputReport specified
    if(options.OutputLog):
        if(len(options.OutputLog) < 2):
            logging.critical("the output log file parameter is invalid")
            return -2
        else:
            #setup file based logging
            filelogger = logging.FileHandler(filename=options.OutputLog, mode='w')
            if(options.debug):
                filelogger.setLevel(logging.DEBUG)
            else:
                filelogger.setLevel(logging.INFO)

            filelogger.setFormatter(formatter)
            logging.getLogger('').addHandler(filelogger)

    logging.info("Log Started: " + datetime.datetime.strftime(datetime.datetime.now(), "%A, %B %d, %Y %I:%M%p" ))

    #Do parameter validation 
    if(options.ws is None):
        logging.critical("No Workspace Set.")
        return -4

    if(options.InputDir is None):
        #assume workspace
        options.InputDir = options.ws
        
    if (not os.path.isdir(options.InputDir)):
        logging.critical("Invalid Path to folder")
        return -5

    if(options.OutputReport is None):
        logging.critical("No OutputReport Path")
        return -6

    PackagesPath = []
    if(options.ppcsv is None):
        logging.info("No packages path Csv String given")
    else:
        pplist = options.ppcsv.strip().split(",")
        for a in pplist:
            if(os.path.isabs(a)):
                PackagesPath.append(a.strip())
            else:
                PackagesPath.append(os.path.join(options.ws, a.strip()))
        logging.debug("PackagesPath: %s" % PackagesPath)

    IgnoreList = ["Build", ".git", ".vs"]
    if(options.IgnoreFoldersCSV is not None):
        IgnoreList.extend(options.IgnoreFoldersCSV.split(","))
    IgnoreList = [ig.strip().lower() for ig in IgnoreList]
    logging.debug("Ignoring Folders: %s" % IgnoreList)


    if(options.BuildReport is None):
        logging.debug("No Build Report specified")
    elif( not os.path.isfile(options.BuildReport)):
        logging.critical("Invalid BuildReport path set")
        return -7


    return RunTool(options.ws, PackagesPath, options.InputDir, options.OutputReport, options.BuildReport, IgnoreList, options.CodebaseName, options.CodebaseVersion, sys.argv )


#--------------------------------
# Control starts here
#
#--------------------------------
if __name__ == '__main__':
    #setup main console as logger
    logger = logging.getLogger('')
    logger.setLevel(logging.DEBUG)
    formatter = logging.Formatter("%(levelname)s - %(message)s")
    console = logging.StreamHandler()
    console.setLevel(logging.CRITICAL)
    console.setFormatter(formatter)
    logger.addHandler(console)

    #call main worker function
    retcode = main()
    #
    # For debugging you can remove the call to main() and hardcode the parameters.  
    # This makes python debugging easier
    #
    #RunTool("c:\\Src\\Core2", [], "c:\\Src\\Core2", "SeanOut.html")

    if retcode != 0:
        logging.critical("Failed.  Return Code: %i" % retcode)
    #end logging
    logging.shutdown()
    sys.exit(retcode)

