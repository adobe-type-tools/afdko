/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)Esys.c	1.2
 * Changed:    2/18/99 09:33:26
 ***********************************************************************/

#include <stdio.h>
#include "Esys.h"
#include <errno.h>


#if MSDOS || WIN32
#include <io.h>
#define fileno _fileno
#endif /* MSDOS */

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>



static void error(char *filename)
	{

	fatal(SFED_MSG_sysFERRORSTR, strerror(errno), filename);
	}

int sysFileExists(char *filename)
	{
	FILE *f;

	if (filename == NULL)
	  return 0;
	if (filename[0] == '\0')
	  return 0;

	f = sysOpenSearchpath(filename);

	if (!f)
	  return 0;
	else
	  {
		fclose(f);
		return 1;
	  }
	}
	
long sysFileLen(FILE *f)
	{
	long at,cur;
	if (f == NULL)
		return (-1);
	cur = ftell(f);	
	at = fseek(f, 0, SEEK_END);
	if (at == -1)
	  return (-1);
	at = ftell(f);
	fseek(f, cur, SEEK_SET);
	return (at);
	}
	
FILE * sysOpenRead(char *filename)
	{
	FILE *f = fopen(filename, "rb");
	if (f == NULL)
		error(filename);
	return f;
	}

FILE * sysOpenWrite(char *filename)
	{
	FILE *f = fopen(filename, "w+b");
	
	if (f == NULL)
		error(filename);
	return f;
	}
	

FILE * sysOpenSearchpath(char *filename)
{
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
	  short fd;
	  FILE *f;
	  
	  file[0] = '\0';
	  sprintf(file, *p, filename);
	  f = fopen(file, "rb");
	  if (f == NULL) continue;
	  fd = fileno(f);
	  if (fd < 0) continue;
	  if ( (f == NULL) || 
	  	(fstat(fd, &st) == (-1)) || ((st.st_mode & S_IFMT) == S_IFDIR) ) 
	  	continue;
	  else
	  	return f;
	 }

  return (NULL);
}

