from __future__ import print_function, division, absolute_import

import os
import shutil
import subprocess
import sys
import tempfile

import defcon
from mutatorMath.ufo.document import DesignSpaceDocumentReader

try:
    import xml.etree.cElementTree as ET
except ImportError:
    import xml.etree.ElementTree as ET

XMLElement = ET.Element
xmlToString = ET.tostring

kTempUFOExt = ".temp.ufo"
kTempDSExt = ".temp.designspace"
kFeaturesFile = "features.fea"

__usage__ = """
buildMasterOTFs.py  1.7 Aug 8 2017
Build master source OpenType/CFF fonts from a Superpolator design space file
and the UFO master source fonts.

python buildMasterOTFs.py -h
python buildMasterOTFs.py -u
python buildMasterOTFs.py  <path to design space file>
"""

__help__  = __usage__  + """
Options:
--mkot   Allows passing a comma-separated set of options to MakeOTF

The script makes a number of assumptions.
1) all the master source fonts are blend compatible in all their data.
2) Each master source font is in a separate directory from the other master
source fonts, and each of these directories provides the default meta data files
expected by the 'makeotf' command:
- features/features.fea
- font.ufo
- FontMenuNameDB within the directory or no more than 4 parent directories up.
- GlyphAliasAndOrderDB within the directory or no more than 4 parent directories up.
3) The Adobe AFDKO is installed: the script calls 'tx' and 'makeotf'.
4) <source> element for the default font in the design space file has
the child element <info copy="1">; this is used to identify which master is
the default font.

Any Python interpreter may be used, not just the AFDKO Python. If the AFDKO is
installed, the command 'buildMasterOTFs' will call the script with the AFDKOPython.

The script will build a master source OpenType/CFF font for each master source
UFO font, using the same file name, but with the extension '.otf'.

UFO masters with partial glyphsets are supported.
"""

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
	command="tx -cff +b -std -no_opt \"%s\" \"%s\" 2>&1" % (otfPath, tempPathCFF)
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

def _determine_which_masters_to_generate(ds_path):
	"""'ds_path' is the path to a designspace file.
	   Returns a list of integers representing the indexes
	   of the temp masters to generate.
	"""
	master_list = ET.parse(ds_path).getroot().find('sources').findall('source')

	# Make a set of the glyphsets of all the masters while collecting each
	# glyphset. Glyph order is ignored.
	all_gsets = set()
	each_gset = []
	for master in master_list:
		master_path = master.attrib['filename']
		ufo_path = os.path.join(os.path.dirname(ds_path), master_path)
		gset = set(defcon.Font(ufo_path).keys())
		all_gsets.update(gset)
		each_gset.append(gset)

	master_indexes = []
	for i, gset in enumerate(each_gset):
		if gset != all_gsets:
			master_indexes.append(i)

	return master_indexes


def buildTempDesignSpace(dsPath):
	"""'dsPath' is the path to the original designspace file.
	   Returns a modified designspace file (for generating temp UFO masters)
	   and a list of paths to the UFO masters (that will be used for generating
	   each master OTF).
	"""
	ds = ET.parse(dsPath).getroot()
	masterList = ds.find('sources').findall('source')

	# Delete the existing instances
	instances = ds.find('instances')
	ds.remove(instances)
	ds.append(XMLElement('instances'))
	instances = ds.find('instances')

	# Don't be wasteful, generate only the necessary temp masters
	temp_masters_to_generate = _determine_which_masters_to_generate(dsPath)

	# Populate the <instances> element with the information needed
	# for generating the temp master(s)
	master_paths = []
	for i, master in enumerate(masterList):
		masterPath = master.attrib['filename']
		if i not in temp_masters_to_generate:
			master_paths.append(masterPath)
			continue
		instance = XMLElement('instance')
		instance.append(master.find('location'))
		instance.append(XMLElement('kerning'))
		instance.append(XMLElement('info'))
		tempMasterPath = os.path.splitext(masterPath)[0] + kTempUFOExt
		master_paths.append(tempMasterPath)
		instance.attrib['filename'] = tempMasterPath
		ufo_path = os.path.join(os.path.dirname(dsPath), masterPath)
		ufo_info = defcon.Font(ufo_path).info
		instance.attrib['familyname'] = ufo_info.familyName
		instance.attrib['stylename'] = ufo_info.styleName
		instance.attrib['postscriptfontname'] = ufo_info.postscriptFontName
		instances.append(instance)
	tempDSPath = os.path.splitext(dsPath)[0] + kTempDSExt
	fp = open(tempDSPath, "wt")
	fp.write(xmlToString(ds))
	fp.close()
	return tempDSPath, master_paths

