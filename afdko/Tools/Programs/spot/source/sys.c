/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    %W%
 * Changed:    %G% %U%
 ***********************************************************************/

#include <stdio.h>

#include "sys.h"


#include <errno.h>
#include <fcntl.h>


#if MSDOS || WIN32
	#include <io.h>
	#define OPEN	_open
	#define OMODE	_O_RDONLY|_O_BINARY
	#define CLOSE	_close
	#define SEEK	_lseek
	#define READ	_read
#else
	#ifdef OSX
		#include <unistd.h>
	#endif
	#define OPEN	open
	#define OMODE	O_RDONLY
	#define CLOSE	close
	#define SEEK	lseek
	#define READ	read
#endif



#if OSX
	#include <sys/time.h>
#else
	#include <time.h>
#endif
#include <string.h>
#if WIN32 || SUNOS
#include <sys/stat.h>
#endif
#include <sys/types.h>

static void error(Byte8 *filename)
	{
#if MACINTOSH
          switch (errno) {
          case -36:       fatal(SPOT_MSG_sysIOERROR, filename);
                break;
          case -39:       fatal(SPOT_MSG_sysEOFERROR, filename);
                break;
          case -51:
          case -43:       fatal(SPOT_MSG_sysNOSUCHF, filename);
                break;          
          default:
                fatal(SPOT_MSG_sysMACFERROR, errno, filename);
          }
#elif OSX
	char errnumber[5];
	sprintf(errnumber, "%d", errno);
	fatal(SPOT_MSG_sysFERRORSTR, errnumber, filename);
#else
	fatal(SPOT_MSG_sysFERRORSTR, strerror(errno), filename);
#endif
	}

IntX sysFileExists(Byte8 *filename)
	{
	IntX fd;

	if (filename == NULL)
	  return 0;
	if (filename[0] == '\0')
	  return 0;
	  

	fd = sysOpenSearchpath(filename);
/* WAS:
	fd = OPEN(filename, OMODE);
*/

	if (fd < 0)
	  return 0;
	else
	  {
		CLOSE(fd);
		return 1;
	  }
	}

IntX sysOpen(Byte8 *filename)
	{
	IntX fd = OPEN(filename, OMODE);
	if (fd == -1)
		error(filename);
	return fd;
	}

void sysClose(IntX fd, Byte8 *filename)
	{
	if (CLOSE(fd) == -1)
		error(filename);
	}

Int32 sysTell(IntX fd, Byte8 *filename)
	{
	Int32 at = SEEK(fd, 0, SEEK_CUR);

	if (at == -1)
		error(filename);
	return at;
	}

void sysSeek(IntX fd, Int32 offset, IntX relative, Byte8 *filename)
	{
	Int32 at = SEEK(fd, offset, relative ? SEEK_CUR : SEEK_SET);

	if (at == -1)
		error(filename);
	}

Int32 sysFileLen(IntX fd)
	{
	Int32 at = SEEK(fd, 0, SEEK_END);

	if (at == -1)
	  return (-1);
	at = SEEK(fd, 0, SEEK_CUR);
	return (at);
	}

IntX sysRead(IntX fd, Card8 *buf, IntX size, Byte8 *filename)
 	{
	IntX count = READ(fd, (char *)buf, size);

	if (count == -1)
		error(filename);
	/*	if (count < size)	error(filename);	*/
	return count;
	}


Byte8 ourtday[32];

Byte8 *sysOurtime(void)
{
  time_t secs;
  static IntX done = 0;

  if (!done)
	{
    struct tm *tmp;
	  ourtday[0] = '\0';
	  (void) time (&secs);
        tmp = localtime(&(secs));
        if (strftime(ourtday, sizeof(ourtday), dateFormat, tmp) == 0) {
            fprintf(stderr, "strftime returned 0");
            exit(EXIT_FAILURE);
        }
	  ourtday[24] = '\0';
	  done = 1;
	}
  return ourtday;
}



#if MACINTOSH

#define kDirSep ':'
  
static void Macerror(Byte8 *filename, OSErr err)
      {
      fatal(SPOT_MSG_sysMACFERROR, err, filename);
      }


static OSErr
FSMakeFSRef(
	FSVolumeRefNum volRefNum,
	SInt32 dirID,
	ConstStr255Param name,
	FSRef *ref)
{
	OSErr		result;
	FSRefParam	pb;
	
	/* check parameters */
	if (NULL == ref) return(paramErr);
	
	pb.ioVRefNum = volRefNum;
	pb.ioDirID = dirID;
	pb.ioNamePtr = (StringPtr)name;
	pb.newRef = ref;
	result = PBMakeFSRefSync(&pb);
	return ( result );
}


