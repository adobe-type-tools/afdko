/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)file.c	1.11
 * Changed:    3/16/99 09:35:29
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
#if USE_STDARG
#include <stdarg.h>
#endif
#include <string.h>
#include "global.h"
#include "file.h"
#include "sys.h"


static struct
	{
	IntX id;			/* File identifier */
	Byte8 *name;		/* File name */
	Card8 buf[BUFSIZ];	/* Input buffer */
	Card8 *next;		/* Next character to read */
	Card8 *end;			/* One past end of filled buffer */
	} file = { (-1), NULL};

#if MACINTOSH
static Byte8 gMacfilename[256] = {0};
#endif

IntX fileExists(Byte8 *filename)
	{
	  return (sysFileExists(filename));
	}

/* Open file read-only */
void fileOpen(Byte8 *filename)
	{
#if MACINTOSH
	  IntX errcode = 0;
	  
	  if (filename == NULL)
		{
		  if (!(global.doingScripting))
			{
			  gMacfilename[0] = '\0';
			  errcode = sysOpenMac(gMacfilename); /* prompts user */
			  if (!errcode)
			  	{
			  	file.id = sysOpenSearchpath(gMacfilename);
			  	file.name = gMacfilename;
			  	file.end = file.next = file.buf;
			  	}
			  else
			  	file.id = (-1);
			}
		   else
		   	 file.id = (-1);
		}
	  else
#endif
		{
/* WAS:	  file.id = sysOpen(filename); */
		  file.id = sysOpenSearchpath(filename);
		  file.name = filename;
		  file.end = file.next = file.buf;
		}
	}

#if MACINTOSH
/* Open file read-only */
void fileOpenMacRes(Byte8 *filename)
      {
        file.id = sysMacOpenRes(filename);
        file.name = filename;
        file.end = file.next = file.buf;
      }
#endif

IntX fileIsOpened(void)
	{
	  return (file.id > 0);
	}

/* Close file */
void fileClose(void)
	{
	if (file.id > 0)
		sysClose(file.id, file.name);
	file.id = (-1);
	/* don't mess with the name field */
	file.next = NULL;
	file.end = NULL;
	}

/* Return file position */
Card32 fileTell(void)
	{
	return sysTell(file.id, file.name) - (file.end - file.next);
	}

/* Seek to absolute or relative offset */
void fileSeek(Card32 offset, int relative)
	{
	Card32 at = sysTell(file.id, file.name);
	Card32 to = relative ? at + offset : offset;

#if OLD
	if (to >= at - (file.end - file.buf) && to < at)
		/* Offset already within current buffer */
		file.next = file.end - (at - to);
	else
		{
		/* Offset outside current buffer */
		sysSeek(file.id, to, 0, file.name);
		file.next = file.end;
		}
#else
	/* require that any SEEK invalidates buffer */
	sysSeek(file.id, to, 0, file.name);
	file.next = file.end = file.buf;
#endif

	}

void fileSeekAbsNotBuffered(Card32 offset)
	{
#if OLD
	  sysSeek(file.id, offset, 0, file.name);
	  file.next = file.end;
#else
	 fileSeek(offset, 0);
#endif
	}


/* Fill buffer */
static void fillBuf(void)
	{
	IntX count = sysRead(file.id, file.buf, BUFSIZ, file.name);
	if (count == 0)
		fatal(SPOT_MSG_EARLYEOF, file.name);
	file.end = file.buf + count;
	file.next = file.buf;
	}

/* Supply specified number of bytes */
void fileReadBytes(Int32 count, Card8 *buf)
	{
	while (count > 0)
		{
		IntX size;
		IntX left = file.end - file.next;

		if (left == 0)
			{
			fillBuf();
			left = file.end - file.next;
			}

		size = (left < count) ? left : count;

		memcpy(buf, file.next, size);
		file.next += size;
		buf += size;
		count -= size;
		}
	}

