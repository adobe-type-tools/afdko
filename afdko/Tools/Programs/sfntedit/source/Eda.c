/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)da.c	1.2
 * Changed:    7/25/95 13:33:56
 ***********************************************************************/

/*
 * Dynamic array support.
 */

#include <stdio.h>

#include "Eda.h"

/* Memory management functions */
static void *(*alloc)(size_t size);
static void *(*resize)(void *old, size_t size);
static void (*dealloc)(void *ptr);

/* Dynamic array object template */
typedef da_DCL(void, DA);

/* Initialize dynamic array */
void da_Init(void *object, unsigned long intl, unsigned long incr)
	{
	DA *da = object;

	da->array = (void *)intl;
	da->cnt = 0;
	da->size = 0;
	da->incr = incr;
	da->init = NULL;
	}

/* Grow dynamic array to accomodate index */
void da_Grow(void *object, size_t element, unsigned long index)
	{
	DA *da = object;
	long newSize = (index + da->incr) / da->incr * da->incr;

	if (da->size == 0)
		{
		if (newSize < (long)da->array)
			newSize = (long)da->array;	/* Use initial allocation */
		da->array = alloc(newSize * element);
		}
	else
		da->array = resize(da->array, newSize * element);

	if (da->init != NULL && da->array != NULL)
		{
		/* Initialize new elements */
		char *p;

		for (p = &((char *)da->array)[da->size * element]; 
			 p < &((char *)da->array)[newSize * element];
			 p += element)
			if (da->init(p))
				break;			/* Client function wants to stop */
		}
	da->size = newSize;
	}

/* Free dynamic array */
void da_Free(void *object)
	{
	dealloc(((DA *)object)->array);
	}

/* Initialize memory management functions */
void da_SetMemFuncs(void *(*Alloc)(size_t size),
					void *(*Resize)(void *old, size_t size),
					void (*Dealloc)(void *ptr))
	{
	alloc = Alloc;
	resize = Resize;
	dealloc = Dealloc;
	}
