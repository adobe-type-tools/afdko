/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Abstract font dump.
 */

#include "absfont.h"
#include "dictops.h"
#include "txops.h"
#include "ctutil.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>
#include <float.h>

#define ARRAY_LEN(a) 	(sizeof(a)/sizeof((a)[0]))
#define DUMP_LANG_GROUP	(1<<0)
#define RND(v)		((float)floor((v) + 0.5))

/* Dump string. */
static void dumpString(abfDumpCtx h, char *name, char *ptr)
	{
    if (ptr == ABF_UNSET_PTR)
		return; 

	fprintf(h->fp, "%-20s", name);

	/* Dump string with carriage return and control character handling */
	fputc('"', h->fp);
	for (;;)
		{
		int c = *ptr++;
		switch (c)
			{
		case '\0':
			fprintf(h->fp, "\"\n");
			return;
		case '\r':
			fputc('\n', h->fp);
			if (*ptr == '\n')
				ptr++;
			break;
		case '\n':
			fputc('\n', h->fp);
			break;
		default:
			if (iscntrl(c))
				fprintf(h->fp, "\\x%02X", c&0xff);
			else
				fputc(c, h->fp);
			break;
			}
		}
	}

/* Dump integer array. */
static void dumpIntArray(abfDumpCtx h, char *name, long cnt, long *array)
	{
	int i;
	char *sep;

	if (cnt == ABF_EMPTY_ARRAY)
		return;

	fprintf(h->fp, "%-20s", name);
	sep = "{";
	for (i = 0; i < cnt; i++)
		{
		fprintf(h->fp, "%s%ld", sep, array[i]);
		sep = ",";
		}
	fprintf(h->fp, "}\n");
	}

/* Dump real array. */
static void dumpRealArray(abfDumpCtx h, char *name, long cnt, float *array)
	{
	int i;
	char *sep;

	if (cnt == ABF_EMPTY_ARRAY)
		return;

	fprintf(h->fp, "%-20s", name);
	sep = "{";
	for (i = 0; i < cnt; i++)
		{
		fprintf(h->fp, "%s%g", sep, array[i]);
		sep = ",";
		}
	fprintf(h->fp, "}\n");
	}

/* Dump FontMatrix. */
static void dumpFontMatrix(abfDumpCtx h, char *name, abfFontMatrix *FontMatrix)
	{
	if (!abfIsDefaultFontMatrix(FontMatrix))
		{
		/* Print values with higher precision than dumpRealArray() provides */
		int i;
		char *sep;
		fprintf(h->fp, "%-20s", name);
		sep = "{";
		for (i = 0; i < FontMatrix->cnt; i++)
			{
			fprintf(h->fp, "%s%.8g", sep, FontMatrix->array[i]);
			sep = ",";
			}
		fprintf(h->fp, "}\n");
		}
	}

