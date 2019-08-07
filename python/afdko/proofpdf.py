# Copyright 2014 Adobe. All rights reserved.

"""
proofpdf.py. A wrapper for the fontpdf.py module. This script verifies
the existence of the specified font files, creates a font class object
with the call-backs required by fontpdf, and translates the command line
options to arguments for the fontpdf module; the latter produces a proof
file using the provided options annd font instance.
"""

import copy
import os
import platform
import re
import sys
import tempfile
import time
import traceback

from fontTools.ttLib import TTFont, getTableModule, TTLibError

from afdko import fdkutils
from afdko.pdflib import ttfpdf, otfpdf
from afdko.pdflib.fontpdf import (
	FontPDFParams, makePDF, makeFontSetPDF, kDrawTag, kDrawPointTag,
	kShowMetaTag, params_doc,
	)

curSystem = platform.system()


__usage__ = """
proofpdf v1.21 Aug 28 2018
proofpdf [-h] [-u]
proofpdf -help_params
proofpdf [-g <glyph list>] [-gf <filename>] [-gpp <number>] [-pt <number>] [-dno] [-baseline <number>] [-black] [-lf <filename>] [-select_hints <0,1,2..> ]  \
[-o <PDF file path> ] -hintplot ] [-charplot] [-digiplot] [-fontplot]  [-waterfallplot] [-wfr <point size list>] font-path1  fontpath2 ...

Glyph Proofing program for OpenType fonts
"""
__help__= __usage__ + """

"charplot", "digiplot", "fontplot", "hintplot", and "waterfallplot" are
all command files that call the proofpdf script with different options.
proofpdf takes as options a list of fonts, and an optional list of
glyphs, and prints a PDF file for the specified font, showing the glyphs
as specified by options.

The five main options, "-charplot", "-digiplot", "-fontplot","-hintplot"
and "waterfallplot", each set a bunch of lower level parameters in order
to produce a particular page layout. All these low-level parameters can
be set by command-line options.

Options:

-u Shows usage

-h Print help

-help_params Shows help abut the low level parameters.

-q Quiet mode

-charplot Sets parameters to show the glyph outline with most labels;
the point labels show only the point position. No hints or alignment
zones are shown; all glyph metric info is shown. Default is one glyph
per page.

-digiplot Sets parameters to show 2 copies of each glyph. The first is
shown with the em-box and the meta data block is filled with text from
an error log file, if one exists. The second shows the outline with some
labels; the point labels show only the point type. Default is one glyph
per page.

-repeatIndex <index>. Used in conjunction with the digiplot command. By default,
options after the 'digiplot' option will affect only the left-side copy of the
glyph. The 'repeatIndex' option can be used to specify that another copy of in
the set  of repeated glyph blocks will be affected by subsequent arguments.
Use '-repeatIndex 0" to specify the left-side glyph block, and '-repeatIndex 1'
to specify the right-side glyph glyph block, as the target for following
arguments.


-fontplot Sets parameters to show the filled glyph outline with no
labels, and some glyph metrics. Default is 156 glyphs per page.

-fontplot2 Sets parameters to show the filled glyph outline with no
labels, and a box showing em-box and height and width, with center
registration marks. Default is 156 glyphs per page.

-fontsetplot. Sets parameters to compare glyphs between different fonts. This
options figures out how many glyphs will fit across a page using a fixed spacing
of em size. It then divides the list of glyphs into groups of of this size. for
each group, the glyphs are shown in  a single line with fixed spacing for each
font in turn. This plot is useful for quickly seeing that the glyphs have the
same GID order between fonts, and that the glyph shape is correct by visually
comparing it to the nominally equivalent glyphs in the same column. Glyphs are
shown in glyph ID order. The font list is shown in a sorted order. The sorting
is first by the length of the charset, then by the charset lists, then by the
font PS name.

-alpha   Sorts the glyphs in alphabetic order by glyph name. Useful when
comparing proofs of fonts with the same charset, but different glyph order.

-hintplot Sets parameters to show the glyph outline with no labels, with
all hints and alignment zones, and with just the name and BBox info.
Default is one glyph per page.

-waterfallplot Sets parameters to show all the glyphs in a font in a series  of
waterfalls of decreasing point size. This options figures out how many glyphs
will fit across a page at the maximum point size, using the average character
width. It then divides the list of glyphs into groups of of this size, and shows
a waterfall for each group. The default list of point sizes for the waterfall
is: (36,24,20,18,16,14,12,10,9,8,7,6)

Note that this option  works very differently form the other proofs, in that the
font data is embedded, and the glyphs are requested by character code. In all
the other modes, the glyph outlines are drawn and filled with PDF drawing
operators, and the font is not embedded. This because the purpose is to check
hint quality at different point sizes, and this can be done only when the font
data is embedded and the glyphs are rasterized by the system or application.

Warning: this option does not yet work with Truetype or CID-keyed fonts.

-wfr <point size list> Overrides the default list of point sizes for the
waterfall mode. Has no effect if '-waterfallplot' is not specified. Example:
   -wfr 72,48,36,24,12,8
Note that the point sizes are separated by commas, with no white space allowed
in the list.


-g <glyphID1>,<glyphID2>,...,<glyphIDn>   Show only the specified list
of glyphs. The list must be comma-delimited, with no white space. The
glyph ID's may be glyph indices, glyph names, or glyph CID's. If the latter,
the CID value must be prefixed with the string "cid". There must be no
white-space in the glyph list. Examples: proofpdf -g A,B,C,69 myFont
proofpdf -g cid01030,cid00034,cid03455,69 myCIDFont.otf

A range of glyphs may be specified by providing two names separated only
by a hyphen:
fontplot -g zero-nine,onehalf myFont
Note that the range will be resolved by filling in the list of glyph
indices (GI) between the GI's of the two glyphs of the range, not by
alphabetic name order.

-gf <file name>   Hint only the list of glyphs contained in the
specified file, The file must contain a comma-delimited list of glyph
identifiers. Any number of space, tab, and new-line characters are
permitted between glyphnames and commas.

-gpp <number> Set the number of glyphs per page. The glyphs are scaled
so that this many glyphs will fit on a page.

-pt <number> Set point size for the glyphs. If supplied, -gpp will be
ignored.

-o <file path> Path name for output pdf file. Must end in "pdf". If
omitted, the default file name is <font name> + ".pdf"

-dno  Do not open the PDF file when done; default is to do so, except for
digiplot. Note: On Mac OS, if the  a PDF reader is already open and showing the
file, you will need to close the file window and re-open in order to see the new
contents. Also, The script does not choose the program to open the PDF; that is
handled by the computer operating system.

-do  Do  open the PDF file when done. Useful for digiplot, where the default is
to not open the pdf file.

-baseline Set baseline font font. This is is otherwise set
to the vale in the parent font BASE table. If the BASE table
is not present, it is set to -120 for CID keyed fonts, else
0.

-black Will set all colors to black; useful when planning to print the PDF.

-lf <filename> Path name to CID layout file. If supplied, and proofing a
CID-keyed font, glyphs will be shown in layout file order, and the
hint dir and row font dir name will be shown in the descriptive meta-data.

-select_hints <list of int replacement block indices> When showing
hints, will show only the specified hint replacement blocks. Example:
      -select_hints 3  will show only the 4th hint replacement block
      -select_hints 0,3  will show only the first and 4th hint replacement block.
Note that the index list must be comma-delimited with no white-space.

-v	Show the vertical metrics, but not horizontal metrics.

-vh Show both the vertical and horizontal metrics.

--<name> <value> If prepended with two hyphens, then this is interpreted
as a name of a low level parameter, which is set to the following value.
Note that any such options should follow the use of any of the five main
parameters, as -hintplot, -charplot, and -digiplot set a number of low
level parameters. Use "-help_params" to see the documentation for the
low level parameters. All sizes are expressed in points, where 72 points
= 1 inch. Fonts must be PostScript.
Examples:
--pageTitleFont "MinionPro-Bold"  #Change the page title font.
--pointLabelFont "Times-Bold"  #Change the font for point labels
--pointLabelColorRGB "(1.0, 0, 0)" #Change the color of the point labels
to red. The value is a triplet of (R G, B) values, where 0 is dark and 1.0 is
light. Note that the RGB values must be enclosed in quotes and
parentheses, and commas separated, as shown. "(0,0,0)" is black,
"(1.0, 1.0, 1.0)" is page white, "(1.0, 0, 0)" is red.
--pointLabelSize 12  #Change the size of the point label text.
"""


