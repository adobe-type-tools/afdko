#FLM: Outline Check
# Reports on, and optionionally attempts to fix possible outline problems
__copyright__ =  """
Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0.
"""

__doc__ = """
CheckOutlines v1.9 Oct 20 2014

This script will apply the Adobe 'focus' checks to the specified glyphs.
It can both report errors, and attempt to fix some of them.

You should always run this script with the "Check Overlaps" and "Check
Coincident Paths" options turned on. If present, these are always
errors, and this library will fix them reliably. However, you should
always check the changes; you may prefer to fix them differently.

If you ask that problems be fixed, the report will note which issues are fixed
by appending "Done" to the error message line.

The other tests are useful, but the issues reported can be present
intentionally in some fonts. Woodcuts and other fonts with short paths
and sharp angles can generate many messages. Also, you should
allow this tool to fix these problems just one glyph at a time, and make
sure you like the changes: the fixes are far less reliable, and will in
some cases mangle the glyph.

Inspect Smoothness. Default off. This test checks that two curve
	segments come together such that there is no angle at the join.
	Another way of saying this is that the two bezier control points
	that control the angle of the join lie on the same straight line
	through the node. It is rare that you would want an angle where two
	curves meet. Fixing this will usually make the curve look better

Inspect Sharp Angles/Spikes. Default off. Very sharp angles will cause
	strange black spikes at large point sizes in PostScript
	interpreters.  These happen at very specific and unpredictable point
	sizes, so the problem cannot be seen by trying just a few integer
	point sizes. The tool will fix this by blunting the point of the
	angle by cutting off the tip and substituting a very short line.

Inspect Small Triangles. Default off. This test verifies that both
	bezier control points lie on the same side of the curve segment they
	control, and that they do not intersect. Following this rule makes
	hinting work better: fewer pixel pop-outs and dents at small point
	sizes.

Inspect Path Direction. Default  is check on, Bad path orientation and
	overlap will make the glyph fill with black incorrectly at some
	point sizes.

Do All Inspection Tests: This includes:

Check that path has more than 2 nodes. This is always a mistake; either
	the path has no internal area, or it has no points at the extremes.
	If the path has no internal area, then the tool will delete it when
	fixing problems. The tool reports:
	"Warning: Subpath with only 2 graphic elements at ..."
		
Check that two successive points are not on top of each other. Always a
	mistake. The tool reports:
	"Need to remove zero-length element"
		
Check for sharp angles. See "-x" above.
		
Check that that a curve does not form a loop. This is always
	unintentional, and PS interpreters really don't like it. The tool
	reports:
	"Need to inspect for possible loop/inflection: "
		
Check if ""Need to fix control point outside of endpoints..." This tests
	that the control points are inside the safe region for drawing. Draw
	a line between the two end-points of a curve. Then draw a
	perpendicular line from each end-point towards the outside of the
	curve. The control points should lie within the region defined by the
	three lines. If not, then at best there is one or more vertical or
	horizontal extremes that do not have a node, and these will cause
	one-pixel pop-outs or dents in the outline at small point sizes. At
	worst, you have accidentally created a control point that points back in
	the direction from which the curve came, and you get an ugly spike
	at large point sizes. The tool does NOT fix these.
		
Check for two successive line segments that could be a single line. Has
	no effect on rasterization, but makes the glyph more complicated to
	edit.
		
Check for a curve that could be a line. If not perfectly flat, this 
	cause pixel pop-outs or dents that would not otherwise happen.
		
Check if the curve is smooth: see test "-s" above.

Do triangle test: see option -3 above.
        	

Tolerance for colinear lines (default  0.01 units). If two successive
	staright lines deviate from being parallel by less then this
	tolerance, then you need only one line segment instead of two. Fix
	these to make the font a little smaller.

Tolerance for arclength-similarity (default 0.055 units). If two
	succeeding curves look alike within this tolerance, you can combine
	them into a single curve.

Tolerance for coincident paths (default 0.909 coverage) If two lines lie
	within this tolerance, then they will look like they are on top of
	each other. This is almost always a mistake in drawing.

"""

