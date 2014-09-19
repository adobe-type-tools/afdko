/*
  cscan.h -- interface to intelligent center-scan fill
*/
/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

#ifndef	CSCAN_H
#define	CSCAN_H

#include "buildch.h"

/* CScanArgs -- Argument to ResetCScan  and FixedFltn */
typedef struct {
  void    *pClientHook;	/* this must be first. coordinate with t1interp.h */
  Fixed len1000;	/* Approximation of point size */
  Fixed idealwidth;	/* Actual stem width */
  DevBBoxRec *clipBBox;	/* if not NULL, rectangle to clip to */
  Fixed     flatness;
  PathProcs *path; 
  boolean fixupOK;	/* Do "fixup" (white connectivity) if appropriate */
  boolean locking;	/* Some part of the char may be "locked" in fontbuild */
  boolean offsetCenterFill;	/* True: do offset-center algorithm */
  void *ctx;        /* cscan context initialized by IniCScan */
} CScanArgs, *PCScanArgs;

/*
 * cscan procedures
 *	IniCScan -- called whenever a new Cscan context is needed.
 *	ResetCScan -- called once per character
 *	CSNewPoint -- moveto operation
 *	CSNewLine -- lineto operation
 *	CSClose -- closepath
 *	CScan -- endchar and rasterize
 */
/* Note:
 * The CScan proceedures may "RAISE" an error (usually BE_INVLCHARSTRING)
 * if an internal CScan error (or perhaps an input data error) is
 * detected. The client is responsible for providing the relevant
 * DURING HANDLER. Further, the client should treat this kind of error
 * as one for which a retry for this glyph/size will not work. Other
 * glyphs in the font may/may not work so this should not be treated
 * as a fatal error for the font.
 */
/* IniCScan sets up memory that will be used by the rest of cscan. 
 * The returned ctx should be put in the CScanArgs that is sent to the other
 * cscan calls.  zone should be an ASZone appropriate for allocating 
 * growable buffers
 */
extern procedure IniCScan (
  void ** ctx, 
  GrowableBuffer *  crossbuf, 
  GrowableBuffer * cscanbuffer, 
  ASZone zone);

extern procedure ResetCScan (PCScanArgs  resetArgs);
extern procedure CSNewPoint (FCd *  pc, PCScanArgs  arg);
extern procedure CSNewLine (FCd *  pc, PCScanArgs  arg);
extern procedure CSClose (PCScanArgs  arg);
extern IntX CScan (
  PCScanArgs arg,  
  RunRec  *  runData, 
  FCdBBox *  charBBox,
  PBCProcs   bcprocs,
  void    *  pClientHook
  );

/*
 * the fixed-point flattener
 */

extern procedure FixedFltn (FCd *  pc0, FCd *  pc1, FCd *  pc2, FCd *  pc3,
				    PCScanArgs   arg);

/*
 * from bcruns.c
 */

extern IntX ConvertBitMap 
  (
    BitMapRec *  pBitMap,
    RunRec *  pRun, 
    BCProcs *  procs,
    void    *  pClientHook
   );
extern IntX ConvertRuns 
  (
    RunRec *  pRun, 
    BitMapRec *  pBitMap, 
    BCProcs *  procs,
    Card16     gridRatio,
    void    *  pClientHook
  );
extern IntX MergeRuns 
  (
    BCRun *  run1, 
    BCRun *  run2,  
    IntX  xOffset, 
    IntX  yOffset,
    RunRec *  dest, 
    BCProcs *  procs,
    void    *  pClientHook
  );



/* These are used by the intellifont package when
   they call the cscan routines.
*/
#define BC_DURING DURING
#define BC_HANDLER HANDLER
#define BC_RERAISE RERAISE
#define BC_END_HANDLER END_HANDLER

#define BC_LimitCheck  (ecComponentPrivBase - BE_MEMORY)
#define BC_ExceptionCode Exception.Code
#endif /* CSCAN_H */
