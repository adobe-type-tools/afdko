# Module used by InstanceGenerator and KernFeatureGenerator

###################################################
### THE VALUES BELOW CAN BE EDITED AS NEEDED ######
###################################################

kMarkFeatureFileName = "features.mark"
kMkmkFeatureFileName = "features.mkmk"
kMarkClassesFileName = "features.markclasses"
kAbvmFeatureFileName = "features.abvm"
kBlwmFeatureFileName = "features.blwm"
kCombMarksClassName = "COMBINING_MARKS"
kLigaturesClassName = "LIGATURES_WITH_%d_COMPONENTS" # The '%d' part is required
kDefaultGenMkmkFeature = False
kDefaultTrimCasingTags = False
kDefaultWriteMarkClassesFile = False
kDefaultIndianScriptsFormat = False
kCasingTagsList = ['LC','UC','SC','AC'] # All the tags must have the same number of characters, and that number must be equal to kCasingTagSize
kCasingTagSize = 2
kRTLtagsList = ['AR','HE'] # Arabic, Hebrew
kIgnoreAnchorTag = "CXT"
kLigatureComponentOrderTags = ['1ST','2ND','3RD','4TH'] # Add more as necessary to a maximum of 9 (nine)
kIndianAboveMarks = "abvm"
kIndianBelowMarks = "blwm"

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
WriteMarkFeaturesFDK.py v2.0 - Mar 08 2013

Contains a Python class (markDataClass) which, when provided with a FontLab font and a path to a folder, 
will output a file named "features.mark", containing a features-file syntax definition of the font's 
mark attachment and achor data. The script can also generate "feature.mkmk" file. If the Indian scripts
option is selected, all or part of the lookups will be written to files named "feature.abvm/blwm".

VERY IMPORTANT: For this script to work, all the combining mark glyphs 
must be added to an OpenType class named 'COMBINING_MARKS'.

When designing the combining marks, often there's the need to make specific versions for uppercase
and small cap glyphs, which are slightly different from the lowercase. For this reason, one might
want to use a casing tag in the anchors' names (e.g. "_aboveLC", "_aboveUC" and "_aboveSC" instead
of just "_above" for all the three cases) so that the position of the anchors can be tested in
FontLab. But this distinction is not necessary for the 'mark' feature, and that is why this script 
allows for those casing tags to be trimmed, by setting the value of 'kDefaultTrimCasingTags'.

If the font has ligatures and the ligatures have anchors, the ligature glyphs have to be put in
OpenType classes named "LIGATURES_WITH_X_COMPONENTS", where 'X' should be replaced by a number
between 2 and 9 (inclusive). Additionally, the names of the anchors used on the ligature glyphs 
need to have a tag (e.g. 1ST, 2ND) which is used for corresponding the anchors with the correct
ligature component. Keep in mind that in Right-To-Left scripts, the first component of the ligature
is the one on the right side of the glyph.

The lookups will have a RightToLeft lookupflag if the name of the anchors contains one of the tags
listed in the kRTLtagsList list.

The anchors that are specific to Indian scripts much be named "abvm" (for Above Marks) or "blwm"
(for Below Marks), and the Indian scripts option needs to be checked in the UI.

==================================================
Versions:
v1.0   - Feb 15 2010 - Initial release
v1.1   - Feb 19 2010 - Added the option to generate 'mkmk' feature
v1.2   - Aug 10 2010 - Added support for ligatures
v1.3   - Apr 21 2011 - Added the ability to ignore certain base anchors
                       Added the option of writing the mark classes in a separate file
v1.4   - Jul 11 2011 - Overhauled the way mark-to-base and mark-to-ligature positions are output; they're now written in separate lookup blocks.
                       Lookup flag "RightToLeft" is now written for Arabic and Hebrew lookups.
