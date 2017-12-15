/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)fvar.c	1.6
 * Changed:    5/19/99 17:21:05
 ***********************************************************************/

#include "fvar.h"
#include "sfnt_fvar.h"

static fvarTbl *fvar = NULL;
static IntX loaded = 0;

void fvarRead(LongN start, Card32 length)
	{
	IntX i;
    int hasInstancePSNames = 0;

	if (loaded)
		return;

	fvar = (fvarTbl *)memNew(sizeof(fvarTbl)); 
	SEEK_ABS(start);

	IN1(fvar->version);
	IN1(fvar->offsetToData);
	IN1(fvar->countSizePairs);
	IN1(fvar->axisCount);
	IN1(fvar->axisSize);
	IN1(fvar->instanceCount);
	IN1(fvar->instanceSize);

    if (fvar->instanceSize == (fvar->axisCount * 4 + 6))
        hasInstancePSNames = 1;
        
	/* Read axes */
	fvar->axis = memNew(sizeof(Axis) * fvar->axisCount);
	for (i = 0; i < fvar->axisCount; i++)
		{
		Axis *axis = &fvar->axis[i];

		IN1(axis->axisTag);
		IN1(axis->minValue);
		IN1(axis->defaultValue);
		IN1(axis->maxValue);
		IN1(axis->flags);
		IN1(axis->nameId);
		}

	/* Read instances */
	fvar->instance = memNew(sizeof(Instance) * fvar->instanceCount);
	for (i = 0; i < fvar->instanceCount; i++)
		{
		IntX j;
		Instance *instance = &fvar->instance[i];

        instance->psNameId = MAX_CARD16;
		IN1(instance->nameId);
		IN1(instance->flags);

		instance->coord = memNew(sizeof(instance->coord[0]) * fvar->axisCount);
		for (j = 0; j < fvar->axisCount; j++)
			IN1(instance->coord[j]);
        if (hasInstancePSNames)
            IN1(instance->psNameId);
		}

	loaded = 1;
	}

void fvarDump(IntX level, LongN start)
	{
	IntX i;

	DL(1, (OUTPUTBUFF, "### [fvar] (%08lx)\n", start));

	DLV(2, "version       =", fvar->version);
	DLx(2, "offsetToData  =", fvar->offsetToData);
	DLu(2, "countSizePairs=", fvar->countSizePairs);
	DLu(2, "axisCount     =", fvar->axisCount);
	DLu(2, "axisSize      =", fvar->axisSize);
	DLu(2, "instanceCount =", fvar->instanceCount);
	DLu(2, "instanceSize  =", fvar->instanceSize);

	/* Dump axes */
	for (i = 0; i < fvar->axisCount; i++)
		{
		Axis *axis = &fvar->axis[i];

		DL(2, (OUTPUTBUFF, "--- axis[%d]\n", i));
		DLT(2, "axisTag     =", axis->axisTag);
		DLF(2, "minValue    =", axis->minValue);
		DLF(2, "defaultValue=", axis->defaultValue);
		DLF(2, "maxValue    =", axis->maxValue);
		DLx(2, "flags       =", axis->flags);
		DLu(2, "nameId      =", axis->nameId);
		}

	/* Dump instances */
	for (i = 0; i < fvar->instanceCount; i++)
		{
		IntX j;
		Instance *instance = &fvar->instance[i];

		DL(2, (OUTPUTBUFF, "--- instance[%d]\n", i));
		DLu(2, "nameId=", instance->nameId);
		DLx(2, "flags= ", instance->flags);

		for (j = 0; j < fvar->axisCount; j++)
			DL(2, (OUTPUTBUFF, "coord[%d]=%08lx (%1.3f)\n", j, instance->coord[j],
				   FIX2FLT(instance->coord[j])));
        if ( instance->psNameId != MAX_CARD16)
            DLu(2, "psNameId=", instance->psNameId);
            
		}
	}

void fvarFree(void)
	{
	IntX i;

	if (!loaded)
		return;

	for (i = 0; i < fvar->instanceCount; i++)
		memFree(fvar->instance[i].coord);
	memFree(fvar->instance);

	memFree(fvar->axis);
	memFree(fvar); fvar = NULL;
	loaded = 0;
	}
