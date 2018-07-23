/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

#include "t1write.h"
#include "dynarr.h"
#include "dictops.h"
#include "txops.h"
#include "ctutil.h"
#include "supportexcept.h"

#include <stdarg.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* Make up operator for internal use */
#define t2_cntroff	t2_reservedESC33

#define ARRAY_LEN(a) 	(sizeof(a)/sizeof((a)[0]))

#define HEX_LINE_LENGTH 64	/* ASCII eexec or charstring line length */
#define HEX_LINE_BYTES	(HEX_LINE_LENGTH/2)

#define ENTER_EEXEC	"currentfile eexec "

#define RND2I(v)	((int)floor((v)+0.5))
/* Map 4-bit nibble to hexadecimal character */
static const unsigned char hexmap[] = "0123456789ABCDEF";

/* ---------------------------- Library Context ---------------------------- */

typedef long Offset;			/* 1-4 byte offset */

typedef struct				/* Map glyph name to its standard encoding */
	{
	unsigned char code;
	char *gname;
	} StdMap;

typedef struct				/* Charstring data */
	{
	long offset;			/* Tmp stream offset */
	size_t length;
	} Cstr;

typedef struct				/* Glyph data */
	{
	abfGlyphInfo *info;
	Cstr cstr;
	} Glyph;

typedef struct				/* Stem hint */
	{
	float edge0;
	float edge1;
	short flags;
	} Stem;

typedef struct				/* Stem3 hints */
	{
	int cnt;
	Stem array[3];
	} Stem3;

struct t1wCtx_
	{
	long flags;				/* Control flags */
#define SEEN_CID_KEYED_GLYPH	(1<<0)
#define SEEN_NAME_KEYED_GLYPH	(1<<1)
#define SEEN_FLEX				(1<<2)
#define HINT_PENDING			(1<<3)
#define INIT_HINTS				(1<<4)
#define EEXEC_BEGIN				(1<<5)
#define EEXEC_ENABLED			(1<<6)
#define IN_FLEX					(1<<7)
	abfTopDict *top;		/* Top Dict data */
	dnaDCL(Glyph, glyphs);	/* Glyph data */
	long CIDCount;			/* Computed CIDCount for font */
	struct					/* Client-specified data */
		{
		long flags;
		int lenIV;
		long maxglyphs;
		char *newline;
		} arg;
	struct					/* Temorary stream */
		{					
		long offset;		/* Buffer offset */
		size_t length;		/* Buffer length */
		char *buf;			/* Buffer beginning */
		char *end;			/* Buffer end */
		char *next;			/* Next byte available (buf <= next < end) */
		} tmp;
	struct					/* Destination stream */
		{
		char buf[BUFSIZ];
		size_t cnt;			/* Bytes in buf */
		} dst;
	struct					/* eexec encryption */
		{
		unsigned short r;	/* Encryption state */
		size_t cnt;			/* Number of chars in last line */
		} eexec;
	dnaDCL(char, cstr);		/* Charstring buffer */
	dnaDCL(Stem, cntrs);	/* Counter list */
	Stem3 hstem3;			/* Horizontal stem3 */
	Stem3 vstem3;			/* Vertical stem3 */
	dnaDCL(Stem, stems);	/* Stem list */
	struct					/* Glyph path */
		{
		float x;
		float y;
		int state;
		} path;
	dnaDCL(Cstr, subrs);	/* Subroutines */
	struct					/* Type 1 operand stack */
		{
		int cnt;
		float array[TX_MAX_OP_STACK_CUBE];
		} stack;
	struct					/* Total charstring data sizes */
		{
		long subrs;
		long glyphs;
		} size;
	struct					/* Hexadecimal encoding overflow buffer */
		{
		int cnt;
		char buf[4];
		} overflow;
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
		t1wSINGCallback sing;
		} cb;
	dnaCtx dna;				/* dynarr context */
	struct 
		{
		long lowLESubrIndex; /* the lowest seen LE reference by GSUBR index The entire GSUBR array is played through, and we copy only those from the low LE num on.*/
		long highLEIndex; /* the lowest seen LE reference by the biased LE iindex. The entire GSUBR array is played through, and we copy only those from the low LE num on.*/
		dnaDCL(Offset, subOffsets);	/* Charstring data accumulator */
		dnaDCL(char, cstrs);	/* Charstring data accumulator */
		} cube_gsubrs;
	struct					/* Error handling */
		{
		_Exc_Buf env;
		int code;
		} err;
	};

/* ----------------------------- Error Handling ---------------------------- */

/* Handle fatal error. */
static void fatal(t1wCtx h, int err_code)
	{
	if (h->stm.dbg != NULL)
		{
		/* Write error message to debug stream */
		char *text = t1wErrStr(err_code);
		(void)h->cb.stm.write(&h->cb.stm, h->stm.dbg, strlen(text), text);
		}
  h->err.code = err_code;
	RAISE(&h->err.env, err_code, NULL);
	}

/* --------------------------- Destination Stream -------------------------- */

/* Write to dst stream. */
static void writeDst(t1wCtx h, size_t count, char *ptr)
	{
	if (h->cb.stm.write(&h->cb.stm, h->stm.dst, count, ptr) != count)
		fatal(h, t1wErrDstStream);
	}

/* Hex encrypt buffer and write to dst stream. */
static void hexEncrypt(t1wCtx h, size_t cnt, unsigned char *p, int newline)
	{
	char buf[HEX_LINE_LENGTH + 2/* Newline */];
	char *q = buf;
	while (cnt--)
		{
		unsigned char c = *p++ ^ h->eexec.r>>8;
		h->eexec.r = (c + h->eexec.r)*52845 + 22719;
		*q++ = hexmap[c>>4&0xf];
		*q++ = hexmap[c&0xf];
		}
	if (newline)
		{
		/* Add newline */
		for (p = (unsigned char *)h->arg.newline; *p != '\0'; p++)
			*q++ = *p;
		h->eexec.cnt = 0;
		}
	writeDst(h, q - buf, buf);
	}

/* Flush dst stream buffer. */
static void flushBuf(t1wCtx h)
	{
	if (h->dst.cnt == 0)
		return;	/* Nothing to do */

	if (h->flags & EEXEC_ENABLED)
		{
		/* Encrypt buffered bytes */
		size_t cnt = h->dst.cnt;
		unsigned char *p = (unsigned char *)h->dst.buf;
		if (h->arg.flags & T1W_ENCODE_BINARY)
			{
			/* Binary eexec; encrypt in-place */
			while (cnt--)
				{
				unsigned char c = *p ^ h->eexec.r>>8;
				h->eexec.r = (c + h->eexec.r)*52845 + 22719;
				*p++ = c;
				}
			writeDst(h, h->dst.cnt, h->dst.buf);
			}
		else
			{
			/* Hexadecimal eexec */
			size_t left;
			if (h->flags & EEXEC_BEGIN)
				{
				left = (HEX_LINE_LENGTH - (sizeof(ENTER_EEXEC) - 1))/2;
				h->flags &= ~EEXEC_BEGIN;
				}
			else
				left = (HEX_LINE_LENGTH - h->eexec.cnt)/2;
			while (left <= cnt)
				{
				hexEncrypt(h, left, p, 1);
				p += left;
				cnt -= left;
				left = HEX_LINE_BYTES;
				}
			if (cnt > 0)
				hexEncrypt(h, cnt, p, 0);
			h->eexec.cnt += cnt*2;
			}
		}
	else
		/* Write buffered bytes */
		writeDst(h, h->dst.cnt, h->dst.buf);

	h->dst.cnt = 0;
	}

/* Write to dst stream buffer. */
static void writeBuf(t1wCtx h, size_t cnt, const char *ptr)
	{
	size_t left = sizeof(h->dst.buf) - h->dst.cnt;	/* Bytes left in buffer */
	while (cnt >= left)
		{
		memcpy(&h->dst.buf[h->dst.cnt], ptr, left);
		h->dst.cnt += left;
		flushBuf(h);
		ptr += left;
		cnt -= left;
		left = sizeof(h->dst.buf);
		}
	if (cnt > 0)
		{
		memcpy(&h->dst.buf[h->dst.cnt], ptr, cnt);
		h->dst.cnt += cnt;
		}
	}

/* Write variable length binary integer value to dst stream. */
static void writeInt(t1wCtx h, size_t N, long value)
	{
	char tmp[4];
	switch (N)
		{
	case 4:
		tmp[0] = (char)(value>>24);
	case 3:
		tmp[1] = (char)(value>>16);
	case 2:
		tmp[2] = (char)(value>>8);
	case 1:
		tmp[3] = (char)value;
		}
	writeBuf(h, N, &tmp[4 - N]);
	}

/* Write real number in ASCII to dst stream. */
static void writeReal(t1wCtx h, float value)
{
	char buf[50];
    if (roundf(value) == value)
        sprintf(buf, "%ld", (long)roundf(value));
    else
    {
        ctuDtostr(buf, sizeof(buf), value, 0, 8); /* 8 places is as good as it gets when converting ASCII real numbers->float-> ASCII real numbers, as happens to all the  PrivateDict values.*/

    }
    writeBuf(h, strlen(buf), buf);
}

/* Write null-terminated string to dst steam. */
static void writeStr(t1wCtx h, const char *s)
	{
	writeBuf(h, strlen(s), s);
	}

/* Write null-terminated string followed by newline. */
static void writeLine(t1wCtx h, char *s)
	{
	writeStr(h, s);
	writeStr(h, h->arg.newline);
	}

/* Write formatted data to dst stream. This function must only be called when
   the maximum size of the resulting formatted string is known in advance. It
   must never be called with a string that has been passed into this library
   since it might cause a buffer overrun. Those strings may be handled safely
   by calling writeStr() directly. */
static void CTL_CDECL writeFmt(t1wCtx h, char *fmt, ...)
	{
	char buf[200];
	va_list ap;
	va_start(ap, fmt);
	vsprintf(buf, fmt, ap);
	writeStr(h, buf);
	va_end(ap);
	}

/* -------------------------- Charstring Handling -------------------------- */

/* Fill buffer from tmp stream. */
static void readTmp(t1wCtx h, long offset)
	{
	h->tmp.offset = offset;
	h->tmp.length = h->cb.stm.read(&h->cb.stm, h->stm.tmp, &h->tmp.buf);
	if (h->tmp.length == 0)
		fatal(h, t1wErrTmpStream);
	h->tmp.next = h->tmp.buf;
	h->tmp.end = h->tmp.buf + h->tmp.length;
	}

/* Write data to tmp file. */
static int writeTmp(t1wCtx h, long cnt, unsigned char *ptr)
	{
	return h->cb.stm.write(&h->cb.stm, h->stm.tmp, cnt, (char *)ptr) != 
		(size_t)cnt;
	}

/* Encrypt charstring and write to tmp stream. */
static int saveCstr(t1wCtx h, abfGlyphInfo *info,
					long length, unsigned char *data, Cstr *cstr)
	{
	/* Encrypted priming bytes */
	static const unsigned char primer[] = {0x1c, 0x60, 0xd8, 0xa8};
	long i;
	unsigned char *p;
	unsigned short r = 0; /* Suppress optimizer warning */

	/* Remember charstring details */
	cstr->offset = h->tmp.offset;
	cstr->length = length;

	if (info != NULL && info->flags & ABF_GLYPH_CID &&
		!(h->arg.flags & T1W_TYPE_HOST))
		{
		/* CID-keyed incremental download; write fd index */
		if (writeTmp(h, 1, &info->iFD))
			return 1;
		cstr->length++;
		}

	/* Handle encryption type */
	switch (h->arg.lenIV)
		{
	case -1:
		goto write_cstr;
	case 0:
		r = 4330;
		break;
	case 1:
		if (writeTmp(h, 1, (unsigned char *)primer))
			return 1;
		r = 27725;
		cstr->length += 1;
		break;
	case 4:
		if (writeTmp(h, 4, (unsigned char *)primer))
			return 1;
		r = 17114;
		cstr->length += 4;
		break;
		}
		
	/* Encrypt charstring in place */
	p = data;
	for (i = 0; i < length; i++)
		{
		unsigned char c = *p ^ (r>>8);
		r = (c + r)*52845 + 22719;
		*p++ = c;
		}

	/* Write charstring */
 write_cstr:
	if (writeTmp(h, length, data))
		return 1;

	h->tmp.offset += cstr->length;

	return 0;
	}

/* Read charstring into dynamic charstring buffer. */
static void readCstr(t1wCtx h, Cstr *cstr)
	{
	long left;
	long length;
	char *p;
	long delta = cstr->offset - h->tmp.offset;
	if (delta >= 0 && (size_t)delta < h->tmp.length)
		/* Offset within current buffer; reposition next byte */
		h->tmp.next = h->tmp.buf + delta;
	else 
		{
		/* Offset outside current buffer; seek to offset and fill buffer */
		if (h->cb.stm.seek(&h->cb.stm, h->stm.tmp, cstr->offset))
			fatal(h, t1wErrTmpStream);
		readTmp(h, cstr->offset);
		}

	/* Fill buffer with charstring */
	if (dnaSetCnt(&h->cstr, 1, cstr->length) == -1)
		fatal(h, t1wErrNoMemory);
	p = h->cstr.array;
	length = cstr->length;
	left = h->tmp.end - h->tmp.next;
	while (left < length)
		{
		/* Copy buffer */
		memcpy(p, h->tmp.next, left);
		p += left;
		length -= left;

		/* Refill buffer */
		readTmp(h, h->tmp.offset + h->tmp.length);
		left = h->tmp.length;
		}
	memcpy(p, h->tmp.next, length);
	h->tmp.next += length;
	}