/* Dump top dictionary. */
static void dumpTopDict(abfDumpCtx h, abfTopDict *top)
	{
	char *OrigFontType;
	char *srcFontType;

	dumpString(h, "version", 		top->version.ptr);
	dumpString(h, "Notice", 		top->Notice.ptr);
	dumpString(h, "Copyright",		top->Copyright.ptr);
	dumpString(h, "FullName",		top->FullName.ptr);
	dumpString(h, "FamilyName", 	top->FamilyName.ptr);
	dumpString(h, "Weight", 		top->Weight.ptr);
    if (top->isFixedPitch           != cff_DFLT_isFixedPitch)
        fprintf(h->fp, "isFixedPitch        true\n");
    if (top->ItalicAngle            != cff_DFLT_ItalicAngle)
        fprintf(h->fp, "ItalicAngle         %g\n", top->ItalicAngle);
    if (top->UnderlinePosition      != cff_DFLT_UnderlinePosition)
        fprintf(h->fp, "UnderlinePosition   %g\n", top->UnderlinePosition);
    if (top->UnderlineThickness     != cff_DFLT_UnderlineThickness)
        fprintf(h->fp, "UnderlineThickness  %g\n", top->UnderlineThickness);
    if (top->UniqueID               != ABF_UNSET_INT)
        fprintf(h->fp, "UniqueID            %ld\n", top->UniqueID);
    if (top->FontBBox[0] != 0.0 || top->FontBBox[1] != 0.0 || 
        top->FontBBox[2] != 0.0 || top->FontBBox[3] != 0.0)
        fprintf(h->fp, "FontBBox            {%g,%g,%g,%g}\n", 
               top->FontBBox[0], top->FontBBox[1], 
               top->FontBBox[2], top->FontBBox[3]);
    if (top->StrokeWidth            != cff_DFLT_StrokeWidth)
        fprintf(h->fp, "StrokeWidth         %g\n", top->StrokeWidth);
    dumpIntArray(h, "XUID", top->XUID.cnt, top->XUID.array);

    /* CFF and Acrobat extensions */
	dumpString(h, "PostScript", 	top->PostScript.ptr);
	dumpString(h, "BaseFontName",	top->BaseFontName.ptr);
    dumpIntArray(h, "BaseFontBlend", 
                 top->BaseFontBlend.cnt, top->BaseFontBlend.array);
    if (top->FSType					!= ABF_UNSET_INT)
        fprintf(h->fp, "FSType              %ld\n", top->FSType);
	switch (top->OrigFontType)
		{
	case abfOrigFontTypeType1:
		OrigFontType = "Type1";
		break;
	case abfOrigFontTypeCID:
		OrigFontType = "CID";
		break;
	case abfOrigFontTypeTrueType:
		OrigFontType = "TrueType";
		break;
	case abfOrigFontTypeOCF:
		OrigFontType = "OCF";
		break;
	default:
		OrigFontType = NULL;
		break;
		}
	if (OrigFontType != NULL)
		fprintf(h->fp, "OrigFontType        %s\n", OrigFontType);
	if (top->WasEmbedded != 0)
		fprintf(h->fp, "WasEmbedded         true\n");

	/* Synthetic font */
	dumpString(h, "SynBaseFontName",top->SynBaseFontName.ptr);

    /* CID-keyed font extensions */
    dumpFontMatrix(h, "cid.FontMatrix",&top->cid.FontMatrix);
	dumpString(h, "cid.CIDFontName",top->cid.CIDFontName.ptr);
	dumpString(h, "cid.Registry", 	top->cid.Registry.ptr);
	dumpString(h, "cid.Ordering", 	top->cid.Ordering.ptr);
    if (top->cid.Supplement         != ABF_UNSET_INT)
        fprintf(h->fp, "cid.Supplement      %ld\n", top->cid.Supplement);
    if (top->cid.CIDFontVersion     != cff_DFLT_CIDFontVersion)
        fprintf(h->fp, "cid.CIDFontVersion  %.03f\n", top->cid.CIDFontVersion);
    if (top->cid.CIDFontRevision    != cff_DFLT_CIDFontRevision)
        fprintf(h->fp, "cid.CIDFontRevision %ld\n", top->cid.CIDFontRevision);
    if (top->cid.CIDCount           != cff_DFLT_CIDCount)
        fprintf(h->fp, "cid.CIDCount        %ld\n", top->cid.CIDCount);
    if (top->cid.UIDBase            != ABF_UNSET_INT)
        fprintf(h->fp, "cid.UIDBase         %ld\n", top->cid.UIDBase);

    /* Supplementary data */
    if (top->sup.flags              != 0)
		{
		char *sep = " (";
        fprintf(h->fp, "sup.flags           0x%08lx", top->sup.flags);
		if (top->sup.flags & ABF_SYN_FONT)
			{
			fprintf(h->fp, "%sABF_SYN_FONT", sep);
			sep = ",";
			}
		if (top->sup.flags & ABF_CID_FONT)
			{
			fprintf(h->fp, "%sABF_CID_FONT", sep);
			sep = ",";
			}
		if (sep[0] == ',')
			fprintf(h->fp, ")");
		fprintf(h->fp, "\n");
		}
	switch (top->sup.srcFontType)
		{
	case abfSrcFontTypeType1Name:
		srcFontType = "Type 1 (name-keyed)";
		break;
	case abfSrcFontTypeType1CID:
		srcFontType = "Type 1 (cid-keyed)";
		break;
    case abfSrcFontTypeCFFName:
        srcFontType = "CFF (name-keyed)";
        break;
    case abfSrcFontTypeCFFCID:
        srcFontType = "CFF (cid-keyed)";
        break;
    case abfSrcFontTypeSVGName:
        srcFontType = "SVG (name-keyed)";
        break;
    case abfSrcFontTypeUFOName:
        srcFontType = "UFO (name-keyed)";
        break;
	case abfSrcFontTypeTrueType:
		srcFontType = "TrueType";
		break;
	default:
		srcFontType = NULL;
		break;
		}
	if (srcFontType != NULL)
		fprintf(h->fp, "sup.srcFontType     %s\n", srcFontType);
    if (top->sup.UnitsPerEm         != 1000)
        fprintf(h->fp, "sup.UnitsPerEm      %ld\n", top->sup.UnitsPerEm);
    if (top->sup.nGlyphs            != ABF_UNSET_INT)
        fprintf(h->fp, "sup.nGlyphs         %ld\n", top->sup.nGlyphs);
    }

