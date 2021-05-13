/***********************************************************************/
/*                                                                     */
/* Copyright 2016 Adobe Systems Incorporated.                          */
/* All rights reserved.                                                */
/***********************************************************************/

/*
 * Feature file grammar
 */

#header <<
/* --- This section will be included in all feat*.c files created. --- */

#include <stdlib.h>	/* For exit in err.h xxx */
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <math.h>

#include "common.h"
#include "feat.h"
#include "OS_2.h"
#include "hhea.h"
#include "vmtx.h"
#include "dynarr.h"

#define MAX_TOKEN	64

extern featCtx h;	/* Not reentrant; see featNew() comments */
extern hotCtx g;

typedef union
	{
	int64_t lval;
	uint64_t ulval;
	char text[MAX_TOKEN];
	} Attrib;

void zzcr_attr(Attrib *attr, int type, char *text);

>>

<<
void featureFile(void);

#include "feat.c"

featCtx h;			/* Not reentrant; see featNew() comments */
hotCtx g;
int sawSTAT = FALSE;
int sawFeatNames = FALSE;
int sawCVParams = FALSE;
>>

/* ----------------------------- Tokens ------------------------------------ */

#lexclass TAG_MODE // --------------------------------------------------------

#token "[\ \t]+"	<<zzskip();>>
#token "\r\n"		<<zzskip(); zzline++;>>
#token "[\r\n]"		<<zzskip(); zzline++;>>

#token K_include	"include"
					<<
					zzmode(INCLUDE_FILE);
					featSetIncludeReturnMode(TAG_MODE);
					zzskip();
					>>

#token ";"         <<zzskip(); zzmode(featGetTagReturnMode());>>	// xxx

#token T_TAG	   "[\0x21-\0x3a\0x3c-\0x7E]{[\0x21-\0x3a\0x3c-\0x7E]}{[\0x21-\0x3a\0x3c-\0x7E]}{[\0x21-\0x3a\0x3c-\0x7E]}"
					<<zzmode(featGetTagReturnMode());>>

#lexclass LABEL_MODE // -------------------------------------------------------

#token "[\ \t]+"	<<zzskip();>>
#token "\r\n"		<<zzskip(); zzline++;>>
#token "[\r\n]"		<<zzskip(); zzline++;>>

#token T_LABEL		"[A-Za-z_0-9.]+"
					<<zzmode(START);>>

#lexclass INCLUDE_FILE // -----------------------------------------------------

#token "[\ \t]+"	<<zzskip();>>
#token "\r\n"		<<zzskip(); zzline++;>>
#token "[\r\n]"		<<zzskip(); zzline++;>>

#token INCLFILE		"\(~[\)]+\)[#; \t\r\n]"
					<<
					zzmode(featGetIncludeReturnMode());
					DF(2, (stderr, "# in(%s)\n", zzlextext));
				    zzskip();
				    featOpenIncludeFile(g, featTrimParensSpaces(zzlextext));
					>>

#lexclass ANON_HDR1_MODE // ---------------------------------------------------

#token "@"			<<featUnexpectedEOF();>>

#token  "[\ \t]+"	<<zzskip();>>
#token	"\r\n"		<<zzskip(); zzline++;>>
#token	"[\r\n]"	<<zzskip(); zzline++;>>
#token  "\{"		<<zzskip(); zzmode(ANON_HDR2_MODE);>>

#lexclass ANON_HDR2_MODE // ---------------------------------------------------

#token  "@"			<<featUnexpectedEOF();>>

#token  "#~[\r\n]*"	<<zzskip();>>
#token  "[\ \t]+"	<<zzskip();>>
#token	"\r\n"		<<zzskip(); zzline++; zzmode(ANON_BODY_MODE);>>
#token	"[\r\n]"	<<zzskip(); zzline++; zzmode(ANON_BODY_MODE);>>

#lexclass ANON_BODY_MODE // ---------------------------------------------------

#token "@"			<<featUnexpectedEOF();>>

#token "\r\n"		<<
					if (featAddAnonDataChar(zzlextext[0], 0) ||
						featAddAnonDataChar(zzlextext[1], 1 /*eol*/))
						zzmode(START);
					zzskip();
					zzline++;
					>>
#token "[\r\n]"		<<
					if (featAddAnonDataChar(zzlextext[0], 1 /*eol*/))
						zzmode(START);
					zzskip();
					zzline++;
					>>

#token "~[]"		<<featAddAnonDataChar(zzlextext[0], 0); zzskip();>>

#lexclass STRING_MODE // ------------------------------------------------------

#token "@"			<<featUnexpectedEOF();>>

#token "\""			<<zzmode(START); zzskip();>>

#token	"\r\n"		<<zzskip(); zzline++;>>
#token	"[\r\n]"	<<zzskip(); zzline++;>>

#token "[\0x20-\0x7E\0x80-\0xF7]"  <<featAddNameStringChar(zzlextext[0]); zzskip();>>

#lexclass START // ------------------------------------------------------------

#token "#~[\r\n]*"	<<zzskip();>>
#token  "[\ \t]+"	<<zzskip();>>
#token	"\r\n"		<<zzskip(); zzline++;>>
#token	"[\r\n]"	<<zzskip(); zzline++;>>

/* Ordering of tokens: list keywords and more specific patterns first */

#token K_include	"include"
					<<zzmode(INCLUDE_FILE);
					  featSetIncludeReturnMode(START);
					  zzskip();>>

#token K_feature		"feature"  	      <<zzmode(TAG_MODE);>>
#token K_table			"table"		      
#token K_script 		"script"   	      <<zzmode(TAG_MODE);>>
#token K_language		"language" 	      <<zzmode(TAG_MODE);>>
#token K_languagesystem "languagesystem"  <<zzmode(TAG_MODE);>>
#token K_subtable       "subtable"
#token K_lookup			"lookup"   	      <<zzmode(LABEL_MODE);>>
#token K_lookupflag		"lookupflag"

#token K_RightToLeft        "RightToLeft"
#token K_IgnoreBaseGlyphs   "IgnoreBaseGlyphs"
#token K_IgnoreLigatures    "IgnoreLigatures"
#token K_IgnoreMarks        "IgnoreMarks"
#token K_UseMarkFilteringSet   "UseMarkFilteringSet"
#token K_MarkAttachmentType "MarkAttachmentType"

#token K_anon   		"(anonymous|anon)"
						<<featSetTagReturnMode(ANON_HDR1_MODE);
						  zzmode(TAG_MODE);>>

#token K_includeDFLT	"includeDFLT"
#token K_excludeDFLT	"excludeDFLT"

#token K_include_dflt	"include_dflt"
#token K_exclude_dflt	"exclude_dflt"

#token K_useExtension	"useExtension"
#token K_BeginValue      "<"
#token K_EndValue      ">"
#token K_enumerate      "enumerate|enum"
#token K_except         "except"
#token K_ignore        	"ignore"
#token K_substitute     "substitute|sub"
#token K_reverse		"reversesub|rsub"
#token K_by             "by"
#token K_from           "from"
#token K_position       "position|pos"
#token K_parameters		"parameters"
#token K_feat_names		"featureNames"
#token K_cv_params "cvParameters"
#token K_cvUILabel	"FeatUILabelNameID"
#token K_cvToolTip	"FeatUITooltipTextNameID"
#token K_cvSampletext	"SampleTextNameID"
#token K_cvParameterLabel	"ParamUILabelNameID"
#token K_cvCharacter	"Character"
#token K_sizemenuname	"sizemenuname"
#token K_contourpoint	"contourpoint"
#token K_device			"device"
#token K_anchor			"anchor"
#token K_anchordef		"anchorDef"
#token K_valueRecordDef "valueRecordDef"
#token K_mark			"mark"
#token K_markClass		"markClass"
#token K_cursive		"cursive"
#token K_markBase		"base"
#token K_markLigature	"ligature|lig"
#token K_LigatureComponent "ligComponent"
#token K_caret			"caret"

#token K_NULL			"NULL"

#token K_BASE			"BASE"		/* Added tag to list in zzcr_attr() */
#token K_HorizAxis_BaseTagList		"HorizAxis.BaseTagList"
														<<zzmode(TAG_MODE);>>
#token K_HorizAxis_BaseScriptList	"HorizAxis.BaseScriptList"
														<<zzmode(TAG_MODE);>>
#token K_VertAxis_BaseTagList		"VertAxis.BaseTagList"
														<<zzmode(TAG_MODE);>>
#token K_VertAxis_BaseScriptList	"VertAxis.BaseScriptList"
														<<zzmode(TAG_MODE);>>

