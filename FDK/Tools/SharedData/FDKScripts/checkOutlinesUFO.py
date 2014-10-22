__copyright__ = """Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
"""

__doc__ = """
"""
__usage__ = """
   checkOutlinesUFO program v1.01 Sep 22 2014
   
   checkOutlinesUFO [-r] [-e]  [-d] [-g glyphList] [-gx glyphList]
 """

kProcessedGlyphLayerName = "com.adobe.type.processedglyphs"

__help__ = __usage__ + """

-nr	do not round point values to integer

-e	remove overlaps

-d	write changed glyphs to default layer instead of '%s'

-g <glyphID1>,<glyphID2>,...,<glyphIDn>
	Check only the specified list of glyphs. The list must be 
	comma-delimited. The glyph ID's may be glyphID's, glyph names, or 
	glyph CID's. If the latter, the CID value must be prefixed with the 
	string "cid". There must be no white-space in the glyph list.
	Examples:
		checkoutlines -g A,B,C,69 myFont
		checkoutlines -g cid1030,cid34,cid3455,69 myCIDFont

-gf <file name>
	Check only the list of glyphs contained in the specified file, The file 
	must contain a comma-delimited list of glyph identifiers. Any number of 
	space, tab, and new-line characters are permitted between glyph names 
	and commas.

-e	Make changes
	Default is to just report findings, and to not change the glyphs.

""" % kProcessedGlyphLayerName

import sys
import os
import re
import defcon
import booleanOperations.booleanGlyph
from robofab.pens.digestPen import DigestPointPen

class focusOptionParseError(KeyError):
	pass

class focusFontError(KeyError):
	pass

class COOptions:
	def __init__(self):
		self.filePath = None
		self.outFilePath = None
		self.logFilePath = None
		self.glyphList = []
		self.allowChanges = 0
		self.doOverlapCheck = 1
		self.writeToDefaultLayer = 0
		self.roundValues = 1
		self.skipIfUnchanged = False
		self.checkAll = False # overrides skipIfUnchanged: forces all glyphs to be processed even if src hasn't changed.

	
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

		if arg == "-u":
			print __usage__
			sys.exit()
		elif arg == "-h":
			print __help__
			sys.exit()
		elif arg == "-d":
			writeToDefaultLayer = 1
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
			skipIfUnchanged = True
		elif arg == "-nr":
			options.roundValues = 0
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
	return mp.getDigestPointsOnly(needSort=True)

def cleanupOutlines(bGlyph):
	""" Remove coincident points. 
	# a point is (segmentType, pt, smooth, name).
	# e.g. (u'curve', (138, -92), False, None)
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
			
			

def splitTouchingPaths(newGlyph):
	""" This hack fixes a design difference between the Adobe checkOutlines logic and booleanGlyph.
	In this case, if only a single point on a contour lines is coincident with the path of the another contour,
	the paths are NOT merged. In the later, they are merged. An example is the vertex of a diamond shape having the same 
	y coordinate as a horizontal line in another path, but no other overlap. 
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
		while j < lenSortList:
			coords1, j1 =  sortlist[j][0][1], sortlist[j][1]
			coords0, j0 = sortlist[j-1][0][1], sortlist[j-1][1]
			if ((j1 - j0) > 1) and (coords0 == coords1):
				newContourPts = contour._points[j0:j1+1]
				np0 = newContourPts[0]
				if np0[0] == 'curve':
					newContourPts[0] = ('line', np0[1], np0[2], np0[3]) 
				newContour = booleanOperations.booleanGlyph.BooleanContour()
				newContour._points = newContourPts
				newContour._clockwise = newContour._get_clockwise()
				#print "coords1, j1", coords1, j1
				#print "coords0, j0 ", coords0, j0
				#print "coords0, j0 ", coords0, j0
				#print "newContour._clockwise", repr(newContour._clockwise)
				if newContour.clockwise == contour.clockwise:
					#print "un-merging", repr(newContour._clockwise)
					del contour._points[j0+1:j1+1]
					# is a tiny path that should get deleted?
					if not isTinyContour(newContour):
						newGlyph.contours.insert(i+1, newContour)
						j +=1
			j +=1
		
		i += 1

def isTinyContour(contour):
	# The Adobe checkOutlinesremoves tiny subpaths create when the start and end path segments cross each other, rather than meet.
	isTiny = 0
	
	numPts = len(contour)
	if numPts > 4:
		return isTiny
		
	j = 1
	while j < numPts:
		coords1 = contour._points[j][1]
		coords0 = contour._points[j-1][1]
		j += 1
		
		if abs(coords1[0] - coords0[0]) >= 5:
			continue
		if abs(coords1[1] - coords0[1]) >= 5:
			continue
		isTiny = 1
		break
	return isTiny


def run(args):
	options = getOptions()
	fontPath = os.path.abspath(options.filePath)
	dFont = defcon.Font(fontPath)
	glyphCount = len(dFont)
	changed = 0
	glyphList = filterGlyphList(options, dFont.keys(), options.filePath)
	if not glyphList:
		raise focusFontError("Error: selected glyph list is empty for font <%s>." % options.filePath)

	if not options.writeToDefaultLayer:
		try:
			processedLayer = dFont.layers[kProcessedGlyphLayerName]
		except KeyError:
			processedLayer = dFont.newLayer(kProcessedGlyphLayerName)
		
	changed = 0
	for glyphName in glyphList:
		dGlyph = dFont[glyphName]
		if dGlyph.components:
			dGlyph.decomposeAllComponents()
		bGlyph = booleanOperations.booleanGlyph.BooleanGlyph(dGlyph)
		cleanupOutlines(bGlyph) # The processing step removes coincident points and co-linear lines.'
		# I need to fix these in the source, or the old vs new digests wil differ.
		olddg = getDigest(bGlyph)
		newPathNum = -1
		prevPathNum = 0
		newGlyph = bGlyph
		while newPathNum != prevPathNum:
			# This hack to get around a bug in booleanGlyph. Consider an M sitting on a separate rectangular crossbar contour,
			# the bottom of the M legs being co-linear with the top of the cross bar. pyClipper will merge only one of the 
			# co-linear lines with each callto removeOverlap(). I suspect that this bug is in pyClipper, but haven't yet looked.
			prevPathNum = len(newGlyph.contours)
			newGlyph = newGlyph.removeOverlap()
			newPathNum = len(newGlyph.contours)
		newdg = getDigest(newGlyph)
		if str(olddg) != str(newdg):
			if not options.allowChanges:
				print "There is an overlap in: %s" % glyphName
				continue
			splitTouchingPaths(newGlyph)
			
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
			if options.roundValues:
				for contour in fixedGlyph:
					for point in contour:
						point.x = round(point.x)
						point.y = round(point.y)
			changed = 1
			print "Removed overlap in: %s" % glyphName
	if changed:
		print "Saving font..."
		dFont.save(formatVersion=3)
		# Now patch the font version to 2 so that Robofont will open it.
		miPath = os.path.join(fontPath, "metainfo.plist")
		f = open(miPath, "rt")
		data = f.read()
		f.close()
		data = re.sub(r"(?P<CAPS><key>formatVersion</key>\s+<integer>)\d+", r"\g<CAPS>2", data)
		f = open(miPath, "wt")
		f.write(data)
		f.close()
		

	return

if __name__=='__main__':
	try:
		run(sys.argv[1:])
	except (focusOptionParseError, focusFontError),e :
		print "Quitting after error.", e
		pass
	
	