def testGlyphSetsCompatible(dsPath):
	# check if the masters all have the same set of glyphs
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
	baseGO = defcon.Font(master_path).keys()
	for master_path in master_paths[1:]:
		master_path = os.path.join(dsDirPath, master_path)
		if baseGO != defcon.Font(master_path).keys():
			glyphSetsDiffer	= True
			break
	return glyphSetsDiffer, master_paths

def parse_makeotf_options(mkot_opts):
	"""Converts a comma-separated string into a space-separated string"""
	return ' '.join(mkot_opts.split(','))

def main(args=None):
	if args is None:
		args = sys.argv[1:]
	if not args:
		args = ["-u"]

	mkot_options = ''

	if '-u' in args:
		print(__usage__)
		return
	if '-h' in args:
		print(__help__)
		return
	if '--mkot' in args:
		index = args.index('--mkot')
		args.pop(index)
		mkot_options = parse_makeotf_options(args.pop(index))

	(dsPath,) = args

	dsDirPath = os.path.dirname(dsPath)
	glyphSetsDiffer, master_paths = testGlyphSetsCompatible(dsPath)

	if glyphSetsDiffer:
		allowDecimalCoords = False
		logFile = tempfile.NamedTemporaryFile(delete=True).name
		tempDSPath, master_paths = buildTempDesignSpace(dsPath)
		print("Generating temp UFO master(s)...")
		ds_doc_reader = DesignSpaceDocumentReader(
			tempDSPath, ufoVersion=2, roundGeometry=(not allowDecimalCoords),
			verbose=False, logPath=logFile)
		ds_doc_reader.process(makeGlyphs=True, makeKerning=True, makeInfo=True)

	print("Building local otf's for master font paths...")
	curDir = os.getcwd()
	dsDir = os.path.dirname(dsPath)
	for master_path in master_paths:
		master_path = os.path.join(dsDir, master_path)
		masterDir = os.path.dirname(master_path)
		ufoName = os.path.basename(master_path)
		if ufoName.endswith(kTempUFOExt):
			otfName = ufoName[:-len(kTempUFOExt)]
			# copy the features.fea file from the original UFO master
			fea_file_path_from = os.path.join(master_path[:-len(kTempUFOExt)]+'.ufo', kFeaturesFile)
			fea_file_path_to = os.path.join(master_path, kFeaturesFile)
			if os.path.exists(fea_file_path_from):
				shutil.copyfile(fea_file_path_from, fea_file_path_to)
		else:
			otfName = os.path.splitext(ufoName)[0]
		otfName = otfName + ".otf"
		os.chdir(masterDir)
		cmd = "makeotf -f \"%s\" -o \"%s\" -r -nS %s 2>&1" % (ufoName, otfName, mkot_options)
		log = runShellCmd(cmd)
		if ("FATAL" in log) or ("Failed to build" in log):
			print(log)

		if not "Built" in str(log):
			print("Error building OTF font for", master_path, log)
			print("makeotf cmd was '%s' in %s." % (cmd, masterDir))
		else:
			print("Built OTF font for", master_path)
			compatibilizePaths(otfName)
			if ufoName.endswith(kTempUFOExt):
				shutil.rmtree(ufoName)
		os.chdir(curDir)

	if glyphSetsDiffer and os.path.exists(tempDSPath):
		os.remove(tempDSPath)

if __name__ == "__main__":
	main()
