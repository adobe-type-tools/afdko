/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Abstract font handling.
 */

#include "absfont.h"
#include "dictops.h"

#include <stdlib.h>
#include <math.h>

#define ARRAY_LEN(a) 	(sizeof(a)/sizeof((a)[0]))

/* Hint zone pair */
typedef struct
	{
	float lo;
	float hi;
	int top;		/* Flags top zone */
	} ZonePair;

static abfString unsetString =
	{
	ABF_UNSET_PTR,
	ABF_UNSET_INT,
	};

/* Initialize top dictionary. */
void abfInitTopDict(abfTopDict *top)
	{
	top->version					= unsetString;
	top->Notice						= unsetString;
	top->Copyright					= unsetString;
	top->FullName					= unsetString;
	top->FamilyName					= unsetString;
	top->Weight						= unsetString;
	top->isFixedPitch				= cff_DFLT_isFixedPitch;
	top->ItalicAngle				= cff_DFLT_ItalicAngle;
	top->UnderlinePosition 			= cff_DFLT_UnderlinePosition;
	top->UnderlineThickness 		= cff_DFLT_UnderlineThickness;
	top->UniqueID					= ABF_UNSET_INT;
	top->FontBBox[0]				= 0.0;
	top->FontBBox[1]				= 0.0;
	top->FontBBox[2]				= 0.0;
	top->FontBBox[3]				= 0.0;
	top->StrokeWidth				= cff_DFLT_StrokeWidth;
	top->XUID.cnt					= ABF_EMPTY_ARRAY;

	/* CFF and Acrobat extensions */
	top->PostScript					= unsetString;
	top->BaseFontName				= unsetString;
	top->BaseFontBlend.cnt			= ABF_EMPTY_ARRAY;
	top->FSType						= ABF_UNSET_INT;
	top->OrigFontType				= ABF_UNSET_INT;
	top->WasEmbedded				= 0;

	/* Synthetic font */
	top->SynBaseFontName			= unsetString;

	/* CID-keyed font extensions */
	top->cid.FontMatrix.cnt			= ABF_EMPTY_ARRAY;
	top->cid.CIDFontName			= unsetString;
	top->cid.Registry				= unsetString;
	top->cid.Ordering				= unsetString;
	top->cid.Supplement				= ABF_UNSET_INT;
	top->cid.CIDFontVersion 		= cff_DFLT_CIDFontVersion;
	top->cid.CIDFontRevision		= cff_DFLT_CIDFontRevision;
	top->cid.CIDCount				= cff_DFLT_CIDCount;
	top->cid.UIDBase				= ABF_UNSET_INT;

	/* Supplementary data */
	top->sup.flags					= 0;
	top->sup.srcFontType			= ABF_UNSET_INT;
	top->sup.filename				= ABF_UNSET_PTR;
	top->sup.UnitsPerEm				= 1000;
	top->sup.nGlyphs				= ABF_UNSET_INT;
	}

/* Initialize Private dictionary. */
static void initPrivateDict(abfPrivateDict *private)
	{
	private->BlueValues.cnt 		= ABF_EMPTY_ARRAY;
	private->OtherBlues.cnt 		= ABF_EMPTY_ARRAY;
	private->FamilyBlues.cnt 		= ABF_EMPTY_ARRAY;
	private->FamilyOtherBlues.cnt	= ABF_EMPTY_ARRAY;
	private->BlueScale 				= (float)cff_DFLT_BlueScale;
	private->BlueShift 				= cff_DFLT_BlueShift;
	private->BlueFuzz 				= cff_DFLT_BlueFuzz;
	private->StdHW 					= ABF_UNSET_REAL;
	private->StdVW 					= ABF_UNSET_REAL;
	private->StemSnapH.cnt			= ABF_EMPTY_ARRAY;
	private->StemSnapV.cnt			= ABF_EMPTY_ARRAY;
	private->ForceBold 				= cff_DFLT_ForceBold;
	private->LanguageGroup 			= cff_DFLT_LanguageGroup;
	private->ExpansionFactor 		= (float)cff_DFLT_ExpansionFactor;
	private->initialRandomSeed 		= cff_DFLT_initialRandomSeed;
	}

/* Initialize font dictionary. */
void abfInitFontDict(abfFontDict *font)
	{
	font->FontName					= unsetString;
	font->PaintType					= cff_DFLT_PaintType;
	font->FontMatrix.cnt			= ABF_EMPTY_ARRAY;
	initPrivateDict(&font->Private);
	}

