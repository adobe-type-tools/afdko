/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/
/* charpath.c */

#include "machinedep.h"
#include "buildfont.h"
#include "bftoac.h"
#include "charpath.h"
#include "chartable.h"
#include "masterfont.h"
#include "opcodes.h"
#include "optable.h"
#include "hintfile.h"
#include "transitionalchars.h"


extern double atan2( double, double );

#define DEBUG_PCP 0 /* debug point closepath */

#define DMIN 50  /* device minimum (one-half of a device pixel) */
#define MAINHINTS -1
#define GROWBUFF 2048 /* Amount to grow output buffer, if necessary. */
/* The following definitions are used when determining
 if a hinting operator used the start, end, average or
 flattened coordinate values. */
/* Segments are numbered starting with 1. The "ghost" segment is 0. */
#define STARTPT 0
#define ENDPT 1
#define AVERAGE 2
#define CURVEBBOX 3
#define FLATTEN 4
#define GHOST 5

boolean flexexists;
extern boolean multiplemaster;
boolean cubeLibrary;
boolean bereallyQuiet = 1;
char *currentChar; /* name of the current char for error messages */
extern boolean ReadCharFile();

#if ! AC_C_LIB
static boolean firstMT;
#endif
static char *startbuff, **outbuff;
static short dirCount, byteCount, buffSize;
static PPathList pathlist = NULL;
static Cd *refPtArray = NULL;
static indx hintsdirIx;
static char outstr[100];

/* Prototypes */
static void AddLine(indx, indx);
static boolean CheckFlexOK(indx);
static boolean ChangetoCurve(indx, indx);
static void CheckFlexValues(short *, indx, indx, boolean *, boolean *);
static void CheckForZeroLengthCP(void);
static void CheckHandVStem3(void);
static void CombinePaths(void);
static boolean CompareCharPaths(char *, boolean);
static void Ct(Cd, Cd, Cd, indx, short);
static boolean CurveBBox(indx, short, long, Fixed *);
static void FindHandVStem3(PHintElt *, indx, boolean *);
static void FreePathElements(indx, indx);
static void GetCoordFromType(short, CdPtr, indx, indx);
static long GetCPIx(indx, long);
static void GetEndPoint(indx, long, Fixed *, Fixed *);
static void GetEndPoints(indx, long, CdPtr, CdPtr);
static void GetFlexCoord(indx, indx, indx, CdPtr);
static void GetPathType(short, char *);
static short GetPointType(short, Fixed, long *);
static void GetRelPos(long, short, Fixed, CdPtr, CdPtr, Fixed *);
static void GetRelativePosition(Fixed, Fixed, Fixed, Fixed, Fixed, Fixed *);
static void Hvct(Cd, Cd, Cd, indx, short);
static void InconsistentPathType(char *, indx, short, short, indx);
static void InconsistentPointCount(char *, indx, int, int);
static void InsertHint(PHintElt, indx, short, short);
static void MtorDt(Cd, indx, short);
static void OptimizeCT(indx);
static void OptimizeMtorDt(indx, short *, boolean *, boolean *);
static void ReadHints(PHintElt, indx);
static int ReadandAssignHints(void);
static void ReadHorVStem3Values(indx, short, short, boolean *);
static void SetSbandWidth(char *, boolean, Transitions *, int);
static void Vhct(Cd, Cd, Cd, indx, short);
static void WriteFlex(indx);
static void WriteHints(indx);
static void WritePathElt(indx, indx, short, indx, short);
static void WriteSbandWidth(void);
static void WriteX(Fixed);
static void WriteY(Fixed);
static void WriteToBuffer(void);
static boolean ZeroLengthCP(indx, indx);

#if AC_C_LIB
void GetMasterDirName(char *dirname, indx ix)
{
	if (dirname)
		dirname[0] = '\0';
}
#endif

/* macros */
#define FixShift (8)
#define IntToFix(i) ((long int)(i) << FixShift)
#define FRnd(x) ((long int)(((x)+(1<<7)) & ~0xFFL))
#define FTrunc8(x) ((long int)((x)>>8))
#define FIXED2FLOAT(x) ((float)((x) / 256.0))
#define FixedToDouble(x) ((double)((x) / 256.0))
#define Frac(x) ((x) & 0xFFL)
#define FixOne (0x100L)
#define FixHalf (0x80L)
#define FixHalfMul(f) ((f) >> 1)
#define FixTwoMul(f) ((f) << 1)
#define TFMX(x) ((x))
#define TFMY(y) (-(y))
#define ITFMX(x) ((x))
#define ITFMY(y) (-(y))
#define WRTNUM(i) {sprintf(outstr, "%d ", (int)(i)); WriteToBuffer(); }
#define WriteStr(str) {\
sprintf(outstr, "%s ", (char *)str); WriteToBuffer(); }
#define WriteSubr(val) {\
sprintf(outstr, "%d subr ", val); WriteToBuffer(); }

/* Checks if buffer needs to grow before writing out string. */
static void WriteToBuffer()
{
    int len = (int)strlen(outstr);
    
    if ((byteCount + len) > buffSize)
    {
        buffSize += GROWBUFF;
#if DOMEMCHECK
        startbuff = (char *)memck_realloc(*outbuff, (buffSize * sizeof(char)));
#else
        startbuff = (char *)ReallocateMem(*outbuff,
                                          (unsigned)(buffSize * sizeof(char)), "file buffer");
#endif
        SetMaxBytes(buffSize);
        *outbuff = startbuff;
        startbuff += byteCount;
        memset (startbuff, 0, GROWBUFF);
        while (*startbuff == '\0') startbuff--;
        startbuff++;
    }
    byteCount += len;
    sprintf(startbuff, "%s", outstr);
    startbuff += len;
}

static void WriteX(x)
Fixed x;
{
    Fixed i = FRnd(x);
    WRTNUM(FTrunc8(i));
}

static void WriteY(y)
Fixed y;
{
    Fixed i = FRnd(y);
    WRTNUM(FTrunc8(i));
}

#define WriteCd(c) {WriteX(c.x); WriteY(c.y);}

static void WriteOneHintVal(val)
Fixed val;
{
    if (Frac(val) == 0)
        WRTNUM(FTrunc8(val))
        else
        {
            WRTNUM(FTrunc8(FRnd(val*100L)))
            WriteStr("100 div ");
        }
}

/* Locates the first CP following the given path element. */
static long GetCPIx(dirIx, pathIx)
indx dirIx;
long pathIx;
{
    indx ix;
    
    for (ix = pathIx; ix < path_entries; ix++)
        if (pathlist[dirIx].path[ix].type == CP)
            return ix;
    sprintf(globmsg, "No closepath in character: %s.\n", currentChar);
    LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
    return (-1);
}

/* Locates the first MT preceding the given path element. */
static int GetMTIx(indx dirIx, indx pathIx)
{
    indx ix;
    
    for (ix = pathIx; ix >= 0; ix--)
        if (pathlist[dirIx].path[ix].type == RMT)
            return ix;
    sprintf(globmsg, "No moveto in character: %s.\n", currentChar);
    LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
    return (-1);
}

/* Locates the first MT SUCCEEDING the given path element. */
static int GetNextMTIx(indx dirIx, indx pathIx)
{
    indx ix;
    
    for (ix = pathIx; ix < path_entries ; ix++)
        if (pathlist[dirIx].path[ix].type == RMT)
            return ix;
    return (-1);
}

static void GetEndPoint(dirIx, pathIx, ptX, ptY)
indx dirIx;
long pathIx;
Fixed *ptX, *ptY;
{
    PCharPathElt pathElt = &pathlist[dirIx].path[pathIx];
    
retry:
    switch(pathElt->type)
    {
        case RMT:
        case RDT:
            *ptX = pathElt->x; *ptY = pathElt->y;
            break;
        case RCT:
            *ptX = pathElt->x3; *ptY = pathElt->y3;
            break;
        case CP:
            while (--pathIx >= 0)
            {
                pathElt = &pathlist[dirIx].path[pathIx];
                if (pathElt->type == RMT)
                    goto retry;
            }
            sprintf(globmsg, "Bad character description file: %s.\n", currentChar);
            LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
        default:
            sprintf(globmsg, "Illegal operator in character file: %s.\n", currentChar);
            LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
    }
}

static void GetEndPoints(dirIx, pathIx, start, end)
indx dirIx;
long pathIx;
Cd *start, *end;
{
    if (pathlist[dirIx].path[pathIx].type == RMT)
    {
        long cpIx;
        
        GetEndPoint(dirIx, pathIx, &start->x, &start->y);
        /* Get index for closepath associated with this moveto. */
        cpIx = GetCPIx(dirIx, pathIx + 1);
        GetEndPoint(dirIx, cpIx - 1, &end->x, &end->y);
    }
    else
    {
        GetEndPoint(dirIx, pathIx-1, &start->x, &start->y);
        GetEndPoint(dirIx, pathIx, &end->x, &end->y);
    }
}

static void GetCoordFromType(short pathtype, CdPtr coord,
                             indx dirix, indx eltno)
{
    switch(pathtype)
    {
        case RMT:
        case RDT:
            (*coord).x = FTrunc8(FRnd(pathlist[dirix].path[eltno].x));
            (*coord).y = FTrunc8(FRnd(pathlist[dirix].path[eltno].y));
            break;
        case RCT:
            (*coord).x = FTrunc8(FRnd(pathlist[dirix].path[eltno].x3));
            (*coord).y = FTrunc8(FRnd(pathlist[dirix].path[eltno].y3));
            break;
        case CP:
            GetCoordFromType(pathlist[dirix].path[eltno-1].type, coord, dirix, eltno-1);
            break;
    };
}

static void GetPathType(short pathtype, char *str)
{
    switch(pathtype)
    {
        case RMT: strcpy(str, "moveto"); break;
        case RDT: strcpy(str, "lineto"); break;
        case RCT: strcpy(str, "curveto"); break;
        case CP:  strcpy(str, "closepath"); break;
        default:
            sprintf(globmsg, "Illegal path type: %d in character: %s.\n", pathtype, currentChar);
            LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
    }
}

static void FreePathElements(startix, stopix)
indx startix, stopix;
{
    indx i, j;
    
    for (j = startix; j < stopix; j++)
    {
		i=0;
#if 0
        PHintElt hintElt, next;
        if (pathlist[j].path != NULL)
        {
            /* Before we can free hint elements will need to know path_entries
             value for char in each directory because this proc can be
             called when characters are inconsistent.
             */
            for (i = 0; i < path_entries; i++)
            {
                hintElt = pathlist[j].path[i].hints;
                while (hintElt != NULL)
                {
                    next = hintElt->next;
#if DOMEMCHECK
                    memck_free(hintElt);
#else
                    UnallocateMem(hintElt);
#endif
                    hintElt = next;
                }
            }
        }
#endif
#if DOMEMCHECK
        memck_free(pathlist[j].mainhints);
        memck_free(pathlist[j].path);
#else
        UnallocateMem(pathlist[j].mainhints);
        UnallocateMem(pathlist[j].path);
#endif
    }
}

