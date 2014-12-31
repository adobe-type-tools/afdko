# Support module for FontLab scripts.
__copyright__ =  """
Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0.
"""
__doc__ = """
AdobeFontLabUtils v1.5 Feb 09 2009.
Support module for FontLab scripts.
Defines commonly used functions and globals.
"""

import sys
import os
import string
import re
import time
import plistlib
import StringIO
from FL import *

kGlyphOrderAndAliasDBName = "GlyphOrderAndAliasDB"
kSharedDataName = "SharedData"


########################################################
# Routines for managing a directory tree of font files, 
# and for exporting/importing ata between the font files
# and a master dictionary file
########################################################

def setFDKToolsPath(toolName):
	""" On Mac, add std FDK path to sys.environ PATH.
	On all, check if tool is available.
	"""
	toolPath = 0
	if sys.platform == "darwin":
		paths = os.environ["PATH"]
		if "FDK/Tools/osx" not in paths:
			home = os.environ["HOME"]
			fdkPath = ":%s/bin/FDK/Tools/osx" % (home)
			os.environ["PATH"] = paths + fdkPath
	
	if os.name == "nt":
		p = os.popen("for %%i in (%s) do @echo. %%~$PATH:i" % (toolName))
		log = p.read()
		p.close()
		log = log.strip()
		if log:
			toolPath = log	
	else:
		p = os.popen("which %s" % (toolName))
		log = p.read()
		p.close()
		log = log.strip()
		if log:
			toolPath = log	
	
	if not toolPath:
		print """
The script cannot run the command-line program '%s'. Please make sure the AFDKO is installed, and the system environment variable PATH
contains the path the to FDK sub-directory containing '%s'.""" % (toolName, toolName)

	return toolPath # get reid of new-line
	
def checkControlKeyPress():
	notPressed = 1
	if os.name == "nt":
		try:
			import win32api
			import win32con
			keyState = win32api.GetAsyncKeyState(win32con.VK_CONTROL)
			if keyState < 0:
				notPressed = 0
		except ImportError:
			print "Note: to be able to set options for this script, you must install"
			print "win32all Python module from Mark Hammond. This can be found at:"
			print "    http://www.python.net/crew/mhammond/win32/Downloads.html"
			print "or http://sourceforge.net/, and search for 'Python Windows Extensions."
	else:
		import Carbon.Evt
		import Carbon.Events
		modifiers = Carbon.Evt.GetCurrentKeyModifiers()
		if modifiers & Carbon.Events.controlKey:
			notPressed = 0
	return notPressed

def checkShiftKeyPress():
	notPressed = 1
	if os.name == "nt":
		try:
			import win32api
			import win32con
			keyState = win32api.GetAsyncKeyState(win32con.VK_SHIFT)
			if keyState < 0:
				notPressed = 0
		except ImportError:
			print "Note: to be able to set options for this script, you must install"
			print "win32all Python module from Mark Hammond. This can be found at:"
			print "    http://www.python.net/crew/mhammond/win32/Downloads.html"
			print "or http://sourceforge.net/, and search for 'Python Windows Extensions."
	else:
		import Carbon.Evt
		import Carbon.Events
		modifiers = Carbon.Evt.GetCurrentKeyModifiers()
		if modifiers & Carbon.Events.shiftKey:
			notPressed = 0
	return notPressed
	
def checkAltKeyPress():
	notPressed = 1
	if os.name == "nt":
		try:
			import win32api
			import win32con
			keyState = win32api.GetAsyncKeyState(win32con.VK_SHIFT)
			if keyState < 0:
				notPressed = 0
		except ImportError:
			print "Note: to be able to set options for this script, you must install"
			print "win32all Python module from Mark Hammond. This can be found at:"
			print "    http://www.python.net/crew/mhammond/win32/Downloads.html"
			print "or http://sourceforge.net/, and search for 'Python Windows Extensions."
	else:
		import Carbon.Evt
		import Carbon.Events
		modifiers = Carbon.Evt.GetCurrentKeyModifiers()
		if modifiers & Carbon.Events.optionKey:
			notPressed = 0
	return notPressed
	