#token K_GDEF			"GDEF"		/* Added tag to list in zzcr_attr() */
#token K_GlyphClassDef	"GlyphClassDef"
#token K_Attach	"Attach"
#token K_GDEFMarkAttachClass "MarkAttachClass"
#token K_LigatureCaret1	"LigatureCaretByPos"
#token K_LigatureCaret2	"LigatureCaretByIndex"
#token K_LigatureCaret3	"LigatureCaretByDev"

#token K_head			"head"		/* Added tag to list in zzcr_attr() */
#token K_FontRevision   "FontRevision"

#token K_hhea			"hhea"		/* Added tag to list in zzcr_attr() */
#token K_Ascender			"Ascender"	
#token K_Descender			"Descender"
#token K_LineGap			"LineGap"

#token K_CaretOffset    "CaretOffset"

#token K_name			"name"		/* Added tag to list in zzcr_attr() */
#token K_nameid			"nameid"

#token K_OS_2			"OS/2"		/* Added tag to list in zzcr_attr() */
#token K_FSType			"FSType"
#token K_FSType2			"fsType"
#token K_LowerOpticalPointSize			"LowerOpSize"
#token K_UpperOpticalPointSize			"UpperOpSize"
#token K_Panose			"Panose"
#token K_TypoAscender	"TypoAscender"
#token K_TypoDescender	"TypoDescender"
#token K_TypoLineGap	"TypoLineGap"
#token K_winAscent	"winAscent"
#token K_winDescent	"winDescent"
#token K_XHeight		"XHeight"
#token K_CapHeight		"CapHeight"
#token K_UnicodeRange	"UnicodeRange"
#token K_CodePageRange	"CodePageRange"
#token K_WeightClass	"WeightClass"
#token K_WidthClass		"WidthClass"
#token K_Vendor			"Vendor"
#token K_FamilyClass	"FamilyClass"

#token K_STAT						"STAT"		/* Added tag to list in zzcr_attr() */
#token K_ElidedFallbackName			"ElidedFallbackName"
#token K_ElidedFallbackNameID		"ElidedFallbackNameID"
#token K_DesignAxis					"DesignAxis"	<<zzmode(TAG_MODE);>>
#token K_AxisValue					"AxisValue"
#token K_flag						"flag"
#token K_location					"location"		<<zzmode(TAG_MODE);>>
#token K_ElidableAxisValueName		"ElidableAxisValueName"
#token K_OlderSiblingFontAttribute	"OlderSiblingFontAttribute"

#token K_vhea               "vhea"		/* Added tag to list in zzcr_attr() */
#token K_VertTypoAscender	"VertTypoAscender"
#token K_VertTypoDescender	"VertTypoDescender"
#token K_VertTypoLineGap	"VertTypoLineGap"

#token K_vmtx               "vmtx"		/* Added tag to list in zzcr_attr() */
#token K_VertOriginY		"VertOriginY"
#token K_VertAdvanceY		"VertAdvanceY"

#token T_FONTREV	"[0-9]+.[0-9]+"
#token T_NUMEXT		"0x[0-9a-fA-F]+|0[0-7]+"
#token T_NUM		"({\-}[1-9][0-9]*)|{\-}0"
#token T_FLOAT		"\-[0-9]+.[0-9]+"

#token T_GCLASS  	"\@[A-Za-z_0-9.\-]+"
#token T_CID		"\\[0-9]+"
#token T_GNAME		"{\\}[A-Za-z_\.]+[A-Za-z_0-9.\-\+\*:\~^!]*"
#token T_STRING		"\""	<<zzmode(STRING_MODE);>>

#token Eof "@"		    <<
						if (featCloseIncludeFile(g, 0))
							zzskip();			/* For an included file */
						else
							featWrapUpFeatFile();	/* For main feature file */
						>>


/* ----------------------------- Rules ------------------------------------- */

// ------- glyph class rules

glyph[char *tok, int allowNotdef]>[GID gid]
	:	<<$gid = 0; /* Suppress optimizer warning */>>
		gname:T_GNAME				<<$gid = featMapGName2GID(g, $gname.text, allowNotdef);
									  if ($tok != NULL)
										  strcpy($tok, $gname.text);>>
	|	cid:T_CID					<<$gid = cid2gid(g, (CID)($cid).lval);>>
	;

/* Returns head of list; any named glyph classes references are copied, unless
   dontcopy is true.
   Note that FEAT_GCLASS is set only for a @CLASSNAME or [...] construct even
   if it contains only a single glyph - this is essential for classifying
   pair pos rules as specific or class pairs */
glyphClass[bool named, bool dontcopy, char *gcname]>[GNode *gnode]
	:	<<
		$gnode = NULL;	/* Suppress compiler warning */
		if ($named)
			gcDefine(gcname);	 /* GNode class assignment */
		>>

		a:T_GCLASS

		<<
		if ($named)
			{
			$gnode   = gcAddGlyphClass($a.text, 1);
			gcEnd(named);
			}
		else if ($dontcopy)
			$gnode = gcLookup($a.text);
		else
			featGlyphClassCopy(g, &($gnode), gcLookup($a.text));
		>>
	|
		(
		<<
		GNode *anonHead;
		GNode *namedHead;
		if (! $named)
			h->gcInsert = &anonHead; /* Anon glyph class (inline) definition */
		>>
		
		"\["
		(
			<<GID gid, endgid;
			  char p[MAX_TOKEN], q[MAX_TOKEN];>>
			glyph[p, TRUE]>[gid]
			<<if (gid == GID_UNDEF) {
				char *secondPart = p;
				char *firstPart = p;
				/* it might be a range.*/
				zzBLOCK(zztasp4);
				zzMake0;
				secondPart = strchr(p, '-');
				if (secondPart != NULL) {
					*secondPart = '\0';
					secondPart++;
					gid = featMapGName2GID(g, firstPart, FALSE );
					endgid  = featMapGName2GID(g, secondPart, FALSE );
					if (gid != 0 && endgid != 0) {
						gcAddRange(gid, endgid, firstPart, secondPart);
					}
					else {
						hotMsg(g, hotFATAL, "incomplete glyph range detected");
					}
				  
				}
				else {
					featMapGName2GID(g, firstPart, FALSE);
					hotMsg(g, hotFATAL, "incomplete glyph range or glyph not in font");
				}
				zzEXIT(zztasp4);
			}
			else
			>>
			( "\-" glyph[q, FALSE]>[endgid]
				<<gcAddRange(gid, endgid, p, q);>>
			|
				<<gcAddGlyph(gid);>>
			)
			|
			b:T_GCLASS				<<gcAddGlyphClass($b.text, named);>>
		)+
		"\]"

		<<
		namedHead = gcEnd(named);
		$gnode = ($named) ? namedHead : anonHead;
		($gnode)->flags |= FEAT_GCLASS;
		>>
		)
	;

//  -------  metrics 
numUInt8>[unsigned char value]
	:	<<$value = 0; /* Suppress optimizer warning */>>
		n:T_NUM		<<
					if ($n.lval < 0 || $n.lval > 255)
						zzerr("not in range 0 .. 255");
					$value = (unsigned char)($n).lval;
					>>
	;		

numInt16>[short value]
	:	<<$value = 0; /* Suppress optimizer warning */>>
		n:T_NUM		<<
					if ($n.lval < -32767 || $n.lval > 32767)
						zzerr("not in range -32767 .. 32767");
					$value = (short)($n).lval;
					>>
	;		

numUInt16>[unsigned value]
	:	<<$value = 0; /* Suppress optimizer warning */>>
		n:T_NUM		<<
					if ($n.lval < 0 || $n.lval > 65535)
						zzerr("not in range 0 .. 65535");
					$value = (unsigned)($n).lval;
					>>
	;		

parameterValue>[short value]
	:	<<
		long retval = 0;
		$value = 0; /* Suppress optimizer warning */
		>>
		d:T_FONTREV		<<
						/* This is actually for picking up fractional point values - i use the T_FONTREV
						pattern as it works fine for fractional point values too. */
						retval = (long)(0.5 + 10*atof(d.text));
						if (retval < 0 || retval > 65535)
							zzerr("not in range 0 .. 65535");
						$value = (short)retval;
						>>
		|
		n:T_NUM		<<
					if ($n.lval < 0 || $n.lval > 65535)
						zzerr("not in range 0 .. 65535");
					$value = (short)($n).lval;
					>>
	;		