static void InconsistentPointCount(char *filename, indx ix,
                                   int entries1, int entries2)
{
    char pathdir1[MAXPATHLEN], pathdir2[MAXPATHLEN];
    
    GetMasterDirName(pathdir1, 0);
    GetMasterDirName(pathdir2, ix);
    sprintf(globmsg, "The character: %s will not be included in the font\n  because the version in %s has a total of %d elements and\n  the one in %s has %d elements.\n", filename, pathdir1, (int)entries1, pathdir2, (int)entries2);
    LogMsg(globmsg, WARNING, OK, TRUE);
}

static void InconsistentPathType(char *filename, indx ix, short type1,
                                 short type2, indx eltno)
{
    char pathdir1[MAXPATHLEN], pathdir2[MAXPATHLEN];
    char typestr1[10], typestr2[10];
    Cd coord1, coord2;
    
    GetPathType(type1, typestr1);
    GetPathType(type2, typestr2);
    GetCoordFromType(type1, &coord1, 0, eltno);
    GetCoordFromType(type2, &coord2, ix, eltno);
    GetMasterDirName(pathdir1, 0);
    GetMasterDirName(pathdir2, ix);
    sprintf(globmsg, "The character: %s will not be included in the font\n  because the version in %s has path type %s at coord: %d %d\n  and the one in %s has type %s at coord %d %d.\n",
            filename, pathdir1, typestr1, (int) coord1.x, (int) coord1.y, pathdir2,
            typestr2, (int) coord2.x, (int) coord2.y);
    LogMsg(globmsg, WARNING, OK, TRUE);
}

/* Returns whether changing the line to a curve is successful. */
static boolean ChangetoCurve(dirIx, pathIx)
indx dirIx, pathIx;
{
    Cd start, end, ctl1, ctl2;
    PCharPathElt pathElt = &pathlist[dirIx].path[pathIx];
    
    if (pathElt->type == RCT)
        return TRUE;
    /* Use the 1/3 rule to convert a line to a curve, i.e. put the control points
     1/3 of the total distance from each end point. */
    GetEndPoints(dirIx, pathIx, &start, &end);
    ctl1.x = FRnd((start.x * 2 + end.x + FixHalf)/3);
    ctl1.y = FRnd((start.y * 2 + end.y + FixHalf)/3);
    ctl2.x = FRnd((end.x * 2 + start.x + FixHalf)/3);
    ctl2.y = FRnd((end.y * 2 + start.y + FixHalf)/3);
    pathElt->type = RCT;
    pathElt->x1 = ctl1.x;
    pathElt->y1 = ctl1.y;
    pathElt->x2 = ctl2.x;
    pathElt->y2 = ctl2.y;
    pathElt->x3 = end.x;
    pathElt->y3 = end.y;
    pathElt->rx1 = ctl1.x - start.x;
    pathElt->ry1 = ctl1.y - start.y;
    pathElt->rx2 = pathElt->x2 - pathElt->x1;
    pathElt->ry2 = pathElt->y2 - pathElt->y1;
    pathElt->rx3 = pathElt->x3 - pathElt->x2;
    pathElt->ry3 = pathElt->y3 - pathElt->y2;
    return TRUE;
}

static boolean ZeroLengthCP(dirIx, pathIx)
indx dirIx, pathIx;
{
    Cd startPt, endPt;
    
    GetEndPoints(dirIx, pathIx, &startPt, &endPt);
    return (startPt.x == endPt.x && startPt.y == endPt.y);
}

/* Subtracts or adds one unit from the segment at pathIx. */
static void AddLine(dirIx, pathIx)
indx dirIx, pathIx;
{
    Fixed fixTwo = IntToFix(2);
    Fixed xoffset = 0, yoffset = 0;
    Fixed xoffsetr = 0, yoffsetr = 0;
    PCharPathElt start, end, thisone, nxtone;
    char dirname[MAXPATHLEN];
    indx  i, n;
    
    if (pathlist[dirIx].path[pathIx].type != RCT)
    {
        if (!bereallyQuiet) {
            GetMasterDirName(dirname, dirIx);
            sprintf(globmsg,
                    "Please convert the point closepath in directory: %s, character: %s to a line closepath.\n", dirname, currentChar);
            LogMsg(globmsg, WARNING, OK, TRUE);
        }
        return;
    }
    i = GetMTIx(dirIx, pathIx) + 1;
    start = &pathlist[dirIx].path[i];
    end = &pathlist[dirIx].path[pathIx];
    /* Check control points to find out if x or y value should be adjusted
     in order to get a smooth curve. */
    switch (start->type)
    {
        case RDT:
            if (!bereallyQuiet) {
                GetMasterDirName(dirname, dirIx);
                sprintf(globmsg, "Please convert the point closepath to a line closepath in directory: %s, character: %s.\n", dirname, currentChar);
                LogMsg(globmsg, WARNING, OK, TRUE);
            }
            return;
            break;
        case RCT:
            if ((ABS(start->x1 - end->x2) < fixTwo) && (ABS(start->x1 - pathlist[dirIx].path[i-1].x) < fixTwo))
                yoffset =
                (start->y1 < end->y2 && end->y2 > 0) || (start->y1 > end->y2 && end->y2 < 0) ? FixOne : -FixOne;
            else if ((ABS(start->y1 - end->y2) < fixTwo) && (ABS(start->y1 - pathlist[dirIx].path[i-1].y) < fixTwo))
                xoffset =
                (start->x1 < end->x2 && end->x2 > 0) || (start->x1 > end->x2 && end->x2 < 0) ? FixOne : -FixOne;
            else
            {
                if (!bereallyQuiet) {
                    GetMasterDirName(dirname, dirIx);
                    sprintf(globmsg, "Could not modify point closepath in directory '%s', character: %s near (%ld, %ld).\n", dirname, currentChar, FTrunc8(end->x), FTrunc8(end->y));
                    LogMsg(globmsg, WARNING, OK, TRUE);
                }
                return;
            }
            break;
        default:
            GetMasterDirName(dirname, dirIx);
            sprintf(globmsg, "Bad character description file: %s/%s.\n", dirname, currentChar);
            LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
    }
    
    thisone = &(pathlist[dirIx].path[pathIx]);
    thisone->x3 += xoffset;
    xoffsetr = (xoffset == 0) ? 0 : ((thisone->rx3 < 0) ? FixOne : -FixOne);
    thisone->rx3 += xoffsetr;
    
    thisone->y3 += yoffset;
    yoffsetr = (yoffset == 0) ? 0 : ((thisone->ry3 < 0) ? FixOne : -FixOne);
    thisone->ry3 += yoffsetr;
    
    /* Now, fix up the following MT's rx1, ry1 values
     This fixes a LOOOONG-standing bug.    renner Wed Jul 16 09:33:50 1997*/
    if ((n = GetNextMTIx (dirIx, pathIx)) > 0) {
        nxtone = &(pathlist[dirIx].path[n]);
        nxtone->rx += (- xoffsetr);
        nxtone->ry += (- yoffsetr);
    }
}

#define PI 3.1415926535
static void BestLine(PCharPathElt start, PCharPathElt end, Fixed *dx, Fixed *dy)
{
    double angle;
    /* control point differences */
    double cx = FixedToDouble(end->x2 - end->x3);
    double cy = FixedToDouble(end->y2 - end->y3);
    
    *dx = *dy = 0;
    
    if (cy == 0.0 && cx == 0.0) {
#if DEBUG_PCP
        fprintf(OUTPUTBUFF, "  cx=%f cy=%f ***\n", cx, cy);
#endif /*DEBUG_PCP*/
        sprintf(globmsg, "Unexpected tangent in character path: %s.\n", currentChar);
        LogMsg(globmsg, WARNING, OK, TRUE);
        return;
    }
    
    angle = atan2(cy, cx);
#if DEBUG_PCP
    fprintf(OUTPUTBUFF, "  cx=%f cy=%f theta=%f %.0f deg\n", cx, cy, angle, angle/PI*180.);
#endif /*DEBUG_PCP*/
    
#if FOUR_WAY
    /* Judy's non-Cube code only moves vertically or horizontally. */
    /* This code produces similar results. */
    if (angle < (-PI * 0.75)) {
        *dx = -FixOne;
    } else if (angle < (-PI * 0.25)) {
        *dy = -FixOne;
    } else if (angle < (PI * 0.25)) {
        *dx = FixOne;
    } else if (angle < (PI * 0.75)) {
        *dy = FixOne;
    } else {
        *dx = -FixOne;
    }
#else /*FOUR_WAY*/
    
    if (angle < (-PI * 0.875)) {
        *dx = -FixOne;
    } else if (angle < (-PI * 0.625)) {
        *dy = -FixOne;
        *dx = -FixOne;
    } else if (angle < (-PI * 0.375)) {
        *dy = -FixOne;
    } else if (angle < (-PI * 0.125)) {
        *dy = -FixOne;
        *dx = FixOne;
    } else if (angle < (PI * 0.125)) {
        *dx = FixOne;
    } else if (angle < (PI * 0.375)) {
        *dx = FixOne;
        *dy = FixOne;
    } else if (angle < (PI * 0.625)) {
        *dy = FixOne;
    } else if (angle < (PI * 0.875)) {
        *dy = FixOne;
        *dx = -FixOne;
    } else {
        *dx = -FixOne;
    }
#endif /*FOUR_WAY*/
    
}

