/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)Dhead.c	1.2
 * Changed:    7/1/98 10:35:32
 ***********************************************************************/

#if OSX
	#include <sys/time.h>
#else
	#include <time.h>
#endif

#include "Dhead.h"
#include "sfnt_head.h"
#include "Dsfnt.h"

static headTbl head1;
static headTbl head2;
static IntX loaded1 = 0;
static IntX loaded2 = 0;
static char *dateFormat = "%a %b %d %H:%M:%S %Y";

void headRead(Card8 which, LongN start, Card32 length)
	{
	  headTbl *head;

	if (which == 1)
	  {
		if (loaded1)	return;
		else head = &head1;
	  }
	else if (which == 2)
	  {
		if (loaded2)	return;
		else head = &head2;
	  }

	SEEK_ABS(which, start);

	IN(which, head->version);
	IN(which, head->fontRevision);
	IN(which, head->checkSumAdjustment);
	IN(which, head->magicNumber);
	IN(which, head->flags);
	IN(which, head->unitsPerEm);
	IN_BYTES(which, sizeof(head->created), head->created);
	IN_BYTES(which, sizeof(head->modified), head->modified);
	IN(which, head->xMin);
	IN(which, head->yMin);
	IN(which, head->xMax);
	IN(which, head->yMax);
	IN(which, head->macStyle);
	IN(which, head->lowestRecPPEM);
	IN(which, head->fontDirectionHint);
	IN(which, head->indexToLocFormat);
	IN(which, head->glyphDataFormat);

	if (which == 1) loaded1 = 1;
	else if (which == 2) loaded2 = 1;
	}


#if FOR_REFERENCE_ONLY
/* Convert ANSI standard date/time format to Apple LongDateTime format.
   Algorithm adapted from standard Julian Day calculation. */
void ANSITime2LongDateTime(struct tm *ansi, longDateTime ldt)
	{
	unsigned long elapsed;
	int month = ansi->tm_mon + 1;
	int year = ansi->tm_year;

	if (month < 3)
		{
		/* Jan and Feb become months 13 and 14 of the previous year */
		month += 12;
		year--;
		}

	/* Calculate elapsed seconds since 1 Jan 1904 00:00:00 */
	elapsed = (((year - 4L) * 365 +		 		/* Add years (as days) */
				year / 4 +						/* Add leap year days */
				(306 * (month + 1)) / 10 - 64 + /* Add month days */
				ansi->tm_mday) * 				/* Add days */
			   24 * 60 * 60 +					/* Convert days to secs */
			   ansi->tm_hour * 60 * 60 +		/* Add hours (as secs) */
			   ansi->tm_min * 60 +				/* Add minutes (as secs) */
			   ansi->tm_sec);					/* Add seconds */

	/* Set value */
	ldt[0] = ldt[1] = ldt[2] = ldt[3] = 0;
	ldt[4] = (char)(elapsed>>24);
	ldt[5] = (char)(elapsed>>16);
	ldt[6] = (char)(elapsed>>8);
	ldt[7] = (char)elapsed;
	}
#endif


/* Convert Apple LongDateTime format to ANSI standard date/time format.
   Algorithm adapted from standard Julian Day calculation. */
static void LongDateTime2ANSITime(struct tm *ansitime, longDateTime ldt)
	{
	unsigned long elapsed = ((unsigned long)ldt[4]<<24 | 
							 (unsigned long)ldt[5]<<16 | 
							 ldt[6]<<8 | 
							 ldt[7]);
	long A = elapsed / (24 * 60 * 60L);
	long B = A + 1524;
	long C = (long)((B - 122.1) / 365.25);
	long D = (long)(C * 365.25);
	long E = (long)((B - D) / 30.6001);
	long F = (long)(E * 30.6001);
	long secs = elapsed - A * (24 * 60 * 60L);

	if (E > 13)
		{
		ansitime->tm_year = C + 1;
		ansitime->tm_mon = E - 14;
		ansitime->tm_yday = B - D - 429;
		}
	else
		{
		ansitime->tm_year = C;
		ansitime->tm_mon = E - 2;
		ansitime->tm_yday = B - D - 64;
		}
	ansitime->tm_mday = B - D - F;
	ansitime->tm_hour = secs / (60 * 60);
	secs -= ansitime->tm_hour * (60 * 60);
	ansitime->tm_min = secs / 60;
	ansitime->tm_sec = secs - ansitime->tm_min * 60;
	ansitime->tm_wday = (A + 5) % 7;
	ansitime->tm_isdst = 0;
#if SUNOS        
	ansitime->tm_zone = "GMT";
#endif
	}



