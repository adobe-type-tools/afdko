_help_ = """
importSVGDocsToSVGTable.py v 1.01 Aug 20 2013

python importSVGDocsToSVGTable.py <path to input TTX file>  <path to folder to contain output SVG files> 

Converts the set of SVG docs to a TTX SVG table definition, and adds
this the TTX file.

Requires that the target TTX file exist. The script will replace any
pre- existing SVG table.

Requires that each SVG doc file  have a name matching "<file base
name>.start GID>.<end GID>.svg"

Removes any existing "glyphID" or "id="glyphX" tags from each svg document, and adds in
new ones, using the  start GID - end GID range.

Does NOT add a colorPalettes element to the SVG table, but will preserve
one if there is a colorPalettes element in a pre-existing SVG table.
"""

__copyright__ = """Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
"""

import os
import sys
import re
try:
    import xml.etree.cElementTree as ET
except ImportError:
    import xml.etree.ElementTree as ET
import copy

XML = ET.XML
XMLElement = ET.Element
xmlToString = ET.tostring

kSVGDocTemplate = """\t\t<svgDoc endGlyphID="%s" startGlyphID="%s">
\t\t\t<![CDATA[<?xml version="1.0" encoding="utf-8"?><!DOCTYPE svg  PUBLIC '-//W3C//DTD SVG 1.1//EN'  'http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd'>%s]]>
\t\t</svgDoc>"""

kSVGTableTemplate = """\t<SVG>
%s
\t</SVG>"""

def getOptions():
	
	if len(sys.argv) !=4:
		print "Need 3 argurments."
		print _help_
		sys.exit(0)
	
	if ("-u" in sys.argv) or ("-h" in sys.argv):
		print _help_
		sys.exit(0)

	ttxFilePath = sys.argv[1]
	svgDir =  sys.argv[2]
	fileBaseName =  sys.argv[3]

	if not os.path.exists(ttxFilePath):
		print "The first argument must be a path to TTX file."
		print "Could not find '%s'." % (ttxFilePath)
		print sys.exit(0)

	if not os.path.exists(svgDir):
		print "The second argument must be a path to a directory containing the SVG docs."
		print "Could not find '%s'." % (svgDir)
		print sys.exit(0)

	return ttxFilePath, svgDir, fileBaseName

def addGlyphID(startGID, endGID, data, filePath):
	m  = re.search(r"<svg\s(.+?)</svg>", data, re.DOTALL)
	if not m:
		raise ValueError("Error: There are no <svg> elements in the file '%s." % ())
	dataList = []
	glyphID = startGID-1
	while m:
		glyphID += 1
		if glyphID > endGID:
			raise ValueError("Error: there are more svg docs in the file than allowed by the endGID value '%s'. %s'." % (endGID, filePath))
			
		start = m.start()
		end = m.end()
		svgData = data[start:end]
		dataList.append(data[:start])
		data = data[end:]
		# remove old ones
		svgData = re.sub(r"glyphid=\".+?\"", "", svgData)
		svgData = re.sub(r"id=\"glyph.+?\"", "", svgData)
		# add new one.
		dataList.append("<svg id=\"glyph%s\" " % (glyphID))
		dataList.append(svgData[5:])
		m  = re.search(r"<svg\s(.+?)</svg>", data, re.DOTALL)
	
	if glyphID != endGID:
		raise ValueError("Error: The number of svg docs in the file ('%s') do not match the startGlyphID-endGlyphID range '%s-'%s'. %s'." % (glyphID -startGID+1,  startGID, endGID, filePath))
	data = "".join(dataList)
	return data

def parseFileName(fileName):
	parts = fileName.split(".")
	try:
		startGID = int(parts[1])
		endGID = int(parts[2])
	except (IndexError, ValueError):
		msg =  "Error. File name must have the format '<base file name>.<integer start GID>.<integer end GID>.svg'. "
		msg +=  "Failed to parse file name '%s'." % (fileName)
		raise ValueError(msg)
	return parts[0], startGID, endGID
	
def byValue(name1, name2):
	parts1 = parseFileName(name1)
	parts2 = parseFileName(name2)
	return cmp(parts1, parts2)
	
def importSVGDocs(ttxFilePath, svgDir, fileBaseName):
	# Build the SVG table text.
	fileList = os.listdir(svgDir)
	fileList = filter(lambda fileName: fileName.startswith(fileBaseName), fileList)
	if len(fileList) == 0:
		print "Could not find any files starting with '%s' in '%s'." % (fileBaseName, svgDir)
		return None
	fileList.sort(byValue)
	svgDocList = []
	for fileName in fileList:
		parts = fileName.split(".")
		try:
			startGID = int(parts[1])
			endGID = int(parts[2])
		except (IndexError, ValueError):
			print "Error. File name must have the format '<base file name>.<integer start GID>.<integer end GID>.svg'."
			print "Failed to parse file name '%s'." % (fileName)
			return None
		filePath = os.path.join(svgDir, fileName)
		fp = open(filePath, "rt")
		data = fp.read()
		fp.close()
		# Make sure that svgDocs have the glyphID attribute, and the right glyphID attribute
		data = addGlyphID(startGID, endGID, data, filePath)
		svgDoc = kSVGDocTemplate % (startGID, endGID, data.strip())
		svgDocList.append(svgDoc)
		
	svgDocList.append("\t")
	tableData = os.linesep.join(svgDocList)
	tableText = kSVGTableTemplate % (tableData + os.linesep + "<colorPalettes></colorPalettes>" + os.linesep)
		
	# Add it to the TTX file.
	fp = open(ttxFilePath, "rt")
	oldData = fp.read()
	fp.close()
	m = re.search(r"<SVG>.+?</SVG>", oldData, re.DOTALL)
	if m:
		# Replace existing table. Extract the color palettes, if any, and add them back in.
		oldSVGData = oldData[m.start():m.end()]
		cm = re.search(r"\s*<colorPalettes>.+?</colorPalettes>\s*", oldSVGData, re.DOTALL)
		if cm:
			cpData = oldSVGData[cm.start():cm.end()]
			tableText = kSVGTableTemplate % (tableData + cpData)
		newData = oldData[:m.start()] + tableText + oldData[m.end():]
	else:
		# Insert table data at end.
		m = re.search(r"\s+</ttFont>", oldData, re.DOTALL)
		if not m:
			print "Error. Could not find string '</ttFont>' in TTX file '%s': must be some other format." % (ttxFilePath)
			return None
		newData = oldData[:m.start()] + tableText + oldData[m.start():]
			
	fp = open(ttxFilePath + ".new", "wt")
	fp.write(newData)
	fp.close()
	
def run():
	ttxFilePath, svgDir, fileBaseName = getOptions()
	svgData = importSVGDocs(ttxFilePath, svgDir, fileBaseName)
	
if __name__ == "__main__":
	run()