/* CUBE VERSION 17-Apr-94 jvz */
/* Curves: subtracts or adds one unit from the segment at pathIx. */
/* Lines: flags the segment at pathIx to be removed later; CP follows it. */
static void AddLineCube(indx dirIx, indx pathIx)
{
    /* Path element types have already been coordinated by ChangeToCurve. */
    /* Hints are only present in the hintsdirIx. */
    
    if (pathlist[dirIx].path[pathIx].type == RDT) {
        pathlist[dirIx].path[pathIx].remove = TRUE;
        if (pathlist[dirIx].path[pathIx + 1].type != CP) {
            sprintf(globmsg, "Expected CP in path: %s.\n", currentChar);
            LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
        }
        
        /* If there's another path in the character, we need to compensate */
        /* because CP does not update currentpoint. */
        
        if (pathIx + 2 < path_entries) {
            if (pathlist[dirIx].path[pathIx + 2].type == RMT) {
                pathlist[dirIx].path[pathIx + 2].rx += pathlist[dirIx].path[pathIx].rx;
                pathlist[dirIx].path[pathIx + 2].ry += pathlist[dirIx].path[pathIx].ry;
            } else {
                sprintf(globmsg, "Expected second RMT in path: %s.\n", currentChar);
                LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
            }
        }
        
        
#if DEBUG_PCP
        if (dirIx == 0)
            fprintf(OUTPUTBUFF, "  RDT\n");
#endif /*DEBUG_PCP*/
        
    } else if (pathlist[dirIx].path[pathIx].type == RCT) {
        Fixed dx = 0;
        Fixed dy = 0;
        PCharPathElt start;
        PCharPathElt end;
        indx mt; /* index of the moveto preceding this path */
        
#if DEBUG_PCP
        if (dirIx == 0)
            fprintf(OUTPUTBUFF, "  RCT\n");
#endif /*DEBUG_PCP*/
        
        mt = GetMTIx(dirIx, pathIx);
        start = &pathlist[dirIx].path[mt + 1];
        end = &pathlist[dirIx].path[pathIx];
        
        /* find nearest grid point we can move to */
        BestLine(start, end, &dx, &dy);
        
#if DEBUG_PCP
        fprintf(OUTPUTBUFF, "    dx %d  dy %d\n", FTrunc8(dx), FTrunc8(dy));
#endif /*DEBUG_PCP*/
        
        /* note that moving rx2, ry2 will also move rx3, ry3 */
        pathlist[dirIx].path[pathIx].x2 += dx;
        pathlist[dirIx].path[pathIx].x3 += dx;
        pathlist[dirIx].path[pathIx].rx2 += dx;
        
        pathlist[dirIx].path[pathIx].y2 += dy;
        pathlist[dirIx].path[pathIx].y3 += dy;
        pathlist[dirIx].path[pathIx].ry2 += dy;
    } else {
        /* Not a RCT or RDT - error - unexpected path element type. */
        sprintf(globmsg, "Bad character description file: %s.\n", currentChar);
        LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
    }
}

/*
 Look for zero length closepaths.  If true then add a small
 one unit line to each design.  Because blending can cause
 small arithmetic errors at each point in the path the
 accumulation of such errors might cause the path to not end
 up at the same point where it started.  This can cause a sharp
 angle segment that may cause spike problems in old rasterizers,
 and in the qreducer (used for filling very large size characters).
 The one unit line, which is drawn by the closepath, takes up the
 slack for the arithmetic errors and avoids the spike problem.
 */
static void CheckForZeroLengthCP()
{
    indx ix, pathIx;
    
    for (ix = 0; ix < dirCount; ix++)
    {
        for (pathIx = 0; pathIx < path_entries; pathIx++)
        {
            if (pathlist[ix].path[pathIx].type == CP && ZeroLengthCP(ix, pathIx))
            {
                if (cubeLibrary)
                    AddLineCube(ix, pathIx - 1);
                else
                    AddLine(ix, pathIx - 1);
            }
        }
    }
}

typedef struct path_names_ {
    char dirnam[MAXPATHLEN];
    char fullfilepath[MAXPATHLEN];
} Path_Name_;

#if !IS_LIB
static Path_Name_ Path_Names[MAXDESIGNS];
#else
static Path_Name_ Path_Names[1];
#endif

/* Checks that character paths for multiple masters have the same
 number of points and in the same path order.  If this isn't the
 case the character is not included in the font. */
static boolean CompareCharPaths(char *filename, boolean fortransitionals)
{
    indx dirix, ix, i;
    long totalPathElt, minPathLen;
    boolean ok = TRUE;
    short type1, type2;
    
    totalPathElt = minPathLen = MAXINT;
    if (pathlist == NULL)
	{
#if DOMEMCHECK
        pathlist = (PPathList)memck_malloc(MAXDESIGNS * sizeof(PathList));
#else
        pathlist = (PPathList) AllocateMem(
                                           (unsigned int) MAXDESIGNS /*dirCount*/,
                                           sizeof(PathList),
                                           "character path list");
#endif
	}
    
    for (dirix = 0; dirix < dirCount; dirix++)
    {
        
#if OLD
        SetMasterDir(dirix);
        SetReadFileName(filename);
        setPrefix(bezdir);
#else
        SetReadFileName(Path_Names[dirix].fullfilepath);
#endif
        ResetMaxPathEntries();
        SetCurrPathList(&pathlist[dirix]);
        path_entries = 0;
        
        /* read hints from special file if it exists */
        if (hintsdirIx == dirix) {
            char hintFileName[MAXPATHLEN];
            sprintf(hintFileName, "%s/hints/%s", Path_Names[dirix].dirnam, filename);
            if (FileExists(hintFileName, FALSE)) {
                if (!ReadCharFile(TRUE, TRUE, FALSE, FALSE))
                    return FALSE;
#if ! AC_C_LIB
                if (!ReadHintsFile(hintFileName, &pathlist[dirix]))
#endif
                    return FALSE;
                
            } else {
                /* read char data and hints from bez file */
                if (!ReadCharFile(TRUE, TRUE, addHints, FALSE))
                    return FALSE;
            }
        } else {
            /* read char data only */
            if (!ReadCharFile(TRUE, TRUE, FALSE, FALSE))
                return FALSE;
        }
        
        if (dirix == 0)
            totalPathElt = path_entries;
        else
            if (path_entries != totalPathElt)
            {
                InconsistentPointCount(filename, dirix, totalPathElt, path_entries);
                ok = FALSE;
            }
        minPathLen = NUMMIN(NUMMIN(path_entries, totalPathElt), minPathLen);
    }
    
    for (dirix = 1; dirix < dirCount; dirix++) {
        for (i = 0; i < minPathLen; i++) {
            if ((type1 = pathlist[0].path[i].type) != (type2 = pathlist[dirix].path[i].type))
            {
                
            if ((type1 == RDT) && (type2 == RCT))
                { /* Change this element in all previous directories to a curve. */
                    ix = dirix - 1;
                    do {
                        ok = ok && ChangetoCurve(ix, i);
                        ix--;
                    } while (ix >= 0);
                }
                else if ((type1 == RCT) && (type2 == RDT))
                    ok = ok && ChangetoCurve(dirix, i);
                else
                {
                    InconsistentPathType(filename, dirix,
                                         pathlist[0].path[i].type, pathlist[dirix].path[i].type, i);
                    ok = FALSE;
                    /* skip to next subpath */
                    while (++i < minPathLen && (pathlist[0].path[i].type != CP));
                }
            }
        }
    }
    return ok;
}

static void SetSbandWidth(char *charname, boolean fortransit, Transitions* trptr, int trgroupnum)
{
#if !AC_C_LIB
    short width;
    Bbox bbox;
#endif
    indx dirix;
    
    for(dirix = 0; dirix < dirCount; dirix++)
    {
        if (fortransit) {
            pathlist[dirix].sb = trptr->transitgrouparray[trgroupnum].assembled_sb[dirix];
            pathlist[dirix].width = trptr->transitgrouparray[trgroupnum].assembled_width[dirix];
        }
        else {
#if AC_C_LIB
            pathlist[dirix].sb = 0;
            pathlist[dirix].width = 1000;
            
#else
            GetWidthandBbox(charname, &width, &bbox, FALSE, dirix);
            pathlist[dirix].sb = bbox.llx;
            pathlist[dirix].width = width;
#endif
        }
    }
}

static void WriteSbandWidth()
{
    short subrix, length, opcount = GetOperandCount(SBX);
    indx ix, j, startix = 0;
    boolean writeSubrOnce, sbsame = TRUE, wsame = TRUE;
    
    for (ix = 1; ix < dirCount; ix++)
    {
        sbsame = sbsame && (pathlist[ix].sb == pathlist[ix-1].sb);
        wsame = wsame && (pathlist[ix].width == pathlist[ix-1].width);
    }
    if (sbsame && wsame)
    {
        sprintf(outstr, "%ld %d ", pathlist[0].sb, pathlist[0].width);
        WriteToBuffer();
    }
    else if (sbsame)
    {
        sprintf(outstr, "%ld ", pathlist[0].sb);
        WriteToBuffer();
        for (j = 0; j < dirCount; j++)
        {
            sprintf(outstr, "%d ",
                    (j == 0) ? pathlist[j].width : pathlist[j].width - pathlist[0].width);
            WriteToBuffer();
        }
        GetLengthandSubrIx(1, &length, &subrix);
        WriteSubr(subrix);
    }
    else if (wsame)
    {
        for (j = 0; j < dirCount; j++)
        {
            sprintf(outstr, "%ld ",
                    (j == 0) ? pathlist[j].sb : pathlist[j].sb - pathlist[0].sb);
            WriteToBuffer();
        }
        GetLengthandSubrIx(1, &length, &subrix);
        WriteSubr(subrix);
        sprintf(outstr, "%d ", pathlist[0].width);
        WriteToBuffer();
    }
    else
    {
        GetLengthandSubrIx(opcount, &length, &subrix);
        if ((writeSubrOnce = (length == opcount)))
        {
            sprintf(outstr, "%ld %d ", pathlist[0].sb, pathlist[0].width);
            WriteToBuffer();
            length = startix = 1;
        }
        for (ix = 0; ix < opcount; ix += length)
        {
            for(j = startix; j < dirCount; j++)
            {
                sprintf(outstr, "%ld ", (ix == 0) ?
                        (j == 0) ? pathlist[j].sb : pathlist[j].sb - pathlist[0].sb :
                        (j == 0) ? (long int)pathlist[j].width : (long int)(pathlist[j].width - pathlist[0].width));
                WriteToBuffer();
            }
            if (!writeSubrOnce || (ix == (opcount - 1)))
                WriteSubr(subrix);
        }
    }
    WriteStr("sbx\n");
}