/* Extended format: hex, oct also allowed */
numUInt16Ext>[unsigned value]
	:	<<$value = 0; /* Suppress optimizer warning */
		  h->linenum = zzline;>>
		m:T_NUMEXT	<<
					if ($m.lval < 0 || $m.lval > 65535)
						zzerr("not in range 0 .. 65535");
					$value = (unsigned)($m).lval;
					>>
		|
		n:T_NUM		<<
					if ($n.lval < 0 || $n.lval > 65535)
						zzerr("not in range 0 .. 65535");
					$value = (unsigned)($n).lval;
					>>
	;

/* Extended format: hex, oct also allowed */
numUInt32Ext>[unsigned value]
	:	<<$value = 0; /* Suppress optimizer warning */
		  h->linenum = zzline;>>
		m:T_NUMEXT	<<
					if ( $m.ulval > 0xFFFFFFFFu)
						zzerr("not in range 0 .. ((1<<32) -1)");
					$value = (unsigned)($m).ulval;
					>>
		|
		n:T_NUM		<<
					if ($n.ulval > 0xFFFFFFFFu)
						zzerr("not in range 0 .. ((1<<32) -1)");
					$value = (unsigned)($n).ulval;
					>>
	;

/* 32-bit signed fixed-point number (16.16) */
numFixed>[Fixed value]
	:	<<
		int64_t retval = 0;
		$value = 0; /* Suppress optimizer warning */
		h->linenum = zzline;
		>>
		(
		f:T_FLOAT	<<retval = floor(0.5 + strtod($f.text, NULL) * 65536);>>
		|
		d:T_FONTREV	<<retval = floor(0.5 + strtod($d.text, NULL) * 65536);>>
		|
		n:T_NUM		<<retval = $n.lval * 65536;>>
		)
		<<
		if (retval < INT_MIN || retval > INT_MAX)
			zzerr("not in range -32768.0 .. 32767.99998");
		$value = (Fixed)retval;
		>>
	;

metric>[short value]
	:	numInt16>[$value]

		<<if(0)goto fail;>> /* Suppress compiler warning (xxx fit antlr) */
	;

/* xxx Produces gcc warning "label `fail' defined but not used". Modify antlr
   to fix this. Or to shut up warning, put -Wno-unused after -Wall */
deviceEntry
	:	<<
		unsigned short ppem;
		short delta;
		>>
		numUInt16>[ppem] numInt16>[delta]	<<deviceAddEntry(ppem, delta);>>

		<<if(0)goto fail;>> /* Suppress compiler warning (xxx fit antlr) */
	;

/* A countour point without the enclosing <> */
contourpoint>[unsigned short value]
	:	<<$value = 0;>> /* Suppress optimizer warning */

		K_contourpoint numUInt16>[$value]
	;

/* xxx Reads in and returns the offset to a <device> */
device>[unsigned short value]
	:	<<$value = 0;>> /* Suppress optimizer warning */

		K_device
		(
		"0"							<<$value = 0;>>
		|
		<<deviceBeg();>>

		deviceEntry ( "," deviceEntry )*

		<<$value = deviceEnd();>>
		)
	;

/* xxx A caret value without the enclosing <> */
caret
	:	<<
		short val;
		unsigned short contPt;
		Offset dev;
		>>
		K_caret
		(
			metric>[val]
			|
			(
				K_BeginValue
				(
				contourpoint>[contPt]
				|
				device>[dev]
				)
				K_EndValue
			)
		)
	;

/*  A value record */
valueRecordDef
	:
	<<
		short metrics[4];
	>>
	K_valueRecordDef
	(
		(
			K_BeginValue 
			metric>[metrics[0]]
			metric>[metrics[1]]
			metric>[metrics[2]]
			metric>[metrics[3]]
			K_EndValue
		)
		|
		(
		metric>[metrics[2]]
		<<
		 metrics[0] = 0;
		 metrics[1] = 0;
		 metrics[3] = 0;
		>>
		)
	)
	valName:T_GNAME				
	<<
	featAddValRecDef(metrics, $valName.text);
	>>
	;
	

valueRecord[GNode* gnode] /* can be used when an anchor is not an alternative */
	:	
	<<
		short metrics[4];
	>>
		(
		K_BeginValue 
		(
			valueDefName:T_GNAME	<< fillMetricsFromValueDef($valueDefName.text, metrics); >>
			|
			(
				metric>[metrics[0]]
				metric>[metrics[1]]
				metric>[metrics[2]]
				metric>[metrics[3]]
			)
		)
		K_EndValue
		<< if (gnode == NULL)
				zzerr("Glyph or glyph class must precede a value record.");
			$gnode->metricsInfo = getNextMetricsInfoRec();
			$gnode->metricsInfo->cnt = 4;
			$gnode->metricsInfo->metrics[0] = metrics[0];
			$gnode->metricsInfo->metrics[1] = metrics[1];
			$gnode->metricsInfo->metrics[2] = metrics[2];
			$gnode->metricsInfo->metrics[3] = metrics[3];
			if(0)goto fail;  /* Suppress compiler warning (xxx fit antlr) */
		>>
		)
	;

valueRecord3[GNode* gnode] /* Used when a single metric value record is possible. */
	:	<< int val;>>
	
		metric>[val]
		
		<< if (gnode == NULL)
				zzerr("Glyph or glyph class must precede a value record.");
			$gnode->metricsInfo = getNextMetricsInfoRec();
			$gnode->metricsInfo->cnt = 1;
			$gnode->metricsInfo->metrics[0] = val;
			if(0)goto fail;  /* Suppress compiler warning (xxx fit antlr) */
		>>
	;

anchorDef
	:
	<<
		short xVal, yVal;
		unsigned short contourIndex;
		int hasContour = 0;
	>>
	K_anchordef
	metric>[xVal]
	metric>[yVal]
	(
		contourpoint>[contourIndex]
		<<hasContour = 1;>>
	)*
	anchor:T_GNAME				
	<<
	featAddAnchorDef( xVal, yVal, contourIndex,  hasContour, $anchor.text);
	>>
	;

anchor[int componentIndex]>[int anchorWasNULL]
	:	<<
		short xVal, yVal;
		unsigned short contourIndex = 0;
		int isNULL = 0;
		int hasContour = 0;
		char* anchorName = NULL;
		$anchorWasNULL = 0;
		>>
		K_BeginValue
		(
		K_anchor 
			<<
			xVal = 0;
			yVal = 0;
			isNULL = 0;
			>>
		(
			(
			metric>[xVal]
			metric>[yVal]
				(
				(contourpoint>[contourIndex]
				<<hasContour = 1;>>
				)
				|
				)
			)
			|
			K_NULL
				<<$anchorWasNULL = 1;
				isNULL = 1;
				>>
			|
			anchorDefName:T_GNAME	<< anchorName = $anchorDefName.text; >>
			
		)
		K_EndValue
		<<featAddAnchor(xVal, yVal, contourIndex, isNULL, hasContour, anchorName, $componentIndex);>>
		)
	<<if(0)goto fail;>> /* Suppress compiler warning (xxx fit antlr) */
	;




// GSUB rules -----------------------------------------------------------------

/* Any sequence of glyphs and/or glyph classes */
pattern[int markedOK]>[GNode *pat]
	:	<<GNode **insert = &$pat;>>
		(
			(
				(
				<<GID gid;>>
	
				glyph[NULL, FALSE]>[gid]
					<<
					*insert = newNode(h);
					(*insert)->gid = gid;
					(*insert)->nextCl = NULL;
					>>
				)		
				|
				(
				<<GNode *gc;>>
	
				glyphClass[false, false, NULL]>[gc]
					<<
					*insert = gc;
					>>
				)
			)
			{	"'"		
				<<
					if ($markedOK)
						{
						/* Mark this node: */
						(*insert)->flags |= FEAT_MARKED;
						}
					else
						zzerr("cannot mark a replacement glyph pattern");
				>>
			}
			(
				K_lookup
				t:T_LABEL
					<<
					{
					int labelIndex;
					if ((*insert) == NULL)
						zzerr("Glyph or glyph class must precede a lookup reference in a contextual rule.");
						
					labelIndex = featGetLabelIndex($t.text);
					(*insert)->lookupLabels[(*insert)->lookupLabelCount] = labelIndex;
					(*insert)->lookupLabelCount++;
					if ((*insert)->lookupLabelCount > 255)
						zzerr("Too many lookup references in one glyph position.");
					$pat->flags |= FEAT_LOOKUP_NODE; /* used to flag that lookup key was used.  */
					}
					>>
			)*
			<<insert = &(*insert)->nextSeq;
			>>
		)+

		<<(*insert) = NULL;>>
	;