error_doc = """
The following is list of the checkOutlines error messages, and a brief
explanation of each. The error messages here may be just the first part
of the actual message, with additional info and cordinates given in the
rest of the actual message.

"Near-miss on orthogonal transition between (path segment1) and
(pathsegment2)"
    These two path segments are so nearly at right angles, that I
    suspect that they were intended to be a right angle.
    
"Need to delete incomplete subpath at (path segment)"
    This contour doesn't have enough points to make a closed path

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
    of the angle..
    
"Need to convert straight curve"
    This path is a curve type, but is straight - might as well make it a
    line.

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
    vertical and horizontal extremes with no control points. This is
    often the result of an accident while drawing.
   
"Need to fix wrong orientation on subpath with original moveto at
    Direction of path is wrong in this contour.
    
"Need to inspect coincident paths: with original moveto at ..."
    Two line or curve segments either coincide, or are so close they
    will look like they coincide.

"Need to inspect for possible loop/inflection (path segment)"
    This looks like a single path that makes a loop. If so, it is very
    bad for rasterizers.

"Need to inspect unsmoothed transition between (two path segments)"
    There is an angle between two path segments which is so shallow that
    it was probably meant to be smooth.
    
"Need to join colinear lines"
    Two straight lines are lined up, or very nearly so. These
     should be replaced by a single line segment

"Need to remove one-unit closepath"
    A one-unit-long line is silly. Make the previous segment extend to
    the end point. No important consequences.
    
"Need to remove zero-length element"
    Two neighboring control points lie on top of each other. This
    confuses rasterizers. Get rid of one.
    
"NOTE: (number) intersections found. Please inspect."
    Two paths cross. This must be fixed.

"Outline's bounding-box (X:%d %d Y:%d %d) looks too large."
    Bounding box of this contour exceeds the font-bounding box, so there
    is probably some error.
    
"Warning: path #%d with original moveto at %d %d has no extreme-points
on its curveto's. Does it need to be curvefit? Please inspect",
    This contour does not have control points at all of its vertical and
    horizontal extremes.
    
"Warning: Subpath with only 2 graphic elements at (path segment)"
    This contour has only two nodes; it is either two superimposed line
    segments, or at least one curve without any point at extremes.
"""

import string
import sys
import os
import re
from FL import *
try:
	import BezChar
	from AdobeFontLabUtils import Reporter, setFDKToolsPath, checkControlKeyPress, checkShiftKeyPress
except ImportError,e:
	print "Failed to find the Adobe FDK support scripts AdobeFontLabUtils.py and BezChar.py."
	print "Please run the script FDK/Tools/FontLab/installFontLabMacros.py script, and try again." 
	print " Current directory:", os.path.abspath(os.getcwd())
	print "Current list of search paths for modules:"
	import pprint
	pprint.pprint(sys.path)
	raise e
import warnings
warnings.simplefilter("ignore", RuntimeWarning) # supress waring about use of os.tempnam().


if os.name == "nt":
	coPath = setFDKToolsPath("checkoutlinesexe.exe")
	if not coPath:
		coPath = setFDKToolsPath("outlineCheck.exe")
		
else:
	coPath = setFDKToolsPath("checkoutlinesexe")
	if not coPath:
		coPath = setFDKToolsPath("outlineCheck")



gLogReporter = None # log file class instance.
debug = 0
kProgressBarThreshold = 8 # I bother witha progress bar just so the user can easily cancel without using CTRL-C
kProgressBarTickStep = 4
logFileName = "CheckOutlines.log" #  Is written to "log" subdirectory from current font.
kPrefsName = "CheckOutline.prefs"

def reportCB(*args):
	for arg in args:
		print "\t\t" + str(arg)

def logMsg(*args):
	global gLogReporter
	# used for printing output to console as well as log file.
	if not gLogReporter:
		for arg in args:
			print arg
	else:
		if gLogReporter.file:
			gLogReporter.write(*args)
		else:
			for arg in args:
				print arg
			gLogReporter = None


