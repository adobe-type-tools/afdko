/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

#include "svgwrite.h"
#include "dynarr.h"
#include "dictops.h"
#include "txops.h"
#include "ctutil.h"
#include "supportexcept.h"

#include <stdarg.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <limits.h>

#define ARRAY_LEN(a) 	(sizeof(a)/sizeof((a)[0]))

/* ---------------------------- Library Context ---------------------------- */

typedef struct				/* Glyph data */
	{
	abfGlyphInfo *info;
	} Glyph;

struct svwCtx_
	{
	int state;				/* 0 == writing to tmp; 1 == writing to dst */
	abfTopDict *top;		/* Top Dict data */
	dnaDCL(Glyph, glyphs);	/* Glyph data */
	struct					/* Client-specified data */
		{
		long flags;			/* See svgwrite.h for flags */
		char *newline;
		} arg;
	struct					/* Destination stream */
		{
		char buf[BUFSIZ];
		size_t cnt;			/* Bytes in buf */
		} dst;
	struct					/* Temporary stream */
		{
		char buf[BUFSIZ];
		size_t cnt;			/* Bytes in buf */
		} tmp;
	struct					/* Glyph path */
		{
		float x;
		float y;
		int state;
		} path;
	struct					/* Streams */
		{
		void *dst;
		void *tmp;
		void *dbg;
		} stm;
	struct					/* Client callbacks */
		{
		ctlMemoryCallbacks mem;
		ctlStreamCallbacks stm;
		} cb;
	dnaCtx dna;				/* dynarr context */
	struct					/* Error handling */
		{
		_Exc_Buf env;
		int code;
		} err;
	};

/* ----------------------------- Error Handling ---------------------------- */

/* Handle fatal error. */
static void fatal(svwCtx h, int err_code)
	{
	if (h->stm.dbg != NULL)
		{
		/* Write error message to debug stream */
		char *text = svwErrStr(err_code);
		(void)h->cb.stm.write(&h->cb.stm, h->stm.dbg, strlen(text), text);
		}
	h->err.code = err_code;
	RAISE(&h->err.env, err_code, NULL);
	}

/* --------------------------- Destination Stream -------------------------- */

/* Flush dst/tmp stream buffer. */
static void flushBuf(svwCtx h)
	{
	void *stm;
	char *buf;
	size_t *cnt;
	int err;
	if (h->state == 0)
		{
		stm = h->stm.tmp;
		buf = h->tmp.buf;
		cnt = &h->tmp.cnt;
		err = svwErrTmpStream;
		}
	else  /* h->state == 1 */
		{
		stm = h->stm.dst;
		buf = h->dst.buf;
		cnt = &h->dst.cnt;
		err = svwErrDstStream;
		}

	if (*cnt == 0)
		return;	/* Nothing to do */

	/* Write buffered bytes */
	if (h->cb.stm.write(&h->cb.stm, stm, *cnt, buf) != *cnt)
		fatal(h, err);

	*cnt = 0;
	}

/* Write to dst/tmp stream buffer. */
static void writeBuf(svwCtx h, size_t writeCnt, const char *ptr)
	{
	char *buf;
	size_t *cnt;
	int err;
	size_t left;
	if (h->state == 0)
		{
		buf = h->tmp.buf;
		cnt = &h->tmp.cnt;
		err = svwErrTmpStream;
		}
	else  /* h->state == 1 */
		{
		buf = h->dst.buf;
		cnt = &h->dst.cnt;
		err = svwErrDstStream;
		}

	left = BUFSIZ - *cnt;	/* Bytes left in buffer */
	while (writeCnt >= left)
		{
		memcpy(&buf[*cnt], ptr, left);
		*cnt += left;
		flushBuf(h);
		ptr += left;
		writeCnt -= left;
		left = BUFSIZ;
		}
	if (writeCnt > 0)
		{
		memcpy(&buf[*cnt], ptr, writeCnt);
		*cnt += writeCnt;
		}
	}

