/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)Dsys.c	1.3
 * Changed:    2/12/99 10:38:09
 ***********************************************************************/

#include <stdio.h>

#include <errno.h>
#include <fcntl.h>

#ifdef __MWERKS__
#include <unix.h>
#include <unistd.h>

#define OPEN  open
#define OMODE O_RDONLY
#define CLOSE close
#define SEEK  lseek
#define READ  read
#endif


#if MSDOS || WIN32
#undef IN /* Dfile.h macro causes havoc */
#include <windows.h>
#include <io.h>
#include <direct.h>
#define OPEN	_open
#define OMODE	_O_RDONLY|_O_BINARY
#define CLOSE	_close
#define SEEK	_lseek
#define READ	_read
#undef IN
#undef MAX_PATH
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#define OPEN	open
#define OMODE	O_RDONLY
#define CLOSE	close
#define SEEK	lseek
#define READ	read
#endif 
#include "Dsys.h"


#include <string.h>

#if WIN32 || SUNOS
#include <sys/types.h>
#include <sys/stat.h>
#endif

static void error(Byte8 *filename)
	{
#ifdef __MWERKS__
	  switch (errno) {
	  case -36:       fatal("I/O Error [%s]\n", filename);
		break;
	  case -39:       fatal("EOF Error [%s]\n", filename);
		break;
	  case -51:
	  case -43:       fatal("No such file? [%s]\n", filename);
		break;
	  default:
		fatal("file error (%d) [%s]\n", errno, filename);
	  }
#else
	  fatal("file error <%s> [%s]\n", strerror(errno), filename);
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
	IntX count = READ(fd, (Byte8 *)buf, size);
	if (count == -1)
		error(filename);
	return count;
	}
	



IntX sysOpenSearchpath(Byte8 *filename)
{
 IntX fd = (-1);
 
	char **p;
	static char *path[] = {
	  "%s",
	  "/tmp/%s",
	  "c:/psfonts/%s",
	  "c:/temp/%s",
	  NULL
	};

	for (p = path; *p != NULL; p++) {  
	  struct stat st;
	  char file[256];
	  
	  file[0] = '\0';
	  sprintf( file, *p, filename);
	  fd = OPEN(file, OMODE);
	  if ( (fd == (-1)) || 
	  	(fstat(fd, &st) == (-1)) || ((st.st_mode & S_IFMT) == S_IFDIR) ) 
	  	continue;
	  else
	  	return fd;
}
	fd = OPEN(filename, OMODE);
	return fd;

  return (-1);
}


IntX sysIsDir(Byte8 *name)
{
  struct stat statbuf;
  if ((name == NULL) || (name[0] == '\0')) return 0;
  if (stat(name, &statbuf) < 0) return (0);
  return ((statbuf.st_mode & S_IFMT) == S_IFDIR);
  

}


IntX sysReadInputDir(Byte8 *dirName, Byte8 ***fileNameList)
{
    int fileCnt= 0;
#ifdef _WIN32
        int i=0;
    LPSTR path;
    int size;
    BOOL bMore;
    LPSTR name;
    HANDLE hFind;
    WIN32_FIND_DATA finddata;
    
    
    if ((dirName == NULL) || (dirName[0] == '\0')) return 0;
    size = strlen(dirName);
    path = (LPSTR)memNew(size + 5); /* 4 for "\*.*" + 1 for NULL */
    lstrcpy(path, dirName);
    lstrcat(path, "\\*.*");
    
    if ((hFind = FindFirstFile(path, &finddata)) == (HANDLE)(-1))
	{
        printf( "Cannot read contents of directory: %s.\n", dirName);
        memFree(path);
        quit(2);
	}
    bMore = TRUE;
    fileCnt = 0;
    while (bMore)					/* count the files */
	{
        if (!(finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            fileCnt++;
        bMore = FindNextFile(hFind, &finddata);
	}
    FindClose(hFind);
    
    *fileNameList = (Byte8 **) memNew ((fileCnt + 1) * sizeof (Byte8 *));
    hFind = FindFirstFile(path, &finddata);
    bMore = (hFind != (HANDLE)(-1));
    i=0;
    while (bMore)					/* record the filenames */
	{
        if (!(finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
            name = (LPSTR) &finddata.cFileName;
            size = lstrlen(name);
            (*fileNameList)[i] = (Byte8 *) memNew ((size + 1) * sizeof (Byte8));
            lstrcpy((*fileNameList)[i], name);
            i++;
		}
        bMore = FindNextFile(hFind, &finddata);
	}
    FindClose(hFind); 
    memFree(path);
	
#else
#ifdef __linux__ 
#define d_namlen d_reclen
#endif
 int i;
  DIR *thedir;
  struct dirent *entp;
	  
  if ((dirName == NULL) || (dirName[0] == '\0')) return 0;
  if ((thedir = opendir(dirName)) == NULL) {
	printf( "Cannot read contents of directory: %s.\n", dirName);
	quit(2);
  }
  fileCnt = 0;
  for (entp = readdir(thedir); entp != NULL; entp = readdir(thedir))
	{
	  if (entp->d_name[0] == '.') continue;
	  fileCnt++;
	}

  *fileNameList = (Byte8 **) memNew ((fileCnt + 1) * sizeof (Byte8 *));
  rewinddir(thedir);
  i=0;
  for (entp=readdir(thedir); entp != NULL; entp=readdir(thedir)) {
	if (entp->d_name[0] == '.') continue;
	(*fileNameList)[i] = (Byte8 *) memNew ((entp->d_namlen + 1) * sizeof (Byte8));
	strcpy((*fileNameList)[i], entp->d_name);
	i++;
  }

  closedir(thedir);
  
#endif
    return (fileCnt);
}
