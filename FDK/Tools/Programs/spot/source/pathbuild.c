/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************
 * SCCS Id:    %W%
 * Changed:    %G% %U%
 ***********************************************************************/

#include <math.h>
#include <stdio.h>
#include "global.h"
#include "pathbuild.h"

double currx, curry; 

static boolean Pathaddmoveto (PPath O[], int subpth, Curve C);
static boolean Pathaddlineto(PPath O[], int subpth, Curve Cv);
static boolean Pathaddcurveto (PPath O[], int subpth, Curve Cv);
static boolean Pathaddclosepath (PPath O[], int subpth, Curve C, boolean isLCP);

void bisectbez (Curve oldc, Curve lft, Curve rgt);

static Pelt FreeEltHead = (Pelt)NULL;
static Pelt _tmpelt;

#define ALLOCELT(e) \
  do {if (FreeEltHead == (Pelt)NULL) {\
        (e) = (Pelt) calloc (1, sizeof (Elt)); \
        if ((e) == (Pelt)NULL) fatal(SPOT_MSG_pathNOPELT); }\
      else { (e) = FreeEltHead; FreeEltHead = FreeEltHead->nextelt;} \
      (e)->elttype = 0;\
      (e)->prevelt= (e)->nextelt = (Pelt)NULL; } while (0)

#define FREEELT(e) \
do{_tmpelt = FreeEltHead; FreeEltHead = (e); (e)->nextelt = _tmpelt;} while (0)


static PPath FreePathHead = (PPath)NULL;
static PPath _tmppath;

#define ALLOCPATH(p) \
 do{if (FreePathHead == (PPath)NULL) {\
  if (((p) = (PPath)calloc (1, sizeof (Path))) == (PPath)NULL) fatal(SPOT_MSG_pathNOSUBP); }\
  else {(p) = FreePathHead; FreePathHead = FreePathHead->matches;} \
  (p)->numelts= 0 ; \
  (p)->elements = (p)->lastelement= (Pelt)NULL; \
  (p)->matches = (PPath)NULL; } while (0)

#define FREEPATH(p) \
do{_tmppath = FreePathHead; FreePathHead = (p); (p)->matches = _tmppath;} while (0)


#define ASSIGNBBX(dest,src) do{(dest).lx = (src).lx; \
  (dest).ly = (src).ly; \
  (dest).hx = (src).hx; \
  (dest).hy = (src).hy; } while(0)


static boolean pathisopen = false;

static void Initload(void)
{
   currx = curry = 0.0;
   pathisopen = false;
}

#define MTCPCOORDS(Cv,xval,yval) (Cv)[XP0]=(Cv)[XP1]=(Cv)[XP2]=(Cv)[XP3] = (xval); \
  (Cv)[YP0]=(Cv)[YP1]=(Cv)[YP2]=(Cv)[YP3] = (yval)


boolean addmoveto (double movx, double movy, POutline Outlin)
{
  Curve C;
  if (pathisopen) {
	addclosepath(currx, curry, Outlin, 1); 
  }

  MTCPCOORDS(C,movx,movy);
  currx = movx;
  curry = movy;
  if (Pathaddmoveto (Outlin->subpath, Outlin->numsubpaths, C)) {
	pathisopen = true;
    return true;
  }
  else return false;
}

boolean addlineto(double sx, double sy, double ex, double ey, boolean isreallyaclosepath, POutline Outlin)
{
  Curve C;

  C[XP0] = C[XP1] = sx; 
  C[YP0] = C[YP1] = sy;
  C[XP2] = C[XP3] = ex;
  C[YP2] = C[YP3] = ey;  
  currx = ex; curry = ey;
  if (isreallyaclosepath && 
      (Outlin->subpath[Outlin->numsubpaths] != (PPath)NULL) &&
      (Outlin->subpath[Outlin->numsubpaths]->numelts < 2)) 
	return(false);

  return (Pathaddlineto (Outlin->subpath, Outlin->numsubpaths, C));
}

