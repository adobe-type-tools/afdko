# Module used by InstanceGenerator and KernFeatureGenerator

###################################################
### THE VALUES BELOW CAN BE EDITED AS NEEDED ######
###################################################

kKernFeatureFileBaseName = "features"
kDefaultMinKern = 3  #inclusive; this means that pairs which EQUAL this ABSOLUTE value will NOT be ignored/trimmed. Anything below WILL.
kDefaultWriteTrimmed = False  #if 'False', trimmed pairs will not be processed and, therefore, will not be written to the 'features.kern' file.
					  #for a different default behavior change the value to 'True'.
kDefaultWriteSubtables = True

kLeftTag = ['_LEFT','_1ST']
kRightTag = ['_RIGHT','_2ND']

kLatinTag = '_LAT'
kGreekTag = '_GRK'
kCyrillicTag = '_CYR'
kArabicTag = '_ARA'
kHebrewTag = '_HEB'


kNumberTag = '_NUM'
kFractionTag = '_FRAC'

kExceptionTag = 'EXC_'

kIgnorePairTag = '.cxt'
	
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
WriteKernFeaturesFDK.py v3.2.1 - Mar 08 2013

Contains a class (KernDataClass) which, when provided with a FontLab font and a path to a folder, will output 
a file named "features.kern", containing a features-file syntax definition of the font's kerning.

