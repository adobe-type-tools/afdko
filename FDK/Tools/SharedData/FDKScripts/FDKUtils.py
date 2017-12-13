"""
FDKUtils.py v 1.2 April 13 6 2016
 A module of functions that are needed by several of the FDK scripts.
"""

__copyright__ = """Copyright 2016 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
"""

import os
import sys
import subprocess
import traceback
import platform
curSystem = platform.system()

AdobeCMAPS = "Adobe Cmaps"
AdobeCharsets = "CID charsets"

class FDKEnvError(KeyError):
	pass

def findFDKDirs():
	fdkScriptsDir = None
	fdkToolsDir = None
	""" Look up the file path to find the "Tools" directory;
	then add the os.name for the executables, and .'FDKScripts' for the scripts.
	"""
	dir = os.path.dirname(__file__)

	while dir:
		if os.path.basename(dir) == "Tools":
			fdkScriptsDir = os.path.join(dir, "SharedData", "FDKScripts")
			if curSystem == "Darwin":
				fdkToolsDir = os.path.join(dir, "osx")
			elif curSystem == "Windows":
				fdkToolsDir = os.path.join(dir, "win")
			elif curSystem == "Linux":
				fdkToolsDir = os.path.join(dir, "linux")
			else:
				print "Fatal error: un-supported platform %s %s." % (os.name, sys.platform)
				raise FDKEnvError

			if not (os.path.exists(fdkScriptsDir) and os.path.exists(fdkToolsDir)):
				print "Fatal error: could not find  the FDK scripts dir %s and the tools directory %s." % (fdkScriptsDir, fdkToolsDir)
				raise FDKEnvError
 
			# the FDK.py bootstrap program already adds fdkScriptsDir to the  sys.path;
			# this is useful only when running the calling script directly using an external Python.
			if not fdkScriptsDir in sys.path:
				sys.path.append(fdkScriptsDir)
			fdkSharedDataDir = os.path.join(dir, "SharedData")
			break
		dir = os.path.dirname(dir)
	return fdkToolsDir,fdkSharedDataDir


def findFDKFile(fdkDir, fileName):
	path = os.path.join(fdkDir, fileName)
	if os.path.exists(path):
            return path
        p1 = path + ".exe"
 	if os.path.exists(p1):
            return p1
        p2 = path + ".cmd"
	if os.path.exists(p2):
            return p2
	if fileName not in ["addGlobalColor"]:
		print "Fatal error: could not find '%s or %s or %s'." % (path,p1,p2)
	raise FDKEnvError

def runShellCmd(cmd):
	try:
		p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT).stdout
		log = p.read()
		return log
	except :
		msg = "Error executing command '%s'. %s" % (cmd, traceback.print_exc())
		print(msg)
		return ""

def runShellCmdLogging(cmd):
	try:
		logList = []
		proc = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
		while 1:
			output = proc.stdout.readline()
			if output:
				print output,
				logList.append(output)
			if proc.poll() != None:
				output = proc.stdout.readline()
				if output:
					print output,
					logList.append(output)
				break
		log = "".join(logList)
	except:
		msg = "Error executing command '%s'. %s" % (cmd, traceback.print_exc())
		print(msg)
		return 1
	return 0

def clean_afdko():
	runShellCmd("pip uninstall afdko -y")
	bin_fileList = [
			  "autohint",
			  "buildCFF2VF",
			  "buildMasterOTFs",
			  "compareFamily",
			  "checkOutlinesUFO",
			  "copyCFFCharstrings",
			  "kernCheck",
			  "makeotf",
			  "makeInstancesUFO",
			  "otc2otf",
			  "otf2otc",
			  "stemHist",
			  "ttxn",
			  "charplot",
			  "digiplot",
			  "fontplot",
			  "fontplot2",
			  "fontsetplot",
			  "hintplot",
			  "waterfallplot",
			  "check_afdko",
			  "clean_afdko",
              "autohintexe",
              "makeotfexe",
              "mergeFonts",
              "rotateFont",
              "sfntdiff",
              "sfntedit",
              "spot",
              "tx",
              "type1",
	]
	log = runShellCmd("which autohintexe")
	log = log.strip()
	if not log:
		print("Did not find FDK tools in the Python bin directory:")
	else:
		basepath = os.path.dirname(log)
		for fileName in bin_fileList:
			binPath = os.path.join(basepath, fileName)
			try:
				os.remove(binPath)
				print("Deleted", binPath)
			except:
				pass
	basepath = None
	foundFDK = False
	try:
		import FDK
		foundFDK = True
	except:
		print("Cannot import FDK - must not be installed")
	if foundFDK:
		import shutil
		basepath = os.path.dirname(FDK.__file__)
		try:
			shutil.rmtree(basepath)
			print("Deleted", basepath)
		except:
			pass
		
	return 
	
def check_afkdo():
	return
	
if __name__ == "__main__":
	clean_afdko()
	
	