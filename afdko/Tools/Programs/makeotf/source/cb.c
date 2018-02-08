/* 
 Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
 This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0.
 */
/***********************************************************************/
/*
 * SCCS Id:    @(#)cb.c	1.11
 * Changed:    3/21/99 17:43:27
 */
/*
   hot library callback support layer.

   This module provides a test client for the hot library. hot library clients
   do not need to include this module; it is mearly provided for reference.
 */

#if __CENTERLINE__
#include "cb.h"
#include "hotconv.h"
#include "dynarr.h"
#else

#ifndef PACKAGE_SPECS
#define PACKAGE_SPECS "package.h"
#endif
#ifndef PACKAGE_SPECS_H
#include PACKAGE_SPECS
#endif

#include "setjmp.h"

extern jmp_buf mark;

#include HOTCONV
#include DYNARR

#endif

#include "cb.h"
/*#include "sun.h"*/
#include "file.h"
#include "fcdb.h"
#include "lstdlib.h"
#include "lstring.h"
#include "lctype.h"
#include "cbpriv.h"
#include "systemspecific.h"
#undef _DEBUG
#include "ctutil.h"
#include <errno.h>

#define FEATUREDIR   "features"

#define INT2FIX(i)  ((int32_t)(i) << 16)

#define WIN_SPACE      32
#define WIN_BULLET     149
#define WIN_NONSYMBOLCHARSET 0

#define WINDOWS_DONT_CARE	(0 << 4)
#define WINDOWS_ROMAN		(1 << 4)
#define WINDOWS_SWISS		(2 << 4)
#define WINDOWS_MODERN		(3 << 4)
#define WINDOWS_SCRIPT		(4 << 4)
#define WINDOWS_DECORATIVE	(5 << 4)
#define MAX_CHAR_NAME_LEN			63		/* Max charname len (inc '\0') */
#define MAX_FINAL_CHAR_NAME_LEN			63		/* Max charname len (inc '\0') */
#ifdef WIN32
char sepch();	/* from WIN.C */

#endif

#ifdef _MSC_VER  /* defined by Microsoft Compiler */
#include <io.h>
#include <fcntl.h>
#include <sys\stat.h>
#endif


/*extern char *font_encoding;
  extern int font_serif;
 */
extern int KeepGoing;

typedef struct {
	unsigned long tag;				/* Anon table tag */
	short refillDone;				/* Done with refilling? */
	dnaDCL(char, data);				/* Data */
} AnonInfo;

typedef struct {						/* Alias name record */
	long iKey;						/* Alias name key index */
	long iFinal;						/* Final name index */
	long iUV;						/* UV override name index */
	long iOrder;						/* order index */
} AliasRec;

/* tc library client callback context */
struct cbCtx_ {
	char *progname;				/* Program name */
	dnaCtx dnaCtx;
	
	struct {						/* Hot library */
		hotCtx ctx;
		hotCallbacks cb;
	} hot;
	
	struct {						/* Postscript font file input */
		File file;
		char buf[BUFSIZ];
		char *(*refill)(cbCtx h, long *count);	/* Buffer refill callback */
		long left;				/* Bytes remaining in segment */
	} ps;
	
	struct {						/* CFF data input/output */
		char *next;				/* Data fill pointer */
		dnaDCL(char, buf);
		short euroAdded;
	} cff;
	
	struct {						/* OTF file input/output */
		File file;
		char buf[BUFSIZ];
	} otf;
	
	struct {						/* Feature file input */
		char *mainFile;			/* Main feature file name */
		char *includeDir[4];	/* font dir*/
		File file;
		char buf[BUFSIZ];		/* Refill buffer for feature file */
		dnaDCL(AnonInfo, anon);	/* Storage for anon tables */
	} feat;
	
	struct {						/* ucs file input/output */
		File file;
	} uvs;
	
	struct {						/* Temporary file input/output */
		File file;
		char buf[BUFSIZ];
	} tmp;
	
	struct {						/* Adobe CMap input */
		File file;
		char buf[BUFSIZ];
	} CMap;
	
	struct {						/* Font conversion database */
		fcdbCtx ctx;
		dnaDCL(File, files);	/* Database file list */
		char buf[BUFSIZ];
		hotEncoding macenc;		/* Mac encoding accumulator */
		unsigned short syntaxVersion;
	} fcdb;
	
	struct {						/* Glyph name aliasing database */
		dnaDCL(AliasRec, recs);	/* Alias name records */
		dnaDCL(char, names);	/* Name string buffer */
		int useFinalNames;
	} alias;
	
	struct {						/* Glyph name aliasing database */
		dnaDCL(AliasRec, recs);	/* final name records */
	} final;
	
	char *matchkey;				/* Temporary lookup key for match functions */
	
	struct {						/* Directory paths */
		char *pfb;
		char *otf;
		char *cmap;
	} dir;
	
	dnaDCL(char, tmpbuf);	/* Temporary buffer */
	hotMacData mac;			/* Mac-specific data from database */
};

/* ----------------------------- Error Handling ---------------------------- */

/* [hot callback] Fatal exception handler */
void myfatal(void *ctx) {
	if (!KeepGoing) {
		cbCtx h = ctx;
		/*This seems to cause all kinds of crashes on Windows and OSX*/
		/* hotFree(h->hot.ctx);*/	/* Free library context */

		free(h);
		exit(1);		/* Could also longjmp back to caller from here */
	}
}

/* [hot callback] Print error message */
void message(void *ctx, int type, char *text) {
	cbCtx h = ctx;

	/* Print type */
	switch (type) {
		case hotNOTE:
			fprintf(stderr, "%s [NOTE] ", h->progname);
			break;

		case hotWARNING:
			fprintf(stderr, "%s [WARNING] ", h->progname);
			break;

		case hotERROR:
			fprintf(stderr, "%s [ERROR] ", h->progname);
			break;

		case hotFATAL:
			fprintf(stderr, "%s [FATAL] ", h->progname);
			break;
	}
	fprintf(stderr, "%s\n", text);
}

/* --------------------------- Memory Management --------------------------- */

/* Make a copy of a string */
static void copyStr(cbCtx h, char **dst, char *src) {
	*dst = cbMemNew(h, strlen(src) + 1);
	strcpy(*dst, src);
}

/* Allocate memory */
void *cbMemNew(cbCtx h, size_t size) {
	void *ptr = malloc(size);
	if (ptr == NULL) {
		cbFatal(h, "out of memory");
	}
	return ptr;
}

static void *CBcbMemNew(void *h, size_t size) {
	return cbMemNew((cbCtx)h, size);
}

/* Resize memory */
void *cbMemResize(cbCtx h, void *old, size_t size) {
	void *new = realloc(old, size);
	if (new == NULL) {
		cbFatal(h, "out of memory");
	}
	return new;
}

static void *CBcbMemResize(void *h, void *old, size_t size) {
	return cbMemResize((cbCtx)h, old, size);
}

/* Free memory */
void cbMemFree(cbCtx h, void *ptr) {
	if (ptr != NULL) {
		free(ptr);
	}
}

static void CBcbMemFree(void *h, void *ptr) {
	if (ptr != NULL) {
		free(ptr);
	}
}

/* ------------------------------- Font Input ------------------------------ */

/* Read 1-byte */
#define read1(h)	fileRead1(&h->ps.file)

/* [hot callback] Return source filename */
static char *psId(void *ctx) {
	cbCtx h = ctx;
	return h->ps.file.name;
}

/* Refill input buffer from CIDFont file */
static char *CIDRefill(cbCtx h, long *count) {
	*count = fileReadN(&h->ps.file, BUFSIZ, h->ps.buf);
	return h->ps.buf;
}

