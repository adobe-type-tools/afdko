/* Copyright 2016 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

#ifndef NAMEREAD_H
#define NAMEREAD_H

#include "ctlshare.h"
#include "sfntread.h"
#include "varread.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Name Table Reader Library
   =======================================
   This library reads the name table.
*/

/* constants/types */

typedef struct					/* name record */
	{
	unsigned short platformId;
	unsigned short platspecId;
	unsigned short languageId;
	unsigned short nameId;
	unsigned short length;
	unsigned long offset;
	} nam_NameRecord;

struct nam_name_;					/* name table */
typedef struct nam_name_ *nam_name;

/* name and cmap table platform and platform-specific ids */
enum
	{
	/* Platform ids */
	NAME_UNI_PLATFORM	= 0,		/* Unicode */
	NAME_MAC_PLATFORM	= 1,		/* Macintosh */
	NAME_WIN_PLATFORM	= 3,		/* Microsoft */

	/* Unicode platform-specific ids */
	NAME_UNI_DEFAULT	= 0,		/* Default semantics */
	NAME_UNI_VER_1_1	= 1,		/* Version 1.1 semantics */
	NAME_UNI_ISO_10646	= 2,		/* ISO 10646 semantics */
	NAME_UNI_VER_2_0	= 3,		/* Version 2.0 semantics */
	
	/* Macintosh platform-specific and language ids */
	NAME_MAC_ROMAN		= 0,		/* Roman */
	NAME_MAC_ENGLISH	= 0,		/* English language */
	
	/* Windows platform-specific and language ids */
	NAME_WIN_SYMBOL		= 0,		/* Undefined index scheme */
	NAME_WIN_UGL		= 1,		/* UGL */
	NAME_WIN_ENGLISH	= 0x409		/* English (American) language */
	};

/* name IDs */
enum {
    NAME_ID_FAMILY      = 1,         /* family name */
    NAME_ID_POSTSCRIPT  = 6,         /* Postscript font name */
    NAME_ID_TYPO_FAMILY = 16,        /* typographic family name */
    NAME_ID_CIDFONTNAME = 20,        /* CID font name */
    NAME_ID_POSTSCRIPT_PREFIX = 25   /* Postscript font name prefix */
    };

/* functions */
nam_name nam_loadname(sfrCtx sfr, ctlSharedStmCallbacks *sscb);

/*  nam_loadname() loads the name table from an SFNT font.
    If not found, NULL is returned.

    sfr - context pointer created by calling sfrNew in sfntread library.

    sscb - a pointer to shared stream callback functions.
*/

void nam_freename(ctlSharedStmCallbacks *sscb, nam_name tbl);

/*  nam_freename() frees the name table loaded by calling nam_loadname().

    sscb - a pointer to shared stream callback functions.

    tbl - a pointer to the name table to be freed.
*/

nam_NameRecord *nam_nameFind(nam_name tbl,
							unsigned short platformId,
							unsigned short platspecId,
							unsigned short languageId,
							unsigned short nameId);

/*  nam_nameFind() looks up the name table for a name record.
    A pointer to a found name record is returned.
    If not found, NULL is returned.

    tbl - name table data loaded by calling nam_loadname in nameread library.

    platformId - the platform ID of the name record to be returned.

    platspecId - the platform specific ID of the name record to be returned.

    languageId - the language ID of the name record to be returned.

    nameId - the name ID in the name table to be retrieved.
*/

long nam_getASCIIName(nam_name tbl,
                    ctlSharedStmCallbacks *sscb,
                    char *buffer, unsigned long bufferLen,
                    unsigned short nameId);

/*  nam_getASCIIName() looks up the name table for an ASCII name string specified by a name ID.
    If a name string contains any Unicode characters outside of the ASCII range,
    they are removed from the result.

    The length of the name is returned as the result.
    If no name is found, -1 is returned.
    If a found name exceeds the given buffer length, -2 is returned.

    tbl - name table data loaded by calling nam_loadname in nameread library.

    sscb - pointer to shared stream callback functions.

    buffer - where a name is returned.

    bufferLen - the allocated byte length of buffer. Note that a null character
    is added at the end of the name string, so the actual length of the name will be at most
    bufferLen-1.

    nameId - the name ID in the name table to be looked up.
*/

long nam_generateInstancePSName(nam_name nameTbl,
                                var_axes axesTbl,
                                ctlSharedStmCallbacks *sscb,
                                float *coords,
                                unsigned short axisCount,
                                int instanceIndex,
                                char *instanceName, unsigned long instanceNameLen);

/*  nam_generateInstancePSName() generates a PostScript font name for the specified instance,
    given the name table and the axes table data obtained from a CFF2 variable font.
    The length of the name is returned as the result.
    If no name is found, -1 is returned.
    If a found name exceeds the given buffer length, -2 is returned.

    nameTbl - name table data loaded by calling nam_loadname in nameread library.

    axesTbl - axes table data loaded by calling var_loadaxes in varread library.

    sscb - pointer to shared stream callback functions.

    coords - the user design vector. May be NULL if the default instance or a named
    instance is specified.

    axisCount - the number of axes in the variable font.

    instanceIndex - specifies the named instance. If -1, coords specifies an
    arbitrary instance.

    instanceName - where a generated PostScript name of the instance is returned.

    instanceNameLen - the allocated byte length of instanceName. Note that a null character
    is added at the end of the name string, so the actual length of the name will be at most
    instanceNameLen-1. */

#ifdef __cplusplus
}
#endif

#endif /* NAMEREAD_H */
