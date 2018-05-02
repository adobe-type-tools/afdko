/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)post.c	1.13
 * Changed:    5/27/99 23:03:31
 ***********************************************************************/

#include <string.h>

#include "post.h"
#include "sfnt_post.h"
#include "maxp.h"
#include "sfnt.h"
#include "CID_.h"

static postTbl *post = NULL;
static IntX loaded = 0;
static Byte8 *applestd[] =
	{
#include "sfnt_applestd.h"
	};
#define STD_LENGTH		TABLE_LEN(applestd)
#define MAX_STD_INDEX  	(STD_LENGTH - 1)
static IntX nNames;
static Card16 nGlyphs;

static Format2_0 *read2_0(Card32 left)
	{
	IntX i;
	Card8 *end;
	Card8 *name;
	IntX nameSize;
	Format2_0 *format = memNew(sizeof(Format2_0));

	IN1(format->numberGlyphs);
	
	/* Read index */
	format->glyphNameIndex =
		memNew(sizeof(format->glyphNameIndex[0]) * format->numberGlyphs);
	for (i = 0; i < format->numberGlyphs; i++)
		IN1(format->glyphNameIndex[i]);

	/* Read name table */
	nameSize = 
		left - (sizeof(format->numberGlyphs) +
				sizeof(format->glyphNameIndex[0]) * format->numberGlyphs);
	format->names = memNew(nameSize + 1);
	IN_BYTES(nameSize, format->names);

	/* Count names in table */
	nNames = 0;
	end = format->names + nameSize;
	for (name = format->names; name < end; name += *name + 1)
		if (*name == '\0')
			break;
		else
			nNames++;

	return format;
	}

static Format2_5 *read2_5(void)
	{
	IntX i;
	Format2_5 *format = memNew(sizeof(Format2_5));

	IN1(format->numberGlyphs);

	format->offset = memNew(sizeof(format->offset[0]) * format->numberGlyphs);
	for (i = 0; i < format->numberGlyphs; i++)
		IN1(format->offset[i]);

	return format;
	}

static Format4_0 *read4_0(LongN start)
	{
	IntX i;
	Format4_0 *format;

	if (maxpGetNGlyphs(&nGlyphs, post_))
		{
		warning(SPOT_MSG_postNONGLYPH);
		return NULL;
		}

	format = memNew(sizeof(Format4_0));
	format->code = memNew(sizeof(format->code[0]) * nGlyphs );
	
	SEEK_ABS(start + TBL_HDR_SIZE);
	for (i = 0; i < nGlyphs; i++)
		IN1(format->code[i]);

	return format;
	}

void postRead(LongN start, Card32 length)
	{
	if (loaded)
		return;

	post = (postTbl *)memNew(sizeof(postTbl));
	SEEK_ABS(start);

	/* Read header */
	IN1(post->version);
	IN1(post->italicAngle);
	IN1(post->underlinePosition);
	IN1(post->underlineThickness);
	IN1(post->isFixedPitch);
	IN1(post->minMemType42);
	IN1(post->maxMemType42);
	IN1(post->minMemType1);
	IN1(post->maxMemType1);

	switch (post->version)
		{
	case VERSION(1,0):
		break;
	case VERSION(2,0):
		post->format = read2_0(length - TBL_HDR_SIZE);
		break;
	case VERSION(2,5):
		post->format = read2_5();
		break;
	case VERSION(3,0):
		break;
	case VERSION(4,0):
		post->format = read4_0(start);
		break;
	default:
		warning(SPOT_MSG_postBADVERS, VERSION_ARG(post->version));
		return;
		}

	loaded = 1;
	}

static Byte8 *tooBig(GlyphId glyphId, IntX *length)
	{
	warning(SPOT_MSG_GIDTOOLARGE, glyphId);
	*length = 0;
	return NULL;
	}

/* Get name from format 1.0 table */
static Byte8 *getName1_0(GlyphId glyphId, IntX *length)
	{
	if (glyphId > MAX_STD_INDEX)
		return tooBig(glyphId, length);
	else
		{
		*length = strlen(applestd[glyphId]);
		return applestd[glyphId];
		}
	}

/* Get name from format 2.0 table */
static Byte8 *getName2_0(Format2_0 *format, GlyphId glyphId, IntX *length)
	{
	if (glyphId > format->numberGlyphs)
		return tooBig(glyphId, length);
	else
		{
		IntX index = format->glyphNameIndex[glyphId];
		Byte8 *name;

		if ((index < 0) || (index > 32767))
		  {
			Byte8 gni[32];
			sprintf(gni, "glyphNameIndex[%d]", glyphId);
			warning(SPOT_MSG_BADINDEX, TAG_ARG(post_), index, gni);
			name = applestd[0];
			*length = strlen(name);
		  }
		else if (index > (long)MAX_STD_INDEX)
			{
			IntX i;
			name = (Byte8 *)format->names;
			for (i = 0; i < index - (IntX)STD_LENGTH; i++)
				name += *name + 1;
			*length = *name++;
			}
		else
			{
			name = applestd[index];
			*length = strlen(name);
			}
		return name;
		}
	}

/* Get name from format 2.5 table */
static Byte8 *getName2_5(Format2_5 *format, GlyphId glyphId, IntX *length)
	{
	if (glyphId > format->numberGlyphs)
		return tooBig(glyphId, length);
	else
		{
		Byte8 *name = applestd[format->offset[glyphId] + glyphId];
		*length = strlen(name);
		return name;
		}
	}

