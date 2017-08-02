/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 *	sfnt write library.
 */

#include "sfntwrite.h"
#include "dynarr.h"
#include "supportexcept.h"

#include <stdlib.h>

#define ARRAY_LEN(a)	(sizeof(a)/sizeof((a)[0]))

/* ---------------------------- Table Definition --------------------------- */

typedef struct					/* Table directory entry */
	{
	unsigned long tag;
	unsigned long checksum;
	unsigned long offset;
	unsigned long length;
	} Entry;
#define ENTRY_SIZE (4*4)

typedef struct					/* sfnt header */
	{
	ctlTag version;
	unsigned short numTables;
	unsigned short searchRange;
	unsigned short entrySelector;
	unsigned short rangeShift;
	dnaDCL(Entry, directory);	/* [numTables] */
	} sfntHdr;
#define HDR_SIZE (4+4*2)

#define HEAD_ADJUST_OFFSET (2*4)/* head table checksum adjustment offset */

typedef struct					/* Table data */
	{
	sfwTableCallbacks *cb;		/* Client callbacks */
	long flags;					/* Status flags */
#define DONT_WRITE		(1<<0) 	/* Don't write this table */
#define USE_CHECKSUM	(1<<1)	/* Use the client-supplied table checksum */
	} Table;

struct sfwCtx_
	{
	int state;					/* Call tracking state */
	dnaDCL(sfwTableCallbacks, callbacks);/* Copy of client callbacks */
	dnaDCL(Table, tables);		/* Table list */
	sfntHdr hdr;				/* sfnt header */
	struct						/* Destination stream */
		{
		void *stm;
		char *next;				/* Next byte available in input buffer */
		long left;				/* Number of bytes available in input buffer */
		} dst;
	struct						/* Client callbacks */
		{
		ctlMemoryCallbacks mem;
		ctlStreamCallbacks stm;
		} cb;
	dnaCtx dna;					/* Dynamic array context */
	struct					/* Error handling */
    {
		_Exc_Buf env;
    } err;
	};

/* ----------------------------- Error Handling ---------------------------- */

/* Handle fatal error. */
static void fatal(sfwCtx h, int err_code)
	{
  RAISE(&h->err.env, err_code, NULL);
	}

/* -------------------------- Safe dynarr Context -------------------------- */

/* Manage memory. */
static void *dna_manage(ctlMemoryCallbacks *cb, void *old, size_t size)
	{
	sfwCtx h = cb->ctx;
	void *ptr = h->cb.mem.manage(&h->cb.mem, old, size);
	if (size > 0 && ptr == NULL)
		fatal(h, sfwErrNoMemory);
	return ptr;
	}

/* Initialize error handling dynarr context. */
static void dna_init(sfwCtx h)
	{
	ctlMemoryCallbacks cb;
	cb.ctx 		= h;
	cb.manage 	= dna_manage;
	h->dna		= dnaNew(&cb, DNA_CHECK_ARGS);
	}

/* --------------------------- Context Management -------------------------- */

/* Validate client and create context. */
sfwCtx sfwNew(ctlMemoryCallbacks *mem_cb, ctlStreamCallbacks *stm_cb,
			  CTL_CHECK_ARGS_DCL)
	{
	sfwCtx h;

	/* Check client/library compatibility */
	if (CTL_CHECK_ARGS_TEST(SFW_VERSION))
		return NULL;

	/* Allocate context */
	h = mem_cb->manage(mem_cb, NULL, sizeof(struct sfwCtx_));
	if (h == NULL)
		return NULL;

	/* Safety initialization */
	h->tables.size = 0;
	h->hdr.directory.size = 0;
	h->dst.stm = NULL;
	h->dna = NULL;

	/* Copy callbacks */
	h->cb.mem = *mem_cb;
	h->cb.stm = *stm_cb;

	/* Set error handler */
  DURING_EX(h->err.env)

    /* Initialize service library */
    dna_init(h);

    /* Initialize */
    h->state = 0;
    dnaINIT(h->dna, h->callbacks, 15, 10);
    dnaINIT(h->dna, h->tables, 15, 10);
    dnaINIT(h->dna, h->hdr.directory, 15, 10);

	HANDLER

		/* Initialization failed */
    sfwFree(h);
    h = NULL;

	END_HANDLER

	return h;
	}

/* Free library context. */
void sfwFree(sfwCtx h)
	{
	if (h == NULL)
		return;

	dnaFREE(h->callbacks);
	dnaFREE(h->tables);
	dnaFREE(h->hdr.directory);
	dnaFree(h->dna);

	h->cb.mem.manage(&h->cb.mem, h, 0);
	return;
	}

/* ------------------------------ Stream Write ----------------------------- */