/* Refill input buffer from PFB file */
static char *PFBRefill(cbCtx h, long *count) {
reload:
	if (h->ps.left >= BUFSIZ) {
		/* Full block */
		*count = fileReadN(&h->ps.file, BUFSIZ, h->ps.buf);
		if (*count != BUFSIZ) {
			fileError(&h->ps.file);
		}
		h->ps.left -= BUFSIZ;
	}
	else if (h->ps.left > 0) {
		/* Partial block */
		*count = fileReadN(&h->ps.file, h->ps.left, h->ps.buf);
		if (*count != h->ps.left) {
			fileError(&h->ps.file);
		}
		h->ps.left = 0;
	}
	else {
		/* New segment; read segment id */
		int escape = read1(h);
		int type = read1(h);

		/* Check segment id */
		if (escape != 128 || (type != 1 && type != 2 && type != 3)) {
			cbFatal(h, "invalid segment [%s]", h->ps.file.name);
		}

		if (type == 3) {
			*count = 0;	/* EOF */
		}
		else {
			/* Read segment length (little endian) */
			h->ps.left = read1(h);
			h->ps.left |= (int32_t)read1(h) << 8;
			h->ps.left |= (int32_t)read1(h) << 16;
			h->ps.left |= (int32_t)read1(h) << 24;

			goto reload;
		}
	}
	return h->ps.buf;
}

/* Determine font type and convert font to CFF */
static char *psConvFont(cbCtx h, int flags, char *filename, int *psinfo, hotReadFontOverrides *fontOverrides) {
	int b0;
	int b1;
	char *FontName;

	fileOpen(&h->ps.file, h, filename, "rb");

	/* Determine font file type */
	b0 = read1(h);
	b1 = read1(h);
	if (b0 == '%' && b1 == '!') {
		/* CIDFont */
		h->ps.refill = CIDRefill;
	}
	else if (b0 == 128 && b1 == 1) {
		/* PFB */
		h->ps.refill = PFBRefill;
	}
	else {
		cbFatal(h, "unknown file type [%s]\n", filename);
	}

	/* Seek back to beginning */
	fileSeek(&h->ps.file, 0, SEEK_SET);
	h->ps.left = 0;
	FontName = hotReadFont(h->hot.ctx, flags, psinfo, fontOverrides);
	fileClose(&h->ps.file);

	return FontName;
}

/* [TC callback] Refill input buffer */
static char *psRefill(void *ctx, long *count) {
	cbCtx h = ctx;
	return h->ps.refill(h, count);
}

/* ------------------------- CFF data input/output ------------------------- */

/* [hot callback] Return CFF data id (for diagnostics) */
static char *cffId(void *ctx) {
	return "internal CFF data buffer";
}

/* [hot callback] Write 1 byte to data buffer */
static void cffWrite1(void *ctx, int c) {
	cbCtx h = ctx;
	*h->cff.next++ = c;
}

/* [hot callback] Write N bytes to data buffer */
static void cffWriteN(void *ctx, long count, char *ptr) {
	cbCtx h = ctx;
	memcpy(h->cff.next, ptr, count);
	h->cff.next += count;
}

/* [hot callback] Seek to offset and return data */
static char *cffSeek(void *ctx, long offset, long *count) {
	cbCtx h = ctx;
	if (offset >= h->cff.buf.cnt || offset < 0) {
		/* Seek outside data bounds */
		*count = 0;
		return NULL;
	}
	else {
		*count = h->cff.buf.cnt - offset;
		return &h->cff.buf.array[offset];
	}
}

/* [hot callback] Refill data buffer from current position */
static char *cffRefill(void *ctx, long *count) {
	/* Never called in this implementation since all data in memory */
	*count = 0;
	return NULL;
}

/* [hot callback] Receive CFF data size */
static void cffSize(void *ctx, long size, int euroAdded) {
	cbCtx h = ctx;
	dnaSET_CNT(h->cff.buf, size);
	h->cff.next = h->cff.buf.array;
	h->cff.euroAdded = euroAdded;
}

/* -------------------------- OTF data input/ouput ------------------------- */

/* [hot callback] Return OTF filename */
static char *otfId(void *ctx) {
	cbCtx h = ctx;
	return h->otf.file.name;
}

/* [hot callback] Write single byte to output file (errors checked on close) */
static void otfWrite1(void *ctx, int c) {
	cbCtx h = ctx;
	fileWrite1(&h->otf.file, c);
}

/* [hot callback] Write multiple bytes to output file (errors checked on
   close) */
static void otfWriteN(void *ctx, long count, char *ptr) {
	cbCtx h = ctx;
	fileWriteN(&h->otf.file, count, ptr);
}

/* [hot callback] Return current file position */
static long otfTell(void *ctx) {
	cbCtx h = ctx;
	return fileTell(&h->otf.file);
}

/* [hot callback] Seek to offset */
static void otfSeek(void *ctx, long offset) {
	cbCtx h = ctx;
	fileSeek(&h->otf.file, offset, SEEK_SET);
}

/* [hot callback] Refill data buffer from file */
static char *otfRefill(void *ctx, long *count) {
	cbCtx h = ctx;
	*count = fileReadN(&h->otf.file, BUFSIZ, h->otf.buf);
	return h->otf.buf;
}

/* -------------------------- Feature file input --------------------------- */

/* Returns mem-allocated path */
static char *findFeatInclFile(cbCtx h, char *filename) {
	char path[FILENAME_MAX + 1];
	char *fullpath;

	path[0] = '\0';
	if (!filename) {
		return NULL;
	}
    /* Check if relative path */
	if (filename[0] != '/') {
		int i;
        /* Look first relative to to the current include directory. If
         not found, check relative to the main feature file.
         */
		for (i = 1; i >= 0; i--) {
			if ((h->feat.includeDir[i] != 0) &&
				(h->feat.includeDir[i][0] != '\0')) {
                sprintf(path, "%s%s%s", h->feat.includeDir[i], sep(), filename);
				if (fileExists(path)) {
					goto found;
				}
			}
		}
		return NULL;			/* Can't find include file (error) */
	}
    else
    {
		if (fileExists(filename)) {
			char *t = strcpy((char*)path, filename);
		}
        else
        {
            return NULL;			/* Can't find include file (error) */
        }
    }
found:
    { /* set the current include directory */
        char *p;
        char featDir[FILENAME_MAX];

        p = strrchr(path, sepch());
		if (p == NULL) {
            /* if there are no directory separators, it is in the main feature file parent dir */
            if (h->feat.includeDir[1] != 0)
                cbMemFree(h, h->feat.includeDir[1]);
		}
        else
        {
        strncpy(featDir, path, p - path);
        featDir[p - path] = '\0';
        if (h->feat.includeDir[1] != 0)
            cbMemFree(h, h->feat.includeDir[1]);
        copyStr(h, &h->feat.includeDir[1], featDir);
        }
    }
	copyStr(h, &fullpath, (path[0] == '\0') ? filename : path);
	return fullpath;
}

/* [hot callback] Open feature file. (name == NULL) indicates main feature
   file. The full file name is returned. */
static char *featOpen(void *ctx, char *name, long offset) {
	cbCtx h = ctx;
	char *fullpath;

	if (name == NULL) {
		/* Main feature file */
		if (h->feat.mainFile != NULL) {
			if (!fileExists(h->feat.mainFile)) {
				cbFatal(h, "Specified feature file not found: %s \n", h->feat.mainFile);
				return NULL;		/* No feature file for this font */
			}
			copyStr(h, &fullpath, h->feat.mainFile);
            if (h->feat.includeDir[1] != 0)
            {
                cbMemFree(ctx, h->feat.includeDir[1]);
                h->feat.includeDir[1] = 0;
            }
 		}
		else {
			return NULL;		/* No feature file for this font */
		}
	}
    else if (offset == 0)
    {
        /* First time called, we get the path used in the feature file */
		fullpath = findFeatInclFile(h, name);
		if (fullpath == NULL) {
			return NULL;		/* Include file not found (error) */
		}
    }
	else {
        char * p;
		/* RE-opening file: name is full path. */
        copyStr(h, &fullpath, name);
        /* Determine dir that feature file's in */
            p = strrchr(fullpath, sepch());	/* xxx won't work for '\' delimiters */
            if (p == NULL) {
                cbMemFree(ctx, h->feat.includeDir[1]);
                h->feat.includeDir[1] = 0;
            }
            else {
                char featDir[FILENAME_MAX];
                strncpy(featDir, fullpath, p - fullpath);
                featDir[p - fullpath] = '\0';
                if (h->feat.includeDir[1] != 0)
                {
                    cbMemFree(ctx, h->feat.includeDir[1]);
                    h->feat.includeDir[1] = 0;
                }
                copyStr(h, &h->feat.includeDir[1], featDir);
            }
	}

	if (h->feat.file.name != NULL) {
		cbFatal(h, "previous feature file not closed\n");
	}

	fileOpen(&h->feat.file, h, fullpath, "rb");
	if (offset != 0) {
		fileSeek(&h->feat.file, offset, SEEK_SET);
	}
	return fullpath;
}

