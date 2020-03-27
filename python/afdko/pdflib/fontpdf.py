"""
fontpdf v1.30 Dec 10 2019. This module is not run stand-alone; it requires
another module, such as proofpdf, in order to collect the options, and call
the MakePDF function.

Prints PDF proof sheet for a font.  It will print a standard title at
the top of the page, and then draws the requested number of glyphs per
page. Each glyph  is an object that draws itself and some meta data in a
 kGlyphSquare x  (kGlyphSquare + rt_metaDataYOffset)
rectangle, where rt_metaDataYOffset is the extra space needed for the
meta data. The script scales the output of each glyph object so that  it
will fit in the specified number of glyphs per page.  What actually gets
drawn for each glyph depends on the options chosen.

The caller of this module must provide the font and glyph data by
sub-classing the FontPDFFont and PDFGlyph classes, and supplying
implementations of the client functions. It should set the fields of the
FontPDFParams instance as desired.

All coordinates are in points, in the usual PostScript model. Note,.
however, that the coordinate system for the page puts (0,0) at the top
right, with the positive Y axis pointing down.
"""
from math import ceil
import os
import re
import time

from fontTools.pens.basePen import BasePen

from afdko import fdkutils
from afdko.pdflib import pdfgen, pdfmetrics
from afdko.pdflib.pdfutils import LINEEND

__version__ = "1.30"

__copyright__ = """Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
"""


params_doc = """
The following are the list of low-level parameters, along with the
default values for the fontplot mode.

The page is built as an array of glyph tiles on the page. Each glyph
tile contains the main outline, with a block of text (the meta data
about the glyph) placed either above or below it.

Many values must be set to either 0 or 1, where 0 means "off" and "1" means "on."

Items that are  expressed design space units are in units in a 1000 unit
em-square, and scale up and down with the size of the glyph tile. Items
that are expressed in points are in units of 1/72 of an inch, and are
relative to PostScript page coordinate system.

Color values are set as a triplet of (Red, Green, Blue) values between 0 (dark)
and 1.0 (light). Examples:
(1.0,0,0) is red
(1.0, 1.0, 1.0) is white
(0, 0, 0) is black

For the FontLab macros, color values be entered as in the examples; for
the command-line tools, the values be enclosed in quotes, such as:
"(1.0, 0, 0)"

--openPDFWhenDone  1  # 0 or 1. Whether to open the PDF file after it is written.


# Page attributes
--pageSize  (612.0, 792.0)            # Integer pair. Size of page in points.
                                       Must be specifed on a command-line
                                       as "(x,y)"
--pageTopMargin  36.0                 # Integer. Point size
--pageBottomMargin  36.0              # Integer. Point size
--pageLeftMargin  36.0                # Integer. Point size
--pageRightMargin  36.0               # Integer. Point size
--pageTitleFont  Times-Bold           # Text string. Font for title
--pageTitleSize  14                   # Integer. Point size used in title
--pageIncludeTitle  1                 # 0 or 1. Whether to include a title on each page
--fontsetGroupPtSize  14              # Integer. Point size for group header and PS name in fontsetplot.

# Page layout attributes
--glyphsPerPage  156                  # Integer. default number of glyphs
                                       per page. Set by the "-gpp <number>"
                                       option.
--userPtSize  None                    # Integer. Point size for glyphs .
                                       Usually calculated from glyphs per
                                       page. Set by the "-pt <number>" option.
--glyphHPadding  150                  # Design space units. Spacing between
                                       glyph tiles on a line, in points.
--glyphVPadding  150                  # Design space units. Spacing  between
                                       lines of glyph tiles, in points.
--doAlphabeticOrder  0                # 0 or 1. Whether to show glyphs in
                                       alphabetic order. Does not work with
                                       CID-keyed fonts

# Main glyph tile elements
# All metrics are specified in design units for a 1000 em-square. Glyph
# data is scaled to a 1000-em square before these metrics are used.
--DrawFilledOutline  1                # 0 or 1. Whether to set whether the
                                       glyph outline is filled or just stroked.
--drawGlyph_Baseline  0               # 0 or 1. Whether to draw the glyph
                                       baseline as a horizontnal line the
                                       width of the em-box.
--drawGlyph_BlueZones  0              # 0 or 1. Whether to draw the font
                                       alignment zones
--drawGlyph_CenterMarks  0            # 0 or 1. Whether to center registration marks
--drawGlyph_GlyphBox  0               # 0 or 1. Whether to draw a box around
                                       the glyph, height = em-square , width=x-advance.
--drawGlyph_CenterMarksWithBox  0     # 0 or 1. Do drawGlyph_GlyphBox and drawGlyph_CenterMarks
--drawGlyph_ContourLabels  0          # 0 or 1. Whether to draw the labels
                                       for each contour
--drawGlyph_EMBox  1                  # 0 or 1. Whether to draw  the em-box
                                       square, startig at the glyph origin.
--drawGlyph_HHints  0                 # 0 or 1. Whether to draw the horizontal
                                       hint zones
--drawGlyph_Outline  1                # 0 or 1. Whether to draw the glyph.
--drawGlyph_VHints  0                 # 0 or 1. Whether to draw the vertical
                                       hint zones
--drawGlyph_XAdvance  1               # 0 or 1. Whether to draw a T marking
                                       both the glyph origin and the x-advance.

--vhintYOffset  10                    # Design space units. default offset
                                       of hint zone; successive zones are
                                       staggered so you can see the overlap more easily.
--hhintXOffset  10                    # Design space units. default offset
                                       of hint zone; successive zones are
                                       staggered so you can see the overlap more easily.
--descenderSpace  None                # Design space units. Space to allow
                                       under the origin to allow for descenders.
                                       Also sets the space from the
                                       origin to the meta data text,
                                       when the latter is below the
                                       glyph. When set to None, the
                                       lowest point in the font is used,
                                       from the font bounding box.


--alignmentZoneColorRGB  (0, 0, 0.75) # Triplet of RGB values. Color of alignment
                                       zones.
--emboxColorRGB  (0.25, 0.25, 0.5)    # Triplet of RGB values. Color of em-box
                                       rectangle.
--hintColorOverlapRGB  (0.6, 0.6, 0)  # Triplet of RGB values. Color for
                                       area where hint zones overlap.
--hintColorRGB  (0, 0.75, 0)          # Triplet of RGB values. Color for
                                       hint zones.
--xAdvanceColorRGB  (0.5, 0, 0)       # Triplet of RGB values. Color of ticks
                                       marks for the x-advance.

# Attributes for glyph path points.
--drawPoint_PointMarks  0             # 0 or 1. Whether to draw marks to
                                       indicate the location of the  points.
--drawPoint_BCPMarks  0               # 0 or 1. Whether to draw marks to
                                       indicate the location of the bezier
                                       control points.
--drawPoint_PointLabels  0            # 0 or 1. Whether to draw a label for
                                       each point.

--pointLabel_doPointIndex  1          # 0 or 1. In the point label, whether
                                       to show its path index number.
--pointLabel_doPointType  1           # 0 or 1. In the point label, whether
                                       to show the sgment type ( c for curve,
                                       l for line, m for move-to).
--pointLabel_doPosition  1            # 0 or 1. In the point label, whether
                                       to show the (x,y) position.
--pointLabelFont  Helvetica           # Text string. Font for point and contour
                                       label text.
--pointLabelSize  16                  # Design space units. Point size used
                                       for point labels.
--pointClosingArrowLength  15         # Design space units. Point size. Length
                                       of arrow used to show closing contour
                                       path segment.
--pointLabel_LineLength  10           # Design space units. Length of tick
                                       mark connecting a point to its label.
--pointLabel_BCP_CrossSize  5         # Design space units. Size of cross
                                       used to mark a BCP, in points.
--pointMarkDiameter  2                # Design space units. Size of dot used
                                       to mark a point, in points.

--contourLabelColorRGB  (0.5, 0, 0)   # Triplet of RGB values. Color of the
                                       counter lables (P 0, P 1, etc).
--pointLabelColorRGB  (0.5, 0, 0)     # Triplet of RGB values. Color of point
                                       labels in glyph contour

--markLineWidth 1                     # The width of the lines used to connect labels to points

# Attributes for descriptive text.
--drawMeta_Name  1                    # 0 or 1. Whether to write the glyph
                                       name at the start of the meta data
                                       text.
--drawMeta_BBox  0                    # 0 or 1. Whether to write the font
                                       bounding box info, in the meta data
                                       text.
--drawMeta_HintDir  0                 # 0 or 1. Whether to write the hint
                                       directory from the CID layout file
                                       (CID-keyed fonts only, requires
                                       layout file path to be specified').
--drawMeta_Hints  1                   # 0 or 1. Whether to write the number
                                       of vertical and horizontal hints,
                                       in the meta data text.
--drawMeta_Parts  0                   # 0 or 1. Whether to write the number
                                       of path segments of each type, in
                                       the meta data text.
--drawMeta_Paths  0                   # 0 or 1. Whether to write the number
                                       of contours, in the meta data text.
--drawMeta_RowFont  0                 # 0 or 1. Whether to write the row
                                       font name and glyph name from the
                                       CID layout file (CID-keyed fonts
                                       only, requires layout file path
                                       to be specified').
--drawMeta_SideBearings  1            # 0 or 1. Whether to write the left
                                       and right side bearings and the x-advance,
                                       in the meta data text.
--drawMeta_V_SideBearings  0          # 0 or 1. Whether to write the top
                                       and bottom side bearings and the y-advance,
                                       in the meta data text.
--drawMeta_WidthOnly  0               # 0 or 1. Whether to write the x-advance
                                       width and the y-advance width, in
                                       the meta data text.

--drawMeta_Outline  0                 # 0 or 1. Whether to draw a small filled
                                       glyph next to the meta data text.
--metaDataFilledGlyphSize  320        # Design space units. Point size for
                                       the small filled glyph next to the
                                       meta data text.
--MetaDataGlyphColorRGB  (0, 0, 0)    # Triplet of RGB values. Color of the
                                       small filled glyph shown next to the
                                       meta data text block when
                                       drawMeta_Outline is set to 1

--metaDataAboveGlyph  1               # 0 or 1. Whether to write the meta
                                       data text above or below the glyph.
--metaDataFont  Times-Bold            # Text string. Font used to write the
                                       meta data text.
--metaDataNameSize  80                # Design space units. Point size used
                                       for the glyph name in the meta data
                                       text
--metaDataTextSize  80                # Design space units. Point size used
                                       for all other items in the meta data
                                       text
--metaDataXoffset  0                  # Design space units. Point size used
                                       to offset the meta data block to the
                                       right from the glyph origin.


Attributes for log file text.
This is shown only in digiplot mode, when each glyph is displayed in a
side-by-side pair. The meta data is written under the right glyph , and
the log file text is written under the left glyph. The text is then read
from the file path named in the errorLogFilePath parameter. Each line is
assumed to be for one glyph. The line must be a series of text tokens
separated by white-space. The first token is assumed to be the glyph
name. The remaining tokens must be either words or x, y coordinate
pairs. The text for each glyph is written under the glyph in columns,
one word or (x,y) coordinate pair per line.
--errorLogFilePath  Error_file.log    # Text string. File path to the error
                                       log file.
--errogLogFont  Times-Bold            # Text string. Font used to write the
                                       text from the error log file
--errorLogColumnHeight  250           # Design space units. column height
                                       for the error log file
--errorLogColumnSpacing  20           # Design space units. column width
                                       for the error log file
--errorLogPointSize  30               # Design space units. Point size for
                                       text used for error log report.

Examples:
--pageTitleFont "MinionPro-Bold"  #Change the page title font.
--pointLabelFont "Times-Bold"  #Change the font for point labels
--pointLabelColorRGB "(1.0, 0, 0)" #Change the color of the point labels
to red. The value is a triplet of (Red Green, Blue) values, where 0 is
dark and 1.0 is light. Note that the RGB values must be enclosed in
quotes and parentheses, and commas separated, as shown. "(0,0,0)" is
black, "(1.0, 1.0, 1.0)" is page white, "(1.0, 0, 0)" is red.
--pointLabelSize 12  #Change the size of the point label text.
"""

