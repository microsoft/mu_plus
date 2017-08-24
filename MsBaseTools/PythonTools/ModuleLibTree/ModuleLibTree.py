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
# Tool recursively finds and prints all the libraries used by a module.  
#

import os, os.path
import sys
import logging
from optparse import OptionParser
import datetime
import re

#get script path
sp = os.path.dirname(os.path.realpath(sys.argv[0]))

#get workspace path
ws = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(sp)))) #UEFI workspace will be 4 levels up from this script

#setup python path for build modules
sys.path.append(os.path.join(ws, "UDK", "MsBaseTools", "PythonTools", "parsers"))

from baseparser import InfParser


def DisplayLibraryTree():
    for lib in LibraryInstanceList:
        if not LibraryInstanceDict.has_key(lib):
            print("{: <30} {: <100}".format(lib, 'Error: library not found in dictionary'))
            continue

        filepath = re.sub(r"/", r"\\", LibraryInstanceDict.get(lib))
        print("{: <15} {: <30}".format('START', lib))

        ip = InfParser()
        ip.SetBaseAbsPath(ws)
        ip.SetPackagePaths(Pkgs)
        ip.ParseFile(filepath)
        #print ip.LibrariesUsed
        
        if ip.LibraryClass != "":
            print("{: <15} {: <250}".format('Instance: ', filepath))
            DefFound = 0
 
            for p in ip.PackagesUsed:
                sp = ip.FindPath(p)
                lf = open(sp, "r")
                DecFile = lf.read()
                lf.close()

                regex = re.escape(ip.LibraryClass) + r"\|(.*?)\n"
                found = re.search(regex, DecFile, re.I)

                if found:
                    sp = found.group(1);
                    sp = os.path.join(os.path.dirname(p), sp)
                    sp = ip.FindPath(sp)
                    sp = re.sub(r"/", r"\\", sp)
                    print("{: <15} {: <250}".format('Definition: ', sp))
                    DefFound = 1
                    break

            if DefFound == 0:
                 print("{: <15} {: <10}".format('Definition: ', 'UNKNOWN'))

        print("{: <15}".format('Libraries:'))
         
        for l in ip.LibrariesUsed:
            regex = r"^(.*?)\n+{\s*" + re.escape(l) + r"\W"
            found = re.search(regex, LibList, re.M|re.I)

            if found:
                path = found.group(1)
                print("{: <15} {: <30} {: <10} {: <250}".format('', l, '', path))

                if LibraryInstanceDict.has_key(l):
                    LibraryInstance = LibraryInstanceDict.get(l)
                    if path != LibraryInstance:
                        print("{: <15} {: <30} {: <10} {: <100}".format('', '', '', '(Error: different instance of the library found)'))
                else:
                    LibraryInstanceDict[l] = path
                    LibraryInstanceList.append(l);

            else:
                print("{: <15} {: <30} {: <10} {: <100}".format('', l, '', 'Error: library not found in libary list'))

        print("{: <15}".format('END\n'))


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
    parser = OptionParser()
    #Output debug log
    parser.add_option("-l", dest="OutputLog", help="Create an output log file: ie -l out.txt", default=None)
    parser.add_option("--pp", dest="Pkgs", help="Comma-separated list of submodule directories", default=None)
    parser.add_option("--inf", dest="InfPath", help="Module/INF file path", default=None)
    parser.add_option("--br", dest="BuildReportPath", help="Build Report file path", default=None)
    #parser.add_option("--LibTreeFile", dest="LibTreeFile", help="File created with list of libraries used by the module/INF", default=None)

    #Turn on dubug level logging
    parser.add_option("--debug", action="store_true", dest="debug", help="turn on debug logging level for file log",  default=False)

    (options, args) = parser.parse_args()

    #setup file based logging if outputReport specified
    if(options.OutputLog):
        if(len(options.OutputLog) < 2):
            logging.critical("the output log file parameter is invalid")
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

    Pkgs = options.Pkgs.split(',')

    f = open(options.BuildReportPath, "r")
    BuildReport = f.read()
    f.close()

    InfPath = re.sub(r"/", r"\\", options.InfPath)
    regex = r"Module INF Path:\s+" + re.escape(InfPath) + r"\n"
    regex = regex + r".*?"
    regex = regex + r"^Library\n"
    regex = regex + r"------------------------------------------------------------------------------------------------------------------------\n"
    regex = regex + r"(.*?)"
    regex = regex + r"<---------------------------------------------------------------------------------------------------------------------->"

    found = re.search(regex, BuildReport, re.M|re.I|re.DOTALL)
    if found:
        LibList = found.group(1)
        LibraryInstanceDict = {}
        LibraryInstanceList = []
        found = re.search(r'([^\\]+\.inf)$', InfPath, re.M|re.I)

        if found:
            module = found.group(1)
            LibraryInstanceDict[module] = InfPath
            LibraryInstanceList.append(module)
            DisplayLibraryTree()
        else:
            logging.critical("Module name not found")
    else:
        logging.critical("Library list for the given module not found in Build Report")

    #end logging
    logging.shutdown()
    sys.exit(0)