static OSStatus
FSMakePath(
	SInt16 volRefNum,
	SInt32 dirID,
	ConstStr255Param name,
	UInt8 *path,
	UInt32 maxPathSize)
{
	OSStatus	result;
	FSRef		ref;
	
	/* check parameters */
	if (NULL == path) return(paramErr);
	
	/* convert the inputs to an FSRef */
	result = FSMakeFSRef(volRefNum, dirID, name, &ref);
	if (result != noErr) return result;
	
	/* and then convert the FSRef to a path */
	result = FSRefMakePath(&ref, path, maxPathSize);
	if (result != noErr) return result;

	return ( result );
}
 

static OSErr FSpGetFullPath(const FSSpec *spec, short *fullPathLength, Handle *fullPath)
{
	OSErr		result;
	OSErr		realResult;
	FSSpec		tempSpec;
	CInfoPBRec	pb;
	
	*fullPathLength = 0;
	*fullPath = NULL;
	
	
	/* Default to noErr */
	realResult = result = noErr;
	
	/* work around Nav Services "bug" (it returns invalid FSSpecs with empty names) */
	if ( spec->name[0] == 0 )
	{
		result = FSMakeFSSpec(spec->vRefNum, spec->parID, spec->name, &tempSpec);
	}
	else
	{
		/* Make a copy of the input FSSpec that can be modified */
		BlockMoveData(spec, &tempSpec, sizeof(FSSpec));
	}
	
	if ( result == noErr )
	{
		if ( tempSpec.parID == fsRtParID )
		{
			/* The object is a volume */
			
			/* Add a colon to make it a full pathname */
			++tempSpec.name[0];
			tempSpec.name[tempSpec.name[0]] = kDirSep;
			
			/* We're done */
			result = PtrToHand(&tempSpec.name[1], fullPath, tempSpec.name[0]);
		}
		else
		{
			/* The object isn't a volume */
			
			/* Is the object a file or a directory? */
			pb.dirInfo.ioNamePtr = tempSpec.name;
			pb.dirInfo.ioVRefNum = tempSpec.vRefNum;
			pb.dirInfo.ioDrDirID = tempSpec.parID;
			pb.dirInfo.ioFDirIndex = 0;
			result = PBGetCatInfoSync(&pb);
			/* Allow file/directory name at end of path to not exist. */
			realResult = result;
			if ( (result == noErr) || (result == fnfErr) )
			{
				/* if the object is a directory, append a colon so full pathname ends with colon */
				if ( (result == noErr) && (pb.hFileInfo.ioFlAttrib & kioFlAttribDirMask) != 0 )
				{
					++tempSpec.name[0];
					tempSpec.name[tempSpec.name[0]] = kDirSep;
				}
				
				/* Put the object name in first */
				result = PtrToHand(&tempSpec.name[1], fullPath, tempSpec.name[0]);
				if ( result == noErr )
				{
					/* Get the ancestor directory names */
					pb.dirInfo.ioNamePtr = tempSpec.name;
					pb.dirInfo.ioVRefNum = tempSpec.vRefNum;
					pb.dirInfo.ioDrParID = tempSpec.parID;
					do	/* loop until we have an error or find the root directory */
					{
						pb.dirInfo.ioFDirIndex = -1;
						pb.dirInfo.ioDrDirID = pb.dirInfo.ioDrParID;
						result = PBGetCatInfoSync(&pb);
						if ( result == noErr )
						{
							/* Append colon to directory name */
							++tempSpec.name[0];
							tempSpec.name[tempSpec.name[0]] = kDirSep;
							
							/* Add directory name to beginning of fullPath */
							(void) Munger(*fullPath, 0, NULL, 0, &tempSpec.name[1], tempSpec.name[0]);
							result = MemError();
						}
					} while ( (result == noErr) && (pb.dirInfo.ioDrDirID != fsRtDirID) );
				}
			}
		}
	}
	
	if ( result == noErr )
	{
		/* Return the length */
		*fullPathLength = GetHandleSize(*fullPath);
		result = realResult;	/* return realResult in case it was fnfErr */
	}
	else
	{
		/* Dispose of the handle and return NULL and zero length */
		if ( *fullPath != NULL )
		{
			DisposeHandle(*fullPath);
		}
		*fullPath = NULL;
		*fullPathLength = 0;
	}
	
	return ( result );	
}



