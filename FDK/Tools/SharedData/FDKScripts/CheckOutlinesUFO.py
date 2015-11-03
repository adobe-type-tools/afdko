__copyright__ = """Copyright 2015 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
"""

__usage__ = """
   checkOutlinesUFO program v1.16 Nov 3 2015

   checkOutlinesUFO [-e] [-g glyphList] [-gf <file name>] [-all] [-noOverlap] [-noBasicChecks] [-setMinArea <n>] [-setTolerance <n>] [-wd]

   Remove path overlaps, and do a few basic outline quality checks.
 """
from ufoTools import kProcessedGlyphsLayerName, kProcessedGlyphsLayer

__help__ = """

-decimal	do not round point values to integer

-e	fix reported problems

-noOverlap	turn off path overlap checks
	This reports path overlaps, and tiny outlines, such as those formed when two
	line segments should meet, but cross each other just before the join,
	defining a triangle a few units on a side.


-noBasicChecks	turn off basic checks:
	- coincident points
	- flat curves (curves which are a straight line)
	- colinear lines (several line segments that define the same path asa single line);

-setMinArea <n>  Set the minimum area for a tiny outline. Default is 25 square
units. Subpaths with a bounding box less than this will be reported, and deleted
if fixed.

-setTolerance <n>  Set the maximum deviation from a straight line allowed.
Default is 0 design space units. This is used to test whether the control points
for a curve define a flat curve, and whether a sequence of line segments
defines a straight line.

-wd	write changed glyphs to default layer instead of '%s'

-g <glyphID1>,<glyphID2>,...,<glyphIDn>
	Check only the specified list of glyphs. The list must be
	comma-delimited. The glyph ID's may be glyphID's, glyph names, or
	glyph CID's. If the latter, the CID value must be prefixed with the
	string "cid". There must be no white-space in the glyph list.
	Examples:
		checkOutlinesUFO -g A,B,C,69 myFont
		checkOutlinesUFO -g cid1030,cid34,cid3455,69 myCIDFont

-gf <file name>
	Check only the list of glyphs contained in the specified file, The file
	must contain a comma-delimited list of glyph identifiers. Any number of
	space, tab, and new-line characters are permitted between glyph names
	and commas.

-all Force all glyphs to be processed. This applies only to UFO fonts, where processed glyphs
	are saved in a layer, and processing of a glyph is skipped if has already been processed.

""" % kProcessedGlyphsLayerName

__doc__ = __usage__ + __help__


import sys
import os
import re
import copy
import defcon
import ufoLib
import subprocess
import shutil
import ufoTools
import booleanOperations.booleanGlyph
from robofab.pens.digestPen import DigestPointPen
from plistlib import readPlist, writePlistToString

import hashlib

kSrcGLIFHashMap = "com.adobe.type.checkOutlinesHashMap"

class focusOptionParseError(KeyError):
	pass

class focusFontError(KeyError):
	pass

kUknownFontType = 0
kUFOFontType = 1
kType1FontType = 2
kCFFFontType = 3
kOpenTypeCFFFontType = 4

