/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************
 * SCCS Id:    @(#)global.c	1.11
 * Changed:    4/7/98 11:09:42
 ***********************************************************************/

/*
 * Miscellaneous functions and system calls.
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h> 
#include <string.h>
#include <ctype.h>
#ifdef _WINDOWS
  #include <time.h>
#else
  #include <sys/time.h>
#endif
#include "Dglobal.h"
#include "Dopt.h"
#include "Dsfnt.h"
#include "Dhead.h"
#include "Dname.h"


Global global;			/* Global data */
IntX level = 0;
IntX DiffExists = 0;

static IntX nameLookupType1=0;
static IntX nameLookupType2=0;
static char *dateFormat = "%a %b %d %H:%M:%S %Y";


/* Warn of missing table */
IntX tableMissing(Card32 table, Card32 client)
	{
	warning("%c%c%c%c can't read %c%c%c%c because table missing\n",
			TAG_ARG(client), TAG_ARG(table));
	return 1;
	}

/* ### Memory management */

/* Quit on memory error */
void memError(void)
	{
	fatal("out of memory\n");
	}

/* Allocate memory */
void *memNew(size_t size)
	{
	void *ptr;
	if (size == 0) size = 4;
	ptr = malloc(size);
	if (ptr == NULL)
		memError();
	return ptr;
	}
	
/* Resize allocated memory */
void *memResize(void *old, size_t size)
	{
	void *new = realloc(old, size);
	if (new == NULL)
		memError();
	return new;
	}

/* Free memory */
void memFree(void *ptr)
	{
	  if (ptr)	free(ptr);
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
IntX getNGlyphs(Card8 which, Card16 *nGlyphs, Card32 client)
	{

#if 1
	return 0;
#else
	if ((sfntReadATable(which, maxp_) || maxpGetNGlyphs(which, nGlyphs, client)) &&
		CFF_GetNGlyphs(which, nGlyphs, client) &&
		TYP1GetNGlyphs(which, nGlyphs, client) &&
		CID_GetNGlyphs(which, nGlyphs, client))
		return 1;
	return 0;
#endif
	}



IntX isCID(Card8 which)
	{	  
#if 1
	  return 0;
#else
	  if (CID_isCID(which)) return 1;
	  if (CFF_isCID(which)) return 1;

	  return 0;
#endif
	}

Byte8 *getFontName(Card8 which)
	{  
	  if (isCID(which))
		return namePostScriptName(which);
	  else 
		return nameFontName(which);
	}


/* Quit processing */
void quit(IntN status)
	{
	/* longjmp(global.env, status + 1); */
	exit(status);
	}

/* Initialize glyph name fetching */
void initGlyphNames(Card8 which)
	{
	  IntX typ;
#if 1
	  typ = 4;
#else
	  if (CFF_InitName(which))
		typ = 3;
	  else if (postInitName(which))
		typ = 1;
	  else if (cmapInitName(which))
		typ = 2;
	  else 
		typ = 4;
#endif

	  if (which == 1)
		nameLookupType1 = typ;
	  else
		nameLookupType2 = typ;
	}

/* Get glyph name. If unable to get a name return "@<gid>" string. Returns
   pointer to SINGLE static buffer so subsequent calls will overwrite. */
Byte8 *getGlyphName(Card8 which, GlyphId glyphId)
	{
#define NAME_LEN 32
	static Byte8 name[NAME_LEN];
#if 1
	sprintf( name, "@%hu", glyphId);
	return name;
#else
	if (which == 1)
	  {
		if (nameLookupType1 == 0) initGlyphNames(1);
		if (nameLookupType1 == 1)
		  p = postGetName(1, glyphId, &length);
		else if (nameLookupType1 == 2)
		  p = cmapGetName(1, glyphId, &length);
		else if (nameLookupType1 == 3)
		  p = CFF_GetName(1, glyphId, &length);
	  }
	else
	  {
		if (nameLookupType2 == 0) initGlyphNames(2);
		if (nameLookupType2 == 1)
		  p = postGetName(2, glyphId, &length);
		else if (nameLookupType2 == 2)
		  p = cmapGetName(2, glyphId, &length);
		else if (nameLookupType2 == 3)
		  p = CFF_GetName(2, glyphId, &length);
	  }
	if (length == 0)
		sprintf( name, "@%hu", glyphId);
	else
		sprintf( name, "%.*s", (length > NAME_LEN) ? NAME_LEN : length, p);
	
	return name;
#endif
#undef NAME_LEN	
	}

static Byte8 tday[32];

Byte8 * fileModTimeString(Card8 which, Byte8 *fname)
{
  Byte8 *str = NULL;
  struct stat s;

  str = headGetModifiedDate(which, TAG('f','i','l','e'));
  if ((str != NULL) && (str[0] != '\0'))
	  return str;
  else
	{
      struct tm *tmp;
     tday[0] = '\0';

        stat(fname, &s);
        tmp = localtime(&(s.st_mtime));
        if (strftime(tday, sizeof(tday), dateFormat, tmp) == 0) {
            fprintf(stderr, "strftime returned 0");
            exit(EXIT_FAILURE);
        }
	  return tday;
	}
}

Byte8 * ourtime(void)
{
  time_t secs;
  struct tm *tmp;
  tday[0] = '\0';
  (void) time(&secs);
  tmp = localtime(&secs);
    if (tmp == NULL) {
        perror("localtime");
        exit(EXIT_FAILURE);
    }

    if (strftime(tday, sizeof(tday), dateFormat, tmp) == 0) {
        fprintf(stderr, "strftime returned 0");
        exit(EXIT_FAILURE);
    }
  return tday;
}
