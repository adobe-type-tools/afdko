/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/
/* machinedep.c

history:

JLee	April 8, 1988

Judy Lee: Wed Jul  6 17:55:30 1988
End Edit History
*/
#include <fcntl.h>      /* Needed only for _O_RDWR definition */
#include <errno.h>


#include <sys/types.h>
#include <sys/stat.h>
#ifdef WIN32
#include <file.h>
#else
#include <sys/file.h>
#endif
#include <time.h>

#include <signal.h>
extern char *ctime();

#ifdef WIN32
#include <windows.h>
/* as of Visual Studi 2005, the POSIX names are deprecated; need to use
Windows specific names instead. 
*/
#include <direct.h>
#define chdir _chdir
#define mkdir _mkdir
#define getcwd _getcwd

#include <io.h>
#define access _access
#else
#include <unistd.h> /* for access(), lockf() */
#endif

#include "ac.h"
#include "fipublic.h"
#include "machinedep.h"




#define CURRENTID "CurrentID"
#define MAXID "MaxID"
#define MAXUNIQUEID 16777215L	/* 2^24 - 1 */
#define BUFFERSZ 512		/* buffer size used by unique id file */
#define MAXIDFILELEN 100	/* maximum length of line in unique id file */

static char ibmfilename[] = "ibmprinterfont.unprot";
static char uniqueIDFile[MAXPATHLEN];
static short warncnt = 0;
#if defined (__MWERKS__)
static char Delimiter[] = "/";
#elif defined (WIN32)
static char Delimiter[] = "\\";
#else
static char Delimiter[] = "/";
#endif

static int (*errorproc)(short); /* proc to be called from LogMsg if error occurs */

/* used for cacheing of log messages */
static char lastLogStr[MAXMSGLEN + 1] = "";
static short lastLogLevel = -1;
static boolean lastLogPrefix;
static int logCount = 0;

static void LogMsg1(char *str, short level, short code, boolean prefix);

short WarnCount()
{
    return warncnt;
}

void ResetWarnCount()
{
    warncnt = 0;
}

#ifdef IS_LIB
#define Write(s) { if (libReportCB != NULL)libReportCB( s);}
#define WriteWarnorErr(f,s) {if (libErrorReportCB != NULL) libErrorReportCB( s);}
#else
#define Write(s) 
#define WriteWarnorErr(f,s)
#endif

void set_errorproc(userproc)
int (*userproc)(short);
{
  errorproc = userproc;
}

/* called by LogMsg and when exiting (tidyup) */
 void FlushLogMsg(void)
{
  /* if message happened exactly 2 times, don't treat it specially */
  if (logCount == 1) {
    LogMsg1(lastLogStr, lastLogLevel, OK, lastLogPrefix);
  } else if (logCount > 1) {
    char newStr[MAXMSGLEN];
    sprintf(newStr, "The last message (%.20s...) repeated %d more times.\n",
	    lastLogStr, logCount);
    LogMsg1(newStr, lastLogLevel, OK, TRUE);
  }
  logCount = 0;
}

 void LogMsg( 
		char *str,			/* message string */
		short level,		/* error, warning, info */
		short code,		/* exit value - if !OK, this proc will not return */
		boolean prefix	/* prefix message with LOGERROR: or WARNING:, as appropriate */)
{
  /* changed handling of this to be more friendly (?) jvz */
  if (strlen(str) > MAXMSGLEN) {
    LogMsg1("The following message was truncated.\n", WARNING, OK, TRUE);
    ++warncnt;
  }
  if (level == WARNING)
    ++warncnt;
  if (!strcmp(str, lastLogStr) && level == lastLogLevel) {
    ++logCount; /* same message */
  } else { /* new message */
    if (logCount) /* messages pending */
      FlushLogMsg();
    LogMsg1(str, level, code, prefix); /* won't return if LOGERROR */
    strncpy(lastLogStr, str, MAXMSGLEN);
    lastLogLevel = level;
    lastLogPrefix = prefix;
  }
}

static void LogMsg1(char *str, short level, short code, boolean prefix)
{
  switch (level)
  {
  case INFO:
    Write(str);
    break;
  case WARNING:
    if (prefix)
      WriteWarnorErr(stderr, "WARNING: ");
    WriteWarnorErr(stderr, str);
    break;
  case LOGERROR:
    if (prefix)
      WriteWarnorErr(stderr, "ERROR: ");
    WriteWarnorErr(stderr, str);
    break;
  default:
     WriteWarnorErr(stderr, "ERROR - log level not recognized: ");
    WriteWarnorErr(stderr, str);
	break;  
  }
  if (level == LOGERROR && (code == NONFATALERROR || code == FATALERROR))
  	{
    (*errorproc)(code);
  	}
}

 void getidfilename(str)