class FontFile(object):
	def __init__(self, fontPath):
		self.fontPath = fontPath
		self.tempUFOPath = None
		self.fontType = kUknownFontType
		self.dFont = None
		self.useHashMap = False
		self.ufoFontHashData = None
		self.ufoFormat = 2
		self.saveToDefaultLayer = 0

	def open(self, useHashMap):
		fontPath = self.fontPath
		try:
			ufoTools.validateLayers(fontPath)
			self.dFont = dFont = defcon.Font(fontPath)
			self.ufoFormat = dFont.ufoFormatVersion
			if self.ufoFormat < 2:
				self.ufoFormat = 2
			self.fontType = kUFOFontType
			self.useHashMap = useHashMap
			self.ufoFontHashData = ufoTools.UFOFontData(fontPath, self.useHashMap, programName=ufoTools.kCheckOutlineName)
			self.ufoFontHashData.readHashMap()

		except ufoLib.UFOLibError,e:
			if (not os.path.isdir(fontPath)) and "metainfo.plist is missing" in e.message:
				# It was a file, but not a UFO font. try converting to UFO font, and try again.
				print "converting to temp UFO font..."
				self.tempUFOPath = tempPath = fontPath + ".temp.ufo"
				if os.path.exists(tempPath):
					 shutil.rmtree(tempPath)
				cmd = "tx -ufo \"%s\" \"%s\"" % (fontPath, tempPath)
				p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT).stdout
				log = p.read()
				if os.path.exists(tempPath):
					try:
						self.dFont = dFont = defcon.Font(tempPath)
					except ufoLib.UFOLibError,e:
						return
					# It must be a font file!
					self.tempUFOPath = tempPath
					# figure out font type.
					try:
						ff = file(fontPath, "rb")
						data = ff.read(10)
						ff.close()
					except (IOError, OSError):
						return
					if data[:4] == "OTTO": # it is an OTF font.
						self.fontType = kOpenTypeCFFFontType
					elif (data[0] == '\1') and (data[1] == '\0'): # CFF file
						self.fontType = kCFFFontType
					elif "%" in data:
						self.fontType = kType1FontType
					else:
						print "Font type is unknown: will not be able to save changes"
			else:
				raise(e)
		return dFont

	def close(self):
		# Differs from save in that it saves just the hash file. To be used when the glif files have not been changed by this program,
		# but the hash file has changed.
		if self.fontType == kUFOFontType:
			self.ufoFontHashData.close() # Write the hash data, if it has changed.

	def save(self):
		print "Saving font..."
		""" A real hack here.
		If the font format was less than 3, we need to save it with the original
		format. I care specifically about RoboFont, which can still read only
		format 2. However, dfont.save() will save the processed layer only if
		the format is 3. If I ask to save(formatVersion=3) when the dfont format
		is 2, then the save function will  first mark all the glyphs in all the layers
		as being 'dirty' and needing to be saved. and also causes the defcon
		font.py:save to delete the original font and write everything anew.

		In order to avoid this, I reach into the defcon code and save only the processed glyphs layer.
		I also set glyphSet.ufoFormatVersion so that the glif files will be set to format 1.
		"""
		from ufoLib import UFOWriter
		writer = UFOWriter(self.dFont.path, formatVersion=2)
		if self.saveToDefaultLayer:
			self.dFont.save()
		else:
			layers = self.dFont.layers
			layer = layers[kProcessedGlyphsLayerName]
			writer._formatVersion = 3
			writer.layerContents[kProcessedGlyphsLayerName] = kProcessedGlyphsLayer
			glyphSet = writer.getGlyphSet(layerName=kProcessedGlyphsLayerName, defaultLayer=False)
			writer.writeLayerContents(layers.layerOrder)
			glyphSet.ufoFormatVersion = 2
			layer.save(glyphSet)
			layer.dirty = False

		if self.fontType == kUFOFontType:
			self.ufoFontHashData.close() # Write the hash data, if it has changed.
		elif self.fontType == kType1FontType:
			cmd = "tx -t1 \"%s\" \"%s\"" % (self.tempUFOPath, self.fontPath)
			p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT).stdout
			log = p.read()
		elif self.fontType == kCFFFontType:
			cmd = "tx -cff +b -std \"%s\" \"%s\"" % (self.tempUFOPath, self.fontPath)
			p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT).stdout
			log = p.read()
		elif self.fontType == kOpenTypeCFFFontType:
			tempCFFPath = self.tempUFOPath + ".cff"
			cmd = "tx -cff +b -std \"%s\" \"%s\"" % (self.tempUFOPath, tempCFFPath)
			p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT).stdout
			log = p.read()
			cmd = "sfntedit -a \"CFF \"=\"%s\" \"%s\"" % (tempCFFPath, self.fontPath)
			p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT).stdout
			log = p.read()
			if os.path.exists(tempCFFPath):
				os.remove(tempCFFPath)
		else:
			print "Font type is unknown: cannot save changes"

		if (self.tempUFOPath != None) and os.path.exists(self.tempUFOPath):
			shutil.rmtree(self.tempUFOPath)

	def checkSkipGlyph(self, glyphName, doAll):
		skip = False
		if self.ufoFontHashData and self.useHashMap:
			width, outlineXML, skip = self.ufoFontHashData.getOrSkipGlyphXML(glyphName, doAll)
		return skip

	def buildGlyphHash(self, width, glyphDigest):
		dataList = [str(width)]
		for x,y in glyphDigest:
			dataList.append("%s%s" %  (x,y))
		dataList.sort()
		data = "".join(dataList)
		if len(data) < 128:
			hash = data
		else:
			hash = hashlib.sha512(data).hexdigest()
		return hash

	def updateHashEntry(self, glyphName, changed):
		if self.ufoFontHashData != None:  # isn't a UFO font.
			self.ufoFontHashData.updateHashEntry(glyphName, changed)

	def clearHashMap(self):
		if self.ufoFontHashData != None:
			self.ufoFontHashData.clearHashMap()

