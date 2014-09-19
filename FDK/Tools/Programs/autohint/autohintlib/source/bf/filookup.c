/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/* filookup.c - looks up keywords in the fontinfo file and returns the value. */

#include "buildfont.h"
#include "filookup.h"
#include "fipriv.h"
#include "fipublic.h"
#include "machinedep.h"
#include "masterfont.h"
#ifdef ACLIB_EXPORTS
#include "ac.h"
#endif
#if SUN
#include      <math.h>
#endif
#define     EOS      ""
#define     NOMATCH     -1
#define MAXFONTNAME 30          /* max length of the FontName = 36 chars  PS
                                   name length max due to LW ROM bug - 7 
                                   chars for Mac coordinating + 1 for null
                                   terminator */

#define DEFAULTOVRSHTPTSIZE 9.51  /* Default overshoot point size. */

static const char *keywordtab[KWTABLESIZE];
static boolean tab_initialized = FALSE;
static FINODE finode;
static FIPTR fiptr = &finode;
static char endNotice[] = "EndNotice";
static char endDirs[] = "EndDirs";
static char endInstance[] = "EndInstances";
static char endVectors[] = "EndVectors";
static char fifilename [MAXPATHLEN];


static short lookup(
    char *
);

static char *search_for_key(
    char *, FILE *
);

static void make_error_return(
    short, char *, char *, char *, char *
);

static  void make_normal_return(
    char *, boolean
);

static  void append_normal_return(
    char *
);

static void check_for_int(
    char *, char *
);

static  void check_for_intormatrix(
    char *, char *
);

static  void check_for_num(
    char *, char *
);

static void check_for_string(
    char *, char *
);

static  void check_for_boolean(
    char *, char *
);

static void check_for_matrix(
    char *, char *, boolean
);

static void check_PS_comment(
    char *
);

static  void checkovershootvalue(
  char *, boolean, boolean
);

static boolean checkovershoots(
  void
);

static char *get_middle_lines(
    char *, char *, boolean
);

static char *get_keyword_value(
    char *, char *
);

static void init_kw_tab(
    void
);

static char *get_base_font_path(
    void
);

static char *GetHVStems(
  char *, boolean
);

static void CheckStemSnap(
  char *
);

#ifdef SUN
static boolean valueOK = TRUE;
#endif

void fiptrfree(FIPTR ptr)
{
#if DOMEMCHECK
	memck_free(ptr->value_string);
#else
	UnallocateMem(ptr->value_string);
#endif
}

/*  Look up the keyword parameter in the table to get the index */
static short lookup(key)
char *key;
{
  indx kwindex;

  for (kwindex = 0; kwindex < KWTABLESIZE; kwindex++)
    if (STREQ(key, keywordtab[kwindex]))
      return (kwindex);
  return (NOMATCH);
}

/* This proc reads through the given file searching for a line
   with its first word the same as key.    If it finds such a
   line it returns it. */
static char *search_for_key(key, file)
char *key;
FILE *file;
{
  char *line;
  char linekey[LINELEN + 1];
#if DOMEMCHECK
  line = memck_malloc((unsigned) LINELEN * sizeof(char));
#else
  line = AllocateMem((unsigned) LINELEN, sizeof(char), "font info buffer");
#endif
  while (fgets(line, LINELEN, file) != NULL)
  {
    if (sscanf(line, " %s", linekey) < 1)
      continue;
    if (STREQ(key, linekey))
      return (line);
  }
#if DOMEMCHECK
  memck_free(line);
#else
  UnallocateMem(line);
#endif
  return (NULL);
}

/*  make_error_return concatenates the error_msgs
    strings together into the string return value and
    assigns the exit status in the return value structure, as well */
static void make_error_return(short exit_status, char *error_msg1, char *error_msg2, char *error_msg3, char *error_msg4)
{
  short msg1_len = (short)strlen(error_msg1);
  short msg2_len = (short)strlen(error_msg2);
  short msg3_len = (short)strlen(error_msg3);
  short msg4_len = (short)strlen(error_msg4);

#if DOMEMCHECK
  memck_free(fiptr->value_string);
#else
  UnallocateMem(fiptr->value_string);
#endif
  fiptr->value_string = AllocateMem((unsigned) (msg1_len + msg2_len + msg3_len
      + msg4_len + 1), sizeof(char), "fontinfo return value");
  sprintf(fiptr->value_string, "%s%s%s%s",
    error_msg1, error_msg2, error_msg3, error_msg4);
  fiptr->exit_status = exit_status;
}

/* make_normal_return copies the string argument into the 
 * the return value string and assigns the normal return 
 * exit status. The caller should free the arg.*/
static  void make_normal_return(char *arg, boolean arg_is_string)
{
#if DOMEMCHECK
	fiptr->value_string = memck_malloc((unsigned) (strlen(arg) + 1) * sizeof(char));
#else
  fiptr->value_string = AllocateMem((unsigned) (strlen(arg) + 1), sizeof(char),
    "fontinfo return");
#endif
  strcpy(fiptr->value_string, arg);
  fiptr->exit_status = NORMAL_RETURN;
  fiptr->value_is_string = arg_is_string;
}

/*
 * append_normal_return appends the string argument at the end of 
 * the return value string and assigns the normal return 
 * exit status. The caller should free the arg, if necessary. */
static  void append_normal_return(arg)
char *arg;
{
  char *temp;

  if (fiptr->value_string == NULL)
  {
#if DOMEMCHECK
	fiptr->value_string = memck_malloc((unsigned) (strlen(arg) + 1) * sizeof(char));
#else
    fiptr->value_string = AllocateMem((unsigned) (strlen(arg) + 1),
      sizeof(char), "fontinfo return");
#endif
    strcpy(fiptr->value_string, arg);
  }
  else
  {
    temp = fiptr->value_string;
#if DOMEMCHECK
	fiptr->value_string = memck_malloc((strlen(temp) + strlen(arg) + 1) * sizeof(char));
#else
    fiptr->value_string = AllocateMem((unsigned)
      (strlen(temp) + strlen(arg) + 1), sizeof(char),
      "fontinfo return");
#endif
    strcat(fiptr->value_string, temp);
    strcat(fiptr->value_string, arg);
#if DOMEMCHECK
	memck_free(temp);
#else
    UnallocateMem(temp);
#endif
  }
  fiptr->exit_status = NORMAL_RETURN;
  fiptr->value_is_string = FALSE;
}