inch = INCH = 72
cm = CM = inch / 2.54
kShowMetaTag = "drawMeta_"
kDrawTag = "drawGlyph_"
kDrawPointTag = "drawPoint_"
kGlyphSquare = 1000
kLineWidth = 0.55
class FontPDFParams:
	def __init__(self):
		# fields beginnin gwith 'rt_' are filled in at run-time - don't try to overriide!
		self.rt_yMin = 0 # minimum of 0 or the font BBox y-min, or the value of descenderSpace.
		self.rt_canvas = None # the rt_canvas object from pdfgen.py
		self.rt_reporter = None # the logging function
		self.rt_filePath ="" 	# path of current font file.
		self.rt_pdfFileName ="" #  output PDf file name. Default is  is font file name + ".pdf"
		self.rt_repeats = 1 # used in the digiplot mode: a glyph is shown more than once, with different parameters.
		self.rt_repeatParamList = [] # a list of FontPDFParams, used only when self.rt_repeats > 0:
							# this happens when a glyph is drawn more than once, and
							# allows different options to be used with each.
		self.rt_errorLogDict = None # a dict, keyed by glyph name. Each value is a list of short strings. If this is not None, then the display of other meta data
							# is supressed, and the error list is shown iinstead.
		self.rt_fileList = []		# list of file to proof
		self.rt_optionGlyphList = [] # list of glyphs requested by user in options
		self.rt_glyphList = [] # actual set of glyphs we will proof: the intersection of rt_optionGlyphList and glyphs that are in the font.
		self.rt_alphaSort = 0 # Sort glyph names alphabetically
		self.rt_doFontSet = 0 # show just one line of glyphs per font.
		self.rt_optionLayoutDict = None # Dictionary representing a layout file for a CID font. If present, use the specified CID order, and
										# show the hint dir and row font dir names for each glyph.
		self.rt_hintTableList  = [] # List of hint replacement block inidicies. Plot only these hints. Set only by  arguments to program.
		self.rt_maxHintLabelWidth = None
		self.rt_glyphHPadding = None # Extra space around glyph; the padding that is used in drawing. When showing hints, this is larger than the original parameter value.
		self.rt_glyphVPadding = None # Extra space around glyph
		self.rt_emBoxScale = None # A scaling factor to allow for the difference between the std 1000 em-square and the font's em-square.
		self.rt_maxHintLabelWidth = None #  This calculated and used only when showing h hintsl is the added to self.glyphHPadding to set self.rt_glyphHPadding.


		# real parameters
		self.doAlphabeticOrder = 0 # show glyph list in alphabetic order rather than GI order.
		self.glyphsPerPage = 1	# glyphs per page
		self.userPtSize = None	# alternate way to set glyphs per page. If not None, then glyphsPerPage s calculated this way.
		self.userBaseLine = None	# Override the default base-line which is deriived from the font.
		self.openPDFWhenDone = 1 # Try and open the resulting PDF file with whatever app is used to open pdf's.

		self.descenderSpace = None # The amout of space allowed for descenders. By default is  Font BBox.ymin, but can be set by parameter.
		self.pageTitleFont = 'Times-Bold'  # Font used for page titles.
		self.pageTitleSize = 14		# point size for page titles
		self.pageIncludeTitle = 1  # include or not the page titles
		self.fontsetGroupPtSize = 14 # pt size for group header text font fontsetplot
		self.pointLabelFont = 'Helvetica'  # Font used for all text in glyph tile.
		self.pointLabelSize = 16 # point size for all text in glyph tile. This is is relative to a glyph tile fo width kGlyphSquare;
						   # the label point size gets scaled with the scalin gof the glyph tile.

		 # page parameters
		self.pageRightMargin = inch*0.5
		self.pageLeftMargin = inch*0.5
		self.pageTopMargin = inch*0.5
		self.pageBottomMargin = inch*0.5
		self.pageSize = (8.5 *inch, 11.0*inch)

		self.pointMarkDiameter = 3 # size of circle to used to mark a point.
		self.pointLabel_BCP_CrossSize = 5 # length of arms of cross that marks bezier contolr point (BCP)  position..
		self.pointLabel_doPointType  = 1 # add point type to point label .
		self.pointLabel_doPosition  = 1 # add point position to point label .
		self.pointLabel_doPointIndex  = 1 # add index to point label .
		self.pointLabel_LineLength = 10 # length of tick mark from a point to its label
		self.pointClosingArrowLength = 15 # length of direction arrow, drawn pointing to the first poin on each path.

		# Whether ot not to draw specified object. Fuctions beginning with kDrawTag and kDrawPointTag are drawn wiithin
		# the scale/offset paramters used for drawing the glyph outline
		# If you want to add a new drawing function, the paramter name here must match
		# the function name in the fontPDFGlyph class. and must be added to the sortedMethodsDraw list.
		self.glyphHPadding = 150 # Extra space around glyph
		self.glyphVPadding = 150 # Extra space around glyph
		self.DrawFilledOutline = 0 # fill main outline
		self.hintColorRGB = (0, 0.75, 0) # RGB, on scale of 0.0 -1.0
		self.hintColorOverlapRGB = 0.60, 0.60, 0
		self.alignmentZoneColorRGB =  (0, 0, 0.75)
		self.emboxColorRGB = (0.25, 0.25, 0.5)
		self.baselineColorRGB = (0.5, 0.25, 0.25)
		self.xAdvanceColorRGB = (0.5, 0, 0)
		self.contourLabelColorRGB = (0.5, 0, 0)
		self.pointLabelColorRGB = (0.5, 0, 0)
		self.markLineWidth = 1
		self.MetaDataGlyphColorRGB = (0,0,0)
		self.hhintXOffset = 10
		self.vhintYOffset = 10
		exec("self." + kDrawTag + "HHints = 1")
		exec("self." + kDrawTag + "VHints = 1")
		exec("self." + kDrawTag + "BlueZones = 1")
		exec("self." + kDrawTag + "Baseline = 1")
		exec("self." + kDrawTag + "EMBox = 1")
		exec("self." + kDrawTag + "CenterMarks = 0")
		exec("self." + kDrawTag + "GlyphBox = 0")
		exec("self." + kDrawTag + "XAdvance = 0")
		exec("self." + kDrawTag + "CenterMarksWithBox = 0")
		exec("self." + kDrawTag + "ContourLabels = 1")
		exec("self." + kDrawTag + "Outline = 1")
		# sortedMethodsDraw controls the order in w the draw methods are executed. A draw function will not be executed if it
		# is not in this list.
		self.sortedMethodsDraw = ["EMBox", "Outline", "HHints", "VHints",  "BlueZones", "Baseline",  "CenterMarks", "CenterMarksWithBox", "GlyphBox", "XAdvance", "ContourLabels"]

		# Although the the same transform is used with the kDrawPointTag functions as with the kDrawTag
		# functions, any text point size is scaled back up by 1/params.emboxScale. so that the point is size is
		# relative to to the glyph tile, not the glyph outline. This is so that the text doesn't get tiny when showing a
		# glyph with  2048 embox.
		exec("self." + kDrawPointTag + "PointMarks = 1")
		exec("self." + kDrawPointTag + "BCPMarks = 1")
		exec("self." + kDrawPointTag + "PointLabels = 1")
		self.sortedMethodsDrawPoint = ["PointMarks", "BCPMarks",  "PointLabels"]


		# Whether or not to draw specified object. Fuctions beginning with kShowMetaTag  are drawn wiith
		# the scale/offset paramters used for drawing the glyph tile.  Pt sizes are relative to the  kGlyphSquare
		# embox, and are scaled with the glyph tile.
		# If you want to add a new drawing function, the paramter name here must match
		# the function name in the fontPDFGlyph class. and must be added to the sortedMethodsMeta list.

		# Meta data is shown under the glyph outline, in a single column. metaDataXoffset sets the left margin
		# of the column. I don't try toright justify, so a long line can run past the right edge of the glyph tile.
		# yMeta grows as needed to contain all the selected meta data entries.

		self.errorLogFilePath = "" # Set to an file path to show log text. Supresses any showMeta display
		self.errorLogColumnHeight= 250  # Used in calcualting yMETA. The area used for showing the log file text is this high,
						# and is the width of the log file.
		self.errogLogFont = self.pageTitleFont  # Font used for showing error log text. This is shown in the meta data area.
		self.errorLogPointSize = 30 # point size for error log text
		self.errorLogColumnSpacing = 20  # point size of spacing between columns  of error log text.

		self.metaDataAboveGlyph = 1 # if true, put meta data above the glyph square; else add it below the glyph square
		self.rt_metaDataYOffset = None # The addional space added to the height of the glyph tile, to accommodate
					#  the meta text block.
		self.metaDataXoffset = 400 # Width of left margin for meta data, relative to o 1000 pt glyph square
		self.metaDataFont = self.pageTitleFont
		self.metaDataTextSize = 24  # point size for meta list, relative to 1000 pt glyph square
		self.metaDataNameSize = self.metaDataTextSize*2 # the glyph name is shown at twice the point size of the other meta data entries.
		self.metaDataFilledGlyphSize = 250 # Point size of filled glyph, Shown in lower right of meta data area under glyph outline.
		exec("self." + kShowMetaTag + "Outline = 1") # shows filled outline in  lower right of meta data area under glyph outline.
		exec("self." + kShowMetaTag + "Name = 1")
		exec("self." + kShowMetaTag + "BBox = 1")
		exec("self." + kShowMetaTag + "SideBearings = 1")
		exec("self." + kShowMetaTag + "V_SideBearings = 1")
		exec("self." + kShowMetaTag + "Parts = 1")
		exec("self." + kShowMetaTag + "Paths = 1")
		exec("self." + kShowMetaTag + "Hints = 1")
		exec("self." + kShowMetaTag + "WidthOnly = 0")
		exec("self." + kShowMetaTag + "HintDir = 0")
		exec("self." + kShowMetaTag + "RowFont = 0")
		# sortedMethodsDraw controls the order in which the draw methods are executed. A draw function wil not be executed if it
		# is not in this list.
		self.sortedMethodsMeta = ["Outline", "Name",  "BBox", "SideBearings",  "V_SideBearings",  "Parts", "Paths", "Hints", "WidthOnly", "HintDir", "RowFont"]
		self.waterfallRange = None

class FontPDFFont:
	def __init__(self, clientFont, params):
		self.clientFont = clientFont
		if params.userBaseLine != None:
			self.baseLine = params.userBaseLine
		else:
			self.baseLine = None
		self.path = params.rt_filePath
		self.isCID = 0
		self.psName = None;
		self.OTVersion = None;
		self.emSquare = None
		self.bbox = None
		self.ascent = None
		self.descent = None
		self.blueZones = None
		self.getEmSquare()
		self.getBaseLine()
		self.getBBox()
		self.GetBlueZones()
		self.AscentDescent()
		return

	def getPSName(self):
		if not self.psName:
			self.psName = self.clientGetPSName()
		return self.psName

	def getOTVersion(self):
		if not self.OTVersion:
			self.OTVersion = self.clientGetOTVersion()
		return self.OTVersion

	def getGlyph(self, glyphName):
		return self.clientGetGlyph(glyphName)

	def getEmSquare(self):
		if not self.emSquare:
			self.emSquare = self.clientGetEmSquare()
		return self.emSquare

	def getBaseLine(self):
		if self.baseLine == None:
			self.baseLine = self.clientGetBaseline()
		return self.baseLine

	def getBBox(self):
		if not self.bbox:
			self.bbox = self.clientGetBBox()
		return self.bbox

	def GetBlueZones(self):
		if not self.blueZones:
			self.blueZones = self.clientGetBlueZones()
		return self.blueZones

	def AscentDescent(self):
		if  self.ascent == None:
			self.ascent, self.descent = self.clientGetAscentDescent()
		if self.ascent == None:
			emsquare = self.getEmSquare()
			bbox = self.getBBox()
			self.ascent = bbox[3]
			self.descent = bbox[1]
			if self.descent > 0:
				self.descent = 0
		return self.ascent, self.descent


	def clientGetPSName(self):
		raise NotImplementedError

	def clientGetOTVersion(self):
		raise NotImplementedError

	def clientGetGlyph(self, glyphName):
		# must return an object subclassed from FontPDFGlyph.
		raise NotImplementedError

	def clientGetEmSquare(self):
		raise NotImplementedError

	def clientGetBaseline(self):
		raise NotImplementedError

	def clientGetBBox(self):
		# must return bbox as [minX,minY, MaxX, maxY]
		raise NotImplementedError

	def clientGetBlueZones(self):
		# This must return a list of blueZone items, one item per fontDict in the font ( Type 1 fonts have 1 font dict, CDIF fonts have several).
		# Each Blue Zone item will be a list of tuples. Each tuple will be a coordinate pair, the absolute low and high values for an alignment zone.
		raise NotImplementedError

	def clientGetAscentDescent(self):
		raise NotImplementedError




class FontPDFPoint:
	MT = "m"
	LT = "l"
	CT = "c"
	def __init__(self, type, pt0, bcp1 = None, bcp2 = None, index = 0):
		self.index = index
		self.type = type
		self.pt0 = pt0
		self.bcp1 = bcp1
		self.bcp2 = bcp2
		self.last = None
		self.next = None

	def __repr__(self):
		text =  "type: %s, pt0 %s" % (self.type, self.pt0)
		if self.bcp1:
			text += " pt1: %s" % (self.bcp1,)
		if self.bcp2:
			text += " pt1: %s" % (self.bcp2,)
		return text

def makeVector(pt1, pt2):
	# return the normalized vector [dx, dy]
	dx = pt2[0] - pt1[0]
	dy = pt2[1] - pt1[1]
	d =  float(dx**2 + dy**2) ** 0.5
	vector = [1,1] # default value if d == 0
	if d != 0:
		vector[0] = dx/d
		vector[1] = dy/d
	return vector

def getTickPos(pt0, pt1, pt2, tickSize, pathisCW):
	vector0 = makeVector(pt0, pt1)
	vector1 = makeVector(pt1, pt2)
	sum = ( vector1[0] + vector0[0], vector1[1] + vector0[1])
	if pathisCW:
		# tickPos is 90 degrees counter-clockwise to the vector "sum"
		tickPos = ((pt1[0] -  (sum[1] * tickSize)), pt1[1] + (sum[0]*tickSize))
	else:
		# tickPos is 90 degrees clockwise to  to the vector "sum"
		tickPos = ((pt1[0] +  (sum[1] * tickSize)), pt1[1] - (sum[0]*tickSize))
	return tickPos