/* Initialize all dictionary data. */
void abfInitAllDicts(abfTopDict *top)
	{
	long i;
	abfInitTopDict(top);
	for (i = 0; i < top->FDArray.cnt; i++)
		abfInitFontDict(&top->FDArray.array[i]);
	}

/* Initialize glyph info structure. */
void abfInitGlyphInfo(abfGlyphInfo *info)
	{
	info->flags			= 0;
	info->tag			= 0;
	info->gname.ptr		= ABF_UNSET_PTR;
	info->gname.impl	= ABF_UNSET_INT;
	info->encoding.code = ABF_GLYPH_UNENC;
	info->encoding.next = ABF_UNSET_PTR;
	info->cid 			= 0;
	info->iFD 			= 0;
	info->sup.begin		= ABF_UNSET_INT;
	info->sup.end		= ABF_UNSET_INT;
    info->blendInfo.numBlends = 0;
    info->blendInfo.vsindex = 0;
    info->blendInfo.blendDeltaArgs = NULL;
	}

/* Check UniqueID values is within 24-bit unsigned range. */
static void checkUniqueID(abfErrCallbacks *cb, long value, int err_code)
	{
	if (value == ABF_UNSET_INT)
		return;
	if (value < 0 || value > 16777215L)
		cb->report_error(cb, err_code, -1);
  	}

/* Validate top dict. */
static void checkTopDict(abfErrCallbacks *cb, abfTopDict *top)
	{
	if (cb == NULL)
		return;	/* Reporting turned off; nothing to do */

	/* Check reserved FSType bits are unset */
	if (top->FSType != ABF_UNSET_INT && (top->FSType & 0xfcf1))
		cb->report_error(cb, abfErrBadFSType, -1);

	/* Check UniqueID ranges */
	checkUniqueID(cb, top->UniqueID, abfErrBadUniqueID);
	if (top->sup.flags & ABF_CID_FONT)
		checkUniqueID(cb, top->cid.UIDBase, abfErrBadUIDBase);

	/* Check UnderlinePosition and UnderlineThickness */
	if (top->UnderlinePosition > 0)
		cb->report_error(cb, abfErrBadULPosition, -1);
	if (top->UnderlineThickness <= 0)
		cb->report_error(cb, abfErrBadULThickness, -1);
	}

/* Check and repair bad boolean values. */
static void checkBooleanValue(abfErrCallbacks *cb, long *value, 
							  int err_code, int iFD)
	{
	if (*value == 0 || *value == 1)
		return;

	/* Invalid value; force to 0 */
	*value = 0;
	if (cb != NULL)
		cb->report_error(cb, err_code, iFD);
	}

/* Validate hint zones array. If zones are invalid try
 * to continue and let the interpreter workaround the
 * problem unless the optional flag is set. If the
 * optional flag is set to 1 it means that this is an
 * optional keyword in the font so if an error is found
 * the array cnt is set to ABF_EMPTY_ARRAY which means
 * the array is not included in the font.
 */
static int checkZoneArray(abfErrCallbacks *cb, 
						  int badarray, int bigvalue, long *cnt, float *array, 
						  abfTopDict *top, int iFD, long optional)
	{
	if (*cnt == ABF_EMPTY_ARRAY)
		return 0;
	else if (*cnt & 1)
		{
		/* Unpaired value */
		cb->report_error(cb, badarray, iFD);
		if (optional) *cnt = ABF_EMPTY_ARRAY;
		return 0;
		}
	else
		{
		long i;
		for (i = 0; i < *cnt; i += 2)
			if (array[i + 0] > array[i + 1])
				{
				/* Misordered pair */
				cb->report_error(cb, badarray, iFD);
				if (optional) *cnt = ABF_EMPTY_ARRAY;
				return 0;
				}
		for (i = 0; i < *cnt; i++)
			if (fabs(array[i]) > top->sup.UnitsPerEm*4.0f)
				/* Suspiciously large value */
				cb->report_error(cb, bigvalue, iFD);
		return 0;
		}
	}