/* If lineargs is an int, put the int to the value string.
   Otherwise, put an error message to the value string. */
static void check_for_int(key, lineargs)
char *key, *lineargs;
{
  indx i, j;
  float f;

  i = sscanf(lineargs, " %f", &f);
  if (i == 1)
  {
    j = (short) f;
    if ((float) j == f)
    {
      sprintf(lineargs, "%d", j);
      make_normal_return(lineargs, FALSE);
      return;
    }
  }
  sprintf(globmsg, "integer expected on same line of %s file as keyword ", fifilename);
  make_error_return(ERROR_RETURN, globmsg, key, "\n    instead of ", lineargs);
}

/* Checks whether lineargs is an int or matrix then calls the
   appropriate procedure. */
static  void check_for_intormatrix(key, lineargs)
char *key, *lineargs;
{
  char *lb = (char *)strchr(lineargs, '[');

  if (lb == 0) /* must be an integer */
    check_for_int(key, lineargs);
  else
    check_for_matrix(key, lineargs, FALSE);
}

/* If lineargs is a decimal number, put the decimal number to
   the value string.  Otherwise, put an error message to the
   value string. */
static  void check_for_num(key, lineargs)
char *key, *lineargs;
{
  short i;
  float f;

  i = sscanf(lineargs, " %g", &f);
  if (i != 1)
  {
    sprintf(globmsg,
      "floating point number expected on same line of %s file as ", fifilename);
    make_error_return(ERROR_RETURN, globmsg, key, "\n  instead of ", lineargs);
  }
  else
  {
    sprintf(lineargs, "%g", f);
    make_normal_return(lineargs, FALSE);
  }
}

/* If lineargs is a string in PostScript format (i.e., enclosed in  
 *  parentheses), put the string to the value string. 
 *  Otherwise, put an error message into the value string. */
static void check_for_string(key, lineargs)
char *key, *lineargs;
{
  char *lp, *rp;

  lp = (char *)strchr(lineargs, '(');
  rp = (char *)strrchr(lineargs, ')');
  if ((lp == 0) || (rp == 0) || (lp > rp)
     || (strspn(rp+1, " \n\r\t\f\v") != strlen(rp+1)))
  {
    sprintf(globmsg,
      "parenthesized string expected on same line of %s file as ", fifilename);
    make_error_return(ERROR_RETURN, globmsg, key, "\n  instead of ", lineargs);
    return;
  }
  rp[0] = '\0';         /* trash character after ) to put in string
               terminator */
  make_normal_return(lp + 1, TRUE);
}

int misspace(int c)
{
	if (c==' ' || c=='\n' || c=='\r' || c=='\t')
		return 1;
	return 0;
}

int misdigit(int c)
{
	return c>='0' && c<='9';
}

/* If lineargs is a boolean, put the boolean to the value string.  
 *  Otherwise, put an error message to the value string. */
static  void check_for_boolean(key, lineargs)
char *key, *lineargs;
{
  indx i;

  for (i = 0; (misspace(lineargs[i])); i++)
  {
  }
  if (!strncmp(&lineargs[i], "false", 5))
    make_normal_return("false", FALSE);
  else if (!strncmp(&lineargs[i], "true", 4))
    make_normal_return("true", FALSE);
  else
  {
    sprintf(globmsg,
      "boolean value expected on the same line of %s file as ", fifilename);
    make_error_return(ERROR_RETURN, globmsg, key, "\n  instead of ", lineargs);
  }
}

/*
   If lineargs is a PostScript format matrix (enclosed in square
   brackets) put the matrix to the value string.  Otherwise, put
   an error message to the value string. */
static void check_for_matrix (char *key, char *lineargs, boolean append)
{
  char *lb, *rb;
  lb = (char *)strchr (lineargs, '[');
  rb = (char *)strrchr (lineargs, ']');
  if ((lb == 0) || (rb == 0) || (lb > rb))
  {
    sprintf(globmsg, "matrix enclosed by square brackets expected on line of\n  %s file with ", fifilename);
    make_error_return (ERROR_RETURN, globmsg, key, "\n  instead of ", lineargs);
    return;
  }
  rb [1] = '\0';   /* trash character after ] to put in string terminator */
  if (!append)
    make_normal_return (lb, FALSE);
  else
    append_normal_return(lb);
}

/*
 *  If line is a PostScript comment, put it in the value string 
 *  Otherwise, put an error message in the value string. */
static void check_PS_comment(line)
char *line;
{
  if (BLANK(line))
    return;
  if (!COMMENT(line))
    make_error_return(ERROR_RETURN, fifilename,
      " file should contain line in PostScript comment form\n  instead of ",
      line, EOS);
  else
    append_normal_return(line);
}

/* This proc reads all of the lines in the fifile, looking for the
 * beginkey line.  Once that is found, it starts appending 
 * the lines which follow up to the line that
 * matches the param, endkey.  If matrix is true then it
 * checks that the line is a matrix before appending otherwise
 * it checks that the line is a comment. */
static char *get_middle_lines(char *beginkey, char *endkey, boolean matrix)
{
  FILE *fifile;
  char *line, linekey[LINELEN], linebuf[LINELEN + 1];
  char fname[MAXPATHLEN];
  char *bfp;

  fifile = ACOpenFile(fifilename, "r", OPENOK);
  if (fifile == NULL)
    return (NULL);

  line = search_for_key(beginkey, fifile);
  if (line == NULL) 
  {
    bfp = get_base_font_path ();
    /* extract line with matching keyword, if any, from the base font 
     * directory's file */
    if (bfp == NULL)
      return (NULL);
    sprintf(fname, "%s%s", bfp, fifilename); 
    fifile = ACOpenFile(fname, "r", OPENOK);
    if (fifile == NULL) 
      return (NULL);
    line = search_for_key (beginkey, fifile);
  }
  while (fgets(linebuf, LINELEN, fifile) != NULL)
  {
    if (sscanf(linebuf, " %s", linekey) < 1)
      continue;
    if (STRNEQ(endkey, linekey))
      if (!matrix)
        check_PS_comment(linebuf);
      else
        check_for_matrix(beginkey, linebuf, TRUE);
    else
    {
      fclose(fifile);
      return (line);
    }
  }
  fclose(fifile);
#if DOMEMCHECK
  memck_free(line);
#else
  UnallocateMem(line);
#endif
  return (NULL);
}

