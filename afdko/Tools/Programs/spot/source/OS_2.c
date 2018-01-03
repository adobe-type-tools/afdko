/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)OS_2.c	1.19
 * Changed:    5/19/99 17:51:52
 ***********************************************************************/

#include "OS_2.h"
#include "sfnt_OS_2.h"
#include "sfnt.h"
static OS_2Tbl *OS_2 = NULL;
static IntX loaded = 0;

void OS_2Read(LongN start, Card32 length)
	{
	if (loaded)
		return;

	OS_2 = (OS_2Tbl *)memNew(sizeof(OS_2Tbl));

	SEEK_ABS(start);
	
	IN1(OS_2->version);
	IN1(OS_2->averageWidth);
	IN1(OS_2->weightClass);
	IN1(OS_2->widthClass);
	IN1(OS_2->type);
	IN1(OS_2->subscriptXSize);
	IN1(OS_2->subscriptYSize);
	IN1(OS_2->subscriptXOffset);
	IN1(OS_2->subscriptYOffset);
	IN1(OS_2->superscriptXSize);
	IN1(OS_2->superscriptYSize);
	IN1(OS_2->superscriptXOffset);
	IN1(OS_2->superscriptYOffset);
	IN1(OS_2->strikeoutSize);
	IN1(OS_2->strikeoutPosition);
	IN1(OS_2->familyClass);
	IN_BYTES(sizeof(OS_2->panose), OS_2->panose);
	IN1(OS_2->charRange[0]);
	IN1(OS_2->charRange[1]);
	IN1(OS_2->charRange[2]);
	IN1(OS_2->charRange[3]);
	IN_BYTES(sizeof(OS_2->vendor), OS_2->vendor);
	IN1(OS_2->selection);
	IN1(OS_2->firstChar);
	IN1(OS_2->lastChar);
	IN1(OS_2->typographicAscent);
	IN1(OS_2->typographicDescent);
	IN1(OS_2->typographicLineGap);
	IN1(OS_2->windowsAscent);
	IN1(OS_2->windowsDescent);
	if (OS_2->version > 0) 
		{
		IN1(OS_2->CodePageRange[0]);
		IN1(OS_2->CodePageRange[1]);
		}
	if (OS_2->version > 1)
		{
		IN1(OS_2->XHeight);
		IN1(OS_2->CapHeight);
		IN1(OS_2->DefaultChar);
		IN1(OS_2->BreakChar);
		IN1(OS_2->maxContext);
		}
    if (OS_2->version > 4)
    {
        IN1(OS_2->usLowerOpticalPointSize);
        IN1(OS_2->usUpperOpticalPointSize);
    }
	loaded = 1;
	}

void dumpWeightClass(IntX level)
	{
	if (level < 3)
		DLu(2, "weightClass       =", OS_2->weightClass);
	else if (level < 5)
		{
		fprintf(OUTPUTBUFF,  "weightClass       =%hu       (", OS_2->weightClass);
		switch (OS_2->weightClass) 
			{
		case 100: fprintf(OUTPUTBUFF,  "Thin"); break;
		case 200: fprintf(OUTPUTBUFF,  "Extra-Light/Ultra-Light"); break;
		case 300: fprintf(OUTPUTBUFF,  "Light"); break;
		case 400: fprintf(OUTPUTBUFF,  "Normal/Regular"); break;
		case 500: fprintf(OUTPUTBUFF,  "Medium"); break;
		case 600: fprintf(OUTPUTBUFF,  "Semi-Bold/Demi-Bold"); break;
		case 700: fprintf(OUTPUTBUFF,  "Bold"); break;
		case 800: fprintf(OUTPUTBUFF,  "Extra-Bold/Ultra-Bold"); break;
		case 900: fprintf(OUTPUTBUFF,  "Black/Heavy"); break;
		default:  fprintf(OUTPUTBUFF,  "Non-Standard value"); break;
			}
		fprintf(OUTPUTBUFF,  ")\n");
		}
	}

void dumpWidthClass(IntX level)
	{
	if (level < 3)
		DLu(2, "widthClass        =", OS_2->widthClass);
	else if (level < 5)
		{
		fprintf(OUTPUTBUFF,  "widthClass        =%hu         (", OS_2->widthClass);
		switch (OS_2->widthClass) 
			{
		case 1: fprintf(OUTPUTBUFF,  "Ultra-Condensed (50%% of normal)"); break;
		case 2: fprintf(OUTPUTBUFF,  "Extra-Condensed (62.5%% of normal)"); break;
		case 3: fprintf(OUTPUTBUFF,  "Condensed (75%% of normal)"); break;
		case 4: fprintf(OUTPUTBUFF,  "Semi-Condensed (87.5%% of normal)"); break;
		case 5: fprintf(OUTPUTBUFF,  "Medium/Normal (100%% of normal)"); break;
		case 6: fprintf(OUTPUTBUFF,  "Semi-Expanded (112.5%% of normal)"); break;
		case 7: fprintf(OUTPUTBUFF,  "Expanded (125%% of normal)"); break;
		case 8: fprintf(OUTPUTBUFF,  "Extra-Expanded (150%% of normal)"); break;
		case 9: fprintf(OUTPUTBUFF,  "Ultra-Expanded (200%% of normal)"); break;
		default:  fprintf(OUTPUTBUFF,  "Non-Standard value"); break;
			}
		fprintf(OUTPUTBUFF,  ")\n");
		}
	}

