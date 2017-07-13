/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * sfnt Format Table Handler.
 */

#include "sfntread.h"
#include "supportexcept.h"

#include <string.h>

#define ARRAY_LEN(a)	(sizeof(a)/sizeof(a[0]))


/* Library context */
struct sfrCtx_					/* Context */
	{
	long flags;					/* Status flags */
#define USED_DEFAULT_STM (1<<0)	/* Default stream in use */
#define USED_CLIENT_STM	 (1<<1)	/* Client stream in use */
#define TTC_STM			 (1<<2)	/* TrueType Collection stream */
	struct						/* sfnt data */
		{
		ctlTag version;			/* Header version */
		long numTables;			/* Table count */
		sfrTable *directory;	/* Table directory */
		long index;				/* Iterator index */
		} sfnt;
	struct						/* TrueType Collection data */
		{
		long DirectoryCount;
		long *TableDirectory;
		long index;				/* Iterator index */
		long origin;
		} TTC;
	struct						/* Client callbacks */
		{
		ctlMemoryCallbacks mem;
		ctlStreamCallbacks stm;
		} cb;
	struct					   	/* Source stream */
		{
		void *stm;
		char *buf;				/* Buffer beginning */
		size_t length;			/* Buffer length */
		char *end;				/* Buffer end */
		char *next;				/* Next byte available (buf <= next < end) */
		} src;
	};

/* ----------------------------- Error Handling ---------------------------- */

/* Handle fatal error. */
static void fatal(sfrCtx h, int err_code)
	{
    RAISE(err_code, NULL);
	}

/* --------------------------- Memory Management --------------------------- */

/* Resize memory. */
static void *memResize(sfrCtx h, void *old, size_t size)
	{
	void *_new = h->cb.mem.manage(&h->cb.mem, old, size);
	if (_new == NULL)
		fatal(h, sfrErrNoMemory);
	return _new;
	}
	
/* Free memory. */
static void memFree(sfrCtx h, void *ptr)
	{
	(void)h->cb.mem.manage(&h->cb.mem, ptr, 0);
	}

/* --------------------------- Context Management -------------------------- */

/* Validate client and create context. */
sfrCtx sfrNew(ctlMemoryCallbacks *mem_cb, ctlStreamCallbacks *stm_cb,
			  CTL_CHECK_ARGS_DCL)
	{
	sfrCtx h;

	/* Check client/library compatibility */
	if (CTL_CHECK_ARGS_TEST(SFR_VERSION))
		return NULL;

	/* Allocate context */
	h = (sfrCtx)mem_cb->manage(mem_cb, NULL, sizeof(struct sfrCtx_));
	if (h == NULL)
		return NULL;

	/* Safety initialization */
	memset(h, 0, sizeof(*h));

	/* Copy callbacks */
	h->cb.mem = *mem_cb;
	h->cb.stm = *stm_cb;

	h->flags = 0;

	return h;
	}

/* Free library context. */
void sfrFree(sfrCtx h)
	{
	if (h == NULL)
		return;
	
	memFree(h, h->sfnt.directory);
	memFree(h, h->TTC.TableDirectory);
	memFree(h, h);
	}

/* ----------------------------- Source Stream ----------------------------- */

/* Fill source buffer. */
static void fillbuf(sfrCtx h)
	{
	h->src.length = h->cb.stm.read(&h->cb.stm, h->src.stm, &h->src.buf);
	if (h->src.length == 0)
		fatal(h, sfrErrSrcStream);
	h->src.next = h->src.buf;
	h->src.end = h->src.buf + h->src.length;
	}

/* Read next sequential source buffer; update offset and return first byte. */
static char nextbuf(sfrCtx h)
	{
	fillbuf(h);
	return *h->src.next++;
	}

/* Read 1-byte unsigned number. */
#define read1(h) \
	((unsigned char)((h->src.next==h->src.end)?nextbuf(h):*h->src.next++))

/* Read 2-byte number. */
static unsigned short read2(sfrCtx h)
	{
	unsigned short value = (unsigned short)read1(h)<<8;
	return value | (unsigned short)read1(h);
	}

/* Read 4-byte unsigned number. */
static unsigned long read4(sfrCtx h)
	{
	unsigned long value = (unsigned long)read1(h)<<24;
	value |= (unsigned long)read1(h)<<16;
	value |= (unsigned long)read1(h)<<8;
	return value | (unsigned long)read1(h);
	}

/* ------------------------------ sfnt Parsing ----------------------------- */

/* Read sfnt table directory. */
static void readSfntDirectory(sfrCtx h, long origin)
	{
	long i;
 	/* Read rest of header */
	h->sfnt.numTables = read2(h);
	(void)read2(h);
	(void)read2(h);
	(void)read2(h);

	/* Read directory */
	h->sfnt.directory = (sfrTable *)memResize(h, h->sfnt.directory,
								  h->sfnt.numTables*sizeof(sfrTable));
	for (i = 0; i < h->sfnt.numTables; i++)
		{
		sfrTable *table = &h->sfnt.directory[i];
		table->tag 		= read4(h);
		table->checksum	= read4(h);
        table->offset	= read4(h) + origin;
 		table->length	= read4(h);
		}

	h->sfnt.index = 0;
	}