/* Write 2-byte number. */
static void write2(sfwCtx h, unsigned short value)
	{
	unsigned char buf[2];
	buf[0] = (unsigned char)(value>>8);
	buf[1] = (unsigned char)value;
	if (h->cb.stm.write(&h->cb.stm, h->dst.stm, 2, (char *)buf) != 2)
		fatal(h, sfwErrDstStream);
	}

/* Write 4-byte number. */
static void write4(sfwCtx h, unsigned long value)
	{
	unsigned char buf[4];
	buf[0] = (unsigned char)(value>>24);
	buf[1] = (unsigned char)(value>>16);
	buf[2] = (unsigned char)(value>>8);
	buf[3] = (unsigned char)value;
	if (h->cb.stm.write(&h->cb.stm, h->dst.stm, 4, (char *)buf) != 4)
		fatal(h, sfwErrDstStream);
	}

/* Write data buffer. */
static void dstWrite(sfwCtx h, size_t count, char *buf)
	{
	if (h->cb.stm.write(&h->cb.stm, h->dst.stm, count, buf) != count)
		fatal(h, sfwErrDstStream);
	}

/* ------------------------------ Stream Read ------------------------------ */

/* Fill source buffer. */
static char fillbuf(sfwCtx h)
	{
	/* 64-bit warning fixed by cast here */
	h->dst.left = (long)(h->cb.stm.read(&h->cb.stm, h->dst.stm, &h->dst.next));
	if (h->dst.left-- == 0)
		fatal(h, sfwErrDstStream);
	return *h->dst.next++;
	}

/* Read 1 byte from stream. */
#define read1(h) \
	((unsigned char)((h->dst.left--==0)? fillbuf(h): *h->dst.next++))

/* Read 4-byte number in big-endian order. */ 
static unsigned long read4(sfwCtx h)
	{
	unsigned long result;
	if (h->dst.left > 3)
		{
		/* All bytes bytes in current block */
		unsigned char *p = (unsigned char *)h->dst.next;
		h->dst.next += 4;
		h->dst.left -= 4;
		return ((unsigned long)p[0]<<24|(unsigned long)p[1]<<16|
				(unsigned long)p[2]<<8 |(unsigned long)p[3]);
		}
	else
		{
		/* Bytes split between blocks */
		result  = (unsigned long)read1(h)<<24;
		result |= (unsigned long)read1(h)<<16;
		result |= (unsigned long)read1(h)<<8;
		result |= (unsigned long)read1(h);
		}
	return result;
	}

/* Seek to specified offset. */
static void dstSeek(sfwCtx h, long offset)
	{
	if (h->cb.stm.seek(&h->cb.stm, h->dst.stm, offset))
		fatal(h, sfwErrDstStream);
	}

/* Return stream position. */
static long dstTell(sfwCtx h)
	{
	long offset = h->cb.stm.tell(&h->cb.stm, h->dst.stm);
	if (offset == -1)
		fatal(h, sfwErrDstStream);
	return offset;
	}

/* ----------------------- Callback Calling Functions ---------------------- */

/* Register client table. */
int sfwRegisterTable(sfwCtx h, sfwTableCallbacks *cb)
	{
	long i;

	/* Validate */
	if (h->state > 1 || cb->fill_table == NULL || cb->write_table == NULL)
		return sfwErrBadCall;

	/* Check for duplicates */
	for (i = 0; i < h->callbacks.cnt; i++)
		if (cb->table_tag == h->callbacks.array[i].table_tag)
			return sfwErrDupTables;

	/* Set error handler */
  DURING_EX(h->err.env)

    /* Copy callbacks */
    *dnaNEXT(h->callbacks) = *cb;

    h->state = 1;

	HANDLER
  	return Exception.Code;
  END_HANDLER

	return sfwSuccess;
	}

/* Make table list */
static void makeTableList(sfwCtx h)
	{
	long i;
	dnaSET_CNT(h->tables, h->callbacks.cnt);
	for (i = 0; i < h->tables.cnt; i++)
		{
		Table *table = &h->tables.array[i];
		table->cb = &h->callbacks.array[i];
		table->flags = 0;
		}
	}

/* Call new callback functions. */
int sfwNewTables(sfwCtx h)
	{
	long i;

	if (h->state > 1)
		return sfwErrBadCall;

	makeTableList(h);

	for (i = 0; i < h->tables.cnt; i++)
		{
		sfwTableCallbacks *cb = h->tables.array[i].cb;
		if (cb->new_table != NULL)
			{
			if (cb->new_table(cb))
				return sfwErrAbort;
			}
		}

	h->state = 2;
	return sfwSuccess;
	}