static boolean CurveBBox(indx dirIx, short hinttype, long pathIx, Fixed *value)
{
    Cd startPt, endPt;
    Fixed llx, lly, urx, ury, minval, maxval;
    Fixed p1, p2, *minbx, *maxbx;
    CharPathElt pathElt;
    
    *value = IntToFix(10000);
    pathElt = pathlist[dirIx].path[pathIx];
    GetEndPoints(dirIx, pathIx, &startPt, &endPt);
    switch (hinttype)
    {
        case RB:
        case RV + ESCVAL:
            minval = TFMY(NUMMIN(startPt.y, endPt.y));
            maxval = TFMY(NUMMAX(startPt.y, endPt.y));
            p1 = TFMY(pathElt.y1);
            p2 = TFMY(pathElt.y2);
            minbx = &lly; maxbx = &ury;
            break;
        case RY:
        case RM + ESCVAL:
            minval = TFMX(NUMMIN(startPt.x, endPt.x));
            maxval = TFMX(NUMMAX(startPt.x, endPt.x));
            p1 = TFMX(pathElt.x1);
            p2 = TFMX(pathElt.x2);
            minbx = &llx; maxbx = &urx;
            break;
        default:
            sprintf(globmsg, "Illegal hint type in character: %s.\n", currentChar);
            LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
    }
    if (p1 - maxval >= FixOne || p2 - maxval >= FixOne ||
        p1 - minval <= FixOne || p2 - minval <= FixOne)
    {
        /* Transform coordinates so I get the same value that AC would give. */
        FindCurveBBox(TFMX(startPt.x), TFMY(startPt.y), TFMX(pathElt.x1), TFMY(pathElt.y1),
                      TFMX(pathElt.x2), TFMY(pathElt.y2), TFMX(endPt.x), TFMY(endPt.y),
                      &llx, &lly, &urx, &ury);
        if (*maxbx > maxval  || minval > *minbx)
        {
            if (minval - *minbx > *maxbx - maxval)
                *value = (hinttype == RB || hinttype == RV + ESCVAL) ? ITFMY(*minbx) : ITFMX(*minbx);
            else
                *value = (hinttype == RB || hinttype == RV + ESCVAL) ? ITFMY(*maxbx) : ITFMX(*maxbx);
            return TRUE;
        }
    }
    return FALSE;
}
#if __CENTERLINE__
static char * _HintType_(int typ)
{
    switch (typ) {
        case GHOST: return ("Ghost"); break;
        case AVERAGE: return ("Average"); break;
        case CURVEBBOX: return ("Curvebbox"); break;
        case ENDPT: return ("Endpt"); break;
        case STARTPT: return ("Startpt"); break;
        case FLATTEN: return ("Flatten"); break;
    }
    return ("Unknown");
}
static char * _HintKind_ (int hinttype)
{
    switch (hinttype)
    {
        case RB: return ("RB"); break;
        case RV + ESCVAL: return ("RV");
            break;
        case RY: return ("RY"); break;
        case RM + ESCVAL: return ("RM");
            break;
    }
    return("??");
}
static char * _elttype_ (int indx)
{
    switch (pathlist[hintsdirIx].path[indx].type) {
        case RMT: return ("Moveto"); break;
        case RDT: return ("Lineto"); break;
        case RCT: return ("Curveto"); break;
        case CP: return ("Closepath"); break;
    }
    return ("Unknown");
}

#endif

static boolean nearlyequal_ (Fixed a, Fixed b, Fixed tolerance)
{
    return (ABS(a - b) <= tolerance);
}

/* Returns whether the hint values are derived from the start,
 average, end or flattened curve with an inflection point of
 the specified path element. Since path element numbers in
 character files start from one and the path array starts
 from zero we need to subtract one from the path index. */
static short GetPointType(short hinttype, Fixed value, long *pathEltIx)
{
    Cd startPt, endPt;
    Fixed startval, endval, loc;
    short pathtype;
    boolean tryAgain = TRUE;
    long pathIx = *pathEltIx - 1;
    
#if __CENTERLINE__
    if (TRACE) {
        fprintf(stderr, "Enter GetPointType: Hinttype=%s @(%.2f) curr patheltix=%d pathIx=%d  <%s>",_HintKind_(hinttype), FIXED2FLOAT(value), *pathEltIx, pathIx, _elttype_(pathIx));
    }
#endif
    
retry:
    GetEndPoints(hintsdirIx, pathIx, &startPt, &endPt);
    switch (hinttype)
    {
        case RB:
        case RV + ESCVAL:
            startval = startPt.y;
            endval = endPt.y;
#if __CENTERLINE__
            if (TRACE) fprintf(stderr, "Startval Y=%.2f EndVal Y=%.2f ", FIXED2FLOAT(startval), FIXED2FLOAT(endval));
#endif
            break;
        case RY:
        case RM + ESCVAL:
            startval = startPt.x;
            endval = endPt.x;
#if __CENTERLINE__
            if (TRACE) fprintf(stderr, "Startval X=%.2f EndVal X=%.2f ", FIXED2FLOAT(startval), FIXED2FLOAT(endval));
#endif
            break;
        default:
            sprintf(globmsg, "Illegal hint type in character: %s.\n", currentChar);
            LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
    }
    
    /* Check for exactly equal first, in case endval = startval + 1. jvz 1nov95 */
    /* Certain cases are still ambiguous. */
    
    if (value == startval) {
#if __CENTERLINE__
        if (TRACE) fprintf(stderr,"==> StartPt\n");
#endif
        return STARTPT;
    }
    else if (value == endval) {
#if __CENTERLINE__
        if (TRACE) fprintf(stderr,"==> EndPt\n");
#endif
        return ENDPT;
    }
    else if (nearlyequal_ (value, startval, FixOne)) {
#if __CENTERLINE__
        if (TRACE) fprintf(stderr,"==> ~StartPt\n");
#endif
        return STARTPT;
    }
    else if (nearlyequal_ (value, endval, FixOne)) {
#if __CENTERLINE__
        if (TRACE) fprintf(stderr,"==> ~EndPt\n");
#endif
        return ENDPT;
    }
    else if (value == (loc = FixHalfMul(startval+endval)) ||
             nearlyequal_ (value, loc, FixOne)) {
#if __CENTERLINE__
        if (TRACE) fprintf(stderr,"==> Average\n");
#endif
        return AVERAGE;
    }
    
    pathtype = pathlist[hintsdirIx].path[pathIx].type;
    if (tryAgain && (pathIx + 1 < path_entries) && (pathtype != CP))
    { /* try looking at other end of line or curve */
        pathIx++;
        *pathEltIx += 1;
        tryAgain = FALSE;
#if __CENTERLINE__
        if (TRACE) fprintf(stderr," (Retry w/PathEltix=%d) ", *pathEltIx);
#endif
        goto retry;
    }
    if (!tryAgain) /* reset pathEltIx to original value */ {
        *pathEltIx -= 1;
#if __CENTERLINE__
        if (TRACE) fprintf(stderr," (reset PathEltix to %d) ", *pathEltIx);
#endif
    }
    if (CurveBBox(hintsdirIx, hinttype, *pathEltIx-1, &loc) &&
        nearlyequal_ (value, loc, FixOne)) {
#if __CENTERLINE__
        if (TRACE) fprintf(stderr,"==> Curvebbox\n");
#endif
        return CURVEBBOX;
    }
#if __CENTERLINE__
    if (TRACE) fprintf(stderr,"==> Flatten fallout\n");
#endif
    return FLATTEN;
}

static void GetRelPos(long pathIx, short hinttype, Fixed hintVal,
                      CdPtr startPt, CdPtr endPt, Fixed *val)
{
    Cd origStart, origEnd;
    
    GetEndPoints(hintsdirIx, pathIx, &origStart, &origEnd);
    if (hinttype == RB || hinttype == (RV + ESCVAL))
        GetRelativePosition(endPt->y, startPt->y, origEnd.y, origStart.y, hintVal, val);
    else
        GetRelativePosition(endPt->x, startPt->x, origEnd.x, origStart.x, hintVal, val);
}

/* Calculates the relative position of hintVal between its endpoints and
 gets new hint value between currEnd and currStart. */
static void GetRelativePosition(currEnd, currStart, end, start, hintVal, fixedRelValue)
Fixed currEnd, currStart, end, start, hintVal, *fixedRelValue;
{
    float relVal;
    
    if ((end - start) == 0)
        *fixedRelValue = (Fixed)LROUND((float)(hintVal - start) + currStart);
    else
    {
        relVal = (float)(hintVal - start)/(float)(end - start);
        *fixedRelValue = (Fixed)LROUND(((currEnd - currStart) * relVal) + currStart);
    }
}

/* For each base design, excluding HintsDir, include the
 hint information at the specified path element. type1
 and type2 indicates whether to use the start, end, avg.,
 curvebbox or flattened curve.  If a curve is to be flattened
 check if this is an "s" curve and use the inflection point.  If not
 then use the same relative position between the two endpoints as
 in the main hints dir.
 hinttype is either RB, RY, RM or RV. pathEltIx
 is the index into the path array where the new hint should
 be stored.  pathIx is the index of the path segment used to
 calculate this particular hint. */
static void InsertHint(PHintElt currHintElt, indx pathEltIx,
                       short type1, short type2)
{
    indx ix, j;
    Cd startPt, endPt;
    PHintElt *hintElt, newEntry;
    CharPathElt pathElt;
    long pathIx;
    short pathtype, hinttype = currHintElt->type;
    Fixed *value, ghostVal, tempVal;
    
#if __CENTERLINE__
    if (TRACE) {
        fprintf(stderr, "InsertHint: ");
        fprintf(stderr,"Type1=%s Type2=%s", _HintType_(type1), _HintType_(type2));    fprintf(stderr, " PathEltIndex= ");
        if (pathEltIx == MAINHINTS) {fprintf(stderr,"MainHints ");}
        else {
            Fixed tx, ty;
            fprintf(stderr,"%d ", pathEltIx);
            if (type1 != GHOST) {
                GetEndPoint(hintsdirIx, currHintElt->pathix1 - 1, &tx, &ty);
                fprintf(stderr,"Start attached to (%.2f %.2f)", FIXED2FLOAT(tx), FIXED2FLOAT(ty));
            }
            if (type2 != GHOST) {
                GetEndPoint(hintsdirIx, currHintElt->pathix2 - 1, &tx, &ty);
                fprintf(stderr,"End attached to (%.2f %.2f)", FIXED2FLOAT(tx), FIXED2FLOAT(ty));
            }
        }
        fprintf(stderr,"\n");
    }
#endif
    
    if (type1 == GHOST || type2 == GHOST)
    /* ghostVal should be -20 or -21 */
        ghostVal = currHintElt->rightortop - currHintElt->leftorbot;
    for (ix = 0; ix < dirCount; ix++)
    {
        if (ix == hintsdirIx)
            continue;
#if DOMEMCHECK
        newEntry = (PHintElt) memck_malloc (sizeof(HintElt));
#else
        newEntry = (PHintElt) AllocateMem (1, sizeof(HintElt), "hint element");
#endif
        newEntry->type = hinttype;
        hintElt =
        (pathEltIx == MAINHINTS ? &pathlist[ix].mainhints : &pathlist[ix].path[pathEltIx].hints);
        while (*hintElt != NULL && (*hintElt)->next != NULL)
            hintElt = &(*hintElt)->next;
        if (*hintElt == NULL)
            *hintElt = newEntry;
        else
            (*hintElt)->next = newEntry;
        for (j = 0; j < 2; j++)
        {
            pathIx = (j == 0 ? currHintElt->pathix1 - 1 : currHintElt->pathix2 - 1);
            pathtype = (j == 0 ? type1 : type2);
            value = (j == 0 ? &newEntry->leftorbot : &newEntry->rightortop);
            if (pathtype != GHOST)
                GetEndPoints(ix, pathIx, &startPt, &endPt);
            switch(pathtype)
            {
                case AVERAGE:
                    *value = ((hinttype == RB || hinttype == (RV + ESCVAL)) ? FixHalfMul(startPt.y+endPt.y) : FixHalfMul(startPt.x+endPt.x));
                    break;
                case CURVEBBOX:
                    if (!CurveBBox(ix, hinttype, pathIx, value))
                    {
                        GetRelPos(pathIx, hinttype,
                                  ((j == 0) ? currHintElt->leftorbot : currHintElt->rightortop),
                                  &startPt, &endPt, &tempVal);
                        *value = FRnd(tempVal);
                    }
                    break;
                case ENDPT:
                    *value = ((hinttype == RB || hinttype == (RV + ESCVAL)) ? endPt.y : endPt.x);
                    break;
                case FLATTEN:
                    pathElt = pathlist[ix].path[pathIx];
                    if (pathElt.type != RCT)
                    {
                        sprintf(globmsg, "Malformed path list: %s, dir: %d, element: %ld != RCT.\n",
                                currentChar, ix, pathIx);
                        LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
                    }
                    if (!GetInflectionPoint(startPt.x, startPt.y, pathElt.x1, pathElt.y1,
                                            pathElt.x2, pathElt.y2, pathElt.x3, pathElt.y3, value))
                    { /* no flat spot found */
                        /* Get relative position of value in currHintElt. */
                        GetRelPos(pathIx, hinttype,
                                  ((j == 0) ? currHintElt->leftorbot : currHintElt->rightortop),
                                  &startPt, &endPt, &tempVal);
                        *value = FRnd(tempVal);
                    }
                    break;
                case GHOST:
                    if (j == 1)
                        *value = newEntry->leftorbot + ghostVal;
                    break;
                case STARTPT:
                    *value = ((hinttype == RB || hinttype == (RV + ESCVAL)) ? startPt.y : startPt.x);
                    break;
                default:
                    sprintf(globmsg, "Illegal point type in character: %s.\n", currentChar);
                    LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
            }
            /* Assign correct value for bottom band if first path element
             is a ghost band. */
            if (j == 1 && type1 == GHOST)
                newEntry->leftorbot = newEntry->rightortop - ghostVal;
        }
    }
}