/* Gets a line that has the keyword and returns the remainder of
   the line. */
static char *get_keyword_value(key, dirpath)
char *key;
char *dirpath;
{
  FILE *fifile;
  char *line, *lineargs;
  char fname[MAXPATHLEN];

  sprintf(fname, "%s%s", dirpath, fifilename); 
  fifile = ACOpenFile(fname, "r", OPENERROR);
  line = search_for_key(key, fifile);
  if (line == NULL)
  {
    fclose(fifile);
    return (NULL);
  }
#if DOMEMCHECK
    lineargs = memck_malloc((unsigned) LINELEN * sizeof(char));
#else
  lineargs = AllocateMem((unsigned) LINELEN, sizeof(char), "fontinfo buffer");
#endif
  strcpy(lineargs, line + strindex(line, key) + strlen(key));
  fclose(fifile);
#if DOMEMCHECK
  memck_free(line);
#else
  UnallocateMem(line);
#endif
  return (lineargs);
}

static void init_kw_tab()
{
  if (tab_initialized)
      return;
  tab_initialized = TRUE;
  /* These are the keywords actually used on the Mac */
  keywordtab[ADOBECOPYRIGHT] = "AdobeCopyright";
  keywordtab[BASEFONTPATH] = "BaseFontPath";
  keywordtab[CHARACTERSET] = "CharacterSet";
  keywordtab[CHARACTERSETFILETYPE] = "CharacterSetFileType";
  keywordtab[CHARACTERSETLAYOUT] = "CharacterSetLayout";
  keywordtab[COMPOSITES] = "Composites";
  keywordtab[COPYRIGHTNOTICE] = "CopyrightNotice";
  keywordtab[CUBELIBRARY] = "CubeLibrary";
  keywordtab[DOMINANTH] = "DominantH";
  keywordtab[DOMINANTV] = "DominantV";
  keywordtab[ENCODING] = "Encoding";
  keywordtab[FAMILYBLUESPATH] = "FamilyBluesPath";
  keywordtab[FAMILYNAME] = "FamilyName";
  keywordtab[FONTMATRIX] = "FontMatrix";
  keywordtab[FONTNAME] = "FontName";
  keywordtab[FORCEBOLD] = "ForceBold";
  keywordtab[FULLNAME] = "FullName";
  keywordtab[ISFIXEDPITCH] = "isFixedPitch";
  keywordtab[ITALICANGLE] = "ItalicAngle";
  keywordtab[PCFILENAMEPREFIX] = "PCFileNamePrefix";
  keywordtab[SCALEPERCENT] = "ScalePercent";
  keywordtab[STEMSNAPH] = "StemSnapH";
  keywordtab[STEMSNAPV] = "StemSnapV";
  keywordtab[SYNTHETICBASE] = "SyntheticBase";
  keywordtab[TRADEMARK] = "Trademark";
  keywordtab[UNDERLINEPOSITION] = "UnderlinePosition";
  keywordtab[UNDERLINETHICKNESS] = "UnderlineThickness";
  keywordtab[WEIGHT] = "Weight";
  keywordtab[VERSION] = "version";

  /* These are the keywords used only for blending, coloring or
     blackbox backward compatibility */
  keywordtab[APPLEFONDID] = "AppleFONDID";
  keywordtab[APPLENAME] = "AppleName";
  keywordtab[ASCENT] = "Ascent";
  keywordtab[ASCENTDESCENTPATH] = "AscentDescentPath";
  keywordtab[ASCHEIGHT] = "AscenderHeight";
  keywordtab[ASCOVERSHOOT] = "AscenderOvershoot";
  keywordtab[AUXHSTEMS] = "AuxHStems";
  keywordtab[AUXVSTEMS] = "AuxVStems";
  keywordtab[AXISLABELS1] = "AxisLabels1";
  keywordtab[AXISLABELS2] = "AxisLabels2";
  keywordtab[AXISLABELS3] = "AxisLabels3";
  keywordtab[AXISLABELS4] = "AxisLabels4";
  keywordtab[AXISMAP1] = "AxisMap1";
  keywordtab[AXISMAP2] = "AxisMap2";
  keywordtab[AXISMAP3] = "AxisMap3";
  keywordtab[AXISMAP4] = "AxisMap4";
  keywordtab[AXISTYPE1] = "AxisType1";
  keywordtab[AXISTYPE2] = "AxisType2";
  keywordtab[AXISTYPE3] = "AxisType3";
  keywordtab[AXISTYPE4] = "AxisType4";
  keywordtab[BASELINEYCOORD] = "BaselineYCoord";
  keywordtab[BASELINEOVERSHOOT] = "BaselineOvershoot";
  keywordtab[BASELINE5] = "Baseline5";
  keywordtab[BASELINE5OVERSHOOT] = "Baseline5Overshoot";
  keywordtab[BASELINE6] = "Baseline6";
  keywordtab[BASELINE6OVERSHOOT] = "Baseline6Overshoot";
  keywordtab[BITSIZES75] = "BitSizes75";
  keywordtab[BITSIZES96] = "BitSizes96";
  keywordtab[CAPHEIGHT] = "CapHeight";
  keywordtab[CAPOVERSHOOT] = "CapOvershoot";
  keywordtab[DESCENT] = "Descent";
  keywordtab[DESCHEIGHT] = "DescenderHeight";
  keywordtab[DESCOVERSHOOT] = "DescenderOvershoot";
  keywordtab[FIGHEIGHT] = "FigHeight";
  keywordtab[FIGOVERSHOOT] = "FigOvershoot";
  keywordtab[FLEXEXISTS] = "FlexExists";
  keywordtab[FLEXOK] = "FlexOK";
  keywordtab[FLEXSTRICT] = "FlexStrict";
  keywordtab[FONTVENDOR] = "FontVendor";
  keywordtab[FORCEBOLDTHRESHOLD] = "ForceBoldThreshold";
  keywordtab[HINTSDIR] = "HintsDir";
  keywordtab[HCOUNTERCHARS] = "HCounterChars";
  keywordtab[HEIGHT5] = "Height5";
  keywordtab[HEIGHT5OVERSHOOT] = "Height5Overshoot";
  keywordtab[HEIGHT6] = "Height6";
  keywordtab[HEIGHT6OVERSHOOT] = "Height6Overshoot";
  keywordtab[PRIMARYINSTANCES] = "PrimaryInstances";
  keywordtab[LCHEIGHT] = "LcHeight";
  keywordtab[LCOVERSHOOT] = "LcOvershoot";
  keywordtab[MASTERDESIGNPOSITIONS] = "MasterDesignPositions";
  keywordtab[MASTERDIRS] = "MasterDirs";
  keywordtab[MSMENUNAME] = "MSMenuName";
  keywordtab[MSWEIGHT] = "MSWeight";
  keywordtab[NCEXISTS] = "NCExists";
  keywordtab[NUMAXES] = "NumAxes";
  keywordtab[ORDINALBASELINE] = "OrdinalBaseline";
  keywordtab[ORDINALOVERSHOOT] = "OrdinalOvershoot";
  keywordtab[ORIGEMSQUAREUNITS] = "OrigEmSqUnits";
  keywordtab[OVERSHOOTPTSIZE] = "OvershootPtsize";
  keywordtab[PAINTTYPE] = "PaintType";
  keywordtab[PGFONTIDNAME] = "PGFontID";
  keywordtab[PHANTOMVECTORS] = "PhantomVectors";
  keywordtab[PI] = "Pi";
  keywordtab[POLYCONDITIONPROC] = "PolyConditionProc";
  keywordtab[POLYCONDITIONTYPE] = "PolyConditionType";
  keywordtab[POLYFLEX] = "PolyFlex";
  keywordtab[POLYFONT] = "PolyFont";
  keywordtab[PRIMOGENITALFONTNAME] = "PrimogenitalFontName";
  keywordtab[SERIF] = "Serif";
  keywordtab[SOURCE] = "Source";
  keywordtab[REGULARINSTANCE] = "RegularInstance";
  keywordtab[RNDSTEMUP] = "RndStemUp";
  keywordtab[STROKEWIDTH] = "StrokeWidth";
  keywordtab[STROKEWIDTHHEAVY] = "StrokeWidthHeavy";
  keywordtab[STYLEBOLD] = "StyleBold";
  keywordtab[STYLECONDENSED] = "StyleCondensed";
  keywordtab[STYLEEXTENDED] = "StyleExtended";
  keywordtab[SUPERIORBASELINE] = "SuperiorBaseline";
  keywordtab[SUPERIOROVERSHOOT] = "SuperiorOvershoot";
  keywordtab[VCOUNTERCHARS] = "VCounterChars";
  keywordtab[VENDORSFONTID] = "VendorsFontID";
  keywordtab[VPMENUNAME] = "VPMenuName";
  keywordtab[VPSTYLE] = "VPStyle";
  keywordtab[VPTYPEFACEID] = "VPTypefaceID";
  keywordtab[BLUEFUZZ]= "BlueFuzz";
  keywordtab[ZONETHRESHOLD]= "ZoneThreshold";
	
  keywordtab[LANGUAGEGROUP] = "LanguageGroup";
}

