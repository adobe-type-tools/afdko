/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)fdsc.c	1.6
 * Changed:    5/19/99 17:21:04
 ***********************************************************************/

#include "fdsc.h"
#include "sfnt_fdsc.h"

static fdscTbl *fdsc = NULL;
static IntX loaded = 0;

void fdscRead(LongN start, Card32 length)
	{
	IntX i;

	if (loaded)
		return;

	fdsc = (fdscTbl *)memNew(sizeof(fdscTbl));
	SEEK_ABS(start);

	/* Read header */
	IN1(fdsc->version);
	IN1(fdsc->nDescriptors);

	/* Read descriptors */
	fdsc->descriptor = memNew(sizeof(FontDescriptor) * fdsc->nDescriptors);
	for (i = 0; i < (IntX)fdsc->nDescriptors; i++)
		{
		FontDescriptor *desc = &fdsc->descriptor[i];
		IN1(desc->tag);
		IN1(desc->value);
		}

	loaded = 1;
	}

void fdscDump(IntX level, LongN start)
	{
	IntX i;

	DL(1, (OUTPUTBUFF, "### [fdsc] (%08lx)\n", start));
	
	/* Dump header */
	DLV(2, "version     =", fdsc->version);
	DL(2, (OUTPUTBUFF, "nDescriptors=%u\n", fdsc->nDescriptors));

	/* Dump descriptors */
	DL(2, (OUTPUTBUFF, "--- descriptor[index]={tag,value}\n"));
	for (i = 0; i < (IntX)fdsc->nDescriptors; i++)
		{
		FontDescriptor *desc = &fdsc->descriptor[i];
		DL(2, (OUTPUTBUFF, "[%d]={%c%c%c%c,%1.3f (%08x)}\n", i,
			   TAG_ARG(desc->tag), FIXED_ARG(desc->value)));
		}
	}

void fdscFree(void)
	{
	if (!loaded)
		return;

	memFree(fdsc->descriptor);
	memFree(fdsc); fdsc = NULL;
	loaded = 0;
	}
