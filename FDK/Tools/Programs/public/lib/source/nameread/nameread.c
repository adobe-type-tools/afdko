/* Copyright 2016 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

#include "ctlshare.h"
#include "nameread.h"
#include "varread.h"
#include "dynarr.h"
#include "sha1.h"

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

/* --------------------------- Constants  --------------------------- */

#define MAX_FAMILY_NAME_LENGTH      64
#define MAX_COORD_STRING_LENGTH     16

/* --------------------------- type Definitions  --------------------------- */

typedef struct					/* bsearch id match record */
	{
	unsigned short platformId;
	unsigned short platspecId;
	unsigned short languageId;
	unsigned short nameId;
	} MatchIds;

struct nam_name_					/* name table */
	{
	unsigned short format;		/* =0 */
	unsigned short count;
	unsigned short stringOffset;
	dnaDCL(nam_NameRecord, records);
	};

/* --------------------------- Functions  --------------------------- */

/* Load the name table. */
nam_name nam_loadname(sfrCtx sfr, ctlSharedStmCallbacks *sscb)
{
    nam_name nameTbl;
	int i;

	sfrTable *table = sfrGetTableByTag(sfr, CTL_TAG('n','a','m','e'));
	if (table == NULL)
    {
		sscb->message(sscb, "name table missing");
		return NULL;
    }
	sscb->seek(sscb, table->offset);

    nameTbl = sscb->memNew(sscb, sizeof(*nameTbl));
    if (!nameTbl) return NULL;

	/* Read and validate table format */
	nameTbl->format = sscb->read2(sscb);
	if (nameTbl->format != 0)
		sscb->message(sscb, "invalid name table format");

	/* Read rest of header */
	nameTbl->count			= sscb->read2(sscb);
	nameTbl->stringOffset 	= sscb->read2(sscb);

	/* Read name records */
    dnaINIT(sscb->dna, nameTbl->records, nameTbl->count, nameTbl->count);
	dnaSET_CNT(nameTbl->records, nameTbl->count);
	for (i = 0; i < nameTbl->records.cnt; i++)
    {
		nam_NameRecord *rec = &nameTbl->records.array[i];
		rec->platformId	= sscb->read2(sscb);
		rec->platspecId = sscb->read2(sscb);
		rec->languageId = sscb->read2(sscb);
		rec->nameId 	= sscb->read2(sscb);
		rec->length 	= sscb->read2(sscb);
		rec->offset 	= table->offset + nameTbl->stringOffset + sscb->read2(sscb);
    }

    return nameTbl;
}

void nam_freename(ctlSharedStmCallbacks *sscb, nam_name tbl)
{
    if (tbl) {
        dnaFREE(tbl->records);
        sscb->memFree(sscb, tbl);
    }
}

/* Match ids. */
static int CTL_CDECL matchIds(const void *key, const void *value)
	{
	MatchIds *a = (MatchIds *)key;
	nam_NameRecord *b = (nam_NameRecord *)value;
	if (a->platformId < b->platformId)
		return -1;
	else if (a->platformId > b->platformId)
		return 1;
	else if (a->platspecId < b->platspecId)
		return -1;
	else if (a->platspecId > b->platspecId)
		return 1;
	else if (a->languageId < b->languageId)
		return -1;
	else if (a->languageId > b->languageId)
		return 1;
	else if (a->nameId < b->nameId)
		return -1;
	else if (a->nameId > b->nameId)
		return 1;
	else 
		return 0;
	}

/* Return name record match specified ids or NULL if no match. */
nam_NameRecord *nam_nameFind(nam_name tbl,
							unsigned short platformId,
							unsigned short platspecId,
							unsigned short languageId,
							unsigned short nameId)
	{
	MatchIds match;

	if (tbl->records.cnt == 0)
		return NULL;	/* name table missing */

	/* Initialize match record */
	match.platformId	= platformId;
	match.platspecId	= platspecId;
	match.languageId	= languageId;
	match.nameId		= nameId;
	
	return (nam_NameRecord *)
		bsearch(&match, tbl->records.array, tbl->records.cnt, 
				sizeof(nam_NameRecord), matchIds); 
	}

