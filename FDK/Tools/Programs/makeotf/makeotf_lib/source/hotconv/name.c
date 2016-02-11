/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/

/*
 *	Naming table.
 */

#include "name.h"

#include <stdlib.h>
#ifdef _WIN32
#include <time.h>
#else
#include <sys/time.h>
#endif
#include <stdio.h>

/* ---------------------------- Table Definition --------------------------- */

typedef struct {
	unsigned short platformId;
	unsigned short platspecId;
	unsigned short languageId;
	unsigned short nameId;
	unsigned short length;
	unsigned short offset;
} NameRecord;
#define NAME_REC_SIZE (2 * 6)

typedef struct {
	unsigned short format;
	unsigned short count;
	unsigned short stringOffset;
	dnaDCL(NameRecord, record);
	dnaDCL(char, strings);
} nameTbl;
#define TBL_HDR_SIZE (2 * 3)

/* Name records are intially sorted by the string and later by id */

struct nameCtx_ {
	unsigned short userNameId;  /* User-defined name id */
	dnaDCL(char, tmp);          /* Temporary conversion buffer */
	dnaDCL(char, addstrs);      /* Added string accumulator */
	nameTbl tbl;                /* Table data */
	hotCtx g;                   /* Package context */
};

/* ----------------------------- Name Retrieval ---------------------------- */

#define MATCH_ANY   0xffff      /* Wildcard name id attribute */

/* Enumerate name string(s) for record(s) matching supplied arguments. Search
   begins at the record indexed by the "index" argument -- initially 0. If the
   search suceeds the index of the record is returned. This index should be
   incremented before the next iteration of the function so that the new search
   begins at the following record. If the search fails, -1 is returned and the
   index will need to be reset to 0 before commencing a new search. */

static int enumNames(nameCtx h, int index,
                     unsigned short platformId,
                     unsigned short platspecId,
                     unsigned short languageId,
                     unsigned short nameId) {
	for (; index < h->tbl.record.cnt; index++) {
		NameRecord *rec = &h->tbl.record.array[index];

		if ((platformId == MATCH_ANY || platformId == rec->platformId) &&
		    (platspecId == MATCH_ANY || platspecId == rec->platspecId) &&
		    (languageId == MATCH_ANY || languageId == rec->languageId) &&
		    (nameId == MATCH_ANY || nameId == rec->nameId)) {
			/* Found match */
			return index;
		}
	}
	/* No match found */
	return -1;
}

/* Check for name. Return 1 if name is absent else 0. */
static int noName(nameCtx        h,
                  unsigned short platformId,
                  unsigned short platspecId,
                  unsigned short languageId,
                  unsigned short nameId) {
	return enumNames(h, 0, platformId, platspecId, languageId, nameId) == -1;
}

/* Check for Windows default name. Return 1 if name is absent else 0. */
static int noWinDfltName(nameCtx h, unsigned short nameId) {
	return enumNames(h, 0,
	                 HOT_NAME_MS_PLATFORM,
	                 HOT_NAME_MS_UGL,
	                 HOT_NAME_MS_ENGLISH,
	                 nameId) == -1;
}

/* Check for Macintosh default name. Return 1 if name is absent else 0. */
static int noMacDfltName(nameCtx h, unsigned short nameId) {
	return enumNames(h, 0,
	                 HOT_NAME_MAC_PLATFORM,
	                 HOT_NAME_MAC_ROMAN,
	                 HOT_NAME_MAC_ENGLISH,
	                 nameId) == -1;
}

/* Get the first name string matching the supplied arguments. If no match is
   found return NULL. */
static char *getName(nameCtx        h,
                     unsigned short platformId,
                     unsigned short platspecId,
                     unsigned short languageId,
                     unsigned short nameId) {
	int i = enumNames(h, 0, platformId, platspecId, languageId, nameId);
	return (i == -1) ? NULL : &h->addstrs.array[h->tbl.record.array[i].offset];
}

/* Get a name and checks characters for for 7-bit cleanliness. Return NULL
   if no name or not 7-bit clean, else name string. */
static char *get7BitCleanName(nameCtx        h,
                              unsigned short platformId,
                              unsigned short platspecId,
                              unsigned short languageId,
                              unsigned short nameId) {
	char *str = getName(h, platformId, platspecId, languageId, nameId);
	if (str != NULL) {
		char *p;
		for (p = str; *p != '\0'; p++) {
			if (*p & 0x80) {
				return NULL;
			}
		}
	}
	return str;
}

/* Get Windows default 7-bit clean name. */
static char *getWinDfltName(nameCtx h, unsigned short nameId) {
	/* rroberts Nov 20, 2002. Stopped using "get7BitCleanName"
	   here, as non-seven-bit lean names can be used. This must be left
	   over from when the code was for converting the a library with no
	   new names. */
	return getName(h,
	               HOT_NAME_MS_PLATFORM,
	               HOT_NAME_MS_UGL,
	               HOT_NAME_MS_ENGLISH,
	               nameId);
}

/* Get Macintosh default 7-bit clean name. */
static char *getMacDfltName(nameCtx h, unsigned short nameId) {
	return getName(h,
	               HOT_NAME_MAC_PLATFORM,
	               HOT_NAME_MAC_ROMAN,
	               HOT_NAME_MAC_ENGLISH,
	               nameId);
}

/* ----------------------------- Name Addition ----------------------------- */