char *str;
{
  strcpy(str, uniqueIDFile);
}

 void set_uniqueIDFile(str)
char *str;
{
  strcpy(uniqueIDFile, str);
}


static char afmfilename[] = "AFM";
static char macfilename[] = "macprinterfont.unprot";


void get_filename(char *name, char *str, const char *basename)
{
  sprintf(name, "%s%s%s", str, Delimiter, basename);
}

void get_afm_filename(name)
char *name;
{
  strcpy(name, afmfilename);
}

 void get_ibm_fontname(ibmfontname)
char *ibmfontname;
{
  strcpy(ibmfontname, ibmfilename);
}

void get_mac_fontname(macfontname)
char *macfontname;
{
  strcpy(macfontname, macfilename);
}

void get_time(time_t* value)
{
	time(value);
}

void get_datetime(datetimestr)
char *datetimestr;
{
  time_t secs;

  get_time(&secs);
  strncpy(datetimestr, ctime(&secs), 24);
  /* skip the 24th character, which is a newline */
  datetimestr[24] = '\0';
}


/* Sets the current directory to the one specified. */
void set_current_dir(current_dir)
char *current_dir;
{
  if ((chdir(current_dir)) != 0) {
    sprintf(globmsg,"Unable to cd to directory: '%s'.\n", current_dir);
    LogMsg(globmsg, WARNING, OK, TRUE);
  }
}


void get_current_dir(char *currdir)
{
  (void) getcwd(currdir, MAXPATHLEN);
}



/* Returns true if the given file exists, is not a directory
   and user has read permission, otherwise it returns FALSE. */
boolean FileExists(const char *filename, short errormsg)
{
  struct stat stbuff;
  int filedesc;

  if ((strlen(filename) == 0) && !errormsg)
    return FALSE;
  /* Check if this file exists and if it is a directory. */
  if (stat(filename, &stbuff) == -1)
  {
    if (errormsg)
    {
      sprintf(globmsg, "The %s file does not exist, but is required.\n", filename);
      LogMsg(globmsg, LOGERROR, OK, TRUE);
    }
    return FALSE;
  }
  if ((stbuff.st_mode & S_IFMT) == S_IFDIR)
  {
    if (errormsg)
    {
      sprintf(globmsg, "%s is a directory not a file.\n", filename);
      LogMsg(globmsg, LOGERROR, OK, TRUE);
    }
    return FALSE;
  }
  else

 /* Check for read permission. */
  if ((filedesc = access(filename, R_OK)) == -1)
  {
    if (errormsg)
    {
      sprintf(globmsg, "The %s file is not accessible.\n", filename);
      LogMsg(globmsg, LOGERROR, OK, TRUE);
    }
    return FALSE;
  }

  return TRUE;
}

boolean CFileExists(const char *filename, short errormsg)
{
	return FileExists(filename, errormsg);
}

boolean DirExists(char *dirname, boolean absolute, boolean create, boolean errormsg)
{
#ifndef WIN32
#pragma unused(absolute)
#endif
    long int access_denied = access(dirname, F_OK);
    
    if (access_denied)
    {
        if (errno == EACCES)
        {
            sprintf(globmsg, "The %s directory cannot be accessed.\n", dirname);
            LogMsg(globmsg, LOGERROR, OK, TRUE);
            return FALSE;
        }
        else
        {
            if (errormsg)
            {
                sprintf(globmsg, "The %s directory does not exist.", dirname);
                LogMsg(globmsg, create?WARNING:LOGERROR, OK, TRUE);
            }
            if (!create)
            {
                if (errormsg)
                    LogMsg("\n", LOGERROR, OK, FALSE);
                return FALSE;
            }
            if (errormsg)
                LogMsg("  It will be created for you.\n", WARNING, OK, FALSE);
            {
#ifdef WIN32
                int result = mkdir(dirname);
#else
                int result = mkdir(dirname, 00775);
#endif
                
                if (result)
                {
                    sprintf(globmsg, "Can't create the %s directory.\n", dirname);
                    LogMsg(globmsg, LOGERROR, OK, TRUE);
                    return FALSE;
                }
            } /* end local block for mkdir */
        }
    }
    return TRUE;
}



/* Returns the full name of the input directory. */
void GetInputDirName(name, suffix)
char *name;
char *suffix;
{
  char currdir[MAXPATHLEN];

  getcwd(currdir,MAXPATHLEN);
  sprintf(name, "%s%s%s", currdir, Delimiter, suffix);
}

unsigned long ACGetFileSize(filename)
char *filename;
{
  struct stat filestat;

  if ((stat(filename, &filestat)) < 0) return (0);
#ifdef CW80
	return((unsigned long) filestat.st_mtime);
#else
  return((unsigned long) filestat.st_size);
#endif
}

