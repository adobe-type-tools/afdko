/* @(#)CM_VerSion publictypes.h atm09 1.4 16563.eco sum= 57080 atm09.004 */
/* @(#)CM_VerSion publictypes.h atm08 1.4 16273.eco sum= 01329 atm08.003 */
/* @(#)CM_VerSion publictypes.h atm07 1.5 16186.eco sum= 34508 atm07.013 */
/* @(#)CM_VerSion publictypes.h atm06 1.11 14016.eco sum= 60180 */
/* @(#)CM_VerSion publictypes.h atm05 1.3 11580.eco sum= 13135 */
/* @(#)CM_VerSion publictypes.h atm04 1.8 07620.eco sum= 25666 */
/* $Header$ */
/*
  publictypes.h
*/
/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */


#ifndef	PUBLICTYPES_H
#define	PUBLICTYPES_H


#if !BC_NT
#include <stdio.h>                      /* Usually define NULL here */
#endif /* !BC_NT */

#include "supportenvironment.h"
/* include procs.h in middle of file */

#define	readonly const

#define	_inline
#define	_priv	extern

/* Control Constructs */

#define until(x) while (!(x))

#define endswitch default:;

/* Inline Functions */

#ifndef ABS
#define ABS(x) ((x)<0?-(x):(x))
#endif
#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

/* Declarative Sugar */

#define forward extern
#if defined(WINATM) && defined(MAKEPUB)
#define PRIVATE
#else
#define PRIVATE static
#endif
#define PUBLIC
#define global extern
#define procedure void

/* Switch in and out assembly portions */

#ifndef	USE68KMATRIX
#define USE68KMATRIX		0
#endif	/* USE68KMATRIX */

#ifndef	USE68KCSCAN
#define USE68KCSCAN	0
#endif	/* USE68KCSCAN */

#ifndef USE68KFONTBUILD
#define USE68KFONTBUILD		0
#endif	/* USE68KFONTBUILD */

#if ! TYPEDEF_boolean           /* Mac environment has its own typedef */
typedef unsigned int boolean;
#endif

#define true 1
#define false 0

/* Various size integers and pointers to them.  Since these
   must be compatible with definitions in the 20XX worlds,
   they have been copied verbatim.  For the meaning of the 
   various types, refer to comments in 20XX world's publictypes.h.

   From the FAQ of comp.lang.c:

        Some vendors of C products for 64-bit machines support 64-bit
        long ints.  Others fear that too much existing code depends on
        sizeof(int) == sizeof(long) == 32 bits, and introduce a new 64- bit
        long long (or __longlong) type instead.
  */

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
/* If C version is known and C99 or above, define types based on inttypes */
#include <stdint.h>
typedef uint8_t		Card8, *PCard8;
#if UNSIGNEDCHARS	/* see environment.h */
typedef int8_t		Int8, *PInt8;
#else /* UNSIGNEDCHARS */
typedef int8_t			Int8, *PInt8;	/* some compilers having a signed "char" treat "signed char" as a different type */
#endif /* UNSIGNEDCHARS */
#if FORCEsignedChar8
typedef int8_t             Char8;
#elif FORCEunsignedChar8
typedef uint8_t           Char8;
#else
typedef int8_t                    Char8;
#endif

#define MAXCard8		((Card8)0xFF)

typedef int16_t		Int16, *PInt16;
#define MAXInt16		((Int16)0x7FFF)
#define MINInt16		((Int16)0x8000)

typedef uint16_t	Card16, *PCard16;
#define MAXCard16		((Card16)0xFFFF)

/*
 * Under 16-bit x86 compilers, the type of a pointer difference is a 
 * signed 16 bits; such an expression may be immediately cast to CardPtrDiff
 * or IntPtrDiff without undesired side-effects.
 */
typedef unsigned long int       CardPtrDiff;    /* unsigned pointer difference */
typedef long int                IntPtrDiff;     /* signed pointer difference */

typedef int32_t			Int32, *PInt32;  /* compiler-dependent? */

typedef uint32_t		Card32, *PCard32;/* compiler-dependent? */

typedef int64_t		Int64, *PInt64;
#define MAXInt64		((Int64)0x7FFFFFFFFFFFFFFF)
#define MINInt64		((Int64)0x8000000000000000)

typedef uint64_t	Card64, *PCard64;
#define MAXCard64		((Card64)0xFFFFFFFFFFFFFFFF)