/* [hot callback] Refill data buffer from file */
static char *featRefill(void *ctx, long *count) {
	cbCtx h = ctx;
	*count = fileReadN(&h->feat.file, BUFSIZ, h->feat.buf);
	return (*count == 0) ? NULL : h->feat.buf;
}

/* [hot callback] Close feature file */
static void featClose(void *ctx) {
	cbCtx h = ctx;
	fileClose(&h->feat.file);
	cbMemFree(h, h->feat.file.name);	/* Alloc in featOpen() */
	h->feat.file.name = NULL;
}

#if 0
#define TAG_ARG(t) (char)((t) >> 24& 0xff), (char)((t) >> 16& 0xff), \
	(char)((t) >> 8& 0xff), (char)((t)& 0xff)

/* [hot callback] Refill anonymous table, selecting by tag */
static char *anonRefill(void *ctx, long *count, unsigned long tag) {
	cbCtx h = ctx;
	int i;

	for (i = 0; i < h->feat.anon.cnt; i++) {
		AnonInfo *ai = &h->feat.anon.array[i];

		if (ai->tag == tag) {
			*count = ai->refillDone++ ? 0 : ai->data.cnt;
			return (*count == 0) ? NULL : ai->data.array;
		}
	}

	cbFatal(h, "unrecognized anon table tag: %c%c%c%c\n", TAG_ARG(tag));
	return 0;			/* Supress compiler warning */
}

#endif

/* [hot callback] Add anonymous data from feature file */
static void featAddAnonData(void *ctx, char *data, long count,
							unsigned long tag) {
# if 0
	/* Sample code for adding anonymous tables */

	cbCtx h = ctx;
	AnonInfo *ai = dnaNEXT(h->feat.anon);
	long destCnt = (count == 0) ? 0 : count - 1;

	/* Store feature file data except for the last newline. */
	ai->tag = tag;
	ai->refillDone = 0;
	dnaSET_CNT(ai->data, destCnt);
	memcpy(ai->data.array, data, destCnt);

	/* Simply add it intact as table data! */
	hotAddAnonTable(h->hot.ctx, tag, anonRefill);
#endif
}

/* [hot callback] Open Unicose Variation Selector file. (name == NULL) indicates not supplied. The full file name is returned. */
static char *uvsOpen(void *ctx, char *name) {
	cbCtx h = ctx;

	if (name == NULL) {
		return NULL;		/* No uvs file for this font */
	}

	fileOpen(&h->uvs.file, h, name, "rb");
	return name;
}

/* [hot callback] Refill data buffer from file. ALways read just one line. */
static char *uvsGetLine(void *ctx,  char *buffer, long *count) {
	cbCtx h = ctx;
	char *buff;
	buff =  fgets(buffer, 255, h->uvs.file.fp);
	if (buff == NULL) {
		*count = 0;
	}
	else {
		*count = strlen(buff);
	}
	if (*count >= 254) {
		char *p;
		/* before echoing this line, look to see where the next new-line in any form is, and terminate the string there.*/
		p = strchr(buffer, '\n');
		if (p == NULL) {
			p = strchr(buffer, '\r');
		}
		if (p != NULL) {
			*p = '\0';
		}
		else {
			buff[64] = 0;
		}
		cbFatal(h, "Line in Unicode Variation Sequence does not end in a new-line.\n\tPlease check if the file type is correct. Line content:\n\t%s\n",  buffer);
	}


	return (buff == NULL) ? NULL : buffer;
}

/* [hot callback] Close feature file */
static void uvsClose(void *ctx) {
	cbCtx h = ctx;
	fileClose(&h->uvs.file);
}

/* --------------------------- Temporary file I/O -------------------------- */

/* On Windows, the stdio.h 'tmpfile' function tries to make temp files in the root
directory, thus requiring administrative privileges. So we first need to use '_tempnam'
to generate a unique filename inside the user's TMP environment variable (or the
current working directory if TMP is not defined). Then we open the temporary file
and return its pointer */
static FILE *_tmpfile()
	{
	FILE *fp = NULL;
#ifdef _WIN32
	char* tempname = NULL;
	int fd, flags, mode;
	flags = _O_BINARY|_O_CREAT|_O_EXCL|_O_RDWR|_O_TEMPORARY;
	mode = _S_IREAD | _S_IWRITE;
	tempname = _tempnam(NULL, "tx_tmpfile");
	if(tempname != NULL)
		{
		fd = _open(tempname, flags, mode);
		if (fd != -1)
			fp = _fdopen(fd, "w+b");
		free(tempname);
		}
#else
	/* Use the default tmpfile on non-Windows platforms */
	fp = tmpfile();
#endif
	return fp;
	}


/* [hot callback] Open temporary file */
static void tmpOpen(void *ctx) {
	cbCtx h = ctx;
	h->tmp.file.name = "tmpfile";
	if ((h->tmp.file.fp = _tmpfile()) == NULL) {
		fileError(&h->tmp.file);
	}
}

/* [hot callback] Write multiple bytes to temporary file */
static void tmpWriteN(void *ctx, long count, char *ptr) {
	cbCtx h = ctx;
	fileWriteN(&h->tmp.file, count, ptr);
}

/* [hot callback] Rewind temporary file */
static void tmpRewind(void *ctx) {
	cbCtx h = ctx;
	fileSeek(&h->tmp.file, 0, SEEK_SET);
}

/* [hot callback] Refill temporary file input buffer */
static char *tmpRefill(void *ctx, long *count) {
	cbCtx h = ctx;
	*count = fileReadN(&h->tmp.file, BUFSIZ, h->tmp.buf);
	return h->tmp.buf;
}

/* [hot callback] Close temporary file */
static void tmpClose(void *ctx) {
	cbCtx h = ctx;

	if (h->tmp.file.fp != NULL) {
		fileClose(&h->tmp.file);
	}

# if CID_CFF_DEBUG	/* turn this on to write CFF file, for debugging CID CFF creation */
	if (h->tmp.file.name != NULL) {
		FILE *tmp_fp;
		size_t n;
		char str[1024];

		sprintf(str, "Saving temp CFF file to: %s%s", h->tmp.file.name, ".cff");
		message(h, hotNOTE, str);

		sprintf(str, "%s%s", h->tmp.file.name, ".cff");
		tmp_fp = fopen(str, "wb");
		if (tmp_fp == NULL) {
			cbFatal(h, "file error <%s> [%s]", strerror(errno), str);
		}

		n = fwrite(h->cff.buf.array, 1, h->cff.buf.cnt, tmp_fp);
		if (n == 0 && ferror(tmp_fp)) {
			cbFatal(h, "file error <%s> [%s]", strerror(errno), str);
		}

		fclose(tmp_fp);
	}
#endif
}

/* ------------------------------- CMap input ------------------------------ */

/* [hot callback] Return CMap filename */
static char *CMapId(void *ctx) {
	cbCtx h = ctx;
	return h->CMap.file.name;
}

/* [hot callback] Refill CMap file input buffer */
static char *CMapRefill(void *ctx, long *count) {
	cbCtx h = ctx;
	*count = fileReadN(&h->CMap.file, BUFSIZ, h->CMap.buf);
	return h->CMap.buf;
}