v1.4.1 - Aug 10 2011 - The contents of the combining marks class and the ligatures classes are now verified for the presence of duplicates.
v1.4.2 - Mar 03 2012 - Fixed bug in output of mkmk feature.
v1.4.3 - Mar 04 2012 - Added checks for nameless anchors.
v1.4.4 - Jun 14 2012 - Added checks to avoid anchors with the same name.
v1.5   - Jun 15 2012 - Added the option to output the lookups in the format required for Indian scripts.
v1.6   - Jul 19 2012 - The MarkAttachmentType lookupflag is no longer added to mark-to-mark lookups meant for the 'abvm' and 'blwm' features.
v2.0   - Mar 08 2013 - Now compatible with UFOs -- this means the module can be used from Fontlab, from the commandline or from RoboFont.

"""

import os, time, re


class WhichApp(object):
	'Testing the environment'
	
	def __init__(self):
		self.inRF = False
		self.inFL = False
		self.inDC = False
		self.appName = "noApp"
		
		if not any((self.inRF, self.inFL, self.inDC)):
			try:
				import RoboFont
				# print 'in Robofont'
				self.inRF = True
				self.appName = 'Robofont'
			except ImportError:
				pass

		if not any((self.inRF, self.inFL, self.inDC)):
			try:
				import flsys
				# print 'In FontLab, dork!'
				self.inFL = True
				self.appName = 'FontLab'
			except ImportError:
				pass

		if not any((self.inRF, self.inFL, self.inDC)):
			try:
				import defcon
				# print 'defcon'
				self.inDC = True
				self.appName = 'Defcon'
			except ImportError:
				pass


class MarkDataClass(object):
	
	def __init__(self, font, folderPath, trimCasingTags=kDefaultTrimCasingTags, genMkmkFeature=kDefaultGenMkmkFeature, writeClassesFile=kDefaultWriteMarkClassesFile, indianScriptsFormat=kDefaultIndianScriptsFormat):
		self.header = ['# Created: %s' % time.ctime()]

		appTest = WhichApp()
		self.inRF = appTest.inRF
		self.inFL = appTest.inFL
		self.inDC = appTest.inDC
		self.appName = appTest.appName

		self.f = font
		self.MM = False
		self.folder = folderPath
		self.trimCasingTags = trimCasingTags
		self.genMkmkFeature = genMkmkFeature
		self.writeClassesFile = writeClassesFile
		self.indianScriptsFormat = indianScriptsFormat
		
		self.lineBreak = '\n'
		self.marksClassList = []
		self.ligatureComponentsList = range(2,len(kLigatureComponentOrderTags)+1) # ligatures have a minimum of 2 components and a maximum determined by the size of kLigatureComponentOrderTags
		self.ligatureClassesDict = {}
		self.allLigaturesList = []
		self.invalidGlyphNamesList = []
		self.repeatedGlyphNamesList = []
		self.markTypesList = []
		
		self.anchorsDataInCombMarksDict = {}
		self.anchorsDataInBaseGlyphsDict = {}
		self.anchorsDataInLigaturesDict = {}
		self.markRelatedGlyphClassLinesList = []
		self.markRelatedMarkClassLinesList = []
		self.baseRelatedBasePosLinesList = []
		self.ligatureRelatedBasePosLinesList = []

		self.anchorsDataInCombMkmksDict = {} # {'below': ['gravebelowcmb,0,-246', 'acutebelowcmb,0,-246'], 'above': ['gravecmb,0,713', 'acutecmb,0,713']}
		self.mkmkRelatedBasePosLinesList = []
		self.classesNote = ''

		if self.inFL:
			self.header.append('# PS Name: %s' % self.f.font_name)
			self.isMMfont(self.f) # sets self.MM to True or False
			
			flC = ReadFontLabClasses(self.f)
			self.marksClassList = flC.marksClassList
			self.ligatureClassesDict = flC.ligatureClassesDict
			
			if not self.MM:
				self.header.append('# MM Inst: %s' % self.f.menu_name)

		else:
			self.header.append('# PS Name: %s' % self.f.info.postscriptFontName)
			self.header.append('# MM Inst: %s' % self.f.info.styleMapFamilyName)

			if self.f.groups.has_key(kCombMarksClassName):
				self.marksClassList = self.f.groups[kCombMarksClassName]
			for compNum in self.ligatureComponentsList:
				if self.f.groups.has_key(kLigaturesClassName % compNum):
		 			self.ligatureClassesDict[compNum] = self.f.groups[kLigaturesClassName % compNum]

		self.header.append('# exported from %s\n\n' % self.appName)
		
		if not len(self.marksClassList):
			print "\tERROR: %s class could not be found or is empty!" % kCombMarksClassName
			return

		self.validateCombMarksClassContents()
		self.validateLigatureClassesContents()
		if len(self.invalidGlyphNamesList):
			print "\tERROR: The glyph(s) listed below could not be found! Please check the combining marks (and ligatures) classes.\n\t%s\n" % ("\n\t".join(self.invalidGlyphNamesList))
			return
		
		if len(self.repeatedGlyphNamesList):
			# the error messages have already been printed
			return
		
		if len(self.ligatureClassesDict):
			self.addAllLigaturesToList()
		
		self.collectAnchorDataFromCombMarks()
		if not len(self.anchorsDataInCombMarksDict):
			print "\tERROR: No valid anchors were found in combining mark glyphs!"
			return
		
		self.buildMarkRelatedLines() # builds the markClass lines and the glyph classes lines
		
		self.markRelatedGlyphClassLinesList.sort()
		self.markRelatedMarkClassLinesList.sort()
		
		self.collectMarkTypesList()
		
		self.collectAnchorDataFromBaseGlyphs()
		if not len(self.anchorsDataInBaseGlyphsDict):
			print "\nWARNING: No valid anchors were found in base glyphs!"
			print
		
		if len(self.ligatureClassesDict):
			self.collectAnchorDataFromLigatureGlyphs()

		self.buildMarkLookups()
		
		if self.writeClassesFile:
			self.writeMarkClassesFile()
		
		self.buildClassesNote()
		self.writeMarkFeatureFile()
		
		if self.genMkmkFeature:
			self.collectAnchorDataFromCombMkmk()
			self.buildMkmkRelatedLines()
			self.writeMkmkFeatureFile()


	def isMMfont(self, font):
		'Checks if the FontLab font is a Multiple Master font.'
		if font[0].layers_number > 1:
			self.MM = True


	def glyphFoundInFont(self, gName):
		if self.inFL:
			if self.f.FindGlyph(gName) != -1:
				return True
		else:
			if gName in self.f.keys():
				return True
		return False
	
	
	def validateCombMarksClassContents(self):
		tempGlist = []
		for gName in self.marksClassList:
			if not self.glyphFoundInFont(gName):
				self.invalidGlyphNamesList.append(gName)
			if gName not in tempGlist:
				tempGlist.append(gName)
			else:	
				self.repeatedGlyphNamesList.append(gName)
				print "ERROR: The glyph named %s is repeated in the class named %s" % (gName, kCombMarksClassName)


	def validateLigatureClassesContents(self):
		tempAllGlist = []
		for element in self.ligatureClassesDict:
			tempGlist = []
			for gName in self.ligatureClassesDict[element]:
				if not self.glyphFoundInFont(gName):
					self.invalidGlyphNamesList.append(gName)
				if gName not in tempGlist:
					tempGlist.append(gName)
					if gName not in tempAllGlist:
						tempAllGlist.append(gName)
					else:	
						self.repeatedGlyphNamesList.append(gName)
						print "ERROR: The glyph named %s is used in more than one ligature class." % (gName)
				else:	
					self.repeatedGlyphNamesList.append(gName)
					print "ERROR: The glyph named %s is repeated in the class named %s" % (gName, kLigaturesClassName % element)


	def addAllLigaturesToList(self):
		for element in self.ligatureClassesDict:
			self.allLigaturesList.extend(self.ligatureClassesDict[element])


	def addAnchorDataToDict(self, anchor, gName, dict):
		if anchor in dict:
			gNameList = dict[anchor]
			gNameList.append(gName)
			dict[anchor] = gNameList
		else:
			dict[anchor] = [gName]


	def trimCaseTags(self, anchorName):
		if len(anchorName) >= kCasingTagSize + 3: # +3 to guarantee that the name is long enough and will remain long enough after being trimmed
			if anchorName[-kCasingTagSize:] in kCasingTagsList:
				return anchorName[:-kCasingTagSize]
		print "NOTE: The name of this anchor was not trimmed: %s\n" % anchorName
		return anchorName


	def collectMarkTypesList(self):
		uniqueMarksList = self.anchorsDataInCombMarksDict.keys()
		for mark in uniqueMarksList:
			markName = mark.split(',')[0][1:]  # ['_above,0,694', '_centerAR,0,0', '_above,0,519'] Removes the initial underscore as well
			if markName not in self.markTypesList:
				self.markTypesList.append(markName)
		self.markTypesList.sort()
		
	
	def collectAnchorDataFromCombMarks(self):
		for gName in self.marksClassList:
			g = self.f[gName]
			markAnchorsNames = []
			for anchorIndex in range(len(g.anchors)):
				anchorName = g.anchors[anchorIndex].name
				if len(anchorName) == 0:
					print "ERROR: Glyph %s has a nameless anchor." % gName
					continue
				if anchorName[0] == '_': # Consider only mark-related anchors (Anchors without undercore are for use in 'mkmk' feature)
					if anchorName in markAnchorsNames:
						print "ERROR: Glyph %s has more than one anchor named %s." % (gName, anchorName)
					else:
						markAnchorsNames.append(anchorName)
						point = g.anchors[anchorIndex]
						if self.trimCasingTags:
							anchorName = self.trimCaseTags(anchorName)
						anchor = "%s,%d,%d" % (anchorName, point.x, point.y)
						self.addAnchorDataToDict(anchor, gName, self.anchorsDataInCombMarksDict)


	def collectAnchorDataFromBaseGlyphs(self):
		if self.inFL:
			glyphs = self.f.glyphs
		else:
			glyphs = []
			glyphOrder = self.f.lib["public.glyphOrder"] # self.f.keys() can't be used because its order is not stable
			for name in glyphOrder:
				glyphs.append(self.f[name])
		for g in glyphs:
			gName = g.name
			if gName not in (self.marksClassList + self.allLigaturesList):
				markNamesLog = []
				for anchorIndex in range(len(g.anchors)):
					anchorName = g.anchors[anchorIndex].name
					if len(anchorName) == 0:
						print "ERROR: Glyph %s has a nameless anchor." % gName
						continue
					if (anchorName[0] != '_') and (kIgnoreAnchorTag not in anchorName): # Consider only base-related anchors (the ones NOT prefixed with the underscore)
						if anchorName in markNamesLog:
							print "ERROR: Glyph %s has more than one anchor named %s." % (gName, anchorName)
						else:
							markNamesLog.append(anchorName)
							point = g.anchors[anchorIndex]
							if self.trimCasingTags:
								anchorName = self.trimCaseTags(anchorName)
							anchor = "%s,%d,%d" % (anchorName, point.x, point.y)
							self.addAnchorDataToDict(anchor, gName, self.anchorsDataInBaseGlyphsDict)


	def collectAnchorDataFromLigatureGlyphs(self):
		for elementNum in self.ligatureClassesDict:
			for gName in self.ligatureClassesDict[elementNum]:
				g = self.f[gName]
				markNamesLog = []
				for anchorIndex in range(len(g.anchors)):
					anchorName = g.anchors[anchorIndex].name
					if len(anchorName) == 0:
						print "ERROR: Glyph %s has a nameless anchor." % gName
						continue
					if (anchorName[0] != '_') and (kIgnoreAnchorTag not in anchorName): # Consider only base-related anchors (the ones NOT prefixed with the underscore)
						if anchorName in markNamesLog:
							print "ERROR: Glyph %s has more than one anchor named %s." % (gName, anchorName)
						else:
							markNamesLog.append(anchorName)
							point = g.anchors[anchorIndex]
							if self.trimCasingTags:
								anchorName = self.trimCaseTags(anchorName)
							
							# Arrange the data before adding it to the dictionary
							for i in range(elementNum):
								elementOrder = kLigatureComponentOrderTags[i]
								if elementOrder in anchorName:
									strippedAnchorName = re.sub(elementOrder, '', anchorName) # remove the component order tag using regexp
									if strippedAnchorName[-1] == "_":
										strippedAnchorName = strippedAnchorName[:-1] # remove the last character of the anchor's name, if it happens to be an underscore
	
									dictKey = "%s,%s" % (strippedAnchorName, gName)
									anchor = "%s,%d,%d" % (elementOrder, point.x, point.y)
									
									if dictKey not in self.anchorsDataInLigaturesDict:
										self.anchorsDataInLigaturesDict[dictKey] = [elementNum, anchor]
									else:
										self.anchorsDataInLigaturesDict[dictKey].append(anchor)
									
									break  # no need to finish the loop since a match was found
						

	def buildMarkRelatedLines(self):
		anchorGroupList = self.anchorsDataInCombMarksDict.keys()
		anchorGroupList.sort()
		for anchor in anchorGroupList:
			anchorName, anchorX, anchorY = anchor.split(',')
			markClassName = "@MC%s" % anchorName
			if len(self.anchorsDataInCombMarksDict[anchor]) > 1: # make a glyph class only when there's more than one glyph
				glyphClassName = "@mGC%s_%s_%s" % (anchorName, anchorX.replace('-','n'), anchorY.replace('-','n')) # Coordinates may be negative, and hyphens are not allowed in class names
				glyphClassContents = ' '.join(self.anchorsDataInCombMarksDict[anchor])
				glyphClassLine = "%s = [%s];\n" % (glyphClassName, glyphClassContents)
				self.markRelatedGlyphClassLinesList.append(glyphClassLine)
				#
				markClassLine = "markClass %s <anchor %s %s> %s;\n" % (glyphClassName, anchorX, anchorY, markClassName)
				self.markRelatedMarkClassLinesList.append(markClassLine)
			else:
				markClassLine = "markClass %s <anchor %s %s> %s;\n" % (self.anchorsDataInCombMarksDict[anchor][0], anchorX, anchorY, markClassName)
				self.markRelatedMarkClassLinesList.append(markClassLine)


	def buildMarkLookups(self):
		for markType in self.markTypesList:
			baseLinesList = self.buildBaseRelatedLines(markType)
			
			# Do base lookups
			if len(baseLinesList):
				self.baseRelatedBasePosLinesList.append('lookup MARK_BASE_%s {\n' % markType)
				if markType[-2:] in kRTLtagsList:
					self.baseRelatedBasePosLinesList.append('\tlookupflag RightToLeft;\n\n')
				self.baseRelatedBasePosLinesList.extend(baseLinesList)
				self.baseRelatedBasePosLinesList.append('} MARK_BASE_%s;\n\n\n' % markType)
			else:
				print "WARNING: The anchor %s is not used in any of the base glyphs." % markType
	
			# Do ligature lookups
			if len(self.anchorsDataInLigaturesDict):
				ligatureLinesList = self.buildLigatureRelatedLines(markType)
				
				if len(ligatureLinesList):
					self.ligatureRelatedBasePosLinesList.append('lookup MARK_LIGATURE_%s {\n' % markType)
					if markType[-2:] in kRTLtagsList:
						self.ligatureRelatedBasePosLinesList.append('\tlookupflag RightToLeft;\n\n')
					self.ligatureRelatedBasePosLinesList.extend(ligatureLinesList)
					self.ligatureRelatedBasePosLinesList.append('} MARK_LIGATURE_%s;\n\n\n' % markType)
				else:
					print "WARNING: The anchor %s is not used in any of the ligature glyphs." % markType

	
	def buildBaseRelatedLines(self, markTypeName):
		anchorGroupList = self.anchorsDataInBaseGlyphsDict.keys() # ['aboveAR,1675,697', 'below,367,-30', ... ]
		anchorGroupList.sort()
		glyphClassLinesList = []
		basePosLinesList = []
		
		for anchor in anchorGroupList:
			anchorName, anchorX, anchorY = anchor.split(',')
			
			if anchorName != markTypeName: # skip anchors that are not the same kind of the one requested
				continue
			
			anchorAndMarkClass = "<anchor %s %s> mark @MC_%s" % (anchorX, anchorY, anchorName)
			
			if len(self.anchorsDataInBaseGlyphsDict[anchor]) > 1: # make a glyph class only when there's more than one glyph
				glyphClassName = "@bGC_%s_%s" % (self.anchorsDataInBaseGlyphsDict[anchor][0], anchorName)
				glyphClassContents = ' '.join(self.anchorsDataInBaseGlyphsDict[anchor])
				glyphClassLine = "\t%s = [%s];\n" % (glyphClassName, glyphClassContents)
				glyphClassLinesList.append(glyphClassLine)
				#
				basePosLine = "\tpos base %s %s;\n" % (glyphClassName, anchorAndMarkClass)
				basePosLinesList.append(basePosLine)
			else:
				basePosLine = "\tpos base %s %s;\n" % (self.anchorsDataInBaseGlyphsDict[anchor][0], anchorAndMarkClass)
				basePosLinesList.append(basePosLine)

		glyphClassLinesList.sort()
		basePosLinesList.sort()
		
		return glyphClassLinesList + basePosLinesList


	def buildLigatureRelatedLines(self, markTypeName):
		anchorGroupList = self.anchorsDataInLigaturesDict.keys() # ['aboveAR,arAlefMaksura_AlefMaksura', 'aboveAR,arAlefMaksura_AlefMaksura.f', 'aboveAR,arAlefMaksura_AlefMaksura.fj',
		anchorGroupList.sort()
		ligaturePosLinesList = []
		
		for anchor in anchorGroupList:
			anchorName, gName = anchor.split(',')
			
			if anchorName != markTypeName: # skip anchors that are not the same kind of the one requested
				continue
			
			elementCount = int(self.anchorsDataInLigaturesDict[anchor][0])
			anchorPositionsList = self.anchorsDataInLigaturesDict[anchor][1:]  # ['1ST,1735,358', '2ND,680,234']
			anchorPositionsList.sort()
			
			if elementCount != len(anchorPositionsList):
				print "NOT IMPLEMENTED WARNING: Number of elements in ligature %s does not match the number of anchors of the type %s." % (gName, anchorName)
				continue
			
			elementsAnchorsAndMarkClassesList = []
			
			for position in anchorPositionsList:
				ord, anchorX, anchorY = position.split(',')
				anchorAndMarkClass = "<anchor %s %s> mark @MC_%s" % (anchorX, anchorY, anchorName)
				elementsAnchorsAndMarkClassesList.append(anchorAndMarkClass)
			
			ligaturePosLine = "\tpos ligature %s %s;\n" % (gName, ' ligComponent '.join(elementsAnchorsAndMarkClassesList))
			ligaturePosLinesList.append(ligaturePosLine)
		
		return ligaturePosLinesList


	def buildClassesNote(self):
		if self.writeClassesFile:
			classesInfoFile = kMarkClassesFileName
		else:
			classesInfoFile = kMarkFeatureFileName
		self.classesNote = "# NOTE: The markClass declarations can be found in the file '%s'.\n\n" % classesInfoFile


	def assembleClassesLines(self):
		classesLinesList = []
		if len(self.markRelatedGlyphClassLinesList):
			classesLinesList.extend(self.markRelatedGlyphClassLinesList)
			classesLinesList.append(self.lineBreak)
		if len(self.markRelatedMarkClassLinesList):
			classesLinesList.extend(self.markRelatedMarkClassLinesList)
			classesLinesList.append(self.lineBreak)
		return classesLinesList

	
	def writeMarkClassesFile(self):
		print '\tSaving %s file...' % kMarkClassesFileName
		filePath = os.path.join(self.folder, kMarkClassesFileName)
		outfile = open(filePath, 'w')
		outfile.write("\n".join(self.header))
		outfile.writelines(self.assembleClassesLines())
		outfile.close()


	def writeMarkFeatureFile(self):
		print '\tSaving %s file...' % kMarkFeatureFileName
		filePath = os.path.join(self.folder, kMarkFeatureFileName)
		outfile = open(filePath, 'w')
		outfile.write("\n".join(self.header))
		if self.writeClassesFile:
			outfile.write(self.classesNote)
		else:
			outfile.writelines(self.assembleClassesLines())
		
		fileHeader = "\n".join(self.header) + self.classesNote

		if self.indianScriptsFormat:
			print '\tSaving %s and %s files...' % (kAbvmFeatureFileName, kBlwmFeatureFileName)
			abvmFilePath = os.path.join(self.folder, kAbvmFeatureFileName)
			blwmFilePath = os.path.join(self.folder, kBlwmFeatureFileName)
			abvmfile = open(abvmFilePath, 'w')
			blwmfile = open(blwmFilePath, 'w')
			abvmfile.write(fileHeader)
			blwmfile.write(fileHeader)
			
			# pos base lookups
			for line in self.baseRelatedBasePosLinesList: # the lines will have to be analized and written to separate files
				if kIndianAboveMarks in line:
					abvmfile.write(line)
				elif kIndianBelowMarks in line:
					blwmfile.write(line)
				else:
					outfile.write(line)
						
			outfile.write(self.lineBreak)

			# pos ligature lookups
			for line in self.ligatureRelatedBasePosLinesList: # the lines will have to be analized and written to separate files
				if kIndianAboveMarks in line:
					abvmfile.write(line)
				elif kIndianBelowMarks in line:
					blwmfile.write(line)
				else:
					outfile.write(line)
						
			abvmfile.close()
			blwmfile.close()

			if not self.genMkmkFeature: # don't try to delete the files if mark-to-mark lookups are still to be processed
				self.checkFileContentsAndDeleteIfEmpty(abvmFilePath, fileHeader)
				self.checkFileContentsAndDeleteIfEmpty(blwmFilePath, fileHeader)

		else: # the Indian scripts option is not checked; all the lines can be written to the file at once
			if len(self.baseRelatedBasePosLinesList):
				outfile.writelines(self.baseRelatedBasePosLinesList)
				outfile.write(self.lineBreak)
			if len(self.ligatureRelatedBasePosLinesList):
				outfile.writelines(self.ligatureRelatedBasePosLinesList)

		outfile.close()

		self.checkFileContentsAndDeleteIfEmpty(filePath, fileHeader)


	def collectAnchorDataFromCombMkmk(self):
		for gName in self.marksClassList:
			g = self.f[gName]
			markAnchorsNames = []
			for anchorIndex in range(len(g.anchors)):
				anchorName = g.anchors[anchorIndex].name
				if len(anchorName) == 0:
					print "ERROR: Glyph %s has a nameless anchor." % gName
					continue
				if anchorName[0] != '_': # Anchors without undercore are for use in 'mkmk' feature
					if anchorName in markAnchorsNames:
						print "ERROR: Glyph %s has more than one anchor named %s." % (gName, anchorName)
					else:
						markAnchorsNames.append(anchorName)
						point = g.anchors[anchorIndex]
						if self.trimCasingTags:
							anchorName = self.trimCaseTags(anchorName)
						gAnchor = "%s,%d,%d" % (gName, point.x, point.y)
						self.addAnchorDataToDict(anchorName, gAnchor, self.anchorsDataInCombMkmksDict)


	def buildMkmkRelatedLines(self):
		anchorGroupList = self.anchorsDataInCombMkmksDict.keys()
		anchorGroupList.sort()
		for anchorName in anchorGroupList:
			if anchorName[-2:] in kRTLtagsList: rtlFlag = 'RightToLeft ' # Check the last two characters of the anchor's name
			else: rtlFlag = ''
			lookupName = "MKMK_MARK_%s" % anchorName
			markClassName = "@MC_%s" % anchorName
			self.mkmkRelatedBasePosLinesList.append("lookup %s {\n" % lookupName)
			if (not (kIndianAboveMarks in markClassName or kIndianBelowMarks in markClassName)) or (not self.indianScriptsFormat): # skip the lookupflag for Indian script lookups but only when the output format is for Indian scripts as well
				self.mkmkRelatedBasePosLinesList.append("\tlookupflag %sMarkAttachmentType %s;\n\n" % (rtlFlag, markClassName))
			mkmkAnchorsList = self.anchorsDataInCombMkmksDict[anchorName]
			mkmkAnchorsList.sort()
			for mkmkAnchor in mkmkAnchorsList:
				glyphName, anchorX, anchorY = mkmkAnchor.split(',')
				mkmkLine = "\tpos mark %s <anchor %s %s> mark %s;\n" % (glyphName, anchorX, anchorY, markClassName)
				self.mkmkRelatedBasePosLinesList.append(mkmkLine)
			self.mkmkRelatedBasePosLinesList.append("} %s;\n\n\n" % lookupName)


	def writeMkmkFeatureFile(self):
		print '\tSaving %s file...' % kMkmkFeatureFileName
		filePath = os.path.join(self.folder, kMkmkFeatureFileName)
		outfile = open(filePath, 'w')
		outfile.write("\n".join(self.header))
		outfile.write(self.classesNote)
		fileHeader = "\n".join(self.header) + self.classesNote
		
		if self.indianScriptsFormat:
			print '\tAdding mark-to-mark lookups to %s and %s files...' % (kAbvmFeatureFileName, kBlwmFeatureFileName)
			abvmFilePath = os.path.join(self.folder, kAbvmFeatureFileName)
			blwmFilePath = os.path.join(self.folder, kBlwmFeatureFileName)
			abvmfile = open(abvmFilePath, 'a') # append the data instead of writting a new file; the file is expected to contain lookups for 'pos base' already
			blwmfile = open(blwmFilePath, 'a')
			
			for line in self.mkmkRelatedBasePosLinesList: # the lines will have to be analized and written to separate files
				if kIndianAboveMarks in line:
					abvmfile.write(line)
				elif kIndianBelowMarks in line:
					blwmfile.write(line)
				else:
					outfile.write(line)
						
			abvmfile.close()
			blwmfile.close()

			self.checkFileContentsAndDeleteIfEmpty(abvmFilePath, fileHeader)
			self.checkFileContentsAndDeleteIfEmpty(blwmFilePath, fileHeader)

		else: # the Indian scripts option is not checked; all the lines can be written to the file at once
			if len(self.mkmkRelatedBasePosLinesList):
				outfile.writelines(self.mkmkRelatedBasePosLinesList)
		
		outfile.close()
		
		self.checkFileContentsAndDeleteIfEmpty(filePath, fileHeader)


	def checkFileContentsAndDeleteIfEmpty(self, filePath, fileHeader):
		file = open(filePath, 'r')
		fileContents = file.read()
		file.close()
		fileContents = fileContents.strip()
		fileHeader = fileHeader.strip()
		
		if fileContents == fileHeader:
			print "\tFile %s is empty. Deleted." % os.path.basename(filePath)
			if os.path.exists(filePath):
				os.remove(filePath)
	


class ReadFontLabClasses(object):
	def __init__(self, font):
		self.marksClassList = []
		self.ligatureClassesDict = {}
		
		for c in font.classes:
			if c[0] not in ['_','.']: # Ignore kerning and metrics classes
				classNameAndContentsList = c.split(':') # Split class name and class contents
				if len(classNameAndContentsList) == 2: # Make sure there are only two elements, the class name and the class contents
		
					if classNameAndContentsList[0] == kCombMarksClassName: # Find class by matching the name
						self.marksClassList = classNameAndContentsList[1].split() # Return the class contents as a list
						continue

					for compNum in self.ligatureComponentsList:
						if classNameAndContentsList[0] == (kLigaturesClassName % compNum): # Find class by matching the name
							self.ligatureClassesDict[compNum] = classNameAndContentsList[1].split() # Return the class contents as a list


