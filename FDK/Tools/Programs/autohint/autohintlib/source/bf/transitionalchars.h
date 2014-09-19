/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/
#ifndef TRANSITIONALCHARS_H
#define TRANSITIONALCHARS_H

#include "masterfontpriv.h"
  
#define TRANSITIONALCHARFILENAME "transitionalchars"

typedef float Ary4[4];

typedef struct _Transitions {
  char *charname;
  short numtransitiongroups; 
  struct _TransitGroup *transitgrouparray; /* ARRAY */
} Transitions;

typedef struct _TransitGroup
{
  char **assembledfilename; /* ARRAY : one filename for each "corner" */
  int *assembled_width;  /* array */
  int *assembled_sb;     /* array */
  Bbox bbx;             /* not of the assembled file, but the super-bbox of all elts */
  int subrindex;        /* subroutine-index the _Merged_ assembledfilename's charpaths */
  short numentries;
  short hintindex;  /* which elt entry is -the- source for hints for this group */
  struct _TransitElt *transiteltarray; /* ARRAY */
  Ary4 minval, maxval;    /* sorted min/max of group's fromval/toval entries */
} TransitionGroup;

typedef struct _tt_pathlist *PTPathList;

typedef struct _TransitElt
{
  char *from_filename, *to_filename;
  short from_width, to_width;
  short from_sb, to_sb;
  Bbox from_bbx, to_bbx;
  PTPathList from_pathlist, to_pathlist; /* opaque */
  unsigned from_ishintdir:1, /* else "to" is hintdir for this pair */
           defaultbehavior:1, /* else, range is in fromval/toval */
           from_isanchored:1, /* if "from" is at a corner */
           to_isanchored:1;   /* if "to" is at a corner */
  Ary4 fromposit, toposit; /* position of the masters in cartesian space */
  Ary4 fromval, toval;     /* cartesian coords where the masters should be utilized */
} TransitElt;


extern void ReadTransitional (
boolean
);

extern void PreReadTransitional();

extern void CopyTransitionalToTable();

extern void AssembleTransitionals();

extern char **TransitionalCharsHintList(
boolean, short *, indx
);

extern void Free_TransitionalList();

extern void GetMastDesPosInfo(MasterDesignPosit **MDPptr, int *NumMast, int *NumAxes);

#endif /* TRANSITIONALCHARS_H */
