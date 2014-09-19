/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)glyf.h	1.1
 * Changed:    4/13/95 10:15:22
 ***********************************************************************/

/*
 * Glyph data table format definition.
 */

#ifndef FORMAT_GLYF_H
#define FORMAT_GLYF_H

typedef struct
	{
	Card16 *endPtsOfContours;
	Card16 instructionLength;
	Card8 *instructions;
	Card8 *flags;
#define ONCURVE			(1<<0)
#define XSHORT			(1<<1)
#define YSHORT			(1<<2)
#define REPEAT			(1<<3)
#define SHORT_X_IS_POS	(1<<4)
#define NEXT_X_IS_ZERO	(1<<4)
#define SHORT_Y_IS_POS	(1<<5)
#define NEXT_Y_IS_ZERO	(1<<5)
	Int16 *xCoordinates;
	Int16 *yCoordinates;
	} Simple;

typedef struct
	{
	Card16 flags;
#define ARG_1_AND_2_ARE_WORDS		(1<<0)
#define ARGS_ARE_XY_VALUES			(1<<1)
#define ROUND_XY_TO_GRID			(1<<2)
#define WE_HAVE_A_SCALE				(1<<3)
#define NON_OVERLAPPING				(1<<4)	/* Apparently obsolete */
#define MORE_COMPONENTS				(1<<5)
#define WE_HAVE_AN_X_AND_Y_SCALE	(1<<6)
#define WE_HAVE_A_TWO_BY_TWO		(1<<7)
#define WE_HAVE_INSTRUCTIONS		(1<<8)
#define USE_MY_METRICS				(1<<9)
	Card16 glyphIndex;
	Int16 arg1;
	Int16 arg2;
	F2Dot14 transform[2][2];
	Card16 instructionLength;
	Card8 *instructions;
	} Component;

typedef struct
	{
	Component *component;
	} Compound;

typedef struct
	{
	Int16 numberOfContours;
	FWord xMin;
	FWord yMin;
	FWord xMax;
	FWord yMax;
	void *data;
	} Glyph;

typedef struct
	{
	Glyph *glyph;
	} glyfTbl;

#endif /* FORMAT_GLYF_H */
