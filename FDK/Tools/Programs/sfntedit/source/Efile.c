/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)Efile.c	1.1
 * Changed:    2/12/99 13:36:15
 ***********************************************************************/

/*
 * Machine independant file operations. The ANSI standard buffered i/o library
 * can't be used here because the Macintosh version doesn't support resource
 * reading. 
 *
 * This code assumes that 32 bits is large enough to hold a file offset. This
 * seems a safe assumtion since other code would break long before
 * fonts/resource files reached this size. 
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "Eglobal.h"
#include "Efile.h"
#include "Esys.h"
#include "errno.h"

extern int doingScripting;
extern char *sourcepath;

#if SUNOS
/* SUNOS doesn't define strerror in libc */
char *strerror(int errno)
	{
	extern int sys_nerr;
	extern char *sys_errlist[];
	return (errno >= 0 && errno <= sys_nerr)? 
		sys_errlist[errno]: "unknown error";
	}
#endif /* SUNOS */

/* Print file error message and quit */
static void fileError(File *fyl)
	{
	fatal(SFED_MSG_sysFERRORSTR, strerror(errno), fyl->name);
	}


int fileExists(char *filename)
	{
	  return (sysFileExists(filename));
	}

/* Open file read-only */
void fileOpenRead(char *filename, File *fyl)
	{
		{
		  fyl->fp = sysOpenSearchpath(filename);
		  if (fyl->fp == NULL)
		  	fileError(fyl);
		  fyl->name = filename;
		}
	}
	
void fileOpenWrite(char *filename, File *fyl)
	{
	  static char Wfnam[256];
		{
		  if (!doingScripting)
		  	{
			  fyl->fp = sysOpenWrite(filename);
			  if (fyl->fp == NULL)
			  	fileError(fyl);
			  fyl->name = filename;
			 }
		   else
		   	{ 
		   		/* make certain that "filename" is unique */
		   		int try = 0;
		  		strcpy(Wfnam, filename);
		   		while (fileExists(Wfnam))
		   		{
		   			sprintf(Wfnam, "%s%d", filename, try++);	
		   		}
		   		fyl->fp = sysOpenWrite(Wfnam);
			    if (fyl->fp == NULL)
			  	  fileError(fyl);
			    fyl->name = Wfnam;
		   	}
		}
	}

int fileIsOpened(File *fyl)
	{
	  return (fyl->fp != NULL);
	}

/* Close file */
void fileClose(File *fyl)
	{
/*	if (fyl->fp != NULL)
		if (ferror(fyl->fp))
			fileError(fyl);
*/
	if (fyl->fp != NULL)
		fclose(fyl->fp);
	fyl->fp = NULL;
	}

/* Return file position */
unsigned long fileTell(File *file)
	{
	long posn;
	if (file->fp != NULL)
		posn = ftell(file->fp);
	else
		fileError(file);
	return posn;
	}

/* Seek on file */
 void fileSeek(File *file, long offset, int wherefrom)
	{
	if (fseek(file->fp, offset, wherefrom) != 0)
		fileError(file);
	}
	
long fileLength(File *fyl)
	{
	if ((fyl == NULL) || (fyl->fp == NULL))
		return (-1);
	return sysFileLen(fyl->fp);
	}

/* Read N bytes from file */
 int fileReadN(File *file, size_t count, void *ptr)
	{
	size_t n = fread(ptr, 1, count, file->fp);

	if (n == 0 && ferror(file->fp))
		fileError(file);
	if (n < count)
		fileError(file);
	return n;
	}


/* 1, 2, and 4 byte big-endian machine independent input */
void fileReadObject(File *fyl, int size, void *obj)
	{
	long value;
	unsigned char b;
#define GET1B fileReadN(fyl, 1, &b)
	
	switch (size)
		{
	case 1:
		GET1B;
		*((unsigned char *)obj) = b;
		break;
	case 2:
		GET1B;	value = b;
		GET1B;  value = value<<8 | b;
		*((unsigned short *)obj) = (unsigned short)value;
		break;
	case 4:
		GET1B;	value = b;
		GET1B;  value = value<<8 | b;
		GET1B;  value = value<<8 | b;
		GET1B;  value = value<<8 | b;
		*((unsigned long *)obj) = value;
		break;
	default:
		fatal(SFED_MSG_BADREADSIZE, size);
		break;
		}
	}
	
/* Write N bytes to file */
 int fileWriteN(File *file, size_t count, void *ptr)
	{
	size_t n = fwrite(ptr, 1, count, file->fp);
	if (n != count)
		fileError(file);
	return n;
	}
/* 1, 2, and 4 byte big-endian machine independent input */
void fileWriteObject(File *fyl, int size, unsigned long value)
	{
	unsigned char b;
#define PUT1B fileWriteN(fyl, 1, &(b))
	
	switch (size)
		{
	case 1:
		b = (unsigned char)(value & 0xFF);
		PUT1B;
		break;
	case 2:
		b = (unsigned char)(value>>8 & 0xFF);
		PUT1B;
		b = (unsigned char)(value    & 0xFF);
		PUT1B;
		break;
	case 4:
		b = (unsigned char)(value>>24 & 0xFF); 
		PUT1B;
		b = (unsigned char)(value>>16 & 0xFF); 
		PUT1B;
		b = (unsigned char)(value>>8  & 0xFF); 
		PUT1B;
		b = (unsigned char)(value     & 0xFF); 
		PUT1B;
		break;
	default:
		fatal(SFED_MSG_BADWRITESIZE, size);
		break;
		}
	}		
	
/* Copy count bytes from source to destination files */
 void fileCopy(File *src, File *dst, size_t count)
	{
	char buf[BUFSIZ];

	while (count > BUFSIZ)
		{
		fileReadN(src, BUFSIZ, buf);
		fileWriteN(dst, BUFSIZ, buf);
		count -= BUFSIZ;
		}

	fileReadN(src, count, buf);
	fileWriteN(dst, count, buf);
	}
