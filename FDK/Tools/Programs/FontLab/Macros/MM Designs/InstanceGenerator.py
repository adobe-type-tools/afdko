#FLM: Instance Generator

###################################################
### THE VALUES BELOW CAN BE EDITED AS NEEDED ######
###################################################

kFontInstanceFileName = "font.pfa"
kInstancesDataFileName = "instances"
kPrefsFileName =  "InstanceGenerator.prefs"
kVFBinstancesFolderName = "_vfbInstances_"

###################################################

__copyright__ = __license__ =  """
Copyright (c) 2006-2013 Adobe Systems Incorporated. All rights reserved.
 
Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"), 
to deal in the Software without restriction, including without limitation 
the rights to use, copy, modify, merge, publish, distribute, sublicense, 
and/or sell copies of the Software, and to permit persons to whom the 
Software is furnished to do so, subject to the following conditions:
 
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.
 
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
DEALINGS IN THE SOFTWARE.
"""

__doc__ = """
Instance Generator v2.4 - Mar 10 2013

This script will generate a set of single-master fonts ("instances") from a Multiple
Master (MM) FontLab font. For each instance, the script will create an Unix-style Type 1 
font file (PFA) and optionally a VFB file. The script can also write 'kern', 'mark' and 
'mkmk' feature files for each instance. These files will be saved in the same folder as 
the Type 1 font instance.

For information about how the "features.kern" file is created, please read the 
documentation in the WriteFeaturesKernFDK.py module that can be found 
in FontLab/Studio5/Macros/System/Modules/

For information about how the "features.mark/mkmk" files are created, please read the 
documentation in the WriteFeaturesMarkFDK.py module that can be found 
in FontLab/Studio5/Macros/System/Modules/

To access the script's options, hold down the CONTROL key while clicking on the play
button to run the script.

The script will first show a file selection dialog for picking the enclosing directory 
where the font instances (and optionally the feature files) will be written to.

The VFB files are written in a sub-directory of the selected folder; this sub-directory 
is named "_vfbInstances_". The Type 1 font files are written to a sub-directory path 
<selected_folder>/<face_name>, where the face name is derived by taking the part of 
the font's PostScript name after the hyphen, or "Regular" if there is no hyphen (e.g. 
if the font's PostScript name is MyFont-BoldItalic, the folder will be named "BoldItalic")

This script depends on info provided by an external simple text file named "instances".
This file must be located in the same folder as the MM FontLab file. Each line specifies 
one instance, as a record of tab-delimited fields. The first 6 fields are always, in order:

  FamilyName : This is the Preferred Family name.
  FontName   : This is the PostScript name.
  FullName   : This is the Preferred Full name.
  Weight     : This is the Weight name.
  Coords     : This is a single value, or a sequence of comma-separated integer values.
			Each integer value corresponds to an axis.
  IsBold     : This must be either 1 (True) or 0 (False). This will be translated into
			Postscript FontDict ForceBold field.

  Examples:
	# Font with 1 axis (single-axis MM font)
	MyFontStd<tab>MyFontStd-Bold<tab>MyFont Std Bold<tab>Bold<tab>650<tab>1
	MyFontStd<tab>MyFontStd-Regular<tab>MyFont Std Regular<tab>Regular<tab>350<tab>0	

	# Font with 2 axis (multi-axis MM font)
	MyFontPro<tab>MyFontPro-Bold<tab>MyFont Pro Bold<tab>Bold<tab>650,300<tab>1
	MyFontPro<tab>MyFontPro-Regular<tab>MyFont Pro Regular<tab>Regular<tab>350,300<tab>0


If only these six fields are used, then there is no need for a header line.
However, if any additional fields are used, then the file must contain a line
starting with "#KEYS:", and continuing with tab-delimited field names. Two of
the additional fields allowed are:

  ExceptionSuffixes : A list of suffixes, used to identify MM exception glyphs. An
  MM exception glyph is one which is designed for use in only one instance, and is
  used by replacing every occurence of the glyphs that match the MM exception
  glyph's base name. The MM exception glyph is used in no other instance. This
  allows the developer to fix problems with just a few glyphs in each instance.
  For example, in the record for HypatiaSansPro-Black the 'instances' file
  specifies an ExceptionSuffix suffix list which contains the suffix "-black", and
  there is an MM exception glyph named "a-black", then the glyph "a" will be
  replaced by the glyph "a-black", and all composite glyphs that use "a" will be
  updated to use the contours from "a-black" instead.
  
  ExtraGlyphs : A list of working glyph names, to be omitted from the instances.
  This may be a complete glyph name, or a wild-card pattern. A pattern may take two
  forms: "*<suffix>", which will match any glyph ending with that suffix, or a
  regular expression which must match entire glyph names. The pattern must begin
  with "^" and end with "$". You do not need to include glyph names which match
  an MM Exception glyph suffix: such glyphs will not be written to any instance.

  Example:
	#KEYS:FamilyName<tab>FontName<tab>FullName<tab>Weight<tab>Coords<tab>IsBold<tab>ExceptionSuffixes<tab>ExtraGlyphs
	MyFontPro<tab>MyFontPro-ExtraLight<tab>MyFont Pro ExtraLight<tab>ExtraLight<tab>0<tab>0<tab><tab>["*-black","*-aux"]
	MyFontPro<tab>MyFontPro-Black<tab>MyFont Pro Black<tab>Black<tab>1000<tab>0<tab>["-black"]<tab>["*-aux"]


The other additional field names are assumed to be the names for Postscript FontDict
keys, such as: BlueScale, BlueShift, BlueFuzz, BlueValues, OtherBlues, FamilyBlues, FamilyOtherBlues, StdHW, StdVW, StemSnapH, StemSnapV

  Example:
	#KEYS:FamilyName<tab>FontName<tab>FullName<tab>Weight<tab>Coords<tab>IsBold<tab>BlueFuzz<tab>BlueScale<tab>BlueValues
	MyFontPro<tab><MyFontPro-Regular<tab>MyFont Pro Regular<tab>Regular<tab>160,451<tab>0<tab>0<tab>0.0479583<tab>[-18 0 395 410 439 453 596 608 615 633 672 678]
	MyFontPro<tab><MyFontPro-Bold<tab>MyFont Pro Bold<tab>Bold<tab>1000,451<tab>1<tab>0<tab>0.0479583<tab>[-18 0 400 414 439 453 584 596 603 621 653 664]


All empty lines and other lines in the "instances" file starting with the number sign (#) will be ignored.

The data supplied in the "instances" file, is used for specifying the instance's values,
and for providing the font names used in the FontInfo dictionary of the Type 1 fonts.

==================================================
Versions:
v1.0   - Apr 24 2007 - Initial release
v1.1   - Dec 03 2007 - Robofab dependency removed (code changes suggested by Karsten Luecke)
v1.2   - Mar 04 2008 - Macro partly rewritten to avoid FontLab Windows crashing
v1.5   - Mar 03 2009 - Added support for "MM exception glyphs" and overrides for the instance font FontDict.
                       Use FDK commands to make font instances, and to remove overlaps and autohint.
v1.6   - May 01 2009 - Added a few more lines to the help text, fixed a syntax warning when opening the dialog.
v2.0   - Feb 15 2010 - Added option to generate 'mark' feature. Improved the code where needed, and removed unused code.
                       Moved the code that creates the files for the command-line makeInstances (mmfont.pfa, temp.composite.dat) 
                       to a separate macro called "SaveFilesForMakeInstances.py"
v2.1   - Feb 19 2010 - Improved the dialog window.
v2.2   - Apr 21 2011 - Added the option of writing the mark classes in a separate file
v2.3   - Jun 15 2012 - Added the option to output the lookups in the format required for Indian scripts.
v2.3.1 - Jul 19 2012 - Changed the description of one of the options in the UI.
v2.4   - Mar 10 2013 - Added subtable-option to dialog window. Other minor improvements.

"""

