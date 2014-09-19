/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/
/* Converts Adobe illustrator files to bezier font editor format files */
/* history:  */
/* JLee - Mar 19, 1987 include debugging flag */
/* JLee - May 29, 1987 include scale factor */
/* JLee - June 3, 1987 include -d option */
/* JLee - October 19, 1987 extract width info from file */
/* JLee - November 5, 1987 fix bug in initialize_widths */

#include "buildfont.h"
#include "bftoac.h"
#include "cryptdefs.h"
#include "fipublic.h"
#include "machinedep.h"


#define	 MAXPOINTS   1500
#
#define	 WIDTH(s)	strindex(s,"%%Note:width") != -1
#define	 ENDPROLOG(s)	strindex(s,"%%EndProlog") != -1
#define	 BEGINSETUP(s)	strindex(s,"%%BeginSetup") != -1
#define	 ENDSETUP(s)	strindex(s,"%%EndSetup") != -1
#define	 TRAILER(s)	strindex(s,"%%Trailer") != -1
#define	 NEWSTROKECOLOR 'G'
#define	 NEWFILLCOLOR	'g'
#define	 MOVETO		'm'
#define	 SMOOTHCURVETO	'c'
#define	 CORNERCURVETO	'C'
#define	 SMOOTHLINETO	'l'
#define	 CORNERLINETO	'L'
#define	 SMOOTHCOPY	'v'
#define	 CORNERCOPY	'V'
#define	 SMOOTHROLL	'y'
#define	 CORNERROLL	'Y'
#define	 FILL		'F'
#define	 FILLANDSTROKE	'B'
#define	 CLOSEANDFILL	'f'
#define	 CLOSEANDSTROKE 's'
#define	 CLOSEANDFILLANDSTROKE	'b'
#define	 STROKE		 'S'
#define	 CLOSEANDNOPAINT 'n'
#define	 NOPAINT	 'N'
#define	 MTINDEX     0
#define	 LTINDEX     1
#define	 CTINDEX     2
#define	 BEZLINESIZE 64

typedef struct path_element
{
    Cd coord;
    short tag;
} path_element;

typedef struct char_width
{
    char name[MAXCHARNAME];
    short width;
} char_width;

/* Globals */
static boolean firstpath;	/* 1st path in this character */
static boolean width_found;	/* width comment in ill. file */
static boolean err;		/* whether error occurred during
                         conversion */
static boolean printmsg;	/* whether to print Converting msg */
static boolean release;		/* indicates release version */
static indx np;			/* number of points in path */
static indx widthcnt;		/* number of character widths */
static long dict_entries;
static float scale;		/* scale factor */
static char_width *widthtab = NULL;
static path_element *path = NULL, *final_path = NULL;
static char tmpnm[sizeof(TEMPFILE) + sizeof(bezdir) + 3];
static short convertedchars;

extern void ConvertInputDirFiles(const char *dirname, tConvertfunc convertproc);

static void illcleanup(
                       void
                       );

static boolean initialize_widths(
                                 void
                                 );

static void process_width(
                          FILE *, const char *, boolean
                          );

static void write_widths_file(
                              boolean
                              );

static boolean width_error(
                           const char *
                           );

static  void toomanypoints(
                           const char *
                           );

static  void convert(
                     FILE *, FILE *, const char *, const char *
                     );

static void do_path(
                    boolean, FILE *, const char *
                    );

static long findarea(
                     CdPtr, CdPtr, CdPtr
                     );

static boolean directionIsCW(
                             void
                             );

static void relative(
                     FILE *
                     );

#if ILLDEBUG
static dump_path(
                 boolean
                 );
#endif


static  void illcleanup()
{
#if DOMEMCHECK
    memck_free(widthtab);
    memck_free(path);
    memck_free(final_path);
#else
    UnallocateMem(widthtab);
    UnallocateMem(path);
    UnallocateMem(final_path);
#endif
}

