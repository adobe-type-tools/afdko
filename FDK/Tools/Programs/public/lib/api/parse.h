/*
  parse.h -- interface for parsing font files
*/
/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

#ifndef	PARSE_H
#define PARSE_H

#if defined COOLTYPE_ENV
/* COOLTYPE specific code here */
#ifndef PACKAGE_SPECS
#define PACKAGE_SPECS "package.h"
#endif

#include BUILDCH
#include ASZONE

#else  /* COOLTYPE_ENV */
/* standard coretype environment */

#include "buildch.h"
#include "supportaszone.h"

#endif /* COOLTYPE_ENV */
/*
 * The font parser justs parses.  It doesn't do storage allocation
 * since the strategies for storing font data will vary widely.  The
 * basic task involves filling out a FontDesc data structure (see
 * the interface to the buildch package).  Callbacks are used to
 * read the font file, report back various font information, and to
 * indicate data that is to be shared with other fonts.
 *
 * Most callback procedure return a boolean: a true result indicates that
 * everything is fine and the parsing will continue; false will abort the
 * parse immediately.  Any non-scalar data received via a callback must be
 * copied---it almost certainly will not stick around after the call.
 *
 * RANDOMACCESS means the parser gives back info on where CharStrings
 * and Subroutines are in the font file.  This can be used to retrieve
 * these things without reparsing the entire file.  The CharString
 * and Subroutine procs have three extra args for this:  the GetBytes
 * buffer count of the first char in the data, the offset into that
 * buffer, and the current value of the key for eexec decryption.  It
 * is the caller's responsibility to figure out how to use this
 * information to best advantage.  It will probably mean keeping some
 * amount of info about each GetByte buffer, etc.
 *
 * COMPOSITE_FONT means the parser will be used with composites fonts.  This
 * version supports several extra public interfaces and callbacks
 * which are discussed below.  [COMPOSITE_FONT parsing is neither documented
 * nor implemented well.  Part of this is because the organization,
 * which seems to work well for PostScript products, is very poor from
 * the perspective of ATM, and part of it is because I am unclear on how
 * all the parseprocs work and what they do.  Paul Haahr, 10 Nov 91.]
 */



/*********************** O P T I O N S ***********************/

/* Options for parsing should be set in the appropriate config file.  These
   are the defaults to use when, for some reason, they are not set in the
   config file.  Defaults for "Kanji" options were taken from ATM-J Mac */

/* ORIGINAL_FONT -- flag used in some mysterious way with OCF */

#ifndef ORIGINAL_FONT
#define ORIGINAL_FONT 1
#endif


/* FAST_GETTOKEN -- if true, GetToken doesn't check for buffer overflow in
   tokens other than charstrings.  Note that overflow can only occur if a
   token is longer than TOKENBUFFER_MIN from parse.h, which should be >= 1024.
   (I strongly recommend turning this off.  (Who is "I"?))  */

#ifndef FAST_GETTOKEN
#define FAST_GETTOKEN 1
#endif


/* COPYPROTECT -- some composite fonts have special copy protection.  This
   flag is therefor always on for composite fonts. */

#ifndef COPYPROTECT
#define COPYPROTECT 1
#endif


/* HEXENCODING -- if true, the parser will be able to deal with hex format
   eexec fonts; the only fonts that are really in this format are
   uncompressed fonts on unix machines. */

#ifndef HEXENCODING
#define HEXENCODING 1
#endif


/* COMPOSITE_FONT -- if true, the parser supports the postscript composite 
   font extensions.  Older code uses the KANJI flag to indicate that
   composite fonts are desired.  Newer code uses the slightly more accurate
   DBCS flag. */

#ifndef	COMPOSITE_FONT
/* Default value depends on these 2 switches */
#ifdef DBCS				/* OEMATM uses the DBCS switch */
#define COMPOSITE_FONT DBCS
#endif

#ifdef KANJI 		/* MAC ATM(?) and WIN ATM use the KANJI switch */
#define COMPOSITE_FONT KANJI
#endif

