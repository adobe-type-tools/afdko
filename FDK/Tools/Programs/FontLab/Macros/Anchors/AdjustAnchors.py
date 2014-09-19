#FLM: Adjust Anchors

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
Adjust Anchors v1.2 - Jul 12 2012

This script provides an UI for adjusting the position of anchors interactively.
FontLab's own UI for ajusting anchors is too poor. 
Opening FontLab's Preview window and selecting the Anchors pane before running
this script, will allow you to preview the adjustments even better.
"""

listGlyphsSelected = []
def getgselectedglyphs(font, glyph, gindex):
	listGlyphsSelected.append(gindex)
fl.ForSelected(getgselectedglyphs)

def getMasterNames(masters, axes):
	global matrix
	masterNames = []
	if masters > 1:
		for m in range(masters):
			mtx = matrix[m]
			masterName = ''
			for i in range(len(axes)):
				masterName += ' ' + axes[i][1] + str(mtx[i])
			masterNames.append(masterName)
	return masterNames

matrix = [
(0,0,0,0),(1,0,0,0),(0,1,0,0),(1,1,0,0),(0,0,1,0),(1,0,1,0),(0,1,1,0),(1,1,1,0),
(0,0,0,1),(1,0,0,1),(0,1,0,1),(1,1,0,1),(0,0,1,1),(1,0,1,1),(0,1,1,1),(1,1,1,1)
]

STYLE_RADIO = STYLE_CHECKBOX + cTO_CENTER

def run(gIndex):
	masters = f[0].layers_number
	axes = f.axis
	masterNames = getMasterNames(masters, axes)
	increment = 0
	if len(axes) == 3:
		increment = 90
	elif len(axes) > 3:
		fl.Message("This macro does not support 4-axis fonts")
		return
	
	fl.EditGlyph(gIndex) # opens Glyph Window in case it's not open yet
	glyphBkupDict = {} # this will store a copy of the edited glyphs and will be used in case 'Cancel' is pressed
				
	class DialogClass:
		def __init__(self):
			self.d = Dialog(self)
			self.d.size = Point(660, 110 + 48*4 + increment)
			self.d.Center()
			self.d.title = 'Adjust Anchors'

			self.anchorList = []
			self.anchorList_index = 0
			self.anchorList_selected = 0
			self.selectedAnchor = None

			self.glyph = f[gIndex]
			self.gIndex = gIndex
			self.gName = self.glyph.name
			self.gHasAnchors = 0
			self.glyphList = []
			self.glyphList_index = 0
			self.glyphList_selected = 0
			self.selectedglyph = None

			self.k_BIG_SHIFT = 20
			self.k_MEDIUM_SHIFT = 5
			self.k_SMALL_SHIFT = 1

			self.Xshift = 0
			self.Yshift = 0
			self.Xorig = 0
			self.Yorig = 0
			self.Xfinal = 0
			self.Yfinal = 0
			
			self.RBmasterIndex = 0
			if fl.layer == 0: self.RBmaster0 = 1
			else: self.RBmaster0 = 0
			if fl.layer == 1: self.RBmaster1 = 1
			else: self.RBmaster1 = 0
			if fl.layer == 2: self.RBmaster2 = 1
			else: self.RBmaster2 = 0
			if fl.layer == 3: self.RBmaster3 = 1
			else: self.RBmaster3 = 0
			if fl.layer == 4: self.RBmaster4 = 1
			else: self.RBmaster4 = 0
			if fl.layer == 5: self.RBmaster5 = 1
			else: self.RBmaster5 = 0
			if fl.layer == 6: self.RBmaster6 = 1
			else: self.RBmaster6 = 0
			if fl.layer == 7: self.RBmaster7 = 1
			else: self.RBmaster7 = 0

			# Fill in the Anchor list
			for anchor in self.glyph.anchors:
				self.anchorList.append(anchor.name)

			# Fill in the Glyph list
			for g in f.glyphs:
				if len(g.anchors) > 0:
					self.glyphList.append(g.name)
			
			# Checks if the initially selected glyph has anchors
			if self.gName in self.glyphList:
				self.gHasAnchors = 1

			posy = 10 + 48*0				#  (xTop , yTop , xBot , yBot)
			self.d.AddControl(BUTTONCONTROL, Rect(310, posy, 350, posy+40), 'Yplus5', STYLE_BUTTON, '+'+ str(self.k_MEDIUM_SHIFT))

			posy = 10 + 24*1
			self.d.AddControl(LISTCONTROL, Rect( 10, posy, 150, posy+110), 'glyphList', STYLE_LIST, 'Glyphs')
			self.d.AddControl(LISTCONTROL, Rect(510, posy, 650, posy+110), 'anchorList', STYLE_LIST, 'Anchors')

			posy = 10 + 48*1
			self.d.AddControl(BUTTONCONTROL, Rect(310, posy, 350, posy+40), 'Yplus1', STYLE_BUTTON, '+'+ str(self.k_SMALL_SHIFT))

			posy = 10 + 48*2
			self.d.AddControl(BUTTONCONTROL, Rect(160, posy, 200, posy+40), 'Xminus20', STYLE_BUTTON, '-'+ str(self.k_BIG_SHIFT))
			self.d.AddControl(BUTTONCONTROL, Rect(210, posy, 250, posy+40), 'Xminus5', STYLE_BUTTON, '-'+ str(self.k_MEDIUM_SHIFT))
			self.d.AddControl(BUTTONCONTROL, Rect(260, posy, 300, posy+40), 'Xminus1', STYLE_BUTTON, '-'+ str(self.k_SMALL_SHIFT))
			self.d.AddControl(STATICCONTROL, Rect(310, posy,    323, posy+20), 'stat_label', STYLE_LABEL+cTO_CENTER, 'x:')
			self.d.AddControl(STATICCONTROL, Rect(323, posy,    360, posy+20), 'Xshift', STYLE_LABEL+cTO_CENTER)
			self.d.AddControl(STATICCONTROL, Rect(310, posy+20, 323, posy+40), 'stat_label', STYLE_LABEL+cTO_CENTER, 'y:')
			self.d.AddControl(STATICCONTROL, Rect(323, posy+20, 360, posy+40), 'Yshift', STYLE_LABEL+cTO_CENTER)
			self.d.AddControl(BUTTONCONTROL, Rect(360, posy, 400, posy+40), 'Xplus1', STYLE_BUTTON, '+'+ str(self.k_SMALL_SHIFT))
			self.d.AddControl(BUTTONCONTROL, Rect(410, posy, 450, posy+40), 'Xplus5', STYLE_BUTTON, '+'+ str(self.k_MEDIUM_SHIFT))
			self.d.AddControl(BUTTONCONTROL, Rect(460, posy, 500, posy+40), 'Xplus20', STYLE_BUTTON, '+'+ str(self.k_BIG_SHIFT))

			for i in range(len(masterNames)):
				posy = 154 + 22*i
				self.d.AddControl(CHECKBOXCONTROL, Rect( 25, posy, 200, posy+20), 'RBmaster'+ str(i), STYLE_RADIO, masterNames[i])

			posy = 10 + 48*3
			self.d.AddControl(BUTTONCONTROL, Rect(310, posy, 350, posy+40), 'Yminus1', STYLE_BUTTON, '-'+ str(self.k_SMALL_SHIFT))
			self.d.AddControl(STATICCONTROL, Rect(528, posy, 650, posy+20), 'stat_label', STYLE_LABEL+cTO_CENTER, 'Original position')
			self.d.AddControl(STATICCONTROL, Rect(530, posy+20, 543, posy+40), 'stat_label', STYLE_LABEL+cTO_CENTER, 'x:')
			self.d.AddControl(STATICCONTROL, Rect(543, posy+20, 580, posy+40), 'Xorig', STYLE_LABEL+cTO_CENTER)
			self.d.AddControl(STATICCONTROL, Rect(590, posy+20, 603, posy+40), 'stat_label', STYLE_LABEL+cTO_CENTER, 'y:')
			self.d.AddControl(STATICCONTROL, Rect(603, posy+20, 640, posy+40), 'Yorig', STYLE_LABEL+cTO_CENTER)

			posy = 10 + 48*4
			self.d.AddControl(BUTTONCONTROL, Rect(310, posy, 350, posy+40), 'Yminus5', STYLE_BUTTON, '-'+ str(self.k_MEDIUM_SHIFT))
			self.d.AddControl(STATICCONTROL, Rect(528, posy, 650, posy+20), 'stat_label', STYLE_LABEL+cTO_CENTER, 'Final position')
			self.d.AddControl(STATICCONTROL, Rect(530, posy+20, 543, posy+40), 'stat_label', STYLE_LABEL+cTO_CENTER, 'x:')
			self.d.AddControl(STATICCONTROL, Rect(543, posy+20, 580, posy+40), 'Xfinal', STYLE_LABEL+cTO_CENTER)
			self.d.AddControl(STATICCONTROL, Rect(590, posy+20, 603, posy+40), 'stat_label', STYLE_LABEL+cTO_CENTER, 'y:')
			self.d.AddControl(STATICCONTROL, Rect(603, posy+20, 640, posy+40), 'Yfinal', STYLE_LABEL+cTO_CENTER)


#====== DIALOG FUNCTIONS =========

		def on_Xminus20(self, code):
			if self.anchorList_selected:
				self.Xshift -= self.k_BIG_SHIFT
				self.d.PutValue('Xshift')
				self.updateXfinal()
				self.update_glyph()
		def on_Xminus5(self, code):
			if self.anchorList_selected:
				self.Xshift -= self.k_MEDIUM_SHIFT
				self.d.PutValue('Xshift')
				self.updateXfinal()
				self.update_glyph()
		def on_Xminus1(self, code):
			if self.anchorList_selected:
				self.Xshift -= self.k_SMALL_SHIFT
				self.d.PutValue('Xshift')
				self.updateXfinal()
				self.update_glyph()
		def on_Xplus1(self, code):
			if self.anchorList_selected:
				self.Xshift += self.k_SMALL_SHIFT
				self.d.PutValue('Xshift')
				self.updateXfinal()
				self.update_glyph()
		def on_Xplus5(self, code):
			if self.anchorList_selected:
				self.Xshift += self.k_MEDIUM_SHIFT
				self.d.PutValue('Xshift')
				self.updateXfinal()
				self.update_glyph()
		def on_Xplus20(self, code):
			if self.anchorList_selected:
				self.Xshift += self.k_BIG_SHIFT
				self.d.PutValue('Xshift')
				self.updateXfinal()
				self.update_glyph()

		def on_Yminus5(self, code):
			if self.anchorList_selected:
				self.Yshift -= self.k_MEDIUM_SHIFT
				self.d.PutValue('Yshift')
				self.updateYfinal()
				self.update_glyph()
		def on_Yminus1(self, code):
			if self.anchorList_selected:
				self.Yshift -= self.k_SMALL_SHIFT
				self.d.PutValue('Yshift')
				self.updateYfinal()
				self.update_glyph()
		def on_Yplus1(self, code):
			if self.anchorList_selected:
				self.Yshift += self.k_SMALL_SHIFT
				self.d.PutValue('Yshift')
				self.updateYfinal()
				self.update_glyph()
		def on_Yplus5(self, code):
			if self.anchorList_selected:
				self.Yshift += self.k_MEDIUM_SHIFT
				self.d.PutValue('Yshift')
				self.updateYfinal()
				self.update_glyph()

		def on_glyphList(self, code):
			self.glyphList_selected = 1
			self.gHasAnchors = 1
			self.d.GetValue('glyphList')
			self.gName = self.glyphList[self.glyphList_index] # Name of the glyph selected on the glyph list
			self.gIndex = f.FindGlyph(self.gName)
			fl.iglyph = self.gIndex  # Switch the glyph on the Glyph Window
			self.glyph = f[self.gIndex]
			self.updateAnchorsList()
			self.resetDialogValues()

		def on_anchorList(self, code):
			self.anchorList_selected = 1
			self.d.GetValue('anchorList')
			self.updateDialogValues()

		def on_RBmaster0(self, code): self.updateRBmaster(0)
		def on_RBmaster1(self, code): self.updateRBmaster(1)
		def on_RBmaster2(self, code): self.updateRBmaster(2)
		def on_RBmaster3(self, code): self.updateRBmaster(3)
		def on_RBmaster4(self, code): self.updateRBmaster(4)
		def on_RBmaster5(self, code): self.updateRBmaster(5)
		def on_RBmaster6(self, code): self.updateRBmaster(6)
		def on_RBmaster7(self, code): self.updateRBmaster(7)

		def on_ok(self, code):
			return 1


#====== RESET FUNCTIONS =========

		def resetDialogValues(self):
			self.resetXorig()
			self.resetYorig()
			self.resetXshift()
			self.resetYshift()
			self.resetXfinal()
			self.resetYfinal()

		def resetXorig(self):
			self.Xorig = 0
			self.d.PutValue('Xorig')

		def resetYorig(self):
			self.Yorig = 0
			self.d.PutValue('Yorig')

		def resetXshift(self):
			self.Xshift = 0
			self.d.PutValue('Xshift')

		def resetYshift(self):
			self.Yshift = 0
			self.d.PutValue('Yshift')

		def resetXfinal(self):
			self.Xfinal = 0
			self.d.PutValue('Xfinal')

		def resetYfinal(self):
			self.Yfinal = 0
			self.d.PutValue('Yfinal')


#====== UPDATE FUNCTIONS =========

		def updateRBmaster(self, newIndex):
			self.RBmasterIndex = newIndex
			if self.RBmasterIndex == 0: self.RBmaster0 = 1
			else: self.RBmaster0 = 0
			if self.RBmasterIndex == 1: self.RBmaster1 = 1
			else: self.RBmaster1 = 0
			if self.RBmasterIndex == 2: self.RBmaster2 = 1
			else: self.RBmaster2 = 0
			if self.RBmasterIndex == 3: self.RBmaster3 = 1
			else: self.RBmaster3 = 0
			if self.RBmasterIndex == 4: self.RBmaster4 = 1
			else: self.RBmaster4 = 0
			if self.RBmasterIndex == 5: self.RBmaster5 = 1
			else: self.RBmaster5 = 0
			if self.RBmasterIndex == 6: self.RBmaster6 = 1
			else: self.RBmaster6 = 0
			if self.RBmasterIndex == 7: self.RBmaster7 = 1
			else: self.RBmaster7 = 0
			for v in ['RBmaster0','RBmaster1','RBmaster2','RBmaster3','RBmaster4','RBmaster5','RBmaster6','RBmaster7']:
				self.d.PutValue(v)
			fl.layer = self.RBmasterIndex
			if self.gHasAnchors and self.anchorList_selected:
				self.updateDialogValues()

		def updateAnchorsList(self):
			self.anchorList = []
			for anchor in self.glyph.anchors:
				self.anchorList.append(anchor.name)
			self.d.PutValue('anchorList')
			self.anchorList_selected = 0
			self.selectedAnchor = None

		def updateDialogValues(self):
			self.selectedAnchor = self.glyph.anchors[self.anchorList_index].Layer(fl.layer)
			self.updateXorig(self.selectedAnchor.x)
			self.updateYorig(self.selectedAnchor.y)
			self.resetXshift()
			self.resetYshift()
			self.updateXfinal()
			self.updateYfinal()

		def updateXorig(self, pos):
			self.Xorig = pos
			self.d.PutValue('Xorig')

		def updateYorig(self, pos):
			self.Yorig = pos
			self.d.PutValue('Yorig')

		def updateXfinal(self):
			if self.anchorList_selected:
				self.Xfinal = self.Xorig + self.Xshift
				self.d.PutValue('Xfinal')

		def updateYfinal(self):
			if self.anchorList_selected:
				self.Yfinal = self.Yorig + self.Yshift
				self.d.PutValue('Yfinal')


		def update_glyph(self):
			if self.anchorList_selected:
				if self.gIndex not in glyphBkupDict:
#					print "Made backup copy of '%s'" % self.glyph.name
					glyphBkupDict[self.gIndex] = Glyph(f[self.gIndex])
				fl.SetUndo(self.gIndex)
				x = self.Xfinal
				y = self.Yfinal
				anchorPosition = Point(x, y)
				anchorIndex = self.anchorList_index
				anchor = self.glyph.anchors[anchorIndex]
				# In single master fonts the adjustment of the anchors cannot be handled by the codepath used for multiple
				# master fonts, because the UI gets updated but the changes are not stored in the VFB file upon saving.
				if masters == 1:
					anchor.x = x
					anchor.y = y
				else:
					anchor.SetLayer(fl.layer, anchorPosition)
				fl.UpdateGlyph(self.gIndex)

		def Run(self):
			return self.d.Run()
	

	d = DialogClass()

	if d.Run() == 1:
		f.modified = 1
	else:
		for gID in glyphBkupDict:
			f[gID] = glyphBkupDict[gID]
			fl.UpdateGlyph(gID)
		f.modified = 0


if __name__ == "__main__":
	f = fl.font
	gIndex = fl.iglyph
	if f is None:
		fl.Message('No font opened')
	elif gIndex < 0:
		if len(listGlyphsSelected) == 0:
			fl.Message('Glyph selection is not valid')
		else:
			gIndex = listGlyphsSelected[0]
			run(gIndex)
	else:
		run(gIndex)