static void ReadHints(hintElt, pathEltIx)
PHintElt hintElt;
indx pathEltIx;
{
    PHintElt currElt = hintElt;
    short pointtype1, pointtype2;
    
    while (currElt != NULL)
    {
        if (currElt->pathix1 != 0)
            pointtype1 =
            GetPointType(currElt->type, currElt->leftorbot, &(currElt->pathix1));
        else pointtype1 = GHOST;
        if (currElt->pathix2 != 0)
            pointtype2 =
            GetPointType(currElt->type, currElt->rightortop, &(currElt->pathix2));
        else pointtype2 = GHOST;
        InsertHint(currElt, pathEltIx, pointtype1, pointtype2);
        currElt = currElt->next;
    }
}

/* Reads hints from hints directory path list and assigns corresponding
 hints to other designs. */
static int ReadandAssignHints()
{
    indx ix;
    
    /* Check for main hints first, i.e. global to character. */
    if (pathlist[hintsdirIx].mainhints != NULL)
        ReadHints(pathlist[hintsdirIx].mainhints, MAINHINTS);
    /* Now check for local hint values. */
    for (ix = 0; ix < path_entries; ix++) {
        if (pathlist[hintsdirIx].path == NULL)
            return 1;
        if (pathlist[hintsdirIx].path[ix].hints != NULL)
            ReadHints(pathlist[hintsdirIx].path[ix].hints, ix);
    }
    return 0;
}

static boolean DoubleCheckFlexVals(dirnum, eltix, hintdirnum)
indx dirnum, eltix, hintdirnum;
{
    boolean vert = (pathlist[hintdirnum].path[eltix].x == pathlist[hintdirnum].path[eltix+1].x3);
    if (vert) {
        return (pathlist[dirnum].path[eltix].x == pathlist[dirnum].path[eltix+1].x3);
    }
    else {
        return (pathlist[dirnum].path[eltix].y == pathlist[dirnum].path[eltix+1].y3);
    }
}

static boolean CheckFlexOK(ix)
indx ix;
{
    indx i;
    boolean flexOK = pathlist[hintsdirIx].path[ix].isFlex;
    PCharPathElt end;
    char pathdir[MAXPATHLEN];
    
    for (i = 0; i < dirCount; i++) {
        if (i == hintsdirIx) continue;
        if (flexOK && (! pathlist[i].path[ix].isFlex))
        {
            if (!DoubleCheckFlexVals(i, ix, hintsdirIx)) {
                end = &pathlist[i].path[ix];
                GetMasterDirName(pathdir, i);
                sprintf(globmsg, "Flex will not be included in character: %s in '%s' at element %d near (%ld, %ld) because the character does not have flex in each design.\n", currentChar, pathdir, (int) ix, FTrunc8(end->x), FTrunc8(end->y));
                LogMsg(globmsg, WARNING, OK, TRUE);
                return FALSE;
            }
            else {
                pathlist[i].path[ix].isFlex = flexOK;
            }
        }
    }
    return flexOK;
}

static void OptimizeCT(ix)
indx ix;
{
    short newtype;
    boolean vhct = TRUE, hvct = TRUE;
    indx i;
    
    for (i = 0; i < dirCount; i++)
        if (pathlist[i].path[ix].rx1 != 0 || pathlist[i].path[ix].ry3 != 0)
        {
            vhct = FALSE;
            break;
        }
    for (i = 0; i < dirCount; i++)
        if (pathlist[i].path[ix].ry1 != 0 || pathlist[i].path[ix].rx3 != 0)
        {
            hvct = FALSE;
            break;
        }
    if (vhct)
        newtype = VHCT;
    else if (hvct)
        newtype = HVCT;
    if (vhct || hvct)
        for (i = 0; i < dirCount; i++)
            pathlist[i].path[ix].type = newtype;
}

static void MtorDt(Cd coord, indx startix, short length)
{
    if (length == 2) {WriteCd(coord);}
    else
        if (startix == 0) WriteX(coord.x);
        else WriteY(coord.y);
}

static void Hvct(Cd coord1, Cd coord2, Cd coord3, indx startix, short length)
{
    indx ix;
    indx lastix = startix + length;
    
    for (ix = startix; ix < lastix; ix++)
        switch(ix)
    {
        case 0:
            WriteX(coord1.x);
            break;
        case 1:
            WriteX(coord2.x);
            break;
        case 2:
            WriteY(coord2.y);
            break;
        case 3:
            WriteY(coord3.y);
            break;
        default:
            sprintf(globmsg, "Invalid index value: %d defined for curveto command1 in character: %s.\n", (int)ix, currentChar);
            LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
            break;
    }
}

static void Vhct(Cd coord1, Cd coord2, Cd coord3, indx startix, short length)
{
    indx ix;
    indx lastix = startix + length;
    
    for (ix = startix; ix < lastix; ix++)
        switch(ix)
    {
        case 0:
            WriteY(coord1.y);
            break;
        case 1:
            WriteX(coord2.x);
            break;
        case 2:
            WriteY(coord2.y);
            break;
        case 3:
            WriteX(coord3.x);
            break;
        default:
            sprintf(globmsg,"Invalid index value: %d defined for curveto command2 in character:%s.\n", (int)ix, currentChar);
            LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
            break;
    }
}

/* length can only be 1, 2, 3 or 6 */
static void Ct(Cd coord1, Cd coord2, Cd coord3, indx startix, short length)
{
    indx ix;
    indx lastix = startix + length;
    
    for (ix = startix; ix < lastix; ix++)
        switch(ix)
    {
        case 0:
            WriteX(coord1.x);
            break;
        case 1:
            WriteY(coord1.y);
            break;
        case 2:
            WriteX(coord2.x);
            break;
        case 3:
            WriteY(coord2.y);
            break;
        case 4:
            WriteX(coord3.x);
            break;
        case 5:
            WriteY(coord3.y);
            break;
        default:
            sprintf(globmsg,"Invalid index value: %d defined for curveto command3 in character: %s.\n", (int)ix, currentChar);
            LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
            break;
    }
}

static void ReadHorVStem3Values(indx pathIx, short eltno, short hinttype,
                                boolean *errormsg)
{
    indx ix;
    PHintElt *hintElt;
    short count, newhinttype;
    boolean ok = TRUE;
    Fixed min, dmin, mid, dmid, max, dmax;
    char dirname[MAXPATHLEN];
    
    for (ix = 0; ix < dirCount; ix++)
    {
        count = 1;
        if (ix == hintsdirIx)
            continue;
        GetMasterDirName(dirname, ix);
        hintElt = (pathIx == MAINHINTS ? &pathlist[ix].mainhints:&pathlist[ix].path[pathIx].hints);
        /* Find specified hint element. */
        while (*hintElt != NULL && count != eltno)
        {
            hintElt = &(*hintElt)->next;
            count++;
        }
        /* Check that RM or RV type is in pairs of threes. */
        if (*hintElt == NULL || (*hintElt)->next == NULL || (*hintElt)->next->next == NULL)
        {
            sprintf(globmsg, "Invalid format for hint operator: hstem3 or vstem3 in character: %s/%s.\n", dirname, currentChar);
            LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
        }
        if ((*hintElt)->type != hinttype || (*hintElt)->next->type != hinttype
            || (*hintElt)->next->next->type != hinttype)
        {
            sprintf(globmsg, "Invalid format for hint operator: hstem3 or vstem3 in character: %s in '%s'.\n", currentChar, dirname);
            LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
        }
        min = (*hintElt)->leftorbot;
        dmin = (*hintElt)->rightortop - min;
        mid = (*hintElt)->next->leftorbot;
        dmid = (*hintElt)->next->rightortop - mid;
        max = (*hintElt)->next->next->leftorbot;
        dmax = (*hintElt)->next->next->rightortop - max;
        /* Check that counters are the same width and stems are the same width. */
        if (dmin != dmax ||
            (((mid + dmid/2) - (min + dmin/2)) != ((max + dmax/2) - (mid + dmid/2))))
        {
            ok = FALSE;
            break;
        }
    }
    if (!ok)
    { /* change RM's to RY's or RV's to RB's for this element in each directory */
        newhinttype = (hinttype == (RM + ESCVAL) ? RY : RB);
        if (*errormsg)
        {
            sprintf(globmsg, "Near miss for using operator: %s in character: %s in '%s'. (min=%ld..%ld[delta=%ld], mid=%ld..%ld[delta=%ld], max=%ld..%ld[delta=%ld])\n",
                    (hinttype == (RM + ESCVAL)) ? "vstem3" : "hstem3",
                    currentChar, dirname,
                    FTrunc8((*hintElt)->leftorbot), FTrunc8((*hintElt)->rightortop), FTrunc8((*hintElt)->rightortop - (*hintElt)->leftorbot),
                    FTrunc8((*hintElt)->next->leftorbot), FTrunc8((*hintElt)->next->rightortop), FTrunc8((*hintElt)->next->rightortop - (*hintElt)->next->leftorbot),
                    FTrunc8((*hintElt)->next->next->leftorbot), FTrunc8((*hintElt)->next->next->rightortop), FTrunc8((*hintElt)->next->next->rightortop - (*hintElt)->next->next->leftorbot));
            LogMsg(globmsg, WARNING, OK, TRUE);
            *errormsg = FALSE;
        }
        for (ix = 0; ix < dirCount; ix++)
        {
            count = 1;
            hintElt = (pathIx == MAINHINTS ? &pathlist[ix].mainhints:&pathlist[ix].path[pathIx].hints);
            /* Find specified hint element. */
            while (*hintElt != NULL && count != eltno)
            {
                hintElt = &(*hintElt)->next;
                count++;
            }
            /* Already checked that hintElt->next, etc. exists,
             so don't need to do it again. */
            (*hintElt)->type = newhinttype;
            (*hintElt)->next->type = newhinttype;
            (*hintElt)->next->next->type = newhinttype;
        }
    }
}