def logMsg(*args):
	for arg in args:
		print(arg)

class FDKEnvironmentError(Exception):
	pass

class OptionParseError(Exception):
	pass

class FontError(Exception):
	pass

def CheckEnvironment():
	tx_path = 'tx'
	txError = 0

	command = "%s -u 2>&1" % tx_path
	report = fdkutils.runShellCmd(command)
	if "options" not in report:
		txError = 1

	if txError:
		logMsg("Please check PATH and/or re-install the afdko. Cannot find: "
									"< %s >." % (tx_path))
		logMsg("or the files referenced by the shell script is missing.")
		raise FDKEnvironmentError

	return tx_path


def fixGlyphNames(glyphName):
	glyphRange = glyphName.split("-")
	if len(glyphRange) >1:
		g1 = fixGlyphNames(glyphRange[0])
		g2 =  fixGlyphNames(glyphRange[1])
		glyphName =  "%s-%s" % (g1, g2)

	elif glyphName[0] == "/":
		glyphName = "cid" + glyphName[1:].zfill(5)

	elif glyphName.startswith("cid") and (len(glyphName) < 8):
		return "cid" + glyphName[3:].zfill(5)

	return glyphName

def parseGlyphListArg(glyphString):
	glyphString = re.sub(r"[ \t\r\n,]+",  ",",  glyphString)
	glyphList = glyphString.split(",")
	glyphList = map(fixGlyphNames, glyphList)
	glyphList =  filter(None, glyphList)
	return glyphList