/* filookup will lookup the keyword in the fontinfo file.
   If the keyword is found and has the proper type, its value
   will be written to the returned value string.  If the
   keyword is optional and not found, a message to that effect
   is returned in the value string.  Otherwise, an error message
   will be written to the returned value string.
*/
extern FIPTR filookup(char *keyword, boolean optional)
{
  short kwindex;
  char *fs;
  char *bfp = NULL;

  init_kw_tab();
  kwindex = lookup(keyword);
  if (kwindex == NOMATCH)
  {
	if ((keyword != NULL) && (keyword[0] != '\0'))
	  {
		make_error_return( ERROR_RETURN,
						  fifilename, " file does not recognize keyword ", keyword, ".");
	  }
	else
	  {
		make_error_return((optional) ? OPTIONAL_NOT_FOUND : ERROR_RETURN,
						  fifilename, " empty keyword. ", EOS, EOS);
	  }
    return (fiptr);
  }
  /* extract line with matching keyword, if any, from the file */
  fs = get_keyword_value(keyword, "");
  if (fs == NULL) {
    bfp = get_base_font_path ();
    /* extract line with matching keyword, if any, from the base font 
     * directory's file */
    if (bfp != NULL)
      fs = get_keyword_value (keyword, bfp);
  }
  if (fs == NULL)
  {
    if (optional)
    {
      fiptr->exit_status = OPTIONAL_NOT_FOUND;
      fiptr->value_string = NULL;
      return (fiptr);
    }
    else
    {
      make_error_return(ERROR_RETURN,
        fifilename, " file does not contain keyword ", keyword, ".");
      return (fiptr);
    }
  }
    
  if (fiptr->value_string != NULL)
  {
#if DOMEMCHECK
	memck_free(fiptr->value_string);
#else
	UnallocateMem(fiptr->value_string);
#endif
  }

  switch (kwindex)
  {
  case APPLENAME:
  case AXISTYPE1:
  case AXISTYPE2:
  case AXISTYPE3:
  case AXISTYPE4:
  case FONTVENDOR:
  case BITSIZES75:
  case BITSIZES96:
  case HINTSDIR:
  case HCOUNTERCHARS:
  case MSMENUNAME:
  case POLYCONDITIONPROC:
  case PRIMOGENITALFONTNAME:
  case SOURCE:
  case VCOUNTERCHARS:
  case VENDORSFONTID:
  case VPMENUNAME:
  case VPSTYLE:
  case ADOBECOPYRIGHT:
  case ASCENTDESCENTPATH:
  case BASEFONTPATH:
  case CHARACTERSET:
  case CHARACTERSETFILETYPE:
  case CHARACTERSETLAYOUT:
  case ENCODING:
  case FAMILYBLUESPATH:
  case FAMILYNAME:
  case FONTNAME:
  case FULLNAME:
  case PCFILENAMEPREFIX:
  case TRADEMARK:
  case VERSION:
  case WEIGHT:
    check_for_string(keyword, fs);
    break;
  case COMPOSITES:
  case CUBELIBRARY:
  case ISFIXEDPITCH:
  case SYNTHETICBASE:
  case FLEXSTRICT:
  case FLEXOK:
  case FLEXEXISTS:
  case FORCEBOLD:
  case NCEXISTS:
  case PI:
  case SERIF:
  case RNDSTEMUP:
    check_for_boolean(keyword, fs);
    break;
   case ASCENT:
   case DESCENT:
  case FORCEBOLDTHRESHOLD:
  case ITALICANGLE:
  case SCALEPERCENT:
  case UNDERLINEPOSITION:
  case UNDERLINETHICKNESS:
  case ORIGEMSQUAREUNITS:
  case OVERSHOOTPTSIZE:
  case STROKEWIDTH:
  case STROKEWIDTHHEAVY:
  case BLUEFUZZ:
  case ZONETHRESHOLD:
    check_for_num(keyword, fs);
    break;
  case MASTERDIRS:
    fs = get_middle_lines(keyword, endDirs, FALSE);
    if (fs == NULL)
      make_error_return(ERROR_RETURN,
        "Could not find keyword ", endDirs, " in file ", fifilename);
    break;
  case COPYRIGHTNOTICE:
    fs = get_middle_lines(keyword, endNotice, FALSE);
    if (fs == NULL)
      make_error_return(ERROR_RETURN,
        "No EndNotice line found in ", fifilename, " file.", EOS);
    break;
  case APPLEFONDID:
  case ASCHEIGHT:
  case ASCOVERSHOOT:
  case BASELINEYCOORD:
  case BASELINEOVERSHOOT:
  case BASELINE5:
  case BASELINE5OVERSHOOT:
  case BASELINE6:
  case BASELINE6OVERSHOOT:
  case CAPHEIGHT:
  case CAPOVERSHOOT:
  case DESCHEIGHT:
  case DESCOVERSHOOT:
  case FIGHEIGHT:
  case FIGOVERSHOOT:
  case HEIGHT5:
  case HEIGHT5OVERSHOOT:
  case HEIGHT6:
  case HEIGHT6OVERSHOOT:
  case LCHEIGHT:
  case LCOVERSHOOT:
  case MSWEIGHT:
  case NUMAXES:
  case ORDINALBASELINE:
  case ORDINALOVERSHOOT:
  case PAINTTYPE:
  case PGFONTIDNAME:
  case POLYCONDITIONTYPE:
  case POLYFONT:
  case SUPERIORBASELINE:
  case SUPERIOROVERSHOOT:
  case VPTYPEFACEID:
  case LANGUAGEGROUP:
    check_for_int(keyword, fs);
    break;
  case AUXHSTEMS:
  case AUXVSTEMS:
  case AXISLABELS1:
  case AXISLABELS2:
  case AXISLABELS3:
  case AXISLABELS4:
  case AXISMAP1:
  case AXISMAP2:
  case AXISMAP3:
  case AXISMAP4:
  case MASTERDESIGNPOSITIONS:
  case POLYFLEX:
  case REGULARINSTANCE:
  case STYLEBOLD:
  case STYLECONDENSED:
  case STYLEEXTENDED:
  case FONTMATRIX:
  case STEMSNAPH:
  case STEMSNAPV:
    check_for_matrix(keyword, fs, FALSE);
    break;
  case DOMINANTH:
  case DOMINANTV:
    check_for_intormatrix(keyword, fs);
    break;
  case PHANTOMVECTORS:
    fs = get_middle_lines(keyword, endVectors, TRUE);
    if (fs == NULL)
      make_error_return(ERROR_RETURN,
        "Could not find keyword ", endVectors, " in file ", fifilename);
    break;
  case PRIMARYINSTANCES:
    fs = get_middle_lines(keyword, endInstance, TRUE);
    if (fs == NULL)
      make_error_return(ERROR_RETURN,
        "Could not find keyword ", endInstance, " in file ", fifilename);
    break;
  default:
    make_error_return(ERROR_RETURN,
      keyword, " is not a known ", fifilename, " file keyword.");
    break;
  }

#if DOMEMCHECK
  memck_free(fs);
#else
  UnallocateMem(fs);
#endif
  return (fiptr);
}