static boolean initialize_widths()
{
    FILE *wfile;
    boolean widths_exist = FALSE;
    long temp;
    indx ix;
    long cnt, maxChars = MAXCHARS;
    char line[MAXLINE + 1];	/* input buffer */
    
#if DOMEMCHECK
	widthtab = (struct char_width *) memck_malloc(maxChars * sizeof(char_width));
#else
    widthtab = (struct char_width *) AllocateMem(maxChars, sizeof(char_width),
                                                 "width table");
#endif
    for (ix = 0; ix < maxChars; ix++)
        widthtab[ix].width = UNINITWIDTH;
    widthcnt = dict_entries = 0;
    if ((widths_exist = CFileExists(WIDTHSFILENAME, FALSE)))
    {
        wfile = ACOpenFile(WIDTHSFILENAME, "r", OPENERROR);
        while (fgets(line, MAXLINE, wfile) != NULL)
        {
            if ((COMMENT(line)) || (BLANK(line)))
                continue;
            if ((ix = strindex(line, "/")) == -1)
                continue;		/* no character name on this line */
            if (widthcnt >= maxChars)
            {
                maxChars += 100;
#if DOMEMCHECK
                widthtab = (struct char_width *) memck_realloc(widthtab, (maxChars * sizeof(struct char_width)));
#else
                widthtab = (struct char_width *) ReallocateMem(
                                                               (char *)widthtab, (unsigned)(maxChars * sizeof(struct char_width)), "width table");
#endif
                for (ix = maxChars - 100; ix < maxChars; ix++)
                    widthtab[ix].width = UNINITWIDTH;
            }
            cnt = sscanf(&line[ix + 1], " %s %ld WDef", widthtab[widthcnt].name, &temp);
            if (cnt != 2)
            {
                fclose(wfile);
                sprintf(globmsg, "%s file line: %s cannot be parsed.\n  It should have the format: /<char name> <width> WDef",
                        WIDTHSFILENAME, line);
                LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
            }
            CharNameLenOK(widthtab[widthcnt].name);
            widthtab[widthcnt].width = (short) temp;
            widthcnt++;
        }
        fclose(wfile);
    }
    return (widths_exist);
}

static void process_width(FILE *infile, const char *name, boolean seen_width)
{
    indx n, ix;
    long width, cnt;
    float real_width, junk;
    char line[MAXLINE + 1];	/* input buffer */
    
    boolean char_exists = FALSE, done = FALSE;
    
    /* read width information */
    fgets(line, MAXLINE, infile);
    cnt = sscanf(line, " %f %f", &real_width, &junk);
    if (cnt != 2)
    {
        fclose(infile);
        sprintf(globmsg, "Width specified for %s character cannot be parsed.\n", name);
        LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
    }
    /* skip past next "end of path" operator */
    while ((!done) && (fgets(line, MAXLINE, infile) != NULL))
    {
        n = (indx)strlen(line);
        switch (line[n - 2])	/* the illustrator operator */
        {
            case CLOSEANDNOPAINT:
            case NOPAINT:
                done = TRUE;
                break;
            case CLOSEANDSTROKE:
            case STROKE:
            case FILL:
            case FILLANDSTROKE:
            case CLOSEANDFILL:
            case CLOSEANDFILLANDSTROKE:
                sprintf(globmsg, "Width filled or stroked in %s character.\n", name);
                LogMsg(globmsg, WARNING, OK, TRUE);
                done = TRUE;
                break;
        }
    }
    width = SCALEDRTOL(real_width, scale);
    width_found = TRUE;
    /* Check if there is already an entry for this char. */
    for (ix = 0; ix < widthcnt; ix++)
    {
        if (!strcmp(name, widthtab[ix].name))
        {
            if (widthtab[ix].width != width)
            {
                if (seen_width)
                {
                    sprintf(globmsg, "2 different widths specified in %s character.\n", name);
                    LogMsg(globmsg, WARNING, OK, TRUE);
                }
                if ((widthtab[ix].width != UNINITWIDTH) || (seen_width))
                {
                    sprintf(globmsg, "Width updated for %s character (old: %d, new: %ld).\n", name, (int) widthtab[ix].width, width);
                    LogMsg(globmsg, INFO, OK, FALSE);
                }
                widthtab[ix].width = (short) width;
            }
            char_exists = TRUE;
            break;
        }
    }
    if (!char_exists)
    {
        strcpy(widthtab[widthcnt].name, name);
        widthtab[widthcnt].width = (short) width;
        widthcnt++;
    }
}