/* Add name to module. If no name with matching attributes already exists the
   new name is added. Otherwise, the new string replaces the old one. */
static void addName(nameCtx h,
                    unsigned short platformId,
                    unsigned short platspecId,
                    unsigned short languageId,
                    unsigned short nameId,
                    size_t length, char *str) {
	char *dst;
	NameRecord *rec;
	int index;
	int omitMacNames =  (h->g->convertFlags & HOT_OMIT_MAC_NAMES); /* omit all Mac platform names. */

    if (platformId == HOT_NAME_MAC_PLATFORM) {
        if (omitMacNames)
            return;
	}
    

    
	if (omitMacNames && (platformId == HOT_NAME_MAC_PLATFORM)) {
		return;
	}

	index = enumNames(h, 0, platformId, platspecId, languageId, nameId);

	if (index != -1) {
		/* Matching name found */
		rec = &h->tbl.record.array[index];
	}
	else {
		/* No matching name found; append name */
		rec = dnaNEXT(h->tbl.record);
		rec->platformId = platformId;
		rec->platspecId = platspecId;
		rec->languageId = languageId;
		rec->nameId = nameId;
	}

	/* Save name's string */
	rec->offset = (unsigned short)h->addstrs.cnt;
	dst = dnaEXTEND(h->addstrs, (long)length + 1);
	strncpy(dst, str, length);
	dst[length] = '\0';
}

/* Add Windows default name. */
static void addWinDfltName(nameCtx h,
                           unsigned short nameId, int length, char *str) {
	addName(h,
	        HOT_NAME_MS_PLATFORM,
	        HOT_NAME_MS_UGL,
	        HOT_NAME_MS_ENGLISH,
	        nameId, length, str);
}

/* Add Macintosh default name. */
static void addMacDfltName(nameCtx h,
                           unsigned short nameId, int length, char *str) {
	addName(h,
	        HOT_NAME_MAC_PLATFORM,
	        HOT_NAME_MAC_ROMAN,
	        HOT_NAME_MAC_ENGLISH,
	        nameId, length, str);
}

/* Add standard Windows and Macintosh default name. */
static void addStdNamePair(nameCtx h, int win, int mac,
                           unsigned short nameId, int length, char *str) {
	if (win) {
		addWinDfltName(h, nameId, length, str);
	}
	if (mac) {
		addMacDfltName(h, nameId, length, str);
	}
}

/* --------------------------- Standard Functions -------------------------- */

void nameNew(hotCtx g) {
	nameCtx h = MEM_NEW(g, sizeof(struct nameCtx_));

	h->userNameId = 256;
	dnaINIT(g->dnaCtx, h->tmp, 250, 250);
	dnaINIT(g->dnaCtx, h->addstrs, 1000, 1000);
	dnaINIT(g->dnaCtx, h->tbl.record, 50, 20);
	dnaINIT(g->dnaCtx, h->tbl.strings, 1000, 1000);

	/* Link contexts */
	h->g = g;
	g->ctx.name = h;
}

/* Add standard Windows and Macintosh default names. */
static void addStdNames(nameCtx h, int win, int mac) {
	hotCtx g = h->g;
	char buf[256];
    int index = 0;

	{
		/* Set font version string. Round to 3 decimal places. Truncated to 3 decimal places by sprintf. */
		double dFontVersion = 0.0005 + FIX2DBL(g->font.version.otf);

		char *id2Tag = "Core"; /* core library name for triggering the CoolType code
		                          that interprets chaining contextual subs rules per the
		                          incorrect way that was built into the first OpenType/CFF fonts
		                          and InDesign 2.0.
		                          Note that the full match string is, as of 1/24/2002 and CoolType v4.5:
		                          "(Version|OTF) 1.+;Core 1\.0\..+;makeotflib1\."
		                          This must match inside the (1,0,0) nameID 5 "Version: string. */
		char *id3Tag = "hotconv"; /* core library name guaranteeing that we DON'T trigger the CoolType code
		                             that interprets chaining contextual subs rules per the
		                             incorrect way that was built into the first OpenType/CFF fonts
		                             and InDesign 2.0 */

		char *idTag = id3Tag;

		dFontVersion = ((int)(1000 * dFontVersion)) / 1000.0;

		if (g->convertFlags & HOT_ID2_CHAIN_CONTXT3) {
			idTag = id2Tag; /* use tag to trigger soecial case behavious by CoolType */
		}
		/* Add Unique name */
		/* xxx is this really needed or just a waste of space? */
		if (g->font.licenseID != NULL) {
			sprintf(buf, "%.3f;%s;%s;%s",
			        dFontVersion,
			        g->font.vendId,
			        g->font.FontName.array,
			        g->font.licenseID);
		}
		else {
			sprintf(buf, "%.3f;%s;%s",
			        dFontVersion,
			        g->font.vendId,
			        g->font.FontName.array);
		}

        /* Don't add if one has already been provided from the feature file */
		index = enumNames(h, index,
		                  MATCH_ANY,
		                  MATCH_ANY,
		                  MATCH_ANY,
		                  HOT_NAME_UNIQUE);
		if (index == -1) {
            addStdNamePair(h, win, mac, HOT_NAME_UNIQUE, strlen(buf), buf);
		}

		/* Make and add version name */
		if (g->font.version.client == NULL) {
			sprintf(buf, "Version %.3f;PS %s;%s %ld.%ld.%ld",
			        dFontVersion,
			        g->font.version.PS,
			        idTag,
			        (g->version >> 16) & 0xff, (g->version >> 8) & 0xff, g->version & 0xff);
		}
		else {
			sprintf(buf, "Version %.3f;PS %s;%s %ld.%ld.%ld;%s",
			        dFontVersion,
			        g->font.version.PS,
			        idTag,
			        (g->version >> 16) & 0xff, (g->version >> 8) & 0xff, g->version & 0xff,
			        g->font.version.client);
		}
	}

    /* Don't add if one has already been provided from the feature file */
    index = enumNames(h, index,
                      MATCH_ANY,
                      MATCH_ANY,
                      MATCH_ANY,
                      HOT_NAME_VERSION);
    if (index == -1) {
        addStdNamePair(h, win, mac, HOT_NAME_VERSION, strlen(buf), buf);
    }

	/* Add PostScript name */
	addStdNamePair(h, win, mac, HOT_NAME_FONTNAME,
	               strlen(g->font.FontName.array), g->font.FontName.array);
}

