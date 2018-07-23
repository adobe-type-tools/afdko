/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)file.c	1.4
 * Changed:    12/15/97 08:57:36
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

#include "Dglobal.h"
#include "Dfile.h"
#include "Dsys.h"

typedef struct
	{
	IntX id;			/* File identifier */
	Byte8 *name;		/* File name */
	Card8 buf[BUFSIZ];	/* Input buffer */
	Card8 *next;		/* Next character to read */
	Card8 *end;			/* One past end of filled buffer */
	}  _file_;

static _file_ file1 = { (-1), NULL};
static _file_ file2 = { (-1), NULL};
static _file_ *filep;

IntX fileExists(Byte8 *filename)
	{
	  return (sysFileExists(filename));
	}

/* Open file read-only */
void fileOpen(Int8 which, Byte8 *filename)
	{
	  {
	  if (which == 1) 
		filep = &file1;
	  else
		filep = &file2;

	  filep->id = sysOpenSearchpath(filename);
	  filep->name = filename;
	  filep->end = filep->next = filep->buf;
	  memset(filep->buf, 0, BUFSIZ);
	  }
	}


IntX fileIsOpened(Card8 which)
	{
	  if (which == 1)
	  {
	  	if (file1.id == sysDIRECTORY) return (1);
		return (file1.id > 0);
	  }
	  else
	  {
	  	if (file2.id == sysDIRECTORY) return (1);
		return (file2.id > 0);
	   }
	}


/* Close file */
void fileClose(Card8 which)
	{
	  if (which == 1) 
		filep = &file1;
	  else
		filep = &file2;
	  if (filep->id > 0)
	  	sysClose(filep->id, filep->name);
	  filep->id = (-1);
	  filep->next = filep->end = NULL;
	  memset(filep->buf, 0, BUFSIZ);
	}

/* Return file position */
Card32 fileTell(Card8 which)
	{
	  if (which == 1)
		return (sysTell(file1.id, file1.name) - (file1.end - file1.next));
	  else
		return (sysTell(file2.id, file2.name) - (file2.end - file2.next));
	}


/* Seek to absolute or relative offset */
void fileSeek(Card8 which, Card32 offset, int relative)
	{
	Card32 at;
	Card32 to;

	if (which == 1) 
	  filep = &file1;
	else
	  filep = &file2;

	at = sysTell(filep->id, filep->name);
	to = relative ? at + offset : offset;
	
	if (to >= at - (filep->end - filep->buf) && to < at)
	  /* Offset already within current buffer */
	  filep->next = filep->end - (at - to);
	else
	  {
		/* Offset outside current buffer */
		sysSeek(filep->id, to, 0, filep->name);
		filep->next = filep->end;
	  }
	}

void fileSeekAbsNotBuffered(Card8 which, Card32 offset)
	{
	  if (which == 1)
		{
		  sysSeek(file1.id, offset, 0, file1.name);
		  file1.next = file1.end = file1.buf;
		  memset(file1.buf, 0, BUFSIZ);
		}
	  else
		{
		  sysSeek(file2.id, offset, 0, file2.name);
		  file2.next = file2.end = file2.buf;
		  memset(file2.buf, 0, BUFSIZ);
		}
	}


/* Fill buffer */
static void fillBuf(_file_ *file)
	{
	IntX count = sysRead(file->id, file->buf, BUFSIZ, file->name);
	if (count == 0)
		fatal("file error <premature EOF> [%s]\n", file->name);
	file->end = file->buf + count;
	file->next = file->buf;
	}

/* Supply specified number of bytes */
void fileReadBytes(Card8 which, Int32 count, Card8 *buf)
	{
	  if (which == 1) 
		filep = &file1;
	  else
		filep = &file2;
	  
	  while (count > 0)
		{
		  IntX size;
		  IntX left = filep->end - filep->next;			  
		  if (left == 0)
			{
			  fillBuf(filep);
			  left = filep->end - filep->next;
			}
		  
		  size = (left < count) ? left : count;
		  
		  memcpy(buf, filep->next, size);
		  filep->next += size;
		  buf += size;
		  count -= size;
		}
	}

/* 1, 2, and 4 byte big-endian machine independent input */
void fileReadObject(Card8 which, IntX size, ...)
	{
	Int32 value;
	va_list ap;

	if (which == 1) 
	  filep = &file1;
	else
	  filep = &file2;
	
	va_start(ap, size);
	if (filep->end - filep->next >= size)
	  /* Request doesn't cross block boundary */
	  switch (size)
		{
		case 1:
		  *va_arg(ap, Int8 *) = *filep->next++;
		  break;
		case 2:
		  value = *filep->next++;
		  *va_arg(ap, Int16 *) = (Int16)(value<<8 | *filep->next++);
		  break;
		case 4:
		  value = *filep->next++;
		  value = value<<8 | *filep->next++;
		  value = value<<8 | *filep->next++;
		  *va_arg(ap, Int32 *) = value<<8 | *filep->next++;
		  break;
		default:
		  fatal("bad input object size [%d]\n", size);
		  break;
		}
	else
	  {
		/* Read across block boundary */
		if (filep->next == filep->end) fillBuf(filep);
		switch (size)
		  {
		  case 1:
			*va_arg(ap, Int8 *) = *filep->next++;
			break;
		  case 2:
			value = *filep->next++;
			if (filep->next == filep->end) fillBuf(filep);
			*va_arg(ap, Int16 *) = (Int16)(value<<8 | *filep->next++);
			break;
		  case 4:
			value = *filep->next++;
			if (filep->next == filep->end) fillBuf(filep);
			value = value<<8 | *filep->next++;
			if (filep->next == filep->end) fillBuf(filep);
			value = value<<8 | *filep->next++;
			if (filep->next == filep->end) fillBuf(filep);
			*va_arg(ap, Int32 *) = value<<8 | *filep->next++;
			break;
		  default:
			fatal("bad input object size [%d]\n", size);
			break;
		  }
	  }
	va_end(ap);
	}


Byte8 *fileName(Card8 which)
	{
	  switch (which)
		{
		case 1:
		  return file1.name;
		  break;
		case 2:
		  return file2.name;
		  break;
		}
	  return NULL;
	}

Card32 fileSniff(Card8 which)
	{
	  IntX count = 0;
	  Card32 value = 0;
	  
	  fileSeekAbsNotBuffered(which, 0);
	  
	  if (which == 1) 
	  	filep = &file1;
	  else
	  	filep = &file2;
	  
	  count = sysRead(filep->id, filep->buf, 4, filep->name);
	  if (count == 0)
		value = 0xBADBAD;
	  else 
		{
		  filep->next = filep->buf;
		  value = *filep->next++;
		  if (filep->next == filep->end) fillBuf(filep);		  
		  value = value<<8 | *filep->next++;
		  if (filep->next == filep->end) fillBuf(filep);		  
		  value = value<<8 | *filep->next++;
		  if (filep->next == filep->end) fillBuf(filep);		  
		  value = value<<8 | *filep->next++;
		}
		
	  fileSeekAbsNotBuffered(which, 0);
	  return value;
      }
 
