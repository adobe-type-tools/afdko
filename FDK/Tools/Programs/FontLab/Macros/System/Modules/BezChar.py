#FLM: Library routine - bez format conversion
######################################################################

__copyright__ =  """
Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0.
"""

__doc__ = """
BezChar.py v1.3 May 18 2009

This module converts between a FontLab glyph and a bez file data string. Used
by the OutlineCheck and AutoHint scripts, to convert FL glyphs to bez programs 
as needed by C librarues that do the hard work.

Methods, Bez To Glyph:
Use a simple tokenizer, as there is no nesting or control logic or state
(beyond the current pt) to deal with. Simply get the next non-whitespace
token, and deal with it, while there is another token. 

We will use only the actual drawing commands, width and bearing. Hint
info will be discarded. 

For each bez file token:

Type	Action
Number	Convert from ascii to number, and push value on argList. Numbers may be int or fractions.
div		divide n-2 token by n-1 token in place in the stack, and pop off n-1 token.
sc		Start path command. Start new contour. Set CurX,Y to  (0,0)
cp		Close path. Do a Contour.close(). Set pt index to zero. Note: any drawing command
 		for Pt 0 will also start a new path.
rmt		Relative moveto. Do move to CurX + n-1, CurY + n. 
hmt		Relative h moveto. Do move to CurX + n. 
vmt		Relative v moveto. Do move to  CurY + n. 

vdt		Relative v line to. Do line-to CurY + n. Set start/end points as a corner pt.
hdt		Relative h line to. Do line-to CurX + n  Set start/end points as a corner pt.
rdt		Relative line to.  Do line-to CurX + n-1, CurY + n.  Set start/end points as a corner pt.

rct		Relative curve to. Calculate:
								dx1 = n-5, dy1 = n-4
								dx2 = dx1 + n-3, dy2 = dy1 + n-2
								dx3 = dx2 + n-1, dy3 = dx2 + n
								Do CurveTo (CurX + dx1, CurY + dy1),
										 (CurX +dx2, CurY + dy2)
										 (CurX + x3, CurY + dx3)
										 Update CurX += dx3, CurY += dx3.
										 Set new pt to be a curve pt.
vhct	Relative vertical horizontal curve to. Calculate:
								dx1 = 0, dy1 = n-3
								dx2 = n-2, dy2 = dy1 + n-1
								dx3 = dx2 + n, dy3 = dy2
								Do Curve to as above.
hvct	Relative horizontal vertical curve to.
								dx1 = n-3, dy1 = 0
								dx2 = dx1 + n-2, dy2 = n-1
								dx3 = dx2, dy3 = dy2 +n
								Do Curve to as above.

prflx1	Start of Type1 style flex ommands. Discard all
flx		Flex coommand. Push back on stack as to rct's.

rb, ry, rm, rv Stem hint, stem3 hint commands. Discard args and command.

sol/eol	Dot Sections. Discard these and arg list.
beginsubr/endsubr Hint replacement subroutine block. Discard
snc/enc	 hint replacement block. Discarded.
newcolors	end of hint replacement block. Discard.
id		Discard
"""

######################################################################
# Imports

from FL import *
import os
import string
import re

######################################################################
# Exceptions



#############################
# Fixed names and assumptions

gDebug = 0

verifyhints = 0 # These should be set to 1 only when testing if hints into Ac match hints out from AC.
verifyhintsubs = 0 
######################################################################
# functions


def DebugPrint(line):
	global gDebug
	
	if gDebug:
		print 'DBG: ' + str(line)
	return None



def FindHint(newhint, hints):
	# Used in ExtractHints to check if a hint value has already been seen.
	for i in range(0, len(hints)):
		hint=hints[i]
		if hint.position==newhint.position and hint.width==newhint.width:
			return i
	return -1

