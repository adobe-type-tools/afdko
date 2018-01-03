/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)FNAM.c	1.8
 * Changed:    5/19/99 17:20:55
 ***********************************************************************/

#include <string.h>

#include "FNAM.h"
#include "sfnt_FNAM.h"
#include "sfnt.h"

static FNAMTbl *FNAM = NULL;
static IntX loaded = 0;

void FNAMRead(LongN start, Card32 length)
	{
	IntX i;

	if (loaded)
		return;

	FNAM = (FNAMTbl *)memNew(sizeof(FNAMTbl));
	SEEK_ABS(start);
	
	IN1(FNAM->version);
	IN1(FNAM->nEncodings);

	FNAM->offset = memNew(sizeof(FNAM->offset[0]) * (FNAM->nEncodings + 1));
	for (i = 0; i < FNAM->nEncodings + 1; i++)
		IN1(FNAM->offset[i]);

	FNAM->encoding = memNew(sizeof(Encoding) * FNAM->nEncodings);
	for (i = 0; i < FNAM->nEncodings; i++)
		{
		IntX j;
		Encoding *encoding = &FNAM->encoding[i];
		Card32 offset = FNAM->offset[i] + SIZEOF(Client, style);

		/* Count clients */
		for (encoding->nClients = 0; offset < FNAM->offset[i + 1];
			 encoding->nClients++)
			{
			Card8 length;

			SEEK_ABS(start + offset);

			IN1(length);
			offset += length + 1 + SIZEOF(Client, style);
			}

		SEEK_ABS(FNAM->offset[i] + start);

		encoding->client = memNew(sizeof(Client) * encoding->nClients);
		for (j = 0; j < encoding->nClients; j++)
			{
			Card8 length;
			Client *client = &encoding->client[j];

			IN1(client->style);
			IN1(length);
			
			client->name = memNew(length + 1);

			IN_BYTES(length, client->name);
			client->name[length] = '\0';
			}
		}
	loaded = 1;
	}

void FNAMDump(IntX level, LongN start)
	{
	IntX i;

	DL(1, (OUTPUTBUFF, "### [FNAM] (%08lx)\n", start));

	DLV(2, "version   =", FNAM->version);
	DLu(2, "nEncodings=", FNAM->nEncodings);

	DL(2, (OUTPUTBUFF, "--- offset[index]=offset\n"));
	for (i = 0; i <= FNAM->nEncodings; i++)
		DL(2, (OUTPUTBUFF, "[%d]=%04hx ", i, FNAM->offset[i]));
	DL(2, (OUTPUTBUFF, "\n"));

	for (i = 0; i < FNAM->nEncodings; i++)
		{
		IntX j;
		Encoding *encoding = &FNAM->encoding[i];

		DL(2, (OUTPUTBUFF, "--- encoding[%d]\n", i));

		DL(2, (OUTPUTBUFF, "--- client[index]={style,length,name}\n"));
		for (j = 0; j < encoding->nClients; j++)
			{
			Client *client = &encoding->client[j];

			DL(2, (OUTPUTBUFF, "[%d]={%u,%lu,<%s>}\n", j, client->style,
				   STR_ARG(client->name)));
			}
		}
	}

void FNAMFree(void)
	{
	IntX i;

    if (!loaded)
		return;

	for (i = 0; i < FNAM->nEncodings; i++)
		{
		IntX j;
		Encoding *encoding = &FNAM->encoding[i];

		for (j = 0; j < encoding->nClients; j++)
			memFree(encoding->client[j].name);
		memFree(encoding->client);
		}
	memFree(FNAM->encoding);
	memFree(FNAM->offset);
	memFree(FNAM); FNAM = NULL;
	loaded = 0;
	}

/* Return FNAM->nEncodings */
IntX FNAMGetNEncodings(Card16 *nEncodings, Card32 client)
	{
	if (!loaded)
		{
		if (sfntReadTable(FNAM_))
			return tableMissing(FNAM_, client);
		}
	*nEncodings = FNAM->nEncodings;
	return 0;
	}