/* Sets the file name from which to get font info to a non-default
   name. */
extern  void SetFntInfoFileName(filename)
char *filename;
{
   strcpy(fifilename, filename);
}

/* Sets the file name from which to get font info to the default
   name. */
extern  void ResetFntInfoFileName(void)
{
   strcpy(fifilename, FIFILENAME);
}

/* Looks up the value of the specified keyword in the fontinfo
   file.  If the keyword doesn't exist and this is an optional
   key, returns a NULL.  Otherwise, returns the value string. */
extern char *GetFntInfo(char *keyword, boolean optional)
{
  FIPTR fptr;
  char *returnstring = NULL;

#ifdef ACLIB_EXPORTS
	if (featurefiledata!=NULL)
	{
		int i;

		for (i=0; i<featurefilesize; i++)
		{
			if(featurefiledata[i].key && !strcmp(featurefiledata[i].key, keyword))
			{
#if DOMEMCHECK
				returnstring = memck_malloc((strlen(featurefiledata[i].value)+1) * sizeof(char));
#else
				returnstring = (char *)AllocateMem((unsigned)strlen(featurefiledata[i].value)+1, sizeof(char), "GetFntInfo return str");
#endif
				strcpy(returnstring, featurefiledata[i].value);
				return returnstring;
			}
		}
	
		if(optional){
			return NULL;
		}else{
			sprintf(globmsg, "ERROR: Fontinfo: Couldn't find fontinfo for %s\n", keyword);
			LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
		}
	}else{
#endif

  fptr = filookup(keyword, optional);
  switch (fptr->exit_status)
  {
  case NORMAL_RETURN:
#if DOMEMCHECK
	returnstring = memck_malloc((strlen(fptr->value_string) + 1) * sizeof(char));
#else
    returnstring = AllocateMem((unsigned)strlen(fptr->value_string) + 1, sizeof(char),
      "return string for fontinfo");
#endif
    strcpy(returnstring, fptr->value_string);
#if DOMEMCHECK
	memck_free(fptr->value_string);
#else
	UnallocateMem(fptr->value_string);
#endif
    break;
  case OPTIONAL_NOT_FOUND:
    returnstring = NULL;
#if DOMEMCHECK
	memck_free(fptr->value_string);
#else
	UnallocateMem(fptr->value_string);
#endif
    break;
  case ERROR_RETURN:
    sprintf(globmsg, "%s\n", fptr->value_string);
#if DOMEMCHECK
	memck_free(fptr->value_string);
#else
	UnallocateMem(fptr->value_string);
#endif
    LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
    break;
  default:
#if DOMEMCHECK
	memck_free(fptr->value_string);
#else
	UnallocateMem(fptr->value_string);
#endif
    LogMsg("Unknown exit status from fontinfo lookup.\n",
      LOGERROR, NONFATALERROR, TRUE);
	break;
  }
  return (returnstring);
#ifdef ACLIB_EXPORTS
	}
#endif
	return NULL;
}

