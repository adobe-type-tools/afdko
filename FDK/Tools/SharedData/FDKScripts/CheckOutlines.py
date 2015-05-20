__copyright__ = """Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
"""


__usage__ = """
CheckOutlines v1.25 May 20 2015

Outline checking program for OpenType/CFF fonts.

Usage: checkOutlines [-h]  [-u] [-he] -g <glyph list>] [-gf <glyphNameList>][-e] [-v] [-s] [-x] [-3] [-4] [-I] [-i] [-O] [-V] [-k] [-C <tol>] [-L <tol>] [-S <tol>] [-K <tol>] [-log <path>] [-o outputFile] input-font-file
Example:  checkOutlines -e -g A-B -o test.otf  MyNewFont.otf
"""

__help__ = __usage__  + """
Report, and optionally fix, problems with glyph outlines. This
program will work with Type 1 fonts and OpenType/CFF fonts. It will
not yet work with TrueType fonts.

In early development, it is a good idea to run this with all the
tests enabled, with the option '-i', and fix all the problems as you
design the glyphs.

However, if you are late in the production cycle and have not been
fixing all the minor problems, then you can suppress the minor
warnings with the option -I; this will report only overlap and
orientation problems, which always need to be fixed.

-u	Print usage

-h	Print help

-he	Print explanation of error messages

-g <glyphID1>,<glyphID2>,...,<glyphIDn>
	Check only the specified list of glyphs. The list must be 
	comma-delimited. The glyph ID's may be glyphID's, glyph names, or 
	glyph CID's. If the latter, the CID value must be prefixed with the 
	string "cid". There must be no white-space in the glyph list.
	Examples:
		checkoutlines -g A,B,C,69 myFont
		checkoutlines -g cid1030,cid34,cid3455,69 myCIDFont

	A range of glyphs may be specified by providing two names separated
	only by a hyphen:
		checkOutlines -g zero-nine,onehalf myFont

	Note that the range will be resolved by expanding the glyph indices
	(GID)'s, not by alphabetic names.

-gf <file name>
	Check only the list of glyphs contained in the specified file, The file 
	must contain a comma-delimited list of glyph identifiers. Any number of 
	space, tab, and new-line characters are permitted between glyph names 
	and commas.

-e	Make changes
	Default is to just report findings, and to not change the glyphs.
	This tool is good at fixing overlaps and path orientation, but the other 
	fixes are not always what you would like best. In rare cases it will 
	mangle the glyph. Always check any changed glyphs after using this tool 
	to fix problems, and look carefully at the changed points at sufficient 
	magnification hat you can see the control points. Note that this script 
	will change only those problems found by the currently selected tests. 
	Also note that the fixed glyphs will lose all hinting, and must be 
	re-hinted.

-o <path>
	Specify the output file. This has effect only if -e is also specified.
	If -e is specified, and no output path is specified, then the changed
	file is written over the original.

-log <path>
	Specify the path for the log file. The path actually used may have a
	number appended to avoid overwriting an existing log file.

-v	Verbose mode. Reports progress, as well as warnings.

-s	Check curve smoothness: default off.
	This test checks that two curve segments come together such that there 
	is no angle at the join. Another way of saying this is that the two 
	bezier control points that control the angle of the join lie on the 
	same straight line through the node. It is rare that you would want an 
	angle where two curves meet. Fixing this will usually make the curve 
	look better.

-x	Check for sharp/spike angles: default off.
	Very sharp angles will cause strange black spikes at large point sizes 
	in PostScript interpreters. These happen at very specific and 
	unpredictable point sizes, so the problem cannot be seen by trying just 
	a few integer point sizes. The tool will fix this by blunting the point 
	of the angle by cutting off the tip and substituting a very short line.
 
-3	Check for 'triangle' tests: default off.
	This test verifies that both bezier control points lie on the same side 
	of the curve segment they control, and that they do not intersect. 
	Following this rule makes hinting work better: fewer pixel pop-outs and 
	dents at small point sizes.

-4	Do nearly horizontal/vertical test: default off.
	This test reports if a line is so nearly vertical or horizontal that
	the editor probably meant it to be exactly vertical or horizontal.

-O	Skip checking path orientations/directions: default on.
	Bad path orientation and overlap will make the glyph fill with black 
	incorrectly at some point sizes.

-i	Do all checks
	This includes:
	- Check that path has more than 2 nodes. This is always a mistake; 
	  either the path has no internal area, or it has no points at the 
	  extremes. If the path has no internal area, then the tool will 
	  delete it when fixing problems. The tool reports:
	  	"Warning: Subpath with only 2 graphic elements at ..."

	- Check that two successive points are not on top of each other.
	  Always a mistake. The tool reports:
		"Need to remove zero-length element"
	
	- Check for sharp angles. See "-x" above.
	
	- Check that a curve does not form a loop. This is always
	  unintentional, and PS interpreters really don't like it.
	  CheckOutlines does NOT fix these. The tool reports:
		"Need to inspect for possible loop/inflection: "
	
	- Check that the control points are inside the region defined by 
	  the endpoints. Draw a line between the two end-points of a curve.
	  Then draw a perpendicular line from each end-point towards the
	  outside of the curve. The control points should lie within the
	  region defined by the three lines. If not, then at best there is
	  one or more vertical or horizontal extremes that do not have a
	  node, and these will cause one-pixel pop-outs or dents in the
	  outline at small point sizes. At worst, you have accidentally
	  created a control point that points back in the direction from
	  which the curve came, and you get an ugly spike at large point
	  sizes. The tool is particularly unreliable at fixing these.
	  The tool reports:
		"Need to fix control point outside of endpoints..."
	
	- Check for two successive line segments that could be a single
	  line. Has no effect on rasterization, but makes the glyph more
	  complicated to edit.
	
	- Check for a curve that could be a line. If not perfectly flat,
	  this  cause pixel pop-outs or dents that would not otherwise
	  happen.
	
	- Check if the curve is smooth. See "-s" above.
	
	- Do triangle test. See "-3" above.
	
	- Do nearly horizontal/vertical test. See "-4" above.
	
-I	Skip checks other than overlap, path orientation, and coincident paths.

-V	Skip looking for overlaps/intersections: default on.
	This is useful when intersections are reported, but you don't seen them 
	in the glyph. The error report can also be caused by path elements that 
	form a loop, or a Bezier control point that is aligned with the path 
	element, but pointing in the wrong direction. When overlap removal is 
	on, you will see only a report of an interection; if you turn overlap 
	removal off with -V, then you will see reports of a possible loop, or 
	control point outside of endpoints.

-k	Skip looking for coincident paths: default on.

-C	Specify tolerance for linear curves: default 0.125 units.
	If the depth of the curve segment is less than this tolerance, then 
	it's almost certainly you meant to have a straight line.

-L	Specify tolerance for colinear lines: default 0.01 units
	If two successive straight lines deviate form being parallel by less 
	then this tolerance, then you need only one line segment instead of 
	two. Fix these to make the font a little smaller.

-S	Specify tolerance for arclength-similarity: default 0.055 units.
	If two succeeding curves look alike within this tolerance, you can 
	combine them into a single curve.

-K	Specify tolerance for coincident paths: default 0.909 coverage.
	If two lines lie within this tolerance, then they will look like they 
	are on top of each other. This is almost always a mistake in drawing.
	
-b	Specify em-square size. This overrides the font's em-square. This is used
	to check for unintended points and paths by looking for paths which are
	entirely outside the rectangle: lowere left = (x = -0.5*em-square, y = -0.5*em-square)
	and upper right = (x = 1.5*em-square, y = 1.5*em-square).
	
-all Force all glyphs to be processed. This applies only to UFO fonts, where processed glyphs
	are saved in a layer, and processing of a glyph is skip[ped if has already been processed.
"""

