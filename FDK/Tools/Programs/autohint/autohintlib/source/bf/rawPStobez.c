/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/
/* rawPStobez.c - converts character description files in raw
   PostScript format to relativized bez format.	 The only
   operators allowed are moveto, lineto, curveto and closepath.
   This program assumes that the rawPS files are located in a
   directory named chars and stores the converted and encrypted
   files with the same name in a directory named bez.

history:

Judy Lee: Tue Jun 14 17:33:13 1988
End Edit History

*/
#include "buildfont.h"
#include "bftoac.h"
#include "cryptdefs.h"
#include "machinedep.h"


#define	 MOVETO		((!strcmp(token, "moveto")))
#define	 LINETO		((!strcmp(token, "lineto")))
#define	 CURVETO	((!strcmp(token, "curveto")))
#define	 CLOSEPATH	((!strcmp(token, "closepath")))
#define	 END		((!strcmp(token, "ed")))
#define	 START		((!strcmp(token, "sc")))

#define SSIZE 20		/* operand stack size */

#if !IS_LIB
static long oprnd_stk[SSIZE];
static short count;
static short convertedchars;
static boolean printmsg;
static long dx, dy;
static char tempname[MAXFILENAME];
static Cd p1, p2, p3, currPt;
static float scale; /* scale factor for each character. */
#endif
static char outstr[MAXLINE];	/* contains string to be encrypted */

static void errmsg(
    char *, char *, FILE *, FILE *
);

static short nextline(
    FILE *
);

static char *ReadrawPSFile(
    FILE *, char *, char *
);

static void SetCd(
    Cd, CdPtr
);

static void putonstk(
    long, char *
);

static void mt(
    void
);

static void dt(
    void
);

static void ct(
    void
);

#ifdef OLDFUNCTIONS


static errmsg(str, name, infile, outfile)
char *str, *name;
FILE *infile, *outfile;
{
  fclose(infile);
  fclose(outfile);
  sprintf(globmsg, "%s in %s file does not have the correct number of operands.\n This file was not converted.\n", str, name);
  LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
}

static short nextline(infile)
FILE *infile;
{
  register short c;

  while (TRUE)
  {
    c = getc(infile);
    if (c == NL || c == '\r' || c == EOF)
      return (c);
  }
}

static char *ReadrawPSFile(stream, tokenPtr, filename)
FILE *stream;
char *tokenPtr, *filename;
{
  register short c;

  /* Skip all white space */
  while (TRUE)
  {
    c = getc(stream);
    if (c == '%')
      c = nextline(stream);
    if (c == EOF)
      return (NULL);
    if (!isspace(c))
    {
      *tokenPtr = c;
      tokenPtr++;
      break;
    }
  }
  while (TRUE)
  {
    c = getc(stream);
    if (c == '%')
      c = nextline(stream);
    if (c == EOF)
    {
      sprintf(globmsg, "Unexpected end of file found in %s directory \n  character description: %s.\n", RAWPSDIR, filename);
      LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
    }
    if (isspace(c))
      break;
    *tokenPtr = c;
    tokenPtr++;
  }
  *tokenPtr = '\0';
  return tokenPtr;
}


static SetCd(a, b)
Cd a;
CdPtr b;
{
  b->x = a.x;
  b->y = a.y;
}

static putonstk(number, charname)
long number;
char * charname;
{
  if (count >= SSIZE)
  {
    sprintf (globmsg, 
      "Maximum stack size exceeded when converting %s file %s.\n", 
      RAWPSDIR, charname);
    LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
  }
  oprnd_stk[count] = number;
  count = count + 1;
}

static mt()
{
  count = count - 1;
  p1.y = oprnd_stk[count--];
  p1.x = oprnd_stk[count];
  dx = SCALEDRTOL((p1.x - currPt.x), scale);
  dy = SCALEDRTOL((p1.y - currPt.y), scale);
  if (dx == 0)
    sprintf(outstr, "%ld %c%c%c\n", dy, 'v', 'm', 't');
  else if (dy == 0)
    sprintf(outstr, "%ld %c%c%c\n", dx, 'h', 'm', 't');
  else
    sprintf(outstr, "%ld %ld %c%c%c\n", dx, dy, 'r', 'm', 't');
  SetCd(p1, &currPt);
}

static dt()
{
  count = count - 1;
  p1.y = oprnd_stk[count--];
  p1.x = oprnd_stk[count];
  dx = SCALEDRTOL((p1.x - currPt.x), scale);
  dy = SCALEDRTOL((p1.y - currPt.y), scale);
  if (dx == 0)
    sprintf(outstr, "%ld %c%c%c\n", dy, 'v', 'd', 't');
  else if (dy == 0)
    sprintf(outstr, "%ld %c%c%c\n", dx, 'h', 'd', 't');
  else
    sprintf(outstr, "%ld %ld %c%c%c\n", dx, dy, 'r', 'd', 't');
  SetCd(p1, &currPt);
}