static void write_widths_file(boolean widths_exist)
{
    FILE *wfile;
    indx i;
    
    if (widthcnt == 0)
    {
        sprintf(globmsg, "No %s file written, because no widths were found.\n",
                WIDTHSFILENAME);
        LogMsg(globmsg, INFO, OK, FALSE);
        return;
    }
    /* if widths.ps file already exists rename it to widths.ps.BAK */
    if (widths_exist)
    {
        char backname[MAXFILENAME];
        
        sprintf(backname, "%s.BAK", WIDTHSFILENAME);
        RenameFile(WIDTHSFILENAME, backname);
    }
    wfile = ACOpenFile(TEMPFILE, "w", OPENERROR);
    fprintf(wfile, "%d dict dup begin\n", (int) NUMMAX(widthcnt, dict_entries));
    for (i = 0; i < widthcnt; i++)
    {
        fprintf(wfile, "/%s %d WDef\n", widthtab[i].name,
                (int) widthtab[i].width);
    }
    fprintf(wfile, "end\n");
    fclose(wfile);
    RenameFile(TEMPFILE, WIDTHSFILENAME);
}

/* This procedure is called if a width comment is not found
 while converting a font illustrator file */
static boolean width_error(const char *name)
{
    indx ix;
    
    /* Check if width is already in width data structure */
    for (ix = 0; ix < widthcnt; ix++)
    {
        if (!strcmp(name, widthtab[ix].name))
        {
            sprintf(globmsg, "No width in %s character description, using\nwidth of %d from %s file.\n", widthtab[ix].name, (int) widthtab[ix].width, WIDTHSFILENAME);
            LogMsg(globmsg, INFO, OK, FALSE);
            return (FALSE);
        }
    }
    if (release)
    {
        sprintf(globmsg, "No width was specified for the %s character.\n", name);
        LogMsg(globmsg, LOGERROR, OK, TRUE);
        return (TRUE);
    }
    else
    {
        sprintf(globmsg,
                "No width was specified for the %s character.\n  The default width of %d will be used.\n",
                name, DEFAULTWIDTH);
        LogMsg(globmsg, WARNING, OK, TRUE);
        strcpy(widthtab[widthcnt].name, name);
        widthtab[widthcnt].width = DEFAULTWIDTH;
        widthcnt++;
        return(FALSE);
    }
}

static  void toomanypoints(const char *name)
{
    sprintf(globmsg, "%s character description has too many points (maximum is %d).\n", name, (int) MAXPOINTS);
    LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
}

