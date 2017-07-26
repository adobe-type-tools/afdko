from __future__ import print_function, division, absolute_import

__copyright__ = """Copyright 2017 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
"""

__usage__ = """
buildMasterOTFs.py  1.5 April 3 2017
Build master source OpenType/CFF fonts from a Superpolator design space file and the UFO master source fonts.

python buildMasterOTFs.py -h
python buildMasterOTFs.py -u
python buildMasterOTFs.py  <path to design space file>
"""

__help__  = __usage__  + """
The script makes a number of assumptions.
1) all the master source fonts are blend compatible in all their data.
2) Each master source font is in a separate directory from the other master
source fonts, and each of these directories provides the default meta data files 
expected by the 'makeotf' command:
- features
- font.ufo
- FontMenuNameDB within the directory or no more than three parent directories up.
- GlyphAliasAndOrderDB within the directory or no more than three parent directories up.
3) The Adobe AFDKO is installed: the script calls 'tx' and 'makeotf'.
4) <source> element for the default font in the design space file has
the child element <info copy="1">; this is used to identify which master is
the default font.

Any Python interpeter may be used, not just the AFDKO Python. If the AFDKO is installed, the command 'buildMasterOTFs' will call the script with the AFDKOPython.

The script will build a master source OpenType/CFF font for each master source UFO font, using the same file name, but with the extension '.otf'.
"""

import os
import re
import sys
import subprocess
import defcon
import copy
from mutatorMath.ufo import build as mutatorMathBuild

try:
    import xml.etree.cElementTree as ET
except ImportError:
    import xml.etree.ElementTree as ET

XML = ET.XML
XMLElement = ET.Element
xmlToString = ET.tostring

kTempUFOExt = ".temp.ufo"

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

def buldTempDesignSpace(dsPath):
	rootET = ET.parse(dsPath)
	masterETs = []
	ds = rootET.getroot()
	sourceET = ds.find('sources')
	masterList = sourceET.findall('source')
	
	# Delete the existing instances
	instanceET = ds.find('instances')
	instances = instanceET.findall("instance")
	for et in instances:
		instanceET.remove(et)
	# Make copies of the masters,, edit them, and add as instances.
	master_paths = []
	for master in masterList:
		instance = copy.deepcopy(master)
		instanceET.append(instance)
		masterPath = master.attrib['filename']
		masterPath = os.path.splitext(masterPath)[0] + kTempUFOExt
		master_paths.append(masterPath)
		instance.attrib['filename'] = masterPath
		instance.attrib['familyname'] = master.attrib['name']
		instance.attrib['postscriptfontname'] = master.attrib['name']
		del instance.attrib['name'] 
		for tag in ['lib', 'info', 'glyph', 'groups']:
			tagList = instance.findall(tag)
			for tagET in tagList:
				instance.remove(tagET)
	tempDSPath = dsPath + ".temp"
	fp = open(tempDSPath, "wt")
	fp.write(xmlToString(ds))
	fp.close()
	return tempDSPath, master_paths

def testGlyphSetsCompatible(dsPath):
	# check if the masters all have the same glyph set and order.
	glyphSetsDiffer = False
	dsDirPath = os.path.dirname(dsPath)
	rootET = ET.parse(dsPath)
	master_paths = []
	ds = rootET.getroot()
	sourceET = ds.find('sources')
	masterList = sourceET.findall('source')
	for et in masterList:
		master_paths.append(et.attrib['filename'])
	master_path = os.path.join(dsDirPath, master_paths[0])
	baseGO = defcon.Font(master_path).glyphOrder
	for master_path in master_paths[1:]:
		master_path = os.path.join(dsDirPath, master_path)
		if baseGO != defcon.Font(master_path).glyphOrder:
			glyphSetsDiffer	= True
			break
	return glyphSetsDiffer, master_paths

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
	
	dsDirPath = os.path.dirname(dsPath)
	glyphSetsDiffer, master_paths = testGlyphSetsCompatible(dsPath)
	
	if glyphSetsDiffer:
		version = 2
		allowDecimalCoords = False
		tempDSPath, master_paths = buldTempDesignSpace(dsPath)
		mutatorMathBuild(documentPath=tempDSPath, outputUFOFormatVersion=version, roundGeometry=(not allowDecimalCoords))		
		
	print("Building local otf's for master font paths...")
	tempT1 = "temp.pfa"
	curDir = os.getcwd()
	dsDir = os.path.dirname(dsPath)
	for master_path in master_paths:
		master_path = os.path.join(dsDir, master_path)
		masterDir = os.path.dirname(master_path)
		ufoName = os.path.basename(master_path)
		if glyphSetsDiffer:
			otfName = ufoName[:-len(kTempUFOExt)]
		else:
			otfName = os.path.splitext(ufoName)[0]
		otfName = otfName + ".otf"	
		os.chdir(masterDir)
		cmd = "makeotf -f \"%s\" -o \"%s\" -r -nS 2>&1" % (ufoName, otfName)
		log = runShellCmd(cmd)
		if ("FATAL" in log) or ("Failed to build" in log):
			# If there is a FontMenuNameDB file, then not finding an entry
			# which matches the PS name is a fatal error. For this case, we
			# just try again, but without the FontMenuNameDB.
			if ("I can't find a Family name" in log) or ("not in Font Menu Name database" in log):
				cmd = "makeotf -f \"%s\" -o \"%s\" -mf None -r -nS 2>&1" % (tempT1, otfName)
				log = runShellCmd(cmd)
				if "FATAL" in log:
					print(log)
				else:
					print("Building '%s' without FontMenuNameDB entry." % (master_path))
			
		if not "Built" in str(log):
			print("Error building OTF font for", master_path, log)
			print("makeotf cmd was '%s' in %s." % (cmd, masterDir))
		else:
			print("Built OTF font for", master_path)
			compatibilizePaths(otfName)
			if os.path.exists(tempT1):
				os.remove(tempT1)
			if os.path.exists(master_path) and glyphSetsDiffer:
				shutil.removetree(master_path)
		os.chdir(curDir)
		
	if os.path.exists(tempDSPath) and glyphSetsDiffer:
		os.remove(tempDSPath)

if __name__ == "__main__":
	main()
