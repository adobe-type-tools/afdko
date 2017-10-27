# otc2otf.py v1.3 October 13 2017
from __future__ import print_function

__copyright__ = """Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
"""

__help__ = """
otc2otf otc2otf.py  [-r] <font.ttc>

Extract all OpenType fonts from the parent OpenType Collection font.
-r  Optional. Report TTC fonts and tables, Will not write the output files.

example:
 AFDKOPython otc2otf.py  -r  LogoCutStd.ttc

The script may be invoked with either the FDK command:
  otc2otf
or directly with the command:
  python <path to FDK directory>/FDK/Tools/SharedData/FDKScripts/otc2otf.py


"""


import sys
import os
import struct

class OTCError(TypeError):
	pass

class FontEntry:

	def __init__(self, sfntType, searchRange, entrySelector, rangeShift):
		self.sfntType = sfntType
		self.searchRange = searchRange
		self.entrySelector = entrySelector
		self.rangeShift = rangeShift
		self.tableList = []
		self.fileName = "temp.tmp.otf"
		self.psName = "PSNameUndefined"
		
	def append(self, tableEntry):
		self.tableList.append(tableEntry)

class TableEntry:
	
	def __init__(self, tag, checkSum, length):
		self.tag = tag
		self.checksum = checkSum
		self.length = length
		self.data = None
		self.offset = None
		self.isPreferred = False

ttcHeaderFormat = ">4sLL"
ttcHeaderFormatDoc = """
		> # big endian
		TTCTag:                  4s # "ttcf"
		Version:                 L  # 0x00010000 or 0x00020000
		numFonts:                L  # number of fonts
		# OffsetTable[numFonts]: L  # array with offsets from beginning of file
		# ulDsigTag:             L  # version 2.0 only
		# ulDsigLength:          L  # version 2.0 only
		# ulDsigOffset:          L  # version 2.0 only
"""


ttcHeaderSize = struct.calcsize(ttcHeaderFormat)

offsetFormat = ">L"
offsetSize = struct.calcsize(">L")

sfntDirectoryFormat = ">4sHHHH"
sfntDirectoryFormatDoc = """
		> # big endian
		sfntVersion:    4s
		numTables:      H    # number of tables
		searchRange:    H    # (max2 <= numTables)*16
		entrySelector:  H    # log2(max2 <= numTables)
		rangeShift:     H    # numTables*16-searchRange
"""

sfntDirectorySize = struct.calcsize(sfntDirectoryFormat)

sfntDirectoryEntryFormat = ">4sLLL"
sfntDirectoryEntryFormatDoc = """
		> # big endian
		tag:            4s
		checkSum:       L
		offset:         L
		length:         L
"""

sfntDirectoryEntrySize = struct.calcsize(sfntDirectoryEntryFormat)


nameRecordFormat = ">HHHHHH"
nameRecordFormatDoc = """
		>	# big endian
		platformID:	H
		platEncID:	H
		langID:		H
		nameID:		H
		length:		H
		offset:		H
"""
nameRecordSize = struct.calcsize(nameRecordFormat)

		
def parseArgs(args):
	doReportOnly = False
	fontPath = None
	argn = len(args)
	i = 0
	while i < argn:
		arg  = args[i]
		i += 1
		
		if arg[0] != '-':
			fontPath = arg
		elif arg == "-r":
			doReportOnly = True
		elif (arg == "-u") or (arg == "-h"):
			print(__help__)
			raise OTCError()
		else:
			raise OTCError("Unknown option '%s'." % (arg))
			
	if fontPath == None:
		raise OTCError("You must specify an OpenType Collection font as the input file..")
	
	allOK = True
	if not os.path.exists(fontPath):
		raise OTCError("Cannot find '%s'." % (fontPath))
		
	with open(fontPath, "rb") as fp:
		data = fp.read(4)
	
	if data != b'ttcf':
		raise OTCErrr("File is not an OpenType Collection file: '%s'." % (fontPath))

	return fontPath, doReportOnly

MSUnicodeKey = (3, 1, 6)
AppleLatinKey = (1, 0, 6)

def getPSName(data):
	format, n, stringOffset = struct.unpack(">HHH", data[:6])
	expectedStringOffset = 6 + n * nameRecordSize
	if stringOffset != expectedStringOffset:
		# XXX we need a warn function
		print("Warning: 'name' table stringOffset incorrect.", end=' ')
		print("Expected: %s; Actual: %s" % (expectedStringOffset, stringOffset))
	stringData = data[stringOffset:]
	startNameRecordData = data[6:]
	psName = None
	
	for nameRecordKey in [ AppleLatinKey, MSUnicodeKey ]:
		if psName != None:
			break
		
		data = startNameRecordData
		for i in range(n):
			if len(data) < 12:
				# compensate for buggy font
				break
			platformID, platEncID, langID, nameID, length, offset = struct.unpack(nameRecordFormat, data[:nameRecordSize])
			data = data[nameRecordSize:]
			if ((platformID, platEncID, nameID) == nameRecordKey):
				psName = stringData[offset:offset+length]
				if nameRecordKey == MSUnicodeKey:
					psName = psName.decode('utf-16be')
				else:
					assert len(psName) == length
				break
			else:
				continue

	if psName == None:
		psName = "PSNameUndefined"
	return psName

