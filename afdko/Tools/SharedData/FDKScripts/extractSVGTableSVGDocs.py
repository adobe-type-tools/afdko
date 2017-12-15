_help_ = """
extractSVGTableSVGDocs.py v 1.01 Aug 20 2013

python extractSVGTableSVGDocs.py <path to input TTX file with SVG table> <path to folder to contain output SVG files> <file base mame>

Read SVG table from a TTX output file, and extract the SVG docs to
separate files.

Records the start/end GID in the SVG doc file name. The svg doc name is
built from the template  "<file base name>.start GID>.<end GID>.svg".

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
		print "The first argument must be a path to TTX file containing an SVG file table definition."
		print "Could not find '%s'." % (ttxFilePath)
		print sys.exit(0)

	if not os.path.exists(svgDir):
		print "Creating dir:", svgDir
		os.makedirs(svgDir)
	return ttxFilePath, svgDir, fileBaseName
	
def extractSVGDocs(ttxFilePath, svgDir, fileBaseName):
	fp = open(ttxFilePath, "rt")
	data = fp.read()
	fp.close()
	fileDataList = []
	m = re.search(r"<SVG>.+?</SVG>", data, re.DOTALL)
	if not m:
		print "Error: could not find SVG table in TTX file '%s'." % (ttxFilePath)
		return None
	svgData = data[m.start():m.end()]
	
	svgTable = XML(svgData)
	for svgDoc in svgTable:
		if not svgDoc.tag == "svgDoc":
			continue
		startGID = svgDoc.get("startGlyphID")
		endGID = svgDoc.get("endGlyphID")
		svgDocData = svgDoc.text.strip()
		svgPath = os.path.join(svgDir, "%s.%s.%s.svg" % (fileBaseName, startGID, endGID))
		fp = open(svgPath, "wt")
		fp.write(svgDocData.encode('utf-8'))
		fp.close()
	
def run():
	ttxFilePath, svgDir, fileBaseName = getOptions()
	extractSVGDocs(ttxFilePath, svgDir, fileBaseName)
	
if __name__ == "__main__":
	run()