static int checkNameChar(unsigned char ch, int isPS)
{
    if (isPS)
    {
        switch (ch) {
        case '[': case ']': case '(': case ')': case '{':
        case '}': case '<': case '>': case '/': case '%':
            return 0;
        default:
            return (ch >= 33 && ch <= 126);
        }
    }
    else
        return (ch >= 32) && (ch <= 126);
}

long nam_getASCIIName(nam_name nameTbl, ctlSharedStmCallbacks *sscb, char *buffer, unsigned long bufferLen, unsigned short nameId, int isPS)
{
    unsigned long i;
    long nameLen;
    unsigned char ch1, ch2;
    nam_NameRecord  *rec;

    rec = nam_nameFind(nameTbl, NAME_WIN_PLATFORM, NAME_WIN_UGL, NAME_WIN_ENGLISH, nameId);
    if (rec && rec->length > 0) {
        nameLen = 0;
        sscb->seek(sscb, rec->offset);
        for (i = 0; i + 1 < (unsigned long)rec->length; i += 2) {
            ch1 = sscb->read1(sscb);
            ch2 = sscb->read1(sscb);
            if (ch1 == 0 && checkNameChar(ch2, isPS)) {
                if ((unsigned long)nameLen + 1 >= bufferLen) {
                    sscb->message(sscb, "a name in the name table is longer than the given buffer");
                    return NAME_READ_NAME_TOO_LONG;
                }
                buffer[nameLen++] = (char)ch2;
            }
        }
        buffer[nameLen] = 0;
        return nameLen;
    }
    rec = nam_nameFind(nameTbl, NAME_MAC_PLATFORM, NAME_MAC_ROMAN, NAME_MAC_ENGLISH, nameId);
    if (rec) {
        nameLen = 0;
        sscb->seek(sscb, rec->offset);
        for (i = 0; i < (unsigned long)rec->length; i++) {
            ch1 = sscb->read1(sscb);
            if (checkNameChar(ch1, isPS)) {
                if ((unsigned long)nameLen + 1 >= bufferLen) {
                    sscb->message(sscb, "a name in the name table is longer than the given buffer");
                    return NAME_READ_NAME_TOO_LONG;
                }
                buffer[nameLen++] = (char)ch1;
            }
        }
        buffer[nameLen] = 0;
        return nameLen;
    }
    return NAME_READ_NAME_NOT_FOUND;
}

static long getPSName(nam_name nameTbl,
                    ctlSharedStmCallbacks *sscb,
                    char *nameBuffer, unsigned long nameBufferLen)
{
    return nam_getASCIIName(nameTbl, sscb, nameBuffer, nameBufferLen, NAME_ID_POSTSCRIPT, 1);
}

static unsigned long removeNonAlphanumChar(char *buffer, unsigned long len)
{
    unsigned long i, j;

    for (i = j = 0; i < len; i++) {
        if (isalnum((unsigned char)buffer[i])) {
            buffer[j++] = buffer[i];
        }
    }
    buffer[j] = 0;
    return j;
}