#ifndef COMPOSITE_FONT			/* No other default applied */
#define	COMPOSITE_FONT	0
#endif

#endif /* COMPOSITE_FONT */

/* RANDOMACCESS -- if true, the callbacks for CharStrings are passed the 
   GetBytes call *	number, character index, and current eexec decryption 
   key in order that it can seek directly to the appropriate place in the 
   font file.  */

#ifndef	RANDOMACCESS
#define	RANDOMACCESS	0
#endif



/******************* P A R S E   P R O C S *******************/


typedef struct _t_ParseProcs {

/*
 * interface to operating system services
 */

  /*
   * GetBytes -- read bytes to parse
   *	a false return indicates some sort of I/O problem; early (unexpected)
   *	end of file being the most common.  EOF is indicated by a return of
   *	0 bytes read.  The client must write the his
   *    pointer to the data into *hndl.  Also, if RANDOMACCESS is set,
   *    the client should know that the number of calls to (*GetBytes),
   *    is referred to later by the 'buffNum' arg to various callbacks.
   */
  boolean (*GetBytes) (char ***  hndl, CardX *  len, void* clientHook);

/*
 * Encoding procedures
 */

  boolean (*UseStandardEncoding) (void* clientHook);
  boolean (*UseSpecialEncoding) (IntX  count, void* clientHook);
  boolean (*SpecialEncoding) (IntX  i, char *  charname, void* clientHook);

  boolean (*UseStandardAccentEncoding) (void* clientHook);
  boolean (*UseSpecialAccentEncoding) (IntX  count, void* clientHook);
  boolean (*SpecialAccentEncoding) (IntX  i, char *  charname, void* clientHook);

/*
 * Font information procedures
 */

  boolean (*StartEexecSection) (void* clientHook);
  boolean (*FontType) (Int32  fontType, void* clientHook);
  boolean (*FontName) (char *  name, void* clientHook);
  boolean (*Notice) (char *  str, void* clientHook);
  boolean (*FullName) (char *  name, void* clientHook);
  boolean (*FamilyName) (char *  name, void* clientHook);
  boolean (*Weight) (char *  str, void* clientHook);
  boolean (*version) (char *  str, void* clientHook);
  boolean (*ItalicAngle) (Fixed  p, void* clientHook);
  boolean (*isFixedPitch) (boolean  flg, void* clientHook);
  boolean (*UnderlinePosition) (Fixed  p, void* clientHook);
  boolean (*UnderlineThickness) (Fixed  p, void* clientHook);
  boolean (*PublicUniqueID) (Int32  id, void* clientHook);
  boolean (*FSType) (Int32 n, void* clientHook);
  boolean (*OrigFontType) (char * str, void* clientHook);


/*
 * CharStrings
 *	AllocCharStrings is first called with an upper bound on the number
 *	of charstrings.  then CharString is called for each charstring;
 *	if RANDOMACCESS is on, enough information is returned that the
 *	caller can recreate the necessary state to read from the middle of
 *	the file.  ShareCharStrings is called before any calls to indicate
 *	that the CharStrings are identical to those from the named font;
 *	if the caller of ParseFont can get to those outlines, it should
 *	return true and the charstrings will be skipped
 */

  boolean (*AllocCharStrings) (IntX  count, void* clientHook);
#if RANDOMACCESS
  boolean (*CharString) (CardX  buffNum, CardX  byteNum, Card32  cryptNum,
  				 char *  key, CharDataPtr  val, CardX  len, void* clientHook);
#else
  boolean (*CharString) (char *  key, CharDataPtr  val, CardX  len, void* clientHook);
#endif
  boolean (*ShareCharStrings) (char *  fontname, void* clientHook);

/*
 * Subroutines
 *	used similarly to the CharStrings procedures.  if AllocSubroutines
 *	gets called with 0, it means that the number of subroutines is not known
 *	ahead of time.  It should be assumed that the number of subroutines is
 *	at least 200.
 */

  boolean (*AllocSubroutines) (IntX  count, void* clientHook);
  boolean (*Subroutine) (IntX  index, CharDataPtr  val, CardX  len, void* clientHook);
  boolean (*ShareSubroutines) (char *  fontname, void* clientHook);

/*
 * multiple master fonts
 *
 * BlendDesignMapping is called once for each axis.
 */

  boolean (*WeightVector) (Fixed *  wv, IntX  wvlen, void* clientHook);
  boolean (*ResizeFontDesc) (FontDesc **  fontdescp, Card32  size, void* clientHook);
  boolean (*BlendNumberDesigns) (CardX  n, void* clientHook);
  boolean (*BlendNumberAxes) (CardX  n, void* clientHook);

  boolean (*BlendAxisType) (CardX  axis, char *  name, void* clientHook);
  boolean (*BlendDesignMapping) (CardX  axis, int  n, Fixed *  from, Fixed *  to, void* clientHook);
  boolean (*BlendDesignPositions) (int  master, Fixed *  dv, void* clientHook);  /* called AFTER 
                                                 BlendNumberDesigns and BlendNumberAxes */
  boolean (*BlendUnderlinePosition) (int  master, Fixed  x, void* clientHook);
  boolean (*BlendUnderlineThickness) (int  master, Fixed  x, void* clientHook);
  boolean (*BlendItalicAngle) (int  master, Fixed  x, void* clientHook);

#if COMPOSITE_FONT
/*
 * COMPOSITE_FONT/composite fonts
 *	TODO: document these
 *
 * procedures that take length arguments can also take -1 to imply
 * an unknown length until the end of the array is seen.  when the
 * call is made with UNKNOWN_LENGTH, the elements will follow, and
 * then an "allocation" call will be made again with the actual
 * length after all the elements have been called.  thus, the
 * /PGFArray field can be filled either with the sequence:
 *
 *	(*procs->AllocPGFArray)(2);
 *	(*procs->PGFArray)(0, "GothicBBB-Medium::JIS83-1");
 *	(*procs->PGFArray)(1, "GothicBBB-Medium::Symbol");
 *
 * or with
 *
 *	(*procs->AllocPGFArray)(UNKNOWN_LENGTH);
 *	(*procs->PGFArray)(0, "GothicBBB-Medium::JIS83-1");
 *	(*procs->PGFArray)(1, "GothicBBB-Medium::Symbol");
 *	(*procs->AllocPGFArray)(2);
 *
 * This is only true in the current implementation for PGFArray,
 * FDepVector, and MDFV; not, for example, any CID "procedures that take length
 * arguments".
 */

#define	UNKNOWN_LENGTH	(-1)

  /* extra encoding support */
  boolean (*UseNamedEncoding) (char *  name, void* clientHook);

  /* renderer/metrics support for COMPOSITE_FONT */
  boolean (*WritingMode) (Int32  wmode, void* clientHook);
  boolean (*CDevProc) (int  cdev, void* clientHook);

  /* composite font features */
  boolean (*OriginalFont) (char *  name, void* clientHook);
  boolean (*FMapType) (IntX  fmt, void* clientHook);
  boolean (*EscChar) (Int32  escape, void* clientHook);
  boolean (*SubsVector) (IntX  len, CharDataPtr  svec, void* clientHook);
  boolean (*AllocFDepVector) (IntX  len, void* clientHook);
  boolean (*FDepVector) (IntX  i, char *  name, void* clientHook);
  boolean (*AllocPGFArray) (IntX  len, void* clientHook);
  boolean (*PGFArray) (IntX  i, char *  name, void* clientHook);
  boolean (*CharOffsets) (IntX  len, CharDataPtr  cp, void* clientHook);
  boolean (*UseNamedCharStrings) (char *  name, void* clientHook);
  boolean (*PrefEnc) (char *  name, void* clientHook);
  boolean (*NumericEncoding) (IntX  index, IntX  n, void* clientHook);

  /* fast composite fonts */
  boolean (*MDID) (Int32  id, void* clientHook);
  boolean (*AllocMDFV) (IntX  len, void* clientHook);
  boolean (*MDFVBegin) (IntX  i, char *  encoding, char *  charstrings, FCdBBox *  bbox, void* clientHook);
  boolean (*MDFVFont) (IntX  index, char *  name, void* clientHook);
  boolean (*MDFVEnd) (IntX  numfonts, FracMtx *  mtx, IntX  cdevproc, void* clientHook);
  boolean (*FDepVector_MDFF) (IntX  i, char *  name, boolean  flag,
				      IntX  mdfv, IntX  len, CharDataPtr  cp, void* clientHook);

  /* primogenital font support */
  boolean (*PGFontID) (Int32  id, void* clientHook);

  /* specifier for alternate rasterizer (eCCRun -> double encryption) */
  boolean (*RunInt) (char *  key, void* clientHook);

#if COPYPROTECT
  boolean (*FontProtection) (IntX  len, CharDataPtr  cp, void* clientHook);
#endif


  /* new composite font support */
  boolean (*GDBytes) (Int32  n, void* clientHook);
  boolean (*FDBytes) (Int32  n, void* clientHook);
  boolean (*CIDCount) (Int32  n, void* clientHook);
  boolean (*CIDMapOffset) (Int32  n, void* clientHook);
  boolean (*CIDFontVersion) (Int32  n, void* clientHook);
  boolean (*Registry) (char *  name, void* clientHook);
  boolean (*Ordering) (char *  name, void* clientHook);
  boolean (*Supplement) (Int32  n, void* clientHook);
  boolean (*FDArrayFontName) (char *  name, void* clientHook);
  boolean (*CIDFDArray) (Int32  n, void* clientHook);             /* number of FontDict's */
  boolean (*BeginCIDFontDict) (Int32  i, FontDesc **  fontp, void* clientHook);
  boolean (*EndCIDFontDict) (Int32  i, void* clientHook);
  boolean (*CIDStartData) (CardX  buffNum, CardX  byteNum, void* clientHook);
  boolean (*UIDBase) (Int32  n, void* clientHook);
  boolean (*XUID) (IntX  n, Int32 *  v, void* clientHook);
  boolean (*GlyphDirectory) (void* clientHook);
  boolean (*WidthsOnly) (boolean widthsOnly, void* clientHook);

  /* CID (NCF) specific private dict support */ 

  boolean (*SDBytes) (Int32  n, void* clientHook);
  boolean (*SubrMapOffset) (Int32  n, void* clientHook);
  boolean (*SubrCount) (Int32  n, void* clientHook);


  /* encoding files and rearranged font */
  boolean (*cidrange) (Card32  srcCodeLo, IntX  srcCodeLoLen,
                               Card32  srcCodeHi, IntX  srcCodeHiLen, Card32  dstGlyphIDLo, void* clientHook);
  boolean (*cidchar) (Card32  srcCode, IntX  srcCodeLen, Card32  dstGlyphID, void* clientHook);
  boolean (*notdefchar) (Card32  srcCode, IntX  srcCodeLen, Card32  dstGlyphID, void* clientHook); 
  boolean (*notdefrange) (Card32  srcCodeLo, IntX  srcCodeLoLen, 
                                  Card32  srcCodeHi, IntX  srcCodeHiLen, Card32  dstGlyphIDLo, void* clientHook);
  boolean (*codespacerange) (Card32  srcCode1, IntX  srcCode1Len, Card32  srcCode2, IntX  srcCode2Len, void* clientHook);
  boolean (*rearrangedfont) (char *  name, void* clientHook);
  boolean (*componentfont) (IntX  i, char *  name, void* clientHook);
  boolean (*endcomponentfont) (IntX  i, void* clientHook);
  boolean (*usematrix) (IntX  i, FixMtx  *m, void* clientHook);
  boolean (*usefont) (Card16  i, void* clientHook);
  boolean (*bfrange_code) (Card32  srcCodeLo, IntX  srcCodeLoLen,
                           Card32  srcCodeHi, IntX  srcCodeHiLen, 
                           Card32  dstCodeLo, IntX  dstCodeLoLen, void* clientHook);
  boolean (*bfrange_name) (Card32  srcCodeLo, IntX  srcCodeLoLen, 
                           Card32  srcCodeHi, IntX  srcCodeHiLen, IntX  i, char *  dstChar, void* clientHook);
  boolean (*bfchar_code) (Card32  srcCode, IntX  srcCodeLen, 
                          Card32  dstCode, IntX  dstCodeLen, void* clientHook);
  boolean (*bfchar_name) (Card32  srcCode, IntX  srcCodeLen, char *  dstCharName, void* clientHook);
  boolean (*UseCMap) (char *  name, void* clientHook);
  boolean (*HostSupport) (char * name, void* clientHook);

#endif /* COMPOSITE_FONT */

  ASZone growbuffZone; /* analogous to BCProcs.growbuffZone in buildch.h */
} ParseFontProcs, *PParseFontProcs;

