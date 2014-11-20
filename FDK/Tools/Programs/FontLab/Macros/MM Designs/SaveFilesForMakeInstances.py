#FLM: Save Files for MakeInstances

###################################################
### THE VALUES BELOW CAN BE EDITED AS NEEDED ######
###################################################

kDefaultMMFontFileName = "mmfont.pfa"
kInstancesDataFileName = "instances"
kCompositeDataName = "temp.composite.dat"

###################################################

__copyright__ =  """
Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0.
"""

__doc__ = """
Save Files for MakeInstances v1.0 - February 15 2010

This script will do part of the work to create a set of single-master fonts
("instances") from a Multiple Master (MM) FontLab font. It will save a 
Type 1 MM font (needed by the makeInstances program) and, in some cases,
a text file named 'temp.composite.dat' that contains data related with 
composite glyphs.

You must then run the makeInstances program to actually build the instance Type 1 
fonts. makeInstances can remove working glyphs, and rename MM exception glyphs. 
It will also do overlap removal, and autohint the instance fonts. This last is 
desirable, as autohinting which is specific to an instance font is usually 
significantly better than the hinting from interpolating the MM font hints. 
As always with overlap removal, you should check all affected glyphs - it
doesn't always do the right thing.

Note that the makeInstances program can be run alone, given an MM Type1 font
file. However, if you use the ExceptionSuffixes keyword, then you must run
this script first. The script will make a file that identifies composite glyphs, 
and allows makeInstances to correctly substitute contours in the composite glyph 
from the exception glyph. This is necessary because FontLab cannot write all the 
composite glyphs as Type 1 composites (also known as SEAC glyphs). This script 
must be run again to renew this data file whenever changes are made to composite 
glyphs.

Both this script and the "makeInstances" program depend on info provided by an
external text file named "instances", which contains all the instance-specific 
values. The "instances" file must be a simple text file, located in the same 
folder as the MM FontLab file. 

For information on how to format the "instances" file, please read the 
documentation in the InstanceGenerator.py script.

==================================================

Versions:
v1.0 - Feb 15 2010 - Initial release

"""

import copy
import re
import os

kFieldsKey = "#KEYS:"
kFamilyName = "FamilyName"
kFontName = "FontName"
kFullName = "FullName"
kWeight = "Weight"
kCoordsKey = "Coords"
kIsBoldKey = "IsBold" # This is changed to kForceBold in the instanceDict when reading in the instance file.
kForceBold = "ForceBold"
kIsItalicKey = "IsItalic"
kExceptionSuffixes = "ExceptionSuffixes"
kExtraGlyphs = "ExtraGlyphs"

kFixedFieldKeys = {
		# field index: key name
		0:kFamilyName,
		1:kFontName,
		2:kFullName,
		3:kWeight,
		4:kCoordsKey,
		5:kIsBoldKey,
		}

kNumFixedFields = len(kFixedFieldKeys)

kBlueScale = "BlueScale"
kBlueShift = "BlueShift"
kBlueFuzz = "BlueFuzz"
kBlueValues = "BlueValues"
kOtherBlues = "OtherBlues"
kFamilyBlues = "FamilyBlues"
kFamilyOtherBlues = "FamilyOtherBlues"
kStdHW = "StdHW"
kStdVW = "StdVW"
kStemSnapH = "StemSnapH"
kStemSnapV = "StemSnapV"

kAlignmentZonesKeys = [kBlueValues, kOtherBlues, kFamilyBlues, kFamilyOtherBlues]
kTopAlignZonesKeys = [kBlueValues, kFamilyBlues]
kMaxTopZonesSize = 14 # 7 zones
kBotAlignZonesKeys = [kOtherBlues, kFamilyOtherBlues]
kMaxBotZonesSize = 10 # 5 zones
kStdStemsKeys = [kStdHW, kStdVW]
kMaxStdStemsSize = 1
kStemSnapKeys = [kStemSnapH, kStemSnapV]
kMaxStemSnapSize = 12 # including StdStem