class COOptions:
	def __init__(self):
		self.filePath = None
		self.outFilePath = None
		self.logFilePath = None
		self.glyphList = []
		self.allowChanges = 0
		self.writeToDefaultLayer = 0
		self.allowDecimalCoords = 0
		self.minArea = 25
		self.tolerance = 0
		self.checkAll = False # forces all glyphs to be processed even if src hasn't changed.
		self.removeCoincidentPointsDone = 0 # processing state flag, used to not repeat coincident point removal.
		self.removeFlatCurvesDone = 0 # processing state flag, used to not repeat flat curve point removal.
		self.clearHashMap = False

		# doOverlapRemoval must come first in the list, since it may cause problems, like co-linear lines,
		# that need to be checked/fixed by later tests.
		self.testList = [doOverlapRemoval, doCleanup]

def parseGlyphListArg(glyphString):
	glyphString = re.sub(r"[ \t\r\n,]+",  ",",  glyphString)
	glyphList = glyphString.split(",")
	glyphList = map(expandNames, glyphList)
	glyphList =  filter(None, glyphList)
	return glyphList

def getOptions():
	global debug

	options = COOptions()
	i = 1
	numOptions = len(sys.argv)
	while i < numOptions:
		arg = sys.argv[i]
		if options.filePath and arg[0] == "-":
			raise focusOptionParseError("Option Error: All file names must follow all other options <%s>." % arg)

		if arg == "-test":
			import doctest
			sys.exit(doctest.testmod().failed)
		elif arg == "-u":
			print __usage__
			sys.exit()
		elif arg == "-h":
			print __doc__
			sys.exit()
		elif arg == "-wd":
			options.writeToDefaultLayer = 1
		elif arg == "-g":
			i = i +1
			glyphString = sys.argv[i]
			if glyphString[0] == "-":
				raise focusOptionParseError("Option Error: it looks like the first item in the glyph list following '-g' is another option.")
			options.glyphList += parseGlyphListArg(glyphString)
		elif arg == "-gf":
			i = i +1
			filePath = sys.argv[i]
			if filePath[0] == "-":
				raise focusOptionParseError("Option Error: it looks like the the glyph list file following '-gf' is another option.")
			try:
				gf = file(filePath, "rt")
				glyphString = gf.read()
				glyphString = glyphString.strip()
				gf.close()
			except (IOError,OSError):
				raise focusOptionParseError("Option Error: could not open glyph list file <%s>." %  filePath)
			options.glyphList += parseGlyphListArg(glyphString)
		elif arg == "-e":
			options.allowChanges = 1
		elif arg == "-noOverlap":
			options.testList.remove(doOverlapRemoval)
		elif arg == "-noBasicChecks":
			options.testList.remove(doCleanup)
		elif arg == "-setMinArea":
			i = i +1
			try:
				options.minArea = int(sys.argv[i])
			except:
				raise focusOptionParseError("The argument following '-setMinArea' must be an integer.")
		elif arg == "-setTolerance":
			i = i +1
			try:
				options.tolerance = int(sys.argv[i])
			except:
				raise focusOptionParseError("The argument following '-setTolerance' must be an integer.")
		elif arg == "-decimal":
			options.allowDecimalCoords = 1
		elif arg == "-all":
			options.checkAll = True
		elif arg == "-clearHashMap":
			options.clearHashMap = True
		elif arg[0] == "-":
			raise focusOptionParseError("Option Error: Unknown option <%s>." %  arg)
		else:
			if options.filePath:
				raise focusOptionParseError("Option Error: You cannot specify more than one file to check <%s>." %  arg)
			options.filePath = arg

		i  += 1
	if not options.filePath:
		raise focusOptionParseError("Option Error: You must provide a font file path.")
	else:
		options.filePath = options.filePath.rstrip(os.sep) # might be a UFO font. auto completion in some shells adds a dir separator, which then causes problems with os.path.dirname().

	if not os.path.exists(options.filePath):
		raise focusOptionParseError("Option Error: The file path does not exist.")

	return options

def getGlyphID(glyphTag, fontGlyphList):
	glyphID = None
	try:
		glyphID = int(glyphTag)
		glyphName = fontGlyphList[glyphID]
	except IndexError:
		pass
	except ValueError:
		try:
			glyphID = fontGlyphList.index(glyphTag)
		except IndexError:
			pass
		except ValueError:
			pass
	return glyphID