import os, sys, re, copy, math, time

try:
	from AdobeFontLabUtils import checkControlKeyPress, checkShiftKeyPress
	import WriteFeaturesKernFDK, WriteFeaturesMarkFDK
except ImportError,e:
	print "Failed to find the Adobe FDK support scripts."
	print "Please run the script FDK/Tools/FontLab/installFontLabMacros.py script, and try again." 
	print "Current directory: ", os.path.abspath(os.getcwd())
	print "Current list of search paths for modules: "
	import pprint
	pprint.pprint(sys.path)
	raise e

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

kHintingKeys = [kBlueScale, kBlueShift, kBlueFuzz, kBlueValues, kOtherBlues, kFamilyBlues, kFamilyOtherBlues, kStdHW, kStdVW, kStemSnapH, kStemSnapV]
kAlignmentZonesKeys = [kBlueValues, kOtherBlues, kFamilyBlues, kFamilyOtherBlues]
kTopAlignZonesKeys = [kBlueValues, kFamilyBlues]
kMaxTopZonesSize = 14 # 7 zones
kFLbugMaxTopZonesSize = 12 # The UI allows assigning all the values, but that's not possible via Python
kBotAlignZonesKeys = [kOtherBlues, kFamilyOtherBlues]
kMaxBotZonesSize = 10 # 5 zones
kFLbugMaxBotZonesSize = 8 # The UI allows assigning all the values, but that's not possible via Python
kStdStemsKeys = [kStdHW, kStdVW]
kMaxStdStemsSize = 1
kStemSnapKeys = [kStemSnapH, kStemSnapV]
kMaxStemSnapSize = 12 # including StdStem
kFLbugMaxStemSnapSize = 11 # The UI allows assigning all the values, but that's not possible via Python


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