/* Dump Private dictionary. */
static void dumpPrivateDict(abfDumpCtx h, abfPrivateDict *private)
    {
    dumpRealArray(h, "BlueValues", 
                  private->BlueValues.cnt, private->BlueValues.array);
    dumpRealArray(h, "OtherBlues", 
                  private->OtherBlues.cnt, private->OtherBlues.array);
    dumpRealArray(h, "FamilyBlues", 
                  private->FamilyBlues.cnt, private->FamilyBlues.array);
    dumpRealArray(h, "FamilyOtherBlues", 
                  private->FamilyOtherBlues.cnt, 
                  private->FamilyOtherBlues.array);
    if (private->BlueScale          != (float)cff_DFLT_BlueScale)
        fprintf(h->fp, "BlueScale           %g\n", private->BlueScale);
    if (private->BlueShift          != cff_DFLT_BlueShift)
        fprintf(h->fp, "BlueShift           %g\n", private->BlueShift);
    if (private->BlueFuzz           != cff_DFLT_BlueFuzz)
        fprintf(h->fp, "BlueFuzz            %g\n", private->BlueFuzz);
    if (private->StdHW              != ABF_UNSET_REAL)
        fprintf(h->fp, "StdHW               %g\n", private->StdHW);
    if (private->StdVW              != ABF_UNSET_REAL)
        fprintf(h->fp, "StdVW               %g\n", private->StdVW);
    dumpRealArray(h, "StemSnapH", 
                  private->StemSnapH.cnt, private->StemSnapH.array);
    dumpRealArray(h, "StemSnapV", 
                  private->StemSnapV.cnt, private->StemSnapV.array);
    if (private->ForceBold          != cff_DFLT_ForceBold)
        fprintf(h->fp, "ForceBold           true\n");
    if (private->LanguageGroup      != cff_DFLT_LanguageGroup)
        fprintf(h->fp, "LanguageGroup       %ld\n", private->LanguageGroup);
    if (private->ExpansionFactor    != (float)cff_DFLT_ExpansionFactor)
        fprintf(h->fp, "ExpansionFactor     %g\n", private->ExpansionFactor);
    if (private->initialRandomSeed  != cff_DFLT_initialRandomSeed)
        fprintf(h->fp, "initialRandomSeed   %g\n", private->initialRandomSeed);
    }

