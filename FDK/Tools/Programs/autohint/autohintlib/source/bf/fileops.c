/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/

#include "buildfont.h"
#include "afmcharsetdefs.h"
#include "cryptdefs.h"
#include "fipublic.h"
#include "machinedep.h"
#include "charpath.h"

#define BAKSUFFIX ".BAK"

static char charsetDir[MAXPATHLEN];
static char charsetname[MAXPATHLEN];
static char charsetPath[MAXPATHLEN];
static char subsetPath[MAXPATHLEN];
static char subsetname[MAXPATHLEN];
static char working_dir[MAXPATHLEN];/* name of directory where bf was invoked*/
static short total_inputdirs = 1;   /* number of input directories           */
static long MaxBytes;
char globmsg[MAXMSGLEN + 1];        /* used to format messages               */
static CharsetParser charsetParser;
static char charsetLayout[MAXPATHLEN];
static char initialWorkingDir[MAXPATHLEN];
static int initialWorkingDirLength;


#if IS_LIB
typedef void *(*AC_MEMMANAGEFUNCPTR)(void *ctxptr, void *old, unsigned long size);
extern AC_MEMMANAGEFUNCPTR AC_memmanageFuncPtr;
extern void *AC_memmanageCtxPtr;
#endif

#include "memcheck.h"


typedef struct
   {
   int lo;
   int hi;
   } SubsetData;

extern boolean multiplemaster; /* from buildfont.c */

extern short strindex(s, t)         /* return index of t in s, -1 if none    */
char *s, *t;
{
  indx i, n;

  n = (indx)strlen(t);
  for (i = 0; s[i] != '\0'; i++)
    if (!strncmp(s + i, t, n))
      return i;
  return -1;
}


char *AllocateMem(unsigned int nelem, unsigned int elsize, const char *description)
{
#if IS_LIB
 char *ptr = (char *)AC_memmanageFuncPtr(AC_memmanageCtxPtr, NULL, nelem * elsize);
 if (NULL != ptr)
	 memset((void *)ptr, 0x0, nelem * elsize);
#else
  char *ptr = (char *)calloc(nelem, elsize);
#endif
  if (ptr == NULL)
  {
    sprintf(globmsg, "Cannot allocate %d bytes of memory for %s.\n",
      (int) (nelem * elsize), description);
    LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
  }
  return (ptr);
}

char *ReallocateMem(char *ptr, unsigned int size, const char *description)
{
#if IS_LIB
 char *newptr = (char *)AC_memmanageFuncPtr(AC_memmanageCtxPtr, (void *)ptr, size);
#else
  char *newptr = (char *)realloc(ptr, size);
#endif
  if (newptr == NULL)
  {
    sprintf(globmsg, "Cannot allocate %d bytes of memory for %s.\n",
      (int) size, description);
    LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
  }
  return (newptr);
}

void UnallocateMem(void *ptr)
{
#if IS_LIB
	AC_memmanageFuncPtr(AC_memmanageCtxPtr, (void *)ptr, 0);
#else
	if (ptr != NULL) {free((char *) ptr); ptr = NULL;}
#endif
}

extern unsigned long CheckFileBufferLen(buffer, filename)
char **buffer, *filename;
{
  long filelen;

  filelen = ACGetFileSize (filename);
  if (filelen > MaxBytes) { /* renner Tue Sep 11 14:59:37 1990 */
#if 0
    fprintf(stderr,"Calling realloc: %ld vs %ld\n", filelen, MaxBytes); fflush(stderr);
#endif
    MaxBytes = filelen + 5;
#if DOMEMCHECK
	*buffer = (char *)memck_realloc(*buffer, (MaxBytes * sizeof(char)));
#else
    *buffer = (char *)ReallocateMem(*buffer, (unsigned)(MaxBytes * sizeof(char)), "file buffer");
#endif
  }
  return filelen;
}

/* ACOpenFile tries to open a file with the access attribute
   specified.  If fopen fails an error message is printed
   and the program exits if severity = FATAL. */