boolean addcurveto (double c0x, double c0y, double c1x, double c1y, double c2x, double c2y, double c3x, double c3y, POutline Outlin)
{
  Curve C;
  C[XP0] = c0x;
  C[YP0] = c0y;
  C[XP1] = c1x;
  C[YP1] = c1y;
  C[XP2] = c2x;
  C[YP2] = c2y;
  C[XP3] = c3x;
  C[YP3] = c3y;
  currx = c3x; curry = c3y;
  return (Pathaddcurveto (Outlin->subpath, Outlin->numsubpaths, C));  
}


boolean addclosepath (double closx, double closy, POutline Outlin, boolean isLCP)
{
  Curve C;
  MTCPCOORDS(C,closx,closy);
  currx = closx;  curry = closy;
  return (Pathaddclosepath (Outlin->subpath, Outlin->numsubpaths, C, isLCP));  
/* NOTE: caller must take care of incrementing Outlin->numsubpaths
   because it is presumably the caretaker of that array-size, and
   should be the one to test for overflow 
*/
}


boolean isaspacechar (POutline OTL)
{
  PPath P;
  P = OTL->subpath[OTL->numsubpaths];
  if ((P == (PPath)NULL) && (OTL->numsubpaths == 0)) return (true);
  return ((OTL->numsubpaths == 0) && 
	  (P->numelts == 0) && 
	  (P->lastelement == (Pelt)NULL));
}

static boolean startpath (PPath pth, Pelt ele)
{
  if (pth == (PPath)NULL) return false;
  if ((pth->numelts != 0) || (pth->lastelement != (Pelt)NULL)) {
    fatal(SPOT_MSG_pathMTMT);
  }
  if (ele == (Pelt)NULL) return false;
  ele->prevelt = (Pelt)NULL;
  ele->nextelt = (Pelt)NULL;
  pth->elements = ele;
  pth->lastelement = ele;
  pth->numelts += 1;
  pth->bbx.lx = pth->bbx.hx = ele->coord[XP0];
  pth->bbx.ly = pth->bbx.hy = ele->coord[YP0];
  return (true);
}

static boolean finishpath (PPath pth, Pelt ele)
{
  if (pth == (PPath)NULL) return false;
  if ((pth->numelts == 0) ||
      (pth->lastelement == (Pelt)NULL)) return false;
  if (ele == (Pelt)NULL) return false;
  if (pth->numelts < 3) return false;
  ele->prevelt = pth->lastelement;
  ele->nextelt = pth->elements; /* CIRCULAR LIST */
  pth->elements->prevelt = ele;
  (pth->lastelement)->nextelt = ele;
  pth->lastelement = ele;
  pth->numelts += 1;
/* ensure that the closepath is at the same place as the initial moveto*/
  MTCPCOORDS(ele->coord, pth->elements->coord[XP0], pth->elements->coord[YP0]);
  ele->prevelt->coord[XP3] = ele->coord[XP0];
  ele->prevelt->coord[YP3] = ele->coord[YP0];
  pathisopen = false;
  return (true);
}

static void BBoxClosure (BBox *dest, BBox *suitor)
{
  if (suitor->lx < dest->lx) dest->lx = suitor->lx;
  if (suitor->ly < dest->ly) dest->ly = suitor->ly;
  if (suitor->hx > dest->hx) dest->hx = suitor->hx;
  if (suitor->hy > dest->hy) dest->hy = suitor->hy;
}


void CtrlPointBBox(Curve crv, BBox *bbox)
{
  int ip;

  bbox->lx = bbox->hx = crv[XP0];
  for (ip = XP1; ip <= XP3; ip++) {
    if (crv[ip] < bbox->lx) bbox->lx = crv[ip];
    if (crv[ip] > bbox->hx) bbox->hx = crv[ip];
  }

  bbox->ly = bbox->hy = crv[YP0];
  for (ip = YP1; ip <= YP3; ip++) {
    if (crv[ip] < bbox->ly) bbox->ly = crv[ip];
    if (crv[ip] > bbox->hy) bbox->hy = crv[ip];
  }
}