#define CHECKREADASSIGN 	\
 if (which == 1){\
 if (!loaded1){\
  if (sfntReadATable(which, head_)){tableMissing(head_, client); goto out;}}\
	head = &head1;}\
 else if (which == 2){\
 if (!loaded2){\
  if (sfntReadATable(which, head_)){tableMissing(head_, client); goto out;}}\
    head = &head2;}


Byte8 tday[32];

Byte8 * headGetCreatedDate(Card8 which, Card32 client)
	{
	  struct tm tmp;
	  headTbl *head;

	  tday[0] = '\0';

	  CHECKREADASSIGN
	  LongDateTime2ANSITime(&tmp, head->created);
        if (strftime(tday, sizeof(tday), dateFormat, &tmp) == 0) {
            fprintf(stderr, "strftime returned 0");
            exit(EXIT_FAILURE);
        }
out:
	  return tday;
	}

Byte8 * headGetModifiedDate(Card8 which, Card32 client)
	{
	  struct tm tmp;
	  headTbl *head;

	  tday[0] = '\0';

	  CHECKREADASSIGN
	  LongDateTime2ANSITime(&tmp, head->created);
        if (strftime(tday, sizeof(tday), dateFormat, &tmp) == 0) {
            fprintf(stderr, "strftime returned 0");
            exit(EXIT_FAILURE);
        }
out:
	  return tday;
	}