typedef uint64_t		longcardinal;
typedef int32_t			integer;
typedef uint32_t		unsignedinteger;

#define MAXInt32		((Int32) 0x7FFFFFFF)
#define MINInt32		((Int32) 0x80000000)
#define MAXCard32		((Card32)0xFFFFFFFF)

typedef uint16_t	cardinal;
#define MAXcardinal		((cardinal)    0xFFFF)
#define MAXlongcardinal		((longcardinal)0xFFFFFFFF)

#define MAXinteger		((integer)        0x7FFFFFFF)
#define MINinteger		((integer)        0x80000000)
#define MAXunsignedinteger	((unsignedinteger)0xFFFFFFFF)

typedef uint64_t	longlongcardinal;       /* longest supported cardinal type */
typedef int64_t		longinteger;            /* longest supported integer type */
typedef uint64_t	unsignedlonginteger;    /* longest supported cardinal type */

#define MAXlonglongcardinal	((longlongcardinal)   0xFFFFFFFFFFFFFFFF)
#define MAXlonginteger		((longinteger)        0x7FFFFFFFFFFFFFFF)
#define MINlonginteger		((longinteger)	      0x8000000000000000)
#define MAXunsignedlonginteger	((unsignedlonginteger)0xFFFFFFFFFFFFFFFF)

#else /* __STDC__VERSION__ */
typedef unsigned char		Card8, *PCard8;
#if UNSIGNEDCHARS	/* see environment.h */
typedef signed char		Int8, *PInt8;
#else /* UNSIGNEDCHARS */
typedef char			Int8, *PInt8;	/* some compilers having a signed "char" treat "signed char" as a different type */
#endif /* UNSIGNEDCHARS */
#if FORCEsignedChar8
typedef signed char             Char8;
#elif FORCEunsignedChar8
typedef unsigned char           Char8;
#else
typedef char                    Char8;
#endif

#define MAXCard8		((Card8)0xFF)

typedef short int		Int16, *PInt16;
#define MAXInt16		((Int16)0x7FFF)
#define MINInt16		((Int16)0x8000)

typedef unsigned short int	Card16, *PCard16;
#define MAXCard16		((Card16)0xFFFF)

/*
 * Under 16-bit x86 compilers, the type of a pointer difference is a 
 * signed 16 bits; such an expression may be immediately cast to CardPtrDiff
 * or IntPtrDiff without undesired side-effects.
 */
typedef unsigned long int       CardPtrDiff;    /* unsigned pointer difference */
typedef long int                IntPtrDiff;     /* signed pointer difference */

#if (ARCH_64BIT == 1)

typedef int			Int32, *PInt32;  /* compiler-dependent? */

typedef unsigned int		Card32, *PCard32;/* compiler-dependent? */

typedef long int		Int64, *PInt64;
#define MAXInt64		((Int64)0x7FFFFFFFFFFFFFFF)
#define MINInt64		((Int64)0x8000000000000000)

typedef unsigned long int	Card64, *PCard64;
#define MAXCard64		((Card64)0xFFFFFFFFFFFFFFFF)

typedef unsigned int		longcardinal;
typedef int			integer;
typedef unsigned int		unsignedinteger;

#else /* (ARCH_64BIT == 1) */

typedef long int		Int32, *PInt32;

typedef unsigned long int	Card32, *PCard32;

typedef unsigned long int	longcardinal;
typedef long int		integer;
typedef unsigned long int	unsignedinteger;

#endif /* (ARCH_64BIT == 1) */

#define MAXInt32		((Int32) 0x7FFFFFFF)
#define MINInt32		((Int32) 0x80000000)
#define MAXCard32		((Card32)0xFFFFFFFF)

typedef unsigned short int	cardinal;
#define MAXcardinal		((cardinal)    0xFFFF)
#define MAXlongcardinal		((longcardinal)0xFFFFFFFF)

#define MAXinteger		((integer)        0x7FFFFFFF)
#define MINinteger		((integer)        0x80000000)
#define MAXunsignedinteger	((unsignedinteger)0xFFFFFFFF)

typedef unsigned long int	longlongcardinal;       /* longest supported cardinal type */
typedef long int		longinteger;            /* longest supported integer type */
typedef unsigned long int	unsignedlonginteger;    /* longest supported cardinal type */

#if (ARCH_64BIT == 1)