/* get the family name prefix for a variable font instance name */
long nam_getFamilyNamePrefix(nam_name nameTbl,
                            ctlSharedStmCallbacks *sscb,
                            char *buffer, unsigned long bufferLen)
{
    long    nameLen;

    nameLen = nam_getASCIIName(nameTbl, sscb, buffer, bufferLen, NAME_ID_POSTSCRIPT_PREFIX, 1);
    if (nameLen == NAME_READ_NAME_NOT_FOUND)
        nameLen = nam_getASCIIName(nameTbl, sscb, buffer, bufferLen, NAME_ID_TYPO_FAMILY, 1);
    if (nameLen == NAME_READ_NAME_NOT_FOUND)
        nameLen = nam_getASCIIName(nameTbl, sscb, buffer, bufferLen, NAME_ID_FAMILY, 1);
    if (nameLen > 0)
        nameLen = removeNonAlphanumChar(buffer, nameLen);

    if (nameLen > MAX_FAMILY_NAME_LENGTH) {
        sscb->message(sscb, "too long family name prefix");
        nameLen = NAME_READ_NAME_TOO_LONG;
    }
    return nameLen;
}

    /* allocate a temporary buffer long enough to hold a generated instance name */
static char *allocNameBuffer(ctlSharedStmCallbacks *sscb, long axisCount, unsigned long *bufferLen)
{
    char    *buffer;

    *bufferLen = MAX_FAMILY_NAME_LENGTH + 10 + (MAX_COORD_STRING_LENGTH * axisCount);
    buffer = (char *)sscb->memNew(sscb, *bufferLen);
    if (buffer == 0) {
        sscb->message(sscb, "failed to allocate memory");
        return NULL;
    }
    return buffer;
}

long nam_getNamedInstancePSName(nam_name nameTbl,
                            var_axes axesTbl,
                            ctlSharedStmCallbacks *sscb,
                            float *coords,
                            unsigned short axisCount,
                            int instanceIndex,
                            char *instanceName, unsigned long instanceNameLen)
{
    unsigned long   familyNameLen = 0;
    long            nameLen = NAME_READ_NAME_NOT_FOUND;
    unsigned short  subfamilyID, postscriptID;
    char            *buffer = 0;
    unsigned long   bufferLen;

    /* if the font is not a variable font or no coordinates are specified, return the default PS name */
    if (!axesTbl || !coords || !axisCount) {
        return getPSName(nameTbl, sscb, instanceName, instanceNameLen);
    }


    /* find a matching named instance in the fvar table */
    if (instanceIndex < 0)
        instanceIndex = var_findInstance(axesTbl, coords, axisCount, &subfamilyID, &postscriptID);
    if ((instanceIndex >= 0) && (postscriptID == 6 || ((postscriptID > 255) && (postscriptID < 32768)))) {
        nameLen = nam_getASCIIName(nameTbl, sscb, instanceName, instanceNameLen, postscriptID, 1);
        if (nameLen > 0)
            return nameLen;
    }

    /* allocate a temporary buffer long enough to hold a generated instance name */
    buffer = allocNameBuffer(sscb, axisCount, &bufferLen);
    if (buffer == NULL)
        return NAME_READ_FAILED;

    /* get the family name prefix */
    nameLen = nam_getFamilyNamePrefix(nameTbl, sscb, buffer, bufferLen);
    if (nameLen <= 0)
        goto cleanup;

    familyNameLen = (unsigned long)nameLen;

    /* append the style name from the instance if there is a matching one */
    if (instanceIndex >= 0) {
        char    *styleName = &buffer[nameLen];
        unsigned long   styleBufferLen = bufferLen - nameLen;
        unsigned long   styleLen;
        
        styleLen = nam_getASCIIName(nameTbl, sscb, styleName, styleBufferLen, subfamilyID, 0);
        if (styleLen > 0)
            styleLen = removeNonAlphanumChar(styleName, styleLen);
        if (styleLen > 0) {
            nameLen = nameLen + styleLen;
        }
        if ((unsigned long)nameLen + 1 > instanceNameLen)
            nameLen = NAME_READ_NAME_TOO_LONG;
        else
            strcpy(instanceName, buffer);
    }
    else
        nameLen = NAME_READ_NAME_NOT_FOUND;

cleanup:
    if (buffer)
        sscb->memFree(sscb, buffer);

    return nameLen;
}

/* convert a fractional number to a string in the buffer.
 * function returns the length of the string.
 * the buffer is assumed to be long enough.
 */