def expandNames(glyphName):
	global nameAliasDict

	glyphRange = glyphName.split("-")
	if len(glyphRange) > 1:
		g1 = expandNames(glyphRange[0])
		g2 =  expandNames(glyphRange[1])
		glyphName =  "%s-%s" % (g1, g2)

	elif glyphName[0] == "/":
		glyphName = "cid" + glyphName[1:].zfill(5)
		if glyphName == "cid00000":
			glyphName = ".notdef"
			nameAliasDict[glyphName] = "cid00000"

	elif glyphName.startswith("cid") and (len(glyphName) < 8):
		glyphName =  "cid" + glyphName[3:].zfill(5)
		if glyphName == "cid00000":
			glyphName = ".notdef"
			nameAliasDict[glyphName] = "cid00000"

	return glyphName

def getGlyphNames(glyphTag, fontGlyphList, fontFileName):
	glyphNameList = []
	rangeList = glyphTag.split("-")
	prevGID = getGlyphID(rangeList[0], fontGlyphList)
	if prevGID == None:
		if len(rangeList) > 1:
			print( "\tWarning: glyph ID <%s> in range %s from glyph selection list option is not in font. <%s>." % (rangeList[0], glyphTag, fontFileName))
		else:
			print( "\tWarning: glyph ID <%s> from glyph selection list option is not in font. <%s>." % (rangeList[0], fontFileName))
		return None
	try:
		gName = fontGlyphList[prevGID]
	except IndexError:
		print ("\tWarning: glyph ID <%s> from glyph selection list option is not in font. <%s>." % (prevGID, fontFileName))
		return None
	glyphNameList.append(gName)

	for glyphTag2 in rangeList[1:]:
		gid = getGlyphID(glyphTag2, fontGlyphList)
		if gid == None:
			print("\tWarning: glyph ID <%s> in range %s from glyph selection list option is not in font. <%s>." % (glyphTag2, glyphTag, fontFileName))
			return None
		for i in range(prevGID+1, gid+1):
			glyphNameList.append(fontGlyphList[i])
		prevGID = gid

	return glyphNameList

def filterGlyphList(options, fontGlyphList, fontFileName):
	# Return the list of glyphs which are in the intersection of the argument list and the glyphs in the font
	# Complain about glyphs in the argument list which are not in the font.
	if not options.glyphList:
		glyphList = fontGlyphList
	else:
		# expand ranges:
		glyphList = []
		for glyphTag in options.glyphList:
			glyphNames = getGlyphNames(glyphTag, fontGlyphList, fontFileName)
			if glyphNames != None:
				glyphList.extend(glyphNames)
	return glyphList

def getDigest(dGlyph):
	"""copied from robofab ObjectsBase.py.
	"""
	mp = DigestPointPen()
	dGlyph.drawPoints(mp)
	digest = list(mp.getDigestPointsOnly(needSort=False))
	digest.append( (len(dGlyph.contours), 0) )
	# we need to round to int.
	intDigest = digest #[ (int(p[0]), int(p[1])) for p in digest]
	return intDigest

def removeCoincidentPoints(bGlyph, changed, msg, options):
	""" Remove coincident points.
	# a point is (segmentType, pt, smooth, name).
	# e.g. (u'curve', (138, -92), False, None)
	"""
	for contour in bGlyph.contours:
		firstOp = point = contour._points[0]
		op = contour._points[0]
		i = 1
		numOps = len(contour._points)
		while i < numOps:
			prevPoint = point
			point = contour._points[i]
			type = point[0]
			if type != None:
				lastOp = op
				op = point
				if lastOp[1] == op[1]:
					# Coincident point. Remove it.
					msg.append( "Coincident points at (%s,%s)." % (op[1]))
					changed = 1
					del contour._points[i]
					numOps -= 1
					if type == "curve":
						# Need to delete the previous two points as well.
						i -= 1
						del contour._points[i]
						i -= 1
						del contour._points[i]
						numOps -= 2
					# Note that we do not increment i - this will now index the next point.
					continue
			i += 1
	return changed, msg


