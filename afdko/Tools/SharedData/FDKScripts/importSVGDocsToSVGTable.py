_help_ = """
importSVGDocsToSVGTable.py v 2.2 Oct 24 2014

python importSVGDocsToSVGTable.py <path to input OTF/TTF file>  <path(s) to SVG files OR to folder tree containing SVG files> 

Converts a set of SVG docs to a TTX SVG table definition, and then adds this table into the font file.
The script will replace any pre-existing SVG table.

Requires the <svg> element of every SVG file to have an 'id' parameter that follows the format 'glyphID' where ID is an integer number.
(If you're using Adobe Illustrator to create the SVG files, giving the name 'glyphID' to the top layer will fulfill the above requirement)

The integer ID values (of glyphID) should match the glyph indexes of the font.
"""

__copyright__ = """Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved."""

import os
import sys
import re
import io # io module is used for dealing with the line-ending differences that exist between platforms
from subprocess import Popen, PIPE
from xml.etree import ElementTree

kSVGDocTemplate = """\t\t<svgDoc endGlyphID="%s" startGlyphID="%s">
\t\t\t<![CDATA[%s]]>
\t\t</svgDoc>"""

kSVGTableTemplate = """<?xml version="1.0" encoding="utf-8"?>
<ttFont sfntVersion="%s">
\t<SVG>
%s
\t\t<colorPalettes></colorPalettes><!-- COLOR PALETTES IN SVG TABLE HAVE BEEN SUPERSEEDED BY CPAL TABLE -->
\t</SVG>
</ttFont>
"""

kSVGDefaultViewBox = """ viewBox="0 %s 1000 1000"\
""" # the placeholder is for the UPM value

kSVGFileNameSuffix = "-SVG"

debug = False


def writeNewFontFile(fontFilePath, ttxFilePath):
	folderPath, fontFileName = os.path.split(fontFilePath)
	fileNameNoExtension, fileExtension = os.path.splitext(fontFileName)
	newFontFilePath = os.path.join(folderPath, "%s%s%s" % (fileNameNoExtension, kSVGFileNameSuffix, fileExtension))
	
	# check if the XML is well formed, before trying to compile a new font with TTX
	try:
		ElementTree.parse(ttxFilePath)
	except ElementTree.ParseError:
		print "ERROR: The SVG's table XML code is malformed."
		if debug:
			print "\tCheck the file %s" % ttxFilePath
		return
	
	cmd = "ttx -o '%s' -m '%s' '%s'" % (newFontFilePath, fontFilePath, ttxFilePath)
	popen = Popen(cmd, shell=True, stdout=PIPE)
	popenout, popenerr = popen.communicate()
	if popenout and debug:
		print popenout
	

def writeTTXfile(fontFilePath, UPM, fontFormat, svgFilePathsList):
	if debug:
		period = ''
		debugSuffix = '-DEBUG'
	else:
		period = '.'
		debugSuffix = ''
	folderPath, fontFileName = os.path.split(fontFilePath)
	fileNameNoExtension, fileExtension = os.path.splitext(fontFileName)
	ttxFilePath = os.path.join(folderPath, "%s%s%s.ttx" % (period, fileNameNoExtension, debugSuffix))
	
	# the svgDoc entries in the SVG table need to be sorted,
	# so first make a dictionary with all the entries
	svgDocDict = {}
	for svgFilePath in svgFilePathsList:
		f = io.open(svgFilePath, "rt", newline=None)
		data = f.read()
		f.close()
		
		# add or offset the viewBox, to adjust for the different coordinate system of SVG (when compared to fonts' coordinate system)
		# https://developer.mozilla.org/en-US/docs/Web/SVG/Tutorial/Positions
		viewBox = re.search(r"<svg.+?(viewBox=[\"|\'][\d, ]+[\"|\']).+?>", data, re.DOTALL)
		if viewBox: # offset the second value (y-min) of the viewBox			
# 			print "BEFORE: ", viewBox.group(1)
			viewBoxValues = re.search(r"viewBox=[\"|\'](\d+)[, ](\d+)[, ](\d+)[, ](\d+)[\"|\']", viewBox.group(1))
			yMin = int(viewBoxValues.group(2))
			newViewBox = "viewBox=\"%s %s %s %s\"" % (viewBoxValues.group(1), yMin + UPM, viewBoxValues.group(3), viewBoxValues.group(4))
# 			print "AFTER:  ", newViewBox
			data = re.sub(viewBox.group(1), newViewBox, data)
		else: # add the viewBox parameter
			data = re.sub(r"<svg", r"<svg" + kSVGDefaultViewBox % UPM, data)
	
		gid = re.search(r"<svg.+?id=\"glyph(\d+)\".+?>", data, re.DOTALL).group(1)
		svgDoc = kSVGDocTemplate % (gid, gid, data.strip())
		svgDocDict[int(gid)] = svgDoc
	
	svgDocIndexList = svgDocDict.keys()
	svgDocIndexList.sort()
	svgDocList = [svgDocDict[index] for index in svgDocIndexList]
	
	tableContents = os.linesep.join(svgDocList)
	tableTotal = kSVGTableTemplate % (fontFormat, tableContents)
	
	f = open(ttxFilePath, "wt")
	f.write(tableTotal)
	f.close()
	
	return ttxFilePath