pattern2[GNode** headP]>[GNode *lastNode]
	:	<<
			GNode **insert;
			insert = $headP;
			if (*insert != NULL)
				insert = &(*insert)->nextSeq; 
		>>
		(
			(
				(
				<<GID gid;>>
	
				glyph[NULL, FALSE]>[gid]
					<<
					*insert = newNode(h);
					(*insert)->gid = gid;
					(*insert)->nextCl = NULL;
					>>
				)		
				|
				(
				<<GNode *gc;>>
	
				glyphClass[false, false, NULL]>[gc]
					<<
					*insert = gc;
					>>
				)
			)
			{	"'"		
				<<
					/* Mark this node: */
					(*insert)->flags |= FEAT_MARKED;
				>>
			}
			<<
			{
			GNode **lastPP = &$lastNode;
			*lastPP = *insert;
			insert = &(*insert)->nextSeq;
			}
			>>
		)+

		<<(*insert) = NULL;>>
	;


/* Single glyph or glyph class */
pattern3[GNode** headP]>[GNode *lastNode]
	:	<<
			GNode **insert;
			insert = $headP;
			if (*insert != NULL)
				insert = &(*insert)->nextSeq; 
		>>
		(
			(
				(
				<<GID gid;>>
	
				glyph[NULL, FALSE]>[gid]
					<<
					*insert = newNode(h);
					(*insert)->gid = gid;
					(*insert)->nextCl = NULL;
					>>
				)		
				|
				(
				<<GNode *gc;>>
	
				glyphClass[false, false, NULL]>[gc]
					<<
					*insert = gc;
					>>
				)
			)
			{	"'"		
				<<
					/* Mark this node: */
					(*insert)->flags |= FEAT_MARKED;
				>>
			}
			<<
			{
			GNode **lastPP = &$lastNode;
			*lastPP = *insert;
			insert = &(*insert)->nextSeq;
			}
			>>
		)

		<<(*insert) = NULL;>>
	;


ignoresub_or_pos
	:	<<
		GNode *targ;
		int doSub = 1;
		unsigned int typeGSUB = GSUBChain;
		h->metricsInfo.cnt = 0;
		>>

		K_ignore (K_substitute | K_reverse <<typeGSUB=GSUBReverse;>> |  K_position <<doSub = 0;>>)
		pattern[1]>[targ]	<<targ->flags |= FEAT_IGNORE_CLAUSE;
							if (doSub) addSub(targ, NULL, typeGSUB, zzline);
							  else       addPos(targ, 0, 0);>>
		(
		","
        pattern[1]>[targ]   <<targ->flags |= FEAT_IGNORE_CLAUSE;
        					if (doSub) addSub(targ, NULL, GSUBChain, zzline);
							  else       addPos(targ, 0, 0);>>
		)*
	;

substitute
	:	<<
		GNode *targ;
		GNode *repl = NULL;
		int targLine;
		int type = 0;
		>>

		{
		K_except				 <<type = GSUBChain; h->syntax.numExcept++;>>
			pattern[1]>[targ]			<<targ->flags |= FEAT_IGNORE_CLAUSE;
										addSub(targ, NULL, type, zzline);>>
			(
			"," pattern[1]>[targ]		<<targ->flags |= FEAT_IGNORE_CLAUSE;
											addSub(targ, NULL, type, zzline);>>
			)*
		}

		(
			(
				K_reverse <<type = GSUBReverse;>>
				pattern[1]>[targ]	<<targLine = zzline;>>
				{
					K_by 
					(
					K_NULL						<<addSub(targ, NULL, type, targLine);>>
					|
					pattern[0]>[repl]			<<addSub(targ, repl, type, targLine);>>
					)
				}
			)
			|
			(
				K_substitute
				pattern[1]>[targ]	<<targLine = zzline;>>
				{
					(
					K_by 
					|
					K_from						<<type = GSUBAlternate;>>
					)
					(
					K_NULL
					| pattern[0]>[repl]			
					)
				}
				<<addSub(targ, repl, type, targLine);>>
			)
		)
	;

// GPOS rules -----------------------------------------------------------------



/* Any sequence of glyphs and/or glyph classes followed by an anchor*/
mark_statement
	:	<<
		GNode *targ;
		int isNull =0;
		targ = NULL;
		h->anchorMarkInfo.cnt = 0;
		>>
		K_markClass
		(
			(
			/* require a glyph or glyph class*/
			<<GID gid;>>
	
			glyph[NULL, FALSE]>[gid]
				<<
				targ = newNode(h);
				targ->gid = gid;
				targ->nextCl = NULL;
				>>
			)		
			|
			(
			<<GNode *gc;>>
	
			glyphClass[false, false, NULL]>[gc]
				<<
				targ = gc;
				>>
			)
		) 
		
		/* now require an anchor.*/
		anchor[0]>[isNull]
		
		/* now get the mark class label */
		(
		a:T_GCLASS

		<<
		featAddMark(targ, $a.text);
		>>
		)
	;



position
	:	<<
		/* This need to work for both single and pair pos lookups, and mark to base lookups. */
		GNode *lastNodeP = NULL;
		GNode *targ = NULL;
		GNode *ruleHead = NULL;
		GNode *temp = NULL;
		int enumerate = 0;
		int labelIndex;
		int type = 0;
		int labelLine;
		h->metricsInfo.cnt = 0;
		h->anchorMarkInfo.cnt = 0;
		>>

		{
			K_enumerate	<<enumerate = 1;>> /* valid only for pair pos rules */
		}
		K_position
		{
			pattern2[&targ]>[lastNodeP] <<ruleHead = lastNodeP;>>
		}
		( /* group everything that follows the optional initial glyph or glyph class */
		
			( 
				(
					valueRecord3[ruleHead] /* simple number */
					|
					valueRecord[ruleHead] /* < a b c d> */
				)
				<<type = GPOSSingle;>> /* Note that this is a first pass setting; could end up being GPOSPair */
				(
					pattern3[&ruleHead]>[lastNodeP] <<ruleHead = lastNodeP;>>
					{
						valueRecord3[ruleHead] /* simple number */
						|
						valueRecord[ruleHead] /* < a b c d> */
					}
				)*
			)
			|
			(
			  (
				K_lookup 		<<labelLine = zzline;>>
				t:T_LABEL
					<<
					if (ruleHead == NULL)
						zzerr("Glyph or glyph class must precede a lookup reference in a contextual rule.");
						
					labelIndex = featGetLabelIndex($t.text);
					ruleHead->lookupLabels[ruleHead->lookupLabelCount] = labelIndex;
					ruleHead->lookupLabelCount++;
					if (ruleHead->lookupLabelCount > 255)
						zzerr("Too many lookup references in one glyph position.");
					type = 0;
					ruleHead = lastNodeP;
					type = GPOSChain;
					>>
				)+
				(
					pattern3[&ruleHead]>[lastNodeP] <<ruleHead = lastNodeP;>>
					(
						K_lookup 		<<labelLine = zzline;>>
						t2:T_LABEL
							<<
							if (ruleHead == NULL)
								zzerr("Glyph or glyph class must precede a lookup reference in a contextual rule.");
								
							labelIndex = featGetLabelIndex($t2.text);
							ruleHead->lookupLabels[ruleHead->lookupLabelCount] = labelIndex;
							ruleHead->lookupLabelCount++;
							if (ruleHead->lookupLabelCount > 255)
								zzerr("Too many lookup references in one glyph position.");
							ruleHead = lastNodeP;
							>>
					)*
				)*
            )
			|
			(
				K_cursive
				cursive[&ruleHead]>[lastNodeP]
				<<
				type = GPOSCursive;
				>>
				{
				pattern2[&lastNodeP]>[lastNodeP]
				}
			)
			|
			(
				K_markBase
				baseToMark[&ruleHead]>[lastNodeP]
				<<type = GPOSMarkToBase;>>
				{
				pattern2[&lastNodeP]>[lastNodeP]
				}
			)
			|
			(
				K_markLigature
				ligatureMark[&ruleHead]>[lastNodeP]
				<<type = GPOSMarkToLigature;>>
				{
				pattern2[&lastNodeP]>[lastNodeP]
				}
			)
			|
			(
				K_mark
				baseToMark[&ruleHead]>[lastNodeP]
				<<type = GPOSMarkToMark;>>
				{
				pattern2[&lastNodeP]>[lastNodeP]
				}
			)
			
		) /* end - group everything that follows the optional initial glyph or glyph class */
		<<
			if (targ ==  NULL) /* there was no contextual look-ahead */
				targ = ruleHead;
			addPos(targ, type, enumerate);
		>>
		;