def GetSharedDataPath():
	sdPath = ""
	for path in sys.path:
                if not re.search(r"FDK/Tools", path):
                    continue
                m = re.search(kSharedDataName, path)
                if not m:
                    continue
		sdPath = path[:m.end()]
			
	if not sdPath:
		print "Error. The path to ",kSharedDataName," is not in the sys.path list."
	elif not os.path.exists(sdPath):
		print "Error.", sdPath,"does not exist."
		sdPath = ""
		
	return sdPath


# fontDirPath is an absolute path to the font dir, supplied by FontLab
# fontPSName is used to get the top family directory from the font library DB file.
# so as to look back up the family tree for the GOASDB.
def GetGOADBPath(fontDirPath, fontPSName):
        goadbPath = ""
        dirPath = fontDirPath
	trys = 3 # look first in the font's dir, then up to two levels up.
        while trys:
            goadbPath = os.path.join(dirPath, kGlyphOrderAndAliasDBName)
            if (goadbPath and os.path.exists(goadbPath)):
                break
            dirPath = os.path.dirname(dirPath)
            trys -= 1
			
	if (goadbPath and os.path.exists(goadbPath)):
            return goadbPath
        
        # default to the global FDK GOADB.
        goadbPath = ""
        sharedDataDir = GetSharedDataPath()
        if sharedDataDir:
                goadbPath = os.path.join(sharedDataDir, kGlyphOrderAndAliasDBName )
	if (goadbPath and os.path.exists(goadbPath)):
            return goadbPath

        print "Error. Could not find", kGlyphOrderAndAliasDBName,", even in FDK Shared Data Dir." 		
        goadbPath = ""
		
	return goadbPath

def SplitGOADBEntries(line):
	global goadbIndex

	entry = string.split(line)
	if (len(entry) < 2) or (len(entry) > 3):
		print "Error in GOADB: bad entry - too many or two few columns <" + line + ">"
		entry = None
	if len(entry) == 3:
		if entry[2][0] != "u":
			print "Error in GOADB: 3rd column must be a uni or u Unicode name <" + line + ">"
			entry = None
	if len(entry) == 2:
		entry.append("")
	
	# Add GOADB index value
	if entry:
		entry.append(goadbIndex)
		goadbIndex = goadbIndex + 1
	return entry

 
########################################################
# Misc utilities
########################################################

def RemoveComment(line):
	try:
		index = string.index(line, "#")
		line = line[:index]
	except:
		pass
	return line

#return list of lines with comments and blank lines removed.
def CleanLines(lineList):
	lineList = map(lambda line: RemoveComment(line) , lineList)
	lineList = filter(lambda line: string.strip(line), lineList)
	return lineList


#split out lines from a stream of file data.
def SplitLines(data):
	lineList = re.findall(r"([^\r\n]+)[\r\n]", data)
	return lineList


def LoadGOADB(filePath):
	""" Read  a glyph alias file for makeOTF into a dict."""
	global goadbIndex
	
	finalNameDict = {}
	productionNameDict = {}
	goadbIndex = 0
	
	gfile = open(filePath,"rb")
	data = gfile.read()
	gfile.close()

	glyphEntryList = SplitLines(data)
	glyphEntryList = CleanLines(glyphEntryList)
	glyphEntryList = map(SplitGOADBEntries, glyphEntryList)
	glyphEntryList = filter(lambda entry: entry, glyphEntryList) # drop out any entry == None
	for entry in glyphEntryList:
		finalNameDict[entry[0]] = [ entry[1], entry[2], entry[3] ]
		if productionNameDict.has_key(entry[1]):
			print "Error in GOADB: more than one final name for a production name!"
			print "\tfinal name 1:", productionNameDict[entry[1]], "Final name 2:", entry[0], "Production name:", entry[1]
			print "\tUsing Final name 2."
		productionNameDict[entry[1]] = [ entry[0], entry[2], entry[3] ]
		
	return finalNameDict, productionNameDict