/* Write integer value as ASCII to dst stream. */
static void writeInt(svwCtx h, long value)
	{
	char buf[50];
	sprintf(buf, "%ld", value);
	writeBuf(h, strlen(buf), buf);
	}

/* Convert a long into a string */
static void svw_ltoa(char* buf, long val)
{
	static char ascii_digit[] = {'0','1','2','3','4','5','6','7','8','9'};
	char *position = buf;
	char *start = buf;
	int digit;
	char tmp; 

	int isneg = val<0;
	if (isneg)
		val = -val;

	/* extract char's */
	do
	{
		digit = val % 10;
		val/=10;
		*position = ascii_digit[digit];
		position++;
	} while (val);

	/* store sign */
	if (isneg)
	{
		*position = '-';
		position++;
	}

	/* terminate */
	*position = '\0';
	position--;

	/* reverse string */
	while (start<position)
	{
		tmp = *position;
		*position = *start;
		*start = tmp;
		position--;
		start++;
	}
}
/* Write real number in ASCII to dst stream. */
#define TX_EPSILON 0.0003 
/*In Xcode, FLT_EPSILON is 1.192..x10-7, but the diff between value-roundf(value) can be 3.05..x10-5, when the input value is from a 24.8 fixed. */
static void writeReal(svwCtx h, float value)
{
	char buf[50];
	/* if no decimal component, perform a faster to string conversion */
	if ((fabs(value-roundf(value)) < TX_EPSILON) && (value>LONG_MIN) && (value<LONG_MAX))
		svw_ltoa(buf, (long)roundf(value));
	else
		ctuDtostr(buf, sizeof(buf), value, 0, 2);
	writeBuf(h, strlen(buf), buf);
}

/* Write null-terminated string to dst steam. */
static void writeStr(svwCtx h, const char *s)
	{
	writeBuf(h, strlen(s), s);
	}

/* Write null-terminated string to dst steam and escape XML reserved characters. */
static void writeXMLStr(svwCtx h, const char *s)
	{
	/* 64-bit warning fixed by cast here */
	long len = (long)strlen(s);
	int i;
	char buf[9];
	unsigned char code;

	for (i=0;i<len;i++)
		{
		code = s[i];
		if (code & 0x80)
			{
			writeStr(h, "&#x");
			sprintf(buf, "%X", code);
			writeStr(h, buf);
			writeStr(h, ";");
			}
		else
			{
			switch(code)
				{
				case '<':
					writeStr(h, "&lt;");
					break;
				case '>':
					writeStr(h, "&gt;");
					break;
				case '&':
					writeStr(h, "&amp;");
					break;
				case '"':
					writeStr(h, "&quot;");
					break;
				default:
					if (code<0x20 && !(code==0x9 || code==0xa || code==0xd))
						continue; /* xml 1.0 limits control points to x9,xa,xd */

					buf[0] = code;
					buf[1] = '\0';
					writeStr(h, buf);
				}
			}
		}
	}

/* Write null-terminated string followed by newline. */
static void writeLine(svwCtx h, char *s)
	{
	writeStr(h, s);
	writeStr(h, h->arg.newline);
	}

/* Write formatted data to dst stream. This function must only be called when
   the maximum size of the resulting formatted string is known in advance. It
   must never be called with a string that has been passed into this library
   since it might cause a buffer overrun. Those strings may be handled safely
   by calling writeStr() directly. */
static void CTL_CDECL writeFmt(svwCtx h, char *fmt, ...)
	{
	char buf[200];
	va_list ap;
	va_start(ap, fmt);
	vsprintf(buf, fmt, ap);
	writeStr(h, buf);
	va_end(ap);
	}

/* --------------------------- Context Management -------------------------- */

