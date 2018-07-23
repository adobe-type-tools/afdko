/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)spotmsgs.c	1.7
 * Changed:    5/19/99 11:20:13
 ***********************************************************************/

#include "spotmsgs.h"

const Byte8 *EnglishMessages[] =
{
"",
"BASE: unknown BaseCoord format [%d]\n", /* SPOT_MSG_BASEUNKCOORD */
"CFF parsing\n", /* SPOT_MSG_CFFPARSING */
"glyphId %d too large (ignored)\n", /* SPOT_MSG_GIDTOOLARGE */
"can't use -b option with more than 1 glyph\n", /* SPOT_MSG_BOPTION */
"ENCO: unknown encoding format <%d> (ignored)\n", /* SPOT_MSG_ENCOUNKENCOD */
"GPOS: unknown single adjustment format [%d]@0x%x\n", /* SPOT_MSG_GPOSUNKSINGL */
"GPOS: unknown pair adjustment format [%d]@0x%x\n", /* SPOT_MSG_GPOSUNKPAIR */
"GPOS: unknown Anchor format [%d]@0x%x\n", /* SPOT_MSG_GPOSUNKANCH */
"GPOS: unknown mark to base format [%d]@0x%x\n", /* SPOT_MSG_GPOSUNKMARKB */
"GPOS: unsupported read lookup type [%d][0x%x]@0x%x\n", /* SPOT_MSG_GPOSUNSRLOOK */
"GPOS: unknown read lookup type [%d][0x%x]@0x%x\n", /* SPOT_MSG_GPOSUNKRLOOK */
"GPOS: NULL feature tag @0x%x\n", /* SPOT_MSG_GPOSNULFEAT */
"GPOS: unsupported dump lookup type [%d][0x%x]@0x%x\n", /* SPOT_MSG_GPOSUNSDLOOK */
"GPOS: unknown dump lookup type [%d][0x%x]@0x%x\n", /* SPOT_MSG_GPOSUNKDLOOK */
"GSUB: unknown single substitution format [%d]@0x%x\n", /* SPOT_MSG_GSUBUNKSINGL */
"GSUB: unknown multiple substution format [%d]@0x%x\n", /* SPOT_MSG_GSUBUNKMULT */
"GSUB: unknown alternate substution format [%d]@0x%x\n", /* SPOT_MSG_GSUBUNKALT */
"GSUB: unknown ligature substitution format [%d]@0x%x\n", /* SPOT_MSG_GSUBUNKLIG */
"GSUB: unknown context substitution format [%d]@0x%x\n",  /* SPOT_MSG_GSUBUNKCTX */
"GSUB: unknown chaining-context substitution format [%d]@0x%x\n", /* SPOT_MSG_GSUBUNKCCTX */
"GSUB: unknown read lookup type [%d][0x%x]@0x%x\n", /* SPOT_MSG_GSUBUNKRLOOK */
"GSUB: Context substitution specifies %d glyphClasses to match, but only %d are defined\n", /* SPOT_MSG_GSUBCTXDEF */
"GSUB: Context substitution specifies more than 1 substitution to be performed. Others will be ignored.\n", /* SPOT_MSG_GSUBCTXCNT */
"GSUB: Context substitution specifies GlyphSequenceIndex > 1. Not yet implemented.\n", /* SPOT_MSG_GSUBCTXNYI */
"GSUB: chain context subst specifies more than 1 substitution to be performed. Others will be ignored\n", /*  SPOT_MSG_GSUBCCTXCNT */
"GSUB: more than 1 item in inputGlyph coverage. Others will be ignored\n", /* SPOT_MSG_GSUBINPUTCNT */
"GSUB: NULL feature tag @0x%x\n", /* SPOT_MSG_GSUBNULFEAT */
"GSUB: cannot evaluate SingleSubst format with more than 1 glyph input\n", /* SPOT_MSG_GSUBEVALCNT */
"GSUB: cannot find glyphID [%d] glyphName [%s] in Coverage\n", /* SPOT_MSG_GSUBNOCOVG */
"GSUB: evaluation of unknown/unsupported lookup type [%d]\n", /* SPOT_MSG_GSUBEUNKLOOK */
"GSUB: cannot evaluate lookup[#%d] of type [%d] with more than 1 subtable [%d]\n", /* SPOT_MSG_GSUBESUBCNT */
"cmap: MS UGL cmap in wrong format [%d]\n", /* SPOT_MSG_cmapBADMSFMT */
"file error <premature EOF> [%s]\n", /* SPOT_MSG_EARLYEOF */
"bad input object size [%d]\n", /* SPOT_MSG_BADREADSIZE */
"%c%c%c%c can't read %c%c%c%c because table missing\n", /* SPOT_MSG_TABLEMISSING */
"out of memory\n", /* SPOT_MSG_NOMOREMEM */
"can't dump all names because can't get nGlyphs\n", /* SPOT_MSG_UNKNGLYPHS */
"glyf: loca offset too large, glyph[%d] (ignored)\n", /* SPOT_MSG_glyfBADLOCA */
"glyf: maxp.maxComponentElements exceeded, glyph[%d] (ignored)\n", /* SPOT_MSG_glyfMAXCMP */
"glyf: compound glyph numberOfContours not supported [%d]\n", /* SPOT_MSG_glyfUNSCOMP */
"glyf: compound glyph point dumping not supported [%hu]\n", /* SPOT_MSG_glyfUNSDCOMP */
"glyf: compound anchor points not supported [%d]\n", /* SPOT_MSG_glyfUNSCANCH */
"head.lsb doesn't match glyf.glyph[%hu].xMin\n", /* SPOT_MSG_glyfLSBXMIN */
"kern: table format %d not supported\n", /* SPOT_MSG_kernUNSTAB */
"loca: bad head.indexToLocFormat [%d]\n", /* SPOT_MSG_locaBADFMT */
"bad file [%s] (ignored)\n", /* SPOT_MSG_BADFILE */
"Cannot alloc path element.\n", /* SPOT_MSG_pathNOPELT */
"Cannot alloc subpath.\n", /* SPOT_MSG_pathNOSUBP */
"Consecutive moveto's in the same subpath\n", /* SPOT_MSG_pathMTMT */
"Cannot allocate Outlines' subpath array\n", /* SPOT_MSG_pathNOOSA */
"post: can't read version 4.0 because can't get nGlyphs\n", /* SPOT_MSG_postNONGLYPH */
"post: bad version %d.%d. (%08x)\n", /* SPOT_MSG_postBADVERS */
"Print string too long\n", /* SPOT_MSG_prufSTR2BIG */
"Print write failure.\n", /* SPOT_MSG_prufWRTFAIL */
".", /* SPOT_MSG_prufPROGRESS */
"Proofed %d pages.\n", /* SPOT_MSG_prufNUMPAGES */
"Output to '%s'.", /* SPOT_MSG_prufOFILNAME */
" Please remove when finished.\n", /* SPOT_MSG_prufPLSCLEAN */
"Spooling to printer '%s'.\n", /* SPOT_MSG_prufSPOOLTO */
"can't open outputfile for PS proofing\n", /* SPOT_MSG_prufNOOPENF */
"Preparing PostScript output: ", /* SPOT_MSG_prufPREPPS */
"can't create PostScript output\n", /* SPOT_MSG_prufCANTPS */
"can't create buffer for printing\n", /* SPOT_MSG_prufNOBUFMEM */
"no glyph shape-tables present?\n", /* SPOT_MSG_prufNOSHAPES */
"Please select a PostScript printer as the default printer\n", /* SPOT_MSG_prufNOTPSDEV */
"sfnt resource id [%d] not found\n", /* SPOT_MSG_NOSUCHSFNT */
"bad tag <%s> (ignored)\n", /* SPOT_MSG_BADTAGSTR */
"bad feat <%s> (ignored)\n", /* SPOT_MSG_BADFEATSTR */
"I/O Error [%s]\n", /* SPOT_MSG_sysIOERROR */
"EOF Error [%s]\n", /* SPOT_MSG_sysEOFERROR */
"No such file? [%s]\n", /* SPOT_MSG_sysNOSUCHF */
"file error (%d) [%s]\n", /* SPOT_MSG_sysMACFERROR */
"file error <%s> [%s]\n", /* SPOT_MSG_sysFERRORSTR */
"Empty script file? [%s]\n", /* SPOT_MSG_BADSCRIPTFILE */
"Executing script-file [%s]\n", /* SPOT_MSG_ECHOSCRIPTFILE */
"Executing command-line: ", /* SPOT_MSG_ECHOSCRIPTCMD */
"%s ", /* SPOT_MSG_RAWSTRING */
"\n", /* SPOT_MSG_EOLN */
"Please select OpenType file to use.\n", /* SPOT_MSG_MACSELECTOTF */
"Please select script-file to execute.\n", /* SPOT_MSG_MACSELECTSCRIPT */
"Missing/unrecognized font filename on commandline\n", /* SPOT_MSG_MISSINGFILENAME */
"%c%c%c%c table may be too large for offset-size.\n", /* SPOT_MSG_TABLETOOBIG */
"Bad/Unknown coverage format [%d]@0x%x\n", /* SPOT_MSG_BADUNKCOVERAGE */
"Bad/Unknown class format [%d]@0x%x\n", /* SPOT_MSG_BADUNKCLASS */
"%c%c%c%c table: invalid index value (%d) at %s.\n", /* SPOT_MSG_BADINDEX */
"Bad/Unknown table format in cmap.\n", /* SPOT_MSG_cmapBADTBL*/
"Done.\n", /* SPOT_MSG_DONE */
"Duplicate kern pairs in same script/lang: %s\n", /* SPOT_MSG_GPOS_DUPKRN */
"Error writing to temp file: output data will be incomplete.", /* SPOT_MSG_FILEFERR */
"GPOS: unknown context substitution format [%d]@0x%x\n",  /* SPOT_MSG_GPOSUNKCTX */
"GPOS: unknown chaining-context substitution format [%d]@0x%x\n", /* SPOT_MSG_GPOSUNKCCTX */
"GPOS: Context substitution specifies %d glyphClasses to match, but only %d are defined\n", /* SPOT_MSG_GPOSCTXDEF */
"GPOS: Context substitution specifies more than 1 substitution to be performed. Others will be ignored.\n", /* SPOT_MSG_GPOSCTXCNT */
"GPOS: Context substitution specifies GlyphSequenceIndex > 1. Not yet implemented.\n", /* SPOT_MSG_GPOSCTXNYI */
"GPOS: chain context subst specifies more than 1 substitution to be performed. Others will be ignored\n", /*  SPOT_MSG_GPOSCCTXCNT */
"proof, AFM, and feature file format dumps not yet supported for context format %d.\n",/* SPOT_MSG_GPOSUFMTCTX */
"proof ,AFM, and feature file format dumps not yet supported for chaining  context format %d.\n", /* SPOT_MSG_GPOSUFMTCCTX */
"AFM format dump not yet supported for chanining  context format %d.\n", /* SPOT_MSG_GPOSUFMTCCTX3 */
"Bad/Unknown Device Table format [%d]@0x%x\n", /* SPOT_MSG_GPOSUFMTDEV */
"proof and feature file format dumps not yet supported for lookup Type 5 context format %d.\n",/* SPOT_MSG_GSUBUFMTCTX */
"proof and feature file format dumps not yet supported for lookup Type 6 chaining  context format %d.\n", /* SPOT_MSG_GSUBUFMTCCTX */
"proof and feature file format dumps do not support recursive calls to contextual lookups. context format %d.\n", /* SPOT_MSG_CNTX_RECURSION */
    "Duplicate glyph in coverage Type1. gid: '%d'.\n", /* SPOT_MSG_DUP_IN_COV */
    "Cannot proof multiple inputs with more than one group greater than 1. Not all substitutions may be displayed.\n", /* SPOT_MSG_GSUBMULTIPLEINPUTS */
};


const Byte8 *spotMsg(IntX msgId)
{
	if ((msgId <= 0) || (msgId > SPOT_MSG_ENDSENTINEL))
		return (0);
	return (EnglishMessages[msgId]);
}
