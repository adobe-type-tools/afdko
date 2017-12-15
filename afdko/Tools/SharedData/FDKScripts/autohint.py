#!/bin/env python
__copyright__ = """Copyright 2016 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
"""

__usage__ = """
autohint  AutoHinting program v1.49 April 27 2016
autohint -h
autohint -u
autohint -hfd
autohint -pfd
autohint [-g <glyph list>] [-gf <filename>] [-xg <glyph list>] [-xgf <filename>] [-cf path] [-a] [-logOnly] [-log <logFile path>] [-r] [-q] [-c] [-nf] [-ns] [-nb] [-wd] [-o <output font path>]  font-path

Auto-hinting program for PostScript and OpenType/CFF fonts.
"""

from ufoTools import kProcessedGlyphsLayerName, kProcessedGlyphsLayer

__help__ = __usage__ + """

Takes a  list fonts, and an optional list of glyphs, and hints the
fonts. If the list of glyphs is supplied, the hinting is limited to the
specified glyphs.

Note that the hinting is better if the font's global alignment zones are
set usefully; at the very least, you should have entered values that
capture capital height, x height, ascender and descender heights, and
ascender and descender overshoot. The reports provided by the stemHist
tool are useful for choosing these.

By default, autothint will hint all glyphs in the font. Options allow you to
specify a sub-set of the glyphs for hinting.

Options:

-h	Print help

-u	Print usage

-hfd  Print a description of the format for defining a set of alternate
alignment zones in an "fontinfo" file.

-pfd  Print the default FDDict values for the source font: the
	alignment zone, stem width, and other global values. This is useful
	as a starting point for building FDDict defintions (see -hfd).

-pfdl Print the list of user-defined FDDict values, and which glyphs
	are included in each. This is useful for checking your FDDict
	definitions and glyph search terms. (see -hfd).
		
-g <glyphID1>,<glyphID2>,...,<glyphIDn>
	Hint only the specified list of glyphs. Note that all glyphs will be 
	written to the output file. The list must be comma-delimited. The glyph 
	references may be glyphID's, glyph names, or glyph CID's. If the latter, 
	the CID value must be prefixed with the string "/". There must be no 
	white-space in the glyph list.
	Examples:
		autohint -g A,B,C,68 myFont
		autohint -g /1030,/434,/1535,68 myCIDFont

	A range of glyphs may be specified by providing two names separated
	only by a hyphen:
		autohint -g zero-nine,onehalf myFont

	Note that the range will be resolved by expanding the glyph indices
	(GID)'s, not by alphabetic names.

-gf <file name>
	Hint only the list of glyphs contained in the specified file. The file 
	must contain a comma-delimited list of glyph identifiers. Any number of 
	space, tab, and new-line characters are permitted between glyph names 
	and commas.
	
-xg, -xgf
	Same as -g and -gf, but will exclude the specified glyphs from hinting.

-cf <path>
	AC will try and add counter hints to only a short hard-coded list of 
	glyphs:
		V counters:  "m", "M", "T", "ellipsis"
		H counters:  "element", "equivalence", "notelement", "divide".
	Counter hints help to keep the space between stems open and equal in size.
	
	To extend this list, use the option -cf followed by a path to a text file.
	The text file must contain one record per line. A  record references one
	glyph, and should consist of a single letter V or H, to indicate whether
	the counters should be vertical or horizontal, followd by a space or tab,
	followed by a glyph name. A maximum of 64 glyph names can be added to either
	the vertical or horizontal list. Example:
		V	ffi
		V	ffl
		V	f_f_j
	
	Alternatively, if there is a file named "fontinfo" in the same directory as
	the source font, this script will look in that file for lines with the format:
		VCounterChar ( <glyph name1>  <glyph name2> ... )
		HCounterChar ( <glyph name1>  <glyph name2> ... )
	and add the referenced glyphs to the counter hint lists.
	Example line:
		VCounterChar (ffi ffl f_f_j)
	
-logOnly
	Do not change any outlines, but report warnings for all selected 
	glyphs, including those already hinted. The option -q is ignored. 
	The option -a is implied.
	
-q	Quiet mode
	Will suppress comments from the auto-hinting library about recommended 
	glyph outline changes.

-c	Permit changes to the glyph outline
	When this is turned on, the autohint program will fix a few issues:
	if there are many hint substitutions, it will try and shuffle the order
	of paths to reduce this, and it will flatten nearly straight curves.
	It no longer blunts sharp angles.That is better done with checkOutlines

-nf	Suppress generation of flex commands

-ns	Suppress hint substitution
	Do this only if you really want the smallest possible font file. This 
	will use only one set of hints for the entire glyph.

-nb	Allow the font to have to no stem widths or blue values specified.
	Without this option, autohint will complain and quit.

-o <output font path>
	If not specified, autohint will write the hinted output to the original 
	font path name.

-log	Write output to a log file

-all Hint all glyphs, even if the source glyphs is unchanged and the
	glyph has been hinted before. Applies only to UFO fonts.

-hf	Use history file.  Will create it if it does not already exist.
	Should not be used with UFO fonts, where another mechanism is used.

-a	Hint all glyphs that are in the history file, or are unhinted.
	Has effect only if the history file is being used.

-r	Re-hint glyphs. Glyphs not in the history file will be hinted even
	if they already have hints. However, glyphs will not be hinted if
	they both have not changed and are in the history file.

 -decimal  Use decimal coordinates, instead of rounding them to the 
 nearest integer value.

-wd	write changed glyphs to default layer instead of '%s'

autohint can also apply different sets of alignment zones while hinting
a particular set of glyphs. This is useful for name-keyed fonts, which,
unlike CID fonts, only have one set of global alignment zones and stem
widths. By default, autohint uses the font's global alignment zones and
stem widths for each glyph. However, if there is a file named "fontinfo"
in the same directory as the input font file, autohint will check the
"fontinfo" file for definitions of alternate sets of alignment zones,
and the matching lists of glyphs to which they should be applied. To see
the format for these entries, use the option "-hfd". This allows one set
of glyphs to be hinted using a different set of zones and stem widths
than other glyphs. This isn't as useful as having real multiple hint
dictionaries in the font, as the final name-keyed font can only have one
set of alignment zones, but it does allow for improved hinting when
different sets of glyphs need different baselines.


autohint can maintain a history file, which allows you to avoid hinting glyphs
that have already been auto-hinted or manually hinted. When this is in use,
autohint will by default hint only  those glyphs that are not
already hinted, and also those glyphs which are hinted, but whose outlines have
changed since the last time autohint was run. autohint knows whether an outline
has changed by storing the outline in the history file whenever the glyph is
hinted, and then consulting the history file when it is asked to hint the glyph
again. By default, autohint does not maintain or use the history file, but this
can be turned on with an option.

When used, the history file is named "<PostScriptName>.plist", in the same
location as the parent font file. For each glyph, autohint stores a simplified
version of the outline coordinates. If this entry is missing for a glyph and the
glyph has hints, then autohint assumes it was manually hinted, and will by
default not hint it again. If the file is missing, autohint will assume that all
the glyphs were manually hinted, and you will have to use the option -a or -r to
hint any glyphs.
""" % (kProcessedGlyphsLayerName)