def expandRanges(tag):
	ptRange = tag.split("-")
	if len(ptRange) > 1:
		g1 = int(ptRange[0])
		g2 =  int(ptRange[1])
		ptRange = range(g1, g2+1)
	else:
		ptRange = [int(tag)]
	return ptRange

def parsePtSizeListArg(ptSizeString):
	ptSizeString = re.sub(r"[ \t\r\n,]+",  ",",  ptSizeString)
	ptsizeList = ptSizeString.split(",")
	ptEntryList = map(expandRanges, ptsizeList)
	ptSizeList = []
	for ptEntry in ptEntryList:
		ptSizeList.extend(ptEntry)
	return ptSizeList


def parseLayoutFile(filePath):
	layoutDict = None

	try:
		fp = open(filePath, "rt")
		data = fp.read()
		fp.close()
	except (IOError,OSError):
		raise OptionParseError("Option Error: could not open and read the layout file <%s>." %  filePath)

	entryList = re.findall(r"(\d+)\s+(\S+)\s+(\S+)\s+(\S+)", data)
	if len(entryList) < 2:
		print("Error: Failed to parse layout file %s. Did not match expected entry format." % filePath)
		raise OptionParseError
	print("Found %s entries in layout file %s. Mac CID: %s" % (len(entryList), os.path.basename(filePath), entryList[-1][0]))
	layoutDict = {}
	index = 0
	for entry in entryList:
		gname = "cid" + entry[0].zfill(5)
		layoutDict[gname] = [entry[1], entry[2], entry[3]] # add cide key-> family, face dir, name
		reversekey = repr(entry[1:])
		layoutDict[reversekey] = [gname, entry[1], entry[2], entry[3]] # add name key -> family, face, cid as cidXXXX
		entryList = None
	if "cid00000" in layoutDict:
		layoutDict[".notdef"] = layoutDict["cid00000"]
	return layoutDict


def setDefaultDigiplotRightSideOptions(rightGlyphParams):
	exec("rightGlyphParams." + kDrawTag + "EMBox = 0")
	exec("rightGlyphParams." + kDrawTag + "XAdvance = 0")
	exec("rightGlyphParams." + kDrawTag + "ContourLabels = 1")
	exec("rightGlyphParams." + kDrawTag + "Outline = 1")
	exec("rightGlyphParams." + kDrawTag + "CenterMarks = 0")
	exec("rightGlyphParams." + kDrawTag + "GlyphBox = 0")
	exec("rightGlyphParams." + kDrawPointTag + "PointMarks = 1")
	exec("rightGlyphParams." + kDrawPointTag + "BCPMarks = 1")
	exec("rightGlyphParams." + kDrawPointTag + "PointLabels = 1")
	rightGlyphParams.pointLabel_doPointType  = 1 # add point type to point label .
	rightGlyphParams.pointLabel_doPosition  = 0 # add point position to point label .
	rightGlyphParams.pointLabel_doPointIndex  = 0 # add index to point label .
	rightGlyphParams.metaDataAboveGlyph = 0 # write meta data below glyph square.
	if rightGlyphParams.rt_optionLayoutDict:
		exec("rightGlyphParams." + kShowMetaTag + "WidthOnly = 1")
		exec("rightGlyphParams." + kShowMetaTag + "HintDir = 1")
		exec("rightGlyphParams." + kShowMetaTag + "RowFont = 1")
		exec("rightGlyphParams." + kShowMetaTag + "Hints = 0")
		exec("rightGlyphParams." + kShowMetaTag + "SideBearings = 0")
		exec("rightGlyphParams." + kShowMetaTag + "V_SideBearings = 0")