static void CurveBBox (Pelt e, BBox *ebx)
{
  Curve tmp, c1, c2;
  BBox b1, b2;
  COPYCOORDS(tmp, e->coord);
  bisectbez (tmp, c1, c2);
  CtrlPointBBox(c1, &b1);
  CtrlPointBBox(c2, &b2);
  BBoxClosure(&b1, &b2);
  ASSIGNBBX((*ebx), b1);
}


#define BIGGESTdouble (1.0e+10)
#define SMALLESTdouble (-1.0e+10)

 boolean addtopath (PPath pth, Pelt ele)
{
  if (pth == (PPath)NULL) return false;
  if ((pth->numelts == 0) ||
      (pth->lastelement == (Pelt)NULL)) return false;
  if (ele == (Pelt)NULL) return false;
  ele->prevelt = pth->lastelement;
  ele->nextelt = (Pelt)NULL;
  ele->ebbx.lx = BIGGESTdouble;	/* infinity */
  ele->ebbx.ly = BIGGESTdouble;	/* infinity */
  ele->ebbx.hx = SMALLESTdouble;	/* -infinity */
  ele->ebbx.hy = SMALLESTdouble;	/* -infinity */
  (pth->lastelement)->nextelt = ele;
  pth->lastelement = ele;
  pth->numelts += 1;
  if (ele->elttype == CTtype) {
    CurveBBox (ele, &(ele->ebbx));    
  }
  else {
    CtrlPointBBox (ele->coord, &(ele->ebbx));
  }

  if ISDTorCTTYPE(ele) {
    BBoxClosure (&(pth->bbx), &(ele->ebbx));
  }
  return (true);
}


#define DEG(x) ((x)*57.29577951308232088)

static boolean Pathaddlineto(PPath O[], int subpth, Curve Cv)
{
  Pelt pel;

  ALLOCELT(pel);
  pel->elttype = DTtype;
  COPYCOORDS(pel->coord, Cv);
  pel->coord[XP1] = pel->coord[XP0];
  pel->coord[YP1] = pel->coord[YP0];
  pel->coord[XP2] = pel->coord[XP3];
  pel->coord[YP2] = pel->coord[YP3];
  return (addtopath (O[subpth], pel));    
}


static boolean Pathaddcurveto (PPath O[], int subpth, Curve Cv)
{
  Pelt pel;
  
  ALLOCELT(pel);
  pel->elttype = CTtype;
  COPYCOORDS(pel->coord, Cv); 
  return (addtopath (O[subpth], pel));
}


static boolean Pathaddmoveto (PPath O[], int subpth, Curve C)
{
  Pelt pel;
  
  ALLOCELT(pel);
  if (O[subpth] == (Path *)NULL)
    ALLOCPATH(O[subpth]);
  pel->elttype = MTtype;
  MTCPCOORDS(pel->coord, C[XP0], C[YP0]);
  return (startpath (O[subpth], pel));
}


static boolean Pathaddclosepath (PPath O[], int subpth, Curve C, boolean isLCP)
{
  Pelt pel;
  
  ALLOCELT(pel);
  pel->elttype = (isLCP) ? LCPtype : PCPtype;
  MTCPCOORDS(pel->coord, C[XP0], C[YP0]);
  return (finishpath (O[subpth], pel));
}

Pelt SUCC(Pelt e)
{
  Pelt Pnx;
  Pnx = e->nextelt;
  if ISMTorCPTYPE(Pnx) {
    /* might be the closepath */
    Pnx = Pnx->nextelt; 
  }
  else return (Pnx);
  if ISMTorCPTYPE(Pnx) {
    /* might be the successive moveto */
    Pnx = Pnx->nextelt;
  }
  return (Pnx);
}

