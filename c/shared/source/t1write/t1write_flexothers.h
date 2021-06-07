/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

"/OtherSubrs[systemdict/internaldict known{1183615869 systemdict/internaldict\n"
"get exec/FlxProc known{save true}{false}ifelse}{userdict/internaldict known\n"
"not{userdict/internaldict{count 0 eq{/internaldict errordict/invalidaccess get\n"
"exec}if dup type/integertype ne{/internaldict errordict/invalidaccess get exec\n"
"}if dup 1183615869 eq{pop 0}{/internaldict errordict/invalidaccess get exec}\n",
"ifelse}dup 14 get 1 25 dict put bind executeonly put}if 1183615869 userdict\n"
"/internaldict get exec/FlxProc known{save true}{false}ifelse}ifelse[systemdict\n"
"/internaldict known not{100 dict/begin cvx/mtx matrix/def cvx}if systemdict\n"
"/currentpacking known{currentpacking true setpacking}if{systemdict\n"
"/internaldict known{1183615869 systemdict/internaldict get exec dup/$FlxDict\n"
"known not{dup dup length exch maxlength eq{pop userdict dup/$FlxDict known not\n",
"{100 dict begin/mtx matrix def dup/$FlxDict currentdict put end}if}{100 dict\n"
"begin/mtx matrix def dup/$FlxDict currentdict put end}ifelse}if/$FlxDict get\n"
"begin}if grestore/exdef{exch def}def/dmin exch abs 100 div def/epX exdef/epY\n"
"exdef/c4y2 exdef/c4x2 exdef/c4y1 exdef/c4x1 exdef/c4y0 exdef/c4x0 exdef/c3y2\n"
"exdef/c3x2 exdef/c3y1 exdef/c3x1 exdef/c3y0 exdef/c3x0 exdef/c1y2 exdef/c1x2\n"
"exdef/c2x2 c4x2 def/c2y2 c4y2 def/yflag c1y2 c3y2 sub abs c1x2 c3x2 sub abs gt\n",
"def/PickCoords{{c1x0 c1y0 c1x1 c1y1 c1x2 c1y2 c2x0 c2y0 c2x1 c2y1 c2x2 c2y2}{\n"
"c3x0 c3y0 c3x1 c3y1 c3x2 c3y2 c4x0 c4y0 c4x1 c4y1 c4x2 c4y2}ifelse/y5 exdef/x5\n"
"exdef/y4 exdef/x4 exdef/y3 exdef/x3 exdef/y2 exdef/x2 exdef/y1 exdef/x1 exdef\n"
"/y0 exdef/x0 exdef}def mtx currentmatrix pop mtx 0 get abs 1e-05 lt mtx 3 get\n"
"abs 1e-05 lt or{/flipXY -1 def}{mtx 1 get abs 1e-05 lt mtx 2 get abs 1e-05 lt\n"
"or{/flipXY 1 def}{/flipXY 0 def}ifelse}ifelse/erosion 1 def systemdict\n",
"/internaldict known{1183615869 systemdict/internaldict get exec dup/erosion\n"
"known{/erosion get/erosion exch def}{pop}ifelse}if yflag{flipXY 0 eq c3y2 c4y2\n"
"eq or{false PickCoords}{/shrink c3y2 c4y2 eq{0}{c1y2 c4y2 sub c3y2 c4y2 sub\n"
"div abs}ifelse def/yshrink{c4y2 sub shrink mul c4y2 add}def/c1y0 c3y0 yshrink\n"
"def/c1y1 c3y1 yshrink def/c2y0 c4y0 yshrink def/c2y1 c4y1 yshrink def/c1x0\n"
"c3x0 def/c1x1 c3x1 def/c2x0 c4x0 def/c2x1 c4x1 def/dY 0 c3y2 c1y2 sub round\n",
"dtransform flipXY 1 eq{exch}if pop abs def dY dmin lt PickCoords y2 c1y2 sub\n"
"abs .001 gt{c1x2 c1y2 transform flipXY 1 eq{exch}if/cx exch def/cy exch def/dY\n"
"0 y2 c1y2 sub round dtransform flipXY 1 eq{exch}if pop def dY round dup 0 ne{\n"
"/dY exdef}{pop dY 0 lt{-1}{1}ifelse/dY exdef}ifelse/erode PaintType 2 ne\n"
"erosion .5 ge and def erode{/cy cy .5 sub def}if/ey cy dY add def/ey ey\n"
"ceiling ey sub ey floor add def erode{/ey ey .5 add def}if ey cx flipXY 1 eq{\n",
"exch}if itransform exch pop y2 sub/eShift exch def/y1 y1 eShift add def/y2 y2\n"
"eShift add def/y3 y3 eShift add def}if}ifelse}{flipXY 0 eq c3x2 c4x2 eq or{\n"
"false PickCoords}{/shrink c3x2 c4x2 eq{0}{c1x2 c4x2 sub c3x2 c4x2 sub div abs}\n"
"ifelse def/xshrink{c4x2 sub shrink mul c4x2 add}def/c1x0 c3x0 xshrink def/c1x1\n"
"c3x1 xshrink def/c2x0 c4x0 xshrink def/c2x1 c4x1 xshrink def/c1y0 c3y0 def\n"
"/c1y1 c3y1 def/c2y0 c4y0 def/c2y1 c4y1 def/dX c3x2 c1x2 sub round 0 dtransform\n",
"flipXY -1 eq{exch}if pop abs def dX dmin lt PickCoords x2 c1x2 sub abs .001 gt\n"
"{c1x2 c1y2 transform flipXY -1 eq{exch}if/cy exch def/cx exch def/dX x2 c1x2\n"
"sub round 0 dtransform flipXY -1 eq{exch}if pop def dX round dup 0 ne{/dX\n"
"exdef}{pop dX 0 lt{-1}{1}ifelse/dX exdef}ifelse/erode PaintType 2 ne erosion\n"
".5 ge and def erode{/cx cx .5 sub def}if/ex cx dX add def/ex ex ceiling ex sub\n"
"ex floor add def erode{/ex ex .5 add def}if ex cy flipXY -1 eq{exch}if\n",
"itransform pop x2 sub/eShift exch def/x1 x1 eShift add def/x2 x2 eShift add\n"
"def/x3 x3 eShift add def}if}ifelse}ifelse x2 x5 eq y2 y5 eq or{x5 y5 lineto}{\n"
"x0 y0 x1 y1 x2 y2 curveto x3 y3 x4 y4 x5 y5 curveto}ifelse epY epX}systemdict\n"
"/currentpacking known{exch setpacking}if/exec cvx/end cvx]cvx executeonly exch\n"
"{pop true exch restore}{systemdict/internaldict known not{1183615869 userdict\n"
"/internaldict get exec exch/FlxProc exch put true}{1183615869 systemdict\n",
"/internaldict get exec dup length exch maxlength eq{false}{1183615869\n"
"systemdict/internaldict get exec exch/FlxProc exch put true}ifelse}ifelse}\n"
"ifelse{systemdict/internaldict known{{1183615869 systemdict/internaldict get\n"
"exec/FlxProc get exec}}{{1183615869 userdict/internaldict get exec/FlxProc get\n"
"exec}}ifelse executeonly}if{gsave currentpoint newpath moveto}executeonly{\n"
"currentpoint grestore gsave currentpoint newpath moveto}executeonly{systemdict\n",
"/internaldict known not{pop 3}{1183615869 systemdict/internaldict get exec dup\n"
"/startlock known{/startlock get exec}{dup/strtlck known{/strtlck get exec}{pop\n"
"3}ifelse}ifelse}ifelse}executeonly]def\n"
