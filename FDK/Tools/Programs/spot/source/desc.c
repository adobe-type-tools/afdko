/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)desc.c	1.3
 * Changed:    12/2/98 09:49:42
 ***********************************************************************/

#include "desc.h"

static Byte8 *platform[] = 
	{
	"Unicode",					/* 0 */
	"Macintosh",				/* 1 */
	"ISO",						/* 2 */
	"Microsoft",				/* 3 */
	"Custom",					/* 4 */
	};

/* ### Unicode ### */
static Byte8 *uniScript[] =
	{
	"Unicode 1.0",				/* 0 */
	"Unicode 1.",				/* 1 */
	"ISO 10646 1993",			/* 2 */
	"UTF-16 BMP", 				/* 3 */
	"UTF-32", 					/* 4 */
	"Unicode Variation Sequences", 					/* 4 */
	};

static Byte8 *uniLanguage[] =
	{
	"Default",					/* 0 */
	};
	
/* ### ISO ### */
static Byte8 *ISOScript[] =
	{
	"7-bit ASCII",				/* 0 */
	"ISO 10646",				/* 1 */
	"ISO 8859-1",				/* 2 */
	};

/* ### Macintosh ### */
static Byte8 *macScript[] =
	{
	"Roman",					/*  0 */
	"Japanese",					/*  1 */
	"Chinese(Traditional)",		/*  2 */
	"Korean",					/*  3 */
	"Arabic",					/*  4 */
	"Hebrew",					/*  5 */
	"Greek",					/*  6 */
	"Russian",					/*  7 */
	"RSymbol",					/*  8 */
	"Devanagri",				/*  9 */
	"Gurmuki",					/* 10 */
	"Gujarati",					/* 11 */
	"Oriya",					/* 12 */
	"Bengali",					/* 13 */
	"Tamil",					/* 14 */
	"Telugu",					/* 15 */
	"Kannada",					/* 16 */
	"Malayalam",				/* 17 */
	"Sinhalese",				/* 18 */
	"Burmese",					/* 19 */
	"Khmer",					/* 20 */
	"Thai",						/* 21 */
	"Laotian",					/* 22 */
	"Georgian",					/* 23 */
	"Armenian",					/* 24 */
	"Chinese(Simplified)",		/* 25 */
	"Tibetan",					/* 26 */
	"Mongolian",				/* 27 */
	"Geez",						/* 28 */
	"Slavic",					/* 29 */
	"Vietnamese",				/* 30 */
	"Sindhi",					/* 31 */
	"Uninterp",					/* 32 */
	};
	