__FDDoc__ = """
By default, autohint uses the font's global alignment zones and stem
widths when hinting each glyph. However, if there is a file named
"fontinfo" in the same directory as the input font file, autohint will
check the "fontinfo" file for definitions of sets of alignment zones ( a
"FDDict"), and the matching lists of glyphs to which they should be
applied. This allows one set of glyphs to be hinted using a different
set of zones and stem widths than other glyphs. This isn't as useful as
having real multiple hint dictionaries in the font, as the final
name-keyed font can only have one set of alignment zones, but it does
allow for improved hinting when different sets of glyphs need different
alignment zones.

If FDDict definitions are used, then the global alignment zones and stem
widths in the source font will be ignored. For any glyphs not covered by
an explicit FDDict definition, autohint will synthesize an dummy FDDict,
where the zones are set outside of the the font's bounding box, so they
will not affect hinting. This is desirable for glyphs that have no
features that need to be aligned.

If autohint finds an FDDict named "FinalFont", then it will write that
set of values to the output font. Otherwise, it will merge all the
alignment zones and stem widths in the union of all the FDDict
definitions. If this merge fails because some of the alignment zones are
stem widths overlap, then you have to provide a "FinalFont" FDDict that
explicitly defines which stems and zones to use in the hinted output
font.

To use a dictionary of alignment zones and stem widths, you need to
define both the dictionary of alternate values, and the set of glyphs to
apply it to. The FDDict must be defined in the file before the set of
glyphs which belong to it. Both the FDDict and the glyph set define a
name; an FDDict is applied to the glyph set with the same name.


If you run autohint with the option "-pfd", it will print out the list
of FDDict values for the source font. You can use this text as a
starting point for your FDDict definitions.

You can also run autohint with the option "-pfdl". This will print the
user-defined FDDict defintions, and the list of glyphs associated with
each FDDict. Youc an use this to check your values, and to check which
glyphs are assigned to which FDDict. In particular, check the glyph list
for the first FDDict "No Alignment Zones": this list exists because
these glyphs did not match in the search terms for any user-defined
FDDict.

The definitions use the following syntax:

begin FDDict <name>
	<key-1> <value-1>
	<key-2> <value-2>
	...
	<key-n> <value-n>
end FDDict <name>

begin GlyphSet <name>
	<glyphname-1> <glyphname-2> ...
	<glyphname-n>
end GlyphSet <name>

The glyph names may be either a real glyph name, or a regular expression
designed to match several names.  A mini regex primer:
^      matches at the start of the glyph name
$      matches at the end
[aABb]  matches any one character in the set a, A, b, B.
[A-Z]  matches any one character in the set comprising the range from A-Z
[abA-Z]  matches any one character in the set comprising set set of a, b,
         and the characters in the range from A-Z
.      matches any single character
+      maches whatever preceded it one or more times
*      matches whatever preceded it none or more times.
\      An escape character that includes the following character without
       the second one being interpreted as a regex special character

Examples:
^[A-Z]$     Matches names with one character in the range from A-Z.
^[A-Z].+    Matches any name where the first character is in the range A-Z,
			and it is followed by one or more characters.
[A-Z].+    Matches any name with a character that is in the range A-Z and
           which is followed by one or more characters.
^[A-Z].*   Matches any name with one or more characters, and
            the first character is in the range A-Z
^.+\.smallcaps   Matches any name that contains ".smallcaps"
^.+\.smallcaps$   Matches any name that ends with ".smallcaps"
^.+\.s[0-24]0$     Matches any name that ends with ".s00",".s10",".s20", or ".s04"


Example FDDict and GlyphSet definitions.
********************************

begin FDDict ST_Smallcaps
	# I like to put the non hitn stuff first.
	OrigEmSqUnits 1000
	FontName AachenStd-Bold # This gets used as the hint dict name if the font is eventually built as a CID font.
	FlexOK true
	
	# Alignment zones. The first is a bottom zone, the rest ar top zones. See below.
	BaselineOvershoot -20
	BaselineYCoord 0
	CapHeight 900
	CapOvershoot 20
	LcHeight 700
	LcOvershoot 15
	
	# Stem widths.
	DominantV [236 267]
	DominantH [141 152]
end FDDict ST_Smallcaps


begin FDDict LM_Smallcaps
	OrigEmSqUnits 1000
	FontName AachenStd-Bold
	BaselineOvershoot -25
	BaselineYCoord 0
	CapHeight 950
	CapOvershoot 25
	LcHeight 750
	LcOvershoot 21
	DominantV [236 267]
	DominantH [141 152]
	FlexOK true
end FDDict LM_Smallcaps


begin GlyphSet LM_Smallcaps
	[Ll]\S+\.smallcap  [Mm]\S+\.smallcap
end GlyphSet LM_Smallcaps


begin GlyphSet ST_Smallcaps
	[Tt]\S+\.smallcap  [Ss]\S+\.smallcap
end GlyphSet ST_Smallcaps

********************************

Note that whitespace must exist between keywords and values, but is
otherwise ignored. "#" is a comment character: any occurrence of "#" and
all following text on a line is skipped. GlyphSet and FDDict definitions
may be intermixed, as long as any FDDict is defined before the GlyphSet
which refers to it.

You must provide at least two BlueValue pairs (the 'BaselineYCoord'
bottom zone and any top zone), and you must provide the DominantH and
DominantV keywords. All other keywords are optional.

The full set of recognized FDDict keywords are:

BlueValue pairs:
	# BaselineOvershoot is a bottom zone, the rest are top zones.
	BaselineYCoord
	BaselineOvershoot
	
	CapHeight 
	CapOvershoot 
	
	LcHeight 
	LcOvershoot 
	
	AscenderHeight 
	AscenderOvershoot 
	
	FigHeight 
	FigOvershoot 
	
	Height5
	Height5Overshoot
	
	Height6
	Height6Overshoot

OtherBlues pairs:
	# These 
	Baseline5Overshoot 
	Baseline5
	
	Baseline6Overshoot 
	Baseline6
	
	SuperiorOvershoot 
	SuperiorBaseline 
	
	OrdinalOvershoot 
	OrdinalBaseline 
	
	DescenderOvershoot 
	DescenderHeight

For zones which capture the bottom of a feature in the glyph, (BaselineYCoord and all the OtherBlues), the value
specifies the top of the zone, and the "Overshoot" is a negative value which specifes the offset to the bottom of the zone, e.g.
	BaselineYCoord 0 
	BaselineOvershoot 12

For zones which capture the top of a feature in the glyph, (teh rest of the BlueValue zones), the value
specifies the bottom of the zone, and the "Overshoot" is a negapositivetive value which specifes the offset to the top of the zone, e.g.
	Height6 800
	Height6Overshoot 20


Note also that there is no implied sequential order of values. Height6 may have a value less than or equal to CapHeight.

The values for keywords in one FontDict definiton are completely independent of the values used in another FontDict. There is
no inheritance from one definition to the next.

All FontDicts must specify at least the BaselineYCoord and one top zone.

Miscellaneous values.
	FontName	PostScript font name. Only used by makeotf when building CID font.
	OrigEmSqUnits Single value: size of em-square. Only used by makeotf when building CID font.
	LanguageGroup 0 or 1. Specifies whether counter hints for ideographic glyphs should be applied.
	DominantV	List of dominant vertical stems, in the form  [ <stem-value-1> <stem-value-2> ...]
	DominantH	List of dominant horizontal  stems, in the form  [ <stem-value-1> <stem-value-2> ...]
	FlexOK		true or false.
	VCounterChars	List of characters to which counter hints may be applied, in the form [ <glyph-name-1> <glyph-name-2> ...]
	HCounterChars	List of characters to which counter hints may be applied, in the form [ <glyph-name-1> <glyph-name-2> ...]

Note for cognoscenti: the  autohint program code ignores StdHW and StdVW
entries if DominantV and DominantH entries are present, so I omit
writing the Std[HV]W keywords to fontinfo file. Also, autohint will add
any non-duplicate stem width values for StemSnap[HV] to the Dominant[HV]
stem width list, but the StemSnap[HV] entries are not necessary if the
full list of stem widths are supplied as values for the Dominant[HV]
keywords, hence I also  write the full stem list for the Dominant[HV]
keywords, and do not write the StemSnap[HV] keywords, to the fontinfo
file. Thsi is technically not right, as DominantHV array is supposed to
hold only two values, but the autohint program doesn't care, and I can
write fewer entries this way.
"""