class ParseError(ValueError):
	pass
	

def validateArrayValues(arrayList, valuesMustBePositive):
	for i in range(len(arrayList)):
		try:
			arrayList[i] = eval(arrayList[i])
		except (NameError, SyntaxError):
			return
		if valuesMustBePositive:
			if arrayList[i] < 0:
				return
	return arrayList


def readInstanceFile(instancesFilePath):
	f = open(instancesFilePath, "rt")
	data = f.read()
	f.close()
	
	lines = data.splitlines()
	
	i = 0
	parseError = 0
	keyDict = copy.copy(kFixedFieldKeys)
	numKeys = kNumFixedFields
	numLines = len(lines)
	instancesList = []
	
	for i in range(numLines):
		line = lines[i]
		
		# Skip over blank lines
		line2 = line.strip()
		if not line2:
			continue

		# Get rid of all comments. If we find a key definition comment line, parse it.
		commentIndex = line.find('#')
		if commentIndex >= 0:
			if line.startswith(kFieldsKey):
				if instancesList:
					print "ERROR: Header line (%s) must preceed a data line." % kFieldsKey
					raise ParseError
				# parse the line with the field names.
				line = line[len(kFieldsKey):]
				line = line.strip()
				keys = line.split('\t')
				keys = map(lambda name: name.strip(), keys)
				numKeys = len(keys)
				k = kNumFixedFields
				while k < numKeys:
					keyDict[k] = keys[k]
					k +=1
				continue
			else:
				line = line[:commentIndex]
				continue

		# Must be a data line.
		fields = line.split('\t')
		fields = map(lambda datum: datum.strip(), fields)
		numFields = len(fields)
		if (numFields != numKeys):
			print "ERROR: In line %s, the number of fields %s does not match the number of key names %s (FamilyName, FontName, FullName, Weight, Coords, IsBold)." % (i+1, numFields, numKeys)
			parseError = 1
			continue
		
		instanceDict= {}
		#Build a dict from key to value. Some kinds of values needs special processing.
		for k in range(numFields):
			key = keyDict[k]
			field = fields[k]
			if not field:
				continue
			if field in ["Default", "None", "FontBBox"]:
				# FontBBox is no longer supported - I calculate the real
				# instance fontBBox from the glyph metrics instead,
				continue
			if key == kFontName:
				value = field
			elif key in [kExtraGlyphs, kExceptionSuffixes]:
				value = eval(field)
			elif key in [kIsBoldKey, kIsItalicKey, kCoordsKey]:
				try:
					value = eval(field) # this works for all three fields.
					
					if key == kIsBoldKey: # need to convert to Type 1 field key.
						instanceDict[key] = value
						# add kForceBold key.
						key = kForceBold
						if value == 1:
							value = "true"
						else:
							value = "false"
					elif key == kIsItalicKey:
						if value == 1:
							value = "true"
						else:
							value = "false"
					elif key == kCoordsKey:
						if type(value) == type(0):
							value = (value,)
				except (NameError, SyntaxError):
					print "ERROR: In line %s, the %s field has an invalid value." % (i+1, key)
					parseError = 1
					continue

			elif field[0] in ["[","{"]: # it is a Type 1 array value. Turn it into a list and verify that there's an even number of values for the alignment zones
				value = field[1:-1].split() # Remove the begin and end brackets/braces, and make a list
				
				if key in kAlignmentZonesKeys:
					if len(value) % 2 != 0:
						print "ERROR: In line %s, the %s field does not have an even number of values." % (i+1, key)
						parseError = 1
						continue
				
				if key in kTopAlignZonesKeys: # The Type 1 spec only allows 7 top zones (7 pairs of values)
					if len(value) > kMaxTopZonesSize:
						print "ERROR: In line %s, the %s field has more than %d values." % (i+1, key, kMaxTopZonesSize)
						parseError = 1
						continue
					else:
						newArray = validateArrayValues(value, False) # False = values do NOT have to be all positive
						if newArray:
							value = newArray
						else:
							print "ERROR: In line %s, the %s field contains invalid values." % (i+1, key)
							parseError = 1
							continue
					currentArray = value[:] # make copy, not reference
					value.sort()
					if currentArray != value:
						print "WARNING: In line %s, the values in the %s field were sorted in ascending order." % (i+1, key)
				
				if key in kBotAlignZonesKeys: # The Type 1 spec only allows 5 top zones (5 pairs of values)
					if len(value) > kMaxBotZonesSize:
						print "ERROR: In line %s, the %s field has more than %d values." % (i+1, key, kMaxBotZonesSize)
						parseError = 1
						continue
					else:
						newArray = validateArrayValues(value, False) # False = values do NOT have to be all positive
						if newArray:
							value = newArray
						else:
							print "ERROR: In line %s, the %s field contains invalid values." % (i+1, key)
							parseError = 1
							continue
					currentArray = value[:] # make copy, not reference
					value.sort()
					if currentArray != value:
						print "WARNING: In line %s, the values in the %s field were sorted in ascending order." % (i+1, key)
				
				if key in kStdStemsKeys:
					if len(value) > kMaxStdStemsSize:
						print "ERROR: In line %s, the %s field can only have %d value." % (i+1, key, kMaxStdStemsSize)
						parseError = 1
						continue
					else:
						newArray = validateArrayValues(value, True) # True = all values must be positive
						if newArray:
							value = newArray
						else:
							print "ERROR: In line %s, the %s field has an invalid value." % (i+1, key)
							parseError = 1
							continue
				
				if key in kStemSnapKeys: # The Type 1 spec only allows 12 stem widths, including 1 standard stem
					if len(value) > kMaxStemSnapSize:
						print "ERROR: In line %s, the %s field has more than %d values." % (i+1, key, kMaxStemSnapSize)
						parseError = 1
						continue
					else:
						newArray = validateArrayValues(value, True) # True = all values must be positive
						if newArray:
							value = newArray
						else:
							print "ERROR: In line %s, the %s field contains invalid values." % (i+1, key)
							parseError = 1
							continue
					currentArray = value[:] # make copy, not reference
					value.sort()
					if currentArray != value:
						print "WARNING: In line %s, the values in the %s field were sorted in ascending order." % (i+1, key)
			else:
				# either a single number or a string.
				if re.match(r"^[-.\d]+$", field):
					value = field #it is a Type 1 number. Pass as is, as a string.
				else:
					value = field
			
			instanceDict[key] = value
				
		if (kStdHW in instanceDict and kStemSnapH not in instanceDict) or (kStdHW not in instanceDict and kStemSnapH in instanceDict):
			print "ERROR: In line %s, either the %s value or the %s values are missing or were invalid." % (i+1, kStdHW, kStemSnapH)
			parseError = 1
		elif (kStdHW in instanceDict and kStemSnapH in instanceDict): # cannot be just 'else' because it will generate a 'KeyError' when these hinting parameters are not provided in the 'instances' file
			if instanceDict[kStemSnapH][0] != instanceDict[kStdHW][0]:
				print "ERROR: In line %s, the first value in %s must be the same as the %s value." % (i+1, kStemSnapH, kStdHW)
				parseError = 1

		if (kStdVW in instanceDict and kStemSnapV not in instanceDict) or (kStdVW not in instanceDict and kStemSnapV in instanceDict):
			print "ERROR: In line %s, either the %s value or the %s values are missing or were invalid." % (i+1, kStdVW, kStemSnapV)
			parseError = 1
		elif (kStdVW in instanceDict and kStemSnapV in instanceDict): # cannot be just 'else' because it will generate a 'KeyError' when these hinting parameters are not provided in the 'instances' file
			if instanceDict[kStemSnapV][0] != instanceDict[kStdVW][0]:
				print "ERROR: In line %s, the first value in %s must be the same as the %s value." % (i+1, kStemSnapV, kStdVW)
				parseError = 1
		
		instancesList.append(instanceDict)
		
	if parseError or len(instancesList) == 0:
		raise(ParseError)
		
	return instancesList