static  void convert(FILE *infile, FILE *outfile, const char *charname, const char *filename)
{
    float color = 0.0;
    float temp1, temp2, temp3, temp4, temp5, temp6;
    indx n;
    long cnt;
    boolean white, seen_width = FALSE, seen_trailer = FALSE;
    char op, line[MAXLINE + 1];	/* input buffer */
    
    /* Skip prolog and script setup sections */
    while (fgets(line, MAXLINE, infile) != NULL)
    {
        if (ENDPROLOG(line))
            break;
    }
    while (fgets(line, MAXLINE, infile) != NULL)
    {
        if (BLANK(line))
            continue;
        else
            break;
    }
    if (BEGINSETUP(line))
    /* assume you will have an EndSetup and won't reach end of file */
        while (fgets(line, MAXLINE, infile) != NULL)
        {
            if (ENDSETUP(line))
                break;			/* EndSetup will be treated as comment below */
        }
    np = 0;
    do
    {
        if (WIDTH(line))
        {
            process_width(infile, charname, seen_width);
            seen_width = TRUE;
            continue;
        }
        if (TRAILER(line))
        {
            if (np != 0)
            {
                sprintf(globmsg, "Incomplete path at end of script section of %s character.\n", filename);
                LogMsg(globmsg, WARNING, OK, TRUE);
            }
            seen_trailer = TRUE;
            break;
        }
        if (COMMENT(line))
            continue;
        n = (indx)strlen(line);
        if (n < 2)
        {
            sprintf(globmsg, "%s character description contains the invalid line:\n  %s.\n", filename, line);
            LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
        }
        switch (op = line[n - 2])	/* the illustrator operator */
        {
            case CLOSEANDNOPAINT:
            case NOPAINT:
            case CLOSEANDSTROKE:
            case STROKE:
            case FILLANDSTROKE:
            case CLOSEANDFILLANDSTROKE:
                sprintf(globmsg, "%d point path with paint operator %c ignored in %s character.\n", np, op, filename);
                LogMsg(globmsg, WARNING, OK, TRUE);
                np = 0;
                continue;
                /* NOTREACHED */
                break;
            case CLOSEANDFILL:
                if (np > 2)
                {
                    if ((path[0].coord.x != path[np - 1].coord.x) ||
                        (path[0].coord.y != path[np - 1].coord.y))
                    {
                        sprintf(globmsg, "%d point path in %s character is not closed.  It will be ignored.\n", np, filename);
                        LogMsg(globmsg, WARNING, OK, TRUE);
                        np = 0;
                        continue;
                    }
                }
            case FILL:
                if (np > 2)
                {
                    white = (color > 0.5);
                    do_path(white, outfile, filename);
                }
                else
                {
                    sprintf(globmsg, "%d point filled path ignored in %s character.\n",
                            np, filename);
                    LogMsg(globmsg, WARNING, OK, TRUE);
                    np = 0;
                    continue;
                }
                np = 0;
                break;
            case NEWFILLCOLOR:
                cnt = sscanf(line, " %f", &color);
                if (cnt != 1)
                {
                    sprintf(globmsg, "Color specified for %s character cannot be parsed.\n", filename);
                    LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
                }
                break;
            case MOVETO:
                if (np != 0)
                {
                    sprintf(globmsg, "%d point path not ended before moveto in %s character.\n  %s", np, filename, line);
                    LogMsg(globmsg, WARNING, OK, TRUE);
                }
                cnt = sscanf(line, " %f %f", &temp1, &temp2);
                if (cnt != 2)
                {
                    sprintf(globmsg, "Moveto for %s character cannot be parsed.\n  %s", filename, line);
                    LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
                }
                path[np].coord.x = SCALEDRTOL(temp1, scale);
                path[np].coord.y = SCALEDRTOL(temp2, scale);
                path[np].tag = MTINDEX;
                np = 1;
                break;
            case SMOOTHLINETO:
            case CORNERLINETO:
                cnt = sscanf(line, " %f %f", &temp1, &temp2);
                if (cnt != 2)
                {
                    sprintf(globmsg, "Lineto for %s character cannot be parsed.\n  %s", filename, line);
                    LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
                }
                if (np >= MAXPOINTS)
                    toomanypoints(filename);
                path[np].coord.x = SCALEDRTOL(temp1, scale);
                path[np].coord.y = SCALEDRTOL(temp2, scale);
                path[np].tag = LTINDEX;
                np++;
                break;
            case SMOOTHCURVETO:
            case CORNERCURVETO:
                cnt = sscanf(line, " %f %f %f %f %f %f",
                             &temp1, &temp2, &temp3, &temp4, &temp5, &temp6);
                if (cnt != 6)
                {
                    sprintf(globmsg, "Curveto for %s character cannot be parsed.\n  %s", filename, line);
                    LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
                }
                if (np > (MAXPOINTS - 3))
                    toomanypoints(filename);
                path[np].coord.x = SCALEDRTOL(temp1, scale);
                path[np].coord.y = SCALEDRTOL(temp2, scale);
                path[np + 1].coord.x = SCALEDRTOL(temp3, scale);
                path[np + 1].coord.y = SCALEDRTOL(temp4, scale);
                path[np + 2].coord.x = SCALEDRTOL(temp5, scale);
                path[np + 2].coord.y = SCALEDRTOL(temp6, scale);
                path[np].tag = path[np + 1].tag = path[np + 2].tag = CTINDEX;
                np += 3;
                break;
            case SMOOTHCOPY:		/* x0 y0 x1 y1 2 copy curveto */
            case CORNERCOPY:
                cnt = sscanf(line, " %f %f %f %f", &temp1, &temp2, &temp3, &temp4);
                if (cnt != 4)
                {
                    sprintf(globmsg, "Curveto for%s  character cannot be parsed.\n  %s", filename, line);
                    LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
                }
                if (np > (MAXPOINTS - 3))
                    toomanypoints(filename);
                path[np].coord = path[np - 1].coord;
                path[np + 1].coord.x = SCALEDRTOL(temp1, scale);
                path[np + 1].coord.y = SCALEDRTOL(temp2, scale);
                path[np + 2].coord.x = SCALEDRTOL(temp3, scale);
                path[np + 2].coord.y = SCALEDRTOL(temp4, scale);
                path[np].tag = path[np + 1].tag = path[np + 2].tag = CTINDEX;
                np += 3;
                break;
            case SMOOTHROLL:
            case CORNERROLL:
                cnt = sscanf(line, " %f %f %f %f", &temp1, &temp2, &temp3, &temp4);
                if (cnt != 4)
                {
                    sprintf(globmsg, "Curveto for %s character cannot be parsed.\n  %s", filename, line);
                    LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
                }
                if (np > (MAXPOINTS - 3))
                    toomanypoints(filename);
                path[np].coord.x = SCALEDRTOL(temp1, scale);
                path[np].coord.y = SCALEDRTOL(temp2, scale);
                path[np + 2].coord.x = path[np + 1].coord.x = SCALEDRTOL(temp3, scale);
                path[np + 2].coord.y = path[np + 1].coord.y = SCALEDRTOL(temp4, scale);
                path[np].tag = path[np + 1].tag = path[np + 2].tag = CTINDEX;
                np += 3;
                break;
        }				/* switch */
    } while (fgets(line, MAXLINE, infile) != NULL);
    if (!seen_trailer)
    {
        sprintf(globmsg, "No \"%%TRAILER\" line in file for %s character\n - file probably truncated.\n", filename);
        LogMsg(globmsg, WARNING, OK, TRUE);
    }
}				/* end convert */