def getOptions(params):
	i = 1
	numOptions = len(sys.argv)
	makeRepeatParams = 0
	params.quietMode = False
	while i < numOptions:
		arg = sys.argv[i]
		if params.rt_fileList and arg[0] == "-":
			raise OptionParseError("Option Error: All file names must follow all other params <%s>." % arg)

		if arg == "-h":
			print(__help__)
			sys.exit()

		elif arg == "-u":
			print(__usage__)
			sys.exit()

		elif arg == "-help_params":
			print(params_doc)
			sys.exit()

		elif arg[:2] == "--":
			name = arg[2:]
			i +=1
			if not hasattr(params, name):
				print("Error: low level parameter name %s does not exist in list of display parameters." % name)
				raise OptionParseError
			else:
				value = None
				arg = sys.argv[i]
				if name.endswith("RGB"):
					match = re.search(r"\(\s*([0-9.-]+)\s*,\s*([0-9.-]+)\s*,\s*([0-9.-]+)\s*\)", arg)
					if match:
						value = ( eval(match.group(1)), eval(match.group(2)), eval(match.group(3)) )
					else:
						print("Error: parameter %s must take an RGB triplet, enclosed in quotes, such as '( 1.0, 0, 0)' for red." % name)
						raise OptionParseError
				elif name == "pageSize":
					match = re.search(r"\(\s*([0-9.-]+)\s*,\s*([0-9.-]+)\s*\)", arg)
					if match:
						value = ( eval(match.group(1)), eval(match.group(2)) )
					else:
						print("Error: parameter %s must take an (x,y) pair, enclosed in quotes, such as ' (612.0, 792.0)'." % name)
						raise OptionParseError
				elif name.endswith("Font") or name.endswith("Path"):
					value = arg # it is text - don't need to convert
				else:
					# Must be numeric.
					try:
						value = int(arg)
					except ValueError:
						print("Error: value for parameter %s must be a number." % name)
						raise OptionParseError
				if value != None:
					exec("params.%s = value" % (name))

		elif arg == "-hintplot":
			exec("params." + kDrawTag + "EMBox = 1")
			exec("params." + kDrawTag + "Baseline = 0")
			exec("params." + kDrawTag + "ContourLabels = 0")
			exec("params." + kDrawPointTag + "PointLabels = 0")
			exec("params." + kShowMetaTag + "Outline = 0")
			exec("params." + kShowMetaTag + "Parts = 0")
			exec("params." + kShowMetaTag + "Paths = 0")
			exec("params." + kShowMetaTag + "Name = 1")
			exec("params." + kShowMetaTag + "BBox = 0")
			exec("params." + kShowMetaTag + "SideBearings = 1")
			exec("params." + kShowMetaTag + "V_SideBearings = 0")
			exec("params." + kShowMetaTag + "Parts = 0")
			exec("params." + kShowMetaTag + "Paths = 0")
			params.glyphsPerPage = 4
			params.pointLabelSize = 32
		elif arg == "-charplot":
			exec("params." + kDrawTag + "HHints = 0")
			exec("params." + kDrawTag + "VHints = 0")
			exec("params." + kDrawTag + "BlueZones = 0")
			exec("params." + kDrawTag + "Baseline = 0")
			exec("params." + kDrawTag + "EMBox = 0")
			params.pointLabel_doPointType  = 0 # add point type to point label .
			params.pointLabel_doPosition  = 1 # add point position to point label .
			params.pointLabel_doPointIndex  = 0 # add index to point label .
		elif arg == "-digiplot":
			params.rt_repeats = 2
			makeRepeatParams = 1
			exec("params." + kDrawTag + "HHints = 0")
			exec("params." + kDrawTag + "VHints = 0")
			exec("params." + kDrawTag + "BlueZones = 0")
			exec("params." + kDrawTag + "Baseline = 0")
			exec("params." + kDrawTag + "EMBox = 1")
			exec("params." + kDrawTag + "XAdvance = 1")
			exec("params." + kDrawTag + "CenterMarks = 1")
			exec("params." + kDrawTag + "ContourLabels = 0")
			exec("params." + kDrawTag + "Outline = 1")
			exec("params." + kDrawPointTag + "PointMarks = 0")
			exec("params." + kDrawPointTag + "BCPMarks = 0")
			exec("params." + kDrawPointTag + "PointLabels = 0")
			params.openPDFWhenDone = 0
			params.glyphsPerPage = 2
			params.descenderSpace = -120
			params.userBaseLine = -120
			# params.errorLogFilePath = "Error_file.log"
			params.errorLogColumnHeight = 250
			params.metaDataAboveGlyph = 0 # write meta data below glyph square.
			params.userPtSize = 255
			params.pageTopMargin  = 0
			params.pageBottomMargin = 0
			params.pageLeftMargin = 20
			params.pageRightMargin  = 20
			params.glyphVPadding  = 0
			params.glyphHPadding   = 0

			rightGlyphParams = copy.copy(params)
			setDefaultDigiplotRightSideOptions(rightGlyphParams)
			params.rt_repeatParamList = [params, rightGlyphParams]

		elif arg == "-repeatIndex":
			index = sys.argv[i+1]
			try:
				index = eval(index)
				if (index < 0) or (index >= len(params.rt_repeatParamList)):
					raise IndexError
			except:
				logMsg( "\tError. Option '%s' must be followed by an index number value, 0 or 1." % (arg))
			params = params.rt_repeatParamList[index]


		elif arg == "-fontplot":
			exec("params." + kDrawTag + "HHints = 0")
			exec("params." + kDrawTag + "VHints = 0")
			exec("params." + kDrawTag + "BlueZones = 0")
			exec("params." + kDrawTag + "Baseline = 0")
			exec("params." + kDrawTag + "EMBox = 1")
			exec("params." + kDrawTag + "XAdvance = 1")
			exec("params." + kDrawTag + "ContourLabels = 0")
			exec("params." + kDrawTag + "Outline = 1")
			exec("params." + kDrawPointTag + "PointMarks = 0")
			exec("params." + kDrawPointTag + "BCPMarks = 0")
			exec("params." + kDrawPointTag + "PointLabels = 0")
			exec("params." + kShowMetaTag + "Outline = 0") # shows filled outline in  lower right of meta data area under glyph outline.
			exec("params." + kShowMetaTag + "Name = 1")
			exec("params." + kShowMetaTag + "BBox = 0")
			exec("params." + kShowMetaTag + "SideBearings = 1")
			exec("params." + kShowMetaTag + "V_SideBearings = 0")
			exec("params." + kShowMetaTag + "Parts = 0")
			exec("params." + kShowMetaTag + "Paths = 0")
			params.DrawFilledOutline = 1
			params.metaDataXoffset = 0
			params.metaDataTextSize = 80
			params.metaDataNameSize = 80
			params.glyphsPerPage = 156
		elif arg == "-fontplot2":
			exec("params." + kDrawTag + "HHints = 0")
			exec("params." + kDrawTag + "VHints = 0")
			exec("params." + kDrawTag + "BlueZones = 0")
			exec("params." + kDrawTag + "Baseline = 0")
			exec("params." + kDrawTag + "EMBox = 0")
			exec("params." + kDrawTag + "XAdvance = 0")
			exec("params." + kDrawTag + "ContourLabels = 0")
			exec("params." + kDrawTag + "Outline = 1")
			exec("params." + kDrawTag + "CenterMarksWithBox = 1")
			exec("params." + kDrawPointTag + "PointMarks = 0")
			exec("params." + kDrawPointTag + "BCPMarks = 0")
			exec("params." + kDrawPointTag + "PointLabels = 0")
			exec("params." + kShowMetaTag + "Outline = 0") # shows filled outline in  lower right of meta data area under glyph outline.
			exec("params." + kShowMetaTag + "Name = 1")
			exec("params." + kShowMetaTag + "BBox = 0")
			exec("params." + kShowMetaTag + "SideBearings = 1")
			exec("params." + kShowMetaTag + "V_SideBearings = 0")
			exec("params." + kShowMetaTag + "Parts = 0")
			exec("params." + kShowMetaTag + "Paths = 0")
			exec("params." + kShowMetaTag + "Hints = 0")
			params.DrawFilledOutline = 1
			params.metaDataXoffset = 0
			params.metaDataTextSize = 80
			params.metaDataNameSize = 80
			params.glyphsPerPage = 156
			params.metaDataAboveGlyph = 0
		elif arg == "-fontsetplot":
			exec("params." + kDrawTag + "HHints = 0")
			exec("params." + kDrawTag + "VHints = 0")
			exec("params." + kShowMetaTag + "Hints = 0")
			exec("params." + kDrawTag + "BlueZones = 0")
			exec("params." + kDrawTag + "Baseline = 0")
			exec("params." + kDrawTag + "EMBox = 0")
			exec("params." + kDrawTag + "XAdvance = 0")
			exec("params." + kDrawTag + "ContourLabels = 0")
			exec("params." + kDrawTag + "Outline = 1")
			exec("params." + kDrawPointTag + "PointMarks = 0")
			exec("params." + kDrawPointTag + "BCPMarks = 0")
			exec("params." + kDrawPointTag + "PointLabels = 0")
			exec("params." + kShowMetaTag + "Outline = 0") # shows filled outline in  lower right of meta data area under glyph outline.
			exec("params." + kShowMetaTag + "Name = 0")
			exec("params." + kShowMetaTag + "BBox = 0")
			exec("params." + kShowMetaTag + "SideBearings = 0")
			exec("params." + kShowMetaTag + "V_SideBearings = 0")
			exec("params." + kShowMetaTag + "Parts = 0")
			exec("params." + kShowMetaTag + "Paths = 0")
			exec("params." + kShowMetaTag + "Hints = 0")
			params.DrawFilledOutline = 1
			params.rt_doFontSet = 1
			if params.userPtSize == None: # set default point size.
				params.userPtSize = 12
		elif arg == "-alpha":
			params.rt_alphaSort = 1
		elif arg == "-waterfallplot":
			if not params.waterfallRange:
				params.waterfallRange = (36,24,20,18,16,14,12,10,9,8,7,6)
		elif arg == "-wfr":
			i = i +1
			sizeString = sys.argv[i]
			if sizeString[0] == "-":
				raise OptionParseError("Option Error: it looks like the first item in the waterfall size list following '-wfr' is another option.")
			params.waterfallRange = parsePtSizeListArg(sizeString)
		elif arg == "-gpp":
			i = i +1
			try:
				params.glyphsPerPage = int( sys.argv[i])
			except ValueError:
				raise OptionParseError("Option Error: -glyphsPerPage must be followed by an integer number")
		elif arg == "-pt":
			i = i +1
			try:
				params.userPtSize = int( sys.argv[i])
			except ValueError:
				raise OptionParseError("Option Error: -pt must be followed by an integer number")
		elif arg == "-g":
			i = i +1
			glyphString = sys.argv[i]
			if glyphString[0] == "-":
				raise OptionParseError("Option Error: it looks like the first item in the glyph list following '-g' is another option.")
			params.rt_optionGlyphList += parseGlyphListArg(glyphString)
		elif arg == "-gf":
			i = i +1
			rt_filePath = sys.argv[i]
			if rt_filePath[0] == "-":
				raise OptionParseError("Option Error: it looks like the the glyph list file following '-gf' is another option.")
			try:
				gf = open(rt_filePath, "rt")
				glyphString = gf.read()
				gf.close()
			except (IOError,OSError):
				raise OptionParseError("Option Error: could not open glyph list file <%s>." %  rt_filePath)
			params.rt_optionGlyphList += parseGlyphListArg(glyphString)
		elif arg == "-o":
			i = i +1
			params.rt_pdfFileName = sys.argv[i]
		elif arg == "-dno":
			params.openPDFWhenDone = 0
		elif arg == "-do":
			params.openPDFWhenDone = 1
		elif arg == "-q":
			params.quietMode = True
		elif arg == "-black":
			params.emboxColorRGB = params.baselineColorRGB = params.xAdvanceColorRGB = params.contourLabelColorRGB = params.pointLabelColorRGB = params.MetaDataGlyphColorRGB = (0,0,0)
			params.hintColorRGB = params.hintColorOverlapRGB = params.alignmentZoneColorRGB = (0.5,0.5,0.5)
		elif arg == "-lf":
			i = i +1
			rt_filePath = sys.argv[i]
			if rt_filePath[0] == "-":
				raise OptionParseError("Option Error: it looks like the the layout file following '-gf' is another option.")
			if not os.path.exists(rt_filePath):
				raise OptionParseError("Option Error: The layout file %s does not exist." % (rt_filePath))
			params.rt_optionLayoutDict = parseLayoutFile(rt_filePath)
			exec("params." + kShowMetaTag + "WidthOnly = 1")
			exec("params." + kShowMetaTag + "HintDir = 1")
			exec("params." + kShowMetaTag + "RowFont = 1")
			exec("params." + kShowMetaTag + "Hints = 0")
			exec("params." + kShowMetaTag + "SideBearings = 0")
			exec("params." + kShowMetaTag + "V_SideBearings = 0")

		elif arg == "-select_hints":
			i = i +1
			indexList = sys.argv[i]
			indexList = re.findall(r"([^,]+)", indexList)
			try:
				indexList = map(int, indexList)
				params.rt_hintTableList = indexList
			except ValueError:
				raise OptionParseError("Option Error: in \" -select_hints %s\,  one of the indices in the argument list is not an integer." %   sys.argv[i])

		elif arg == "-baseline":
			i = i +1
			try:
				params.userBaseLine= int( sys.argv[i])
			except ValueError:
				raise OptionParseError("Option Error: -pt must be followed by an integer number")
		elif arg == "-v":
			exec("params." + kShowMetaTag + "SideBearings = 0")
			exec("params." + kShowMetaTag + "V_SideBearings = 1")
		elif arg == "-vh":
			exec("params." + kShowMetaTag + "SideBearings = 1")
			exec("params." + kShowMetaTag + "V_SideBearings = 1")
		elif arg[0] == "-":
			raise OptionParseError("Option Error: Unknown option <%s>." %  arg)
		else:
			params.rt_fileList += [arg]
		i  += 1
	if not params.rt_fileList:
		raise OptionParseError("Option Error: You must provide at least one font file path.")

	return params


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