def saveCompositeInfo(fontMM, mmParentDir):
	filePath = os.path.join(mmParentDir, kCompositeDataName)
	numGlyphs = len(fontMM)
	glyphDict = {}
	numMasters = fontMM.glyphs[0].layers_number
	for gid in range(numGlyphs):
		glyph = fontMM.glyphs[gid]
		lenComps = len(glyph.components)
		if lenComps == 0:
			continue
		compList = []
		glyphDict[glyph.name] = compList
		numBaseContours = glyph.GetContoursNumber()
		pathIndex = numBaseContours
		for cpi in range(lenComps):
			component = glyph.components[cpi]
			compGlyph = fontMM.glyphs[component.index]
			compName = compGlyph.name,
			compEntry = [compName, numBaseContours + cpi]
			metricsList = [None]*numMasters
			seenAnyChange = 0
			for mi in range(numMasters):
				shift = component.deltas[mi]
				scale = component.scales[mi]
				shiftEntry = scaleEntry = None
				if (shift.x != 0) or (shift.y != 0):
					shiftEntry = (shift.x, shift.y)
				if (scale.x != 1.0) or (scale.y !=1.0 ):
					scaleEntry = (scale.x, scale.y)
				if scaleEntry or shiftEntry:
					metricsEntry = (shiftEntry, scaleEntry)
					seenAnyChange = 1
				else:
					metricsEntry = None
				metricsList[mi] = metricsEntry
			compName = fontMM.glyphs[component.index].name
			if seenAnyChange:
				compList.append([compName, pathIndex, metricsList])
			else:
				compList.append([compName, pathIndex, None])
			pathIndex += compGlyph.GetContoursNumber()
			
	fp = open(filePath, "wt")
	fp.write(repr(glyphDict))
	fp.close()
				