extern FILE *ACOpenFile(char * filename, char *access, short severity)
{
  FILE *stream;
  char dirname[MAXPATHLEN];

  stream = fopen(filename, access);
  if (stream == NULL) {
    GetInputDirName(dirname,"");
    switch (severity) {
    case (OPENERROR):
      sprintf(globmsg, "The %s file does not exist or is not accessible (currentdir='%s').\n", filename, dirname);
      LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
      break;
    case (OPENWARN):
      sprintf(globmsg, "The %s file does not exist or is not accessible (currentdir='%s').\n", filename, dirname);
      LogMsg(globmsg, WARNING, OK, TRUE);
      break;
    default:
      break;
    }
  }
  return (stream);
}

extern void FileNameLenOK(filename)
char *filename;
{
  if (strlen(filename) >= MAXFILENAME)
  {
    sprintf(globmsg, "File name: %s exceeds max allowable length of %d.\n",
      filename, (int) MAXFILENAME);
    LogMsg(globmsg, LOGERROR, FATALERROR, TRUE);
  }
}

extern void CharNameLenOK(charname)
char *charname;
{
  if (strlen(charname) >= MAXCHARNAME)
  {
    sprintf(globmsg,
      "Character name: %s exceeds maximum allowable length of %d.\n",
      charname, (int) MAXCHARNAME);
    LogMsg(globmsg, LOGERROR, FATALERROR, TRUE);
  }
}

extern void PathNameLenOK(pathname)
char *pathname;
{
  if (strlen(pathname) >= MAXPATHLEN)
  {
    sprintf(globmsg,
      "File name: %s exceeds maximum allowable length of %d.\n",
      pathname, (int) MAXPATHLEN);
    LogMsg(globmsg, LOGERROR, FATALERROR, TRUE);
  }
}

extern boolean BAKFile(filename)
char *filename;
{
  short length = (short)strlen(filename);
  if (length <= 4) return FALSE;
  if (!strcmp(&filename[length-4], BAKSUFFIX))
    return TRUE;
  return FALSE;
}

extern long GetMaxBytes()
{
  return MaxBytes;
}

extern void SetMaxBytes (value)
long value;
{
  MaxBytes = value;
}

/* Sets the global variable charsetDir. */
extern void set_charsetdir(dirname)
char *dirname;
{
  strcpy(charsetDir, dirname);
}

extern void getcharsetname(csname)
char *csname;
{
  strcpy (csname, charsetname);
}

void
SetCharsetParser(char *fileTypeString)
   {
   if (fileTypeString == NULL)
      charsetParser = bf_CHARSET_STANDARD;
   else if (STREQ(fileTypeString, "CID"))
      charsetParser = bf_CHARSET_CID;
   else
      charsetParser = bf_CHARSET_STANDARD;
   }

CharsetParser
GetCharsetParser()
   {
   return charsetParser;
   }

void
SetCharsetLayout(char *fontinfoString)
   {
   strcpy(charsetLayout, (fontinfoString == NULL)
         ? BF_STD_LAYOUT : fontinfoString);
   }

char *
GetCharsetLayout()
   {
   return charsetLayout;
   }

#define INCREMENT 3
static SubsetData *subsetdata = NULL;
static int allocation = INCREMENT;
static int allocated = 0;
static boolean usesSubset = FALSE;

boolean
UsesSubset(void)
   {
   return usesSubset;
   }

void
SetSubsetName(char *name)
   {
   strcpy(subsetname, name);
#if DOMEMCHECK
   subsetdata =(SubsetData *)memck_malloc(INCREMENT * sizeof(SubsetData));
#else
   subsetdata =(SubsetData *)AllocateMem(INCREMENT, sizeof(SubsetData), "SetSubsetName");
#endif
   usesSubset = TRUE;
   }

char *
GetSubsetName(void)
   {
   return(subsetname);
   }

char *
GetSubsetPath(void)
   {
   return(subsetPath);
   }

void
LoadSubsetData(void)
   {
   FILE *fp = ACOpenFile(subsetPath, "r", OPENERROR);
   int lo;
   int hi;
   int scanned;

   while ((scanned = fscanf(fp, "%d-%d", &lo, &hi)) > 0)
      {
      if (scanned == 1)
         subsetdata[allocated].lo = subsetdata[allocated].hi = lo;
      else
         {
         subsetdata[allocated].lo = lo;
         subsetdata[allocated].hi = hi;
         }

      if (++allocated >= allocation)
         {
         allocation += INCREMENT;
#if DOMEMCHECK
		 subsetdata = (SubsetData *)memck_realloc(subsetdata, (allocation*sizeof(SubsetData)));
#else
         subsetdata = (SubsetData *)ReallocateMem((char *)subsetdata,
               allocation*sizeof(SubsetData), "LoadSubsetData");
#endif
         }
      }

   fclose(fp);
   }

