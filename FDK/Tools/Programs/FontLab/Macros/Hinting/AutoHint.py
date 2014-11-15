#FLM: Auto-Hint
__copyright__ =  """
Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0.
"""

__doc__ = """
AutoHint v1.9 May 1 2009

This script will apply the Adobe 'AC' auto-hinting rules to the specified
glyphs.

Good hinting depends on having useful values for the global alignment zones and
standard stem widths. Please check these before running the AC module. You can
use the 'Report Stem and Alignment Values' script to get a frequency count of
stems and character extremes in the font.

By default, AutoHint will hint all glyphs in the font. Options allow you to
specify a sub-set of the glyphs for hinting.

If the check box 'Allow changes to glyph outlines' is marked, then AutoHint may
change the coordinates. It will preserve the outline shape, but will do things
like add a node at vertical or horizontal extreme, and remove nodes on a
straight line. In general, you should not allow this after you consider the
outlines to be final; AutoHint is just a program, and will make changes you will
not like. If you want to allow AutoHint to change the glyphs, apply it one glyph
at a time, and inspect the changes.

If the check box 'Verbose progress" is checked, then the AutoHint module will
comment on the progress of the hinting, noting such things as near-misses to a
standard stem-width.

The progress of the hinting is reported bout to the 'Output' window, and to a
log file "log\AutoHint.logvXXX'. This log file is written relative to the parent
font file.

Note that a whole family can be hinted at once by opening all the fonts in the
family, and then marking the check-box 'All open fonts'.

AutoHint can maintain a history file, which allows you to avoid hinting glyphs
that have already been auto-hinted or manually hinted. When this is in use,
AutoHint will by default hint only  those glyphs that are not already hinted,
and also those glyphs which are hinted, but whose outlines have changed since
the last time AutoHint was run. AutoHint knows whether an outline has changed by
storing the outline in the history file whenever the glyph is hinted, and then
consulting the history file when it is asked to hint the glyph again. By
default, AutoHint does not maintain or use the history file, but this can be
turned on with an option.

When used, the history file is named "<PostScriptName>.plist", in the same
location as the parent font file. For each glyph, AutoHint stores a simplified
version of the outline coordinates. If this entry is missing for a glyph and the
glyph has hints, then AutoHint assumes it was manually hinted, and will by
default not hint it again. 

The option "Use history file" will cause AutoHint to maintian a history file
when being run.

The option "Re-hint unknown glyphs" will cause AutoHint to re-hint all selected
glyphs. It has effect only when the history file is being used.

The option 'Re-hint unknown glyphs", will cause Autohint to re-hint glyphs which
Autohint thinks are manually hinted. It has effect only when the history file is
being used.

The option If the file is missing, AutoHint will assume that all the glyphs were
manually hinted, and you will have to use the options "Å"  or "Re-hint unknown
glyphs" to hint any glyphs.

 """

import string
import sys
import os
import time
import sys
import plistlib
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
	haveAC = setFDKToolsPath("autohintexe.exe")
else:
	haveAC = setFDKToolsPath("autohintexe")

# Test settings. Edit as needed.
# To check that the new hints match exactly the existing hints in the font, to compare the current calculations against previous calculations,
# just set the following four values to the opposite Boolean values.
verifyhints=0		# compare the new hints against the old ones.
verifyhintsubs=0	# compare the new hint substitution table against the old one.
changeglyphs=1		# if 0, keeps the script from changing any glyph data.
compatibleMode = 1	# this forces AClib to always put the dominant set of hints at the start of the
					#charstring, even if this means it is immediately replaced by the hint layer for the first part
					# of the glyph. We set this true only when comparing output to the original Unix AC tool.


# Global constants.
kFontPlistSuffix  = ".plist"
kACIDKey = "com.adobe.AC" # Key for AC values in the font plist file.
acLogFileName = "AutoHint.log" #  Is written to "log" subdirectory from current font.
gLogReporter = None # log file class instance.
global debug
debug = 0
kProgressBarThreshold = 8 # I bother witha progress bar just so the user can easily cancel without using CTRL-C
kProgressBarTickStep = 4
kIGlyphListFile = "hintList.txt"
kPrefsName =  "AutoHint.prefs"