/* Dump font dictionary. */
static void dumpFontDict(abfDumpCtx h, abfFontDict *font)
    {
	dumpString(h, "FontName",			font->FontName.ptr);
    if (font->PaintType             != cff_DFLT_PaintType)
        fprintf(h->fp, "PaintType           %ld\n", font->PaintType);
    dumpFontMatrix(h, "FontMatrix", &font->FontMatrix);
    fprintf(h->fp, "## Private\n");
    dumpPrivateDict(h, &font->Private);
    }

/* Begin font dump by dumping dicts. */
void abfDumpBegFont(abfDumpCtx h, abfTopDict *top)
	{
	long i;

	if (h->level < 0 || h->level > 6)
		h->level = 1;	/* Set default for invalid levels */
        
    if (h->level <= 3)
    {
        long i,j;
        
        fprintf(h->fp, "## Filename %s\n",
                (top->sup.filename == ABF_UNSET_PTR)?
                "<unknown>": top->sup.filename);
        
        fprintf(h->fp, "## Top Dict\n");
        dumpTopDict(h, top);
        for (i = 0; i < top->FDArray.cnt; i++)
        {
            if (h->fdCnt == 0)
            {
                fprintf(h->fp, "## FontDict[%ld]\n", i);
                dumpFontDict(h, &top->FDArray.array[i]);
            }
            else
            {
                
                if (h->excludeSubset)
                {
                    unsigned int match = 0;
                    for (j = 0; j < h->fdCnt; j++)
                    {
                        if (i == h->fdArray[j])
                        {
                            match = 1;
                            break;
                        }
                    }
                    if (!match)
                    {
                        fprintf(h->fp, "## FontDict[%ld]\n", i);
                        dumpFontDict(h, &top->FDArray.array[i]);
                    }
                    
                }
                else
                {
                    for (j = 0; j < h->fdCnt; j++)
                    {
                        if (i == h->fdArray[j])
                        {
                            fprintf(h->fp, "## FontDict[%ld]\n", i);
                            dumpFontDict(h, &top->FDArray.array[i]);
                           break;
                        }
                    }
                }
               
                
                
            }
            
        }
    }
		
	if (h->level == 0)
		return;

	h->flags = 0;
	for (i = 0; i < top->FDArray.cnt; i++)
		if (top->FDArray.array[i].Private.LanguageGroup == 1)
			{
			h->flags |= DUMP_LANG_GROUP;
			break;
			}
	h->left = 0;

	/* Print glyph comment */
	if (top->sup.flags & ABF_CID_FONT)
		fprintf(h->fp, "## glyph[tag] {cid,iFD");
	else
		fprintf(h->fp, "## glyph[tag] {name,encoding");
	if (h->flags & DUMP_LANG_GROUP)
		fprintf(h->fp, ",LanguageGroup");
	if (h->level == 1 || h->level == 4)
		fprintf(h->fp, "}\n");
	else
		fprintf(h->fp, ",path}\n");
	}

/* ------------------------------- Glyph Dump ------------------------------ */

/* Dump glyph beginning. */
static int glyphBeg(abfGlyphCallbacks *cb, abfGlyphInfo *info)
	{
	abfDumpCtx h = cb->direct_ctx;

	cb->info = info;
	if (h->level == 0)
		return ABF_SKIP_RET;

	fprintf(h->fp, "glyph[%hu] {", info->tag);
	if (info->flags & ABF_GLYPH_CID)
		/* Dump CID-keyed glyph */
		fprintf(h->fp, "%hu,%u", info->cid, info->iFD);
	else
		{
		/* Dump name-keyed glyph */
		abfEncoding *enc = &info->encoding;
		fprintf(h->fp, "%s", info->gname.ptr);
		if (enc->code == ABF_GLYPH_UNENC)
			fprintf(h->fp, ",-");
		else
			{
			/* Dump encoding */
			char *sep = ",";
			do
				{
				if (info->flags & ABF_GLYPH_UNICODE)
					{
					if (enc->code < 0x10000)
						fprintf(h->fp, "%sU+%04lX", sep, enc->code);
					else
						fprintf(h->fp, "%sU+%lX", sep, enc->code);
					}
				else
					fprintf(h->fp, "%s0x%02lX", sep, enc->code);
				sep = "+";
				enc = enc->next;
				}
			while (enc != NULL);
			}
		}
	if (h->flags & DUMP_LANG_GROUP)
		fprintf(h->fp, ",%d", (info->flags & ABF_GLYPH_LANG_1) != 0);
	switch (h->level)
		{
	case 1: case 4:
		fprintf(h->fp, "}\n");
		return ABF_SKIP_RET;
	case 2: case 5:
		fprintf(h->fp, ",");
		break;
	case 3: case 6:
		fprintf(h->fp, ",\n");
		break;
		}
	h->left = 0;

	return ABF_CONT_RET;
	}