/* Open CMap file */
static void CMapOpen(void *ctx, char *filename) {
	cbCtx h = ctx;
	fileOpen(&h->CMap.file, h, filename, "rb");
}

/* Close CMap file */
static void CMapClose(void *ctx) {
	cbCtx h = ctx;
	fileClose(&h->CMap.file);
}

// ------------------------ Font Conversion Database -----------------------

// [fcdb callback] Refill current database input file buffer.
static char *fcdbRefill(void *ctx, unsigned fileid, size_t *count) {
	cbCtx h = ctx;
	File *file = &h->fcdb.files.array[fileid];
	*count = fileReadN(file, BUFSIZ, h->fcdb.buf);
	if (*count == 0) {
		// Signal EOF
		h->fcdb.buf[0] = '\0';
		*count = 1;
	}
	return h->fcdb.buf;
}

// [fcdb callback] Get record from database file at given offset and length.
static void fcdbGetBuf(void *ctx,
					   unsigned fileid, long offset, long length, char *buf) {
	cbCtx h = ctx;
	File *file = &h->fcdb.files.array[fileid];
	fileSeek(file, offset, SEEK_SET);
	if (fileReadN(file, length, buf) != length)
		fileError(file);
}

// [fcdb callback] Report database parsing warning.
static void fcdbError(void *ctx, unsigned fileid, long line, int errid) {
	cbCtx h = ctx;
	static char *msgs[fcdbErrCnt] =
	{
		"syntax error",
		"duplicate record",
		"record key length too long",
		"bad name id range",
		"bad code range",
		"empty name",
		"Compatible Full name may be specified only for the Mac platform.",
		"Both version 1 and version 2 syntax is present in the Font Menu Name DB file: name table font menu names may be in error."
	};
	cbWarning(h, "%s [%s:%d] (record skipped) (fcdbError)",
			  msgs[errid], h->fcdb.files.array[fileid].name, line);
}

// [fcdb callback] Add name from requested font record.
static int fcdbAddName(void *ctx,
					   unsigned short platformId, unsigned short platspecId,
					   unsigned short languageId, unsigned short nameId,
					   char *str) {
	cbCtx h = ctx;
	return hotAddName(h->hot.ctx,
					  platformId, platspecId, languageId, nameId, str);
}

#undef TRY_LINKS
#ifdef TRY_LINKS
// [fcdb callback] Add style link from requested font record.
void fcdbAddLink(void *ctx, int style, char *fontname) {
	printf("link: %s=%s\n", (style == fcdbStyleBold)? "Bold":
		   (style == fcdbStyleItalic)? "Italic" : "Bold Italic", fontname);
}
#endif

// [fcdb callback] Add encoding from requested font record.
static void fcdbAddEnc(void *ctx, int code, char *gname) {
	cbCtx h = ctx;
	if (h->mac.encoding == NULL) {
		// Initialize encoding accumulator
		int i;
		for (i = 0; i < 256; i++) {
			h->fcdb.macenc[i] = ".notdef";
		}
		h->mac.encoding = &h->fcdb.macenc;
	}
	h->fcdb.macenc[code] = gname;
}

static void fcdSetMenuVersion(void *ctx, unsigned fileid, unsigned short syntaxVersion) {
	cbCtx h = ctx;
	if ((syntaxVersion != 1) && (syntaxVersion != 2)) {
		cbFatal(h, "Syntax version of the Font Menu Name Db file %s is not recognized.",
				h->fcdb.files.array[fileid].name);
	}
	else {
		h->fcdb.syntaxVersion = syntaxVersion;
	}
}

/* ----------------------- Glyph Name Alias Database  ---------------------- */

static void gnameError(cbCtx h, char *message, char *filename, long line) {
	long messageLen = strlen(message) + strlen(filename);

	if (messageLen > (256 - 5)) {
		/* 6 is max probable length of decimal line number in Glyph Name Alias Database file */
		cbWarning(h, "Glyph Name Alias Database error message [%s:$d] + file name is too long. Please move Database file to shorter absolute path.", message, line);
	}
	else {
		cbWarning(h, "%s [%s:%d] (record skipped)(gnameError)", message, filename, line);
	}
}

/* Validate glyph name and return a NULL pointer if the name failed to validate
   else return a pointer to character that stopped the scan. */

/* Next state table */
static unsigned char nextFinal[3][4] = {
    /*  A-Za-z_	0-9		.		*		index  */
    /* -------- ------- ------- ------- ------ */
    {	1,		0,		2,		0 },	/* [0] */
    {	1,		1,		1,		0 },	/* [1] */
    {	1,		2,		2,		0 },	/* [2] */
};

/* Action table */
#define	Q_	(1 << 0)	/* Quit scan on unrecognized character */
#define	E_	(1 << 1)	/* Report syntax error */

static unsigned char actionFinal[3][4] = {
    /*  A-Za-z_	0-9		.		*		index  */
    /* -------- ------- ------- ------- ------ */
    {	0,		E_,		0,		Q_ },	/* [0] */
    {	0,		0,		0,		Q_ },	/* [1] */
    {	0,		0,		0,		E_ },	/* [2] */
};

/* Allow glyph names to start with numbers. */
static unsigned char actionDev[3][4] = {
    /*  A-Za-z_	0-9		.		*		index  */
    /* -------- ------- ------- ------- ------ */
    {	0,		0,		0,		Q_ },	/* [0] */
    {	0,		0,		0,		Q_ },	/* [1] */
    {	0,		0,		0,		E_ },	/* [2] */
};

enum {
    finalName,
    sourceName,
    uvName
};

static char *gnameScan(cbCtx h, char *p, unsigned char* action, unsigned char* next, int nameType) {

	char *start = p;
	int state = 0;

	for (;;) {
		int actn;
		int class;
		int c = *p;

		/* Determine character class */
		if (isalpha(c) || c == '_') {
			class = 0;
		}
		else if (isdigit(c)) {
			class = 1;
		}
        else if (c == '.') {
            class = 2;
        }
        else if ((nameType==sourceName) && (c == '.' || c == '+' || c == '*' || c == ':' || c == '~' || c == '^' || c == '!' || c == '-')) {
            class = 2;
        }
        else if ((nameType==uvName) && (c == ',')) {
            class = 2;
        }
		else {
			class = 3;
		}

		/* Fetch action and change state */
		actn = (int)(action[state*4 + class]);
		state = (int)(next[state*4 + class]);

		/* Performs actions */
		if (actn == 0) {
			p++;
			continue;
		}
		if (actn & E_) {
			return NULL;
		}
		if (actn & Q_) {
			if (p - start > MAX_CHAR_NAME_LEN) {
				return NULL;	/* Maximum glyph name length exceeded */
			}
			return p;
		}
	}
}

static char* gnameDevScan(cbCtx h, char *p) {
    char *val = gnameScan(h, p, (unsigned char*)actionDev, (unsigned char*)nextFinal, sourceName);
    return val;
}

static char* gnameFinalScan(cbCtx h, char *p) {
    char *val = gnameScan(h, p, (unsigned char*)actionFinal, (unsigned char*)nextFinal, finalName);
    return val;
}

static char* gnameUVScan(cbCtx h, char *p) {
    char *val = gnameScan(h, p, (unsigned char*)actionFinal, (unsigned char*)nextFinal, uvName);
    return val;
}


/* Match alias name record. */
static int CDECL matchAliasRec(const void *key, const void *value) {
	AliasRec *alias = (AliasRec *)value;
	struct cbCtx_ *h = (cbCtx)key;
	char *aliasString;

	aliasString = &h->alias.names.array[alias->iKey];
	return strcmp(h->matchkey, (const char *)aliasString);
}

static int CDECL matchAliasRecByFinal(const void *key, const void *value) {
    AliasRec *finalAlias = (AliasRec *)value;
    struct cbCtx_ *h = (cbCtx)key;
    char *finalString;

    finalString = &h->alias.names.array[finalAlias->iFinal];
    return strcmp(h->matchkey, (const char *)finalString);
}