error_doc = """
The following is list of the checkOutlines error messages, and a brief
explanation of each. The error messages here may be just the first part
of the actual message, with additional info and coordinates given in the
rest of the actual message.

The phrase 'subpath' means what in FontLab is a contour: a complete
closed line.

The phrase 'path element' means a line segment from one on-curve node to the
next on a contour.

The phrase 'point' means an on-curve node.

The phrase 'control point' means an off-curve bezier curve control
handle end-point.


"Near-miss on orthogonal transition between (path segment1) and
(pathsegment2)"
	These two path segments are so nearly at right angles, that I
	suspect that they were intended to be a right angle.
    
"Need to delete incomplete subpath at (path segment)"
	This contour doesn't have enough points to make a closed contour.

Need to delete orphan subpath with moveto at (path segment)"
	This contour is outside the font BBox, and is therefore was probably
	drawn accidentally.
    
"May need to insert extremum in element (path segment)"
	Looks like the path element contains a horizontal or vertical
	extreme; if you want this hinted, you need a control point there.
    
"Need to remove double-sharp element"
"Need to clip sharp angle between two path segments)"
"Extremely sharp angle between (two path segments)"
	Very sharp angles can cause PostScript interpreters to draw long
	spikes. Best to blunt these with a short line-segment across the end
	of the angle.
    
"Need to convert straight curve"
	This path element is a curve type, but is straight - might as well 
	make it a line.

"Need to delete tiny subpath at (path segment)"
	This contour is so small, it must be have been drawn accidentally.
    
"Need to fix coincident control points"
	Two control points lie on top of each other. This can act like an
	intersection.
 
"Need to fix control point outside of endpoints"
"Need to fix point(s) outside triangle by  %d units in X and %d units in Y"
	The Bezier control points for a curve segment must lie on the same
	side of the curve segment, and within lines orthogonal to the curve
	segment at the end points. When this is not the case, there are
	vertical and horizontal extremes with no points. This is
	often the result of an accident while drawing.
   
"Need to fix wrong orientation on subpath with original moveto at
	Direction of subpath is wrong in this contour.
    
"Need to inspect coincident paths: with original moveto at ..."
	Two line or curve segments either coincide, or are so close they
	will look like they coincide. checkOutline will generate false
	positives if both paths have the same bounding box; for example, if
	an 'x' is drawn with two symmetrical bars crossing each other, they
	will have the same bounding box, and checkOutlines will report them
	as being coincident paths.

"Need to inspect for possible loop/inflection (path segment)"
	This looks like a single path element that makes a loop. If so, it is very
	bad for rasterizers.

"Need to inspect unsmoothed transition between (two path segments)"
	There is an angle between two path elements which is so shallow that
	it was probably meant to be smooth.
    
"Need to join colinear lines"
	Two straight lines are lined up, or very nearly so. These
	should be replaced by a single line segment

"Need to remove one-unit closepath"
	A one-unit-long line is silly. Make the previous segment extend to
	the end point. No important consequences.
    
"Need to remove zero-length element"
	Two neighboring points lie on top of each other. This
	confuses rasterizers. Get rid of one.
    
"Warning: (number) intersections found. Please inspect."
	Two path elements cross. This must be fixed.
	This is usually caused by two subpaths that intersect, but it can
	also be caused by a single path element that forms a loop. To find the
	latter, run checkOutlines with the -V option, and look for errors that
	did not appear when intersections were being removed.

"Outline's bounding-box (X:%d %d Y:%d %d) looks too large."
	Bounding box of this contour exceeds the font-bounding box, so there
	is probably some error.
    
"Warning: path #%d with original moveto at %d %d has no extreme-points
	on its curveto's. Does it need to be curvefit? Please inspect",
	This contour does not have points at all of its vertical and
	horizontal extremes.
    
"Warning: Subpath with only 2 graphic elements at (path segment)"
	This contour has only two nodes; it is either two superimposed line
	segments, or at least one curve without any point at extremes.
"""
# Methods:

# Parse args. If glyphlist is from file, read in entire file as single string,
# and remove all whitespace, then parse out glyph-names and GID's.


import sys
import os
import re
import time
from fontTools.ttLib import TTFont, getTableModule
import traceback
import tempfile
import FDKUtils
from BezTools import *
import ufoTools
import shutil

haveFocus = 1
debug = 0
kDefaultLogFileName = "checkOutlines.log"
gLogFile = None
kSrcGLIFHashMap = "com.adobe.type.checkOutlinesHashMap"

kTempFilepath = tempfile.mktemp()
kTempFilepathOut = kTempFilepath + ".new"
kTempCFFSuffix = ".temp.ac.cff"

class focusOptions:
	def __init__(self):
		self.filePath = None
		self.outFilePath = None
		self.logFilePath = None
		self.glyphList = []
		self.allowChanges = 0
		self.beVerbose = 0
		self.doOverlapCheck = 1
		self.skipInspectionTests = 0
		self.doSmoothnessTest = 0
		self.doSpikeTest = 0
		self.doTriangleTest = 0
		self.doNearlyVH = 0
		self.doPathDirectionTest = 1
		self.doCoincidentPathTest = 1
		self.curveTolerance = ""
		self.lineTolerance = ""
		self.arcTolerance = ""
		self.pathTolerance = ""
		self.emSquare = ""
		self.checkAll = False # forces all glyphs to be processed even if src hasn't changed.
		self.allowDecimalCoords = 0