boolean
InSubsetData(int cid)
   {
   int i;

   for (i = 0; i < allocated; i++)
      {
      if ((cid >= subsetdata[i].lo) && (cid <= subsetdata[i].hi))
         return TRUE;
      }

   return FALSE;
   }

/*****************************************************************************/
/* Returns the name of the character set file.  If the filename exists on    */
/* the current directory use that file, otherwise, preface it with the       */
/* character set directory name.                                             */
/*****************************************************************************/
void
setcharsetname(boolean release, char *csname, char *baseFontPath)
   {
   char *filename;
   long i;
   char delimit[2];

   charsetname[0] = csname[0] = '\0';

   filename = GetFntInfo("CharacterSet", !release);
   if (filename == NULL)
      {
      LogMsg("CharacterSet keyword not found in fontinfo file.\n",
            WARNING, OK, TRUE);
      return;
      }

   if (baseFontPath != NULL)
      {
      sprintf(charsetname, "%s%s", baseFontPath, filename);
      sprintf(subsetPath, "%s%s", baseFontPath, subsetname);
      }
   else
      {
      sprintf(charsetname, "%s", filename);
      sprintf(subsetPath, "%s", subsetname);
      }

   if (!CFileExists(charsetname, FALSE))
      {
      get_filedelimit(delimit);
      if (((i = (long)strlen(charsetDir)) == 0) || (charsetDir[i - 1] == delimit[0]))
         delimit[0] = '\0';

      if (charsetParser == bf_CHARSET_CID)
         {
         sprintf(charsetname, "%s%s%s%s%s%s%s",
               charsetDir, delimit,
               filename, delimit,
               BF_LAYOUTS, delimit,
               charsetLayout);
         }
      else
         sprintf(charsetname, "%s%s%s", charsetDir, delimit, filename);
      }

   if (!CFileExists(subsetPath, FALSE))
      {
      get_filedelimit(delimit);
      if (((i = (long)strlen(charsetDir)) == 0) || (charsetDir[i - 1] == delimit[0]))
         delimit[0] = '\0';

      if (charsetParser == bf_CHARSET_CID)
         {
         sprintf(subsetPath, "%s%s%s%s%s%s%s",
               charsetDir, delimit,
               filename, delimit,
               BF_SUBSETS, delimit,
               subsetname);
         }
      else
         sprintf(subsetPath, "%s%s%s", charsetDir, delimit, subsetname);
      }
   
#if DOMEMCHECK
	memck_free(filename);
#else
   UnallocateMem(filename);
#endif
   PathNameLenOK(charsetname);
   strcpy(csname, charsetname);
   strcpy(charsetPath, charsetname);
   }

extern void get_working_dir(dirname)
char *dirname;
{
  strcpy(dirname, working_dir);
}

extern void init_working_dir()
{
  GetFullPathname(working_dir, 0, MININT);
  strcpy(initialWorkingDir, working_dir);
  initialWorkingDirLength = (int)strlen(initialWorkingDir);
}

extern void set_working_dir()
{
  set_current_dir(working_dir);
}

extern char *GetFItoAFMCharSetName()
{
  char *filename;
  
  if (strlen(charsetDir) == 0)
    return NULL; 
#if DOMEMCHECK
  filename = memck_malloc((strlen(charsetDir) + strlen(AFMCHARSETTABLE) + 2) * sizeof(char));
#else
  filename = AllocateMem((unsigned int)(strlen(charsetDir) + strlen(AFMCHARSETTABLE) + 2),
    sizeof(char), "AFM charset filename");
#endif
  get_filename(filename, charsetDir, AFMCHARSETTABLE);
  return filename;
}

extern short GetTotalInputDirs()
{
  return total_inputdirs;
}

extern void SetTotalInputDirs(short total)
{
  total_inputdirs = total;
}

/* insures that the meaningful data in the buffer is terminated
 * by an end of line marker and the null terminator for a string. */
