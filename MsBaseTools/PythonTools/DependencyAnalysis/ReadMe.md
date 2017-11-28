# Dependency Analysis Tool
## &#x1F539; Copyright
Copyright (c) 2017, Microsoft Corporation

All rights reserved. Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

## &#x1F539; About
This tool parses a UEFI Edk2 code tree and generates a HTML report that allows a developer to evaluate code dependencies.  The user can review each dependency using a fully searchable and filterable table and/or generate "network" graphs of the dependencies.  Dependencies can be grouped by module, package, and type to allow complete review.  

### &#x1F538; DependencyAnalysisTool.py
This is the parsing tool where all the UEFI code is inspected.  This python code relies on the MS Core UEFI Python library for file parsing and path management.  The tool will collapse all UEFI data into JSON and will then insert it into a HTML file based on the template.  

### &#x1F538; Dep_template.html
This is a single HTML file that allows the user to interact with the parsed data.  This template will be read by the python code and parsed data will be inserted as JSON into the predetermined spot.  The whole contents will then be written to the output report as specified.  This template file follows the MS Core UEFI HTML template design pattern and contains common tabs such as basic info, errors, and about.  The About tab contains attribution to all other Javascript and CSS libraries used to make the report.  It also contains custom tabs for thsi report.  


## &#x1F539; DependencyAnalysisTool command line options

| command line Syntax | What it is | Required | Extra Help
| --- | --- | --- | --- |
| -h, --help | Show help message | NO | |
| --Workspace *path* | absolute path to Edk2 workspace | YES | Should be the root workspace
| --OutputReport *reportfile* | Path to output report html | YES ||
| --InputDir *dir*| Folder to start parsing INFs | NO | Default will be same as workspace|
| --BuildReport *path* | Path to edk2 build report | NO | If not set the "as built" section of the report will be empty
|--CodebaseName *name* | Meaningful name for user. | NO | Will show up in report
|--CodebaseVersion *ver* | Meaningful version for user| NO | Will show up in report
|--PackagesPathCsvString *string* | a CSV string containing any Packages Path arguments used for the Edk2 build | NO|if your build uses package path you should set this the same so parsing is correct
|--IgnoreFolderCsvString *string* | a CSV string containing any folders you don't want parsed | NO | For example Build Output, version control folders, or directories you just want to ignore
|--debug | Turn on debug logging level for the file logger | NO ||
|-l *outputlogfile* | Create an output log for the parsing tool | NO ||


## &#x1F539; Example Usage
### Run on edk2 
1. Assume git repo at c:\src\edk2
1. Assume user doesn't want to parse some of the code that is less commonly used. 
1. No as built info.  If you wanted that you would need to turn on the edk2 build report and then add the --BuildReport *path* to the tool

> DependencyAnalysisTool.py --Workspace c:\src\edk2 --OutputReport Edk2_core.html --CodebaseName "TianoCore Edk2 Master" --CodebaseVersion b8c6c545385da1fec9f0851e2d4f1b769ff121af --IgnoreFolderCsvString "EdkCompatibilityPkg, StdLib, apppkg, armplatformpkg, armvirtpkg, beagleboardpkg, CorebootModulePkg, CorebootPayloadPkg, DuetPkg, EdkShellBinPkg, EdkShellPkg, EmbeddedPkg, EmulatorPkg, FatBinPkg, IntelFspPkg, IntelFspWrapperPkg, Nt32Pkg, Omap35xxPkg, OptionRomPkg, QuarkPlatformPkg, QuarkSocPkg, ShellBinPkg, StdLibPrivateInternalFiles, UnixPkg, Vlv2DeviceRefCodePkg, Vlv2TbltDevicePkg, ovmfpkg" -l Edk2DepTool.log --debug




 