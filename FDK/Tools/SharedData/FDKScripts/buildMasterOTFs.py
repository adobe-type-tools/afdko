from __future__ import print_function, division, absolute_import

__copyright__ = """Copyright 2016 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
"""

__usage__ = """
buildMasterOTFs.py  1.1 Nov 2 2016
Build master source OpenType/CFF fonts from a Superpolator design space file and the UFO master source fonts.

python buildMasterOTFs.py -h
python buildMasterOTFs.py -u
python buildMasterOTFs.py  <path to design space file>
"""

__help__  = __usage__  + """
The script makes a number of assumptions.
1) all the master source fonts are blend compatible in all their data.
2) For each source master font, all the files needed to build an OT font with makeotf are in the same directory as the UFO font, and have the default file names expected by makeotf. There must be a FontMenuNameDB file, with an entry which matches the PostScript name of the default font.
3) The Adobe AFDKO is installed: the script calls 'tx' and 'makeotf'.
4) <source> element for the default font in the design space file has
the child element <info copy="1">; this is used to identify which master is
the default font.

Any Python interpeter may be used, not just the AFDKO Python. If the AFDKO is installed, teh command 'buildMasterOTFs' will call the script with the AFDKOPython.

The script will build a master source OpenType/CFF font for each master source UFO font, using the same file name, but with the extension '.otf'.
"""

import os
import re
import sys
import subprocess

try:
    import xml.etree.cElementTree as ET
except ImportError:
    import xml.etree.ElementTree as ET

XML = ET.XML
XMLElement = ET.Element
xmlToString = ET.tostring


class DSFont(object):
	def __init__(self, filePath):
		self.filePath = filePath
		self.fileName = os.path.basename(filePath)
		self.buildDir = os.path.dirname(filePath)
		self.locations = []
		
	def __str__(self):
		txtList = ["%s %s" % (self.filename , self.name )]
		for loc in self.locations:
			txtList.append(str(loc))
		return " ".join(txtList)
		
	def locationKey(self):
		txtList = []
		for loc in self.locations:
			txtList.append(str(loc))
		keyStr = " ".join(txtList)
		return keyStr
		
class DSLocation(object):
	def __init__(self, name, coord):
		self.name = name
		self.coord = coord
		
	def __str__(self):
		return "%s %s" % (self.name , self.coord )

def runShellCmd(cmd):
	try:
		p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT).stdout
		log = p.read()
		return log
	except :
		msg = "Error executing command '%s'. %s" % (cmd, traceback.print_exc())
		print(msg)
		return ""

def compatibilizePaths(otfPath):
	tempPathCFF = otfPath + ".temp.cff"
	command="tx  -cff +b -std -no_opt \"%s\" \"%s\" 2>&1" % (otfPath, tempPathCFF)
	report = runShellCmd(command)
	if "fatal" in str(report):
		print(report)
		sys.exit(1)
	command="sfntedit -a \"CFF \"=\"%s\" \"%s\" 2>&1" % (tempPathCFF, otfPath)
	report = runShellCmd(command)
	if "FATAL" in str(report):
		print(report)
		sys.exit(1)
	os.remove(tempPathCFF)	

def main(args=None):
	if args is None:
		args = sys.argv[1:]
	
	if '-u' in args:
		print(__usage__)
		return
	if '-h' in args:
		print(__help__)
		return

	(dsPath,) = args
	
	dsET = ET.parse(dsPath)
	dsXML = dsET.getroot()
	
	# Identify masters
	masterList = []
	
	dsDirPath = os.path.dirname(dsPath)
	err = 0
	masterList = []
	print("Collecting master font paths...")
	for sources in dsXML.findall('sources'):
		for source in sources.findall('source'):
			ufoPath = os.path.join(dsDirPath, source.attrib["filename"])
			dsFont = DSFont(ufoPath)
			masterList.append(dsFont)
			for location in source.findall('location'):
				for dimension in location.findall('dimension'):
					name = dimension.attrib["name"]
					value = eval(dimension.attrib["xvalue"] )
					dsFont.locations.append(DSLocation(name, value))
			
	haveData = True
	for masterDSFont in masterList:
		if not os.path.exists(masterDSFont.filePath):
			haveData = False
			print("Missing master font '%s'." % (masterDSFont.filePath))
			

	if not haveData:
		print("Quitting.")
		return
		
	print("Building local otf's for master font paths...")
	tempT1 = "temp.pfa"
	curDir = os.getcwd()
	for masterDSFont in masterList:
		ufoName = os.path.basename(masterDSFont.filePath)
		otfName = os.path.splitext(ufoName)[0] + ".otf"
		os.chdir(masterDSFont.buildDir)
		cmd = "tx -t1 \"%s\" \"%s\" " % (masterDSFont.fileName, tempT1)
		log = runShellCmd(cmd)
		cmd = "makeotf -f \"%s\" -o \"%s\"" % (tempT1, otfName)
		log = runShellCmd(cmd)
		if "FATAL" in log:
			# If there is a FontMenuNameDB file, then not finding an entry
			# which matches the PS name is a fatal error. For this case, we
			# just try again, but without the FontMenuNameDB.
			if "I can't find a Family name" in log:
				cmd = "makeotf -f \"%s\" -o \"%s\" -mf None" % (tempT1, otfName)
				log = runShellCmd(cmd)
			
		if not "Built" in str(log):
			print("Error building OTF font for", masterDSFont.filePath, log)
		else:
			print("Built OTF font for", masterDSFont.filePath)
			compatibilizePaths(otfName)
		os.remove(tempT1)
		os.chdir(curDir)

if __name__ == "__main__":
	main()