def findExtraGlyphMatches(extraGlyphs, charDict):
	charList = charDict.keys()
	charList.sort()
	glyphList = []
	glyphDict = {}
	for line in extraGlyphs:
		if line.startswith('*'):
			matchList = filter(lambda name: name.endswith(line[1:]), charList)
			if matchList:
				glyphList.extend(matchList)
			else:
				print "WARNING: Extra glyph suffix '%s' did not match in the char list." % (line)
		elif line.startswith('^'):
			matchList = filter(lambda name: re.search(line, name), charList)
			if matchList:
				glyphList.extend(matchList)
			else:
				print "WARNING: Extra glyph search pattern '%s' did not match in the char list." % (line)
		else:
			if charDict.has_key(line):
				glyphList.append(line)
			
	# We do not need to eliminate duplicates; that happens when these lists are added to extraDict.
	for name in glyphList:
		glyphDict[name] = 1
	return glyphDict
	
	
def findExceptionGlyphMatches(exceptionSuffixes, charDict):
	charList = charDict.keys()
	charList.sort()
	glyphList = []
	for suffix in exceptionSuffixes:
		matchList = filter(lambda name: name.endswith(suffix), charList)
		if not matchList:
			print "WARNING: Exception glyph suffix '%s' did not match in the char list." % (suffix)
		else:
			matchList2 = []
			missingList = []
			for name in matchList:
				charList.remove(name)
				if charDict.has_key(name[:-len(suffix)]):
					matchList2.append([name, name[:-len(suffix)]] )
				else:
					missingList.append(name)
			if matchList2:
				glyphList.extend(matchList2)
			if missingList:
				print "WARNING: Exception glyphs '%s' did not have alternates without suffix '%s', and were skipped." % (missingList, suffix)
	glyphDict = {}
	for entry in glyphList:
		glyphDict[entry[1]] = entry[0] # key is std name, value is exception name
	return glyphDict


def getExceptionNames(f, instanceDict):
	extraGlyphDict = exceptionDict = {}
	charList = [glyph.name for glyph in f.glyphs]
	charDict = {}
	for glyphName in charList:
		charDict[glyphName] = 1
		
	extraGlyphs = instanceDict.get(kExtraGlyphs, None)
	if extraGlyphs:
		extraGlyphDict = findExtraGlyphMatches(extraGlyphs, charDict)
	
	exceptionSuffixes = instanceDict.get(kExceptionSuffixes, None)
	if exceptionSuffixes:
		exceptionDict = findExceptionGlyphMatches(exceptionSuffixes, charDict)
		
	return extraGlyphDict, exceptionDict