def debugLogMsg(*args):
	if debug:
		logMsg(*args)


class CheckOutlineOptions:
	# Holds the options for the module.
	# The values of all member items NOT prefixed with "_" are written to/read from
	# a preferences file.
	# This also gets/sets the same member fields in the passed object.
	def __init__(self):
		# Selection control fields
		self.CmdList1 = ["doWholeFamily", "doAllOpenFonts", "doCurrentFont"] 
		self.CmdList2 = ["doAllGlyphs", "doSelectedGlyphs"]
		self.doWholeFamily = 0
		self.doAllOpenFonts = 0
		self.doCurrentFont = 1
		self.doAllGlyphs = 0
		self.doSelectedGlyphs = 1
		
		# Test control fields
		self.beVerbose = 0
		self.doOverlapCheck = 1
		self.doCoincidentPathTest = 1
		self.skipInspectionTests = 1
		self.doAllInspectionTests = 0
		self.doSmoothnessTest = 0
		self.doSpikeTest = 0
		self.doTriangleTest = 0
		self.doPathDirectionTest = 1
		self.doFixProblems = 0
		self.curveTolerance = ""
		self.lineTolerance = ""
		self.pathTolerance = ""
		self.debug = 0

		# items not written to prefs
		self._prefsBaseName = kPrefsName
		self._prefsPath = None

	def _getPrefs(self, callerObject = None):
		foundPrefsFile = 0

		# We will put the prefs file in a directory "Preferences" at the same level as the Macros directory
		dirPath = os.path.dirname(BezChar.__file__)
		name = " "
		while name and (name.lower() != "macros"):
			name = os.path.basename(dirPath)
			dirPath = os.path.dirname(dirPath)
		if name.lower() != "macros" :
			dirPath = None

		if dirPath:
			dirPath = os.path.join(dirPath, "Preferences")
			if not os.path.exists(dirPath): # create it so we can save a prefs file there later.
				try:
					os.mkdir(dirPath)
				except (IOError,OSError):
					logMsg("Failed to create prefs directory %s" % (dirPath))
					return foundPrefsFile
		else:
			return foundPrefsFile

		# the prefs directory exists. Try and open the file.
		self._prefsPath = os.path.join(dirPath, self._prefsBaseName)
		if os.path.exists(self._prefsPath):
			try:
				pf = file(self._prefsPath, "rt")
				data = pf.read()
				prefs = eval(data)
				pf.close()
			except (IOError, OSError):
				logMsg("Prefs file exists but cannot be read %s" % (self._prefsPath))
				return foundPrefsFile

			# We've successfully read the prefs file
			foundPrefsFile = 1
			kelList = prefs.keys()
			for key in kelList:
				exec("self.%s = prefs[\"%s\"]" % (key,key))

		# Add/set the memebr fields of the calling object
		if callerObject:	
			keyList = dir(self)
			for key in keyList:
				if key[0] == "_":
					continue
				exec("callerObject.%s = self.%s" % (key, key))

		return foundPrefsFile


	def _savePrefs(self, callerObject = None):
		prefs = {}
		if not self._prefsPath:
			return

		keyList = dir(self)
		for key in keyList:
			if key[0] == "_":
				continue
			if callerObject:
				exec("self.%s = callerObject.%s" % (key, key))
			exec("prefs[\"%s\"] = self.%s" % (key, key))
		try:
			pf = file(self._prefsPath, "wt")
			pf.write(repr(prefs))
			pf.close()
			logMsg("Saved prefs in %s." % self._prefsPath)
		except (IOError, OSError):
			logMsg("Failed to write prefs file in %s." % self._prefsPath)

