#FLM: Mark Feature Generator

###################################################
### THE VALUES BELOW CAN BE EDITED AS NEEDED ######
###################################################

kInstancesDataFileName = "instances"
kPrefsFileName =  "MarkFeatureGenerator.prefs"

###################################################

__copyright__ = __license__ =  """
Copyright (c) 2010-2013 Adobe Systems Incorporated. All rights reserved.
 
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
Mark Feature Generator v1.3.2 - Mar 10 2013

This script will generate a set of "features.mark" files from the mark data (anchors
and combining marks class) of a Multiple Master (MM) FontLab font, or one "features.mark" 
file if the font is Single Master (SM). The "features.mark" file is a text file containing 
the font's mark attachment data in features-file definition syntax.

The script can also generate "feature.mkmk" file(s).

For information about how the "features.mark/mkmk" files are created, please read the 
documentation in the WriteFeaturesMarkFDK.py module that can be found 
in FontLab/Studio5/Macros/System/Modules/

For information on how to format the "instances" file, please read the documentation in the 
InstanceGenerator.py script.

To access the script's options, hold down the CONTROL key while clicking on the play
button to run the script.

VERY IMPORTANT: For this script to work, all the combining mark glyphs 
must be added to an OpenType class named 'COMBINING_MARKS'.

If the font has ligatures and the ligatures have anchors, the ligature glyphs have to be put in
OpenType classes named "LIGATURES_WITH_X_COMPONENTS", where 'X' should be replaced by a number
between 2 and 9 (inclusive). Additionally, the names of the anchors used on the ligature glyphs 
need to have a tag (e.g. 1ST, 2ND) which is used for corresponding the anchors with the correct
ligature component. Keep in mind that in Right-To-Left scripts, the first component of the ligature
is the one on the right side of the glyph.

The script will first show a file selection dialog for choosing a folder. The "features.mark" 
file(s) will be written to that folder, if the font is SM, or to a sub-directory path 
<selected_folder>/<face_name>, if the font is MM. In this case, the face name is derived by 
taking the part of the font's PostScript name after the hyphen, or "Regular" is there is no 
hyphen. (e.g. if the font's PostScript name is MyFont-BoldItalic, the folder will be named 
"BoldItalic")

If the font is MM, the script requires a file, named "instances", which contains all the 
instance-specific values. The "instances" file must be a simple text file, located in the 
same folder as the MM FontLab file. 

==================================================
Versions:
v1.0   - Feb 15 2010 - Initial release
v1.1   - Feb 19 2010 - Added the option to generate 'mkmk' feature
v1.2   - Apr 21 2011 - Added the option of writing the mark classes in a separate file
v1.3   - Jun 15 2012 - Added the option to output the lookups in the format required for Indian scripts.
v1.3.1 - Jul 19 2012 - Changed the description of one of the options in the UI.
v1.3.2 - Mar 10 2013 - Minor improvements.

"""

import os, sys, re, copy, math, time

try:
	from AdobeFontLabUtils import checkControlKeyPress, checkShiftKeyPress
	import WriteFeaturesMarkFDK
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


def handleInstanceLight(f, fontInstanceDict, instanceInfo):
	# Set names
	f.font_name = fontInstanceDict[kFontName]
	
	instValues = fontInstanceDict[kCoordsKey]
	
	# This name does not go into the CFF font header. It's used in the 'features.kern' to have a record of the instance.
	# Builds information about the source font and instance values
	for x in range(len(instValues)):
		instanceInfo += '_' + str(instValues[x])
	f.menu_name = instanceInfo
	
	return f


def makeFaceFolder(root, folder):
	facePath = os.path.join(root, folder)
	if not os.path.exists(facePath):
		os.makedirs(facePath)
	return facePath


def handleFontLight(folderPath, fontMM, fontInstanceDict, options):
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
	fontInstance = handleInstanceLight(fontInstance, fontInstanceDict, instanceInfo)
	
	instanceFolder = makeFaceFolder(folderPath, faceName)
	
	WriteFeaturesMarkFDK.MarkDataClass(fontInstance, instanceFolder, options.trimCasingTags, options.genMkmkFeature, options.writeClassesFile, options.indianScriptsFormat)


