#FLM: Anchors Output

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
Outputs all the anchor data to text file(s) named anchors_X (where X represents 
the index of the font's master).
"""

import os


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

	def outputLayer(self, num):
		self.anchorsList = []
		for anchor in self.struct[num]:
			self.anchorEntry = "%s\t%s\t%d\t%d\n" % (anchor.parent, anchor.name, anchor.x, anchor.y)
			self.anchorsList.append(self.anchorEntry)
		return self.anchorsList


def run():
	# Initialize object
	anchorsData = myAnchorLayers(numMasters)
	
	# Collect anchors data
	for glyph in f.glyphs:
		glyphName = glyph.name

		for anchorIndex in range(len(glyph.anchors)):
			anchorName = glyph.anchors[anchorIndex].name
			
			# Skip nameless anchors
			if not len(anchorName):
				print "ERROR: Glyph %s has a nameless anchor. Skipped." % glyphName
				continue

			tempSlice = []
			
			for masterIndex in range(numMasters):
				point = glyph.anchors[anchorIndex].Layer(masterIndex)
				anchorX, anchorY = point.x, point.y
				newAnchor = myAnchor(glyphName, anchorName, anchorX, anchorY)
				tempSlice.append(newAnchor)
			
			anchorsData.addToLayers(tempSlice)

	if not len(anchorsData.outputLayer(0)):
		print "The font has no anchors!"
		return
	
	# Write files:
	for layer in range(numMasters):
		filename = "anchors_%s" % layer
		print "Writing file %s..." % filename
		outfile = open(filename, 'w')
		for anchorLine in anchorsData.outputLayer(layer):
			outfile.write(anchorLine)
		outfile.close()

	print 'Done!'


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