/* Delete a target name from list if the same as reference name. */
static void deleteName(nameCtx        h,
                       unsigned short platformId,
                       unsigned short targetId,
                       unsigned short referenceId) {
	int index = 0;
	for (;; ) {
		NameRecord *rec;
		char *str;
		index = enumNames(h, index,
		                  platformId,
		                  MATCH_ANY,
		                  MATCH_ANY,
		                  targetId);
		if (index == -1) {
			break;
		}
		rec = &h->tbl.record.array[index];
		str = getName(h,
		              platformId,
		              rec->platspecId,
		              rec->languageId,
		              referenceId);
		if (str != NULL && strcmp(&h->addstrs.array[rec->offset], str) == 0) {
			/* Match found; mark for removal */
			rec->platformId = MATCH_ANY;
		}
		index++;
	}
}

/* Translate utf8 string to target mac script/langid. */
/*Depends on hard-coded list of supported Mac encodings. See codepage.h,
   macEnc definitions in map.c */
typedef UV_BMP MacEnc[256];
static MacEnc macRomanEnc = {
#include "macromn0.h"   /* 0. Mac Roman */
};

static char mapUV2MacRoman(UV uv) {
	char macChar = 0;
	int i;

	for (i = 0; i < 256; i++) {
		if (macRomanEnc[i] == uv) {
			macChar = i;
		}
	}

	return macChar;
}

/* translate string to mac platform default = macRoman script and language */
/* Does an in-place translation of a UTF-8 string.
   we can do this because the Mac string will be shorter than or equal to the UTF8 string. */
/* NOTE! This uses the h->tmp string to store the translated string, so don't call it again
   until you have copied the temp string contents */
static char *translate2MacDflt(nameCtx h, char *src) {
	char *end, *dst, *begin;
	short int uv = 0;
	char *uvp = (char *)&uv;

	long length = strlen(src);
	end = src + length;
	dst = dnaGROW(h->tmp, length); /* This translation can only make the string shorter */
	begin = dst;

	while (src < end) {
		unsigned s0 = *src++ & 0xff;
		uv = 0;
		if (s0 < 0xc0) {
			/* 1-byte */
			uv = s0;
		}
		else {
			unsigned s1 = *src++ & 0xff;
			if (s0 < 0xe0) {
				/* 2-byte */
#ifdef __LITTLE_ENDIAN__
				uvp[1] = s0 >> 2 & 0x07;
				uvp[0] = s0 << 6 | (s1 & 0x3f);
#else
				uvp[0] = s0 >> 2 & 0x07;
				uvp[1] = s0 << 6 | (s1 & 0x3f);
#endif
				}
			else {
				/* 3-byte */
				unsigned s2 = *src++;
#ifdef __LITTLE_ENDIAN__
				uvp[1] = s0 << 4 | (s1 >> 2 & 0x0f);
				uvp[0] = s1 << 6 | (s2 & 0x3f);
#else
				uvp[0] = s0 << 4 | (s1 >> 2 & 0x0f);
				uvp[1] = s1 << 6 | (s2 & 0x3f);
#endif
			}
		}
		if (uv) {
			char macChar = mapUV2MacRoman(uv);
			if (macChar == 0) {
				hotMsg(h->g, hotFATAL, "Could not translate UTF8 glyph code into Mac Roman in name table name %s", begin);
			}
			*dst++ = macChar;
		}
	}
	*dst = 0;
	return begin;
}