static void do_path(boolean white, FILE *outfile, const char *name)
{
    indx n, elem;
    path_element *p, *fp;
    long tag;
    boolean cw, reverse;
    boolean open = ((path[0].coord.x != path[np - 1].coord.x) ||
                    (path[0].coord.y != path[np - 1].coord.y));
    
    cw = directionIsCW();
#if ILLDEBUG
    fprintf(OUTPUTBUFF, "%s\n", (white ? "WHITE" : "BLACK"));
#endif
    reverse = ((white && !cw) || (!white && cw));
    if (reverse)	/* reverse path
                     direction */
    {
        n = np;
        p = &path[np - 1];
        
        /* Start at end with a moveto and adjust tags going backwards. */
        tag = p->tag;
        p->tag = MTINDEX;
        p--;
        n--;
#if ILLDEBUG
        fprintf(OUTPUTBUFF, "Reversing %d point path in character %s.\n", np, name);
#endif
        
        while (n > 0)
        {
            switch (tag)
            {
                case CTINDEX:
                    p -= 2;
                    tag = p->tag;
                    p->tag = CTINDEX;
                    p--;
                    n -= 3;
                    break;
                    
                case LTINDEX:
                    tag = p->tag;
                    p->tag = LTINDEX;
                    p--;
                    n--;
                    break;
                    
                case MTINDEX:
                    fprintf(OUTPUTBUFF, "Moveto on reverse in character %s.\n", name);
                    cleanup(NONFATALERROR);
                    break;
            }
        }
        
#if ILLDEBUG
        dump_path(reverse);
        fprintf(OUTPUTBUFF, "Path before restarting, np=%d\n", np);
#endif
        
        /* If last point is a Lineto, replace it with a closepath. If there are
         no Lineto's, add a closepath at the end. Otherwise, find some Lineto,
         replace it with a closepath and rearrange the path so that the
         closepath is at the end. */
        elem = np - 1;
        if (!open)
        {
            
            if (path[0].tag != LTINDEX)
            {
                p = &path[np - 1];
                for (n = np - 1; n >= 0; n--, p--)
                {
                    if (p->tag == LTINDEX)
                    {
                        elem = n - 1;
                        np--;
                        break;
                    }
                }
            }
            else np--;
        }
        
        p = &path[elem];
        fp = &final_path[0];
        fp->tag = MTINDEX;
        
        if (p->tag == MTINDEX)
        {
            fp->coord = p->coord;
            fp++;
            p--;
        }
        else
        {
            fp->coord = path[elem + 1].coord;
            fp++;
        }
        
        for (n = 1; n < np; n++)
        {				/* copies from path [elem] or path[elem-1] to
                         path[0] */
            *fp = *p;
            fp++;
            p--;
            if (--elem < 0)
            {				/* resets p to copy from path[np-1] to
                             path[elem] */
                p = &path[np - 1];
                elem = np - 1;
            }
        }
    }
    else				/* dump as is -- do not reverse direction */
    {
#if ILLDEBUG
        fprintf(OUTPUTBUFF, "No reverse\n");
        dump_path(reverse);
#endif
        /* If last point is a Lineto, replace it with a closepath. If there are
         no Lineto's, add a closepath at the end.  Otherwise, find some Lineto,
         replace it with a closepath and rearrange the path so that the
         closepath is at the end. */
        elem = 0;
        if (!open)
        {
            
            if (path[np - 1].tag != LTINDEX)
            {
                p = &path[0];
                for (n = 0; n < np; n++, p++)
                {
                    if (p->tag == LTINDEX)
                    {
                        elem = n + 1;
                        np--;
                        break;
                    }
                }
            }
            else {
                np--;
            }
        }
        p = &path[elem];
        fp = &final_path[0];
        fp->tag = MTINDEX;
        if (p->tag == MTINDEX)
        {
            fp->coord = p->coord;
            p++;
        }
        else
        {
            fp->coord = path[elem - 1].coord;
        }
        fp++;
        for (n = 1; n < np; n++)
        {				/* copies from path[elem] or path[elem+1] to
                         path[np] */
            *fp = *p;
            fp++;
            p++;
            if (++elem >= np)
            {				/* resets to copy from path[0] to path[elem] */
                p = &path[0];
                elem = 0;
            }
        }
    }
    relative(outfile);
}				/* end do_path */