static OSStatus
FSPathMakeFSSpec(
	const UInt8 *path,
	FSSpec *spec,
	Boolean *isDirectory)	/* can be NULL */
{
	OSStatus	result;
	FSRef		ref;
	
	/* check parameters */
	if (NULL == spec) return(paramErr);
	
	/* convert the POSIX path to an FSRef */
	result = FSPathMakeRef(path, &ref, isDirectory);
	if (result != noErr) return (result);
	
	/* and then convert the FSRef to an FSSpec */
	result = FSGetCatalogInfo(&ref, kFSCatInfoNone, NULL, NULL, spec, NULL);
	return ( result );
}


IntX sysOpenMac(Byte8 *OUTfilename)
      {
    Str255    myAppName = "\pspot";
 	OSErr     myErr = noErr;
 	NavReplyRecord   myReply;
 	NavDialogOptions  myDialogOptions;
 	AEKeyword     theKeyword;
    DescType      actualType;
    Size          actualSize;
    FSSpec        documentFSSpec;
    
 	NavGetDefaultDialogOptions(&myDialogOptions);
 	myDialogOptions.dialogOptionFlags -= kNavNoTypePopup;
 	myDialogOptions.dialogOptionFlags -= kNavAllowMultipleFiles;
 	BlockMoveData(myAppName, myDialogOptions.clientName, myAppName[0] + 1);

 	myErr = NavGetFile(NULL, &myReply, &myDialogOptions, NULL, NULL, 
 						(NavObjectFilterUPP)NULL, NULL, NULL);
 	if ((myErr == noErr) && myReply.validRecord) 
 		{
 		/* Get a pointer to selected file */
         myErr = AEGetNthPtr(&(myReply.selection), 1,
                            typeFSS, &theKeyword,
                             &actualType, &documentFSSpec,
                             sizeof(documentFSSpec),
                             &actualSize);
        if (myErr == noErr)
        	{
#if defined(__MWERKS__)
			Handle fullPathHdl = nil;
			short fullPathLen;
			myErr = FSpGetFullPath((const struct FSSpec *)&documentFSSpec, &fullPathLen, &fullPathHdl);
			if (myErr == noErr)
				{
				if (fullPathHdl && *fullPathHdl)
					{
					strncpy((char *)OUTfilename, (char *)(*fullPathHdl), fullPathLen);
					OUTfilename[fullPathLen] = '\0';
					}
				else
					{
					OUTfilename[0] = '\0'; /* failed */
					return (-1);
					}
				}
			else
				{
				OUTfilename[0] = '\0'; /* failed */
				return (-1);
				}
			if (fullPathHdl)
				{
				DisposeHandle(fullPathHdl);
				fullPathHdl = nil;
				}
			
#else        	
			myErr = FSMakePath(documentFSSpec.vRefNum, documentFSSpec.parID, documentFSSpec.name, OUTfilename, 255);
#endif
			return myErr;
			}
 		}
 	NavDisposeReply(&myReply);
 	return (-1);/* failed */
 	      
#if OLD 
		StandardFileReply theReply;
/*		Int16 fd; */
		IntX fd = (-1);
		OUTfilename[0] = '\0';
		StandardGetFile (nil, -1, nil, &theReply);
		if (!theReply.sfGood) return (-1);
		FSSpec2Path(&theReply.sfFile, (unsigned char *)OUTfilename); p2cstr(OUTfilename);
		fd = OPEN(OUTfilename, OMODE);
		if (fd == -1)
			error(OUTfilename);
			
		return fd;
#endif /* OLD */		
       }


FILE *sysPutMacFILE(Byte8 *defaultname, Byte8 *OUTfilename)
       {
       		FILE *f;
       		char filename[256];
       		int i=0;
       		
       		strcpy(filename, defaultname);
       		f=fopen(filename, "r");
       		while (f!=NULL){
       			fclose(f);
       			i++;
       			sprintf(filename, "%s%d", defaultname, i);
       			f=fopen(filename, "r");
       		}
       		fclose(f);
       		f=fopen(filename, "w");
       		strcpy(OUTfilename, filename);
       		return f;
       		
       
		 /*StandardFileReply theReply;
		 OSErr err;
		 Str255 deflt;
         short fd = (-1);
		 strcpy((char *)deflt, defaultname); c2pstr((char *)deflt);
		 StandardPutFile ("\pSave SPOT PS output as:", deflt, &theReply);
		 if (!theReply.sfGood) return (NULL);
		 strncpy((char *)OUTfilename, (const char *)(&(theReply.sfFile.name[1])), theReply.sfFile.name[0]);
		 err = FSpCreate(&theReply.sfFile, 'rbDG', 'TEXT', theReply.sfScript);
		 if (err && (err != dupFNErr))
		   Macerror(defaultname, err);
		 else
		   { 
		     FSSpec2Path(&theReply.sfFile, (unsigned char *)OUTfilename);
			 p2cstr(OUTfilename);
			 return (fopen((const char *)OUTfilename, "w"));
		   }*/
	   }