/* Add menu names and standard names. */
static void fillNames(nameCtx h) {
	char *Family;
	char *Subfamily;
	int win;
	int mac;
	int index;
	char *tempString;
	NameRecord *rec;
	static char *styles[4] = {
		"Regular", "Bold", "Italic", "Bold Italic"
	};
	char *style = styles[h->g->font.flags & 0x3];
	char Full[HOT_MAX_MENU_NAME * 2];
	int doWarning = 0;
	int doV1Names =  (h->g->convertFlags & HOT_USE_V1_MENU_NAMES); /* if the Font Menu DB file is using hte original V1 syntax, we write the mac Preferred Family
	                                                                  and Preferred Subfamily names to id's 1 and 2 rather than 16 and 17 */
	int doOTSpecName4 =  !(h->g->convertFlags & HOT_USE_OLD_NAMEID4); /* write Windows name ID 4 correctly in Win platform. */
	int omitMacNames =  (h->g->convertFlags & HOT_OMIT_MAC_NAMES); /* omit all Mac platform names. */

	tempString = getName(h, HOT_NAME_MS_PLATFORM,
	                     HOT_NAME_MS_UGL,
	                     MATCH_ANY,
	                     HOT_NAME_PREF_FAMILY);
	if (tempString == NULL) {
		/* Use thefamily Font name; this is generated from the PS family name if there is no Font Mneu Name DB. */
		tempString = getName(h, HOT_NAME_MS_PLATFORM,
		                     HOT_NAME_MS_UGL,
		                     MATCH_ANY,
		                     HOT_NAME_FAMILY);

		if (tempString == NULL) {
			hotMsg(h->g, hotFATAL, "I can't find a Family name for this font !");
		}
		addWinDfltName(h, HOT_NAME_FAMILY, strlen(tempString), tempString);
	}


	/**************************** First fill out all the Windows menu names */

	/* All and only the font menu name specified in the FontMenuNameDB entry are present at this point.
	   Only one  Windows Preferred Family is guaranteed to be defined (name ID 16); the rest may have to be
	   derived from these two values.	This call is about deriving the missing names */

	if (NULL != getName(h, MATCH_ANY,
	                    MATCH_ANY,
	                    MATCH_ANY,
	                    HOT_NAME_FAMILY)) {
		doWarning = 1; /* If the user entered any explicit HOT_NAME_FAMILY names, we should warn of any HOT_NAME_PREF_FAMILY that are not matched by
	                      a HOT_NAME_FAMILY name entry */
	}
	/* Copy HOT_NAME_PREF_FAMILY entries to HOT_NAME_FAMILY.
	   We need to add a compatible family name for each Preferred Family name which does not already have a HOT_NAME_FAMILY name.
	   That way, the name ID 16's will get eiliminated, and we will be left with just name ID 1's, rather than just name ID's 16.
	 */

	index = 0;
	for (;; ) {
		/* Index through all recs for HOT_NAME_PREF_FAMILY */
		index = enumNames(h, index,
		                  HOT_NAME_MS_PLATFORM,
		                  HOT_NAME_MS_UGL,
		                  MATCH_ANY,
		                  HOT_NAME_PREF_FAMILY);
		if (index == -1) {
			break; /* no more MS UGL HOT_NAME_PREF_FAMILY recs */
		}
		/* If there is not already a compatible Family Name, add the HOT_NAME_PREF_FAMILY value  */
		rec = &h->tbl.record.array[index];
		Family = &h->addstrs.array[rec->offset];
		if (NULL == getName(h, HOT_NAME_MS_PLATFORM,
		                    rec->platspecId,
		                    rec->languageId,
		                    HOT_NAME_FAMILY)) {
			addName(h,
			        HOT_NAME_MS_PLATFORM,
			        rec->platspecId,
			        rec->languageId,
			        HOT_NAME_FAMILY, strlen(Family), Family);
			if (doWarning) {
				hotMsg(h->g, hotWARNING, "The Font Menu Name DB entry for this font is missing an MS Platform Compatible Family Name entry to match the MS Platform Preferred Family Name for language ID %d. Using the Preferred Name only.",  rec->languageId);
			}
		}

		index++;
	}

	/* Any MS UGL HOT_NAME_PREF_SUBFAMILY names must be explicitly defined; else we will add  only
	   the HOT_NAME_SUBFAMILY values. */

	/* Add HOT_NAME_SUBFAMILY  and HOT_NAME_FULL for all MS Platform HOT_NAME_FAMILY names, using the derived style value.
	   There is no way to add these from the Font Menu Name DB, so we don't have to check
	   if there is already a value matching any particular HOT_NAME_FAMILY */
	index = 0;
	for (;; ) {
		index = enumNames(h, index,
		                  HOT_NAME_MS_PLATFORM,
		                  HOT_NAME_MS_UGL,
		                  MATCH_ANY,
		                  HOT_NAME_FAMILY);
		if (index == -1) {
			break;
		}
		rec = &h->tbl.record.array[index];
		addName(h,
		        HOT_NAME_MS_PLATFORM,
		        HOT_NAME_MS_UGL,
		        rec->languageId,
		        HOT_NAME_SUBFAMILY, strlen(style), style);

		index++;
	}


	/* Now add Windows name ID 4, Full Name. This to come last as it it requires the SubFamilyName to exist */
	if (!doOTSpecName4) {
		/* add Windows name ID 4 with the PS name as the value. */
		addWinDfltName(h, HOT_NAME_FULL, strlen(h->g->font.FontName.array),
		               h->g->font.FontName.array);
	}
	else {
		/* Full name = compaitble Family name + compatible Subfamily name.
		   If SubFamily name is Regular, we omit it.
		 */
		index = 0;
        
        /* Skip this if we already have one defined by the feature file > Not usually
         allowed, but the prohibition can be overriden */
        index = enumNames(h, index,
                          HOT_NAME_MS_PLATFORM,
                          HOT_NAME_MS_UGL,
                          MATCH_ANY,
                          HOT_NAME_FULL);
        if (index == -1)
        { /* no predefined MS UGL HOT_NAME_FAMILY recs */
            index = 0;
            for (;; ) {
                
                
                /* We make an entry for each HOT_NAME_PREF_FAMILY */
                index = enumNames(h, index,
                                  HOT_NAME_MS_PLATFORM,
                                  HOT_NAME_MS_UGL,
                                  MATCH_ANY,
                                  HOT_NAME_FAMILY);
                if (index == -1) {
                    break; /* no more MS UGL HOT_NAME_FAMILY recs */
                }
                /* If there is not already a compatible Family Name, add the HOT_NAME_FAMILY value  */
                rec = &h->tbl.record.array[index];
                Family = &h->addstrs.array[rec->offset];
                
                /* See if we can get a  subfamily name with the same script/language settings */
                Subfamily = getName(h,
                                    HOT_NAME_MS_PLATFORM,
                                    rec->platspecId,
                                    rec->languageId,
                                    HOT_NAME_SUBFAMILY);
                
                if (Subfamily == NULL) {
                    Subfamily = getWinDfltName(h, HOT_NAME_SUBFAMILY);
                }
                else if (strcmp(Subfamily, "Regular") == 0) {
                    addName(h,
                            HOT_NAME_MS_PLATFORM,
                            rec->platspecId,
                            rec->languageId,
                            HOT_NAME_FULL, strlen(Family), Family);
                }
                else {
                    sprintf(Full, "%s %s", Family, Subfamily);
                    addName(h,
                            HOT_NAME_MS_PLATFORM,
                            rec->platspecId,
                            rec->languageId,
                            HOT_NAME_FULL, strlen(Full), Full);
                }
                index++;
            }
        }
	}


	/**************************** Now fill out all the Mac menu names. This is slightly differnt due to V1 vs V2 syntax diffs. */
	if (!omitMacNames) {
		/* If there is no Mac Preferred Family, we get the default Win Preferred Family name.
		   This is the only one we copy to the Mac platform, as it is the only one we can
		   translate easily form Win UGL to Mac encoding.
		 */
		Family = NULL;
		Subfamily = NULL;
		if (noMacDfltName(h, HOT_NAME_PREF_FAMILY)) {
			tempString = getWinDfltName(h, HOT_NAME_PREF_FAMILY);
			if (tempString != NULL) {
				Family = translate2MacDflt(h, tempString);
				addMacDfltName(h, HOT_NAME_PREF_FAMILY, strlen(Family), Family);

				/* In version 1 sytnax, there is never a Mac Compatible Family name entry, so just copy
				   the HOT_NAME_PREF_FAMILY to HOT_NAME_FAMILY */
				if (doV1Names) {
					addMacDfltName(h, HOT_NAME_FAMILY, strlen(Family), Family);
				}
			}
		}

		/* If we are using V2 syntax, and there is no Mac HOT_NAME_FAMILY specified, we get it from the
		   Windows default HOT_NAME_FAMILY name. We do NOT do this if we are using v1 syntax. */

		if (noMacDfltName(h, HOT_NAME_FAMILY)) {
			if (!doV1Names) {
				char *tempString;
				tempString = getWinDfltName(h, HOT_NAME_FAMILY);
				if (tempString != NULL) {
					Family = translate2MacDflt(h, tempString);
					addMacDfltName(h, HOT_NAME_FAMILY, strlen(Family), Family);
				}
			}
		}

		/* Copy HOT_NAME_PREF_FAMILY entries to HOT_NAME_FAMILY. Same logic as for Windows names.
		   For V2 syntax, there may be HOT_NAME_FAMILY entries, so we need to make sure we don't add conflicting entries.
		 */

		index = 0;
		for (;; ) {
			/* Index through all recs for HOT_NAME_PREF_FAMILY */
			index = enumNames(h, index,
			                  HOT_NAME_MAC_PLATFORM,
			                  MATCH_ANY,
			                  MATCH_ANY,
			                  HOT_NAME_PREF_FAMILY);
			if (index == -1) {
				break; /* no more Mac UGL HOT_NAME_PREF_FAMILY recs */
			}
			/* If there is not already a compatible Family Name, add the HOT_NAME_PREF_FAMILY value  */
			rec = &h->tbl.record.array[index];
			if (NULL == getName(h, HOT_NAME_MAC_PLATFORM,
			                    rec->platspecId,
			                    rec->languageId,
			                    HOT_NAME_FAMILY)) {
				Family = &h->addstrs.array[rec->offset];
				addName(h,
				        HOT_NAME_MAC_PLATFORM,
				        rec->platspecId,
				        rec->languageId,
				        HOT_NAME_FAMILY, strlen(Family), Family);
				if (doWarning && (!doV1Names)) {
					hotMsg(h->g, hotWARNING, "The Font Menu Name DB entry for this font is missing a Mac Platform Compatible Family Name entry to match the Mac Platform Preferred Family Name for language ID %d. Using the Preferred Name only.",  rec->languageId);
				}
			}
			index++;
		}
		doWarning = 0;


		/* Now for the Mac sub-family modifiers */

		/*  If there are no HOT_NAME_PREF_SUBFAMILY names at all,
		   and there is a default MS  HOT_NAME_PREF_SUBFAMILY, we will use that.
		   Otherwise, Mac HOT_NAME_PREF_SUBFAMILY names must be explicitly defined.
		 */
		if (noMacDfltName(h, HOT_NAME_PREF_SUBFAMILY)) {
			char *tempString;

			tempString = getWinDfltName(h, HOT_NAME_PREF_SUBFAMILY);
			if (tempString != NULL) {
				Subfamily = translate2MacDflt(h, tempString);
				addMacDfltName(h, HOT_NAME_PREF_SUBFAMILY, strlen(Subfamily), Subfamily);
				if (doV1Names) {
					addMacDfltName(h, HOT_NAME_SUBFAMILY, strlen(Subfamily), Subfamily);
				}
			}
		}

		/* For V1 syntax, any HOT_NAME_PREF_SUBFAMILY must be copied to the HOT_NAME_SUBFAMILY entries,
		   so that the HOT_NAME_PREF_SUBFAMILY entries will later be eliminated.

		   We didn't do this for Windows, as for Windows, name ID 2 is always the compatible subfamily names. However, in V1 syntax,
		   Mac name ID 2 contains the Preferred Sub Family names. */
		if (doV1Names) {
			index = 0;
			for (;; ) {
				/* Index through all recs for HOT_NAME_PREF_SUBFAMILY */
				index = enumNames(h, index,
				                  HOT_NAME_MAC_PLATFORM,
				                  MATCH_ANY,
				                  MATCH_ANY,
				                  HOT_NAME_PREF_SUBFAMILY);
				if (index == -1) {
					break; /* no more Mac UGL HOT_NAME_PREF_FAMILY recs */
				}
				/* If there is not already a compatible Family Name, add the HOT_NAME_PREF_FAMILY value  */
				rec = &h->tbl.record.array[index];
				if (NULL == getName(h, HOT_NAME_MAC_PLATFORM,
				                    rec->platspecId,
				                    rec->languageId,
				                    HOT_NAME_SUBFAMILY)) {
					Subfamily = &h->addstrs.array[rec->offset];
					addName(h,
					        HOT_NAME_MAC_PLATFORM,
					        rec->platspecId,
					        rec->languageId,
					        HOT_NAME_SUBFAMILY, strlen(Subfamily), Subfamily);
				}
				index++;
			}
		}


		/* Now for all Mac HOT_NAME_FAMILY names, fill in the HOT_NAME_SUBFAMILY names. These may not yet
		   be filled in even for V1 syntax, despie the loop above. If the user specified Mac HOT_NAME_PREF_FAMILY names
		   but not HOT_NAME_PREF_SUBFAMILY names, there will still be no HOT_NAME_SUBFAMILY entries
		 */
		index = 0;
		for (;; ) {
			index = enumNames(h, index,
			                  HOT_NAME_MAC_PLATFORM,
			                  MATCH_ANY,
			                  MATCH_ANY,
			                  HOT_NAME_FAMILY);
			if (index == -1) {
				break;
			}
			rec = &h->tbl.record.array[index];
			if (NULL == getName(h, HOT_NAME_MAC_PLATFORM,
			                    rec->platspecId,
			                    rec->languageId,
			                    HOT_NAME_SUBFAMILY)) {
				addName(h,
				        HOT_NAME_MAC_PLATFORM,
				        rec->platspecId,
				        rec->languageId,
				        HOT_NAME_SUBFAMILY, strlen(style), style);
			}
			index++;
		}


		/* Done with Mac Preferred and Win Compatible Family and SubFamily names */

		/* Add mac.Full (name ID 4) for all mac.Family names. compatible Family name + compatible Subfamily name. */

		index = 0;
		for (;; ) {
			char *Family;
			char *Subfamily;
            int localIndex;
            
            /* Don't add Mac FULl names if they are already predefined from the feature file. */
            localIndex = enumNames(h, index,
			                  HOT_NAME_MAC_PLATFORM,
                              MATCH_ANY,
                              MATCH_ANY,
			                  HOT_NAME_FULL);
			if (localIndex != -1) {
				break;            }

			if (doOTSpecName4) {
				index = enumNames(h, index,
				                  HOT_NAME_MAC_PLATFORM,
				                  MATCH_ANY,
				                  MATCH_ANY,
				                  HOT_NAME_FAMILY);
				if (index == -1) {
					break;
				}
				rec = &h->tbl.record.array[index];
				Family = &h->addstrs.array[rec->offset];

				Subfamily = getName(h,
				                    HOT_NAME_MAC_PLATFORM,
				                    rec->platspecId,
				                    rec->languageId,
				                    HOT_NAME_SUBFAMILY);

				if (Subfamily == NULL) {
					char *tempString;
					tempString = getWinDfltName(h, HOT_NAME_SUBFAMILY);
					if (tempString != NULL) {
						Subfamily = translate2MacDflt(h, tempString);
					}
				}

				if (Subfamily == NULL) {
					hotMsg(h->g, hotFATAL, "no Mac subfamily name specified");
				}
				else if (strcmp(Subfamily, "Regular") == 0) {
					addName(h,
					        HOT_NAME_MAC_PLATFORM,
					        rec->platspecId,
					        rec->languageId,
					        HOT_NAME_FULL, strlen(Family), Family);
				}
				else {
					char Full[HOT_MAX_MENU_NAME * 2];
					sprintf(Full, "%s %s", Family, Subfamily);
					addName(h,
					        HOT_NAME_MAC_PLATFORM,
					        rec->platspecId,
					        rec->languageId,
					        HOT_NAME_FULL, strlen(Full), Full);
				}
				index++;
			}
			else {
				index = enumNames(h, index,
				                  HOT_NAME_MAC_PLATFORM,
				                  MATCH_ANY,
				                  MATCH_ANY,
				                  HOT_NAME_PREF_FAMILY);
				if (index == -1) {
					break;
				}
				rec = &h->tbl.record.array[index];
				Family = &h->addstrs.array[rec->offset];
				Subfamily = getName(h,
				                    HOT_NAME_MAC_PLATFORM,
				                    rec->platspecId,
				                    rec->languageId,
				                    HOT_NAME_PREF_SUBFAMILY);
				if (Subfamily == NULL) {
					Subfamily = getName(h,
					                    HOT_NAME_MAC_PLATFORM,
					                    rec->platspecId,
					                    rec->languageId,
					                    HOT_NAME_SUBFAMILY);
				}
				if (Subfamily == NULL) {
					char *tempString;
					tempString = getWinDfltName(h, HOT_NAME_PREF_SUBFAMILY);
					if (tempString != NULL) {
						Subfamily = translate2MacDflt(h, tempString);
					}
				}
				if (Subfamily == NULL) {
					char *tempString;
					tempString = getWinDfltName(h, HOT_NAME_SUBFAMILY);
					if (tempString != NULL) {
						Subfamily = translate2MacDflt(h, tempString);
					}
				}

				if (Subfamily == NULL) {
					hotMsg(h->g, hotFATAL, "no Mac subfamily name specified");
				}
				else if (strcmp(Subfamily, "Regular") == 0) {
					addName(h,
					        HOT_NAME_MAC_PLATFORM,
					        rec->platspecId,
					        rec->languageId,
					        HOT_NAME_FULL, strlen(Family), Family);
				}
				else {
					char Full[HOT_MAX_MENU_NAME * 2];
					sprintf(Full, "%s %s", Family, Subfamily);
					addName(h,
					        HOT_NAME_MAC_PLATFORM,
					        rec->platspecId,
					        rec->languageId,
					        HOT_NAME_FULL, strlen(Full), Full);
				}
				index++;
			}
		}
	}

	/* Remove unnecessary ( i.e. duplicate)  names */
	/* For example, if the user specified a Mac Compatible Full Name ( name id 18) in the FontMenuNameDB,
	   and it is the same as the derived Preferred Full Name (id 4), then we can omit the Mac Compatible Full Name ( name id 18). */

	deleteName(h,
	           HOT_NAME_MS_PLATFORM,
	           HOT_NAME_PREF_FAMILY,
	           HOT_NAME_FAMILY);
	deleteName(h,
	           HOT_NAME_MS_PLATFORM,
	           HOT_NAME_PREF_SUBFAMILY,
	           HOT_NAME_SUBFAMILY);

	if (!omitMacNames) {
		deleteName(h,
		           HOT_NAME_MAC_PLATFORM,
		           HOT_NAME_PREF_FAMILY,
		           HOT_NAME_FAMILY);
		deleteName(h,
		           HOT_NAME_MAC_PLATFORM,
		           HOT_NAME_PREF_SUBFAMILY,
		           HOT_NAME_SUBFAMILY);
		deleteName(h,
		           HOT_NAME_MAC_PLATFORM,
		           HOT_NAME_COMP_FULL,
		           HOT_NAME_FULL);
	}
	/* Determine if platform names are present */
	win = !noName(h, HOT_NAME_MS_PLATFORM, MATCH_ANY, MATCH_ANY, MATCH_ANY);
	mac = !noName(h, HOT_NAME_MAC_PLATFORM, MATCH_ANY, MATCH_ANY, MATCH_ANY);

	addStdNames(h, win, mac);
}