#if COMPOSITE_FONT
/* The minimum must be long enough to hold the longest token in the font. 
   For composite fonts longets token is the string containing the Character
   offset data. The data is 6 bytes for each character in a row. The longest
   therefore is 6 * 256 = 1536. */
#define TOKENBUFFER_MIN  1536
#else
#define TOKENBUFFER_MIN  1024
#endif
#define	TOKENBUFFER_GROW 1024

#define	MAXBLENDDESIGNMAP 64	/* maximum number of points in the design map */


#if COMPOSITE_FONT

#define	PGFontType	1000

/* types of CDevProc results */
#define	NoCDevProc	0
#define	StdCDevProc	1
#define	UnknownCDevProc	-1

#endif



/******************** I N T E R F A C E S ********************/


/*
 * InitParseTables
 *	call at the dawn of time to initialize internal data structures for
 *	the parser.
 *
 *	can return either PE_NOERR or PE_CANTHAPPEN
 */
#ifdef __cplusplus
extern "C" {
#endif

extern IntX InitParseTables ();


/*
 * ParseFont
 *	parse a font file.  the caller should have opened the file and
 *	should pass the contents to the ParseFont through the procs->GetBytes
 *	callback routine.
 */

extern IntX ParseFont (
  FontDesc **  fontdescp,	/* where to store the font data */
  ParseFontProcs *  procs,	/* Callback procs */
  GrowableBuffer *  tokBuffer,	/* Growable buffer to contain a token. */
  GrowableBuffer *  inBuf,	/* input lookaside buffer */
  void *  pClientHook		/* Client's data */
);

#if COMPOSITE_FONT
extern IntX ParseEncoding (
  ParseFontProcs *  parseprocs, /* callback procs */
  GrowableBuffer *  growbuf,    /* growable buffer to contain a token. */
  GrowableBuffer *  lookaside,  /* input lookaside buffer */
  void *  pClientHook		/* Client's data */
);
#endif

#ifdef __cplusplus
};
#endif

/* ParseFont return codes */
#define PE_EXITEARLY	  1     /* StartEexecSection returned false */
#define PE_NOERR	  0	/* No error */
#define PE_READ		 -1	/* I/O problem reading file. Probably EOF. */
#define PE_BIGTOKEN	 -2	/* Token too big */
#define PE_CALLER	 -3	/* Callback proc said abort (except GetBytes) */
#define PE_BADFILE	 -4	/* Found something unexpected in font file */
#define PE_BADFONTTYPE	 -5	/* Font is not type 0 or 1 */
#define PE_BUFFSMALL	 -6	/* Some buffer is too small */
#define PE_CARTBADSYNTH	 -7	/* A bad synthetic cartridge font. */
#define	PE_BADBLEND	 -8	/* an inconsistency in the number of blends */
#define	PE_CANTHAPPEN	 -9	/* assertion failure---internal problem */
#define	PE_BADNUMBER	-10	/* poorly formatted number */
#define PE_BADVERSION	-11	/* CID version information was inconsistent */

#endif	/* PARSE_H */