void dumpFamilyClass(IntX level)
	{
	if (level < 3)
		DLx(2, "familyClass       =", OS_2->familyClass);
	else if (level < 5)
		{
		IntX class = OS_2->familyClass>>8 & 0xff;
		IntX subclass = OS_2->familyClass & 0xff;

		fprintf(OUTPUTBUFF,  "familyClass       =%04hx       (class   =", OS_2->familyClass);
		switch (class)
			{
		case 0: fprintf(OUTPUTBUFF,  "No Classification"); break;
		case 1: fprintf(OUTPUTBUFF,  "Oldstyle Serifs"); break;
		case 2: fprintf(OUTPUTBUFF,  "Transitional Serifs"); break;
		case 3: fprintf(OUTPUTBUFF,  "Modern Serifs"); break;
		case 4: fprintf(OUTPUTBUFF,  "Clarendon Serifs"); break;
		case 5: fprintf(OUTPUTBUFF,  "Slab Serifs"); break;
		case 6: fprintf(OUTPUTBUFF,  "Reserved"); goto reserved;
		case 7: fprintf(OUTPUTBUFF,  "Freeform Serifs"); break;
		case 8: fprintf(OUTPUTBUFF,  "Sans Serif"); break;
		case 9: fprintf(OUTPUTBUFF,  "Ornamentals"); break;
		case 10: fprintf(OUTPUTBUFF,  "Scripts"); break;
		case 11: fprintf(OUTPUTBUFF,  "Reserved"); goto reserved;
		case 12: fprintf(OUTPUTBUFF,  "Symbolic"); break;
		case 13: fprintf(OUTPUTBUFF,  "Reserved"); goto reserved;
		case 14: fprintf(OUTPUTBUFF,  "Reserved"); goto reserved;
		default: fprintf(OUTPUTBUFF,  "Unknown"); goto reserved;
			}
			
		fprintf(OUTPUTBUFF,  "\n                               subclass=");
		switch (class)
			{
		case 0: fprintf(OUTPUTBUFF,  "No Classification"); break;

		case 1:
			switch (subclass)
				{
			case 0: fprintf(OUTPUTBUFF,  "No Classification"); break;
			case 1: fprintf(OUTPUTBUFF,  "IBM Rounded Legibility (e.g., IBM Sonoran Serif)");
				break;
			case 2: fprintf(OUTPUTBUFF,  "Garalde (e.g., ITC Garamond)"); break;
			case 3: fprintf(OUTPUTBUFF,  "Venetian (e.g., Goudy)"); break;
			case 4: fprintf(OUTPUTBUFF,  "Modified Venetian (e.g., Palatino)"); break;
			case 5: fprintf(OUTPUTBUFF,  "Dutch Modern (e.g., Times New Roman)"); break;
			case 6: fprintf(OUTPUTBUFF,  "Dutch Traditional (e.g., IBM Press Roman)"); break;
			case 7: fprintf(OUTPUTBUFF,  "Contemporary (e.g., University)"); break;
			case 8: fprintf(OUTPUTBUFF,  "Calligraphic"); break;
			case 15: fprintf(OUTPUTBUFF,  "Miscellaneous"); break;
			default: fprintf(OUTPUTBUFF,  "Reserved"); break;
				}
			break;

		case 2:
			switch (subclass)
				{
			case 0: fprintf(OUTPUTBUFF,  "No Classification"); break;
			case 1: fprintf(OUTPUTBUFF,  "Direct Line (e.g., MT Baskerville)"); break;
			case 2: fprintf(OUTPUTBUFF,  "Script (e.g., IBM Nasseem)"); break;
			case 15: fprintf(OUTPUTBUFF,  "Miscellaneous"); break;
			default: fprintf(OUTPUTBUFF,  "Reserved"); break;
				}
			break;

		case 3:
			switch (subclass)
				{
			case 0: fprintf(OUTPUTBUFF,  "No Classification"); break;
			case 1: fprintf(OUTPUTBUFF,  "Italian (e.g., MT Bodoni)"); break;
			case 2: fprintf(OUTPUTBUFF,  "Script (e.g., IBM Narkissim)"); break;
			case 15: fprintf(OUTPUTBUFF,  "Miscellaneous"); break;
			default: fprintf(OUTPUTBUFF,  "Reserved"); break;
				}
			break;

		case 4:
			switch (subclass)
				{
			case 0: fprintf(OUTPUTBUFF,  "No Classification"); break;
			case 1: fprintf(OUTPUTBUFF,  "Clarendon (e.g., Clarendon)"); break;
			case 2: fprintf(OUTPUTBUFF,  "Modern (e.g., MT Century Schoolbook)"); break;
			case 3: fprintf(OUTPUTBUFF,  "Traditional (e.g., MT Century)"); break;
			case 4: fprintf(OUTPUTBUFF,  "Newspaper (e.g., Excelsior)"); break;
			case 5: fprintf(OUTPUTBUFF,  "Stub Serif (e.g., Cheltenham)"); break;
			case 6: fprintf(OUTPUTBUFF,  "Monotone (e.g., ITC Korinna)"); break;
			case 7: fprintf(OUTPUTBUFF,  "Typewrite (e.g., Prestige Elite)"); break;
			case 15: fprintf(OUTPUTBUFF,  "Miscellaneous"); break;
			default: fprintf(OUTPUTBUFF,  "Reserved"); break;
				}
			break;

		case 5:
			switch (subclass)
				{
			case 0: fprintf(OUTPUTBUFF,  "No Classification"); break;
			case 1: fprintf(OUTPUTBUFF,  "Monotone (e.g., ITC Lubalin)"); break;
			case 2: fprintf(OUTPUTBUFF,  "Humanist (e.g., Candida)"); break;
			case 3: fprintf(OUTPUTBUFF,  "Geometric (e.g., MT Rockwell)"); break;
			case 4: fprintf(OUTPUTBUFF,  "Swiss (e.g., Serifa)"); break;
			case 5: fprintf(OUTPUTBUFF,  "Typewriter (e.g., Courier)"); break;
			case 15: fprintf(OUTPUTBUFF,  "Miscellaneous"); break;
			default: fprintf(OUTPUTBUFF,  "Reserved"); break;
				}
			break;

		case 7:
			switch (subclass)
				{
			case 0: fprintf(OUTPUTBUFF,  "No Classification"); break;
			case 1: fprintf(OUTPUTBUFF,  "Modern (e.g., ITC Souvenir)"); break;
			case 15: fprintf(OUTPUTBUFF,  "Miscellaneous"); break;
			default: fprintf(OUTPUTBUFF,  "Reserved"); break;
				}
			break;

		case 8:
			switch (subclass)
				{
			case 0: fprintf(OUTPUTBUFF,  "No Classification"); break;
			case 1: 
				printf( "IBM Neo-grotesque Gothic (e.g., IBM Sonoran Sans)"); 
				break;
			case 2: fprintf(OUTPUTBUFF,  "Humanist (e.g., Optima)"); break;
			case 3: fprintf(OUTPUTBUFF,  "Low-x Round Geometric (e.g., Futura)"); break;
			case 4: 
				fprintf(OUTPUTBUFF,  "High-x Round Geometric "
					   "(e.g., ITC Avant Garde Gothic)"); 
				break;
			case 5: fprintf(OUTPUTBUFF,  "Neo-grotesque Gothic (e.g., Helvetica)"); break;
			case 6: fprintf(OUTPUTBUFF,  "Modified Neo-grotesque Gothic (e.g., Univers)"); 
				break;
			case 9: fprintf(OUTPUTBUFF,  "Typewriter Gothic (e.g., IBM Letter Gothic)"); 
				break;
			case 10: fprintf(OUTPUTBUFF,  "Matrix (e.g., IBM Matrix Gothic)"); break;
			case 15: fprintf(OUTPUTBUFF,  "Miscellaneous"); break;
			default: fprintf(OUTPUTBUFF,  "Reserved"); break;
				}
			break;

		case 9:
			switch (subclass)
				{
			case 0: fprintf(OUTPUTBUFF,  "No Classification"); break;
			case 1: fprintf(OUTPUTBUFF,  "Engraver (e.g., Copperplate)"); break;
			case 2: fprintf(OUTPUTBUFF,  "Black letter (e.g., Old English)"); break;
			case 3: fprintf(OUTPUTBUFF,  "Decorative (e.g., Saphire)"); break;
			case 4: fprintf(OUTPUTBUFF,  "Three Dimensional (e.g., Thorne Shaded)"); break;
			case 15: fprintf(OUTPUTBUFF,  "Miscellaneous"); break;
			default: fprintf(OUTPUTBUFF,  "Reserved"); break;
				}
			break;

		case 10:
			switch (subclass)
				{
			case 0: fprintf(OUTPUTBUFF,  "No Classification"); break;
			case 1: fprintf(OUTPUTBUFF,  "Uncial (e.g., Libra)"); break;
			case 2: fprintf(OUTPUTBUFF,  "Brush Joined (e.g., Mistral)"); break;
			case 3: fprintf(OUTPUTBUFF,  "Formal Joined (e.g., Coronet)"); break;
			case 4: fprintf(OUTPUTBUFF,  "Monotone Joined (e.g., Kaufmann)"); break;
			case 5: fprintf(OUTPUTBUFF,  "Calligraphic (e.g., Thompson Quillscript)"); break;
			case 6: fprintf(OUTPUTBUFF,  "Brush Unjoined (e.g., Saltino)"); break;
			case 7: fprintf(OUTPUTBUFF,  "Formal Unjoined (e.g., Virtuosa)"); break;
			case 8: fprintf(OUTPUTBUFF,  "Monotone Unjoined (e.g., Gilles Gothic)"); break;
			case 15: fprintf(OUTPUTBUFF,  "Miscellaneous"); break;
			default: fprintf(OUTPUTBUFF,  "Reserved"); break;
				}
			break;

		case 12:
			switch (subclass)
				{
			case 0: fprintf(OUTPUTBUFF,  "No Classification"); break;
			case 3: fprintf(OUTPUTBUFF,  "Mixed Serif (e.g., IBM Symbol)"); break;
			case 6: fprintf(OUTPUTBUFF,  "Oldstyle Serif (e.g., IBM Sonoran Pi Serif)"); 
				break;
			case 7: 
				fprintf(OUTPUTBUFF,  "Neo-grotesque Sans Serif (e.g., IBM Sonoran Pi Sans)");
				break;
			case 15: fprintf(OUTPUTBUFF,  "Miscellaneous"); break;
			default: fprintf(OUTPUTBUFF,  "Reserved"); break;
				}
			break;
			}

	reserved:
		fprintf(OUTPUTBUFF,  ")\n");
		}
	}