static unsigned long stringizeNum(char *buffer, unsigned long bufferLen, float value)
{
    unsigned long   i = 0;
    float   eps = 0.5f / (1 << 16);
    float   hi, lo;
    int     intPart;
    float   fractPart;
    int     neg = 0;
    int     power10;
    int     digit;

    if (value < 0.0f) {
        neg = 1;
        value = -value;
    }

    hi = value + eps;
    lo = value - eps;

    intPart = (int)floor(hi);
    if (intPart != (int)floor(lo)) {
        fractPart = 0.0f;
    } else {
        fractPart = hi - intPart;
    }
    
    if ((intPart == 0) && (fractPart == 0.0f)) {
        buffer[i++] = '0';
        return i;
    }

    if (neg) {
        if (i + 1 >= bufferLen) return 0;
        buffer[i++] = '-';
    }

    /* integer part */
    for (digit = 0, power10 = 1; intPart >= power10;) {
        digit++;
        power10 *= 10;
    }
    while (digit-- > 0) {
        power10 /= 10;
        buffer[i++] = '0' + (intPart / power10);
        intPart %= power10;
    }

    /* fractional part */
    if (fractPart >= eps) {
        buffer[i++] = '.';

        do {
            fractPart *= 10;
            eps *= 10;
            intPart = (int)fractPart;
            buffer[i++] = '0' + intPart;
            fractPart -= intPart;
        } while (fractPart >= eps);
    }

    return i;
}

long nam_generateArbitraryInstancePSName(nam_name nameTbl,
                            var_axes axesTbl,
                            ctlSharedStmCallbacks *sscb,
                            float *coords,
                            unsigned short axisCount,
                            char *instanceName, unsigned long instanceNameLen)
{
    unsigned long   familyNameLen = 0;
    long            nameLen;
    unsigned short  axis;
    char            *buffer = 0;
    unsigned long   bufferLen;

    /* if the font is not a variable font or no coordinates are specified, return the default PS name */
    if (!axesTbl || !coords || !axisCount) {
        return getPSName(nameTbl, sscb, instanceName, instanceNameLen);
    }


    /* allocate a temporary buffer long enough to hold a generated instance name */
    buffer = allocNameBuffer(sscb, axisCount, &bufferLen);
    if (buffer == NULL)
        return NAME_READ_FAILED;

    /* get the family name prefix */
    nameLen = nam_getFamilyNamePrefix(nameTbl, sscb, buffer, bufferLen);
    if (nameLen <= 0)
        goto cleanup;

    familyNameLen = (unsigned long)nameLen;

    /* create an arbitrary instance name from the coordinates */
    for (axis = 0; axis < axisCount; axis++) {
        unsigned long   coordStrLen;
        int i;
        unsigned long   tag;

        /* value range check */
        if (fabs(coords[axis]) > 32768.0f) {
            sscb->message(sscb, "coordinates value out of range: %f", coords[axis]);
            nameLen = 0;
            goto cleanup;
        }

        /* convert a coordinates value to string */
        buffer[nameLen++] = '_';
        coordStrLen = stringizeNum(&buffer[nameLen], bufferLen - nameLen, coords[axis]);
        nameLen += coordStrLen;

        /* append a axis name */
        if (var_getAxis(axesTbl, axis, &tag, NULL, NULL, NULL, NULL)) {
            sscb->message(sscb, "failed to get axis information");
            nameLen = 0;
            goto cleanup;
        }
        for (i = 3; i >= 0; i--) {
            char    ch = (char)((tag >> (i * 8)) & 0xff);
            if (ch < 32 || ch > 126) {
                sscb->message(sscb, "invalid character found in an axis tag");
                continue;
            }
            if (ch == ' ') continue;
            buffer[nameLen++] = ch;
        }
    }
    buffer[nameLen++] = 0;
    if (nameLen <= (long)instanceNameLen) {
        strncpy(instanceName, buffer, nameLen);
        instanceName[nameLen] = 0;
     }
    else
        nameLen = NAME_READ_NAME_TOO_LONG;

cleanup:
    if (buffer)
        sscb->memFree(sscb, buffer);

    return nameLen;
}