/* This proc looks up an integer fontinfo value and returns:
   1) the integer value found for the keyword
   2) MAXINT if the keyword was not found
   In case of an error, cleanup is called, which never returns
 */

extern int GetFIInt(char *keyword, boolean optional)
{
  FIPTR fptr;
  int temp;

  fptr = filookup(keyword, optional);
  switch (fptr->exit_status)
  {
  case NORMAL_RETURN:
    sscanf(fptr->value_string, "%d", &temp);
#if DOMEMCHECK
	memck_free(fptr->value_string);
#else
	UnallocateMem(fptr->value_string);
#endif
    return (temp);
    break;
  case OPTIONAL_NOT_FOUND:
    if (optional)
    {
#if DOMEMCHECK
	memck_free(fptr->value_string);
#else
	UnallocateMem(fptr->value_string);
#endif
      return (MAXINT);
    }
    /* fall through intentionally */
  case ERROR_RETURN:
    sprintf(globmsg, "%s\n", fptr->value_string);
#if DOMEMCHECK
	memck_free(fptr->value_string);
#else
	UnallocateMem(fptr->value_string);
#endif
    LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
  default:
#if DOMEMCHECK
	memck_free(fptr->value_string);
#else
	UnallocateMem(fptr->value_string);
#endif
    LogMsg("Unknown exit status from fontinfo lookup.\n",
      LOGERROR, NONFATALERROR, TRUE);
  }
  return (MAXINT);
}

/* FreeFontInfo frees the memory associated with the pointer ptr.
   This is to reclaim the storage allocated in GetFntInfo.  */
extern void FreeFontInfo(char *ptr)
{
#if DOMEMCHECK
	memck_free(ptr);
#else
   UnallocateMem(ptr);
#endif
}

/* Appends Aux{H,V}Stems which is optional to StemSnap{H,V} respectively. */
static char *GetHVStems(char *kw, boolean optional)
{
  char *fistr1, *fistr2, *newfistr;
  char *end, *start;
  
  fistr1 = GetFntInfo(( (STREQ(kw, "AuxHStems")) ? "StemSnapH" : "StemSnapV"), optional);
  fistr2 = GetFntInfo(kw, ACOPTIONAL);
  if (fistr2 == NULL) return fistr1;
  if (fistr1 == NULL) return fistr2;
  /* Merge two arrays. */
#if DOMEMCHECK
newfistr = memck_malloc((strlen(fistr1) + strlen(fistr2) + 1) * sizeof(char));
#else
  newfistr = AllocateMem(
    (unsigned) (strlen(fistr1) + strlen(fistr2) + 1), sizeof(char), "Aux stem value");
#endif
  end = (char *)strrchr(fistr1, ']');
  end[0] = '\0';
  start = (char *)strchr(fistr2, '[');
  start[0] = ' ';
  sprintf(newfistr, "%s%s", fistr1, fistr2);
#if DOMEMCHECK
	memck_free(fistr1);
#else
   UnallocateMem(fistr1);
#endif
#if DOMEMCHECK
	memck_free(fistr2);
#else
   UnallocateMem(fistr2);
#endif
  return newfistr;
}

/* This procedure parses the various fontinfo file stem keywords:
   StemSnap{H,V}, Dominant{H,V} and Aux{H,V}Stems.  If Aux{H,V}Stems
   is specified then the StemSnap{H,V} values are automatically
   added to the stem array.  ParseIntStems guarantees that stem values
   are unique and in ascending order.
   If blendstr is NULL it means we just want the values from the
   current font directory and not for all input directories
   (e.g., for a multi-master font).
 */  