/* Make panose dump string */
static Byte8 *DLPanose(Byte8 buf[11])
	{
	IntX i;

	for (i = 0; i < 10; i++)
		buf[i] = OS_2->panose[i] + ((OS_2->panose[i] < 10) ? '0' : 'a' - 10);
	buf[10] = '\0';

	return buf;
	}

void dumpPanoseElement(IntX i, IntX cnt, const Byte8 *variation[])
	{
	IntX value = OS_2->panose[i];
	
	if (i != 0)
		fprintf(OUTPUTBUFF,  "                               ");
	fprintf(OUTPUTBUFF,  "%-16s (%1x)=", variation[0], value);

	if (value == 0)
		fprintf(OUTPUTBUFF,  "Any");
	else if (value > cnt - 1)
		fprintf(OUTPUTBUFF,  "Unknown");
	else
		fprintf(OUTPUTBUFF,  "%s", variation[value]);
		   
	if (i == 9)
		fprintf(OUTPUTBUFF,  ")\n");
	else
		fprintf(OUTPUTBUFF,  "\n");
	}
	
void dumpPanose2()
{
static Byte8 panose[11];
static const Byte8 *variation0[] =
		{
		"Family Kind",
		"No Fit",
		"Text & Display",
		"Script",
		"Decorative",
		"Pictorial",
		};
	static const Byte8 *variation1[] =
		{
		"Serif Style",
		"No Fit",
		"Cove",
		"Obtuse Cove",
		"Square Cove",
		"Obtuse Square Cove",
		"Square",
		"Thin",
		"Bone",
		"Exaggerated",
		"Triangle",
		"Normal Sans",
		"Obtuse Sans",
		"Perp Sans",
		"Flared",
		"Rounded",
		};
	static const Byte8 *variation2[] =
		{
		"Weight",
		"No Fit",
		"Very Light",
		"Light",
		"Thin",
		"Book",
		"Medium",
		"Demi",
		"Bold",
		"Heavy",
		"Black",
		"Nord",
		};
	static const Byte8 *variation3[] =
		{
		"Proportion",
		"No Fit",
		"Old Style",
		"Modern",
		"Even Width",
		"Expanded",
		"Condensed",
		"Very Expanded",
		"Very Condensed",
		"Monospaced",
		};
	static const Byte8 *variation4[] =
		{
		"Contrast",
		"No Fit",
		"None",
		"Very Low",
		"Low",
		"Medium Low",
		"Medium",
		"Medium High",
		"High",
		"Very High",
		};
	static const Byte8 *variation5[] =
		{
		"Stroke Variation",
		"No Fit",
		"Gradual/Diagonal",
		"Gradual/Transitional",
		"Gradual/Vertical",
		"Gradual/Horizontal",
		"Rapid/Vertical",
		"Rapid/Horizontal",
		"Instant/Vertical",
		};
	static const Byte8 *variation6[] =
		{
		"Arm Style",
		"No Fit",
		"Straight Arms/Horizontal",
		"Straight Arms/Wedge",
		"Straight Arms/Vertical",
		"Straight Arms/Single Serif",
		"Straight Arms/Double Serif",
		"Non-Straight Arms/Horizontal",
		"Non-Straight Arms/Wedge",
		"Non-Straight Arms/Vertical",
		"Non-Straight Arms/Single Serif",
		"Non-Straight Arms/Double Serif",
		};
	static const Byte8 *variation7[] =
		{
		"Letterform",
		"No Fit",
		"Normal/Contact",
		"Normal/Weighted",
		"Normal/Boxed",
		"Normal/Flattened",
		"Normal/Rounded",
		"Normal/OffCenter",
		"Normal/Square",
		"Oblique/Contact",
		"Oblique/Weighted",
		"Oblique/Boxed",
		"Oblique/Flattened",
		"Oblique/Rounded",
		"Oblique/OffCenter",
		"Oblique/Square",
		};
	static const Byte8 *variation8[] =
		{
		"Midline",
		"No Fit",
		"Standard/Trimmed",
		"Standard/Pointed",
		"Standard/Serifed",
		"High/Trimmed",
		"High/Pointed",
		"High/Serifed",
		"Constant/Trimmed",
		"Constant/Pointed",
		"Constant/Serifed",
		"Low/Trimmed",
		"Low/Pointed",
		"Low/Serifed",
		};
	static const Byte8 *variation9[] =
		{
		"X-height",
		"No Fit",
		"Constant/Small",
		"Constant/Standard",
		"Constant/Large",
		"Ducking/Small",
		"Ducking/Standard",
		"Ducking/Large",
		};

	
	   fprintf(OUTPUTBUFF,  "panose            =%s (", DLPanose(panose));
	   dumpPanoseElement(0, TABLE_LEN(variation0), variation0);
	   dumpPanoseElement(1, TABLE_LEN(variation1), variation1);
	   dumpPanoseElement(2, TABLE_LEN(variation2), variation2);
	   dumpPanoseElement(3, TABLE_LEN(variation3), variation3);
	   dumpPanoseElement(4, TABLE_LEN(variation4), variation4);
	   dumpPanoseElement(5, TABLE_LEN(variation5), variation5);
	   dumpPanoseElement(6, TABLE_LEN(variation6), variation6);
	   dumpPanoseElement(7, TABLE_LEN(variation7), variation7);
	   dumpPanoseElement(8, TABLE_LEN(variation8), variation8);
	   dumpPanoseElement(9, TABLE_LEN(variation9), variation9);
	}

