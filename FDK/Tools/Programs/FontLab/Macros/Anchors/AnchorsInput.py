#FLM: Anchors Input

__copyright__ =  """
Copyright (c) 2012 Adobe Systems Incorporated

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
"""

__doc__ = """
Adds anchors to the glyphs by reading the data from external text file(s) named
anchors_X (where X represents the index of the font's master).
The simple text file(s) containing the anchor data must have four tab-separated
columns that follow this format:
glyphName	anchorName	anchorXposition	anchorYposition
"""

import os, time


class myAnchor:
	def __init__(self, parent, name, x=0, y=0):
		self.parent = parent
		self.name = name
		self.x = x
		self.y = y


class myAnchorLayers:
	def __init__(self, layers):
		self.layers = layers
		self.struct = [[] for i in range(self.layers)] # This way the multidimensional list is created without referencing

	def addToLayers(self, anchorsList):
		for i in range(self.layers):
			self.struct[i].append(anchorsList[i])

	def addAnchorToLayer(self, anchor, layerIndex):
		self.struct[layerIndex].append(anchor)

	def getLayer(self, layerIndex):
		return self.struct[layerIndex]

	def outputLayer(self, num):
		self.anchorsList = []
		for anchor in self.struct[num]:
			self.anchorEntry = "%s\t%s\t%d\t%d\n" % (anchor.parent, anchor.name, anchor.x, anchor.y)
			self.anchorsList.append(self.anchorEntry)
		return self.anchorsList


def readFile(filePath):
	file = open(filePath, 'r')
	fileContent = file.read().splitlines()
	file.close()
	return fileContent


def findAnchorIndex(anchorName, glyph):
	index = None
	for i in range(len(glyph.anchors)):
		if glyph.anchors[i].name == anchorName:
			index = i
			return index
	return index


def run():
	print "Working..."
	# Initialize object
	anchorsData = myAnchorLayers(numMasters)


	# Read files
	for index in range(numMasters):
		filePath = "anchors_%s" % index
		
		if not os.path.isfile(filePath):
			print "ERROR: File %s not found." % filePath
			return
		
		anchorLayer = readFile(filePath)
		
		for lineIndex in range(len(anchorLayer)):
			anchorValuesList = anchorLayer[lineIndex].split('\t')
			if len(anchorValuesList) != 4: # Four columns: glyph name, anchor name, anchor X postion, anchor Y postion
				print "ERROR: Line #%d does not have 4 columns. Skipped." % (lineIndex +1)
				continue
			
			glyphName = anchorValuesList[0]
			anchorName = anchorValuesList[1]
			
			if not len(glyphName) or not len(anchorName):
				print "ERROR: Line #%d has no glyph name or no anchor name. Skipped." % (lineIndex +1)
				continue

			try:
				anchorX = int(anchorValuesList[2])
				anchorY = int(anchorValuesList[3])
			except:
				print "ERROR: Line #%d has an invalid anchor position value. Skipped." % (lineIndex +1)
				continue
			
			newAnchor = myAnchor(glyphName, anchorName, anchorX, anchorY)
			anchorsData.addAnchorToLayer(newAnchor, index)
	

	# Remove all anchors
	for glyph in f.glyphs:
		if len(glyph.anchors) > 0:
			anchorIndexList = range(len(glyph.anchors))
			anchorIndexList.reverse()
			for i in anchorIndexList: # delete from last to first
				del glyph.anchors[i]
			fl.UpdateGlyph(glyph.index)

	# Add all anchors
	for master in range(numMasters):
		anchorList = anchorsData.getLayer(master)
		
		for anchor in anchorList:
			# Make sure that the glyph exists
			if fl.font.FindGlyph(anchor.parent) != -1:
				glyph = f[anchor.parent]
			else:
				print "ERROR: Glyph %s not found in the font." % anchor.parent
				continue
			
			# Create the anchor in case it doesn't exist already
			if not glyph.FindAnchor(anchor.name):
				newAnchor = Anchor(anchor.name, anchor.x, anchor.y)
				glyph.anchors.append(newAnchor)
				fl.SetUndo(glyph.index)
			else:
				if master == 0: # check only on the first master (since the names of the anchors are the same in all masters)
					print "WARNING: Anchor exists already: %s\t%s" % (anchor.parent, anchor.name)
			
			# Reposition the anchor only in the masters other than the first
			if master > 0:
				anchorIndex = findAnchorIndex(anchor.name, glyph)
				glyph.anchors[anchorIndex].Layer(master).x = anchor.x
				glyph.anchors[anchorIndex].Layer(master).y = anchor.y

			#glyph.mark = 125
			fl.UpdateGlyph(glyph.index)	


	f.modified = 1
	print 'Done! (%s)' % time.strftime("%H:%M:%S", time.localtime())


if __name__ == "__main__":
	f = fl.font
	if len(f):
		numMasters = f[0].layers_number
		
		if (f.file_name):
			fpath, fname = os.path.split(f.file_name) # The current file and path
			os.chdir(fpath) # Change current directory to location of open font
			run()
		else:
			print "The font needs to be saved first!"
	else:
		print "Font has no glyphs!"