/* Go through each hint element and check that all rm's and rv's
 meet the necessary criteria. */
static void FindHandVStem3(hintElt, pathIx, errormsg)
PHintElt *hintElt;
indx pathIx;
boolean *errormsg;
{
    short count = 1;
    
    while (*hintElt != NULL)
    {
        if ((*hintElt)->type == (RM + ESCVAL) || (*hintElt)->type == (RV + ESCVAL))
        {
            ReadHorVStem3Values(pathIx, count, (*hintElt)->type, errormsg);
            /* RM's and RV's are in pairs of 3 */
            hintElt = &(*hintElt)->next->next->next;
            count += 3;
        }
        else
        {
            hintElt = &(*hintElt)->next;
            count++;
        }
    }
}

static void CheckHandVStem3()
{
    indx ix;
    boolean errormsg = TRUE;
    
    FindHandVStem3(&pathlist[hintsdirIx].mainhints, MAINHINTS, &errormsg);
    for (ix = 0; ix < path_entries; ix++)
        FindHandVStem3(&pathlist[hintsdirIx].path[ix].hints, ix, &errormsg);
}

static void CheckFlexValues(short *operator, indx eltix, indx flexix,
                            boolean *xequal, boolean *yequal)
{
    indx ix;
    Cd coord;
    
    *operator = RMT;
    if (flexix < 2) return;
    
    *xequal = *yequal = TRUE;
    for (ix = 1; ix < dirCount; ix++)
        switch (flexix)
    {
        case 2:
            if ((coord.x = pathlist[ix].path[eltix].rx2) != pathlist[ix-1].path[eltix].rx2)
                *xequal = FALSE;
            if ((coord.y = pathlist[ix].path[eltix].ry2) != pathlist[ix-1].path[eltix].ry2)
                *yequal = FALSE;
            break;
        case 3:
            if ((coord.x = pathlist[ix].path[eltix].rx3) != pathlist[ix-1].path[eltix].rx3)
                *xequal = FALSE;
            if ((coord.y = pathlist[ix].path[eltix].ry3) != pathlist[ix-1].path[eltix].ry3)
                *yequal = FALSE;
            break;
        case 4:
            if ((coord.x = pathlist[ix].path[eltix+1].rx1) != pathlist[ix-1].path[eltix+1].rx1)
                *xequal = FALSE;
            if ((coord.y = pathlist[ix].path[eltix+1].ry1) != pathlist[ix-1].path[eltix+1].ry1)
                *yequal = FALSE;
            break;
        case 5:
            if ((coord.x = pathlist[ix].path[eltix+1].rx2) != pathlist[ix-1].path[eltix+1].rx2)
                *xequal = FALSE;
            if ((coord.y = pathlist[ix].path[eltix+1].ry2) != pathlist[ix-1].path[eltix+1].ry2)
                *yequal = FALSE;
            break;
        case 6:
            if ((coord.x = pathlist[ix].path[eltix+1].rx3) != pathlist[ix-1].path[eltix+1].rx3)
                *xequal = FALSE;
            if ((coord.y = pathlist[ix].path[eltix+1].ry3) != pathlist[ix-1].path[eltix+1].ry3)
                *yequal = FALSE;
            break;
        case 7:
            if ((coord.x = pathlist[ix].path[eltix+1].x3) != pathlist[ix-1].path[eltix+1].x3)
                *xequal = FALSE;
            if ((coord.y = pathlist[ix].path[eltix+1].y3) != pathlist[ix-1].path[eltix+1].y3)
                *yequal = FALSE;
            break;
    }
    if (!(*xequal) && !(*yequal))
        return;
    
    if (*xequal && (coord.x == 0))
    {
        *operator = VMT;
        *xequal = FALSE;
    }
    if (*yequal && (coord.y == 0))
    {
        *operator = HMT;
        *yequal = FALSE;
    }
}

static void GetFlexCoord(rmtCt, dirix, eltix, coord)
indx rmtCt, dirix, eltix;
CdPtr coord;
{
    switch (rmtCt)
    {
        case 0:
            (*coord).x = refPtArray[dirix].x - pathlist[dirix].path[eltix].x;
            (*coord).y = refPtArray[dirix].y - pathlist[dirix].path[eltix].y;
            break;
        case 1:
            (*coord).x = pathlist[dirix].path[eltix].x1 - refPtArray[dirix].x;
            (*coord).y = pathlist[dirix].path[eltix].y1 - refPtArray[dirix].y;
            break;
        case 2:
            (*coord).x = pathlist[dirix].path[eltix].rx2;
            (*coord).y = pathlist[dirix].path[eltix].ry2;
            break;
        case 3:
            (*coord).x = pathlist[dirix].path[eltix].rx3;
            (*coord).y = pathlist[dirix].path[eltix].ry3;
            break;
        case 4:
            (*coord).x = pathlist[dirix].path[eltix+1].rx1;
            (*coord).y = pathlist[dirix].path[eltix+1].ry1;
            break;
        case 5:
            (*coord).x = pathlist[dirix].path[eltix+1].rx2;
            (*coord).y = pathlist[dirix].path[eltix+1].ry2;
            break;
        case 6:
            (*coord).x = pathlist[dirix].path[eltix+1].rx3;
            (*coord).y = pathlist[dirix].path[eltix+1].ry3;
            break;
        case 7:
            (*coord).x = pathlist[dirix].path[eltix+1].x3;
            (*coord).y = pathlist[dirix].path[eltix+1].y3;
            break;
    }
}

/*  NOTE:  Mathematical transformations are applied to the multi-master
 data in order to decrease the computation during font execution.
 See /user/foley/atm/blendfont1.910123 for the definition of this
 transformation.  This transformation is applied whenever charstring
 data is written  (i.e. sb and width, flex, hints, dt, ct, mt) AND
 an OtherSubr 7 - 11 will be called. */
static void WriteFlex(eltix)
indx eltix;
{
#if ! AC_C_LIB
    boolean vert = (pathlist[hintsdirIx].path[eltix].x == pathlist[hintsdirIx].path[eltix+1].x3);
    Cd coord, coord0; /* array of reference points */
    boolean xsame, ysame, writeSubrOnce;
    char operator[MAXOPLEN]; /* rmt, hmt, vmt */
    short optype;
    indx ix, j, opix, startix;
    short opcount, subrIx, length;
    
#if DOMEMCHECK
    refPtArray = (Cd *) memck_malloc (dirCount * sizeof(Cd));
#else
    refPtArray = (Cd *) AllocateMem (dirCount, sizeof(Cd), "reference point array");
#endif
    for (ix = 0; ix < dirCount; ix++)
    {
        refPtArray[ix].x = (vert ? pathlist[ix].path[eltix].x : pathlist[ix].path[eltix].x3);
        refPtArray[ix].y = (vert ? pathlist[ix].path[eltix].y3 : pathlist[ix].path[eltix].y);
    }
    WriteStr("1 subr\n");
    for (j = 0; j < 8; j++)
    {
        if (j == 7)
            WRTNUM(DMIN);
        xsame = ysame = FALSE;
        CheckFlexValues(&optype, eltix, j, &xsame, &ysame);
        opcount = GetOperandCount(optype);
        if ((xsame && !ysame) || (!xsame && ysame))
            GetLengthandSubrIx((opcount = 1), &length, &subrIx);
        else
            GetLengthandSubrIx(opcount, &length, &subrIx);
        GetOperator(optype, operator);
        GetFlexCoord(j, 0, eltix, &coord);
        coord0.x = coord.x;
        coord0.y = coord.y;
        if (j == 7) {
            if (!xsame && (optype == VMT))
                WriteX(coord.x); /* usually=0. cf bottom of "CheckFlexValues" */
        }
        if (xsame && ysame)
        {WriteCd(coord);}
        else if (xsame)
        {
            WriteX(coord.x);
            if (optype != HMT)
            {
                for (ix = 0; ix < dirCount; ix++)
                {
                    GetFlexCoord(j, ix, eltix, &coord);
                    WriteY((ix == 0 ? coord.y : coord.y - coord0.y));
                }
                WriteSubr(subrIx);
            }
        }
        else if (ysame)
        {
            if (optype != VMT)
            {
                for (ix = 0; ix < dirCount; ix++)
                {
                    GetFlexCoord(j, ix, eltix, &coord);
                    WriteX((ix == 0 ? coord.x : coord.x - coord0.x));
                }
                WriteSubr(subrIx);
            }
            WriteY(coord.y);
        }
        else
        {
            startix = 0;
            if ((writeSubrOnce = (length == opcount)))
            {
                if (optype == HMT)
                    WriteX(coord.x);
                else if (optype == VMT)
                    WriteY(coord.y);
                else WriteCd(coord);
                length = startix = 1;
            }
            for (opix = 0; opix < opcount; opix += length)
            {
                for (ix = startix; ix < dirCount; ix++)
                {
                    GetFlexCoord(j, ix, eltix, &coord);
                    if (ix != 0)
                    {
                        coord.x -= coord0.x;
                        coord.y -= coord0.y;
                    }
                    switch (optype)
                    {
                        case HMT: WriteX(coord.x); break;
                        case VMT: WriteY(coord.y); break;
                        case RMT: MtorDt(coord, opix, length); break;
                    }
                }
                if (!writeSubrOnce || (opix == (opcount - 1)))
                    WriteSubr(subrIx);
            } /* end of for opix */
        } /* end of last else clause */
        if (j != 7)
        {
            sprintf(outstr, "%s 2 subr\n", operator);
            WriteToBuffer();
        }
        if (j == 7) {
            if (!ysame && (optype == HMT))
                WriteY(coord.y);  /* usually=0. cf bottom of "CheckFlexValues" */
        }
    } /* end of j for loop */
    WriteStr("0 subr\n");
    flexexists = TRUE;
#if DOMEMCHECK
    memck_free (refPtArray);
#else
    UnallocateMem (refPtArray);
#endif
    
#endif /* AC_C_LIB */
}