void dumpPanose3()
{
static Byte8 panose[11];
static const Byte8 *variation0[] =
		{
		"Family Kind",
		"No Fit",
		"Text & Display",
		"Script",
		"Decorative",
		"Pictorial",
		};
	static const Byte8 *variation1[] =
		{
		"Tool Kind",
		"No Fit",
		"Flat Nib",
		"Pressure Point",
		"Engraved",
		"Ball (Round Cap)",
		"Brush",
		"Rough",
		"Felt Pen/Brush Tip",
		"Wild Brush - Drips a lot",
		};
	static const Byte8 *variation2[] =
		{
		"Weight",
		"No Fit",
		"Very Light",
		"Light",
		"Thin",
		"Book",
		"Medium",
		"Demi",
		"Bold",
		"Heavy",
		"Black",
		"Nord",
		};
	static const Byte8 *variation3[] =
		{
		"Spacing",
		"No Fit",
		"Proportional Spaced",
		"Monospaced",
		};
	static const Byte8 *variation4[] =
		{
		"Aspect Ratio",
		"No Fit",
		"Very Condensed",
		"Condensed",
		"Normal",
		"Expanded",
		"Very Expanded",
		};
	static const Byte8 *variation5[] =
		{
		"Contrast",
		"No Fit",
		"None",
		"Very Low",
		"Low",
		"Medium Low",
		"Medium",
		"Medium High",
		"High",
		"Very High",
		};
	static const Byte8 *variation6[] =
		{
		"Topology",
		"No Fit",
		"Roman Disconnected",
		"Roman Trailing",
		"Roman Connected",
		"Cursive Disconnected",
		"Cursive Trailing",
		"Cursive Connected",
		"Blackletter Disconnected",
		"Blackletter Trailing",
		"Blackletter Connected",
		};
	static const Byte8 *variation7[] =
		{
		"Form",
		"No Fit",
		"Upright / No Wrapping",
		"Upright / Some Wrapping",
		"Upright / More Wrapping",
		"Upright / Extreme Wrapping",
		"Oblique / No Wrapping",
		"Oblique / Some Wrapping",
		"Oblique / More Wrapping",
		"Oblique / Extreme Wrapping",
		"Exaggerated / No Wrapping",
		"Exaggerated / Some Wrapping",
		"Exaggerated / More Wrapping",
		"Exaggerated / Extreme Wrapping",
		};
	static const Byte8 *variation8[] =
		{
		"Finial",
		"No Fit",
		"None / No loops",
		"None / Closed loops",
		"None / Open loops",
		"Sharp / No loops",
		"Sharp / Closed loops",
		"Sharp / Open loops",
		"Tapered / No loops",
		"Tapered / Closed loops",
		"Tapered / Open loops",
		"Round / No loops",
		"Round / Closed loops",
		"Round / Open loops",
		};
	static const Byte8 *variation9[] =
		{
		"X-ascent",
		"No Fit",
		"Very low",
		"Low",
		"Medium",
		"High",
		"Very High",
		};

	
	   fprintf(OUTPUTBUFF,  "panose            =%s (", DLPanose(panose));
	   dumpPanoseElement(0, TABLE_LEN(variation0), variation0);
	   dumpPanoseElement(1, TABLE_LEN(variation1), variation1);
	   dumpPanoseElement(2, TABLE_LEN(variation2), variation2);
	   dumpPanoseElement(3, TABLE_LEN(variation3), variation3);
	   dumpPanoseElement(4, TABLE_LEN(variation4), variation4);
	   dumpPanoseElement(5, TABLE_LEN(variation5), variation5);
	   dumpPanoseElement(6, TABLE_LEN(variation6), variation6);
	   dumpPanoseElement(7, TABLE_LEN(variation7), variation7);
	   dumpPanoseElement(8, TABLE_LEN(variation8), variation8);
	   dumpPanoseElement(9, TABLE_LEN(variation9), variation9);
	}