This script allows the user to customize the minimum kerning value, i.e. the kerning threshold. If the "kDefaultMinKern"
value is set to 5 (five), kern pair values of -4, -3, -2, -1, 0, 1, 2, 3 and 4 will be ignored. To include the
ignored pairs in the 'features.kern' file, change the value of "kDefaultWriteTrimmed" to 'True'. Keep in mind that the
trimmed pairs will appear in the 'features.kern' file, but they won't be implemented in the font, because
the lines will be preceded by a number sign (#).

VERY IMPORTANT: For the creation of the 'features.kern' file to work well, the following class naming rules 
should be followed:

	Left side classes must contain the string "_LEFT"
	Right side classes must contain the string "_RIGHT"
	Classes to be used on both left and right sides of a kern pair must NOT contain "_LEFT" or "_RIGHT"
	
	Latin glyph classes should contain the string "_LAT"
	Greek glyph classes must contain the string "_GRK"
	Cyrillic glyph classes must contain the string "_CYR"
	
	Exception classes must start with the string "EXC_" and the key glyph of the class (marked by a single 
		quote [']) can NOT be the same as the key glyph of another kerning class.
	
	Examples of kerning classes (in FontLab syntax):
		_A_LC_RIGHT: a' a.end aacute acircumflex adieresis agrave ae
		_T_UC: T' Tcaron Tcommaaccent Tcedilla
		_K_UC_LEFT_LAT: K' K.alt Kcommaaccent
		_E_SC_LEFT_LAT: e.sc' E.sc aeacute.sc AEacute.sc ebreve.sc Ebreve.sc
		_GHE_UC_LEFT_CYR: Ghe' Ghe.up Gje
		_EPSILON_LC_GRK: epsilon' epsilonacute epsilonasper epsilonasperacute
		_EXC_E_LC_LEFT_LAT: edieresis' egrave ebreve ecaron emacron etilde

There is no absolute requirement to use any of the strings above, but their usage will facilitate the creation 
of a correct "features.kern" file, which will prevent kerning class overlapping and subtable overflow.

The default strings values (_LEFT, _LAT, etc.) can be replaced for other values as needed, by editing the 
entries located at the top of this file.

If a script tag (_LAT, _GRK or _CYR) is used in one kerning class, all kerning classes related with 
that script MUST use the tag string as well.

Classes that do not contain a side tag (_LEFT or _RIGHT), will be used on both sides of a kerning pair. Classes
containing only symmetrical glyphs, normally do not require a side tag. Nonetheless, adding side tags is a good 
practice, as it can prevent overlap of classes.

It is possible to do multiple kerning exceptions at once, by grouping all the exception glyphs in the same 
class, and making a kerning exception with the key glyph of the class. However, the key glyph of an exception
class can NOT be the key glyph of another class at the same time.

In order to process all RTL kerning pairs as such, all the RTL glyphs need to be included in kerning classes 
tagged as RTL (via the usage of _ARA or _HEB in the classes' names). A RTL kerning class needs to be created
even if its content is one single glyph.

==================================================
Versions:
v1.0   - Apr 24 2007 - Initial release
v1.1   - Dec 03 2007 - Robofab dependency removed (code changes suggested by Karsten Luecke)
v2.0   - Feb 15 2010 - Rewrite to make the module self contained and tidy. Fixed a bug that prevented representative glyphs to be found.
v2.1   - Feb 19 2010 - Added checks to verify the existence of kern pairs and kern classes
v2.2   - Jun 10 2010 - Enabled multiple names for tagging the side of kerning classes. Added support for RTL kerning.
v2.3   - Jan 17 2011 - Revised the support for RTL kerning. Added documentation regarding the setup requirements for RTL kerning.
v2.3.1 - Feb 12 2011 - Fixed a bug related with the handling of RTL kerning class pairs: pairs where only the right class was RTL were being sorted incorrectly.
v2.3.2 - Jun 20 2011 - Added an option to ignore some pairs, based on the name of the left glyph
v2.4   - Oct 01 2012 - Changed the way exceptions are handled, to support single class-to-glyph and glyph-to-class kerns for glyphs that are not in a kerning class to begin with.
v2.5   - Oct 04 2012 - Made MM compatible for the sake of eliminating a separate MM module.
v2.5.1 - Oct 15 2012 - Minor fixes and cleanup.
v3.0   - Oct 24 2012 - Complete re-write. 
					   Now, the kern feature file will identify all levels of exceptions. Single-glyph classes are no longer necessary for the left-to-right part of the kerning.
					   (The single-glyph kerning classes were used for subtable assignment. Later in the process, single-class-to-single-class pairs were dissolved into glyph-to-glyph pairs.
					   Only pairs that have a class as their first element should be in a subtable. For now, only class-to-class pairs are split into subtables.
					   Any glyphs that are not in a kerning class are now assumed to be in LTR kerning.
					   This is an area that might need more work -- the need for creating single-glyph kerning classes may be counter-intuitive; 
					   the subtable problem could also be solved e.g. by adding glyph notes).
v3.1   - Nov 30 2012 - Now compatible with UFOs -- this means the module can be used from Fontlab, from the commandline or from RoboFont.
v3.2   - Jan 24 2013 - Made subtabling optional, changed subtabling behaviour to also include glyph-to-class pairs.
v3.2.1 - Mar 08 2013 - Minor improvements.

"""

import os, time, itertools


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



class ReadFontLabClasses(object):
	def __init__(self, font):
		self.f = font
		self.keyGlyphs = {}
		self.groups = {}
		self.kernClassOrder = []
		
		classes = []
		for c in self.f.classes:
			if c[0] == '_':
				classes.append(c)
		
		sep = ":"
		for c in classes:
			repFound = False

			className      = c.split(sep)[0]			# FL class name, e.g. _L_LC_LEFT
			OTgroupName    = '@%s' % className[1:]		# OT group name, e.g. @L_LC_LEFT
			glyphList      = c.split(sep)[1].split()
			cleanGlyphList = [i.strip("'") for i in glyphList] # strips out the keyglyph marker

			for g in glyphList:
				if g[-1] == "'":  # finds keyglyph
					rep = g.strip("'")  
					repFound = True
					break
				else:
					rep = glyphList[0]

			if repFound == False: 
				print "\tWARNING: Kerning class %s has no explicit key glyph.\n\tUsing first glyph found (%s)." % (c, rep)

			self.kernClassOrder.append(OTgroupName)
			self.keyGlyphs[OTgroupName] = rep
			self.groups[OTgroupName] = cleanGlyphList 



class KernDataClass(object):

	def __init__(self, font, folderPath, minKern=kDefaultMinKern, writeTrimmed=kDefaultWriteTrimmed, writeSubtables=kDefaultWriteSubtables):
		self.header = ['# Created: %s' % time.ctime()]

		appTest = WhichApp()
		self.inRF = appTest.inRF
		self.inFL = appTest.inFL
		self.inDC = appTest.inDC
		self.appName = appTest.appName

		# self.outputFileName = '%s_%s' % (kKernFeatureFileBaseName, self.appName)
		self.outputFileName = kKernFeatureFileBaseName
				
		self.f = font
		self.MM = False

		self.folder = folderPath
		self.minKern = minKern
		self.writeTrimmed = writeTrimmed
		self.writeSubtables = writeSubtables

		self.kerning = {}
		self.groups = {}

		self.keyGlyphs = {} # Dict from keyGlyph to groupName 'r': '_R_LC_LEFT_LAT'
		self.leftKeyGlyphs = {}
		self.rightKeyGlyphs = {}
		
		self.totalKernPairs = 0
		self.trimmedPairs = 0
		self.processedPairs = 0
		self.notProcessed = 0
		
		# kerning lists for pairs only
		self.group_group = []
		self.glyph_glyph = []
		self.glyph_group = []
		self.group_glyph = []

		# kerning subtables containing pair-value combinations
		self.glyph_glyph_dict = {}
		self.glyph_glyph_exceptions_dict = {}
		self.glyph_group_dict = {}
		self.glyph_group_exceptions_dict = {}
		self.group_glyph_dict = {}
		self.group_glyph_exceptions_dict = {}
		self.group_group_dict = {}		
		self.predefined_exceptions_dict = {}
		
		self.RTLglyph_glyph_dict = {}
		self.RTLglyph_glyph_exceptions_dict = {}
		self.RTLglyph_group_dict = {}
		self.RTLglyph_group_exceptions_dict = {}
		self.RTLgroup_glyph_dict = {}
		self.RTLgroup_glyph_exceptions_dict = {}
		self.RTLgroup_group_dict = {}		
		self.RTLpredefined_exceptions_dict = {}
		
		self.grouped_right = []
		self.grouped_left = []
		self.output = []
		
		self.subtbBreak = '\nsubtable;'
		self.lkupRTLopen = '\n\nlookup RTL_kerning {\nlookupflag RightToLeft IgnoreMarks;\n'
		self.lkupRTLclose = '\n\n} RTL_kerning;\n'
	

		if self.inFL: 
			self.header.append('# PS Name: %s' % self.f.font_name)
			self.isMMfont(self.f) # sets self.MM to True or False
			flC = ReadFontLabClasses(self.f)
			self.groups = flC.groups
			self.keyGlyphs = flC.keyGlyphs
			self.kernClassOrder = flC.kernClassOrder
			self.analyzeGroups()
			self.kerning = self.readFLkerning()

			if not self.MM:
				self.header.append('# MM Inst: %s' % self.f.menu_name)

		else:
			self.header.append('# PS Name: %s' % self.f.info.postscriptFontName)
			self.header.append('# MM Inst: %s' % self.f.info.styleMapFamilyName)
			self.groups.update(self.f.groups)
			self.kernClassOrder = sorted(self.f.groups.keys())
			self.analyzeGroups()
			self.kerning = self.f.kerning


		self.header.append('# MinKern: +/- %s inclusive' % self.minKern)
		self.header.append('# exported from %s' % self.appName)


		self.totalKernPairs = len(self.kerning)
		if not len(self.kerning):
			print "\tERROR: The font has no kerning!"
			return

		
		self.processKerningPairs()
		self.findExceptions()
		self.makeOutput()
		self.sanityCheck()
		self.writeDataToFile()


	def isMMfont(self, font):
		'Checks if the FontLab font is a Multiple Master font.'
		if font[0].layers_number > 1:
			self.MM = True


	def isGroup(self, name):
		'Checks for the first character of a group name. Returns True if it is "@" (OT KerningClass).'
		if name[0] == '@':
			return True
		else:
			return False


	def explode(self, leftClass, rightClass):
		'Returns a list of tuples, containing all possible combinations of elements in both input lists.'
		return list(itertools.product(leftClass, rightClass))


	def getKeyGlyphs(self, classList):
		'Returns a dictionary keyGlyph: className for a given list of classNames.'
		specificKeyGlyphs = {}
		for i in classList:
			keyGlyph = self.keyGlyphs[i]
			specificKeyGlyphs[keyGlyph] = i
		return specificKeyGlyphs


	def getClass(self, glyphName, side):
		'Replaces a glyph name by its class name, in case it is a key glyph for that side.'

		if side == 'left':
			if glyphName in self.leftKeyGlyphs:
				return self.leftKeyGlyphs[glyphName]
			else:
				return glyphName

		if side == 'right':
			if glyphName in self.rightKeyGlyphs:
				return self.rightKeyGlyphs[glyphName]
			else:
				return glyphName
				
			
	def checkGroupForTag(self, tag, groupName):
		'Checks if a tag (e.g. _CYR, _EXC, _LAT) exists in a group name (e.g. @A_LC_LEFT_LAT)'
		if tag in groupName:
			return True
		else:
			return False


	def returnGroupTag(self, groupName):
		'Returns group tag (e.g. _CYR, _EXC, _LAT) for a given group name (e.g. @A_LC_LEFT_LAT)'
		tags = [kLatinTag, kGreekTag, kCyrillicTag, kArabicTag, kHebrewTag, kNumberTag, kFractionTag, kExceptionTag]
		foundTag = None
		
		for tag in tags:
			if self.checkGroupForTag(tag, groupName):
				foundTag = tag
				break
		return foundTag
				

	def checkPairForTag(self, tag, pair):
		'Checks if a tag (e.g. _ARA, _EXC, _LAT) exists in one of both sides of a kerning pair (e.g. arJeh @ALEF_2ND_ARA)'
		left = pair[0]
		right = pair[1]

		if tag in left:
			return True
		elif tag in right:
			return True
		else:
			return False

			
	def checkForRTL(self, pair):
		'Checks if a given kerning pair is RTL. (Must involve a class on at least one side.)'
		RTLkerningTagsList = [kArabicTag , kHebrewTag]
		isRTLpair = False
		for tag in RTLkerningTagsList:
			if self.checkPairForTag(tag, pair):
				isRTLpair = True
				break
		return isRTLpair


	def readFLkerning(self):
		'Reads FontLab kerning and converts it into a UFO-style kerning dict.'
		kerning = {}
		glyphs = self.f.glyphs
		for gIdx in range(len(glyphs)):
			gName = str(glyphs[gIdx].name)
			gKerning = glyphs[gIdx].kerning
			for gKern in gKerning:
				gNameRightglyph = str(glyphs[gKern.key].name)

				if self.MM:
					kernValue = '<%s>' % ' '.join( map( str, gKern.values ) )  # gl.kerning[p].values is a list holding kern values of each master
				else:
					kernValue = int(gKern.value)
				
				pair = self.getClass(gName,'left'), self.getClass(gNameRightglyph,'right')
				
				kerning[pair] = kernValue
		return kerning
		

	def splitClasses(self, leftTagsList, rightTagsList):
		'Splits kerning classes into left and right sides; and assigns both sides classes without explicit side-flag.'

		ll = []
		rl = []

		for cl in self.groups: # loop through kerning classes
			if any([tag in cl for tag in leftTagsList]):
				ll.append(cl)
			elif any([tag in cl for tag in rightTagsList]):
				rl.append(cl)
			else:
				ll.append(cl)
				rl.append(cl)
	
		return ll, rl


	def dict2pos(self, dictionary, min=0, enum=False, RTL=False):
		'''
		Turns a dictionary to a list of kerning pairs. In a single master font, the function 
		can filter kerning pairs whose absolute value does not exceed a given threshold.
		'''
		data = []				# normal output data
		additional_data = []	# for sorting group, glyph and group,group kerning within subtables

		trimmed = 0

		for pair in dictionary:

			if RTL: 
				kernValue = int(dictionary[pair].split()[2])
				valueString = '<%s 0 %s 0>' % (kernValue, kernValue)
			else: 
				kernValue = dictionary[pair]
				valueString = kernValue

			string =  'pos %s %s;' % (' '.join(pair), valueString) 
			enumstring = 'enum %s' % string

			if self.MM: # no filtering happening in MM.
				data.append(string)

			elif enum:
				data.append(enumstring)
				
			else:
				if abs(kernValue) < min:
					if self.writeTrimmed:
						if self.isGroup(pair[0]) and not self.isGroup(pair[1]):
							additional_data.append('# %s' % string)
						else:
							data.append('# %s' % string)
					trimmed += 1
					
				if abs(kernValue) >= min:
					if self.isGroup(pair[0]) and not self.isGroup(pair[1]):
						additional_data.append(string)
					else:
						data.append(string)

		self.trimmedPairs += trimmed
		data.sort()

		if len(additional_data) > 0:
			additional_data.sort()
			additional_data.append('') # small break between group, glyph and group, group pairs; within the subtable.
			data = additional_data + data

		return '\n'.join(data)
	
	
	def analyzeGroups(self):
		'Uses self.groups for analysis and splitting.'
		if not len(self.groups):
			print "\tWARNING: The font has no kerning classes! Trimming switched off."
			# If there are no kerning classes present, there is no way to distinguish between
			# low-value pairs that just result from interpolation; and exception pairs. 
			# Consequently, trimming is switched off here.
			self.minKern = 0
			
		else:
			self.leftClasses, self.rightClasses = self.splitClasses(kLeftTag, kRightTag)

			# Lists of all glyphs that belong to kerning classes.
			for i in self.leftClasses:
				self.grouped_left.extend(self.groups[i])
			for i in self.rightClasses:
				self.grouped_right.extend(self.groups[i])
			
			if self.inFL: 
				# If in FontLab, creates dictionaries of left and right key glyphs with class names.
				
				self.leftKeyGlyphs = self.getKeyGlyphs(self.leftClasses) # e.g. 'guillemotleft': '@GUILLEFT' etc.
				self.rightKeyGlyphs = self.getKeyGlyphs(self.rightClasses)


	def processKerningPairs(self):
		'Sorting the kerning into various buckets.'
		
		print '\tProcessing kerning pairs...'
		
		for (left, right), value in sorted(self.kerning.items()[::-1]):
			pair = (left, right)

			# Skip pairs in which the name of the left glyph contains the ignore tag.
			if kIgnorePairTag in left:
				self.notProcessed += 1
				continue
			
			# Looking for pre-defined exception pairs, and filtering them out.
			if self.checkPairForTag(kExceptionTag, pair):
				self.predefined_exceptions_dict[pair] = self.kerning[pair]
				del self.kerning[pair]
			
			else:
				# Filtering the kerning by type.
			 	if self.isGroup(left):
			 		if self.isGroup(right):
			 			self.group_group.append((left, right))
			 		else:
			 			self.group_glyph.append((left, right))

			 	else:
					if self.isGroup(right):
						self.glyph_group.append((left, right))
					else:
						self.glyph_glyph.append((left, right))

		
		# Quick sanity check
		if len(self.glyph_group) + len(self.glyph_glyph) + len(self.group_glyph) + len(self.group_group) != len(self.kerning)-self.notProcessed: 
			print 'Something went wrong: kerning lists do not match the amount of kerning pairs present in the font.'
				
				

	def findExceptions(self):
		'Process lists (glyph_group, group_group etc.) created above to find out which pairs are exceptions, and which just are just normal pairs'
		
		# glyph to group pairs:
		# ---------------------

		for (g, gr) in self.glyph_group:
			isRTLpair = self.checkForRTL((g, gr))
			group = self.groups[gr]
			if g in self.grouped_left:
				# it is a glyph_to_group exception!
				if isRTLpair:
					self.RTLglyph_group_exceptions_dict[g, gr] = '<%s 0 %s 0>' % (self.kerning[g, gr], self.kerning[g, gr])
				else:
					self.glyph_group_exceptions_dict[g, gr] = self.kerning[g, gr]
			else:
				for i in group:
					pair = (g, i)
					if pair in self.glyph_glyph:
						# that pair is a glyph_to_glyph exception!
						if isRTLpair:
							self.RTLglyph_glyph_exceptions_dict[pair] = '<%s 0 %s 0>' % (self.kerning[pair], self.kerning[pair])
						else:
							self.glyph_glyph_exceptions_dict[pair] = self.kerning[pair]
							
				else:
					if isRTLpair:
						self.RTLglyph_group_dict[g, gr] = '<%s 0 %s 0>' % (self.kerning[g, gr], self.kerning[g, gr])
					else:
						self.glyph_group_dict[g, gr] = self.kerning[g, gr]
	

		# group to glyph pairs:
		# ---------------------

		for (gr, g) in self.group_glyph:
			isRTLpair = self.checkForRTL((gr, g))
			group = self.groups[gr]
			if g in self.grouped_right:
				# it is a group_to_glyph exception!
				if isRTLpair:
					self.RTLgroup_glyph_exceptions_dict[gr, g] = '<%s 0 %s 0>' % (self.kerning[gr, g], self.kerning[gr, g])
				else:
					self.group_glyph_exceptions_dict[gr, g] = self.kerning[gr, g]
			else:
				for i in group:
					pair = (i, g)
					if pair in self.glyph_glyph:
						# that pair is a glyph_to_glyph exception!
						if isRTLpair:
							self.RTLglyph_glyph_exceptions_dict[pair] = '<%s 0 %s 0>' % (self.kerning[pair], self.kerning[pair])
						else:
							self.glyph_glyph_exceptions_dict[pair] = self.kerning[pair]
				else:
					if isRTLpair:
						self.RTLgroup_glyph_dict[gr, g] = '<%s 0 %s 0>' % (self.kerning[gr, g], self.kerning[gr, g])
					else:
						self.group_glyph_dict[gr, g] = self.kerning[gr, g]
		

		# group to group pairs:
		# ---------------------

		explodedPairList = []
		RTLexplodedPairList = []
		for (lgr, rgr) in self.group_group:
			isRTLpair = self.checkForRTL((lgr, rgr))
			lgroup = self.groups[lgr]
			rgroup = self.groups[rgr]
			if isRTLpair:
				self.RTLgroup_group_dict[lgr, rgr] = '<%s 0 %s 0>' % (self.kerning[lgr, rgr], self.kerning[lgr, rgr])
				RTLexplodedPairList.extend(self.explode(lgroup, rgroup))
			else:
				self.group_group_dict[lgr, rgr] = self.kerning[lgr, rgr]
				explodedPairList.extend(self.explode(lgroup, rgroup))
				# list of all possible pair combinations for the @class @class kerning pairs of the font.

		exceptionPairs = set.intersection(set(explodedPairList), set(self.glyph_glyph))
		RTLexceptionPairs = set.intersection(set(RTLexplodedPairList), set(self.glyph_glyph))
		# Finds the intersection of the exploded pairs with the glyph_glyph pairs collected above.
		# Those must be exceptions, as they occur twice (once in class-kerning, once as a single pair).
		
		
		for pair in exceptionPairs:
			self.glyph_glyph_exceptions_dict[pair] = self.kerning[pair]

		for pair in RTLexceptionPairs:
			self.RTLglyph_glyph_exceptions_dict[pair] = '<%s 0 %s 0>' %  (self.kerning[pair], self.kerning[pair])
			


		# glyph to glyph pairs (No RTL possible, as of now. RTL pairs are now only identified by their group name, this must be changed one day (to a glyph note, for instance).)
		# ---------------------
		
		for (lg, rg) in self.glyph_glyph:
			pair = (lg, rg)
			if not pair in self.glyph_glyph_exceptions_dict and not pair in self.RTLglyph_glyph_exceptions_dict:
				self.glyph_glyph_dict[pair] = self.kerning[pair]



	def makeOutput(self):
		'Building the output data.'

		# kerning classes:
		# ----------------

		for kernClass in self.kernClassOrder:
			glyphList = self.groups[kernClass]
			glyphString = ' '.join(glyphList)
			self.output.append( '%s = [%s];' % (kernClass, glyphString) )


		# ------------------
		# LTR kerning pairs:
		# ------------------

		order = [
		# dictName							# minKern		# comment							# enum
		(self.predefined_exceptions_dict,	0,				'\n# pre-defined exceptions:',		True),
		(self.glyph_glyph_dict,				self.minKern,	'\n# glyph, glyph:',				False),
		(self.glyph_group_exceptions_dict,	0,				'\n# glyph, group exceptions:',		True),
		(self.group_glyph_exceptions_dict,	0,				'\n# group, glyph exceptions:',		True),
		(self.glyph_glyph_exceptions_dict,	0,				'\n# glyph, glyph exceptions:',		False),
		]

		orderExtension = [ 
		# in case no subtables are desired
		(self.glyph_group_dict,				self.minKern,	'\n# glyph, group:',				False),
		(self.group_glyph_dict,				self.minKern,	'\n# group, glyph:',				False),
		(self.group_group_dict,				self.minKern,	'\n# group, group:',				False)
		]
		

		if not self.writeSubtables:
			order.extend(orderExtension)
			

		for dictName, minKern, comment, enum in order:
			if len(dictName):
				self.processedPairs += len(dictName)
				self.output.append(comment)
				self.output.append(self.dict2pos(dictName, minKern, enum))


		if self.writeSubtables:
			subtablesCreated = 0
			# Keeping track of the number of subtables created;
			# There is no necessity to add a "subtable;" statement before the first subtable.

			# glyph-class subtables
			# ---------------------
			glyph_to_class_subtables = MakeSubtables(self.glyph_group_dict, checkSide='second').subtables
			self.output.append( '\n# glyph, group:' )

			for table in glyph_to_class_subtables:
				if len(table):
					self.processedPairs += len(table)
					subtablesCreated += 1
				
					if subtablesCreated > 1:
						self.output.append( self.subtbBreak )
		
					self.output.append( self.dict2pos(table, self.minKern) )


			# class-class subtables
			# ---------------------
			self.group_group_dict.update(self.group_glyph_dict)

			class_to_class_subtables = MakeSubtables(self.group_group_dict).subtables
			self.output.append( '\n# group, glyph and group, group:' )

			for table in class_to_class_subtables:
				if len(table):
					self.processedPairs += len(table)
					subtablesCreated += 1
				
					if subtablesCreated > 1:
						self.output.append( self.subtbBreak )

					self.output.append( self.dict2pos(table, self.minKern) )


		# ------------------
		# RTL kerning pairs:
		# ------------------

		RTLorder = [
		# dictName								# minKern		# comment								# enum
		(self.RTLpredefined_exceptions_dict,	0,				'\n# RTL pre-defined exceptions:',		True),
		(self.RTLglyph_glyph_dict,				self.minKern,	'\n# RTL glyph, glyph:',				False),
		(self.RTLglyph_group_exceptions_dict,	0,				'\n# RTL glyph, group exceptions:',		True),
		(self.RTLgroup_glyph_exceptions_dict,	0,				'\n# RTL group, glyph exceptions:',		True),
		(self.RTLglyph_glyph_exceptions_dict,	0,				'\n# RTL glyph, glyph exceptions:',		False),
		]

		RTLorderExtension = [ 
		# in case no subtables are desired
		(self.RTLglyph_group_dict,				self.minKern,	'\n# RTL glyph, group:',				False),
		(self.RTLgroup_glyph_dict,				self.minKern,	'\n# RTL group, glyph:',				False),
		(self.RTLgroup_group_dict,				self.minKern,	'\n# RTL group, group:',				False)
		]


		if not self.writeSubtables:
			RTLorder.extend(RTLorderExtension)
		
		# checking if RTL pairs exist
		RTLpairsExist = False
		allRTL = RTLorderExtension + RTLorder
		for dictName, minKern, comment, enum in allRTL:
			if len(dictName):
				RTLpairsExist = True
				break
		
		if RTLpairsExist:
			self.output.append(self.lkupRTLopen)
		
			for dictName, minKern, comment, enum in RTLorder:
				if len(dictName):
					self.processedPairs += len(dictName)
					self.output.append(comment)
					self.output.append( self.dict2pos(dictName, minKern, enum, RTL=True) )
		

		if RTLpairsExist and self.writeSubtables:
			RTLsubtablesCreated = 0

			# RTL glyph-class subtables
			# -------------------------
			RTL_glyph_class_subtables = MakeSubtables(self.RTLglyph_group_dict, checkSide='second', RTL=True).subtables
			self.output.append( '\n# RTL glyph, group:' )

			for table in RTL_glyph_class_subtables:
				if len(table):
					self.processedPairs += len(table)
					RTLsubtablesCreated += 1
				
					if RTLsubtablesCreated > 1:
						self.output.append( self.subtbBreak )
		
					self.output.append( self.dict2pos(table, self.minKern, RTL=True) )


			# RTL class-class subtables
			# -------------------------
			self.RTLgroup_group_dict.update(self.RTLgroup_glyph_dict)

			RTL_class_class_subtables = MakeSubtables(self.RTLgroup_group_dict, RTL=True).subtables
			self.output.append( '\n# RTL group, glyph and group, group:' )

			for table in RTL_class_class_subtables:
				if len(table):
					self.processedPairs += len(table)
					RTLsubtablesCreated += 1
				
					if RTLsubtablesCreated > 1:
						# This would happen when both Arabic and Hebrew glyphs are present in one font.
						self.output.append( self.subtbBreak )
					
					self.output.append( self.dict2pos(table, self.minKern, RTL=True) )
					
					
		if RTLpairsExist:
			self.output.append(self.lkupRTLclose)
			

	def sanityCheck(self):
		'Checks if the number of kerning pairs input equals the number of kerning entries output.'
		
		if self.totalKernPairs != self.processedPairs + self.notProcessed: # len(self.allKernPairs) + self.notProcessed - self.numBreaks + self.trimmedPairs:
			print 'Something went wrong...'
			print 'Kerning pairs provided: %s' % self.totalKernPairs
			print 'Kern entries generated: %s' % (self.processedPairs + self.notProcessed)
			print 'Pairs not processed: %s' % (self.totalKernPairs - (self.processedPairs+self.notProcessed))


	def writeDataToFile(self):

		if self.MM:
			kKernFeatureFileName = '%s.%s' % (self.outputFileName, 'mmkern')
		else:
			kKernFeatureFileName = '%s.%s' % (self.outputFileName, 'kern')

		print '\tSaving %s file...' % kKernFeatureFileName
		if self.trimmedPairs > 0:
			print '\tTrimmed pairs: %s' % self.trimmedPairs
		
		filePath = os.path.join(self.folder, kKernFeatureFileName)

		outfile = open(filePath, 'w')
		outfile.write('\n'.join(self.header))
		outfile.write('\n\n')
		if len(self.output):
			outfile.write('\n'.join(self.output))
			outfile.write('\n')
		outfile.close()
		if not self.inFL: print '\tOutput file written to %s' % filePath



class MakeSubtables(KernDataClass):
	def __init__(self, kernDict, checkSide='first', RTL=False):
		self.kernDict  = kernDict
		self.RTL       = RTL		# Is the kerning RTL or not?
		self.checkSide = checkSide	# Which side of the pair is triggering the subtable decision? 
									# "first" would be the left side for LTR, right for RTL.
									# "second" would be the right side for LTR, left for RTL.
		
		self.otherPairs_dict = {}
		# Container for any pairs that cannot be assigned to a specific language tag.

		self.LTRtagDict = {
			kLatinTag: {},
			kGreekTag: {},
			kCyrillicTag: {},
			kArabicTag: {},
			kHebrewTag: {},
			kNumberTag: {},
			kFractionTag: {},
			'other': self.otherPairs_dict
		}

		self.RTLtagDict = {
			kArabicTag: {},
			kHebrewTag: {},
			'other': self.otherPairs_dict
		}
		
		self.subtableOrder = [kLatinTag, kGreekTag, kCyrillicTag, kArabicTag, kHebrewTag, kNumberTag, kFractionTag, 'other']
		# The order in which subtables are written
		
		if RTL:
			self.subtables = [self.RTLtagDict[i] for i in self.subtableOrder if i in self.RTLtagDict]
		else:
			self.subtables = [self.LTRtagDict[i] for i in self.subtableOrder]
		

		'Split class-to-class kerning into subtables.'
		if self.checkSide == 'first':
			# Creates 'traditional' subtables, for class-to-class, and class-to-glyph kerning.
			for pair in self.kernDict.keys()[::-1]: 
				first, second, tagDict = self.analyzePair(pair)
				
				for tag in tagDict:
					if self.checkGroupForTag(tag, first):
						tagDict[tag][pair] = kernDict[pair]
						del self.kernDict[pair]

			for pair in self.kernDict:
				self.otherPairs_dict[pair] = self.kernDict[pair]
			

		if self.checkSide == 'second':

			# Create dictionary of all glyphs on the left side, and the language tags of classes those glyphs are kerned against (e.g. _LAT, _GRK)
			kernPartnerLanguageTags = {}
			for pair in self.kernDict:
				first, second, tagDict = self.analyzePair(pair)

				if not first in kernPartnerLanguageTags:
					kernPartnerLanguageTags[first] = set([])
				kernPartnerLanguageTags[first].add(self.returnGroupTag(pair[1]))

			for pair in self.kernDict.keys()[::-1]:
				first, second, tagDict = self.analyzePair(pair)
				
				for tag in tagDict:
					if self.checkGroupForTag(tag, second) and len(kernPartnerLanguageTags[first]) == 1:
						# Using the previously created kernPartnerLanguageTags
						# If any glyph is kerned against more than one language system, it has to go to the 'otherPairs_dict' subtable.
						tagDict[tag][pair] = self.kernDict[pair]
						del self.kernDict[pair]
						
			for pair in self.kernDict:
				self.otherPairs_dict[pair] = self.kernDict[pair]
				

		
	def analyzePair(self, pair):
		if self.RTL:
			first   = pair[0]
			second  = pair[1]
			tagDict = self.RTLtagDict
				
		else:
			first   = pair[0]
			second  = pair[1]
			tagDict = self.LTRtagDict

		return first, second, tagDict