static void WriteHints(pathEltIx)
indx pathEltIx;
{
    indx ix, opix, startix;
    short rmcount, rvcount, hinttype;
    short opcount, subrIx, length;
    PHintElt *hintArray;
    boolean lbsame, rtsame, writeSubrOnce;
    
    /* hintArray contains the pointers to the beginning of the linked list of hints for
     each design at pathEltIx. */
#if DOMEMCHECK
    hintArray = (PHintElt *) memck_malloc(dirCount * sizeof(HintElt *));
#else
    hintArray = (PHintElt *) AllocateMem(dirCount, sizeof(HintElt *), "hint element array");
#endif
    /* Initialize hint array. */
    for (ix = 0; ix < dirCount; ix++)
        hintArray[ix] =
        (pathEltIx == MAINHINTS ? pathlist[ix].mainhints : pathlist[ix].path[pathEltIx].hints);
    if (pathEltIx != MAINHINTS)
        WriteStr("beginsubr snc\n");
    rmcount = rvcount = 0;
    while (hintArray[0] != NULL)
    {
        startix = 0;
        hinttype = hintArray[hintsdirIx]->type;
        for (ix = 0; ix < dirCount; ix++)
        {
            hintArray[ix]->rightortop -= hintArray[ix]->leftorbot; /* relativize */
            if ((hinttype == RY || hinttype == (RM + ESCVAL)) && !cubeLibrary)
            /* if it is a cube library, sidebearings are considered to be zero */
            /* for normal fonts, translate vstem hints left by sidebearing */
                hintArray[ix]->leftorbot -= IntToFix(pathlist[ix].sb);
        }
        lbsame = rtsame = TRUE;
        for (ix = 1; ix < dirCount; ix++)
        {
            if (hintArray[ix]->leftorbot != hintArray[ix-1]->leftorbot)
                lbsame = FALSE;
            if (hintArray[ix]->rightortop != hintArray[ix-1]->rightortop)
                rtsame = FALSE;
        }
        if (lbsame && rtsame)
        {
            WriteOneHintVal(hintArray[0]->leftorbot);
            WriteOneHintVal(hintArray[0]->rightortop);
        }
        else if (lbsame)
        {
            WriteOneHintVal(hintArray[0]->leftorbot);
            for (ix = 0; ix < dirCount; ix++)
                WriteOneHintVal((ix == 0 ? hintArray[ix]->rightortop :
                                 hintArray[ix]->rightortop - hintArray[0]->rightortop));
            GetLengthandSubrIx(1, &length, &subrIx);
            WriteSubr(subrIx);
        }
        else if (rtsame)
        {
            for (ix = 0; ix < dirCount; ix++)
                WriteOneHintVal((ix == 0 ? hintArray[ix]->leftorbot :
                                 hintArray[ix]->leftorbot - hintArray[0]->leftorbot));
            GetLengthandSubrIx(1, &length, &subrIx);
            WriteSubr(subrIx);
            WriteOneHintVal(hintArray[0]->rightortop);
        }
        else
        {
            opcount = GetOperandCount(hinttype);
            GetLengthandSubrIx(opcount, &length, &subrIx);
            if ((writeSubrOnce = (length == opcount)))
            {
                WriteOneHintVal(hintArray[0]->leftorbot);
                WriteOneHintVal(hintArray[0]->rightortop);
                length = startix = 1;
            }
            for (opix = 0; opix < opcount; opix+=length)
            {
                for (ix = startix; ix < dirCount; ix++)
                {
                    if (opix == 0)
                        WriteOneHintVal((ix == 0 ? hintArray[ix]->leftorbot :
                                         hintArray[ix]->leftorbot - hintArray[0]->leftorbot));
                    else
                        WriteOneHintVal((ix == 0 ? hintArray[ix]->rightortop :
                                         hintArray[ix]->rightortop - hintArray[0]->rightortop));
                }
                if (!writeSubrOnce || (opix == (opcount - 1)))
                    WriteSubr(subrIx);
            }
        }
        switch (hinttype)
        {
            case RB:
                WriteStr("rb\n");
                break;
            case RV + ESCVAL:
                if (++rvcount == 3)
                {
                    rvcount = 0;
                    WriteStr("rv\n");
                }
                break;
            case RY:
                WriteStr("ry\n");
                break;
            case RM + ESCVAL:
                if (++rmcount == 3)
                {
                    rmcount = 0;
                    WriteStr("rm\n");
                }
                break;
            default:
                sprintf(globmsg, "Illegal hint type: %d in character: %s.\n", hinttype, currentChar);
                LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
        }
        for (ix = 0; ix < dirCount; ix++)
            hintArray[ix] = (hintArray[ix]->next == NULL) ? NULL : hintArray[ix]->next;
    } /* end of while */
    if (pathEltIx != MAINHINTS)
        WriteStr("endsubr enc\nnewcolors\n");
#if DOMEMCHECK
    memck_free(hintArray);
#else
    UnallocateMem (hintArray);
#endif
}


static void WritePathElt(indx dirIx, indx eltIx, short pathType,
                         indx startix, short length)
{
    Cd c1, c2, c3;
    PCharPathElt path, path0;
    
    path = &pathlist[dirIx].path[eltIx];
    path0 = &pathlist[0].path[eltIx];
    
    switch(pathType)
    {
        case HDT:
        case HMT:
            WriteX((dirIx == 0 ? path->rx : path->rx - path0->rx));
            break;
        case VDT:
        case VMT:
            WriteY((dirIx == 0 ? path->ry : path->ry - path0->ry));
            break;
        case RDT:
        case RMT:
            c1.x = (dirIx == 0 ? path->rx : path->rx - path0->rx);
            c1.y = (dirIx == 0 ? path->ry : path->ry - path0->ry);
            MtorDt(c1, startix, length);
            break;
        case HVCT:
        case VHCT:
        case RCT:
            if (dirIx == 0)
            {
                c1.x = path->rx1; c1.y = path->ry1;
                c2.x = path->rx2; c2.y = path->ry2;
                c3.x = path->rx3; c3.y = path->ry3;
            }
            else
            {
                c1.x = path->rx1 - path0->rx1;
                c1.y = path->ry1 - path0->ry1;
                c2.x = path->rx2 - path0->rx2;
                c2.y = path->ry2 - path0->ry2;
                c3.x = path->rx3 - path0->rx3;
                c3.y = path->ry3 - path0->ry3;
            }
            if (pathType == RCT)
                Ct(c1, c2, c3, startix, length);
            else if (pathType == HVCT)
                Hvct(c1, c2, c3, startix, length);
            else Vhct(c1, c2, c3, startix, length);
            break;
        case CP:
            break;
        default:
        {
            sprintf(globmsg, "Illegal path operator %d found in character: %s.\n",
                    (int)pathType, currentChar);
            LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
        }
    }
}

static void OptimizeMtorDt(eltix, op, xequal, yequal)
indx eltix;
short *op;
boolean *xequal, *yequal;
{
    indx ix;
    
    *xequal = *yequal = TRUE;
    for (ix = 1; ix < dirCount; ix++)
    {
        *xequal = *xequal &&
        (pathlist[ix].path[eltix].rx == pathlist[ix-1].path[eltix].rx);
        *yequal = *yequal &&
        (pathlist[ix].path[eltix].ry == pathlist[ix-1].path[eltix].ry);
    }
    if (*xequal && pathlist[0].path[eltix].rx == 0)
    {
        *op = (*op == RMT) ? VMT : VDT;
        *xequal = FALSE;
    }
    else if (*yequal && pathlist[0].path[eltix].ry == 0)
    {
        *op = (*op == RMT) ? HMT : HDT;
        *yequal = FALSE;
    }
}

static boolean CoordsEqual(indx dir1, indx dir2, indx opIx, indx eltIx, short op)
{
    PCharPathElt path1 = &pathlist[dir1].path[eltIx], path2 = &pathlist[dir2].path[eltIx];
    char dirname[MAXPATHLEN];
    
    switch(opIx)
    {
        case 0:
            if (op == RCT || op == HVCT)
                return (path1->rx1 == path2->rx1);
            else /* op == VHCT */
                return (path1->ry1 == path2->ry1);
            break;
        case 1:
            if (op == RCT)
                return (path1->ry1 == path2->ry1);
            else
                return (path1->rx2 == path2->rx2);
            break;
        case 2:
            if (op == RCT)
                return (path1->rx2 == path2->rx2);
            else
                return (path1->ry2 == path2->ry2);
            break;
        case 3:
            if (op == RCT)
                return (path1->ry2 == path2->ry2);
            else if (op == HVCT)
                return (path1->ry3 == path2->ry3);
            else /* op == VHCT */
                return (path1->rx3 == path2->rx3);
            break;
        case 4:
            return (path1->rx3 == path2->rx3);
            break;
        case 5:
            return (path1->ry3 == path2->ry3);
            break;
        default:
            GetMasterDirName(dirname, dir1);
            sprintf(globmsg, "Invalid index value: %d defined for curveto command4 in character: %s. Op=%d, dir=%s near (%ld %ld).\n", (int)opIx, currentChar, (int)op, dirname, FTrunc8(path1->x), FTrunc8(path1->y));
            LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
            break;
    }
	
	return 0;
}

/* Checks if path element values are the same for the RCT, HVCT and VHCT
 operators in each master directory between operands startIx to
 startIx + length.  Returns TRUE if they are the same and FALSE otherwise. */
static boolean SamePathValues(indx eltIx, short op, indx startIx, short length)
{
    indx ix, dirIx;
    /*  PCharPathElt path0 = &pathlist[0].path[eltIx]; */
    boolean same = TRUE;
    
    for (ix = 0; ix < length; ix++)
    {
        for (dirIx = 1; dirIx < dirCount; dirIx++)
            if (!(same = same && CoordsEqual(dirIx, 0, startIx, eltIx, op)))
                return FALSE;
        startIx++;
    }
    return TRUE;
}