void dumpPanose4()
{
static Byte8 panose[11];
static const Byte8 *variation0[] =
		{
		"Family Kind",
		"No Fit",
		"Text & Display",
		"Script",
		"Decorative",
		"Pictorial",
		};
	static const Byte8 *variation1[] =
		{
		"Class",
		"Derivative",
		"Non-standard Topology",
		"Non-standard Elements",
		"Non-standard Aspect",
		"Initials",
		"Cartoon",
		"Picture Stems",
		"Ornamented",
		"Text and Backgroud",
		"Collage ",
		"Montage",
		};
	static const Byte8 *variation2[] =
		{
		"Weight",
		"No Fit",
		"Very Light",
		"Light",
		"Thin",
		"Book",
		"Medium",
		"Demi",
		"Bold",
		"Heavy",
		"Black",
		"Nord",
		};
	static const Byte8 *variation3[] =
		{
		"Aspect",
		"No Fit",
		"Super Condensed",
		"Very Condensed",
		"Condensed",
		"Normal",
		"Extended",
		"Very Extended",
		"Super Extended",
		};
	static const Byte8 *variation4[] =
		{
		"Contrast",
		"No Fit",
		"None",
		"Very Low",
		"Low",
		"Medium Low",
		"Medium",
		"Medium High",
		"High",
		"Very High",
		"Horizontal Low",
		"Horizontal Medium",
		"Horizontal High",
		"Broken",
		};
	static const Byte8 *variation5[] =
		{
		"Serif Variant",
		"No Fit",
		"Cove",
		"Obtuse Cove",
		"Square Cove",
		"Obtuse Square Cove",
		"Square",
		"Thin",
		"Oval",
		"Exaggerated",
		"Triangle",
		"Normal Sans",
		"Obtuse Sans",
		"Perpendicular Sans",
		"Flared",
		"Rounded",
		"Script",
		};
	static const Byte8 *variation6[] =
		{
		"Treatment",
		"No Fit",
		"None - Standard Solid Fill",
		"White / No Fill",
		"Patterned Fill",
		"Complex Fill",
		"Shaped Fill",
		"Drawn / Distressed",
		};
	static const Byte8 *variation7[] =
		{
		"Lining",
		"No Fit",
		"None",
		"Inline",
		"Outline",
		"Engraved (Multiple Lines) ",
		"Shadow",
		"Relief",
		"Backdrop",
		};
	static const Byte8 *variation8[] =
		{
		"Topology",
		"No Fit",
		"Standard",
		"Square",
		"Multiple Segment",
		"Deco (E,M,S) Waco midlines",
		"Uneven Weighting",
		"Diverse Arms",
		"Diverse Forms",
		"Lombardic Forms",
		"Upper Case in Lower Case",
		"Implied Topology",
		"Horseshoe E and A",
		"Cursive",
		"Blackletter",
		"Swash Variance",
		};
	static const Byte8 *variation9[] =
		{
		"Range of characters",
		"No Fit",
		"Extended Collection",
		"Litterals",
		"No Lower Case",
		"Small Caps",
		};

	
	   fprintf(OUTPUTBUFF,  "panose            =%s (", DLPanose(panose));
	   dumpPanoseElement(0, TABLE_LEN(variation0), variation0);
	   dumpPanoseElement(1, TABLE_LEN(variation1), variation1);
	   dumpPanoseElement(2, TABLE_LEN(variation2), variation2);
	   dumpPanoseElement(3, TABLE_LEN(variation3), variation3);
	   dumpPanoseElement(4, TABLE_LEN(variation4), variation4);
	   dumpPanoseElement(5, TABLE_LEN(variation5), variation5);
	   dumpPanoseElement(6, TABLE_LEN(variation6), variation6);
	   dumpPanoseElement(7, TABLE_LEN(variation7), variation7);
	   dumpPanoseElement(8, TABLE_LEN(variation8), variation8);
	   dumpPanoseElement(9, TABLE_LEN(variation9), variation9);
	}

void dumpPanose5()
{
static Byte8 panose[11];
static const Byte8 *variation0[] =
		{
		"Family Kind",
		"No Fit",
		"Text & Display",
		"Script",
		"Decorative",
		"Pictorial",
		};
	static const Byte8 *variation1[] =
		{
		"Kind",
		"No Fit",
		"Montages",
		"Pictures",
		"Shapes",
		"Scientific",
		"Music",
		"Expert",
		"Patterns",
		"Boarders",
		"Icons",
		"Logos",
		"Industry specific",
		};
	static const Byte8 *variation2[] =
		{
		"Weight",
		"No Fit",
		};
	static const Byte8 *variation3[] =
		{
		"Spacing",
		"No Fit",
		"Proportional Spaced",
		"Monospaced",
		};
	static const Byte8 *variation4[] =
		{
		"Aspect Ratio",
		"No Fit",
		};
	static const Byte8 *variation5[] =
		{
		"Aspect ratio of character 94",
		"No Fit",
		"No Width",
		"Exceptionally Wide",
		"Super Wide",
		"Very Wide",
		"Wide",
		"Normal",
		"Narrow",
		"Very Narrow",
		};
	static const Byte8 *variation6[] =
		{
		"Aspect ratio of character 119",
		"No Fit",
		"No Width",
		"Exceptionally Wide",
		"Super Wide",
		"Very Wide",
		"Wide",
		"Normal",
		"Narrow",
		"Very Narrow",
		};
	static const Byte8 *variation7[] =
		{
		"Aspect ratio of character 157",
		"No Fit",
		"No Width",
		"Exceptionally Wide",
		"Super Wide",
		"Very Wide",
		"Wide",
		"Normal",
		"Narrow",
		"Very Narrow",
		};
	static const Byte8 *variation8[] =
		{
		"Aspect ratio of character 163",
		"No Fit",
		"No Width",
		"Exceptionally Wide",
		"Super Wide",
		"Very Wide",
		"Wide",
		"Normal",
		"Narrow",
		"Very Narrow",
		};
	static const Byte8 *variation9[] =
		{
		"Aspect ratio of character 211",
		"No Fit",
		"No Width",
		"Exceptionally Wide",
		"Super Wide",
		"Very Wide",
		"Wide",
		"Normal",
		"Narrow",
		"Very Narrow",
		};

	
	   fprintf(OUTPUTBUFF,  "panose            =%s (", DLPanose(panose));
	   dumpPanoseElement(0, TABLE_LEN(variation0), variation0);
	   dumpPanoseElement(1, TABLE_LEN(variation1), variation1);
	   dumpPanoseElement(2, TABLE_LEN(variation2), variation2);
	   dumpPanoseElement(3, TABLE_LEN(variation3), variation3);
	   dumpPanoseElement(4, TABLE_LEN(variation4), variation4);
	   dumpPanoseElement(5, TABLE_LEN(variation5), variation5);
	   dumpPanoseElement(6, TABLE_LEN(variation6), variation6);
	   dumpPanoseElement(7, TABLE_LEN(variation7), variation7);
	   dumpPanoseElement(8, TABLE_LEN(variation8), variation8);
	   dumpPanoseElement(9, TABLE_LEN(variation9), variation9);
	}