class FontPDFPen(BasePen):
    def __init__(self, glyphSet=None):
        BasePen.__init__(self, glyphSet)
        self.pathList = []
        # These all get set when the outline is drawn
        self.numMT = self.numLT = self.numCT = self.numPaths = self.total = 0
        self.curPt = [0, 0]
        self.noPath = 1

    def _moveTo(self, pt):
        if self.noPath:
            self.pathList.append([])
        self.noPath = 0
        self.numMT += 1
        pdfPoint = FontPDFPoint(FontPDFPoint.MT, pt, index=self.total)
        self.total += 1
        self.curPt = pt
        curPath = self.pathList[-1]
        curPath.append(pdfPoint)

    def _lineTo(self, pt):
        if self.noPath:
            self.pathList.append([])
        self.noPath = 0
        self.numLT += 1
        pdfPoint = FontPDFPoint(FontPDFPoint.LT, pt, index=self.total)
        self.total += 1
        self.pathList[-1].append(pdfPoint)
        self.curPt = pt

    def _curveToOne(self, pt1, pt2, pt3):
        if self.noPath:
            self.pathList.append([])
        self.numCT += 1
        pdfPoint = FontPDFPoint(
            FontPDFPoint.CT, pt3, pt1, pt2, index=self.total)
        self.total += 1
        self.pathList[-1].append(pdfPoint)
        self.curPt = pt3

    def _closePath(self):
        self.numPaths += 1
        self.noPath = 1

    def _endPath(self):
        self.numPaths += 1


