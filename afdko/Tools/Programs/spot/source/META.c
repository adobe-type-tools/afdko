/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)META.c	1.9
 * Changed:    5/19/99 17:20:55
 ***********************************************************************/

#include <string.h>

#include "META.h"
#include "sfnt_META.h"
#include <ctype.h>

static METATbl *META = NULL;
static IntX loaded = 0;

static const char *getMETAkeyword(Card16 index);
	

/* Read Pascal string */
static Card8 *readString(Card32 offset, Card16 length)
	{
	Card8 *newstr;
	Card32 save = TELL();
	
	SEEK_ABS(offset);
	
	newstr = memNew(length + 1);
	IN_BYTES(length, newstr);
	newstr[length] = '\0';
	
	SEEK_ABS(save);
	return newstr;
	}
	
static void readMetaStrings(METARecord *rec, CardX fourbyteoffsets, Card32 tablestart)
	{
	IntX i;
	Card32 save = TELL();
	
	SEEK_ABS(tablestart + rec->hdrOffset);
	for (i = 0; i < rec->nMetaEntry; i++)
		{
		Card16 off16;
		Card32 off32;
		METAString *s = da_INDEX(rec->stringentry, i);
		
		IN1(s->labelID);
		IN1(s->stringLen);
		if (fourbyteoffsets)
			{
			IN1(off32);	s->stringOffset = off32;
			}
		else
			{
			IN1(off16);	s->stringOffset = off16;
			}
		}
	SEEK_ABS(save);
	}

void METARead(LongN start, Card32 length)
	{
	IntX i;
	CardX fourbyteoffsets = 0;

	if (loaded)
		return;

	META = (METATbl *)memNew(sizeof(METATbl));

	SEEK_ABS(start);

	IN1(META->tableVersionMajor);
	IN1(META->tableVersionMinor);
	IN1(META->metaEntriesVersionMajor);
	IN1(META->metaEntriesVersionMinor);
	IN1(META->unicodeVersion);
	IN1(META->metaFlags);
	IN1(META->nMetaRecs);
	da_INIT(META->record, META->nMetaRecs, 1);
	fourbyteoffsets = (META->metaFlags & META_FLAGS_4BYTEOFFSETS);
	
	for (i = 0; i < META->nMetaRecs; i++)
		{
		Card16 off16;
		Card32 off32;
		METARecord *rec = da_INDEX(META->record, i);
		IN1(rec->glyphID);
		IN1(rec->nMetaEntry);
		if (fourbyteoffsets)
			{
			IN1(off32);	rec->hdrOffset = off32;
			}
		else
			{
			IN1(off16);	rec->hdrOffset = off16;
			}
		da_INIT(rec->stringentry, rec->nMetaEntry, 1);
		}
	
	for (i = 0; i < META->nMetaRecs; i++)
		{
		METARecord *rec = da_INDEX(META->record, i);
		readMetaStrings(rec, fourbyteoffsets, start);
		}
			
	loaded = 1;
	}


