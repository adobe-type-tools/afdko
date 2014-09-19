""" AFDKOPython copyCFFCharstrings.py -src <file1> -dst <file2>[,file3,...filen_n]

Copies the CFF charstrings and subrs from src to dst fonts.

"""

__help__ = """AFDKOPython copyCFFCharstrings.py -src <file1> -dst <file2>
Copies the CFF charstrings and subrs from src to dst.
v1.002 Aug 27 1014
"""

import os
import sys
import re
import struct
import subprocess
from fontTools.ttLib import TTFont, getTableModule, TTLibError
import traceback

class LocalError(TypeError):
	pass
	
def getOpts():
	srcPath = None
	dstPath = None
	i = 0
	args = sys.argv[1:]
	while i < len(args):
		arg = args[i]
		i += 1
		if arg =="-dst":
			dstPath = args[i]
			i += 1
		elif arg == "-src":
			srcPath = args[i]
			i += 1
		else:
			print "Did not recognize argument: ", arg
			print __help__
			raise LocalError()
			
	if not (srcPath and dstPath):
		print "You must supply source and destination font paths."
		print __help__
		raise LocalError()
	
	if not os.path.exists(srcPath):
		print "Source path does not exist:", srcPath
		raise LocalError()
	
	dstPathList = dstPath.split(",")
	for dstPath in dstPathList:
		if not os.path.exists(dstPath):
			print "Destination path does not exist:", dstPath
			raise LocalError()
	return srcPath, dstPathList


def makeTempOTF(srcPath):
	ff = file(srcPath, "rb")
	data = ff.read()
	ff.close()
	try:
		ttFont = TTFont()
		cffModule = getTableModule('CFF ')
		cffTable = cffModule.table_C_F_F_('CFF ')
		ttFont['CFF '] = cffTable
		cffTable.decompile(data, ttFont)
	except:
		print "\t%s" %(traceback.format_exception_only(sys.exc_type, sys.exc_value)[-1])
		print "Attempted to read font %s  as CFF." % filePath
		raise LocalError("Error parsing font file <%s>." % filePath)
	return ttFont

def getTTFontCFF(filePath):
	isOTF = True
	try:
		ttFont = TTFont(filePath)
	except (IOError, OSError):
		raise LocalError("Error opening or reading from font file <%s>." % filePath)
	except TTLibError:
		# Maybe it is a CFF. Make a dummy TTF font for fontTools to work with.
		ttFont = makeTempOTF(filePath)
		isOTF = False
	try:
		cffTable = ttFont['CFF ']
	except KeyError:
		raise LocalError("Error: font is not a CFF font <%s>." % filePath)
	return ttFont, cffTable.cff, isOTF

def validatePD(srcPD, dstPD):
	# raise LocalError if the hints differ.
	for key in ["BlueScale", "BlueShift", "BlueFuzz", "BlueValues", "OtherBlues", "FamilyBlues", "FamilyOtherBlues", "StemSnapH", "StemSnapV", "StdHW", "StdVW", "ForceBold", "LanguageGroup"]:
		err = 0
		if dstPD.rawDict.has_key(key):
			if not srcPD.rawDict.has_key(key):
				err = 1
			else:
				srcVal = eval("srcPD.%s" % (key))
				dstVal = eval("dstPD.%s" % (key))
				if (srcVal != dstVal):
					err = 1
		elif srcPD.rawDict.has_key(key):
			err = 1
		if err:
			break
	if err:
		print "Quitting. FDArray Private hint info  does not match for FD[%s]." % (i)
		raise LocalError()
	return

def copyData(srcPath, dstPath):

	srcTTFont, srcCFFTable, srcIsOTF = getTTFontCFF(srcPath)
	srcTopDict = srcCFFTable.topDictIndex[0]

	dstTTFont, dstCFFTable, dstIsOTF = getTTFontCFF(dstPath)
	dstTopDict = dstCFFTable.topDictIndex[0]
	
	# Check that ROS, charset, and hinting parameters all match.
	if srcTopDict.ROS != dstTopDict.ROS:
		print "Quitting. ROS does not match. src: %s dst: %s." % (srcTopDict.ROS,  dstTopDict.ROS)
		return
		
	if srcTopDict.CIDCount != dstTopDict.CIDCount:
		print "Quitting. CIDCount does not match. src: %s dst: %s." % (srcTopDict.CIDCount,  dstTopDict.CIDCount)
		return
		
	if srcTopDict.charset != dstTopDict.charset:
		print "Quitting. charset does not match.."
		return
	
	numFD = len(srcTopDict.FDArray)
	if numFD != len(dstTopDict.FDArray):
		print "Quitting. FDArray count does not match. src: %s dst: %s." % (srcTopDict.FDArray.count,  dstTopDict.FDArray.count)
		return
	
	for i in range(numFD):
		srcFD = srcTopDict.FDArray[i]
		# srcFD.FontName
		srcPD = srcFD.Private
		dstFD = dstTopDict.FDArray[i]
		dstPD = dstFD.Private
		validatePD(srcPD, dstPD) # raises LocalError if the hints differ.
		
	# All is OK. Update the font names.	
	for i in range(numFD):
		srcFD = srcTopDict.FDArray[i]
		dstFD = dstTopDict.FDArray[i]
		srcFD.FontName = dstFD.FontName
	
	# Update the CID name.
	if dstTopDict.rawDict.has_key("version"):
		srcTopDict.version = dstTopDict.version

	if dstTopDict.rawDict.has_key("Notice"):
		srcTopDict.Notice = dstTopDict.Notice

	if dstTopDict.rawDict.has_key("Copyright"):
		srcTopDict.Copyright = dstTopDict.Copyright

	if dstTopDict.rawDict.has_key("FullName"):
		srcTopDict.FullName = dstTopDict.FullName

	if dstTopDict.rawDict.has_key("FamilyName"):
		srcTopDict.FamilyName = dstTopDict.FamilyName

	if dstTopDict.rawDict.has_key("Weight"):
		srcTopDict.Weight = dstTopDict.Weight

	if dstTopDict.rawDict.has_key("UniqueID"):
		srcTopDict.UniqueID = dstTopDict.UniqueID

	if dstTopDict.rawDict.has_key("XUID"):
		srcTopDict.XUID = dstTopDict.XUID

	if dstTopDict.rawDict.has_key("CIDFontVersion"):
		srcTopDict.CIDFontVersion = dstTopDict.CIDFontVersion

	if dstTopDict.rawDict.has_key("CIDFontRevision"):
		srcTopDict.CIDFontRevision = dstTopDict.CIDFontRevision

	
	for i in range(len(srcCFFTable.fontNames)):
		srcCFFTable.fontNames[i] = dstCFFTable.fontNames[i]
	
	cffTable = srcTTFont['CFF ']
	
	outputFile = dstPath + ".new"
	if dstIsOTF:
		dstTTFont['CFF '] = cffTable
		dstTTFont.save(outputFile)
		print "Wrote new OTF file:", outputFile
	else:
		data = cffTable.compile(dstTTFont)
		tf = file(outputFile, "wb")
		tf.write(data)
		tf.close()
		print "Wrote new CFF file:", outputFile
		
	srcTTFont.close()
	dstTTFont.close()
	
def run():
	srcPath, dstPathList = getOpts()

	for dstPath in dstPathList:
		copyData(srcPath, dstPath)
	
if __name__ == "__main__":
	#try:
		run()
	#except LocalError:
	#	pass