def ExtractHints(inlines, glyph):
	# Extract the hint values and replacement table from the bez format outline data.
	newhhints=[]
	newvhints=[]
	newreplacetable=[]
	stack=[]
	nodeindex=-1
	hasSubstitution=0
	discardPreFlexOps = 0
	
	for line in inlines:
		tokens=string.split(line)
		for token in tokens:
		
			if  token in ["preflx1", "preflx2"]:
				discardPreFlexOps = 1
				continue
			elif (token == "flex"): # arg stack is the 6 rcurveto points. 
				discardPreFlexOps = 0
				nodeindex=nodeindex+2
				stack=stack[:-12]
				nodeindex=nodeindex+1

			if discardPreFlexOps:
				stack=[]
				continue

			if token[0]=='%':
				break
			try:
				value=string.atoi(token)
				stack.append(value)
			except ValueError:
				if token=="div":
					result = stack[-2]/stack[-1]
					if result<0:
						result = result + 1
					stack = stack[:-2]
					stack.append(result)
				elif token=="beginsubr":
					pass
				elif token=="endsubr":
					pass
				elif token=="snc":
					newR=Replace(255, nodeindex+1)
					newreplacetable.append(newR)
					hasSubstitution=1
				elif token=="enc":
					pass
				elif token=="newcolors":
					pass
				elif token=="sc":
					pass
				elif token=="cp":
					pass
				elif token=="ed":
					pass
				elif token=="rmt":
					stack=stack[:-2]
					nodeindex=nodeindex+1
				elif token=="hmt":
					stack=stack[:-1]
					nodeindex=nodeindex+1
				elif token=="rdt":
					stack=stack[:-2]
					nodeindex=nodeindex+1
				elif token=="rct":
					nodeindex=nodeindex+1
				elif token=="rcv":
					nodeindex=nodeindex+1
				elif token=="vmt":
					stack=stack[:-1]
					stack=stack[:-6]
					nodeindex=nodeindex+1
				elif token=="hvct":
					stack=stack[:-4]
					nodeindex=nodeindex+1
				elif token=="vhct":
					stack=stack[:-4]
					nodeindex=nodeindex+1
				elif token=="vdt":
					stack=stack[:-1]
					nodeindex=nodeindex+1
				elif token=="hdt":
					stack=stack[:-1]
					nodeindex=nodeindex+1
				elif token=="ry" or token=="rm":
					newhint=Hint(stack[-2], stack[-1])
					hindex=FindHint(newhint, newvhints)
					if hindex==-1:
						newvhints.append(newhint)
						hindex=len(newvhints)-1
					if not newreplacetable:
						newR=Replace(255, nodeindex+1)
						newreplacetable.append(newR)
					newR=Replace(2, hindex)
					newreplacetable.append(newR)
					stack=stack[:-2]
				elif token=="rb" or token=="rv":
					newhint=Hint(stack[-2], stack[-1])
					hindex=FindHint(newhint, newhhints)
					if hindex==-1:
						newhhints.append(newhint)
						hindex=len(newhhints)-1
					if not newreplacetable:
						newR=Replace(255, nodeindex+1)
						newreplacetable.append(newR)
					newR=Replace(1, hindex)
					newreplacetable.append(newR)
					stack=stack[:-2]
				else:
					print("Error when extracting new hints: unknown bez operator <%s> for glyph <%s>: " % (token,glyph.name) )
					return	

	if not hasSubstitution:
		newreplacetable=[]
	
	# Test clauses for checking if new AC works the same as the old.
	# Verify the hints. Used to make sure that a newer version of AC produces the same results ans an older version.
	if(verifyhints):
		if len(glyph.hhints) != len(newhhints):
			DebugPrint( glyph.name)
			DebugPrint( glyph.hhints)
			DebugPrint( newhhints)
		else:
			for hint in newhhints:
				if FindHint(hint, glyph.hhints)==-1:
					DebugPrint( glyph.name)
					DebugPrint( glyph.hhints)
					DebugPrint( newhhints)
					break
		
		if len(glyph.vhints) != len(newvhints):
			DebugPrint( glyph.name)
			DebugPrint( glyph.vhints)
			DebugPrint( newvhints)
		else:
			for hint in newvhints:
				if FindHint(hint, glyph.vhints)==-1:
					DebugPrint( glyph.name)
					DebugPrint( glyph.vhints)
					DebugPrint( newvhints)
					break
	
	if(verifyhintsubs):
		if len(glyph.replace_table) != len(newreplacetable):
			DebugPrint( glyph.name)
			DebugPrint("")
			DebugPrint( glyph.replace_table)
			DebugPrint( newreplacetable)
		else:
			for i in range(0, len(newreplacetable)):
				if newreplacetable[i].type!=glyph.replace_table[i].type or newreplacetable[i].index!=glyph.replace_table[i].index:
					DebugPrint( glyph.name)
					DebugPrint(" ")
					DebugPrint( glyph.replace_table)
					DebugPrint( newreplacetable)
					break

	return (newreplacetable, newhhints, newvhints)