def filterGlyphList(params, fontGlyphList, fontFileName):
	# Return the list of glyphs which are in the intersection of the argument list and the glyphs in the font
	# Complain about glyphs in the argument list which are not in the font.
	if not params.rt_optionGlyphList:
		glyphList = fontGlyphList
	else:
		# expand ranges:
		glyphList = []
		for glyphTag in params.rt_optionGlyphList:
			glyphNames = getGlyphNames(glyphTag, fontGlyphList, fontFileName)
			if glyphNames != None:
				glyphList.extend(glyphNames)
	return glyphList


def openFile(path, txPath):
	# If input font is  CFF or PS, build a dummy ttFont.
	tempPathCFF = None
	cffPath = None

	# If it is CID-keyed font, we need to convert it to a name-keyed font. This is a hack, but I really don't want to add CID support to
	# the very simple-minded PDF library.
	command="%s   -dump -0  \"%s\" 2>&1" % (txPath, path)
	report = fdkutils.runShellCmd(command)
	if "CIDFontName" in report:
		tfd,tempPath1 = tempfile.mkstemp()
		os.close(tfd)
		command="%s   -t1 -decid -usefd 0  \"%s\" \"%s\" 2>&1" % (txPath, path, tempPath1)
		report = fdkutils.runShellCmd(command)
		if "fatal" in report:
			logMsg(report)
			logMsg("Failed to convert CID-keyed font %s to a temporary Typ1 data file." % path)

		tfd,tempPathCFF = tempfile.mkstemp()
		os.close(tfd)
		command="%s   -cff +b  \"%s\" \"%s\" 2>&1" % (txPath, tempPath1, tempPathCFF)
		report = fdkutils.runShellCmd(command)
		if "fatal" in report:
			logMsg(report)
			logMsg("Failed to convert CID-keyed font %s to a temporary CFF data file." % path)
		cffPath = tempPathCFF
		os.remove(tempPath1)

	elif os.path.isdir(path):
		# See if it is a UFO font by truing to dump it.
		command="%s   -dump -0  \"%s\" 2>&1" % (txPath, path)
		report = fdkutils.runShellCmd(command)
		if not "sup.srcFontType" in report:
			logMsg(report)
			logMsg("Failed to open directory %s as a UFO font." % path)

		tfd,tempPathCFF = tempfile.mkstemp()
		os.close(tfd)
		command="%s   -cff +b  \"%s\" \"%s\" 2>&1" % (txPath, path, tempPathCFF)
		report = fdkutils.runShellCmd(command)
		if "fatal" in report:
			logMsg(report)
			logMsg("Failed to convert ufo font %s to a temporary CFF data file." % path)
		cffPath = tempPathCFF

	else:
		try:
			with open(path, "rb") as ff:
				head = ff.read(10)
		except (IOError, OSError):
			traceback.print_exc()
			raise FontError("Failed to open and read font file %s. Check file/directory permissions." % path)

		if len(head) < 10:
			raise FontError("Error: font file was zero size: may be a resource fork font, which this program does not process. <%s>." % path)

		# it is an OTF/TTF font, can process file directly
		elif head[:4] in (b"OTTO", b"true", b"\0\1\0\0"):
			try:
				ttFont = TTFont(path)
			except (IOError, OSError):
				raise FontError("Error opening or reading from font file <%s>." % path)
			except TTLibError:
				raise

			if 'CFF ' not in ttFont and 'glyf' not in ttFont:
				raise FontError("Error: font is not a CFF or TrueType font <%s>." % path)

			return ttFont, tempPathCFF

		# It is not an SFNT file.
		elif head[0:2] == b'\x01\x00': # CFF file
			cffPath = path

		elif b"%" not in head:
			# not a PS file either
			logMsg("Font file must be a PS, CFF or OTF  fontfile: %s." % path)
			raise FontError("Font file must be PS, CFF or OTF file: %s." % path)

		else:  # It is a PS file. Convert to CFF.
			tfd,tempPathCFF = tempfile.mkstemp()
			os.close(tfd)
			cffPath = tempPathCFF
			command="%s   -cff +b  \"%s\" \"%s\" 2>&1" % (txPath, path, tempPathCFF)
			report = fdkutils.runShellCmd(command)
			if "fatal" in report:
				logMsg("Attempted to convert font %s  from PS to a temporary CFF data file." % path)
				logMsg(report)
				raise FontError("Failed to convert PS font %s to a temp CFF font." % path)

	# now package the CFF font as an OTF font
	with open(cffPath, "rb") as ff:
		data = ff.read()
	try:
		ttFont = TTFont()
		cffModule = getTableModule('CFF ')
		cffTable = cffModule.table_C_F_F_('CFF ')
		ttFont['CFF '] = cffTable
		cffTable.decompile(data, ttFont)
	except:
		traceback.print_exc()
		logMsg("Attempted to read font %s  as CFF." % path)
		raise FontError("Error parsing font file <%s>." % path)
	return ttFont, tempPathCFF


