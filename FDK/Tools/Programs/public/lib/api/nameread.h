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
    NAME_ID_COPYRIGHT   = 0,         /* copyright notice */
    NAME_ID_FAMILY      = 1,         /* family name */
    NAME_ID_SUBFAMILY   = 2,         /* sub-family name */
    NAME_ID_UNIQUEID    = 3,         /* unique font identifier */
    NAME_ID_FULL        = 4,         /* full name */
    NAME_ID_VERSION     = 5,         /* version */
    NAME_ID_POSTSCRIPT  = 6,         /* Postscript font name */
    NAME_ID_TRADEMARK   = 7,         /* trademark */
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

enum {
    NAME_READ_FAILED            = 0,
    NAME_READ_NAME_NOT_FOUND    = -1,
    NAME_READ_NAME_TOO_LONG     = -2
};

long nam_getASCIIName(nam_name tbl,
                    ctlSharedStmCallbacks *sscb,
                    char *buffer, unsigned long bufferLen,
                    unsigned short nameId,
                    int isPS);

/*  nam_getASCIIName() looks up the name table for an ASCII name string specified by a name ID.
    If a name string contains any Unicode characters outside of the ASCII range,
    they are removed from the result.

    The length of the name is returned as the result.
    If no name is found, NAME_READ_NAME_NOT_FOUND is returned.
    If a found name exceeds the given buffer length, NAME_READ_NAME_TOO_LONG is returned.

    tbl - name table data loaded by calling nam_loadname in nameread library.

    sscb - pointer to shared stream callback functions.

    buffer - where a name is returned.

    bufferLen - the allocated byte length of buffer. Note that a null character
    is added at the end of the name string, so the actual length of the name will be at most
    bufferLen-1.

    nameId - the name ID in the name table to be looked up.

    isPS - if non-zero, the returned name must be Postscript-safe
*/

long nam_getFamilyNamePrefix(nam_name nameTbl,
                            ctlSharedStmCallbacks *sscb,
                            char *buffer, unsigned long bufferLen);
/*  nam_getFamilyNamePrefix() returns a font name prefix string to be used
    to name a CFF2 variable font instance.
    The length of the name is returned as the result.
    If no name is found, NAME_READ_INST_NOT_FOUND is returned.
    If a found name exceeds the given buffer length, NAME_READ_NAME_TOO_LONG is returned.

    nameTbl - name table data loaded by calling nam_loadname in nameread library.

    sscb - pointer to shared stream callback functions.

    coords - the user design vector. May be NULL if the default instance or a named
    instance is specified.

    buffer - where a font name prefix stirng is returned.

    bufferLen - the allocated byte length of buffer. Note that a null character
    is added at the end of the name string, so the actual length of the name will be at most
    bufferLen-1. */

long nam_getNamedInstancePSName(nam_name nameTbl,
                            var_axes axesTbl,
                            ctlSharedStmCallbacks *sscb,
                            float *coords,
                            unsigned short axisCount,
                            int instanceIndex,
                            char *instanceName, unsigned long instanceNameLen);

/*  nam_getNamedInstancePSName() looks up the fvar table for a named instnace of a CFF2 variable font
    specified either by its instance index or a design vector of the instance.
    The length of the name is returned as the result.
    If no name is found, NAME_READ_INST_NOT_FOUND is returned.
    If a found name exceeds the given buffer length, NAME_READ_NAME_TOO_LONG is returned.

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

long nam_generateArbitraryInstancePSName(nam_name nameTbl,
                            var_axes axesTbl,
                            ctlSharedStmCallbacks *sscb,
                            float *coords,
                            unsigned short axisCount,
                            char *instanceName, unsigned long instanceNameLen);

/*  nam_generateArbitraryInstancePSName() generate a PS name of an arbitrary instance
    of a CFF2 variable font specified by a design vector of the instance.
    The length of the name is returned as the result.
    If no name is found, NAME_READ_INST_NOT_FOUND is returned.
    If a found name exceeds the given buffer length, NAME_READ_NAME_TOO_LONG is returned.

    nameTbl - name table data loaded by calling nam_loadname in nameread library.

    axesTbl - axes table data loaded by calling var_loadaxes in varread library.

    sscb - pointer to shared stream callback functions.

    coords - the user design vector.

    axisCount - the number of axes in the variable font.

    instanceName - where a generated PostScript name of the instance is returned.

    instanceNameLen - the allocated byte length of instanceName. Note that a null character
    is added at the end of the name string, so the actual length of the name will be at most
    instanceNameLen-1. */

long nam_generateLastResortInstancePSName(nam_name nameTbl,
                                var_axes axesTbl,
                                ctlSharedStmCallbacks *sscb,
                                float *coords,
                                unsigned short axisCount,
                                char *instanceName, unsigned long instanceNameLen);

/*  nam_generateLastResortInstancePSName() generates a unique PostScript font name
    for the specified instance of a CFF2 variable font by hashing the coordinate values
    as the last resort.
    The length of the name is returned as the result.
    If no name is found, NAME_READ_INST_NOT_FOUND is returned.
    If a name exceeds the given buffer length, NAME_READ_NAME_TOO_LONG is returned.

    nameTbl - name table data loaded by calling nam_loadname in nameread library.

    axesTbl - axes table data loaded by calling var_loadaxes in varread library.

    sscb - pointer to shared stream callback functions.

    coords - the user design vector. May be NULL if the default instance or a named
    instance is specified.

    axisCount - the number of axes in the variable font.

    instanceName - where a generated PostScript name of the instance is returned.

    instanceNameLen - the allocated byte length of instanceName. Note that a null character
    is added at the end of the name string, so the actual length of the name will be at most
    instanceNameLen-1. */

#ifdef __cplusplus
}
#endif

#endif /* NAMEREAD_H */
