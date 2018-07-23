/* @(#)CM_VerSion dict.c atm08 1.2 16245.eco sum= 52433 atm08.002 */
/* @(#)CM_VerSion dict.c atm07 1.2 16164.eco sum= 31460 atm07.012 */
/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

#include "dict.h"
#include "cs.h"

#include <stdio.h>
#include <string.h>

/* Save integer in dict */
void dictSaveInt(DICT *dict, long i) {
	char t[5];
	int length = csEncInteger(i, t);
	memcpy(dnaEXTEND(*dict, length), t, length);
}

/* Save real or integer number in dict */
void dictSaveNumber(DICT *dict, double d) {
	char buf[50];
	int value;          /* Current nibble value */
	int last = 0;       /* Last nibble value */
	int odd = 0;        /* Flags odd nibble */
	long i = (long)d;

	if (i == d) {
		/* Value was integer */
		dictSaveInt(dict, i);
		return;
	}

	/* Convert to string */
	sprintf(buf, "%g", d);

	*dnaNEXT(*dict) = cff_BCD;

	for (i = buf[0] == '0';; i++) {
		switch (buf[i]) {
			case '\0':
				/* Terminate number */
				*dnaNEXT(*dict) = odd ? last << 4 | 0xf : 0xff;
				return;

			case '+':
				continue;

			case '-':
				value = 0xe;
				break;

			case '.':
				value = 0xa;
				break;

			case 'E':
			case 'e':
				value = (buf[++i] == '-') ? 0xc : 0xb;
				break;

			default:
				value = buf[i] - '0';
				break;
		}

		if (odd) {
			*dnaNEXT(*dict) = last << 4 | value;
		}
		else {
			last = value;
		}
		odd = !odd;
	}
}

/* Save real or integer Type 2 number in dict */
void dictSaveT2Number(DICT *dict, double d) {
	long i = (long)d;
	if (i == d) {
		/* Value was integer */
		dictSaveInt(dict, i);
	}
	else {
		/* Value was real; save as fixed point */
		char t[5];
		int length = csEncFixed(DBL2FIX(d), t);
		memcpy(dnaEXTEND(*dict, length), t, length);
	}
}

