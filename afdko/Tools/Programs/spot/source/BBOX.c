/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)BBOX.c	1.6
 * Changed:    5/19/99 17:20:52
 ***********************************************************************/

#include "BBOX.h"
#include "sfnt_BBOX.h"

static BBOXTbl *BBOX = NULL;
static IntX loaded = 0;

void BBOXRead(LongN start, Card32 length)
	{
	IntX i;

	if (loaded)
		return;

	BBOX = (BBOXTbl *)memNew(sizeof(BBOXTbl));

	SEEK_ABS(start);
	
	IN1(BBOX->version);
	IN1(BBOX->flags);
	IN1(BBOX->nGlyphs);
	IN1(BBOX->nMasters);

	BBOX->bbox = memNew(sizeof(BBox) * BBOX->nGlyphs);
	for (i = 0; i < BBOX->nGlyphs; i++)
		{
		IntX j;
		BBox *bbox = &BBOX->bbox[i];

		bbox->left = memNew(sizeof(FWord) * BBOX->nMasters);
		for (j = 0; j < BBOX->nMasters; j++)
			IN1(bbox->left[j]);

		bbox->bottom = memNew(sizeof(FWord) * BBOX->nMasters);
		for (j = 0; j < BBOX->nMasters; j++)
			IN1(bbox->bottom[j]);

		bbox->right = memNew(sizeof(FWord) * BBOX->nMasters);
		for (j = 0; j < BBOX->nMasters; j++)
			IN1(bbox->right[j]);

		bbox->top = memNew(sizeof(FWord) * BBOX->nMasters);
		for (j = 0; j < BBOX->nMasters; j++)
			IN1(bbox->top[j]);
		}

	loaded = 1;
	}

void BBOXDump(IntX level, LongN start)
	{
	IntX i;
	IntX j;

	DL(1, (OUTPUTBUFF, "### [BBOX] (%08lx)\n", start));

	DLV(2, "version =", BBOX->version);
	DLu(2, "flags   =", BBOX->flags);
	DLu(2, "nGlyphs =", BBOX->nGlyphs);
	DLu(2, "nMasters=", BBOX->nMasters);

	if (BBOX->nMasters == 1)
		{
		DL(3, (OUTPUTBUFF, "--- bbox[glyphId]={left,bottom,right,top}\n"));

		for (i = 0; i < BBOX->nGlyphs; i++)
			{
			BBox *bbox = &BBOX->bbox[i];
			DL(3, (OUTPUTBUFF, "[%d]={%hd,%hd,%hd,%hd} ", i,
				   bbox->left[0], bbox->bottom[0],
				   bbox->right[0], bbox->top[0]));
			}
		DL(3, (OUTPUTBUFF, "\n"));
		}
	else
		{
		DL(3, (OUTPUTBUFF, "--- bbox[glyphId]={{left+},{bottom+},{right+},{top+}}\n"));
		for (i = 0; i < BBOX->nGlyphs; i++)
			{
			BBox *bbox = &BBOX->bbox[i];
			DL(3, (OUTPUTBUFF, "[%d]={{", i));

			for (j = 0; j < BBOX->nMasters; j++)
				DL(3, (OUTPUTBUFF, "%hd%s", bbox->left[j],
					   j == BBOX->nMasters - 1 ? "},{" : ","));

			for (j = 0; j < BBOX->nMasters; j++)
				DL(3, (OUTPUTBUFF, "%hd%s", bbox->bottom[j],
					   j == BBOX->nMasters - 1 ? "},{" : ","));

			for (j = 0; j < BBOX->nMasters; j++)
				DL(3, (OUTPUTBUFF, "%hd%s", bbox->right[j],
					   j == BBOX->nMasters - 1 ? "},{" : ","));

			for (j = 0; j < BBOX->nMasters; j++)
				DL(3, (OUTPUTBUFF, "%hd%s", bbox->top[j],
					   j == BBOX->nMasters - 1 ? "}} " : ","));
			}
		DL(3, (OUTPUTBUFF, "\n"));
		}
	}

void BBOXFree(void)
	{
	IntX i;

    if (!loaded)
		return;

	for (i = 0; i < BBOX->nMasters; i++)
		{
		BBox *bbox = &BBOX->bbox[i];

		memFree(bbox->left);
		memFree(bbox->bottom);
		memFree(bbox->right);
		memFree(bbox->top);
		}
	memFree(BBOX->bbox);
	memFree(BBOX); BBOX = NULL;
	loaded = 0;
	}