void dumpPanose(IntX level)
	{
	static Byte8 panose[11];
	
	if (level < 3)
		DL(2, (OUTPUTBUFF, "panose            =%s\n", DLPanose(panose)));
	else if (level < 5)
	   {
	   	 DLPanose(panose);
	   	 switch(panose[0])
	   	 {
	   	 	case '0':
	   	 		dumpPanose2();
	   	 		break;
	   	 	case '1':
	   	 		dumpPanose2();
	   	 		break;
	   	 	case '2':
	   	 		dumpPanose2();
	   	 		break;
	   	 	case '3':
	   	 		dumpPanose3();
	   	 		break;
	   	 	case '4':
	   	 		dumpPanose4();
	   	 		break;
	   	 	case '5':
	   	 		dumpPanose5();
	   	 		break;
	   	 	default:
	   	 		fprintf(OUTPUTBUFF,  "OTFProof [WARNING]: unknown Family Kind in Panose[0]:%c.\n", panose[0]);
	   	 		
		}
		}
	}
	
	
	
	
	
void dumpRangeDesc(Byte8 *fieldName, Card32 fieldValue, const Byte8 *rangeDesc[])
	{
	IntX j;
	IntX set = 0;
	IntX mask = 1;

	fprintf(OUTPUTBUFF,  "%-18s=%08lx", fieldName, fieldValue);
	for (j = 0; j < 32; j++)
		{
		if (fieldValue & mask)
			{
			if (set)
				fprintf(OUTPUTBUFF,  "\n                               %s", rangeDesc[j]);
			else
				{
				fprintf(OUTPUTBUFF,  "   (%s", rangeDesc[j]);
				set = 1;
				}
			}
		mask <<= 1;
		}
	if (set)
		fprintf(OUTPUTBUFF,  ")\n");
	else
		fprintf(OUTPUTBUFF,  "\n");
	}