class FDKEnvironmentError(AttributeError):
	pass

class focusOptionParseError(KeyError):
	pass

class focusFontError(KeyError):
	pass

kProgressChar = "."


		
def logMsg(*args):
	for arg in args:
		msg = str(arg).strip()
		if not msg:
			print
			sys.stdout.flush()
			if gLogFile:
				gLogFile.write(os.linesep)
				gLogFile.flush()
			return
			
		msg = re.sub(r"[\r\n]", " ", msg)
		if msg[-1] == ",":
			msg = msg[:-1]
			if msg == kProgressChar:
				sys.stdout.write(msg) # avoid space, which is added by 'print'
			print msg,
			sys.stdout.flush()
			if gLogFile:
				gLogFile.write(msg)
				gLogFile.flush()
		else:
			print msg
			sys.stdout.flush()
			if gLogFile:
				gLogFile.write(msg + os.linesep)
				gLogFile.flush()

def CheckEnvironment():
	txPath = 'tx'
	txError = 0
	command = "%s -u 2>&1" % (txPath)
	report = FDKUtils.runShellCmd(command)
	if "options" not in report:
			txError = 1
	
	if  txError:
		logMsg("Please re-install the FDK. The path to the program 'tx' is not in the environment variable PATH.")
		raise FDKEnvironmentError

	command = "checkoutlinesexe -u 2>&1"
	report = FDKUtils.runShellCmd(command)
	if "version" not in report:
		logMsg("Please re-install the FDK. The path to the program 'checkoutlinesexe' is not in the environment variable PATH.")
		raise FDKEnvironmentError

	return

global nameAliasDict
nameAliasDict = {}

def aliasName(glyphName):
	if nameAliasDict:
		alias = nameAliasDict.get(glyphName, glyphName)
		return alias
	return glyphName


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

def parseGlyphListArg(glyphString):
	glyphString = re.sub(r"[ \t\r\n,]+",  ",",  glyphString)
	glyphList = glyphString.split(",")
	glyphList = map(expandNames, glyphList)
	glyphList =  filter(None, glyphList)
	return glyphList


def getOptions():
	global debug
	
	options = focusOptions()
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
		elif arg == "-he":
			print error_doc
			sys.exit()
		elif arg == "-d":
			debug = 1
		elif arg == "-decimal":
			options.allowDecimalCoords = True
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
				gf.close()
			except (IOError,OSError):
				raise focusOptionParseError("Option Error: could not open glyph list file <%s>." %  filePath) 
			options.glyphList += parseGlyphListArg(glyphString)
		elif arg == "-e":
			options.allowChanges = 1
		elif arg == "-v":
			options.beVerbose = 1
		elif arg == "-V":
			options.doOverlapCheck = 0
		elif arg == "-I":
			options.skipInspectionTests = 1
		elif arg == "-i":
			options.skipInspectionTests = 0
			options.doSmoothnessTest = 1
			options.doSpikeTest = 1
	 		options.doTriangleTest = 1
	 		options.doNearlyVH = 1
		elif arg == "-s":
			options.doSmoothnessTest = 1
		elif arg == "-x":
			options.doSpikeTest = 1
		elif arg == "-3":
			options.doTriangleTest = 1
		elif arg == "-4":
			options.doNearlyVH = 1
		elif arg == "-O":
			options.doPathDirectionTest = 0
		elif arg == "-k":
			options.doCoincidentPathTest = 0
		elif arg == "-C":
			i = i + 1
			options.curveTolerance = sys.argv[i]
		elif arg == "-L":
			i = i + 1
			options.lineTolerance = sys.argv[i]
		elif arg == "-S":
			i = i + 1
			options.arcTolerance = sys.argv[i]
		elif arg == "-K":
			i = i + 1
			options.pathTolerance = sys.argv[i]
		elif arg == "-b":
			i = i + 1
			options.emSquare = sys.argv[i]
		elif arg == "-o":
			i = i + 1
			options.outFilePath = sys.argv[i]
		elif arg == "-log":
			i = i + 1
			options.logFilePath = sys.argv[i]
		elif arg == "-all":
			options.checkAll = True
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