/* Validate StemSnap array and Std value. */
static void checkStemSnap(abfErrCallbacks *cb, abfTopDict *top,
						  int nostd, int badstd, int badstemsnap,
						  float *std, long *cnt, float *array, int iFD)
	{
	long i;

	if (*cnt != ABF_EMPTY_ARRAY)
		{ /* Check that StemSnap elements are in order */
		for (i = 1; i < *cnt; i++)
			if (array[i - 1] >= array[i])
				{
				cb->report_error(cb, badstemsnap, iFD);
				/* Don't write out this array if it's invalid. */
				*cnt = ABF_EMPTY_ARRAY;
				break;
				}
		}

	if (*std == ABF_UNSET_REAL)
		{
		if (*cnt != ABF_EMPTY_ARRAY)
		{
			cb->report_error(cb, nostd, iFD);
			*std = array[0];
		}
		return;
		}
	else if (*std < 0 || *std > top->sup.UnitsPerEm/2.0f)
		{
		cb->report_error(cb, badstd, iFD);
		*std = (*cnt != ABF_EMPTY_ARRAY) ? array[0] : 0; /* force stem value to a legal size */
		}
	
	if (*cnt == ABF_EMPTY_ARRAY)
		return;

	/* Check std value represented */
	for (i = 0; i < *cnt; i++)
		if (array[i] == *std)
			return;

	/* Std value isn't in StemSnap array so force
	 * it to the value of the first element.
	 */
	*std = array[0];

	cb->report_error(cb, badstd, iFD);
	}

/* Compare zone pair. */
static int CTL_CDECL cmpZonePair(const void *first, const void *second)
	{
	const ZonePair *a = first;
	const ZonePair *b = second;
	if (a->lo < b->lo)
		return -1;
	else if (a->lo > b->lo)
		return 1;
	else
		return 0;
	}
		
/* Validate hint zones. */
static void checkZones(abfErrCallbacks *cb, 
					   int overlap,
					   long bluescnt, float *bluesarray,
					   long othercnt, float *otherarray,
					   abfTopDict *top, abfPrivateDict *private, int iFD)
	{
	long i;
	long cnt;
	ZonePair *last;
	ZonePair array[7 + 5];
	float max;

	if (bluescnt == ABF_EMPTY_ARRAY && othercnt == ABF_EMPTY_ARRAY)
		return;

	/* Combine and sort arrays by increasing pairs */
	cnt = 0;
	for (i = 0; i < bluescnt; i += 2)
		{
		ZonePair *pair	= &array[cnt++];
		pair->lo		= bluesarray[i + 0];
		pair->hi 		= bluesarray[i + 1];
		pair->top	 	= i > 0;
		}
	for (i = 0; i < othercnt; i += 2)
		{
		ZonePair *pair	= &array[cnt++];
		pair->lo		= otherarray[i + 0];
		pair->hi		= otherarray[i + 1];
		pair->top		= 0;
		}
	qsort(array, cnt, sizeof(ZonePair), cmpZonePair);

	/* Check BlueScale against maximum zone height. */
	max = 0;
	for (i = 0; i < cnt; i++)
		{
		float height = array[i].hi - array[i].lo;
		if (height > max)
			max = height;
		}
	if (max > 0 && (private->BlueScale*max > 1.0 || private->BlueScale*max < 0.5))
		cb->report_error(cb, abfErrBadBlueScale, iFD);

	/* Check for overlapping zones */
	last = &array[0];
	for (i = 1; i < cnt; i++)
		{
		ZonePair *curr = &array[i];
		if (last->top == curr->top &&
			last->hi + 2*private->BlueFuzz + 1 > curr->lo)
			{
			cb->report_error(cb, overlap, iFD);
			break;
			}
		last = curr;
		}
	}

/* Validate font dict. Currently only the contents of Private dict is 
   checked. */