static int CDECL cmpAlias(const void *first, const void *second, void *ctx) {
	struct cbCtx_ *h = (cbCtx)ctx;
	AliasRec *alias1 = (AliasRec *)first;
	AliasRec *alias2 = (AliasRec *)second;

	return strcmp(&h->alias.names.array[alias1->iKey], &h->alias.names.array[alias2->iKey]);
}

static int CDECL cmpFinalAlias(const void *first, const void *second, void *ctx) {
	struct cbCtx_ *h = (cbCtx)ctx;
	AliasRec *alias1 = (AliasRec *)first;
	AliasRec *alias2 = (AliasRec *)second;

	return strcmp(&h->alias.names.array[alias1->iFinal], &h->alias.names.array[alias2->iFinal]);
}

/* Parse glyph name aliasing file.

   Glyph Name Aliasing Database File Format
   ========================================
   The glyph alias database uses a simple line-oriented ASCII format.

   Lines are terminated by either a carriage return (13), a line feed (10), or
   a carriage return followed by a line feed. Lines may not exceed 255
   characters in length (including the line terminating character(s)).

   Any of the tab (9), vertical tab (11), form feed (12), or space (32)
   characters are regarded as "blank" characters for the purposes of this
   description.

   Empty lines, or lines containing blank characters only, are ignored.

   Lines whose first non-blank character is hash (#) are treated as comment
   lines which extend to the line end and are ignored.

   All other lines must contain two glyph names separated by one or more blank
   characters. A working glyph name may be up to 63 characters in length,  a
   final character name may be up to 32 characters in length, and may only
   contain characters from the following set:

   A-Z  capital letters
   a-z	lowercase letters
   0-9	figures
    _	underscore
    .   period
    -	hyphen
    +   plus
    *   asterisk
    :   colon
    ~   tilde
    ^   asciicircum
    !   exclam

   A glyph name may not begin with a figure or consist of a period followed by
   a sequence of digits only.

   The first glyph name on a line is a final name that is stored within an
   OpenType font and second glyph name is an alias for the final name. The
   second name may be optionally followed a comment beginning with a hash (#)
   character and separated from the second name by one or more blank
   characters.

   If a final name has more that one alias it may be specified on a separate
   line beginning with the same final name.

   An optional third field may contain a glyph name in the form of 'u' + hexadecimal
   Unicode value. This will override Makeotf's default heuristics for assigning a
   UV, including any UV suggested by the form of the final name.

   All alias names must be
   unique. */

#define maxLineLen 1024

void cbAliasDBRead(cbCtx h, char *filename) {
	File file;
	long lineno;
	long iOrder = -1;
	char buf[maxLineLen];

	h->alias.recs.cnt = 0;
	h->final.recs.cnt = 0;

	fileOpen(&file, h, filename, "r");
	for (lineno = 1; fileGetLine(&file, buf, maxLineLen) != NULL; lineno++) {
		int iNL = strlen(buf) - 1;
        char *final;
        char *alias;
        char *uvName;
        char *p = buf;

        /* Skip blanks */
        while (isspace(*p)) {
            p++;
        }

        if (*p == '\0' || *p == '#') {
            continue;	/* Skip blank or comment line */
        }
        
        if (buf[iNL] != '\n') {
            cbFatal(h, "GlyphOrderAndAliasDB line is longer than limit of %d characters. [%s line number: %d]\n", maxLineLen, filename, lineno);
        }
        
        iOrder++;
        /* Parse final name */
        final = p;
        p = gnameFinalScan(h, final);
        if (p == NULL || !isspace(*p)) {
            goto syntaxError;
        }
        *p = '\0';
        if (strlen(final) > MAX_FINAL_CHAR_NAME_LEN) {
            cbWarning(h, "final name %s is longer (%d) than limit %d, in %s line %d.\n", final, strlen(final), MAX_FINAL_CHAR_NAME_LEN, filename, lineno);
        }

        /* Skip blanks */
        do {
            p++;
        }
        while (isspace(*p));

        /* Parse alias name */
        alias = p;
        p = gnameDevScan(h, alias);
        if (p == NULL || !isspace(*p)) {
            goto syntaxError;
        }
        *p = '\0';
        if (strlen(alias) > MAX_CHAR_NAME_LEN) {
            cbWarning(h, "alias name %s is longer  (%d) than limit %d, in %s line %d.\n", alias, strlen(alias), MAX_CHAR_NAME_LEN, filename, lineno);
        }

        /* Skip blanks. Since line is null terminated, will not go past end of line. */
        do {
            p++;
        }
        while (isspace(*p));

        /* Parse uv override name */
        /* *p is either '\0' or '#' or a uv-name.  */
        uvName = p;
        if (*p != '\0') {
            if (*p == '#') {
                *p = '\0';
            }
            else {
                uvName = p;
                p = gnameUVScan(h, uvName);
                if (p == NULL || !isspace(*p)) {
                    goto syntaxError;
                }
                *p = '\0';
            }
        }

        if (*p == '\0' || *p == '#') {
            size_t index, finalIndex;
            AliasRec *previous;

            h->matchkey = alias;

            /* build sorted list of alias names */
            if (bsearch(h, h->alias.recs.array, h->alias.recs.cnt,
                            sizeof(AliasRec), matchAliasRec)) {
                gnameError(h, "duplicate name", filename, lineno);
                continue;
            }
            else {
                index = h->alias.recs.cnt;
            }

            /* local block - NOT under closest if statement */
            {
                /* Add string */
                long length;
                AliasRec *new = &dnaGROW(h->alias.recs, h->alias.recs.cnt)[index];

                /* Make hole */
                memmove(new + 1, new, sizeof(AliasRec) * (h->alias.recs.cnt++ - index));

                /* Fill record */
                new->iKey = h->alias.names.cnt;
                length = strlen(alias) + 1;
                memcpy(dnaEXTEND(h->alias.names, length), alias, length);
                new->iFinal = h->alias.names.cnt;
                length = strlen(final) + 1;
                memcpy(dnaEXTEND(h->alias.names, length), final, length);
                new->iUV = h->alias.names.cnt;
                length = strlen(uvName) + 1;
                memcpy(dnaEXTEND(h->alias.names, length), uvName, length);
                new->iOrder = iOrder;
            }


            /* build sorted list of final names */
            h->matchkey = final;
            previous = bsearch(h, h->final.recs.array, h->final.recs.cnt,
                               sizeof(AliasRec), matchAliasRecByFinal);
            if (previous) {
                char *previousUVName;
                previousUVName = &h->alias.names.array[previous->iUV];

                if (strcmp(previousUVName,uvName)) {
                    gnameError(h, "duplicate final name, with different uv ovveride", filename, lineno);
                }
                    continue; /* it is not an error to have more than one final name entry, but we don;t want to entry duplicates in the search array */
            }
            else {
                finalIndex = h->final.recs.cnt;
            }

            /* local block - NOT under closest if statement. If we get here, both alias and final names are new. */
            {
                /* Add string */
                AliasRec *newFinal;
                AliasRec *newAlias = &h->alias.recs.array[index];
                newFinal = &dnaGROW(h->final.recs, h->final.recs.cnt)[finalIndex];

                /* Make hole */
                memmove(newFinal + 1, newFinal, sizeof(AliasRec) * (h->final.recs.cnt++ - finalIndex));

                /* Fill record */
                newFinal->iKey = newAlias->iKey;
                newFinal->iFinal = newAlias->iFinal;
                newFinal->iUV = newAlias->iUV;
                newFinal->iOrder = newAlias->iOrder;
            }
        }		/* end if *p == \0 */

        continue;	/* avoid final syntaxError */

syntaxError:
        gnameError(h, "syntax error", filename, lineno);
    }
	fileClose(&file);
	ctuQSort(h->alias.recs.array, h->alias.recs.cnt, sizeof(AliasRec),
			 cmpAlias, h);
	ctuQSort(h->final.recs.array, h->final.recs.cnt, sizeof(AliasRec),
			 cmpFinalAlias, h);

#if 0	/* xxx remove when fully tested */
	{
		int i;
		for (i = 0; i < h->alias.recs.cnt; i++) {
			AliasRec *alias = &h->alias.recs.array[i];
			printf("%s	%s %d\n",
				   &h->alias.names.array[alias->iFinal],
				   &h->alias.names.array[alias->iKey], alias->iOrder);
		}
	}
#endif
}