/* Compare for fill sequence. */
static int CTL_CDECL cmpFill(const void *first, const void *second)
	{
	sfwTableCallbacks *a = ((Table *)first)->cb;
	sfwTableCallbacks *b = ((Table *)second)->cb;
	if (a->fill_seq < b->fill_seq)
		return -1;
	else if (a->fill_seq > b->fill_seq)
		return 1;
	else if (a->table_tag < b->table_tag)
		return -1;
	else if (a->table_tag > b->table_tag)
		return 1;
	else
		return 0;
	}

/* Call fill callback functions. */
int sfwFillTables(sfwCtx h)
	{
	long nextPwr;
	int pwr2;
	int log2;
	long i;

	if (h->state == 0)
		return sfwErrBadCall;

	if (h->tables.cnt == 0)
		makeTableList(h);

	/* Sort into fill order */
	qsort(h->tables.array, h->tables.cnt, sizeof(Table), cmpFill);

	h->hdr.numTables = 0;
	for (i = 0; i < h->tables.cnt; i++)
		{
		int dont_write;
		sfwTableCallbacks *cb;
		Table *table = &h->tables.array[i];

		if (!table)
			continue;
        cb = table->cb;
		if (!cb)
			{
			table->flags = DONT_WRITE;
			continue;
			}

		if (cb->fill_table(cb, &dont_write))
			return sfwErrAbort;

		table->flags = 0;
		if (dont_write)
			table->flags |= DONT_WRITE;
		else
			h->hdr.numTables++;
		}

	/* Calculate binary search fields */
	nextPwr = 2;
	for (log2 = 0; nextPwr <= h->hdr.numTables; log2++)
		nextPwr *= 2;
	pwr2 = nextPwr / 2;

	h->hdr.searchRange = ENTRY_SIZE*pwr2;
	h->hdr.entrySelector = log2;
	h->hdr.rangeShift = ENTRY_SIZE*(h->hdr.numTables - pwr2);

	h->state = 3;
	return sfwSuccess;
	}

/* Compare for write sequence. */
static int CTL_CDECL cmpWrite(const void *first, const void *second)
	{
	sfwTableCallbacks *a = ((Table *)first)->cb;
	sfwTableCallbacks *b = ((Table *)second)->cb;
	if (a->write_seq < b->write_seq)
		return -1;
	else if (a->write_seq > b->write_seq)
		return 1;
	else if (a->table_tag < b->table_tag)
		return -1;
	else if (a->table_tag > b->table_tag)
		return 1;
	else
		return 0;
	}

/* Write all the tables. Return 1 on client abort else 0. */
static int writeTables(sfwCtx h, long origin)
	{
	int i;
	long before;

	/* Sort into write order */
	qsort(h->tables.array, h->tables.cnt, sizeof(Table), cmpWrite);

	/* Write tables */
	h->hdr.directory.cnt = 0;
	before = dstTell(h);
	for (i = 0; i < h->tables.cnt; i++)
		{
		Table *table = &h->tables.array[i];
		if (!(table->flags & DONT_WRITE))
			{
			static const char zeros[] = {0, 0, 0};
			int pad;
			long after;
			sfwTableCallbacks *cb = table->cb;
			Entry *entry = dnaNEXT(h->hdr.directory);
			int use_checksum = 0;

			if (cb->write_table(cb, &h->cb.stm, h->dst.stm, 
								&use_checksum, &entry->checksum))
				return 1;
			if (use_checksum)
				table->flags |= USE_CHECKSUM;
			after = dstTell(h);
 
			/* Pad to 4-byte boundary */
			pad = (4 - (after&3))&3;
			if (pad != 0)
				dstWrite(h, pad, (char *)zeros);

			entry->tag = cb->table_tag;
			entry->offset = before - origin;
			entry->length = after - before;

			before = after + pad;
			}
		}
	return 0;
	}

/* Compare directory tag fields. */
static int CTL_CDECL cmpDirTags(const void *first, const void *second)
	{
	const Entry *a = first;
	const Entry *b = second;
	if (a->tag < b->tag)
		return -1;
	else if (a->tag > b->tag)
		return 1;
	else 
		return 0;
	}

/* Write sfnt header. */
static void writeHdr(sfwCtx h)
	{
	long i;

	/* Sort directory into tag order */
	qsort(h->hdr.directory.array, h->hdr.numTables, sizeof(Entry), cmpDirTags);

	write4(h, h->hdr.version);
	write2(h, h->hdr.numTables);
	write2(h, h->hdr.searchRange);
	write2(h, h->hdr.entrySelector);
	write2(h, h->hdr.rangeShift);

	/* Write directory */
	for (i = 0; i < h->hdr.numTables; i++)
		{
		Entry *entry = &h->hdr.directory.array[i];
		write4(h, entry->tag);
		write4(h, entry->checksum);
		write4(h, entry->offset);
		write4(h, entry->length);
		}
	}