# Methods:

# Parse args. If glyphlist is from file, read in entire file as single string,
# and remove all whitespace, then parse out glyph-names and GID's.

# For each font name:
#    use fontTools library to open font and extract CFF table. 
#    If error, skip font and report error.
#    filter specified glyph list, if any, with list of glyphs in the font.
#    open font plist file, if any. If not, create empty font plist.
#    build alignment zone string
#    for identifier in glyph-list:
#        Get T2 charstring for glyph from parent font CFF table. If not present, report and skip.
#        get new alignment zone string if FD array index (which font dict is used) has changed.
#        Convert to bez time
#        Build autohint point list string, used to tell if glyph has been changed since the last time it was hinted.
#        If requested, check against plist dict, and skip if glyph is 
#        already hinted or is manually hinted.
#        Call auto-hint library on bez string.
#        If change to the point list is permitted and happened, rebuild
#        autohint point list string.
#        Convert bez string to T2 charstring, and update parent font CFF.
#        add glyph hint entry to plist file
#    save font plist file.

import sys
import os
import re
import time
from fontTools.ttLib import TTFont, getTableModule
import plistlib
import warnings
from BezTools import *
import FDKUtils
import ufoTools
import traceback
import shutil

warnings.simplefilter("ignore", RuntimeWarning) # supress waring about use of os.tempnam().

