/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */


/*
 * Miscellaneous memory leak checker.
 */

#include "memcheck.h"
#include "stdio.h"

#ifdef MEMCHECK
static int counter=0;
#define MAXPOINTERS 100000
static int pointersneeded=MAXPOINTERS;


typedef struct {
	void * ptr;
	int refs;
} memEntry;

static memEntry entries[MAXPOINTERS];
#endif

char *memtemp[] =
{
#ifdef MEMCHECK
#include "memchecklist.h"

#else
	0
#endif
};


void memAllocated(void *ptr)
{
#ifdef MEMCHECK
	int i;
	int j=0;

	for(i=0; memtemp[i]!=0; i++)
		if (ptr==memtemp[i])
			j++;

	for(i=counter-1; i>=0; i--)
		if (entries[i].refs==0)
		{
			entries[i].refs=1;
			entries[i].ptr=ptr;
			return;
		}
	if(counter<MAXPOINTERS)
	{
		entries[counter].refs=1;
		entries[counter++].ptr=ptr;
	}else
		pointersneeded++;
#endif
}


void memDeallocated(void *ptr)
{
#ifdef MEMCHECK
	int i;

	for(i=counter-1; i>=0; i--)
		if (entries[i].ptr==ptr)
		{
			entries[i].refs--;
			if (entries[i].refs<0)
				fprintf(stderr, "Freeing %hx multiple times\n", ptr);
			break;
		}
#endif
}

void memReport(void)
{
#ifdef MEMCHECK
	int i, errorcounter=0;
	FILE *f;

	if (pointersneeded>MAXPOINTERS)
		fprintf(stderr, "Memcheck needs %d entries.\n", pointersneeded);
	
	f=fopen("C:\\PerforceDepotFDK\\font_development\\FDK\\Tools\\Programs\\otfproof\\source\\memchecklist.h", "w");

	for(i=0; i<counter; i++)
		if (entries[i].refs>0)
		{
			fprintf(f, "0x%lx, \n", entries[i].ptr);
			errorcounter++;
		}
	fprintf(f, "0  /*%d entries*/\n", errorcounter);
	fclose(f);
#endif
}