Pelt PRED(Pelt e)
{
  Pelt Pnx;
  Pnx = e->prevelt;
  if ISMTorCPTYPE(Pnx) {
    /* might be the closepath */
    Pnx = Pnx->prevelt; 
  }
  else return (Pnx);
  if ISMTorCPTYPE(Pnx) {
    /* might be the successive moveto */
    Pnx = Pnx->prevelt;
  }
  return (Pnx);
}


 void curve2line (Pelt pel)
{
  pel->coord[XP1] = pel->coord[XP0];
  pel->coord[YP1] = pel->coord[YP0];
  pel->coord[XP2] = pel->coord[XP3];
  pel->coord[YP2] = pel->coord[YP3];
  pel->elttype = DTtype;
  /* check successor */
  if (pel->nextelt->elttype == PCPtype) pel->nextelt->elttype = LCPtype;
}

#define onethird 0.33333333333333

 void line2curve (Pelt pel)
{
  double cbase, sbase;
  cbase = pel->coord[XP3] - pel->coord[XP0];
  sbase = pel->coord[YP3] - pel->coord[YP0];
  pel->coord[XP1] = pel->coord[XP0] + onethird * cbase;
  pel->coord[YP1] = pel->coord[YP0] + onethird * sbase;
  pel->coord[XP2] = pel->coord[XP3] - onethird * cbase;
  pel->coord[YP2] = pel->coord[YP3] - onethird * sbase;
  pel->elttype = CTtype;
  /* check successor */
  if (pel->nextelt->elttype == LCPtype) pel->nextelt->elttype = PCPtype;
}


void free_path(PPath *pp)
{
  register int ecount;
  int emax;
  register Pelt e, enxt;

  /* nuke the elements list */
  enxt = (Pelt)NULL;
  /* break the circular list */
  if ((*pp)->lastelement != (Pelt)NULL)
    (*pp)->lastelement->nextelt = (Pelt)NULL;
  emax = (*pp)->numelts;
  for (ecount = 0, e=(*pp)->elements;
       (e != (Pelt)NULL) && (ecount < emax);
       ecount++, e=enxt) {
    enxt = e->nextelt;
    /* free the elt*/
    FREEELT(e);
  }

  FREEPATH(*pp);
}


void free_Outlines(POutline O)
{
  register int i;

  for (i = 0; i < MAXNUMPATHS; i++) {
	if (O->subpath[i] != (PPath)NULL) {
	  free_path (&(O->subpath[i]));
	  O->subpath[i] = (PPath)NULL;
	}
  }
  memFree(O->subpath);
  O->subpath = NULL;
}

void init_Outlines(POutline O) 
{
  int j;

  if ((O)->subpath == (PPath *)NULL) {
	if (((O)->subpath = (PPath *) calloc (MAXNUMPATHS, sizeof (PPath))) == (PPath *)NULL) {
	  fatal(SPOT_MSG_pathNOOSA);
	}
  }
  for (j = 0; j < MAXNUMPATHS; j++)
	(O)->subpath[j] = (PPath)NULL;
  (O)->numsubpaths = 0;

  Initload();
}


 void compute_outline_bbx (POutline OTL)
{
  PPath pth;
  register int i;

  OTL->Obbx.lx = BIGGESTdouble;
  OTL->Obbx.ly = BIGGESTdouble;
  OTL->Obbx.hx = SMALLESTdouble;
  OTL->Obbx.hy = SMALLESTdouble;

  for (i=0; i < OTL->numsubpaths; i++) {
    pth = OTL->subpath[i];
    BBoxClosure (&(OTL->Obbx), &(pth->bbx));
  }
}


#define CHOP_EPS .002
#define GENAVG(a,b,t) ((a) + (t) * ((b)-(a)))
#define GENAVG2(a,b) (((a) + (b)) * 0.5)

static boolean practicallyequal (double a, double b)
{
  double d;
  d = fabs((a - b));
  return (d < CHOP_EPS);
}