def buildArgString(options):
	arg_string = ""
	if  options.allowChanges:
		arg_string = arg_string + " -n"
	if options.beVerbose:
		arg_string = arg_string + " -v"
	if options.skipInspectionTests:
		arg_string = arg_string + " -I"
	if options.doSmoothnessTest:
		arg_string = arg_string + " -s"
	if options.doSpikeTest:
		arg_string = arg_string + " -x"
	if options.doTriangleTest:
		arg_string = arg_string + " -3"
	if options.doNearlyVH:
		arg_string = arg_string + " -4"
	if not options.doPathDirectionTest:
		arg_string = arg_string + " -O"
	if not options.doOverlapCheck:
		arg_string = arg_string + " -V"
	if not options.doCoincidentPathTest:
		arg_string = arg_string + " -k"
	if options.curveTolerance:
		arg_string = arg_string + " -C " + str(options.curveTolerance)
	if options.lineTolerance:
		arg_string = arg_string + " -L " + str(options.lineTolerance)
	if options.arcTolerance:
		arg_string = arg_string + " -S " + str(options.arcTolerance)
	if options.pathTolerance:
		arg_string = arg_string + " -K " + str(options.pathTolerance)
	if options.emSquare:
		arg_string = arg_string + " -b " + str(options.emSquare)
	return arg_string


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

def getGlyphNames(glyphTag, fontGlyphList, fontFileName):
	glyphNameList = []
	rangeList = glyphTag.split("-")
	prevGID = getGlyphID(rangeList[0], fontGlyphList)
	if prevGID == None:
		if len(rangeList) > 1:
			logMsg( "\tWarning: glyph ID <%s> in range %s from glyph selection list option is not in font. <%s>." % (rangeList[0], glyphTag, fontFileName))
		else:
			logMsg( "\tWarning: glyph ID <%s> from glyph selection list option is not in font. <%s>." % (rangeList[0], fontFileName))
		return None
	glyphNameList.append(fontGlyphList[prevGID])

	for glyphTag2 in rangeList[1:]:
		#import pdb
		#pdb.set_trace()
		gid = getGlyphID(glyphTag2, fontGlyphList)
		if gid == None:
			logMsg( "\tWarning: glyph ID <%s> in range %s from glyph selection list option is not in font. <%s>." % (glyphTag2, glyphTag, fontFileName))
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

def openFile(path, outFilePath,useHashMaps):
	if os.path.isfile(path):
		font =  openOpenTypeFile(path, outFilePath)
	else:
		# maybe it is a a UFO font.
		# We use the hash map to skip previously processed files only if options.allowChanges and not doAll:
		# if we are just reporting issues, we always want to process all glyphs.
		font =  openUFOFile(path, outFilePath, useHashMaps)
	return font

def openUFOFile(path, outFilePath, useHashMaps):
	# Check if has glyphs/contents.plist
	contentsPath = os.path.join(path, "glyphs", "contents.plist")
	if not os.path.exists(contentsPath):
		msg = "Font file must be a PS, CFF, OTF, or ufo font file: %s." % (path)
		logMsg(msg)
		raise focusFontError(msg)

	# If user has specified a path other than the source font path, then copy the entire
	# ufo font, and operate on the copy.
	if (outFilePath != None) and (os.path.abspath(path) != os.path.abspath(outFilePath)):
		msg = "Copying from source UFO font to output UFO font before processing..."
		logMsg(msg)
		if os.path.exists(outFilePath):
			shutil.rmtree(outFilePath)
		shutil.copytree(path , outFilePath)
		path = outFilePath
	font = ufoTools.UFOFontData(path, useHashMaps, ufoTools.kCheckOutlineName)
	font.useProcessedLayer = False # always read glyphs from default layer.
	font.requiredHistory = [] # Programs in this list must be run before autohint, if the outlines have been edited.
	return font
	