void headDiff(LongN offset1, LongN offset2)
{

  if (head1.version != head2.version)
	{
	  DiffExists++;
	  note("< head version=%ld.%ld (%08lx)\n", VERSION_ARG(head1.version));
	  note("> head version=%ld.%ld (%08lx)\n", VERSION_ARG(head2.version));
	}
  if (head1.fontRevision != head2.fontRevision)
	{
	  DiffExists++;
	  note("< head fontRevision=%ld.%ld (%08lx)\n", VERSION_ARG(head1.fontRevision));
	  note("> head fontRevision=%ld.%ld (%08lx)\n", VERSION_ARG(head2.fontRevision));
	}
  if (head1.checkSumAdjustment != head2.checkSumAdjustment)
	{
	  DiffExists++;
	  note("< head checkSumAdjustment=%08lx\n", head1.checkSumAdjustment );
	  note("> head checkSumAdjustment=%08lx\n", head2.checkSumAdjustment );
	}
  if (head1.magicNumber != head2.magicNumber)
	{
	  DiffExists++;
	  note("< head magicNumber=%08lx\n", head1.magicNumber );
	  note("> head magicNumber=%08lx\n", head2.magicNumber );
	}
  if (head1.flags != head2.flags)
	{
	  DiffExists++;
	  note("< head flags=%04hx\n", head1.flags );
	  note("> head flags=%04hx\n", head2.flags );
	}
  if (head1.unitsPerEm != head2.unitsPerEm)
	{
	  DiffExists++;
	  note("< head unitsPerEm=%hu\n", head1.unitsPerEm );
	  note("> head unitsPerEm=%hu\n", head2.unitsPerEm );
	}
  if (head1.created != head2.created)
	{
	  DiffExists++;
	  note("< head created=%x%x%x%x%x%x%x%x (%s)\n",
		   head1.created[0], head1.created[1], 
		   head1.created[2], head1.created[3],
		   head1.created[4], head1.created[5], 
		   head1.created[6], head1.created[7], headGetCreatedDate(1, head_));
	  note("> head created=%x%x%x%x%x%x%x%x (%s)\n",
		   head2.created[0], head2.created[1], 
		   head2.created[2], head2.created[3],
		   head2.created[4], head2.created[5], 
		   head2.created[6], head2.created[7], headGetCreatedDate(2, head_));
	}
  if (head1.modified != head2.modified)
	{
	  DiffExists++;
	  note("< head modified=%x%x%x%x%x%x%x%x (%s)\n",
		   head1.modified[0], head1.modified[1], 
		   head1.modified[2], head1.modified[3],
		   head1.modified[4], head1.modified[5], 
		   head1.modified[6], head1.modified[7], headGetModifiedDate(1, head_));

	  note("> head modified=%x%x%x%x%x%x%x%x (%s)\n",
		   head2.modified[0], head2.modified[1], 
		   head2.modified[2], head2.modified[3],
		   head2.modified[4], head2.modified[5], 
		   head2.modified[6], head2.modified[7], headGetModifiedDate(2, head_));
	}
  if (head1.xMin != head2.xMin)
	{
	  DiffExists++;
	  note("< head xMin=%hd\n", head1.xMin );
	  note("> head xMin=%hd\n", head2.xMin );
	}
  if (head1.yMin != head2.yMin)
	{
	  DiffExists++;
	  note("< head yMin=%hd\n", head1.yMin );
	  note("> head yMin=%hd\n", head2.yMin );
	}
  if (head1.xMax != head2.xMax)
	{
	  DiffExists++;
	  note("< head xMax=%hd\n", head1.xMax );
	  note("> head xMax=%hd\n", head2.xMax );
	}
  if (head1.yMax != head2.yMax)
	{
	  DiffExists++;
	  note("< head yMax=%hd\n", head1.yMax );
	  note("> head yMax=%hd\n", head2.yMax );
	}
  if (head1.macStyle != head2.macStyle)
	{
	  DiffExists++;
	  note("< head macStyle=%04hx\n", head1.macStyle );
	  note("> head macStyle=%04hx\n", head2.macStyle );
	}
  if (head1.lowestRecPPEM != head2.lowestRecPPEM)
	{
	  DiffExists++;
	  note("< head lowestRecPPEM=%hu\n", head1.lowestRecPPEM );
	  note("> head lowestRecPPEM=%hu\n", head2.lowestRecPPEM );
	}
  if (head1.fontDirectionHint != head2.fontDirectionHint)
	{
	  DiffExists++;
	  note("< head fontDirectionHint=%hd\n", head1.fontDirectionHint );
	  note("> head fontDirectionHint=%hd\n", head2.fontDirectionHint );
	}
  if (head1.indexToLocFormat != head2.indexToLocFormat)
	{
	  DiffExists++;
	  note("< head indexToLocFormat=%hd\n", head1.indexToLocFormat );
	  note("> head indexToLocFormat=%hd\n", head2.indexToLocFormat );
	}
  if (head1.glyphDataFormat != head2.glyphDataFormat)
	{
	  DiffExists++;
	  note("< head glyphDataFormat=%hd\n", head1.glyphDataFormat );
	  note("> head glyphDataFormat=%hd\n", head2.glyphDataFormat );
	}
}

void headFree(Card8 which)
	{
	if (which == 1) loaded1 = 0;
	else if (which == 2) loaded2 = 0;
	}

/* Return head.indexToLocFormat */
IntX headGetLocFormat(Card8 which, Card16 *locFormat, Card32 client)
	{
	  headTbl *head;

	  CHECKREADASSIGN

	  *locFormat = head->indexToLocFormat;
	  return 0;

out: return 1;
	}

/* Return head.uintsPerEm */
IntX headGetUnitsPerEm(Card8 which, Card16 *unitsPerEm, Card32 client)
	{
	  headTbl *head;
	  CHECKREADASSIGN
	  *unitsPerEm = head->unitsPerEm;
	  return 0;
	out: return 1;
	}

/* Return head.flags & head_SET_LSB */
IntX headGetSetLsb(Card8 which, Card16 *setLsb, Card32 client)
	{
	  headTbl *head;
	  CHECKREADASSIGN
	*setLsb = (head->flags & head_SET_LSB) != 0;
	return 0;
	out: return 1;
	}

/* Return head.xMin, head.yMin, head.xMax, head.yMax */
IntX headGetBBox(Card8 which, Int16 *xMin, Int16 *yMin, Int16 *xMax, Int16 *yMax, 
				 Card32 client)
	{
	  headTbl *head;

	  CHECKREADASSIGN

	*xMin = head->xMin;
	*yMin = head->yMin;
	*xMax = head->xMax;
	*yMax = head->yMax;
	return 0;

out:
	  *xMin = 0;
	  *yMin = 0;
	  *xMax = 0;
	  *yMax = 0;
	  return 1;
	}