/* Hex encode line and write to dst stream. */
static size_t hexEncode(t1wCtx h, int col, size_t cnt, char *p)
	{
	char line[HEX_LINE_LENGTH + 4];
	char *beg = &line[col];
	char *end = &line[HEX_LINE_LENGTH];
	char *q = beg;

	if (h->overflow.cnt > 0)
		{
		/* Add overflow from previous line; col will be 0 */
		memcpy(line, h->overflow.buf, h->overflow.cnt);
		q += h->overflow.cnt;
		}

	if (h->arg.flags & T1W_ENCODE_ASCII85)
		/* Use ASCII85 filter */
		while (cnt)
			{
			int i;
			unsigned long tuple = 0;
			int n = (cnt > 3)? 4: cnt;

			/* Make 4-tuple */
			switch (n)
				{
			case 4:
				tuple = (*p++&0xff)<<24;
				/* Fall through */
			case 3:
				tuple |= (*p++&0xff)<<16;
				/* Fall through */
			case 2:
				tuple |= (*p++&0xff)<<8;
				/* Fall through */
			case 1:
				tuple |= *p++&0xff;
				break;
				}

			/* Pad partial 4-tuple */
			for (i = n; i < 4; i++)
				tuple <<= 8;
			cnt -= n;

			/* Make whole or partial 5-tuple */
			for (i = 4; i >= 0; i--)
				{
				q[i] = (char)('!' + tuple%85);
				tuple /= 85;
				}
			q += n + 1;
			if (q >= end)
				break;
			}
	else
		/* Use ASCIIHex filter */
		while (cnt)
			{
			int c = *p++;
			cnt--;
			*q++ = hexmap[c>>4&0xf];
			*q++ = hexmap[c&0xf];
			if (q >= end)
				break;
			}

	if (q > end)
		{
		/* Save overflow for next line */
		h->overflow.cnt = q - end;
		memcpy(h->overflow.buf, &line[HEX_LINE_LENGTH], h->overflow.cnt);
		q -= h->overflow.cnt;
		}
	else
		h->overflow.cnt = 0;

	if (cnt > 0 || h->overflow.cnt > 0)
		{
		/* Add newline */
		char *p;
		for (p = h->arg.newline; *p != '\0'; p++)
			*q++ = *p;
		}
	writeBuf(h, q - beg, beg);

	return cnt;
	}

/* Write dictionary charstring. */
static void writeCstr(t1wCtx h, int col, Cstr *cstr, int put)
	{
	long length;
	char *p;
	
	/* Read charstring from tmp stream */
	readCstr(h, cstr);
	length = h->cstr.cnt;
	p = h->cstr.array;

	if (h->arg.flags & (T1W_ENCODE_BINARY|T1W_TYPE_HOST))
		{
		/* Write binary charstring */
		if (h->top->sup.flags & ABF_CID_FONT)
			writeFmt(h, " %ld : ", length);
		else
			writeFmt(h, " %ld -| ", length);
		writeBuf(h, length, p);
		}
	else
		{
		/* Write hex charstring */
		if (col > HEX_LINE_LENGTH - 4)
			{
			/* Identifier too long; start on next line */
			writeStr(h, h->arg.newline);
			col = 0;
			}
		
		if (h->arg.flags & T1W_ENCODE_ASCII85)
			{
			writeStr(h, " <~");
			col += 3;
			}
		else
			{
			writeStr(h, " <");
			col += 2;
			}

		/* Write charstring data */
		while (length > 0 || h->overflow.cnt > 0)
			{
			size_t oldlength = length;
			length = hexEncode(h, col, length, p);
			p += oldlength - length;
			col = 0;
			}
		
		if (h->arg.flags & T1W_ENCODE_ASCII85)
			writeStr(h, "~>");
		else
			writeStr(h, ">");
		}

	if (put)
		writeLine(h, " |");
	else
		writeLine(h, " |-");
	}

/* --------------------------------- Subrs --------------------------------- */

/* Save subr. */
static long saveSubr(t1wCtx h, size_t length, unsigned char *data)
	{
	Cstr *cstr;
	long index = dnaNext(&h->subrs, sizeof(Cstr));
	if (index == -1)
		fatal(h, t1wErrNoMemory);
	cstr = &h->subrs.array[index];
	if (saveCstr(h, NULL, length, data, cstr))
		fatal(h, t1wErrTmpStream);
	h->size.subrs += cstr->length;
	return index;
	}

/* Save standard subr. */
static void saveStdSubr(t1wCtx h, size_t length, const unsigned char *data)
	{
	if (h->arg.lenIV != -1)
		{
		/* Copy to cstr buffer before encrypting */
		if (dnaSetCnt(&h->cstr, 1, length) == -1)
			fatal(h, t1wErrNoMemory);
		memcpy(h->cstr.array, data, length);
		data = (unsigned char *)h->cstr.array;
		}
	(void)saveSubr(h, length, (unsigned char *)data);
	}

/* Save standard subrs to tmp stream. */
static void saveStdSubrs(t1wCtx h)
	{
	static const unsigned char subr0[] =
		/* 3 0 callother pop pop setcurrentpoint return */
		{0x8e, 0x8b, 0xc, 0x10, 0xc, 0x11, 0xc, 0x11, 0xc, 0x21, 0xb};
	static const unsigned char subr1[] =
		/* 0 1 callother return */
		{0x8b, 0x8c, 0xc, 0x10, 0xb};
	static const unsigned char subr2[] =
		/* 0 2 callother return */
		{0x8b, 0x8d, 0xc, 0x10, 0xb};
	static const unsigned char subr3[] =
		/* return */
		{0xb};
	static const unsigned char subr4[] =
		/* 3 1 3 callother pop callsubr return */
		{0x8e, 0x8c, 0x8e, 0xc, 0x10, 0xc, 0x11, 0xa, 0xb};

	if (h->arg.flags & T1W_TYPE_ADDN)
		return;	/* Not needed by incremental download addition fonts */
	
	saveStdSubr(h, sizeof(subr0), subr0);
	saveStdSubr(h, sizeof(subr1), subr1);
	saveStdSubr(h, sizeof(subr2), subr2);
	saveStdSubr(h, sizeof(subr3), subr3);
	saveStdSubr(h, sizeof(subr4), subr4);
	}

/* Save standard subrs to tmp stream. */
static void saveCubeSubrs(t1wCtx h)
	{
	int i;
	long leIndex = -1;  /* index of the first Library Element in the source font gsubrs */
	long numSourceGSubrs = h->cube_gsubrs.subOffsets.cnt;
 	long leLen;
	long leOffset;

 	if ( h->cube_gsubrs.lowLESubrIndex < 0)
		return; /* aren't any to merge. */
		
	leIndex = numSourceGSubrs - (108 + h->cube_gsubrs.highLEIndex);
	if (leIndex > h->cube_gsubrs.lowLESubrIndex)
		leIndex = h->cube_gsubrs.lowLESubrIndex;
	 h->cube_gsubrs.lowLESubrIndex= leIndex; /* We need the final value in writeSubrs */
	
	if (leIndex == 0)
		leOffset = 0;
	else
		leOffset = h->cube_gsubrs.subOffsets.array[leIndex-1];
	leLen = h->cube_gsubrs.cstrs.cnt - leOffset;
	
	{/* h->cube_gsubrs.subOffsets.array[leIndex] contains the offset to the byte after the *end* of 
	subr  leIndex. If leIndex is 0, then the start offset is 0 else it is the offset for the previous le Index. */
	
	long startOffset = leOffset;
	long endOffset = h->cube_gsubrs.subOffsets.array[leIndex];
	unsigned char *data = (unsigned char *)&h->cube_gsubrs.cstrs.array[startOffset];
	size_t length = endOffset - startOffset;
	
	(void)saveSubr(h, length, data);

	i = leIndex+1;
	while (i < numSourceGSubrs)
		{
		startOffset = endOffset;
		endOffset = h->cube_gsubrs.subOffsets.array[i];
		data = (unsigned char *)&h->cube_gsubrs.cstrs.array[startOffset];
		length = endOffset - startOffset;
		(void)saveSubr(h, length, data);
		i++;
		}
	}
  }

/* Write subr at specified index. */
static void writeSubr(t1wCtx h, int index)
	{
	writeFmt(h, "dup %d", index);
	writeCstr(h, 5, &h->subrs.array[index], 1);
	}
		
/* Write standard subrs. */
static void writeSubrs(t1wCtx h)
	{
	if (h->subrs.cnt == 0)
		return;
	if (h->cube_gsubrs.subOffsets.cnt > 0)
		{
		int i;
		writeFmt(h, "/Subrs %d array ", h->subrs.cnt);
		for (i=0; i < h->subrs.cnt; i++)
			{
			writeSubr(h, i);
			}
		}
	else
		{
		writeLine(h, "/Subrs 5 array");
		writeSubr(h, 0);
		writeSubr(h, 1);
		writeSubr(h, 2);
		writeSubr(h, 3);
		writeSubr(h, 4);
		}
	writeLine(h, "def");
	}

/* ---------------------------- Dictionary Keys ---------------------------- */

/* Return 1 if string contains unbalanced parentheses or backslash characters
   which need escaping else return 0. */
static int strNeedsEscape(char *str)
	{
	char *p;
	int state = 0;
	for (p = str; *p != '\0'; p++)
		switch (*p)
			{
		case '(':
			state++;
			break;
		case ')':
			if (--state < 0)
				return 1;
			break;
		case '\\':
			return 1;
			}
	return state != 0;
	}

/* Write string, escaping parenthesis and backslash characters. */
static void writeEscapedStr(t1wCtx h, char *str)
	{
	char *p;
	for (p = str; *p != '\0'; p++)
		switch (*p)
			{
		case '(':
		case ')':
		case '\\':
			writeFmt(h, "\\%c", *p);
			break;
		default:
			writeBuf(h, 1, p);
			}
	}

/* Write PostScript definition of string object. */
static void writeStringDef(t1wCtx h, char *key, char *value)
	{
	if (value == ABF_UNSET_PTR)
		return;
	writeFmt(h, "/%s (", key);
	if (strNeedsEscape(value))
		writeEscapedStr(h, value);
	else
		writeStr(h, value);
	writeLine(h, ") def");
	}

/* Write PostScript definition of literal object. */
static void writeLiteralDef(t1wCtx h, char *key, const char *value)
	{
	if (value == ABF_UNSET_PTR)
		return;
	writeFmt(h, "/%s /", key);
	writeStr(h, value);
	writeLine(h, " def");
	}

/* Write PostScript definition of boolean object. */
static void writeBooleanDef(t1wCtx h, char *key, int value)
	{
	writeFmt(h, "/%s %s def%s", key, value? "true": "false", h->arg.newline);
	}

/* Write PostScript definition of integer object. */
static void writeIntDef(t1wCtx h, char *key, long value)
	{
	if (value == ABF_UNSET_INT)
		return;
	writeFmt(h, "/%s %ld def%s", key, value, h->arg.newline);
	}

/* Write PostScript definition of real object. */
static void writeRealDef(t1wCtx h, char *key, float value)
{
    char buf[50];
	if (value == ABF_UNSET_REAL)
		return;
    
    if (roundf(value) == value)
    {
        sprintf(buf, "%ld", (long)roundf(value));
    }
    else
    {
        ctuDtostr(buf, sizeof(buf), value, 0, 8); /* 8 places is as good as it gets when converting ASCII real numbers->float-> ASCII real numbers, as happens to all the  PrivateDict values.*/
    }
    writeFmt(h, "/%s ", key);
    writeFmt(h, "%s", buf);
    writeLine(h, " def");
}

/* Write PostScript Version of real object. */
static void writeVersion(t1wCtx h, float value)
{
	if (value == ABF_UNSET_REAL)
		return;
    
    writeFmt(h, "%.3f", value);
}

/* Write PostScript Version of real object. */
static void writeVersionDef(t1wCtx h, char *key, float value)
{
	if (value == ABF_UNSET_REAL)
		return;
    
    writeFmt(h, "/%s ", key);
    writeFmt(h, "%.3f", value);
    writeLine(h, " def");
}

/* Write PostScript definition of integer array object. */
static void writeIntArrayDef(t1wCtx h, char *key, long cnt, long *array)
	{
	char *sep;
	if (cnt == ABF_EMPTY_ARRAY)
		return;
	writeFmt(h, "/%s [", key);
	sep = "";
	while (cnt--)
		{
		writeFmt(h, "%s%ld", sep, *array++);
		sep = " ";
		}
	writeLine(h, "] def");
	}

/* Write PostScript definition of real array object. */
static void writeRealArrayDef(t1wCtx h, char *key, long cnt, float *array)
	{
    char* sep;
	if (cnt == ABF_EMPTY_ARRAY)
		return;
	writeFmt(h, "/%s [", key);
    sep = "";
	while (cnt--)
        {
            char buf[50];
            float value = *array++;
            writeStr(h, sep);
            if (roundf(value) == value)
            {
                sprintf(buf, "%ld", (long)roundf(value));
            }
            else
            {
                ctuDtostr(buf, sizeof(buf), value, 0, 8); /* 8 places is as good as it gets when converting ASCII real numbers->float-> ASCII real numbers, as happens to all the  PrivateDict values.*/
            }
            writeFmt(h, "%s", buf);
            sep = " ";
        }
	writeLine(h, "] def");
	}

/* --------------------------- PostScript Output --------------------------- */

/* Return the string to be used as the value of the /OrigFontType key. If no
   key should be output NULL is returned. */
static const char *getOrigFontTypeValue(t1wCtx h)
	{
	switch (h->top->OrigFontType)
		{
	case abfOrigFontTypeType1:
		return "Type1";
	case abfOrigFontTypeCID:
		return "CID";
	case abfOrigFontTypeTrueType:
		return "TrueType";
	case abfOrigFontTypeOCF:
		return "OCF";
		}

	switch (h->top->sup.srcFontType)
		{
	case abfSrcFontTypeType1Name:
	case abfSrcFontTypeCFFName:
		if (h->top->sup.flags & ABF_CID_FONT)
			return "Type1";	/* Name-keyed to cid-keyed conversion */
		else
			break;
            case abfSrcFontTypeType1CID:
            case abfSrcFontTypeCFFCID:
                if (h->top->sup.flags & ABF_CID_FONT)
                    break;
                else
                    return "CID";	/* CID-keyed to name-keyed conversion */
            case abfSrcFontTypeSVGName:
                return "SVG";	
            case abfSrcFontTypeUFOName:
                return "UFO";	
	case abfSrcFontTypeTrueType:
		return "TrueType";	/* TrueType to name- or cid-keyed conversion */
		}

	return NULL;
	}

