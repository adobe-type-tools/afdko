/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************
 * SCCS Id:    @(#)global.c	1.20
 * Changed:    8/3/99 10:11:17
 ***********************************************************************/

/*
 * Miscellaneous functions and system calls.
 */

#include <stdarg.h> 
#include <string.h>
#include <ctype.h>

#include "global.h"
#include "opt.h"
#include "sfnt.h"
#include "maxp.h"
#include "TYP1.h"
#include "CID_.h"
#include "post.h"
#include "cmap.h"
#include "CFF_.h"
#include "glyf.h"
#include "head.h"
#include "name.h"
#include "map.h"
#if MEMCHECK
	#include "memcheck.h"
#endif
Global global;			/* Global data */
static IntX nameLookupType=0;
extern jmp_buf mark;
char *dateFormat = "%a %b %d %H:%M:%S %Y";

/* ### Error reporting */

/* Print fatal message */
void fatal(IntX msgfmtID, ...)
	{
	const Byte8 *fmt;
	va_list ap;

	fflush(OUTPUTBUFF);
	va_start(ap, msgfmtID);
	fprintf(stderr, "%s [FATAL]: ", global.progname);
	fmt = spotMsg(msgfmtID);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	/* longjmp(mark, -1); */
	exit(1);
	}

/* Print informational message */
void message(IntX msgfmtID, ...)
	{
	const Byte8 *fmt;
	va_list ap;

	fflush(OUTPUTBUFF);
	va_start(ap, msgfmtID);
	fprintf(stderr, "%s [MESSAGE]: ", global.progname);
	fmt = spotMsg(msgfmtID);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	}

/* Print warning message */
void warning(IntX msgfmtID, ...)
	{
	const Byte8 *fmt;
	va_list ap;

	fflush(OUTPUTBUFF);
	va_start(ap, msgfmtID);
	fprintf(stderr, "%s [WARNING]: ", global.progname);
	fmt = spotMsg(msgfmtID);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	}

void featureWarning(IntX level, IntX msgfmtID, ...)
	{
	const Byte8 *fmt;
	va_list ap;

	fflush(OUTPUTBUFF);
	va_start(ap, msgfmtID);
	fmt = spotMsg(msgfmtID);
	fprintf(OUTPUTBUFF, "# [WARNING]: ");
	vfprintf(OUTPUTBUFF, fmt, ap);

	//fprintf(stderr, "%s [WARNING]: ", global.progname);
	//vfprintf(stderr, fmt, ap);
	va_end(ap);
	}

/* Print Simple informational message */
void inform(IntX msgfmtID, ...)
	{
	const Byte8 *fmt;
	va_list ap;

#ifndef EXECUTABLE
	if(opt_Present("-O"))
		return;
#endif
	fflush(stderr);
	va_start(ap, msgfmtID);
	fmt = spotMsg(msgfmtID);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	}

/* Warn of missing table */
IntX tableMissing(Card32 table, Card32 client)
	{
	warning(SPOT_MSG_TABLEMISSING, TAG_ARG(client), TAG_ARG(table));
	return 1;
	}

/* ### Memory management */

/* Quit on memory error */
void memError(void)
	{
	fatal(SPOT_MSG_NOMOREMEM);
	}

/* Allocate memory */
void *memNew(size_t size)
	{
	void *ptr;
	if (size == 0) size = 4;
	ptr = malloc(size);
	if (ptr == NULL)
		memError();
	else
		memset((char*)ptr, 0, size);
#if MEMCHECK
	memAllocated(ptr);
#endif
	return ptr;
	}
	
/* Resize allocated memory */
void *memResize(void *old, size_t size)
{
	void *new = realloc(old, size);
	if (new == NULL)
		memError();
	if (old!=new){
#if MEMCHECK
		memDeallocated(old);
		memAllocated(new);
#endif
	}
	return new;
}

/* Free memory */
void memFree(void *ptr)
	{
	if (ptr!=NULL){
#if MEMCHECK
		memDeallocated(ptr);
#endif
		free(ptr);
		}
	}

/* ### Miscellaneous */

/* Parse id list of the following forms: "N" the id N, and "N-M" the ids N-M */
IntX parseIdList(Byte8 *list, IdList *ids)
	{
	Byte8 *p;

	for (p = strtok(list, ","); p != NULL; p = strtok(NULL, ","))
		{
		IntX one;
		IntX two;

		if (sscanf(p, "%d-%d", &one, &two) == 2)
			;
		else if (sscanf(p, "%d", &one) == 1)
			two = one;
		else
			return 1;
			
		if (one < 0 || two < 0)
			return 1;	/* Negative numbers aren't allowed */

		/* Add id range */
		if (one > two)
			while (one >= two)
				*da_NEXT(*ids) = one--;
		else
			while (one <= two)
				*da_NEXT(*ids) = one++;
		}
	return 0;
	}

/* Get number of glyphs in font */
IntX getNGlyphs(Card16 *nGlyphs, Card32 client)
	{
	*nGlyphs = 0;
    if (nameLookupType == 0)
    	initGlyphNames();
    
	if ((nameLookupType == 1) || (nameLookupType == 2))
		maxpGetNGlyphs(nGlyphs, client);
	else if (nameLookupType == 2)
		maxpGetNGlyphs(nGlyphs, client);
	else if (nameLookupType == 3)
		CFF_GetNGlyphs(nGlyphs, client);
	else if (nameLookupType == 4)
		TYP1GetNGlyphs(nGlyphs, client);
	else if (nameLookupType == 5)
		CID_GetNGlyphs(nGlyphs, client);
	else
		maxpGetNGlyphs(nGlyphs, client);
	if (!*nGlyphs)
		return 1;
	return 0;
	}

void getMetrics(GlyphId glyphId, 
						   IntX *origShift, 
						   IntX *lsb, IntX *rsb, IntX *hwidth, 
						   IntX *tsb, IntX *bsb, IntX *vwidth, IntX *yorig)
	{
	  /* check to see that glyphId is within range: */
	  {
		Card16 nglyphs;
		if (getNGlyphs(&nglyphs, TAG('g','l','o','b'))) /*maxpGetNGlyphs(&nglyphs, TAG('g','l','o','b')))*/
		  {
			glyphId = 0;
		  }
		if (glyphId >= nglyphs)
		  {
			warning(SPOT_MSG_GIDTOOLARGE, glyphId);
			glyphId = 0;
		  }
	  }

	  /* try CFF first */
	  CFF_getMetrics(glyphId, 
					 origShift, 
					 lsb, rsb, hwidth, 
					 tsb, bsb, vwidth, yorig);
	  
	  if ((*origShift == 0) &&
		  (*lsb == 0) && (*rsb == 0) &&
		  (*hwidth == 0) && (*vwidth == 0) &&
		  (*tsb == 0) && (*bsb == 0))
		{
		  glyfgetMetrics(glyphId, 
						 origShift, 
						 lsb, rsb, hwidth, 
						 tsb, bsb, vwidth, yorig);
		}
	}


IntX getFontBBox (Int16 *xMin, Int16 *yMin, Int16 *xMax, Int16 *yMax)
	{
	  if (headGetBBox(xMin, yMin, xMax, yMax, TAG('g','l','o','b')))
		{
		  return CFF_GetBBox(xMin, yMin, xMax, yMax, TAG('g','l','o','b'));
		}
	 return 0;
   }

IntX isCID(void)
	{	  
	  if (CID_isCID()) return 1;
	  if (CFF_isCID()) return 1;
	  return 0;
	}

Byte8 *getFontName(void)
	{  
	  if (isCID())
		{
		  return namePostScriptName();
		}
	  else 
		{
		  return nameFontName();
		}
	}
/*#include "setjmp.h"

/extern jmp_buf mark;
*/
/* Quit processing */
void quit(IntN status)
	{
	exit(0);
	/* longjmp(global.env, status+1); */
	}

/* Initialize glyph name fetching */
void initGlyphNames(void)
	{
	  if (CFF_InitName())
		nameLookupType = 3;
	  else if (postInitName())
		nameLookupType = 1;
	  else if (cmapInitName())
		nameLookupType = 2;
	  else if (0 == sfntReadTable(TYP1_)) 
		nameLookupType = 4;
	  else if (0 == sfntReadTable(CID__) )
		nameLookupType = 5;
	  else
		nameLookupType = 6;
	}

/* Get glyph name. If unable to get a name return "@<gid>" string. Returns
   pointer to SINGLE static buffer so subsequent calls will overwrite. */
Byte8 *getGlyphName(GlyphId glyphId, IntX forProofing)
	{
	static Byte8 name[NAME_LEN + 1];
	static Byte8 nicename[NAME_LEN + 7]; /* allow an extra 6 chars for GID. */
	Byte8 *p;
	IntX length = 0;

	if (glyphId==0xffff)
		{
		sprintf(name, "undefined");
		return name;
		}
	
    if (nameLookupType == 0) initGlyphNames();

	/* check to see that glyphId is within range: */
	{
	  Card16 nglyphs;
	  if (getNGlyphs(&nglyphs, TAG('g','l','o','b'))) /*maxpGetNGlyphs(&nglyphs, TAG('g','l','o','b')))*/
		{
		  sprintf(name, "@%hu", glyphId); 
		  return name;
		}
	  if (glyphId >= nglyphs)
		{
		  warning(SPOT_MSG_GIDTOOLARGE, glyphId);
		  glyphId = 0;
		}
	}

	if (nameLookupType == 1)
		p = postGetName(glyphId, &length);
	else if (nameLookupType == 2)
		p = cmapGetName(glyphId, &length);
	else if (nameLookupType == 3)
	  p = CFF_GetName(glyphId, &length, forProofing);

	/* For names derived from the post table or from the cmap table, we add the GID as a string.
	This is because more than one glyph can end up with the same name under some circumstances.
	Also needed for nameLookupType == 4, where we can't get the name.
	*/
	
	if (length == 0)
	  {
		if (global.flags & SUPPRESS_GID_IN_NAME)
			sprintf(name, "%hu", glyphId);
		else
			sprintf(name, "%hu@%hu", glyphId, glyphId);
	  }
	else
	  {
		if (!opt_Present("-m"))
		  {
			if ((( (nameLookupType == 1) || (nameLookupType == 2) )) && (!(global.flags & SUPPRESS_GID_IN_NAME)))
			  	sprintf(name, "%.*s@%hu", (length > (NAME_LEN-6) ) ? (NAME_LEN-6) : length, p, glyphId);
			else
				sprintf(name, "%.*s", (length > NAME_LEN) ? NAME_LEN : length, p);
		  }
		else
		  { /* perform nice name mapping */
			const Byte8 *p2 = NULL;
			IntX newlen = 0;

			sprintf(name, "%.*s", (length > NAME_LEN) ? NAME_LEN : length, p);
			if ((p2 = map_nicename(name, &newlen)) != NULL)
			  {
			    if (forProofing)
			    	{
#define INDICATOR_MARKER 0x27 /* tick mark */
					sprintf(nicename, "%c%.*s", INDICATOR_MARKER,
						(newlen > NAME_LEN) ? NAME_LEN : newlen, p2);
					}
				else
					{
					sprintf(nicename, "%.*s",
						(newlen > NAME_LEN) ? NAME_LEN : newlen, p2);
					}
			  	if ((( (nameLookupType == 1) || (nameLookupType == 2) )) && (!(global.flags & SUPPRESS_GID_IN_NAME)))
			  		{
					/* Don't need to shorten name is if is < NAME_LEN
						when the GID is added; nicename has the space. */
					char* p  = &nicename[newlen];
			  		sprintf(p, "@%hu",glyphId);
			  		}
			  		
				return nicename;
			  }
			else
			  {
			  if ((( (nameLookupType == 1) || (nameLookupType == 2) )) && (!(global.flags & SUPPRESS_GID_IN_NAME)))
				sprintf(name, "%.*s@%hu", (length > (NAME_LEN-6) ) ? (NAME_LEN-6) : length, p, glyphId);
			  else
			  	sprintf(name, "%.*s", (length > NAME_LEN) ? NAME_LEN : length, p);
			  }
		  }
	  }

	return name;
#undef NAME_LEN	
	}

/* Dump all glyph names in font */
void dumpAllGlyphNames(IntN docrlfs)
	{
	IntX i;
	Card16 nGlyphs;

	initGlyphNames();
	if (getNGlyphs(&nGlyphs, TAG('d','u','m','p')))
		{
		warning(SPOT_MSG_UNKNGLYPHS);
		return;
		}

	fprintf(OUTPUTBUFF,  "--- names[glyphId]=<name>\n");
	for (i = 0; i < nGlyphs; i++) 
		{
			fprintf(OUTPUTBUFF,  "[%d]=<%s> ", i, getGlyphName(i, 0));
			if (docrlfs)
				fprintf(OUTPUTBUFF,  "\n");
		}
	fprintf(OUTPUTBUFF,  "\n");
	}

