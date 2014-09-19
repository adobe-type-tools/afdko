/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)MMFX.c	1.4
 * Changed:    5/19/99 17:21:00
 ***********************************************************************/

#include "MMFX.h"
#include "sfnt_MMFX.h"
#include "CFF_.h"
#include "dump.h"
#include "sfnt.h"

static MMFXTbl *MMFX = NULL;
static IntX loaded = 0;
static Card32 minoffset = MAX_CARD32;
static Card32 maxoffset = 0;


void MMFXRead(LongN start, Card32 length)
	{
	IntX i;
	Card32 lenstr;

	if (loaded)
		return;

	MMFX = (MMFXTbl *)memNew(sizeof(MMFXTbl));
	SEEK_ABS(start);

	IN1(MMFX->version);
	IN1(MMFX->nMetrics);
	IN1(MMFX->offSize);

	MMFX->offset = memNew(sizeof(Int32) * (MMFX->nMetrics + 1));

	for (i = 0; i < MMFX->nMetrics; i++)
		{
		  if (MMFX->offSize == 2)
			{
			  Int16 tmp;
			  IN1(tmp);
			  MMFX->offset[i] = tmp;
			}
		  else
			{
			  IN1(MMFX->offset[i]);
			}
		  if (MMFX->offset[i] < (Int32)minoffset) minoffset = MMFX->offset[i];
		  if (MMFX->offset[i] > (Int32)maxoffset) maxoffset = MMFX->offset[i];
		}


	lenstr = (start + length) - TELL();
	MMFX->offset[MMFX->nMetrics] = lenstr + 1;
	MMFX->cstrs = memNew(sizeof(Card8) * (lenstr + 1));
	SEEK_ABS(start + minoffset);
	IN_BYTES(lenstr, MMFX->cstrs);
	loaded = 1;
	}

void MMFXDump(IntX level, LongN start)
	{
	IntX i, pos;
	Int16 tmp;
	Card8 *ptr;
	Card16 nMasters;

	DL(1, (OUTPUTBUFF, "### [MMFX] (%08lx)\n", start));

	DLV(2, "Version  =", MMFX->version);
	DLu(2, "nMetrics =", MMFX->nMetrics);
	DLu(2, "offSize  =", MMFX->offSize);

	DL(2, (OUTPUTBUFF, "--- offset[index]=offset\n"));
	if (MMFX->offSize == 2)
	  {
		for (i = 0; i < MMFX->nMetrics; i++)
		  {
			tmp = (Int16)MMFX->offset[i];
			DL(2, (OUTPUTBUFF, "[%d]=%04hx ", i, tmp) );
		  }
		DL(2, (OUTPUTBUFF, "\n"));
	  }
	else
	  {
		for (i = 0; i < MMFX->nMetrics; i++)
		  DL(2, (OUTPUTBUFF, "[%d]=%08lx ", i, MMFX->offset[i]) );
		DL(2, (OUTPUTBUFF, "\n"));
	  }
	DL(2, (OUTPUTBUFF, "\n"));

	CFF_GetNMasters(&nMasters, MMFX_);

	DL(2, (OUTPUTBUFF, "--- cstring[index]=<charstring ops>\n"));
	for (i = 0; i < MMFX->nMetrics; i++)
	  {
		pos = MMFX->offset[i] - minoffset;
		ptr = &(MMFX->cstrs[pos]);
		if (i < 8)
		  switch (i)
			{
			case 0:	DL(2, (OUTPUTBUFF, "[0=Zero]           = <") ); break;
			case 1:	DL(2, (OUTPUTBUFF, "[1=Ascender]       = <") ); break;
			case 2:	DL(2, (OUTPUTBUFF, "[2=Descender]      = <") ); break;
			case 3:	DL(2, (OUTPUTBUFF, "[3=LineGap]        = <") ); break;
			case 4:	DL(2, (OUTPUTBUFF, "[4=AdvanceWidthMax]= <") ); break;
			case 5:	DL(2, (OUTPUTBUFF, "[5=AvgCharWidth]   = <") ); break;
			case 6:	DL(2, (OUTPUTBUFF, "[6=xHeight]        = <") ); break;
			case 7:	DL(2, (OUTPUTBUFF, "[7=CapHeight]      = <") ); break;
			}
		else
		  {
			DL(2, (OUTPUTBUFF, "[%d]= <", i) );
		  }
		
		dump_csDump(MAX_INT32, ptr, nMasters);
	
		DL(2, (OUTPUTBUFF, ">\n"));
	  }
	DL(2, (OUTPUTBUFF, "\n"));
  }

void MMFXDumpMetric(Card32 MetricID)
	{
	  IntX pos;
	  Card8 *ptr;
	  Card16 nMasters;

	  if (!loaded)
		{
		  if (sfntReadTable(MMFX_))
			  return;
		}
	  if (MetricID >= MMFX->nMetrics)
		return;
	  CFF_GetNMasters(&nMasters, MMFX_);		  
	  pos = MMFX->offset[MetricID] - minoffset;
	  ptr = &(MMFX->cstrs[pos]);

	  dump_csDumpDerel(ptr, nMasters);
	}

void MMFXFree(void)
	{
	if (!loaded)
		return;

	memFree(MMFX->offset);
	memFree(MMFX->cstrs);
	memFree(MMFX); MMFX = NULL;
	loaded = 0;
	}