void init_vector(Pelt e, Vector *v)
{
  double vx, vy, d;
  Pelt s;

  switch ((int)(e->elttype))
	{
	case CTtype :
	  vx = e->coord[XP1] - e->coord[XP0];
	  vy = e->coord[YP1] - e->coord[YP0];
	  break;

	case DTtype:
	  vx = e->coord[XP3] - e->coord[XP0];
	  vy = e->coord[YP3] - e->coord[YP0];
	  break;

	case LCPtype:
	  s = SUCC(e);
	  if (s->elttype == DTtype) 
		{
		  vx = s->coord[XP3] - s->coord[XP0];
		  vy = s->coord[YP3] - s->coord[YP0];
		}
	  else 
		{
		  vx = s->coord[XP1] - s->coord[XP0];
		  vy = s->coord[YP1] - s->coord[YP0];
		}
	  break;

	default:
	  vx = vy = 0.0;
	  break;
	}
  /* normalize */
  d = sqrt(vx*vx + vy*vy);
  if (practicallyequal(d, 0.0))	{
	v->x = v->y = 1.0;
  }
  else	{
	v->x = vx/d;
	v->y = vy/d;
  }
}

void end_vector(Pelt e, Vector *v)
{
  double vx, vy, d;
  Pelt s;

  switch((int)(e->elttype))
	{
	case CTtype:
	  vx = e->coord[XP3] - e->coord[XP2];
	  vy = e->coord[YP3] - e->coord[YP2];
	  break;

	case DTtype:
	  vx = e->coord[XP3] - e->coord[XP0];
	  vy = e->coord[YP3] - e->coord[YP0];
	  break;

	case LCPtype:
	  s = PRED(e);
	  vx = s->coord[XP3] - s->coord[XP0];
	  vy = s->coord[YP3] - s->coord[YP0];
	  break;

	default:
	  vx = vy = 0.0;
	  break;
	}	  
  /* normalize */
  d = sqrt(vx*vx + vy*vy);
  if (practicallyequal(d, 0.0))	{
	v->x = v->y = 1.0;
  }
  else	{
	v->x = vx/d;
	v->y = vy/d;
  }
}


 void bisectbez (Curve oldc, Curve lft, Curve rgt)
/* on exit, lft on [0..1] follows path of input oldc on [0..0.5] */
/* rgt on [0..1] follows path of oldc on [0.5..1] */
{
  double tmpx, tmpy;

  lft[XP0] = oldc[XP0];
  lft[YP0] = oldc[YP0];
  rgt[XP3] = oldc[XP3];
  rgt[YP3] = oldc[YP3];

  if (practicallyequal(oldc[XP0], oldc[XP1]) &&
      practicallyequal(oldc[YP0], oldc[YP1]) &&
      practicallyequal(oldc[XP2], oldc[XP3]) &&
      practicallyequal(oldc[YP2], oldc[YP3]))  {	
    /* a line */
    lft[XP1] = oldc[XP0]; lft[YP1] = oldc[YP0];
    rgt[XP2] = oldc[XP3]; rgt[YP2] = oldc[YP3];
    lft[XP3] = lft[XP2] = rgt[XP0] = rgt[XP1] = GENAVG2(oldc[XP1], oldc[XP2]);
    lft[YP3] = lft[YP2] = rgt[YP0] = rgt[YP1] = GENAVG2(oldc[YP1], oldc[YP2]);
  }
  else {
    lft[XP1] = GENAVG2(oldc[XP0], oldc[XP1]);
    lft[YP1] = GENAVG2(oldc[YP0], oldc[YP1]);
    tmpx = GENAVG2(oldc[XP1], oldc[XP2]);	  
    tmpy = GENAVG2(oldc[YP1], oldc[YP2]);
    rgt[XP2] = GENAVG2(oldc[XP2], oldc[XP3]);
    rgt[YP2] = GENAVG2(oldc[YP2], oldc[YP3]);
    lft[XP2] = GENAVG2(lft[XP1], tmpx);	
    lft[YP2] = GENAVG2(lft[YP1], tmpy);
    rgt[XP1] = GENAVG2(tmpx, rgt[XP2]); 
    rgt[YP1] = GENAVG2(tmpy, rgt[YP2]); 
    lft[XP3] = rgt[XP0] = GENAVG2(lft[XP2], rgt[XP1]);
    lft[YP3] = rgt[YP0] = GENAVG2(lft[YP2], rgt[YP1]);
  }
}		