def saveFiles():
	try:
		parentDir = os.path.dirname(os.path.abspath(fl.font.file_name))
	except AttributeError:
		print "The font has not been saved. Please save the font and try again."
		return

	instancesFilePath = os.path.join(parentDir, kInstancesDataFileName)
	
	if not os.path.isfile(instancesFilePath):
		print "Could not find the file named '%s' in the path below\n\t%s" % (kInstancesDataFileName, parentDir)
		return

	try:
		print "Parsing instances file..."
		instancesList = readInstanceFile(instancesFilePath)
	except ParseError:
		print "Error parsing file or file is empty."
		return

	# Set FontLab preferences
	flPrefs = Options()
	flPrefs.Load()
	flPrefs.T1Terminal = 0 # so we don't have to close the dialog with each instance.
	flPrefs.T1Encoding = 1 # always write Std Encoding.
	flPrefs.T1Decompose = 1 # Do  decompose SEAC chars
	flPrefs.T1Autohint = 0 # Do not autohint unhinted chars

	# Generate mmfont.pfa
	pfaPath = os.path.join(parentDir, kDefaultMMFontFileName)
	print "Saving Type 1 MM font file to:%s\t%s" % (os.linesep, pfaPath)
	fl.GenerateFont(eval("ftTYPE1ASCII_MM"), pfaPath)
	
	# Save the composite glyph data, but only if it's necessary
	if (kExceptionSuffixes in instancesList[0] or kExtraGlyphs in instancesList[0]):
		compositePath = os.path.join(parentDir, kCompositeDataName)
		print "Saving composite glyphs data to:%s\t%s" % (os.linesep, compositePath)
		saveCompositeInfo(fl.font, parentDir)

	print "Done!"


def run():
	global debug
	if fl.count == 0:
		print 'No font opened.'
		return

	if len(fl.font) == 0:
		print 'The font has no glyphs.'
		return

	if fl.font[0].layers_number == 1:
		print 'The font is not MM.'
		return

	else:
		saveFiles()


if __name__ == "__main__":
	run()