/* Compare name records */
static int CDECL cmpNames(const void *first, const void *second) {
	const NameRecord *a = first;
	const NameRecord *b = second;
	if (a->platformId < b->platformId) {
		return -1;
	}
	else if (a->platformId > b->platformId) {
		return 1;
	}
	else if (a->platspecId < b->platspecId) {
		return -1;
	}
	else if (a->platspecId > b->platspecId) {
		return 1;
	}
	else if (a->languageId < b->languageId) {
		return -1;
	}
	else if (a->languageId > b->languageId) {
		return 1;
	}
	else if (a->nameId < b->nameId) {
		return -1;
	}
	else if (a->nameId > b->nameId) {
		return 1;
	}
	else {
		return 0;
	}
}

/* Translate UTF-8 strings to target platform. */
static char *translate(nameCtx h, unsigned short platformId,
                       int *length, char *src) {
	*length = strlen(src);
	switch (platformId) {
		case HOT_NAME_MS_PLATFORM: { /* Convert UTF-8 to 16-bit */
			char *end = src + *length;
			char *dst = dnaGROW(h->tmp, *length * 2);

			while (src < end) {
				unsigned s0 = *src++ & 0xff;
				if (s0 < 0xc0) {
					/* 1-byte */
					*dst++ = 0;
					*dst++ = s0;
				}
				else {
					unsigned s1 = *src++ & 0xff;
					if (s0 < 0xe0) {
						/* 2-byte */
						*dst++ = s0 >> 2 & 0x07;
						*dst++ = s0 << 6 | (s1 & 0x3f);
					}
					else {
						/* 3-byte */
						unsigned s2 = *src++;
						*dst++ = s0 << 4 | (s1 >> 2 & 0x0f);
						*dst++ = s1 << 6 | (s2 & 0x3f);
					}
				}
			}
			*length = dst - h->tmp.array;
			return h->tmp.array;
		}

		case HOT_NAME_MAC_PLATFORM:
			/* pass thru - Mac strings are expected to already be translated to the target
			   script/lang-id. */
			break;
	}
	return src;
}