def proofMakePDF(pathList, params, txPath):
	#    use fontTools library to open font and extract CFF table.
	#    If error, skip font and report error.
	if params.rt_doFontSet:
		pdfFontList = []
		logMsg("")
		logMsg( "Collecting font data from:")
		fontCount = 0
		for path in pathList:
			fontFileName = os.path.basename(path)
			params.rt_filePath = os.path.abspath(path)
			logMsg( "\t%s." % (path))
			try:
				ttFont, tempPathCFF = openFile(path, txPath)
			except FontError:
				print(traceback.format_exception_only(sys.exc_type, sys.exc_value)[-1])
				return
			try:
				fontGlyphList = ttFont.getGlyphOrder()
			except FontError:
				print(traceback.format_exception_only(sys.exc_type, sys.exc_value)[-1])
				return


			#   filter specified list, if any, with font list.
			glyphList = filterGlyphList(params, fontGlyphList, fontFileName)
			if not glyphList:
				raise FontError("Error: selected glyph list is empty for font <%s>." % fontFileName)
			params.rt_reporter = logMsg

			if ttFont.has_key("CFF "):
				pdfFont = otfpdf.txPDFFont(ttFont, params)
			elif ttFont.has_key("glyf"):
				pdfFont = ttfpdf.txPDFFont(ttFont, params)
			else:
				logMsg( "Quitting. Font type is not recognized. %s.." % (path))
				break
			if tempPathCFF:
				pdfFont.path = tempPathCFF
			pdfFontList.append([glyphList, pdfFont, tempPathCFF])

		doProgressBar = not params.quietMode
		pdfFilePath = makeFontSetPDF(pdfFontList, params, doProgressBar)
		for entry in pdfFontList:
			tempPathCFF =  entry[2]
			pdfFont = entry[1]
			pdfFont.clientFont.close()
			if tempPathCFF:
				os.remove(tempPathCFF)

		logMsg( "Wrote proof file %s. End time: %s." % (pdfFilePath, time.asctime()))
		if pdfFilePath and params.openPDFWhenDone:
			if curSystem == "Windows":
				command = 'start "" "%s"' % pdfFilePath
				fdkutils.runShellCmdLogging(command)
			elif curSystem == "Linux":
				command = 'xdg-open "%s" &' % pdfFilePath
				fdkutils.runShellCmdLogging(command)
			else:
				command = 'open "%s" &' % pdfFilePath
				fdkutils.runShellCmdLogging(command)

	else:
		tmpList = []
		for path in pathList:
			fontFileName = os.path.basename(path)
			params.rt_filePath = os.path.abspath(path)
			if not params.quietMode:
				logMsg("")
				logMsg( "Proofing font %s. Start time: %s." % (path, time.asctime()))
			try:
				ttFont, tempPathCFF = openFile(path, txPath)
				fontGlyphList = ttFont.getGlyphOrder()
			except FontError:
				print(traceback.format_exception_only(sys.exc_type, sys.exc_value)[-1])
				return

			#   filter specified list, if any, with font list.
			params.rt_glyphList = filterGlyphList(params, fontGlyphList, fontFileName)
			if not params.rt_glyphList:
				raise FontError("Error: selected glyph list is empty for font <%s>." % fontFileName)
			params.rt_reporter = logMsg

			if "CFF " in ttFont:
				pdfFont = otfpdf.txPDFFont(ttFont, params)
			elif "glyf" in ttFont:
				pdfFont = ttfpdf.txPDFFont(ttFont, params)
			else:
				logMsg( "Quitting. Font type is not recognized. %s.." % (path))
				return

			if tempPathCFF:
				pdfFont.path = tempPathCFF
			doProgressBar = not params.quietMode

			pdfFilePath = makePDF(pdfFont, params, doProgressBar)
			ttFont.close()

			if tempPathCFF:
				os.remove(tempPathCFF)

			if not params.quietMode:
				logMsg("Wrote proof file %s. End time: %s." % (pdfFilePath, time.asctime()))
			if pdfFilePath and params.openPDFWhenDone:
				if curSystem == "Windows":
					command = 'start "" "%s"' % pdfFilePath
					fdkutils.runShellCmdLogging(command)
				elif curSystem == "Linux":
					command = 'xdg-open "%s" &' % pdfFilePath
					fdkutils.runShellCmdLogging(command)
				else:
					command = 'open "%s" &' % pdfFilePath
					fdkutils.runShellCmdLogging(command)


def charplot():
	sys.argv.insert(1, "-charplot")
	main()


def digiplot():
	sys.argv.insert(1, "-digiplot")
	main()


def fontplot():
	sys.argv.insert(1, "-fontplot")
	main()


def fontplot2():
	sys.argv.insert(1, "-fontplot2")
	main()


def fontsetplot():
	sys.argv.insert(1, "-fontsetplot")
	main()


def hintplot():
	sys.argv.insert(1, "-hintplot")
	main()


def waterfallplot():
	sys.argv.insert(1, "-waterfallplot")
	main()


def main():
	try:
		txPath = CheckEnvironment()
	except FDKEnvironmentError as e:
		print(e)
		return
	try:
		params = getOptions(FontPDFParams())
	except OptionParseError as e:
		print(e)
		print(__usage__)
		return


	# verify that all files exist.
	haveFiles = 1
	for path in params.rt_fileList:
		if not os.path.exists(path):
			print("File does not exist: <%s>." % path)
			haveFiles = 0
	if not haveFiles:
		return

	try:
		proofMakePDF(params.rt_fileList, params, txPath)
	except (FontError, NotImplementedError) as e:
		print("\t exception %s" % e)
		traceback.print_exc()
	return


if __name__ == '__main__':
	main()
