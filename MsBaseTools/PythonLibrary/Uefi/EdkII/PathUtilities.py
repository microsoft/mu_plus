import os
import logging

#
# Class to help convert from absolute path to EDK2 build path
# using workspace and packagepath variables
#
class Edk2Path(object):
    def __init__(self, ws, packagepathlist):
        self.WorkspacePath = ws
        self.PackagePathList = packagepathlist


    def GetEdk2RelativePathFromAbsolutePath(self, abspath):
        relpath = None
        found = False
        for a in self.PackagePathList:
            stripped = abspath.lower().partition(a.lower())[2]
            if stripped:  
                #found our path...now lets correct for case
                relpath = abspath[len(a):]
                found = True
                logging.debug("Successfully converted AbsPath to Edk2Relative Path using PackagePath")
                logging.debug("AbsolutePath: %s found in PackagePath: %s" % (abspath, a))
                break

        if(not found):
            #try to strip the workspace
            stripped = abspath.lower().partition(self.WorkspacePath.lower())[2]
            if stripped:  
                #found our path...now lets correct for case
                relpath = abspath[len(self.WorkspacePath):]
                found = True
                logging.debug("Successfully converted AbsPath to Edk2Relative Path using WorkspacePath")
                logging.debug("AbsolutePath: %s found in Workspace: %s" % (abspath, self.WorkspacePath))
            
        if(found):
            return relpath.lstrip(os.sep)

        #didn't find the path for conversion. 
        logging.error("Failed to convert AbsPath to Edk2Relative Path")
        logging.error("AbsolutePath: %s" % abspath)
        return None
    
    def GetAbsolutePathOnThisSytemFromEdk2RelativePath(self, relpath):
        abspath = os.path.join(self.WorkspacePath, relpath)
        if os.path.exists(abspath):
            return abspath

        for a in self.PackagePathList:
            abspath = os.path.join(a, relpath)
            if(os.path.exists(abspath)):
                return abspath
        logging.error("Failed to convert Edk2Relative Path to an Absolute Path on this system.")
        logging.error("Relative Path: %s" % relpath)
        
        return None

    def GetContainingPackage(self, InputPath):
        for x in reversed(InputPath.split(os.path.sep)):
            if x.lower().endswith("pkg"):
                return x
        return None


        