#define MAXlonglongcardinal	((longlongcardinal)   0xFFFFFFFFFFFFFFFF)
#define MAXlonginteger		((longinteger)        0x7FFFFFFFFFFFFFFF)
#define MINlonginteger		((longinteger)	      0x8000000000000000)
#define MAXunsignedlonginteger	((unsignedlonginteger)0xFFFFFFFFFFFFFFFF)

#else /* (ARCH_64BIT == 1) */

#define MAXlonglongcardinal	((longlongcardinal)   0xFFFFFFFF)
#define MAXlonginteger		((longinteger)        0x7FFFFFFF)
#define MINlonginteger		((longinteger)        0x80000000)
#define MAXunsignedlonginteger	((unsignedlonginteger)0xFFFFFFFF)

#endif /* (ARCH_64BIT == 1) */
#endif /* __STDC_VERSION__ */

#if 0 /* XXX */
typedef integer (*PIntProc)();	/* pointer to procedure returning integer */
#endif

/* The following definitions of CardX and IntX differ from "int" and 
 * "unsigned int" in that they mean  "use whatever width you like".  
 * It is assumed to be at least 16 bits. If FORCE16BITS is true 
 * these types will be forced to 16 bits (short int). If FORCE32BITS
 * is true these types will be forced to 32 bits.
 */
#ifndef	FORCE16BITS
#define	FORCE16BITS	0
#endif	/* FORCE16BITS */

#ifndef FORCE32BITS
#define FORCE32BITS     0
#endif  /* FORCE32BITS */

#if FORCE16BITS
typedef unsigned short int CardX;
typedef short int IntX;
#elif FORCE32BITS
typedef unsigned long CardX;
typedef long IntX;
#else
typedef unsigned int CardX;
typedef int IntX;
#endif

/* Bit fields */

typedef unsigned BitField;

/* Reals and Fixed point */

typedef float real, *Preal;
typedef double longreal;

#if !MACROM || __GNUC__         /* MACROM is defined only for 68k Mac builds */
#if (ARCH_64BIT == 1)
typedef      int Fixed; /*  16 bits of integer, 16 bits of fraction */
#else /* (ARCH_64BIT == 1) */
typedef long int Fixed;	/*  16 bits of integer, 16 bits of fraction */
#endif /* (ARCH_64BIT == 1) */
#endif
typedef Fixed *PFixed;

/* Co-ordinates */

#if PSENVIRONMENT
/*
 * The PostScript world defines the following types.  None of the
 * Type 1 packages need them, and there is a name clash with a standard
 * Macintosh type, so we hide the definitions.
 */
typedef real Component;

typedef struct _t_RCd {
  Component x,
            y;
} RCd, *PRCd;
#endif

typedef struct _t_FCd {
  Fixed x,
        y;
} FCd, *PFCd;

typedef struct _t_DevCd {
  Int16   x,
          y;
} DevCd, *PDevCd;

typedef struct _t_DevBBoxRec {
  DevCd   bl,
          tr;
  } DevBBoxRec, *PDevBBox;

/* Characters and strings */

typedef unsigned char character, *string, *charptr;

/* Inline string function from C library */

#define	NUL	'\0'

/* Generic Pointers */

#ifndef NULL
#define	NULL	0
#endif

#define	NIL	NULL

typedef	void (*PVoidProc)();	/* Pointer to procedure returning no result */

#if 0   /* XXX */
/* Codes for exit system calls */

#if VAXC
#define exitNormal 1
#define exitError 2
#else
#define exitNormal 0
#define exitError 1
#endif
#endif  /* 0 XXX */

#if 0			/* Don't think we need this... MWB */
/* Generic ID for contexts, spaces, name cache, etc. */

#define	BitsInGenericIndex	10
#define	BitsInGenericGeneration	(32 - BitsInGenericIndex)

#define	MAXGenericIDIndex	((Card32)((1 << BitsInGenericIndex) - 1))
#define	MAXGenericIDGeneration	((Card32)((1 << BitsInGenericGeneration) - 1))

typedef union {
  struct {
    BitField index:BitsInGenericIndex;		/* Reusable component */
    BitField generation:BitsInGenericGeneration;/* Non-reusable component */
  } id;
  Card32  stamp;		/* Unique combined id (index, generation) */
} GenericID;			/* For contexts, spaces, streams, names, etc */
#endif


/* PROCS.H */
/*#include "procs.h"*/



#endif	/* PUBLICTYPES_H */