/* Validate client and create context. */
svwCtx svwNew(ctlMemoryCallbacks *mem_cb, ctlStreamCallbacks *stm_cb,
			  CTL_CHECK_ARGS_DCL)
	{
	svwCtx h;

	/* Check client/library compatibility */
	if (CTL_CHECK_ARGS_TEST(SVW_VERSION))
		return NULL;

	/* Allocate context */
	h = mem_cb->manage(mem_cb, NULL, sizeof(struct svwCtx_));
	if (h == NULL)
		return NULL;

	/* Safety initialization */
	h->state = 0;
	h->top = NULL;
	h->glyphs.size = 0;
	h->dna = NULL;
	h->stm.dst = NULL;
	h->stm.dbg = NULL;
	h->err.code = svwSuccess;

	/* Copy callbacks */
	h->cb.mem = *mem_cb;
	h->cb.stm = *stm_cb;

	/* Initialize service library */
	h->dna = dnaNew(&h->cb.mem, DNA_CHECK_ARGS);
	if (h->dna == NULL)
		goto cleanup;

	dnaINIT(h->dna, h->glyphs, 256, 750);

	/* Open debug stream */
	h->stm.dbg = h->cb.stm.open(&h->cb.stm, SVW_DBG_STREAM_ID, 0);

	return h;

 cleanup:
	/* Initialization failed */
	svwFree(h);
	return NULL;
	}

/* Free context. */
void svwFree(svwCtx h)
	{
	if (h == NULL)
		return;

	/* Close debug stream */
	if (h->stm.dbg != NULL)
		(void)h->cb.stm.close(&h->cb.stm, h->stm.dbg);

	dnaFREE(h->glyphs);
	dnaFree(h->dna);

	/* Free library context */
	h->cb.mem.manage(&h->cb.mem, h, 0);

	return;
	}

/* Begin font. */
int svwBegFont(svwCtx h, long flags)
	{
	/* Validate glyphnames */
	switch (flags & SVW_GLYPHNAMES_MASK)
		{
	case SVW_GLYPHNAMES_NONE:
	case SVW_GLYPHNAMES_ALL:
	case SVW_GLYPHNAMES_NONASCII:
		break;
	default:
		return svwErrBadCall;
		}

	/* Validate newline */
	switch (flags & SVW_NEWLINE_MASK)
		{
	default:
	case SVW_NEWLINE_UNIX:
		h->arg.newline = "\n";
		break;
	case SVW_NEWLINE_WIN:
		h->arg.newline = "\r\n";
		break;
	case SVW_NEWLINE_MAC:
		h->arg.newline = "\r";
		break;
		}

	/* Initialize */
	h->arg.flags = flags;
	h->tmp.cnt = 0;
	h->dst.cnt = 0;
	h->glyphs.cnt = 0;
	h->path.state = 0;
	h->top = NULL;
	h->stm.dst = NULL;

	/* Open tmp stream */
	h->stm.tmp = h->cb.stm.open(&h->cb.stm, SVW_TMP_STREAM_ID, 0);
	if (h->stm.tmp == NULL)
		return svwErrTmpStream;
	h->state = 0;  /* Indicates writing to tmp stream */

	return svwSuccess;
	}

