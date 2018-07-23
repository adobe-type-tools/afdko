/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/
/* charpath.h */

#ifndef CHARPATH_H
#define CHARPATH_H

typedef struct _t_hintelt {
  struct _t_hintelt *next;
  short int type; /* RB, RY, RM, RV */
  Fixed leftorbot, rightortop;
  long pathix1, pathix2;
  } HintElt, *PHintElt;
  
typedef struct _t_charpathelt {
  short int type; /* RMT, RDT, RCT, CP */
  /* the following fields must be cleared in charpathpriv.c/CheckPath */
  boolean isFlex:1, sol:1, eol:1, remove:1;
  int unused:12;
  PHintElt hints;
  Fixed x, y, x1, y1, x2, y2, x3, y3; /* absolute coordinates */
  long rx, ry, rx1, ry1, rx2, ry2, rx3, ry3;  /* relative coordinates */
  } CharPathElt, *PCharPathElt;

typedef struct _t_pathlist {
  PCharPathElt path;
  PHintElt mainhints;
  long sb;
  short width;
} PathList, *PPathList;

extern long path_entries;  /* number of elements in a character path */
extern boolean addHints;  /* whether to include hints in the font */

extern PCharPathElt AppendCharPathElement(int);

extern boolean CheckCharPath(char *, char *);

extern short GetOperandCount(short);

extern void GetLengthandSubrIx(short, short *, short *);

extern void ResetMaxPathEntries(void);

extern void SetCurrPathList(PPathList);

extern void SetHintsDir(indx);

extern int GetHintsDir(void);

extern void SetHintsElt(short, CdPtr, long, long, boolean);

extern void SetNoHints(void);

#endif /*CHARPATH_H*/