static Byte8 *macLanguage[] =
	{
	"English",					/*   0 */
	"French",					/*   1 */
	"German",					/*   2 */
	"Italian",					/*   3 */
	"Dutch",					/*   4 */
	"Swedish",					/*   5 */
	"Spanish",					/*   6 */
	"Danish",					/*   7 */
	"Portuguese",				/*   8 */
	"Norwegian",				/*   9 */
	"Hebrew",					/*  10 */
	"Japanese",					/*  11 */
	"Arabic",					/*  12 */
	"Finnish",					/*  13 */
	"Greek",					/*  14 */
	"Icelandic",				/*  15 */
	"Maltese",					/*  16 */
	"Turkish",					/*  17 */
	"Croatian",					/*  18 */
	"Chinese(Traditional)",		/*  19 */
	"Urdu",						/*  20 */
	"Hindi",					/*  21 */
	"Thai",						/*  22 */
	"Korean",					/*  23 */
	"Lithuanian",				/*  24 */
	"Polish",					/*  25 */
	"Hungarian",				/*  26 */
	"Estonian",					/*  27 */
	"Lettish",					/*  28 */
	"Saamisk",					/*  29 */
	"Faeroese",					/*  30 */
	"Farsi",					/*  31 */
	"Russian",					/*  32 */
	"Chinese(Simplified)",		/*  33 */
	"Flemish",					/*  34 */
	"Irish",					/*  35 */
	"Albanian",					/*  36 */
	"Romanian",					/*  37 */
	"Czech",					/*  38 */
	"Slovak",					/*  39 */
	"Slovenian",				/*  40 */
	"Yiddish",					/*  41 */
	"Serbian",					/*  42 */
	"Macedonian",				/*  43 */
	"Bulgarian",				/*  44 */
	"Ukrainian",				/*  45 */
	"Byelorussian",				/*  46 */
	"Uzbek",					/*  47 */
	"Kazakh",					/*  48 */
	"Azerbaijani",				/*  49 */
	"Azerbaijan(Armenian)",	/*  50 */
	"Armenian",					/*  51 */
	"Georgian",					/*  52 */
	"Moldavian",				/*  53 */
	"Kirghiz",					/*  54 */
	"Tajiki",					/*  55 */
	"Turkmen",					/*  56 */
	"Mongolian",				/*  57 */
	"Mongolian(Cyrillic)",		/*  58 */
	"Pashto",					/*  59 */
	"Kurdish",					/*  60 */
	"Kashmiri",					/*  61 */
	"Sindhi",					/*  62 */
	"Tibetan",					/*  63 */
	"Nepali",					/*  64 */
	"Sanskrit",					/*  65 */
	"Marathi",					/*  66 */
	"Bengali",					/*  67 */
	"Assamese",					/*  68 */
	"Gujarati",					/*  69 */
	"Punjabi",					/*  70 */
	"Oriya",					/*  71 */
	"Malayalam",				/*  72 */
	"Kannada",					/*  73 */
	"Tamil",					/*  74 */
	"Telugu",					/*  75 */
	"Sinhalese",				/*  76 */
	"Burmese",					/*  77 */
	"Khmer",					/*  78 */
	"Lao",						/*  79 */
	"Vietnamese",				/*  80 */
	"Indonesian",				/*  81 */
	"Tagalog",					/*  82 */
	"MalayRoman",				/*  83 */
	"MalayArabic",				/*  84 */
	"Amharic",					/*  85 */
	"Tigrinya",					/*  86 */
	"Galla",					/*  87 */
	"Somali",					/*  88 */
	"Swahili",					/*  89 */
	"Ruanda",					/*  90 */
	"Rundi",					/*  91 */
	"Chewa",					/*  92 */
	"Malagasy",					/*  93 */
	"Esperanto",				/*  94 */
	"Welsh",					/* 128 */
	"Basque",					/* 129 */
	"Catalan",					/* 130 */
	"Latin",					/* 131 */
	"Quechua",					/* 132 */
	"Guarani",					/* 133 */
	"Aymara",					/* 134 */
	"Tatar",					/* 135 */
	"Uighur",					/* 136 */
	"Dzongkha",					/* 137 */
	"Javanese",					/* 138 */
	"Sundanese",				/* 139 */
	};

/* ### Microsoft ### */
static Byte8 *MSScript[] =
	{
	"Symbol",					/* 0 */
	"Unicode BMP only",         /* 1 */
	"ShiftJIS",					/* 2 */
	"PRC",						/* 3 */
    "Big5",						/* 4 */
	"Wangsung",					/* 5 */
	"Johab",					/* 6 */
	"--unknown--",				/* 7 */
	"--unknown--",				/* 8 */
	"--unknown--",				/* 9 */
	"Unicode UCS-4",					/* 10 */
	};
	
typedef struct
	{
	Card16 LCID;
	Byte8 *desc;
	} MSLangEntry;

/* Must be sorted by LCID */
static MSLangEntry MSLanguage[] =
	{
	{0x0401, "Arabic(Saudi Arabia)"},
	{0x0402, "Bulgarian"},
	{0x0403, "Catalan"},
	{0x0404, "Chinese(Taiwan)"},
	{0x0405, "Czech"},
	{0x0406, "Danish"},
	{0x0407, "German(Standard)"},
	{0x0408, "Greek"},
	{0x0409, "English(American)"},
	{0x040a, "Spanish(Traditional)"},
	{0x040b, "Finnish"},
	{0x040c, "French(Standard)"},
	{0x040e, "Hungarian"},
	{0x040f, "Icelandic"},
	{0x0410, "Italian(Standard)"},
	{0x0411, "Japanese"},
	{0x0412, "Korean"},
	{0x0413, "Dutch(Standard)"},
	{0x0414, "Norwegian(Bokmal)"},
	{0x0415, "Polish"},
	{0x0416, "Portuguese(Brazilian)"},
	{0x0418, "Romanian"},
	{0x0419, "Russian"},
	{0x041a, "Croatian"},
	{0x041b, "Slovak"},
	{0x041c, "Albanian"},
	{0x041d, "Swedish"},
	{0x041f, "Turkish"},
	{0x0422, "Ukrainian"},
	{0x0423, "Byelorussian"},
	{0x0424, "Slovenian"},
	{0x0425, "Estonian"},
	{0x0426, "Latvian"},  
	{0x0427, "Lithuanian"},
	{0x042d, "Basque"},
	{0x042f, "Macedonian"},
	{0x0804, "Chinese(PR China)"},
	{0x0807, "German(Swiss)"},
	{0x0809, "English(British)"},
	{0x080a, "Spanish(Mexican)"},
	{0x080c, "French(Belgian)"},
	{0x0810, "Italian(Swiss)"},
	{0x0813, "Belgian(Flemmish)"},
	{0x0814, "Norwegian(Nynorsk)"},
	{0x0816, "Portuguese(Standard)"},
	{0x0c04, "Chinese(Hong Kong)"},
	{0x0c07, "German(Austrian)"},
	{0x0c09, "English(Australian)"},
	{0x0c0a, "Spanish(Mordern)"},
	{0x0c0c, "French(Canadian)"},
	{0x1004, "Chinese(Singapore)"},
	{0x1007, "German(Luxemborg)"},
	{0x1009, "English(Canadian)"},
	{0x100c, "French(Swiss)"},
	{0x1407, "German(Liechtenstein)"},
	{0x1409, "English(New Zealand)"},
	{0x140c, "French(Luxemborg)"},
	{0x1809, "English(Ireland)"},
	};