/* Write name-keyed FontInfo dict. */
static void writeFontInfoDict(t1wCtx h, abfTopDict *top)
	{
	const char *OrigFontType = NULL;
	int WasEmbedded = 0;
	int host = (h->arg.flags & T1W_TYPE_HOST) != 0;
	int size =
		(3 /* Headroom for new key addition */ +
		 (top->Notice.ptr != ABF_UNSET_PTR) +
		 (top->Weight.ptr != ABF_UNSET_PTR) +
		 (top->ItalicAngle != cff_DFLT_ItalicAngle) /* ItalicAngle */ +
		 (top->FSType != ABF_UNSET_INT));

	if (host)
		size +=
			((top->version.ptr != ABF_UNSET_PTR) +
			 (top->Copyright.ptr != ABF_UNSET_PTR) +
			 (top->FullName.ptr != ABF_UNSET_PTR) +
			 (top->FamilyName.ptr != ABF_UNSET_PTR) +
			 (top->isFixedPitch != cff_DFLT_isFixedPitch) /* isFixedPitch */ +
			 (top->UnderlinePosition != cff_DFLT_UnderlinePosition) /* UnderlinePosition */ +
			 (top->UnderlineThickness != cff_DFLT_UnderlineThickness) /* UnderlineThickness */);
	else
		{
		if (top->FSType == ABF_UNSET_INT)
			{
			OrigFontType = getOrigFontTypeValue(h);
			WasEmbedded = top->WasEmbedded;
			}

		size +=
			((top->BaseFontName.ptr != ABF_UNSET_PTR) +
			 (top->BaseFontBlend.cnt != ABF_EMPTY_ARRAY) +
			 (OrigFontType != NULL) +
			 (WasEmbedded != 0) +
			 ((top->sup.flags & ABF_SING_FONT)? 2: 0));
		}

	writeFmt(h, "/FontInfo %d dict dup begin%s", size, h->arg.newline);
	if (host)
		writeStringDef(h, "version", top->version.ptr);
    if (top->Notice.ptr != ABF_UNSET_PTR)
        writeStringDef(h, "Notice", top->Notice.ptr);
	if (host)
		{
		if (top->Copyright.ptr != ABF_UNSET_PTR)
            writeStringDef(h, "Copyright", top->Copyright.ptr);
        if (top->FullName.ptr != ABF_UNSET_PTR)
            writeStringDef(h, "FullName", top->FullName.ptr);
        if (top->FamilyName.ptr != ABF_UNSET_PTR)
            writeStringDef(h, "FamilyName", top->FamilyName.ptr);
		}
	if (top->Weight.ptr != ABF_UNSET_PTR)
        writeStringDef(h, "Weight", top->Weight.ptr);
    if (top->ItalicAngle != cff_DFLT_ItalicAngle)
        writeRealDef(h, "ItalicAngle", top->ItalicAngle);
	if (host)
		{
        if (top->isFixedPitch != cff_DFLT_isFixedPitch)
            writeBooleanDef(h, "isFixedPitch", top->isFixedPitch);
        if (top->UnderlinePosition != cff_DFLT_UnderlinePosition)
            writeRealDef(h, "UnderlinePosition", top->UnderlinePosition);
        if (top->UnderlineThickness != cff_DFLT_UnderlineThickness)
            writeRealDef(h, "UnderlineThickness", top->UnderlineThickness);
		}
	else
		{
		writeStringDef(h, "BaseFontName", top->BaseFontName.ptr);
		writeIntArrayDef(h, "BaseFontBlend", 
						 top->BaseFontBlend.cnt, top->BaseFontBlend.array);
		writeLiteralDef(h, "OrigFontType", OrigFontType);
		if (WasEmbedded)
			writeBooleanDef(h, "WasEmbedded", 1);
		if (top->sup.flags & ABF_SING_FONT)
			{
			long i;
			writeBooleanDef(h, "isSINGglyphlet", 1);
			writeFmt(h, "/NameToGID %ld dict dup begin%s", 
					 h->glyphs.cnt - 1, h->arg.newline);
			for (i = 1; i < h->glyphs.cnt; i++)
				writeFmt(h, "/%s %ld def%s", 
						 h->glyphs.array[i].info->gname.ptr, i, 
						 h->arg.newline);
			writeLine(h, "end def");
			}
		}
	if ((top->FSType != ABF_UNSET_INT))
        writeIntDef(h, "FSType", top->FSType);
	writeLine(h, "end def");
	}

/* Write FontMatrix. */
static void writeFontMatrix(t1wCtx h, abfFontMatrix *matrix)
	{
	if (matrix->cnt == ABF_EMPTY_ARRAY)
		writeLine(h, "/FontMatrix [0.001 0 0 0.001 0 0] def");
	else
		writeRealArrayDef(h, "FontMatrix", matrix->cnt, matrix->array);
	}

/* Match std name. */
static int CTL_CDECL matchStd(const void *key, const void *value)
	{
	return strcmp((char *)key, ((StdMap *)value)->gname);
 	}

/* Get standard encoding from glyph name. Return -1 if no standard encoding
   else code. */
static int getStdEnc(t1wCtx h, char *gname)
	{
	static StdMap stdenc[] =
		{
#include "stdenc3.h"
		};
	StdMap *map = (StdMap *)
		bsearch(gname, stdenc, ARRAY_LEN(stdenc), sizeof(StdMap), matchStd);
	return (map == NULL)? -1: map->code;
	}

/* Write host/base Encoding. */
static void writeEncoding(t1wCtx h)
	{
	char *custom[256];
	long i;
	int useStdEnc;	/* Flags if standard encoding can be used */

	/* Make empty encoding */
	for (i = 0; i < 256; i++)
		custom[i] = NULL;

	/* Build encoding */
	useStdEnc = 1;
	for (i = 0; i < h->glyphs.cnt; i++)
		{
		abfGlyphInfo *info = h->glyphs.array[i].info;
		abfEncoding *enc = &info->encoding;
		if (enc->code == ABF_GLYPH_UNENC ||
			info->flags & ABF_GLYPH_UNICODE)
			{
			/* Unencoded glyph; check it's not in standard encoding */
			if (useStdEnc && getStdEnc(h, info->gname.ptr) != -1)
				useStdEnc = 0;
			}
		else
			/* Encoded glyph */
			do
				{
				/* Add to custom encoding */
				int code = enc->code&0xff;
				/* If this is the .notdef glyph then don't
				 * overwrite the custom encoded value.
				 */
				if (strcmp(info->gname.ptr, ".notdef") == 0)
				{
					enc = enc->next;
					continue;
				}
				if (custom[code] != NULL)
					fatal(h, t1wErrDupEnc);
				custom[code] = info->gname.ptr;
				enc = enc->next;

				/* Check in standard encoding at same code point */
				if (useStdEnc && getStdEnc(h, info->gname.ptr) != code)
					useStdEnc = 0;
				}
			while (enc != NULL);
		}

	if ( (h->arg.flags & T1W_FORCE_STD_ENCODING) || (useStdEnc && h->arg.flags & T1W_TYPE_HOST))
		/* Write standard encoding */
		writeLine(h, "/Encoding StandardEncoding def");
	else
		{
		/* Write custom encoding */
		writeLine(h, "/Encoding 256 array");
		writeLine(h, "0 1 255 {1 index exch /.notdef put} for");
		for (i = 0; i < 256; i++)
			if (custom[i] != NULL)
				{
				writeFmt(h, "dup %ld /", i);
				writeStr(h, custom[i]);
				writeLine(h, " put");
				}
		writeLine(h, "def");
		}
	}

/* Write text array with newline translation. */
static void writeTextArray(t1wCtx h, int cnt, char *array[])
	{
	int i;
	for (i = 0; i < cnt; i++)
		{
		char *beg = array[i];
		do
			{
			char *end = strchr(beg, '\n');
			writeBuf(h, end - beg, beg);
			writeStr(h, h->arg.newline);
			beg = end + 1;
			}
		while (*beg != '\0');
		}
	}

/* Write OtherSubrs array */
static void writeOtherSubrs(t1wCtx h, abfPrivateDict *private)
	{
	static char *hintothers[] =
		{
#include "t1write_hintothers.h"
		};
	static char *flexothers[] =
		{
#include "t1write_flexothers.h"
		};
	static char *gcothers[] =
		{
#include "t1write_gcothers.h"
		};
	static char *procsetothers[] =
		{
#include "t1write_procsetothers.h"
		};

	if (h->arg.flags & T1W_OTHERSUBRS_PRIVATE)
		{
		/* Private OtherSubrs */
		if (h->top->sup.flags & ABF_CID_FONT)
			{
			/* CID-keyed */
			if (private->LanguageGroup == 1)
				writeTextArray(h, ARRAY_LEN(gcothers), gcothers);
			else
				writeTextArray(h, ARRAY_LEN(hintothers), hintothers);
			}
		else
			{
			/* Name-keyed */
			if (h->flags & SEEN_FLEX && (h->arg.flags & T1W_TYPE_HOST))
				writeTextArray(h, ARRAY_LEN(flexothers), flexothers);
			else
				writeTextArray(h, ARRAY_LEN(hintothers), hintothers);
			}
		}
	else
		/* Procset OtherSubrs */
		writeTextArray(h, ARRAY_LEN(procsetothers), procsetothers);
	}

/* Write Private dict. */
static void writePrivateDict(t1wCtx h, abfPrivateDict *private, long SDBytes)
	{
	int cid = (h->top->sup.flags & ABF_CID_FONT) != 0;
	int size =
		(1 /* BlueValues */ +	
		 (private->OtherBlues.cnt != ABF_EMPTY_ARRAY) +
		 (private->FamilyBlues.cnt != ABF_EMPTY_ARRAY) +
		 (private->FamilyOtherBlues.cnt != ABF_EMPTY_ARRAY) +
		 (private->BlueScale != (float)cff_DFLT_BlueScale) +
		 (private->BlueShift != cff_DFLT_BlueShift) +
		 (private->BlueFuzz != cff_DFLT_BlueFuzz) +
		 (private->StdHW != ABF_UNSET_REAL) +
		 (private->StdVW != ABF_UNSET_REAL) +
		 (private->StemSnapH.cnt != ABF_EMPTY_ARRAY) +
		 (private->StemSnapV.cnt != ABF_EMPTY_ARRAY) +
		 (private->ForceBold != cff_DFLT_ForceBold) +
		 (private->LanguageGroup != cff_DFLT_LanguageGroup)*2 +
		 (private->ExpansionFactor != (float)cff_DFLT_ExpansionFactor) +
		 (private->initialRandomSeed != (float)cff_DFLT_initialRandomSeed)+
		 1 /* password */ +
		 (h->arg.lenIV != 4) +
		 1 /* MinFeature */ +
		 1 /* OtherSubrs */);

	if (cid)
		size +=
			(1 /* size of SubrMapOffset */ +
			 1 /* size of SDBytes */ +
			 1 /* size of SubrCount */);
	else if (h->arg.flags & (T1W_ENCODE_BINARY|T1W_TYPE_HOST))
		size +=
			(3 /* -|, |-, | */ +
			 (h->subrs.cnt > 0));
	else
		size +=
			(2 /* |-, | */ +
			 (h->subrs.cnt > 0));

	/* Begin Private dict */
	if (cid)
		writeFmt(h, "/Private %d dict dup begin%s", size, h->arg.newline);
	else
		{
		writeLine(h, "dup /Private");
		writeFmt(h, "%d dict dup begin%s", size, h->arg.newline);

		/* Write dict keys */
		if (h->arg.flags & (T1W_ENCODE_BINARY|T1W_TYPE_HOST))
			writeLine(h, "/-| {string currentfile exch readstring pop} def");
		writeLine(h, "/|- {def} def");
		writeLine(h, "/| {put} def");
		}
	if (private->BlueValues.cnt == ABF_EMPTY_ARRAY)
		/* According to the Black Book the BlueValues array is required but it
		   may be empty. However, some level 2 printers will reject the font if
		   presented with and empty BlueValues array so what follows must serve
		   as a representation of empty instead. */
		writeLine(h, "/BlueValues [0 0] def");
	else
		writeRealArrayDef(h, "BlueValues", private->BlueValues.cnt, 
						  private->BlueValues.array);
	writeRealArrayDef(h, "OtherBlues", private->OtherBlues.cnt, 
					  private->OtherBlues.array);
	writeRealArrayDef(h, "FamilyBlues", private->FamilyBlues.cnt, 
					  private->FamilyBlues.array);
	writeRealArrayDef(h, "FamilyOtherBlues", private->FamilyOtherBlues.cnt, 
					  private->FamilyOtherBlues.array);
	if (private->BlueScale != (float)cff_DFLT_BlueScale)
		writeRealDef(h, "BlueScale", private->BlueScale);
	if (private->BlueShift != cff_DFLT_BlueShift)
		writeRealDef(h, "BlueShift", private->BlueShift);
	if (private->BlueFuzz != cff_DFLT_BlueFuzz)
		writeRealDef(h, "BlueFuzz", private->BlueFuzz);
	if (private->StdHW != ABF_UNSET_REAL)
		writeRealArrayDef(h, "StdHW", 1, &private->StdHW);
	if (private->StdVW != ABF_UNSET_REAL)
		writeRealArrayDef(h, "StdVW", 1, &private->StdVW);
	writeRealArrayDef(h, "StemSnapH", 
					  private->StemSnapH.cnt, private->StemSnapH.array);
	writeRealArrayDef(h, "StemSnapV", 
					  private->StemSnapV.cnt, private->StemSnapV.array);
	if (private->ForceBold != cff_DFLT_ForceBold)
		writeBooleanDef(h, "ForceBold", private->ForceBold);
	if (private->LanguageGroup != cff_DFLT_LanguageGroup)
		{
		writeIntDef(h, "LanguageGroup", private->LanguageGroup);
		writeLine(h, "/RndStemUp false def");
		}
	if (private->ExpansionFactor != (float)cff_DFLT_ExpansionFactor)
		writeRealDef(h, "ExpansionFactor", private->ExpansionFactor);
	if (private->initialRandomSeed != (float)cff_DFLT_initialRandomSeed)
		writeRealDef(h, "initialRandomSeed", private->initialRandomSeed);
	writeLine(h, "/password 5839 def");
	switch (h->arg.lenIV)
		{
	case -1:
		writeLine(h, "/lenIV -1 def");
		break;
	case 0: case 1:
		writeIntDef(h, "lenIV", h->arg.lenIV);
		break;
		}
	writeLine(h, "/MinFeature {16 16} def");
	writeOtherSubrs(h, private);
	if (cid)
		{
		writeIntDef(h, "SubrMapOffset", 0);
		writeIntDef(h, "SDBytes", SDBytes);
		if (h->cube_gsubrs.subOffsets.cnt  > 0)
			writeIntDef(h, "SubrCount", h->subrs.cnt);
		else
			writeIntDef(h, "SubrCount", 5);
		writeLine(h, "end def");
		}
	else
		{
		writeSubrs(h);
		writeLine(h, "put");
		}
	}

/* Write comment identifying library version and copyright notice if 
   applicable. */