def removeTinySubPaths(bGlyph, minArea, changed, msg):
	"""
	Removes tiny subpaths that are created by overlap removal when the start
	and end path segments cross each other, rather than meet.

	>>> from defcon.objects.glyph import Glyph
	>>> import booleanOperations.booleanGlyph

	# large contour
	>>> g = Glyph()
	>>> p = g.getPen()
	>>> p.moveTo((100,100))
	>>> p.lineTo((200,200))
	>>> p.lineTo((0,100))
	>>> p.closePath()
	>>> assert (len(g[0]) == 3), "Contour does not have the expected number of points"
	>>> assert (g.bounds == (0, 100, 200, 200)), "The contour's bounds do not match the expected"
	>>> bg = booleanOperations.booleanGlyph.BooleanGlyph(g)
	>>> removeTinySubPaths(bg, 25, 0, []) == (0, [])
	True

	# small contour
	>>> g = Glyph()
	>>> p = g.getPen()
	>>> p.moveTo((1,1))
	>>> p.lineTo((2,2))
	>>> p.lineTo((0,1))
	>>> p.closePath()
	>>> assert (len(g[0]) == 3), "Contour does not have the expected number of points"
	>>> assert (g.bounds == (0, 1, 2, 2)), "The contour's bounds do not match the expected"
	>>> bg = booleanOperations.booleanGlyph.BooleanGlyph(g)
	>>> removeTinySubPaths(bg, 25, 0, []) == (0, ['Contour 0 is too small: bounding box is less than minimum area. Start point: ((1, 1)).'])
	True
	"""
	numContours = len(bGlyph.contours)
	ci = 0
	while ci < numContours:
		contour = bGlyph.contours[ci]
		ci += 1
		onLinePts = filter(lambda pt: pt[0] != None, contour._points)
		numPts = len(onLinePts)
		if numPts <= 4:
			cBounds = contour.bounds
			cArea = (cBounds[2] - cBounds[0])*(cBounds[3] - cBounds[1])
			if cArea < minArea:
				ci -= 1
				numContours -= 1
				msg.append("Contour %s is too small: bounding box is less than minimum area. Start point: (%s)." % (ci, contour._points[0][1]))
				del bGlyph.contours[ci]
	return changed, msg


def isColinearLine(b3, b2, b1, tolerance = 0):
	# b1-b3 are three points  [x, y] to test.
	# b1 = start point of first line, b2 is start point of second, b3 is end point of second line.
	# tolerance is how tight the match must be in design units, default 0.
	isCL = 1 # is a co-linear line
	x1 = b1[-2]
	y1 = b1[-1]
	x2 = b2[-2]
	y2 = b2[-1]
	x3 = b3[-2]
	y3 = b3[-1]


	dx2 = x2 -x1
	dy2 = y2 -y1
	if (dx2 == 0) and (dy2 == 0):
		return 1

	if (x3-x1) == 0:
		if abs(dx2) > tolerance:
			return 0
		else:
			return 1

	if (y3-y1) == 0:
		if abs(dy2) > tolerance:
			return 0
		else:
			return 1

	dx3 = x3 -x2
	dy3 = y3 -y2
	if (dx3 == 0) and (dy3 == 0):
		return 1

	if (dx2) == 0:
		if abs(x3 -x1) > tolerance:
			return 0
		else:
			return 1

	if (dy2) == 0:
		if abs(y3 -y1) > tolerance:
			return 0
		else:
			return 1

	a = float(dy2)/(dx2)
	b = y1-a*x1
	if abs(a) <= 1:
		y3t = a*x3+b
		if abs(y3t -y3) > tolerance:
			return 0
		else:
			return 1
	else:
		x3t = float(y3-b)/a
		if abs(x3t -x3) > tolerance:
			return 0
		else:
			return 1

	#if dx or dy == 0, then  matching would be caught be previous arg test.
	return isCL

def removeFlatCurves(newGlyph, changed, msg, options):
	""" Remove flat curves.
	# a point is (segmentType, pt, smooth, name).
	# e.g. (u'curve', (138, -92), False, None)
	"""
	for contour in newGlyph.contours:
		numOps = len(contour._points)
		i = 0
		while i < numOps:
			p0 = contour._points[i]
			type = p0[0]

			if type == "curve":
				p3 = contour._points[i-3]
				p2 = contour._points[i-2]
				p1 = contour._points[i-1]
				# fix flat curve.
				isCL1 = isColinearLine(p3[1], p2[1], p0[1], options.tolerance)
				if isCL1:
					isCL2 = isColinearLine(p3[1], p1[1], p0[1], options.tolerance)
					if isCL2:
						msg.append("Flat curve at (%s, %s)." % (p0[1]))
						changed = 1
						p0 = list(p0)
						p0[0] = type = "line"
						contour._points[i] = tuple(p0)
						numOps -= 2
						if i > 0:
							del contour._points[i-1]
							del contour._points[i-2]
							i -= 2
						else:
							del contour._points[i-1]
							del contour._points[i-1]
			i += 1
	return changed, msg