/* Call write callbacks and write sfnt. */
int sfwWriteTables(sfwCtx h, void *stm, ctlTag sfnt_tag)
	{
	Entry *entry;
	unsigned long sum;
	int do_seek;
	long i;
	long origin;
	long offset;
	long dirSize = HDR_SIZE + ENTRY_SIZE*h->hdr.numTables;
	char *filler = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";

	if (h->state != 3)
		return sfwErrBadCall;

	if (stm == NULL)
		{
		/* Open default destination stream */
		h->dst.stm = h->cb.stm.open(&h->cb.stm, SFW_DST_STREAM_ID, 0);
		if (h->dst.stm == NULL)
			fatal(h, sfwErrDstStream);
		}
	else
		h->dst.stm = stm;

	/* Set error handler */
  DURING_EX(h->err.env)

    origin = dstTell(h);

    /* Skip header. A simple "dstSeek(h, origin + dirSize);" should be all that
       is required here but Acrobat on OS X has mapped the seek callback to a
       function that is incapable of seeking beyond the end of a file.
       Therefore, we write the appropriate number of zero bytes instead. */
    dstWrite(h, HDR_SIZE, filler);
    for (i = 0; i < h->hdr.numTables; i++)
      dstWrite(h, ENTRY_SIZE, filler);

    /* Write tables */
    if (writeTables(h, origin))
      return sfwErrAbort;

    /* Read tables and compute table checksums */
    do_seek = 1;
    offset = 0;
    entry = h->hdr.directory.array;
    for (i = 0; i < h->tables.cnt; i++)
      {
      Table *table = &h->tables.array[i];
      if (!(table->flags & DONT_WRITE))
        {
        if (table->flags & USE_CHECKSUM)
          do_seek = 1;
        else
          {
          long j;
          int nLongs = (entry->length + 3) / 4;
          
          if (entry->tag == CTL_TAG('h','e','a','d'))
            offset = entry->offset + HEAD_ADJUST_OFFSET;
          
          if (do_seek)
            {
            /* Seek to table offset */
            dstSeek(h, origin + entry->offset);
            h->dst.left = 0;
            do_seek = 0;
            }

          sum = 0;
          for (j = 0; j < nLongs; j++)
            sum += read4(h);
          entry->checksum = sum;
          }
        entry++;
        }
      }

    /* Write header */
    h->hdr.version = sfnt_tag;
    dstSeek(h, origin);
    writeHdr(h);

    if (offset != 0)
      {
      /* head table present; read header and compute header checksum */
      dstSeek(h, origin);
      h->dst.left = 0;
      sum = 0;
      for (i = 0; i < dirSize; i += 4)
        sum += read4(h);

      /* Add in table checksums */
      for (i = 0; i < h->hdr.numTables; i++)
        sum += h->hdr.directory.array[i].checksum;

      /* Write head table checksum adjustment */
      dstSeek(h, origin + offset);
      write4(h, 0xb1b0afba - sum);
      }

    if (stm == NULL)
      {
      /* Close output stream */
      if (h->cb.stm.close(&h->cb.stm, h->dst.stm))
        fatal(h, sfwErrDstStream);
      }

    h->state = 4;

	HANDLER
  	return Exception.Code;
  END_HANDLER

	return sfwSuccess;
	}

/* Call reuse callback functions. */
int sfwReuseTables(sfwCtx h)
	{
	long i;
	for (i = 0; i < h->tables.cnt; i++)
		{
		sfwTableCallbacks *cb = h->tables.array[i].cb;
		if (cb->reuse_table != NULL)
			{
			if (cb->reuse_table(cb))
				return sfwErrAbort;
			}
		}
	h->state = 2;
	return sfwSuccess;
	}

/* Call free callback functions. */
int sfwFreeTables(sfwCtx h)
	{
	long i;
	for (i = 0; i < h->tables.cnt; i++)
		{
		sfwTableCallbacks *cb = h->tables.array[i].cb;
		if (cb->free_table != NULL)
			{
			if (cb->free_table(cb))
				return sfwErrAbort;
			}
		}
	return sfwSuccess;
	}

/* Get version numbers of libraries. */
void sfwGetVersion(ctlVersionCallbacks *cb)
	{
	if (cb->called & 1<<SFW_LIB_ID)
		return;	/* Already enumerated */

	/* Support libraries */
	dnaGetVersion(cb);

	/* This library */
	cb->getversion(cb, SFW_VERSION, "sfntwrite");

	/* Record this call */
	cb->called |= 1<<SFW_LIB_ID;
	}

/* Map error code to error string. */
char *sfwErrStr(int err_code)
	{
	static char *errstrs[] =
		{
#undef CTL_DCL_ERR
#define CTL_DCL_ERR(name,string)	string,
#include "sfwerr.h"
		};
	return (err_code < 0 || err_code >= (int)ARRAY_LEN(errstrs))?
		"unknown error": errstrs[err_code];
	}