parameters
	:	<<
		short params[MAX_FEAT_PARAM_NUM]; /* allow for feature param size up to MAX_FEAT_PARAM_NUM params. */
		short value;
		short index = 0;
		h->featNameID = 0;
		>>

		K_parameters  parameterValue>[value]	<< params[index] = value;
											index++;
										>> 
					( parameterValue>[value]	<< if (index == MAX_FEAT_PARAM_NUM)
												 zzerr("Too many size parameter values");
											params[index] = value;
											index++;
										>> 
					)+
					<<addFeatureParam(g, params, index);>>
	;

nameEntry>[long plat, long spec, long lang]
	:
		<<
		$plat = -1;		/* Suppress optimizer warning */
		$spec = -1;		/* Suppress optimizer warning */
		$lang = -1;		/* Suppress optimizer warning */

		h->nameString.cnt = 0;
		>>
		(
			K_name					
			{
				numUInt16Ext>[$plat]
					<<
					if ($plat != HOT_NAME_MS_PLATFORM &&
						$plat != HOT_NAME_MAC_PLATFORM)
						hotMsg(g, hotFATAL,
							   "platform id must be %d or %d [%s %d]",
							   HOT_NAME_MS_PLATFORM, HOT_NAME_MAC_PLATFORM,
							   INCL.file, h->linenum);
					>>
					{
					numUInt16Ext>[$spec]
					numUInt16Ext>[$lang]
					}
			}
			T_STRING
			";"
		)
	;

featureNameEntry
	:
		<< long plat, spec, lang; >>
		nameEntry>[plat, spec, lang]
		<<
		sawFeatNames = TRUE;
		addFeatureNameString(plat, spec, lang);
		>>
	;

featureNames
	:
	<<
	sawFeatNames = TRUE;
	h->featNameID = nameReserveUserID(h->g);
	>>
	K_feat_names
	"\{"
	(
		(featureNameEntry)+
	)
	"\}"
	<<
	addFeatureNameParam(h->g, h->featNameID);
	>>
	;


cvParameterBlock
	:
	<<
	sawCVParams = TRUE;
	h->cvParameters.FeatUILabelNameID = 0;
	h->cvParameters.FeatUITooltipTextNameID = 0;
	h->cvParameters.SampleTextNameID = 0;
	h->cvParameters.NumNamedParameters = 0;
	h->cvParameters.FirstParamUILabelNameID = 0;
	h->cvParameters.charValues.cnt = 0;
	h->featNameID = nameReserveUserID(h->g);
	>>
	K_cv_params
	"\{"
		{
			K_cvUILabel
			"\{"
			(
				(featureNameEntry)+
				<<
				addCVNameID(h->featNameID, cvUILabelEnum);
				>>
			)
			"\}"
			";" 
		}
		{
			K_cvToolTip
			"\{"
			(
				(featureNameEntry)+
				<<
				addCVNameID(h->featNameID, cvToolTipEnum);
				>>
			)
			"\}"
			";" 
		}
		{
			K_cvSampletext
			"\{"
			(
				(featureNameEntry)+
				<<
				addCVNameID(h->featNameID, cvSampletextEnum);
				>>
			)
			"\}"
			";" 
		}
		{
			(
			K_cvParameterLabel
			"\{"
			(
				(featureNameEntry)+
				<<
				addCVNameID(h->featNameID, kCVParameterLabelEnum);
				>>
			)
			"\}"
			";" 
			)+
		}
		{
			(
			K_cvCharacter
				(
				<<
				unsigned long uv = 0;
				>>
				numUInt32Ext>[uv]
				<<
				addCVParametersCharValue(uv);
				>>
				)
			";" 
			)+
		}

	"\}"
	<<
	addCVParam(h->g);
	>>
	;

// Assignment rules -----------------------------------------------------------

/* This expects the cursive base glyph or glyph clase
followed by two anchors.
*/
cursive[GNode** headP]>[GNode *lastNode]
	:	<<
			GNode **insert;
			int isNull = 0;
			insert = $headP;
			if (*insert != NULL)
				insert = &(*insert)->nextSeq; 
		>>
		(
			(
				(
				<<GID gid;>>
	
				glyph[NULL, FALSE]>[gid]
					<<
					*insert = newNode(h);
					(*insert)->gid = gid;
					(*insert)->nextCl = NULL;
					>>
				)		
				|
				(
				<<GNode *gc;>>
	
				glyphClass[false, false, NULL]>[gc]
					<<
					*insert = gc;
					>>
				)
				
			)
			
			{	"'"		
				<<
					/* Mark this node: */
					(*insert)->flags |= FEAT_MARKED;
					>>
			}
			<<
			{
			GNode **lastPP = &$lastNode;
			*lastPP = *insert;
			(*insert)->flags |= FEAT_IS_BASE_NODE;
			insert = &(*insert)->nextSeq;
			}
			>>
		)
		/* now for the two anchors */
		anchor[0]>[isNull]
		anchor[0]>[isNull]
		<<(*insert) = NULL;>>
	;

/* This expects the base to mark base glyph or glyph clase
followed by one anchor, follwed by none or more contextul glyph or glyph classes, then by 'mark' and mark class, then by more glyphs.
*/
baseToMark[GNode** headP]>[GNode *lastNode]
	:	<<
			GNode **insert;
			GNode **markInsert = NULL; /* This is used to hold the union of the mark classes, for use in contextual lookups */
			int isNull;
			insert = $headP;
			if (*insert != NULL)
				insert = &(*insert)->nextSeq; 
		>>
		(
			(
				(
				<<GID gid;>>
	
				glyph[NULL, FALSE]>[gid]
					<<
					*insert = newNode(h);
					(*insert)->gid = gid;
					(*insert)->nextCl = NULL;
					>>
				)		
				|
				(
				<<GNode *gc;>>
	
				glyphClass[false, false, NULL]>[gc]
					<<
					*insert = gc;
					>>
				)
				
			)
			
			{	"'"		
				<<
					/* Mark this node: */
					(*insert)->flags |= FEAT_MARKED;
					>>
			}
			<<
			{
			GNode **lastPP = &$lastNode;
			*lastPP = *insert;
			(*insert)->flags |= FEAT_IS_BASE_NODE;
			insert = &(*insert)->nextSeq;
			}
			>>
		)
		{ /* This differs from teh bock above in that the glyphs are not flagged as being the base glyph. */
			(
				(
					(
					<<GID gid;>>
		
					glyph[NULL, FALSE]>[gid]
						<<
						*insert = newNode(h);
						(*insert)->gid = gid;
						(*insert)->nextCl = NULL;
						>>
					)		
					|
					(
					<<GNode *gc;>>
		
					glyphClass[false, false, NULL]>[gc]
						<<
						*insert = gc;
						>>
					)
					
				)
				
				{	"'"		
					<<
						/* Mark this node: */
						(*insert)->flags |= FEAT_MARKED;
						>>
				}
				<<
				{
				GNode **lastPP = &$lastNode;
				*lastPP = *insert;
				insert = &(*insert)->nextSeq;
				}
				>>
			)+
		}
		/* now for the  anchor and mark sequences */
		(
			(
				anchor[0]>[isNull]
				K_mark
				a:T_GCLASS
				<<
				addMarkClass($a.text); /* add mark class reference to current AnchorMarkInfo for this rule */
				>>

				{	"'"		
				<<
					if (markInsert == NULL)
						{
						markInsert = featGlyphClassCopy(g,insert, gcLookup($a.text));
						(*insert)->flags |= FEAT_MARKED;
						}
					else
						markInsert = featGlyphClassCopy(g, markInsert, gcLookup($a.text));
					/* Mark this node: */
				>>
				}

			)+
			<<
			if (markInsert != NULL)
				{
				GNode **lastPP = &$lastNode;
				*lastPP = *insert;
				(*insert)->flags |= FEAT_IS_MARK_NODE;
				insert = &(*insert)->nextSeq;
				}
			>>
		)
		
	;