def removeColinearLines(newGlyph, changed, msg, options):
	""" Remove colinear line- curves.
	# a point is (segmentType, pt, smooth, name).
	# e.g. (u'curve', (138, -92), False, None)
	"""
	for contour in newGlyph.contours:
		numOps = len(contour._points)
		if numOps < 3:
			continue # Need at least two line segments to have a co-linear line.
		p0 = contour._points[0]
		i = 0
		while i < numOps:
			p0 = contour._points[i]
			type = p0[0]
			# Check for co-linear line.
			if type == "line":
				p1 = contour._points[i-1]
				if p1[0] == "line": # if p1 is a curve  point, no need to do colinear check!
					p2 = contour._points[i-2]
					isCL1 = isColinearLine(p2[1], p1[1], p0[1], options.tolerance)
					if isCL1:
						msg.append( "Colinear line at (%s, %s)." % (p1[1]))
						changed = 1
						del contour._points[i-1]
						numOps -= 1
						if numOps < 3:
							break # Need at least two line segments to have a co-linear line.
						i -= 1
			i += 1
	return changed, msg


def splitTouchingPaths(newGlyph):
	""" This hack fixes a design difference between the Adobe checkOutlines logic and booleanGlyph.
	With checkOutlines, if only a single point on a contour lines is coincident with the path of the another contour,
	the paths are NOT merged. with booleanGlyph, they are merged. An example is the vertex of a diamond shape having the same
	y coordinate as a horizontal line in another path, but no other overlap.
	However, this works only when a single point on one contour is coincident with another contour, with no other overlap.
	If there is more than one point of contact, then result is separate inner (unfilled) contour. This logic works only when the
	touching contour is merged with the other contour.
	"""
	numPaths = len(newGlyph.contours)
	i = 0
	while i < numPaths:
		contour = newGlyph.contours[i]
		numPts = len(contour._points)
		sortlist = [[ contour._points[j], j] for j in range(numPts)]
		sortlist = filter(lambda entry: entry[0][0] != None, sortlist)
		sortlist.sort()
		j = 1
		lenSortList = len(sortlist)
		haveNewContour = 0
		while j < lenSortList:
			coords1, j1 =  sortlist[j][0][1], sortlist[j][1]
			coords0, j0 = sortlist[j-1][0][1], sortlist[j-1][1]
			if (numPts != (j1+1)) and ((j1 - j0) > 1) and (coords0 == coords1):
				# If the contour has two non-contiguous points that are coincident, and are not the
				# start and end points, then we have  contour that can be pinched off.
				# I don't bother to test that j0 is the start point, as UFO paths are always closed.
				newContourPts = contour._points[j0:j1+1]
				np0 = newContourPts[0]
				if np0[0] == 'curve':
					newContourPts[0] = ('line', np0[1], np0[2], np0[3])
				newContour = booleanOperations.booleanGlyph.BooleanContour()
				newContour._points = newContourPts
				newContour._clockwise = newContour._get_clockwise()
				#print "i", i
				#print "coords1, j1", coords1, j1
				#print "coords0, j0 ", coords0, j0
				#print "newContour._clockwise", repr(newContour._clockwise)
				if 1: #newContour.clockwise == contour.clockwise:
					#print "un-merging", newContour._points, repr(newContour._clockwise)
					del contour._points[j0+1:j1+1]
					newGlyph.contours.insert(i+1, newContour)
					haveNewContour = 1
					numPaths += 1
					break
			j +=1
		if haveNewContour:
			continue
		i += 1

def roundPt(pt):
	pt = map(int, pt)
	return pt

def doOverlapRemoval(bGlyph, oldDigest, changed, msg, options):
	changed, msg = removeCoincidentPoints(bGlyph, changed, msg, options)
	options.removeCoincidentPointsDone = 1
	changed, msg = removeFlatCurves(bGlyph, changed, msg, options)
	changed, msg = removeColinearLines(bGlyph, changed, msg, options)
	options.removeFlatCurvesDone = 1
	# I need to fix these in the source, or the old vs new digests will differ, as BooleanOperations removes these
	# even if it does not do overlap removal.
	oldDigest = list(getDigest(bGlyph))
	oldDigest.sort()
	oldDigest = map(roundPt, oldDigest)
	newDigest = []
	prevDigest = oldDigest
	newGlyph = bGlyph

	while newDigest != prevDigest:
		# This hack to get around a bug in booleanGlyph. Consider an M sitting on a separate rectangular crossbar contour,
		# the bottom of the M legs being co-linear with the top of the cross bar. pyClipper will merge only one of the
		# co-linear lines with each call to removeOverlap(). I suspect that this bug is in pyClipper, but haven't yet looked.
		prevDigest = newDigest
		newGlyph = newGlyph.removeOverlap()
		newDigest = list(getDigest(newGlyph))
		newDigest.sort()
		newDigest = map(roundPt, newDigest) # The new path points sometimes come back with very small fractional parts to to rounding issues.

	# Can't use change in path number to see if something has changed - overlap removal can add and subtract paths.
	if str(oldDigest) != str(newDigest):
		changed = 1
		msg.append( "There is an overlap.")
	# don't need to remove coincident points again. These are not added by overlap removal.
	changed, msg = removeTinySubPaths(newGlyph, options.minArea, changed, msg) # Tiny subpaths are added by overlap removal.
	return newGlyph, newDigest, changed, msg