kACIDKey = "AutoHintKey"

gLogFile = None
kFontPlistSuffix  = ".plist"
kTempCFFSuffix = ".temp.ac.cff"

class ACOptions:
	def __init__(self):
		self.inputPath = None
		self.outputPath = None
		self.glyphList = []
		self.excludeGlyphList = 0
		self.usePlistFile = 0
		self.hintAll = 0
		self.rehint = 0
		self.verbose = 1
		self.allowChanges = 0
		self.noFlex = 0
		self.noHintSub = 0
		self.allow_no_blues = 0
		self.hCounterGlyphs = []
		self.vCounterGlyphs = []
		self.counterHintFile = None
		self.logOnly = 0
		self.logFilePath = None
		self.printDefaultFDDict = 0
		self.printFDDictList = 0
		self.debug = 0
		self.allowDecimalCoords = 0
		self.writeToDefaultLayer = 0
		
class ACOptionParseError(KeyError):
	pass

class ACFontInfoParseError(KeyError):
	pass

class ACFontError(KeyError):
	pass

class ACHintError(KeyError):
	pass

class FDKEnvironmentError(AttributeError):
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
			else:
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

def ACreport(*args):
	# long function used by the hinting library
	for arg in args:
		print arg,
	if arg[-1] != os.linesep:
		print

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

	command = "autohintexe -u 2>&1"
	report = FDKUtils.runShellCmd(command)
	if "version" not in report:
		logMsg("Please re-install the FDK. The path to the program 'autohintexe' is not in the environment variable PATH.")
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


def parseCounterHintData(path):
	hCounterGlyphList = []
	vCounterGlyphList = []
	gf = file(path, "rt")
	data = gf.read()
	gf.close()
	lines = re.findall(r"([^\r\n]+)", data)
	# strip blank and comment lines
	lines = filter(lambda line: re.sub(r"#.+", "", line), lines)
	lines = filter(lambda line: line.strip(), lines)
	for line in lines:
		fields = line.split()
		if (len(fields) != 2) or (fields[0] not in ["V", "v", "H", "h"]) :
			print "\tError: could not process counter hint line '%s' in file %s. Doesn't look like V or H followed by a tab or space, and then a glyph name." % (line, path)
		elif  fields[0] in ["V", "v"]:
			vCounterGlyphList.append(fields[1])
		else:
			hCounterGlyphList.append(fields[1])
	return hCounterGlyphList, vCounterGlyphList
	
	

def checkFontinfoFile(options):
	# Check if there ia a makeotf fontinfo file in the input font directory. If so, 
	# get any Vcounter or HCouunter glyphs from it.
	srcFontInfo = os.path.dirname(options.inputPath)
	srcFontInfo = os.path.join(srcFontInfo, "fontinfo")
	if os.path.exists(srcFontInfo):
		fi = open(srcFontInfo, "rU")
		data = fi.read()
		fi.close()
		data = re.sub(r"#[^\r\n]+", "", data)
		counterGlyphLists = re.findall(r"([VH])CounterChars\s+\(\s*([^\)\r\n]+)\)", data)
		for entry in counterGlyphLists:
			glyphList = entry[1].split()
			if glyphList:
				if entry[0] == "V":
					options.vCounterGlyphs.extend(glyphList)
				else:
					options.hCounterGlyphs.extend(glyphList)
					
		if 	options.vCounterGlyphs or options.hCounterGlyphs:
			options.counterHintFile = srcFontInfo

