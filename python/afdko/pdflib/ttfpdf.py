"""
otfpdf v1.4.2 Jul 15 2019
provides support for the proofpdf script,  for working with OpenType/TTF
fonts. Provides an implementation of the fontpdf font object. Cannot be
run alone.
"""
__copyright__ = """Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
"""

from fontTools.pens.boundsPen import BoundsPen
from afdko.pdflib.fontpdf import FontPDFGlyph, FontPDFFont, FontPDFPen

__version__ = "1.4.2"


class txPDFFont(FontPDFFont):

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

	def clientGetPSName(self):
		psName = 'None'
		name_tb = self.clientFont['name']
		mac_name_id_6 = name_tb.getName(6, 1, 0, 0)
		win_name_id_6 = name_tb.getName(6, 3, 1, 1033)
		if mac_name_id_6:
			return mac_name_id_6.string.decode()
		elif win_name_id_6:
			return win_name_id_6.string.decode('utf_16_be')
		return psName

	def clientGetOTVersion(self):
		version =  self.clientFont['head'].fontRevision
		majorVersion = int(version)
		minorVersion = str(int( 1000*(0.0005 + version -majorVersion) )).zfill(3)
		versionString = "%s.%s" % (majorVersion, minorVersion)
		#print versionString
		return versionString

	def clientGetGlyph(self, glyphName):
		return txPDFGlyph(self, glyphName)

	def clientGetEmSquare(self):
		emSquare =  self.clientFont['head'].unitsPerEm
		return emSquare

	def clientGetBaseline(self):
		baseLine = 0
		txFont = self.clientFont
		try:
			unicodeRange2 = txFont["OS/2"].ulUnicodeRange2
			if unicodeRange2 & 0x10000: # supports CJK  ideographs
				baseTag = "ideo"
			else:
				baseTag = "romn"
			baseTable =  self.clientFont['BASE']
			baseTagIndex = baseTable.table.HorizAxis.BaseTagList.BaselineTag.index( baseTag)
			baseScript = None
			for baseRecord in baseTable.table.HorizAxis.BaseScriptList.BaseScriptRecord:
				if baseRecord.BaseScriptTag == "latn":
					baseScript = baseRecord.BaseScript
					baseLine = baseScript.BaseValues.BaseCoord[baseTagIndex].Coordinate
					break
		except (KeyError, AttributeError):
			baseLine = 0

		return baseLine

	def clientGetBBox(self):
		headTable =  self.clientFont['head']
		return [headTable.xMin, headTable.yMin, headTable.xMax, headTable.yMax]

	def clientGetBlueZones(self):
		blueValues = [[]]
		return blueValues
	def clientGetAscentDescent(self):
		txFont = self.clientFont
		try:
			os2Table =  self.clientFont['OS/2']
			return os2Table.sTypoAscender, os2Table.sTypoDescender
		except KeyError:
			return None, None

class  txPDFGlyph(FontPDFGlyph):

	def clientInitData(self):
		self.isTT = 1
		self.isCID = 0
		txFont = self.parentFont.clientFont
		glyphSet = txFont.getGlyphSet(preferCFF=1)
		clientGlyph = glyphSet[self.name]
		# Get the list of points
		pen = FontPDFPen(None)
		clientGlyph.draw(pen)

		if not hasattr(txFont, 'vmetrics'):
			try:
				txFont.vmetrics = txFont['vmtx'].metrics
			except KeyError:
				txFont.vmetrics = None
			try:
				txFont.vorg = txFont['VORG']
			except KeyError:
				txFont.vorg = None

		self.hhints = []
		self.vhints =[]
		self.numMT = pen.numMT
		self.numLT = pen.numLT
		self.numCT = pen.numCT
		self.numPaths = pen.numPaths
		self.pathList = pen.pathList
		for path in self.pathList :
			lenPath = len(path)
			path[-1].next = path[0]
			path[0].last = path[-1]
			if lenPath > 1:
				path[0].next = path[1]
				path[-1].last = path[-2]
				for i in range(lenPath)[1:-1]:
					pt = path[i]
					pt.next =  path[i+1]
					pt.last =  path[i-1]

		assert len(self.pathList) == self.numPaths, " Path lengths don't match %s %s" % (len(self.pathList) , self.numPaths)
		# get the bbox and width.
		pen = BoundsPen(None)
		clientGlyph.draw(pen)
		self.xAdvance = clientGlyph.width
		glyph_bounds = pen.bounds
		if not glyph_bounds:
			self.BBox = [0, 0, 0, 0]
		else:
			self.BBox = [round(item) for item in glyph_bounds]

		self.yOrigin = self.parentFont.emSquare + self.parentFont.getBaseLine()
		if txFont.vorg:
			try:
				self.yOrigin  = txFont.vorg[self.name]
			except KeyError:
				if txFont.vmetrics:
					try:
						mtx = txFont.vmetrics[self.name]
						self.yOrigin = mtx[1] + self.BBox[3]
					except KeyError:
						pass

		haveVMTX = 0
		if txFont.vmetrics:
			try:
				mtx = txFont.vmetrics[self.name]
				self.yAdvance = mtx[0]
				self.tsb = mtx[1]
				haveVMTX =1
			except KeyError:
				pass
		if not haveVMTX:
			self.yAdvance = self.parentFont.getEmSquare()
			self.tsb = self.yOrigin - self.BBox[3] + self.parentFont.getBaseLine()

		# Get the fdIndex, so we can laterdetermine which set of blue values to use.
		self.fdIndex = 0
		return



