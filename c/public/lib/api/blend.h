/* @(#)CM_VerSion blend.h atm09 1.2 16563.eco sum= 33799 atm09.004 */
/* @(#)CM_VerSion blend.h atm08 1.6 16380.eco sum= 02355 atm08.007 */
/* @(#)CM_VerSion blend.h atm07 1.4 16131.eco sum= 11163 atm07.012 */
/* @(#)CM_VerSion blend.h atm05 1.3 11580.eco sum= 36804 */
/* @(#)CM_VerSion blend.h atm04 1.4 06339.eco sum= 46474 */
/* $Header$ */
/*
  blend.h -- interface to weight vector computations
*/
/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

#ifndef	BLEND_H
#define	BLEND_H

#include "supportaszone.h"

#ifndef	MAXAXES
#define	MAXAXES		15	/* maximum number of dimensions of the design space */
#endif /* MAXAXES */

#ifndef MAXWEIGHT
#define	MAXWEIGHT	16	/* maximum number of elements in the weight vector */
#endif /* MAXWEIGHT */

#define MAXFONTFIT	2	/* maximum number of axes that FontFit() can handle */

/* DS_allocfunc -- type of a DesignSpace allocation function, ARGDECL workaround */
typedef void *(*DS_allocfunc) (IntX  nbytes, void *  tag);

/* DesignSpace -- the normalized design space for a particular font */
typedef struct DesignSpace DesignSpace;

extern DesignSpace *MakeDesignSpace (
  IntX  ndimen,			/* the number of axes */
  IntX  nmasters,		/* the number of designs in the master */
  DS_allocfunc  alloc,      	/* a memory allocation function */
  void *  tag			/* a tag to be passed to the allocator */
);

extern boolean SetMasterDesignPosition (DesignSpace *  ds, CardX  n, Fixed *  dv);

/*  
 * If ndvSubroutine and cdvSubroutine are both NULL, then an old-style
 * 2^ds->ndimen mapping will be attempted; *ndv will be read and *wv
 * will be written.  lenBuildCharArray, SureMalloc and Free will
 * not be examined.
 * 
 * If ndvSubroutine is non-NULL then *udv will be read and *ndv will be
 * written.  If cdvSubroutine is non-NULL, then it will be
 * executed; if cdvSubroutine is NULL and wv is non-NULL, an old-style
 * 2^ds->ndimen mapping will be attempted.  Successful conversion to a
 * weight vector by either means causes a write to *wv.
 * 
 * Unless the storage pointed to by udv, ndv, or wv is specified here as
 * being read or written, the pointer will not be examined.
 * 
 * lenBuildCharArray must be greater than the highest-numbered BCA entry
 * addressed by *ndvSubroutine or *cdvSubroutine.
 * 
 * Neither SureMalloc nor Free are permitted to raise exceptions,
 * that is, to call longjmp().  SureMalloc may indicate failure to
 * allocate storage only by returning NULL.
 * 
 * XXX: Should GetWeightVector() be enhanced to be able to convert 
 *      UDV to NDV according to a Type 1 /BlendDesignMap array?
 */

extern boolean GetWeightVector (
  DesignSpace *  ds,	        /* design space structure pointer */
  Fixed *       wv,		/* weight vector (W), maximal length */
  Fixed *       ndv,		/* normalized design vector (RW) */
  Fixed *       udv,		/* user design vector (R) */
  Card16        charStringType, /* (R) */
  Card8 *       cdvSubroutine,  /* pointer to CDV subroutine */
  Card8 *       ndvSubroutine,  /* pointer to NDV subroutine */
  Fixed *       blendDesignMap, /* array of pairs: {user, normalized} */
  CardX         lenBuildCharArray, /* how much storage might be required? */
  ASZone	oneSizeZone, /* analogous to BCProcs.oneSizeZone in buildch.h */
  void  *       pClientHook	/* passed through to Memory callbacks */

);

/*
 * FontFit -- multiple master contraint matching
 */

typedef struct {
  Fixed lo, hi;		/* in normalized design vector space */
} FontFitBounds;

extern boolean FontFit (
  DesignSpace * 	ds,
  Fixed * 		dv,
  Fixed * 		wv,
  IntX 			nvary,
  Int16 * 		vary,
  FontFitBounds * 	clamp,
  Fixed * 		target,
  Fixed ** 		metric
);

typedef struct {
  Fixed			origdv[MAXAXES];
  Int16			nvary;
  Int16			vary[MAXFONTFIT];
  FontFitBounds		clamp[MAXFONTFIT];
  Int16			npreset;
  Fixed			target[MAXFONTFIT-1];
  Fixed			collapsed[MAXFONTFIT-1][1 << MAXFONTFIT];
  Int16			nextrema[MAXFONTFIT-1];
  FCd			extrema[MAXFONTFIT-1][1 << MAXFONTFIT];
} FontFitDomain;

extern boolean SetupFontFit (
  FontFitDomain * 	domain,
  DesignSpace * 	ds,
  Fixed * 		origdv,
  IntX 			nvary,
  Int16 * 		vary,
  FontFitBounds * 	clamp,
  IntX 			npreset,
  Fixed * 		target,
  Fixed ** 		metric
);

extern boolean SolveFontFit (
  FontFitDomain * 	domain,
  DesignSpace * 	ds,
  Fixed * 		dv,
  Fixed * 		wv,
  Fixed * 		target,
  Fixed ** 		metric
);

typedef struct {
  Fixed			lo0, lo1, hi0, hi1;	/* clamp.lo @ 0, clamp.lo @ 1, ... */
} FontFitExtra;

extern boolean ExtrapolateFontFit (
  FontFitDomain * 	domain,
  DesignSpace * 	ds,
  Fixed * 		dv,
  Fixed * 		wv,
  Fixed * 		target,
  Fixed ** 		metric,
  FontFitExtra * 	extra
);

#endif	/* BLEND_H */