/* Dump instruction according to dump level. */
static void CTL_CDECL dumpInstr(abfGlyphCallbacks *cb, char *fmt, ...)
	{
	abfDumpCtx h = cb->direct_ctx;
	va_list ap;
	va_start(ap, fmt);
	switch (h->level)
		{
	case 2: case 5:
		{
		int length;
		char buf[128];
		vsprintf(buf, fmt, ap);
		length = strlen(buf);
		if (length > h->left)
			{
			char *p;
			/* Find break point */
			for (p = &buf[h->left]; *p != ' '; p--)
				;
			fprintf(h->fp, "%.*s\n", (int)(p - buf), buf);
			fprintf(h->fp, "%s", p);
			h->left = 79 - (strlen(p) + 1);
			}
		else
			{
			fprintf(h->fp, "%s", buf);
			h->left -= length;
			}
		}
		break;
	case 3: case 6:
		fprintf(h->fp, " ");
		vfprintf(h->fp, fmt, ap);
		fprintf(h->fp, "\n");
		break;
	default:
		break;
		}
	va_end(ap);
	}

/* Dump glyph width. */
static void glyphWidth(abfGlyphCallbacks *cb, float hAdv)
	{
	dumpInstr(cb, " %g width", roundf(hAdv));
	}

/* Write real number in ASCII to dst stream. */
#define TX_EPSILON 0.0003
/*In Xcode, FLT_EPSILON is 1.192..x10-7, but the diff between value-roundf(value) can be 3.05x10-5, when the input value is an integer. */
static void writeReal(char* buf, float value)
{
	char tmp[50];
    int l;
    
	/* if no decimal component, perform a faster to string conversion */
	if ((fabs(value-roundf(value)) < TX_EPSILON) && (value>LONG_MIN) && (value<LONG_MAX))
        sprintf(tmp, " %ld", (long int)roundf(value));
	else
	{
        float value2 = RND_ON_WRITE(value); // to avoid getting -0 from 0.0004.
        if ((value2 == 0) && (value < 0))
            value2 = 0;
        sprintf(tmp, " %.2f", value2);
        l = strlen(tmp)-1;
        if ((tmp[l] == '0') && (tmp[l-1] == '0'))
        {
            tmp[l-2] = 0;
        }
     }
    strcat(buf, tmp);
}

/* Dump glyph move. */
static void glyphMove(abfGlyphCallbacks *cb, float x0, float y0)
	{
    char buf[128];
    buf[0] = 0;
    writeReal(buf, x0);
    writeReal(buf, y0);
    strcat(buf, " move");
    dumpInstr(cb, "%s", buf);
	}

/* Dump glyph line. */
static void glyphLine(abfGlyphCallbacks *cb, float x1, float y1)
	{
    char buf[128];
    buf[0] = 0;
    writeReal(buf, x1);
    writeReal(buf, y1);
    strcat(buf, " line");
    dumpInstr(cb, "%s", buf);
	}

/* Dump glyph curve. */
static void glyphCurve(abfGlyphCallbacks *cb,
						   float x1, float y1, 
						   float x2, float y2, 
						   float x3, float y3)
	{
    char buf[128];
    buf[0] = 0;
    writeReal(buf, x1);
    writeReal(buf, y1);
    writeReal(buf, x2);
    writeReal(buf, y2);
    writeReal(buf, x3);
    writeReal(buf, y3);
    strcat(buf, " curve");
    dumpInstr(cb, "%s", buf);
}