static long findarea(CdPtr p1, CdPtr p2, CdPtr sp)
{
    long v1x, v1y, v2x, v2y;
    
    v1x = p1->x - sp->x;
    v1y = p1->y - sp->y;
    v2x = p2->x - sp->x;
    v2y = p2->y - sp->y;
    /* Cross product = area of parallelogram defined by vectors v1 & v2. */
    return ((v1x * v2y) - (v2x * v1y));
}

static boolean directionIsCW()
{
    path_element *p = path;
    long area, n;
    Cd c1, c2, old_point, sp;
    
    area = 0;
    n = np;
    /* Initialize the stationary point about which the area is computed */
    sp = p->coord;
    /* Traverse list of points, accumulating total area. */
    while (n > 0)
    {
        switch (p->tag)
        {
            case MTINDEX:
                n--;
                break;
            case LTINDEX:
                area += findarea(&old_point, &p->coord, &sp);
                n--;
                break;
            case CTINDEX:
                c1 = p->coord;
                p++;
                c2 = p->coord;
                p++;
                area = area + findarea(&old_point, &c1, &sp) + findarea(&c1, &c2, &sp)
                + findarea(&c2, &p->coord, &sp);
                n -= 3;
                break;
        }
        old_point = p->coord;
        p++;
    }
    return (area < 0.0);
}				/* direction */

/* Convert to relative format */

