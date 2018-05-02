/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)CID_.c	1.9
 * Changed:    5/19/99 17:51:51
 ***********************************************************************/

#include "CID_.h"
#include "sfnt_CID_.h"
#include "sfnt.h"

static CID_Tbl *CID_ = NULL;
static IntX loaded = 0;

void CID_Read(LongN start, Card32 length)
	{
	  if (loaded)
		return;

	  CID_ = (CID_Tbl *)memNew(sizeof(CID_Tbl));

	SEEK_ABS(start);

	IN1(CID_->Version);
	IN1(CID_->Flags);
	IN1(CID_->CIDCount);
	IN1(CID_->TotalLength);
	IN1(CID_->AsciiLength);
	IN1(CID_->BinaryLength);
	IN1(CID_->FDCount);

	loaded = 1;
	}

void CID_Dump(IntX level, LongN start)
	{
	DL(1, (OUTPUTBUFF, "### [CID ] (%08lx)\n", start));

	DLV(2, "Version     =", CID_->Version);
	DLx(2, "Flags       =", CID_->Flags);
	DLu(2, "CIDCount    =", CID_->CIDCount);
	DLX(2, "TotalLength =", CID_->TotalLength);
	DLX(2, "AsciiLength =", CID_->AsciiLength);
	DLX(2, "BinaryLength=", CID_->BinaryLength);
	DLu(2, "FDCount     =", CID_->FDCount);
	}

void CID_Free(void)
	{
	  if (!loaded) return;
	  memFree(CID_); CID_ = NULL;
	loaded = 0;
	}

/* Return CID_->CIDCount */
IntX CID_GetNGlyphs(Card16 *nGlyphs, Card32 client)
	{
	if (!loaded)
		{
		if (sfntReadTable(CID__))
			return tableMissing(CID__, client);
		}
	*nGlyphs = CID_->CIDCount;
	return 0;
	}

IntX CID_isCID(void)
	{
	  if (loaded) 
		{
		  return 1;
		}
	  else
		{
		  if (sfntReadTable(CID__))
			return 0;
		}
	  return 1;
	}