def doCheck(options):
	global gLogReporter
	arg_string = ""
	if options.doFixProblems:
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
	if options.pathTolerance:
		arg_string = arg_string + " -K " + str(options.pathTolerance)

	if options.doAllOpenFonts:
		numOpenFonts = fl.count
	elif options.doWholeFamily:
		# close all fonts not in the family; open the ones that are missing.
		numOpenFonts = fl.count
	elif options.doCurrentFont:
		numOpenFonts = 1
	else:
		print "Error: unsupported option for font selection."

	fileName = fl.font.file_name
	if not fileName:
		fileName = "Font-Undefined"
	logDir = os.path.dirname(fileName)
	gLogReporter = Reporter(os.path.join(logDir, logFileName))

	tempBaseName = os.tempnam()
	options.tempBez = tempBaseName + ".bez"
		
	# set up progress bar
	if options.doAllGlyphs:
		numGlyphs = len(fl.font)
	else:
		numGlyphs = fl.count_selected
	numProcessedGlyphs = numGlyphs * numOpenFonts
	if numProcessedGlyphs > kProgressBarThreshold:
		fl.BeginProgress("Checking glyphs...", numProcessedGlyphs)
	tick = 0
	if options.doAllGlyphs:
		for fi in range(numOpenFonts):
			if numOpenFonts == 1:
				font = fl.font
			else:
				font = fl[fi]

			logMsg("Checking %s." % (font.font_name))
			lenFont = len(font)
			for gi in range(lenFont):
				glyph = font[gi]
				CheckGlyph(options, glyph, arg_string)
				if numGlyphs > kProgressBarThreshold:
					tick = tick + 1
					if (tick % kProgressBarTickStep == 0):
						result = fl.TickProgress(tick)
						if not result:
							break
				CheckGlyph(options, glyph, arg_string)
			
	elif options.doSelectedGlyphs:
		if numOpenFonts == 1: # we can use the current  selection
			font = fl.font
			lenFont =  len(font)
			logMsg("Checking %s." % (font.font_name))
			for gi in range(lenFont):
				if fl.Selected(gi):
					glyph = font[gi]
					if numGlyphs > kProgressBarThreshold:
						tick = tick + 1
						if (tick % kProgressBarTickStep == 0):
							result = fl.TickProgress(tick)
							if not result:
								break
					CheckGlyph(options, glyph, arg_string)

		else: # we can't assume that GI's are the same in every font. 
			# Collect the selected glyph names from the current font,
			# and then index in all fonts by name.
			nameList = []
			for gi in range(lenFont):
				if fl.Selected(gi):
					nameList.append(fl.font[gi].name)
			for fi in range(numOpenFonts):
				font = fl[fi]
				logMsg("Checking %s." % (font.font_name))
				lenFont = len(font)
				for gname in nameList:
					gi = font.FindGlyph(gname)
					if gi > -1:
						glyph = font[gi]
						if numGlyphs > kProgressBarThreshold:
							tick = tick + 1
							if (tick % kProgressBarTickStep == 0):
								result = fl.TickProgress(tick)
								if not result:
									break
						CheckGlyph(options, glyph, arg_string)
					else:
						line = "Glyph in not in font"
						logMsg("\t" + line + os.linesep)
			 
	else:
		print "Error: unsupported option for glyph selection."
	gLogReporter.close()
	if numGlyphs > kProgressBarThreshold:
		fl.EndProgress()
	
	gLogReporter = None
	# end of doCheck