ligatureMark[GNode** headP]>[GNode *lastNode]
	:	<<
			GNode **insert;
			GNode **markInsert = NULL; /* This is used to hold the union of the mark classes, for use in contextual lookups */
			int componentIndex = 0;
			insert = $headP;
			if (*insert != NULL)
				insert = &(*insert)->nextSeq; 
		>>
		(
			(
				(
				<<GID gid;>>
	
				glyph[NULL, FALSE]>[gid]
					<<
					*insert = newNode(h);
					(*insert)->gid = gid;
					(*insert)->nextCl = NULL;
					>>
				)		
				|
				(
				<<GNode *gc;>>
	
				glyphClass[false, false, NULL]>[gc]
					<<
					*insert = gc;
					>>
				)
				
			)
			
			{	"'"		
				<<
					/* Mark this node: */
					(*insert)->flags |= FEAT_MARKED;
					>>
			}
			<<
			{
			GNode **lastPP = &$lastNode;
			*lastPP = *insert;
			(*insert)->flags |= FEAT_IS_BASE_NODE;
			insert = &(*insert)->nextSeq;
			}
			>>
		)
		{ /* This differs from the block above in that the glyphs are not flagged as being the base glyph. */
			(
				(
					(
					<<GID gid;>>
		
					glyph[NULL, FALSE]>[gid]
						<<
						*insert = newNode(h);
						(*insert)->gid = gid;
						(*insert)->nextCl = NULL;
						>>
					)		
					|
					(
					<<GNode *gc;>>
		
					glyphClass[false, false, NULL]>[gc]
						<<
						*insert = gc;
						>>
					)
					
				)
				
				{	"'"		
					<<
						/* Mark this node: */
						(*insert)->flags |= FEAT_MARKED;
						>>
				}
				<<
				{
				GNode **lastPP = &$lastNode;
				*lastPP = *insert;
				insert = &(*insert)->nextSeq;
				}
				>>
			)+
		}
		/* now for the  anchor and mark sequences */
		(
			(
				(
					<<
						int isNULL = 0; 
						int seenMark = 0;
					>>
					anchor[componentIndex]>[isNULL]
					{
					K_mark
					a:T_GCLASS
					<<
						addMarkClass($a.text); /* add mark class reference to current AnchorMarkInfo for this rule */
						seenMark = 1;
					>>
					}
					<<
						if (!seenMark && !isNULL)
							zzerr("In mark to ligature, non-null anchor must be followed by a mark class.");
					>>
				)
				{
				K_LigatureComponent
				<<componentIndex++;>>
				}
				{	"'"		
				<<
				/* For contextual rules, we need the union of all the mark glyphs as a glyph node in the context */
				if (markInsert == NULL)
					{
					markInsert = featGlyphClassCopy(g,insert, gcLookup($a.text));
					(*insert)->flags |= FEAT_MARKED;
					}
				else
					markInsert = featGlyphClassCopy(g, markInsert, gcLookup($a.text));
				>>
				}
			)+
			<<
			if (markInsert != NULL)
				{
				GNode **lastPP = &$lastNode;
				*lastPP = *insert;
				(*insert)->flags |= FEAT_IS_MARK_NODE;
				insert = &(*insert)->nextSeq;
				}
			>>
		)
		
	;

	
glyphClassAssign
	:	<<GNode *tmp1;>>
		a:T_GCLASS "=" glyphClass[true, false, $a.text]>[tmp1]
	;

scriptAssign
	:	K_script t:T_TAG		<<checkTag($t.ulval, scriptTag, 1);>>
	;

languageAssign
	:	<<
		int langChange;
		int incDFLT = 1;
		int oldFormatSeen = 0;
		>>
		K_language t:T_TAG	<<langChange = checkTag($t.ulval, languageTag,1);>>
		(
			K_excludeDFLT	<<incDFLT = 0; oldFormatSeen = 1; >>
		|	K_includeDFLT	<<incDFLT = 1; oldFormatSeen = 1;>>
		|	K_exclude_dflt	<<incDFLT = 0; >>
		|	K_include_dflt	<<incDFLT = 1;>>
		|
		)
		<<
		if (langChange != -1)
			includeDFLT(incDFLT, langChange, oldFormatSeen);
		>>
	;

namedLookupFlagValue[unsigned short *val]
	:
		<<
		unsigned short umfIndex = 0;
		unsigned short gdef_markclass_index = 0;
		>>
		/* This sets val from the named lookup and, if K_MarkAttachmentType, the class index. */
		K_RightToLeft	   <<setLkpFlagAttribute(val,otlRightToLeft,0);>>
		|
		K_IgnoreBaseGlyphs <<setLkpFlagAttribute(val,otlIgnoreBaseGlyphs,0);>>
		|
		K_IgnoreLigatures  <<setLkpFlagAttribute(val,otlIgnoreLigatures,0);>>
		|
		K_IgnoreMarks      <<setLkpFlagAttribute(val,otlIgnoreMarks,0);>>
		|
		K_UseMarkFilteringSet
		(
		<<GNode *gc;>>
		glyphClass[false, true, NULL]>[gc]
		<<
			getMarkSetIndex(gc, &umfIndex);
			setLkpFlagAttribute(val,otlUseMarkFilteringSet, umfIndex);
		>>
		)
		|
		(
		K_MarkAttachmentType
		(
			numUInt8>[gdef_markclass_index]
			|
			(
				<<GNode *gc;>>
				glyphClass[false, true, NULL]>[gc]
				<<
				 getGDEFMarkClassIndex(gc, &gdef_markclass_index);
				>>
			)
		
		)
		<<setLkpFlagAttribute(val, otlMarkAttachmentType, gdef_markclass_index);
		>>
		)
	;

lookupflagAssign
	:	<<
		unsigned short val = 0;
		>>

		K_lookupflag
		(
			numUInt16>[val]		/* Explicit value */
			|
			namedLookupFlagValue[&val]
		)+

		<< setLkpFlag(val); >>
	;

featureUse
	:	K_feature t:T_TAG		<<aaltAddFeatureTag($t.ulval);>>
	;

subtable
	:	K_subtable		        <<subtableBreak();>>
	;
	
sizemenuname
	:	
			<<
			long plat = -1;		/* Suppress optimizer warning */
			long spec = -1;		/* Suppress optimizer warning */
			long lang = -1;		/* Suppress optimizer warning */
			
			h->nameString.cnt = 0;
			>>
			(
			K_sizemenuname					
			{
			numUInt16Ext>[plat]
				<<
				if (plat != HOT_NAME_MS_PLATFORM &&
					plat != HOT_NAME_MAC_PLATFORM)
					hotMsg(g, hotFATAL,
						   "platform id must be %d or %d [%s %d]",
						   HOT_NAME_MS_PLATFORM, HOT_NAME_MAC_PLATFORM,
						   INCL.file, h->linenum);
				>>
			{
			numUInt16Ext>[spec]
			numUInt16Ext>[lang]
			}
			}
			T_STRING
			)
		<< addSizeNameString(plat, spec, lang);>>
	
	;
		
statement
	:	( featureUse		// e.g.  feature vert;	# only in 'aalt'
		| scriptAssign		// e.g.  script kana;
		| languageAssign	// e.g.  language DEU;
		| lookupflagAssign	// e.g.  lookupflag RightToLeft;
							
		| glyphClassAssign	// e.g.  @vowels = [a e i o u];
							
		| ignoresub_or_pos	// e.g.  ignore substitute @LET a, a @LET;
		| substitute		// e.g.  sub f i by fi;
		| mark_statement	// e.g.  mark one <-80 0 -160 0>;
		| position			// e.g.  pos one <-80 0 -160 0>;
							//       pos A y -50;
		| parameters	    // e.g.  parameters 100 3 80 139;  # only in 'size'
		| sizemenuname		// e.g.  sizemenuname "MinionPro Caption"; # 'size'

		| featureNames
		
		| subtable	        // e.g.  subtable;

		|					// e.g.  ;
		) ";"
	;

lookupBlockOrUse
	:	<<int labelLine; int useExtension = 0;>>
		K_lookup t:T_LABEL		<<labelLine = zzline;>>
		( {
		  K_useExtension		<<useExtension = 1;>>
		  }
		  "\{"					<<
                                checkLkpName($t.text, labelLine, 1, 0);
								if (useExtension)
									flagExtension(1);
                                >>
		  (statement)+
		  "\}"					<<zzmode(LABEL_MODE);>>
		  u:T_LABEL				<<checkLkpName($u.text, 0, 0, 0);>>
		|						<<useLkp($t.text);>>
		)
		";"
	;

lookupBlockStandAlone
	:	<<int labelLine; int useExtension = 0;>>
		K_lookup t:T_LABEL		<<labelLine = zzline;>>
		( {
		  K_useExtension		<<useExtension = 1;>>
		  }
		  "\{"					<<
                                checkLkpName($t.text, labelLine, 1,1 );
								if (useExtension)
									flagExtension(1);
                                >>
		  (statement)+
		  "\}"					<<zzmode(LABEL_MODE);>>
		  u:T_LABEL				<<checkLkpName($u.text, 0, 0, 1);>>
		)
		";"
	;

featureBlock
	:	K_feature t:T_TAG		<<checkTag($t.ulval, featureTag, 1);>>
		{
		K_useExtension			<<flagExtension(0);>>
		}
		"\{"
		( statement
		| lookupBlockOrUse
		| cvParameterBlock
		)+
		"\}"					<<zzmode(TAG_MODE);>>
		u:T_TAG					<<checkTag($u.ulval, featureTag, 0);>>
		";"
	;