def getOptions():
	global gLogFile
	options = ACOptions()
	i = 1
	numOptions = len(sys.argv)
	while i < numOptions:
		arg = sys.argv[i]
		if options.inputPath:
			raise ACOptionParseError("Option Error: All options must preceed the  input font path <%s>." % arg) 

		if arg == "-h":
			print __help__
			command = "autohintexe -v"
			report = FDKUtils.runShellCmd(command)
			logMsg( report)
			raise ACOptionParseError
		elif arg == "-u":
			print __usage__
			command = "autohintexe -v"
			report = FDKUtils.runShellCmd(command)
			logMsg( report)
			raise ACOptionParseError
		elif arg == "-hfd":
			print __FDDoc__
			raise ACOptionParseError
		elif arg == "-pfd":
			options.printDefaultFDDict = 1
		elif arg == "-pfdl":
			options.printFDDictList = 1
		elif arg == "-hf":
			options.usePlistFile = 1
		elif arg == "-a":
			options.hintAll = 1
		elif arg == "-all":
			options.hintAll = 1
		elif arg == "-r":
			options.rehint = 1
		elif arg == "-q":
			options.verbose = 0
		elif arg == "-c":
			options.allowChanges = 1
		elif arg == "-nf":
			options.noFlex = 1
		elif arg == "-ns":
			options.noHintSub = 1
		elif arg == "-nb":
			options.allow_no_blues = 1
		elif arg in ["-xg", "-g"]:
			if arg == "-xg":
				options.excludeGlyphList = 1
			i = i +1
			glyphString = sys.argv[i]
			if glyphString[0] == "-":
				raise ACOptionParseError("Option Error: it looks like the first item in the glyph list following '-g' is another option.") 
			options.glyphList += parseGlyphListArg(glyphString)
		elif arg in ["-xgf", "-gf"]:
			if arg == "-xgf":
				options.excludeGlyphList = 1
			i = i +1
			filePath = sys.argv[i]
			if filePath[0] == "-":
				raise ACOptionParseError("Option Error: it looks like the the glyph list file following '-gf' is another option.") 
			try:
				gf = file(filePath, "rt")
				glyphString = gf.read()
				gf.close()
			except (IOError,OSError):
				raise ACOptionParseError("Option Error: could not open glyph list file <%s>." %  filePath) 
			options.glyphList += parseGlyphListArg(glyphString)
		elif arg == "-cf":
			i = i +1
			filePath = sys.argv[i]
			if filePath[0] == "-":
				raise ACOptionParseError("Option Error: it looks like the the counter hint glyph list file following '-cf' is another option.") 
			try:
				options.counterHintFile = filePath
				options.hCounterGlyphs, options.vCounterGlyphs = parseCounterHintData(filePath)
			except (IOError,OSError):
				raise ACOptionParseError("Option Error: could not open counter hint glyph list  file <%s>." %  filePath) 
		elif arg == "-logOnly":
			options.logOnly = 1
		elif arg == "-log":
			i = i +1
			options.logFilePath = sys.argv[i]
			gLogFile = open(options.logFilePath, "wt")
		elif arg == "-o":
			i = i +1
			options.outputPath = sys.argv[i]
		elif arg == "-d":
			options.debug = 1
		elif arg in ["-decimal", "-dec"]:
			options.allowDecimalCoords = True
		elif arg =="-wd":
			options.writeToDefaultLayer = 1
		elif arg[0] == "-":
			raise ACOptionParseError("Option Error: Unknown option <%s>." %  arg) 
		else:
			options.inputPath = arg
		i  += 1
	if not options.inputPath:
		raise ACOptionParseError("Option Error: You must provide a font file path.") 

	if not os.path.exists(options.inputPath):
		raise ACOptionParseError("Option Error: The input font file path %s' does not exist." % (options.inputPath)) 
	else:
		options.inputPath = options.inputPath.rstrip(os.sep) # might be a UFO font. auto completion in some shells adds a dir separator, which then causes problems with os.path.dirname().

	checkFontinfoFile(options)
	
	if options.logOnly:
		options.verbose = 1
		options.hintAll = 1
		
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
		if options.excludeGlyphList:
			newList = filter(lambda name: name not in glyphList, fontGlyphList)
			glyphList = newList
	return glyphList



def  openFontPlistFile(psName, dirPath):
	# Find or create the plist file. This hold a Python dictionary in repr() form,
	# key: glyph name, value: outline point list
	# This is used to determine which glyphs are manually hinted, and which have changed since the last
	# hint pass. 
	fontPlist = None
	filePath = None
	isNewPlistFile = 1
	pPath1 = os.path.join(dirPath, psName + kFontPlistSuffix)
	if  os.path.exists(pPath1):
		filePath = pPath1
	else: # Crude approach to file length limitations. Since Adobe keeps face info in separate directories, I don't worry about name collisions.
		pPath2 = os.path.join(dirPath, psName[:-len(kFontPlistSuffix)] + kFontPlistSuffix)
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


fontInfoKeywordList = [
'FontName', #string
'OrigEmSqUnits',
'LanguageGroup',
'DominantV', #array
'DominantH', #array
'FlexOK', #string
'BlueFuzz',
'VCounterChars', #counter
'HCounterChars', #counter
'BaselineYCoord',
'BaselineOvershoot',
'CapHeight',
'CapOvershoot',
'LcHeight',
'LcOvershoot',
'AscenderHeight',
'AscenderOvershoot',
'FigHeight',
'FigOvershoot',
'Height5',
'Height5Overshoot',
'Height6',
'Height6Overshoot',
'DescenderOvershoot',
'DescenderHeight',
'SuperiorOvershoot',
'SuperiorBaseline',
'OrdinalOvershoot',
'OrdinalBaseline',
'Baseline5Overshoot',
'Baseline5',
'Baseline6Overshoot',
'Baseline6',
]

integerPattern = """ -?\d+"""
arrayPattern = """ \[[ ,0-9]+\]"""
stringPattern = """ \S+"""
counterPattern = """ \([\S ]+\)"""