void RenameFile(oldName, newName)
char *oldName, *newName;
{
  if(FileExists(newName, FALSE))
  {
	if(remove(newName))
	{
		sprintf(globmsg, "Could not remove file: %s for renaming (%d).\n", newName, errno);
		LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);	
	}
  }
  if(rename(oldName, newName))
  {
    sprintf(globmsg, "Could not rename file: %s to: %s (%d).\n", oldName, newName, errno );
    LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
  }
}


void MoveDerivedFile(filename, basedir, fromDir, toDir)
char *filename, *basedir, *fromDir, *toDir;
{
  char fromname[MAXPATHLEN], toname[MAXPATHLEN];

  if (basedir == NULL)
  {
    get_filename(fromname, fromDir, filename);
    get_filename(toname, toDir, filename);
  }
  else
  {
    sprintf(fromname, "%s%s%s%s", basedir, fromDir, Delimiter, filename);
    sprintf(toname, "%s%s%s%s", basedir, toDir, Delimiter, filename);
  }
  RenameFile(fromname, toname);
}




void AppendFile (start, end)
char *start, *end;
{
  FILE *file1, *file2;
  char buffer [200];
  file2 = ACOpenFile (end, "r", OPENOK);
  if (file2 == NULL) 
    return;
  file1 = ACOpenFile (start, "a", OPENWARN);
  while (fgets (buffer, 200, file2) != NULL)
    fputs (buffer, file1);
  fclose (file1);
  fclose (file2);
} 



/* Converts :'s to /'s.  Colon's before the first alphabetic
   character are converted to ../ */
char *CheckBFPath(baseFontPath)
char *baseFontPath;
{
  char *uponelevel = "../";
  char *startpath, *oldsp;
  char newpath[MAXPATHLEN];
  short length;

  newpath[0] = '\0';
  if (strchr(baseFontPath, *Delimiter) == NULL)
  {
    if (*baseFontPath == *Delimiter)
      LogMsg("BaseFontPath keyword in fontinfo file must indicate\
  a relative path name.\n", LOGERROR, NONFATALERROR, TRUE);
    return baseFontPath;
  }

  /* Find number of colons at the start of the path */
  switch (length=(short)strspn(baseFontPath, Delimiter))
  {
    case (0):
      break;
    case (1):
      LogMsg("BaseFontPath keyword in fontinfo file must indicate\
  a relative path name.\n", LOGERROR, NONFATALERROR, TRUE);
      break;
    default:
      /* first two colons => up one, any subsequent single colon => up one */
      for (startpath = baseFontPath + 1; 
           startpath < baseFontPath + length; startpath++)
        if (*startpath == *Delimiter)
          strcat(newpath, uponelevel);
      break;
  }

  length = (short)strlen(startpath);
  /* replace all colons by slashes in remainder of input string -
    Note: this does not handle > 1 colon embedded in the middle of
    a Mac path name correctly */
  for (oldsp = startpath; startpath < oldsp + length; startpath++)
    if (*startpath == *Delimiter)
      *startpath = *Delimiter;
#if DOMEMCHECK
	startpath = memck_malloc((strlen(newpath) + length + 1) * sizeof(char));
#else
  startpath = AllocateMem(((unsigned int)strlen(newpath)) + length + 1, sizeof(char),
    "return string for CheckBFPath");
#endif
  strcpy (startpath, newpath);
  strcat(startpath, oldsp);
#if DOMEMCHECK
		memck_free(baseFontPath);
#else
		UnallocateMem(baseFontPath);
#endif
  return startpath;
}

/* Dummy procedures */

 void CreateResourceFile(filename)
char *filename;
{
#ifndef WIN32
#pragma unused(filename)
#endif
}

/* Returns full pathname of current working directory. */
void GetFullPathname(char *dirname, short vRefNum, long dirID)
{
#ifndef WIN32
#pragma unused(vRefNum)
#pragma unused(dirID)
#endif
	getcwd(dirname, MAXPATHLEN); 
   /* Append unix delimiter. */
   strcat(dirname, Delimiter);
}

/* Creates a file called .BFLOCK or .ACLOCK in the current directory.
   If checkexists is TRUE this proc will check if .BFLOCK or .ACLOCK exists
   before creating it.  If it does exist an error message will
   be displayed and buildfont will exit.
   The .BFLOCK file is used to indicate that buildfont is currently
   being run in the directory. The .ACLOCK file is used to indicate 
   that AC is currently being run in the directory. */