baseScript[int vert, long nTag]
	:	<<
		int num;
	dnaDCL(short, coord);
	dnaINIT(g->dnaCtx, coord, 5, 5);
		>>

		s:T_TAG <<zzmode(TAG_MODE);>>
		d:T_TAG
		(
		numInt16>[num]			<<*dnaNEXT(coord) = num;>>
		)+

		<<
		if (coord.cnt != $nTag)
			zzerr("number of coordinates not equal to number of "
				  "baseline tags");
		BASEAddScript(g, $vert, $s.ulval, $d.ulval, coord.array);
		dnaFREE(coord);
		>>
	;

axisSpecs
	:	<<
		int vert = 0;
		dnaDCL(Tag, tagList);
		dnaINIT(g->dnaCtx, tagList, 5, 5);
		>>

		(K_HorizAxis_BaseTagList | K_VertAxis_BaseTagList <<vert = 1;>>)
		(
		t:T_TAG		<<*dnaNEXT(tagList) = $t.ulval;
					  /* printf("<%c%c%c%c> ", TAG_ARG(t.ulval)); */
					  zzmode(TAG_MODE);>>
		)+
		<<BASESetBaselineTags(g, vert, tagList.cnt, tagList.array);>>

		(
		K_HorizAxis_BaseScriptList
			<<if (vert == 1) zzerr("expecting \"VertAxis.BaseScriptList\"");>>
		| K_VertAxis_BaseScriptList
			<<if (vert == 0) zzerr("expecting \"HorizAxis.BaseScriptList\"");>>
		)

		baseScript[vert, tagList.cnt]
		(
		","								<<zzmode(TAG_MODE);>>
		baseScript[vert, tagList.cnt]
		)*

		";"		
		<<dnaFREE(tagList);>>
	;

table_BASE
	:	t:K_BASE		<<checkTag($t.ulval, tableTag, 1);>>
		"\{"
			axisSpecs
			{axisSpecs}
		"\}"					<<zzmode(TAG_MODE);>>
		u:T_TAG					<<checkTag($u.ulval, tableTag, 0);>>
		";"
	;

table_OS_2
	:	t:K_OS_2		<<checkTag($t.ulval, tableTag, 1);>>
		"\{"
		(
			(
			<<
			char panose[10];
			short valInt16;
			unsigned short arrayIndex;
			unsigned short valUInt16;
			short codePageList[kLenCodePageList];
			short unicodeRangeList[kLenUnicodeList];
			h->nameString.cnt = 0;
			>>

			K_TypoAscender  numInt16>[valInt16]
									<<g->font.TypoAscender = valInt16;>>
			|              					                               
			K_TypoDescender numInt16>[valInt16]
									<<g->font.TypoDescender = valInt16;>>
			|
			K_TypoLineGap 	numInt16>[valInt16]
									<<g->font.TypoLineGap = valInt16;>>
			|
			K_winAscent numInt16>[valInt16]
									<<g->font.winAscent = valInt16;>>
			|
			K_winDescent 	numInt16>[valInt16]
									<<g->font.winDescent = valInt16;>>
			|
			K_XHeight	 	metric>[valInt16]
									<<g->font.win.XHeight = valInt16;>>
			|
			K_CapHeight	 	metric>[valInt16]
									<<g->font.win.CapHeight = valInt16;>>
			|
			K_Panose numUInt8>[panose[0]]
					 numUInt8>[panose[1]]
					 numUInt8>[panose[2]]
					 numUInt8>[panose[3]]
					 numUInt8>[panose[4]]
					 numUInt8>[panose[5]]
					 numUInt8>[panose[6]]
					 numUInt8>[panose[7]]
					 numUInt8>[panose[8]]
					 numUInt8>[panose[9]]  <<OS_2SetPanose(g, panose);>>
			|              					                               
			K_FSType numUInt16>[valUInt16]
									<<OS_2SetFSType(g, valUInt16);>>
			|              					                               
			K_FSType2 numUInt16>[valUInt16]
									<<OS_2SetFSType(g, valUInt16);>>
			|              					                               
			K_UnicodeRange
			  (
				<<for (arrayIndex = 0; arrayIndex < kLenUnicodeList; arrayIndex++) unicodeRangeList[arrayIndex] = kCodePageUnSet; arrayIndex = 0; >>
				(
				numUInt16>[valUInt16] <<if (arrayIndex < kLenUnicodeList) unicodeRangeList[arrayIndex] = valUInt16; arrayIndex++;>>
				)+
				<<featSetUnicodeRange(g, unicodeRangeList);>>
			  )
			|              					                               
			K_CodePageRange
			  (
				<<for (arrayIndex = 0; arrayIndex < kLenCodePageList; arrayIndex++) codePageList[arrayIndex] = kCodePageUnSet; arrayIndex = 0;>>
				(
				numUInt16>[valUInt16] <<if (arrayIndex < kLenCodePageList) codePageList[arrayIndex] = valUInt16; arrayIndex++;>>
				)+
				<<featSetCodePageRange(g, codePageList);>>
			  )
			|              					                               
			K_WeightClass numUInt16>[valUInt16]
									<<OS_2SetWeightClass(g, valUInt16);>>
			|              					                               
			K_WidthClass numUInt16>[valUInt16]
									<<OS_2SetWidthClass(g, valUInt16);>>
			|
			K_LowerOpticalPointSize numUInt16>[valUInt16]
									<<OS_2LowerOpticalPointSize(g, valUInt16);>>
			|              					                               
			K_UpperOpticalPointSize numUInt16>[valUInt16]
									<<OS_2UpperOpticalPointSize(g, valUInt16);>>
			|              					                               
			K_FamilyClass numUInt16Ext>[valUInt16]
									<<OS_2FamilyClass(g, valUInt16);>>
			|              					                               
			(K_Vendor T_STRING)
									<<addVendorString(g);>>
			|
			)
			";"
		)+
		"\}"			<<zzmode(TAG_MODE);>>
		u:T_TAG			<<checkTag($u.ulval, tableTag, 0);>>
		";"
	;

statNameEntry
	:
		<< long plat, spec, lang; >>
		nameEntry>[plat, spec, lang]
		<< addUserNameString(plat, spec, lang);>>
	;

designAxis
	:
		<<
		uint16_t ordering;
		h->featNameID = 0;
		>>
		K_DesignAxis t:T_TAG numUInt16>[ordering]
		"\{"
			(statNameEntry)+
		"\}"
		<<
		STATAddDesignAxis(g, $t.ulval, h->featNameID, ordering);
		h->featNameID = 0;
		>>
	;

axisValueFlag[uint16_t *flags]
	:
		K_OlderSiblingFontAttribute     << *flags |= 0x0001; >>
		|
		K_ElidableAxisValueName << *flags |= 0x0002; >>
	;

axisValueFlags>[uint16_t flags]
	:
		<< $flags = 0; >>
		K_flag (axisValueFlag[&$flags])+
		";"
	;

axisValueLocation>[uint16_t format, Tag tag, Fixed value, Fixed min, Fixed max]
	:
		K_location t:T_TAG << $tag = $t.ulval; >> numFixed>[$value]
		( ";" << $format = 1; >>
		| numFixed>[$min] << $format = 3; >>
		  {numFixed>[$max] << $format = 2; >> } ";"
		)
	;

axisValue
	:
		<<
		uint16_t flags = 0;
		uint16_t format = 0, prev = 0;
		dnaDCL(Tag, axisTags);
		dnaDCL(Fixed, values);
		Fixed value, min, max;
		Tag axisTag;
		h->featNameID = 0;
		dnaINIT(g->dnaCtx, axisTags, 1, 5);
		dnaINIT(g->dnaCtx, values, 1, 5);
		>>
		K_AxisValue
		"\{"
			( statNameEntry
			| axisValueFlags>[flags]
			| axisValueLocation>[format, axisTag, value, min, max]
			  <<
			  if (prev && (prev != 1 || format != prev))
				zzerr("AxisValue with unsupported multiple location statements");
			  *dnaNEXT(axisTags) = axisTag;
			  *dnaNEXT(values) = value;
			  prev = format;
			  >>
			)+
		"\}"
		<<
		if (!format)
			zzerr("AxisValue missing location statement");
		if (!h->featNameID)
			zzerr("AxisValue missing name entry");
		STATAddAxisValueTable(g, format, axisTags.array, values.array,
				      values.cnt, flags, h->featNameID,
				      min, max);
		dnaFREE(axisTags);
		dnaFREE(values);
		>>
	;