static ct()
{
  count = count - 1;
  p3.y = oprnd_stk[count--];
  p3.x = oprnd_stk[count--];
  p2.y = oprnd_stk[count--];
  p2.x = oprnd_stk[count--];
  p1.y = oprnd_stk[count--];
  p1.x = oprnd_stk[count];
  if ((currPt.x == p1.x) && (p3.y == p2.y))	/* vhct */
    sprintf(outstr, "%ld %ld %ld %ld %c%c%c%c\n",
      SCALEDRTOL((p1.y - currPt.y), scale), SCALEDRTOL((p2.x - p1.x), scale),
      SCALEDRTOL((p2.y - p1.y), scale), SCALEDRTOL((p3.x - p2.x), scale),
      'v', 'h', 'c', 't');
  else if ((currPt.y == p1.y) && (p3.x == p2.x))	/* hvct */
    sprintf(outstr, "%ld %ld %ld %ld %c%c%c%c\n",
      SCALEDRTOL((p1.x - currPt.x), scale), SCALEDRTOL((p2.x - p1.x), scale),
      SCALEDRTOL((p2.y - p1.y), scale), SCALEDRTOL((p3.y - p2.y), scale),
      'h', 'v', 'c', 't');
  else				/* rct */
    sprintf(outstr, "%ld %ld %ld %ld %ld %ld %c%c%c\n",
      SCALEDRTOL((p1.x - currPt.x), scale), SCALEDRTOL((p1.y - currPt.y), scale),
      SCALEDRTOL((p2.x - p1.x), scale), SCALEDRTOL((p2.y - p1.y), scale),
      SCALEDRTOL((p3.x - p2.x), scale), SCALEDRTOL((p3.y - p2.y), scale),
      'r', 'c', 't');
  SetCd(p3, &currPt);
}

extern void convert_PScharfile(const char *charname, const char *filename)
{
  FILE *infile, *outfile;
  char token[50];
  long number;
  char inname[MAXPATHLEN], outname[MAXPATHLEN];

  get_filename(inname, RAWPSDIR, filename);
  get_filename(outname, bezdir, filename);
  infile = ACOpenFile(inname, "r", OPENWARN);
  if (infile == NULL)
    return;
  outfile = ACOpenFile(tempname, "w", OPENERROR);
  DoInitEncrypt(outfile, OTHER, HEX, MAXINT32, FALSE);
  WriteStart(outfile, charname);
  count = 0;
  currPt.x = 0;
  currPt.y = 0;
  while (ReadrawPSFile(infile, token, inname) != NULL)
  {
    if (sscanf(token, " %ld", &number) == 1)
    {
      putonstk(number, charname);
      continue;
    }
    else			/* must be a string */
    {
      if (START)
	continue;
      if (END)
	break;
      if (CLOSEPATH)
      {
	(void) DoContEncrypt("cp\n", outfile, FALSE, INLEN);
	continue;
      }
      else if (MOVETO)
	if (count != 2)
	  errmsg("moveto", inname, infile, outfile);
	else
	  mt();
      else if (LINETO)
	if (count != 2)
	  errmsg("lineto", inname, infile, outfile);
	else
	  dt();
      else if (CURVETO)
	if (count != 6)
	  errmsg("curveto", inname, infile, outfile);
	else
	  ct();
      else			/* Unrecognized operator */
      {
	fclose(infile);
	fclose(outfile);
	unlink(tempname);
	sprintf(globmsg, "Unknown operator: %s in character description: %s.\n", token, inname);
	LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
      }
      count = 0;
      (void) DoContEncrypt(outstr, outfile, FALSE, INLEN);
    }
  }
  (void) DoContEncrypt("ed\n", outfile, FALSE, INLEN);
  fclose(infile);
  fclose(outfile);
  RenameFile(tempname, outname);
  if (printmsg)
  {
    LogMsg("Converting PostScript(R) language files ...",
      INFO, OK, FALSE);
    printmsg = FALSE;
  }
  convertedchars++;
}

extern convert_rawPSfiles(release)
boolean release;
{
  boolean result = TRUE;

  /* initialize globals in this source file */
  count = convertedchars = 0;
  printmsg = TRUE;
  set_scale(&scale);

  get_filename(tempname, bezdir, TEMPFILE);
#if SUN
  result = ConvertCharFiles(
    RAWPSDIR, release, scale, convert_PScharfile);
#elif WIN32
  result = ConvertCharFiles(
    RAWPSDIR, release, scale, convert_PScharfile);
#else
  ConvertInputDirFiles(RAWPSDIR, convert_PScharfile);
#endif
  if (convertedchars > 0)
  {
    sprintf(globmsg, " %d files converted.\n", (int) convertedchars);
    LogMsg(globmsg, INFO, OK, FALSE);
    if (scale != 1.0)
    {
      sprintf(globmsg, "Widths in the original %s file were not scaled.\n", WIDTHSFILENAME);
      LogMsg(globmsg, WARNING, OK, TRUE);
    }
  }
  if (!result) cleanup(NONFATALERROR);
}

#endif


int WriteStart(FILE *outfile, const char *name)
{
  sprintf(outstr, "%%%s\nsc\n", name);
  (void) DoContEncrypt(outstr, outfile, FALSE, INLEN);
  return 0;
}