/* Finish reading font. */
int svwEndFont(svwCtx h, abfTopDict *top)
	{
	size_t cntTmp = 0;
	size_t cntRead = 0;
	size_t cntWrite = 0;
	char *pBuf = NULL;

	/* Check for errors when accumulating glyphs */
	if (h->err.code != 0)
		return h->err.code;

	h->top = top;

	/* Set error handler */
  DURING_EX(h->err.env)

    flushBuf(h);  /* Flush tmp stream */
    h->state = 1;  /* Indicates writing to dst stream */

    /* Open dst stream */
    h->stm.dst = h->cb.stm.open(&h->cb.stm, SVW_DST_STREAM_ID, 0);
    if (h->stm.dst == NULL)
      fatal(h, svwErrDstStream);

    if(h->arg.flags & SVW_STANDALONE)
    {
      char tmpBuf[64];
      writeLine(h, "<?xml version=\"1.0\" encoding=\"utf-8\"?>");
      writeStr(h, "<!-- Generator: Adobe svgwrite library ");
      sprintf(tmpBuf, "%1d.%1d.%1d", CTL_SPLIT_VERSION(SVW_VERSION));
      writeStr(h, tmpBuf);
      writeLine(h, " -->");
      writeLine(h, "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.0//EN\" \"http://www.w3.org/TR/SVG/DTD/svg10.dtd\">");
      writeLine(h, "<svg>");
    }

    writeStr(h, "<font horiz-adv-x=\"");
    writeInt(h, h->top->sup.UnitsPerEm);
    writeLine(h, "\">");

    /* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
  This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
    if(h->top->Notice.ptr != ABF_UNSET_PTR)
      {
      writeStr(h, "<!-- ");
      writeXMLStr(h, h->top->Notice.ptr);
      writeLine(h, " -->");
      }
    else if(h->top->Copyright.ptr != ABF_UNSET_PTR)
      {
      writeStr(h, "<!-- ");
      writeXMLStr(h, h->top->Copyright.ptr);
      writeLine(h, " -->");
      }

    /* Add derivative copyright notice if necessary: */
    if ((h->top->Notice.ptr == ABF_UNSET_PTR ||
       strstr(h->top->Notice.ptr, "Adobe") == NULL) &&
      (h->top->Copyright.ptr == ABF_UNSET_PTR ||
       strstr(h->top->Copyright.ptr, "Adobe") == NULL))
      {
      /* Non-Adobe font; add Adobe copyright to derivative work */
      time_t now = time(NULL);
      writeFmt(h, "<!-- Copyright: Copyright %d Adobe System Incorporated. "
           "All rights reserved. -->%s", 
           localtime(&now)->tm_year + 1900, h->arg.newline);
      }

    writeStr(h, "<font-face font-family=\"");
    if (h->top->sup.flags & ABF_CID_FONT)
      writeXMLStr(h, h->top->cid.CIDFontName.ptr);
    else
      writeXMLStr(h, h->top->FDArray.array[0].FontName.ptr);
    writeStr(h, "\"");

    writeStr(h, " units-per-em=\"");
    writeInt(h, h->top->sup.UnitsPerEm);
    writeStr(h, "\"");

    writeStr(h, " underline-position=\"");
    writeReal(h, h->top->UnderlinePosition);
    writeStr(h, "\"");

    writeStr(h, " underline-thickness=\"");
    writeReal(h, h->top->UnderlineThickness);
    writeLine(h, "\"/>");

    /* Transfer tmp stream to dst stream */
    if ((cntTmp = h->cb.stm.tell(&h->cb.stm, h->stm.tmp)) == (unsigned long)-1)
      fatal(h, svwErrTmpStream);
    if (h->cb.stm.seek(&h->cb.stm, h->stm.tmp, 0) != 0)
      fatal(h, svwErrTmpStream);
    while((cntRead = h->cb.stm.read(&h->cb.stm, h->stm.tmp, &pBuf)) != 0)
      {
      cntWrite = (cntTmp<cntRead)?cntTmp:cntRead;
      writeBuf(h, cntWrite, pBuf);
      cntTmp -=cntRead;
      }

    writeLine(h, "</font>");

    if(h->arg.flags & SVW_STANDALONE)
      writeLine(h, "</svg>");

    flushBuf(h);

    /* Close tmp stream */
    if (h->cb.stm.close(&h->cb.stm, h->stm.tmp) == -1)
      fatal(h, svwErrTmpStream);
    h->stm.tmp = NULL;

    /* Close dst stream */
    if (h->cb.stm.close(&h->cb.stm, h->stm.dst) == -1)
      return svwErrDstStream;

	HANDLER

		if (h->stm.tmp)
			h->cb.stm.close(&h->cb.stm, h->stm.tmp);
		if (h->stm.dst)
			h->cb.stm.close(&h->cb.stm, h->stm.dst);
		return h->err.code;

	END_HANDLER

	return svwSuccess;
	}