static void relative(FILE *outfile)
{
    path_element *p, *p1, *p2;
    long i, dx, dy;
#if ILLDEBUG
    static char op[3] = {'M', 'L', 'C'};
#endif
    static Cd cp;			/* current position */
    static char buff[500];
    
    p = &final_path[0];
    i = np;
#if ILLDEBUG
    fprintf(OUTPUTBUFF, "Path before relativising,np=%d\n", np);
    while (i > 0)
    {
        if (p->tag == CTINDEX)
        {
            fprintf(OUTPUTBUFF, "%ld %ld ", p->coord.x, p->coord.y);
            p++;
            fprintf(OUTPUTBUFF, "%ld %ld ", p->coord.x, p->coord.y);
            p++;
            fprintf(OUTPUTBUFF, "%ld %ld %c\n", p->coord.x, p->coord.y, op[p->tag]);
            p++;
            i -= 3;
        }
        else
        {
            fprintf(OUTPUTBUFF, "%ld %ld %c\n", p->coord.x, p->coord.y, op[p->tag]);
            p++;
            i--;
        }
    }
#endif
    p = &final_path[0];
    i = np;
    while (i > 0)
    {
        switch (p->tag)
        {
            case MTINDEX:
                if (firstpath)
                {
                    sprintf(buff, "%ld %ld rmt\n", p->coord.x, p->coord.y);
                    (void) DoContEncrypt(buff, outfile, FALSE, INLEN);
                    firstpath = FALSE;
                }
                else
                {
                    dy = p->coord.y - cp.y;
                    if ((dx = p->coord.x - cp.x) == 0)
                    {
                        sprintf(buff, "%ld vmt\n", dy);
                        (void) DoContEncrypt(buff, outfile, FALSE, INLEN);
                    }
                    else if (dy == 0)
                    {
                        sprintf(buff, "%ld hmt\n", dx);
                        (void) DoContEncrypt(buff, outfile, FALSE, INLEN);
                    }
                    else
                    {
                        sprintf(buff, "%ld %ld rmt\n", dx, dy);
                        (void) DoContEncrypt(buff, outfile, FALSE, INLEN);
                    }
                }
                cp = p->coord;
                p++;
                i--;
                break;
            case LTINDEX:
                dy = p->coord.y - cp.y;
                if ((dx = p->coord.x - cp.x) == 0)
                {
                    sprintf(buff, "%ld vdt\n", dy);
                    (void) DoContEncrypt(buff, outfile, FALSE, INLEN);
                }
                else if (dy == 0)
                {
                    sprintf(buff, "%ld hdt\n", dx);
                    (void) DoContEncrypt(buff, outfile, FALSE, INLEN);
                }
                else
                {
                    sprintf(buff, "%ld %ld rdt\n", dx, dy);
                    (void) DoContEncrypt(buff, outfile, FALSE, INLEN);
                }
                cp = p->coord;
                p++;
                i--;
                break;
            case CTINDEX:
                p1 = p + 1;
                p2 = p + 2;
                
                if (((dx = (p->coord.x - cp.x)) == 0) && (p2->coord.y == p1->coord.y))	/* vhct */
                {
                    sprintf(buff, "%ld %ld %ld %ld vhct\n",
                            p->coord.y - cp.y, p1->coord.x - p->coord.x,
                            p1->coord.y - p->coord.y, p2->coord.x - p1->coord.x);
                    (void) DoContEncrypt(buff, outfile, FALSE, INLEN);
                }
                else if (((dy = (p->coord.y - cp.y)) == 0) && (p2->coord.x == p1->coord.x))	/* hvct */
                {
                    sprintf(buff, "%ld %ld %ld %ld hvct\n",
                            dx, p1->coord.x - p->coord.x,
                            p1->coord.y - p->coord.y, p2->coord.y - p1->coord.y);
                    (void) DoContEncrypt(buff, outfile, FALSE, INLEN);
                }
                else			/* rct */
                {
                    sprintf(buff, "%ld %ld ", dx, dy);
                    (void) DoContEncrypt(buff, outfile, FALSE, INLEN);
                    sprintf(buff, "%ld %ld ", p1->coord.x - p->coord.x, p1->coord.y - p->coord.y);
                    (void) DoContEncrypt(buff, outfile, FALSE, INLEN);
                    sprintf(buff, "%ld %ld rct\n", p2->coord.x - p1->coord.x, p2->coord.y - p1->coord.y);
                    (void) DoContEncrypt(buff, outfile, FALSE, INLEN);
                }
                cp = p2->coord;
                p += 3;
                i -= 3;
                break;
        }
    }
    (void) DoContEncrypt("cp\n", outfile, FALSE, INLEN);	/* closepath */
}				/* end relative */

#if ILLDEBUG
static dump_path(boolean reverse)
{
    static char op[3] = {'M', 'L', 'C'};
    long n;
    path_element *p;
    
    n = np;
    if (reverse)
    {
        /* Dump path after reversal. */
        fprintf(OUTPUTBUFF, "Dump of reversed path\n");
        p = &path[np - 1];
        while (n > 0)
        {
            if (p->tag == CTINDEX)
            {
                fprintf(OUTPUTBUFF, "%-10.3d %-10.3d", p->coord.x, p->coord.y);
                p--;
                fprintf(OUTPUTBUFF, "%-10.3d %-10.3d", p->coord.x, p->coord.y);
                p--;
                fprintf(OUTPUTBUFF, "%-10.3d %-10.3d %c\n", p->coord.x, p->coord.y, op[p->tag]);
                p--;
                n -= 3;
            }
            else
            {
                fprintf(OUTPUTBUFF, "%-10.3d %-10.3d %c\n", p->coord.x, p->coord.y, op[p->tag]);
                p--;
                n--;
            }
        }
    }
    else
    {
        fprintf(OUTPUTBUFF, "Dump of unreversed path\n");
        p = path;
        while (n > 0)
        {
            if (p->tag == CTINDEX)
            {
                fprintf(OUTPUTBUFF, "%-10.3d %-10.3d", p->coord.x, p->coord.y);
                p++;
                fprintf(OUTPUTBUFF, "%-10.3d %-10.3d", p->coord.x, p->coord.y);
                p++;
                fprintf(OUTPUTBUFF, "%-10.3d %-10.3d %c\n", p->coord.x, p->coord.y, op[p->tag]);
                p++;
                n -= 3;
            }
            else
            {
                fprintf(OUTPUTBUFF, "%-10.3d %-10.3d %c\n", p->coord.x, p->coord.y, op[p->tag]);
                p++;
                n--;
            }
        }
    }
}

