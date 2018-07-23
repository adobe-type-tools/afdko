/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)common.h	1.11
 * Changed:    7/1/98 10:39:29
 ***********************************************************************/

/*
 * Shared format definitions.
 */

#ifndef FORMAT_COMMON_H
#define FORMAT_COMMON_H

/* Apple's definitions */
typedef Int32	Fixed;
#define	FWord	Int16
#define	uFWord	Card16
#define F2Dot14	Int16
#define F4Dot12 Int16
#define VERSION(a,b) \
	(((Int32)(a)<<16)|(b)<<12) /* Strange but true, thanks Apple */

/* Return field size from declaration (definition not needed). Not strictly
   portable but too useful to ignore! */
#define SIZEOF(s,f) sizeof(((s *)0)->f)

typedef Card16 GlyphId;			/* A Glyph id; 0- .notdef, 0xffff- undefined */

#ifndef TTO_H
typedef struct _Lookup *Lookup;	/* Opaque pointer to Apple Lookup table */
#endif

/* Table tags */
#define TAG(a,b,c,d) ((Card32)(a)<<24|(Card32)(b)<<16|(c)<<8|(d))

#define ALMX_ TAG('A','L','M','X')
#define BASE_ TAG('B','A','S','E')
#define BBOX_ TAG('B','B','O','X')
#define BLND_ TAG('B','L','N','D')
#define CFF__ TAG('C','F','F',' ')
#define CID__ TAG('C','I','D',' ')
#define CNAM_ TAG('C','N','A','M')
#define CNPT_ TAG('C','N','P','T')
#define CSNP_ TAG('C','S','N','P')
#define EBDT_ TAG('E','B','D','T')
#define DSIG_ TAG('D','S','I','G')
#define EBLC_ TAG('E','B','L','C')
#define ENCO_ TAG('E','N','C','O')
#define FNAM_ TAG('F','N','A','M')
#define FNTP_ TAG('F','N','T','P')
#define GDEF_ TAG('G','D','E','F')
#define GLOB_ TAG('G','L','O','B')
#define GPOS_ TAG('G','P','O','S')
#define GSUB_ TAG('G','S','U','B')
#define HFMX_ TAG('H','F','M','X')
#define JSTF_ TAG('J','S','T','F')
#define KERN_ TAG('K','E','R','N')
#define LTSH_ TAG('L','T','S','H')
#define META_ TAG('M','E','T','A')
#define METR_ TAG('M','E','T','R')
#define MMFX_ TAG('M','M','F','X')
#define MMSD_ TAG('M','M','S','D')
#define MMVR_ TAG('M','M','V','R')
#define OS_2_ TAG('O','S','/','2')
#define OTTO_ TAG('O','T','T','O')
#define PNAM_ TAG('P','N','A','M')
#define ROTA_ TAG('R','O','T','A')
#define SING_ TAG('S','I','N','G')
#define SUBS_ TAG('S','U','B','S')
#define TYP1_ TAG('T','Y','P','1')
#define VFMX_ TAG('V','F','M','X')
#define WDTH_ TAG('W','D','T','H')
#define bdat_ TAG('b','d','a','t')
#define bhed_ TAG('b','h','e','d')
#define bits_ TAG('b','i','t','s')
#define bloc_ TAG('b','l','o','c')
#define bsln_ TAG('b','s','l','n')
#define cmap_ TAG('c','m','a','p')
#define cvt__ TAG('c','v','t',' ')
#define fbit_ TAG('f','b','i','t')
#define fdsc_ TAG('f','d','s','c')
#define feat_ TAG('f','e','a','t')
#define fvar_ TAG('f','v','a','r')
#define fpgm_ TAG('f','p','g','m')
#define gasp_ TAG('g','a','s','p')
#define glyf_ TAG('g','l','y','f')
#define hdmx_ TAG('h','d','m','x')
#define head_ TAG('h','e','a','d')
#define hhea_ TAG('h','h','e','a')
#define hmtx_ TAG('h','m','t','x')
#define just_ TAG('j','u','s','t')
#define kern_ TAG('k','e','r','n')
#define lcar_ TAG('l','c','a','r')
#define loca_ TAG('l','o','c','a')
#define maxp_ TAG('m','a','x','p')
#define mort_ TAG('m','o','r','t')
#define name_ TAG('n','a','m','e')
#define opbd_ TAG('o','p','b','d')
#define post_ TAG('p','o','s','t')
#define prep_ TAG('p','r','e','p')
#define prop_ TAG('p','r','o','p')
#define sfnt_ TAG('s','f','n','t')
#define trak_ TAG('t','r','a','k')
#define true_ TAG('t','r','u','e')
#define ttcf_ TAG('t','t','c','f')
#define typ1_ TAG('t','y','p','1')
#define mor0_ TAG('m','o','r','0')
#define vhea_ TAG('v','h','e','a')
#define vmtx_ TAG('v','m','t','x')
#define VORG_ TAG('V','O','R','G')


#endif /* FORMAT_COMMON_H */