/* Get version numbers of libraries. */
void svwGetVersion(ctlVersionCallbacks *cb)
	{
	if (cb->called & 1<<SVW_LIB_ID)
		return;	/* Already enumerated */

	/* Support libraries */
	dnaGetVersion(cb);

	/* This library */
	cb->getversion(cb, SVW_VERSION, "svwrite");

	/* Record this call */
	cb->called |= 1<<SVW_LIB_ID;
	}

/* Map error code to error string. */
char *svwErrStr(int err_code)
	{
	static char *errstrs[] =
		{
#undef CTL_DCL_ERR
#define CTL_DCL_ERR(name,string)	string,
#include "svwerr.h"
		};
	return (err_code < 0 || err_code >= (int)ARRAY_LEN(errstrs))?
		"unknown error": errstrs[err_code];
	}

/* ------------------------------ Glyph Path  ------------------------------ */

/* Begin new glyph definition. */
static int glyphBeg(abfGlyphCallbacks *cb, abfGlyphInfo *info)
	{
	svwCtx h = cb->direct_ctx;
	char buf[9];
	int isASCII = 0;

	cb->info = info;

	if (h->err.code != 0)
		return ABF_FAIL_RET;	/* Pending error */
	else if (h->path.state != 0)	
		{
		/* Call sequence error */
		h->err.code = svwErrBadCall;
		return ABF_FAIL_RET;
		}
	else if ( !(info->flags & ABF_GLYPH_UNICODE) )
		{
		/* Unicode value not specified */
		h->err.code = svwErrNotUnicode;
		return ABF_FAIL_RET;
		}
	else if (info->flags & ABF_GLYPH_SEEN)
		return ABF_SKIP_RET;	/* Ignore duplicate glyph */
	else if ( info->encoding.code < 0x20 &&
			  !(info->encoding.code == 0x9 || 
			    info->encoding.code == 0xa || 
			    info->encoding.code == 0xd))
		return ABF_SKIP_RET;	/* xml 1.0 limits control points to x9,xa,xd */
	else if ( info->encoding.code == 0xFFFE )
		return ABF_SKIP_RET;	/* 0XFFFE is a unicode surrogate block not allowed by xml 1.0 */

	/* Initialize */
	h->path.x = 0;
	h->path.y = 0;
	h->path.state = 1;

	if (info->encoding.code == 0xFFFF)  /* Signifies .notdef glyph */
		writeStr(h, "<missing-glyph");
	else
		{
		writeStr(h, "<glyph unicode=\"");
		if((info->encoding.code & 0xFFFFFF80) == 0)
			{
			char ascii=(char)info->encoding.code;
			isASCII = 1;
			switch(ascii)
				{
				case '&':
					writeStr(h, "&amp;");
					break;
				case '"':
					writeStr(h, "&quot;");
					break;
				case '<':
					writeStr(h, "&lt;");
					break;
				case '>':
					writeStr(h, "&gt;");
					break;
				default:
					buf[0] = ascii;
					buf[1] = '\0';
					writeStr(h, buf);
				}
			}
		else /* Not ASCII */
			{
			writeStr(h, "&#x");
			sprintf(buf, "%lX", info->encoding.code);
			writeStr(h, buf);
			writeStr(h, ";");
			}
		writeStr(h, "\"");
		if(((h->arg.flags & SVW_GLYPHNAMES_NONASCII) && !isASCII) ||
		   (h->arg.flags & SVW_GLYPHNAMES_ALL))
			{
			writeStr(h, " glyph-name=\"");
			if (info->gname.ptr != NULL)
				writeXMLStr(h, info->gname.ptr);
			else
				{
				sprintf(buf, "%05hu", info->cid);
				writeStr(h, buf);
				}

			writeStr(h, "\"");
			}
		}

	return ABF_CONT_RET;
	}

