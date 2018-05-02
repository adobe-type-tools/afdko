/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)TYP1.c	1.8
 * Changed:    5/19/99 17:51:52
 ***********************************************************************/

#include "TYP1.h"
#include "sfnt_TYP1.h"
#include "sfnt.h"

static TYP1Tbl *TYP1 = NULL;
static IntX loaded = 0;

void TYP1Read(LongN start, Card32 length)
	{
	  if (loaded)
		return;

	  TYP1 = (TYP1Tbl *)memNew(sizeof(TYP1Tbl));
	SEEK_ABS(start);

	IN1(TYP1->Version);
	IN1(TYP1->Flags);
	IN1(TYP1->GlyphCount);
	IN1(TYP1->TotalLength);
	IN1(TYP1->AsciiLength);
	IN1(TYP1->BinaryLength);
	IN1(TYP1->SubrMaxLength);

	loaded = 1;
	}

void TYP1Dump(IntX level, LongN start)
	{
	DL(1, (OUTPUTBUFF, "### [TYP1] (%08lx)\n", start));

	DLV(2, "Version      =", TYP1->Version);
	DLx(2, "Flags        =", TYP1->Flags);
	DLu(2, "GlyphCount   =", TYP1->GlyphCount);
	DLX(2, "TotalLength  =", TYP1->TotalLength);
	DLX(2, "AsciiLength  =", TYP1->AsciiLength);
	DLX(2, "BinaryLength =", TYP1->BinaryLength);
	DLX(2, "SubrMaxLength=", TYP1->SubrMaxLength);
	}

void TYP1Free(void)
	{
	  if (!loaded) return;
	  memFree(TYP1); TYP1 = NULL;
	loaded = 0;
	}

/* Return TYP1->GlyphCount */
IntX TYP1GetNGlyphs(Card16 *nGlyphs, Card32 client)
	{
	if (!loaded)
		{
		if (sfntReadTable(TYP1_))
			return tableMissing(TYP1_, client);
		}
	*nGlyphs = TYP1->GlyphCount;
	return 0;
	}