static void writeIdentComment(t1wCtx h)
	{
	enum { MAX_VERSION_SIZE = 100 };
	char version_buf[MAX_VERSION_SIZE+1];
	writeFmt(h, "%%ADOt1write: (%s)%s",
			 CTL_SPLIT_VERSION(version_buf, T1W_VERSION), h->arg.newline);
	if ((h->top->Notice.ptr == ABF_UNSET_PTR ||
		 strstr(h->top->Notice.ptr, "Adobe") == NULL) &&
		(h->top->Copyright.ptr == ABF_UNSET_PTR ||
		 strstr(h->top->Copyright.ptr, "Adobe") == NULL))
		{
		/* Non-Adobe font; add Adobe copyright to derivative work */
		time_t now = time(NULL);
		writeFmt(h, "%%%%Copyright: Copyright %d Adobe System Incorporated. "
				 "All rights reserved.%s", 
				 localtime(&now)->tm_year + 1900, h->arg.newline);
		}
	}

/* Write FontBBox. */
static void writeFontBBox(t1wCtx h, float *FontBBox)
	{
	writeStr(h, "/FontBBox {");
	writeReal(h, roundf(FontBBox[0]));
	writeStr(h, " ");
	writeReal(h, roundf(FontBBox[1]));
	writeStr(h, " ");
	writeReal(h, roundf(FontBBox[2]));
	writeStr(h, " ");
	writeReal(h, roundf(FontBBox[3]));
	writeLine(h, "} def");
	}

/* Write XUID. */
static void writeXUID(t1wCtx h)
	{
	if (h->top->XUID.cnt == ABF_EMPTY_ARRAY)
		return;

	writeIntArrayDef(h, "XUID", h->top->XUID.cnt, h->top->XUID.array);

	if (h->arg.flags & T1W_TYPE_HOST)
		return;

	/* Undefine /XUID on level-2 interpreters prior to version 3011 */
	writeLine(h, "systemdict /languagelevel known {mark {systemdict");
	writeLine(h, "/version get exec cvi 3011 le {currentdict /XUID");
	writeLine(h, "undef } if} stopped cleartomark} if");
	}

/* --------------------------- Name-keyed Fonts  --------------------------- */

/* Write name-keyed glyphs. */
static void writeNameKeyedGlyphs(t1wCtx h)
	{
	long i;
	for (i = 0; i < h->glyphs.cnt; i++)
		{
		Glyph *glyph = &h->glyphs.array[i];
		writeStr(h, "/");
		writeStr(h, glyph->info->gname.ptr);
		writeCstr(h, 1 + strlen(glyph->info->gname.ptr), &glyph->cstr, 0);
		}
	}

/* Write host type CharStrings dict. */
static void writeHostCharStrings(t1wCtx h)
	{
	writeLine(h, "dup /CharStrings");
	writeFmt(h, "%ld dict dup begin%s", (h->arg.flags & T1W_TYPE_HOST)? 
			 h->glyphs.cnt: h->arg.maxglyphs, h->arg.newline);
	writeNameKeyedGlyphs(h);
	writeLine(h, "end put");
	}

/* Sequential read on SING glyphlet stream. */
static size_t singRead(t1wCtx h, char **ptr)
	{
	size_t length = h->cb.stm.read(&h->cb.stm, h->cb.sing.stm, ptr);
	if (length == 0)
		fatal(h, t1wErrSINGStream);
	return length;
	}

/* Write SING glyphlet to sfnts array. */
static void writeSINGGlyphlet(t1wCtx h)
{
	size_t total;

	/* Prime stream */
	if (h->cb.sing.get_stream(&h->cb.sing))
		fatal(h, t1wErrSINGStream);
	total = h->cb.sing.length;

	if (total > 65535)
		/* The glyphlet must be able to fit in a single PostScript string
		   because there is no meaningful way to break CFF table data. */
		fatal(h, t1wErrSINGStream);

	writeLine(h, "/sfnts [");
	if (h->arg.flags & T1W_ENCODE_BINARY)
		{
		writeFmt(h, "%lu -| ", total);
		while (total > 0)
			{
			char *ptr;
			size_t cnt = singRead(h, &ptr);
			if (cnt > total)
				cnt = total;
			writeBuf(h, cnt, ptr);
			total -= cnt;
			}
		writeStr(h, h->arg.newline);
		}
	else
		{
		writeStr(h, "<");
		while (total > 0)
			{
			char *ptr;
			size_t cnt = singRead(h, &ptr);

			if (cnt > total)
				cnt = total;
			total -= cnt;

			while (cnt > 0 || h->overflow.cnt > 0)
				{
				/* Write whole lines */
				size_t oldcnt = cnt;
				cnt = hexEncode(h, 0, cnt, ptr);
				ptr += oldcnt - cnt;
				}
			}
		writeLine(h, ">");
		}
	writeLine(h, "] def");
}

/* Write regular kind of name-keyed font. */
static void writeRegNameKeyedFont(t1wCtx h)
	{
	int size;
	int host = (h->arg.flags & T1W_TYPE_HOST) != 0;
	abfTopDict *top = h->top;
	abfFontDict *font = &top->FDArray.array[0];

	/* Write header comments */
	if (host)
		{
		writeStr(h, "%!FontType1-1.1: ");
		writeStr(h, font->FontName.ptr);
		if (top->version.ptr != ABF_UNSET_PTR)
			{
			writeStr(h, " ");
			writeStr(h, top->version.ptr);
			}
		writeStr(h, h->arg.newline);
		}
	writeIdentComment(h);
	if (host)
		{
		writeStr(h, "%%BeginResource: font ");
		writeStr(h, font->FontName.ptr);
		writeStr(h, h->arg.newline);		
		}

	/* Calculate dict size */
	size =
		(3 /* Headroom for new key addition */ +
		 1 /* FontType */ +
		 (font->FontName.ptr != ABF_UNSET_PTR) +
		 1 /* FontInfo */ +
		 1 /* PaintType */ +
		 1 /* FontMatrix */ +
		 1 /* Encoding */ +
		 (top->UniqueID != ABF_UNSET_INT) +
		 1 /* FontBBox */ +
		 (top->StrokeWidth != cff_DFLT_StrokeWidth) +
		 (top->XUID.cnt != ABF_EMPTY_ARRAY) +
		 1 /* Private */ +
		 1 /* CharStings */ +
		 ((top->sup.flags & ABF_SING_FONT) != 0));

	/* Write dict keys */
	writeFmt(h, "%d dict dup begin%s", size, h->arg.newline);
	writeIntDef(h, "FontType", 1);
	writeLiteralDef(h, "FontName", font->FontName.ptr);
	writeFontInfoDict(h, top);
	writeIntDef(h, "PaintType", font->PaintType);
	writeFontMatrix(h, &font->FontMatrix);
	writeEncoding(h);
	writeIntDef(h, "UniqueID", top->UniqueID);
	writeFontBBox(h, top->FontBBox);
    if (top->StrokeWidth != cff_DFLT_StrokeWidth)
		writeRealDef(h, "StrokeWidth", top->StrokeWidth);
	writeXUID(h);
	writeLine(h, "end");
	if (host)
		{
		/* Enter eexec */
		writeStr(h, ENTER_EEXEC);
		flushBuf(h);
		h->eexec.r = 55665;
		h->eexec.cnt = 0;
		h->flags |= (EEXEC_BEGIN|EEXEC_ENABLED);
		writeBuf(h, 4, "cccc");
		}
	else
		writeLine(h, "systemdict begin");
	writePrivateDict(h, &font->Private, 0);
	writeHostCharStrings(h);
	if (top->sup.flags & ABF_SING_FONT)
		writeSINGGlyphlet(h);
	writeLine(h, "end");
	writeLine(h, "dup /FontName get exch definefont pop");
	if (host)
		{
		long i;

		/* Exit eexec */
		writeLine(h, "mark");
		writeLine(h, "currentfile closefile");
		flushBuf(h);
		h->flags &= ~EEXEC_ENABLED;
		writeStr(h, h->arg.newline);
		for (i = 0; i < 8; i++)
			writeLine(h, 
					  "00000000000000000000000000000000"
					  "00000000000000000000000000000000");
		writeLine(h, "cleartomark");
		writeLine(h, "%%EndResource");
		writeLine(h, "%%EOF");
		}
	else
		writeLine(h, "end");
	}

/* Write incremental addition Encoding. */
static void writeAddnEncoding(t1wCtx h, char *FontName)
	{
	long i;
	int seenEnc = 0;
	for (i = 0; i < h->glyphs.cnt; i++)
		{
		abfGlyphInfo *info = h->glyphs.array[i].info;
		abfEncoding *enc = &info->encoding;
		if (enc->code != ABF_GLYPH_UNENC && !(info->flags & ABF_GLYPH_UNICODE))
			/* Encoded glyph */
 			do
				{
				if (!seenEnc)
					{
					/* Fetch encoding */
					if (h->arg.flags & T1W_DONT_LOOKUP_BASE)
						writeStr(h, FontName);
					else
					{
						writeStr(h, "/");
						writeStr(h, FontName);
						writeStr(h, " findfont");
					}
					writeLine(h, " /Encoding get");
					seenEnc = 1;
					}

				/* Add encoding */
				writeFmt(h, "dup %d /", enc->code&0xff);
				writeStr(h, info->gname.ptr);
				writeLine(h, " put");

				enc = enc->next;
				}
			while (enc != NULL);
		}
	if (seenEnc)
		writeLine(h, "pop");
	}

/* Write incremental additon name-keyed font. */
static void writeAddnNameKeyedFont(t1wCtx h)
	{
	char *FontName = h->top->FDArray.array[0].FontName.ptr;
	writeIdentComment(h);
	writeLine(h, "systemdict begin");
	if (h->arg.flags & T1W_DONT_LOOKUP_BASE)
		writeStr(h, FontName);
	else
	{
		writeStr(h, "/");
		writeStr(h, FontName);
		writeStr(h, " findfont");
	}
	writeLine(h, " dup");
	writeLine(h, "/Private get dup rcheck");
	writeLine(h, "{begin true}{pop false}ifelse exch");
	writeLine(h, "/CharStrings get begin");
	writeLine(h, "systemdict /gcheck known {currentglobal "
			  "currentdict gcheck setglobal} if");
	if (h->glyphs.cnt == 0)
		fatal(h, t1wErrNoGlyphs);
	writeNameKeyedGlyphs(h);
	writeLine(h, "systemdict /gcheck known {setglobal} if end {end} if");
	writeLine(h, "end");
	writeAddnEncoding(h, FontName);
	}

/* Write name-keyed font. */
static void writeNameKeyedFont(t1wCtx h)
	{
	if (h->flags & SEEN_CID_KEYED_GLYPH)
		fatal(h, t1wErrGlyphType);

	/* Validate name-keyed data */
	if (h->top->FDArray.cnt != 1 ||
		h->top->FDArray.array[0].FontName.ptr == ABF_UNSET_PTR)
		fatal(h, t1wErrBadDict);

	switch (h->arg.flags & T1W_TYPE_MASK)
		{
	case T1W_TYPE_HOST:
	case T1W_TYPE_BASE:
		writeRegNameKeyedFont(h);
		break;
	case T1W_TYPE_ADDN:
		writeAddnNameKeyedFont(h);
		break;
		}
	}

/* ---------------------------- CID-Keyed Fonts ---------------------------- */

/* Calculate map offset size. */
static long MapOffSize(long size)
	{
	if (size == 0)
		return 0;
	else if (size < 1<<8)
		return 1;
	else if (size < 1<<16)
		return 2;
	else if (size < 1<<24)
		return 3;
	else
		return 4;
	}

/* Calculate map size. */
static long MapSize(long FDBytes, long *MapBytes, 
					long map_cnt, long cstr_cnt, long cstr_size)
	{
	long lastsize = 0;
	*MapBytes = 1;
	for (;;)
		{
		long size = ((map_cnt + 1)*(FDBytes + *MapBytes) +/* map */
					 cstr_size);						  /* charstring data */
		*MapBytes = MapOffSize(size);
		if (size == lastsize)
			break;
		lastsize = size;
		}
	return lastsize;
	}

/* Compute StartData resource size. */
static long prepStartData(t1wCtx h, long *SDBytes, 
						  long *FDBytes, long *GDBytes, long *CIDMapOffset)
	{
	long subrSize;
	long startDataSize;
	*FDBytes = 1;

	/* Compute subrs size */
	subrSize = MapSize(0, SDBytes, h->subrs.cnt, h->subrs.cnt, h->size.subrs);

	/* if writing Std Subrs, they precede the glyph charstrings.
	If writing cube subrs, they follow the glyph charstrings.
	*/
	if  (h->cube_gsubrs.subOffsets.cnt > 0) /* if have cube LE subrs */
		*CIDMapOffset = 0; /* glyphs are at start, subrs follow. */
	else
		*CIDMapOffset = subrSize; /* glyphs follow subrs - need to skip over subrs to get to glyphs. */

	if (!(h->arg.flags & T1W_TYPE_HOST))
		{
		*GDBytes = 1;
		return subrSize;
		}

	/* Compute startData size */
	startDataSize = MapSize(*FDBytes, GDBytes, h->CIDCount, h->glyphs.cnt, subrSize + h->size.glyphs);

	return startDataSize;
	}

/* Write FDArray. */
static void writeFDArray(t1wCtx h, long FDCnt, unsigned char *FDMap, long SDBytes)
	{
	long i;
	long j;
	int host = (h->arg.flags & T1W_TYPE_HOST) != 0;

	j = 0;
	writeFmt(h, "/FDArray %ld array%s", 
			 host? FDCnt: h->top->FDArray.cnt, h->arg.newline);
	for (i = 0; i < h->top->FDArray.cnt; i++)
		if (!host || FDMap[i])
			{
			abfFontDict *font = &h->top->FDArray.array[i];
			int size = (1 /* FontType */ +
						1 /* FontMatrix */ +
						1 /* PaintType */ +
						1 /* Private */);
			if (host || h->arg.flags & T1W_ENABLE_CMP_TEST)
				size += (font->FontName.ptr != ABF_UNSET_PTR);

			/* Write font dict */
			writeFmt(h, "dup %ld%s", j, h->arg.newline);
			writeLine(h, "%ADOBeginFontDict");
			writeFmt(h, "%d dict dup begin%s", size, h->arg.newline);
			writeIntDef(h, "FontType", 1);
			if (host || h->arg.flags & T1W_ENABLE_CMP_TEST)
				writeLiteralDef(h, "FontName", font->FontName.ptr);
			writeIntDef(h, "PaintType", font->PaintType);
			writeFontMatrix(h, &font->FontMatrix);
		
			/* Write Private dict */
			writeLine(h, "%ADOBeginPrivateDict");
			writePrivateDict(h, &font->Private, SDBytes);
			writeLine(h, "%ADOEndPrivateDict");
		
			writeLine(h, "end put");
			writeLine(h, "%ADOEndFontDict");

			if (host)
				FDMap[i] = (unsigned char)j; /* Remap dictionary for CIDMap */
			j++;
			}
	writeLine(h, "def");
	}