/* Add horizontal advance width. */
static void glyphWidth(abfGlyphCallbacks *cb, float hAdv)
	{
	svwCtx h = cb->direct_ctx;

	if (h->err.code != 0)
		return;	/* Pending error */
	else if (h->path.state != 1)
		{
		/* Call sequence error */
		h->err.code = svwErrBadCall;
		return;
		}

	writeStr(h, " horiz-adv-x=\"");
	writeInt(h, (long)roundf(hAdv));
	writeStr(h, "\"");

	h->path.state = 2;
	}

/* Add move to path. */
static void glyphMove(abfGlyphCallbacks *cb, float x0, float y0)
	{
	svwCtx h = cb->direct_ctx;
	h->path.x = x0;
	h->path.y = y0;

	if (h->err.code != 0)
		return;	/* Pending error */
	else if (h->path.state < 2)
		{
		/* Call sequence error */
		h->err.code = svwErrBadCall;
		return;
		}

	if (h->path.state < 3)
		/* First moveto for this glyph implies start of path data. */
		writeStr(h, " d=\"");
	else
		{
		/* Close path. */
		if (h->arg.flags & SVW_ABSOLUTE)
			writeStr(h, " Z");
		else
			writeStr(h, " z");
		writeStr(h, " ");
		}

	if (h->arg.flags & SVW_ABSOLUTE)
		{
		writeStr(h, "M ");
		writeReal(h, x0);
		writeStr(h, ",");
		writeReal(h, y0);
		}
	else
		{
		writeStr(h, "M");
		writeReal(h, x0);
		writeStr(h, ",");
		writeReal(h, y0);
		}
	h->path.state = 3;
	}

/* Add line to path. */
static void glyphLine(abfGlyphCallbacks *cb, float x1, float y1)
	{
	svwCtx h = cb->direct_ctx;
	float dx1 = x1 - h->path.x; 
	float dy1 = y1 - h->path.y;
	h->path.x = x1;
	h->path.y = y1;

	if (h->err.code != 0)
		return;	/* Pending error */
	else if (h->path.state != 3)
		{
		/* Call sequence error */
		h->err.code = svwErrBadCall;
		return;
		}
	if (h->arg.flags & SVW_ABSOLUTE)
		{
		writeStr(h, " L ");
		writeReal(h, x1);
		writeStr(h, ",");
		writeReal(h, y1);
		}
	else
		{
		writeStr(h, "l");
		writeReal(h, dx1);
		writeStr(h, ",");
		writeReal(h, dy1);
		}
 }

/* Add curve to path. */
static void glyphCurve(abfGlyphCallbacks *cb, 
					   float x1, float y1, 
					   float x2, float y2, 
					   float x3, float y3)
	{
	svwCtx h = cb->direct_ctx;
	h->path.x = x3;
	h->path.y = y3;

	if (h->err.code != 0)
		return;	/* Pending error */
	else if (h->path.state != 3)
		{
		/* Call sequence error */
		h->err.code = svwErrBadCall;
		return;
		}

	if (h->arg.flags & SVW_ABSOLUTE)
		writeStr(h, " C ");
	else
		writeStr(h, "C");

	writeReal(h, x1);
	writeStr(h, ",");
	writeReal(h, y1);
	writeStr(h, " ");
	writeReal(h, x2);
	writeStr(h, ",");
	writeReal(h, y2);
	writeStr(h, " ");
	writeReal(h, x3);
	writeStr(h, ",");
	writeReal(h, y3);
	
	}