#if TC_DEBUG
void dictDump(tcCtx g, DICT *dict) {
	static char *opname[32] = {
		/*  0 */ "version",
		/*  1 */ "Notice",
		/*  2 */ "FullName",
		/*  3 */ "FamilyName",
		/*  4 */ "Weight",
		/*  5 */ "FontBBox",
		/*  6 */ "BlueValues",
		/*  7 */ "OtherBlues",
		/*  8 */ "FamilyBlues",
		/*  9 */ "FamilyOtherBlues",
		/* 10 */ "StdHW",
		/* 11 */ "StdVW",
		/* 12 */ "escape",
		/* 13 */ "UniqueID",
		/* 14 */ "XUID",
		/* 15 */ "charset",
		/* 16 */ "Encoding",
		/* 17 */ "CharStrings",
		/* 18 */ "Private",
		/* 19 */ "Subrs",
		/* 20 */ "defaultWidthX",
		/* 21 */ "nominalWidthX",
		/* 22 */ "reserved22",
		/* 23 */ "reserved23",
		/* 24 */ "reserved24",
		/* 25 */ "reserved25",
		/* 26 */ "reserved26",
		/* 27 */ "reserved27",
		/* 28 */ "shortint",
		/* 29 */ "longint",
		/* 30 */ "BCD",
		/* 31 */ "T2",
	};
	static char *escopname[] = {
		/*  0 */ "Copyright",
		/*  1 */ "isFixedPitch",
		/*  2 */ "ItalicAngle",
		/*  3 */ "UnderlinePosition",
		/*  4 */ "UnderlineThickness",
		/*  5 */ "PaintType",
		/*  6 */ "CharstringType",
		/*  7 */ "FontMatrix",
		/*  8 */ "StrokeWidth",
		/*  9 */ "BlueScale",
		/* 10 */ "BlueShift",
		/* 11 */ "BlueFuzz",
		/* 12 */ "StemSnapH",
		/* 13 */ "StemSnapV",
		/* 14 */ "ForceBold",
		/* 15 */ "ForceBoldThreshold",
		/* 16 */ "lenIV",
		/* 17 */ "LanguageGroup",
		/* 18 */ "ExpansionFactor",
		/* 19 */ "initialRandomSeed",
		/* 20 */ "SyntheticBase",
		/* 21 */ "PostScript",
		/* 22 */ "BaseFontName",
		/* 23 */ "BaseFontBlend",
		/* 24 */ "MultipleMaster",
		/* 25 */ "reserved25",
		/* 26 */ "BlendAxisTypes",
		/* 27 */ "reserved27",
		/* 28 */ "reserved28",
		/* 29 */ "reserved29",
		/* 30 */ "ROS",
		/* 31 */ "CIDFontVersion",
		/* 32 */ "CIDFontRevision",
		/* 33 */ "CIDFontType",
		/* 34 */ "CIDCount",
		/* 35 */ "UIDBase",
		/* 36 */ "FDArray",
		/* 37 */ "FDIndex",
		/* 38 */ "FontName",
		/* 39 */ "Chameleon",
	};
	int i;
	unsigned char *buf = (unsigned char *)dict->array;

	printf("--- dict\n");
	i = 0;
	while (i < dict->cnt) {
		int op = buf[i];
		switch (op) {
			case cff_version:
			case cff_Notice:
			case cff_FullName:
			case cff_FamilyName:
			case cff_Weight:
			case cff_FontBBox:
			case cff_BlueValues:
			case cff_OtherBlues:
			case cff_FamilyBlues:
			case cff_FamilyOtherBlues:
			case cff_StdHW:
			case cff_StdVW:
			case cff_UniqueID:
			case cff_XUID:
			case cff_charset:
			case cff_Encoding:
			case cff_CharStrings:
			case cff_Private:
			case cff_Subrs:
			case cff_defaultWidthX:
			case cff_nominalWidthX:
			case cff_reserved22:
			case cff_reserved23:
			case cff_reserved24:
			case cff_reserved25:
			case cff_reserved26:
			case cff_reserved27:
				printf("%s ", opname[op]);
				i++;
				break;

			case cff_escape: {
				/* Process escaped operator */
				unsigned int escop = buf[i + 1];
				if (escop > TABLE_LEN(escopname) - 1) {
					printf("? ");
				}
				else {
					printf("%s ", escopname[escop]);
				}
				i += 2;
				break;
			}

			case cff_shortint:
				/* 2 byte number */
				printf("%d ", buf[i + 1] << 8 | buf[i + 2]);
				i += 3;
				break;

			case cff_longint:
				/* 5 byte number */
				printf("%ld ", ((long)buf[i + 1] << 24 | (long)buf[i + 2] << 16 |
				                (long)buf[i + 3] << 8 | (long)buf[i + 4]));
				i += 5;
				break;

			case cff_BCD: {
				int count = 0;
				int byte = 0; /* Suppress optimizer warning */
				for (;; ) {
					int nibble;

					if (count++ & 1) {
						nibble = byte & 0xf;
					}
					else {
						byte = buf[++i];
						nibble = byte >> 4;
					}
					if (nibble == 0xf) {
						break;
					}

					printf("%c", "0123456789.EE?-?"[nibble]);
					if (nibble == 0xc) {
						printf("-");
					}
				}
				printf(" ");
				i++;
			}
			break;

			case cff_T2:
				printf("T2 ");
				i += csDump(-1, &buf[i + 1], g->nMasters, 0) + 1;
				break;

			case 247:
			case 248:
			case 249:
			case 250:
				/* +ve 2 byte number */
				printf("%d ", 108 + 256 * (buf[i] - 247) + buf[i + 1]);
				i += 2;
				break;

			case 251:
			case 252:
			case 253:
			case 254:
				/* -ve 2 byte number */
				printf("%d ", -108 - 256 * (buf[i] - 251) - buf[i + 1]);
				i += 2;
				break;

			case 255:
				printf("? ");
				i++;
				break;

			default:
				/* 1 byte number */
				printf("%d ", buf[i] - 139);
				i++;
				break;
		}
	}
	printf("\n");
}

#endif /* TC_DEBUG */