extern void ParseIntStems(char *kw, boolean optional, long maxstems, int *stems, long *pnum, char *blendstr)
{
  char c;
  char *line;  
  int val, cnt, i, ix, j, temp, targetCnt = -1, total = 0;
  boolean singleint = FALSE;
  short dirCount = (blendstr == NULL ? 1 : GetTotalInputDirs());
  char *initline;

  *pnum = 0;
  for (ix = 0; ix < dirCount; ix++)
  {
    cnt = 0;
    if (blendstr != NULL)
      SetMasterDir(ix);
    if (STREQ(kw, "AuxHStems") || STREQ(kw, "AuxVStems"))
      initline = GetHVStems(kw, optional);
    else initline = GetFntInfo (kw, optional);
    if (initline == NULL)
    {
    if (targetCnt > 0)
         {
        sprintf(globmsg, "The keyword: %s does not have the same number of values\n  in each master design.\n", kw);
        LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
         }
      else
            continue;  /* optional keyword not found */
    }
    line = initline;
    /* Check for single integer instead of matrix. */
    if ((strlen(line) != 0) && (strchr(line, '[') == 0))
    {
      singleint = TRUE;
      goto numlst;
    }
    while (TRUE) 
    {
      c = *line++;
      switch (c) 
      {
        case 0: *pnum = 0; 
#if DOMEMCHECK
				memck_free(initline);
#else
				UnallocateMem(initline);
#endif 
			return;
        case '[': goto numlst;
        default: break;
      }
    }
    numlst:
    while (*line != ']') 
    {
      while (misspace(*line)) line++; /* skip past any blanks */
      if (sscanf(line, " %d", &val) < 1) break;
      if (total >= maxstems)
      {
        sprintf (globmsg, "Cannot have more than %d values in fontinfo file array: \n  %s\n", (int) maxstems, initline);
      LogMsg (globmsg, LOGERROR, NONFATALERROR, TRUE);
      }
      if (val < 1)
      {
        sprintf (globmsg, "Cannot have a value < 1 in fontinfo file array: \n  %s\n", line);
        LogMsg (globmsg, LOGERROR, NONFATALERROR, TRUE);
      }
      stems[total++] = val;
      cnt++;
      if (singleint) break;
      while (misdigit(*line)) line++; /* skip past the number */
    }
    /* insure they are in order */
    for (i = *pnum; i < total; i++)
      for (j = i+1; j < total; j++)
        if (stems[i] > stems [j])
        {
          temp = stems [i];
          stems [i] = stems [j];
          stems [j] = temp;
        }
    /* insure they are unique - note: complaint for too many might precede
       guarantee of uniqueness */
    for (i = *pnum; i < total-1; i++)
      if (stems[i] == stems [i+1])
      {
        for (j = (i+2); j < total; j++)
          stems [j-1] = stems [j];
        total--;
        cnt--;
      }
    if (ix > 0 && (cnt != targetCnt))
    {
#if DOMEMCHECK
		memck_free(initline);
#else
		UnallocateMem(initline);
#endif
      sprintf(globmsg, "The keyword: %s does not have the same number of values\n  in each master design.\n", kw);
      LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
    }
    targetCnt = cnt;
    *pnum += cnt;
#if DOMEMCHECK
		memck_free(initline);
#else
		UnallocateMem(initline);
#endif
  } /* end of for loop */
  if (blendstr == NULL || total == 0)
    return;
  /* Write array of blended snap values to blendstr to be put in
     Blend dict of Private dictionary. */
  WriteBlendedArray(stems, total, cnt, blendstr);
  /* Reset pnum so just the first set of snap values is written
     in the top level Private dictionary. */
  *pnum = cnt;
}

/* This proc checks for the existence of StemSnap{H,V}.  If it is
   specified in the fontinfo file then it checks that the corresponding
   Dominant{H,V} value (if it exists) is also in the StemSnap array.
   If it is not an error message is issued.  Also checks that if there
   are more than two values in the StemSnap{H,V} arrays then there must
   be two values in the Dominant{H,V} arrays. */
static void CheckStemSnap(direction)
char *direction;
{
  int stems[MAXSTEMSNAP];
  long stemcnt, dominantVal = MAXINT, domValCnt = 0;
  char key[15];
  indx ix;
  
  key[0] = '\0';
  sprintf(key, "Dominant%s", direction);
  ParseIntStems (key, ACOPTIONAL, (long) MAXDOMINANTSTEMS, stems, &stemcnt, NULL);
  if (stemcnt > 0)
  {
    dominantVal = stems[0];
    domValCnt = stemcnt;
  }
  sprintf(key, "StemSnap%s", direction);
  ParseIntStems (key, ACOPTIONAL, (long) MAXSTEMSNAP, stems, &stemcnt, NULL);
  if (stemcnt <= 0)
    return;
  if (dominantVal == MAXINT)
  {
    sprintf(globmsg, "A StemSnap%s array is specified, but a Dominant%s value\n  is not defined.  This may cause stem width problems.\n", direction, direction);
    LogMsg(globmsg, WARNING, OK, TRUE);
    return;
  }
  if ((stemcnt > 2) && (domValCnt != MAXDOMINANTSTEMS))
  {
    sprintf(globmsg, "There must be two values in the Dominant{H,V} arrays if there are\n  more than two values in the StemSnap{H,V} arrays in the %s file.\n", FIFILENAME);
    LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
  }
  for (ix = 0; ix < stemcnt; ix++)
    if (dominantVal == stems[ix])
      return;
  sprintf(globmsg, "The Dominant%s value: %ld must be included in\n  the StemSnap%s array in the %s file.\n", direction, dominantVal, direction, FIFILENAME);
  LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
}