static void * nam_sha1_malloc(size_t size, void *hook)
{
    ctlSharedStmCallbacks   *sscb = (ctlSharedStmCallbacks *)hook;
    return sscb->memNew(sscb, size);
}

static void nam_sha1_free(sha1_pctx ctx, void *hook)
{
    ctlSharedStmCallbacks   *sscb = (ctlSharedStmCallbacks *)hook;
    sscb->memFree(sscb, ctx);
}

long nam_generateLastResortInstancePSName(nam_name nameTbl,
                                var_axes axesTbl,
                                ctlSharedStmCallbacks *sscb,
                                float *coords,
                                unsigned short axisCount,
                                char *instanceName, unsigned long instanceNameLen)
{
    unsigned long   familyNameLen = 0;
    long            nameLen;
    char            *buffer = 0;
    unsigned long   bufferLen;
    unsigned long   hashLen;
    unsigned long   i;

    /* if the font is not a variable font or no coordinates are specified, return the default PS name */
    if (!axesTbl || !coords || !axisCount) {
        return getPSName(nameTbl, sscb, instanceName, instanceNameLen);
    }

    /* allocate a temporary buffer long enough to hold a generated instance name */
    buffer = allocNameBuffer(sscb, axisCount, &bufferLen);
    if (buffer == NULL)
        return NAME_READ_FAILED;

    /* get the family name prefix */
    nameLen = nam_getFamilyNamePrefix(nameTbl, sscb, buffer, bufferLen);
    if (nameLen <= 0)
        goto cleanup;

    familyNameLen = (unsigned long)nameLen;

    /* generate a SHA1 value of the generated name as the identififier in a last resort name.
     */
    hashLen = sizeof(sha1_hash);
    for (i = 0; i < 2; i++) {
        if (familyNameLen + hashLen * 2 + 5 < instanceNameLen)
            break;

        if (i >= 1) {
            sscb->message(sscb, "name buffer not long enough to generate a last resort variable font instance name");
            nameLen = -2;
            goto cleanup;
        }
        hashLen /= 2;   /* give it another chance by halvening the hash length */
    }
    nameLen = familyNameLen;
    strncpy(instanceName, buffer, nameLen);
    instanceName[nameLen++] = '-';

    {
        sha1_pctx   sha1_ctx = sha1_init(nam_sha1_malloc, sscb);
        sha1_hash   hash;
        int         error;
    
        if (!sha1_ctx) {
            nameLen = 0;
            goto cleanup;
        }

        error = sha1_update(sha1_ctx, (unsigned char*)buffer, nameLen - 1);
        error |= sha1_finalize(sha1_ctx, nam_sha1_free, hash, sscb);

        if (error) {
            sscb->message(sscb, "failed to generate hash during a last resort variable font instance name generation");
            nameLen = 0;
            goto cleanup;
        }

        /* append the hash value has a hex string */
        for (i = 0; i < hashLen; i++) {
            static const char hexChar[] = "0123456789ABCDEF";
            unsigned char   byte = hash[i];
            instanceName[nameLen++] = hexChar[byte >> 4];
            instanceName[nameLen++] = hexChar[byte & 0xF];
        }

        instanceName[nameLen++] = '.';
        instanceName[nameLen++] = '.';
        instanceName[nameLen++] = '.';
        instanceName[nameLen++] = 0;

        sscb->message(sscb, "last resort variable font instance name %s generated for %s",
                            instanceName, buffer);
    }

cleanup:
    if (buffer)
        sscb->memFree(sscb, buffer);

    return nameLen;
}