def handleExceptionGlyphs(f, fontInstanceDict):
	extraDict, exceptionDict = getExceptionNames(f, fontInstanceDict)
	if exceptionDict:
		stdnames = exceptionDict.keys()
		stdnames.sort()
		for x in range(len(stdnames)):
			stdName = stdnames[x]
			exceptionName = exceptionDict[stdName]
			if f.FindGlyph(stdName) != -1:
				f[stdName].Clear()  # removes the original outlines
				f[stdName].Insert(f[exceptionName])  # places the new outlines
				extraDict[exceptionName] = 1 # add exception glyph to name to delete.
			else:
				print "\t- exception glyph target %s not found" % stdName
				
	if extraDict:
		extraGlyphs = extraDict.keys()
		extraGlyphs.sort()
		for name in extraGlyphs:
			glyphIndex = f.FindGlyph(name)
			if glyphIndex != -1:
				del f.glyphs[glyphIndex]
			else:
				print "\t- extra glyph %s not found" % name


def assignHintingParams(f, fontInstanceDict):
	if kBlueScale in fontInstanceDict:
		f.blue_scale[0] = eval(fontInstanceDict[kBlueScale])
	
	if kBlueShift in fontInstanceDict:
		f.blue_shift[0] = eval(fontInstanceDict[kBlueShift])
	
	if kBlueFuzz in fontInstanceDict:
		f.blue_fuzz[0] = eval(fontInstanceDict[kBlueFuzz])
	
	if kBlueValues in fontInstanceDict:
		if len(fontInstanceDict[kBlueValues]) > kFLbugMaxTopZonesSize: # FontLab bug
			print "WARNING: The last %d values were dropped from the %s field. Due to a bug in FontLab Studio 5.x, the maximum number of zones that can be assigned via Python is %d." % (kMaxTopZonesSize-kFLbugMaxTopZonesSize, kBlueValues, kFLbugMaxTopZonesSize/2)
			fontInstanceDict[kBlueValues] = fontInstanceDict[kBlueValues][:kFLbugMaxTopZonesSize]
		f.blue_values_num = len(fontInstanceDict[kBlueValues])
		for x in range(f.blue_values_num):
			f.blue_values[0][x] = fontInstanceDict[kBlueValues][x]
	
	if kFamilyBlues in fontInstanceDict:
		if len(fontInstanceDict[kFamilyBlues]) > kFLbugMaxTopZonesSize: # FontLab bug
			print "WARNING: The last %d values were dropped from the %s field. Due to a bug in FontLab Studio 5.x, the maximum number of zones that can be assigned via Python is %d." % (kMaxTopZonesSize-kFLbugMaxTopZonesSize, kFamilyBlues, kFLbugMaxTopZonesSize/2)
			fontInstanceDict[kFamilyBlues] = fontInstanceDict[kFamilyBlues][:kFLbugMaxTopZonesSize]
		f.family_blues_num = len(fontInstanceDict[kFamilyBlues])
		for x in range(f.family_blues_num):
			f.family_blues[0][x] = fontInstanceDict[kFamilyBlues][x]
	
	if kOtherBlues in fontInstanceDict:
		if len(fontInstanceDict[kOtherBlues]) > kFLbugMaxBotZonesSize: # FontLab bug
			print "WARNING: The last %d values were dropped from the %s field. Due to a bug in FontLab Studio 5.x, the maximum number of zones that can be assigned via Python is %d." % (kMaxBotZonesSize-kFLbugMaxBotZonesSize, kOtherBlues, kFLbugMaxBotZonesSize/2)
			fontInstanceDict[kOtherBlues] = fontInstanceDict[kOtherBlues][:kFLbugMaxBotZonesSize]
		f.other_blues_num = len(fontInstanceDict[kOtherBlues])
		for x in range(f.other_blues_num):
			f.other_blues[0][x] = fontInstanceDict[kOtherBlues][x]
	
	if kFamilyOtherBlues in fontInstanceDict:
		if len(fontInstanceDict[kFamilyOtherBlues]) > kFLbugMaxBotZonesSize: # FontLab bug
			print "WARNING: The last %d values were dropped from the %s field. Due to a bug in FontLab Studio 5.x, the maximum number of zones that can be assigned via Python is %d." % (kMaxBotZonesSize-kFLbugMaxBotZonesSize, kFamilyOtherBlues, kFLbugMaxBotZonesSize/2)
			fontInstanceDict[kFamilyOtherBlues] = fontInstanceDict[kFamilyOtherBlues][:kFLbugMaxBotZonesSize]
		f.family_other_blues_num = len(fontInstanceDict[kFamilyOtherBlues])
		for x in range(f.family_other_blues_num):
			f.family_other_blues[0][x] = fontInstanceDict[kFamilyOtherBlues][x]
	
	if kStemSnapH in fontInstanceDict:
		if len(fontInstanceDict[kStemSnapH]) > kFLbugMaxStemSnapSize: # FontLab bug
			print "WARNING: The last %d values were dropped from the %s field. Due to a bug in FontLab Studio 5.x, the maximum number of stems that can be assigned via Python is %d." % (kMaxStemSnapSize-kFLbugMaxStemSnapSize, kStemSnapH, kFLbugMaxStemSnapSize)
			fontInstanceDict[kStemSnapH] = fontInstanceDict[kStemSnapH][:kFLbugMaxStemSnapSize]
		f.stem_snap_h_num = len(fontInstanceDict[kStemSnapH])
		for x in range(f.stem_snap_h_num):
			f.stem_snap_h[0][x] = fontInstanceDict[kStemSnapH][x]

	if kStemSnapV in fontInstanceDict:
		if len(fontInstanceDict[kStemSnapV]) > kFLbugMaxStemSnapSize: # FontLab bug
			print "WARNING: The last %d values were dropped from the %s field. Due to a bug in FontLab Studio 5.x, the maximum number of stems that can be assigned via Python is %d." % (kMaxStemSnapSize-kFLbugMaxStemSnapSize, kStemSnapV, kFLbugMaxStemSnapSize)
			fontInstanceDict[kStemSnapV] = fontInstanceDict[kStemSnapV][:kFLbugMaxStemSnapSize]
		f.stem_snap_v_num = len(fontInstanceDict[kStemSnapV])
		for x in range(f.stem_snap_v_num):
			f.stem_snap_v[0][x] = fontInstanceDict[kStemSnapV][x]
	