void dumpUnicodeRanges(IntX level)
	{
	static const Byte8 *rangeDesc[] =
		{
		"Basic Latin",									/*   0 */
		"Latin-1 Supplement",							/*   1 */
		"Latin Extended-A",								/*   2 */
		"Latin Extended-B",								/*   3 */
		"IPA Extensions",								/*   4 */
		"Spacing Modifier Letters",						/*   5 */
		"Combining Diacritical Marks",					/*   6 */
		"Basic Greek",									/*   7 */
		"Greek Symbols and Coptic",						/*   8 */
		"Cyrillic",										/*   9 */
		"Armenian",										/*  10 */

		"Basic Hebrew",									/*  11 */
		"Hebrew Extended (A and B blocks combined)",	/*  12 */
		"Basic Arabic",									/*  13 */
		"Arabic Extended",								/*  14 */
		"Devanagari",									/*  15 */
		"Bengali",										/*  16 */
		"Gurmukhi",										/*  17 */
		"Gujarati",										/*  18 */
		"Oriya",										/*  19 */

		"Tamil",										/*  20 */
		"Telugu",										/*  21 */
		"Kannada",										/*  22 */
		"Malayalam",									/*  23 */
		"Thai",											/*  24 */
		"Lao",											/*  25 */
		"Basic Georgian",								/*  26 */
		"Georgian Extended",							/*  27 */
		"Hangul Jamo",									/*  28 */
		"Latin Extend Additional",						/*  29 */

		"Greek Extended",								/*  30 */
		"General Punctuation",							/*  31 */
		"Superscripts and Subscripts",					/*  32 */
		"Currency Symbols",								/*  33 */
		"Combining Diacritical Marks for Symbols",		/*  34 */
		"Letterlike Symbols",							/*  35 */
		"Number Forms",									/*  36 */
		"Arrows",										/*  37 */
		"Mathematical Operators",						/*  38 */
		"Miscellaneous technical",						/*  39 */

		"Control Pictures",								/*  40 */
		"Optical Character Recognition",				/*  41 */
		"Enclosed Alphanumerics",						/*  42 */
		"Box Drawing",									/*  43 */
		"Block Elements",								/*  44 */
		"Geometric Shapes",								/*  45 */
		"Miscellaneous Symbols",						/*  46 */
		"Dingbats",										/*  47 */
		"CJK Symbols and Punctuation",					/*  48 */
		"Hiragana",										/*  49 */

		"Katakana",										/*  50 */
		"Bopomofo",										/*  51 */
		"Hangul Compatibility Jamo",					/*  52 */
		"CJK Miscellaneous",							/*  53 */
		"Enclosed CJK Letters and Months",				/*  54 */
		"CJK Compatibility",							/*  55 */
		"Hangul",										/*  56 */
		"Surrogates",									/*  57 */
		"Reserved 58",									/*  58 */
		"CJK Unified Ideographs",						/*  59 */

		"Private Use Area",								/*  60 */
		"CJK Compatibility Ideographs",					/*  61 */
		"Alphabetic Presentation Forms",				/*  62 */
		"Arabic Presentation Forms-A",					/*  63 */
		"Combining Half Marks",							/*  64 */
		"CJK Compatibility Forms",						/*  65 */
		"Small Form Variants",							/*  66 */
		"Arabic Presentation Forms-B",					/*  67 */
		"Halfwidth and Fullwidth Forms",				/*  68 */
		"Specials",										/*  69 */

		"Tibetan",										/*  70 */
		"Syriac",										/*  71 */
		"Thaana",										/*  72 */
		"Sinhala",										/*  73 */
		"Myanmar",										/*  74 */
		"Ethiopic",										/*  75 */
		"Cherokee",										/*  76 */
		"Unified Canadian Syllabics",					/*  77 */
		"Ogham",										/*  78 */
		"Runic",										/*  79 */

		"Khmer",											/*  80 */
		"Mongolian",									/*  81 */
		"Braille",										/*  82 */
		"Yi",											/*  83 */
		"Reserved 84",									/*  84 */
		"Old Italic",									/*  85 */
		"Gothic",									/*  86 */
		"Deseret",									/*  87 */
		"Byzantine Musical Symbols and Musical Symbols",									/*  88 */
		"Mathematical Alphanumeric Symbols",									/*  89 */

		"Supplementary Private Use Area-A",			 						/*  90 */
		"Supplementary Private Use Area-B",									/*  91 */
		"Reserved 92",									/*  92 */
		"Reserved 93",									/*  93 */
		"Reserved 94",									/*  94 */
		"Reserved 95",									/*  95 */
		"Reserved 96",									/*  96 */
		"Reserved 97",									/*  97 */
		"Reserved 98",									/*  98 */
		"Reserved 99",									/*  99 */

		"Reserved 100",									/* 100 */
		"Reserved 101",									/* 101 */
		"Reserved 102",									/* 102 */
		"Reserved 103",									/* 103 */
		"Reserved 104",									/* 104 */
		"Reserved 105",									/* 105 */
		"Reserved 106",									/* 106 */
		"Reserved 107",									/* 107 */
		"Reserved 108",									/* 108 */
		"Reserved 109",									/* 109 */

		"Reserved 110"									/* 110 */
		"Reserved 111",									/* 111 */
		"Reserved 112",									/* 112 */
		"Reserved 113",									/* 113 */
		"Reserved 114",									/* 114 */
		"Reserved 115",									/* 115 */
		"Reserved 116",									/* 116 */
		"Reserved 117",									/* 117 */
		"Reserved 118",									/* 118 */
		"Reserved 119",									/* 119 */

		"Reserved 120"									/* 120 */
		"Reserved 121",									/* 121 */
		"Reserved 122",									/* 122 */
		"Reserved 123",									/* 123 */
		"Reserved 124",									/* 124 */
		"Reserved 125",									/* 125 */
		"Reserved 126",									/* 126 */
		"Reserved 127",									/* 127 */
		};
	if (level < 3)
		{
		DLX(2, "unicodeRange1     =", OS_2->charRange[0]);
		DLX(2, "unicodeRange2     =", OS_2->charRange[1]);
		DLX(2, "unicodeRange3     =", OS_2->charRange[2]);
		DLX(2, "unicodeRange4     =", OS_2->charRange[3]);
		}
	else if (level < 5)
		{
		dumpRangeDesc("unicodeRange1", OS_2->charRange[0], &rangeDesc[0]);
		dumpRangeDesc("unicodeRange2", OS_2->charRange[1], &rangeDesc[32]);
		dumpRangeDesc("unicodeRange3", OS_2->charRange[2], &rangeDesc[64]);
		dumpRangeDesc("unicodeRange4", OS_2->charRange[3], &rangeDesc[96]);
		}
	}

void dumpCodePageRanges(IntX level)
	{
	static const Byte8 *rangeDesc[] =
		{
		"Latin 1 (1252)",								/*   0 */
		"Latin 2: Eastern Europe (1250)",				/*   1 */
		"Cyrillic (1251)",								/*   2 */
		"Greek (1253)",									/*   3 */
		"Turkish (1254)",								/*   4 */
		"Hebrew (1255)",								/*   5 */
		"Arabic (1256)",								/*   6 */
		"Windows Baltic (1257)",						/*   7 */
		"Vietnamese",									/*   8 */
		"Reserved 9",									/*   9 */
		"Reserved 10",									/*  10 */

		"Reserved 11",									/*  11 */
		"Reserved 12",									/*  12 */
		"Reserved 13",									/*  13 */
		"Reserved 14",									/*  14 */
		"Reserved 15",									/*  15 */
		"Thai (874)",									/*  16 */
		"JIS/Japan (932)",								/*  17 */
		"Chinese: Simplified (936)",					/*  18 */
		"Korean Wansung (949)",							/*  19 */

		"Chinese: Traditional (950)",					/*  20 */
		"Korean Johab (1361)",							/*  21 */
		"Reserved 22",									/*  22 */
		"Reserved 23",									/*  23 */
		"Reserved 24",									/*  24 */
		"Reserved 25",									/*  25 */
		"Reserved 26",									/*  26 */
		"Reserved 27",									/*  27 */
		"Reserved 29",									/*  28 */
		"Macintosh Character Set (US Roman)",			/*  29 */

		"OEM Character Set",							/*  30 */
		"Symbol Character Set",							/*  31 */
		"Reserved OEM 32",								/*  32 */
		"Reserved OEM 33",								/*  33 */
		"Reserved OEM 34",								/*  34 */
		"Reserved OEM 35",								/*  35 */
		"Reserved OEM 36",								/*  36 */
		"Reserved OEM 37",								/*  37 */
		"Reserved OEM 38",								/*  38 */
		"Reserved OEM 39",								/*  39 */

		"Reserved OEM 40",								/*  40 */
		"Reserved OEM 41",								/*  41 */
		"Reserved OEM 42",								/*  42 */
		"Reserved OEM 43",								/*  43 */
		"Reserved OEM 44",								/*  44 */
		"Reserved OEM 45",								/*  45 */
		"Reserved OEM 46",								/*  46 */
		"Reserved OEM 47",								/*  47 */
		"IBM Greek (869)",								/*  48 */
		"MS-DOS Russian (866)",							/*  49 */

		"MS-DOS Nordic (865)",							/*  50 */
		"Arabic (864)",									/*  51 */
		"MS-DOS Canadian French (863)",					/*  52 */
		"Hebrew (862)",									/*  53 */
		"MS-DOS Icelandic (861)",						/*  54 */
		"MS-DOS Portuguese (860)",						/*  55 */
		"IBM Turkish (857)",							/*  56 */
		"IBM Cyrillic; primarily Russian (855)",		/*  57 */
		"Latin 2 (852)",								/*  58 */
		"MS-DOS Baltic (775)",							/*  59 */

		"Greek; former 437 G (737)",					/*  60 */
		"Arabic; ASMO 708 (708)",						/*  61 */
		"WE/Latin 1 (850)",								/*  62 */
		"US (437)",										/*  63 */
		};
	if (level < 3)
		{
		DLX(2, "codePageRange1    =", OS_2->CodePageRange[0]);
		DLX(2, "codePageRange2    =", OS_2->CodePageRange[1]);
		}
	else if (level < 5)
		{
		dumpRangeDesc("codePageRange1", OS_2->CodePageRange[0], &rangeDesc[0]);
		dumpRangeDesc("codePageRange2", OS_2->CodePageRange[1], &rangeDesc[32]);
		}
	}