/* Generic operator callback; ignored. */
static void glyphGenop(abfGlyphCallbacks *cb, int cnt, float *args, int op)
	{
	svwCtx h = cb->direct_ctx;

	if (h->err.code != 0)
		return;	/* Pending error */
	else if (h->path.state < 2)
		{
		/* Call sequence error */
		h->err.code = svwErrBadCall;
		return;
		}

	/* Do nothing; ignore generic operator. */
	}

/* Seac operator callback. It is an error to call this when writing SVG
   fonts. Instead, clients should flatten seac operators into compound
   glyphs. */
static void glyphSeac(abfGlyphCallbacks *cb,
					  float adx, float ady, int bchar, int achar)
	{
	svwCtx h = cb->direct_ctx;
	
	if (h->err.code != 0)
		return;	/* Pending error */

	/* Call sequence error; seac glyphs should be expanded by the client. */
	h->err.code = svwErrBadCall;
	}

/* End glyph definition. */
static void glyphEnd(abfGlyphCallbacks *cb)
	{
	svwCtx h = cb->direct_ctx;

	if (h->err.code != 0)
		return;
	else if (h->path.state < 2)
		{
		/* Call sequence error */
		h->err.code = svwErrBadCall;
		return;
		}

	if (h->path.state >= 3)
		{
		/* Close path. */
		if (h->arg.flags & SVW_ABSOLUTE)
			writeStr(h, " Z\"");
		else
			writeStr(h, "z\"");
		}
	writeLine(h, "/>");

	h->path.state = 0;
	}
	
static void glyphCompose(abfGlyphCallbacks *cb,  int cubeLEIndex, float x0, float y0, int numDV, float* ndv)
	{
	svwCtx h = cb->direct_ctx;
	int i = 0;


	if (h->path.state < 3)
		/* First moveto/compose for this glyph implies start of path data. */
		writeStr(h, " d=\"");
	else if (h->path.state == 3)
		{
		/* Close path. */
		if (h->arg.flags & SVW_ABSOLUTE)
			writeStr(h, " Z");
		else
			writeStr(h, " z");
		writeStr(h, " ");
		}

	writeStr(h, "B ");
	writeInt(h, cubeLEIndex);
	writeStr(h, ",");
	writeReal(h, x0);
	writeStr(h, ",");
	writeReal(h, y0);
	while (i < numDV)
		{
		writeStr(h, ",");
		writeInt(h, (long)ndv[i++]);
		}
	h->path.state = 3; /* make sure we write the final "Z". */
	}


static void glyphTransform(abfGlyphCallbacks *cb,  float rotate, float scaleX, float scaleY, float skewX, float skewY)
	{
	svwCtx h = cb->direct_ctx;


	if (h->path.state < 3)
		/* First moveto/compose for this glyph implies start of path data. */
		writeStr(h, " d=\"");
	else if (h->path.state == 3)
		{
		/* Close path. */
		if (h->arg.flags & SVW_ABSOLUTE)
			writeStr(h, " Z");
		else
			writeStr(h, " z");
		writeStr(h, " ");
		}

    /* Rotate is expressed in degrees in absfont.  */
	writeStr(h, "R ");
	writeReal(h, rotate);
	writeStr(h, ",");
	writeReal(h, scaleX);
	writeStr(h, ",");
	writeReal(h, scaleY);
	if ((skewX != 1.0) || (skewY != 1.0))
	{
		writeStr(h, ",");
		writeReal(h, skewX);
		writeStr(h, ",");
		writeReal(h, skewY);
	}
	writeStr(h, " ");
	
	h->path.state = 4; /* we do NOT want the following compose op to write "Z" to close the preceding path. */
	}


/* Public callback set */
const abfGlyphCallbacks svwGlyphCallbacks =
	{
	NULL,
	NULL,
	NULL,
	glyphBeg,
	glyphWidth,
	glyphMove,
	glyphLine,
	glyphCurve,
	NULL,
	NULL,
	glyphGenop,
	glyphSeac,
	glyphEnd,
	NULL,
	NULL,
	glyphCompose,
	glyphTransform,
	};