/* used to overide AliasDB when -q option is used: Usage scenario:
   default options are read in and processed from the project file, then
   the user overrides -r with -q. */
void cbAliasDBCancel(cbCtx h) {
	h->alias.names.cnt = 0;
	h->alias.recs.cnt = 0;
	h->final.recs.cnt = 0;
}

/* [hot callback] Convert alias name to final name. */
static char *getFinalGlyphName(void *ctx, char *gname) {
    cbCtx h = ctx;
    AliasRec *alias;
    h->matchkey = gname;
    alias = (AliasRec *)bsearch(h, h->alias.recs.array, h->alias.recs.cnt,
                                sizeof(AliasRec), matchAliasRec);
    return (alias == NULL) ? gname : &(h->alias.names.array[alias->iFinal]);
}

/* [hot callback] Convert final name to src name. */
static char *getSrcGlyphName(void *ctx, char *gname) {
    cbCtx h = ctx;
    AliasRec *alias;
    h->matchkey = gname;
    alias = (AliasRec *)bsearch(h, h->final.recs.array, h->final.recs.cnt,
                                sizeof(AliasRec), matchAliasRecByFinal);
    return (alias == NULL) ? gname : &(h->alias.names.array[alias->iKey]);
}


/* [hot callback] Get UV override in form of u<UV Code> glyph name. */
static char *getUVOverrideName(void *ctx, char *gname) {
	cbCtx h = ctx;
	AliasRec *alias;
	char *uvName = NULL;

	if (h->alias.recs.cnt == 0) {
		return NULL;
	}

	/* Assume that  gname is an alias name from the GAODB. */
	h->matchkey = gname;
	if (h->alias.useFinalNames) {
		/* Assume that gname is an final name from the GAODB. */
		alias = (AliasRec *)bsearch(h, h->final.recs.array, h->final.recs.cnt,
									sizeof(AliasRec), matchAliasRecByFinal);
		if (alias != NULL) {
			uvName =  &h->alias.names.array[alias->iUV];
			if (*uvName == '\0') {
				uvName = NULL;
			}
		}
	}
	else {
		alias = (AliasRec *)bsearch(h, h->alias.recs.array, h->alias.recs.cnt,
									sizeof(AliasRec), matchAliasRec);
		if (alias != NULL) {
			uvName =  &h->alias.names.array[alias->iUV];
			if (*uvName == '\0') {
				uvName = NULL;
			}
		}
	}



	return uvName;
}

/* [hot callback] Get alias name and order. */
static void getAliasAndOrder(void *ctx, char *oldName, char **newName, long int *order) {
	cbCtx h = ctx;
	AliasRec *alias;
	h->matchkey = oldName;
	alias = (AliasRec *)bsearch(h, h->alias.recs.array, h->alias.recs.cnt,
								sizeof(AliasRec), matchAliasRec);
	if (alias != NULL) {
		*newName = &h->alias.names.array[alias->iFinal];
		*order = alias->iOrder;
	}
	else {
		/* if it wasn't a "friendly" name, maybe it was already a final name */
		alias = (AliasRec *)bsearch(h, h->final.recs.array, h->final.recs.cnt,
									sizeof(AliasRec), matchAliasRecByFinal);
		if (alias != NULL) {
			*newName = &h->alias.names.array[alias->iFinal];
			*order = alias->iOrder;
		}
		else {
			*newName = NULL;
			*order = -1;
		}
	}
}

/* ---------------------------- Callback Context --------------------------- */

static void anonInit(void *ctx, long count, AnonInfo *ai) {
	dnaCtx localDnaCtx = (dnaCtx)ctx;
	long i;
	for (i = 0; i < count; i++) {
		dnaINIT(localDnaCtx, ai->data, 1, 3);	/* xxx */
		ai++;
	}
	return;
}

/* Create new callback context */
cbCtx cbNew(char *progname, char *pfbdir, char *otfdir,
			char *cmapdir, char *featdir, dnaCtx mainDnaCtx) {
	static hotCallbacks template = {
		NULL,		/* Callback context; set after creation */
		myfatal,
		message,
		CBcbMemNew,
		CBcbMemResize,
		CBcbMemFree,
		psId,
		psRefill,
		cffId,
		cffWrite1,
		cffWriteN,
		cffSeek,
		cffRefill,
		cffSize,
		otfId,
		otfWrite1,
		otfWriteN,
		otfTell,
		otfSeek,
		otfRefill,
		featOpen,
		featRefill,
		featClose,
		featAddAnonData,
		tmpOpen,
		tmpWriteN,
		tmpRewind,
		tmpRefill,
		tmpClose,
        getFinalGlyphName,
        getSrcGlyphName,
 		getUVOverrideName,
		getAliasAndOrder,
		uvsOpen,
		uvsGetLine,
		uvsClose,
	};
	fcdbCallbacks fcdbcb;
	cbCtx h = malloc(sizeof(struct cbCtx_));
	if (h == NULL) {
		fprintf(stderr, "%s [FATAL]: out of memory\n", progname);
		exit(1);	/* Could also longjmp back to caller from here */
	}


	/* Initialize context */
	h->progname = progname;
	h->dir.pfb = pfbdir;
	h->dir.otf = otfdir;
	h->dir.cmap = cmapdir;

	h->hot.cb = template;	/* Copy template */
	h->hot.cb.ctx = h;

	h->dnaCtx = mainDnaCtx;

	dnaINIT(mainDnaCtx, h->cff.buf, 50000, 150000);
	h->cff.euroAdded = 0;
	h->feat.includeDir[0] = 0;
	h->feat.includeDir[1] = 0;
	h->feat.includeDir[2] = 0;
	h->feat.includeDir[3] = 0;
	h->feat.file.name = NULL;
	dnaINIT(mainDnaCtx, h->feat.anon, 1, 3);	/* xxx */
	h->feat.anon.func = anonInit;
	h->hot.ctx = hotNew(&h->hot.cb);
	dnaINIT(mainDnaCtx, h->tmpbuf, 32, 32);
	h->mac.encoding = NULL;
	h->mac.cmapScript = HOT_CMAP_UNKNOWN;
	h->mac.cmapLanguage = HOT_CMAP_UNKNOWN;

	h->tmp.file.fp = NULL;	/*CFFDBG part of hack to print out temp cff file for CID fonts */
	h->tmp.file.name = NULL;/*CFFDBG part of hack to print out temp cff file for CID fonts */

	/* Initialize font conversion database */
	fcdbcb.ctx = h;
	fcdbcb.refill = fcdbRefill;
	fcdbcb.getbuf = fcdbGetBuf;
	fcdbcb.addname = fcdbAddName;
#ifdef TRY_LINKS
	fcdbcb.addlink  = fcdbAddLink;
#else
	fcdbcb.addlink  = NULL;
	   #endif
	fcdbcb.addenc = fcdbAddEnc;
	fcdbcb.error = fcdbError;
	fcdbcb.setMenuVersion = fcdSetMenuVersion;

	h->fcdb.ctx = fcdbNew(&fcdbcb, mainDnaCtx);
	h->fcdb.syntaxVersion = 0;

	dnaINIT(mainDnaCtx, h->fcdb.files, 5, 5);

	dnaINIT(mainDnaCtx, h->alias.recs, 700, 200);
	dnaINIT(mainDnaCtx, h->alias.names, 15000, 5000);
	dnaINIT(mainDnaCtx, h->final.recs, 700, 200);

	return h;
}