def MakeGlyphNodesFromBez(glyphName, bezData):
	""" Draw un-encrypted bez data stream to empty char."""
	# init current drawing point.
	CurX = 0
	CurY = 0
	newPath = 1
	nodes = []
	
	ok = 1

	try:
		bezData =  re.sub(r"%.+?([\n\r])", r"\1", bezData) # supress comments
	except:
		print "Error in ", glyphName
		print "BezData", repr(bezData)
		return ""

	bezTokens = re.findall(r"(\S+)", bezData)
	if not bezTokens:
		return ""

	gArgumentStack = []
	numTokens = len(bezTokens)
	discardPreFlexOps = 0

	i = 0
	while i < numTokens:
		# If it is a number, push it on the stack
		arg = bezTokens[i]
		i = i + 1
		try:
			argnum = int(arg)
			gArgumentStack.append(argnum)
			continue
		except ValueError:
			pass

		if  arg in ["preflx1", "preflx2"]:
			discardPreFlexOps = 1
			continue
		elif (arg == "flex"): # arg stack is the 6 rcurveto points. Insert them and let the regular processing  take care of them.
			discardPreFlexOps = 0
			bezTokens[i:i] = gArgumentStack[0:6] + ["rct"] +  gArgumentStack[6:11] + ["rct"] 

		if discardPreFlexOps:
			gArgumentStack = []
			continue

		lineType = ""

		if arg[-2:] == "mt": #It's a move to command.
			lineType = nMOVE
			if arg == "rmt":
				x0 = CurX + gArgumentStack[-2]
				y0 = CurY + gArgumentStack[-1]
			elif arg == "hmt":
				x0 = CurX + gArgumentStack[-1]
				y0 = CurY
			elif arg == "vmt":
				x0 = CurX
				y0 = CurY + gArgumentStack[-1]
				
		elif (arg[-2:] == "ct") or (arg[-2:] == "cv") : #It's a curve-to command.
			lineType = nCURVE
			if 	(arg == "rct") or (arg == "rcv"):
				# (x1,y1) is first bezier control point, (x2,y2) is second  bezier control point point, (x0,y0) is final point.
				x1 = CurX + gArgumentStack[0]
				y1 = CurY + gArgumentStack[1]
				x2 = x1 + gArgumentStack[2]
				y2 = y1 + gArgumentStack[3]
				x0 = x2 + gArgumentStack[4]
				y0 = y2 + gArgumentStack[5]
			elif arg == "vhct":
				x1 = CurX
				y1 = CurY + gArgumentStack[0]
				x2 = x1 + gArgumentStack[1]
				y2 = y1 + gArgumentStack[2]
				x0 = x2 + gArgumentStack[3]
				y0 = y2
			elif arg == "hvct":
				x1 = CurX + gArgumentStack[0]
				y1 = CurY
				x2 = x1 + gArgumentStack[1]
				y2 = y1 + gArgumentStack[2]
				x0 = x2
				y0 = y2 + gArgumentStack[3]

		elif (arg[-2:] == "dt"): #It's a line-to command.
			lineType = nLINE
			if arg == "rdt":
				x0 = CurX + gArgumentStack[-2]
				y0 = CurY + gArgumentStack[-1]
			elif arg == "hdt":
				x0 = CurX + gArgumentStack[-1]
				y0 = CurY
			elif arg == "vdt":
				x0 = CurX
				y0 = CurY + gArgumentStack[-1]

		elif (arg == "div"):
			# Yes, I know that this will discard the fractional part, but FontLab doesn't support fractions
			# Note that so far I have seen this used only in hint values, which are discarded here.
			frac = gArgumentStack[-2]/gArgumentStack[-1]
			gArgumentStack = gArgumentStack[:-2] + [frac]
			
		elif arg == "preflx1":
			# the preflx1/  preflx2 sequence provides the same i as the flex sequence; the difference is that the 
			#  preflx1/preflx2 sequence provides the argument values needed for building a Type1 string
			# while the flex sequence is simply the 6 rcurveto points. Both sequences are always provided.
			pass
			
		elif arg == "preflx2":
			pass

		elif arg == "flx":
			lineType = nCURVE
			# (x1,y1) is first bezier control point, (x2,y2) is second  bezier control point point, (x0,y0) is final point.
			x1 = CurX + gArgumentStack[0]
			y1 = CurY + gArgumentStack[1]
			x2 = x1 + gArgumentStack[2]
			y2 = y1 + gArgumentStack[3]
			x0 = x2 + gArgumentStack[4]
			y0 = y2 + gArgumentStack[5]

		else: # All the other commands we throw away.
			if arg in ["cp", "sc", "ry", "rb", "rm","rv", "sol", "eol", "snc","enc","beginsubr","endsubs","endsubr","newcolors", "ed", "id"]:
				pass
			else:
				print "An unknown command <" + arg + ">!"

		gArgumentStack = []
		if lineType:
			p= Point(x0, y0)
			n = Node(lineType, p)
			if lineType == nCURVE:
				n[1] = Point(x1, y1)
				n[2] = Point(x2, y2)
			CurX = x0
			CurY = y0
			nodes =  nodes + [n]
				
					
	return nodes