def handleInstance(f, fontInstanceDict, instanceInfo):
	# Set names
	f.family_name = fontInstanceDict[kFamilyName]
	f.font_name = fontInstanceDict[kFontName]
	f.full_name = fontInstanceDict[kFullName]
	f.weight = fontInstanceDict[kWeight]
	
	instValues = fontInstanceDict[kCoordsKey]
	isBold = fontInstanceDict[kIsBoldKey]
	
	# This name does not go into the CFF font header. It's used in the 'features.kern' to have a record of the instance.
	# Builds information about the source font and instance values
	for x in range(len(instValues)):
		instanceInfo += '_' + str(instValues[x])
	f.menu_name = instanceInfo
	
	# Set Bold bit
	f.force_bold[0] = isBold

	# Determine if it's necessary to assign hinting parameters to the instance
	processHintingParams = False
	for key in fontInstanceDict:
		if key in kHintingKeys:
			processHintingParams = True
			break
	if processHintingParams:
		print '\tAssigning hinting parameters...'
		assignHintingParams(f, fontInstanceDict)

	if (kExceptionSuffixes in fontInstanceDict or kExtraGlyphs in fontInstanceDict):
		if (kExceptionSuffixes not in fontInstanceDict):
			fontInstanceDict[kExceptionSuffixes] = [] # in case nothing was provided, assign an empty list
		print '\tProcessing exception glyphs and/or extra glyphs...'
		handleExceptionGlyphs(f, fontInstanceDict)

	# Flatten glyphs
	print '\tDecomposing glyphs and removing overlaps...'
	for g in f.glyphs:
		g.Decompose()
		g.SelectAll()
		g.RemoveOverlap()

	return f


def makeFaceFolder(root, folder):
	facePath = os.path.join(root, folder)
	if not os.path.exists(facePath):
		os.makedirs(facePath)
	return facePath