/* Add Adobe CMap to conversion */
static void addCMap(cbCtx h, char *cmapfile) {
	if (cmapfile != NULL) {
		char cmappath[FILENAME_MAX + 1];

		sprintf(cmappath, "%s%s", h->dir.cmap, cmapfile);

		CMapOpen(h, cmappath);
		hotAddCMap(h->hot.ctx, CMapId, CMapRefill);
		CMapClose(h);
	}
}

/* Make OTF pathname */
static void makeOTFPath(cbCtx h, char *otfpath, char *FontName) {
	int length = strlen(FontName);

	if (length > 27) {
		/* FontName too long for Mac if used as filename; shorten it */
		typedef struct {
			char *style;
			char *abrev;
		} Rule;
		static Rule rules[] =	/* Taken from ADS TN#5088 (shortest first) */
		{
			{ "Bold",		 "Bd" },
			{ "Book",		 "Bk" },
			{ "Demi",		 "Dm" },
			{ "Nord",		 "Nd" },
			{ "Semi",		 "Sm" },
			{ "Thin",		 "Th" },
			{ "Black",		 "Blk" },
			{ "Extra",		 "X" },
			{ "Heavy",		 "Hv" },
			{ "Light",		 "Lt" },
			{ "Roman",		 "Rm" },
			{ "Super",		 "Su" },
			{ "Swash",		 "Sw" },
			{ "Ultra",		 "Ult" },
			{ "Expert",		 "Exp" },
			{ "Italic",		 "It" },
			{ "Kursiv",		 "Ks" },
			{ "Medium",		 "Md" },
			{ "Narrow",		 "Nr" },
			{ "Poster",		 "Po" },
			{ "Script",		 "Scr" },
			{ "Shaded",		 "Sh" },
			{ "Sloped",		 "Sl" },
			{ "Compact",	 "Ct" },
			{ "Display",	 "DS" },
			{ "Oblique",	 "Obl" },
			{ "Outline",	 "Ou" },
			{ "Regular",	 "Rg" },
			{ "Rounded",	 "Rd" },
			{ "Slanted",	 "Sl" },
			{ "Titling",	 "Ti" },
			{ "Upright",	 "Up" },
			{ "Extended",	 "Ex" },
			{ "Inclined",	 "Ic" },
			{ "Alternate",	 "Alt" },
			{ "Condensed",	 "Cn" },
			{ "Oldestyle",	 "OS" },
			{ "Ornaments",	 "Or" },
			{ "Compressed",	 "Cm" },
		};

		cbWarning(h, "filename too long [%s] (editing)", FontName);

		/* Size buffer and make copy */
		strcpy(dnaGROW(h->tmpbuf, length), FontName);

		do {
			unsigned int i;
			for (i = 0; i < sizeof(rules) / sizeof(rules[0]); i++) {
				Rule *rule = &rules[i];
				char *p = strstr(h->tmpbuf.array, rule->style);
				if (p != NULL) {
					/* Found match */
					int stylelen = strlen(rule->style);
					int abrevlen = strlen(rule->abrev);
					int shrinklen = stylelen - abrevlen;

					/* Edit FontName */
					memmove(p, rule->abrev, abrevlen);
					memmove(p + abrevlen, p + stylelen,
							length - (p - h->tmpbuf.array) - shrinklen);

					length -= shrinklen;
					goto matched;
				}
			}
			cbWarning(h, "filename too long [%s] (truncating)", FontName);
			length = 27;
matched:
			;
		}
		while (length > 27);

		h->tmpbuf.array[length] = '\0';
		sprintf(otfpath, "%s%s.otf", h->dir.otf, h->tmpbuf.array);
	}
	else {
		sprintf(otfpath, "%s%s.otf", h->dir.otf, FontName);
	}
}

/* 5 functions below were formerly part of sun.c*/

static char *parseInstance(char *rawstring, char *finalstring, int nAxes) {
	char *p;
	int i = 0;
	int a;

	p = rawstring;
	finalstring[0] = '\0';
	p = strchr(p++, '[');
	for (a = 0; a < nAxes; a++) {
		while (isdigit(*p)) { /* axis value */
			finalstring[i++] = *p++;
		}

		p = strchr(p++, '(');
		if (*p == ' ') {
			p++;
		}
		finalstring[i++] = ' ';	/* space */
		finalstring[i++] = *p++;/* letter */
		finalstring[i++] = *p++;/* letter */
		finalstring[i++] = ' ';	/* space */
		while ((*p == ')') || (*p == ' ')) {
			p++;
		}
	}
	p = strchr(p++, ']');

	if (finalstring[i - 1] == ' ') {
		finalstring[i - 1] = '\0';
	}
	finalstring[i] = '\0';
	return p;
}

/* instanceName is of the form:  "300 LT 440 NR"
   regCoords is of the form: "[399 110]"
 */
static int isRegularInstance(char *instanceName, char *regCoords, int nAxes) {
	char inst[HOT_MAX_MENU_NAME];
	char reg[HOT_MAX_MENU_NAME];
	char *p;
	int reg1, reg2, reg3, reg4;
	int in1, in2, in3, in4;

	strcpy(reg, regCoords);
	p = reg;
	while (*p) {
		if (!isdigit(*p)) {
			*p = ' ';
		}
		p++;
	}

	strcpy(inst, instanceName);
	p = inst;
	while (*p) {
		if (!isdigit(*p)) {
			*p = ' ';
		}
		p++;
	}

	sscanf(reg, "%d %d %d %d", &reg1, &reg2, &reg3, &reg4);

	sscanf(inst, "%d %d %d %d", &in1, &in2, &in3, &in4);

	switch (nAxes) {
		case 4:
			if (reg4 != in4) {
				return 0;
			}
			/* nobreak */
			
		case 3:
			if (reg3 != in3) {
				return 0;
			}
			/* nobreak */
			
		case 2:
			if (reg2 != in2) {
				return 0;
			}
			/* nobreak */
			
		case 1:
			if (reg1 != in1) {
				return 0;
			}
			/* nobreak */
	}

	return 1;
}

static void parseStyles(char *stylestring, int *point0, int *delta0, int *point1, int *delta1) {
	char *p;
	char str[HOT_MAX_MENU_NAME];
	int n;

	strcpy(str, stylestring);
	p = str;
	while (*p) {
		if (!isdigit(*p)) {
			*p = ' ';
		}
		p++;
	}

	*point0 = *delta0 = *point1 = *delta1 = 0;
	n = sscanf(str, "%d %d %d %d", point0, delta0, point1, delta1);
}

/*This used to be handled in sun.c but is now platform independent in this Python version.
 */

static void ProcessFontInfo(hotCtx g, char *version, char *FontName, int psinfo,
							hotMacData *mac, long otherflags, short fsSelectionMask_on, short fsSelectionMask_off,
							unsigned short os2Version, char *licenseID) {
	hotWinData win;
	hotCommonData common;


	win.Family = WINDOWS_ROMAN;/* This is not currently used by the hot lib; it currently always sets
								   OS2.sFamily to "undefined". */
	win.CharSet = WIN_NONSYMBOLCHARSET;
	win.DefaultChar = WIN_SPACE;	/* We don't have any fonts that use the bullet as the .notdef. */
	win.BreakChar = WIN_SPACE;

	/* Create flags */
	common.flags = HOT_WIN | (psinfo & HOT_EURO_ADDED);

	if (otherflags & OTHERFLAGS_ISWINDOWSBOLD) {
		common.flags |= HOT_BOLD;
	}
	if (otherflags & OTHERFLAGS_ISITALIC) {
		common.flags |= HOT_ITALIC;
	}
	if (otherflags & OTHERFLAGS_DOUBLE_MAP_GLYPHS) {
		common.flags |= HOT_DOUBLE_MAP_GLYPHS;
	}

	common.nKernPairs = 0;
	common.clientVers = version;
	common.licenseID = licenseID;
	common.nStyles = 0;
	common.nInstances = 0;
	common.encoding = NULL;

	common.fsSelectionMask_on = fsSelectionMask_on;
	common.fsSelectionMask_off = fsSelectionMask_off;
	common.os2Version = os2Version;
	hotAddMiscData(g, &common, &win, mac);
}

