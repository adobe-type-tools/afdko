/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Abstract font descriptor management.
 */

#include "abfdesc.h"
#include "absfont.h"
#include "dictops.h"

#if PLAT_SUN4
#include "sun4/fixstring.h"
#else /* PLAT_SUN4 */
#include <string.h>
#endif  /* PLAT_SUN4 */

/* Fill out the size fields in an abfFontDescElement. */
static void fillFontDescElementSizes(abfFontDict *font,
									 abfFontDescSpecialValues *sv,
									 abfFontDescElement *elem)
	{
	abfPrivateDict *Private = &font->Private;

	elem->flags = 0;
	elem->valueCnt = 0;

	/* Count fixed-size values */
	if (sv->CharstringType != cff_DFLT_CharstringType)
		{
		elem->flags |= ABF_DESC_CharstringType;
		elem->valueCnt++;
		}
	if (font->PaintType != cff_DFLT_PaintType)
		{
		elem->flags |= ABF_DESC_PaintType;
		elem->valueCnt++;
		}
	if (Private->BlueScale != (float)cff_DFLT_BlueScale)
		{
		elem->flags |= ABF_DESC_BlueScale;
		elem->valueCnt++;
		}
	if (Private->BlueShift != cff_DFLT_BlueShift)
		{
		elem->flags |= ABF_DESC_BlueShift;
		elem->valueCnt++;
		}
	if (Private->BlueFuzz != cff_DFLT_BlueFuzz)
		{
		elem->flags |= ABF_DESC_BlueFuzz;
		elem->valueCnt++;
		}
	if (Private->StdHW != ABF_UNSET_REAL)
		{
		elem->flags |= ABF_DESC_StdHW;
		elem->valueCnt++;
		}
	if (Private->StdVW != ABF_UNSET_REAL)
		{
		elem->flags |= ABF_DESC_StdVW;
		elem->valueCnt++;
		}
	if (Private->ForceBold != cff_DFLT_ForceBold)
		{
		elem->flags |= ABF_DESC_ForceBold;
		elem->valueCnt++;
		}
	if (Private->LanguageGroup != cff_DFLT_LanguageGroup)
		{
		elem->flags |= ABF_DESC_LanguageGroup;
		elem->valueCnt++;
		}
	if (Private->ExpansionFactor != (float)cff_DFLT_ExpansionFactor)
		{
		elem->flags |= ABF_DESC_ExpansionFactor;
		elem->valueCnt++;
		}
	if (Private->initialRandomSeed != cff_DFLT_initialRandomSeed)
		{
		elem->flags |= ABF_DESC_initialRandomSeed;
		elem->valueCnt++;
		}
	if (sv->defaultWidthX != cff_DFLT_defaultWidthX)
		{
		elem->flags |= ABF_DESC_defaultWidthX;
		elem->valueCnt++;
		}
	if (sv->nominalWidthX != cff_DFLT_nominalWidthX)
		{
		elem->flags |= ABF_DESC_nominalWidthX;
		elem->valueCnt++;
		}
	if (sv->lenSubrArray != 0)
		{
		elem->flags |= ABF_DESC_lenSubrArray;
		elem->valueCnt++;
		}
	if (!abfIsDefaultFontMatrix(&font->FontMatrix))
		{
		elem->flags |= ABF_DESC_FontMatrix;
		elem->valueCnt += 6;
		}

	/* Count variable-size values */
	elem->BlueValues 		= (unsigned char)Private->BlueValues.cnt;
	elem->OtherBlues 		= (unsigned char)Private->OtherBlues.cnt;
	elem->FamilyBlues 		= (unsigned char)Private->FamilyBlues.cnt;
	elem->FamilyOtherBlues	= (unsigned char)Private->FamilyOtherBlues.cnt;
	elem->StemSnapH 		= (unsigned char)Private->StemSnapH.cnt;
	elem->StemSnapV 		= (unsigned char)Private->StemSnapV.cnt;

	/* Totalize values */
	elem->valueCnt += (elem->BlueValues +
					   elem->OtherBlues +
					   elem->FamilyBlues +
					   elem->FamilyOtherBlues+
					   elem->StemSnapH +
					   elem->StemSnapV);
	}

/* Save float value in destination array. */
static float *saveValue(float *dst, float src, int save)
	{
	if (save != 0)
		*dst++ = src;
	return dst;
	}

/* Save float values in destination array. */
static float *saveValues(float *dst, float *src, int cnt)
	{
	if (cnt > 0)
		memmove(dst, src, cnt*sizeof(float));
	return dst + cnt;
	}