class FontPDFGlyph:
	def __init__(self, parentFont, glyphName):
		self.name = glyphName
		self.parentFont = parentFont
		self.numMT = self.numLT =  self.numCT = self.numPaths = 0 # These all get set when the outline is drawn.
		self.pathList = [[]]
		self.hintTable = []
		self.vhints = []
		self.hhints = []
		self.BBox = [0,0,0,0] # [left, bottom, right, top], aka (xMin,rt_yMin, (xMax, yMax)
		self.xAdvance = 0
		self.yAdvance = 0
		self.yOrigin = 0
		self.isTT = 0 # used to determine path direction
		self.isCID = 0 # used to determine if the font is CID-keyed
		self.clientInitData()
		self.extraY = None # height needed to accomodate the meta data block.

	def clientInitData(self):
		# must override to fill in all the data items that are defined in the init function.
		raise NotImplementedError


	def draw(self, params, repeat):
		rt_canvas = params.rt_canvas
		rt_canvas.translate(params.rt_glyphHPadding, params.rt_glyphVPadding)

		scale = params.rt_emBoxScale
		glyphX = 0
		rt_yMin = params.rt_yMin
		rt_metaDataYOffset = params.rt_metaDataYOffset
		if self.extraY == None:
			extraY = params.rt_metaDataYOffset
			#if params.metaDataAboveGlyph:
			#	extraY -= params.rt_yMin # The total excursion of the glyph tile below the origin = (abs(font min) + yMetaHeight
			extraY += (params.pointLabel_LineLength + params.pointLabelSize)*2 # Need some margin for the labels that stick out below the outlne.
			self.extraY = extraY
		else:
			extraY = self.extraY

		if not params.metaDataAboveGlyph:
			glyphY = extraY # glyph origin is  < height of meta data> above the bottom of the glyph tile (0,0). This already i
		else:
			glyphY = 0
		glyphY -= rt_yMin # rt_yMin is 0 or negative. We are moving the glyph up by the OS/2 Descender value.
		# as it will draw below y = 0 by this value.
		#rt_canvas.rect(-params.rt_glyphHPadding, -params.rt_glyphVPadding, kGlyphSquare+ (2*params.rt_glyphHPadding), kGlyphSquare + (2*params.rt_glyphVPadding) + rt_metaDataYOffset-rt_yMin)
		#rt_canvas.line(-params.rt_glyphHPadding,glyphY, kGlyphSquare+ (2*params.rt_glyphHPadding), glyphY)
		# If we are doing any of the hint plots, then the glyph needs to be scaled down additionally,
		#  in order to accomodate a margin around the em-box for the hint labels.

		rt_canvas.saveState()
		rt_canvas.translate(glyphX, glyphY) # 1000 em-square coords
		rt_canvas.transform(scale, 0, 0, scale, 0, 0) # shift the glyph square up from the bottom of the tile by glyphY, and scale by the ratio kGlyphSquare/ emSquare

		for clientMethod in params.sortedMethodsDraw:
			methodName =  kDrawTag + clientMethod
			if getattr(params, methodName, 0):
				getattr(self, methodName)(params)

		rt_canvas.setFont( params.pointLabelFont, params.pointLabelSize)
		for path  in self.pathList:
			for pt in path:
				for clientMethod in params.sortedMethodsDrawPoint:
					methodName =  kDrawPointTag + clientMethod
					if getattr(params, methodName, 0):
						getattr(self, methodName)(path, pt, params)

		rt_canvas.restoreState()
		# back to 1000 em-square coords.
		# now draw meta data.
		if params.metaDataAboveGlyph:
			metaDataTopY = self.extraY + kGlyphSquare
		else:
			metaDataTopY = self.extraY

		self.cur_x =  params.metaDataXoffset
		self.cur_y = metaDataTopY

		if (repeat == 0) and (params.rt_repeats > 1):
			if params.rt_errorLogDict:
				try:
					errorList = params.rt_errorLogDict[self.name]
					self.writeErrorList(errorList, params)
				except KeyError:
					pass
		else:
			params.rt_canvas.setFont( params.metaDataFont, params.metaDataTextSize)
			for clientMethod in params.sortedMethodsMeta:
				methodName =  kShowMetaTag + clientMethod
				state = eval("params." + methodName)
				try:
					state = eval(state)
				except TypeError:
					pass
				if  state:
					self.cur_y -= params.metaDataTextSize
					eval("self." + methodName + "(params)")


	def writeErrorList(self, errorList, params):
		# At this point, I expect ErrorList to be a list of short strings.
		rt_canvas = params.rt_canvas
		ptSize = params.errorLogPointSize
		rt_canvas.setFont( params.errogLogFont,  ptSize)
		posX = 0
		posY = self.cur_y - ptSize

		# calculate column wiidth
		listLen = map(len, errorList)
		maxLen = max(listLen)
		maxString = errorList[listLen.index(maxLen)]
		columnWidth = pdfmetrics.stringwidth(maxString, params.errogLogFont) * 0.001 * ptSize
		columnAdvance = columnWidth +  params.errorLogColumnSpacing
		for textString in errorList:
			if  ((posY - 2*ptSize) < 0) and ((posX + (columnAdvance + columnWidth)) > kGlyphSquare):
				rt_canvas.drawString(posX, posY, textString + "...")
				break # We filled up the available display area.
			else:
				rt_canvas.drawString(posX, posY, textString)
			posY -= ptSize
			if (posY - ptSize) < 0: # I allow a margin of one line at the bottom of the meta area.
				posY = self.cur_y - ptSize
				posX += columnAdvance


	def drawGlyph_Outline(self, params, fill = 0):
		if params.DrawFilledOutline:
			fill = 1
		rt_canvas = params.rt_canvas
		curX = 0
		curY = 0
		p = rt_canvas.beginPath()
		for path  in self.pathList:
			for pdfPoint in path:
				if pdfPoint.type == FontPDFPoint.MT: # Dont't need to draw anything.
					curX = pdfPoint.pt0[0]
					curY = pdfPoint.pt0[1]
					p.moveTo(curX , curY)
				if pdfPoint.type == FontPDFPoint.LT:
					curX = pdfPoint.pt0[0]
					curY = pdfPoint.pt0[1]
					p.lineTo(curX , curY)
				elif pdfPoint.type == FontPDFPoint.CT:
					x1 = pdfPoint.bcp1[0]
					y1 = pdfPoint.bcp1[1]
					x2 =  pdfPoint.bcp2[0]
					y2 = pdfPoint.bcp2[1]
					curX = pdfPoint.pt0[0]
					curY = pdfPoint.pt0[1]
					p.curveTo(x1, y1, x2, y2, curX, curY)
			firstPT = path[0].pt0
			if (curX, curY)  != firstPT:
				p.lineTo( firstPT[0] , firstPT[1] )
		rt_canvas.drawPath(p, 1, fill)


	def drawGlyph_EMBox(self, params):
		baseLine = self.parentFont.getBaseLine()
		emSquare = self.parentFont.getEmSquare()
		rt_canvas = params.rt_canvas
		rt_canvas.saveState()
		emboxColorRGB = params.emboxColorRGB
		rt_canvas.setStrokeColorRGB(emboxColorRGB[0], emboxColorRGB[1], emboxColorRGB[2])
		rt_canvas.setLineWidth(params.markLineWidth)
		rt_canvas.rect(0, baseLine, emSquare, emSquare) # by definition; the glyph is scaled so the embox is kGlyphSquare pts.
		rt_canvas.restoreState()

	def drawGlyph_CenterMarks(self, params):
		baseLine = self.parentFont.getBaseLine()
		emSquare = self.parentFont.getEmSquare()
		yStart = baseLine
		height =emSquare
		if params.drawMeta_V_SideBearings:
			# Make box fit yorig to advance width.
			yStart = self.yOrigin - self.yAdvance
			height = self.yAdvance

		rt_canvas = params.rt_canvas
		rt_canvas.saveState()
		xAdvanceColorRGB = params.xAdvanceColorRGB
		rt_canvas.setStrokeColorRGB(xAdvanceColorRGB[0], xAdvanceColorRGB[1], xAdvanceColorRGB[2])
		y2 = baseLine + (emSquare // 2)
		x2 = self.xAdvance // 2
		tickSize = 10
		rt_canvas.setLineWidth(params.markLineWidth)
		rt_canvas.line(-tickSize, y2, 0, y2)
		rt_canvas.line(self.xAdvance, y2, self.xAdvance + tickSize, y2)
		rt_canvas.line(x2, yStart + height, x2, yStart + height+tickSize)
		rt_canvas.line(x2, yStart-tickSize, x2, yStart)
		rt_canvas.restoreState()

	def drawGlyph_CenterMarksWithBox(self, params):
		self.drawGlyph_CenterMarks(params)
		self.drawGlyph_GlyphBox(params)

	def drawGlyph_GlyphBox(self, params):
		yStart = self.parentFont.getBaseLine()
		height = self.parentFont.getEmSquare()
		if params.drawMeta_V_SideBearings:
			# Make box fit yorig to advance width.
			yStart = self.yOrigin - self.yAdvance
			height = self.yAdvance
		rt_canvas = params.rt_canvas
		rt_canvas.saveState()
		emboxColorRGB = params.emboxColorRGB
		rt_canvas.setStrokeColorRGB(emboxColorRGB[0], emboxColorRGB[1], emboxColorRGB[2])
		rt_canvas.setLineWidth(params.markLineWidth)
		rt_canvas.rect(0, yStart, self.xAdvance, height) # Second pair of (x,y) coord in .rect are relative to first.
		rt_canvas.restoreState()


	def drawGlyph_Baseline(self, params):
		baseLine = self.parentFont.getBaseLine()
		emSquare = self.parentFont.getEmSquare()
		rt_canvas = params.rt_canvas
		rt_canvas.saveState()
		baselineColorRGB = params.emboxColorRGB
		rt_canvas.setLineWidth(params.markLineWidth)
		rt_canvas.setStrokeColorRGB(baselineColorRGB[0], baselineColorRGB[1], baselineColorRGB[2])
		rt_canvas.line(0, baseLine, emSquare, baseLine)
		rt_canvas.restoreState()

	def drawGlyph_XAdvance(self, params):
		rt_canvas = params.rt_canvas
		rt_canvas.saveState()
		xAdvanceColorRGB = params.xAdvanceColorRGB
		rt_canvas.setStrokeColorRGB(xAdvanceColorRGB[0], xAdvanceColorRGB[1], xAdvanceColorRGB[2])
		advance = self.xAdvance
		tickSize = 10
		rt_canvas.setLineWidth(params.markLineWidth)
		rt_canvas.line(0, -tickSize/2, 0, tickSize/2)
		rt_canvas.line(0, 0, tickSize*2.0/3, 0)
		rt_canvas.line(advance, -tickSize/2, advance, tickSize/2)
		rt_canvas.line(advance-tickSize*2.0/3, 0, advance,0)
		rt_canvas.restoreState()

	def drawGlyph_ContourLabels(self, params):
		pathIndex = 0
		rt_canvas = params.rt_canvas
		rt_canvas.saveState()
		contourLabelColorRGB = params.contourLabelColorRGB
		for path in self.pathList:
			pointPDF = path[0]
			p1 = pointPDF.pt0

			# Get the incoming vector. If this poin tis a cureve-to, this is bcp2, else it is the position of the previous point.
			if pointPDF.bcp2:
				p0 = pointPDF.bcp2
			else:
				p0 = pointPDF.last.pt0
				if p1 == p0: # final point was coincdent with current
					if pointPDF.last.bcp2:
						p0 = pointPDF.last.bcp2
					else:
						p0 = pointPDF.last.last.pt0

			if  pointPDF.next.bcp1:
				p2 = pointPDF.next.bcp1
			else:
				p2 = pointPDF.next.pt0
				startPt = path[0]
			tickSize = params.pointLabel_LineLength
			rt_canvas.setLineWidth(params.markLineWidth)
			tickPos = getTickPos(p2, p1, p0, tickSize, self.isTT)
			text = "P %s" % (pathIndex,)
			textWidth = pdfmetrics.stringwidth(text, params.pointLabelFont) * 0.001 * params.pointLabelSize
			textPos = [tickPos[0], tickPos[1]]
			if textPos[0] <  p1[0]:
				textPos[0]  = textPos[0] - textWidth
			if textPos[1] <  p1[1]:
				textPos[1]   = textPos[1] - tickSize
			rt_canvas.line(p1[0], p1[1], tickPos[0], tickPos[1])
			rt_canvas.saveState()
			rt_canvas.setFillColorRGB(contourLabelColorRGB[0], contourLabelColorRGB[1], contourLabelColorRGB[2])
			rt_canvas.setStrokeColorRGB(contourLabelColorRGB[0], contourLabelColorRGB[1], contourLabelColorRGB[2])
			rt_canvas.setFont( params.pointLabelFont, tickSize)
			rt_canvas.drawString( textPos[0], textPos[1], text)
			rt_canvas.restoreState()
			pathIndex += 1
		rt_canvas.restoreState()

	def drawGlyph_HHints(self, params):
		numHints = len(self.hhints)
		numTables = len(self.hintTable)

		rt_canvas = params.rt_canvas
		rt_canvas.saveState()
		ptSize = params.pointLabelSize
		rt_canvas.setFont( params.pointLabelFont,  ptSize)
		prevHint = None
		xOffset = params.hhintXOffset  # used to make the hint rect tick out a bit from the em-box on both sides
		for i in list(range(numHints)):
			hint = self.hhints[i]
			h1 = float(eval(hint[0]))
			width = float( eval(hint[1]))
			h2 = h1 + width
			if params.rt_hintTableList:
				showHint = 0 # only show hints that are in the matching hint replacement blocks.
			else:
				showHint = 1 # by default, show all hints
			hintString1 = "h(%s, %s)  %s"  % (h1, h2, width)
			hintString2 = ""
			for j in list(range(numTables)):
				table = self.hintTable[j]
				if not table: # there is no hint replacement in the glyph.
					showHint = 1
					break
				if i in table[0]:
					if j in params.rt_hintTableList:
						showHint = 1
					if hintString2:
						hintString2 += " %s " % (j)
					else:
						hintString2 = "in groups %s " % (j)
			if not showHint:
				continue
			if prevHint and (prevHint[1] >= h1):
				hintColorOverlapRGB = params.hintColorOverlapRGB
				rt_canvas.setStrokeColorRGB(hintColorOverlapRGB[0], hintColorOverlapRGB[1], hintColorOverlapRGB[2])
				rt_canvas.setFillColorRGB(hintColorOverlapRGB[0] *0.6, hintColorOverlapRGB[1] *0.6, hintColorOverlapRGB[2] *0.6)
				xOffset += 2 # so the rects of different colors don't exactly overlap
			else:
				hintColorRGB = params.hintColorRGB
				rt_canvas.setStrokeColorRGB(hintColorRGB[0], hintColorRGB[1], hintColorRGB[2])
				rt_canvas.setFillColorRGB(hintColorRGB[0] *0.6, hintColorRGB[1] *0.6, hintColorRGB[2] *0.6)
			x0 = -xOffset
			x1 = kGlyphSquare+xOffset
			if (i%2) == 1:
				posX = x1
			else:
				posX = x0  - pdfmetrics.stringwidth(hintString1, params.pointLabelFont) * 0.001 * ptSize
			rt_canvas.setLineWidth(params.markLineWidth)
			rt_canvas.rect(x0, h1, x1-x0, width)
			posY = h1 + 0.2* ptSize
			rt_canvas.drawString(posX, posY , hintString1)
			if hintString2:
				posY = posY + 1.2*ptSize
				rt_canvas.drawString(posX, posY, hintString2)
			prevHint = (h1, h2)
		rt_canvas.restoreState()

	def drawGlyph_VHints(self, params):
		numHints = len(self.vhints)
		numTables = len(self.hintTable)
		baseLine = self.parentFont.getBaseLine()
		rt_canvas = params.rt_canvas
		rt_canvas.saveState()
		ptSize = params.pointLabelSize
		rt_canvas.setFont( params.pointLabelFont,  ptSize)
		prevHint = None
		yOffset = params.vhintYOffset # used to make the hinte rect tick out a bit from the em-box on both sides
		for i in list(range(numHints)):
			hint = self.vhints[i]
			h1 = float(eval(hint[0]))
			width = float( eval(hint[1]))
			h2 = h1 + width
			if params.rt_hintTableList:
				showHint = 0 # only show hints that are in the matching hint replacement blocks.
			else:
				showHint = 1 # by default, show all hints
			hintString1 = "v(%s, %s)  %s"  % (h1, h2, width)
			hintString2 = ""
			for j in list(range(numTables)):
				table = self.hintTable[j]
				if not table: # there is no hint replacement in the glyph.
					showHint = 1
					break
				if i in table[1]:
					if j in params.rt_hintTableList:
						showHint = 1
					if hintString2:
						hintString2 += " %s " % (j)
					else:
						hintString2 = "in groups %s " % (j)
			if not showHint:
				continue
			if prevHint and (prevHint[1] >= h1):
				hintColorOverlapRGB = params.hintColorOverlapRGB
				rt_canvas.setStrokeColorRGB(hintColorOverlapRGB[0], hintColorOverlapRGB[1], hintColorOverlapRGB[2])
				rt_canvas.setFillColorRGB(hintColorOverlapRGB[0] *0.6, hintColorOverlapRGB[1] *0.6, hintColorOverlapRGB[2] *0.6)
				yOffset += 2
			else:
				hintColorRGB = params.hintColorRGB
				rt_canvas.setStrokeColorRGB(hintColorRGB[0], hintColorRGB[1], hintColorRGB[2])
				rt_canvas.setFillColorRGB(hintColorRGB[0] *0.6, hintColorRGB[1] *0.6, hintColorRGB[2] *0.6)
			y0 = baseLine-yOffset
			y1 =  baseLine + kGlyphSquare+yOffset
			if (i%2) == 1:
				posY = y1 +  (ptSize *0.2)
				posY2 = posY + (ptSize*1.2)
			else:
				posY = y0 -  (ptSize *1.2)
				posY2 = posY - (ptSize*1.2)
			rt_canvas.setLineWidth(params.markLineWidth)
			rt_canvas.rect(h1, y0, width, y1 - y0)
			posX = h1
			rt_canvas.drawString(posX, posY , hintString1)
			if hintString2:
				rt_canvas.drawString(posX, posY2, hintString2)

			prevHint = (h1, h2, width)
		rt_canvas.restoreState()

	def drawGlyph_BlueZones(self, params):
		try:
			blueZones = self.parentFont.GetBlueZones()[self.fdIndex]
		except IndexError:
			return
		blueZones.sort()
		rt_canvas = params.rt_canvas
		rt_canvas.saveState()
		ptSize = params.pointLabelSize
		rt_canvas.setFont( params.pointLabelFont,  ptSize)
		alignmentZoneColorRGB = params.alignmentZoneColorRGB
		rt_canvas.setStrokeColorRGB(alignmentZoneColorRGB[0], alignmentZoneColorRGB[1], alignmentZoneColorRGB[2])
		rt_canvas.setFillColorRGB(alignmentZoneColorRGB[0] *0.5, alignmentZoneColorRGB[1] *0.5, alignmentZoneColorRGB[2] *0.5)
		blueZoneOffset = 5 # used to make the blue zone rect tick out a bit from the em-box on both sides
		x0 = -blueZoneOffset
		for i, zone in enumerate(blueZones):
			y0 = zone[0]
			y1 = zone[1]
			width =  y1 - y0
			textString = "<%s, %s> %s" % (y0, y1,width)
			if (i%2) == 0:
				textXPos = blueZoneOffset + kGlyphSquare - (pdfmetrics.stringwidth(textString, params.pointLabelFont) * 0.001 * ptSize)
			else:
				textXPos = x0

			textYpos =  y1 + 0.2 * ptSize
			rt_canvas.setLineWidth(params.markLineWidth)
			rt_canvas.rect(x0, y0, kGlyphSquare+(blueZoneOffset*2), width)
			rt_canvas.drawString(textXPos, textYpos, textString)
		rt_canvas.restoreState()



	def drawPoint_BCPMarks(self, path, pointPDF , params):
		if pointPDF.bcp1:
			rt_canvas = params.rt_canvas
			tickSize = params.pointLabel_BCP_CrossSize/2.0
			rt_canvas.setLineWidth(params.markLineWidth)
			stroke = fill=1
			pts = [pointPDF.bcp1, pointPDF.bcp2]
			for pt in pts:
				x0  = pt[0]
				y0  = pt[1]
				rt_canvas.line(x0+tickSize, y0, x0-tickSize, y0 )
				rt_canvas.line(x0, y0+tickSize, x0, y0-tickSize )

	def drawPoint_PointLabels(self, path, pointPDF , params):
		# Draw a tick mark, and a label at the end of the tick mark.
		# the tick mark vector direction is determined by:
		# -  adding the the vector from the previous point to the current point to the vector from the current point to the next point
		# - normalize the to unit lenght, then scale per the parameter pointLabelSize
		#  - determing the vector 90 degrees clockwise ( +y, -x)
		# The text strin gis then written to the end of the ector, but offset when necessary to not ovelapr the tick mark.
		# Note that I need to invert the matrix. Thi s is because the overall page grid +y is down;
		# when drawing the glyph I invert this, so text will come out upside down; hence I need to invert again to draw text.
		pointLabelColorRGB = params.pointLabelColorRGB
		rt_canvas = params.rt_canvas

		if pointPDF == path[0]:
			if path[-1].pt0 == pointPDF.pt0:
				return # Don't draw label for first point in path if last point s coincident with it.
		text = ""
		if params.pointLabel_doPointType:
			text += pointPDF.type
		if params.pointLabel_doPointIndex :
			text += " %s" % (pointPDF.index)
		if params.pointLabel_doPosition:
			text += " (%s, %s) " % (pointPDF.pt0[0],  pointPDF.pt0[1])
		textWidth = pdfmetrics.stringwidth(text, params.pointLabelFont) * 0.001 * params.pointLabelSize

		p1 = pointPDF.pt0

		# Get the incoming vector. If this poin tis a cureve-to, this is bcp2, else it is the position of the previous point.
		if pointPDF.bcp2:
			p0 = pointPDF.bcp2
		else:
			p0 = pointPDF.last.pt0
			if p1 == p0: # final point was coincdent with current
				if pointPDF.last.bcp2:
					p0 = pointPDF.last.bcp2
				else:
					p0 = pointPDF.last.last.pt0

		if  pointPDF.next.bcp1:
			p2 = pointPDF.next.bcp1
		else:
			p2 = pointPDF.next.pt0
		tickSize = params.pointLabel_LineLength
		rt_canvas.setLineWidth(params.markLineWidth)
		tickPos = getTickPos(p0, p1, p2, tickSize, self.isTT)
		rt_canvas = params.rt_canvas
		rt_canvas.line(p1[0], p1[1], tickPos[0], tickPos[1])
		textPos = [tickPos[0], tickPos[1]]
		if textPos[0] <  p1[0]:
			textPos[0]  = textPos[0] - textWidth
		if textPos[1] <  p1[1]:
			textPos[1]   = textPos[1] - params.pointLabelSize
		rt_canvas.saveState()
		rt_canvas.setFillColorRGB(pointLabelColorRGB[0], pointLabelColorRGB[1], pointLabelColorRGB[2])
		rt_canvas.setStrokeColorRGB(pointLabelColorRGB[0], pointLabelColorRGB[1], pointLabelColorRGB[2])
		rt_canvas.drawString( textPos[0], textPos[1], text)
		rt_canvas.restoreState()

	def drawPoint_PointMarks(self, path, pointPDF , params):
		getattr(self, f'drawPoint{pointPDF.type}')(params, pointPDF)

	def drawPointm(self, params, pt):
		rt_canvas = params.rt_canvas
		# Draw arrow-head based on pt.
		size = params.pointClosingArrowLength

		if  pt.last.bcp2: # if the last point is a curve-to, then the best pos for the base of the arrow head vector is the incoming BCP.
			p0 = pt.last.bcp2
		else:
			p0 = pt.last.pt0 # if last pt is a line-to, it is usually co-incident with the first point, and we have to look to the previous point to get the vector.
			if p0 == pt.pt0:
				p0 =  pt.last.last.pt0

		x0 = p0[0]
		y0 =  p0[1]
		x1 = pt.pt0[0]
		y1 =  pt.pt0[1]
		if (y1 == y0): # arrow is horizontal
			y3 = y1 + size/4.0
			y4 = y1 - size/4.0
			if x0 < x1:
				x3 = x4 = x1 - size
			else:
				x3 = x4 = x1 + size
		elif (x1 == x0): # arrow is vertical
			if y0 < y1:
				y3 = y4 = y1 -size
			else:
				y3 = y4 = y1 +size
			x3 = x1 + size/4.0
			x4 = x1 - size/4.0
		else:
			slope = (x1 - x0)/ float(y1 - y0)
			dy = (size**2/(slope**2 + 1)) **0.5
			dx = slope*dy
			if y0 < y1:
				x2 = x1 - dx
				y2 = y1 - dy
			else:
				x2 = x1 + dx
				y2 = y1 + dy
			x3 = x2 + dy/4.0
			y3 = y2 - dx/4.0
			x4 = x2 - dy/4.0
			y4 = y2 + dx/4.0
		rt_canvas.setLineWidth(params.markLineWidth)
		rt_canvas.line(x1,y1,x3,y3 )
		rt_canvas.line(x3,y3,x4,y4 )
		rt_canvas.line(x4,y4,x1,y1 )

	def drawPointl(self, params, pt):
		rt_canvas = params.rt_canvas
		size = params.pointMarkDiameter
		stroke = fill=1
		rt_canvas.setLineWidth(params.markLineWidth)
		rt_canvas.circle(pt.pt0[0], pt.pt0[1], size/2.0, stroke, fill)

	def drawPointc(self, params, pt):
		rt_canvas = params.rt_canvas
		size = params.pointMarkDiameter
		rt_canvas.setLineWidth(params.markLineWidth)
		stroke = fill=1
		rt_canvas.circle(pt.pt0[0], pt.pt0[1], size/2.0, stroke, fill)

	def drawMeta_Outline(self, params):
		rt_canvas = params.rt_canvas
		rt_canvas.saveState()
		MetaDataGlyphColorRGB = params.MetaDataGlyphColorRGB
		rt_canvas.setFillColorRGB(MetaDataGlyphColorRGB[0], MetaDataGlyphColorRGB[1], MetaDataGlyphColorRGB[2])
		glyphScale =  params.metaDataFilledGlyphSize/float(kGlyphSquare)
		rt_canvas.transform(glyphScale, 0, 0, glyphScale, 0, self.cur_y - (kGlyphSquare + params.rt_yMin)*glyphScale)
		self.drawGlyph_Outline(params, fill = 1)
		self.drawGlyph_XAdvance(params)
		#self.drawGlyphEMBox(params)
		rt_canvas.restoreState()

	def drawMeta_Name(self, params):
		rt_canvas = params.rt_canvas
		rt_canvas.saveState()
		rt_canvas.setFont( params.metaDataFont, params.metaDataNameSize)
		self.cur_y += params.metaDataTextSize # undo default increment
		self.cur_y -= params.metaDataNameSize
		rt_canvas = params.rt_canvas
		gName = self.name
		if self.isCID and gName.startswith("cid"):
			gName = "CID " + gName[3:]
		if params.rt_optionLayoutDict and (not self.isCID) and not gName.startswith("cid"):
			rowDir = os.path.dirname(params.rt_filePath)
			hintDir = os.path.basename(os.path.dirname(rowDir))
			rowDir = os.path.basename(rowDir)
			reversekey = repr((hintDir, rowDir, self.name))
			try:
				entry = params.rt_optionLayoutDict[reversekey]
				cidName =  entry[0].zfill(5)
			except KeyError:
				cidName = ''
			nameString = "%s   %s" % (gName, cidName)
		else:
			nameString = gName
		rt_canvas.drawString(self.cur_x, self.cur_y, nameString)
		rt_canvas.restoreState()

	def drawMeta_BBox(self, params):
		rt_canvas = params.rt_canvas
		bbox = self.BBox
		rt_canvas.drawString(self.cur_x, self.cur_y, "min = %s, %s  max = %s, %s" % (bbox[0], bbox[1], bbox[2], bbox[3]))

	def drawMeta_SideBearings(self, params):
		rt_canvas = params.rt_canvas
		bbox = self.BBox
		width = self.xAdvance
		lsb = bbox[0]
		rsb = width - bbox[2]
		rt_canvas.drawString(self.cur_x, self.cur_y, "L = %s, R = %s Width = %s" % (lsb, rsb, width))

	def drawMeta_V_SideBearings(self, params):
		rt_canvas = params.rt_canvas
		baseLine = self.parentFont.getBaseLine() # baseLine is usually a negative number
		bbox = self.BBox
		y_origin = self.yOrigin
		tsb = self.tsb
		bsb =  bbox[1] - (y_origin -  self.yAdvance)
		rt_canvas.drawString(self.cur_x, self.cur_y, "T = %s, B = %s, vAdv = %s" % (tsb, bsb, self.yAdvance))

	def drawMeta_Parts(self, params):
		params.rt_canvas.drawString(self.cur_x, self.cur_y, "Parts: m = %s, l = %s, c = %s, total = %s" % (self.numMT, self.numLT, self.numCT, self.numMT +  self.numLT + self.numCT))

	def drawMeta_Paths(self, params):
		params.rt_canvas.drawString(self.cur_x, self.cur_y, "Paths:  %s" % (self.numPaths))

	def drawMeta_Hints(self, params):
		numHhints = len(self.hhints)
		numVhints = len(self.vhints)
		params.rt_canvas.drawString(self.cur_x, self.cur_y, "Hints:  %s horiz: %s, vert: %s" % (numHhints + numVhints, numHhints, numVhints))

	def _get_cid_layout_entry(self, params):
		if (self.parentFont.isCID or (self.name.startswith("cid") and
			re.match(r"\d+", self.name[3:]))):
			# use CID key to access layout dict
			# zero-pad the CID glyph name
			glyph_name = "cid" + self.name[3:].zfill(5)
			return params.rt_optionLayoutDict.get(glyph_name, None)

	def drawMeta_HintDir(self, params):
		hintDir = None
		layout_entry = self._get_cid_layout_entry(params)
		if layout_entry:
			try:
				hintDir = layout_entry[0]
			except IndexError:
				pass
		if hintDir:
			params.rt_canvas.drawString(
				self.cur_x, self.cur_y, "HintDir = %s" % hintDir)

	def drawMeta_RowFont(self, params):
		rowDir = None
		layout_entry = self._get_cid_layout_entry(params)
		if layout_entry:
			try:
				rowDir = layout_entry[1:3]
			except IndexError:
				pass
		if rowDir:
			params.rt_canvas.drawString(
				self.cur_x, self.cur_y, "RowDir = %s" % '/'.join(rowDir))

	def drawMeta_WidthOnly(self, params):
		params.rt_canvas.drawString(self.cur_x, self.cur_y, "Width = %s, Y Advance = %s" % ( self.xAdvance, self.yAdvance))


def getTitleHeight(params):
	pageTitleSize = params.pageTitleSize
	cur_y = params.pageSize[1] - (params.pageTopMargin + pageTitleSize)
	cur_y -= pageTitleSize*1.2
	cur_y -= pageTitleSize/2
	return  cur_y - pageTitleSize # Add some space below the title line.

def doFontSetTitle(rt_canvas, params, numPages):
	pageTitleFont = params.pageTitleFont
	pageTitleSize = params.pageTitleSize
	pageIncludeTitle = params.pageIncludeTitle
	# Set 0,0 to be at top right of page.

	if pageIncludeTitle:
		rt_canvas.setFont(pageTitleFont, pageTitleSize)
	title = "FontSet Proof"
	rightMarginPos = params.pageSize[0]-params.pageRightMargin
	cur_y = params.pageSize[1] - (params.pageTopMargin + pageTitleSize)
	if pageIncludeTitle:
		rt_canvas.drawString(params.pageLeftMargin, cur_y, title)
		rt_canvas.drawRightString(rightMarginPos, cur_y, time.asctime())
	cur_y -= pageTitleSize*1.2
	pageString = 'page %d' % (rt_canvas.getPageNumber())
	if pageIncludeTitle:
		rt_canvas.drawRightString(rightMarginPos, cur_y, pageString)
	cur_y -= pageTitleSize/2
	if pageIncludeTitle:
		rt_canvas.setLineWidth(3)
		rt_canvas.line(params.pageLeftMargin, cur_y, rightMarginPos, cur_y)
		#reset carefully afterwards
		rt_canvas.setLineWidth(1)
	return  cur_y - pageTitleSize # Add some space below the title line.

def doTitle(rt_canvas, pdfFont, params, numGlyphs, numPages = None):
	pageTitleFont = params.pageTitleFont
	pageTitleSize = params.pageTitleSize
	pageIncludeTitle = params.pageIncludeTitle
	# Set 0,0 to be at top right of page.

	if pageIncludeTitle:
		rt_canvas.setFont(pageTitleFont, pageTitleSize)
	title = "%s   OT version %s " % (pdfFont.getPSName(), pdfFont.getOTVersion() )
	rightMarginPos = params.pageSize[0]-params.pageRightMargin
	cur_y = params.pageSize[1] - (params.pageTopMargin + pageTitleSize)
	if pageIncludeTitle:
		rt_canvas.drawString(params.pageLeftMargin, cur_y, title)
		rt_canvas.drawRightString(rightMarginPos, cur_y, time.asctime())
	cur_y -= pageTitleSize*1.2
	if numPages == None:
		numPages = numGlyphs // params.glyphsPerPage
		if (numGlyphs % params.glyphsPerPage) > 0:
			numPages +=1
	pageString = '   %d of %s' % (rt_canvas.getPageNumber(), numPages)
	if pageIncludeTitle:
		rt_canvas.drawRightString(rightMarginPos, cur_y, pageString)
	cur_y -= pageTitleSize/2
	if pageIncludeTitle:
		rt_canvas.setLineWidth(3)
		rt_canvas.line(params.pageLeftMargin, cur_y, rightMarginPos, cur_y)
		#reset carefully afterwards
		rt_canvas.setLineWidth(1)
	return  cur_y - pageTitleSize # Add some space below the title line.

def  getMetaDataHeight(params, fontYMin) :
	if params.rt_metaDataYOffset == None:
		yMeta = 0
		metaEntryList =  filter(lambda fieldName: fieldName[:len(kShowMetaTag)] == kShowMetaTag, dir(params))
		for fieldName in metaEntryList:
			if eval("params." + fieldName):
				if fieldName == "drawMeta_Name":
					yMeta += params.metaDataNameSize
				elif fieldName == "drawMeta_Outline":
					pass
				else:
					yMeta += params.metaDataTextSize
		if  params.drawMeta_Outline:
			glyphHeight = (-fontYMin * params.metaDataFilledGlyphSize/float(kGlyphSquare))+ params.metaDataFilledGlyphSize # in pts
			if yMeta <  glyphHeight:
				yMeta = glyphHeight
		params.rt_metaDataYOffset  = yMeta

	if params.errorLogFilePath and (params.errorLogColumnHeight > yMeta):
		params.rt_metaDataYOffset = params.errorLogColumnHeight


def setDefaultHPadding(params, init = None):
	 # Need to allow extra space for the horizontal and vertical hint labels
	if init:
		params.rt_glyphHPadding = init
	else:
		params.rt_glyphHPadding = params.glyphHPadding

	if  params.drawGlyph_HHints:
		params.rt_maxHintLabelWidth = pdfmetrics.stringwidth("<999.0, 999.0> 999.0", params.pointLabelFont) * 0.001 *  params.pointLabelSize
		params.rt_glyphHPadding += params.hhintXOffset + params.rt_maxHintLabelWidth

def setDefaultVPadding(params, init = None):
	if init:
		params.rt_glyphVPadding = init
	else:
		params.rt_glyphVPadding = params.glyphVPadding

	if  params.drawGlyph_VHints:
		params.rt_glyphVPadding += params.vhintYOffset +  (2*1.2*params.pointLabelSize) # amount the hints stick out, plus two lines of type.


def getLayout(params, extraY, yTop):
	if params.userPtSize != None:
		return getLayoutFromUserPtSize(params, extraY, yTop)
	else:
		return getLayoutFromGPP(params, extraY, yTop)

def getLayoutFromUserPtSize(params, extraY, yTop):
	# figure how many glyphs can go on a page. This complicated by the fact that a glyph can be repeated more than once.
	numAcross = numDown = scale = xAdvance = yAdvance = leftPadding =  topPadding = 0
	pageWidth = float(params.pageSize[0] - (params.pageRightMargin + params.pageLeftMargin))
	pageHeight = float(yTop - params.pageBottomMargin)

	glyphWidthUnscaled = glyphHeightUnscaled = kGlyphSquare
	glyphWidthUnscaled += 2*params.rt_glyphHPadding
	glyphWidthUnscaled = glyphWidthUnscaled * params.rt_repeats
	glyphHeightUnscaled += extraY + 2*params.rt_glyphVPadding

	scale = params.userPtSize/float(kGlyphSquare)
	glyphWidth = int(scale*glyphWidthUnscaled)
	glyphHeight = int(scale*(glyphHeightUnscaled))
	numAcross = int(pageWidth/glyphWidth)
	if numAcross < 1:
		setDefaultHPadding(params, 0)
		glyphWidthUnscaled = (kGlyphSquare+ 2*params.rt_glyphHPadding) * params.rt_repeats
		glyphWidth = int(scale*glyphWidthUnscaled)
		numAcross = 1
	numDown = int(pageHeight/glyphHeight)
	if numDown < 1:
		setDefaultVPadding(params, 0)
		glyphHeightUnscaled = kGlyphSquare + extraY + 2*params.rt_glyphVPadding
		glyphHeight = int(scale*(glyphHeightUnscaled))
		numDown = 1

	# Distribute any extra space between the glyphs
	# The full spacing is used between the glyphs; 1/2 this spacing is left on the left and right end of the lines
	# in addition to the margins,
	leftPadding =  (pageWidth - (numAcross * glyphWidth)) / (numAcross * params.rt_repeats) # the space between glyphs
	if leftPadding < 0:
		leftPadding = 0
	xAdvance = int(glyphWidth/params.rt_repeats  + leftPadding)
	leftPadding = int(leftPadding/ 2) # Ithe full spacing goes between the glyphs

	# Since the first line starts at (yTop - (yAdvance + topPadding), and
	# yAdvance already has topPadding in it, what we return is -topPadding/2.
	# so that the first line will have 1/2 the inter-line spacing.
	topPadding = (pageHeight - (numDown * glyphHeight) )/ numDown
	if topPadding < 0:
		topPadding = 0
	yAdvance =int( glyphHeight + topPadding)
	topPadding = -int(topPadding /2)

	#print "DBG getLayout numAcross %s, numDown %s, scale %s, xAdvance %s, yAdvance %s, leftPadding %s, topPadding %s." % \
	#	(numAcross, numDown, scale, xAdvance, yAdvance, leftPadding, topPadding)

	return numAcross, numDown, scale, xAdvance, yAdvance, leftPadding, topPadding

def getLayoutFromGPP(params, extraY, yTop):
	# figure how many glyphs across and down. This complicated by the fact that a glyph can be repeated more than once.
	glyphsPerPage = params.glyphsPerPage
	if glyphsPerPage == 1:
		params.rt_glyphHPadding = params.rt_glyphVPadding = 0

	numAcross = numDown = scale = xAdvance = yAdvance = leftPadding =  topPadding = 0
	pageWidth = float(params.pageSize[0] - (params.pageRightMargin + params.pageLeftMargin))
	pageHeight = float(yTop - params.pageBottomMargin)

	glyphWidthUnscaled = (kGlyphSquare + 2*params.rt_glyphHPadding) *params.rt_repeats
	glyphHeightUnscaled = kGlyphSquare + extraY + 2*params.rt_glyphVPadding

	tryCount = 0
	while  1: # If we end up with one across or down, we see the padding around the glyph to 0, and try again.
		if tryCount > 1:
			break
		# An iterative solution is the simplest.
		n = 0
		while n < glyphsPerPage:
			n += 1
			numDown1 = n
			scale1 = pageHeight/(numDown1 *glyphHeightUnscaled)
			numAcross1  = int(pageWidth/(scale1*glyphWidthUnscaled))
			numGlyphs1 = numAcross1 * numDown1

			numAcross2 = n
			scale2 = pageWidth/(numAcross2 *glyphWidthUnscaled)
			numDown2  = int(pageHeight/(scale2*glyphHeightUnscaled))
			numGlyphs2 = numAcross2 * numDown2

			if (numGlyphs2 >= glyphsPerPage) and (numGlyphs1 >= glyphsPerPage):
				if scale1 >= scale2:
					scale = scale1
					numAcross = numAcross1
					numDown = numDown1
				else:
					scale = scale2
					numAcross = numAcross2
					numDown = numDown2
				break
			elif (numGlyphs2 >= glyphsPerPage):
				scale = scale2
				numAcross = numAcross2
				numDown = numDown2
				break
			elif (numGlyphs1 >= glyphsPerPage):
				scale = scale1
				numAcross = numAcross1
				numDown = numDown1
				break

		if tryCount > 0:
			setDefaultHPadding(params)
			setDefaultVPadding(params)
			glyphWidthUnscaled = (kGlyphSquare + 2*params.rt_glyphHPadding) *params.rt_repeats
			glyphHeightUnscaled = kGlyphSquare + extraY + 2*params.rt_glyphVPadding

		# If we ended up with one glyph across or down, we don't need padding between glyphs, and can make the glyph a little larger.
		if (numDown == 1):
			setDefaultVPadding(params, 0)
			glyphHeightUnscaled = kGlyphSquare + extraY + 2*params.rt_glyphVPadding

		if (numAcross == 1):
			setDefaultHPadding(params, 0)
			glyphWidthUnscaled = (kGlyphSquare + 2*params.rt_glyphHPadding) *params.rt_repeats

		tryCount +=1

	glyphHeight = scale * glyphHeightUnscaled
	glyphWidth = scale * glyphWidthUnscaled
	# Distribute any extra space between the glyphs
	# The full spacing is used between the glyphs; 1/2 this spacing is left on the left and right end of the lines
	# in addition to the margins,

	leftPadding =  (pageWidth - (numAcross * glyphWidth)) / (numAcross * params.rt_repeats) # the space between glyphs
	if leftPadding < 0:
		leftPadding = 0
	xAdvance = int(glyphWidth/params.rt_repeats + leftPadding)
	leftPadding = int(leftPadding/ 2) # the full spacing goes between the glyphs

	# Since the first line starts at (yTop - (yAdvance + topPadding), and
	# yAdvance already has topPadding in it, what we return is -topPadding/2.
	# so that the first line will have 1/2 the inter-line spacing.
	topPadding = (pageHeight - (numDown * glyphHeight) )/ numDown
	if topPadding < 0:
		topPadding = 0
	yAdvance =int( glyphHeight + topPadding)
	topPadding = -int(topPadding /2)

	#print "DBG getLayout numAcross %s, numDown %s, scale %s, xAdvance %s, yAdvance %s, leftPadding %s, topPadding %s." % \
	#	(numAcross, numDown, scale, xAdvance, yAdvance, leftPadding, topPadding)
	return numAcross, numDown, scale, xAdvance, yAdvance, leftPadding, topPadding

def parseErrorLog(errorLines, path):
	# parse an error log file, and return an errorLog dict. This is keyed by glyph name, Each value is a list of short strings.
	# This is for the Morisawa error log.
	# The first token on a line is the glyph name. Remaining tokens are either an error name, one of a pair of integer x,y coordinates
	# or one of a pair of text-named coordinates beginning with x or y.
	errorDict = {}
	errorList = []
	lastCoord = ""
	curGlyphName = ""
	errorLines = map(lambda line: line.strip(), errorLines)
	errorLines = filter(lambda line: line and (line[0] != "#"), errorLines)
	lineno = 0
	for line in errorLines:
		lineno = lineno + 1
		tokenList = line.split()
		curGlyphName = tokenList[0]
		tokenList = tokenList[1:]
		errorList = []
		errorDict[curGlyphName] = errorList

		# If this is a CID name, it may end up being referenced as the CId number without
		# a cid prefix. Derive the numeric form, and add that as well.
		if curGlyphName.startswith("cid"):
			try:
				eval(curGlyphName[3:])
				#if name is in a CID form, index it with and without leading zeroes.
				curGlyphName = curGlyphName[3:]
				errorDict[curGlyphName] = errorList
				index = curGlyphName.rfind("0")
				if index > -1:
					curGlyphName = curGlyphName[index:]
					errorDict[curGlyphName] = errorList
			except:
				pass # it only looked like a cid name.
		else: # Maybe it is a CID name, but without the cid prefix, just the numbers
			try:
				origName = curGlyphName
				if curGlyphName[0] == "0":
					# erode leading zeros
					while  curGlyphName[0] == "0":
						curGlyphName = curGlyphName[1:]
				eval(curGlyphName)
				errorDict[curGlyphName] = errorList
				if len(curGlyphName) <5:
					curGlyphName = curGlyphName.zfill(5)
					errorDict[curGlyphName] = errorList
				cidName = "cid" + curGlyphName
				errorDict[curGlyphName] = errorList
			except:
				pass

		for token in tokenList:
			# Is it a coordinate?
			try:
				if not re.match(r"[xy][xy]*\d+", token):
					int(token)
				# if we get here, its either a numeric value, or matches the pattern.
				if lastCoord:
					errorList.append(lastCoord + ", " + token)
					lastCoord = ""
				else:
					lastCoord = token
				continue
			except ValueError:
				lastCoord = ""

			# It is something else. Append it.
			errorList.append(token)
	return errorDict

class ProgressBar:
	kProgressBarTickStep = 100
	kText = "Processing %s of %s glyphs. Time remaining:  %s min %s sec"
	def __init__(self, maxItems, title):
		self.tickCount = 0
		self.maxCount = maxItems
		self.title = title

	def StartProgress(self, startText = None):
		self.startTime = time.time()
		self.tickCount = 0
		if startText:
			print(startText)

	def DoProgress(self, tickCount):
		if  tickCount and ((tickCount % self.kProgressBarTickStep) == 0):
			self.tickCount = tickCount
			curTime = time.time()
			perGlyph = (curTime - self.startTime)/(float(tickCount))
			timeleft = int(perGlyph * (self.maxCount - tickCount))
			minutesLeft = int(timeleft /60)
			secondsLeft = timeleft % 60
			print(self.kText % (tickCount, self.maxCount, minutesLeft,
								     secondsLeft))

	def EndProgress(self):
		print("Saving file...")


def makePDF(pdfFont, params, doProgressBar=True):
	if not params.rt_filePath:
		params.rt_reporter( "Skipping font. Calling program must set params.rt_filePath.")
		return
	fontPath = params.rt_filePath
	if params.rt_pdfFileName:
		pdfPath = params.rt_pdfFileName
	else:
		pdfPath = f"{os.path.splitext(fontPath)[0]}.pdf"
	params.rt_canvas = rt_canvas = pdfgen.Canvas(pdfPath, pagesize=params.pageSize, bottomup = 1)

	if params.waterfallRange:
		makeWaterfallPDF(params, pdfFont, doProgressBar)
	else:
		makeProofPDF(pdfFont, params, doProgressBar)
	return pdfPath



class  FontInfo:
	def __init__(self, pdfFont, glyphList):
		self.pdfFont = pdfFont
		self.glyphList = glyphList # I pass in the glyph list rather than getting the font char set,
									# becuase the user may have spacifued a subset.
		self.numGlyphs = len(glyphList)
		self.FontName = pdfFont.getPSName()
		bboxList = pdfFont.getBBox()
		self.FontBBox = "[ %s %s %s %s ]" % (bboxList[0], bboxList[1], bboxList[2], bboxList[3])
		if pdfFont.clientFont.has_key('CFF '):
			StemV = None
			try:
				StemV = pdfFont.clientFont['CFF '].cff.topDictIndex[0].Private.StdVW
			except AttributeError:
				pass
			if StemV == None:
				try:
					StemV = pdfFont.clientFont['CFF '].cff.topDictIndex[0].Private.StemSnapV
					StemV = StemV[0]
				except AttributeError:
					self.StemV = 100
			if StemV != None:
				self.StemV = StemV

			StemH = None
			try:
				StemH = pdfFont.clientFont['CFF '].cff.topDictIndex[0].Private.StdHW
			except AttributeError:
				pass
			if StemH == None:
				try:
					StemH = pdfFont.clientFont['CFF '].cff.topDictIndex[0].Private.StemSnapH
					StemH = StemH[0]
				except AttributeError:
					pass
			if StemH != None:
				self.StemH = StemH
		else:
			self.StemV = 100 # arbitrary value to make a working FontDescriptor

		self.Flags = (1<<2) # mark it as a symbolic font, so that  glyph names outside the PDF Std set will be recognized.

		# We need to get the widths for all the glyphs, so we can do basic layout.
		self.widthDict = {}
		txreport = None
		if self.pdfFont.clientFont.has_key('hmtx'):
			hmtx = self.pdfFont.clientFont['hmtx']
			for name in self.glyphList:
				self.widthDict[name] = hmtx[name][0]
		else:
			# have to use tx.
			### glyph[tag] {gname,enc,width,{left,bottom,right,top}}
			# glyph[1] {space,0x0020,250,{0,0,0,0}}
			command = "tx -mtx \"%s\"" % (self.pdfFont.path)
			txreport = fdkutils.runShellCmd(command)
			widths = re.findall(r"glyph\S+\s+{([^,]+),[^,]+,([^,]+),{[-0-9]+,[-0-9]+,[-0-9]+,[-0-9]+}}", txreport)
			if pdfFont.isCID:
				for name, width in widths:
					self.widthDict["cid%s" % (name.zfill(5))] = eval(width)
			else:
				for name, width in widths:
					self.widthDict[name] = eval(width)

		if pdfFont.clientFont.has_key('OS/2'):
			os2 = pdfFont.clientFont['OS/2']
			if os2.fsSelection & 1:
				self.Flags +=1<<7
			self.CapHeight = os2.sCapHeight
			self.Ascent = os2.sTypoAscender
			self.Descent = os2.sTypoDescender
			self.Leading = os2.sTypoLineGap
			self.avgWidth = os2.xAvgCharWidth
		else:
			if txreport == None:
				command = "tx -mtx \"%s\"" % (self.pdfFont.path)
				txreport = fdkutils.runShellCmd(command)
			# try for "H"
			match = re.search(r"glyph\S+\s+{H,[^,]+,[^,]+,{[-0-9]+,[-0-9]+,[-0-9]+,([-0-9]+)}}", txreport)
			if match:
				self.CapHeight = eval(match.group(1))
			else:
				self.CapHeight = 750 # real hack, but this doesn;t make much difference in this PDF.
			# try for "d"
			match = re.search(r"glyph\S+\s+{d,[^,]+,[^,]+,{[-0-9]+,[-0-9]+,[-0-9]+,([-0-9]+)}}", txreport)
			if match:
				self.Ascent = eval(match.group(1))
			else:
				self.Ascent = 750 # real hack, but this doesn;t make much difference in this PDF.
			# try for "p"
			match = re.search(r"glyph\S+\s+{p,[^,]+,[^,]+,{[-0-9]+,([-0-9]+),[-0-9]+,[-0-9]+}}", txreport)
			if match:
				self.Descent = eval(match.group(1))
			else:
				self.Descent = 750 # real hack, but this doesn;t make much difference in this PDF.
			self.Leading = 200 # real hack, but this doesn;t make much difference in this PDF.

			widthList = self.widthDict.values()
			avgWidth = 0
			for width in widthList:
				avgWidth += width
			self.avgWidth = int(round(0.5 + float(avgWidth)/len(widthList)))


		if pdfFont.clientFont.has_key('post'):
			post = pdfFont.clientFont['post']
			self.ItalicAngle = post.italicAngle
		else:
			self.ItalicAngle = 0

	def setEncoding(self, fontInfo, glyphList):
		"""" We can assume that the range [numAcross*wi -  numAcross*(w+1)]will always contain some glyphs from the list."""

		self.firstChar = firstChar = 64
		numGlyphs = len(glyphList)
		if numGlyphs > (255-firstChar):
			numGlyphs = (255-firstChar)
			glyphList = glyphList[:numGlyphs]
		lastChar = firstChar + numGlyphs

		encodingList = []
		notdefList1 = " /.notdef" * firstChar
		notdefList2 = " /.notdef"*(255 -lastChar)
		diffList = " /" + " /".join(glyphList)
		if notdefList2:
			encodingList = ["<< /Type /Encoding",
							"/BaseEncoding /MacRoman",
							"/Differences",
							"[ 0 %s" % (notdefList1),
							" %s %s" % (firstChar, diffList),
							" %s %s" % (lastChar+1, notdefList2),
							"]",
							">>"
							]
		else:
			encodingList = ["<< /Type /Encoding",
							"/BaseEncoding /MacRoman",
							"/Differences",
							"[ 0 %s" % (notdefList1),
							" %s %s" % (firstChar, diffList),
							"]",
							">>"
							]
		# Now get the widths for this vector.

		widthList = ["["]
		for name in glyphList:
			widthList.append(str(self.widthDict[name]))
		widthList += ["]"]
		fontInfo.encoding = LINEEND.join(encodingList)
		fontInfo.firstChar = firstChar
		fontInfo.lastChar = lastChar
		fontInfo.widths =  LINEEND.join(widthList)

		return

	def getFontDescriptorText(self):
		"""
      Build font stream object.
      """
		if self.pdfFont.clientFont.has_key('CFF '):
			fontStream = self.pdfFont.clientFont['CFF '].compile(self.pdfFont.clientFont)
			formatName = "/FontFile3"
			if self.pdfFont.isCID:
				fontStreamType = "/CIDFontType0C"
				fontType = "/Type0"
			else:
				fontStreamType = "/Type1C"
				fontType = "/Type1"

		elif self.pdfFont.clientFont.has_key('glyf'):
			fontStreamType = None # don't need Subtype key/value in teh stream object for TrueType fonts.
			with open(self.pdfFont.path, "rb") as fp:
				fontStream = fp.read()
			formatName = "/FontFile2"
			fontType = "/TrueType"
		else:
			print("Font type not supported.")
			raise TypeError

		text = []
		text.append("<< ")
		text.append("/Type /FontDescriptor")
		text.append("/FontName /%s" % (self.FontName))
		text.append("/Flags %s" % (self.Flags))
		text.append("/FontBBox %s" % (self.FontBBox))
		text.append("/StemV %s" % (self.StemV))
		if hasattr(self, "StemH") and self.StemH != None:
			text.append("/StemH %s" % (self.StemH))
		text.append("/CapHeight %s" % (self.CapHeight))
		text.append("/Ascent %s" % (self.Ascent))
		text.append("/Descent %s" % (self.Descent))
		text.append("/Leading %s" % (self.Leading))
		text.append("/ItalicAngle %s" % (self.ItalicAngle))
		text.append(formatName +  " %s 0 R")
		text.append(">>")
		text = LINEEND.join(text)
		return text, formatName, fontStream, fontType, fontStreamType


def getFontDescriptorItems(fontDescriptor):
	fdItemText, formatName, fontStream, fontType, fontStreamType = fontDescriptor.getFontDescriptorText()
	return fdItemText, fontType, formatName, fontStream, fontStreamType

def getEncodingInfo(fontInfo):
	firstChar = lastChar = widths = None
	return fontInfo.firstChar, fontInfo.lastChar, fontInfo.widths


def doWaterfall(params, glyphList, fontInfo, wi, cur_y, cur_x, pdfFont, yTop, numGlyphs, numPages):
	rt_canvas = params.rt_canvas
	fontPath = params.rt_filePath
	embeddedFontPSName = fontInfo.FontName
	fontInfo.setEncoding(fontInfo, glyphList)
	rt_canvas.addFont(embeddedFontPSName, fontInfo.encoding, fontInfo, getFontDescriptorItems, getEncodingInfo)
	leading = None
	text = ''
	for char_int in range(fontInfo.firstChar, fontInfo.lastChar):
		if char_int == 92:  # backslash
			text += '\\\\'
		else:
			text += chr(char_int)
	for gSize in params.waterfallRange:
		cur_y -= gSize*1.2
		if ((cur_y) < params.pageBottomMargin):
			rt_canvas.showPage()
			yTop = doTitle(rt_canvas, pdfFont, params, numGlyphs, numPages)
			cur_x = params.pageLeftMargin
			cur_y = yTop - gSize*1.2
		rt_canvas.setFont(embeddedFontPSName, gSize, leading, fontInfo.encoding)
		rt_canvas.drawString(cur_x, cur_y, text)
	return cur_y

def makeWaterfallPDF(params, pdfFont, doProgressBar):
	"""
	This is a very different approach than makeProofPDF.
	The latter uses std PDF commands to draw label text and
	and to plot the line segments of the glyph paths.
	makeWaterfallPDF needs to show the effects of hinting,
	and so uses the  PDF show text commands to request that
	glyphs be done by referencing them as character codes in
	a string, using an embedded font. Since the very basic
	PDF support in this package allows only charcodes 0-255,
	we need to first divide the font into several embedded
	fonts, then embed the fonts, then add show
	text commands referencing each font in turn. To simplify the logic, I actually divide
	the  original font into embedded fonts such that each embedded font
	contains only as many characters as  will fit across the
	page.
	"""

	numGlyphs = len(params.rt_glyphList)
	yTop = getTitleHeight(params)
	rt_canvas = params.rt_canvas
	rt_canvas.setPageCompression(0)
	if doProgressBar:
		progressBarInstance = ProgressBar(numGlyphs, "Proofing font...", )
		progressBarInstance.StartProgress()

	waterfallRange = list(params.waterfallRange)

	# Embed font, with an initial encoding that will encode the first 256 glyphs at char codes 0-255.
	psName = pdfFont.getPSName()
	fontInfo = FontInfo(pdfFont, params.rt_glyphList)
	embeddedFontPSName = fontInfo.FontName
	numGlyphs = len(params.rt_glyphList)
	groupIndex = 0
	fontInfo.setEncoding(fontInfo, params.rt_glyphList[:256])
	rt_canvas.addFont(embeddedFontPSName, fontInfo.encoding, fontInfo, getFontDescriptorItems, getEncodingInfo)

	# figure out layout.
	pageWidth = float(params.pageSize[0] - (params.pageRightMargin + params.pageLeftMargin))
	pageHeight = float(yTop - params.pageBottomMargin)
	maxSize = maxLabelSize = max(waterfallRange)
	# Figure out y-height of waterall, and write pt size headers.
	cur_y = yTop
	pageTitleFont = params.pageTitleFont
	pageTitleSize = params.pageTitleSize
	if maxLabelSize > pageTitleSize:
		maxLabelSize = pageTitleSize
	ptSizeTemplate = "%s pts   "
	ptWidth = pdfmetrics.stringwidth(ptSizeTemplate % ("00"), pageTitleFont)*0.001*maxLabelSize

	waterfallHeight = 0 # page height of one water fall  one row per pt size.
	for gSize in waterfallRange:
		waterfallHeight += gSize*1.2
		cur_y -= gSize*1.2
		labelSize = gSize
		if labelSize > pageTitleSize:
			labelSize = pageTitleSize
		rt_canvas.setFont(pageTitleFont, labelSize)
		rt_canvas.drawString(params.pageLeftMargin, cur_y, ptSizeTemplate % gSize)
	cur_y = yTop


	# Build the list of strings, based on their widths.
	# If the glyph string is too long to font on one line, divide it up into
	# a list of glyphs strings, each of which do fit on one line.
	glyphLists = []
	glyphList = []
	stringWidth = ptWidth # allow for initial pt size field on first group.
	cur_x = params.pageLeftMargin + ptWidth
	for name in params.rt_glyphList:
		width = fontInfo.widthDict[name]*maxSize/1000.0
		if stringWidth + width < pageWidth:
			glyphList.append(name)
			stringWidth += width
		else:
			glyphLists.append(glyphList)
			glyphList = [name]
			stringWidth = width
	if glyphList:
		glyphLists.append(glyphList)

	numWaterfallsOnPage = int(pageHeight / waterfallHeight)
	numWaterFalls = len(glyphLists)

	if numWaterfallsOnPage > 0:
		numPages = int(ceil(float(numWaterFalls) / numWaterfallsOnPage))
	else:
		numPages = int(ceil(float(numWaterFalls) * waterfallHeight / pageHeight))

	if numPages == 0:
		numPages = 1
	doTitle(rt_canvas, pdfFont, params, numGlyphs, numPages)

	for wi in range(numWaterFalls):
		cur_y = doWaterfall(params, glyphLists[wi], fontInfo, wi, cur_y, cur_x, pdfFont, yTop, numGlyphs, numPages)
		cur_x = params.pageLeftMargin
	if doProgressBar:
		progressBarInstance.EndProgress()
	rt_canvas.showPage()
	rt_canvas.save()


def makeFontSetPDF(pdfFontList, params, doProgressBar=True):
	"""
	This PDF will show a set of glyphs from each font, one font per font, with
	fixed spacing so that the glyphs in a column should all nominally be the
	same.

	It will first figure out how many glyphs per line and lines per page can be
	shown at the given point size, with a line spacing of em-size*1.2, and with a
	fixed column spacing of em-size. It will then divide the glyph list up into
	groups by the number across the page. For each group, it will show the
	glyphs in the group for each font.

	An entry in the pdfFontList is [glyphList, pdfFont, tempCFFPath]
	"""
	# Sort fonts with same charsets together. Sort by: len charset, charset, ps name.
	sortList = sorted(map(lambda entry: [ len(entry[0]), entry[0], entry[1].getPSName(), entry[1]], pdfFontList))
	pdfFontList = list(map(lambda entry: [entry[1], entry[3]], sortList))

	if params.rt_pdfFileName:
		pdfPath = params.rt_pdfFileName
	else:
		# We put the PDF wherever the first font is.
		firstPDFFont = pdfFontList[0][1]
		fontPath = params.rt_filePath
		pdfPath = f"{os.path.splitext(fontPath)[0]}.fontset.pdf"
	params.rt_canvas = pdfgen.Canvas(pdfPath, pagesize=params.pageSize, bottomup = 1)

	# figure out how much space to leave at start of line for PS names and fond index fields.
	psNameSize = params.fontsetGroupPtSize
	pageTitleFont = params.pageTitleFont
	maxLen = 0
	maxPSName = None
	for entry in pdfFontList:
		psName = entry[1].getPSName()
		psNameLen = len(psName)
		if psNameLen > maxLen:
			maxLen = psNameLen
			maxPSName = psName
	indexString = "%s" % (len(pdfFontList))
	psNameAndIndex = maxPSName + " " + indexString
	psNameWidth = pdfmetrics.stringwidth(psNameAndIndex, pageTitleFont)  * 0.001 * psNameSize
	indexWidth = pdfmetrics.stringwidth(indexString, pageTitleFont)  * 0.001 * psNameSize
	# glyph width equivalents of ps name.
	kPSNameFieldWidth = int( 1 + (psNameWidth/params.userPtSize)) # The space to leave at the begining of a line for the PS name, in em-spaces.
	kFontIndexWidth = int( 1 + (indexWidth/params.userPtSize))  # The space to leave at the begining of every line for the font index, in em-spaces.

	# get layout parameters
	scaledxAdvance = params.userPtSize
	scaledYAdvance = int(params.userPtSize*1.2)
	scaledYAdvanceGroupTitle = int(psNameSize*1.2)
	yTop = getTitleHeight(params)
	pageWidth = float(params.pageSize[0] - (params.pageRightMargin + params.pageLeftMargin))
	pageHeight = float(yTop - params.pageBottomMargin)

	numAcross = int(pageWidth/params.userPtSize) - kFontIndexWidth # -kFontIndexWidth em-spaces is to allow for an initial font index in the line.
	linesPerPage = int(pageHeight/scaledYAdvance)
	# collect max number of glyph names in a font. Allow for different charsets in the fonts.
	maxNames = 0
	for entry in pdfFontList:
		lenList = len(entry[0])
		if lenList > maxNames:
			maxNames = lenList
	numGlyphs = maxNames + kPSNameFieldWidth # allow for kPSNameFieldWidth em-spaces for the font PS name in the first group.
	numGroups = int(1 + float(numGlyphs)/numAcross)
	numFonts = len(pdfFontList)
	numLines =  int(1 + float(numGlyphs * numFonts) /numAcross)
	numPages = 	int(1 + float(numLines + (numGroups*3))/linesPerPage)

	# For widow control: move a whole group to the next page, if the group doesn't fit on a  page and there are at least five groups to a page.
	linesPerGroup = numFonts + 1
	groupsPerPage = int(linesPerPage/linesPerGroup)
	doWidowControl =  groupsPerPage > 1
	if 	doWidowControl:
		numPages = 	int(1 + (float(numGroups)/groupsPerPage))

	rt_canvas = params.rt_canvas
	if doProgressBar:
		progressBarInstance = ProgressBar(numPages, "Proofing font...", )
		progressBarInstance.kProgressBarTickStep = 1
		progressBarInstance.kText = "Created %s of about %s pages. Time remaining:  %s min %s sec"
		progressBarInstance.StartProgress("Writing PDF pages...")

	params.rt_scale = scale = params.userPtSize/float(kGlyphSquare)

	cur_x = params.pageLeftMargin
	cur_y = yTop
	doFontSetTitle(rt_canvas, params, numPages)

	pageCount = 0
	params.rt_glyphHPadding = params.rt_glyphVPadding = params.rt_metaDataYOffset = 0
	rt_canvas.setFont( params.pageTitleFont, params.userPtSize)
	lineCount = 0
	startgli = 0
	endgli = startgli + numAcross - kPSNameFieldWidth


	groupsOnPageCount = 0
	for groupIndex in list(range(numGroups)):
		fountCount = 0
		groupsOnPageCount += 1
		if doWidowControl and (groupsOnPageCount > groupsPerPage):
			pageCount += 1
			groupsOnPageCount = 1
			if doProgressBar:
				progressBarInstance.DoProgress(pageCount)
			rt_canvas.showPage()
			yTop = doFontSetTitle(rt_canvas, params, numPages)
			rt_canvas.setFont( params.pageTitleFont, params.userPtSize)
			cur_y = yTop - scaledYAdvance
			#print "Widowing"
			#print "New page: pageCount %s. groupsOnPageCount %s. groupsPerPage %s. groupIndex %s" % (pageCount, groupsOnPageCount, groupsPerPage, groupIndex)
		cur_x = params.pageLeftMargin
		rt_canvas.setFont( params.pageTitleFont, psNameSize)
		rt_canvas.drawString(cur_x, cur_y, "Glyph group %s of %s" % (groupIndex+1, numGroups))
		rt_canvas.setFont( params.pageTitleFont, params.userPtSize)
		cur_y -= scaledYAdvance
		lineCount +=3
		for entry in pdfFontList:
			cur_x = params.pageLeftMargin
			pdfFont = entry[1]
			params.rt_emBoxScale = kGlyphSquare/float( pdfFont.getEmSquare())
			glyphList = entry[0]
			if endgli >= len(glyphList):
				font_endgli = len(glyphList) -1
			else:
				font_endgli = endgli
			if startgli >= len(glyphList):
				continue
			if groupIndex == 0:
				# For the first group only, show the font PS names.
				rt_canvas.setFont( params.pageTitleFont, psNameSize)
				psName = entry[1].getPSName()
				indexString = "%s" % (fountCount)
				psNameAndIndex = psName + " " + indexString
				rt_canvas.drawString(cur_x, cur_y, "%s" % (psNameAndIndex))
				cur_x += (kPSNameFieldWidth*scaledxAdvance)
				rt_canvas.setFont( params.pageTitleFont, params.userPtSize)
			else:
				rt_canvas.setFont( params.pageTitleFont, psNameSize)
				rt_canvas.drawString(cur_x, cur_y, "%s" % (fountCount))
				cur_x += (kFontIndexWidth*scaledxAdvance)
				rt_canvas.setFont( params.pageTitleFont, params.userPtSize)
			if startgli < len(glyphList):
				curGlyphList = glyphList[startgli:font_endgli+1]
				for glyphName in curGlyphList:
					pdfGlyph = pdfFont.getGlyph(glyphName)
					rt_canvas.saveState()
					rt_canvas.translate(cur_x, cur_y)
					rt_canvas.scale(scale, scale)
					pdfGlyph.draw(params, 0)
					rt_canvas.restoreState()
					cur_x += scaledxAdvance
			lineCount +=1
			fountCount += 1
			cur_y -= scaledYAdvance
			if ((cur_y) < params.pageBottomMargin):
				pageCount += 1
				groupsOnPageCount = 1
				if doProgressBar:
					progressBarInstance.DoProgress(pageCount)
				rt_canvas.showPage()
				yTop = doFontSetTitle(rt_canvas, params, numPages)
				rt_canvas.setFont( params.pageTitleFont, params.userPtSize)
				cur_y = yTop - scaledYAdvance
				#print "NOT Widowing!!!"
				#print "New page: pageCount %s. groupsOnPageCount %s. groupsPerPage %s. groupIndex %s" % (pageCount, groupsOnPageCount, groupsPerPage, groupIndex)
		# set new start/end in glyph list with each new group
		startgli = font_endgli + 1
		endgli = startgli + numAcross

	pageCount += 1
	if doProgressBar:
		progressBarInstance.DoProgress(pageCount)
		progressBarInstance.EndProgress()
	rt_canvas.showPage()
	rt_canvas.save()
	return pdfPath



def makeProofPDF(pdfFont, params, doProgressBar=True):
	# Collect log file text, if any.
	if params.errorLogFilePath:
		if not os.path.isfile(params.errorLogFilePath):
			print("Warning: log file %s does not exist or is not a file." %
					repr(params.errorLogFilePath))
		else:
			with open(params.errorLogFilePath, "r") as lf:
				errorLines = lf.readlines()
			params.rt_errorLogDict = parseErrorLog(errorLines, params.errorLogFilePath)
			keys = params.rt_errorLogDict.keys()
			keys.sort()

	# Determine layout

	# If the em-box is not 1000, we handle that by scaling the glyph outline
	# down. However, this means that all the text and tick sizes used with the
	# main outline must be scaled up by that amount.
	emBoxScale = params.rt_emBoxScale = kGlyphSquare/float( pdfFont.getEmSquare())
	params.pointLabelSize = params.pointLabelSize/params.rt_emBoxScale
	params.pointMarkDiameter = params.pointMarkDiameter/params.rt_emBoxScale
	params.pointLabel_BCP_CrossSize = params.pointLabel_BCP_CrossSize/params.rt_emBoxScale
	params.pointLabel_LineLength = params.pointLabel_LineLength/params.rt_emBoxScale
	params.pointClosingArrowLength = params.pointClosingArrowLength/params.rt_emBoxScale
	params.hhintXOffset = params.hhintXOffset/params.rt_emBoxScale
	params.vhintYOffset = params.vhintYOffset/params.rt_emBoxScale
	rt_canvas  = params.rt_canvas
	numGlyphs = len(params.rt_glyphList)
	yTop = getTitleHeight(params)
	if params.descenderSpace == None:
		params.rt_yMin = fontYMin = pdfFont.descent
	else:
		params.rt_yMin = fontYMin =  params.descenderSpace

	if fontYMin > 0:
		fontYMin = 0 # we want to add extra Y to the glyph tile when the descender is below the embox, but not subtract it when it is above.
		params.rt_yMin = 0

	setDefaultHPadding(params)
	setDefaultVPadding(params)

	getMetaDataHeight(params, fontYMin) # how high the meta data block will be for each glyph, unscaled.
	yMetaHeight = params.rt_metaDataYOffset
	extraY = yMetaHeight
	if (params.metaDataAboveGlyph == 0):
		extraY -= fontYMin # The total excursion of the glyph tile below the origin = (abs(font min) + yMetaHeight
	extraY += (params.pointLabel_LineLength + params.pointLabelSize)*2 # Need some margin for the labels that stick out below the outlne.
	numAcross, numDown, scale, xAdvance, yAdvance, leftPadding, topPadding = getLayout(params, extraY, yTop)
	params.rt_scale = scale
	if params.userPtSize:
		numOnPage = numAcross * numDown
	else:
		numOnPage = params.glyphsPerPage
	cur_x = params.pageLeftMargin + leftPadding
	cur_y = yTop - (topPadding + yAdvance)

	giRange = list(range(numGlyphs))
	if params.doAlphabeticOrder:
		params.rt_glyphList.sort()
	elif params.rt_optionLayoutDict and pdfFont.isCID:
		# enforce layout dict order.
		dictList = []
		for gi in giRange:
			name = params.rt_glyphList[gi]
			try:
				entry = params.rt_optionLayoutDict[name]
				dictList.append([entry[0], entry[1], name])
			except KeyError:
				dictList.append(["zUndefined", "zUndefined", name])
		dictList.sort()
		params.rt_glyphList = map(lambda entry: entry[-1], dictList)

	doTitle(rt_canvas, pdfFont, params, numGlyphs)

	if doProgressBar:
		progressBarInstance = ProgressBar(numGlyphs, "Proofing font...", )
		progressBarInstance.StartProgress()

	rowIndex = 0
	colIndex = 0
	for gi in giRange:
		if doProgressBar:
			progressBarInstance.DoProgress(gi)
		if  rowIndex >= numAcross:
			rowIndex = 0
			colIndex += 1
			cur_y -= yAdvance
			cur_x = params.pageLeftMargin + leftPadding
		if  (colIndex >= numDown) or (gi and ((gi % numOnPage) == 0) ):
			rowIndex = 0
			colIndex = 0
			rt_canvas.showPage()
			yTop = doTitle(rt_canvas, pdfFont, params, numGlyphs)
			cur_x = params.pageLeftMargin + leftPadding
			cur_y = yTop - (topPadding + yAdvance)
		rowIndex += 1

		pdfGlyph = pdfFont.getGlyph(params.rt_glyphList[gi])
		pdfGlyph.extraY = extraY
		if not params.rt_repeatParamList:
			params.rt_repeatParamList = [params]*params.rt_repeats

		for ri in list(range(params.rt_repeats)):
			curParams = params.rt_repeatParamList[ri]
			if ri > 0:
				fieldNames = vars(params)
				fieldNames = [name for name in vars(params) if name.startswith("rt_")]
				for name in fieldNames:
					setattr(curParams, name, getattr(params, name))

			rt_canvas.saveState()
			rt_canvas.translate(cur_x, cur_y)
			rt_canvas.scale(scale, scale)
			pdfGlyph.draw(curParams, ri)
			rt_canvas.restoreState()
			cur_x += xAdvance

	if doProgressBar:
		progressBarInstance.EndProgress()
	rt_canvas.showPage()
	rt_canvas.save()


def makeKernPairPDF(pdfFont, kernOverlapList, params, doProgressBar=True):
	"""
	This PDF will show a set of glyphs from each font, one font per font, with
	fixed spacing so that the glyphs in a column should all nominally be the
	same.

	It will first figure out how many glyphs per line and lines per page can be
	shown at the given point size, with a line spacing of em-size*1.2, and with a
	fixed column spacing of em-size. It will then divide the glyph list up into
	groups by the number across the page. For each group, it will show the
	glyphs in the group for each font.

	An entry in the pdfFontList is [glyphList, pdfFont, tempCFFPath]
	"""
	if params.rt_pdfFileName:
		pdfPath = params.rt_pdfFileName
	else:
		# We put the PDF wherever the  font is.
		fontPath = params.rt_filePath
		pdfPath = os.path.splitext(fontPath)[0] + ".kc.pdf"

	params.rt_canvas = pdfgen.Canvas(pdfPath, pagesize=params.pageSize, bottomup = 1)

	# figure out how much space to leave at start of line for PS names and fond index fields..
	maxLen = 0

	# get layout parameters
	params.rt_emBoxScale = kGlyphSquare/float( pdfFont.getEmSquare())
	scaledYAdvance = int(params.userPtSize*1.2)
	yTop = getTitleHeight(params)
	pageWidth = float(params.pageSize[0] - (params.pageRightMargin + params.pageLeftMargin))
	pageHeight = float(yTop - params.pageBottomMargin)

	linesPerPage = int(pageHeight/scaledYAdvance)
	numLines = len(kernOverlapList)
	numPages = 	int(round(0.5 + float(numLines)/linesPerPage))

	rt_canvas = params.rt_canvas
	if doProgressBar:
		progressBarInstance = ProgressBar(numPages, "Proofing font...", )
		progressBarInstance.kProgressBarTickStep = 1
		progressBarInstance.kText = "Finished %s of %s pages. Time remaining:  %s min %s sec"
		progressBarInstance.StartProgress("Writing PDF pages...")

	params.rt_scale = scale = params.userPtSize/float(kGlyphSquare)

	cur_x = params.pageLeftMargin
	numGlyphs = 0
	yTop = doTitle(rt_canvas, pdfFont, params, numGlyphs, numPages)
	cur_y = yTop - scaledYAdvance

	params.rt_glyphHPadding = params.rt_glyphVPadding = params.rt_metaDataYOffset = 0
	rt_canvas.setFont( params.pageTitleFont, params.pageTitleSize)

	labelMargin = 4*params.userPtSize
	lineCount = 0
	pageCount = 0
	for entry in kernOverlapList:
		cur_x = params.pageLeftMargin
		leftName,rightName, kernValue, overlap, overlapPtSize = entry

		pdfGlyph = pdfFont.getGlyph(leftName)
		rt_canvas.saveState()
		rt_canvas.translate(cur_x, cur_y)
		rt_canvas.scale(scale, scale)
		pdfGlyph.draw(params, 0)
		rt_canvas.restoreState()
		cur_x += int((pdfGlyph.xAdvance +kernValue)*scale)
		pdfGlyph = pdfFont.getGlyph(rightName)
		rt_canvas.saveState()
		rt_canvas.translate(cur_x, cur_y)
		rt_canvas.scale(scale, scale)
		pdfGlyph.draw(params, 0)
		rt_canvas.restoreState()
		cur_x += int(pdfGlyph.xAdvance*scale)
		if cur_x < labelMargin:
			cur_x = labelMargin
		rt_canvas.drawString(cur_x, cur_y, " %s %s kern %s, pixel overlap area %s at %s ppem." % (leftName, rightName, kernValue, overlap, overlapPtSize))
		cur_y -= scaledYAdvance
		lineCount +=1
		if ((cur_y) < params.pageBottomMargin):
			pageCount += 1
			if doProgressBar:
				progressBarInstance.DoProgress(pageCount)
			rt_canvas.showPage()
			yTop = 	doTitle(rt_canvas, pdfFont, params, numGlyphs, numPages)
			cur_y = yTop - scaledYAdvance
	if doProgressBar:
		progressBarInstance.EndProgress()
	rt_canvas.showPage()
	rt_canvas.save()
	return pdfPath