class ACError(KeyError):
	pass

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

def GetGlyphNamesFromFile(fontPath):
	fileList = []
	importName = os.path.join(fontPath, kIGlyphListFile)
	if not os.path.isfile(importName):
		print "There must be a file named  %s to provide the list of glyph names." % importName
		return []
	ifile = file(importName, "rt")
	data = ifile.read()
	ifile.close()
	data =  re.sub(r"#.+?([\n\r])", r"\1", data) # supress comments
	lines =  re.findall(r"([^\r\n]+)", data)
	#remove blank lines
	lines = map(lambda line: line.strip(), lines)
	lines = filter(None, lines)
	return lines


flexPatthern = re.compile(r"preflx1[^f]+preflx2[\r\n](-*\d+\s+-*\d+\s+-*\d+\s+-*\d+\s+-*\d+\s+-*\d+\s+)(-*\d+\s+-*\d+\s+-*\d+\s+-*\d+\s+-*\d+\s+-*\d+\s+).+?flx([\r\n])",  re.DOTALL)
commentPattern = re.compile(r"[^\r\n]*%[^\r\n]*[\r\n]")
hintGroupPattern = re.compile(r"beginsubr.+?newcolors[\r\n]", re.DOTALL)
whiteSpacePattern = re.compile(r"\s+", re.DOTALL)
def makeACIdentifier(bezText):
	# Get rid of all the hint operators and their args 
	# collapse flex to just the two rct's
	bezText = commentPattern.sub("", bezText)
	bezText = hintGroupPattern.sub("", bezText)
	bezText = flexPatthern.sub( "\1 rct\3\2 rct\3", bezText)
	bezText = whiteSpacePattern.sub("", bezText)
	return bezText

def CompareNodes(newNodes, flNodes, layer):
	# Compare new and old fontLab glyph nodes to see if the outline was changed by AC.
	# return 1 if nodes are different, else 0.
	changed = 0
	numNodes = len(newNodes)
	if numNodes != len(flNodes):
		changed = 1
	else:
		for i in range(numNodes):
			if newNodes[i].type != flNodes[i].type:
				changed = 1
				break

			points = newNodes[i].points
			if layer > 1:
				flpoints = flNodes[i].Layer(layer)
			else:
				flpoints = flNodes[i].points
			if points != flpoints:
				changed = 1
				break

	return changed


def  openFontPlistFile(psName, dirPath):
	# Find or create the plist file. This hold a Python dictionary in repr() form,
	# key: glyph name, value: outline point list
	# This is used to determine which glyphs are manually hinted, and which have changed since the last
	# hint pass. 
	fontPlist = None
	filePath = None
	isNewPlistFile = 1
	if dirPath:
		pPath1 = os.path.join(dirPath, psName + kFontPlistSuffix)
	else:
		pPath1 = psName + kFontPlistSuffix
	
	if  os.path.exists(pPath1):
		filePath = pPath1
	else: # Crude approach to file length limitations. Since Adobe keeps face info in separate directories, I don't worry about name collisions.
		if dirPath:
			pPath2 = os.path.join(dirPath, psName[:-len(kFontPlistSuffix)] + kFontPlistSuffix)
		else:
			pPath2 = psName[:-len(kFontPlistSuffix)] + kFontPlistSuffix
		
		if os.path.exists(pPath2):
			filePath = pPath2
	if not filePath:
		filePath = pPath1
	else:
		try:
			fontPlist = plistlib.Plist.fromFile(filePath)
			isNewPlistFile = 0
		except (IOError, OSError):
			raise ACFontError("\tError: font plist file exists, but coud not be read <%s>." % filePath)		
		except:
			raise ACFontError("\tError: font plist file exists, but coud not be parsed <%s>." % filePath)
		
	if  fontPlist == None:
		fontPlist =  plistlib.Plist()
	if not fontPlist.has_key(kACIDKey):
		fontPlist[kACIDKey] = {}
	return fontPlist, filePath, isNewPlistFile