def doCleanup(newGlyph, oldDigest, changed, msg, options):
	newDigest = oldDigest

	if newGlyph == None:
		return newGlyph, newDigest, changed, msg

	if oldDigest == None:
		oldDigest = list(getDigest(newGlyph))
		oldDigest.sort()
		oldDigest = map(roundPt, oldDigest)

	# Note that these removeCoincidentPointsDone and removeFlatCurvesDone get called only if doOverlapRemoval is NOT called.
	if not options.removeCoincidentPointsDone:
		changed, msg = removeCoincidentPoints(newGlyph, changed, msg, options)
		options.removeCoincidentPointsDone = 1
	if not options.removeFlatCurvesDone:
		changed, msg = removeFlatCurves(newGlyph, changed, msg, options)
	# I call removeColinearLines even when doOverlapRemoval is called, as the latter can introduce new co-linear points.
	changed, msg = removeColinearLines(newGlyph, changed, msg, options)

	return newGlyph, newDigest, changed, msg


def restoreContourOrder(fixedGlyph, originalContours):
	""" The pyClipper library first sorts all the outlines by x position, then y position.
	I try to undo that, so that un-touched contours will end up in the same order as the
	in the original, and any conbined contours will end up in a similar order.
	The reason I try to match new contours to the old is to reduce arbitraryness in the new contour order.
	If contours are sorted in x, then y position, then contours in the same
	glyph from two different  instance fonts which are created from a set of
	master designs, and then run through this program may, end up with different
	contour order. I can't completely avoid this, but I can considerably reduce how often it happens.
	"""
	newContours = list(fixedGlyph)
	newIndexList = range(len(newContours))
	newList = [[i,newContours[i]] for i in newIndexList]
	oldIndexList = range(len(originalContours))
	oldList = [[i,originalContours[i]] for i in oldIndexList]
	orderList = []

	# Match start points, This will fix the order of the contours that have not been touched.
	for i in newIndexList:
		ci, contour = newList[i]
		firstPoint = contour[0]
		for j in oldIndexList:
			ci2, oldContour = oldList[j]
			oldFP = oldContour[0]
			if (firstPoint.x == oldFP.x) and (firstPoint.y == oldFP.y):
				newList[i] = None
				orderList.append([ci2, ci])
				break
	newList = filter(lambda entry: entry != None, newList)

	# New contour start point did not match any of old contour start points.
	# Look through original contours, and see if the start point for any old contour
	# is on the new contour.
	numC = len(newList)
	if numC > 0: # If the new contours aren't already all matched..
		newIndexList = range(numC)
		for i in newIndexList:
			ci, contour = newList[i]
			for j in oldIndexList:
				ci2, oldContour = oldList[j]
				oldFP = oldContour[0]
				matched = 0
				pi = -1
				for point in contour:
					pi += 1
					if point.segmentType == None:
						continue
					if (oldFP.x == point.x) and (oldFP.y == point.y):
						newList[i] = None
						orderList.append([ci2, ci])
						matched = 1
						contour.setStartPoint(pi)
						break
				if matched:
					break

	newList = filter(lambda entry: entry != None, newList)
	numC = len(newList)
	# Start points didn't all match. Check each extreme for a match.
	for ti in range(4):
		if numC > 0:
			newIndexList = range(numC)
			for i in newIndexList:
				ci, contour = newList[i]
				maxP = contour[0]
				for point in contour:
					if point.segmentType == None:
						continue
					if ti == 0:
						if maxP.y < point.y:
							maxP = point
					elif ti == 1:
						if maxP.y > point.y:
							maxP = point
					elif ti == 2:
						if maxP.x < point.x:
							maxP = point
					elif ti == 3:
						if maxP.x > point.x:
							maxP = point
				# Now search the old contour list.
				for j in oldIndexList:
					ci2, oldContour = oldList[j]
					matched = 0
					for point in oldContour:
						if point.segmentType == None:
							continue
						if (maxP.x == point.x) and (maxP.y == point.y):
							newList[i] = None
							orderList.append([ci2, ci])
							matched = 1
							break
					if matched:
						break
			newList = filter(lambda entry: entry != None, newList)
			numC = len(newList)

	# Now re-order the new list
	orderList.sort()
	newContourList= []
	for ci2, ci in orderList:
		newContourList.append(newContours[ci])

	# If the algorithm didn't work for some contours,
	# just add them on the end.
	if numC != 0:
		for ci, contour in newList:
			newContourList.append(contour)
			keys = dir(contour)
			keys.sort()

	numC = len(newContourList)
	fixedGlyph.clearContours()
	for contour in newContourList:
		fixedGlyph.appendContour(contour)