/* Get name from format 4.0 table */
static Byte8 *getNam4_0(Format4_0 *format, GlyphId glyphId, IntX *length)
	{
	if (glyphId >= nGlyphs)
		return tooBig(glyphId, length);
	else
		{
		static Byte8 gni[32];
		sprintf(gni, "a%hu", format->code[glyphId]);
		*length = strlen(gni);
		return gni;
		}
	}

/* Initialize name fetching */
IntX postInitName(void)
	{
	if (!loaded)
		{
		if (sfntReadTable(post_))
			return 0;
		}

	switch (post->version)
		{
	case VERSION(1,0):
	case VERSION(2,0):
	case VERSION(2,5):
	case VERSION(4,0):
		return 1;
	case VERSION(3,0):
		break;
	default:
		warning(SPOT_MSG_postBADVERS, VERSION_ARG(post->version));
		break;
		}
	return 0;
	}

/* Get glyph name for glyphId */
Byte8 *postGetName(GlyphId glyphId, IntX *length)
	{
	switch (post->version)
		{
	case VERSION(1,0):
		return getName1_0(glyphId, length);
	case VERSION(2,0):
		return getName2_0((Format2_0 *)post->format, glyphId, length);
	case VERSION(2,5):
		return getName2_5((Format2_5 *)post->format, glyphId, length);
	case VERSION(4,0):
		return getNam4_0((Format4_0 *)post->format, glyphId, length);
		}
	*length = 0;
	return NULL;	
	}

static void dump2_0(Format2_0 *format, IntX level)
	{
	IntX i;
	Byte8 *name = (Byte8 *)format->names;

	DL(2, (OUTPUTBUFF, "--- format 2.0\n"));
	DLu(2, "numberGlyphs=", format->numberGlyphs);

	DL(2, (OUTPUTBUFF, "--- glyphNameIndex[glyphId]=value\n"));
	for (i = 0; i < format->numberGlyphs; i++)
	  {
		IntX ix = format->glyphNameIndex[i];
		if ((ix < 0) || (ix > 32767))
		  {
			Byte8 gni[32];
			sprintf(gni, "glyphNameIndex[%d]", i);
			warning(SPOT_MSG_BADINDEX, TAG_ARG(post_), ix, gni);
		  }
		DL(2, (OUTPUTBUFF, "[%d]=%hu ", i, (Card16)ix));
	  }
	DL(2, (OUTPUTBUFF, "\n"));

	if (nNames > 0)
		{
		DL(2, (OUTPUTBUFF, "--- names[index]={len,<name>}\n"));
		for (i = 0; i < nNames; i++)
			{
			IntX length = *name++;

			DL(2, (OUTPUTBUFF, "[%d]={%u,<%.*s>} ", i, length, length, name));
			name += length;
			}
		DL(2, (OUTPUTBUFF, "\n"));
		}
	}

static void dump2_5(Format2_5 *format, IntX level)
	{
	IntX i;

	DL(2, (OUTPUTBUFF, "--- format 2.5\n"));
	DLu(2, "numberGlyphs=", format->numberGlyphs);

	DL(2, (OUTPUTBUFF, "--- offset[glyphId]=value\n"));
	for (i = 0; i < format->numberGlyphs; i++)
		DL(2, (OUTPUTBUFF, "[%d]=%u ", i, format->offset[i]));
	DL(2, (OUTPUTBUFF, "\n"));
	}
	
static void dump4_0(Format4_0 *format, IntX level)
	{
	IntX i;

	DL(2, (OUTPUTBUFF, "--- format 4.0\n"));
	DL(2, (OUTPUTBUFF, "--- code[glyphId]=code\n"));
	for (i = 0; i < nGlyphs; i++)
		DL(2, (OUTPUTBUFF, "[%d]=%hu ", i, format->code[i]));
	DL(2, (OUTPUTBUFF, "\n"));
	}
	
void postDump(IntX level, LongN start)
	{
	DL(1, (OUTPUTBUFF, "### [post] (%08lx)\n", start));

	if (!loaded)
		return;

	DLV(2, "version           =", post->version);
	DLF(2, "italicAngle       =", post->italicAngle);
	DLs(2, "underlinePosition =", post->underlinePosition);
	DLs(2, "underlineThickness=", post->underlineThickness);
	DLU(2, "isFixedPitch      =", post->isFixedPitch);
	DLU(2, "minMemType42      =", post->minMemType42);
	DLU(2, "maxMemType42      =", post->maxMemType42);
	DLU(2, "minMemType1       =", post->minMemType1);
	DLU(2, "maxMemType1       =", post->maxMemType1);

	switch (post->version)
		{
	case VERSION(1,0):
		break;
	case VERSION(2,0):
		dump2_0((Format2_0 *)post->format, level);
		break;
	case VERSION(2,5):
		dump2_5((Format2_5 *)post->format, level);
		break;
	case VERSION(3,0):
		break;
	case VERSION(4,0):
		dump4_0((Format4_0 *)post->format, level);
		}
	}

static void free2_0(Format2_0 *format)
	{
	memFree(format->glyphNameIndex);
	memFree(format->names); 
	memFree(format);
	}

static void free2_5(Format2_5 *format)
	{
	memFree(format->offset);
	memFree(format);
	}

static void free4_0(Format4_0 *format)
	{
	memFree(format->code);
	memFree(format);
	}

void postFree(void)
	{
	if (!loaded)
		return;

	switch (post->version)
		{
	case VERSION(2,0):
		free2_0((Format2_0 *)post->format);
		break;
	case VERSION(2,5):
		free2_5((Format2_5 *)post->format);
		break;
	case VERSION(4,0):
		free4_0((Format4_0 *)post->format);
		break;
		}
	memFree(post); post=NULL;
	loaded = 0;
	}