void METADump(IntX level, LongN start)
	{
	IntX i, j;

	DL(1, (OUTPUTBUFF, "### [META] (%08lx)\n", start));

	DLu(2, "tableVersionMajor      =", META->tableVersionMajor);
	DLu(2, "tableVersionMinor      =", META->tableVersionMinor);
	DLu(2, "metaEntriesVersionMajor=", META->metaEntriesVersionMajor);
	DLu(2, "metaEntriesVersionMinor=", META->metaEntriesVersionMinor);
	DL(2, (OUTPUTBUFF, 
		   "unicodeVersion         =%d.%d.%d (%d) (0x%08x)\n",
		META->unicodeVersion / 10000,
		(META->unicodeVersion / 100) % 100,
		(META->unicodeVersion % 100),
		META->unicodeVersion, META->unicodeVersion));	 
	DLx(2, "metaFlags              =", META->metaFlags);
	DLs(2, "nMetaRecs              =", META->nMetaRecs);
	for (i = 0; i < META->nMetaRecs; i++)
		{
		METARecord *rec = da_INDEX(META->record, i);
		DL(2, (OUTPUTBUFF, "--- Record (%d)\n", i));
		DLu(2, "glyphID    =", rec->glyphID);
		DLs(2, "nMetaEntry =", rec->nMetaEntry);
		DLU(2, "hdrOffset  =", rec->hdrOffset);
		for (j = 0; j < rec->nMetaEntry; j++)
			{
			METAString *s = da_INDEX(rec->stringentry, j);
			Card8 *news;
			Card8 *p, *end;
			const char *metatag;
			
			metatag = getMETAkeyword(s->labelID);
			if (!metatag) metatag = "Unknown";
			if (level >= 5)
				{
				DL(5, (OUTPUTBUFF, "String #%03d <%s>\tID=%3hu Len=%3hu Offset=%08x \t<",
					j, metatag, s->labelID, s->stringLen, s->stringOffset));
				}
			else
				{
				DL(2, (OUTPUTBUFF, "String #%03d <%s>\tID=%3hu Len=%3hu \t<",
					j, metatag, s->labelID, s->stringLen));
				}
			news = readString(start + s->stringOffset, s->stringLen);
			p = news;
			end = news + s->stringLen;
			while (p < end)
				{
				int code = *p++;
				if ((code > 0x1F) && (code < 0x7F))
					{
					DL(2,(OUTPUTBUFF, "%c", code & 0xFF));
					}
				else
					{
					DL(2, (OUTPUTBUFF, "\\%02x", code & 0xFF));
					}
				}
			DL(2, (OUTPUTBUFF, ">\n"));
			memFree(news);
			}
		}
	}


void METAFree(void)
	{
	IntX i;
    if (!loaded)
		return;
	for (i = 0; i < META->nMetaRecs; i++)
		{
		METARecord *rec = da_INDEX(META->record, i);
		da_FREE(rec->stringentry);
		}
	da_FREE(META->record);
	memFree(META); META=NULL;
	loaded = 0;
	}


typedef struct tMETAkeywordsbyindex_ 
	{
	Card16 index;
	const char * string;
	} tMETAkeywordsbyindex;