def ConvertFLGlyphToBez(glyph, layer=1):
	# ConvertFontLab glyph data to a bez format outline program.
	outstring=""
	outstring = outstring + "% "+glyph.name+"\n" # we use \n rather than os.linesep because the AClib expects
												# line endings to be \n .
	outstring = outstring + "sc\n"
	currx=0
	curry=0
	starteddrawing=0
	for node in glyph.nodes:
		if layer > 1:
			nodePoints = node.Layer(layer)
		else:
			nodePoints = node.points
		finalPoint = nodePoints[0]

		if node.type==nMOVE:
			if starteddrawing:
				outstring = outstring + "cp\n"
			if finalPoint.x==currx:
				outstring = outstring + str(finalPoint.y-curry) + " vmt\n"
			elif finalPoint.y==curry:
				outstring = outstring + str(finalPoint.x-currx) + " hmt\n"
			else: 	
				outstring = outstring + str(finalPoint.x-currx)+" "+str(finalPoint.y-curry)+" rmt\n"
			currx=finalPoint.x
			curry=finalPoint.y
		elif node.type==nLINE:
		  	starteddrawing=1
			if currx==finalPoint.x:
				outstring = outstring + str(finalPoint.y-curry)+ " vdt\n"
			elif curry==finalPoint.y:
				outstring = outstring + str(finalPoint.x-currx)+ " hdt\n"
			else:
				outstring = outstring + str(finalPoint.x-currx)+ " " + str(finalPoint.y-curry)+ " rdt\n"
			currx=finalPoint.x
			curry=finalPoint.y
		elif node.type==nCURVE:
			starteddrawing=1
			if currx==nodePoints[1].x and nodePoints[2].y==nodePoints[0].y:
				outstring = outstring + str(nodePoints[1].y-curry)+" "+str(nodePoints[2].x-nodePoints[1].x)+" "
				outstring = outstring + str(nodePoints[2].y-nodePoints[1].y)+" "+str(nodePoints[0].x-nodePoints[2].x)+" vhct\n"
				currx=nodePoints[0].x
				curry=nodePoints[0].y
			elif  curry==nodePoints[1].y and nodePoints[2].x==nodePoints[0].x:
				outstring = outstring + str(nodePoints[1].x-currx)+" "+str(nodePoints[2].x-nodePoints[1].x)+" "
				outstring = outstring + str(nodePoints[2].y-nodePoints[1].y)+" "+str(nodePoints[0].y-nodePoints[2].y)+" hvct\n"
				currx=nodePoints[0].x
				curry=nodePoints[0].y
			else:
			  	outstring = outstring + str(nodePoints[1].x-currx)+" "+str(nodePoints[1].y-curry)+" "+str(nodePoints[2].x-nodePoints[1].x)+" "
				outstring = outstring + str(nodePoints[2].y-nodePoints[1].y)+" "+str(nodePoints[0].x-nodePoints[2].x)+" "+str(nodePoints[0].y-nodePoints[2].y)+" rct\n"
				currx=nodePoints[0].x
				curry=nodePoints[0].y
		elif node.type==nOFF:
			return 1, ""
		else:
		  	raise SyntaxError( "Error when converting glyphs to hinting (bez) format: Unknown node type= <%s> in glyph <%s>." % (str(node.type), glyph.name))
	if starteddrawing:
		outstring = outstring + "cp\n"
	outstring = outstring + "ed\n"
	return outstring

	