def handleFont(folderPath, fontMM, fontInstanceDict, options):
	try:
		faceName = fontInstanceDict[kFontName].split('-')[1]
	except IndexError:
		faceName = 'Regular'

	print
	print faceName
	
	fontName = fontInstanceDict[kFontName]
	instValues = fontInstanceDict[kCoordsKey]

	try:
		fontInstance = Font(fontMM, instValues)  # creates instance
	except:
		print "Error: Could not create instance <%s> (%s)" % (instValues, fontName)
		return

	instanceInfo = os.path.basename(fontMM.file_name) # The name of the source MM VFB is recorded as part of the info regarding the instance
	fontInstance = handleInstance(fontInstance, fontInstanceDict, instanceInfo)
	fl.Add(fontInstance)
	
	if (options.genVFBs):
		print '\tSaving .vfb file...'
		vfbFolder = makeFaceFolder(folderPath, kVFBinstancesFolderName)
		vfbPath = os.path.join(vfbFolder, fontName)
		fl.Save((vfbPath + '.vfb'))
	
	print '\tSaving %s file...' % kFontInstanceFileName
	pfaFolder = makeFaceFolder(folderPath, faceName)
	pfaPath = os.path.join(pfaFolder, kFontInstanceFileName)
	fl.GenerateFont(eval("ftTYPE1ASCII"), pfaPath)
	
	if (options.genKernFeature):
		print "\tGenerating 'kern' feature..."
		WriteFeaturesKernFDK.KernDataClass(fontInstance, pfaFolder, options.minKern, options.writeTrimmed, options.writeSubtables)

	if (options.genMarkFeature):
		if (options.genMkmkFeature):
			print "\tGenerating 'mark' and 'mkmk' features..."
		else:
			print "\tGenerating 'mark' feature..."
		WriteFeaturesMarkFDK.MarkDataClass(fontInstance, pfaFolder, options.trimCasingTags, options.genMkmkFeature, options.writeClassesFile, options.indianScriptsFormat)
	
	fontInstance.modified = 0
	fl.Close(fl.ifont)


def makeInstances(options):
	fontMM = fl.font # MM Font
	axisNum = int(math.log(fontMM[0].layers_number, 2)) # Number of axis in font
	
	try:
		parentDir = os.path.dirname(os.path.abspath(fontMM.file_name))
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

	# A few checks before proceeding...
	if instancesList:
		# Make sure that the instance values is compatible with the number of axis in the MM font
		for i in range(len(instancesList)):
			instanceDict = instancesList[i]
			axisVal = instanceDict[kCoordsKey] # Get AxisValues strings
			if axisNum != len(axisVal):
				print 'ERROR:  The %s value for the instance named %s in the %s file is not compatible with the number of axis in the MM source font.' % (kCoordsKey, instanceDict[kFontName], kInstancesDataFileName)
				return
	
	folderPath = fl.GetPathName("Select parent directory to output instances")

	# Cancel was clicked or Esc key was pressed
	if not folderPath:
		return

	t1 = time.time()  # Initiates a timer of the whole process
	
	# Make sure that the Encoding options are set to 'StandardEncoding'
	flPrefs = Options()
	flPrefs.Load()
	flPrefs.T1Terminal = 0 # so we don't have to close the dialog with each instance.
	flPrefs.T1Encoding = 1 # always write Std Encoding.
	flPrefs.T1Decompose = 1 # Do  decompose SEAC chars
	
	# Process instances
	for fontInstance in instancesList:
		handleFont(folderPath, fontMM, fontInstance, options)

	t2 = time.time()
	elapsedSeconds = t2-t1
	
	if (elapsedSeconds/60) < 1:
		print '\nCompleted in %.1f seconds.\n' % elapsedSeconds
	else:
		print '\nCompleted in %.1f minutes.\n' % (elapsedSeconds/60)