elidedFallbackName
	:
		<<
		long plat, spec, lang;
		h->featNameID = 0;
		>>
		K_ElidedFallbackName
		"\{"
			(
				nameEntry>[plat, spec, lang]
				<< addUserNameString(plat, spec, lang); >>
			)+
		"\}"
		<<
		if (!STATSetElidedFallbackNameID(g, h->featNameID))
			zzerr("ElidedFallbackName already defined.");
		h->featNameID = 0;
		>>
	;

elidedFallbackNameID
	:
		<< long nameID; >>
		K_ElidedFallbackNameID numUInt16Ext>[nameID]
		<<
		if (!STATSetElidedFallbackNameID(g, nameID))
			zzerr("ElidedFallbackName already defined.");
		>>
	;

table_STAT
	: t:K_STAT			<<checkTag($t.ulval, tableTag, 1);>>
		<<
		sawSTAT = TRUE;
		>>
		"\{"
		(
			(
			designAxis
			|
			axisValue
			|
			elidedFallbackName
			|
			elidedFallbackNameID
			|
			)
			";"
		)+
		"\}"			<<zzmode(TAG_MODE);>>
		u:T_TAG			<<checkTag($u.ulval, tableTag, 0);>>
		";"
	;

glyphClassOptional>[GNode *gnode]
	:	<<$gnode = NULL;>> /* Suppress optimizer warning */

		glyphClass[false, false, NULL]>[$gnode]
		|				<<$gnode = NULL;>>
	;

table_GDEF
	:	t:K_GDEF		<<checkTag($t.ulval, tableTag, 1);>>
		"\{"
		(
			(
			<<GNode *gc[4];
			short val;
			>>

			K_GlyphClassDef glyphClassOptional>[gc[0]] ","
							glyphClassOptional>[gc[1]] ","
							glyphClassOptional>[gc[2]] ","
							glyphClassOptional>[gc[3]]
							<<setGDEFGlyphClassDef(gc[0],gc[1],gc[2],gc[3]);>>
			|
			(
				K_Attach 
				pattern[0]>[gc[0]]
				(
				metric>[val]
				<<addGDEFAttach(gc[0], val);>>
				)+
			)
			|
			(
				K_LigatureCaret1 
				pattern[0]>[gc[0]]
				<<initGDEFLigatureCaretValue();>>
				(
				metric>[val]
				<<addGDEFLigatureCaretValue(val);>>
				)+
				<<setGDEFLigatureCaret(gc[0], 1);>>
			)
			|

			(
				K_LigatureCaret2
				pattern[0]>[gc[0]]
				<<initGDEFLigatureCaretValue();>>
				(
				metric>[val]
				<<addGDEFLigatureCaretValue(val);>>
				)+
				<<setGDEFLigatureCaret(gc[0], 2);>>
			)
			|
			)
			";"
		)*
		"\}"			<<zzmode(TAG_MODE);>>
		u:T_TAG			<<checkTag($u.ulval, tableTag, 0);>>
		";"
	;

table_head
	:	t:K_head		<<checkTag($t.ulval, tableTag, 1);>>
		"\{"
			(
			K_FontRevision r:T_FONTREV	<<setFontRev($r.text);>>
			|
			)
			";"
		"\}"			<<zzmode(TAG_MODE);>>
		u:T_TAG			<<checkTag($u.ulval, tableTag, 0);>>
		";"
	;

table_hhea
	:	t:K_hhea		<<checkTag($t.ulval, tableTag, 1);>>
		"\{"
			(
				(
				<<short value;>>
	
				K_CaretOffset numInt16>[value]	<<hheaSetCaretOffset(g, value);>>
				|
				K_Ascender numInt16>[value]	<<g->font.hheaAscender = value;>>
				|
				K_Descender numInt16>[value]	<<g->font.hheaDescender = value;>>
				|
				K_LineGap numInt16>[value]	<<g->font.hheaLineGap = value;>>
				|
				)
				";"
			)+
		"\}"			<<zzmode(TAG_MODE);>>
		u:T_TAG			<<checkTag($u.ulval, tableTag, 0);>>
		";"
	;

table_name
	:	t:K_name		<<checkTag($t.ulval, tableTag, 1);>>
		"\{"
		(
		<<
		int ignoreRec = 0;	/* Suppress optimizer warning */
		long id = 0;		/* Suppress optimizer warning */
		long plat = 0;		/* Suppress optimizer warning */
		long spec = 0;		/* Suppress optimizer warning */
		long lang = 0;		/* Suppress optimizer warning */
		>>
			(
			K_nameid	<<
						h->nameString.cnt = 0;
						plat = -1;
						spec = -1;
						lang = -1;
						ignoreRec = 0;
						>>

				numUInt16Ext>[id]
				<<
				if (sawSTAT && id > 255)
					hotMsg(g, hotFATAL, "name table should be defined before "
							"STAT table with nameids above 255");
				if (sawCVParams && id > 255)
					hotMsg(g, hotFATAL, "name table should be defined before "
							"GSUB cvParameters with nameids above 255");
				if (sawFeatNames && id > 255)
					hotMsg(g, hotFATAL, "name table should be defined before "
							"GSUB featureNames with nameids above 255");
				>>
				{
				numUInt16Ext>[plat]
					<<
					if (plat != HOT_NAME_MS_PLATFORM &&
						plat != HOT_NAME_MAC_PLATFORM)
						hotMsg(g, hotFATAL,
							   "platform id must be %d or %d [%s %d]",
							   HOT_NAME_MS_PLATFORM, HOT_NAME_MAC_PLATFORM,
							   INCL.file, h->linenum);
					>>
				{
				numUInt16Ext>[spec]
				numUInt16Ext>[lang]
				}
				}
				T_STRING
			|
			)
			";"		<<if (!ignoreRec) addNameString(plat, spec, lang, id);>>
		)+
		"\}"		<<zzmode(TAG_MODE);>>
		u:T_TAG		<<checkTag($u.ulval, tableTag, 0);>>
		";"
	;

table_vhea
	:	t:K_vhea		<<checkTag($t.ulval, tableTag, 1);>>
		"\{"
		(
			(
			<<short value;>>

			K_VertTypoAscender  numInt16>[value]
									<<g->font.VertTypoAscender = value;>>
			|              					                               
			K_VertTypoDescender numInt16>[value]
									<<g->font.VertTypoDescender = value;>>
			|
			K_VertTypoLineGap 	numInt16>[value]
									<<g->font.VertTypoLineGap = value;>>
			|
			)
			";"
		)+
		"\}"					<<zzmode(TAG_MODE);>>
		u:T_TAG					<<checkTag($u.ulval, tableTag, 0);>>
		";"
	;

table_vmtx
	:	t:K_vmtx		<<checkTag($t.ulval, tableTag, 1);>>
		"\{"
		(
			(
			<<GID gid; short value;>>

			K_VertOriginY  glyph[NULL, FALSE]>[gid]  numInt16>[value]
				<<
				hotAddVertOriginY(g, gid, value, INCL.file, zzline);
				>>
			|
			K_VertAdvanceY  glyph[NULL, FALSE]>[gid]  numInt16>[value]
				<<
				hotAddVertAdvanceY(g, gid, value, INCL.file, zzline);
				>>
			|
			)
			";"
		)+
		"\}"					<<zzmode(TAG_MODE);>>
		u:T_TAG					<<checkTag($u.ulval, tableTag, 0);>>
		";"
	;


tableBlock
	:	K_table
		(	table_BASE
		|	table_GDEF
		|	table_head
		|	table_hhea
		|	table_name
		|	table_OS_2
		|	table_STAT
		|	table_vhea
		|	table_vmtx
		)
	;

languagesystemAssign
	:	K_languagesystem s:T_TAG <<zzmode(TAG_MODE);>> t:T_TAG
			<<addLangSys($s.ulval, $t.ulval, 1);>>
	;

topLevelStatement
	:	(	glyphClassAssign
		|	languagesystemAssign
		|	mark_statement
		|	anchorDef
		|	valueRecordDef
		|
		)
		";"
	;

anonBlock
	:	K_anon t:T_TAG		<<h->anonData.tag = $t.ulval;>>
			   				<<featAddAnonData(); featSetTagReturnMode(START);>>
	;

featureFile
	:	(	topLevelStatement
		|	featureBlock
		|	tableBlock
		|	anonBlock
		|	lookupBlockStandAlone
		)*

		<<if (zzchar != EOF) zzerrCustom("expecting EOF");>>
			// Eof itself is intercepted by the tokenizer.
	;