/* ### Names ### */
static Byte8 *name[] =
	{
	"Copyright",	/* 0 */
	"Family",		/* 1 */
	"Style",		/* 2 */
	"UniqueId",		/* 3 */
	"Full",			/* 4 */
	"Version",		/* 5 */
	"PostScript",	/* 6 */
	"Trademark",	/* 7 */
	"Foundry",		/* 8 */
	};

static Byte8 *unknown = "--unknown--";

#define SIZE(t) (sizeof(t)/sizeof(t[0]))

/* Return platform decription */
Byte8 *descPlat(Card16 platformId)
	{
	if (platformId < SIZE(platform))
		return platform[platformId];
	else
		return unknown;
	}

/* Match LCID */
static IntN matchLCIDs(const void *key, const void *value)
	{
	Card16 a = *(Card16 *)key;
	Card16 b = ((MSLangEntry *)value)->LCID;
	if (a < b)
		return -1;
	else if (a > b)
		return 1;
	else
		return 0;
	}

/* Lookup Microsoft LCID */
static Byte8 *MSLangLookup(Card16 LCID)
	{
	MSLangEntry *entry = 
		(MSLangEntry *)bsearch(&LCID, MSLanguage, SIZE(MSLanguage),
							   sizeof(MSLangEntry), matchLCIDs);
	return entry == NULL ? unknown : entry->desc;
	}

/* Return script description */
Byte8 *descScript(Card16 platformId, Card16 scriptId)
	{
	switch (platformId)
		{
	case 0:
		if (scriptId < SIZE(uniScript))
			return uniScript[scriptId];
		else 
			return unknown;
	case 1:
		if (scriptId < SIZE(macScript))
			return macScript[scriptId];
		else
			return unknown;
	case 2:
		if (scriptId < SIZE(ISOScript))
			return ISOScript[scriptId];
		else
			return unknown;
	case 3:
		if (scriptId < SIZE(MSScript))
			return MSScript[scriptId];
		else
			return unknown;
	case 4:
		return "--custom--";
	default:
		return unknown;
		}
	}

/* Return language description */
Byte8 *descLang(Card16 cmap, Card16 platformId, Card16 languageId)
	{
	switch (platformId)
		{
	case 0:
		if (languageId < SIZE(uniLanguage))
			return uniLanguage[languageId];
		else 
			return unknown;
	case 1:
		if (cmap)
			{
			/* cmap languages start from 1 */
			if (languageId == 0)
				return "Unspecific";
			else
				languageId--;
			}
		if (languageId < SIZE(macLanguage))
			return macLanguage[languageId];
		else
			return unknown;
	case 2:
		return "--ISO--";
	case 3:
		if (cmap)
			return "--vers--";
		else
			return MSLangLookup(languageId);
	case 4:
		return "--cust--";
	default:
		return unknown;
		}
	}

/* Return name description */
Byte8 *descName(Card16 nameId)
	{
	if (nameId < SIZE(name))
		return name[nameId];
	else if (nameId < 255)
		return "Standard";
	else
		return "Font-specific";
	}