/* Write glyph directory. */
static void writeGlyphDirectory(t1wCtx h)
	{
	long i;

	/* Write charstrings */
	for (i = 0; i < h->glyphs.cnt; i++)
		{
		char buf[50];
		Glyph *glyph = &h->glyphs.array[i];
		sprintf(buf, "%hu", glyph->info->cid);
		writeStr(h, buf);
		writeCstr(h, strlen(buf), &glyph->cstr, 1);
		}
	
	/* Write terminator */
	writeLine(h, "!");
	}

/* Write StartData resource. Because hex StartData isn't supported by the CSL
   we never use that format even if the client specified ASCII encoding. */
static void writeStartData(t1wCtx h, unsigned char *FDMap, long StartDataSize, 
						   long SDBytes, long FDBytes, long GDBytes)
	{
	long i;
	long cid;
	long offset;
	char buf[100];
	Glyph *glyph;

	if (h->arg.flags & T1W_TYPE_BASE && !(h->arg.flags & T1W_ENCODE_BINARY))
		{
		/* Call procset that adds standard subrs. WARNING: This procset
		   hardcodes and emits the standard 5 subroutines. If these subroutines
		   need to be modified or additions need to be made then this procset
		   must be changed. */
		writeLine(h, "ct_AddStdCIDMap");
		return;
		}

	/* Write header */
	sprintf(buf, "(Binary) %ld StartData ", StartDataSize);
	writeFmt(h, "%%%%BeginData: %ld Binary Bytes%s",
			 strlen(buf) + StartDataSize + strlen(h->arg.newline),
			 h->arg.newline);
	writeStr(h, buf);

	/* Write SubrMap */
	offset = (h->subrs.cnt + 1)*SDBytes;
	for (i = 0; i < h->subrs.cnt; i++)
		{
		writeInt(h, SDBytes, offset);
		offset += h->subrs.array[i].length;
		}
	/* Write sentinel */
	writeInt(h, SDBytes, offset);
	
	/* Write subr data */
	for (i = 0; i < h->subrs.cnt; i++)
		{
		readCstr(h, &h->subrs.array[i]);
		writeBuf(h, h->cstr.cnt, h->cstr.array);
		}

	if (h->arg.flags & T1W_TYPE_BASE)
		goto trailer;

	/* Write CIDMap */
	glyph = h->glyphs.array;
	offset += (h->CIDCount + 1)*(FDBytes + GDBytes);
	for (cid = 0; cid < h->CIDCount; cid++)
		if (glyph->info->cid == cid)
			{
			/* Write interval */
			writeInt(h, FDBytes, FDMap[glyph->info->iFD]);
			writeInt(h, GDBytes, offset);
			offset += glyph->cstr.length;
			glyph++;
			}
		else
			{
			/* Empty interval */
			writeInt(h, FDBytes, 0);
			writeInt(h, GDBytes, offset);
			}
	/* Write sentinel */
	writeInt(h, FDBytes, 0);
	writeInt(h, GDBytes, offset);

	/* Write glyph data */
	for (i = 0; i < h->glyphs.cnt; i++)
		{
		readCstr(h, &h->glyphs.array[i].cstr);
		writeBuf(h, h->cstr.cnt, h->cstr.array);
		}

	/* Write trailer */
 trailer:
	writeStr(h, h->arg.newline);
	writeLine(h, "%%EndData");
	}

/* Compare cids. */
static int CTL_CDECL cmpCIDs(const void *first, const void *second)
	{
	unsigned short a = ((Glyph *)first)->info->cid;
	unsigned short b = ((Glyph *)second)->info->cid;
	if (a < b)
		return -1;
	else if (a > b)
		return 1;
	else
		return 0;
	}

/* Write host CID-keyed font. */
static void writeHostCIDKeyedFont(t1wCtx h)
	{
	unsigned char FDMap[256];
	long i;
	long SDBytes;
	long FDBytes;
	long GDBytes;
	long CIDMapOffset;
	long StartDataSize;
	int fi_size;
	int size;
	const char *OrigFontType = NULL;
	int WasEmbedded = 0;
	long FDCnt = 0;	/* Suppress optimizer warning */
	int host = (h->arg.flags & T1W_TYPE_HOST) != 0;
	abfTopDict *top = h->top;

	if (host)
		h->CIDCount++;	/* Convert max CID to CIDCount */
	else
		h->CIDCount = top->cid.CIDCount;	/* Set CIDCount from client */

	/* Compute StartData size */
	StartDataSize = 
		prepStartData(h, &SDBytes, &FDBytes, &GDBytes, &CIDMapOffset);

	if (host)
		{
		/* Sort glyphs by CID */
		qsort(h->glyphs.array, h->glyphs.cnt, sizeof(Glyph), cmpCIDs);
	
		/* Check for CID 0 */
		if (h->glyphs.cnt == 0 || h->glyphs.array[0].info->cid != 0)
			fatal(h, t1wErrNoCID0);

		writeLine(h, "%!PS-Adobe-3.0 Resource-CIDFont");
		}
	writeIdentComment(h);

	/* Write header DSC */
	writeLine(h, "%%DocumentNeededResources: ProcSet (CIDInit)");
	writeLine(h, "%%IncludeResource: ProcSet (CIDInit)");
	writeStr(h, "%%BeginResource: CIDFont (");
	writeStr(h, top->cid.CIDFontName.ptr);
	writeFmt(h, ")%s", h->arg.newline);
	writeStr(h, "%%Title: (");
	writeStr(h, top->cid.CIDFontName.ptr);
	writeStr(h, " ");
	writeStr(h, top->cid.Registry.ptr);
	writeStr(h, " ");
	writeStr(h, top->cid.Ordering.ptr);
	writeFmt(h, " %ld)%s", top->cid.Supplement, h->arg.newline);
	writeStr(h, "%%Version: ");
	writeVersion(h, top->cid.CIDFontVersion);
	writeStr(h, h->arg.newline);

	writeLine(h, "/CIDInit /ProcSet findresource begin");

	/* Compute size of FontInfo dict */
    fi_size =(3 /* Headroom for new key addition */ +
                  (top->Notice.ptr != ABF_UNSET_PTR) +
                  (top->Weight.ptr != ABF_UNSET_PTR) +
                    (top->ItalicAngle != cff_DFLT_ItalicAngle) /* ItalicAngle */ +
                    (top->FSType != ABF_UNSET_INT));
	if (host)
    {
        fi_size +=
        ((top->Copyright.ptr != ABF_UNSET_PTR) +
         (top->FullName.ptr != ABF_UNSET_PTR) +
         (top->FamilyName.ptr != ABF_UNSET_PTR) +
         (top->isFixedPitch != cff_DFLT_isFixedPitch) /* isFixedPitch */ +
         (top->UnderlinePosition != cff_DFLT_UnderlinePosition) /* UnderlinePosition */ +
         (top->UnderlineThickness != cff_DFLT_UnderlineThickness) /* UnderlineThickness */);
    }
	else
		{
		if (top->FSType == ABF_UNSET_INT)
			{
			OrigFontType = getOrigFontTypeValue(h);
			WasEmbedded = top->WasEmbedded;
			}

		fi_size += ((OrigFontType != NULL) +
					(WasEmbedded != 0));
		}

	/* Compute size of top dict */
	size =
		(3 + /* Headroom for new key addition */ +
		 1 + /* CIDFontName */ +
		 1 + /* CIDFontType */ +
		 1 + /* CIDSystemInfo */ +
		 1 + /* FontBBox */ +
		 (top->cid.UIDBase != ABF_UNSET_INT) +	/* Optional entries */
		 (top->XUID.cnt != ABF_EMPTY_ARRAY) +
		 (fi_size != 0) +						
		 1 /* CIDMapOffset */ +
		 1 /* FDBytes */ +
		 1 /* GDBytes */ +
		 1 /* CIDCount */ +
		 (top->cid.FontMatrix.cnt != ABF_EMPTY_ARRAY) +
		 1 /* FDArray */);
	if (host)
		size += (top->cid.CIDFontVersion != ABF_UNSET_REAL);
	else
		size += 1; /* CDevProc */

	writeFmt(h, "%d dict begin%s", size, h->arg.newline);
	writeLiteralDef(h, "CIDFontName", top->cid.CIDFontName.ptr);
	if (host)
		writeVersionDef(h, "CIDFontVersion", top->cid.CIDFontVersion);
	writeLine(h, "/CIDFontType 0 def");
	writeLine(h, "/CIDSystemInfo 3 dict dup begin");
	writeStringDef(h, "Registry", top->cid.Registry.ptr);
	writeStringDef(h, "Ordering", top->cid.Ordering.ptr);
	writeIntDef(h, "Supplement", top->cid.Supplement);
	writeLine(h, "end def");
	writeFontBBox(h, top->FontBBox);
	writeIntDef(h, "UIDBase", top->cid.UIDBase);
	writeXUID(h);
	if (fi_size > 0)
		{
		writeFmt(h, "/FontInfo %d dict dup begin%s", fi_size, h->arg.newline);
		writeStringDef(h, "Notice", top->Notice.ptr);
		if (host)
        {
            writeStringDef(h, "Copyright", top->Copyright.ptr);
            writeStringDef(h, "FullName", top->FullName.ptr);
            writeStringDef(h, "FamilyName", top->FamilyName.ptr);
        }
		else 
			{
			writeLiteralDef(h, "OrigFontType", OrigFontType);
			if (WasEmbedded)
				writeBooleanDef(h, "WasEmbedded", 1);
			}
        writeStringDef(h, "Weight", top->Weight.ptr);
        if (top->ItalicAngle != cff_DFLT_ItalicAngle)
            writeRealDef(h, "ItalicAngle", top->ItalicAngle);
        if (host)
        {
            if (top->isFixedPitch != cff_DFLT_isFixedPitch)
                writeBooleanDef(h, "isFixedPitch", top->isFixedPitch);
            if (top->UnderlinePosition != cff_DFLT_UnderlinePosition)
                writeRealDef(h, "UnderlinePosition", top->UnderlinePosition);
            if (top->UnderlineThickness != cff_DFLT_UnderlineThickness)
                writeRealDef(h, "UnderlineThickness", top->UnderlineThickness);
        }
		writeIntDef(h, "FSType", top->FSType);
		writeLine(h, "end def");
		}
	writeIntDef(h, "CIDMapOffset", CIDMapOffset);
	writeIntDef(h, "FDBytes", FDBytes);
	writeIntDef(h, "GDBytes", GDBytes);
	writeIntDef(h, "CIDCount", h->CIDCount);
	if (top->cid.FontMatrix.cnt != ABF_EMPTY_ARRAY)
		writeFontMatrix(h, &top->cid.FontMatrix);
	else if (h->top->FDArray.cnt == 1)
	{ /* This code is to workaround a bug in Lexmark interpreters
	   * where an oblique FontMatrix in the FDArray isn't handled
	   * correctly. If there is no FontMatrix in the top dict and
	   * only a single element in the FDArray and the font dict
	   * contains a non-default FontMatrix (not .001 0 0 .001 0 0)
	   * then the the non-default FontMatrix is copied to the top
	   * dict and the font dict's matrix is replaced with identity.
	   */
		abfFontDict *font = &h->top->FDArray.array[0];
		float *fdMatrix = font->FontMatrix.array;

		if (font->FontMatrix.cnt == 6 &&
			(fdMatrix[0] != 0.001f ||
			 fdMatrix[1] != 0.0 ||
			 fdMatrix[2] != 0.0 ||
			 fdMatrix[3] != 0.001f ||
			 fdMatrix[4] != 0.0 ||
			 fdMatrix[5] != 0.0)) /* non-default font matrix */
		{
			memcpy(top->cid.FontMatrix.array, fdMatrix, font->FontMatrix.cnt * sizeof(float));
			top->cid.FontMatrix.cnt = font->FontMatrix.cnt;
			writeFontMatrix(h, &top->cid.FontMatrix);
			font->FontMatrix.cnt = 6;
			fdMatrix[0] = 1;
			fdMatrix[1] = 0;
			fdMatrix[2] = 0;
			fdMatrix[3] = 1;
			fdMatrix[4] = 0;
			fdMatrix[5] = 0;
		}
	}

	if (host)
		{
		/* Mark used dictionaries */
		memset(FDMap, 0, h->top->FDArray.cnt);
		for (i = 0; i < h->glyphs.cnt; i++)
			FDMap[h->glyphs.array[i].info->iFD] = 1;

		FDCnt = 0;
		for (i = 0; i < h->top->FDArray.cnt; i++)
			FDCnt += FDMap[i];
		}
	else
		{
		/* Some clone PS interpreters don't define the CDevProc, e.g., HP 4550
		   a L3 clone, which means that vertical text will be printed
		   horizontally. To avoid this problem the CDevProc is defined in every
		   downloaded CIDFont. 

		   Note: the baseline is still hardcoded at .88 from the top of the em
		   square which is correct for current Adobe CJK fonts but unlikely to
		   be correct for all non-Adobe fonts or even for future Adobe fonts.
		   We may want to pass this information (when it's available) via the
		   abstract font in the future. */
		long UnitsPerEm = (h->top->sup.UnitsPerEm == ABF_UNSET_INT)?
			1000: h->top->sup.UnitsPerEm;
  
		writeFmt(h, "/CDevProc {pop pop pop pop pop "
				 "0 %ld 7 index 2 div %ld} def%s", 
				 -UnitsPerEm, (long)(UnitsPerEm*0.88 + 0.5), h->arg.newline);
		}

	writeFDArray(h, FDCnt, FDMap, SDBytes);

	if (!host)
		{
		/* Write incremental base download GlyphDirectory */
		writeFmt(h, "/GlyphDirectory %ld dict def%s", 
				 h->arg.maxglyphs, h->arg.newline);
		writeLine(h, "ct_GlyphDirProcs begin");
		writeLine(h, "GlyphDirectory");
		writeLine(h, "+");
		writeGlyphDirectory(h);
		writeLine(h, "end");
		}
	
	writeStartData(h, FDMap, StartDataSize, SDBytes, FDBytes, GDBytes);
	writeLine(h, "%%EndResource");
	if (host)
		writeLine(h, "%%EOF");
	}