def openOpenTypeFile(path, outFilePath):
	# If input font is  CFF or PS, build a dummy ttFont in memory..
	# return ttFont, and flag if is a real OTF font Return flag is 0 if OTF, 1 if CFF, and 2 if PS/
	fontType  = 0 # OTF
	tempPathCFF = path + kTempCFFSuffix
	try:
		ff = file(path, "rb")
		data = ff.read(10)
		ff.close()
	except (IOError, OSError):
		logMsg("Failed to open and read font file %s." % path)

	if data[:4] == "OTTO": # it is an OTF font, can process file directly
		try:
			ttFont = TTFont(path)
		except (IOError, OSError):
			raise focusFontError("Error opening or reading from font file <%s>." % path)
		except TTLibError:
			raise focusFontError("Error parsing font file <%s>." % path)

		try:
			cffTable = ttFont["CFF "]
		except KeyError:
			raise focusFontError("Error: font is not a CFF font <%s>." % fontFileName)

	else:
	
		# It is not an OTF file.
		if (data[0] == '\1') and (data[1] == '\0'): # CFF file
			fontType = 1
			tempPathCFF = path
		elif not "%" in data:
			#not a PS file either
			logMsg("Font file must be a PS, CFF or OTF  fontfile: %s." % path)
			raise focusFontError("Font file must be PS, CFF or OTF file: %s." % path)
	
		else:  # It is a PS file. Convert to CFF.	
			fontType =  2
			print "Converting Type1 font to temp CFF font file..."
			command="tx  -cff +b -std \"%s\" \"%s\" 2>&1" % (path, tempPathCFF)
			report = FDKUtils.runShellCmd(command)
			if "fatal" in report:
				logMsg("Attempted to convert font %s  from PS to a temporary CFF data file." % path)
				logMsg(report)
				raise focusFontError("Failed to convert PS font %s to a temp CFF font." % path)
		
		# now package the CFF font as an OTF font.
		ff = file(tempPathCFF, "rb")
		data = ff.read()
		ff.close()
		try:
			ttFont = TTFont()
			cffModule = getTableModule('CFF ')
			cffTable = cffModule.table_C_F_F_('CFF ')
			ttFont['CFF '] = cffTable
			cffTable.decompile(data, ttFont)
		except:
			logMsg( "\t%s" %(traceback.format_exception_only(sys.exc_type, sys.exc_value)[-1]))
			logMsg("Attempted to read font %s  as CFF." % path)
			raise focusFontError("Error parsing font file <%s>." % path)

	fontData = CFFFontData(ttFont, path, outFilePath, fontType, logMsg)
	return fontData