def CheckGlyph(options, flGlyph, arg_string):
	logMsg("\tglyph %s." % flGlyph.name)
	numPaths =  flGlyph.GetContoursNumber()
	numLayers = flGlyph.layers_number
	mastersNodes = []
	if numLayers == 0:
		numLayers = 1 # allow for old FontLab variation.
	changedGlyph = 0
	for layer in range(numLayers):
		try:
			bezData = BezChar.ConvertFLGlyphToBez(flGlyph, layer)
		except SyntaxError,e:
			logMsg(e)
			logMsg("Skipping glyph %s." % flGlyph.name)
		bp = open(options.tempBez, "wt")
		bp.write(bezData)
		bp.close()
		command = "%s -o %s %s 2>&1" % (coPath, arg_string, options.tempBez)
		p = os.popen(command)
		log = p.read()
		myLength = len(log)
		p.close()
		if log:
			msg = log
			logMsg( msg)
		if options.debug:
			print options.tempBez
			print command
			print log
			
		if  options.doFixProblems and (myLength > 0):
			if os.path.exists(options.tempBez):
				bp = open(options.tempBez, "rt")
				newBezData = bp.read()
				bp.close()
				if not options.debug:
					os.remove(options.tempBez)
			else:
				newBezData = None
				msg = "Skipping glyph %s. Failure in processing outline data" % (flGlyph.name)
				logMsg( msg)
				return
			changedGlyph = 1
			nodes  = BezChar.MakeGlyphNodesFromBez(flGlyph.name, newBezData)
			mastersNodes.append(nodes)
			
	if changedGlyph:
		flGlyph.RemoveHints(1)
		flGlyph.RemoveHints(2)
		flGlyph.Clear()
		if numLayers == 1:
			flGlyph.Insert(nodes, 0)
		else:
			# make sure we didn't end up with different node lists when working with MM designs.
			nlen = len(mastersNodes[0])
			numMasters = len(mastersNodes)
			for list in mastersNodes[1:]:
				if nlen != len(list):
					logMsg("Error: node lists after fixup are not same length in all masters, Skipping %s." % flGlyph.name)
					raise ACError
			flGlyph.Insert(nodes, 0)

			for i in range(nlen):
				mmNode = flGlyph[i]
				points = mmNode.points
				mmNode.type = mastersNodes[0][i].type
				for j in range(numMasters):
					targetPointList = mmNode.Layer(j)
					smNode = mastersNodes[j][i]
					srcPointList = smNode.points
					for pi in range(mmNode.count):
						targetPointList[pi].x = srcPointList[pi].x
						targetPointList[pi].y = srcPointList[pi].y
	return 