def readFontFile(fontOffset, data, tableDict, doReportOnly):
	sfntType, numTables, searchRange, entrySelector, rangeShift = struct.unpack(sfntDirectoryFormat, data[fontOffset:fontOffset + sfntDirectorySize])
	fontEntry = FontEntry(sfntType, searchRange, entrySelector, rangeShift)
	entryData = data[fontOffset + sfntDirectorySize:]
	i = 0
	seenGlyf = False
	while i < numTables:
		tag, checksum, offset, length = struct.unpack(sfntDirectoryEntryFormat, entryData[:sfntDirectoryEntrySize])
		tableEntry = TableEntry(tag, checksum, length)
		tableEntry.offset = offset
		tableEntry.data = data[offset:offset+length]
		fontEntry.tableList.append(tableEntry)
		if tag == b"name":
			fontEntry.psName = getPSName(tableEntry.data)
		elif tag == b"glyf":
			seenGlyf = True
		entryData =  entryData[sfntDirectoryEntrySize:]
		i += 1
	if seenGlyf:
			fontEntry.fileName = fontEntry.psName + ".ttf"
	else:
			fontEntry.fileName = fontEntry.psName + ".otf"
		
	if doReportOnly:
		print("%s" % (fontEntry.fileName))
	for tableEntry in fontEntry.tableList:
		try:
			tableEntry = tableDict[tableEntry.offset]
			if doReportOnly:
				print("\t%s offset: 0x%08X, already seen" % (tableEntry.tag.decode('ascii'), tableEntry.offset))
		except KeyError:
			tableDict[tableEntry.offset] = tableEntry
			if doReportOnly:
				print("\t%s offset: 0x%08X, checksum: 0x%08X, length: 0x%08X" % (tableEntry.tag.decode('ascii'), tableEntry.offset, tableEntry.checksum, tableEntry.length))
	return fontEntry


def writeOTFFont(fontEntry):

	dataList = []
	numTables = len(fontEntry.tableList)
	# Build the SFNT header
	data = struct.pack(sfntDirectoryFormat, fontEntry.sfntType, numTables, fontEntry.searchRange, fontEntry.entrySelector, fontEntry.rangeShift)
	dataList.append(data)
	
	fontOffset = sfntDirectorySize + numTables*sfntDirectoryEntrySize
	# Set the offsets in the tables.
	for tableEntry in fontEntry.tableList:
		tableEntry.offset = fontOffset
		fontOffset += tableEntry.length
	
	# build table entries in sfnt directory
	for tableEntry in fontEntry.tableList:
		tableData = struct.pack(sfntDirectoryEntryFormat, tableEntry.tag, tableEntry.checksum, tableEntry.offset, tableEntry.length)
		dataList.append(tableData)
	
	# add table data to font.
	for tableEntry in fontEntry.tableList:
		tableData = tableEntry.data
		dataList.append(tableData)
	
	fontData = b"".join(dataList)
	
	with open(fontEntry.fileName, "wb") as fp:
		fp.write(fontData)
	return

def run(args):
	fontPath, doReportOnly = parseArgs(args)
	print("Input font:", fontPath)
	

	with open(fontPath, "rb") as fp:
		data = fp.read()

	TTCTag, version, numFonts = struct.unpack(ttcHeaderFormat, data[:ttcHeaderSize])
	offsetdata = data[ttcHeaderSize:]
	
	fontList = []
	i = 0
	tableDict = {} # Used to record whcih tables have been reported before.
	while i < numFonts:
		offset = struct.unpack(offsetFormat, offsetdata[:offsetSize])[0]
		if doReportOnly:
			print("font %s offset: %s/0x%08X." % (i, offset,offset), end=' ')
		fontEntry = readFontFile(offset, data, tableDict, doReportOnly)
		fontList.append(fontEntry)
		offsetdata = offsetdata[offsetSize:]
		i += 1
		
	if not doReportOnly:		
		for fontEntry in fontList:
			writeOTFFont(fontEntry)
			print("Output font:", fontEntry.fileName)
	print("Done")

if __name__ == "__main__":
	try:
		run(sys.argv[1:])
	except (OTCError) as e:
		print(e)
		
