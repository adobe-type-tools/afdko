/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/
#include "buildfont.h"
#include "transitionalchars.h"

/* Called when making an AFM file. */

extern void WriteCharMetrics(FILE *, indx);

extern indx create_entry(char *name,
			 char *filename,
			 short width,
			 boolean derived,
			 indx dirix,
			 short masters,
			 short hintDir);

extern long setnoncomps(void);

extern void AddCCtoTable(char *, BboxPtr, BboxPtr, short, indx);

extern void AddTransitionaltoTable(char *charname, Transitions *tr);

extern void SetupCharTable(void);

/* free strings allocated for char_table */
extern void FreeCharTab(char *);

extern void sortchartab(boolean);

extern void GetWidthandBbox(char *, short *, BboxPtr, boolean, indx);

/* Returns whether the given character is in the char set and has
a bez file. */
extern boolean CharExists(char *);

/* Check that a bez file has been seen for the given file name */
extern boolean CharFileExists(char *);

extern void set_char_width(char *, long, short *, indx);

extern long writechars(boolean, char **, long *, boolean, boolean);

extern long writesubrs(long *);

extern long getCapDY(char *, boolean, indx);

extern void writefontbbox(char *);

extern void WriteBaseDesignBBox(FILE *, indx);

extern long CharNameCost(void);

extern void SetCharEncoding(boolean, boolean);