def parseFontInfoString(fontInfoString):
	for item in fontInfoKeywordList:
		if item in ['FontName', 'FlexOK']:
			matchingExp = item + stringPattern
		elif item in ['VCounterChars', 'HCounterChars']:
			matchingExp = item + counterPattern
		elif item in ['DominantV', 'DominantH']:
			matchingExp = item + arrayPattern
		else:
			matchingExp = item + integerPattern
		
		try:
			print '\t%s' % re.search(matchingExp, fontInfoString).group()
		except:
			pass
	
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

def openFile(path, outFilePath, useHashMap):
	if os.path.isfile(path):
		font =  openOpenTypeFile(path, outFilePath)
	else:
		# maybe it is a a UFO font.
		# We always use the hash map to skip glyphs that have been previously processed, unless the user has report only, not make changes.
		font =  openUFOFile(path, outFilePath, useHashMap)
	return font

def openUFOFile(path, outFilePath, useHashMap):
	# Check if has glyphs/contents.plist
	contentsPath = os.path.join(path, "glyphs", "contents.plist")
	if not os.path.exists(contentsPath):
		msg = "Font file must be a PS, CFF, OTF, or ufo font file: %s." % (path)
		logMsg(msg)
		raise ACFontError(msg)
	
	# If user has specified a path other than the source font path, then copy the entire
	# ufo font, and operate on the copy.
	if (outFilePath != None) and (os.path.abspath(path) != os.path.abspath(outFilePath)):
		msg = "Copying from source UFO font to output UFO font before processing..."
		logMsg(msg)
		if os.path.exists(outFilePath):
			shutil.rmtree(outFilePath)
		shutil.copytree(path , outFilePath)
		path = outFilePath
	font = ufoTools.UFOFontData(path, useHashMap, ufoTools.kAutohintName)
	font.useProcessedLayer = True
	font.requiredHistory.append(ufoTools.kCheckOutlineName) # Programs in this list must be run before autohint, if the outlines have been edited.
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
			raise ACFontError("Error opening or reading from font file <%s>." % path)
		except TTLibError:
			raise ACFontError("Error parsing font file <%s>." % path)

		try:
			cffTable = ttFont["CFF "]
		except KeyError:
			raise ACFontError("Error: font is not a CFF font <%s>." % fontFileName)

	else:
	
		# It is not an OTF file.
		if (data[0] == '\1') and (data[1] == '\0'): # CFF file
			fontType = 1
			tempPathCFF = path
		elif not "%" in data:
			#not a PS file either
			logMsg("Font file must be a PS, CFF or OTF  fontfile: %s." % path)
			raise ACFontError("Font file must be PS, CFF or OTF file: %s." % path)
	
		else:  # It is a PS file. Convert to CFF.	
			fontType =  2
			print "Converting Type1 font to temp CFF font file..."
			command="tx  -cff +b -std \"%s\" \"%s\" 2>&1" % (path, tempPathCFF)
			report = FDKUtils.runShellCmd(command)
			if "fatal" in report:
				logMsg("Attempted to convert font %s  from PS to a temporary CFF data file." % path)
				logMsg(report)
				raise ACFontError("Failed to convert PS font %s to a temp CFF font." % path)
		
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
			raise ACFontError("Error parsing font file <%s>." % path)

	fontData = CFFFontData(ttFont, path, outFilePath, fontType, logMsg)
	return fontData


def removeTempFiles(fileList):
	for filePath in fileList:
		if os.path.exists(filePath):
			os.remove(filePath)

def cmpFDDictEntries(entry1, entry2):
	# entry = [glyphName, [fdIndex, glyphListIndex] ]
	if entry1[1][1] > entry2[1][1]:
		return 1
	elif entry1[1][1] < entry2[1][1]:
		return -1
	else:
		return 0