/* Read TrueType Collection TableDirectory. */
static void readTTCDirectory(sfrCtx h, long origin)
	{
	long i;

	/* Skip version since we are only interested in version 1.0 fields*/
	(void)read4(h);

	/* Read directory */
	h->TTC.DirectoryCount = read4(h);
	h->TTC.TableDirectory = (long *)memResize(h, h->TTC.TableDirectory, 
									  h->TTC.DirectoryCount*sizeof(long*));
    h->flags |= TTC_STM; /* readSfntDirectory( reads in to h->TTC.TableDirectory[i].directory if TTC_STM is set.*/
        
	for (i = 0; i < h->TTC.DirectoryCount; i++)
	{
        h->TTC.TableDirectory[i] = read4(h) + origin;
    }
	h->TTC.origin = origin;
	h->TTC.index = 0;

	}

/* Begin reading new font. */
int sfrBegFont(sfrCtx h, void *stm, long origin, ctlTag *sfnt_tag)
	{
	if (h->flags & (USED_CLIENT_STM|USED_DEFAULT_STM))
		; /* Stream already set */
	else if (stm == NULL)
		{
		/* Open default source stream */
		h->src.stm = h->cb.stm.open(&h->cb.stm, SFR_SRC_STREAM_ID, 0);
		if (h->src.stm == NULL)
			return sfrErrSrcStream;
		h->flags |= USED_DEFAULT_STM;
		}
	else
		{
		/* Use client-supplied stream */
		h->src.stm = stm;
		h->flags |= USED_CLIENT_STM;
		}

	/* Seek to sfnt header and fill buffer */
	if (h->cb.stm.seek(&h->cb.stm, h->src.stm, origin))
		return sfrErrSrcStream;

	/* Set error handler */
    DURING

        fillbuf(h);

        /* Read and validate sfnt version */
        *sfnt_tag = read4(h);
        switch (*sfnt_tag)
            {
        case sfr_v1_0_tag:
        case sfr_true_tag:
        case sfr_OTTO_tag:
        case sfr_typ1_tag:
            readSfntDirectory(h, (h->flags & TTC_STM)? h->TTC.origin: origin);
            break;
        case sfr_ttcf_tag:
            readTTCDirectory(h, origin);
            break;
        default:
            E_RETURN(sfrErrBadSfnt);
            }

    HANDLER
        return Exception.Code;
    END_HANDLER

	return sfrSuccess;
	}

/* Get the next offset from the TTC table directory. */
long sfrGetNextTTCOffset(sfrCtx h)
	{
	if (!(h->flags & TTC_STM))
		return 0;	/* Invalid call */

	if (h->TTC.index == h->TTC.DirectoryCount)
		{
		/* At end of list; reset iterator */
		h->TTC.index = 0;
		return 0;
		}

	/* Iterate list */
	return h->TTC.TableDirectory[h->TTC.index++];
	}

/* Get the next table element from the sfnt table directory. */
sfrTable *sfrGetNextTable(sfrCtx h)
	{
	if (h->sfnt.directory == NULL)
		return 0;	/* Invalid call */

	if (h->sfnt.index == h->sfnt.numTables)
		{
		/* At end of list; reset iterator */
		h->sfnt.index = 0;
		return 0;
		}

	/* Iterate list */
	return &h->sfnt.directory[h->sfnt.index++];
	}

/* Get table from sfnt by its tag. */
sfrTable *sfrGetTableByTag(sfrCtx h, ctlTag tag)
	{
	long i;

	if (h->sfnt.directory == NULL)
		return NULL;	/* Invalid call */

	/* Table count is small so use non-optimized linear search. This search
	   isn't optimized because although the TrueType specification says the
	   sfnt directory entries must be sorted by ascending tag order there are
	   shipping fonts that don't do this - sigh! */
	for (i = 0; i < h->sfnt.numTables; i++)
		{
		sfrTable *table = &h->sfnt.directory[i];
		if (tag == table->tag)
			return table;
		}

	return NULL;	/* Not found */
	}

/* Finish reading font. */
int sfrEndFont(sfrCtx h)
	{
	if (h->flags & USED_DEFAULT_STM)
		{
		/* Close default source stream */
		if (h->cb.stm.close(&h->cb.stm, h->src.stm) == -1)
			return sfrErrSrcStream;
		}
	h->flags = 0;

	return sfrSuccess;
	}

/* Get version numbers of libraries. */
void sfrGetVersion(ctlVersionCallbacks *cb)
	{
	if (cb->called & 1<<SFR_LIB_ID)
		return;	/* Already enumerated */

	/* This library */
	cb->getversion(cb, SFR_VERSION, "sfntread");

	/* Record this call */
	cb->called |= 1<<SFR_LIB_ID;
	}

/* Convert error code to error string. */
char *sfrErrStr(int err_code)
	{
	static char *errstrs[] =
		{
#undef CTL_DCL_ERR
#define CTL_DCL_ERR(name,string)	string,
#include "sfrerr.h"
		};
	return (err_code < 0 || err_code >= (int)ARRAY_LEN(errstrs))?
		"unknown error": errstrs[err_code];
	}