class OutlineCheckDialog:
	def __init__(self):
		dWidth = 500
		dMargin = 25

		# Selection control positions
		xMax = dWidth - dMargin
		y0 = dMargin + 20
		x1 =  dMargin + 20
		x2 = dMargin + 300
		y1 = y0 + 30
		y2 = y1 + 30
		lastSelectionY = y2+40
		
		# Test control positions
		yTopFrame = lastSelectionY + 10
		yt1= yTopFrame + 20 
		xt1 = dMargin + 20
		buttonSpace = 20
		toleranceWidth = 50
		toleranceHeight = 25
		xt2 = xt1
		yt2 = yt1 + buttonSpace
		xt3 = xt2
		yt3 = yt2 + buttonSpace
		xt4 = xt3
		yt4 = yt3 + buttonSpace
		xt5 = xt4
		yt5 = yt4 + buttonSpace
		xt6 = xt5
		yt6 = yt5 + buttonSpace
		xt7 = xt6
		yt7 = yt6 + buttonSpace
		xt8 = xt7
		yt8 = yt7 + buttonSpace
		xt9 = xt8
		yt9 = yt8 + buttonSpace
		xt10 = xt9
		yt10 = yt9 + buttonSpace
		lastButtonX = xt10
		lastButtony = yt10
		# tolerance values
		xt11 = lastButtonX + 100
		yt11 = lastButtony + toleranceHeight  + 5
		xt12 = xt11
		yt12 = yt11  + toleranceHeight + 5
		xt13 = xt11
		yt13 = yt12 + toleranceHeight + 5
		
		lastY = yt13 + 40

		dHeight = lastY + 50
		xt14 = lastButtonX
		yt14 = dHeight - 35
		
		self.d = Dialog(self)
		self.d.size = Point(dWidth, dHeight)
		self.d.Center()
		self.d.title = "Outline Checker"
		self.options = CheckOutlineOptions()
		self.options._getPrefs(self) # This both loads prefs and assigns the member fields of the dialog.
		options.debug = debug

		
		# no font warning.
		self.d.AddControl(STATICCONTROL, Rect(aIDENT, aIDENT, aIDENT, aIDENT), "frame", STYLE_FRAME) 

		if fl.font == None:
			self.d.AddControl(STATICCONTROL, Rect(aIDENT2, aIDENT2, aAUTO, aAUTO), "label", STYLE_LABEL, "Please first open a font.")
			return

		self.d.AddControl(STATICCONTROL, Rect(dMargin, dMargin, xMax, lastSelectionY), "frame", STYLE_FRAME, "Glyph Selection") 

		self.d.AddControl(CHECKBOXCONTROL, Rect(x1, y1, x1+200, aAUTO), "doAllGlyphs", STYLE_CHECKBOX, "Process all glyphs") 

		self.d.AddControl(CHECKBOXCONTROL, Rect(x1, y2, x1+200, aAUTO), "doSelectedGlyphs", STYLE_CHECKBOX, "Process selected glyphs") 

		self.d.AddControl(CHECKBOXCONTROL, Rect(x2, y0, x2 + 200, aAUTO), "doCurrentFont", STYLE_CHECKBOX, "Current Font") 

		self.d.AddControl(CHECKBOXCONTROL, Rect(x2, y1, x2 + 200, aAUTO), "doAllOpenFonts", STYLE_CHECKBOX, "All open fonts") 

		self.d.AddControl(CHECKBOXCONTROL, Rect(x2, y2, x2 + 200, aAUTO), "doWholeFamily", STYLE_CHECKBOX, "All fonts in family") 


		self.d.AddControl(STATICCONTROL, Rect(dMargin, yTopFrame, xMax, lastY), "frame2", STYLE_FRAME, "Test Selections") 

		self.d.AddControl(CHECKBOXCONTROL, Rect(xt1, yt1, xt1+200, aAUTO), "beVerbose", STYLE_CHECKBOX, "List checks done in log") 

		self.d.AddControl(CHECKBOXCONTROL, Rect(xt2, yt2, xt1+200, aAUTO), "doOverlapCheck", STYLE_CHECKBOX, "Check Overlaps") 

		self.d.AddControl(CHECKBOXCONTROL, Rect(xt3, yt3, xt1+200, aAUTO), "doCoincidentPathTest", STYLE_CHECKBOX, "Check Coincident Paths") 

		self.d.AddControl(CHECKBOXCONTROL, Rect(xt4, yt4, xt1+200, aAUTO), "skipInspectionTests", STYLE_CHECKBOX, "Skip All Inspection Tests") 

		self.d.AddControl(CHECKBOXCONTROL, Rect(xt5, yt5, xt1+200, aAUTO), "doAllInspectionTests", STYLE_CHECKBOX, "Do All Inspection Tests") 

		self.d.AddControl(CHECKBOXCONTROL, Rect(xt6, yt6, xt1+200, aAUTO), "doSmoothnessTest", STYLE_CHECKBOX, "Inspect Smoothness") 

		self.d.AddControl(CHECKBOXCONTROL, Rect(xt7, yt7, xt1+200, aAUTO), "doSpikeTest", STYLE_CHECKBOX, "Inspect Sharp Angles/Spikes") 

		self.d.AddControl(CHECKBOXCONTROL, Rect(xt8, yt8, xt1+200, aAUTO), "doTriangleTest", STYLE_CHECKBOX, "Inspect Small Triangles") 

		self.d.AddControl(CHECKBOXCONTROL, Rect(xt9, yt9, xt1+200, aAUTO), "doPathDirectionTest", STYLE_CHECKBOX, "Inspect Path Direction") 

		self.d.AddControl(CHECKBOXCONTROL, Rect(xt10, yt10, xt1+300, aAUTO), "doFixProblems", STYLE_CHECKBOX, "Fix Problems (always save font before!)") 

		self.d.AddControl(EDITCONTROL, Rect(xt11, yt11, xt11 +toleranceWidth, yt11 + toleranceHeight ), "curveTolerance", STYLE_EDIT, "default 0.125 units") 
		self.d.AddControl(STATICCONTROL, Rect(xt11+toleranceWidth + 8, yt11+5, xt12+toleranceWidth + 8 + 200, yt11+ toleranceHeight ), "curveToleranceLabel", STYLE_LABEL, "tolerance for linear curves") 

		self.d.AddControl(EDITCONTROL, Rect(xt12, yt12, xt12 +toleranceWidth, yt12 + toleranceHeight), "lineTolerance", STYLE_EDIT, "default 0.01  units") 
		self.d.AddControl(STATICCONTROL, Rect(xt12+toleranceWidth + 8, yt12+5, xt12+toleranceWidth + 8 + 200, yt12 + toleranceHeight ), "lineToleranceLabel", STYLE_LABEL, "tolerance for colinear lines") 

		self.d.AddControl(EDITCONTROL, Rect(xt13, yt13, xt13 +toleranceWidth, yt13 + toleranceHeight ), "pathTolerance", STYLE_EDIT, "default 0.909 units")
		self.d.AddControl(STATICCONTROL,  Rect(xt13+toleranceWidth + 8, yt13+5,xt12+toleranceWidth + 8 + 200, yt13 + toleranceHeight ), "pathToleranceLabel", STYLE_LABEL, "tolerance for coincident paths")

		self.d.AddControl(BUTTONCONTROL, Rect(xt14, yt14, xt14+60, yt14+20), "help", STYLE_BUTTON, "Help") 

	def toggleOffAllOtherSelections(self, cmd, cmdList):
		exec("curVal = self." + cmd)
		if curVal:
			curVal = 0
		else:
			curVal = 1
		for cmd_name in cmdList:
			if cmd_name == cmd:
				continue
				
			if cmd_name == "doGlyphString":
				self.doGlyphString = ""
				self.d.PutValue("doGlyphString")
			else:
				exec("self." + cmd_name + " = curVal")
				exec("self.d.PutValue(\"" + cmd_name + "\")")
				if curVal:
					break

	def on_doAllGlyphs(self, code):
		self.d.GetValue("doAllGlyphs")
		self.toggleOffAllOtherSelections("doAllGlyphs", self.CmdList2)

	def on_doSelectedGlyphs(self, code):
		self.d.GetValue("doSelectedGlyphs")
		self.toggleOffAllOtherSelections("doSelectedGlyphs", self.CmdList2)

	def on_doCurrentFont(self, code):
		self.d.GetValue("doCurrentFont")
		self.toggleOffAllOtherSelections("doCurrentFont", self.CmdList1)

	def on_doAllOpenFonts(self, code):
		self.d.GetValue("doAllOpenFonts")
		self.toggleOffAllOtherSelections("doAllOpenFonts", self.CmdList1)

	def on_doWholeFamily(self, code):
		self.d.GetValue("doWholeFamily")
		self.toggleOffAllOtherSelections("doWholeFamily", self.CmdList1)

	def on_doOverlapCheck(self, code):
		self.d.GetValue("doOverlapCheck")

	def on_skipInspectionTests(self, code):
		self.d.GetValue("skipInspectionTests")
		if self.skipInspectionTests:
			self.doSmoothnessTest = 0
			self.d.PutValue("doSmoothnessTest")
			self.doSpikeTest = 0
			self.d.PutValue("doSpikeTest")
			self.doTriangleTest = 0
			self.d.PutValue("doTriangleTest")
		else:
			# if one is on, don't turn on the others.
			if not (self.doSmoothnessTest or \
					self.doSpikeTest or \
					self.doTriangleTest):
				self.doSmoothnessTest = 1
				self.d.PutValue("doSmoothnessTest")
				self.doSpikeTest = 1
				self.d.PutValue("doSpikeTest")
				self.doTriangleTest = 1
				self.d.PutValue("doTriangleTest")

	def on_doSmoothnessTest(self, code):
		self.d.GetValue("doSmoothnessTest")
		if self.doSmoothnessTest and self.skipInspectionTests:
			self.skipInspectionTests = 0
			self.d.PutValue("skipInspectionTests")
		elif (not self.skipInspectionTests) and \
					 not (self.doSmoothnessTest or \
					self.doSpikeTest or \
					self.doTriangleTest):
			self.skipInspectionTests = 1
			self.d.PutValue("skipInspectionTests")
						

	def on_doSpikeTest(self, code):
		self.d.GetValue("doSpikeTest")
		if self.doSpikeTest and self.skipInspectionTests:
			self.skipInspectionTests = 0
			self.d.PutValue("skipInspectionTests")
		elif (not self.skipInspectionTests) and \
					 not (self.doSmoothnessTest or \
					self.doSpikeTest or \
					self.doTriangleTest):
			self.skipInspectionTests = 1
			self.d.PutValue("skipInspectionTests")
		
	def on_doTriangleTest(self, code):
		self.d.GetValue("doTriangleTest")
		if self.doTriangleTest and self.skipInspectionTests:
			self.skipInspectionTests = 0
			self.d.PutValue("skipInspectionTests")
		elif (not self.skipInspectionTests) and \
					 not (self.doSmoothnessTest or \
					self.doSpikeTest or \
					self.doTriangleTest):
			self.skipInspectionTests = 1
			self.d.PutValue("skipInspectionTests")
		
	def on_doPathDirectionTest(self, code):
		self.d.GetValue("doPathDirectionTest")
		
	def on_doCoincidentPathTest(self, code):
		self.d.GetValue("doCoincidentPathTest")
		
	def on_doFixProblems(self, code):
		self.d.GetValue("doFixProblems")
		
	def on_beVerbose(self, code):
		self.d.GetValue("beVerbose")
	
	def on_curveTolerance(self, code):
		self.d.GetValue("curveTolerance")
	
	def on_colinearLineTolerance(self, code):
		self.d.GetValue("lineTolerance")
	
	def on_curveTolerance(self, code):
		self.d.GetValue("pathTolerance")
	

	def on_ok(self, code):
		self.result = 1
		# update options
		self.options._savePrefs(self) # update prefs file
	
	def on_cancel(self, code):
		self.result = 0
	
	#OutlineCheckDialog
	def on_help(self, code):
		self.result = 2
		self.d.End() # only ok and cancel buttons do this automatically.
	
	def Run(self):
		self.d.Run()
		return self.result


	#OutlineCheckDialog