def run(args):

	options = getOptions()
	fontPath = os.path.abspath(options.filePath)
	dFont = None
	fontFile = FontFile(fontPath)
	dFont = fontFile.open(options.allowChanges) # We allow use of a hash map to skip glyphs only if fixing glyphs
	if options.clearHashMap:
		fontFile.clearHashMap()
		return

	if dFont == None:
		print "Could not open  file: %s." % (fontPath)
		return

	glyphCount = len(dFont)
	changed = 0
	glyphList = filterGlyphList(options, dFont.keys(), options.filePath)
	if not glyphList:
		raise focusFontError("Error: selected glyph list is empty for font <%s>." % options.filePath)

	if not options.writeToDefaultLayer:
		try:
			processedLayer = dFont.layers[kProcessedGlyphsLayerName]
		except KeyError:
			processedLayer = dFont.newLayer(kProcessedGlyphsLayerName)
	else:
		processedLayer = None
		fontFile.saveToDefaultLayer = 1
	fontChanged = 0
	lastHadMsg = 0
	seenGlyphCount = 0
	processedGlyphCount = 0
	glyphList.sort()
	for glyphName in glyphList:
		changed = 0
		seenGlyphCount +=1
		msg = []

		# fontFile.checkSkipGlyph updates the hash map for the glyph, so we call it even when
		# the  '-all' option is used.
		skip = fontFile.checkSkipGlyph(glyphName, options.checkAll)
		#Note: this will delete glyphs from the processed layer, if the glyph hash has changed.
		if skip:
			continue
		processedGlyphCount += 1

		dGlyph = dFont[glyphName]
		if dGlyph.components:
			dGlyph.decomposeAllComponents()
		newGlyph = booleanOperations.booleanGlyph.BooleanGlyph(dGlyph)
		glyphDigest = None
		for test in options.testList:
			if test != None:
				newGlyph, glyphDigest, changed, msg = test(newGlyph, glyphDigest, changed, msg, options)

		if len(msg) == 0:
			if lastHadMsg:
				print
			print ".",
			lastHadMsg = 0
		else:
			print os.linesep + glyphName, " ".join(msg),
			if changed and options.allowChanges:
				fontChanged = 1
			lastHadMsg = 1
		if changed and options.allowChanges:
			originalContours = list(dGlyph)
			fontFile.updateHashEntry(glyphName, changed)
			if options.writeToDefaultLayer:
				fixedGlyph = dGlyph
				fixedGlyph.clearContours()
			else:
				processedLayer.newGlyph(glyphName) # will replace any pre-existing glyph
				fixedGlyph = processedLayer[glyphName]
				fixedGlyph.width = dGlyph.width
				fixedGlyph.height = dGlyph.height
				fixedGlyph.unicodes = dGlyph.unicodes
			pointPen = fixedGlyph.getPointPen()
			newGlyph.drawPoints(pointPen)
			if options.allowDecimalCoords:
				for contour in fixedGlyph:
					for point in contour:
						point.x = round(point.x, 3)
						point.y = round(point.y, 3)
			else:
				for contour in fixedGlyph:
					for point in contour:
						point.x = int(round(point.x))
						point.y = int(round(point.y))
			restoreContourOrder(fixedGlyph, originalContours)
		sys.stdout.flush() # Needed when the script is called from another script with Popen().
	# update layer plist: the hash check call may have deleted processed layer glyphs because the default layer glyph is newer.

	# At this point, we may have deleted glyphs  in the processed layer.writer.getGlyphSet()
	# will fail unless we update the contents.plist file to match.
	if options.allowChanges:
		ufoTools.validateLayers(fontPath, False)

	if not fontChanged:
		print
		fontFile.close() # Even if the program didn't change any glyphs, we should still save updates to the src glyph hash file.
	else:
		print
		fontFile.save()

	if processedGlyphCount != seenGlyphCount:
		print "Skipped %s of %s glyphs." % (seenGlyphCount - processedGlyphCount, seenGlyphCount)
	print "Done with font"
	return

if __name__=='__main__':
	try:
		run(sys.argv[1:])
	except (focusOptionParseError, focusFontError),e :
		print "Quitting after error.", e
		pass