/* Find matching string in the final string array. */
static long matchString(nameCtx h, int limit, size_t length, char *str) {
	int i;
	for (i = 0; i < limit; i++) {
		NameRecord *rec = &h->tbl.record.array[i];
		if (length == rec->length &&
		    memcmp(str, &h->tbl.strings.array[rec->offset], length) == 0) {
			/* Found match; return offset */
			return rec->offset;
		}
	}
	/* No match */
	return -1;
}

int nameFill(hotCtx g) {
	nameCtx h = g->ctx.name;
	int i;

	fillNames(h);

#if 0
	{
		/* Print compatible name verification */
		char *win = getWinDfltName(h, HOT_NAME_FAMILY);
		char *mac = getMacDfltName(h, HOT_NAME_COMP_FULL);
		if (mac == NULL) {
			mac = getMacDfltName(h, HOT_NAME_FULL);
		}
		printf("%s|%s|%s\n", g->font.FontName.array, win, mac);
	}
#endif

	/* Sort by ids; deleted records float to the top */
	qsort(h->tbl.record.array, h->tbl.record.cnt, sizeof(NameRecord), cmpNames);

	/* Translate and copy strings to final buffer, removing duplicates */
	for (i = 0; i < h->tbl.record.cnt; i++) {
		char *str;
		int length;
		long offset;
		NameRecord *rec = &h->tbl.record.array[i];

		if (rec->platformId == MATCH_ANY) {
			/* First deleted record; adjust record count */
			h->tbl.record.cnt = i;
			break;
		}

		/* Update record to point to new string */
		str = translate(h, rec->platformId, &length,
		                &h->addstrs.array[rec->offset]);
		offset = matchString(h, i, length, str);
		if (offset == -1) {
			/* No match; add new string */
			rec->offset = (unsigned short)h->tbl.strings.cnt;
			memcpy(dnaEXTEND(h->tbl.strings, length), str, length);
		}
		else {
			/* Found match; update offset */
			rec->offset = (unsigned short)offset;
		}
		rec->length = length;
	}

	/* Initialize header */
	h->tbl.format = 0;
	h->tbl.count = (unsigned short)h->tbl.record.cnt;
	h->tbl.stringOffset =
	    (unsigned short)(TBL_HDR_SIZE + NAME_REC_SIZE * h->tbl.record.cnt);

	return 1;
}

