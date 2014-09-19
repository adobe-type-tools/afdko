/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************
 * SCCS Id:    @(#)pathbuild.h	1.2
 * Changed:    6/26/98 11:17:38
 ***********************************************************************/

#if __bool_true_false_are_defined
typedef bool boolean;
#else
typedef enum {false, true} boolean;
#endif

#define FREENONNULL(p) if (p != NULL) {memFree((char *) p); p = NULL;}

/* define types for moveto, lineto, curveto, line-closepath, point-closepath */
#define MTtype 001
#define DTtype 002
#define CTtype 003
#define LCPtype 004
#define PCPtype 005


#define ISMTorCPTYPE(pe) (((pe)->elttype == MTtype) || ((pe)->elttype == PCPtype) || ((pe)->elttype == LCPtype))
#define ISMTTYPE(pe) ((pe)->elttype == MTtype)
#define ISDTorCTTYPE(pe) (((pe)->elttype == DTtype) || ((pe)->elttype == CTtype))
#define ISCPorLCPTYPE(pe) (((pe)->elttype == PCPtype) || ((pe)->elttype == LCPtype))

#define XP0 0
#define XP1 1
#define XP2 2
#define XP3 3
#define YP0 4
#define YP1 5
#define YP2 6
#define YP3 7
#define X 0
#define Y 4
#define NP 4

#define NCOORDS 8

typedef double Curve[NCOORDS], *PCurve;  /* The control point representation */

typedef struct _BBox {double lx, ly, hx, hy;} BBox;

typedef struct _Elt 
{
  struct _Elt *prevelt, *nextelt;
  unsigned elttype:3; /* type of element: moveto, cuveto ... */
  BBox ebbx;
  Curve coord;	/* coordinates of points */
} Elt, *Pelt;

typedef struct _Path 
{
   int numelts;
   Pelt elements;
   Pelt lastelement;
   BBox bbx;	/* of the path */
   struct _Path *matches; /* for alloc bookkeeping */
} Path, *PPath;

#define MAXNUMPATHS 256

#define COPYCOORDS(to,from) do{int _i; for(_i=0;_i<NCOORDS;_i++)(to)[_i] = (from)[_i];} while(0)

typedef struct _Outline
{
  int numsubpaths;
  PPath *subpath; /* array of subpaths */
  BBox Obbx;	/* of the whole outline */
} Outline, *POutline;

extern void init_Outlines(POutline O);
extern void free_Outlines(POutline O);

extern boolean addmoveto (double, double, POutline);
extern boolean addlineto(double, double, double, double, boolean, POutline);
extern boolean addcurveto(double, double, double, double, double, double, double, double, POutline);
extern boolean addclosepath(double, double, POutline, boolean);

extern void compute_outline_bbx(POutline);
extern Pelt SUCC(Pelt);
extern Pelt PRED(Pelt);
extern boolean isaspacechar(POutline);

extern double currx, curry;

typedef struct _vector
	{
	double x;
	double y;
	} Vector;

/* return normalized vectors */
extern void init_vector(Pelt e, Vector *v);
extern void end_vector(Pelt e, Vector *v);