void dumpSelection(IntX level)
	{
	static const Byte8 *styleDesc[] =
		{
		"ITALIC",		/* 0 */
		"UNDERSCORE",	/* 1 */
		"NEGATIVE",		/* 2 */
		"OUTLINED",		/* 3 */
		"STRIKEOUT",	/* 4 */
		"BOLD",			/* 5 */
		"REGULAR",		/* 6 */
		};

	if (level < 3)
		DLx(2, "selection         =", OS_2->selection);
	else if (level < 5)
		{
		IntX j;
		IntX set = 0;
		IntX mask = 1;

		fprintf(OUTPUTBUFF,  "selection         =%04hx", OS_2->selection);
		for (j = 0; j < 7; j++)
			{
			if (OS_2->selection & mask)
				{
				if (set)
					fprintf(OUTPUTBUFF,  "\n                               %s", 
						   styleDesc[j]);
				else
					{
					fprintf(OUTPUTBUFF,  "       (%s", styleDesc[j]);
					set = 1;
					}
				}
			mask <<= 1;
			}
		if (set)
			fprintf(OUTPUTBUFF,  ")\n");
		else
			fprintf(OUTPUTBUFF,  "\n");
		}
	}

void OS_2Dump(IntX level, LongN start)
	{
	DL(1, (OUTPUTBUFF, "### [OS/2] (%08lx)\n", start));

	DLu(2, "version           =", OS_2->version);
	DLs(2, "averageWidth      =", OS_2->averageWidth);
	dumpWeightClass(level);
	dumpWidthClass(level);
	DLx(2, "type              =", OS_2->type);
	DLs(2, "subscriptXSize    =", OS_2->subscriptXSize);
	DLs(2, "subscriptYSize    =", OS_2->subscriptYSize);
	DLs(2, "subscriptXOffset  =", OS_2->subscriptXOffset);
	DLs(2, "subscriptYOffset  =", OS_2->subscriptYOffset);
	DLs(2, "superscriptXSize  =", OS_2->superscriptXSize);
	DLs(2, "superscriptYSize  =", OS_2->superscriptYSize);
	DLs(2, "superscriptXOffset=", OS_2->superscriptXOffset);
	DLs(2, "superscriptYOffset=", OS_2->superscriptYOffset);
	DLs(2, "strikeoutSize     =", OS_2->strikeoutSize);
	DLs(2, "strikeoutPosition =", OS_2->strikeoutPosition);
	dumpFamilyClass(level);
	dumpPanose(level);
	dumpUnicodeRanges(level);
	DL(2, (OUTPUTBUFF, "vendor            =%.*s\n", 
		   (IntN)sizeof(OS_2->vendor), OS_2->vendor));
	dumpSelection(level);
	DL(2, (OUTPUTBUFF, "firstChar         =U+%04hX\n", OS_2->firstChar));
	DL(2, (OUTPUTBUFF, "lastChar          =U+%04hX\n", OS_2->lastChar));
	DLs(2, "TypoAscender =", OS_2->typographicAscent);
	DLs(2, "TypoDescender=", OS_2->typographicDescent);
	DLs(2, "TypoLineGap  =", OS_2->typographicLineGap);
	DLu(2, "windowsAscent     =", OS_2->windowsAscent);
	DLu(2, "windowsDescent    =", OS_2->windowsDescent);
	if (OS_2->version > 0) 
		dumpCodePageRanges(level);
    if (OS_2->version > 1)
    {
        DLs(2, "xHeight           =", OS_2->XHeight);
        DLs(2, "capHeight         =", OS_2->CapHeight);
        DL(2, (OUTPUTBUFF, "defaultChar       =U+%04hX\n", OS_2->DefaultChar));
        DL(2, (OUTPUTBUFF, "breakChar         =U+%04hX\n", OS_2->BreakChar));
        DLu(2, "maxContext        =", OS_2->maxContext);
    }
    if (OS_2->version > 4)
    {
        fprintf(OUTPUTBUFF,  "usLowerOpticalPointSize    = %hu (TWIPS), %.2f (pts)\n",
                OS_2->usLowerOpticalPointSize, OS_2->usLowerOpticalPointSize/20.0);
        fprintf(OUTPUTBUFF,  "usUpperOpticalPointSize    = %hu (TWIPS), %.2f (pts)\n",
                OS_2->usUpperOpticalPointSize, OS_2->usUpperOpticalPointSize/20.0);
    }
}
	
void OS_2GetTypocenders(Int32 *ascender, Int32*descender)
{
	if (!loaded)
		{
		  if (sfntReadTable(OS_2_))
			{	
				*ascender=0;
				*descender=0;
			}
		}
	*ascender=OS_2->typographicAscent;
	*descender=OS_2->typographicDescent;
}

void OS_2Free(void)
	{
	  if (!loaded) return;
	  memFree(OS_2); OS_2 = NULL;
	  loaded = 0;
	}
