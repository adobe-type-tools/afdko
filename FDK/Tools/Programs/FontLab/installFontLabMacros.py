#!/bin/env python
#installFontLabMacros.py
__copyright__ =  """
Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
"""

__doc__ = """
v 1.1 Feb 09 2009

Add the path to FDK/Tools/SharedData/FDScripts to the site-packages path
Copy all the FontLab-related scripts from the Adobe FDK.

Copies each directory under <FDKRoot>/Tools/FontLab
to the appropriate place under FontLab/macros.
"""

import sys
import re
import os
import shutil
import traceback

class InstallError(IOError):
	pass

import stat
kPermissions = stat.S_IWRITE | stat.S_IREAD | stat.S_IRGRP | stat.S_IWGRP | stat.S_IRUSR | stat.S_IWUSR

def copyFile(srcPath, dstPath):
		if os.path.isfile(srcPath):
			try:
				shutil.copy(srcPath, dstPath)
				shutil.copystat(srcPath, dstPath)
				os.chmod(dstPath, kPermissions)
			except IOError:
				print "Failed to copy file %s to %s." % (srcPath, dstPath)
				print traceback.format_exception_only(sys.exc_type, sys.exc_value)[-1]
				print "Quitting - not all files were copied."
				raise InstallError
			print "Copied: %s to dir %s" % (srcPath, dstPath)
			

def copyDir(srcDirPath, destDirPath):
	if not os.path.exists(destDirPath):
		os.mkdir(destDirPath)
		
	fileList = os.listdir(srcDirPath)
	for fileName in fileList:
		srcPath = os.path.join(srcDirPath, fileName)
		if os.path.isfile(srcPath):
			name,ext = os.path.splitext(fileName)
			if not ext in [".so", ".pyd", ".py"]:
				continue
			dstPath = os.path.join(destDirPath, fileName)
			copyFile(srcPath, dstPath)
		elif os.path.isdir(srcPath):
			newDestDirPath = os.path.join(destDirPath, fileName)
			copyDir(srcPath, newDestDirPath)
		else:
			print "Say what? this file is neither a dir nor a regular file:", srcPath
		
def run():
	# define where we will copy things
	if len(sys.argv) != 2:
		print "You must provide the path to the FontLab Macros directory where the scripts should go."
		print " An example for Mac OSX:"
		print "python installFontLabMacros.py /Users/<your Mac user name>/Library/Application\ Support/FontLab/Studio\ 5/Macros"
		return
	destBasePath = sys.argv[1]
	if not os.path.isdir(destBasePath):
		print "The path you supplied does not exist or is not a directory. Try again."
		return
	
	# Find the path to the <FDKRoot>/Tools/FontLab/
	fdkToolsDir = os.path.dirname(os.path.abspath(__file__)) # Tools/FontLab
	fdkToolsDir = os.path.dirname(fdkToolsDir) # Tools/
	

	srcBasePath = os.path.join(fdkToolsDir, "FontLab", "Macros")
	if not os.path.exists(srcBasePath):
		print "Error. Quitting. The FDK path to the FontLab scripts does not exist %s ." % srcBasePath
		print "Please run the FDK script FinishInstall.py, then restart FontLab."
		return
		
	# copy all files from the other directories at this level to the directories of
	# the same name under FontLab
	dirList = os.listdir(srcBasePath)
	for dirName in dirList:
		srcDirPath = os.path.join(srcBasePath, dirName)
		if not os.path.isdir(srcDirPath):
			continue
		destDirPath = os.path.join(destBasePath, dirName)
		copyDir(srcDirPath, destDirPath)

	FDKFontLabModuleList = ["plistlib.py",
							"fontPDF.py",
							"otfPDF.py",
							"pdfdoc.py",
							"pdfgen.py",
							"pdfgeom.py",
							"pdfmetrics.py",
							"pdfutils.py",
							"ttfPDF.py",
							"Py23AC.pyd",
							"Py24AC.pyd",
							"Py23focusdll.pyd",
							"Py24focusdll.pyd",
							]
	fdkOtherScriptsDir = os.path.join(fdkToolsDir, "SharedData", "FDKScripts")
	for fileName in FDKFontLabModuleList:
		srcPath = os.path.join(fdkOtherScriptsDir, fileName)
		dstPath = os.path.join(destBasePath, "System", "Modules", fileName)
		copyFile(srcPath, dstPath)
		

if __name__ == "__main__":
	try:
		run()
	except InstallError:
		pass