/* Write incremental addition CID-keyed font. */
static void writeAddnCIDKeyedFont(t1wCtx h)
	{
	char *FontName = h->top->cid.CIDFontName.ptr;
	writeIdentComment(h);
	writeStr(h, "ct_GlyphDirProcs begin");
	writeStr(h, "/");
	writeStr(h, FontName);
	writeFmt(h, " %ld GetGlyphDirectory%s", h->glyphs.cnt, h->arg.newline);
	writeGlyphDirectory(h);
	writeLine(h, "end");
	}

/* Write cid-keyed font. */
static void writeCIDKeyedFont(t1wCtx h)
	{
	if (h->flags & SEEN_NAME_KEYED_GLYPH)
		fatal(h, t1wErrGlyphType);

	/* Validate CID data */
	if (h->top->cid.Registry.ptr == ABF_UNSET_PTR ||
		h->top->cid.Ordering.ptr == ABF_UNSET_PTR ||
		h->top->cid.Supplement == ABF_UNSET_INT ||
		h->top->cid.CIDFontName.ptr == ABF_UNSET_PTR)
		fatal(h, t1wErrBadCIDDict);

	/* Validate FDArray */
	if (h->top->FDArray.cnt < 1 || h->top->FDArray.cnt > 256)
		fatal(h, t1wErrBadFDArray);

	switch (h->arg.flags & T1W_TYPE_MASK)
		{
	case T1W_TYPE_HOST:
		writeHostCIDKeyedFont(h);
		break;
	case T1W_TYPE_BASE:
		writeHostCIDKeyedFont(h);
		break;
	case T1W_TYPE_ADDN:
		writeAddnCIDKeyedFont(h);
		break;
		}
	}

/* --------------------------- Context Management -------------------------- */

/* Validate client and create context. */
t1wCtx t1wNew(ctlMemoryCallbacks *mem_cb, ctlStreamCallbacks *stm_cb,
			  CTL_CHECK_ARGS_DCL)
	{
	t1wCtx h;

	/* Check client/library compatibility */
	if (CTL_CHECK_ARGS_TEST(T1W_VERSION))
		return NULL;

	/* Allocate context */
	h = mem_cb->manage(mem_cb, NULL, sizeof(struct t1wCtx_));
	if (h == NULL)
		return NULL;

	/* Safety initialization */
	h->glyphs.size = 0;
	h->cstr.size = 0;
	h->cntrs.size = 0;
	h->stems.size = 0;
	h->subrs.size = 0;
	h->overflow.cnt = 0;
	h->dna = NULL;
	h->stm.dst = NULL;
	h->stm.tmp = NULL;
	h->stm.dbg = NULL;
	h->err.code = t1wSuccess;
	h->cb.sing.get_stream = NULL;

	/* Copy callbacks */
	h->cb.mem = *mem_cb;
	h->cb.stm = *stm_cb;

	/* Initialize service library */
	h->dna = dnaNew(&h->cb.mem, DNA_CHECK_ARGS);
	if (h->dna == NULL)
		goto cleanup;

	dnaINIT(h->dna, h->glyphs, 256, 750);
	dnaINIT(h->dna, h->cstr, 500, 5000);
	dnaINIT(h->dna, h->cntrs, 20, 80);
	dnaINIT(h->dna, h->stems, 20, 80);
	dnaINIT(h->dna, h->subrs, 5, 100);

	h->cube_gsubrs.highLEIndex = -1000;
	h->cube_gsubrs.lowLESubrIndex = 1000;
	dnaINIT(h->dna, h->cube_gsubrs.subOffsets, 214, 10);
	dnaINIT(h->dna, h->cube_gsubrs.cstrs, 1028, 1028);

	/* Open tmp stream */
	h->stm.tmp = h->cb.stm.open(&h->cb.stm, T1W_TMP_STREAM_ID, 0);
	if (h->stm.tmp == NULL)
		goto cleanup;

	/* Open debug stream */
	h->stm.dbg = h->cb.stm.open(&h->cb.stm, T1W_DBG_STREAM_ID, 0);

	return h;

 cleanup:
	/* Initialization failed */
	t1wFree(h);
	return NULL;
	}

/* Free context. */
void t1wFree(t1wCtx h)
	{
	if (h == NULL)
		return;

	/* Close tmp stream */
	if (h->stm.tmp != NULL)
		(void)h->cb.stm.close(&h->cb.stm, h->stm.tmp);

	/* Close debug stream */
	if (h->stm.dbg != NULL)
		(void)h->cb.stm.close(&h->cb.stm, h->stm.dbg);

	dnaFREE(h->glyphs);
	dnaFREE(h->cstr);
	dnaFREE(h->cntrs);
	dnaFREE(h->stems);
	dnaFREE(h->subrs);
	dnaFREE(h->cube_gsubrs.subOffsets);
	dnaFREE(h->cube_gsubrs.cstrs);
	dnaFree(h->dna);

	/* Free library context */
	h->cb.mem.manage(&h->cb.mem, h, 0);

	return;
	}

/* Begin font. */
int t1wBegFont(t1wCtx h, long flags, int lenIV, long maxglyphs)
	{
	/* Validate font type */
	switch (flags & T1W_TYPE_MASK)
		{
	case T1W_TYPE_HOST:
	case T1W_TYPE_BASE:
	case T1W_TYPE_ADDN:
		break;
	default:
		return t1wErrBadCall;
		}

	/* Validate encoding */
	switch (flags & T1W_ENCODE_MASK)
		{
 	case T1W_ENCODE_BINARY:
	case T1W_ENCODE_ASCII:
	case T1W_ENCODE_ASCII85:
		break;
	default:
		return t1wErrBadCall;
		}

	/* Validate OtherSubrs location */
	switch (flags & T1W_OTHERSUBRS_MASK)
		{
 	case T1W_OTHERSUBRS_PRIVATE:
	case T1W_OTHERSUBRS_PROCSET:
		break;
	default:
		return t1wErrBadCall;
		}

	/* Validate newline */
	switch (flags & T1W_NEWLINE_MASK)
		{
	case T1W_NEWLINE_UNIX:
		h->arg.newline = "\n";
		break;
	case T1W_NEWLINE_WIN:
		h->arg.newline = "\r\n";
		break;
	case T1W_NEWLINE_MAC:
		h->arg.newline = "\r";
		break;
	default:
		return t1wErrBadCall;
		}

	/* Validate lenIV */
	switch (lenIV)
		{
	case -1:
	case 0: case 1: case 4:
		break;
	default:
		return t1wErrBadCall;
        }

	/* Validate maxglyphs */
	if (maxglyphs < 0)
		return t1wErrBadCall;

	/* Initialize */
	h->CIDCount = 0;
	h->arg.flags = flags;
	h->arg.lenIV = lenIV;
	h->arg.maxglyphs = maxglyphs;
	h->glyphs.cnt = 0;
	h->path.state = 0;
	h->subrs.cnt = 0;
	h->flags = 0;
	h->size.subrs = 0;
	h->size.glyphs = 0;
	
	/* Reset tmp stream */
	if (h->cb.stm.seek(&h->cb.stm, h->stm.tmp, 0))
		return t1wErrTmpStream;
	h->tmp.offset = 0;

	/* Set error handler */
  DURING_EX(h->err.env)

    /* Save standard subrs */
    saveStdSubrs(h);

  HANDLER

  	return Exception.Code;

  END_HANDLER

	return t1wSuccess;
	}

/* Finish reading font. */
int t1wEndFont(t1wCtx h, abfTopDict *top)
	{

	int destFileOpened = 0;
	if (h->cube_gsubrs.subOffsets.cnt > 0)
		saveCubeSubrs(h);
		
	/* Check for errors when accumulating glyphs */
	if (h->err.code != 0)
		return h->err.code;

	if ((top->sup.flags & ABF_SING_FONT) && h->cb.sing.get_stream == NULL)
		return t1wErrBadCall;

	/* Set error handler */
  DURING_EX(h->err.env)

    /* Open dst stream */
    h->stm.dst = h->cb.stm.open(&h->cb.stm, T1W_DST_STREAM_ID, 0);
    if (h->stm.dst == NULL)
      fatal(h, t1wErrDstStream);

    destFileOpened = 1;
      
    /* Seek to start of tmp stream and fill buffer */
    if (h->cb.stm.seek(&h->cb.stm, h->stm.tmp, 0))
      fatal(h, t1wErrTmpStream);
    readTmp(h, 0);

    /* Write out font */
    h->top = top;
    h->dst.cnt = 0;
    if (top->sup.flags & ABF_CID_FONT)
      writeCIDKeyedFont(h);
    else
      writeNameKeyedFont(h);

    flushBuf(h);

    /* Close dst stream */
    if (h->cb.stm.close(&h->cb.stm, h->stm.dst) == -1)
      return t1wErrDstStream;

	HANDLER

		if (destFileOpened)
			h->cb.stm.close(&h->cb.stm, h->stm.dst);
		return Exception.Code;

  END_HANDLER

	return t1wSuccess;
	}

/* Copy SING callback. */
int t1wSetSINGCallback(t1wCtx h, t1wSINGCallback *sing_cb)
	{
	h->cb.sing = *sing_cb;
	return 0;
	}

/* Get version numbers of libraries. */
void t1wGetVersion(ctlVersionCallbacks *cb)
{
    if (cb->called & 1<<T1W_LIB_ID)
        return;	/* Already enumerated */
    
    /* Support libraries */
    dnaGetVersion(cb);
    
    /* This library */
    cb->getversion(cb, T1W_VERSION, "t1write");
    
    /* Record this call */
    cb->called |= 1<<T1W_LIB_ID;
}

void t1wUpdateGlyphNames(t1wCtx h, char *glyphNames)
{
    int i;
    
    for (i =0; i < h->glyphs.cnt; i++)
    {
        abfString *gName = &h->glyphs.array[i].info->gname;
        gName->ptr = &glyphNames[gName->impl];
    }
}


/* Map error code to error string. */
char *t1wErrStr(int err_code)
	{
	static char *errstrs[] =
		{
#undef CTL_DCL_ERR
#define CTL_DCL_ERR(name,string)	string,
#include "t1werr.h"
		};
	return (err_code < 0 || err_code >= (int)ARRAY_LEN(errstrs))?
		"unknown error": errstrs[err_code];
	}
	
	
/* ------------ global subroutines support --------------------------------------------------*/
/* Check new LE sur to se if it is the lowest one seen */
static void seenLEIndex(t1wCtx h, long leIndex)
	{
	if (leIndex < 0 )
		{
		/* we are being called when processing a SETWV op in an LE. The subr index is the
		current number of gsubrs. */
		if (h->cube_gsubrs.lowLESubrIndex > h->cube_gsubrs.subOffsets.cnt)
			h->cube_gsubrs.lowLESubrIndex = h->cube_gsubrs.subOffsets.cnt;
		}
	else
		{
		/* we are being called when processing a callgrel op in an LE. The subr index is the
		biased such that index decrements from the first, but hte last index is -107. We can't resolve
		that to real subr number until we know the number of subrs. */
		if (h->cube_gsubrs.highLEIndex < leIndex)
			h->cube_gsubrs.highLEIndex = leIndex;
		}
	
	}


static void addCubeGSUBR(t1wCtx h, char* cstr, long length)
	{
	unsigned char* start;
	Offset* subOffset;
	subOffset = dnaNEXT(h->cube_gsubrs.subOffsets);
	start =  (unsigned char *)dnaEXTEND(h->cube_gsubrs.cstrs, (long)length);
	*subOffset = h->cube_gsubrs.cstrs.cnt;
	memcpy(start, cstr, length);
	}


/* ------------------------------ Glyph Path  ------------------------------ */

/* Begin new glyph definition. */
static int glyphBeg(abfGlyphCallbacks *cb, abfGlyphInfo *info)
	{
	t1wCtx h = cb->direct_ctx;

	cb->info = info;

	if (h->err.code != 0)
		return ABF_FAIL_RET;	/* Pending error */
	else if (h->path.state != 0)	
		{
		/* Call sequence error */
		h->err.code = t1wErrBadCall;
		return ABF_FAIL_RET;
		}
	else if (info->flags & ABF_GLYPH_SEEN)
		return ABF_SKIP_RET;	/* Ignore duplicate glyph */

	/* Initialize */
	h->path.x = 0;
	h->path.y = 0;
	h->cstr.cnt = 0;
	h->flags |= INIT_HINTS;

	if (info->flags & ABF_GLYPH_CID)
		{
		if (h->CIDCount < info->cid)
			h->CIDCount = info->cid;
		h->flags |= SEEN_CID_KEYED_GLYPH;
		}
	else 
		{
		if (info->gname.ptr == NULL || info->gname.ptr[0] == '\0')
			return ABF_FAIL_RET;	/* No glyph name in name-keyed font */
		h->flags |= SEEN_NAME_KEYED_GLYPH;
		}

	h->path.state = 1;

	return (h->arg.flags & T1W_WIDTHS_ONLY)? ABF_WIDTH_RET: ABF_CONT_RET;
	}

/* Reserve space in charstring to accomodate specified worst case args and 
   ops. Return 0 if space reserved else 1. Worst case numeric arg is sequence
   "5-byte-num 2-byte-num div". */
static int accomodate(t1wCtx h, int args, int ops)
	{
	long newsize = h->cstr.cnt + args*(5 + 2 + 2) + ops*2;
	if (newsize >= h->cstr.size)
		{
		if (dnaGrow(&h->cstr, 1, newsize))
			{
			h->err.code = t1wErrNoMemory;
			return 1;
			}
		}
	return 0;
	}

/* Save op code in charstring. */
static void saveOp(t1wCtx h, int op)
	{
	if (op & 0xff00)
		h->cstr.array[h->cstr.cnt++] = tx_escape;
	h->cstr.array[h->cstr.cnt++] = (unsigned char)op;
	}

/* Save integer in charstring. */
static void saveInt(t1wCtx h, long i)
	{
	unsigned char *t = (unsigned char *)&h->cstr.array[h->cstr.cnt];

	/* Choose format */
	if (-107 <= i && i <= 107)
		{
		/* Single byte number */
		t[0] = (unsigned char)(i + 139);
		h->cstr.cnt += 1;
		}
	else if (108 <= i && i <= 1131)
		{
		/* Positive 2-byte number */
		i -= 108;
		t[0] = (unsigned char)((i>>8) + 247);
		t[1] = (unsigned char)i;
		h->cstr.cnt += 2;
		}
	else if (-1131 <= i && i <= -108)
		{
		/* Negative 2-byte number */
		i += 108;
		t[0] = (unsigned char)((-i>>8) + 251);
		t[1] = (unsigned char)-i;
		h->cstr.cnt += 2;
		}
	else
		{
		/* Signed 5-byte number */
		t[0] = (unsigned char)255;
		t[1] = (unsigned char)(i>>24);
		t[2] = (unsigned char)(i>>16);
		t[3] = (unsigned char)(i>>8);
		t[4] = (unsigned char)i;
		h->cstr.cnt += 5;
		}
	}