/* Takes multiple path descriptions for the same character name and
 combines them into a single path description using new subroutine
 calls 7 - 11. */
static void CombinePaths()
{
#if ! AC_C_LIB
    indx ix, eltix, opix, startIx, dirIx;
    short length, subrIx, opcount, op;
    char operator[MAXOPLEN];
    boolean xequal, yequal;
    
    if (!cubeLibrary)
        WriteSbandWidth();
    if (addHints && (pathlist[hintsdirIx].mainhints != NULL))
        WriteHints(MAINHINTS);
    WriteStr("sc\n");
    firstMT = TRUE;
    for (eltix = 0; eltix < path_entries; eltix++)
    {
        /* A RDT may be tagged 'remove' because it is followed by a point CP. */
        /* See AddLineCube(). */
        if (pathlist[0].path[eltix].remove)
            continue;
        
        xequal = yequal = FALSE;
        if (addHints && (pathlist[hintsdirIx].path[eltix].hints != NULL))
            WriteHints(eltix);
        switch (pathlist[0].path[eltix].type)
        {
            case RMT:
                if (firstMT && !cubeLibrary) /* translate by sidebearing value */
                /* don't want this for cube */
                    for (ix = 0; ix < dirCount; ix++)
                        pathlist[ix].path[eltix].rx -= IntToFix(pathlist[ix].sb);
                firstMT = FALSE;
            case RDT:
            case CP:
                break;
            case RCT:
                if (CheckFlexOK(eltix))
                {
                    WriteFlex(eltix);
                    /* Since we know the next element is a flexed curve and
                     has been written out we skip it. */
                    eltix++;
                    continue;
                }
                /* Try to use optimized operators. */
                if ((pathlist[0].path[eltix].rx1 == 0 && pathlist[0].path[eltix].ry3 == 0)
                    || (pathlist[0].path[eltix].ry1 == 0 && pathlist[0].path[eltix].rx3 == 0))
                    OptimizeCT(eltix);
                break;
            default:
                sprintf(globmsg, "Unknown operator in character: %s.\n", currentChar);
                LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
        }
        op = pathlist[0].path[eltix].type;
        if (op != RCT && op != HVCT && op != VHCT && op != CP)
        /* Try to use optimized operators. */
            OptimizeMtorDt(eltix, &op, &xequal, &yequal);
        startIx = 0;
        opcount = GetOperandCount(op);
        GetLengthandSubrIx(opcount, &length, &subrIx);
        if (xequal && yequal)
        {
            WritePathElt(0, eltix, op, 0, opcount);
        }
        else if (xequal)
        {
            WriteX(pathlist[0].path[eltix].rx);
            if (op != HMT && op != HDT)
            {
                for (ix = 0; ix < dirCount; ix++)
                    WriteY(((ix == 0) ?pathlist[ix].path[eltix].ry :
                            pathlist[ix].path[eltix].ry - pathlist[0].path[eltix].ry));
                GetLengthandSubrIx(1, &length, &subrIx);
                WriteSubr(subrIx);
            }
        }
        else if (yequal)
        {
            if (op != VMT && op != VDT)
            {
                for (ix = 0; ix < dirCount; ix++)
                    WriteX(((ix ==0) ? pathlist[ix].path[eltix].rx :
                            pathlist[ix].path[eltix].rx - pathlist[0].path[eltix].rx));
                GetLengthandSubrIx(1, &length, &subrIx);
                WriteSubr(subrIx);
            }
            WriteY(pathlist[0].path[eltix].ry);
        }
        else
            for (opix = 0; opix < opcount; opix += length)
            {
                WritePathElt(0, eltix, op, opix, length);
                startIx = opix;
                if (op == RCT || op == HVCT || op == VHCT)
                    if (SamePathValues(eltix, op, startIx, length))
                        continue;
                for (ix = 0; ix < length; ix++)
                {
                    for (dirIx = 1; dirIx < dirCount; dirIx++)
                        WritePathElt(dirIx, eltix, op, startIx, 1);
                    startIx++;
                }
                if (subrIx >= 0 && op != CP)
                    WriteSubr(subrIx);
            } /* end of for opix */
        GetOperator(op, operator);
        WriteStr(operator);
    } /* end of for eltix */
    WriteStr("ed\n");
#endif /* AC_C_LIB */  
}

/* Returns number of operands for the given operator. */
extern short GetOperandCount(short op)
{
    short count;
    
    if (op < ESCVAL)
        switch(op)
    {
        case CP:
        case HDT:
        case HMT:
        case VDT:
        case VMT:
            count = 1;
            break;
        case RMT:
        case RDT:
        case RB:
        case RY:
        case SBX:
            count = 2;
            break;
        case HVCT:
        case VHCT:
            count = 4;
            break;
        case RCT:
            count = 6;
            break;
        default:
            sprintf(globmsg, "Unknown operator in character: %s.\n", currentChar);
            LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
            break;
    }
    else				/* handle escape operators */
        switch (op - ESCVAL)
    {
        case RM:
        case RV:
            count = 2;
            break;
    }
    return count;
}

/* Returns the subr number to use for a given operator in subrIx and
 checks that the argument length of each subr call does not
 exceed the font interpreter stack limit. */
extern void GetLengthandSubrIx(short opcount, short *length, short *subrIx)
{
    
    if (((opcount * dirCount) > FONTSTKLIMIT) && opcount != 1)
        if ((opcount/2 * dirCount) > FONTSTKLIMIT)
            if ((2 * dirCount) > FONTSTKLIMIT)
                *length = 1;
            else *length = 2;
            else *length = opcount/2;
            else *length = opcount;
    if (((*length) * dirCount) > FONTSTKLIMIT) {
        sprintf(globmsg, "Font stack limit exceeded for character: %s.\n", currentChar);
        LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
    }
    if (!cubeLibrary) {
        switch (*length) {
            case 1: *subrIx = 7; break;
            case 2: *subrIx = 8; break;
            case 3: *subrIx = 9; break;
            case 4: *subrIx = 10; break;
            case 6: *subrIx = 11; break;
            default:
                sprintf(globmsg, "Illegal operand length for character: %s.\n", currentChar);
                LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
                break;
        }
    } else { /* CUBE */
        switch (dirCount) {
            case 2:
                switch (*length) {
                    case 1: *subrIx = 7; break;
                    case 2: *subrIx = 8; break;
                    case 3: *subrIx = 9; break;
                    case 4: *subrIx = 10; break;
                    case 6: *subrIx = 11; break;
                    default:
                        sprintf(globmsg, "Illegal operand length for character: %s.\n", currentChar);
                        LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
                        break;
                }
                break;
                
            case 4:
                switch (*length) {
                    case 1: *subrIx = 12; break;
                    case 2: *subrIx = 13; break;
                    case 3: *subrIx = 14; break;
                    case 4: *subrIx = 15; break;
                    default:
                        sprintf(globmsg, "Illegal operand length for character: %s.\n", currentChar);
                        LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
                        break;
                }
                break;
                
            case 8:
                switch (*length) {
                    case 1: *subrIx = 16; break;
                    case 2: *subrIx = 17; break;
                    default:
                        sprintf(globmsg, "Illegal operand length for character: %s.\n", currentChar);
                        LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
                        break;
                }
                break;
                
            case 16:
                switch (*length) {
                    case 1: *subrIx = 18; break;
                    default:
                        sprintf(globmsg, "Illegal operand length for character: %s.\n", currentChar);
                        LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
                        break;
                }
                break;
                
            default:
                LogMsg("Illegal dirCount.\n", LOGERROR, NONFATALERROR, TRUE);
                break;
        }
    }
}

/**********
 Normal MM fonts have their dimensionality wired into the subrs.
 That is, the contents of subr 7-11 are computed on a per-font basis.
 Cube fonts can be of 1-4 dimensions on a per-character basis.
 But there are only a few possible combinations of these numbers
 because we are limited by the stack size:
 
 dimensions  arguments      values    subr#
 1            1            2        7
 1            2            4        8
 1            3            6        9
 1            4            8       10
 1            6           12       11
 2            1            4       12
 2            2            8       13
 2            3           12       14
 2            4           16       15
 3            1            8       16
 3            2           16       17
 4            1           16       18
 
 *************/

extern void SetHintsDir(dirIx)
indx dirIx;
{
    hintsdirIx = dirIx;
}

extern int GetHintsDir(void)
{
    return hintsdirIx;
}
/*
 extern boolean MergeCharPaths(outbuffer, charname, filename,
 fortransit, tr, trgroupnum)
 char **outbuffer, *charname, *filename;
 boolean fortransit;
 Transitions *tr;
 int trgroupnum;
 {
 boolean ok;
 int i, ix;
 
 byteCount = 1;  
 buffSize = GetMaxBytes();
 outbuff = outbuffer;
 startbuff = *outbuffer;
 memset(startbuff, 0, buffSize);
 currentChar = charname;
 
 #if DEBUG_PCP
 fprintf(OUTPUTBUFF, "start character %s\n", charname);
 #endif 
 
 if (!fortransit) {
 dirCount = GetTotalInputDirs();
 for (i = 0; i < dirCount; i++) {
 GetMasterDirName(Path_Names[i].dirnam, i);
 sprintf(Path_Names[i].fullfilepath, "%s/%s/%s", 
 Path_Names[i].dirnam, BEZDIR, filename);
 }
 }
 else {
 i = GetNumAxes();
 switch (i) {
 case 1:       dirCount = 2;       break;
 case 2:       dirCount = 4;       break;
 case 3:       dirCount = 8;       break;
 case 4:       dirCount = 16;      break;
 default: dirCount = 0; break;
 }
 for (i = 0; i < dirCount; i++) {
 strcpy(Path_Names[i].fullfilepath, tr->transitgrouparray[trgroupnum].assembledfilename[i]);
 ix = strindex(Path_Names[i].fullfilepath, "/");
 strncpy(Path_Names[i].dirnam, Path_Names[i].fullfilepath, ix);
 Path_Names[i].dirnam[ix] = '\0';
 }
 }
 
 if (ok = CompareCharPaths(filename, fortransit))
 {
 CheckForZeroLengthCP();
 SetSbandWidth(charname, fortransit, tr, trgroupnum);
 if (addHints && hintsdirIx >= 0 && path_entries > 0)
 {
 if (ReadandAssignHints()) {
 sprintf(globmsg, "Path problem in ReadAndAssignHints, character %s.\n", charname);
 LogMsg(globmsg, LOGERROR, FATALERROR, TRUE);
 }
 CheckHandVStem3();
 }
 CombinePaths();
 }
 FreePathElements(0, dirCount);
 return ok;
 }
 
 
 */