static void checkFontDict(abfErrCallbacks *cb, 
						  abfTopDict *top, abfFontDict *font, int iFD)
	{
	abfPrivateDict *private = &font->Private;
	
	/* Check booleans */
	checkBooleanValue(cb, &top->isFixedPitch, abfErrBadisFixedPitch, iFD);
	checkBooleanValue(cb, &private->ForceBold, abfErrBadForceBold, iFD);
	checkBooleanValue(cb, &private->LanguageGroup,abfErrBadLanguageGroup, iFD);

	if (cb == NULL)
		return;	/* Reporting turned off; nothing more to do */

	/* Check StemSnap arrays */
	checkStemSnap(cb, top,
				  abfErrNoStdHW,
				  abfErrBadStdHW, 
				  abfErrBadStemSnapH,
				  &private->StdHW,
				  &private->StemSnapH.cnt,
				  private->StemSnapH.array,
				  iFD);
	checkStemSnap(cb, top,
				  abfErrNoStdVW,
				  abfErrBadStdVW, 
				  abfErrBadStemSnapV,
				  &private->StdVW,
				  &private->StemSnapV.cnt,
				  private->StemSnapV.array,
				  iFD);

	if (private->BlueValues.cnt == ABF_EMPTY_ARRAY &&
		private->OtherBlues.cnt == ABF_EMPTY_ARRAY &&
		private->FamilyBlues.cnt == ABF_EMPTY_ARRAY &&
		private->FamilyOtherBlues.cnt == ABF_EMPTY_ARRAY)
		{
		/* Report non-default Blue-key values when no zones */
		if (private->BlueScale != (float)cff_DFLT_BlueScale)
			cb->report_error(cb, abfErrUselessBlueScale, iFD);
		if (private->BlueShift != cff_DFLT_BlueShift)
			cb->report_error(cb, abfErrUselessBlueShift, iFD);
		if (private->BlueFuzz != cff_DFLT_BlueFuzz)
			cb->report_error(cb, abfErrUselessBlueFuzz, iFD);
		return;
		}

    
	/* Check each hint zone array for errors */	
	if (checkZoneArray(cb,
					   abfErrBadBlueValues, 
					   abfErrBigBlueValues, 
					   &private->BlueValues.cnt, 
					   private->BlueValues.array,
					   top, iFD, 0) ||
		checkZoneArray(cb, 
					   abfErrBadOtherBlues, 
					   abfErrBigOtherBlues, 
					   &private->OtherBlues.cnt, 
					   private->OtherBlues.array,
					   top, iFD, 1) ||
		checkZoneArray(cb, 
					   abfErrBadFamilyBlues, 
					   abfErrBigFamilyBlues,
					   &private->FamilyBlues.cnt, 
					   private->FamilyBlues.array,
					   top, iFD, 1) ||
		checkZoneArray(cb, 
					   abfErrBadFamilyOtherBlues, 
					   abfErrBigFamilyOtherBlues,
					   &private->FamilyOtherBlues.cnt, 
					   private->FamilyOtherBlues.array,
					   top, iFD, 1))
		return;	/* Can't continue with malformed zones */

	/* Check regular and family zones for overlap */
	checkZones(cb, 
			   abfErrRegularOverlap,
			   private->BlueValues.cnt, private->BlueValues.array,
			   private->OtherBlues.cnt, private->OtherBlues.array,
			   top, private, iFD);
	checkZones(cb,
			   abfErrFamilyOverlap,
			   private->FamilyBlues.cnt, private->FamilyBlues.array,
			   private->FamilyOtherBlues.cnt, private->FamilyOtherBlues.array,
			   top, private, iFD);
	}

/* Validate all dictionaries. */
void abfCheckAllDicts(abfErrCallbacks *cb, abfTopDict *top)
	{
	checkTopDict(cb, top);
	if (top->FDArray.cnt == 1)
		checkFontDict(cb, top, &top->FDArray.array[0], -1);
	else
		{
		long i;
		for (i = 0; i < top->FDArray.cnt; i++)
			checkFontDict(cb, top, &top->FDArray.array[i], i);
		}
	}

/* Return 1 if "FontMatrix" is the default matrix otherwise return 0. */
int abfIsDefaultFontMatrix(const abfFontMatrix *FontMatrix)
	{
	return (FontMatrix->cnt == ABF_EMPTY_ARRAY ||
			(FontMatrix->array[0] == 0.001f &&
			 FontMatrix->array[1] == 0.0 &&
			 FontMatrix->array[2] == 0.0 && 
			 FontMatrix->array[3] == 0.001f &&
			 FontMatrix->array[4] == 0.0 && 
			 FontMatrix->array[5] == 0.0));
	}

/* Map error code to error string. */
char *abfErrStr(int err_code)
	{
	static char *errstrs[] =
		{
#undef CTL_DCL_ERR
#define CTL_DCL_ERR(name,string)	string,
#include "abferr.h"
		};
	return (err_code < 0 || err_code >= (int)ARRAY_LEN(errstrs))?
		"unknown error": errstrs[err_code];
	}

/* Get version numbers of libraries. */
void abfGetVersion(ctlVersionCallbacks *cb)
	{
	if (cb->called & 1<<ABF_LIB_ID)
		return;	/* Already enumerated */

	cb->getversion(cb, ABF_VERSION, "absfont");

	/* Record this call */
	cb->called |= 1<<ABF_LIB_ID;
	}