/* Save real number in charstring. */
static void saveFlt(t1wCtx h, float r)
	{
 	long i = (long)r;
	if (i == r)
		/* Non-fractional arg; save as integer */
		saveInt(h, i);
	else
		{
		/* Fractional arg; construct with div */
		int denom;
		float s = r*10.0f;
		float half = (r < 0)? -0.5f: 0.5f;
		i = (long)(s + half);
		if (fabs(s - i) < 0.05)
			/* Use N 10 div */
			denom = 10;
		else
			{
			/* Use N 100 div */
			i = (long)(r*100.0f + half);
			denom = 100;
			}
		saveInt(h, i);
		saveInt(h, denom);
		saveOp(h, tx_div);
		}
	}

/* Reserve space and save subroutine call. */
static void saveCall(t1wCtx h, int subrnum)
	{
	if (accomodate(h, 1, 1))
		return;

	saveInt(h, subrnum);
	saveOp(h, tx_callsubr);
	}

/* Initialize hints. */
static void clearHints(t1wCtx h)
	{
	h->cntrs.cnt = 0;
	h->hstem3.cnt = 0;
	h->vstem3.cnt = 0;
	h->stems.cnt = 0;
	h->flags &= ~HINT_PENDING;
	}

/* Add horizontal advance width. */
static void glyphWidth(abfGlyphCallbacks *cb, float hAdv)
	{
	t1wCtx h = cb->direct_ctx;

	if (h->err.code != 0)
		return;	/* Pending error */
	else if (h->path.state != 1)
		{
		/* Call sequence error */
		h->err.code = t1wErrBadCall;
		return;
		}

	clearHints(h);

	if (accomodate(h, 2, 1))
		return;

	saveInt(h, 0);
	saveFlt(h, roundf(hAdv));
	saveOp(h, t1_hsbw);

	h->path.state = 2;
	}

/* Save OtherSubr <number> call. */
static void saveOtherSubr(t1wCtx h, int number)
	{
	int i;
	if (accomodate(h, h->stack.cnt + 2, 1))
		return;
	for (i = h->stack.cnt - 1; i >= 0; i--)
		saveFlt(h, h->stack.array[i]);
	saveInt(h, h->stack.cnt);
	saveInt(h, number);
	saveOp(h, t1_callother);
	h->stack.cnt = 0;
	}

/* Push OtherSubr argument on stack. */
static void pushOtherArg(t1wCtx h, float arg)
	{
	if (h->stack.cnt == T1_MAX_OP_STACK - 2)
		saveOtherSubr(h, t1_otherCntr1);
	h->stack.array[h->stack.cnt++] = arg;
	}

/* Save counters. */
static void saveCntrs(t1wCtx h)
	{
	long i;
	int vert;
	int lastdir;

	/* Mark all h/v transitions as new groups */
	lastdir = h->cntrs.array[0].flags & ABF_VERT_STEM;
	for (i = 1; i < h->cntrs.cnt; i++)
		{
		Stem *stem = &h->cntrs.array[i];
		int thisdir = stem->flags & ABF_VERT_STEM;
		if (lastdir != thisdir) 
			{
			stem->flags |= ABF_NEW_GROUP;
			lastdir = thisdir;
			}
		}

	h->stack.cnt = 0;
	for (vert = 1; vert >= 0; vert--)
		{
		int endGroup = 1;
		int nGroups = 0;
		for (i = h->cntrs.cnt - 1; i >= 0; i--)
			{
			Stem *stem = &h->cntrs.array[i];
			if (((stem->flags & ABF_VERT_STEM) != 0) == vert)
				{
				float prev;
				int newGroup = stem->flags & ABF_NEW_GROUP;
				if (newGroup)
					{
					prev = 0;
					nGroups++;
					}
				else
					prev = h->cntrs.array[i - 1].edge1;
				if (endGroup)
					{
					/* Last stem in group; reverse edges */
					pushOtherArg(h, stem->edge0 - stem->edge1);
					pushOtherArg(h, stem->edge1 - prev);
					}
				else
					{
					/* Save stem in delta form */
					pushOtherArg(h, stem->edge1 - stem->edge0);
					pushOtherArg(h, stem->edge0 - prev);
					}
				endGroup = newGroup;
				}
			}
		pushOtherArg(h, (float)nGroups);
		}
	saveOtherSubr(h, t1_otherCntr2);
	}

/* Save h/v stem operators. */
static void saveStemOps(t1wCtx h, long cnt, Stem *stems)
	{
	long i;

	for (i = 0; i < cnt; i++)
		{
		Stem *stem = &stems[i];

		if (accomodate(h, 2, 1))
			return;

		saveFlt(h, stem->edge0);
		saveFlt(h, stem->edge1 - stem->edge0);
		saveOp(h, (stem->flags & ABF_VERT_STEM)? tx_vstem: tx_hstem);
		}
	}

/* Save stem3 operator. */
static void saveStem3Op(t1wCtx h, Stem3 *stem3, int op)
	{
	if (stem3->cnt == 3)
		{
		long i;

		if (accomodate(h, 6, 1))
			return;
		
		for (i = 0; i < 3; i++)
			{
			Stem *stem = &stem3->array[i];
			saveFlt(h, stem->edge0);
			saveFlt(h, stem->edge1 - stem->edge0);	
			}
		saveOp(h, op);
		}
	else if (stem3->cnt > 0)
		/* Not 3 stems; save as regular stems */
		saveStemOps(h, stem3->cnt, stem3->array);
	}

/* Save stems list. */
static void saveStems(t1wCtx h)
	{
	if (h->flags & INIT_HINTS)
		{
		if (h->cntrs.cnt > 0)
			/* Save counter control (global coloring) data */
			saveCntrs(h);
		}

	if ((h->hstem3.cnt > 0 || h->vstem3.cnt > 0 || h->stems.cnt > 0) &&
		!(h->flags & INIT_HINTS))
		saveCall(h, 4);	/* Call hintsubs subr */

	saveStem3Op(h, &h->hstem3, t1_hstem3);
	saveStem3Op(h, &h->vstem3, t1_vstem3);
	saveStemOps(h, h->stems.cnt, h->stems.array);

	clearHints(h);
	h->flags &= ~INIT_HINTS;
	}

/* Add move to path. */
static void glyphMove(abfGlyphCallbacks *cb, float x0, float y0)
	{
	t1wCtx h = cb->direct_ctx;
    float dx0;
    float dy0;
    x0 = (float)RND_ON_WRITE(x0);  // need to round to 2 decimal places, else get cumulative error when reading the relative coords. This is becuase decimal valuea wre stored as at most "<int> 100 div" aka 2 decimal places.
    y0 = (float)RND_ON_WRITE(y0);
	dx0 = x0 - h->path.x;
	dy0 = y0 - h->path.y;
	h->path.x = x0;
	h->path.y = y0;

	if (h->err.code != 0)
		return;	/* Pending error */
	else if ( !(cb->info->flags & ABF_GLYPH_CUBE_GSUBR) && (h->path.state < 2))
		{
		/* Call sequence error */
		h->err.code = t1wErrBadCall;
		return;
		}

	if (accomodate(h, 0, 1))
		return;

	if ( !(cb->info->flags & ABF_GLYPH_CUBE_GSUBR) && !(h->flags & IN_FLEX) && h->path.state > 2)
		saveOp(h, t1_closepath);

	if (h->flags & HINT_PENDING)
		saveStems(h);

	if (accomodate(h, 2, 1))
		return;

	/* Choose format */
	if (dx0 == 0.0)
		{
		saveFlt(h, dy0);
		saveOp(h, tx_vmoveto);
		}
	else if (dy0 == 0.0)
		{
		saveFlt(h, dx0);
		saveOp(h, tx_hmoveto);
		}
	else
		{
		saveFlt(h, dx0);
		saveFlt(h, dy0);
		saveOp(h, tx_rmoveto);
		}

	h->path.state = 3;
	}

/* Add line to path. */
static void glyphLine(abfGlyphCallbacks *cb, float x1, float y1)
	{
	t1wCtx h = cb->direct_ctx;
    float dx1;
    float dy1;
    x1 = (float)RND_ON_WRITE(x1);  // need to round to 2 decimal places, else get cumulative error when reading the relative coords.
    y1 = (float)RND_ON_WRITE(y1);
    dx1 = x1 - h->path.x;
    dy1 = y1 - h->path.y;
	h->path.x = x1;
	h->path.y = y1;

	if (h->err.code != 0)
		return;	/* Pending error */
	else if ( !(cb->info->flags & ABF_GLYPH_CUBE_GSUBR) && (h->path.state != 3))
		{
		/* Call sequence error */
		h->err.code = t1wErrBadCall;
		return;
		}
	if (h->flags & HINT_PENDING)
		saveStems(h);

	if (accomodate(h, 2, 1))
		return;

	/* Choose format */
	if (dx1 == 0.0)
		{
		saveFlt(h, dy1);
		saveOp(h, tx_vlineto);
		}
	else if (dy1 == 0.0)
		{
		saveFlt(h, dx1);
		saveOp(h, tx_hlineto);
		}
	else
		{
		saveFlt(h, dx1); 
		saveFlt(h, dy1);
		saveOp(h, tx_rlineto);
		}
	}

/* Add curve to path. */
static void glyphCurve(abfGlyphCallbacks *cb, 
					   float x1, float y1, 
					   float x2, float y2, 
					   float x3, float y3)
	{
	t1wCtx h = cb->direct_ctx;
    float dx1;
    float dy1;
    float dx2;
    float dy2;
    float dx3;
    float dy3;

    x1 = (float)RND_ON_WRITE(x1); // need to round to 2 decimal places, else get cumulative error when reading the relative coords.
    y1 = (float)RND_ON_WRITE(y1);
    x2 = (float)RND_ON_WRITE(x2);
    y2 = (float)RND_ON_WRITE(y2);
    x3 = (float)RND_ON_WRITE(x3);
    y3 = (float)RND_ON_WRITE(y3);

    dx1 = x1 - h->path.x;
    dy1 = y1 - h->path.y;
    dx2 = x2 - x1;
    dy2 = y2 - y1;
    dx3 = x3 - x2;
    dy3 = y3 - y2;
        
    
    h->path.x = x3;
	h->path.y = y3;

	if (h->err.code != 0)
		return;	/* Pending error */
	else if ( !(cb->info->flags & ABF_GLYPH_CUBE_GSUBR) && (h->path.state != 3))
		{
		/* Call sequence error */
		h->err.code = t1wErrBadCall;
		return;
		}

	if (h->flags & HINT_PENDING)
		saveStems(h);

	if (accomodate(h, 6, 1))
		return;

	/* Choose format */
	if (dx1 == 0.0 && dy3 == 0.0)
		{
		saveFlt(h, dy1);
		saveFlt(h, dx2); 
		saveFlt(h, dy2);
		saveFlt(h, dx3);
		saveOp(h, tx_vhcurveto);
		}
	else if (dy1 == 0.0 && dx3 == 0.0)
		{
		saveFlt(h, dx1);
		saveFlt(h, dx2);
		saveFlt(h, dy2);
		saveFlt(h, dy3);
		saveOp(h, tx_hvcurveto);
		}
	else
		{
		saveFlt(h, dx1); 
		saveFlt(h, dy1);
		saveFlt(h, dx2); 
		saveFlt(h, dy2);
		saveFlt(h, dx3); 
		saveFlt(h, dy3);
		saveOp(h, tx_rrcurveto);
		}
	}

/* Add stem hint. */
static void glyphStem(abfGlyphCallbacks *cb, 
					  int flags, float edge0, float edge1)
	{
	t1wCtx h = cb->direct_ctx;
	long index;
	Stem *stem;

	if (h->err.code != 0)
		return;
	else if ( !(cb->info->flags & ABF_GLYPH_CUBE_GSUBR) && (h->path.state < 2))
		{
		/* Call sequence error */
		h->err.code = t1wErrBadCall;
		return;
		}

	if (flags & ABF_NEW_HINTS)
		{
		if (h->flags & INIT_HINTS &&
			h->arg.flags & (T1W_TYPE_HOST|T1W_ENABLE_CMP_TEST))
			/* Retain initial hints for host type fonts or comparison tests */
			saveStems(h);
		else
			{
			/* Discard accumulated hints */
			h->hstem3.cnt = 0;
			h->vstem3.cnt = 0;
			h->stems.cnt = 0;
			}
		}

	if (flags & ABF_CNTR_STEM)
		{
		/* Global coloring */
		index = dnaNext(&h->cntrs, sizeof(Stem));
		if (index == -1)
	 		goto no_memory;
		stem = &h->cntrs.array[index];
		}
	else if (flags & ABF_STEM3_STEM)
		{
		/* h/vstem3 */
		Stem3 *stem3 = (flags & ABF_VERT_STEM)? &h->vstem3: &h->hstem3;
		if (stem3->cnt == 3)
			return;
		stem = &stem3->array[stem3->cnt++];
		}
	else
		{
		/* h/vstem */
		index = dnaNext(&h->stems, sizeof(Stem));
		if (index == -1)
			goto no_memory;
		stem = &h->stems.array[index];
		}

	stem->edge0 = edge0;
	stem->edge1 = edge1;
	stem->flags = flags;
	h->flags |= HINT_PENDING;
	return;

 no_memory:
	h->err.code = t1wErrNoMemory;
	}