/* From SING DLL's public/api/METAtags.h */
enum METAtagsenum 
	{
	kMETA_MojikumiX4051 = 0,
	kMETA_UNIUnifiedBaseChars = 1,
	kMETA_BaseFontName = 2,
	kMETA_Language = 3,
	kMETA_CreationDate = 4,
	kMETA_FoundryName = 5,
	kMETA_FoundryCopyright = 6,
	kMETA_OwnerURI = 7,
	kMETA_WritingScript = 8,
	kMETA_StrokeCount = 10,
	kMETA_IndexingRadical = 11,
	kMETA_RemStrokeCount = 12,
	kMETA_ContentComponents = 13,
	kMETA_UNIIdeoDescString = 14,
	kMETA_DesignCategory = 15,
	kMETA_DesignWeight = 16,
	kMETA_DesignWidth = 17,
	kMETA_DesignAngle = 18,
	kMETA_CIDBaseChars = 20,
	kMETA_IsUpdatedGlyph = 21,
	kMETA_VendorGlyphURI = 22,
	kMETA_ReplacementFor = 23,
	kMETA_LocalizedInfo = 24,
	kMETA_LocalizedBaseFontName = 25,
	kMETA_UNICompositionEquivalent = 26,
	kMETA_ReadingString_GlyphName = 100,
	kMETA_ReadingString_JapaneseYomi = 101,
	kMETA_ReadingString_JapaneseTranslit = 102,
	kMETA_ReadingString_ChineseZhuyin = 103,
	kMETA_ReadingString_MandarinPinyinTranslit = 104,
	kMETA_ReadingString_CantonesePinyinTranslit = 105,
	kMETA_ReadingString_ChineseWadeGilesTranslit = 106,
	kMETA_ReadingString_ChineseYaleTranslit = 107,
	kMETA_ReadingString_ChineseGwoyeuTranslit = 108,
	kMETA_ReadingString_KoreanJamo = 109,
	kMETA_ReadingString_KoreanISOTranslit = 110,
	kMETA_ReadingString_VietnameseQuocNgu = 111,
	kMETA_ReadingString_VietnameseVIQRTranslit = 112,
	kMETA_ReadingString_IPA = 113,
	kMETA_ReadingString_EnglishTrans = 114,
	kMETA_AltIndexingRadical_KangXi = 200,
	kMETA_AltIndexingRadical_GB2312 = 201,
	kMETA_AltIndexingRadical_JIS0213 = 202,
	kMETA_AltLookup_HanYuDa = 250,
	kMETA_AltLookup_Zhonghua = 251,
	kMETA_AltLookup_KangXi = 252,
	kMETA_AltLookup_DaiKanwa = 253,
	kMETA_AltLookup_Chungwen = 254,
	kMETA_AltLookup_Mathews = 255,
	kMETA_AltLookup_Nelson = 256,
	kMETA_AltLookup_DaeJaweon = 257,

	kMETA_UNICODEproperty_GeneralCategory = 500,
	kMETA_UNICODEproperty_BidiCategory = 501,
	kMETA_UNICODEproperty_CombiningClass = 502,
	kMETA_UNICODEproperty_Math = 503,
	kMETA_UNICODEproperty_Mirrored = 504,
	kMETA_UNICODEproperty_JamoShortname = 505,
	kMETA_UNICODEproperty_Directionality = 506,
	kMETA_UNICODEproperty_Nonbreak = 507,
	kMETA_UNICODEproperty_Delimiter = 508,
	kMETA_UNICODEproperty_ZeroWidth = 509,
	kMETA_UNICODEproperty_Space = 510,
	kMETA_UNICODEproperty_WhiteSpace = 511,
	kMETA_UNICODEproperty_ISOControl = 512,
	kMETA_UNICODEproperty_IgnorableControl = 513,
	kMETA_UNICODEproperty_BidiControl = 514,
	kMETA_UNICODEproperty_JoinControl = 515,
	kMETA_UNICODEproperty_FormatControl = 516,
	kMETA_UNICODEproperty_Alphabetic = 517,
	kMETA_UNICODEproperty_Ideographic = 518,
	kMETA_UNICODEproperty_Composite = 519,
	kMETA_UNICODEproperty_Numeric = 520,
	kMETA_UNICODEproperty_DecimalDigit = 521,
	kMETA_UNICODEproperty_Extender = 522,
	kMETA_UNICODEproperty_HexDigit = 523,
	kMETA_UNICODEproperty_PairedPunct = 524,
	kMETA_UNICODEproperty_LeftofPair = 525,
	kMETA_UNICODEproperty_Dash = 525,
	kMETA_UNICODEproperty_Hyphen = 527,
	kMETA_UNICODEproperty_Punctuation = 528,
	kMETA_UNICODEproperty_QuotMark = 529,
	kMETA_UNICODEproperty_TermPunct = 530,
	kMETA_UNICODEproperty_CurrencySymbol = 531,
	kMETA_UNICODEproperty_Diacritic = 532,
	kMETA_UNICODEproperty_IdentifierPart = 533,
	kMETA_UNICODEproperty_Nonspacing = 534,
	kMETA_UNICODEproperty_Uppercase = 535,
	kMETA_UNICODEproperty_Lowercase = 536,
	kMETA_UNICODEproperty_Titlecase = 537,
	kMETA_UNICODEproperty_LineSep = 538,
	kMETA_UNICODEproperty_ParagraphSep = 539,

	kMETA_Layout_VerticalMode = 600,
	kMETA_Layout_FitFlush = 601,
	kMETA_UNIDerivedByOpenType = 602,
	kMETA_UNIOpenTypeYields = 603,
	kMETA_CIDDerivedByOpenType = 604,
	kMETA_CIDOpenTypeYields = 605,

	kMETA_VendorSpecific_first = 20000,
	kMETA_VendorSpecific_last = 32767,

	/* pseudo-labelIDs.  */
	kMETA_UniqueName = 65000,
	kMETA_GlyphletVersion = 65001,
	kMETA_MainGID = 65002,
	kMETA_EmbeddingInfo = 65003,
	kMETA_UnitsPerEm = 65004,
	kMETA_VertAdvance = 65005,
	kMETA_VertOrigin = 65006,
	kMETA_UnicodeVersion = 65007,
	kMETA_nMetaRecords = 65008,
	kMETA_RepositoryFilename = 65009,
	kMETA_RepositoryFilesize = 65010,

	/* For use within SING Services library only. Derived entries. */
	kMETA_BaseFontNamePrefix = 65509,
	kMETA_CIDBaseChars_RO_substring = 65510,
	kMETA_UNIDerivedByOpenType_FEAT_substring = 65511,
	kMETA_UNIOpenTypeYields_FEAT_substring = 65512,
	kMETA_CIDDerivedByOpenType_FEAT_substring = 65513,
	kMETA_CIDOpenTypeYields_FEAT_substring = 65514,
	kMETA_ReadingString_JapaneseYomiNormKatakana = 65515,

	kMETA_StaticFileMarker1 = 65530,
	kMETA_StaticFileMarker2 = 65531,
	kMETA_StaticFileMarker3 = 65532,

	kMETA_EnumCount
	};