def makeFeature(options):
	try:
		parentDir = os.path.dirname(os.path.abspath(fl.font.file_name))
	except AttributeError:
		print "The font has not been saved. Please save the font and try again."
		return

	if fl.font[0].layers_number == 1:
		fontSM = fl.font # Single Master Font
	else:
		fontMM = fl.font # MM Font
		axisNum = int(math.log(fontMM[0].layers_number, 2)) # Number of axis in font
		
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
	
	folderPath = fl.GetPathName("Select parent directory to output file(s)")

	# Cancel was clicked or Esc key was pressed
	if not folderPath:
		return

	t1 = time.time()  # Initiates a timer of the whole process
	
	if fl.font[0].layers_number == 1:
		print fontSM.font_name
		WriteFeaturesMarkFDK.MarkDataClass(fontSM, folderPath, options.trimCasingTags, options.genMkmkFeature, options.writeClassesFile, options.indianScriptsFormat)
	else:
		for fontInstance in instancesList:
			handleFontLight(folderPath, fontMM, fontInstance, options)

	t2 = time.time()
	elapsedSeconds = t2-t1
	
	if (elapsedSeconds/60) < 1:
		print '\nCompleted in %.1f seconds.\n' % elapsedSeconds
	else:
		print '\nCompleted in %.1f minutes.\n' % (elapsedSeconds/60)


class MarkGenOptions:
	# Holds the options for the module.
	# The values of all member items NOT prefixed with "_" are written to/read from
	# a preferences file.
	# This also gets/sets the same member fields in the passed object.
	def __init__(self):
		self.trimCasingTags = 0
		self.genMkmkFeature = 0
		self.writeClassesFile = 0
		self.indianScriptsFormat = 0
		
		# items not written to prefs
		self._prefsBaseName = kPrefsFileName
		self._prefsPath = None

	def _getPrefs(self, callerObject = None):
		foundPrefsFile = 0

		# We will put the prefs file in a directory "Preferences" at the same level as the Macros directory
		dirPath = os.path.dirname(WriteFeaturesMarkFDK.__file__)
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


class MarkGenDialog:
	def __init__(self):
		""" NOTE: the Get and Save preferences class methods access the preference values as fields
		of the dialog by name. If you want to change a preference value, the dialog control value must have
		the same field name.
		"""
		dWidth = 350
		dMargin = 25
		xMax = dWidth - dMargin

		# Mark Feature Options section
		xC1 = dMargin + 20 # Left indent of the "Generate..." options
		yC0 = 0
		yC1 = yC0 + 30
		yC2 = yC1 + 30
		yC3 = yC2 + 30
		yC4 = yC3 + 30
		endYsection = yC4 + 30
		
		dHeight = endYsection  + 70 # Total height of dialog
		
		self.d = Dialog(self)
		self.d.size = Point(dWidth, dHeight)
		self.d.Center()
		self.d.title = "Mark Feature Generator Preferences"

		self.options = MarkGenOptions()
		self.options._getPrefs(self) # This both loads prefs and assigns the member fields of the dialog.
		
		self.d.AddControl(CHECKBOXCONTROL, Rect(xC1, yC1, xMax, aAUTO), "genMkmkFeature", STYLE_CHECKBOX, " Write mark-to-mark lookups")
		self.d.AddControl(CHECKBOXCONTROL, Rect(xC1, yC2, xMax, aAUTO), "trimCasingTags", STYLE_CHECKBOX, " Trim casing tags on anchor names")
		self.d.AddControl(CHECKBOXCONTROL, Rect(xC1, yC3, xMax, aAUTO), "writeClassesFile", STYLE_CHECKBOX, " Write mark classes in separate file")
		self.d.AddControl(CHECKBOXCONTROL, Rect(xC1, yC4, xMax, aAUTO), "indianScriptsFormat", STYLE_CHECKBOX, " Format the output for Indian scripts")

	def on_genMkmkFeature(self, code):
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

	else:
		dontShowDialog = 1
		result = 2
		dontShowDialog = checkControlKeyPress()
		debug = not checkShiftKeyPress()
		if dontShowDialog:
			print "Hold down CONTROL key while starting this script in order to set options.\n"
			options = MarkGenOptions()
			options._getPrefs() # load current settings from prefs
			makeFeature(options)
		else:
			IGd = MarkGenDialog()
			result = IGd.Run() # returns 0 for cancel, 1 for ok
			if result == 1:
				options = MarkGenOptions()
				options._getPrefs() # load current settings from prefs
				makeFeature(options)


if __name__ == "__main__":
	run()
