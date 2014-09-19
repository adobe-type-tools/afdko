/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Font merging support.
 */

#include "absfont.h"

#if PLAT_SUN4
#include "sun4/fixstring.h"
#else /* PLAT_SUN4 */
#include <string.h>
#endif  /* PLAT_SUN4 */

/* Compare two private dicts to see if they are similar enough to merge. */
static int comparePrivateDicts(abfPrivateDict *private1, 
							   abfPrivateDict *private2)
	{
	int i;

	/* note: for all arrays, unused arrays are init'd with a cnt of 0. */
	
	if (private1->BlueValues.cnt != private2->BlueValues.cnt)
		return 1;

	for (i = 0; i < private1->BlueValues.cnt;  i++)
		if (private1->BlueValues.array[i] != private2->BlueValues.array[i])
			return 1;
			
	if (private1->OtherBlues.cnt != private2->OtherBlues.cnt)
		return 1;

	for (i = 0; i < private1->OtherBlues.cnt; i++)
		if (private1->OtherBlues.array[i] != private2->OtherBlues.array[i])
			return 1;
			
	if (private1->FamilyBlues.cnt != private2->FamilyBlues.cnt)
		return 1;

	for (i = 0; i < private1->FamilyBlues.cnt; i++)
		if (private1->FamilyBlues.array[i] != private2->FamilyBlues.array[i])
			return 1;
			
	if (private1->FamilyOtherBlues.cnt != private2->FamilyOtherBlues.cnt)
		return 1;

	for (i = 0; i < private1->FamilyOtherBlues.cnt; i++)
		if (private1->FamilyOtherBlues.array[i] != 
			private2->FamilyOtherBlues.array[i])
			return 1;
			
	if (private1->StemSnapH.cnt != private2->StemSnapH.cnt)
		return 1;

	for (i = 0; i < private1->StemSnapH.cnt; i++)
		if (private1->StemSnapH.array[i] != private2->StemSnapH.array[i])
			return 1;
			
	if (private1->StemSnapV.cnt != private2->StemSnapV.cnt)
		return 1;

	for (i = 0; i < private1->StemSnapV.cnt; i++)
		if (private1->StemSnapV.array[i] != private2->StemSnapV.array[i])
			return 1;
			
	if (private1->BlueScale != private2->BlueScale ||
		private1->BlueShift != private2->BlueShift ||
		private1->BlueFuzz != private2->BlueFuzz ||
		private1->StdHW != private2->StdHW ||
		private1->StdVW != private2->StdVW ||
		private1->ForceBold != private2->ForceBold ||
		private1->LanguageGroup != private2->LanguageGroup ||
		private1->ExpansionFactor != private2->ExpansionFactor)
		return 1;
		
	return 0;
	}

/* Compare two top dicts to see if they are similar enough to merge.
   Assume client has assured that they are valid before call. */
int	abfCompareTopDicts(abfTopDict *top1, abfTopDict *top2)
	{
	int i;
	int firstIsCID = (top1->sup.flags & ABF_CID_FONT) != 0;
	int secondIsCID = (top2->sup.flags & ABF_CID_FONT) != 0;
	
	if (firstIsCID != secondIsCID)
		return 1;
	
	/* For non-CID fonts, compare the private dicts */
	if (!firstIsCID)
		{
		if (abfCompareFontDicts(&top1->FDArray.array[0], 
								&top2->FDArray.array[0]))
			return 1;
		}
			
	/* Otherwise, we don't compare the font dicts at all; that is handled by
	   the destination font module. */
	
	/* now for the less likely top dict differences */
	if (top1->isFixedPitch != top2->isFixedPitch ||
		top1->ItalicAngle != top2->ItalicAngle ||
		top1->StrokeWidth != top2->StrokeWidth)
		return 1;

	if (firstIsCID)
		{
		if (top1->cid.Registry.ptr && top2->cid.Registry.ptr && 
			strcmp(top1->cid.Registry.ptr, top2->cid.Registry.ptr) != 0)
			return 1;

		if (top1->cid.Ordering.ptr && top2->cid.Ordering.ptr && 
			strcmp(top1->cid.Ordering.ptr, top2->cid.Ordering.ptr) != 0)
			return 1;

		if (top1->cid.FontMatrix.cnt != top2->cid.FontMatrix.cnt)
			return 1;

		for (i = 0; i <top1->cid.FontMatrix.cnt; i++)
			if (top1->cid.FontMatrix.array[i] != top2->cid.FontMatrix.array[i])
				return 1;
			
		}
	
	return 0;
	}

/* Compare two font dicts to see if they are similar enough to merge. */
int	abfCompareFontDicts(abfFontDict *font1, abfFontDict *font2)
	{
	int i;

	if (font1->PaintType != font2->PaintType)
		return 1;
	
	if (comparePrivateDicts(&font1->Private, &font2->Private))
		return 1;
	
	if (font1->FontMatrix.cnt != font2->FontMatrix.cnt)
		return 1;

	for (i = 0; i < font1->FontMatrix.cnt; i++)
		if (font1->FontMatrix.array[i] != font2->FontMatrix.array[i])
			return 1;
	
	return 0;
	}