/* Add flex hint. */
static void glyphFlex(abfGlyphCallbacks *cb, float depth,
					  float x1, float y1, 
					  float x2, float y2, 
					  float x3, float y3,
					  float x4, float y4, 
					  float x5, float y5, 
					  float x6, float y6)
	{
	t1wCtx h = cb->direct_ctx;
	float x0 = h->path.x;
	float y0 = h->path.y;

	if (h->err.code != 0)
		return;	/* Pending error */
	else if ( !(cb->info->flags & ABF_GLYPH_CUBE_GSUBR) && (h->path.state != 3))
		{
		/* Call sequence error */
		h->err.code = t1wErrBadCall;
		return;
		}

	if (h->flags & HINT_PENDING)
		saveStems(h);

	h->flags |= IN_FLEX;

    x1 = (float)RND_ON_WRITE(x1); // need to round to 2 decimal places, else get cumulative error when reading the relative coords.
    y1 = (float)RND_ON_WRITE(y1);
    x2 = (float)RND_ON_WRITE(x2);
    y2 = (float)RND_ON_WRITE(y2);
    x3 = (float)RND_ON_WRITE(x3);
    y3 = (float)RND_ON_WRITE(y3);
    x4 = (float)RND_ON_WRITE(x4);
    y4 = (float)RND_ON_WRITE(y4);
    x5 = (float)RND_ON_WRITE(x5);
    y5 = (float)RND_ON_WRITE(y5);
    x6 = (float)RND_ON_WRITE(x6);
    y6 = (float)RND_ON_WRITE(y6);

	saveCall(h, 1);
	if (fabs(x6 - x0) > fabs(y6 - y0))
		glyphMove(cb, x3, y0);
	else
		glyphMove(cb, x0, y3);
	saveCall(h, 2);
	glyphMove(cb, x1, y1); saveCall(h, 2);
	glyphMove(cb, x2, y2); saveCall(h, 2);
	glyphMove(cb, x3, y3); saveCall(h, 2);
	glyphMove(cb, x4, y4); saveCall(h, 2);
	glyphMove(cb, x5, y5); saveCall(h, 2);
	glyphMove(cb, x6, y6); saveCall(h, 2);

	h->flags &= ~IN_FLEX;

	if (accomodate(h, 4, 1))
		return;
		
	saveFlt(h, depth);
	saveFlt(h, x6); 
	saveFlt(h, y6);
	saveInt(h, 0);
	saveOp(h, tx_callsubr);

	h->flags |= SEEN_FLEX;
	}

/* Add generic operator. */
static void glyphGenop(abfGlyphCallbacks *cb, int cnt, float *args, int op)
	{
	t1wCtx h = cb->direct_ctx;

	if (cb->info->flags & ABF_GLYPH_CUBE_GSUBR)
		{
		if (accomodate(h, cnt, 1))
			return;

		switch (op)
			{
		case tx_return:
			if (cnt)
				{
				while (cnt--)
					saveFlt(h, *args++);
				}
			break;
		case tx_SETWV1:
		case tx_SETWV2:
		case tx_SETWV3:
		case tx_SETWV4:
		case tx_SETWV5:
            while (cnt--)
                saveFlt(h, *args++);
            seenLEIndex(h, -1);
            saveOp(h, op);
            break;
        case tx_SETWVN:
            while (cnt-- > 1)
                saveFlt(h, *args++);
            saveInt(h, (int)*args++);
            seenLEIndex(h, -1);
            saveOp(h, op);
            break;
		case tx_callgrel:
			{
			seenLEIndex(h, (long)args[cnt-1]);
			while (cnt--)
				saveFlt(h, *args++);
			saveOp(h, op);
			break;
			}
		default:
			{
			while (cnt--)
				saveFlt(h, *args++);
			saveOp(h, op);
			break;
			}
			}
		}
	else
		{
		if (h->err.code != 0)
			return;	/* Pending error */
		else if ( !(cb->info->flags & ABF_GLYPH_CUBE_GSUBR) && (h->path.state < 2))
			{
			/* Call sequence error */
			h->err.code = t1wErrBadCall;
			return;
			}

		if (h->flags & HINT_PENDING)
			saveStems(h);

		switch (op)
			{
		case t2_cntron:
			/* 0 6 callother */
			h->stack.cnt = 0;
			saveOtherSubr(h, t1_otherGlobalColor); 
			break;
		case t2_cntroff:
			/* 0 0 2 13 callother */
			h->stack.cnt = 0;
			pushOtherArg(h, 0);
			pushOtherArg(h, 0);
			saveOtherSubr(h, t1_otherCntr2);
			break;
		default:
			if (accomodate(h, cnt, 1))
				return;
			while (cnt--)
				saveFlt(h, *args++);
			saveOp(h, op);
			break;
			}
		}
	}

/* Add seac operator. */
static void glyphSeac(abfGlyphCallbacks *cb,
					  float adx, float ady, int bchar, int achar)
	{
	t1wCtx h = cb->direct_ctx;
	
	if (h->err.code != 0)
		return;	/* Pending error */
	else if ( !(cb->info->flags & ABF_GLYPH_CUBE_GSUBR) && (h->path.state != 2))
		{
		/* Call sequence error */
		h->err.code = t1wErrBadCall;
		return;
		}

	if (accomodate(h, 5, 1))
		return;

	saveInt(h, 0);
	saveFlt(h, adx);
	saveFlt(h, ady);
	saveInt(h, bchar);
	saveInt(h, achar);
	saveOp(h, t1_seac);

	h->path.state = 4;
	}

/* End glyph definition. */
static void glyphEnd(abfGlyphCallbacks *cb)
	{
	t1wCtx h = cb->direct_ctx;
	Glyph *glyph;
	long index;

	if (h->err.code != 0)
		return;
	else if ( !(cb->info->flags & ABF_GLYPH_CUBE_GSUBR) && (h->path.state < 2))
		{
		/* Call sequence error */
		h->err.code = t1wErrBadCall;
		return;
		}

	if (cb->info->flags & ABF_GLYPH_CUBE_GSUBR)
		{
		saveOp(h, tx_return);
		addCubeGSUBR(h, h->cstr.array,  h->cstr.cnt);
		h->path.state = 0;
		return;
		}

	/* Save closepath endchar */
	if (accomodate(h, 0, 2))
		return;
	if ( !(cb->info->flags & ABF_GLYPH_CUBE_GSUBR) && (h->path.state > 2))
		saveOp(h, t1_closepath);
	saveOp(h, tx_endchar);

	/* Allocate glyph */
	index = dnaNext(&h->glyphs, sizeof(Glyph));
	if (index == -1)
		{
		h->err.code = t1wErrNoMemory;
		return;
		}
	glyph = &h->glyphs.array[index];
	glyph->info = cb->info;

	/* Save charstring */
	if (saveCstr(h, glyph->info,
				 h->cstr.cnt, (unsigned char *)h->cstr.array, &glyph->cstr))
		{
		h->err.code = t1wErrTmpStream;
		return;
		}

	h->size.glyphs += glyph->cstr.length;

	h->path.state = 0;
	}
static void glyphCubeBlend(abfGlyphCallbacks *cb, unsigned int nBlends, unsigned int numVals, float* blendVals)
	{
	t1wCtx h = cb->direct_ctx;
	unsigned int i = 0;


	while (i < numVals)
		{
		saveFlt(h, blendVals[i]);
		}
	switch(nBlends)
		{
		case 1:
			saveOp(h, tx_BLEND1);
			break;
		case 2:
			saveOp(h, tx_BLEND2);
			break;
		case 3:
			saveOp(h, tx_BLEND3);
			break;
		case 4:
			saveOp(h, tx_BLEND4);
			break;
		case 6:
			saveOp( h, tx_BLEND6);
			break;
		}

	}

static void glyphCubeSetWV(abfGlyphCallbacks *cb,  unsigned int numDV)
	{
	t1wCtx h = cb->direct_ctx;

	switch(numDV)
		{
		case 1:
			saveOp(h, tx_SETWV1);
			break;
		case 2:
			saveOp(h, tx_SETWV2);
			break;
		case 3:
			saveOp(h, tx_SETWV3);
			break;
		case 4:
			saveOp(h, tx_SETWV4);
			break;
        case 5:
            saveOp( h, tx_SETWV5);
            break;
        default:
            saveOp( h, tx_SETWVN);
		}
	}

static void glyphCubeCompose(abfGlyphCallbacks *cb,  int cubeLEIndex, float x0, float y0, int numDV, float* ndv)
	{
	t1wCtx h = cb->direct_ctx;
	int i = 0;
	float dx0;
	float dy0;

    if (accomodate(h, 3+numDV, 1))
        return;

	saveInt(h, (long)cubeLEIndex); /* library element value */
	/* Compute delta and update current point */
	dx0 = x0 - h->path.x; 
	dy0 = y0 - h->path.y;
	h->path.x = x0;
	h->path.y = y0;
	saveInt(h, (long)dx0);	
	saveInt(h, (long)dy0);	
	
	while (i < numDV)
		{
		saveInt(h, (long)ndv[i++]);
		}
	saveOp(h, tx_compose);
	}
	
static void cubeTransform(abfGlyphCallbacks *cb,  float rotate, float scaleX, float scaleY, float skewX, float skewY)
	{
	t1wCtx h = cb->direct_ctx;
    /* Rotation is expressed in degrees. Rotation is *100, 
     and the scale/skew are *1000 to avoid the use of the div
     oerator in the charstring.  */
    if (accomodate(h, 5, 1))
        return;

    saveInt(h, RND2I(rotate*100));
	saveInt(h, RND2I(scaleX*1000));
	saveInt(h, RND2I(scaleY*1000));
	saveInt(h, RND2I(skewX*1000));
	saveInt(h, RND2I(skewY*1000));
	saveOp(h, tx_transform);
	}


/* Public callback set */
const abfGlyphCallbacks t1wGlyphCallbacks =
	{
	NULL,
	NULL,
	NULL,
	glyphBeg,
	glyphWidth,
	glyphMove,
	glyphLine,
	glyphCurve,
	glyphStem,
	glyphFlex,
	glyphGenop,
	glyphSeac,
	glyphEnd,
	glyphCubeBlend,
	glyphCubeSetWV,
	glyphCubeCompose,
	cubeTransform,
	};

/* ----------------------------- Debug Support ----------------------------- */

#if T1W_DEBUG

/* Debug Type 1 charstring. */
static void dbt1cstr(long length, unsigned char *cstr)
	{
	static char *opname[32] =
		{
		/*  0 */ "reserved0",
		/*  1 */ "hstem",
		/*  2 */ "compose",
		/*  3 */ "vstem",
		/*  4 */ "vmoveto",
		/*  5 */ "rlineto",
		/*  6 */ "hlineto",
		/*  7 */ "vlineto",
		/*  8 */ "rrcurveto",
		/*  9 */ "closepath",
		/* 10 */ "callsubr",
		/* 11 */ "return",
		/* 12 */ "escape",
		/* 13 */ "hsbw",
		/* 14 */ "endchar",
		/* 15 */ "vsindex",
		/* 16 */ "blend",
		/* 17 */ "callgrel",
		/* 18 */ "reserved18",
		/* 19 */ "reserved19",
		/* 20 */ "reserved20",
		/* 21 */ "rmoveto",
		/* 22 */ "hmoveto",
		/* 23 */ "reserved23",
		/* 24 */ "reserved24",
		/* 25 */ "reserved25",
		/* 26 */ "reserved26",
		/* 27 */ "reserved27",
		/* 28 */ "reserved28",
		/* 29 */ "reserved29",
		/* 30 */ "vhcurveto",
		/* 31 */ "hvcurveto",
		};
	static char *escopname[] =
		{
		/*  0 */ "dotsection",
		/*  1 */ "vstem3",
		/*  2 */ "hstem3",
		/*  3 */ "and",
		/*  4 */ "or",
		/*  5 */ "not",
		/*  6 */ "seac",
		/*  7 */ "sbw",
		/*  8 */ "store",
		/*  9 */ "abs",
		/* 10 */ "add",
		/* 11 */ "sub",
		/* 12 */ "div",
		/* 13 */ "load",
		/* 14 */ "neg",
		/* 15 */ "eq",
		/* 16 */ "callother",
		/* 17 */ "pop",
		/* 18 */ "drop",
		/* 19 */ "reservedESC19",
		/* 20 */ "put",
		/* 21 */ "get",
		/* 22 */ "ifelse",
		/* 23 */ "random",
		/* 24 */ "mul",
		/* 25 */ "div2",
		/* 26 */ "sqrt",
		/* 27 */ "dup",
		/* 28 */ "exch",
		/* 29 */ "index",
		/* 30 */ "roll",
		/* 31 */ "reservedESC31",
		/* 32 */ "reservedESC32",
		/* 33 */ "setcurrentpoint",
            /* 39 */ "blend1",
            /* 40 */ "blend2",
            /* 41 */ "blend3",
            /* 42 */ "blend4",
            /* 43 */ "blend6",
            /* 44 */ "setwv1",
            /* 45 */ "setwv2",
            /* 46 */ "setwv3",
            /* 47 */ "setwv4",
            /* 48 */ "setwv5",
            /* 49 */ "setwvn",
            /* 50 */ "transform",
		};
	long i = 0;

	while (i < length)
		{
		int op = cstr[i];
		switch (op)
			{
		case tx_reserved0:
		case tx_hstem:
		case tx_compose:
		case tx_vstem:
		case tx_vmoveto:
		case tx_rlineto:
		case tx_hlineto:
		case tx_vlineto:
		case tx_rrcurveto:
		case t1_closepath:
		case tx_callsubr:
		case tx_return:
		case t1_hsbw:
		case tx_endchar:
		case t1_moveto:
        case t1_reserved16:
        case tx_callgrel:
		case t1_reserved18:
		case t1_reserved19:
		case t1_reserved20:
		case tx_rmoveto:
		case tx_hmoveto:
		case t1_reserved23:
		case t1_reserved24:
		case t1_reserved25:
		case t1_reserved26:
		case t1_reserved27:
		case t1_reserved28:
		case t1_reserved29:
		case tx_vhcurveto:
		case tx_hvcurveto:
			printf("%s ", opname[op]);
			i++;
			break;
		case tx_escape:
			{
			/* Process escaped operator */
			int escop = cstr[i + 1];
			if (escop > ARRAY_LEN(escopname) - 1)
				printf("? ");
			else
				printf("%s ", escopname[escop]);
			i += 2;
			}
			break;
		case 247: 
		case 248: 
		case 249: 
		case 250:
			/* Positive 2 byte number */
			printf("%d ", 108 + 256 * (cstr[i] - 247) + cstr[i + 1]);
			i += 2;
			break;
		case 251:
		case 252:
		case 253:
		case 254:
			/* Negative 2 byte number */
			printf("%d ", -108 - 256 * (cstr[i] - 251) - cstr[i + 1]);
			i += 2;
			break;
		case 255:
			{
			/* 5 byte number */
			long value = (long)cstr[i + 1]<<24 | (long)cstr[i + 2]<<16 |
				(long)cstr[i + 3]<<8 | (long)cstr[i + 4];
			printf("%ld ", value);
			i += 5;
			break;
			}
		default:
			/* 1 byte number */
			printf("%d ", cstr[i] - 139);
			i++;
			break;
			}
		}
	printf("\n");
	}

/* This function just serves to suppress annoying "defined but not used"
   compiler messages when debugging */
static void CTL_CDECL dbuse(int arg, ...)
	{
	dbuse(0, dbt1cstr);
	}

#endif /* T1W_DEBUG */