#if USE_STDARG
/* 1, 2, and 4 byte big-endian machine independent input */
void fileReadObject(IntX size, ...)
	{
	Int32 value;
	va_list ap;

	va_start(ap, size);
	if (file.end - file.next >= size)
		/* Request doesn't cross block boundary */
		switch (size)
			{
		case 1:
			*va_arg(ap, Int8 *) = *file.next++;
			break;
		case 2:
			value = *file.next++;
			*va_arg(ap, Int16 *) = value<<8 | *file.next++;
			break;
		case 4:
			value = *file.next++;
			value = value<<8 | *file.next++;
			value = value<<8 | *file.next++;
			*va_arg(ap, Int32 *) = value<<8 | *file.next++;
			break;
		default:
			fatal(SPOT_MSG_BADREADSIZE, size);
			break;
			}
	else
		{
		/* Read across block boundary */
		if (file.next == file.end) fillBuf();
		switch (size)
			{
		case 1:
			*va_arg(ap, Int8 *) = *file.next++;
			break;
		case 2:
			value = *file.next++;
			if (file.next == file.end) fillBuf();
			*va_arg(ap, Int16 *) = value<<8 | *file.next++;
			break;
		case 4:
			value = *file.next++;
			if (file.next == file.end) fillBuf();
			value = value<<8 | *file.next++;
			if (file.next == file.end) fillBuf();
			value = value<<8 | *file.next++;
			if (file.next == file.end) fillBuf();
			*va_arg(ap, Int32 *) = value<<8 | *file.next++;
			break;
		default:
			fatal(SPOT_MSG_BADREADSIZE, size);
			break;
			}
		}
	va_end(ap);
	}
#else
/* 1, 2, and 4 byte big-endian machine independent input */
void fileReadObject(IntX size, void *ap)
	{
	Int32 value, final;

	if (file.end - file.next >= size)
		/* Request doesn't cross block boundary */
		switch (size)
			{
		case 1:
			final = *file.next++;
			*((Int8 *)ap) = (Int8)final;
			break;
		case 2:
			value = *file.next++;
			final = value<<8 | *file.next++;
			*((Int16 *)ap) = (Int16)final;
			break;
		case 4:
			value = *file.next++;
			value = value<<8 | *file.next++;
			value = value<<8 | *file.next++;
			final = value<<8 | *file.next++;
			*((Int32 *)ap) = (Int32)final;
			break;
		default:
			fatal(SPOT_MSG_BADREADSIZE, size);
			break;
			}
	else
		{
		/* Read across block boundary */
		if (file.next == file.end) fillBuf();
		switch (size)
			{
		case 1:
			final = *file.next++;
			*((Int8 *)ap) = (Int8)final;
			break;
		case 2:
			value = *file.next++;
			if (file.next == file.end) fillBuf();
			final = value<<8 | *file.next++;
			*((Int16 *)ap) = (Int16)final;
			break;
		case 4:
			value = *file.next++;
			if (file.next == file.end) fillBuf();
			value = value<<8 | *file.next++;
			if (file.next == file.end) fillBuf();
			value = value<<8 | *file.next++;
			if (file.next == file.end) fillBuf();
			final = value<<8 | *file.next++;
			*((Int32 *)ap) = (Int32)final;
			break;
		default:
			fatal(SPOT_MSG_BADREADSIZE, size);
			break;
			}
		}
	}
#endif

Byte8 *fileName(void)
	{
	return file.name;
	}

Card32 fileSniff(void)
	{
	  IntX count = 0;
	  Card32 value;
	  count = sysRead(file.id, file.buf, 4, file.name);
	  file.end = file.buf + 4;
	  if (count == 0)
		value = 0xBADBAD;
	  else 
		{
		  file.next = file.buf;
		  value = *file.next++;
		  value = value<<8 | *file.next++;
		  value = value<<8 | *file.next++;
		  value = value<<8 | *file.next++;
		}
	  return value;
      }