class InstGenOptions:
	# Holds the options for the module.
	# The values of all member items NOT prefixed with "_" are written to/read from
	# a preferences file.
	# This also gets/sets the same member fields in the passed object.
	def __init__(self):
		self.genKernFeature = 1
		self.genMarkFeature = 0
		self.genMkmkFeature = 0
		self.writeClassesFile = 0
		self.indianScriptsFormat = 0
		self.trimCasingTags = 0
		self.genVFBs = 0
		self.minKern = 3
		self.writeTrimmed = 0
		self.writeSubtables = 1
		
		# items not written to prefs
		self._prefsBaseName = kPrefsFileName
		self._prefsPath = None

	def _getPrefs(self, callerObject = None):
		foundPrefsFile = 0

		# We will put the prefs file in a directory "Preferences" at the same level as the Macros directory
		dirPath = os.path.dirname(WriteFeaturesKernFDK.__file__)
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
					print("Failed to create prefs directory %s" % (dirPath))
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
				print("Prefs file exists but cannot be read %s" % (self._prefsPath))
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
			print("Saved prefs in %s." % self._prefsPath)
		except (IOError, OSError):
			print("Failed to write prefs file in %s." % self._prefsPath)


class InstGenDialog:
	def __init__(self):
		""" NOTE: the Get and Save preferences class methods access the preference values as fields
		of the dialog by name. If you want to change a preference value, the dialog control value must have
		the same field name.
		"""
		dWidth = 350
		dMargin = 25
		xMax = dWidth - dMargin

		# General Options section
		xA1 = dMargin + 20 # Left indent of the "Generate..." options
		yA0 = dMargin
		yA1 = yA0 + 30 # Y position of first option
		yA2 = yA1 + 30
		yA3 = yA2 + 30
		endYsection1 = yA3 + 30
		
		# Kern Feature Options section
		xB0 = xA1 - 5
		xB1 = xA1 + 20 # Left indent of "Minimum kerning" text
		xB2 = xA1
		yB0 = endYsection1 + 20
		yB1 = yB0 + 30 
		yB2 = yB1 + 30
		yB3 = yB2 + 30
		endYsection2 = yB3 + 30

		# Mark Feature Options section
		xC1 = xA1
		yC0 = endYsection2 + 20
		yC1 = yC0 + 30 
		yC2 = yC1 + 30 
		yC3 = yC2 + 30
		yC4 = yC3 + 30
		endYsection3 = yC4 + 30
		
		dHeight = endYsection3  + 70 # Total height of dialog
		
		self.d = Dialog(self)
		self.d.size = Point(dWidth, dHeight)
		self.d.Center()
		self.d.title = "Instance Generator Preferences"

		self.options = InstGenOptions()
		self.options._getPrefs(self) # This both loads prefs and assigns the member fields of the dialog.
		
		self.d.AddControl(STATICCONTROL,	Rect(dMargin, yA0, xMax, endYsection1), "frame", STYLE_LABEL, "General Options")
		self.d.AddControl(CHECKBOXCONTROL, Rect(xA1, yA1, xMax, aAUTO), "genKernFeature", STYLE_CHECKBOX, " Generate 'kern' feature")
		self.d.AddControl(CHECKBOXCONTROL, Rect(xA1, yA2, xMax, aAUTO), "genMarkFeature", STYLE_CHECKBOX, " Generate 'mark' feature")
		self.d.AddControl(CHECKBOXCONTROL, Rect(xA1, yA3, xMax, aAUTO), "genVFBs", STYLE_CHECKBOX, " Save FontLab VFB files of each instance")

		self.d.AddControl(STATICCONTROL,	Rect(dMargin, yB0, xMax, endYsection2), "frame2", STYLE_LABEL, "Kern Feature Options")
		self.d.AddControl(EDITCONTROL,	Rect(xB0, yB1-5, xB0+20, aAUTO), "minKern", STYLE_EDIT+cTO_CENTER) 
		self.d.AddControl(STATICCONTROL,	Rect(xB1, yB1, xMax, aAUTO), "legend", STYLE_LABEL, " Minimum kern value (inclusive)")
		self.d.AddControl(CHECKBOXCONTROL,	Rect(xB2, yB2, xMax, aAUTO), "writeTrimmed", STYLE_CHECKBOX, " Write trimmed pairs")
		self.d.AddControl(CHECKBOXCONTROL,	Rect(xB2, yB3, xMax, aAUTO), "writeSubtables", STYLE_CHECKBOX, " Write subtables")

		self.d.AddControl(STATICCONTROL,	Rect(dMargin, yC0, xMax, endYsection3), "frame3", STYLE_LABEL, "Mark Feature Options")
		self.d.AddControl(CHECKBOXCONTROL, Rect(xC1, yC1, xMax, aAUTO), "genMkmkFeature", STYLE_CHECKBOX, " Write mark-to-mark lookups")
		self.d.AddControl(CHECKBOXCONTROL, Rect(xC1, yC2, xMax, aAUTO), "trimCasingTags", STYLE_CHECKBOX, " Trim casing tags on anchor names")
		self.d.AddControl(CHECKBOXCONTROL, Rect(xC1, yC3, xMax, aAUTO), "writeClassesFile", STYLE_CHECKBOX, " Write mark classes in separate file")
		self.d.AddControl(CHECKBOXCONTROL, Rect(xC1, yC4, xMax, aAUTO), "indianScriptsFormat", STYLE_CHECKBOX, " Format the output for Indian scripts")

		helpYPos = dHeight-35
		self.d.AddControl(BUTTONCONTROL, Rect(dMargin, helpYPos, dMargin+60, helpYPos+20), "help", STYLE_BUTTON, "Help")

		# Uncheck "genMkmkFeature" if "genMarkFeature" is 0 in the saved options
		if self.genMarkFeature == 0:
			self.genMkmkFeature = 0


	def on_genKernFeature(self, code):
		self.d.GetValue("genKernFeature")
		
	def on_genMarkFeature(self, code):
		self.d.GetValue("genMarkFeature")
		# Disable "genMkmkFeature" if "genMarkFeature" is not checked
		self.d.Enable("genMkmkFeature", self.genMarkFeature)

	def on_genVFBs(self, code):
		self.d.GetValue("genVFBs")

	def on_minKern(self, code):
		self.d.GetValue("minKern")

	def on_writeTrimmed(self, code):
		self.d.GetValue("writeTrimmed")

	def on_writeSubtables(self, code):
		self.d.GetValue("writeSubtables")

	def on_genMkmkFeature(self, code):
		# It's not possible to disable the control when the dialog is created, probably due to a bug in FontLab
		# therefore the disable code is put here as well
		self.d.Enable("genMkmkFeature", self.genMarkFeature)
		# Do not allow the checkbox to toggle if "genMarkFeature" is not checked
		if self.genMarkFeature == 0:
			self.genMkmkFeature = 0
			self.d.PutValue("genMkmkFeature")
		else:
			self.d.GetValue("genMkmkFeature")

	def on_trimCasingTags(self, code):
		self.d.GetValue("trimCasingTags")

	def on_writeClassesFile(self, code):
		self.d.GetValue("writeClassesFile")

	def on_indianScriptsFormat(self, code):
		self.d.GetValue("indianScriptsFormat")

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
	

