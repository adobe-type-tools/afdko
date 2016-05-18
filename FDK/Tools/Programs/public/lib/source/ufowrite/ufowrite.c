/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

#include "ufowrite.h"
#include "dynarr.h"
#include "dictops.h"
#include "txops.h"
#include "ctutil.h"

#include <setjmp.h>
#include <stdarg.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <limits.h>

#define ARRAY_LEN(a) 	(sizeof(a)/sizeof((a)[0]))

#define XML_HEADER "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
#define PLIST_DTD_HEADER "<!DOCTYPE plist PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">"

#ifdef _WIN32
	#define round round_int
#endif
/* ---------------------------- Library Context ---------------------------- */

typedef struct				/* Glyph data */
{
    char glyphName[FILENAME_MAX];
    char glifFileName[FILENAME_MAX];
} Glyph;

typedef struct
{
    int opType;
    float coords[6];	/* Float matrix */
    char* pointName;
} OpRec;

enum
{
	movetoType,
	linetoType,
	curvetoType,
    initialCurvetoType,
    finalCurvetoType,
	closepathType,
} OpType;


struct ufwCtx_
{
	int state;				/* 0 == writing to tmp; 1 == writing to dst */
	abfTopDict *top;		/* Top Dict data */
	dnaDCL(Glyph, glyphs);	/* Glyph data */
    int lastiFD;            /* The index into teh FD array of the last glyph seen. Used only when the source is a CID font.*/
	struct					/* Client-specified data */
    {
		long flags;			/* See ufowrite.h for flags */
		char *glyphLayer;
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
		dnaDCL(OpRec, opList);
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
		jmp_buf env;
		int code;
    } err;
};

/* ----------------------------- Error Handling ---------------------------- */

/* Handle fatal error. */
static void fatal(ufwCtx h, int err_code)
{
	if (h->stm.dbg != NULL)
    {
		/* Write error message to debug stream */
		char *text = ufwErrStr(err_code);
		(void)h->cb.stm.write(&h->cb.stm, h->stm.dbg, strlen(text), text);
    }
	h->err.code = err_code;
	longjmp(h->err.env, 1);
}

#if _WIN32
static int round_int( double r )
{
     return (int)((r > 0.0) ? (r + 0.5) : (r - 0.5)); 
}
#endif

/* --------------------------- Destination Stream -------------------------- */

/* Flush dst/tmp stream buffer. */
static void flushBuf(ufwCtx h)
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
		err = ufwErrTmpStream;
    }
	else  /* h->state == 1 */
    {
		stm = h->stm.dst;
		buf = h->dst.buf;
		cnt = &h->dst.cnt;
		err = ufwErrDstStream;
    }
    
	if (*cnt == 0)
		return;	/* Nothing to do */
    
    if (setjmp(h->err.env))
        return;
    
	/* Write buffered bytes */
	if (h->cb.stm.write(&h->cb.stm, stm, *cnt, buf) != *cnt)
		fatal(h, err);
    
	*cnt = 0;
}