def doHinting(options):
	global gLogReporter
	result = 1

	if fl.count < 1:
		return

	fl.SetUndo()
	logMsg(" ")
	if (options.doAllOpenFonts):
		fontRange = range(fl.count)
	elif options.doCurrentFont:
		fontRange = [fl.ifont]
	else:
		logMsg("Error: unsupported option for font selection.")
		return

	if options.getNamesFromFile:
		fontPath = fl.font.file_name
		if not fontPath:
			print "Cannot find list file next to font file, when font file doesn't yet exist."
			return
			
		fontPath = os.path.dirname(fontPath)
		nameList = GetGlyphNamesFromFile(fontPath)
		numGlyphs = len(nameList)
		if not nameList:
			print "No names found in glyph name list file."
			return
	# Temproary data file paths used with the autohintexe program.
	tempBaseName = os.tempnam()
	options.tempBez = tempBaseName + ".bez"
	options.tempBezNew = options.tempBez + ".new"
	options.tempFI = tempBaseName + ".fi"

	for fi in fontRange:
		font = fl[fi]
		
		if not options.getNamesFromFile:
			# Because of the options.doSelectedGlyphs option and the fact that we can't assume
			# that GI's are the same in every font, we first 
			# collect the selected glyph names from the current font,
			# and then index glyphs in all fonts by name.
			nameList = []
			if options.doSelectedGlyphs:
				# Note that I am getting the name list from the selected glyphs in the active font
				# which is not necessarily the one  being processed.
				for gi in range(len(fl.font)):
					if fl.Selected(gi):
						nameList.append(fl.font[gi].name)

			if options.doAllGlyphs:
				for gi in range(len(font)):
					nameList.append(font[gi].name)

			if not nameList:
				logMsg("No glyphs selected for font %s." % os.path.basename(font.file_name))
				continue

		# set up progress bar
		numGlyphs = len(nameList)
		if numGlyphs > kProgressBarThreshold:
			fl.BeginProgress("Checking glyphs...", numGlyphs)
		tick = 0


		# Create font-specific log file.
		fontName = font.font_name
		if not fontName:
			fontName = "FontName-Undefined"
		filePath = font.file_name
		if not filePath:
			filePath = fontName
		gLogReporter = Reporter(os.path.join(os.path.dirname(filePath), acLogFileName))
		if not gLogReporter.file:
			gLogReporter = None

		# load fontPlist.
		fontPlist = None
		isNewPlistFile = 0
		if options.doHistoryFile:
			fontPlist, fontPlistFilePath, isNewPlistFile = openFontPlistFile(fontName, os.path.dirname(filePath))
			if isNewPlistFile and (not (options.doReHintUnknown or options.doHintAll)):
				logMsg("No hint info plist file was found, so all glyphs are unknown to AC. To hint all glyphs, run AC again with option to hint all glyphs unconditionally.")
				return

		logMsg("Autohinting starting for font", os.path.basename(filePath), time.asctime())
		lenFont = len(font)
		fontInfo = BezChar.GetACFontInfoFromFLFont(font)
		fp = open(options.tempFI, "wt") # For name-keyed ofnts, there is only one fontinfo string.
		fp.write(fontInfo)
		fp.close()
		anyGlyphChanged = 0
		for gname in nameList:
			gi = font.FindGlyph(gname)
			if gi > -1: # not all open fonts will have the same list of glyphs.
				flGlyph = font.glyphs[gi]
				glyphChanged = Run_AC(flGlyph, fontInfo, fontPlist, options, isNewPlistFile)
				if glyphChanged:
					anyGlyphChanged = 1
				fl.UpdateGlyph(gi)
				if numGlyphs > kProgressBarThreshold:
					tick = tick + 1
					if (tick % kProgressBarTickStep == 0):
						result = fl.TickProgress(tick)
						if not result:
							break
		if fontPlist:
			fontPlist.write(fontPlistFilePath)
		if gLogReporter:
			gLogReporter.close()
			gLogReporter = None
		font.modified = 1


		
		if numGlyphs > kProgressBarThreshold:
			# can end the progress bar only if we started it.
			fl.EndProgress()

		if (not anyGlyphChanged) and options.doHistoryFile:
			if (options.doReHintUnknown):
				logMsg("No new hints. All selected glyphs were hinted and had same outline as recorded  in  %s." % (os.path.basename(fontPlistFilePath)))
			else:
				logMsg("No new hints. All selected glyphs either were hinted and had the same outline as recorded in  %s, or were not referenced in the hint info file." % (os.path.basename(fontPlistFilePath)))
	

	logMsg("All done with AC %s" % time.asctime())