class InstGenHelpDialog:
	def __init__(self):
		dWidth = 700
		dMargin = 25
		dHeight = 500
		self.result = 0
		
		self.d = Dialog(self)
		self.d.size = Point(dWidth, dHeight)
		self.d.Center()
		self.d.title = "Instance Generator Help"
		self.d.ok = "Back"
		self.d.cancel = "Exit"
	
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
		dontShowDialog = 1
		result = 2
		dontShowDialog = checkControlKeyPress()
		debug =  not checkShiftKeyPress()
		if dontShowDialog:
			print "Hold down CONTROL key while starting this script in order to set options.\n"
			options = InstGenOptions()
			options._getPrefs() # load current settings from prefs
			makeInstances(options)
		else:
			# I do this funky little loop so that control will return to the main dialog after showing help.
			# Odd things happen to the dialog focus if you call the help dialog directly from the main dialog.
			# in FontLab 4.6
			while result == 2:
				IGd = InstGenDialog()
				result = IGd.Run() # returns 0 for cancel, 1 for ok, 2 for help
				if result == 1:
					options = InstGenOptions()
					options._getPrefs() # load current settings from prefs
					makeInstances(options)
				elif result == 2:
					IGh = InstGenHelpDialog()
					result = IGh.Run() # returns 0 for cancel, 2 for ok


if __name__ == "__main__":
	run()