def GetACFontInfoFromFLFont(font):
	# The AC library needs the global font hint zones and standard stem widths. Collect it
	# into a text string.
	# The text format is arbitrary, inherited from very old software, but there is no real need to change it.
  	fontinfo=[]
	fontinfo.append("OrigEmSqUnits")
	fontinfo.append(str(font.upm) ) # OrigEmSqUnits

	fontinfo.append("FontName")
	fontName = font.font_name
	if not fontName:
		fontName = "Font-Undefined"
	fontinfo.append(fontName ) # FontName

	fontinfo.append("FlexOK")
	fontinfo.append("true")
	
	if hasattr(font, "blue_values") and (len(font.blue_values) > 0)  and (len(font.blue_values[0]) > 0):
		fontinfo.append("BaselineOvershoot")
		fontinfo.append(str(font.blue_values[0][0])) # BaselineOvershoot
	
		if len(font.blue_values[0])>1:
			fontinfo.append("BaselineYCoord")
			fontinfo.append(str(font.blue_values[0][1])) # BaselineYCoord
	
	
		if len(font.blue_values[0])>2:
			fontinfo.append("LcHeight")
			fontinfo.append(str(font.blue_values[0][2]) ) # LcHeight
			
			fontinfo.append("LcOvershoot")
			fontinfo.append(str(font.blue_values[0][3]-font.blue_values[0][2]),) # LcOvershoot
	
		if len(font.blue_values[0])>4:
			fontinfo.append("CapHeight")
			fontinfo.append(str(font.blue_values[0][4]), ) # CapHeight
			
			fontinfo.append("CapOvershoot")
			fontinfo.append(str(font.blue_values[0][5]-font.blue_values[0][4]),) # CapOvershoot
		else:
			fontinfo.append("CapOvershoot")
			fontinfo.append("0") #CapOvershoot = 0 needs a value!
	
		if len(font.blue_values[0])>6:
			fontinfo.append("AscenderHeight")
			fontinfo.append(str(font.blue_values[0][6]), ) # AscenderHeight
			
			fontinfo.append("AscenderOvershoot")
			fontinfo.append(str(font.blue_values[0][7]-font.blue_values[0][6]),) # AscenderOvershoot
	
		if len(font.blue_values[0])>8:
			fontinfo.append("FigHeight")
			fontinfo.append(str(font.blue_values[0][8]), ) # FigHeight
			
			fontinfo.append("FigOvershoot")
			fontinfo.append(str(font.blue_values[0][9]-font.blue_values[0][8]),) # FigOvershoot
	else:
		# garbage values that will permit the program to run.
		fontinfo.append("BaselineOvershoot")
		fontinfo.append("-250") # BaselineOvershoot
		fontinfo.append("BaselineYCoord")
		fontinfo.append("-250") # BaselineYCoord
		fontinfo.append("CapHeight")
		fontinfo.append("1100") # BaselineYCoord
		fontinfo.append("AscenderHeight")
		fontinfo.append("1100") # BaselineYCoord
	
	if hasattr(font, "other_blues") and len(font.other_blues[0])>0:
		fontinfo.append("DescenderHeight")
		fontinfo.append(str(font.other_blues[0][1]), ) # DescenderHeight
		
		fontinfo.append("DescenderOvershoot")
		fontinfo.append(str(font.other_blues[0][0]-font.other_blues[0][1]),) # DescenderOvershoot

	if len(font.other_blues[0])>2:
		fontinfo.append("Baseline5")
		fontinfo.append(str(font.other_blues[0][3]), ) # Baseline5
		
		fontinfo.append("Baseline5Overshoot")
		fontinfo.append(str(font.other_blues[0][2]-font.other_blues[0][3]),) # Baseline5Overshoot

	if len(font.other_blues[0])>4:
		fontinfo.append("SuperiorBaseline")
		fontinfo.append(str(font.other_blues[0][5]), ) # SuperiorBaseline
		
		fontinfo.append("SuperiorOvershoot")
		fontinfo.append(str(font.other_blues[0][4]-font.other_blues[0][5]),) # SuperiorOvershoot

	
	fontinfo.append("DominantV")
	tempstring="["
	for value in font.stem_snap_v[0]:
		tempstring=tempstring+str(value)+" "
	tempstring=tempstring+"]"
	fontinfo.append(tempstring)
	
	fontinfo.append("StemSnapV")
	fontinfo.append(tempstring)
	
	fontinfo.append("DominantH")
	tempstring="["
	for value in font.stem_snap_h[0]:
		tempstring=tempstring+str(value)+" "
	tempstring=tempstring+"]"
	fontinfo.append(tempstring)
	
	fontinfo.append("StemSnapH")
	fontinfo.append(tempstring)

	fontinfoStr = " ".join(fontinfo)
	return fontinfoStr

