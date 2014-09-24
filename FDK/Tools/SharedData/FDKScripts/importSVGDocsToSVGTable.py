_help_ = """
importSVGDocsToSVGTable.py v 2.0 Sep 23 2014

python importSVGDocsToSVGTable.py <path to input OTF/TTF file>  <path(s) to SVG files OR to folder containing SVG files> 

Converts a set of SVG docs to a TTX SVG table definition, and then adds this table into the font file.
The script will replace any pre-existing SVG table.

Requires the <svg> element of every SVG file to have an 'id' parameter that follows the format 'glyphID' where ID is an integer number.
(If you're using Adobe Illustrator to create the SVG files, giving the name 'glyphID' to the top layer will fulfill the above requirement)

The integer ID values (of glyphID) should match the glyph indexes of the font.
"""

__copyright__ = """Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
"""

import os
import sys
import re
import io # io module is used for dealing with the line-ending differences that exist between platforms
from subprocess import Popen, PIPE

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

kSVGFileNameSuffix = "-SVG"

debug = False


def writeNewFontFile(fontFilePath, ttxFilePath):
	folderPath, fontFileName = os.path.split(fontFilePath)
	fileNameNoExtension, fileExtension = os.path.splitext(fontFileName)
	newFontFilePath = os.path.join(folderPath, "%s%s%s" % (fileNameNoExtension, kSVGFileNameSuffix, fileExtension))
	
	cmd = "ttx -o '%s' -m '%s' '%s'" % (newFontFilePath, fontFilePath, ttxFilePath)
	popen = Popen(cmd, shell=True, stdout=PIPE)
	popenout, popenerr = popen.communicate()
	if popenout:
		output = popenout
	else:
		return
	

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
		
		gid = re.search(r"<svg.+?id=\"glyph(\d+)\".+?>", data, re.DOTALL).group(1)
		svgDoc = kSVGDocTemplate % (gid, gid, data.strip())
		svgDocDict[gid] = svgDoc
	
	svgDocIndexList = svgDocDict.keys()
	svgDocIndexList.sort()
	svgDocList = [svgDocDict[index] for index in svgDocIndexList]
	
	tableContents = os.linesep.join(svgDocList)
	tableTotal = kSVGTableTemplate % (fontFormat, tableContents)
	
	# add 'transform' parameter to all <svg> elements (it does NOT check if the parameter already exists)
	tableFinal = re.sub(r"(<svg) ", r"""\1 transform="translate(0 -""" + UPM + """)\" """, tableTotal)
	
	f = open(ttxFilePath, "wt")
	f.write(tableFinal)
	f.close()
	
	if not len(svgDocList):
		print "ERROR: Could not assemble any data for the SVG table."
		return ttxFilePath, False # ok2continue
	else:
		return ttxFilePath, True


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
		# read file
		f = open(filePath, "rt")
		data = f.read()
		f.close()
		
		# find <xml> header
		xml = re.search(r"<\?xml.+?\?>", data)
		if not xml:
			print "WARNING: Could not find <xml> header in SVG file. Skiping %s" % (filePath)
			continue

		# find <svg> blob
		svg = re.search(r"<svg.+?>.+?</svg>", data, re.DOTALL)
		if not svg:
			print "WARNING: Could not find <svg> element in SVG file. Skiping %s" % (filePath)
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
	return headUPM


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
		dir = svgFilesOrFolder[0]
		svgFilePathsList = [os.path.join(dir, file) for file in os.listdir(dir)] # Assemble the full paths, not just file names
	else:
		svgFilePathsList = []
		for file in svgFilesOrFolder[:]: # copy
			if os.path.isfile(file):
				svgFilePathsList.append(file)
	
	svgFilePathsList = validateSVGfiles(svgFilePathsList)

	return fontFilePath, UPM, fontFormat, svgFilePathsList


def run():
	fontFilePath, UPM, fontFormat, svgFilePathsList = getOptions()
	ttxFilePath, ok2continue = writeTTXfile(fontFilePath, UPM, fontFormat, svgFilePathsList)
	
	if ok2continue:
		writeNewFontFile(fontFilePath, ttxFilePath)
	
	# delete temporary TTX file
	if os.path.exists(ttxFilePath) and not debug:
		os.remove(ttxFilePath)


if __name__ == "__main__":
	run()