boolean createlockfile(fileToOpen, baseFontPath)
char *fileToOpen;
char *baseFontPath;  /* for error msg only; cd has been done */
{
  FILE *bf;

  if (CFileExists(BFFILE, FALSE))
  {
    sprintf(globmsg, "BuildFont is already running in the %s directory.\n", 
      (strlen(baseFontPath))? baseFontPath : "current");
    LogMsg(globmsg, LOGERROR, OK, TRUE);
    return FALSE;
  }
  if (CFileExists(ACFILE, FALSE))
  {
    sprintf(globmsg, "AC is already running in the %s directory.\n", 
      (strlen(baseFontPath))? baseFontPath : "current");
    LogMsg(globmsg, LOGERROR, OK, TRUE);
    return FALSE;
  }
  bf = ACOpenFile(fileToOpen, "w", OPENOK);
  if (bf == NULL)
  {
    sprintf(globmsg, 
      "Cannot open the %s%s file.\n", baseFontPath, fileToOpen);
    LogMsg(globmsg, LOGERROR, OK, TRUE);
    return FALSE;
  }
  fclose(bf);
  return TRUE;
}

void SetMacFileType(filename, filetype)
char *filename;
char *filetype;
{
#ifndef WIN32
#pragma unused(filename)
#pragma unused(filetype)
#endif
}


void closefiles()
{
#ifndef IS_LIB
	Write("Warning: closefiles unimplemented\n");
#endif
 /* register int counter, bad_ones = 0;
*/
  /* leave open stdin, stdout, stderr and console */
  /*for (counter = 4; counter < _NFILE; counter++)
    if (fclose(&_file[counter]))
      bad_ones++;
      */
}

void get_filedelimit(delimit)
char *delimit;
{
  strcpy(delimit, Delimiter);
}

/*
extern get_afm_filename(char *name)
{
  char fontname[MAXFILENAME];

  get_mac_fontname(fontname);
  sprintf(name, "%s%s", fontname, ".AFM");
  FileNameLenOK(name);
}

extern get_ibm_fontname(ibmfontname)
char *ibmfontname;
{
  char *fontinfostr;

  if ((fontinfostr = GetFntInfo("PCFileNamePrefix", ACOPTIONAL)) == NULL)
    strcpy(ibmfontname, ibmfilename);
  else
  {
    if (strlen(fontinfostr) != 5)
    {
      sprintf(globmsg, "PCFileNamePrefix in the %s file must be exactly 5 characters.\n", FIFILENAME);
      LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
    }
    strcpy(ibmfontname, fontinfostr);
    strcat(ibmfontname, "___.PFB");
    FreeFontInfo(fontinfostr);
  }
}

extern get_mac_fontname(macfontname)
char *macfontname;
{
  char *fontinfostr, *fontname;

  fontinfostr = GetFntInfo("FontName", MANDATORY);
  fontname = printer_filename(fontinfostr);
  strcpy(macfontname, fontname);
  FreeFontInfo(fontinfostr);
  UnallocateMem(fontname);
}

extern get_time(value)
long *value;
{
  GetDateTime(value);
}

extern get_datetime(char *datetimestr)
{
  long secs;
  char datestr[25], timestr[25];

  get_time(&secs);
  IUDateString(secs, abbrevDate, datestr);
  IUTimeString(secs, TRUE, timestr);
  sprintf(datetimestr, "%p %p", datestr, timestr);
}
*/
/* copies from one path ref num to another - can be different forks of same file */



 char *MakeTempName(char *dirprefix, char *fileprefix)
{
#ifndef WIN32
#pragma unused(dirprefix)
#pragma unused(fileprefix)
#endif
	return NULL;
}

 int AutoCrit (char *filenameparam, char *goo)
{
#ifndef WIN32
#pragma unused(filenameparam)
#pragma unused(goo)
#endif
	return 0;
}



int bf_alphasort(const void *f1, const void *f2)
{
	struct direct **mf1, **mf2;
	mf1=(struct direct **)f1;
	mf2=(struct direct **)f2;
	return strcmp((*mf1)->d_name, (*mf2)->d_name);
}

#if defined(_MSC_VER) && ( _MSC_VER < 1800)
float roundf(float x)
{
    float val =  (float)((x < 0) ? (ceil((x)-0.5)) : (floor((x)+0.5)));
    return val;
}
#endif


 unsigned char *CtoPstr(char *filename)
{
	int i, length=(int)strlen(filename);
	for (i=length; i>0; i--)
		filename[i]=filename[i-1];
	filename[0]=length;
	return (unsigned char*)filename;
}

 char *PtoCstr(unsigned char *filename)
{
	int i, length=filename[0];
	for (i=0; i<length; i++)
		filename[i]=filename[i+1];
	filename[i]='\0';
	
	return (char *)filename;
}