class ACHelpDialog:
	def __init__(self):
		dWidth = 700
		dMargin = 25
		dHeight = 500
		self.result = 0
		
		self.d = Dialog(self)
		self.d.size = Point(dWidth, dHeight)
		self.d.Center()
		self.d.title = "Auto Hinting Help"
	
		editControl = self.d.AddControl(LISTCONTROL, Rect(dMargin, dMargin, dWidth-dMargin, dHeight-50), "helpText", STYLE_LIST, "")
		self.helpText = __doc__.splitlines() + error_doc.splitlines()
		self.helpText = map(lambda line: "   " + line, self.helpText)
		self.d.PutValue("helpText")


	def on_cancel(self, code):
		self.result = 0
	
	def on_ok(self, code):
		self.result = 2
	
	def Run(self):
		self.d.Run()
		return self.result


def run():
	global debug
	
	dontShowDialog = 1
	if coPath:
		result = 2
		dontShowDialog = checkControlKeyPress()
		debug =  not checkShiftKeyPress()

		if dontShowDialog:
			print "Hold down CONTROL key while starting this script in order to set options."
			if fl.count == 0:
				print "You must have a font open to run auto-hint"
			else:
				options = CheckOutlineOptions()
				options._getPrefs() # load current settings from prefs
				options.debug = debug
				doCheck(options)
		else:
			# I do this funky little loop so that control will return to the main dialgo after showing help.
			# Odd things happen to the dialog focus if you call the help dialgo directly from the main dialog.
			# in FontLab 4.6
			while result == 2:
				d = OutlineCheckDialog()
				result = d.Run() # returns 0 for cancel, 1 for ok, 2 for help
				if result == 1:
					options = CheckOutlineOptions()
					options._getPrefs() # load current settings from prefs
					options.debug = debug
					doCheck(options)
				elif result == 2:
					ACh = ACHelpDialog()
					result = ACh.Run() # returns 0 for cancel, 2 for ok
					
if __name__ == '__main__':
	run()