/* Fill out the value fields in an abfFontDescElement. */
static void fillFontDescElementValues(abfFontDict *font,
									  abfFontDescSpecialValues *sv,
									  abfFontDescElement *elem)
	{
	float *values = elem->values;
	abfPrivateDict *Private = &font->Private;

	/* Fill single-valued parameter values */
	values = saveValue(values, (float)sv->CharstringType,
					   elem->flags & ABF_DESC_CharstringType);
	values = saveValue(values, (float)font->PaintType,
					   elem->flags & ABF_DESC_PaintType);
	values = saveValue(values, Private->BlueScale,
					   elem->flags & ABF_DESC_BlueScale);
	values = saveValue(values, Private->BlueShift,
					   elem->flags & ABF_DESC_BlueShift);
	values = saveValue(values, Private->BlueFuzz,
					   elem->flags & ABF_DESC_BlueFuzz);
	values = saveValue(values, Private->StdHW,
					   elem->flags & ABF_DESC_StdHW);
	values = saveValue(values, Private->StdVW,
					   elem->flags & ABF_DESC_StdVW);
	values = saveValue(values, (float)Private->LanguageGroup,
					   elem->flags & ABF_DESC_LanguageGroup);
	values = saveValue(values, (float)Private->ForceBold,
					   elem->flags & ABF_DESC_ForceBold);
	values = saveValue(values, Private->ExpansionFactor,
					   elem->flags & ABF_DESC_ExpansionFactor);
	values = saveValue(values, Private->initialRandomSeed,
					   elem->flags & ABF_DESC_initialRandomSeed);
	values = saveValue(values, sv->defaultWidthX,
					   elem->flags & ABF_DESC_defaultWidthX);
	values = saveValue(values, sv->nominalWidthX,
					   elem->flags & ABF_DESC_nominalWidthX);
	values = saveValue(values, (float)sv->lenSubrArray,
					   elem->flags & ABF_DESC_lenSubrArray);

	/* Fill multi-valued parameter values */
	values = saveValues(values, font->FontMatrix.array, 
						(elem->flags & ABF_DESC_FontMatrix)? 6: 0);
	values = saveValues(values, Private->BlueValues.array, elem->BlueValues);
	values = saveValues(values, Private->OtherBlues.array, elem->OtherBlues);
	values = saveValues(values, Private->FamilyBlues.array, elem->FamilyBlues);
	values = saveValues(values, Private->FamilyOtherBlues.array,
						elem->FamilyOtherBlues);
	values = saveValues(values, Private->StemSnapH.array, elem->StemSnapH);
	values = saveValues(values, Private->StemSnapV.array, elem->StemSnapV);
	}

/* Create abstract font descriptor. */
abfFontDescHeader *abfNewFontDesc(ctlMemoryCallbacks *mem_cb, 
								  abfFontDescCallbacks *desc_cb,
								  long lenGSubrArray,
								  abfTopDict *top)
	{
	long i;
	size_t size;
	abfFontDescHeader *hdr;
	abfFontDescElement *elem;
	abfFontDescSpecialValues sv;

	/* Compute total structure size */
	size = sizeof(abfFontDescHeader);
	for (i = 0; i < top->FDArray.cnt; i++)
		{
		abfFontDescElement tmp;
		desc_cb->getSpecialValues(desc_cb, (int)i, &sv);
		fillFontDescElementSizes(&top->FDArray.array[i], &sv, &tmp);
		size += (sizeof(abfFontDescElement) + 
				 (tmp.valueCnt - 1)*sizeof(tmp.values[0]));
		}

	/* Allocate structure */
	hdr = (abfFontDescHeader *)mem_cb->manage(mem_cb, NULL, size);
	if (hdr == NULL)
		return NULL;
	// memset(hdr, 0, size);


	/* Fill header */
	hdr->length = (unsigned short)size;
	hdr->FDElementCnt = (unsigned short)top->FDArray.cnt;
	memcpy(hdr->FontBBox, top->FontBBox, sizeof(hdr->FontBBox));
	hdr->StrokeWidth = top->StrokeWidth;
	hdr->lenGSubrArray = (float)lenGSubrArray;
	
	/* Fill elements */
	elem = ABF_GET_FIRST_DESC(hdr);
	for (i = 0; i < top->FDArray.cnt; i++)
		{
		abfFontDict *fd = &top->FDArray.array[i];
		desc_cb->getSpecialValues(desc_cb, (int)i, &sv);
		fillFontDescElementSizes(fd, &sv, elem);
		fillFontDescElementValues(fd, &sv, elem);
		elem = ABF_GET_NEXT_DESC(elem);
		}

	return hdr;
	}

/* Return FontMatrix from font descriptor. */
float *abfGetFontDescMatrix(abfFontDescHeader *hdr, int iFD)
	{
	if (iFD >= 0 && iFD < hdr->FDElementCnt)
		{
		/* Skip to FDArray element */
		abfFontDescElement *elem = ABF_GET_FIRST_DESC(hdr);
		while (iFD--)
			elem = ABF_GET_NEXT_DESC(elem);

		if (elem->flags & ABF_DESC_FontMatrix)
			{
			/* Non-default FontMatrix; count values that precede it */
			long count;
			long bits = elem->flags & (ABF_DESC_FontMatrix - 1);
			for (count = 0; bits; count++)
				bits &= bits - 1;
			if (count + 6 <= (long)elem->valueCnt)
				return &elem->values[count];
			}
		}
	return NULL;
	}

/* Free abstract font descriptor. */
void abfFreeFontDesc(ctlMemoryCallbacks *mem_cb, abfFontDescHeader *hdr)
	{
	(void)mem_cb->manage(mem_cb, hdr, 0);
	}