/* Write to dst/tmp stream buffer. */
static void writeBuf(ufwCtx h, size_t writeCnt, const char *ptr)
{
	char *buf;
	size_t *cnt;
	int err;
	size_t left;
	if (h->state == 0)
    {
		buf = h->tmp.buf;
		cnt = &h->tmp.cnt;
		err = ufwErrTmpStream;
    }
	else  /* h->state == 1 */
    {
		buf = h->dst.buf;
		cnt = &h->dst.cnt;
		err = ufwErrDstStream;
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
static void writeInt(ufwCtx h, long value)
{
	char buf[50];
	sprintf(buf, "%ld", value);
	writeBuf(h, strlen(buf), buf);
}

/* Convert a long into a string */
static void ufw_ltoa(char* buf, long val)
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
/*In Xcode, FLT_EPSILON is 1.192..x10-7, but the diff between value-roundf(value) can be 3.05..x10-5, when the input value was a 24.8 Fixed. */
static void writeReal(ufwCtx h, float value)
{
	char buf[50];
	/* if no decimal component, perform a faster to string conversion */
	if ((fabs(value-roundf(value)) < TX_EPSILON) && value>LONG_MIN && value<LONG_MAX)
		ufw_ltoa(buf, (long)roundf(value));
	else
		ctuDtostr(buf, value, 0, 2);
	writeBuf(h, strlen(buf), buf);
}

/* Write null-terminated string to dst steam. */
static void writeStr(ufwCtx h, const char *s)
{
	writeBuf(h, strlen(s), s);
}

/* Write null-terminated string to dst steam and escape XML reserved characters. */
static void writeXMLStr(ufwCtx h, const char *s)
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
static void writeLine(ufwCtx h, char *s)
{
	writeStr(h, s);
	writeStr(h, "\n");
}

/* Write formatted data to dst stream. This function must only be called when
 the maximum size of the resulting formatted string is known in advance. It
 must never be called with a string that has been passed into this library
 since it might cause a buffer overrun. Those strings may be handled safely
 by calling writeStr() directly. */
static void CTL_CDECL writeFmt(ufwCtx h, char *fmt, ...)
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
ufwCtx ufwNew(ctlMemoryCallbacks *mem_cb, ctlStreamCallbacks *stm_cb,
			  CTL_CHECK_ARGS_DCL)
{
	ufwCtx h;
    
	/* Check client/library compatibility */
	if (CTL_CHECK_ARGS_TEST(UFW_VERSION))
		return NULL;
    
	/* Allocate context */
	h = mem_cb->manage(mem_cb, NULL, sizeof(struct ufwCtx_));
	if (h == NULL)
		return NULL;
    
	/* Safety initialization */
	h->state = 0;
	h->top = NULL;
	h->glyphs.size = 0;
    h->path.opList.size= 0;

	h->dna = NULL;
	h->stm.dst = NULL;
	h->stm.dbg = NULL;
	h->err.code = ufwSuccess;
    h->lastiFD = ABF_UNSET_INT;
    
	/* Copy callbacks */
	h->cb.mem = *mem_cb;
	h->cb.stm = *stm_cb;
    
	/* Initialize service library */
	h->dna = dnaNew(&h->cb.mem, DNA_CHECK_ARGS);
	if (h->dna == NULL)
		goto cleanup;
    
    h->arg.glyphLayer = "glyphs";
    
	dnaINIT(h->dna, h->glyphs, 256, 750);
	dnaINIT(h->dna, h->path.opList, 256, 750);
    
	/* Open debug stream */
	h->stm.dbg = h->cb.stm.open(&h->cb.stm, UFW_DBG_STREAM_ID, 0);
    
	return h;
    
cleanup:
	/* Initialization failed */
	ufwFree(h);
	return NULL;
}

/* Free context. */
void ufwFree(ufwCtx h)
{
	if (h == NULL)
		return;
    
	/* Close debug stream */
	if (h->stm.dbg != NULL)
	{
		(void)h->cb.stm.close(&h->cb.stm, h->stm.dbg);
		h->stm.dbg = NULL;
	}
	dnaFREE(h->glyphs);
	dnaFREE(h->path.opList);
	dnaFree(h->dna);
    
	/* Free library context */
	h->cb.mem.manage(&h->cb.mem, h, 0);
    
	return;
}

/* Begin font. */
int ufwBegFont(ufwCtx h, long flags, char *glyphLayerDir)
{
    
	/* Initialize */
	h->arg.flags = flags;
	h->tmp.cnt = 0;
	h->dst.cnt = 0;
	h->glyphs.cnt = 0;
	h->path.opList.cnt = 0;
	h->path.state = 0;
	h->top = NULL;
	h->stm.dst = NULL;
    h->state = 1;  /* Indicates writing to dst stream */
    
    if (glyphLayerDir != NULL)
        h->arg.glyphLayer = glyphLayerDir;
    
    /* Set error handler */
    if (setjmp(h->err.env))
        return h->err.code;
    
    return ufwSuccess;
}

static void writeContents(ufwCtx h)
{
    Glyph* glyphRec;
    char buffer[FILENAME_MAX];
    int i;
	/* Set error handler */
	if (setjmp(h->err.env))
	{
		if (h->stm.dst)
			h->cb.stm.close(&h->cb.stm, h->stm.dst);
		return;
	}
    
	h->state = 1;  /* Indicates writing to dst stream */
    
    
    /* Open fontinfo.plist file as dst stream */
    
    sprintf(buffer, "%s/%s", h->arg.glyphLayer, "contents.plist");
    h->cb.stm.clientFileName = buffer;
    h->stm.dst = h->cb.stm.open(&h->cb.stm, UFW_DST_STREAM_ID, 0);
    if (h->stm.dst == NULL)
        fatal(h, ufwErrDstStream);
    
    writeLine(h, XML_HEADER);
    writeLine(h, PLIST_DTD_HEADER);
    writeLine(h, "<plist version=\"1.0\">");
    writeLine(h, "<dict>");
    for (i = 0; i < h->glyphs.cnt; i++)
    {
        glyphRec = &h->glyphs.array[i];
        sprintf(buffer, "\t<key>%s</key>", glyphRec->glyphName);
        writeLine(h, buffer);
        sprintf(buffer, "\t<string>%s</string>", glyphRec->glifFileName);
        writeLine(h, buffer);
    }
    
    
    writeLine(h, "</dict>");
    writeLine(h, "</plist>");
    
    /* Close dst stream */
    flushBuf(h);
	h->cb.stm.close(&h->cb.stm, h->stm.dst);
    
    return;
    
}

static void writeGlyphOrder(ufwCtx h)
{
    Glyph* glyphRec;
    char buffer[FILENAME_MAX];
    int i;
    
	/* Set error handler */
	if (setjmp(h->err.env))
	{
		if (h->stm.dst)
			h->cb.stm.close(&h->cb.stm, h->stm.dst);
		return;
	}
    
	h->state = 1;  /* Indicates writing to dst stream */
    
    
    /* Open lib.plist file as dst stream */
    
    sprintf(buffer, "%s", "lib.plist");
    h->cb.stm.clientFileName = buffer;
    h->stm.dst = h->cb.stm.open(&h->cb.stm, UFW_DST_STREAM_ID, 0);
    if (h->stm.dst == NULL)
        fatal(h, ufwErrDstStream);
    
    writeLine(h, XML_HEADER);
    writeLine(h, PLIST_DTD_HEADER);
    writeLine(h, "<plist version=\"1.0\">");
    writeLine(h, "<dict>");
    writeLine(h, "\t<key>public.glyphOrder</key>");
    writeLine(h, "\t<array>");
    for (i = 0; i < h->glyphs.cnt; i++)
    {
        glyphRec = &h->glyphs.array[i];
        sprintf(buffer, "\t\t<string>%s</string>", glyphRec->glyphName);
        writeLine(h, buffer);
    }
    
    
    writeLine(h, "\t</array>");
    writeLine(h, "</dict>");
    writeLine(h, "</plist>");
    
    
	/* Close dst stream */
    flushBuf(h);
	h->cb.stm.close(&h->cb.stm, h->stm.dst);
    
    return;
}
static void writeMetaInfo(ufwCtx h)
{
    char buffer[FILENAME_MAX];
    
	/* Set error handler */
	if (setjmp(h->err.env))
	{
		if (h->stm.dst)
			h->cb.stm.close(&h->cb.stm, h->stm.dst);
		return;
	}
    
	h->state = 1;  /* Indicates writing to dst stream */
    
    
    /* Open lib.plist file as dst stream */
    
    sprintf(buffer, "%s", "metainfo.plist");
    h->cb.stm.clientFileName = buffer;
    h->stm.dst = h->cb.stm.open(&h->cb.stm, UFW_DST_STREAM_ID, 0);
    if (h->stm.dst == NULL)
        fatal(h, ufwErrDstStream);
    
    writeLine(h, XML_HEADER);
    writeLine(h, PLIST_DTD_HEADER);
    writeLine(h, "<plist version=\"1.0\">");
    writeLine(h, "<dict>");
    writeLine(h, "\t<key>creator</key>");
    writeLine(h, "\t<string>com.adobe.type.tx</string>");
    writeLine(h, "\t<key>formatVersion</key>");
    writeLine(h, "\t<integer>2</integer>");
    writeLine(h, "</dict>");
    writeLine(h, "</plist>");
    
    
	/* Close dst stream */
    flushBuf(h);
	h->cb.stm.close(&h->cb.stm, h->stm.dst);
    
    return;
}

static void setStyleName(char* dst, char* postScriptName)
{
    /* Copy text after '-'; if none, return empty string.*/
    char* p = &postScriptName[0];
    while ((*p != '-') && (*p != 0x00))
        p++;
    
    if (*p == 0x00)
    {
        *dst =0;
        return;
    }
    p++;
    strcpy(dst, p);
}

static void setVersionMajor(char* dst, char* version)
{
    /* Copy text up to '.' Skip leading zeroes.*/
    int seenNonZero = 0;
    char* p = &version[0];
    while ((*p != '.') && (*p != 0x00))
    {
        if ((!seenNonZero) && (*p == '0'))
        {
           p++; 
        }
        else
        {
        *dst++ = *p++;
        seenNonZero = 1;
        }
    }
    *dst = 0x00;
}

static void setVersionMinor(char* dst, char* version)
{
    /* Copy text after '.'; if none, return '0'.*/
    int seenNonZero = 0;
    char* p = &version[0];
    while ((*p != '.') && (*p != 0x00))
        p++;
    
    if (*p == 0x00)
    {
        dst[0] = '0';
        dst[1] = 0x00;
        return;
    }
    p++; /* skip the period */
    while (*p != 0x00)
    {
        if ((!seenNonZero) && (*p == '0'))
        {
            p++;
        }
        else
        {
            *dst++ = *p++;
            seenNonZero = 1;
        }
    }
    if (!seenNonZero)
        *dst++ = '0';
    *dst = 0x00;
}

static int writeFontInfo(ufwCtx h, abfTopDict *top)
{
    char buffer[FILENAME_MAX];
    char buffer2[FILENAME_MAX];
    abfFontDict* fontDict0;
	abfPrivateDict* privateDict;
    int i;
    
    if (h->lastiFD !=  ABF_UNSET_INT)
        fontDict0 = &(top->FDArray.array[h->lastiFD]);
    else
        fontDict0 = &(top->FDArray.array[0]);
    privateDict = &(fontDict0->Private);
    
	/* Set error handler */
	if (setjmp(h->err.env))
	{
		if (h->stm.dst)
			h->cb.stm.close(&h->cb.stm, h->stm.dst);
		return h->err.code;
	}
    
	h->state = 1;  /* Indicates writing to dst stream */
    
    
    /* Open fontinfo.plist file as dst stream */
    
    sprintf(buffer, "%s", "fontinfo.plist");
    h->cb.stm.clientFileName = buffer;
    h->stm.dst = h->cb.stm.open(&h->cb.stm, UFW_DST_STREAM_ID, 0);
    if (h->stm.dst == NULL)
        fatal(h, ufwErrDstStream);
    
    writeLine(h, XML_HEADER);
    writeLine(h, PLIST_DTD_HEADER);
    writeLine(h, "<plist version=\"1.0\">");
    writeLine(h, "<dict>");
    
    /* This is what I care about the most. Add the rest in the order of the
     UFO 3 spec. */
    writeLine(h, "\t<key>postscriptFontName</key>");
    if (top->sup.flags & ABF_CID_FONT)
    {
        sprintf(buffer, "\t<string>%s</string>", top->cid.CIDFontName.ptr);
        writeLine(h, buffer);
        setStyleName(buffer2, top->cid.CIDFontName.ptr);
        writeLine(h, "\t<key>styleName</key>");
        sprintf(buffer, "\t<string>%s</string>", buffer2);
        writeLine(h, buffer);
    }
    else
    {
        sprintf(buffer, "\t<string>%s</string>", fontDict0->FontName.ptr);
        writeLine(h, buffer);
        setStyleName(buffer2, fontDict0->FontName.ptr);
        writeLine(h, "\t<key>styleName</key>");
        sprintf(buffer, "\t<string>%s</string>", buffer2);
        writeLine(h, buffer);
    }
    
    if (top->FamilyName.ptr != NULL)
    {
        writeLine(h, "\t<key>familyName</key>");
        sprintf(buffer, "\t<string>%s</string>", top->FamilyName.ptr);
        writeLine(h, buffer);
    }
    
    if (top->sup.flags & ABF_CID_FONT)
    {
        if (top->cid.CIDFontVersion != 0)
        {
            char versionStr[32];
            sprintf(versionStr, "%.3f", top->cid.CIDFontVersion);
            setVersionMajor(buffer2, versionStr);
            writeLine(h, "\t<key>versionMajor</key>");
            sprintf(buffer, "\t<integer>%s</integer>", buffer2);
            writeLine(h, buffer);
            setVersionMinor(buffer2, versionStr);
            writeLine(h, "\t<key>versionMinor</key>");
            sprintf(buffer, "\t<integer>%s</integer>", buffer2);
            writeLine(h, buffer);
        }
    }
    else
    {
        if (top->version.ptr != NULL)
        {
            setVersionMajor(buffer2, top->version.ptr);
            writeLine(h, "\t<key>versionMajor</key>");
            sprintf(buffer, "\t<integer>%s</integer>", buffer2);
            writeLine(h, buffer);
            setVersionMinor(buffer2, top->version.ptr);
            writeLine(h, "\t<key>versionMinor</key>");
            sprintf(buffer, "\t<integer>%s</integer>", buffer2);
            writeLine(h, buffer);
        }
    }
    
    if (top->Copyright.ptr != NULL)
    {
        writeLine(h, "\t<key>copyright</key>");
        sprintf(buffer, "\t<string>%s</string>", top->Copyright.ptr);
        writeLine(h, buffer);
    }
    
    if (top->Notice.ptr != NULL)
    {
        writeLine(h, "\t<key>trademark</key>");
        sprintf(buffer, "\t<string>%s</string>", top->Notice.ptr);
        writeLine(h, buffer);
    }
    
    writeLine(h, "\t<key>unitsPerEm</key>");
    if (fontDict0->FontMatrix.cnt > 0)
    {
        sprintf(buffer, "\t<integer>%d</integer>",
                (int)round(1.0/fontDict0->FontMatrix.array[0])
                );
        writeLine(h, buffer);
        
    }
    else{
        writeLine(h, "\t<integer>1000</integer>");
    }
    
    if (top->ItalicAngle != cff_DFLT_ItalicAngle)
    {
        writeLine(h, "\t<key>italicAngle</key>");
        sprintf(buffer, "\t<real>%.8f</real>", top->ItalicAngle);
        writeLine(h, buffer);
    }
    
    /* Now for all the other PostScript-specific UFO data */
    if (top->FullName.ptr != NULL)
    {
        writeLine(h, "\t<key>postscriptFullName</key>");
        sprintf(buffer, "\t<string>%s</string>", top->FullName.ptr);
        writeLine(h, buffer);
    }
    
    if (top->Weight.ptr != NULL)
    {
        writeLine(h, "\t<key>postscriptWeightName</key>");
        sprintf(buffer, "\t<string>%s</string>", top->Weight.ptr);
        writeLine(h, buffer);
    }
    
    if (top->UnderlinePosition != cff_DFLT_UnderlinePosition)
    {
        writeLine(h, "\t<key>postscriptUnderlinePosition</key>");
        sprintf(buffer, "\t<integer>%d</integer>", (int)round(0.5 + top->UnderlinePosition));
        writeLine(h, buffer);
    }
    
    if (top->UnderlineThickness != cff_DFLT_UnderlineThickness)
    {
        writeLine(h, "\t<key>postscriptUnderlineThickness</key>");
        sprintf(buffer, "\t<integer>%d</integer>", (int)round(0.5 + top->UnderlineThickness));
        writeLine(h, buffer);
    }
    
    if ((top->isFixedPitch != cff_DFLT_isFixedPitch) && (top->isFixedPitch > 0))
    {
        writeLine(h, "\t<key>postscriptIsFixedPitch</key>");
        writeLine(h, "\t<true/>");
    }
    
    /* All the Blue values */
    if (privateDict->BlueValues.cnt != ABF_EMPTY_ARRAY)
    {
        writeLine(h, "\t<key>postscriptBlueValues</key>");
        writeLine(h, "\t<array>");
        for (i = 0; i < privateDict->BlueValues.cnt; i++)
        {
            float stem = privateDict->BlueValues.array[i];
            if (stem == ((int)stem))
                sprintf(buffer, "\t\t<integer>%d</integer>", (int)stem);
            else
                sprintf(buffer, "\t\t<real>%.2f</real>", stem);
            writeLine(h, buffer);
            
        }
        writeLine(h, "\t</array>");
    }
    
    if (privateDict->OtherBlues.cnt != ABF_EMPTY_ARRAY)
    {
        writeLine(h, "\t<key>postscriptOtherBlues</key>");
        writeLine(h, "\t<array>");
        for (i = 0; i < privateDict->OtherBlues.cnt; i++)
        {
            float stem = privateDict->OtherBlues.array[i];
            if (stem == ((int)stem))
                sprintf(buffer, "\t\t<integer>%d</integer>", (int)stem);
            else
                sprintf(buffer, "\t\t<real>%.2f</real>", stem);
            writeLine(h, buffer);
        }
        writeLine(h, "\t</array>");
    }
    
    if (privateDict->FamilyBlues.cnt != ABF_EMPTY_ARRAY)
    {
        writeLine(h, "\t<key>postscriptFamilyBlues</key>");
        writeLine(h, "\t<array>");
        for (i = 0; i < privateDict->FamilyBlues.cnt; i++)
        {
            float stem = privateDict->FamilyBlues.array[i];
            if (stem == ((int)stem))
                sprintf(buffer, "\t\t<integer>%d</integer>", (int)stem);
            else
                sprintf(buffer, "\t\t<real>%.2f</real>", stem);
            writeLine(h, buffer);
        }
        writeLine(h, "\t</array>");
    }
    
    if (privateDict->FamilyOtherBlues.cnt != ABF_EMPTY_ARRAY)
    {
        writeLine(h, "\t<key>postscriptFamilyOtherBlues</key>");
        writeLine(h, "\t<array>");
        for (i = 0; i < privateDict->FamilyOtherBlues.cnt; i++)
        {
            float stem = privateDict->FamilyOtherBlues.array[i];
            if (stem == ((int)stem))
                sprintf(buffer, "\t\t<integer>%d</integer>", (int)stem);
            else
                sprintf(buffer, "\t\t<real>%.2f</real>", stem);
            writeLine(h, buffer);
            
        }
        writeLine(h, "\t</array>");
    }
 
    if (privateDict->StemSnapH.cnt != ABF_EMPTY_ARRAY)
    {
        writeLine(h, "\t<key>postscriptStemSnapH</key>");
        writeLine(h, "\t<array>");
        for (i = 0; i < privateDict->StemSnapH.cnt; i++)
        {
            float stem = privateDict->StemSnapH.array[i];
            if (stem == ((int)stem))
                sprintf(buffer, "\t\t<integer>%d</integer>", (int)stem);
            else
                sprintf(buffer, "\t\t<real>%.2f</real>", stem);
            writeLine(h, buffer);
            
        }
        writeLine(h, "\t</array>");
    }
    else if (privateDict->StdHW != ABF_UNSET_REAL)
    {
        float stem = privateDict->StdHW;
        writeLine(h, "\t<key>postscriptStemSnapH</key>");
        writeLine(h, "\t<array>");
        if (stem == ((int)stem))
            sprintf(buffer, "\t\t<integer>%d</integer>", (int)stem);
        else
            sprintf(buffer, "\t\t<real>%.2f</real>", stem);
        writeLine(h, buffer);
        writeLine(h, "\t</array>");
    }
    
    if (privateDict->StemSnapV.cnt != ABF_EMPTY_ARRAY)
    {
        writeLine(h, "\t<key>postscriptStemSnapV</key>");
        writeLine(h, "\t<array>");
        for (i = 0; i < privateDict->StemSnapV.cnt; i++)
        {
            float stem = privateDict->StemSnapV.array[i];
            if (stem == ((int)stem))
                sprintf(buffer, "\t\t<integer>%d</integer>", (int)stem);
            else
                sprintf(buffer, "\t\t<real>%.2f</real>", stem);
            writeLine(h, buffer);
            
        }
        writeLine(h, "\t</array>");
    }
    else if (privateDict->StdVW != ABF_UNSET_REAL)
    {
        float stem = privateDict->StdVW;
        writeLine(h, "\t<key>postscriptStemSnapV</key>");
        writeLine(h, "\t<array>");
        if (stem == ((int)stem))
            sprintf(buffer, "\t\t<integer>%d</integer>", (int)stem);
        else
            sprintf(buffer, "\t\t<real>%.2f</real>", stem);
        writeLine(h, buffer);
        writeLine(h, "\t</array>");
    }
    
    if (privateDict->BlueScale != ABF_UNSET_REAL)
    {
        char *p;
        char valueBuffer[50];
        /* 8 places is as good as it gets when converting ASCII real numbers->float-> ASCII real numbers, as happens to all the  PrivateDict values.*/
        sprintf(valueBuffer, "%.8f", privateDict->BlueScale);
        p = strchr(valueBuffer, '.');
        if (p != NULL) {
            /* Have decimal point. Remove trailing zeroes.*/
            int l = strlen(p);
            p += l-1;
            while(*p == '0')
            {
                *p = '\0';
                p--;
            }
            if (*p == '.') {
                *p = '\0';
            }
        }

        writeLine(h, "\t<key>postscriptBlueScale</key>");
        sprintf(buffer, "\t<real>%s</real>", valueBuffer);
        writeLine(h, buffer);
    }
    
    
    if (privateDict->BlueShift != ABF_UNSET_REAL)
    {
        writeLine(h, "\t<key>postscriptBlueShift</key>");
        sprintf(buffer, "\t<integer>%d</integer>", (int)(0.5 + privateDict->BlueShift));
        writeLine(h, buffer);
    }
    
    
    if (privateDict->BlueFuzz != ABF_UNSET_REAL)
    {
        writeLine(h, "\t<key>postscriptBlueFuzz</key>");
        sprintf(buffer, "\t<integer>%d</integer>",  (int)(0.5 + privateDict->BlueFuzz));
        writeLine(h, buffer);
    }
    
    
    if ((privateDict->ForceBold != ABF_UNSET_INT) && (privateDict->ForceBold > 0))
    {
        writeLine(h, "\t<key>postscriptForceBold</key>");
        writeLine(h, "\t<true/>");
    }
    
    
    /* Finish the file */
    writeLine(h, "</dict>");
    writeLine(h, "</plist>");
    flushBuf(h);
    
    /* Close dst stream */
	if (h->cb.stm.close(&h->cb.stm, h->stm.dst) == -1)
		return ufwErrDstStream;
    
	return ufwSuccess;
}

/* Finish reading font. */
int ufwEndFont(ufwCtx h, abfTopDict *top)
{
	size_t cntTmp = 0;
	size_t cntRead = 0;
	size_t cntWrite = 0;
	char *pBuf = NULL;
    int errCode = ufwSuccess;
    
	/* Check for errors when accumulating glyphs */
	if (h->err.code != 0)
		return h->err.code;
    
	h->top = top;
    errCode = writeFontInfo(h, top);
    if (errCode != ufwSuccess)
        return errCode;
    
    writeContents(h);
    writeGlyphOrder(h);
    writeMetaInfo(h);
    h->state = 0;  /* Indicates writing to temp stream */
	return ufwSuccess;
}

/* Get version numbers of libraries. */
void ufwGetVersion(ctlVersionCallbacks *cb)
{
	if (cb->called & 1<<UFW_LIB_ID)
		return;	/* Already enumerated */
    
	/* Support libraries */
	dnaGetVersion(cb);
    
	/* This library */
	cb->getversion(cb, UFW_VERSION, "ufwrite");
    
	/* Record this call */
	cb->called |= 1<<UFW_LIB_ID;
}

/* Map error code to error string. */
char *ufwErrStr(int err_code)
{
	static char *errstrs[] =
    {
#undef CTL_DCL_ERR
#define CTL_DCL_ERR(name,string)	string,
#include "ufowerr.h"
    };
	return (err_code < 0 || err_code >= (int)ARRAY_LEN(errstrs))?
    "unknown error": errstrs[err_code];
}

/* ------------------------------ Glyph Path  ------------------------------ */

static void mapGlyphToGLIFName(char* glyphName, char*glifName)
{
    char* p = &glyphName[0];
    char* q = &glifName[0];
    
    if (*p == '.')
    {
        *q++ = '_';
        p++;
    }
    
    while (*p != 0x00)
    {
        if ((q-&glifName[0]) >= FILENAME_MAX)
        {
            glifName[FILENAME_MAX-1] = 0;
            printf("Warning! glif name '%s' is longer than the file name buffer size.\n", glifName);
            break;
        }
        
        if (((*p >= 'A') && (*p <= 'Z')) || ((*p >= 0x00) && (*p <= 0x1F)))
        {
            *q++ = *p++;
            *q++ = '_';
            
        }
        else if ((*p >= 0x00) && (*p <= 0x1F))
        {
            p++;
            *q++ = '_';
            
        }
        else
        {
            char code = *p;
            switch (code)
            {
                case '*':
                case '+':
                case '/':
                case ':':
                case '<':
                case '>':
                case '?':
                case '[':
                case '\\':
                case ']':
                case 0x7F:
                    *q++ = '_';
                    break;
                default:
                    *q++ = *p++;
                    break;
                    
            }
        }
        
    }
    *q = 0x00;
    strcat(glifName, ".glif");
}

/* Begin new glyph definition. */
static int glyphBeg(abfGlyphCallbacks *cb, abfGlyphInfo *info)
{
	ufwCtx h = cb->direct_ctx;
	char buf[9];
    char glyphName[FILENAME_MAX];
    char glifName[FILENAME_MAX];
    char glifRelPath[FILENAME_MAX];
    Glyph *glyphRec;
    
	cb->info = info;
    
	if (h->err.code != 0)
		return ABF_FAIL_RET;	/* Pending error */
	else if (h->path.state != 0)
    {
		/* Call sequence error */
		h->err.code = ufwErrBadCall;
		return ABF_FAIL_RET;
    }
	else if (info->flags & ABF_GLYPH_SEEN)
		return ABF_SKIP_RET;	/* Ignore duplicate glyph */
    
    if (info->flags & ABF_GLYPH_CID)
    {
        h->lastiFD = info->iFD;
        sprintf(glyphName, "cid%05hu", info->cid);
    }
    else
    {
        sprintf(glyphName, "%s", info->gname.ptr);
        sprintf(glifName, "%s", info->gname.ptr);
    }
    
    mapGlyphToGLIFName(glyphName, glifName);
    
	/* Initialize */
	h->path.x = 0;
	h->path.y = 0;
	h->path.state = 1;
    
    if (setjmp(h->err.env))
        return h->err.code;
    
    /* Open GLIF file as dst stream */
    sprintf(glifRelPath, "%s/%s", h->arg.glyphLayer, glifName);
    h->cb.stm.clientFileName = glifRelPath;
    h->stm.dst = h->cb.stm.open(&h->cb.stm, UFW_DST_STREAM_ID, 0);
    if (h->stm.dst == NULL)
        fatal(h, ufwErrDstStream);
    // write start of GL:IF file and open glyph element.
    writeLine(h, XML_HEADER);
    writeStr(h, "<glyph");
    
    // write glyph name
    writeStr(h, " name=\"");
    writeStr(h, glyphName);
    writeStr(h, "\"");
    
    // close glyph element
    writeLine(h, " format=\"1\" >");
    
    // write encoding, if present.
    
    if (info->flags & ABF_GLYPH_UNICODE)
    {
        writeStr(h, "\t<unicode hex=\"");
        sprintf(buf, "%06lX", info->encoding.code);
        writeStr(h, buf);
        writeLine(h, "\"/>");
    }
    
    glyphRec = dnaNEXT(h->glyphs);
    strcpy(glyphRec->glyphName, glyphName);
    strcpy(glyphRec->glifFileName, glifName);
	return ABF_CONT_RET;
}

/* Add horizontal advance width. */
static void glyphWidth(abfGlyphCallbacks *cb, float hAdv)
{
	ufwCtx h = cb->direct_ctx;
    
	if (h->err.code != 0)
		return;	/* Pending error */
	else if (h->path.state != 1)
    {
		/* Call sequence error */
		h->err.code = ufwErrBadCall;
		return;
    }
    
	writeStr(h, "\t<advance width=\"");
	writeInt(h, (long)roundf(hAdv));
	writeLine(h, "\"/>");
    
	h->path.state = 2;
}

static void writeGlyphMove(ufwCtx h, float x0, float y0)
{
    writeStr(h, "\t\t\t<point x=\"");
    writeReal(h, x0);
    writeStr(h, "\" y=\"");
    writeReal(h, y0);
    writeLine(h, "\" type=\"move\"/>");
    
}

static void writeGlyphLine(ufwCtx h, float x0, float y0)
{
    writeStr(h, "\t\t\t<point x=\"");
    writeReal(h, x0);
    writeStr(h, "\" y=\"");
    writeReal(h, y0);
    writeLine(h, "\" type=\"line\"/>");
    
}

static void writeGlyphCurve(ufwCtx h, float* coords)
{
    writeStr(h, "\t\t\t<point x=\"");
    writeReal(h, coords[0]);
    writeStr(h, "\" y=\"");
    writeReal(h, coords[1]);
    writeLine(h, "\" />");
    
    
    writeStr(h, "\t\t\t<point x=\"");
    writeReal(h, coords[2]);
    writeStr(h, "\" y=\"");
    writeReal(h, coords[3]);
    writeLine(h, "\" />");
    
    writeStr(h, "\t\t\t<point x=\"");
    writeReal(h, coords[4]);
    writeStr(h, "\" y=\"");
    writeReal(h, coords[5]);
    writeLine(h, "\" type=\"curve\"/>");
}

static void writeGlyphInitialCurve(ufwCtx h, float* coords)
{
    writeStr(h, "\t\t\t<point x=\"");
    writeReal(h, coords[0]);
    writeStr(h, "\" y=\"");
    writeReal(h, coords[1]);
    writeLine(h, "\" type=\"curve\"/>");
}

static void writeGlyphFinalCurve(ufwCtx h, float* coords)
{
    writeStr(h, "\t\t\t<point x=\"");
    writeReal(h, coords[0]);
    writeStr(h, "\" y=\"");
    writeReal(h, coords[1]);
    writeLine(h, "\" />");
    
    
    writeStr(h, "\t\t\t<point x=\"");
    writeReal(h, coords[2]);
    writeStr(h, "\" y=\"");
    writeReal(h, coords[3]);
    writeLine(h, "\" />");
}


static void writeContour(ufwCtx h)
{
    OpRec* opRec;
    OpRec* opRec2;
    float* lastCoords;
    
    int i;
    
    if (h->path.opList.cnt < 2)
    {
        h->path.opList.cnt = 0;
        return; /* Don't write paths with only a single move-to. UFO fonts can make these. */
    }
    
    /* Fix up the start op. UFo fonts require a completely closed path, and no initial move-to.
    If the last op coords are not the same as the move-to:
      - change the initial move-to to a line-to.
    else if the last op is a line-to:
       - change the initial move-to op to a line-to, and remove the last op.
    else if the last op is a curve-to:
       - change the initial move-to type to a "initial curve-to" type - just writes the final curve point
      -  change the final curve-to type to a "final curve-to" type - just writes the first two points of the curve.
     */
    opRec = &h->path.opList.array[0];
    opRec2 = &h->path.opList.array[h->path.opList.cnt -1];
    if (opRec2->opType == curvetoType)
    {
        lastCoords = &(opRec2->coords[4]);
    }
    else
    {
        lastCoords = opRec2->coords;
    }
    
    if ((lastCoords[0] != opRec->coords[0]) || (lastCoords[1] != opRec->coords[1]))
    {
        opRec->opType = linetoType;
    }
    else if (opRec2->opType == linetoType)
    {
        opRec->opType = linetoType;
        h->path.opList.cnt--;
    }
    else if (opRec2->opType == curvetoType)
    {
        opRec->opType = initialCurvetoType;
        opRec2->opType = finalCurvetoType;
    }
    
    
    writeStr(h, "\t\t<contour>\n");
    for (i=0; i< h->path.opList.cnt; i++)
    {
        opRec = &h->path.opList.array[i];
        switch (opRec->opType)
        {
            case movetoType:
                writeGlyphMove(h, opRec->coords[0], opRec->coords[1]);
                break;
            case linetoType:
                writeGlyphLine(h, opRec->coords[0], opRec->coords[1]);
                break;
            case curvetoType:
                writeGlyphCurve(h, opRec->coords);
                break;
            case initialCurvetoType:
                writeGlyphInitialCurve(h, opRec->coords);
                break;
            case finalCurvetoType:
                writeGlyphFinalCurve(h, opRec->coords);
                break;
            default:
                break;
       }
     }
    writeStr(h, "\t\t</contour>\n");
    h->path.opList.cnt = 0;
}

/* Add move to path. */
static void glyphMove(abfGlyphCallbacks *cb, float x0, float y0)
{
    OpRec* opRec;
	ufwCtx h = cb->direct_ctx;
	h->path.x = x0;
	h->path.y = y0;
    
	if (h->err.code != 0)
		return;	/* Pending error */
	else if (h->path.state < 2)
    {
		/* Call sequence error */
		h->err.code = ufwErrBadCall;
		return;
    }
    
	if (h->path.state == 3)
    {
        /* Write previous path. */
        writeContour(h);
    }
    else if (h->path.state < 3)
    {
        /* First path data seen in glyph */
        writeLine(h, "\t<outline>");
    }
    else if (h->path.state > 3)
    {
        /* Call sequence error */
        h->err.code = ufwErrBadCall;
        return;
    }
    opRec = dnaNEXT(h->path.opList);
    opRec->opType = movetoType;
    opRec->coords[0] = x0;
    opRec->coords[1] = y0;
    
    h->path.state = 3;
}

/* Add line to path. */
static void glyphLine(abfGlyphCallbacks *cb, float x1, float y1)
{
    OpRec* opRec;
	ufwCtx h = cb->direct_ctx;
	float dx1 = x1 - h->path.x;
	float dy1 = y1 - h->path.y;
	h->path.x = x1;
	h->path.y = y1;
    
	if (h->err.code != 0)
		return;	/* Pending error */
	else if (h->path.state != 3)
    {
		/* Call sequence error */
		h->err.code = ufwErrBadCall;
		return;
    }
    
    opRec = dnaNEXT(h->path.opList);
    opRec->opType = linetoType;
    opRec->coords[0] = x1;
    opRec->coords[1] = y1;
}

/* Add curve to path. */
static void glyphCurve(abfGlyphCallbacks *cb,
					   float x1, float y1,
					   float x2, float y2,
					   float x3, float y3)
{
    OpRec* opRec;
	ufwCtx h = cb->direct_ctx;
	h->path.x = x3;
	h->path.y = y3;
    
	if (h->err.code != 0)
		return;	/* Pending error */
	else if (h->path.state != 3)
    {
		/* Call sequence error */
		h->err.code = ufwErrBadCall;
		return;
    }
    
    opRec = dnaNEXT(h->path.opList);
    opRec->opType = curvetoType;
    opRec->coords[0] = x1;
    opRec->coords[1] = y1;
    opRec->coords[2] = x2;
    opRec->coords[3] = y2;
    opRec->coords[4] = x3;
    opRec->coords[5] = y3;

}

/* Generic operator callback; ignored. */
static void glyphGenop(abfGlyphCallbacks *cb, int cnt, float *args, int op)
{
	ufwCtx h = cb->direct_ctx;
    
	if (h->err.code != 0)
		return;	/* Pending error */
	else if (h->path.state < 2)
    {
		/* Call sequence error */
		h->err.code = ufwErrBadCall;
		return;
    }
    
	/* Do nothing; ignore generic operator. */
}

/* Seac operator callback. It is an error to call this when writing ufo
 fonts. Instead, clients should flatten seac operators into compound
 glyphs. */
static void glyphSeac(abfGlyphCallbacks *cb,
					  float adx, float ady, int bchar, int achar)
{
	ufwCtx h = cb->direct_ctx;
	
	if (h->err.code != 0)
		return;	/* Pending error */
    
	/* Call sequence error; seac glyphs should be expanded by the client. */
	h->err.code = ufwErrBadCall;
}

/* End glyph definition. */
static void glyphEnd(abfGlyphCallbacks *cb)
{
    
	ufwCtx h = cb->direct_ctx;
    
	if (h->err.code != 0)
		return;
	else if (h->path.state < 2)
    {
		/* Call sequence error */
		h->err.code = ufwErrBadCall;
		return;
    }
    
	if (h->path.state >= 3) /* have seen a move to. */
        writeContour(h);

	if (h->path.state < 3)/* have NOT seen a move to, hence have never emitted an <outline> tag. */
        writeLine(h, "\t<outline>");
    
    writeLine(h, "\t</outline>");
    writeLine(h, "</glyph>");
	h->path.state = 0;
    
    flushBuf(h);
    /* Close dst stream */
    h->cb.stm.close(&h->cb.stm, h->stm.dst);
    
    
    return;
    
}


/* Public callback set */
const abfGlyphCallbacks ufwGlyphCallbacks =
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
	NULL, // compose
	NULL, // transform
};