kDefaultReportExtension = "log"
kDefaultLogSubdirectory = "logs"
kDefaultVersionDigits = 3
kWriteBoth = 3
kWriteStdOut = 1
kWriteFile = 2

class Reporter:
	""" Logging class to let me echo output to both/either screen and a log file.
		Makes log files with same base name as font file, and special extension.
		Default extension is supplied, can be overridden.
		Trys to put log file in subdirectory under font file home directory."""

	def __init__(self, fileOrPath, extension = kDefaultReportExtension):
		self.file = None
		self.fileName  = None
		self.state = kWriteBoth
		if type(fileOrPath) == type(" "):

			# try to find or make log directory for report file.
			dir,name = os.path.split(fileOrPath)
			logDir = os.path.join(dir, kDefaultLogSubdirectory)
			if not os.path.exists(logDir):
				try:
					os.mkdir(logDir)
				except IOError:
					print "Failed to make log file subdir:", logDir
					return

			if os.path.exists(logDir):
				fileOrPath = os.path.join(logDir, name)

			basePath, fileExt = os.path.splitext(fileOrPath)
			self.fileName = self.makeSafeReportName(basePath, extension)
			try:
				self.file = open(self.fileName, "wt")
			except IOError:
				print "Failed to open file", self.fileName
				return
		else:
			self.fileName = None
			self.file = fileOrPath
		return

	def makeSafeReportName(self, baseFilePath, extension):
		global kDefaultVersionDigits

		""" make a report file name with a number 1 greater than any
		existing report file name with the same extension. We know the
		baseFilePath exists, as it comes from an open font file. We will
		not worry about 32 char name limits -> Mac OS X and Windows 2000
		only.
		"""
		n = 1
		dir, file = os.path.split(baseFilePath)
		numPattern = re.compile(file + "." + extension + r"v0*(\d+)$")
		fileList = os.listdir(dir)
		for file in fileList:
			match = numPattern.match(file)
			if match:
				num = match.group(1)
				num = eval(num)
				if num >= n:
					n = num + 1

		if n > (10**kDefaultVersionDigits - 1):
			kDefaultVersionDigits = kDefaultVersionDigits +1
		filePath = baseFilePath + "." + extension + "v" + str(n).zfill(kDefaultVersionDigits)
		return filePath

	def write(*args):
		self = args[0]
		text = []
		for arg in args[1:]:
			try:
				text.append(str(arg))
			except:
				text.append(repr(arg))
		text = " ".join(text)

		if (self.state == kWriteBoth):
			print text
			if (self.file != sys.stdout):
				self.file.write(text + os.linesep)
		elif (self.state == kWriteFile):
				self.file.write(text + os.linesep)
		elif (self.state == kWriteStdOut):
			print text

	def set_state(self, state):
		self.state = state

	def close(self):
		if  self.file  and (self.file != sys.stdout):
			self.file.close()
		if self.fileName:
			print "Log saved to ", self.fileName

	def read(*args): # added to make this class look more like a file.
		pass

def getLatestReport(baseFilePath, extension=kDefaultReportExtension):
	""" FInd the latest report matching the path and extension.
	Assume that the highest number is the most recent.
	"""
	n = 1
	dir, file = os.path.split(baseFilePath)
	logsDir = os.path.join(dir, kDefaultLogSubdirectory)
	matchString = r"%s.%sv0*(\d+)$" % (file, extension)
	print matchString
	numPattern = re.compile(matchString)
	fileList = os.listdir(logsDir)
	print fileList
	latestFile = ""
	for file in fileList:
		match = numPattern.match(file)
		if match:
			num = match.group(1)
			num = eval(num)
			if num >= n:
				n = num
				latestFile = file
	if latestFile:
		filePath = os.path.join(logsDir, latestFile)
	else:
		filePath = ""

	return filePath