def Run_AC(flGlyph, fontInfo, fontPlist, options, isNewPlistFile):
	# Convert Fl glyph data to the bez format, call the AC library, and
	# then update the glyph with the new hint data, and possibly the new
	# outline data.
	glyphChanged = 0
	if len(flGlyph.nodes) == 0:
		logMsg("Skipping glyph %s. A composite or Non-marking glyph - nothing to hint." % flGlyph.name)
		return glyphChanged
	numLayers = flGlyph.layers_number
	if numLayers == 0:
		numLayers = 1 # allow for old FontLab variation.

	mastersNodes = []
	masterHints = []
	outlinesChanged = 0
	glyphChanged = 0
	hasHints  = flGlyph.hhints or flGlyph.vhints
	prevACIdentifier = None
	for layer in range(numLayers):
		try:
			bezData = BezChar.ConvertFLGlyphToBez(flGlyph, layer)
		except (ACError, SyntaxError),e:
			logMsg(e)
			logMsg("Error in parsing FontLab glyph. Skipping glyph %s." % flGlyph.name)
			return glyphChanged
			
		if layer == 0 and options.doHistoryFile:
			ACidentifier = makeACIdentifier(bezData) # no hints in this, so it does nto need special processing.

			# If the glyph does not have hints, we always hint it.
			if hasHints and (not options.doHintAll):
				# If the glyph is not in the  plist file, then we skip it unless kReHintUnknown is set.
				# If the glyph is in the plist file and the outlien ahs changed, we hint it. 
				try:
					(prevACIdentifier, ACtime) =  fontPlist[kACIDKey][flGlyph.name]
				except KeyError:
					if not (options.doReHintUnknown):
						# Glyphs is hinted, but not referenced in the plist file. Skip it
						if  not isNewPlistFile:
							# Comment only if there is a plist file; otherwise, we'd be complaining for almost every glyph.
							logMsg("\t%s Skipping glyph - it has hints, but it is not in the hint info plist file." % flGlyph.name)
						return glyphChanged

				if prevACIdentifier == ACidentifier: # there is an entry, in the plist file and it matches what's in the font.
					return glyphChanged

		logMsg("Hinting %s." % flGlyph.name)
		bp = open(options.tempBez, "wt")
		bp.write(bezData)
		bp.close()

		if os.path.exists(options.tempBezNew):
			os.remove(options.tempBezNew)
		
		if options.beVerbose:
			verboseArg = ""
		else:
			verboseArg = " -q"
	
		if options.allowPathChanges:
			suppressEditArg = ""
		else:
			suppressEditArg = " -e"
	
		if options.noHintSubstitions:
			supressHintSubArg = " -n"
		else:
			supressHintSubArg = ""
	
		command = "autohintexe %s%s%s -s .new -f \"%s\" \"%s\" 2>&1" % (verboseArg, suppressEditArg, supressHintSubArg, options.tempFI, options.tempBez)
		p = os.popen(command)
		log = p.read()
		p.close()
		if log:
			msg = log
			logMsg( msg)
		if options.debug:
			print options.tempBez
			print options.tempFI
			print command
			print log
		if os.path.exists(options.tempBezNew):
			bp = open(options.tempBezNew, "rt")
			newBezData = bp.read()
			bp.close()
			if options.debug:
				msg =  "Wrote AC fontinfo data file to", options.tempFI
				logMsg( msg)
				msg = "Wrote AC output bez file to", options.tempBezNew
				logMsg( msg)
			else:
				os.remove(options.tempBezNew)
		else:
			newBezData = None
			msg = "Skipping glyph %s. Failure in processing outline data" % (flGlyph.name)
			logMsg( msg)
			return glyphChanged
			
		if not newBezData:
			msg = "Skipping glyph %s. Failure in processing outline data" % (flGlyph.name)
			logMsg( msg)
			return glyphChanged
			
		if not (("ry" in newBezData[:200]) or ("rb" in newBezData[:200]) or ("rm" in newBezData[:200]) or ("rv" in newBezData[:200])):
			msg = "Skipping glyph %s. No hints added!" % (flGlyph.name)
			logMsg( msg)
			return glyphChanged
		
		if  options.doHistoryFile:
			ACidentifier = makeACIdentifier(newBezData)


		# If we asked AC to fix up the outlines while hinting and it did so, then we need
		# to convert the bez data back to a node list.				
		if options.allowPathChanges:
			if  options.doHistoryFile:
				if prevACIdentifier and (prevACIdentifier != ACidentifier):
					logMsg("\t%s Glyph outline changed" % name)
					outlinesChanged = 1
			else:
					outlinesChanged = 1

		nodes = BezChar.MakeGlyphNodesFromBez(flGlyph.name, newBezData)
		mastersNodes.append(nodes)

		inlines = newBezData.splitlines()
		(newreplacetable, newhhints, newvhints) = BezChar.ExtractHints(inlines, flGlyph)
		masterHints.append((newreplacetable, newhhints, newvhints))

	glyphChanged = 1
	if changeglyphs: # changeglyphs is a test flag to leave the glyphs unchanged when verifying auto-hinting.
		if options.doHistoryFile:
			fontPlist[kACIDKey][flGlyph.name] = (ACidentifier, time.asctime())
		if outlinesChanged:
			flGlyph.Clear()
			if numLayers == 1:
				flGlyph.Insert(nodes, 0)
			else:
				# make sure we didn't end up with different node lists when workign with MM designs.
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
							

		# clear old hints before adding new ones.
		flGlyph.RemoveHints(1)
		flGlyph.RemoveHints(2)

		if numLayers == 1:
			for hint in newhhints:
				flGlyph.hhints.append(hint) 
			for hint in newvhints:
				flGlyph.vhints.append(hint) 
			for rep in newreplacetable:
				flGlyph.replace_table.append(rep)
		else:
			numMasters = len(masterHints)
			(replacetable, hhints, vhints) = masterHints[0]
			for hint in hhints:
				flGlyph.hhints.append(hint) 
			for hint in vhints:
				flGlyph.vhints.append(hint) 
			for rep in replacetable:
				flGlyph.replace_table.append(rep)

			for j in range(numMasters)[1:]:
				(newreplacetable, newhhints, newvhints) = masterHints[0]
				if (len(replacetable) != len(newreplacetable)) or (len(hhints) != len(newhhints)) or (len(vhints) != len(newvhints)):
					msg = "Skipping glyph %s. Hints differ in length for different masters"
					raise ACError(msg)

				for i in range(len(newhhints)):
					flGlyph.hhints[i].positions[j] = newhhints[i].position
					flGlyph.hhints[i].widths[j] = newhhints[i].width
				for i in range(len(newvhints)):
					flGlyph.vhints[i].positions[j] = newvhints[i].position
					flGlyph.vhints[i].widths[j] = newvhints[i].width
	return glyphChanged