/* Dump glyph stem. */
static void glyphStem(abfGlyphCallbacks *cb, 
					  int flags, float edge0, float edge1)
	{
    char buf[128];
    buf[0] = 0;
        
	if (flags & ABF_NEW_HINTS)
		dumpInstr(cb, " newhints");
	if (flags & ABF_NEW_GROUP)
		dumpInstr(cb, " newgroup");
	if (flags & ABF_CNTR_STEM)
	{
        writeReal(buf, edge0);
        writeReal(buf, edge1);
        strcat(buf, (flags & ABF_VERT_STEM)? " vcntr": " hcntr");
       // dumpInstr(cb, " %.3f %.3f %s",
		//		  edge0, edge1, (flags & ABF_VERT_STEM)? "vcntr": "hcntr");
    }
	else if (flags & ABF_STEM3_STEM)
	{
        writeReal(buf, edge0);
        writeReal(buf, edge1);
        strcat(buf, (flags & ABF_VERT_STEM)? " vstem3": " hstem3");
		//dumpInstr(cb, " %.3f %.3f %s",
		//		  edge0, edge1, (flags & ABF_VERT_STEM)? "vstem3": "hstem3");
    }
	else
		{
		float width = RND(edge1 - edge0);
		if (width == ABF_EDGE_HINT_LO)
		{
            writeReal(buf, edge1);
            strcat(buf, (flags & ABF_VERT_STEM)? " leftedge": " bottomedge");
            //dumpInstr(cb, " %.3f %s", edge1,
			//		  (flags & ABF_VERT_STEM)? "leftedge": "bottomedge");
		}
        else if (width == ABF_EDGE_HINT_HI)
		{
            writeReal(buf, edge0);
            strcat(buf, (flags & ABF_VERT_STEM)? " rightedge": " topedge");
            // dumpInstr(cb, " %.3f %s", edge0,
			//		  (flags & ABF_VERT_STEM)? "rightedge": "topedge");
        }
		else
		{
            writeReal(buf, edge0);
            writeReal(buf, edge1);
            strcat(buf, (flags & ABF_VERT_STEM)? " vstem": " hstem");
            //dumpInstr(cb, " %.3f %.3f %s", edge0, edge1,
			//		  (flags & ABF_VERT_STEM)? "vstem": "hstem");

        }		}
    dumpInstr(cb, "%s", buf);
        
	}

/* Dump glyph flex. */
static void glyphFlex(abfGlyphCallbacks *cb, float depth,
					  float x1, float y1, 
					  float x2, float y2, 
					  float x3, float y3,
					  float x4, float y4, 
					  float x5, float y5, 
					  float x6, float y6)
	{
        char buf[128];
        buf[0] = 0;

        
        writeReal(buf, x1);
        writeReal(buf, y1);
        writeReal(buf, x2);
        writeReal(buf, y2);
        writeReal(buf, x3);
        writeReal(buf, y3);
        dumpInstr(cb, "%s", buf);
        buf[0] = 0;
        writeReal(buf, x4);
        writeReal(buf, y4);
        writeReal(buf, x5);
        writeReal(buf, y5);
        writeReal(buf, x6);
        writeReal(buf, y6);
        writeReal(buf, depth);
        strcat(buf, " flex");
        dumpInstr(cb, "%s", buf);

        //dumpInstr(cb, " %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f flex",
		//   x1, y1, x2, y2, x3, y3, x4, y4, x5, y5, x6, y6, depth);
	}