def checkFile(path, options):
	#    use fontTools library to open font and extract CFF table. 
	#    If error, skip font and report error.
	seenChangedGlyph = 0
	fontFileName = os.path.basename(path)
	logMsg("Checking font %s. Start time: %s." % (path, time.asctime()))
	try:
		fontData = openFile(path, options.outFilePath, options.allowChanges)
		fontData.allowDecimalCoords = options.allowDecimalCoords
	except (IOError, OSError):
		logMsg( traceback.format_exception_only(sys.exc_type, sys.exc_value)[-1])
		raise focusFontError("Error opening or reading from font file <%s>." % fontFileName)
	except:
		logMsg( traceback.format_exception_only(sys.exc_type, sys.exc_value)[-1])
		raise focusFontError("Error parsing font file <%s>." % fontFileName)

	#   filter specified list, if any, with font list.
	fontGlyphList = fontData.getGlyphList()
	glyphList = filterGlyphList(options, fontGlyphList, fontFileName)
	if not glyphList:
		raise focusFontError("Error: selected glyph list is empty for font <%s>." % fontFileName)


	removeHints = 1
	reportCB = logMsg
	reportText = ""

	# If the user has not specified an em-square, and the font has am em-square other than 1000, supply it.
	if not options.emSquare:
		emSquare = fontData.getUnitsPerEm()
		if emSquare != "1000":
			options.emSquare = emSquare
			
	arg_string = buildArgString(options)
		
	dotCount = 0		
	seenGlyphCount = 0
	processedGlyphCount = 0
	for name in glyphList:
		seenGlyphCount +=1 
		if options.beVerbose:
			logMsg("Checking %s -- ," % ( aliasName(name) )) # output message when -v option is used
		else:
			logMsg(".,")
			dotCount += 1
			if dotCount > 40:
				dotCount = 0
				logMsg("") # I do this to never have more than 40 dots on a line.
				# This in turn give reasonable performance when calling checkOutlines in a subprocess
				# and getting output with std.readline()

		if os.path.exists(kTempFilepathOut):
			os.remove(kTempFilepathOut)
		
		# 	Convert to bez format
		bezString, width = fontData.convertToBez(name, removeHints, options.beVerbose, options.checkAll)
		if bezString == None:
			continue
		processedGlyphCount += 1

		if "mt" not in bezString:
			# skip empty glyphs.
			continue
			
		# write bez file and run checkoutlinesexe
		fp = open(kTempFilepath, "wt")
		fp.write(bezString)
		fp.close()
		command = "checkoutlinesexe -o %s %s" % (arg_string, kTempFilepath)
		if debug:
			print "calling command", command
		log = FDKUtils.runShellCmd(command)
		# The suffix saying what bez file was written isn't useful here.
		log = re.sub(r"Wrote fixed file.+\s*", "", log)
		if log:
			if not options.beVerbose:
				dotCount = 0
				logMsg("")
				logMsg("Checking %s -- ," % (aliasName(name))) # output message when -v option is NOT used
				logMsg(log)
			else:
				logMsg(log)
		if  options.allowChanges and os.path.exists(kTempFilepathOut):
			fp = open(kTempFilepathOut, "rt")
			bezData = fp.read()
			fp.close()
			if bezData != bezString:
				fontData.updateFromBez(bezData, name, width, options.beVerbose)
				seenChangedGlyph = 1

	if not options.beVerbose:
		logMsg("")
	if seenChangedGlyph:
		# save fontFile.
		fontData.saveChanges()
	else:
		fontData.close()

	if processedGlyphCount != seenGlyphCount:
		logMsg("Skipped %s of %s glyphs." % (seenGlyphCount - processedGlyphCount, seenGlyphCount))
	logMsg("Done with font %s. End time: %s." % (path, time.asctime()))
	if debug:
		print "Temp bez file in", kTempFilepath
		print "Temp bez file out", kTempFilepathOut
	else:
		if os.path.exists(kTempFilepathOut):
			os.remove(kTempFilepathOut)
		if os.path.exists(kTempFilepath):
			os.remove(kTempFilepath)
		# remove temp file left over from openFile.
		tempPathCFF = path + kTempCFFSuffix
		if os.path.exists(tempPathCFF):
			os.remove(tempPathCFF)

def openLogFile(userLogFilePath, fontFilePath):
	logFile = None
	logFilePath = None
	if userLogFilePath:
		logFilePath = userLogFilePath
	else:
		logFilePath = kDefaultLogFileName
		fontDir = os.path.dirname(fontFilePath)
		if fontDir:
			logFilePath = os.path.join(fontDir, logFilePath)
				
	# Get an unused name.
	tries = 1
	newPath = logFilePath
	while tries < 1000:
		fileExists = os.path.exists(newPath)
		if not fileExists:
			logFilePath = newPath
			break
		newPath = "%s.%03d" % (logFilePath, tries)
		tries +=1
		
	if fileExists:
		logMsg("Failed to open log file %s; couldn't find an un-used file name with a lower suffix. Report will be written only to screen." % (newPath))
		logFilePath = None
		return logFile, logFilePath
			
	try:
		logFile = open(logFilePath, "wt")
	except (IOError, OSError):
		logFile = None
		logMsg( traceback.format_exception_only(sys.exc_type, sys.exc_value)[-1])
		logMsg("Failed to open log file %s. Report will be written only to screen." % (logFilePath))
	return logFile, logFilePath


def main():
	global gLogFile
	try:
		CheckEnvironment()
	except FDKEnvironmentError:
		return
	try:
		options = getOptions()
	except focusOptionParseError:
		logMsg( traceback.format_exception_only(sys.exc_type, sys.exc_value)[-1])
		return

	# verify that all files exist.
	if not os.path.exists(options.filePath):
		logMsg("File does not exist: <%s>." % options.filePath)
	else:
		if options.logFilePath:
			gLogFile, logFilePath = openLogFile(options.logFilePath, options.filePath)
		try:
			checkFile(options.filePath, options)
		except (focusFontError):
			logMsg("\t%s" %(traceback.format_exception_only(sys.exc_type, sys.exc_value)[-1]))
		if gLogFile:
			gLogFile.close()
			gLogFile = None
			logMsg("Log file written to %s" % (logFilePath))
	return


if __name__=='__main__':
	main()
	
	