void nameWrite(hotCtx g) {
	int i;
	nameCtx h = g->ctx.name;

	/* Write header */
	OUT2(h->tbl.format);
	OUT2(h->tbl.count);
	OUT2(h->tbl.stringOffset);

	/* Write name records */
	for (i = 0; i < h->tbl.count; i++) {
		NameRecord *rec = &h->tbl.record.array[i];

		OUT2(rec->platformId);
		OUT2(rec->platspecId);
		OUT2(rec->languageId);
		OUT2(rec->nameId);
		OUT2(rec->length);
		OUT2(rec->offset);
	}

	/* Write the string pool */
	OUTN(h->tbl.strings.cnt, h->tbl.strings.array);
}

void nameReuse(hotCtx g) {
	nameCtx h = g->ctx.name;
	h->userNameId = 256;
	h->tmp.cnt = 0;
	h->addstrs.cnt = 0;
	h->tbl.record.cnt = 0;
	h->tbl.strings.cnt = 0;
}

void nameFree(hotCtx g) {
	nameCtx h = g->ctx.name;
	dnaFREE(h->tmp);
	dnaFREE(h->addstrs);
	dnaFREE(h->tbl.record);
	dnaFREE(h->tbl.strings);
	MEM_FREE(g, g->ctx.name);
}