/* Convert font */
void cbConvert(cbCtx h, int flags, char *clientVers,
			   char *pfbfile, char *otffile,
			   char *featurefile, char *hcmapfile, char *vcmapfile, char *mcmapfile, char *uvsFile,
			   long otherflags, short macScript, short macLanguage,
			   long addGlyphWeight, unsigned long maxNumSubrs,
			   short fsSelectionMask_on, short fsSelectionMask_off,
			   unsigned short os2Version, char *licenseID) {
	int psinfo;
	int type;
	char *p;
	char *FontName;
	char pfbpath[FILENAME_MAX + 1];
	char otfpath[FILENAME_MAX + 1];
	int freeFeatName = 0;
	unsigned long hotConvertFlags = 0;
	int releasemode = otherflags & OTHERFLAGS_RELEASEMODE;


	if (otherflags & OTHERFLAGS_DO_ID2_GSUB_CHAIN_CONXT) {
		hotConvertFlags |= HOT_ID2_CHAIN_CONTXT3;
	}

	if (otherflags & OTHERFLAGS_ALLOW_STUB_GSUB) {
		hotConvertFlags |= HOT_ALLOW_STUB_GSUB;
	}
	if (otherflags & OTHERFLAGS_OLD_SPACE_DEFAULT_CHAR) {
		hotConvertFlags |= HOT_OLD_SPACE_DEFAULT_CHAR;
	}

	if (h->fcdb.syntaxVersion == 1) {
		hotConvertFlags |= HOT_USE_V1_MENU_NAMES;
	}

        if (otherflags & OTHERFLAGS_OLD_NAMEID4)
            hotConvertFlags |= HOT_USE_OLD_NAMEID4;
	if (otherflags & OTHERFLAGS_OMIT_MAC_NAMES) {
		hotConvertFlags |= HOT_OMIT_MAC_NAMES;
	}
    
    
	if (otherflags & OTHERFLAGS_STUB_CMAP4) {
		hotConvertFlags |= HOT_STUB_CMAP4;
	}
	if (otherflags & OTHERFLAGS_OVERRIDE_MENUNAMES) {
		hotConvertFlags |= HOT_OVERRIDE_MENUNAMES;
	}
    if (otherflags & OTHERFLAGS_DO_NOT_OPTIMIZE_KERN) {
        hotConvertFlags |= HOT_DO_NOT_OPTIMIZE_KERN;
    }
    if (otherflags & OTHERFLAGS_ADD_STUB_DSIG) {
        hotConvertFlags |= HOT_ADD_STUB_DSIG;
    }
    
    if (otherflags & OTHERFLAGS_VERBOSE) {
        hotConvertFlags |= HOT_CONVERT_VERBOSE;
    }
    
    if (otherflags & OTHERFLAGS_FINAL_NAMES) {
        hotConvertFlags |= HOT_CONVERT_FINAL_NAMES;
    }
    
    hotSetConvertFlags(h->hot.ctx, hotConvertFlags);
    
	if (flags & HOT_RENAME) {
		h->alias.useFinalNames = 1;
	}
	else {
		h->alias.useFinalNames = 0;
	}
	/* Make PFB path */
	sprintf(pfbpath, "%s%s", h->dir.pfb, pfbfile);

	/*CFFDBG part of hack to print out temp cff file for CID fonts */
	/*h->CMap.file.name set here is used only to trigger dbg behaviour in
	   tempClose() */
	if (hcmapfile != NULL) {
		h->tmp.file.name = pfbpath;
	}

	if (!fileExists(pfbpath)) {
		char buf[1024];
		sprintf(buf, "Specified source font file not found: %s \n", pfbpath);
		message(h, hotERROR, buf);
		return;
	}
	if ((featurefile != NULL) && (!fileExists(featurefile))) {
		char buf[1024];
		sprintf(buf, "Specified feature file not found: %s \n", featurefile);
		message(h, hotERROR, buf);
		return;
	}

	/* Convert font to CFF */
	{
		hotReadFontOverrides fontOverrides;
		fontOverrides.syntheticWeight = addGlyphWeight;
		fontOverrides.maxNumSubrs = maxNumSubrs;
		FontName = psConvFont(h, flags, pfbpath, &psinfo, &fontOverrides);
	}

	type = psinfo & HOT_TYPE_MASK;
	if (h->cff.euroAdded) {
		psinfo |= HOT_EURO_ADDED;
	}

	if (uvsFile != NULL) {
		hotAddUVSMap(h->hot.ctx, uvsFile);
	}

	/* Determine dir that feature file's in */
	h->feat.mainFile = featurefile;
	if (featurefile != NULL) {
		p = strrchr(featurefile, sepch());	/* xxx won't work for '\' delimiters */
		if (p == NULL) {
			h->feat.includeDir[0] = curdir();
		}
		else {
			char featDir[FILENAME_MAX];
			strncpy(featDir, featurefile, p - featurefile);
			featDir[p - featurefile] = '\0';
			copyStr(h, &h->feat.includeDir[0], featDir);
			freeFeatName = 1;
		}
	}

	if (type == hotCID) {
		/* Add CMaps */
		if (hcmapfile == NULL) {
			cbFatal(h, "no CMaps specified [%s]\n", pfbpath);
		}

		addCMap(h, hcmapfile);
		addCMap(h, vcmapfile);
		addCMap(h, mcmapfile);
	}

	// Get database data via callbacks
	if (h->fcdb.files.cnt == 0) {
		{
			fcdbGetRec(h->fcdb.ctx, FontName);
			cbWarning(h, "Font Menu Name database is not specified or not found .[%s]", FontName);
		}
	}
	else if (fcdbGetRec(h->fcdb.ctx, FontName)) {
        {
			cbWarning(h, "not in Font Menu Name database [%s]", FontName);
		}
	}


	// Make sure that GOADB file has been read in, if required
	if ((flags & HOT_RENAME) && (h->alias.recs.cnt < 1) && (type != hotCID)) {
		cbWarning(h, "Glyph renaming is requested, but the Glyph Alias And Order DB file was not specified.");
	}

	h->mac.cmapScript = macScript;	/* Used in hotAddmiscData, in ProcessFontInfo */
	h->mac.cmapLanguage = macLanguage;

	ProcessFontInfo(h->hot.ctx, clientVers, FontName, psinfo,
					&h->mac, otherflags, fsSelectionMask_on, fsSelectionMask_off, os2Version, licenseID);

	/* Make OTF path */
	if (otffile == NULL) {
		makeOTFPath(h, otfpath, FontName);
	}
	else {
		sprintf(otfpath, "%s%s", h->dir.otf, otffile);
	}



	/* Write OTF file */
	fileOpen(&h->otf.file, h, otfpath, "w+b");
	hotConvert(h->hot.ctx);
	fileClose(&h->otf.file);

	if (freeFeatName) {
		cbMemFree(h, h->feat.includeDir[0]);
	}
	h->feat.anon.cnt = 0;
}

// Read font conversion database
void cbFCDBRead(cbCtx h, char *filename) {
	unsigned fileid = h->fcdb.files.cnt;
	fileOpen(dnaNEXT(h->fcdb.files), h, filename, "rb");
	fcdbAddFile(h->fcdb.ctx, fileid, h);
}

// Free callback context
void cbFree(cbCtx h) {
	int i;

	hotFree(h->hot.ctx);
	dnaFREE(h->cff.buf);
	dnaFREE(h->tmpbuf);
	for (i = 0; i < h->feat.anon.size; i++) {
		dnaFREE(h->feat.anon.array[i].data);
	}
	dnaFREE(h->feat.anon);

	// Free database resources
	fcdbFree(h->fcdb.ctx);
	for (i = 0; i < h->fcdb.files.cnt; i++) {
		fileClose(&h->fcdb.files.array[i]);
	}
	dnaFREE(h->fcdb.files);

	dnaFREE(h->alias.recs);
	dnaFREE(h->final.recs);
	dnaFREE(h->alias.names);

	free(h);
}