#endif

void convert_illcharfile(const char *charname, const char *filename)
{
    FILE *infile, *outfile;
    char inname[MAXPATHLEN], outname[MAXPATHLEN];
    
    firstpath = TRUE;
    width_found = FALSE;
    get_filename(inname, ILLDIR, filename);
    get_filename(outname, bezdir, filename);
    
    infile = ACOpenFile(inname, "r", OPENWARN);
    if (infile == NULL) return;
    outfile = ACOpenFile(tmpnm, "w", OPENERROR);
    DoInitEncrypt(outfile, OTHER, HEX, (long) BEZLINESIZE, FALSE);
    WriteStart(outfile, charname);
    convert(infile, outfile, charname, filename);
    (void) DoContEncrypt("ed\n", outfile, FALSE, INLEN);
    if (!width_found)
        err = err || width_error(charname);
    fclose(infile);
    fclose(outfile);
    RenameFile(tmpnm, outname);
    if (printmsg)
    {
        LogMsg ("Converting Adobe Illustrator(R) files ...",
                INFO,OK,FALSE);
        printmsg = FALSE;
    }
    convertedchars++;
}

extern void set_scale(float *newscale)
{
    float temp;
    long cnt;
    char *fontinfostr;
    
    *newscale = 1;
    fontinfostr = GetFntInfo("ScalePercent", ACOPTIONAL);
    if (fontinfostr == NULL)
        return;
    cnt = sscanf(fontinfostr, " %f", &temp);
    if (cnt != 1)
    {
        sprintf(globmsg, "ScalePercent line of fontinfo file invalid.\n");
#if DOMEMCHECK
		memck_free(fontinfostr);
#else
		UnallocateMem(fontinfostr);
#endif
        LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
    }
#if DOMEMCHECK
    memck_free(fontinfostr);
#else
    UnallocateMem(fontinfostr);
#endif
    *newscale = temp / (float) 100;
}


void convert_illfiles(boolean rel)
{
    boolean widths_exist;
    boolean result = TRUE;
    
    release = rel;
    printmsg = TRUE;
    illcleanup();	/* just in case last caller exited with error */
    widths_exist = initialize_widths();
    set_scale(&scale);
    get_filename(tmpnm, bezdir, TEMPFILE);
    
#if DOMEMCHECK
    path = (struct path_element *) memck_malloc(MAXPOINTS * sizeof(path_element));
    final_path = (struct path_element *) memck_malloc(MAXPOINTS * sizeof(path_element));
#else
    path = (struct path_element *) AllocateMem(MAXPOINTS, sizeof(path_element),
                                               "path structure");
    final_path = (struct path_element *) AllocateMem(MAXPOINTS, sizeof(path_element), "final path structure");
#endif
    convertedchars = 0;
#if SUN
	ConvertCharFiles(ILLDIR, release, scale, convert_illcharfile);
#elif WIN32
	ConvertCharFiles(ILLDIR, release, scale, convert_illcharfile);
#else
	ConvertInputDirFiles(ILLDIR, convert_illcharfile);
#endif
    if (convertedchars > 0)
    {
        sprintf (globmsg, " %d files converted.\n", (int) convertedchars);
        LogMsg (globmsg, INFO,OK,FALSE);
        if (scale != 1.0)
        {
            sprintf(globmsg, "Widths in the original %s file were not scaled.\n", WIDTHSFILENAME);
            LogMsg(globmsg, WARNING, OK, TRUE);
        }
    }
    if (err || !result)
        cleanup(NONFATALERROR);
    write_widths_file(widths_exist);
    illcleanup();
}