const tMETAkeywordsbyindex METAKeywordsbyIndex [] =  
{
	{kMETA_MojikumiX4051, "MojikumiX4051" },
	{kMETA_UNIUnifiedBaseChars, "UNIUnifiedBaseChars" },
	{kMETA_BaseFontName, "BaseFontName" },
	{kMETA_Language, "Language" },
	{kMETA_CreationDate, "CreationDate" },
	{kMETA_FoundryName, "FoundryName" },
	{kMETA_FoundryCopyright, "FoundryCopyright" },
	{kMETA_OwnerURI, "OwnerURI" },
	{kMETA_WritingScript, "WritingScript" },
	{kMETA_StrokeCount, "StrokeCount" },
	{kMETA_IndexingRadical, "IndexingRadical" },
	{kMETA_RemStrokeCount, "RemStrokeCount" },
	{kMETA_ContentComponents, "ContentComponents" },
	{kMETA_UNIIdeoDescString, "UNIIdeoDescString" },
	{kMETA_DesignCategory, "DesignCategory" },
	{kMETA_DesignWeight, "DesignWeight" },
	{kMETA_DesignWidth, "DesignWidth" },
	{kMETA_DesignAngle, "DesignAngle" },
	{kMETA_CIDBaseChars, "CIDBaseChars" },
	{kMETA_IsUpdatedGlyph, "IsUpdatedGlyph" },
	{kMETA_VendorGlyphURI, "VendorGlyphURI" },
	{kMETA_ReplacementFor, "ReplacementFor" },
	{kMETA_LocalizedInfo, "LocalizedInfo" },
	{kMETA_LocalizedBaseFontName, "LocalizedBaseFontName" },
	{kMETA_UNICompositionEquivalent, "UNICompositionEquivalent" },
	{kMETA_ReadingString_GlyphName, "ReadingString_GlyphName" },
	{kMETA_ReadingString_JapaneseYomi, "ReadingString_JapaneseYomi" },
	{kMETA_ReadingString_JapaneseTranslit, "ReadingString_JapaneseTranslit" },
	{kMETA_ReadingString_ChineseZhuyin, "ReadingString_ChineseZhuyin" },
	{kMETA_ReadingString_MandarinPinyinTranslit, "ReadingString_MandarinPinyinTranslit" },
	{kMETA_ReadingString_CantonesePinyinTranslit, "ReadingString_CantonesePinyinTranslit" },
	{kMETA_ReadingString_ChineseWadeGilesTranslit, "ReadingString_ChineseWadeGilesTranslit" },
	{kMETA_ReadingString_ChineseYaleTranslit, "ReadingString_ChineseYaleTranslit" },
	{kMETA_ReadingString_ChineseGwoyeuTranslit, "ReadingString_ChineseGwoyeuTranslit" },
	{kMETA_ReadingString_KoreanJamo, "ReadingString_KoreanJamo" },
	{kMETA_ReadingString_KoreanISOTranslit, "ReadingString_KoreanISOTranslit" },
	{kMETA_ReadingString_VietnameseQuocNgu, "ReadingString_VietnameseQuocNgu" },
	{kMETA_ReadingString_VietnameseVIQRTranslit, "ReadingString_VietnameseVIQRTranslit" },
	{kMETA_ReadingString_IPA, "ReadingString_IPA" },
	{kMETA_ReadingString_EnglishTrans, "ReadingString_EnglishTrans" },
	{kMETA_AltIndexingRadical_KangXi, "AltIndexingRadical_KangXi" },
	{kMETA_AltIndexingRadical_GB2312, "AltIndexingRadical_GB2312" },
	{kMETA_AltIndexingRadical_JIS0213, "AltIndexingRadical_JIS0213" },
	{kMETA_AltLookup_HanYuDa, "AltLookup_HanYuDa" },
	{kMETA_AltLookup_Zhonghua, "AltLookup_Zhonghua" },
	{kMETA_AltLookup_KangXi, "AltLookup_KangXi" },
	{kMETA_AltLookup_DaiKanwa, "AltLookup_DaiKanwa" },
	{kMETA_AltLookup_Chungwen, "AltLookup_Chungwen" },
	{kMETA_AltLookup_Mathews, "AltLookup_Mathews" },
	{kMETA_AltLookup_Nelson, "AltLookup_Nelson" },
	{kMETA_AltLookup_DaeJaweon, "AltLookup_DaeJaweon" },

	{kMETA_UNICODEproperty_GeneralCategory, "UNICODEproperty_GeneralCategory" },
	{kMETA_UNICODEproperty_BidiCategory, "UNICODEproperty_BidiCategory" },
	{kMETA_UNICODEproperty_CombiningClass, "UNICODEproperty_CombiningClass" },
	{kMETA_UNICODEproperty_Math, "UNICODEproperty_Math" },
	{kMETA_UNICODEproperty_Mirrored, "UNICODEproperty_Mirrored" },
	{kMETA_UNICODEproperty_JamoShortname, "UNICODEproperty_JamoShortname" },
	{kMETA_UNICODEproperty_Directionality, "UNICODEproperty_Directionality" },
	{kMETA_UNICODEproperty_Nonbreak, "UNICODEproperty_Nonbreak" },
	{kMETA_UNICODEproperty_Delimiter, "UNICODEproperty_Delimiter" },
	{kMETA_UNICODEproperty_ZeroWidth, "UNICODEproperty_ZeroWidth" },
	{kMETA_UNICODEproperty_Space, "UNICODEproperty_Space" },
	{kMETA_UNICODEproperty_WhiteSpace, "UNICODEproperty_WhiteSpace" },
	{kMETA_UNICODEproperty_ISOControl, "UNICODEproperty_ISOControl" },
	{kMETA_UNICODEproperty_IgnorableControl, "UNICODEproperty_IgnorableControl" },
	{kMETA_UNICODEproperty_BidiControl, "UNICODEproperty_BidiControl" },
	{kMETA_UNICODEproperty_JoinControl, "UNICODEproperty_JoinControl" },
	{kMETA_UNICODEproperty_FormatControl, "UNICODEproperty_FormatControl" },
	{kMETA_UNICODEproperty_Alphabetic, "UNICODEproperty_Alphabetic" },
	{kMETA_UNICODEproperty_Ideographic, "UNICODEproperty_Ideographic" },
	{kMETA_UNICODEproperty_Composite, "UNICODEproperty_Composite" },
	{kMETA_UNICODEproperty_Numeric, "UNICODEproperty_Numeric" },
	{kMETA_UNICODEproperty_DecimalDigit, "UNICODEproperty_DecimalDigit" },
	{kMETA_UNICODEproperty_Extender, "UNICODEproperty_Extender" },
	{kMETA_UNICODEproperty_HexDigit, "UNICODEproperty_HexDigit" },
	{kMETA_UNICODEproperty_PairedPunct, "UNICODEproperty_PairedPunct" },
	{kMETA_UNICODEproperty_LeftofPair, "UNICODEproperty_LeftofPair" },
	{kMETA_UNICODEproperty_Dash, "UNICODEproperty_Dash" },
	{kMETA_UNICODEproperty_Hyphen, "UNICODEproperty_Hyphen" },
	{kMETA_UNICODEproperty_Punctuation, "UNICODEproperty_Punctuation" },
	{kMETA_UNICODEproperty_QuotMark, "UNICODEproperty_QuotMark" },
	{kMETA_UNICODEproperty_TermPunct, "UNICODEproperty_TermPunct" },
	{kMETA_UNICODEproperty_CurrencySymbol, "UNICODEproperty_CurrencySymbol" },
	{kMETA_UNICODEproperty_Diacritic, "UNICODEproperty_Diacritic" },
	{kMETA_UNICODEproperty_IdentifierPart, "UNICODEproperty_IdentifierPart" },
	{kMETA_UNICODEproperty_Nonspacing, "UNICODEproperty_Nonspacing" },
	{kMETA_UNICODEproperty_Uppercase, "UNICODEproperty_Uppercase" },
	{kMETA_UNICODEproperty_Lowercase, "UNICODEproperty_Lowercase" },
	{kMETA_UNICODEproperty_Titlecase, "UNICODEproperty_Titlecase" },
	{kMETA_UNICODEproperty_LineSep, "UNICODEproperty_LineSep" },
	{kMETA_UNICODEproperty_ParagraphSep, "UNICODEproperty_ParagraphSep" },

	{kMETA_Layout_VerticalMode, "Layout_VerticalMode" },
	{kMETA_Layout_FitFlush, "Layout_FitFlush" },
	{kMETA_UNIDerivedByOpenType, "UNIDerivedByOpenType" },
	{kMETA_UNIOpenTypeYields, "UNIOpenTypeYields" },
	{kMETA_CIDDerivedByOpenType, "CIDDerivedByOpenType" },
	{kMETA_CIDOpenTypeYields, "CIDOpenTypeYields" },
	{kMETA_VendorSpecific_first, "VendorSpecific" },
	{kMETA_VendorSpecific_last, "VendorSpecific" },
	{kMETA_UniqueName, "UniqueName" },
	{kMETA_GlyphletVersion, "GlyphletVersion" },
	{kMETA_MainGID, "MainGID" },
	{kMETA_EmbeddingInfo, "EmbeddingInfo" },
	{kMETA_UnitsPerEm, "UnitsPerEm" },
	{kMETA_VertAdvance, "VertAdvance" },
	{kMETA_VertOrigin, "VertOrigin" },
	{kMETA_UnicodeVersion, "UnicodeVersion"},
	{kMETA_nMetaRecords, "nMetaRecs"},
	{kMETA_BaseFontNamePrefix, "BaseFontNamePrefix"},
	{kMETA_CIDBaseChars_RO_substring, "CIDBaseCharRegistryOrdering"},
	{kMETA_UNIDerivedByOpenType_FEAT_substring, "UNIDerivedByOTFeature"},
	{kMETA_UNIOpenTypeYields_FEAT_substring, "UNIOTYieldsFeature"},
	{kMETA_CIDDerivedByOpenType_FEAT_substring, "CIDDerivedByOTFeature"},
	{kMETA_CIDOpenTypeYields_FEAT_substring, "CIDOTYieldsFeature"},
	{kMETA_RepositoryFilename, "RepositoryFilename"},
	{kMETA_RepositoryFilesize, "RepositoryFilesize"},
};


static IntN cmpMETAindex(const void *key, const void *value)
{
	Card16 i1 = *(Card16 *)key;
	Card16 i2 = ((tMETAkeywordsbyindex *)value)->index;
	if (i1 < i2) 
		return (-1);
	else if (i1 > i2)
		return (1);
	else
		return (0);
}

#define ARRAY_LEN(a) (sizeof(a)/sizeof((a)[0]))

static const char *getMETAkeyword(Card16 index)
{
	tMETAkeywordsbyindex *match = NULL;
	size_t len;
	
	if ((index >= kMETA_VendorSpecific_first) &&
		(index <= kMETA_VendorSpecific_last))
		return ("VendorSpecific");
	len = ARRAY_LEN(METAKeywordsbyIndex);
	match = (tMETAkeywordsbyindex *)
		bsearch((const void *)&index, (const void *)&METAKeywordsbyIndex, 
				len,
				sizeof(tMETAkeywordsbyindex), cmpMETAindex);
	return (match == NULL) ? "Unknown" : match->string;
}