/* Dump glyph general operator. */
static void glyphGenop(abfGlyphCallbacks *cb, 
					   int cnt, float *args, int op)
	{
	static char *opnames[] =
		{
		"reserved0",
		"hstem",
		"compose",
		"vstem",
		"vmoveto",
		"rlineto",
		"hlineto",
		"vlineto",
		"rrcurveto",
		"reserved9",
		"callsubr",
		"return",
		"escape",
		"reserved13",
		"endchar",
		"reserved15",
		"reserved16",
		"callgrel",
		"hstemhm",
		"hintmask",
		"cntrmask",
		"rmoveto",
		"hmoveto",
		"vstemhm",
		"rcurveline",
		"rlinecurve",
		"vvcurveto",
		"hhcurveto",
		"shortint",
		"callgsubr",
		"vhcurveto",
		"hvcurveto",
		}; 
	static char *escopnames[] =
		{
		"dotsection",
		"reservedESC1",
		"reservedESC2",
		"and",
		"or",
		"not",
		"reservedESC6",
		"reservedESC7",
		"reservedESC8",
		"abs",
		"add",
		"sub",
		"div",
		"reservedESC13",
		"neg",
		"eq",
		"reservedESC16",
		"reservedESC17",
		"drop",
		"reservedESC19",
		"put",
		"get",
		"ifelse",
		"random",
		"mul",
		"reservedESC25",
		"sqrt",
		"dup",
		"exch",
		"index",
		"roll",
		"reservedESC31",
		"reservedESC32",
		"cntroff",	/* Reserved but used as cntroff by implementation */
		"hflex",
		"flex",
		"hflex1",
		"flex1",
		"cntron",
		};
	int hi = op>>8 & 0xff;
	int lo = op & 0xff;
	char buf[80];
	char *p = buf;

	/* Format args */
	while (cnt--)
		{
		sprintf(p, " %g", *args++);
		p += strlen(p);
		}

	/* Format operator */
	if (hi)
		{
		if (hi != tx_escape)
			sprintf(p, " invalid");
		else if (lo >= (int)ARRAY_LEN(escopnames))
			sprintf(p, " reservedESC%d", lo);
		else
			sprintf(p, " %s", escopnames[lo]);
		}
	else if (lo >= (int)ARRAY_LEN(opnames))
		sprintf(p, " reserved%d", lo);
	else 
		sprintf(p, " %s", opnames[lo]);

	dumpInstr(cb, buf);
	}

/* Dump glyph seac. */
static void glyphSeac(abfGlyphCallbacks *cb, 
						  float adx, float ady, int bchar, int achar)
	{
	dumpInstr(cb, " %g %g %d %d seac", adx, ady, bchar, achar);
	}

/* Dump glyph end. */
static void glyphEnd(abfGlyphCallbacks *cb)
	{
	abfDumpCtx h = cb->direct_ctx;

	dumpInstr(cb, " endchar}");
	switch (h->level)
		{
	case 2: case 5:
		fprintf(h->fp, "\n");
		break;
		}
	}

static void glyphCompose(abfGlyphCallbacks *cb,  int cubeLEIndex, float x0, float y0, int numDV, float* ndv)
	{
	int i = 0;

	char buf[250];
	char *p = buf;

	sprintf(p, " %d %f %f", cubeLEIndex, x0, y0);
	p += strlen(p);
	while (i < numDV)
		{
		sprintf(p, " %g", ndv[i++]);
		p += strlen(p);
		}
	dumpInstr(cb, "%s compose", buf);
	}


static void glyphTransform(abfGlyphCallbacks *cb,  float rotate, float scaleX, float scaleY, float skewX, float skewY)
	{
	char buf[250];
	char *p = buf;

	sprintf(p, " %f %f %f %f %f", rotate, scaleX, scaleY, skewX, skewY);
	dumpInstr(cb, "%s transform", buf);
	}



/* Dump glyph callbacks */
const abfGlyphCallbacks abfGlyphDumpCallbacks =
	{
	NULL,
	NULL,
	NULL,
	glyphBeg,
	glyphWidth,
	glyphMove,
	glyphLine,
	glyphCurve,
	glyphStem,
	glyphFlex,
	glyphGenop,
	glyphSeac,
	glyphEnd,
	NULL,
	NULL,
	glyphCompose,
	glyphTransform,
	};