extern void
CheckRequiredKWs (void)
/*****************************************************************************/
/* This proc attempts to test for required fontinfo keywords early in the    */
/* running of buildfont, rather than have the lack of one of them cause      */
/* buildfont to blow up just as it goes to assemble the font.  It can only   */
/* check a subset of the keywords that potentially will be required, since   */
/* whether coloring keywords are required is data dependent. FontName,       */
/* version and CharacterSet entries are tested early enough elsewhere.       */
/*****************************************************************************/
   {
   char *s;
/*   int stems[MAXDOMINANTSTEMS];
   long stemcnt, bands; */
#ifdef SUN
   float overshootptsize = (float)DEFAULTOVRSHTPTSIZE;
#endif
   s = GetFntInfo("FontName", MANDATORY);
   if (strlen (s) >= MAXFONTNAME)
      {
      sprintf(globmsg, "FontName in %s exceeds max length of %d.\n",
      fifilename, (int) (MAXFONTNAME-1));
#if DOMEMCHECK
		memck_free(s);
#else
		UnallocateMem(s);
#endif
      LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
      }
#if DOMEMCHECK
		memck_free(s);
#else
		UnallocateMem(s);
#endif
   s = GetFntInfo("Encoding",
         scalinghints || (GetCharsetParser() == bf_CHARSET_CID));
#if DOMEMCHECK
		memck_free(s);
#else
		UnallocateMem(s);
#endif
   s = GetFntInfo("AdobeCopyright", scalinghints);
#if DOMEMCHECK
		memck_free(s);
#else
		UnallocateMem(s);
#endif
   s = GetFntInfo("FullName", scalinghints);
#if DOMEMCHECK
		memck_free(s);
#else
		UnallocateMem(s);
#endif
   s = GetFntInfo("FamilyName", scalinghints);
#if DOMEMCHECK
		memck_free(s);
#else
		UnallocateMem(s);
#endif
   s = GetFntInfo("Weight", scalinghints);
#if DOMEMCHECK
		memck_free(s);
#else
		UnallocateMem(s);
#endif
   s = GetFntInfo("isFixedPitch", scalinghints);
#if DOMEMCHECK
		memck_free(s);
#else
		UnallocateMem(s);
#endif
   s = GetFntInfo("UnderlinePosition", scalinghints);
#if DOMEMCHECK
		memck_free(s);
#else
		UnallocateMem(s);
#endif
   s = GetFntInfo("UnderlineThickness", scalinghints);
#if DOMEMCHECK
		memck_free(s);
#else
		UnallocateMem(s);
#endif
   s = GetFntInfo("Trademark", ACOPTIONAL);
#if DOMEMCHECK
		memck_free(s);
#else
		UnallocateMem(s);
#endif
   s = GetFntInfo("Composites", ACOPTIONAL);
#if DOMEMCHECK
		memck_free(s);
#else
		UnallocateMem(s);
#endif
   s = GetFntInfo("ScalePercent", ACOPTIONAL);
#if DOMEMCHECK
		memck_free(s);
#else
		UnallocateMem(s);
#endif

#ifdef SUN
   if (scalinghints)
      {
      SetFntInfoFileName (SCALEDHINTSINFO);
      s = GetFntInfo ("OrigEmSqUnits", MANDATORY);
#if DOMEMCHECK
		memck_free(s);
#else
		UnallocateMem(s);
#endif
      }
   s = GetFntInfo("FlexOK", ACOPTIONAL);
#if DOMEMCHECK
		memck_free(s);
#else
		UnallocateMem(s);
#endif
   s = GetFntInfo("RndStemUp", ACOPTIONAL);
#if DOMEMCHECK
		memck_free(s);
#else
		UnallocateMem(s);
#endif
   s = GetFntInfo("HCounterChars", ACOPTIONAL);
#if DOMEMCHECK
		memck_free(s);
#else
		UnallocateMem(s);
#endif
   s = GetFntInfo("VCounterChars", ACOPTIONAL);
#if DOMEMCHECK
		memck_free(s);
#else
		UnallocateMem(s);
#endif
   ParseIntStems("DominantV", MANDATORY, (long) MAXDOMINANTSTEMS, stems,
         &stemcnt, NULL);
   ParseIntStems ("DominantH", ACOPTIONAL, (long) MAXDOMINANTSTEMS, stems,
         &stemcnt, NULL);
   CheckStemSnap ("H");
   CheckStemSnap ("V");

   bands = build_bands ();
   if (scalinghints || (bands == 0))
      {
      ResetFntInfoFileName ();
      return;
      }
   if ((s = GetFntInfo("OvershootPtsize", ACOPTIONAL)) != NULL)
      overshootptsize = atof(s) - 0.49;
   if (checkbandwidths(overshootptsize, TRUE) ||
      checkbandwidths(overshootptsize, FALSE) ||
      checkbandspacing() || !checkovershoots())
   LogMsg("Can't continue because of problem with "
         "alignment zones in fontinfo file.\n", LOGERROR, NONFATALERROR, TRUE);
#endif
   }



#ifdef SUN

static checkovershootvalue(keyword, optional, positive)
char *keyword;
boolean optional, positive;
{
  int value;

  if ((value = GetFIInt(keyword, optional)) != MAXINT)
  {
    if (positive)
    {
      if (value < 0)
      {
        sprintf(globmsg, "The keyword %s has a value of %d.\n  It should be greater than or equal to 0.\n", keyword, value);
        LogMsg(globmsg, LOGERROR, OK, TRUE);
        valueOK = FALSE;
      }
    }
    else
      if (value > 0)
      {
        sprintf(globmsg, "The keyword %s has a value of %d.\n  It should be less than or equal to 0.\n", keyword, value);
        LogMsg(globmsg, LOGERROR, OK, TRUE);
        valueOK = FALSE;
      }
  }
}

/* Returns TRUE if all values have the correct sign i.e. positive or negative. */
static boolean checkovershoots()
{
  valueOK = TRUE;
  checkovershootvalue("BaselineOvershoot", MANDATORY, FALSE);
  checkovershootvalue("CapOvershoot", MANDATORY, TRUE);
  checkovershootvalue("LcOvershoot", ACOPTIONAL, TRUE);
  checkovershootvalue("FigOvershoot", ACOPTIONAL, TRUE);
  checkovershootvalue("SuperiorOvershoot", ACOPTIONAL, FALSE);
  checkovershootvalue("OrdinalOvershoot", ACOPTIONAL, FALSE);
  checkovershootvalue("AscenderOvershoot", ACOPTIONAL, TRUE);
  checkovershootvalue("DescenderOvershoot", ACOPTIONAL, FALSE);
  checkovershootvalue("Baseline5Overshoot", ACOPTIONAL, FALSE);
  checkovershootvalue("Baseline6Overshoot", ACOPTIONAL, FALSE);
  checkovershootvalue("Height5Overshoot", ACOPTIONAL, TRUE);
  checkovershootvalue("Height6Overshoot", ACOPTIONAL, TRUE);
  return valueOK;
}

#endif

/*
 * get_base_font_path reads the BaseFontPath entry out of this 
 * directory's fontinfo, if any, and returns the base font path name, 
 * which is guaranteed to end in a delimiter */
static char *
get_base_font_path ()
{
 char *line = get_keyword_value ("BaseFontPath", EOS);
 char *bfp;
 char *lp, *rp;
 char name[MAXPATHLEN];
 int bfp_len;
 int i;

 if (line == NULL)
   return (NULL);
#if DOMEMCHECK
	bfp = memck_malloc((strlen (line) + 3) *sizeof(char));
#else
 bfp = AllocateMem ((unsigned) (strlen (line) + 3), 
   sizeof(char), "font info buffer");
#endif
 i = sscanf (line, "%s", bfp);
 if (i != 1)
   return (NULL);
 lp = (char *)strchr (bfp, '(');
 rp = (char *)strrchr (bfp, ')');
 if ((lp == 0) || (rp == 0) || (rp < lp))
   return (NULL);
 bfp_len = (int)(rp - lp - 1);
 strncpy (line, lp+1, bfp_len); 
 line [bfp_len] = '\0';
 line = CheckBFPath(line);
 get_filename(name, line, "");  /* Adds appropriate delimiter. */
 strcpy(line, name);
#if DOMEMCHECK
 memck_free(bfp);
#else
 UnallocateMem (bfp);
#endif
 return (line);
} 