def hintFile(options):

	path = options.inputPath
	fontFileName = os.path.basename(path)
	logMsg("Hinting font %s. Start time: %s." % (path, time.asctime()))

	try:
		useHashMap = not options.logOnly # for UFO fonts only. We always use the hash map, unless the user has said to only report issues.
		fontData = openFile(path, options.outputPath, useHashMap)
		fontData.allowDecimalCoords = options.allowDecimalCoords
		if options.writeToDefaultLayer and hasattr(fontData, "setWriteToDefault"): # UFO fonts only
			fontData.setWriteToDefault()
	except (IOError, OSError):
		logMsg( traceback.format_exception_only(sys.exc_type, sys.exc_value)[-1])
		raise ACFontError("Error opening or reading from font file <%s>." % fontFileName)
	except:
		logMsg( traceback.format_exception_only(sys.exc_type, sys.exc_value)[-1])
		raise ACFontError("Error parsing font file <%s>." % fontFileName)

	#   filter specified list, if any, with font list.
	fontGlyphList = fontData.getGlyphList()
	glyphList = filterGlyphList(options, fontGlyphList, fontFileName)
	if not glyphList:
		raise ACFontError("Error: selected glyph list is empty for font <%s>." % fontFileName)

	# temp file names for input and output bez files, and for the fontinfo file.
	tempBaseName = os.tempnam()
	tempBez = tempBaseName + ".bez"
	tempBezNew = tempBez + ".new"
	tempFI = tempBaseName + ".fi"
	
	#print "tempBaseName", tempBaseName
	psName = fontData.getPSName()
	
	if (not options.logOnly) and options.usePlistFile:
		fontPlist, fontPlistFilePath, isNewPlistFile = openFontPlistFile(psName, os.path.dirname(path))
		if isNewPlistFile and  not (options.hintAll or options.rehint):
			logMsg("No hint info plist file was found, so all glyphs are unknown to autohint. To hint all glyphs, run autohint again with option -a to hint all glyphs unconditionally.")
			logMsg("Done with font %s. End time: %s." % (path, time.asctime()))
			fontData.close()
			return

	# Check counter glyphs, if any.
	if options.hCounterGlyphs or options.vCounterGlyphs:
		missingList = filter(lambda name: name not in fontGlyphList, options.hCounterGlyphs + options.vCounterGlyphs)
		if missingList:
			logMsg( "\tError: glyph named in counter hint list file '%s' are not in font: %s" % (options.counterHintFile, missingList) )

	#    build alignment zone string
	if (options.printDefaultFDDict):
		logMsg("Showing default FDDict Values:")
		fdDict = fontData.getFontInfo(psName, path, options.allow_no_blues, options.noFlex, options.vCounterGlyphs, options.hCounterGlyphs)
		parseFontInfoString(str(fdDict))
		fontData.close()
		return

	fdGlyphDict, fontDictList = fontData.getfdInfo(psName, path, options.allow_no_blues, options.noFlex, options.vCounterGlyphs, options.hCounterGlyphs, glyphList)

	if options.printFDDictList:
		# Print the user defined FontDicts, and exit.
		if fdGlyphDict:
			logMsg("Showing user-defined FontDict Values:")
			for fi in range(len(fontDictList)):
				fontDict = fontDictList[fi]
				logMsg("")
				logMsg(fontDict.DictName)
				parseFontInfoString(str(fontDict))
				gnameList = []
				itemList = fdGlyphDict.items()
				itemList.sort(cmpFDDictEntries)
				for gName, entry in itemList:
					if entry[0] == fi:
						gnameList.append(gName)
				logMsg("%d glyphs:" % len(gnameList))
				if len(gnameList) > 0:
					gTxt = " ".join(gnameList)
				else:
					gTxt = "None"
				logMsg(gTxt)
		else:
			logMsg("There are no user-defined FontDict Values.")
		tempPathCFF = options.inputPath + kTempCFFSuffix
		removeTempFiles( [tempPathCFF] )
		fontData.close()
		return
		
	if fdGlyphDict == None:
		fdDict = fontDictList[0]
		fp = open(tempFI, "wt")
		fp.write(fdDict.getFontInfo())
		fp.close()
	else:
		if not options.verbose:
			logMsg("Note: Using alternate FDDict global values from fontinfo file for some glyphs. Remove option '-q' to see which dict is used for which glyphs.")


	#    for identifier in glyph-list:
	# 	Get charstring.
	removeHints = 1
	isCID = fontData.isCID()
	lastFDIndex = None
	reportCB = ACreport
	anyGlyphChanged = 0
	pListChanged = 0
	if isCID:
		options.noFlex = 1
		
	if options.verbose:
		verboseArg = ""
	else:
		verboseArg = " -q" 
		dotCount = 0
		curTime = time.time()

	if options.allowChanges:
		suppressEditArg = ""
	else:
		suppressEditArg = " -e"

	if options.noHintSub:
		supressHintSubArg = " -n"
	else:
		supressHintSubArg = ""

	if options.allowDecimalCoords:
		decimalArg = " -d"
	else:
		decimalArg = ""

	dotCount = 0
	seenGlyphCount = 0
	processedGlyphCount = 0
	for name in glyphList:
		prevACIdentifier = None
		seenGlyphCount +=1

		# 	Convert to bez format
		bezString, width, hasHints = fontData.convertToBez(name, removeHints, options.verbose, options.hintAll)
		processedGlyphCount += 1
		if bezString == None:
			continue

		if "mt" not in bezString:
			# skip empty glyphs.
			continue
		# get new fontinfo string if FD array index has changed, as
		# as each FontDict has different alignment zones.
		gid = fontData.getGlyphID(name)
		if isCID: #
			fdIndex = fontData.getfdIndex(gid)
			if not fdIndex == lastFDIndex:
				lastFDIndex = fdIndex
				fdDict = fontData.getFontInfo(psName, path, options.allow_no_blues, options.noFlex, options.vCounterGlyphs, options.hCounterGlyphs, fdIndex)
				fp = open(tempFI, "wt")
				fp.write(fdDict.getFontInfo())
				fp.close()
		else:
			if (fdGlyphDict != None):
				try:
					fdIndex = fdGlyphDict[name][0]
				except KeyError:
					# use default dict.
					fdIndex = 0
				if lastFDIndex != fdIndex:
					lastFDIndex = fdIndex
					fdDict = fontDictList[fdIndex]
					fp = open(tempFI, "wt")
					fp.write(fdDict.getFontInfo())
					fp.close()
			

		# 	Build autohint point list identifier

		oldBezString = ""
		oldHintBezString = ""
		if (not options.logOnly) and options.usePlistFile:
			# If the glyph is not in the  plist file, then we skip it unless kReHintUnknown is set.
			# If the glyph is in the plist file and the outline has changed, we hint it. 
			ACidentifier = makeACIdentifier(bezString)
			try:
				(prevACIdentifier, ACtime, oldBezString, oldHintBezString) =  fontPlist[kACIDKey][name]
			except ValueError:
				(prevACIdentifier, ACtime) =  fontPlist[kACIDKey][name]
				oldBezString = oldHintBezString = ""
			except KeyError:
				pListChanged = 1 # Didn't have an entry in tempList file, so we will add one.
				if hasHints and not options.rehint:
					# Glyphs is hinted, but not referenced in the plist file. Skip it unless options.rehint is seen
					if  not isNewPlistFile:
						# Comment only if there is a plist file; otherwise, we'd be complaining for almost every glyph.
						logMsg("%s Skipping glyph - it has hints, but it is not in the hint info plist file." % aliasName(name))
						dotCount = 0
					continue
			if prevACIdentifier and (prevACIdentifier == ACidentifier): # there is an entry in the plist file and it matches what's in the font.
				if hasHints and not (options.hintAll or options.rehint):
					continue
			else:
				pListChanged = 1

		if options.verbose:
			if fdGlyphDict:
				logMsg("Hinting %s with fdDict %s." % (aliasName(name), fdDict.DictName) )
			else:
				logMsg("Hinting %s." % aliasName(name))
		else:
			logMsg(".,")
			dotCount += 1
			if dotCount > 40:
				dotCount = 0
				logMsg("") # I do this to never have more than 40 dots on a line.
				# This in turn give reasonable performance when calling autohint in a subprocess
				# and getting output with std.readline()

		# 	Call auto-hint library on bez string.
		bp = open(tempBez, "wt")
		bp.write(bezString)
		bp.close()

		#print "oldBezString", oldBezString
		#print ""
		#print "bezString", bezString
		
		if oldBezString != "" and oldBezString == bezString:
			newBezString = oldHintBezString
		else:
			if os.path.exists(tempBezNew):
				os.remove(tempBezNew)
			command = "autohintexe %s%s%s%s -s .new -f \"%s\" \"%s\"" % (verboseArg, suppressEditArg, supressHintSubArg, decimalArg, tempFI, tempBez)
			if  options.debug:
				print command
			report = FDKUtils.runShellCmd(command)
			if report:
				if not options.verbose:
					logMsg("") # end series of "."
				logMsg(report)
			if os.path.exists(tempBezNew):
				bp = open(tempBezNew, "rt")
				newBezString = bp.read()
				bp.close()
				if options.debug:
					print "Wrote AC fontinfo data file to", tempFI
					print "Wrote AC output bez file to", tempBezNew
				else:
					os.remove(tempBezNew)
			else:
				newBezString = None
			
		if not newBezString:
			if not options.verbose:
				logMsg("")
			logMsg("%s Error - failure in processing outline data." % aliasName(name))
			continue
			
		if not (("ry" in newBezString[:200]) or ("rb" in newBezString[:200]) or ("rm" in newBezString[:200]) or ("rv" in newBezString[:200])):
			print "No hints added!"

		if options.logOnly:
			continue
			
		# 	Convert bez to charstring, and update CFF.
		anyGlyphChanged = 1
		fontData.updateFromBez(newBezString, name, width, options.verbose)

		
		if options.usePlistFile:
			bezString = "%% %s%s%s" % (name, os.linesep, newBezString)
			ACidentifier = makeACIdentifier(bezString)
			# add glyph hint entry to plist file
			if options.allowChanges:
				if prevACIdentifier and (prevACIdentifier != ACidentifier):
					logMsg("\t%s Glyph outline changed" % aliasName(name))
					dotCount = 0

			fontPlist[kACIDKey][name] = (ACidentifier, time.asctime(), bezString, newBezString )

	if not options.verbose:
		print "" # print final new line after progress dots.

	if  options.debug:
		print "Wrote input AC bez file to", tempBez
	else:
		tempPathCFF = options.inputPath + kTempCFFSuffix # created when a PS file is opened.
		removeTempFiles( [tempBez, tempBezNew, tempFI, tempPathCFF] )
					
	if not options.logOnly:
		if anyGlyphChanged:
			logMsg("Saving font file with new hints..." + time.asctime())
			fontData.saveChanges()
		else:
			fontData.close()
			if options.usePlistFile:
				if options.rehint:
					logMsg("No new hints. All glyphs had hints that matched the hint record file %s." % (fontPlistFilePath))
				if options.hintAll:
					logMsg("No new hints. All glyphs had hints that matched the hint history file %s, or were not in the history file and already had hints." % (fontPlistFilePath))
				else:
					logMsg("No new hints. All glyphs were already hinted.")
			else:
				logMsg("No glyphs were hinted.")
	if options.usePlistFile and (anyGlyphChanged or pListChanged):
		#  save font plist file.
		fontPlist.write(fontPlistFilePath)
	if processedGlyphCount != seenGlyphCount:
		logMsg("Skipped %s of %s glyphs." % (seenGlyphCount - processedGlyphCount, seenGlyphCount))
	logMsg("Done with font %s. End time: %s." % (path, time.asctime()))

def main():

	try:
		CheckEnvironment()
	except FDKEnvironmentError,e:
		logMsg(e)
		return

	try:
		options = getOptions()
	except ACOptionParseError,e:
		logMsg(e)
		return

	# verify that all files exist.
	try:
		hintFile(options)
	except (ACFontError, ufoTools.UFOParseError),e:
		logMsg("\t%s" % e)
	if gLogFile:
		gLogFile.close()
		
	return


if __name__=='__main__':
	main()
	