def validateSVGfiles(svgFilePathsList):
	"""
	Light validation of SVG files.
	Checks that:
		- there is an <xml> header
		- there is an <svg> element
		- the 'id' parameter in the <svg> element follows the format 'glyphID' where ID is an integer number
		- there are no repeated glyphIDs
	"""
	validatedPaths = []
	glyphIDsFound = []
	
	for filePath in svgFilePathsList:
		# skip hidden files (filenames that start with period)
		fileName = os.path.basename(filePath)
		if fileName[0] == '.':
			continue
		
		# read file
		f = open(filePath, "rt")
		data = f.read()
		f.close()
		
		# find <xml> header
		xml = re.search(r"<\?xml.+?\?>", data)
		if not xml:
			print "WARNING: Could not find <xml> header in the file. Skiping %s" % (filePath)
			continue

		# find <svg> blob
		svg = re.search(r"<svg.+?>.+?</svg>", data, re.DOTALL)
		if not svg:
			print "WARNING: Could not find <svg> element in the file. Skiping %s" % (filePath)
			continue
		
		# check 'id' value
		id = re.search(r"<svg.+?id=\"glyph(\d+)\".+?>", data, re.DOTALL)
		if not id:
			print "WARNING: The 'id' parameter in the <svg> element does not match the format 'glyphID'. Skiping %s" % (filePath)
			continue
		
		gid = id.group(1)
		
		# look out for duplicate IDs
		if gid in glyphIDsFound:
			print "WARNING: The 'id' value 'glyph%s' is already being used by another SVG file. Skiping %s" % (gid, filePath)
			continue
		else:
			glyphIDsFound.append(gid)
		
		validatedPaths.append(filePath)
	
# 	print glyphIDsFound
# 	print validatedPaths
	return validatedPaths


def getFontUPM(fontFilePath):
	"""
	Use spot to get the UPM value from the head table of the font.
	If this fails we can assume that the file is not a font.
	"""
	cmd = "spot -t head '%s'" % fontFilePath
	popen = Popen(cmd, shell=True, stdout=PIPE)
	popenout, popenerr = popen.communicate()
	if popenout:
		output = popenout
	else:
		return
	
	headUPM = re.search("unitsPerEm\s*=(\d+)", output).group(1)
	return int(headUPM)


def getFontFormat(fontFilePath):
	# these lines were scavenged from fontTools
	f = open(fontFilePath, "rb")
	header = f.read(256)
	formatTag = header[:4]
	return repr(formatTag)[1:-1]


def getOptions():
	
	if len(sys.argv) < 3:
		print "ERROR: Not enough arguments."
		print _help_
		sys.exit(0)
	
	if ("-u" in sys.argv) or ("-h" in sys.argv):
		print _help_
		sys.exit(0)

	fontFilePath = sys.argv[1]
	svgFilesOrFolder = sys.argv[2:]

	if not os.path.isfile(fontFilePath):
		print "ERROR: The first argument must be a path to an OTF or TTF file."
		print sys.exit(0)

	UPM = getFontUPM(fontFilePath)
	
	if not UPM:
		print "ERROR: Invalid font file."
		print sys.exit(0)

	fontFormat = getFontFormat(fontFilePath)
	
	if os.path.isdir(svgFilesOrFolder[0]): # If the first path is a folder
		svgFilePathsList = []
		dir = svgFilesOrFolder[0]
		for dirName, subdirList, fileList in os.walk(dir): # Support nested folders
			for file in fileList:
				svgFilePathsList.append(os.path.join(dirName, file)) # Assemble the full paths, not just file names
	else:
		svgFilePathsList = []
		for file in svgFilesOrFolder[:]: # copy
			if os.path.isfile(file):
				svgFilePathsList.append(file)
	
	svgFilePathsList = validateSVGfiles(svgFilePathsList)

	return fontFilePath, UPM, fontFormat, svgFilePathsList


def run():
	fontFilePath, UPM, fontFormat, svgFilePathsList = getOptions()

	if len(svgFilePathsList): # after validation there may not be any SVG files left to process
		ttxFilePath = writeTTXfile(fontFilePath, UPM, fontFormat, svgFilePathsList)
	
		writeNewFontFile(fontFilePath, ttxFilePath)
	
		# delete temporary TTX file
		if os.path.exists(ttxFilePath) and not debug:
			os.remove(ttxFilePath)
	
	else:
		print "ERROR: Could not assemble any data for the SVG table."


if __name__ == "__main__":
	run()