FILE *sysPutMacDesktopFILE (Byte8 *defaultname, Byte8 *OUTfilename)    
	{
		long DesktopDirID;
		short vr;
		OSErr err=0;
		Str255 deflt;
		FSSpec DesktopSpec;
		IntX retries=0;
		
		strcpy((char *)deflt, defaultname); c2pstrcpy(deflt, (const char *)deflt);
		/* locate Desktop folder */
		err = FindFolder (kOnSystemDisk, kDesktopFolderType, kDontCreateFolder, &vr, &DesktopDirID);
		if (err)
			Macerror(defaultname, err);
		/* make filespec for Desktop */
retry:
		err = FSMakeFSSpec (vr, DesktopDirID, deflt, &DesktopSpec);
		err = FSpCreate(&DesktopSpec, 'rbDG', 'TEXT', 0);
		if (err)
			{
			 if (err != dupFNErr)
		   		Macerror(defaultname, err);
		   	 else
		   	 	{ /* non-unique name on Desktop. Try to make unique--useful when scripting */
		   	 	 sprintf((char *)deflt, "%s%d", defaultname, retries); 
		   	 	 c2pstrcpy(deflt, (const char *)deflt);
		   	 	 retries++;
		   	 	 goto retry;
		   	 	}
		   	 }
		 else 
		 { 		 
		 	FSMakePath(DesktopSpec.vRefNum, DesktopSpec.parID, DesktopSpec.name, (UInt8 *)OUTfilename, 255);	 	
			return (fopen((const char *)OUTfilename, "w"));
		 }
		 return NULL;
	}  
	
IntX sysMacDesktopFileExists (Byte8 *defaultname, Byte8 *OUTfilename)    
	{
		long DesktopDirID;
		short vr;
		OSErr err=0;
		Str255 deflt;
		FSSpec DesktopSpec;
		short refNum;
				
		strcpy((char *)deflt, defaultname); c2pstrcpy(deflt, (const char *)deflt);
		/* locate Desktop folder */
		err = FindFolder (kOnSystemDisk, kDesktopFolderType, kDontCreateFolder, &vr, &DesktopDirID);
		if (err)
			Macerror(defaultname, err);
		/* make filespec for Desktop */

		err = FSMakeFSSpec (vr, DesktopDirID, deflt, &DesktopSpec);
		if (err) return (0);
		err = FSpOpenDF(&DesktopSpec, fsRdPerm, &refNum);
		if (err) return (0);
		{ 
		 	FSMakePath(DesktopSpec.vRefNum, DesktopSpec.parID, DesktopSpec.name, (UInt8 *)OUTfilename, 255);	
			return (1);
		 }
	}  
	
IntX sysMacOpenRes(Byte8 *filename)
	{
	Int16 fd;
	CInfoPBRec pb;
	Str255 name;
	Str255 vol;
	IntX length;

	if (filename == NULL) return (-1);
	if (filename[0] == '\0') return (-1);
	length = strlen(filename);
	/* Make Pascal strings */
	strncpy((char *)&vol[1], filename, vol[0] = length);
	strncpy((char *)&name[1], filename, name[0] = length);

	/* Make parameter block */
	pb.hFileInfo.ioCompletion = NULL;
	pb.hFileInfo.ioNamePtr = vol;
	pb.hFileInfo.ioVRefNum = 0;
	pb.hFileInfo.ioFDirIndex = 0;
	pb.hFileInfo.ioDirID = 0;

	fd=open(filename, O_RSRC|O_RDONLY);
	if (fd < 0)
		error(filename);
	return fd;
	}

#endif


IntX sysOpenSearchpath(Byte8 *filename)
{
 IntX fd;
 
#if WIN32 || SUNOS
	char **p;
	static char *path[] = {
	  "%s",
	  "/tmp/%s",
	  "c:/psfonts/%s",
	  "c:/temp/%s",
	  NULL
	};

	for (p = path; *p != NULL; p++) {  
	  struct _stat st;
	  char file[256];
	  
	  file[0] = '\0';
	  sprintf(file, *p, filename);
	  fd = OPEN(file, OMODE);
	  if ( (fd == (-1)) || 
	  	(_fstat(fd, &st) == (-1)) || ((st.st_mode & _S_IFMT) == _S_IFDIR) ) 
	  	continue;
	  else
	  	return fd;
	 }
#else
	fd = OPEN(filename, OMODE);
	return fd;
#endif

  return (-1);
}