extern long ACReadFile(textptr, fd, filename, filelength)
char *textptr;
FILE *fd;
char *filename;
long filelength;
{
  long cc;

  cc = ReadDecFile(
    fd, filename, textptr, TRUE, MAXINT, (unsigned long) (filelength),
    OTHER);
  fclose(fd);
  if (textptr[cc - 1] != NL || textptr[cc - 1] != '\r')
    textptr[cc++] = NL;
  textptr[cc] = '\0';
  return ((long) cc); 
}           /* ACReadFile */

boolean
IsHintRowMatch(char *hintDirName, char *rowDirName)
   {
   char fontDirName[MAXPATHLEN];
   int fontDirLength;

   /**************************************************************************/
   /* It is necessary to concatenate the hintDirName and rowDirName when     */
   /* doing comparisons because each of these may involve multiple path      */
   /* components: a/b and x/y/z -> a/b/x/y/z                                 */
   /**************************************************************************/
   sprintf(fontDirName, "%s/%s/", hintDirName, rowDirName);
   fontDirLength = (int)strlen(fontDirName);
   return ( (initialWorkingDirLength >= fontDirLength) &&
            (strcmp(initialWorkingDir + initialWorkingDirLength - fontDirLength,
               fontDirName) == 0));
   }

boolean
IsInFullCharset(char *bezName)
   {
   FILE *charsetFile;
   char line[501];
   char hintDirName[128];
   char rowDirName[128];
   char cname[128];

   if ((charsetFile = ACOpenFile(charsetPath, "r", OPENOK)) == NULL)
      return FALSE;
   while (fgets(line, 500, charsetFile) != NULL)
      {
      switch (charsetParser)
         {
      case bf_CHARSET_CID:
         if (  (sscanf(line, "%*d%s%s%s", hintDirName, rowDirName, cname)
                  == 3) &&
               IsHintRowMatch(hintDirName, rowDirName) &&
               (STREQ(cname, bezName)))
            {
            fclose(charsetFile);
            return TRUE;
            }
         break;
      case bf_CHARSET_STANDARD:
      default:
         if (  (sscanf(line, "%s", cname) == 1) &&
               (STREQ(cname, bezName)))
            {
            fclose(charsetFile);
            return TRUE;
            }
         break;
         }
      }

   fclose(charsetFile);
   return FALSE;
   }

/*****************************************************************************/
/* ReadNames is used for reading each successive character name from the     */
/* character set file.   It returns next free location for characters.       */
/*****************************************************************************/
extern char *
ReadNames(char *cname, char *filename, long *masters, long *hintDir,
      FILE *stream)
   {
#ifdef IS_GGL
	/* This should never be called with the GGL */
	return NULL;
#else
   int total_assigns;
   char line[501];
   static char *result;
   int done = 0;
   int cid;
   char hintDirName[128];
   char rowDirName[128];
/*   char fontDirName[256]; */
/*   int workingDirLength; */
/*   int fontDirLength; */

   while (!done && ((result = fgets(line, 500, stream)) != NULL))
      {
      switch (charsetParser)
         {
      case bf_CHARSET_CID:
         total_assigns = sscanf(line, "%d%s%s%s",
               &cid, hintDirName, rowDirName, cname);
         if (total_assigns == 4)
            {
            if (IsHintRowMatch(hintDirName, rowDirName))
               {
		 if (subsetdata)  {
		   if (InSubsetData(cid)) {
		     strcpy(filename, cname);
		     *masters = GetTotalInputDirs();
		     *hintDir = GetHintsDir();
		     done = 1;
		   }
		   else
		     done = 0;
		 }
		 else {
		   strcpy(filename, cname);
		   *masters = GetTotalInputDirs();
		   *hintDir = GetHintsDir();
		   done = 1;
		 }
               }
            }
         break;

      case bf_CHARSET_STANDARD:
      default:
         total_assigns = sscanf(line, " %s %s %ld %ld",
               cname, filename, masters, hintDir);
         if (total_assigns >= 1)
            {
            CharNameLenOK(cname);

            if (total_assigns == 1) strcpy(filename, cname);

	    if (multiplemaster) {
	      FileNameLenOK(filename);

	      if (total_assigns < 3) *masters = GetTotalInputDirs();

	      if (total_assigns < 4)
		*hintDir = GetHintsDir();
	      else
		--*hintDir; /* hintDir is zero-origin */
	    }
	    else {
	      strcpy(filename, cname);
	      *masters = 1;
	      *hintDir = 0;
	    }

            done = 1;
            }
         break;
         }
      }

   return result;
#endif
   }