class ACOptions:
	# Holds the options for the module.
	# The values of all member items NOT prefixed with "_" are written to/read from
	# a preferences file.
	# This also gets/sets the same member fields in the passed object.
	def __init__(self):
		# Selection control fields
		self.doAllOpenFonts = 0
		self.doCurrentFont = 1
		self.doAllGlyphs = 0
		self.doSelectedGlyphs = 1
		self.getNamesFromFile = 0
	
		# Operation control fields
		self.doHistoryFile = 0
		self.doReHintUnknown = 1
		self.doHintAll = 0
		self.allowPathChanges = 0
		self.noHintSubstitions = 0
		self.beVerbose = 1
		self.noHintSub = 0
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

		# Add/set the member fields of the calling object
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


class ACDialog:
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
		lastSelectionY = y2 + 40
		
		# Test control positions
		yTopFrame = lastSelectionY + 10
		yt1= yTopFrame + 20 
		xt1 = dMargin + 20
		buttonSpace = 20
		yt2 = yt1 + 30
		yt3 = yt2 + 30
		yt4 = yt3 + 40
		yt5 = yt4 + 35		
		yt6 = yt5 + 35		
		lastY = yt6 + 40

		dHeight = lastY + 50
		
		self.d = Dialog(self)
		self.d.size = Point(dWidth, dHeight)
		self.d.Center()
		self.d.title = "Auto Hinting"

		self.CmdList1 = ["doAllOpenFonts", "doCurrentFont"] # define list of mutually exclusive check box commands.
		self.CmdList2 = ["doAllGlyphs", "doSelectedGlyphs", "getNamesFromFile"]
		self.options = ACOptions()
		self.options._getPrefs(self) # This both loads prefs and assigns the member fields of the dialog.
		self.options.debug = debug # debug is global, set if the SHIFT key is held down while starting the script.

		self.d.AddControl(STATICCONTROL, Rect(aIDENT, aIDENT, aIDENT, aIDENT), "frame", STYLE_FRAME) 

		# Complain if there are no fonts.
		if fl.font == None:
			self.d.AddControl(STATICCONTROL, Rect(aIDENT2, aIDENT2, aAUTO, aAUTO), "label", STYLE_LABEL, "Please first open a font.")
			return

		self.d.AddControl(STATICCONTROL, Rect(dMargin, dMargin, xMax, lastSelectionY), "frame", STYLE_FRAME, "Glyph Selection") 

		self.d.AddControl(CHECKBOXCONTROL, Rect(x1, y0, x1+200, aAUTO), "doAllGlyphs", STYLE_CHECKBOX, "Process all glyphs") 

		self.d.AddControl(CHECKBOXCONTROL, Rect(x1, y1, x1+200, aAUTO), "doSelectedGlyphs", STYLE_CHECKBOX, "Process selected glyphs") 

		self.d.AddControl(CHECKBOXCONTROL, Rect(x1, y2, x1+200, y2+30), "getNamesFromFile", STYLE_CHECKBOX, "Get glyph names from file %s" % kIGlyphListFile) 

		self.d.AddControl(CHECKBOXCONTROL, Rect(x2, y0, x2+100, aAUTO), "doCurrentFont", STYLE_CHECKBOX, "Current Font") 

		self.d.AddControl(CHECKBOXCONTROL, Rect(x2, y1, x2+200, aAUTO), "doAllOpenFonts", STYLE_CHECKBOX, "All open fonts") 



		self.d.AddControl(STATICCONTROL, Rect(dMargin, yTopFrame, xMax, lastY), "frame2", STYLE_FRAME, "Operations") 


		self.d.AddControl(CHECKBOXCONTROL, Rect(xt1, yt1, xt1+300, yt1+30), "allowPathChanges", STYLE_CHECKBOX, "Allow changes to glyph outline") 

		self.d.AddControl(CHECKBOXCONTROL, Rect(xt1, yt2, xt1+300, yt2+30), "noHintSub", STYLE_CHECKBOX, "Do not allow hint replacement.") 

		self.d.AddControl(CHECKBOXCONTROL, Rect(xt1, yt3, xt1+300, yt3+30), "beVerbose", STYLE_CHECKBOX, "Verbose progress") 

		self.d.AddControl(CHECKBOXCONTROL, Rect(xt1, yt4, xt1+400, yt4+30), "doHistoryFile", STYLE_CHECKBOX, "Maintain history file") 

		self.d.AddControl(CHECKBOXCONTROL, Rect(xt1, yt5, xt1+400, yt5+30), "doReHintUnknown", STYLE_CHECKBOX, "Re-hint unknown glyphs. (Warning: these may have been manually hinted.)") 

		self.d.AddControl(CHECKBOXCONTROL, Rect(xt1, yt6, xt1+300, yt6+30), "doHintAll", STYLE_CHECKBOX, "Hint all specified glyphs") 

		helpYPos =  dHeight-35
		self.d.AddControl(BUTTONCONTROL, Rect(xt1, helpYPos, xt1+60, helpYPos+20), "help", STYLE_BUTTON, "Help") 


	def toggleOffAllOtherSelections(self, cmd, cmdList, state):
		if state: # Fl Dialog doesn't know about True and False.
			state = 1
		else:
			state = 0
		for cmd_name in cmdList:
			if cmd_name == cmd:
				continue
							
			exec("curVal = self." + cmd_name)
			exec("self." + cmd_name + " = %s" % state)
			exec("self.d.PutValue(\"" + cmd_name + "\")")
			if state:
				# If we are turning off an option,
				# then in this toggel function, we want to turn on only the first
				# item which is not the command we just turned off.
				break
	
	def on_doAllGlyphs(self, code):
		self.d.GetValue("doAllGlyphs")
		self.toggleOffAllOtherSelections("doAllGlyphs", self.CmdList2, not self.doAllGlyphs)
		
	def on_doSelectedGlyphs(self, code):
		self.d.GetValue("doSelectedGlyphs")
		self.toggleOffAllOtherSelections("doSelectedGlyphs", self.CmdList2, not self.doSelectedGlyphs)

	def on_getNamesFromFile(self, code):
		self.d.GetValue("getNamesFromFile")
		self.toggleOffAllOtherSelections("getNamesFromFile", self.CmdList2, not self.getNamesFromFile)

	def on_doHistoryFile(self, code):
		self.d.GetValue("doHistoryFile")

	def on_doReHintUnknown(self, code):
		self.d.GetValue("doReHintUnknown")

	def on_doHintAll(self, code):
		self.d.GetValue("doHintAll")

	def on_doCurrentFont(self, code):
		self.d.GetValue("doCurrentFont")
		self.toggleOffAllOtherSelections("doCurrentFont", self.CmdList1, not self.doCurrentFont)

	def on_doAllOpenFonts(self, code):
		self.d.GetValue("doAllOpenFonts")
		self.toggleOffAllOtherSelections("doAllOpenFonts", self.CmdList1, not self.doAllOpenFonts)

	def on_allowPathChanges(self, code):
		self.d.GetValue("allowPathChanges")

	def on_noHintSub(self, code):
		self.d.GetValue("noHintSub")

	def on_beVerbose(self, code):
		self.d.GetValue("beVerbose")

	def on_ok(self,code):
		self.result = 1
		# update options
		self.options._savePrefs(self) # update prefs file

	def on_cancel(self, code):
		self.result = 0
	
	def on_help(self, code):
		self.result = 2
		self.d.End() # only ok and cancel buttons do this automatically.
	
	def Run(self):
		self.d.Run()
		return self.result

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
		self.helpText = __doc__.split("\n")
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
	if haveAC:
		result = 2
		dontShowDialog = checkControlKeyPress()
		debug =  not checkShiftKeyPress()
		if dontShowDialog:
			print "Hold down CONTROL key while starting this script in order to set options."
			if fl.count == 0:
				print "You must have a font open to run auto-hint"
			else:
				options = ACOptions()
				options._getPrefs() # load current settings from prefs
				options.debug = debug
				doHinting(options)
		else:
			# I do this funky little loop so that control will return to the main dialog after showing help.
			# Odd things happen to the dialog focus if you call the help dialgo directly from the main dialog.
			# in FontLab 4.6
			while result == 2:
				ACd = ACDialog()
				result = ACd.Run() # returns 0 for cancel, 1 for ok, 2 for help
				if result == 1:
					options = ACOptions()
					options._getPrefs() # load current settings from prefs
					options.debug = debug
					doHinting(options)
				elif result == 2:
					ACh = ACHelpDialog()
					result = ACh.Run() # returns 0 for cancel, 2 for ok

if __name__ == '__main__':
	run()