/* ------------------------ Supplementary Functions ------------------------ */

/* Add registered name with specified attributes. */
void nameAddReg(hotCtx g,
                unsigned short platformId,
                unsigned short platspecId,
                unsigned short languageId,
                unsigned short nameId, char *str) {
	nameCtx h = g->ctx.name;
	addName(h, platformId, platspecId, languageId, nameId, strlen(str), str);
}

/* Add user-defined name as with default Windows and Macintosh attributes. */
unsigned short nameAddUser(hotCtx g, char *str) {
	nameCtx h = g->ctx.name;
	addWinDfltName(h, h->userNameId, strlen(str), str);
	addMacDfltName(h, h->userNameId, strlen(str), str);
	return h->userNameId++;
}

/* Add user-defined name with specified attributes. */
unsigned short nameReserveUserID(hotCtx g) {
	nameCtx h = g->ctx.name;
	unsigned short nameId =  h->userNameId++;
	return nameId;
}

/* Report  error if the names are missing for the nameid. */
int nameVerifyDefaultNames(hotCtx g, unsigned short nameId) {
	int returnVal = 0;

	nameCtx h = g->ctx.name;
	if (noWinDfltName(h, nameId)) {
		returnVal = MISSING_WIN_DEFAULT_NAME;
	}
	if (noMacDfltName(h, nameId)) {
		returnVal |= MISSING_MAC_DEFAULT_NAME;
	}

	return returnVal;
}
