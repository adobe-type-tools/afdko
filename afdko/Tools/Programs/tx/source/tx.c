/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Type eXchange.
 */

#include "ctlshare.h"

#define TX_VERSION CTL_MAKE_VERSION(1,1,0)

#include "cfembed.h"
#include "cffread.h"
#include "cffwrite.h"
#include "ctutil.h"
#include "dynarr.h"
#include "pdfwrite.h"
#include "sfntread.h"
#include "t1read.h"
#include "ttread.h"
#include "t1write.h"
#include "svread.h"
#include "svgwrite.h"
#include "uforead.h"
#include "ufowrite.h"
#include "varread.h"
#include "txops.h"
#include "dictops.h"
#include "abfdesc.h"
#include "supportframepixeltypes.h"
#include "sha1.h"

#undef global	/* Remove conflicting definition from buildch */

#include <stdio.h>
#include <stdlib.h>

#if PLAT_MAC
#include <console.h>
#include <file_io.h>
#endif /* PLAT_MAC */

#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <math.h>
#include <sys/stat.h>
#include <limits.h>

#if _WIN32
#include <fcntl.h>
#include <io.h>
#define stat _stat
#define S_IFDIR _S_IFDIR
#include <direct.h> /* to get _mkdir() */
#include <time.h>
#else
#include <sys/time.h>
#endif

/* -------------------------------- Options -------------------------------- */

/* Note: options.h must be ascii sorted by option string */

enum					/* Option enumeration */
	{
	opt_None,			/* Not an option */
#define DCL_OPT(string,index)	index,
#include "options.h"
	opt_Count
	};

static const char *options[] =
	{
#undef DCL_OPT
#define DCL_OPT(string,index)	string,
#include "options.h"
	};

/* ----------------------- Miscellaneous Definitions ----------------------- */

#define ARRAY_LEN(t) 	(sizeof(t)/sizeof((t)[0]))

/* Predefined tags */
#define CID__	CTL_TAG('C','I','D',' ')	/* sfnt-wrapped CID table */
#define POST_	CTL_TAG('P','O','S','T')	/* Macintosh POST resource */
#define TYP1_	CTL_TAG('T','Y','P','1')	/* GX Type 1 sfnt table */
#define sfnt_	CTL_TAG('s','f','n','t')	/* sfnt resource */

/* File signatures */
#define sig_PostScript0	CTL_TAG('%',  '!',0x00,0x00)
#define sig_PostScript1	CTL_TAG('%',  'A',0x00,0x00)	/* %ADO... */
#define sig_PostScript2	CTL_TAG('%',  '%',0x00,0x00)	/* %%... */
#define sig_PFB   	   	((ctlTag)0x80010000)
#define sig_CFF		   	((ctlTag)0x01000000)
#define sig_CFF2		   	((ctlTag)0x02000000)
#define sig_OTF		   	CTL_TAG('O','T','T','O')
#define sig_MacResource	((ctlTag)0x00000100)
#define sig_AppleSingle	((ctlTag)0x00051600)
#define sig_AppleDouble	((ctlTag)0x00051607)
#define sig_UFO            CTL_TAG('<',  '?','x','m')
/* Generate n-bit mask */
#define N_BIT_MASK(n)	(~(~0UL<<(n)))
enum { MAX_VERSION_SIZE = 100 };

typedef struct txCtx_ *txCtx;/* tx program context */

typedef struct				/* Macintosh resource info */
	{
	ctlTag type;
	unsigned short id;
	unsigned long name;		/* Name offset then name index */
	unsigned char attrs;
	unsigned long offset;	/* Offset to resource data (excludes length) */
	unsigned long length;	/* Length of resource data */
	} ResInfo;

typedef struct				/* AppleSingle/Double entry descriptor */
	{
	unsigned long id;
	long offset;
	unsigned long length;
	} EntryDesc;

typedef struct				/* Glyph name to Unicode value mapping */
	{
	char *gname;
	unsigned short uv;
	} Name2UV;

enum						/* Stream types */
	{
	stm_Src,				/* Source */
	stm_Dst,				/* Destination */
	stm_Tmp,				/* Temporary */
	stm_Dbg					/* Debug */
	};

#define TMPSIZE	50000		/* Temporary stream buffer size */

typedef struct				/* Data stream */
	{
	short type;
	short flags;
#define STM_TMP_ERR		(1<<0)	/* Temporary stream error occured */
#define STM_DONT_CLOSE	(1<<1)	/* Don't close stream */
	char *filename;
	FILE *fp;
	char *buf;
	size_t pos;				/* Tmp stream position */
	} Stream;

typedef struct				/* Font record */
	{
	int type;				/* Font technology type */
	int iTTC;				/* TrueType Collection index */
	long offset;			/* File offset */
	} FontRec;

typedef struct				/* CFF subr data */
	{
	unsigned long count;
	unsigned char offSize;
	unsigned long offset;	/* Offset array base */
	unsigned long dataref;
	dnaDCL(unsigned char, stemcnt);	/* Per-subr stem count */
	unsigned short bias;
	} SubrInfo;

typedef struct 				/* cmap format 4 segment */
	{
	unsigned short endCode;
	unsigned short startCode;
	short idDelta;
	unsigned long idRangeOffset;	/* changed from unsigned short */
	} cmapSegment4;

typedef struct
{
    unsigned long regionCount;
} RegionInfo;

enum						/* Charstring types */
	{
	dcf_CharString,
	dcf_GlobalSubr,
	dcf_LocalSubr
	};

typedef float FltMtx[6];	/* Float matrix */

/* Function types */
typedef size_t (*SegRefillFunc)(txCtx h, char **ptr);
typedef void (*abfGlyphWidthCallback)(abfGlyphCallbacks *cb, float hAdv);
typedef void (*DumpElementProc)(txCtx h, long index, const ctlRegion *region);

enum						/* Source font technology types */
	{
	src_Type1,				/* Type 1 */
	src_OTF,				/* OTF */
	src_CFF,				/* Naked CFF */
	src_TrueType,			/* TrueType */
	src_SVG,				/* SVG data. */
	src_UFO,				/* UFO data. */
	};

enum						/* Operation modes */
	{
	mode_dump,
	mode_ps,
	mode_afm,
	mode_path,
	mode_cff,
	mode_cef,
	mode_pdf,
	mode_mtx,
	mode_t1,
    mode_bc, /* is deprecated, but still support the option in order to be able to print an error message */
	mode_svg,
    mode_ufow,
	mode_dcf
	};

/* Memory allocation failure simulation control values */
enum
	{
	FAIL_REPORT		= -1,	/* Report total calls to mem_manage() */
	FAIL_INACTIVE	= -2	/* Inactivate memory allocation failure */
	};

struct txCtx_
	{
	char *progname;			/* This program's name (for diagnostics) */
	long flags;				/* Control flags */
#define SEEN_MODE	(1<<0)	/* Flags mode option seen */
#define DONE_FILE	(1<<1)	/* Processed font file */
#define DUMP_RES	(1<<2)	/* Print mac resource map */
#define DUMP_ASD	(1<<3)	/* Print AppleSingle/Double data */
#define AUTO_FILE_FROM_FILE	(1<<4)	/* Gen. dst filename from src filename */
#define AUTO_FILE_FROM_FONT	(1<<5)	/* Gen. dst filename from src FontName */
#define SUBSET_OPT	(1<<6)	/* Subsetting option specified */
#define EVERY_FONT	(1<<7)	/* Read every font from multi-font file */
#define SHOW_NAMES	(1<<8)	/* Show filename and FontName being processed */
#define PRESERVE_GID  (1<<9)  /* Preserve gids when subsetting */
#define NO_UDV_CLAMPING (1<<10) /* Don't clamp UVD's */
#define SUBSET__EXCLUDE_OPT	(1<<11)	/* use glyph list to exclude glyphs, instead of including them */
#define SUBSET_SKIP_NOTDEF  (1<<12)	/* While this is set, don't force the notdef into the current subset. */
#define SUBSET_HAS_NOTDEF  (1<<13)	/* Indcates that notdef has been added, no need to force it in.*/
#define PATH_REMOVE_OVERLAP (1<<14) /* Do not remove path overlaps */
#define PATH_SUPRESS_HINTS (1<<15) /* Do not remove path overlaps */
	int mode;				/* Current mode */
	char *modename;			/* Name of current mode */
	abfTopDict *top;		/* Top dictionary */
	struct					/* Source data */
		{
		int type;			/* Font technology type */
		Stream stm;			/* Input stream */
		long offset;		/* Buffer offset */
		long length;		/* Buffer length */
		char buf[BUFSIZ];	/* Buffer data */
		char *end;			/* One past end of buffer */
		char *next;			/* Next byte available */
		int print_file;		/* Flags when to print filename ahead of debug */
		dnaDCL(abfGlyphInfo *, glyphs);/* Source glyph list when subsetting */
		dnaDCL(abfGlyphInfo *, exclude);/* Excluded glyph list when subsetting */
        dnaDCL(float, widths);/* Source glyph width for -t1 -3 mode */
        dnaDCL(Stream, streamStack);/* support recursive opening of source stream files ( for components) */
		} src;
	struct					/* Destination data */
		{
		Stream stm;			/* Output stream */
		char buf[BUFSIZ];	/* Buffer data */
		void (*begset)	(txCtx h);
		void (*begfont)	(txCtx h, abfTopDict *top);
		void (*endfont)	(txCtx h);
		void (*endset)	(txCtx h);
		} dst;
	dnaDCL(FontRec, fonts);	/* Source font records */
	struct					/* Macintosh resources */
		{
		dnaDCL(ResInfo, map);/* Resouce map */
		dnaDCL(char, names);/* Resouce names */
		} res;
	struct					/* AppleSingle/Double data */
		{
		unsigned long magic;/* Magic #, 00051600-single, 00051607-double */
		unsigned long version;/* Format version */
		dnaDCL(EntryDesc, entries);/* Entry descriptors */
		} asd;
	struct					/* Font data segment */
		{
		SegRefillFunc refill;/* Format-specific refill */
		size_t left;		/* Bytes remaining in segment */
		} seg;
	struct					/* Script file data */
		{
		char *buf;			/* Whole file buffer */
		dnaDCL(char *, args);/* Arg list */
		} script;
	struct					/* File processing */
		{
		char *sr;			/* Source root path */
		char *sd;			/* Source directory path */
		char *dd;			/* Destination directory path */
		char src[FILENAME_MAX];
		char dst[FILENAME_MAX];
		} file;
	struct					/* Random subset data */
		{
		dnaDCL(unsigned short, glyphs);/* Tag list */
		dnaDCL(char, args);	/* Simulated -g args */
		} subset;
	struct					/* Option args */
		{
		char *U;
		char *i;
		char *p;
		char *P;
		struct				/* Glyph list (g option) */
			{
			int cnt;		/* Substring count */
			char *substrs;	/* Concatenated substrings */
			} g;
		struct
			{
			int level;
			} path;
		struct				/* cef-specific */
			{
			unsigned short flags;	/* cefEmbedSpec flags */
			char *F;
			} cef;
		} arg;
	struct					/* t1read library */
		{
		t1rCtx ctx;
		Stream tmp;
		Stream dbg;
		long flags;
		} t1r;
	struct					/* cffread library */
		{
		cfrCtx ctx;
		Stream dbg;
		long flags;
		} cfr;
	struct					/* ttread library */
		{
		ttrCtx ctx;
		Stream dbg;
		long flags;
		} ttr;
    struct					/* svread library */
    {
        svrCtx ctx;
        Stream tmp;
        Stream dbg;
        long flags;
    } svr;
    struct					/* uforead library */
    {
        ufoCtx ctx;
        Stream src;
        Stream dbg;
        long flags;
        char* altLayerDir;
    } ufr;
	struct					/* cffwrite library */
		{
		cfwCtx ctx;
		Stream tmp;
		Stream dbg;
		long flags;
        unsigned long maxNumSubrs;
        } cfw;
	struct					/* cfembed library */
		{
		cefCtx ctx;
		Stream src;
		Stream tmp0;
		Stream tmp1;
		dnaDCL(cefSubsetGlyph, subset);
		dnaDCL(char *, gnames);
		dnaDCL(unsigned short, lookup);	/* Glyph lookup */
		} cef;
	struct					/* abf facilities */
		{
		abfCtx ctx;
		struct abfDumpCtx_ dump;
		struct abfDrawCtx_ draw;
		struct abfMetricsCtx_ metrics;
		struct abfAFMCtx_ afm;
		} abf;
	struct					/* pdfwrite library */
		{
		pdwCtx ctx;
		long flags;
		long level;
		} pdw;
	struct					/* Metrics mode data */
    	{
		int level;			/* Output level */
		struct				/* Metric data */
			{
			struct abfMetricsCtx_ ctx;
			abfGlyphCallbacks cb;
			} metrics;
		struct				/* Aggregate bbox */
			{
		    float left;
			float bottom;
			float right;
			float top;
			struct			/* Glyph that set value */
				{
				abfGlyphInfo *left;
				abfGlyphInfo *bottom;
				abfGlyphInfo *right;
				abfGlyphInfo *top;
				} setby;
			} bbox;
		} mtx;
	struct					/* t1write library */
		{
		long options;		/* Control options */
#define T1W_NO_UID		(1<<0)	/* Remove UniqueID keys */
#define T1W_DECID		(1<<1)	/* -decid option */
#define T1W_USEFD		(1<<2)	/* -usefd option */
#define T1W_REFORMAT 	(1<<3)	/* -pfb or -LWFN options */
#define T1W_WAS_EMBEDDED (1<<4)	/* +E option */
		t1wCtx ctx;
		Stream tmp;
		Stream dbg;
		long flags;			/* Library flags */
		int lenIV;
        long maxGlyphs;
		int fd;				/* -decid target fd */
		dnaDCL(char, gnames);	/* -decid glyph names */
		} t1w;
        struct					/* svgwrite library */
		{
            long options;		/* Control options */
            svwCtx ctx;
            unsigned short unrec;
            Stream tmp;
            Stream dbg;
            long flags;			/* Library flags */
		} svw;
        struct					/* ufowrite library */
		{
            ufwCtx ctx;
            Stream tmp;
            Stream dbg;
            long flags;			/* Library flags */
		} ufow;
	struct					/* Dump cff mode */
		{
		long flags;			/* Control flags */
#define DCF_Header				(1<<0)
#define DCF_NameINDEX			(1<<1)
#define DCF_TopDICTINDEX		(1<<3)
#define DCF_StringINDEX			(1<<4)
#define DCF_GlobalSubrINDEX		(1<<5)
#define DCF_Encoding			(1<<6)
#define DCF_Charset				(1<<7)
#define DCF_FDSelect			(1<<8)
#define DCF_FDArrayINDEX		(1<<9)
#define DCF_CharStringsINDEX	(1<<10)
#define DCF_PrivateDICT			(1<<11)
#define DCF_LocalSubrINDEX		(1<<12)
#define DCF_AllTables			N_BIT_MASK(13)
#define DCF_BreakFlowed			(1<<13)	/* Break flowed objects */
#define DCF_TableSelected		(1<<14)	/* -T option used */
#define DCF_Flatten			(1<<15)	/* Flatten charstrings */
#define DCF_SaveStemCnt			(1<<16)	/* Save h/vstems counts */
#define DCF_IS_CUBE 			(1<<17)	/* Font has Cube data - use different stack and op limits. */
#define DCF_IS_CFF2 			(1<<18)	/* Font has CFF table is CFF 2 */
#define DCF_END_HINTS 			(1<<19)	/* have seen moveto */

		int level;			/* Dump level */
		char *sep;			/* Flowed text separator */
		SubrInfo global;	/* Global subrs */
		dnaDCL(SubrInfo, local);	/* Local subrs */
		dnaDCL(unsigned char, glyph);	/* Per-glyph stem count */
        dnaDCL(RegionInfo, varRegionInfo);
		long stemcnt;		/* Current stem count */
        long vsIndex;       /* needed to derive numRegions */
        long numRegions;    /* needed to decode blend args */
		SubrInfo *fd;		/* Current local info */
		} dcf;
	struct					/* Operand stack */
		{
		long cnt;
		float array[CFF2_MAX_OP_STACK];
		} stack;
	struct					/* Font Dict filter */
		{
        dnaDCL(int, fdIndices);/* Source glyph width for -t1 -3 mode */
		} fd;
	struct					/* OTF cmap encoding */
		{
		dnaDCL(unsigned long, encoding);
		dnaDCL(cmapSegment4, segment);
		} cmap;
	struct					/* Library contexts */
		{
		dnaCtx dna;			/* dynarr library */
		sfrCtx sfr;			/* sfntread library */
		} ctx;
	struct					/* Callbacks */
		{
		ctlMemoryCallbacks mem;
		ctlStreamCallbacks stm;
		abfGlyphCallbacks glyph;
		abfGlyphBegCallback saveGlyphBeg;
    abfGlyphCallbacks save;
    int selected;
		} cb;
	struct					/* Memory allocation failure simulation data */
		{
		long iCall;			/* Index of next call to mem_manange */
		long iFail;	/* Index of failing call or FAIL_REPORT or FAIL_INACTIVE */
		} failmem;
	long maxOpStack;
	};

/* Check stack contains at least n elements. */
#define CHKUFLOW(n) \
	do{if(h->stack.cnt<(n))fatal(h, "Type 2 stack underflow");}while(0)

/* Check stack has room for n elements. */
#define CHKOFLOW(n) \
	do{if(h->stack.cnt+(n)>h->maxOpStack)\
	fatal(h, "Type 2 stack overflow");}while(0)

/* Stack access without check. */
#define INDEX(i) (h->stack.array[i])
#define POP() (h->stack.array[--h->stack.cnt])
#define PUSH(v) (h->stack.array[h->stack.cnt++]=(float)(v))

/* SID to standard string map */
static char *sid2std[] =
	{
#include "stdstr1.h"
	};

static void dumpCstr(txCtx h, const ctlRegion *region, int inSubr);
static void condAddNotdef(txCtx h);
static void callbackSubset(txCtx h);
static void txFree(txCtx h);

/* ----------------------------- Error Handling ---------------------------- */

/* If first debug message for source file; print filename */
static void printFilename(txCtx h)
	{
	fflush(stdout);
	if (h->src.print_file)
		{
		fprintf(stderr, "%s: --- %s\n", h->progname,
				strcmp(h->src.stm.filename, "-") == 0? "stdin":
				h->src.stm.filename);
		h->src.print_file = 0;
		}
	}

/* Fatal exception handler. */
static void CTL_CDECL fatal(txCtx h, char *fmt, ...)
	{
	printFilename(h);
	if (fmt != NULL)
		{
		/* Print error message */
		va_list ap;
		fprintf(stderr, "%s: ", h->progname);
		va_start(ap, fmt);
		vfprintf(stderr, fmt, ap);
		va_end(ap);
		fprintf(stderr, "\n");
		}
	fprintf(stderr, "%s: fatal error\n", h->progname);
	txFree(h);
	exit(EXIT_FAILURE);
	}

/* Print file error message and quit */
static void fileError(txCtx h, char *filename)
	{
	fatal(h, "file error <%s> [%s]", strerror(errno), filename);
	}

/* --------------------------- Memory Management --------------------------- */

/* Allocate memory. */
static void *memNew(txCtx h, size_t size)
	{
	void *ptr = malloc(size);
	if (ptr == NULL)
		fatal(h, "no memory");
	return ptr;
	}

/* Free memory. */
static void memFree(txCtx h, void *ptr)
	{
	if (ptr != NULL)
		free(ptr);
	}

/* ---------------------------- Memory Callbacks --------------------------- */

/* Manage memory. */
static void *mem_manage(ctlMemoryCallbacks *cb, void *old, size_t size)
	{
	if (size > 0)
		{
		txCtx h = cb->ctx;
		if (h->failmem.iCall++ == h->failmem.iFail)
			{
			/* Simulate memory allocation failure */
			fprintf(stderr, "mem_manage() failed on call %ld.\n",
					h->failmem.iCall - 1L);
			return NULL;
			}
		else if (old == NULL)
			return malloc(size);		/* size != 0, old == NULL */
		else
			return realloc(old, size);	/* size != 0, old != NULL */
		}
	else
		{
		if (old == NULL)
			return NULL;				/* size == 0, old == NULL */
		else
			{
			free(old);					/* size == 0, old != NULL */
			return NULL;
			}
		}
	}

/* Initialize memory callbacks. */
static void memInit(txCtx h)
	{
	h->cb.mem.ctx = h;
	h->cb.mem.manage = mem_manage;
	h->failmem.iCall = 0;
	h->failmem.iFail = FAIL_INACTIVE;
	}

/* Manage memory and handle failure. */
static void *safeManage(ctlMemoryCallbacks *cb, void *old, size_t size)
	{
	txCtx h = cb->ctx;
	void *ptr = h->cb.mem.manage(&h->cb.mem, old, size);
	if (size > 0 && ptr == NULL)
		fatal(h, "no memory");
	return ptr;
	}

/* ------------------------------- Tmp Stream ------------------------------ */

/* Intialize tmp stream. */
static void tmpSet(Stream *s, char *filename)
	{
	s->type = stm_Tmp;
	s->flags = 0;
	s->filename = filename;
	s->fp = NULL;
	s->buf = NULL;
	s->pos = 0;
	}

/* On Windows, the stdio.h 'tmpfile' function tries to make temp files in the root
directory, thus requiring administrative privileges. So we first need to use '_tempnam'
to generate a unique filename inside the user's TMP environment variable (or the
current working directory if TMP is not defined). Then we open the temporary file
and return its pointer */
static FILE *_tmpfile()
	{
#ifdef _WIN32
	FILE *fp = NULL;
	char* tempname;
	int flags, mode;
	flags = _O_BINARY|_O_CREAT|_O_EXCL|_O_RDWR|_O_TEMPORARY;
	mode = _S_IREAD | _S_IWRITE;
	tempname = _tempnam(NULL, "tx_tmpfile");
	if(tempname != NULL)
		{
		int fd = _open(tempname, flags, mode);
		if (fd != -1)
			fp = _fdopen(fd, "w+b");
		free(tempname);
		}
#else
	FILE *fp;
	/* Use the default tmpfile on non-Windows platforms */
	fp = tmpfile();
#endif
	return fp;
	}

/* Open tmp stream. */
static Stream *tmp_open(txCtx h, Stream *s)
	{
	s->buf = memNew(h, TMPSIZE + BUFSIZ);
	memset(s->buf, 0, TMPSIZE + BUFSIZ);
	s->fp = _tmpfile();
	if (s->fp == NULL)
		fileError(h, s->filename);
	return s;
	}

/* Seek on tmp stream. */
static int tmp_seek(Stream *s, long offset)
	{
	s->pos = offset;
	return (offset < TMPSIZE)? 0: fseek(s->fp, offset - TMPSIZE, SEEK_SET);
	}

/* Return tmp stream position. */
static size_t tmp_tell(Stream *s)
	{
	return s->pos;
	}

/* Read tmp stream. */
static size_t tmp_read(Stream *s, char **ptr)
	{
	size_t length;
	if (s->pos < TMPSIZE)
		{
		/* Using buffer */
		*ptr = s->buf + s->pos;
		length = TMPSIZE - s->pos;

		/* Anticipate next read */
		if (fseek(s->fp, 0, SEEK_SET) == -1)
			{
			s->flags |= STM_TMP_ERR;
			return 0;
			}
		}
	else
		{
		/* Using file */
		*ptr = s->buf + TMPSIZE;
		length = fread(*ptr, 1, BUFSIZ, s->fp);
		}
	s->pos += length;
	return length;
	}

/* Write to tmp stream. */
static size_t tmp_write(Stream *s, size_t count, char *ptr)
	{
	if (s->pos < TMPSIZE)
		{
		size_t length;
		/* Writing to buffer */
		length = TMPSIZE - s->pos;
		if (length > count)
			memcpy(s->buf + s->pos, ptr, count);
		else
			{
			/* Buffer overflow; completely fill buffer */
			memcpy(s->buf + s->pos, ptr, length);

			/* Write rest to tmp file */
			if (fseek(s->fp, 0, SEEK_SET) == -1)
				{
				s->flags = STM_TMP_ERR;
				return 0;
				}
			count = length + fwrite(ptr + length, 1, count - length, s->fp);
			}
		}
	else
		/* Writing to file */
		count = fwrite(ptr, 1, count, s->fp);
	s->pos += count;
	return count;
	}

/* Close tmp stream. */
static int tmp_close(txCtx h, Stream *s)
	{
	int result = (s->fp != NULL)? fclose(s->fp): 0;
	if (s->buf != NULL)
		memFree(h, s->buf);
	s->fp = NULL;
	s->buf = NULL;
	s->pos = 0;
	return result;
	}

/* ---------------------------- Stream Callbacks --------------------------- */

/* Open stream. */
static void *stm_open(ctlStreamCallbacks *cb, int id, size_t size)
    {
    txCtx h = cb->direct_ctx;
	Stream *s = NULL;	/* Suppress optimizer warning */
    switch (id)
        {
	case CEF_SRC_STREAM_ID:
		/* Open CEF source stream */
		s = &h->cef.src;
		if (strcmp(s->filename, "-") == 0)
			s->fp = stdin;
		else
			{
			s->fp = fopen(s->filename, "rb");
			if (s->fp == NULL)
				return NULL;
			}
		break;
	case T1R_SRC_STREAM_ID:
	case CFR_SRC_STREAM_ID:
	case TTR_SRC_STREAM_ID:
	case SVR_SRC_STREAM_ID:
		/* Source stream already open; just return it */
		s = &h->src.stm;
		break;
    case UFO_SRC_STREAM_ID:
        {
        if (cb->clientFileName != NULL)
            {
            char buffer[FILENAME_MAX];
            sprintf(buffer, "%s/%s", h->file.src, cb->clientFileName);
            s = &h->src.stm;

            s->fp = fopen(buffer, "rb");
            if (s->fp == NULL)
                {
                return NULL;
                }
            *dnaNEXT(h->src.streamStack) = *s;

            }
        else
            return NULL;
       break;
        }
	case CEF_DST_STREAM_ID:
		/* Open CEF destination stream */
		s = &h->dst.stm;
		if (strcmp(s->filename, "-") == 0)
			s->fp = stdout;
		else
			{
			s->fp = fopen(s->filename, "w+b");
			if (s->fp == NULL)
				return NULL;
			}
		break;
	case CFW_DST_STREAM_ID:
	case T1W_DST_STREAM_ID:
	case PDW_DST_STREAM_ID:
	case SVW_DST_STREAM_ID:
		/* Open destination stream */
		s = &h->dst.stm;
		if (strcmp(s->filename, "-") == 0)
			s->fp = stdout;
		else
			{
			s->fp = fopen(s->filename, "wb");
			if (s->fp == NULL)
				return NULL;
			}
		break;
    case UFW_DST_STREAM_ID:
    {

       if (cb->clientFileName != NULL)
        {
			char buffer[FILENAME_MAX];
            sprintf(buffer, "%s/%s", h->file.dst, cb->clientFileName);
            s = &h->dst.stm;

            s->fp = fopen(buffer, "wt");
            if (s->fp == NULL)
            {
                return NULL;
            }

        }
        else
            return NULL;
        break;
    }
	case CEF_TMP0_STREAM_ID:
		s = &h->cef.tmp0;
		tmp_open(h, s);
		break;
	case CEF_TMP1_STREAM_ID:
		s = &h->cef.tmp1;
		tmp_open(h, s);
		break;
	case T1R_TMP_STREAM_ID:
		s = &h->t1r.tmp;
		tmp_open(h, s);
		break;
    case SVR_TMP_STREAM_ID:
        s = &h->svr.tmp;
        tmp_open(h, s);
        break;
	case CFW_TMP_STREAM_ID:
		s = &h->cfw.tmp;
		tmp_open(h, s);
		break;
	case T1W_TMP_STREAM_ID:
		s = &h->t1w.tmp;
		tmp_open(h, s);
		break;
	case SVW_TMP_STREAM_ID:
		s = &h->svw.tmp;
		tmp_open(h, s);
		break;
	case T1R_DBG_STREAM_ID:
		s = &h->t1r.dbg;
		break;
    case SVR_DBG_STREAM_ID:
        s = &h->svr.dbg;
        break;
    case UFO_DBG_STREAM_ID:
        s = &h->ufr.dbg;
        break;
    case UFW_DBG_STREAM_ID:
        s = &h->ufow.dbg;
        break;
	case CFR_DBG_STREAM_ID:
		s = &h->cfr.dbg;
		break;
	case TTR_DBG_STREAM_ID:
		s = &h->ttr.dbg;
		break;
	case CFW_DBG_STREAM_ID:
		s = &h->cfw.dbg;
		if (s->fp == NULL)
			return NULL;
		break;
	case T1W_DBG_STREAM_ID:
		s = &h->t1w.dbg;
		if (s->fp == NULL)
			return NULL;
		break;
    case SVW_DBG_STREAM_ID:
        s = &h->svw.dbg;
        if (s->fp == NULL)
            return NULL;
        break;
	default:
		fatal(h, "invalid stream open");
        }
    return s;
    }

/* Seek to stream position. */
static int stm_seek(ctlStreamCallbacks *cb, void *stream, long offset)
    {
	if (offset >= 0)
		{
		Stream *s = stream;
		switch (s->type)
			{
		case stm_Src:
		case stm_Dst:
		case stm_Dbg:
			return fseek(s->fp, offset, SEEK_SET);
		case stm_Tmp:
			return tmp_seek(s, offset);
			}
		}
	return -1;	/* Bad seek */
	}

/* Return stream position. */
static long stm_tell(ctlStreamCallbacks *cb, void *stream)
    {
	Stream *s = stream;
	switch (s->type)
		{
	case stm_Src:
	case stm_Dbg:
		return ftell(s->fp);
		break;
	case stm_Dst:
		if (0 != strcmp("-", s->filename))
			return ftell(s->fp);
		break;

	case stm_Tmp:
		return (long)tmp_tell(s);
		}
	return 0;	/* Suppress compiler warning */
    }

/* Read from stream. */
static size_t stm_read(ctlStreamCallbacks *cb, void *stream, char **ptr)
    {
	Stream *s = stream;
	switch (s->type)
		{
	case stm_Src:
		{
		txCtx h = cb->direct_ctx;
		if (h->seg.refill != NULL)
			return h->seg.refill(h, ptr);
		}
		/* Fall through */
	case stm_Dst:
		*ptr = s->buf;
		return fread(s->buf, 1, BUFSIZ, s->fp);
	case stm_Tmp:
		return tmp_read(s, ptr);
		}
	return 0;	/* Suppress compiler warning */
    }

/* Write to stream. */
static size_t stm_write(ctlStreamCallbacks *cb, void *stream,
						size_t count, char *ptr)
    {
	Stream *s = stream;
	switch (s->type)
		{
	case stm_Src:
	case stm_Dst:
		return fwrite(ptr, 1, count, s->fp);
	case stm_Tmp:
		return tmp_write(s, count, ptr);
	case stm_Dbg:
		{
		txCtx h = cb->direct_ctx;
		printFilename(h);
		fprintf(stderr, "%s: (%s) %.*s\n",
				h->progname, s->filename, (int)count, ptr);
		}
		return count;
		}
	return 0;	/* Suppress compiler warning */
    }

/* Return stream status. */
static int stm_status(ctlStreamCallbacks *cb, void *stream)
	{
	Stream *s = stream;
	if (s->type == stm_Tmp)
		{
		if (s->flags & STM_TMP_ERR)
			return CTL_STREAM_ERROR;
		else if (s->pos < TMPSIZE)
			return CTL_STREAM_OK;
		}
	if (feof(s->fp))
		return CTL_STREAM_END;
	else if (ferror(s->fp))
		return CTL_STREAM_ERROR;
	else
		return CTL_STREAM_OK;
	}

/* Close stream. */
static int stm_close(ctlStreamCallbacks *cb, void *stream)
    {
    txCtx h = cb->direct_ctx;
	Stream *s = stream;
	if (s->type == stm_Tmp)
		return tmp_close(cb->direct_ctx, s);
	else if (s->fp == NULL || s->fp == stdout || s->flags & STM_DONT_CLOSE)
		return 0;
	else
		{
        int retval;
		FILE *fp = s->fp;
		retval =  fclose(fp);
        s->fp = NULL;	/* Avoid re-close */
        if (s->type == stm_Src)
        {
        h->src.streamStack.cnt--;
        if (h->src.streamStack.cnt > 0)
            *s = h->src.streamStack.array[h->src.streamStack.cnt-1];
        }
        return retval;
		}
    }

/* Initialize stream record. */
static void stmSet(Stream *s, int type, char *filename, char *buf)
	{
	s->type = type;
	s->flags = 0;
	s->filename = filename;
	s->fp = NULL;
	s->buf = buf;
	s->pos = 0;
	}

/* Initialize debug stream. */
static void dbgSet(Stream *s, char *filename)
	{
	s->type = stm_Dbg;
	s->flags = STM_DONT_CLOSE;
	s->filename = filename;
	s->fp = stderr;
	s->buf = NULL;
	s->pos = 0;
	}

/* Close steam at exit if still open. */
static void stmFree(txCtx h, Stream *s)
	{
	if (s->fp != NULL)
		(void)fclose(s->fp);
	}

/* Initialize stream callbacks and stream records. */
static void stmInit(txCtx h)
	{
	h->cb.stm.direct_ctx	= h;
    h->cb.stm.indirect_ctx 	= NULL;
    h->cb.stm.clientFileName = NULL;
	h->cb.stm.open			= stm_open;
	h->cb.stm.seek			= stm_seek;
	h->cb.stm.tell			= stm_tell;
	h->cb.stm.read			= stm_read;
	h->cb.stm.write			= stm_write;
	h->cb.stm.status		= stm_status;
	h->cb.stm.close 		= stm_close;

	stmSet(&h->src.stm, stm_Src, h->file.src, h->src.buf);
	stmSet(&h->dst.stm, stm_Dst, h->file.dst, h->dst.buf);

	stmSet(&h->cef.src, stm_Src, h->file.src, h->src.buf);

	tmpSet(&h->cef.tmp0, "(cef) tmpfile0");
	tmpSet(&h->cef.tmp1, "(cef) tmpfile1");
	tmpSet(&h->t1r.tmp, "(t1r) tmpfile");
	tmpSet(&h->cfw.tmp, "(cfw) tmpfile");
	tmpSet(&h->t1w.tmp, "(t1w) tmpfile");
	tmpSet(&h->svw.tmp, "(svw) tmpfile");
	tmpSet(&h->svw.tmp, "(ufw) tmpfile");

	dbgSet(&h->t1r.dbg, "t1r");
	dbgSet(&h->cfr.dbg, "cfr");
    dbgSet(&h->svr.dbg, "svr");
    dbgSet(&h->ufr.dbg, "ufr");
    dbgSet(&h->ufow.dbg, "ufw");
	dbgSet(&h->ttr.dbg, "ttr");
	dbgSet(&h->cfw.dbg, "cfw");
	dbgSet(&h->t1w.dbg, "t1w");
	dbgSet(&h->svw.dbg, "svw");
	}

/* ----------------------------- File Handling ----------------------------- */

/* Initialize destination filename. */
static void dstFileSetName(txCtx h, char *filename)
	{
	if (h->file.dd != NULL)
		sprintf(h->file.dst, "%s/%s", h->file.dd, filename);
	else
		strcpy(h->file.dst, filename);
	}

/* Set automatic destination filename. */
static void dstFileSetAutoName(txCtx h, abfTopDict *top)
	{
	char buf[FILENAME_MAX];
	char *filename;

	if (h->flags & AUTO_FILE_FROM_FILE)
		{
		char *p = strrchr(h->file.src, '/');
		if (p == NULL)
			p = strrchr(h->file.src, '\\');
		strcpy(buf, (p == NULL)? h->file.src: p + 1);
		p = strrchr(buf, '.');
		if (p != NULL)
			*p = '\0';
		filename = buf;
		}
	else if (h->flags & AUTO_FILE_FROM_FONT)
		filename = (top->sup.flags & ABF_CID_FONT)?
			top->cid.CIDFontName.ptr: top->FDArray.array[0].FontName.ptr;
	else
		return;

	if (h->file.dd != NULL)
		sprintf(h->file.dst, "%s/%s.%s", h->file.dd, filename,h->modename);
	else
		sprintf(h->file.dst, "%s.%s", filename, h->modename);
	}

/* Open destination file. */
static void dstFileOpen(txCtx h, abfTopDict *top)
	{
	dstFileSetAutoName(h, top);

	if (h->dst.stm.fp != NULL)
		return;	/* Already open */

	/* Open dstination file */
	if (strcmp(h->dst.stm.filename, "-") == 0)
		h->dst.stm.fp = stdout;
	else
		{
		h->dst.stm.fp = fopen(h->dst.stm.filename, "w");
		if (h->dst.stm.fp == NULL)
			fileError(h, h->dst.stm.filename);
		}
	}

/* Close destination file. */
static void dstFileClose(txCtx h)
	{
	if (h->dst.stm.fp != stdout)
		{
		if (fclose(h->dst.stm.fp))
			fileError(h, h->dst.stm.filename);
		}
	h->dst.stm.fp = NULL;
	}

/* ------------------------------- Data Input ------------------------------ */

/* Fill source buffer. */
static void fillbuf(txCtx h, long offset)
	{
	h->src.length = (long)fread(h->src.buf, 1, sizeof(h->src.buf), h->src.stm.fp);
	if (h->src.length == 0)
		{
		if (feof(h->src.stm.fp))
			fatal(h, "end of file [%s]", h->src.stm.filename);
		else
            fileError(h, h->src.stm.filename);
		}
    else
        {
            h->src.offset = offset;
            h->src.next = h->src.buf;
            h->src.end = h->src.buf + h->src.length;
        }
    return;
	}

/* Read next sequential source buffer; update offset and return first byte. */
static char nextbuf(txCtx h)
	{
	fillbuf(h, h->src.offset + h->src.length);
	return *h->src.next++;
	}

/* Seek to buffered data byte. */
static void bufSeek(txCtx h, long offset)
	{
	long delta = offset - h->src.offset;
	if (delta >= 0 && delta < h->src.length)
		/* Offset within current buffer; reposition next byte */
		h->src.next = h->src.buf + delta;
	else
		{
		if (fseek(h->src.stm.fp, offset, SEEK_SET))
			fileError(h, h->src.stm.filename);
		fillbuf(h, offset);
		}
	}

/* Copy count bytes from source stream. */
static void readN(txCtx h, size_t count, char *ptr)
	{
	size_t left = h->src.end - h->src.next;

	while (left < count)
		{
		/* Copy buffer */
		memcpy(ptr, h->src.next, left);
		ptr += left;
		count -= left;

		/* Refill buffer */
		fillbuf(h, h->src.offset + h->src.length);
		left = h->src.length;
		}

	memcpy(ptr, h->src.next, count);
	h->src.next += count;
	}

#define read1(h)	\
	(unsigned char)((h->src.next==h->src.end)?nextbuf(h):*h->src.next++)

/* Read 2-byte number. */
static unsigned short read2(txCtx h)
	{
	unsigned short value = read1(h)<<8;
	return value | read1(h);
	}

/* Read 2-byte signed number. */
static short sread2(txCtx h)
	{
	unsigned short value = read1(h)<<8;
	value |= read1(h);
#if SHRT_MAX == 32767
	return (short)value;
#else
	return (short)((value > 32767)? value - 65536: value);
#endif
	}

/* Read 4-byte number. */
static unsigned long read4(txCtx h)
	{
	unsigned long value = (unsigned long)read1(h)<<24;
	value |= (unsigned long)read1(h)<<16;
	value |= read1(h)<<8;
	return value | read1(h);
	}

/* Read 1-, 2-, 3-, or 4-byte number. */
static unsigned long readn(txCtx h, int n)
	{
	unsigned long value = 0;
	switch (n)
		{
	case 4:
		value = read1(h);
	case 3:
		value = value<<8 | read1(h);
	case 2:
		value = value<<8 | read1(h);
	case 1:
		value = value<<8 | read1(h);
		}
	return value;
	}

/* ------------------------- RNG-Related Functions  ------------------------ */

/* Seed RNG with scrambled time. */
static void seedtime(void)
	{
	time_t now = time(NULL);
	srand((unsigned)(now*now));
	}

/* Return a random number in the range [0 - N). */
static long randrange(long N)
	{
	return (long)((double)rand() / ((double)RAND_MAX + 1)*N);
	}

/* ------------------------------- dump mode ------------------------------- */

/* Begin font set. */
static void dump_BegSet(txCtx h)
	{
	}

/* Begin font. */
static void dump_BegFont(txCtx h, abfTopDict *top)
	{
	dstFileOpen(h, top);
	h->abf.dump.fp = h->dst.stm.fp;
    if (h->fd.fdIndices.cnt > 0)
    {
        h->abf.dump.excludeSubset = (h->flags & SUBSET__EXCLUDE_OPT);
        h->abf.dump.fdCnt = h->fd.fdIndices.cnt;
        h->abf.dump.fdArray = h->fd.fdIndices.array;
    }
	top->sup.filename =
		(strcmp(h->src.stm.filename, "-") == 0)? "stdin": h->src.stm.filename;
	abfDumpBegFont(&h->abf.dump, top);
	}

/* End font. */
static void dump_EndFont(txCtx h)
	{
	}

/* End font set. */
static void dump_EndSet(txCtx h)
	{
	dstFileClose(h);
	}

/* Setup dump mode. */
static void dump_SetMode(txCtx h)
	{
	/* Initialize control data */
	h->abf.dump.level = 1;

	/* Set mode name */
	h->modename	= "dump";

	/* Set library functions */
	h->dst.begset	= dump_BegSet;
	h->dst.begfont	= dump_BegFont;
	h->dst.endfont	= dump_EndFont;
	h->dst.endset	= dump_EndSet;

	/* Initialize glyph callbacks */
	h->cb.glyph = abfGlyphDumpCallbacks;
	h->cb.glyph.direct_ctx = &h->abf.dump;

	/* Set source library flags */
	h->t1r.flags = T1R_USE_MATRIX;
	h->cfr.flags = CFR_USE_MATRIX;

	h->mode = mode_dump;
	}

/* Print text parameter. */
static void printText(int cnt, char *text[])
	{
	int i;
	for (i = 0; i < cnt; i++)
		printf("%s", text[i]);
	}

/* Mode-specific help. */
static void dump_Help(txCtx h)
	{
	static char *text[] =
		{
#include "dump.h"
		};
	printText(ARRAY_LEN(text), text);
	}

/* -------------------------------- ps mode -------------------------------- */

/* Begin font set. */
static void ps_BegSet(txCtx h)
	{
	}

/* Begin font. */
static void ps_BegFont(txCtx h, abfTopDict *top)
	{
	if (h->abf.draw.level == 1 && h->arg.g.cnt == 0)
		fatal(h,
			  "to use -1 option with all glyphs specify "
			  "an all-glyph range with -g 0-N option");
	dstFileOpen(h, top);
	h->abf.draw.fp = h->dst.stm.fp;
	if (h->src.type == src_TrueType)
		h->abf.draw.flags |= ABF_FLIP_TICS;
	else
		h->abf.draw.flags &= ~ABF_FLIP_TICS;
	top->sup.filename =
		(strcmp(h->src.stm.filename, "-") == 0)? "stdin": h->src.stm.filename;
	abfDrawBegFont(&h->abf.draw, top);
	}

/* End font. */
static void ps_EndFont(txCtx h)
	{
	abfDrawEndFont(&h->abf.draw);
	}

/* End font set. */
static void ps_EndSet(txCtx h)
	{
	dstFileClose(h);
	}

/* Setup ps mode. */
static void ps_SetMode(txCtx h)
	{
	/* Initialize control data */
	h->abf.draw.flags = 0;
	h->abf.draw.level = 0;

	/* Set mode name */
	h->modename	= "ps";

	/* Set library functions */
	h->dst.begset	= ps_BegSet;
	h->dst.begfont	= ps_BegFont;
	h->dst.endfont	= ps_EndFont;
	h->dst.endset	= ps_EndSet;

	/* Initialize glyph callbacks */
	h->cb.glyph = abfGlyphDrawCallbacks;
	h->cb.glyph.direct_ctx = &h->abf.draw;

	/* Set source libarary flags */
	h->t1r.flags = (T1R_UPDATE_OPS|T1R_USE_MATRIX);
	h->cfr.flags = (CFR_UPDATE_OPS|CFR_USE_MATRIX);

	h->mode = mode_ps;
	}

/* Mode-specific help. */
static void ps_Help(txCtx h)
	{
	static char *text[] =
		{
#include "ps.h"
		};
	printText(ARRAY_LEN(text), text);
	}

/* -------------------------------- afm mode ------------------------------- */

/* Begin font set. */
static void afm_BegSet(txCtx h)
	{
	}

/* Begin new font. */
static void afm_BegFont(txCtx h, abfTopDict *top)
	{
	dstFileOpen(h, top);
	h->abf.afm.fp = h->dst.stm.fp;
	top->sup.filename =
		(strcmp(h->src.stm.filename, "-") == 0)? "stdin": h->src.stm.filename;
	abfAFMBegFont(&h->abf.afm, top);
	}

/* End new font. */
static void afm_EndFont(txCtx h)
	{
	abfAFMEndFont(&h->abf.afm);
	}

/* End font set. */
static void afm_EndSet(txCtx h)
	{
	dstFileClose(h);
	}

/* Setup afm mode. */
static void afm_SetMode(txCtx h)
	{
	/* Set mode name */
	h->modename	= "afm";

	/* Set library functions */
	h->dst.begset	= afm_BegSet;
	h->dst.begfont	= afm_BegFont;
	h->dst.endfont	= afm_EndFont;
	h->dst.endset	= afm_EndSet;

	/* Initialize glyph callbacks */
	h->cb.glyph = abfGlyphAFMCallbacks;
	h->cb.glyph.direct_ctx = &h->abf.afm;

	/* Set source libarary flags */
	h->t1r.flags = (T1R_UPDATE_OPS|T1R_USE_MATRIX);
	h->cfr.flags = (CFR_UPDATE_OPS|CFR_USE_MATRIX);

	h->mode = mode_afm;
	}

/* Mode-specific help. */
static void afm_Help(txCtx h)
	{
	static char *text[] =
		{
#include "afm.h"
		};
	printText(ARRAY_LEN(text), text);
	}

/* ------------------------------ Path Library ----------------------------- */

/* Begin font set. */
static void path_BegSet(txCtx h)
	{
	}

/* Begin new font. */
static void path_BegFont(txCtx h, abfTopDict *top)
	{
	dstFileOpen(h, top);
	top->sup.filename =
		(strcmp(h->src.stm.filename, "-") == 0)? "stdin": h->src.stm.filename;

	if (h->arg.path.level == 1)
		{
		/* Prepare draw facility */
		h->abf.draw.fp = h->dst.stm.fp;
		if (h->src.type == src_TrueType)
			h->abf.draw.flags |= ABF_FLIP_TICS;
		else
			h->abf.draw.flags &= ~ABF_FLIP_TICS;
		abfDrawBegFont(&h->abf.draw, top);
		}
	else
		{
		/* Prepare dump facility */
		h->abf.dump.fp = h->dst.stm.fp;
        h->abf.dump.left = 0;
        h->abf.dump.excludeSubset = 0;
        h->abf.dump.fdCnt = 0;
		abfDumpBegFont(&h->abf.dump, top);
		}

	if (abfBegFont(h->abf.ctx, top))
		fatal(h, NULL);
	}

/* End new font. */
static void path_EndFont(txCtx h)
	{
	if (h->arg.path.level == 1)
		abfDrawEndFont(&h->abf.draw);

	/* Initialize glyph callbacks */
	if (h->arg.path.level == 0)
		{
		h->cb.glyph = abfGlyphDumpCallbacks;
		h->cb.glyph.direct_ctx = &h->abf.dump;
		}
	else
		{
		h->cb.glyph = abfGlyphDrawCallbacks;
		h->cb.glyph.direct_ctx = &h->abf.draw;
		}
    if (h->flags & PATH_SUPRESS_HINTS)
    {
        h->cb.glyph.stem = NULL;
        h->cb.glyph.flex = NULL;
    }

	if (abfEndFont(h->abf.ctx, ABF_PATH_REMOVE_OVERLAP, &h->cb.glyph))
		fatal(h, NULL);
	}

/* End font set. */
static void path_EndSet(txCtx h)
	{
	if (abfFree(h->abf.ctx))
		fatal(h, NULL);
	}

/* Set control functions. */
static void path_SetMode(txCtx h)
	{
	h->abf.draw.flags = 0;
	h->abf.draw.level = 1;
	h->abf.dump.level = 6;

	/* Set mode name */
	h->modename = "path";

	/* Set library functions */
	h->dst.begset	= path_BegSet;
	h->dst.begfont	= path_BegFont;
	h->dst.endfont	= path_EndFont;
	h->dst.endset	= path_EndSet;

	if (h->abf.ctx == NULL)
		{
		/* Create library context */
		h->abf.ctx = abfNew(&h->cb.mem, ABF_CHECK_ARGS);
		if (h->abf.ctx == NULL)
			fatal(h, "(abf) can't init lib");
		}

	/* Initialize glyph callbacks */
	h->cb.glyph = abfGlyphPathCallbacks;
	h->cb.glyph.direct_ctx = h->abf.ctx;

	h->t1r.flags |= (T1R_UPDATE_OPS|T1R_USE_MATRIX);
	h->cfr.flags |= (CFR_UPDATE_OPS|CFR_USE_MATRIX);

	h->mode = mode_path;
	}

/* Mode-specific help. */
static void path_Help(txCtx h)
	{
	static char *text[] =
		{
#include "path.h"
		};
	printText(ARRAY_LEN(text), text);
	}

/* -------------------------------- cff mode ------------------------------- */

/* Reset the glyph flags. */
static void resetGlyphs(txCtx h)
	{
    int result = 1;

	switch (h->src.type)
		{
	case src_Type1:
		result = t1rResetGlyphs(h->t1r.ctx);
		break;
	case src_OTF:
	case src_CFF:
		result = cfrResetGlyphs(h->cfr.ctx);
		break;
	case src_TrueType:
		result = ttrResetGlyphs(h->ttr.ctx);
		break;
	case src_SVG:
		result = svrResetGlyphs(h->svr.ctx);
		break;

	case src_UFO:
		result = ufoResetGlyphs(h->ufr.ctx);
		break;

		}
    if (result)
        fatal(h, NULL);
	}

/* Begin font set. */
static void cff_BegSet(txCtx h)
	{
	if (cfwBegSet(h->cfw.ctx, h->cfw.flags))
		fatal(h, NULL);
	}

/* Begin font. */
static void cff_BegFont(txCtx h, abfTopDict *top)
{
    dstFileSetAutoName(h, top);
    if (h->flags & PATH_REMOVE_OVERLAP)
    {
        h->cb.glyph = abfGlyphPathCallbacks;
        h->cb.glyph.direct_ctx = h->abf.ctx;
        if (abfBegFont(h->abf.ctx, top))
            fatal(h, NULL);
    }
    else
    {
        h->cb.glyph = cfwGlyphCallbacks;
        h->cb.glyph.direct_ctx = h->cfw.ctx;

        if (!(h->cfw.flags & CFW_WRITE_CFF2))
        {
            /* This keeps these callbacks from being used when
             writing a regular CFF, and avoids the overhead of porcessing the
             source CFF2 blend args */
            h->cb.glyph.moveVF = NULL;
            h->cb.glyph.lineVF = NULL;
            h->cb.glyph.curveVF = NULL;
            h->cb.glyph.stemVF = NULL;
        }

        if (cfwBegFont(h->cfw.ctx, NULL, h->cfw.maxNumSubrs))
            fatal(h, NULL);
    }
    if (h->flags & PATH_SUPRESS_HINTS)
    {
        h->cb.glyph.stem = NULL;
        h->cb.glyph.flex = NULL;
    }
}

/* End font. */
static void cff_EndFont(txCtx h)
	{
    if (h->flags & PATH_REMOVE_OVERLAP)
        {
        if (cfwBegFont(h->cfw.ctx, NULL, h->cfw.maxNumSubrs))
            fatal(h, NULL);

        resetGlyphs(h);
        h->cb.glyph = cfwGlyphCallbacks;
        h->cb.glyph.direct_ctx = h->cfw.ctx;
        if (!(h->cfw.flags & CFW_WRITE_CFF2))
        {
            /* This keeps these callbacks from being used when
             writing a regular CFF, and avoids the overhead of trying to process the
             source CFF2 blend args */
            h->cb.glyph.moveVF = NULL;
            h->cb.glyph.lineVF = NULL;
            h->cb.glyph.curveVF = NULL;
            h->cb.glyph.stemVF = NULL;
        }
        if (h->flags & PATH_SUPRESS_HINTS)
        {
            h->cb.glyph.stem = NULL;
            h->cb.glyph.flex = NULL;
        }
        if (abfEndFont(h->abf.ctx, ABF_PATH_REMOVE_OVERLAP, &h->cb.glyph))
            fatal(h, NULL);

        if (cfwEndFont(h->cfw.ctx, h->top))
                fatal(h, NULL);
        }
    else
        {
        if (cfwEndFont(h->cfw.ctx, h->top))
            fatal(h, NULL);
        }
	}

/* End font set. */
static void cff_EndSet(txCtx h)
	{
	if (cfwEndSet(h->cfw.ctx))
		fatal(h, NULL);
	if (abfFree(h->abf.ctx))
		fatal(h, NULL);
	}

/* Setup cff mode. */
static void cff_SetMode(txCtx h)
	{
	/* Initialize control data */
	/* This is now set at the start of parseArgs
	h->cfw.flags = 0;
	*/

	/* Set mode name */
	h->modename	= "cff";

	/* Set library functions */
	h->dst.begset	= cff_BegSet;
	h->dst.begfont	= cff_BegFont;
	h->dst.endfont	= cff_EndFont;
	h->dst.endset	= cff_EndSet;

	if (h->cfw.ctx == NULL)
		{
		/* Create library context */
		h->cfw.ctx = cfwNew(&h->cb.mem, &h->cb.stm, CFW_CHECK_ARGS);
		if (h->cfw.ctx == NULL)
			fatal(h, "(cfw) can't init lib");
		}

        /* The default callbacks. These get reset in cff_BegFont() and cff_EndFont()
         as some options play the font data through a different library on a first pass,
         before writing to cff on a second pass */

        h->cb.glyph = cfwGlyphCallbacks;
        h->cb.glyph.direct_ctx = h->cfw.ctx;

        if (!(h->cfw.flags & CFW_WRITE_CFF2))
        {
            /* This keeps these callbacks from being used when
             writing a regular CFF, and avoids the overhead of porcessing the
             source CFF2 blend args */
            h->cb.glyph.moveVF = NULL;
            h->cb.glyph.lineVF = NULL;
            h->cb.glyph.curveVF = NULL;
            h->cb.glyph.stemVF = NULL;
        }

        if (h->abf.ctx == NULL)
        {
        /* Create library context */
        h->abf.ctx = abfNew(&h->cb.mem, ABF_CHECK_ARGS);
        if (h->abf.ctx == NULL)
            fatal(h, "(abf) can't init lib");
        }

	/* Set source library flags */
	/* These are now set at the start of parseArgs
	h->t1r.flags = 0;
	h->cfr.flags = 0;
	*/

	h->mode = mode_cff;
	}

/* Mode-specific help. */
static void cff_Help(txCtx h)
	{
	static char *text[] =
		{
#include "cff.h"
		};
	printText(ARRAY_LEN(text), text);
	}

/* ------------------------------ Preserve GID ------------------------------ */

#define PRESERVE_CHARSTRING (1<<15)

static void callbackPreserveGlyph(txCtx h, int type, unsigned short id,
                                  char *name) {
  h->src.glyphs.array[id]->flags |= PRESERVE_CHARSTRING;
}

/* Begin glyph path. */
static int preserveGlyphBeg(abfGlyphCallbacks *cb, abfGlyphInfo *info) {
	txCtx h = cb->direct_ctx;
  cb->info = info;
  h->cb.selected =
    (h->src.glyphs.array[info->tag]->flags & PRESERVE_CHARSTRING) != 0;
  return h->cb.save.beg(&h->cb.save, info);
}

/* Save glyph width. */
static void preserveGlyphWidth(abfGlyphCallbacks *cb, float hAdv) {
	txCtx h = cb->direct_ctx;
	h->cb.save.width(&h->cb.save, hAdv);
}

/* Add move to path. */
static void preserveGlyphMove(abfGlyphCallbacks *cb, float x0, float y0) {
	txCtx h = cb->direct_ctx;
  if (h->cb.selected) {
	  h->cb.save.move(&h->cb.save, x0, y0);
  }
}

/* Add line to path. */
static void preserveGlyphLine(abfGlyphCallbacks *cb, float x1, float y1) {
	txCtx h = cb->direct_ctx;
  if (h->cb.selected) {
	  h->cb.save.line(&h->cb.save, x1, y1);
  }
}

/* Add curve to path. */
static void preserveGlyphCurve(abfGlyphCallbacks *cb,
					   float x1, float y1,
					   float x2, float y2,
					   float x3, float y3) {
	txCtx h = cb->direct_ctx;
  if (h->cb.selected) {
	  h->cb.save.curve(&h->cb.save, x1, y1, x2, y2, x3, y3);
  }
}

static void preserveGlyphStem(abfGlyphCallbacks *cb,
                              int flags, float edge0, float edge1) {
  txCtx h = cb->direct_ctx;
  if (h->cb.selected) {
    h->cb.save.stem(&h->cb.save, flags, edge0, edge1);
  }
}

static void preserveGlyphFlex(abfGlyphCallbacks *cb, float depth,
                 float x1, float y1,
                 float x2, float y2,
                 float x3, float y3,
                 float x4, float y4,
                 float x5, float y5,
                 float x6, float y6) {
  txCtx h = cb->direct_ctx;
  if (h->cb.selected) {
    h->cb.save.flex(&h->cb.save, depth, x1, y1, x2, y2, x3, y3, x4, y4, x5, y5, x6, y6);
  }
}

/* Ignore general glyph operator. */
static void preserveGlyphGenop(abfGlyphCallbacks *cb,
					   int cnt, float *args, int op) {
	txCtx h = cb->direct_ctx;
  if (h->cb.selected) {
	  h->cb.save.genop(&h->cb.save, cnt, args, op);
  }
}

/* Handle seac operator. */
static void preserveGlyphSeac(abfGlyphCallbacks *cb,
					  float adx, float ady, int bchar, int achar) {
	txCtx h = cb->direct_ctx;
  if (h->cb.selected) {
	  h->cb.save.seac(&h->cb.save, adx, ady, bchar, achar);
  }
}

/* End glyph path. */
static void preserveGlyphEnd(abfGlyphCallbacks *cb) {
	txCtx h = cb->direct_ctx;
  h->cb.save.end(&h->cb.save);
}

static void preserveCubeBlend(abfGlyphCallbacks *cb, unsigned int nBlends, unsigned int numVals, float* blendVals)
{
  	txCtx h = cb->direct_ctx;
    if (h->cb.selected) {
        h->cb.save.cubeBlend(&h->cb.save, nBlends, numVals, blendVals);
    }

}
static void preserveCubeSetwv(abfGlyphCallbacks *cb, unsigned int numDV)
{
    txCtx h = cb->direct_ctx;
    if (h->cb.selected) {
        h->cb.save.cubeSetwv(&h->cb.save, numDV);
    }
}
static void preserveCubeCompose(abfGlyphCallbacks *cb, int cubeLEIndex, float x0, float y0, int numDV, float* ndv)
{
    txCtx h = cb->direct_ctx;
    if (h->cb.selected) {
        h->cb.save.cubeCompose(&h->cb.save, cubeLEIndex, x0, y0, numDV, ndv);
    }
}
static void preserveCubeTransform(abfGlyphCallbacks *cb, float rotate, float scaleX, float scaleY, float skew, float skewY)
{
    txCtx h = cb->direct_ctx;
    if (h->cb.selected) {
        h->cb.save.cubeTransform(&h->cb.save, rotate, scaleX, scaleY, skew, skewY);
    }
}



/* preserve mode callbacks template. */
static abfGlyphCallbacks preserveGlyphCallbacks =
{
    NULL,
    NULL,
    NULL,
    preserveGlyphBeg,
    preserveGlyphWidth,
    preserveGlyphMove,
    preserveGlyphLine,
    preserveGlyphCurve,
    preserveGlyphStem,
    preserveGlyphFlex,
    preserveGlyphGenop,
    preserveGlyphSeac,
    preserveGlyphEnd,
    preserveCubeBlend,
    preserveCubeSetwv,
    preserveCubeCompose,
    preserveCubeTransform,
    NULL,
    NULL,
    NULL,
    NULL,
};

/* ------------------------------- Glyph List ------------------------------ */

/* Begin new glyph definition for gathering glyph info. */
static int getGlyphBeg(abfGlyphCallbacks *cb, abfGlyphInfo *info)
	{
	txCtx h = cb->indirect_ctx;
	*dnaNEXT(h->src.glyphs) = info;
	return ABF_SKIP_RET;
	}

/* Make glyph list from font. */
static void getGlyphList(txCtx h)
	{
	if (h->src.glyphs.cnt > 0)
		return;	/* Already have list for this font */

	h->cb.saveGlyphBeg = h->cb.glyph.beg;

	/* Insert data gather function */
    h->cb.glyph.beg = getGlyphBeg;
	h->cb.glyph.indirect_ctx = h;

	/* Iterate glyphs */
	switch (h->src.type)
		{
	case src_Type1:
		if (t1rIterateGlyphs(h->t1r.ctx, &h->cb.glyph) ||
			t1rResetGlyphs(h->t1r.ctx))
			fatal(h, NULL);
		break;
	case src_OTF:
	case src_CFF:
		if (cfrIterateGlyphs(h->cfr.ctx, &h->cb.glyph) ||
			cfrResetGlyphs(h->cfr.ctx))
			fatal(h, NULL);
		break;
	case src_TrueType:
		if (ttrIterateGlyphs(h->ttr.ctx, &h->cb.glyph) ||
			ttrResetGlyphs(h->ttr.ctx))
			fatal(h, NULL);
		break;
	case src_SVG:
		if (svrIterateGlyphs(h->svr.ctx, &h->cb.glyph) ||
			svrResetGlyphs(h->svr.ctx))
			fatal(h, NULL);
		break;

	case src_UFO:
		if (ufoIterateGlyphs(h->ufr.ctx, &h->cb.glyph) ||
			ufoResetGlyphs(h->ufr.ctx))
			fatal(h, NULL);
		break;

		}

	/* Restore saved function */
	h->cb.glyph.beg = h->cb.saveGlyphBeg;
	}

/* Compare glyphs by their name. */
static int CTL_CDECL cmpByName(const void *first, const void *second)
	{
	return strcmp((*(abfGlyphInfo **)first)->gname.ptr,
				  (*(abfGlyphInfo **)second)->gname.ptr);
	}

/* Sort glyph list by glyph name. */
static void sortGlyphsByName(txCtx h)
	{
	qsort(h->src.glyphs.array, h->src.glyphs.cnt,
		  sizeof(h->src.glyphs.array[0]), cmpByName);
	}

/* Compare glyphs by their fd. */
static int CTL_CDECL cmpByFD(const void *first, const void *second)
	{
	const abfGlyphInfo *a = *(abfGlyphInfo **)first;
	const abfGlyphInfo *b = *(abfGlyphInfo **)second;
	if (a->iFD < b->iFD)
		return -1;
	else if (a->iFD > b->iFD)
		return 1;
	else if (a->cid < b->cid)
		return -1;
	else if (a->cid > b->cid)
		return 1;
	else
		return 0;
	}

/* Sort glyph list by FD index. */
static void sortGlyphsByFD(txCtx h)
	{
	qsort(h->src.glyphs.array, h->src.glyphs.cnt,
		  sizeof(h->src.glyphs.array[0]), cmpByFD);
	}

/* Make glyph subset from glyph list. */
static void makeSubsetGlyphList(txCtx h)
	{
	long i;
	dnaSET_CNT(h->subset.glyphs, h->src.glyphs.cnt);
	for (i = 0; i < h->src.glyphs.cnt; i++)
		h->subset.glyphs.array[i] = h->src.glyphs.array[i]->tag;
	}

/* Construct arg buffer from subset list to simulated -g option. */
static void makeSubsetArgList(txCtx h)
	{
	long i;
	long rangecnt = 0;
	unsigned short first = h->subset.glyphs.array[0];
	unsigned short last = first;
	h->subset.args.cnt = 0;
	for (i = 1; i <= h->subset.glyphs.cnt; i++)
		{
        unsigned short curr;
         if (i < h->subset.glyphs.cnt)
             curr = h->subset.glyphs.array[i];
        else
            curr = 0;

		if (last + 1 != curr)
			{
			char buf[12];	/* 5 digits + hyphen + 5 digits + nul */
			if (first == last)
				sprintf(buf, "%hu", last);
			else
				sprintf(buf, "%hu-%hu", first, last);
			strcpy(dnaEXTEND(h->subset.args, (long)strlen(buf) + 1), buf);
			first = curr;
			rangecnt++;
			}
		last = curr;
		}
	h->arg.g.cnt = rangecnt;
	h->arg.g.substrs = h->subset.args.array;
	}

/* ----------------------------- Subset Parsing ---------------------------- */

enum				/* Glyph selector types */
	{
	sel_by_tag,
	sel_by_cid,
	sel_by_name
	};

/* Parse subset args. */
static void parseSubset(txCtx h, void (*select)(txCtx h, int type,
												unsigned short id, char *name))
	{
	long i;
	char *p = h->arg.g.substrs;

	for (i = 0; i < h->arg.g.cnt; i++)
		{
		unsigned short id;
		unsigned short lo;
		unsigned short hi;

		if (*p == '/')
			{
			/* CID */
			if (sscanf(p, "/%hu-/%hu", &lo, &hi) == 2)
				;
			else if (sscanf(p, "/%hu", &lo) == 1)
				hi = lo;
			else
				goto next;

			for (id = lo; id <= hi; id++)
				select(h, sel_by_cid, id, NULL);

			}
		else if (isdigit(*p))
			{
			/* Tag */
			if (sscanf(p, "%hu-%hu", &lo, &hi) == 2)
				;
			else if (sscanf(p, "%hu", &lo) == 1)
				hi = lo;
			else
				goto next;

			for (id = lo; id <= hi; id++)
				select(h, sel_by_tag, id, NULL);
			}
		else
			/* Name */
			select(h, sel_by_name, 0, p);

		/* Advance to next substring */
	next:
		while (*p++ != '\0')
			;
		}
	}

/* Parse subset args. */
static void parseFDSubset(txCtx h)
{
	long i;
	char *p = h->arg.g.substrs;

	for (i = 0; i < h->arg.g.cnt; i++)
    {
		unsigned short id;
		unsigned short lo;
		unsigned short hi;

        if (isdigit(*p))
        {
			/* Tag */
			if (sscanf(p, "%hu-%hu", &lo, &hi) == 2)
				;
			else if (sscanf(p, "%hu", &lo) == 1)
				hi = lo;
			else
				goto next;

			for (id = lo; id <= hi; id++)
                *dnaNEXT(h->fd.fdIndices)= id;
       }
		else
        {
            fatal(h, "-fd argument is not an integer.");

        }
		/* Advance to next substring */
	next:
		while (*p++ != '\0')
			;
    }
}


/* -------------------------------- cef mode ------------------------------- */

/* Begin font set. */
static void cef_BegSet(txCtx h)
	{
	}

/* Begin font. */
static void cef_BegFont(txCtx h, abfTopDict *top)
	{
	dstFileSetAutoName(h, top);
	}

/* Compare glyphs by their CID. */
static int CTL_CDECL cef_cmpByCID(const void *first, const void *second,
								  void *ctx)
	{
	txCtx h = ctx;
	abfGlyphInfo *a = h->src.glyphs.array[*(unsigned short *)first];
	abfGlyphInfo *b = h->src.glyphs.array[*(unsigned short *)second];
	if (a->cid < b->cid)
		return -1;
	else if (a->cid > b->cid)
		return 1;
	else
		return 0;
	}

/* Match glyph by its CID. */
static int CTL_CDECL cef_matchByCID(const void *key, const void *value,
									void *ctx)
	{
	txCtx h = ctx;
	unsigned short a = *(unsigned short *)key;
	unsigned short b = h->src.glyphs.array[*(unsigned short *)value]->cid;
	if (a < b)
		return -1;
	else if (a > b)
		return 1;
	else
		return 0;
 	}

/* Compare glyphs by their name. */
static int CTL_CDECL cef_cmpByName(const void *first, const void *second,
								   void *ctx)
	{
	txCtx h = ctx;
	return strcmp(h->src.glyphs.array[*(unsigned short *)first]->gname.ptr,
				  h->src.glyphs.array[*(unsigned short *)second]->gname.ptr);
	}

/* Match glyph by its name. */
static int CTL_CDECL cef_matchByName(const void *key, const void *value,
									 void *ctx)
	{
	txCtx h = ctx;
	return strcmp(key,
				  h->src.glyphs.array[*(unsigned short *)value]->gname.ptr);
	}

/* Make glyph lookup array. */
static void makeGlyphLookup(txCtx h, ctuCmpFunc cmp)
	{
	long i;

	/* Make lookup array of all tags */
	dnaSET_CNT(h->cef.lookup, h->src.glyphs.cnt);
	for (i = 0; i < h->cef.lookup.cnt; i++)
		h->cef.lookup.array[i] = (unsigned short)i;

	/* Sort array */
	ctuQSort(h->cef.lookup.array, h->cef.lookup.cnt,
			 sizeof(unsigned short), cmp, h);
	}

/* Lookup glyph. */
static void lookupGlyph(txCtx h, void *key, ctuMatchFunc match)
	{
	size_t index;
	if (ctuLookup(key, h->cef.lookup.array, h->cef.lookup.cnt,
				  sizeof(unsigned short), match, &index, h))
		dnaNEXT(h->cef.subset)->id =
			h->src.glyphs.array[h->cef.lookup.array[index]]->tag;
	}

/* Select subset glyph. */
static void selectGlyph(txCtx h, int type, unsigned short id, char *gname)
	{
	switch (type)
		{
	case sel_by_tag:
		if (id < h->src.glyphs.cnt)
			dnaNEXT(h->cef.subset)->id = id;
		break;
	case sel_by_cid:
		if (h->top->sup.flags & ABF_CID_FONT)
			{
			if (h->cef.lookup.cnt == 0)
				/* Make CID lookup list */
				makeGlyphLookup(h, cef_cmpByCID);
			lookupGlyph(h, &id, cef_matchByCID);
			}
		break;
	case sel_by_name:
		if (!(h->top->sup.flags & ABF_CID_FONT))
			{
			if (h->cef.lookup.cnt == 0)
				/* Make glyph name lookup list */
				makeGlyphLookup(h, cef_cmpByName);
			lookupGlyph(h, gname, cef_matchByName);
			}
		break;
		}
	}

/* Match glyph name */
static int CTL_CDECL matchName(const void *key, const void *value)
	{
	return strcmp((char *)key, ((Name2UV *)value)->gname);
	}

/* Map glyph name to Unicode value using simplified assignment algorithm. */
static unsigned short mapName2UV(txCtx h, char *gname, unsigned short *unrec)
	{
	static const Name2UV agl[] =
		{
#include "agl2uv.h"
		};
	Name2UV *map = (Name2UV *)bsearch(gname, agl, ARRAY_LEN(agl),
									  sizeof(Name2UV), matchName);
	if (map != NULL)
		return map->uv;	/* Match found */

	/* Not found */
	if (strcmp(gname, ".notdef") == 0)
		return 0xFFFF;	/* No encoding for .notdef */

	if (gname[0] == 'u' &&
		gname[1] == 'n' &&
		gname[2] == 'i' &&
		isxdigit(gname[3]) && !islower(gname[3]) &&
		isxdigit(gname[4]) && !islower(gname[4]) &&
		isxdigit(gname[5]) && !islower(gname[5]) &&
		isxdigit(gname[6]) && !islower(gname[6]) &&
		gname[7] == '\0')
		/* uni<CODE> name; return hex part */
		return (unsigned short)strtol(&gname[3], NULL, 16);

	/* return Private Use Area UV */
	return (*unrec)++;
	}

/* Get User Design Vector. */
static float *getUDV(txCtx h)
	{
	static float UDV[CFF2_MAX_AXES];
	int i;
	char *p;
	char *q;

	if (h->arg.U == NULL)
		return NULL;

	/* Parse User Design Vector */
	for (i = 0; i < CFF2_MAX_AXES; i++)
		UDV[i] = 0.0;

	p = h->arg.U;
	for (i = 0; i < CFF2_MAX_AXES; i++)
		{
		UDV[i] = (float)strtod(p, &q);
		if (p == q || (*q != ',' && *q != '\0'))
			fatal(h, "bad UDV");
		else if (*q == '\0')
			break;
		p = q + 1;
		}

	return UDV;
	}

/* Print CEF subset specification. */
static void printSpec(txCtx h, cefEmbedSpec *spec)
	{
	char *p;
	long i;

	printf("--- CEF subset:\n"
		   "SRC font	%s\n"
		   "SRC glyphs	%ld\n"
		   "DST font	%s\n"
		   "DST glyphs	%ld\n",
		   h->cef.src.filename, h->src.glyphs.cnt,
		   h->dst.stm.filename, spec->subset.cnt);

	p = (h->top->sup.flags & ABF_CID_FONT)? "/": "";
	for (i = 0; i < spec->subset.cnt; i++)
		{
		printf("%s%hu", p, spec->subset.array[i].id);
		p = (h->top->sup.flags & ABF_CID_FONT)? ",/": ",";
		}
	printf("\n");

	if (spec->subset.names == NULL)
		return;

	/* Print glyph names */
	p = "";
	for (i = 0; i < spec->subset.cnt; i++)
		{
		unsigned short id = spec->subset.array[i].id;
		if (id < CEF_VID_BEGIN)
			printf("%s%s", p, spec->subset.names[id]);
		else
			printf("%svid-%hu", p, id);
		p = ",";
		}
	printf("\n");
	}

/* CEF glyph mapping callback. */
static void cefGlyphMap(cefMapCallback *cb,
						unsigned short gid, abfGlyphInfo *info)
	{
	int cid = info->flags & ABF_GLYPH_CID;
	if (gid == 0)
		{
		if (cid)
			printf("DST map [gid]=/cid\n");
		else
			printf("DST map [gid]=<gname>\n");
		}
	if (cid)
		printf("[%hu]=/%hu ", gid, info->cid);
	else
		printf("[%hu]=<%s> ", gid, info->gname.ptr);
	}

/* End font. */
static void cef_EndFont(txCtx h)
	{
	cefMapCallback map;
	cefEmbedSpec spec;
	int result;
	long i;
	unsigned short unrec;

	getGlyphList(h);

	if (h->arg.g.cnt == 0)
		{
		/* Whole font subset */
		dnaSET_CNT(h->cef.subset, h->src.glyphs.cnt);
		for (i = 0; i < h->cef.subset.cnt; i++)
			h->cef.subset.array[i].id = (unsigned short)i;
		}
	else
		{
		h->cef.subset.cnt = 0;
		h->cef.lookup.cnt = 0;
		parseSubset(h, selectGlyph);
		}

	h->cef.gnames.cnt = 0;
	unrec = 0xE000;	/* Start of Private Use Area */

	if (h->top->sup.flags & ABF_CID_FONT)
		/* Make CID subset, encoding all glyphs in PUA */
		for (i = 0; i < h->cef.subset.cnt; i++)
			{
			cefSubsetGlyph *dst = &h->cef.subset.array[i];
			abfGlyphInfo *src = h->src.glyphs.array[dst->id];
			dst->id = src->cid;
			dst->uv = unrec++;
			}
	else
		{
		/* Make tag list and assign Unicode encoding */
		for (i = 0; i < h->cef.subset.cnt; i++)
			{
			cefSubsetGlyph *dst = &h->cef.subset.array[i];
			abfGlyphInfo *src = h->src.glyphs.array[dst->id];
			dst->uv = mapName2UV(h, src->gname.ptr, &unrec);
			}

		/* Decide whether to use name list */
		switch (h->src.type)
			{
		case src_Type1:
			break;			/* Use names */
		case src_OTF:
		case src_CFF:
			if (rand() & 0x0100)
				goto initspec;
			break;			/* Use names 50% of the time */
		case src_TrueType:
			goto initspec;	/* Don't use names */
			}

		/* Make name list */
		dnaSET_CNT(h->cef.gnames, h->src.glyphs.cnt + 1);
		for (i = 0; i < h->src.glyphs.cnt; i++)
			h->cef.gnames.array[i] = h->src.glyphs.array[i]->gname.ptr;
		h->cef.gnames.array[i] = NULL;
		}

#if 0
	{
	/* Add 2 virtual glyphs */
	cefSubsetGlyph *glyph = dnaEXTEND(h->cef.subset, 2);
	glyph[0].id = CEF_VID_BEGIN + 0;
	glyph[0].uv = 0xa000;
	glyph[1].id = CEF_VID_BEGIN + 1;
	glyph[1].uv = 0xa001;
	}
#endif

	/* Initialize embedding spec. */
 initspec:
	spec.flags			= h->arg.cef.flags;
	spec.newFontName	= h->arg.cef.F;
	spec.UDV			= getUDV(h);
	spec.URL			= NULL;
	spec.subset.cnt		= h->cef.subset.cnt;
	spec.subset.array	= h->cef.subset.array;
	spec.subset.names	= (h->cef.gnames.cnt > 0)? h->cef.gnames.array: NULL;
	spec.kern.cnt		= 0;

	printSpec(h, &spec);

	/* Turn off segmentation on source stream */
	h->seg.refill = NULL;

	/* Initialize glyph mapping callback */
	map.ctx = NULL;
	map.glyphmap = cefGlyphMap;

	if (h->arg.cef.flags & CEF_WRITE_SVG)
		cefSetSvwFlags(h->cef.ctx, h->svw.flags);

	/* Make embedding font */
	result = cefMakeEmbeddingFont(h->cef.ctx, &spec, &map);
	if (result)
		fatal(h, "(cef) %s", cefErrStr(result));
	else
		printf("\n");
	}

/* End font set. */
static void cef_EndSet(txCtx h)
	{
	}

/* Setup cef mode. */
static void cef_SetMode(txCtx h)
	{
	/* Initialize args */
	h->arg.cef.F = NULL;
	h->arg.cef.flags = 0;
	h->svw.flags = SVW_NEWLINE_UNIX;  /* In case cfembed library used in svgwrite mode */

	/* Set mode name */
	h->modename	= "cef";

	/* Set library functions */
	h->dst.begset	= cef_BegSet;
	h->dst.begfont	= cef_BegFont;
	h->dst.endfont	= cef_EndFont;
	h->dst.endset	= cef_EndSet;

	if (h->cef.ctx == NULL)
		{
		h->cef.ctx = cefNew(&h->cb.mem, &h->cb.stm, CEF_CHECK_ARGS);
		if (h->cef.ctx == NULL)
			fatal(h, "(cef) can't init lib");
		}

	/* Set source libarary flags */
	h->t1r.flags = 0;
	h->cfr.flags = 0;

	h->mode = mode_cef;
	}

/* Mode-specific help. */
static void cef_Help(txCtx h)
	{
	static char *text[] =
		{
#include "cef.h"
		};
	printText(ARRAY_LEN(text), text);
	}

/* -------------------------------- pdf mode ------------------------------- */

/* Begin font set. */
static void pdf_BegSet(txCtx h)
	{
	}

/* Begin new font. */
static void pdf_BegFont(txCtx h, abfTopDict *top)
	{
	dstFileOpen(h, top);
	top->sup.filename =
		(strcmp(h->src.stm.filename, "-") == 0)? "stdin": h->src.stm.filename;

	if (h->src.type == src_TrueType)
		h->pdw.flags |= PDW_FLIP_TICS;
	else
		h->pdw.flags &= ~PDW_FLIP_TICS;

	if (pdwBegFont(h->pdw.ctx, h->pdw.flags, h->pdw.level, top))
		fatal(h, NULL);
	}

/* End new font. */
static void pdf_EndFont(txCtx h)
	{
	if (pdwEndFont(h->pdw.ctx))
		fatal(h, NULL);
	}

/* End font set. */
static void pdf_EndSet(txCtx h)
	{
	}

/* Set control functions. */
static void pdf_SetMode(txCtx h)
	{
	h->pdw.flags = 0;
	h->pdw.level = 0;

	/* Set mode name */
	h->modename = "pdf";

	/* Set library functions */
	h->dst.begset	= pdf_BegSet;
	h->dst.begfont	= pdf_BegFont;
	h->dst.endfont	= pdf_EndFont;
	h->dst.endset	= pdf_EndSet;

	if (h->pdw.ctx == NULL)
		{
		/* Create library context */
		h->pdw.ctx = pdwNew(&h->cb.mem, &h->cb.stm, PDW_CHECK_ARGS);
		if (h->pdw.ctx == NULL)
			fatal(h, "(pdw) can't init lib");
		}

	/* Initialize glyph callbacks */
	h->cb.glyph = pdwGlyphCallbacks;
	h->cb.glyph.direct_ctx = h->pdw.ctx;

	h->t1r.flags |= (T1R_UPDATE_OPS|T1R_USE_MATRIX);
	h->cfr.flags |= (CFR_UPDATE_OPS|CFR_USE_MATRIX);

	h->mode = mode_pdf;
	}

/* Mode-specific help. */
static void pdf_Help(txCtx h)
	{
	static char *text[] =
		{
#include "pdf.h"
		};
	printText(ARRAY_LEN(text), text);
	}

/* ------------------------------- mtx mode ------------------------------- */

/* Begin glyph path. */
static int mtxGlyphBeg(abfGlyphCallbacks *cb, abfGlyphInfo *info)
	{
	txCtx h = cb->direct_ctx;
	cb->info = info;
	return h->mtx.metrics.cb.beg(&h->mtx.metrics.cb, info);
	}

/* Save glyph width. */
static void mtxGlyphWidth(abfGlyphCallbacks *cb, float hAdv)
	{
	txCtx h = cb->direct_ctx;
	h->mtx.metrics.cb.width(&h->mtx.metrics.cb, hAdv);
	}

/* Add move to path. */
static void mtxGlyphMove(abfGlyphCallbacks *cb, float x0, float y0)
	{
	txCtx h = cb->direct_ctx;
	h->mtx.metrics.cb.move(&h->mtx.metrics.cb, x0, y0);
	}

/* Add line to path. */
static void mtxGlyphLine(abfGlyphCallbacks *cb, float x1, float y1)
	{
	txCtx h = cb->direct_ctx;
	h->mtx.metrics.cb.line(&h->mtx.metrics.cb, x1, y1);
	}

/* Add curve to path. */
static void mtxGlyphCurve(abfGlyphCallbacks *cb,
					   float x1, float y1,
					   float x2, float y2,
					   float x3, float y3)
	{
	txCtx h = cb->direct_ctx;
	h->mtx.metrics.cb.curve(&h->mtx.metrics.cb, x1, y1, x2, y2, x3, y3);
	}

/* Ignore general glyph operator. */
static void mtxGlyphGenop(abfGlyphCallbacks *cb,
					   int cnt, float *args, int op)
	{
	/* Nothing to do */
	}

/* Handle seac operator. */
static void mtxGlyphSeac(abfGlyphCallbacks *cb,
					  float adx, float ady, int bchar, int achar)
	{
	/* Nothing to do */
	}

/* End glyph path. */
static void mtxGlyphEnd(abfGlyphCallbacks *cb)
	{
	txCtx h = cb->direct_ctx;
	abfMetricsCtx g = &h->mtx.metrics.ctx;
	abfGlyphInfo *info = cb->info;

	h->mtx.metrics.cb.end(&h->mtx.metrics.cb);

	fprintf(h->dst.stm.fp, "glyph[%hu] {", info->tag);
	if (info->flags & ABF_GLYPH_CID)
		/* Dump CID-keyed glyph */
		fprintf(h->dst.stm.fp, "%hu,%hhu", info->cid, info->iFD);
	else
		{
		/* Dump name-keyed glyph */
		abfEncoding *enc = &info->encoding;
		fprintf(h->dst.stm.fp, "%s", info->gname.ptr);
		if (enc->code == ABF_GLYPH_UNENC)
			fprintf(h->dst.stm.fp, ",-");
		else
			{
			/* Dump encoding */
			char *sep = ",";
			do
				{
				if (info->flags & ABF_GLYPH_UNICODE)
					fprintf(h->dst.stm.fp, "%s0x%04lX", sep, enc->code);
				else
					fprintf(h->dst.stm.fp, "%s0x%02lX", sep, enc->code);
				sep = "+";
				enc = enc->next;
				}
			while (enc != NULL);
			}
		}

	if (h->mtx.level & 1)
		/* Real metrics */
		fprintf(h->dst.stm.fp, ",%g,{%g,%g,%g,%g}}\n", g->real_mtx.hAdv,
				g->real_mtx.left, g->real_mtx.bottom,
				g->real_mtx.right, g->real_mtx.top);
	else
		/* Integer metrics */
		fprintf(h->dst.stm.fp, ",%ld,{%ld,%ld,%ld,%ld}}\n", g->int_mtx.hAdv,
				g->int_mtx.left, g->int_mtx.bottom,
				g->int_mtx.right, g->int_mtx.top);

	if (h->mtx.level > 1)
		{
		/* Compute aggregate bounding box */
		if (g->real_mtx.left   != 0 ||
			g->real_mtx.bottom != 0 ||
			g->real_mtx.right  != 0 ||
			g->real_mtx.top    != 0)
			{
			/* Marking glyph */
			if (h->mtx.bbox.left   == 0 &&
				h->mtx.bbox.bottom == 0 &&
				h->mtx.bbox.right  == 0 &&
				h->mtx.bbox.top    == 0)
				{
				/* First marking glyph; set all values */
				h->mtx.bbox.left =   g->real_mtx.left;
				h->mtx.bbox.bottom = g->real_mtx.bottom;
				h->mtx.bbox.right =  g->real_mtx.right;
				h->mtx.bbox.top =    g->real_mtx.top;

				h->mtx.bbox.setby.left   = info;
				h->mtx.bbox.setby.bottom = info;
				h->mtx.bbox.setby.right  = info;
				h->mtx.bbox.setby.top    = info;
				}
			else
				{
				if (h->mtx.bbox.left > g->real_mtx.left)
					{
					h->mtx.bbox.left = g->real_mtx.left;
					h->mtx.bbox.setby.left = info;
					}
				if (h->mtx.bbox.bottom > g->real_mtx.bottom)
					{
					h->mtx.bbox.bottom = g->real_mtx.bottom;
					h->mtx.bbox.setby.bottom = info;
					}
				if (h->mtx.bbox.right < g->real_mtx.right)
					{
					h->mtx.bbox.right = g->real_mtx.right;
					h->mtx.bbox.setby.right = info;
					}
				if (h->mtx.bbox.top < g->real_mtx.top)
					{
					h->mtx.bbox.top = g->real_mtx.top;
					h->mtx.bbox.setby.top = info;
					}
				}
			}
		}
	}

/* Mtx mode callbacks template. */
static abfGlyphCallbacks mtxGlyphCallbacks =
{
    NULL,
    NULL,
    NULL,
    mtxGlyphBeg,
    mtxGlyphWidth,
    mtxGlyphMove,
    mtxGlyphLine,
    mtxGlyphCurve,
    NULL,
    NULL,
    mtxGlyphGenop,
    mtxGlyphSeac,
    mtxGlyphEnd,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};

/* Begin font set. */
static void mtx_BegSet(txCtx h)
	{
	}

/* Begin font. */
static void mtx_BegFont(txCtx h, abfTopDict *top)
	{
	dstFileOpen(h, top);

	h->mtx.bbox.left = 0;
	h->mtx.bbox.bottom = 0;
	h->mtx.bbox.right = 0;
	h->mtx.bbox.top = 0;

	if (top->sup.flags & ABF_CID_FONT)
		fprintf(h->dst.stm.fp,
				"### glyph[tag] {cid,fd,width,{left,bottom,right,top}}\n");
	else
		fprintf(h->dst.stm.fp,
				"### glyph[tag] {gname,enc,width,{left,bottom,right,top}}\n");
	}

/* End font. */
static void mtx_EndFont(txCtx h)
	{
	if (h->mtx.level > 1)
		{
		/* Print bbox information */
		fprintf(h->dst.stm.fp, "### aggregate\n");
		if (h->mtx.level == 2)
			fprintf(h->dst.stm.fp, "bbox  {%g,%g,%g,%g}\n",
					floor(h->mtx.bbox.left), floor(h->mtx.bbox.bottom),
					ceil(h->mtx.bbox.right), ceil(h->mtx.bbox.top));
		else
			fprintf(h->dst.stm.fp, "bbox  {%g,%g,%g,%g}\n",
					h->mtx.bbox.left, h->mtx.bbox.bottom,
					h->mtx.bbox.right, h->mtx.bbox.top);

		if (h->mtx.bbox.left != 0 ||
			h->mtx.bbox.bottom != 0 ||
			h->mtx.bbox.right != 0 ||
			h->mtx.bbox.top != 0)
			{
			/* bbox was set; print setting glyph(s) */
			fprintf(h->dst.stm.fp, "tag   {%hu,%hu,%hu,%hu}\n",
					h->mtx.bbox.setby.left->tag,
					h->mtx.bbox.setby.bottom->tag,
					h->mtx.bbox.setby.right->tag,
					h->mtx.bbox.setby.top->tag);
			if (h->top->sup.flags & ABF_CID_FONT)
				fprintf(h->dst.stm.fp, "cid   {%hu,%hu,%hu,%hu}\n",
						h->mtx.bbox.setby.left->cid,
						h->mtx.bbox.setby.bottom->cid,
						h->mtx.bbox.setby.right->cid,
						h->mtx.bbox.setby.top->cid);
			else
				fprintf(h->dst.stm.fp, "gname {%s,%s,%s,%s}\n",
						h->mtx.bbox.setby.left->gname.ptr,
						h->mtx.bbox.setby.bottom->gname.ptr,
						h->mtx.bbox.setby.right->gname.ptr,
						h->mtx.bbox.setby.top->gname.ptr);
			}
		}
	}

/* End font set. */
static void mtx_EndSet(txCtx h)
	{
	dstFileClose(h);
	}

/* Setup mtx mode. */
static void mtx_SetMode(txCtx h)
	{
	h->mtx.level = 0;

	/* Set mode name */
	h->modename	= "mtx";

	/* Set library functions */
	h->dst.begset	= mtx_BegSet;
	h->dst.begfont	= mtx_BegFont;
	h->dst.endfont	= mtx_EndFont;
	h->dst.endset	= mtx_EndSet;

	/* Initialize glyph callbacks */
	h->cb.glyph = mtxGlyphCallbacks;
	h->cb.glyph.direct_ctx = h;

	/* Initialize metrics facility */
	h->mtx.metrics.cb = abfGlyphMetricsCallbacks;
	h->mtx.metrics.cb.direct_ctx = &h->mtx.metrics.ctx;
	h->mtx.metrics.ctx.flags = 0;

	/* Set source libarary flags */
	h->t1r.flags = (T1R_UPDATE_OPS|T1R_USE_MATRIX);
	h->cfr.flags = (CFR_UPDATE_OPS|CFR_USE_MATRIX);

	h->mode = mode_mtx;
	}

/* Mode-specific help. */
static void mtx_Help(txCtx h)
	{
	static char *text[] =
		{
#include "mtx.h"
		};
	printText(ARRAY_LEN(text), text);
	}

/* --------------------------------- t1 mode ------------------------------- */

/* Begin font set. */
static void t1_BegSet(txCtx h)
	{
	}

/* Begin new glyph definition for patching glyph info. if -decid enabled. */
static int t1_GlyphBeg(abfGlyphCallbacks *cb, abfGlyphInfo *info)
{
	txCtx h = cb->indirect_ctx;
	char gname[64];
    unsigned int nameLen;


	if (info->flags & ABF_GLYPH_SEEN)
		return ABF_SKIP_RET;	/* Already in subset */

	if (h->t1w.fd == -1)
	{
		h->t1w.fd = info->iFD;	/* First glyph; set target fd */
	}
	else if (info->cid == 0) /* For CID 0, always just use current iFD. */
	{
		info->iFD = h->t1w.fd;
	}
	else if (info->iFD != h->t1w.fd)
		{
			if (h->t1w.options & T1W_USEFD)
				info->iFD = h->t1w.fd;
			else
				fatal(h, "selected glyphs span multiple FDs");
		}
	/* Create glyph name */
	if (info->cid == 0)
		strcpy(gname, ".notdef");
	else
		sprintf(gname, "cid%hu", info->cid);
    nameLen = strlen(gname) + 1;
    if ((h->t1w.gnames.size) < (h->t1w.gnames.cnt + nameLen))
    {
        dnaINDEX(h->t1w.gnames,h->t1w.gnames.size + nameLen);
         /* Update all the gname ptrs, as we just moved h->t1w.gnames.array,and all the info->gname.ptr are invalid. */
        t1wUpdateGlyphNames(h->t1w.ctx, h->t1w.gnames.array);
    }

    info->gname.ptr = &h->t1w.gnames.array[h->t1w.gnames.cnt];
    info->gname.impl = h->t1w.gnames.cnt; // hold current char index, to make renumbering this easier!
	strcpy(info->gname.ptr, gname);
	h->t1w.gnames.cnt += (long)nameLen;

	info->flags &= ~ABF_GLYPH_CID;	/* Convert to name-keyed glyph */

	return t1wGlyphCallbacks.beg(cb, info);
}

/* Begin font. */
static void t1_BegFont(txCtx h, abfTopDict *top)
	{
	h->t1w.maxGlyphs = 0;

	/* Initialize glyph callbacks */
    if (h->flags & PATH_REMOVE_OVERLAP)
        {
        h->cb.glyph = abfGlyphPathCallbacks;
        h->cb.glyph.direct_ctx = h->abf.ctx;
        }
    else
        {
        h->cb.glyph = t1wGlyphCallbacks;
        h->cb.glyph.direct_ctx = h->t1w.ctx;
        }
    if (h->flags & PATH_SUPRESS_HINTS)
    {
        h->cb.glyph.stem = NULL;
        h->cb.glyph.flex = NULL;
    }

	if (h->t1w.options & T1W_DECID)
		{
		/* Convert cid-keyed font to name-keyed font */
		if (!(top->sup.flags & ABF_CID_FONT))
			fatal(h, "-decid specified for non-CID font");

		/* Initialize */
		h->t1w.fd = -1;
		(void)dnaGROW(h->t1w.gnames,
					  ((h->subset.glyphs.cnt > 0)?
					   h->subset.glyphs.cnt: top->sup.nGlyphs)*8);
		h->t1w.gnames.cnt = 0;

		/* Replace callback */
		if (!(h->flags & PATH_REMOVE_OVERLAP))
            h->cb.glyph.beg = t1_GlyphBeg;
		h->cb.glyph.indirect_ctx = h;
		}
	if (h->t1w.options & T1W_WAS_EMBEDDED)
		top->WasEmbedded = 1;

	if (h->t1w.flags & T1W_TYPE_BASE)
		h->t1w.maxGlyphs = (top->sup.flags & ABF_CID_FONT)?
			top->cid.CIDCount: top->sup.nGlyphs;

	dstFileSetAutoName(h, top);

	/* Initialize glyph callbacks */
    if (h->flags & PATH_REMOVE_OVERLAP)
        {
        if (abfBegFont(h->abf.ctx, top))
            fatal(h, NULL);
        }
    else
        {
        if (t1wBegFont(h->t1w.ctx, h->t1w.flags, h->t1w.lenIV, h->t1w.maxGlyphs))
            fatal(h, NULL);
        }
	}

/* Copy length bytes from source file to destination file. */
static void copyFile(txCtx h, size_t length,
					 FILE *src, char *srcfile,
					 FILE *dst, char *dstfile)
	{
	char buf[BUFSIZ];
	size_t cnt;

	/* Write whole buffers */
	cnt = length/BUFSIZ;
	while (cnt--)
		if (fread(buf, 1, BUFSIZ, src) != BUFSIZ)
			fileError(h, srcfile);
		else if (fwrite(buf, 1, BUFSIZ, dst) != BUFSIZ)
			fileError(h, dstfile);

	/* Write partial buffers */
	cnt = length%BUFSIZ;
	if (fread(buf, 1, cnt, src) != cnt)
		fileError(h, srcfile);
	else if (fwrite(buf, 1, cnt, dst) != cnt)
		fileError(h, dstfile);
	}

/* Copy PFB segment. */
static void copyPFBSegment(txCtx h, int type, long length,
						   FILE *src, char *srcfile,
						   FILE *dst, char *dstfile)
	{
	/* Write segment header */
	putc(128, dst);
	putc(type, dst);
	putc(length, dst);
	putc(length>>8, dst);
	putc(length>>16, dst);
	putc(length>>24, dst);

	/* Copy segment data */
	copyFile(h, length, src, srcfile, dst, dstfile);
	}

/* Write PFB format file. */
static void writePFB(txCtx h, FILE *font, char *fontfile,
					 long begBinary, long begTrailer, long endTrailer)
	{
	char *tmpfil = "(t1w) reformat tmpfil";
	FILE *tmp = _tmpfile();
	if (tmp == NULL)
		fileError(h, tmpfil);

	/* Write pfb font to tmp file */
	copyPFBSegment(h, 1, begBinary, font, fontfile, tmp, tmpfil);
	copyPFBSegment(h, 2, begTrailer - begBinary, font, fontfile, tmp, tmpfil);
	copyPFBSegment(h, 1, endTrailer - begTrailer, font, fontfile, tmp, tmpfil);
	copyPFBSegment(h, 3, 0, font, fontfile, tmp, tmpfil);

	/* Copy tmp file to font file */
	font = freopen(fontfile, "wb", font);
	if (font == NULL)
		fileError(h, fontfile);
	rewind(tmp);
	copyFile(h, endTrailer + 4*(1 + 1 + 4), tmp, tmpfil, font, fontfile);

	/* Close tmp file */
	if (fclose(tmp) == EOF)
		fileError(h, tmpfil);
	}

/* Write 2-byte big-endian number. */
static void write2(FILE *fp, unsigned short value)
	{
	putc(value>>8, fp);
	putc(value, fp);
	}

/* Write 4-byte big-endian number. */
static void write4(FILE *fp, unsigned long value)
	{
	putc(value>>24, fp);
	putc(value>>16, fp);
	putc(value>>8, fp);
	putc(value, fp);
	}

/* Write padding bytes */
static void writePad(FILE *fp, long cnt)
	{
	while (cnt--)
		putc(0, fp);
	}

/* Copy POST resource. */
static void copyPOSTRes(txCtx h, int type, long length,
						FILE *src, char *srcfile,
						FILE *dst, char *dstfile)
	{
	/* Write resource header */
	write4(dst, 1 + 1 + length);
	putc(type, dst);
	putc(0, dst);

	/* Copy resource data */
	copyFile(h, length, src, srcfile, dst, dstfile);
	}

/* Copy POST resource. */
static void writeSection(txCtx h, int type, long length,
						 FILE *src, char *srcfile,
						 FILE *dst, char *dstfile)
	{
	/* Write full-length resouces */
	int cnt = length/2046;
	while (cnt--)
		copyPOSTRes(h, type, 2046, src, srcfile, dst, dstfile);

	/* Write partial resource */
	cnt = length%2046;
	if (cnt > 0 || length == 0)
		copyPOSTRes(h, type, cnt, src, srcfile, dst, dstfile);
	}

/* Write reference. */
static void writeRef(FILE *fp, int *id, long *offset, long length)
	{
	write2(fp, (unsigned short)((*id)++));
	write2(fp, (unsigned short)-1);
	putc(0, fp);
	putc(*offset>>16, fp);
	putc(*offset>>8, fp);
	putc(*offset, fp);
	write4(fp, 0);
	*offset += length + (4 + 1 + 1);
	}

/* Write references. */
static void writeRefs(FILE *fp, int *id, long *offset, long length)
	{
	int cnt = length/2046;
	while (cnt--)
		writeRef(fp, id, offset, 2046);

	cnt = length%2046;
	if (cnt > 0 || length == 0)
		writeRef(fp, id, offset, cnt);
	}

/* Write LWFN format file. */
static void writeLWFN(txCtx h, FILE *font, char *fontfile,
					  long begBinary, long begTrailer, long endTrailer)
	{
	enum
		{
		MAP_HEADER_LEN	= 16 + 4 + 4*2,
		TYPE_LIST_LEN	= 2 + 4 + 2*2,
		REFERENCE_LEN	= 2*2 + 1 + 3 + 4
		};
	int id;
	long offset;
	long length0 = begBinary;					/* Text header */
	long length1 = begTrailer - begBinary;		/* Binary section */
	long length2 = endTrailer - begTrailer;		/* Text trailer */
	long length3 = 0;							/* End-of-font program */
	int rescnt = ((length0 + 2045)/2046 +
				  (length1 + 2045)/2046 +
				  (length2 + 2045)/2046 +
				  1);
	long datalen = rescnt*(4 + 1 + 1) + endTrailer;
	long maplen = MAP_HEADER_LEN + TYPE_LIST_LEN + rescnt*REFERENCE_LEN;
	char *tmpfil = "(t1w) reformat tmpfile";
	FILE *tmp = _tmpfile();
	if (tmp == NULL)
		fileError(h, tmpfil);

	/* Write resource header */
	write4(tmp, 256);
	write4(tmp, 256 + datalen);
	write4(tmp, datalen);
	write4(tmp, maplen);

	/* Write system and application data area */
	writePad(tmp, 112 + 128);

	/* Write resources */
	writeSection(h, 1, length0, font, fontfile, tmp, tmpfil);
	writeSection(h, 2, length1, font, fontfile, tmp, tmpfil);
	writeSection(h, 1, length2, font, fontfile, tmp, tmpfil);
	writeSection(h, 5, length3, font, fontfile, tmp, tmpfil);

	/* Write map header */
	writePad(tmp, 16 + 4 + 2);
	write2(tmp, 0);
	write2(tmp, MAP_HEADER_LEN);
	write2(tmp, (unsigned short)maplen);

	/* Write type list */
	write2(tmp, 0);
	write4(tmp, CTL_TAG('P','O','S','T'));
	write2(tmp, (unsigned short)(rescnt - 1));
	write2(tmp, TYPE_LIST_LEN);

	/* Write reference list */
	id = 501;
	offset = 0;
	writeRefs(tmp, &id, &offset, length0);
	writeRefs(tmp, &id, &offset, length1);
	writeRefs(tmp, &id, &offset, length2);
	writeRefs(tmp, &id, &offset, length3);

	/* Name list empty */

	/* Copy tmp file to font file */
	font = freopen(fontfile, "wb", font);
	if (font == NULL)
		fileError(h, fontfile);
	rewind(tmp);
	copyFile(h, 256 + datalen + maplen, tmp, tmpfil, font, fontfile);

	/* Close tmp file */
	if (fclose(tmp) == EOF)
		fileError(h, tmpfil);
	}

/* Perform platform-specific reformatting of file. */
static void t1_Reformat(txCtx h)
	{
	int state;
	long begBinary;		/* Offset to start of binary section */
	long begTrailer;	/* Offset to start of trailer */
	long endTrailer;	/* Offset to EOF */
	char *fontfile = h->dst.stm.filename;
	FILE *font = fopen(fontfile, "rb");
	if (font == NULL)
		fileError(h, fontfile);

	/* Find "eexec " string */
	state = 0;
	for (;;)
		switch (fgetc(font))
			{
		case 'e':
			switch (state)
				{
			case 0:
				state = 1;
				break;
			case 1:
				state = 2;
				break;
			case 3:
				state = 4;
				break;
			default:
				state = 0;
				break;
				}
			break;
		case 'x':
			state = (state == 2)? 3: 0;
			break;
		case 'c':
			state = (state == 4)? 5: 0;
			break;
		case ' ':
			if (state == 5)
				{
				begBinary = ftell(font);
				if (begBinary == -1)
					fileError(h, fontfile);
				goto findTrailer;
				}
			else
				state = 0;
			break;
		case EOF:
			if (feof(font))
				fatal(h, "can't find eexec");
			else
				fileError(h, fontfile);
			break;
		}

 findTrailer:
	/* Find a line of 64 '0' chars */
	state = 0;
	for (;;)
		switch (fgetc(font))
			{
		case '0':
			if (++state == 64)
				{
				begTrailer = ftell(font);
				if (begTrailer == -1)
					fileError(h, fontfile);
				begTrailer -= 64;
				goto findEOF;
				}
			break;
		case EOF:
			if (feof(font))
				fatal(h, "can't find tailer");
			else
				fileError(h, fontfile);
			break;
		default:
			state = 0;
			break;
		}

 findEOF:
	/* Find end-of-file */
	if (fseek(font, 0, SEEK_END))
		fileError(h, fontfile);
	endTrailer = ftell(font);
	if (endTrailer == -1)
		fileError(h, fontfile);

	rewind(font);
	switch (h->t1w.flags & T1W_NEWLINE_MASK)
		{
	case T1W_NEWLINE_WIN:
		writePFB(h, font, fontfile, begBinary, begTrailer, endTrailer);
		break;
	case T1W_NEWLINE_MAC:
		writeLWFN(h, font, fontfile, begBinary, begTrailer, endTrailer);
		break;
		}

	if (fclose(font) == EOF)
		fileError(h, fontfile);
	}

/* End font. */
static void t1_EndFont(txCtx h)
	{
	if (h->t1w.options & T1W_DECID)
		{
		/* Add .notdef (if not already added) */
		if (h->src.type == src_Type1)
			(void)t1rGetGlyphByCID(h->t1r.ctx, 0, &h->cb.glyph);

		else if (h->src.type == src_CFF)
			(void)cfrGetGlyphByCID(h->cfr.ctx, 0, &h->cb.glyph);

		/* Convert to name-keyed font */
		h->top->sup.flags &= ~ABF_CID_FONT;
		h->top->FDArray.cnt = 1;
		h->top->FDArray.array = &h->top->FDArray.array[h->t1w.fd];
		h->t1w.options |= T1W_NO_UID;	/* Clear UniqueIDs */
 		}

	if (h->t1w.options & T1W_NO_UID)
		{
		h->top->UniqueID = ABF_UNSET_INT;
		h->top->XUID.cnt = ABF_EMPTY_ARRAY;
		h->top->cid.UIDBase = ABF_UNSET_INT;
		}

	if (h->t1w.options & T1W_REFORMAT &&
		strcmp(h->dst.stm.filename, "-") == 0)
		fatal(h, "stdout can't be used with -pfb or -LWFN options");

    if (!(h->flags & PATH_REMOVE_OVERLAP))
        {
        if (t1wEndFont(h->t1w.ctx, h->top))
            fatal(h, NULL);
        }
    else
        {
        if (t1wBegFont(h->t1w.ctx, h->t1w.flags, h->t1w.lenIV, h->t1w.maxGlyphs))
            fatal(h, NULL);

        resetGlyphs(h);
        h->cb.glyph = t1wGlyphCallbacks;
        if (h->flags & PATH_SUPRESS_HINTS)
        {
            h->cb.glyph.stem = NULL;
            h->cb.glyph.flex = NULL;
        }
        h->cb.glyph.direct_ctx = h->t1w.ctx;
        h->cb.glyph.indirect_ctx = h;
        if (h->t1w.options & T1W_DECID)
            h->cb.glyph.beg = t1_GlyphBeg;
        if (abfEndFont(h->abf.ctx, ABF_PATH_REMOVE_OVERLAP, &h->cb.glyph))
            fatal(h, NULL);

        if (t1wEndFont(h->t1w.ctx, h->top))
            fatal(h, NULL);
        }

	if (h->t1w.options & T1W_REFORMAT)
		t1_Reformat(h);
	}

/* End font set. */
static void t1_EndSet(txCtx h)
	{
	}

/* Stream SING glyphlet callback. */
static int get_stream(t1wSINGCallback *sing_cb)
	{
	long length;
	txCtx h = sing_cb->ctx;
	FILE *fp = h->src.stm.fp;

	/* Get file size and seek to start */
	if (fseek(fp, 0, SEEK_END) != 0 ||
		(length = ftell(fp)) == -1 ||
		fseek(fp, 0, SEEK_SET) != 0)
		return 1;

	/* Update returned data */
	sing_cb->stm = &h->src.stm;
	sing_cb->length = length;

	return 0;
	}

/* Setup t1 mode. */
static void t1_SetMode(txCtx h)
	{
	/* Initialize control data */
	/* This is now set at the start of parseArgs
	h->t1w.options = 0;
	*/
	h->t1w.flags = (T1W_TYPE_HOST|
					T1W_ENCODE_ASCII|
					T1W_OTHERSUBRS_PRIVATE|
					T1W_NEWLINE_UNIX);
	h->t1w.lenIV = 4;
	h->t1w.fd = -1;
	/* Set mode name */
	h->modename	= "t1";

	/* Set library functions */
	h->dst.begset	= t1_BegSet;
	h->dst.begfont	= t1_BegFont;
	h->dst.endfont	= t1_EndFont;
	h->dst.endset	= t1_EndSet;

	if (h->t1w.ctx == NULL)
		{
		/* Create library context */
		t1wSINGCallback sing_cb;
		sing_cb.ctx = h;
		sing_cb.stm = 0;
		sing_cb.length = 0;
		sing_cb.get_stream = get_stream;
		h->t1w.ctx = t1wNew(&h->cb.mem, &h->cb.stm, T1W_CHECK_ARGS);
		if (h->t1w.ctx == NULL || t1wSetSINGCallback(h->t1w.ctx, &sing_cb))
			fatal(h, "(t1w) can't init lib");
		}

    if (h->abf.ctx == NULL)
        {
        /* Create library context */
        h->abf.ctx = abfNew(&h->cb.mem, ABF_CHECK_ARGS);
        if (h->abf.ctx == NULL)
            fatal(h, "(abf) can't init lib");
        }

	/* Set source library flags */
	/* These are now set at the start of parseArgs
	h->t1r.flags = 0;
	h->cfr.flags = 0;
	*/

	h->mode = mode_t1;
	}

/* Mode-specific help. */
static void t1_Help(txCtx h)
	{
	static char *text[] =
		{
#include "t1.h"
		};
	printText(ARRAY_LEN(text), text);
	}

/* -------------------------------- svg mode ------------------------------- */

/* Begin font set. */
static void svg_BegSet(txCtx h)
	{
	}

/* Begin new glyph definition. */
static int svg_GlyphBeg(abfGlyphCallbacks *cb, abfGlyphInfo *info)
	{
	txCtx h = cb->indirect_ctx;

	/* Determine (or fabricate) Unicode value for glyph. */
	info->flags |= ABF_GLYPH_UNICODE;
	if (h->top->sup.flags & ABF_CID_FONT)
		{
		info->encoding.code = h->svw.unrec;
		h->svw.unrec++;
		}
	else
		info->encoding.code = mapName2UV(h, info->gname.ptr, &h->svw.unrec);

	return svwGlyphCallbacks.beg(cb, info);
	}

/* Begin font. */
static void svg_BegFont(txCtx h, abfTopDict *top)
	{
	h->cb.glyph.beg = svg_GlyphBeg;
	h->cb.glyph.indirect_ctx = h;

	h->svw.unrec = 0xE000;	/* Start of Private Use Area */

	dstFileSetAutoName(h, top);
	if (svwBegFont(h->svw.ctx, h->svw.flags))
		fatal(h, NULL);
	}

/* End font. */
static void svg_EndFont(txCtx h)
	{
	if (svwEndFont(h->svw.ctx, h->top))
		fatal(h, NULL);
	}

/* End font set. */
static void svg_EndSet(txCtx h)
	{
	}

/* Setup svg mode. */
static void svg_SetMode(txCtx h)
	{
	/* Initialize control data */
	h->svw.options = 0;
	h->svw.flags = SVW_NEWLINE_UNIX;

	/* Set mode name */
	h->modename	= "svg";

	/* Set library functions */
	h->dst.begset	= svg_BegSet;
	h->dst.begfont	= svg_BegFont;
	h->dst.endfont	= svg_EndFont;
	h->dst.endset	= svg_EndSet;

	if (h->svw.ctx == NULL)
		{
		/* Create library context */
		h->svw.ctx = svwNew(&h->cb.mem, &h->cb.stm, SVW_CHECK_ARGS);
		if (h->svw.ctx == NULL)
			fatal(h, "(svw) can't init lib");
		}

	/* Initialize glyph callbacks */
	h->cb.glyph = svwGlyphCallbacks;
	h->cb.glyph.direct_ctx = h->svw.ctx;

	/* Set source library flags */
	h->t1r.flags |= T1R_UPDATE_OPS;
	h->cfr.flags |= CFR_UPDATE_OPS;

	h->mode = mode_svg;
	}

/* Mode-specific help. */
static void svg_Help(txCtx h)
	{
	static char *text[] =
		{
#include "svg.h"
		};
	printText(ARRAY_LEN(text), text);
	}


/* -------------------------------- ufo write mode ------------------------------- */

static void mkdir_tx(txCtx h, char* dirPath)
{
	int dirErr;
#ifdef _WIN32
	dirErr = _mkdir(dirPath);
#else
	dirErr = mkdir(dirPath,  0000700 | 0000070 | 0000007);
#endif

	if (dirErr != 0)
		fatal(h, "Failed to creater directory '%s'.\n", dirPath);
}

/* Begin font set. */
static void ufw_BegSet(txCtx h)
{
}

/* Begin new glyph definition. */
static int ufw_GlyphBeg(abfGlyphCallbacks *cb, abfGlyphInfo *info)
{
	return ufwGlyphCallbacks.beg(cb, info);
}

/* Begin font. */
static void ufw_BegFont(txCtx h, abfTopDict *top)
{
    struct stat fileStat;
    int statErrNo;

	h->cb.glyph.beg = ufw_GlyphBeg;
	h->cb.glyph.indirect_ctx = h;

    /* Make sure the user isn't sending the output to std out */
    if (strcmp(h->dst.stm.filename, "-") == 0)
    {
        fatal(h, "Please specify a file path for the destination UFO font. UFO fonts cannot be serialized to stdout.");
    }
    /* if the UFO parent dir does not exist, make it.
     If it does exist, complain and quit. */
    statErrNo = stat(h->dst.stm.filename,  &fileStat);
    if (statErrNo == 0)
    {
        fatal(h, "Destination UFO font already exists:  %s.\n", h->dst.stm.filename);
    }
    else
    {
		char buffer[FILENAME_MAX];
        mkdir_tx(h, h->dst.stm.filename);
        if (h->ufr.altLayerDir != NULL)
            sprintf(buffer, "%s/%s", h->file.dst, h->ufr.altLayerDir);
        else
            sprintf(buffer, "%s/%s", h->file.dst, "glyphs");
        mkdir_tx(h, buffer);

    }

	dstFileSetAutoName(h, top);
	if (ufwBegFont(h->ufow.ctx, h->ufow.flags, h->ufr.altLayerDir))
		fatal(h, NULL);
}

/* End font. */
static void ufw_EndFont(txCtx h)
{
	if (ufwEndFont(h->ufow.ctx, h->top))
		fatal(h, NULL);
}

/* End font set. */
static void ufw_EndSet(txCtx h)
{
}


static void ufo_SetMode(txCtx h)
{
	/* Initialize control data */
	h->ufow.flags = 0;

	/* Set mode name */
	h->modename	= "ufo_w";

	/* Set library functions */
	h->dst.begset	= ufw_BegSet;
	h->dst.begfont	= ufw_BegFont;
	h->dst.endfont	= ufw_EndFont;
	h->dst.endset	= ufw_EndSet;

	if (h->ufow.ctx == NULL)
    {
		/* Create library context */
		h->ufow.ctx = ufwNew(&h->cb.mem, &h->cb.stm, UFW_CHECK_ARGS);
		if (h->ufow.ctx == NULL)
			fatal(h, "(ufow) can't init lib");
    }

	/* Initialize glyph callbacks */
	h->cb.glyph = ufwGlyphCallbacks;
	h->cb.glyph.direct_ctx = h->ufow.ctx;

	/* Set source library flags. It is harmless to declare a font to be cune, and we have to flatten them if going to UFO */
	h->t1r.flags = T1R_UPDATE_OPS|T1R_FLATTEN_CUBE|T1R_IS_CUBE;
	h->cfr.flags = CFR_UPDATE_OPS|CFR_FLATTEN_CUBE|T1R_IS_CUBE;

	h->mode = mode_ufow;
}

/* Mode-specific help. */
static void ufo_Help(txCtx h)
{
	static char *text[] =
    {
#include "ufo.h"
    };
	printText(ARRAY_LEN(text), text);
}

/* -------------------------------- dcf mode ------------------------------- */

#define tx_reserved2  2
#define t2_reserved17 17


/* Return size of region. */
static long sizeRegion(const ctlRegion *region)
{
	return region->end - region->begin;
}


/* Begin flowed text. */
static void flowBeg(txCtx h)
	{
	h->dcf.sep = "";
	}

/* Title flowed text. */
static void flowTitle(txCtx h, char *title)
	{
	flowBeg(h);
	fprintf(h->dst.stm.fp, "--- %s\n", title);
	}

/* End flowed text. */
static void flowEnd(txCtx h)
	{
	fprintf(h->dst.stm.fp, "\n");
	}

/* Add flowed text. */
static void flowAdd(txCtx h, char *fmt, ...)
	{
	va_list ap;
	if (h->dcf.flags & DCF_SaveStemCnt)
		return;
	va_start(ap, fmt);
	vfprintf(h->dst.stm.fp, fmt, ap);
	va_end(ap);
	}

/* Break flowed text. */
static void flowBreak(txCtx h, char *fmt, ...)
	{
	va_list ap;
	va_start(ap, fmt);
	fprintf(h->dst.stm.fp, "%s", h->dcf.sep);
	vfprintf(h->dst.stm.fp, fmt, ap);
	va_end(ap);
	h->dcf.sep = (h->dcf.flags & DCF_BreakFlowed)? "\n": " ";
	}

/* Begin flowed element. */
static void flowElemBeg(txCtx h, long index, int complex)
	{
	fprintf(h->dst.stm.fp, "%s[%ld]={%s", h->dcf.sep, index,
			(complex && (h->dcf.flags & DCF_BreakFlowed))? "\n": "");
	}

/* Add argument to flowed element. */
static void flowArg(txCtx h, char *fmt, ...)
	{
	va_list ap;
	if (h->dcf.flags & DCF_SaveStemCnt)
		return;
	va_start(ap, fmt);
	fprintf(h->dst.stm.fp, "%s", h->dcf.sep);
	vfprintf(h->dst.stm.fp, fmt, ap);
	va_end(ap);
	h->dcf.sep = " ";
	}

/* Add operator to flowed element. */
static void flowOp(txCtx h, char *fmt, ...)
	{
	va_list ap;
	if (h->dcf.flags & DCF_SaveStemCnt)
		return;
	va_start(ap, fmt);
	fprintf(h->dst.stm.fp, "%s", h->dcf.sep);
	vfprintf(h->dst.stm.fp, fmt, ap);
	va_end(ap);
	h->dcf.sep = (h->dcf.flags & DCF_BreakFlowed)? "\n  ": " ";
	}

/* End flowed element. */
static void flowElemEnd(txCtx h)
	{
	fprintf(h->dst.stm.fp, "}");
	h->dcf.sep = (h->dcf.flags & DCF_BreakFlowed)? "\n": " ";
	}

/* Begin font set. */
static void dcf_BegSet(txCtx h)
	{
	}

/* Dump table tag line. */
static void dumpTagLine(txCtx h, char *title, const ctlRegion *region)
	{
	if (h->dcf.level < 1)
		{
		static char dots[] = " ................";
		fprintf(h->dst.stm.fp, "### %s%.*s (%08lx-%08lx)\n",
				title, (int)(sizeof(dots) - 1 - strlen(title)), dots,
				region->begin, region->end - 1L);
		}
	else
		fprintf(h->dst.stm.fp, "### %s (%08lx-%08lx)\n",
				title, region->begin, region->end - 1L);
	}

/* Dump CFF header. */
static unsigned short dcf_DumpHeader(txCtx h, const ctlRegion *region)
	{
    unsigned short major = 0;
	FILE *fp = h->dst.stm.fp;
    bufSeek(h, region->begin);
    major = read1(h);

	if (!(h->dcf.flags & DCF_Header) || region->begin == -1)
		return major;

	dumpTagLine(h, "Header", region);
	if (h->dcf.level < 1)
		return major;

	fprintf(fp, "major  =%u\n", major);
	fprintf(fp, "minor  =%u\n", read1(h));
	fprintf(fp, "hdrSize=%u\n", read1(h));
	fprintf(fp, "offSize=%u\n", read1(h));
    return major;
	}

/* Dump CFF INDEX. */
static void dumpINDEX(txCtx h, char *title, const ctlRegion *region,
					  DumpElementProc dumpElement)
	{
	dumpTagLine(h, title, region);
	if (h->dcf.level < 1)
		return;
	else
		{
		unsigned long i;
		long offset;
		long dataref;
        unsigned long count;
        unsigned short countSize;
		unsigned char offSize = 0;	/* Suppress optimizer warning */
		ctlRegion element;
		FILE *fp = h->dst.stm.fp;

		/* Read header */
		bufSeek(h, region->begin);


        if (h->dcf.flags & DCF_IS_CFF2)
        {
            countSize = 4;
            count = read4(h);
        }
        else
        {
            count = read2(h);
            countSize = 2;
        }

        if (count > 0)
            offSize = read1(h);

		if (h->dcf.level < 5)
			{
			/* Dump header */
			fprintf(fp, "count  =%lu\n", count);
			if (count == 0)
				return;
			fprintf(fp, "offSize=%u\n", offSize);

			/* Dump offset array */
			flowTitle(h, "offset[index]=value");
			for (i = 0; i <= count; i++)
				flowBreak(h, "[%ld]=%lu", i, readn(h, offSize));
			flowEnd(h);
			}
		else if (count == 0)
			{
			fprintf(fp, "empty\n");
			return;
			}

		/* Compute offset array base and data reference offset */
		offset = region->begin + countSize + 1;
		dataref = offset + (count + 1)*offSize - 1;

		/* Dump object data */
		flowTitle(h, "object[index]={value}");
		bufSeek(h, offset);
		element.begin = dataref + readn(h, offSize);
		offset += offSize;
		for (i = 0; i < count; i++)
			{
			bufSeek(h, offset);
			element.end = dataref + readn(h, offSize);
			offset += offSize;
			dumpElement(h, i, &element);
			element.begin = element.end;
			}
		flowEnd(h);
		}
	}

/* Dump string data. */
static void dumpString(txCtx h, const ctlRegion *region)
	{
	long cnt = sizeRegion(region);
	bufSeek(h, region->begin);
	while (cnt-- > 0)
		flowAdd(h, "%c", read1(h));
	}

/* Dump Name INDEX element. */
static void dumpNameElement(txCtx h, long index, const ctlRegion *region)
	{
	flowElemBeg(h, index, 0);
	dumpString(h, region);
	flowElemEnd(h);
	}

/* Dump Name INDEX table. */
static void dcf_DumpNameINDEX(txCtx h, const ctlRegion *region)
	{
	if (!(h->dcf.flags & DCF_NameINDEX) || region->begin == -1)
		return;

	dumpINDEX(h, "Name INDEX", region, dumpNameElement);
	}

/* Dump CFF DICT. */
static void dumpDICT(txCtx h, const ctlRegion *region)
	{
	static char *opname[32] =
		{
		/*  0 */ "version",
		/*  1 */ "Notice",
		/*  2 */ "FullName",
		/*  3 */ "FamilyName",
		/*  4 */ "Weight",
		/*  5 */ "FontBBox",
		/*  6 */ "BlueValues",
		/*  7 */ "OtherBlues",
		/*  8 */ "FamilyBlues",
		/*  9 */ "FamilyOtherBlues",
		/* 10 */ "StdHW",
		/* 11 */ "StdVW",
		/* 12 */ "escape",
		/* 13 */ "UniqueID",
		/* 14 */ "XUID",
		/* 15 */ "charset",
		/* 16 */ "Encoding",
		/* 17 */ "CharStrings",
		/* 18 */ "Private",
		/* 19 */ "Subrs",
		/* 20 */ "defaultWidthX",
		/* 21 */ "nominalWidthX",
		/* 22 */ "vsindex",
		/* 23 */ "blend",
		/* 24 */ "VarStore",
		/* 25 */ "maxstack",
		/* 26 */ "reserved26",
		/* 27 */ "reserved27",
		/* 28 */ "shortint",
		/* 29 */ "longint",
		/* 30 */ "real",
		/* 31 */ "blendLE",
		};
	static char *escopname[] =
		{
		/*  0 */ "Copyright",
		/*  1 */ "isFixedPitch",
		/*  2 */ "ItalicAngle",
		/*  3 */ "UnderlinePosition",
		/*  4 */ "UnderlineThickness",
		/*  5 */ "PaintType",
		/*  6 */ "CharstringType",
		/*  7 */ "FontMatrix",
		/*  8 */ "StrokeWidth",
		/*  9 */ "BlueScale",
		/* 10 */ "BlueShift",
		/* 11 */ "BlueFuzz",
		/* 12 */ "StemSnapH",
		/* 13 */ "StemSnapV",
		/* 14 */ "ForceBold",
		/* 15 */ "reservedESC15",
		/* 16 */ "lenIV",
		/* 17 */ "LanguageGroup",
		/* 18 */ "ExpansionFactor",
		/* 19 */ "initialRandomSeed",
		/* 20 */ "SyntheticBase",
		/* 21 */ "PostScript",
		/* 22 */ "BaseFontName",
		/* 23 */ "BaseFontBlend",
		/* 24 */ "numMasters",
		/* 25 */ "reservedESC25",
		/* 26 */ "reservedESC26",
		/* 27 */ "reservedESC27",
		/* 28 */ "reservedESC28",
		/* 29 */ "reservedESC29",
		/* 30 */ "ROS",
		/* 31 */ "CIDFontVersion",
		/* 32 */ "CIDFontRevision",
		/* 33 */ "CIDFontType",
		/* 34 */ "CIDCount",
		/* 35 */ "UIDBase",
		/* 36 */ "FDArray",
		/* 37 */ "FDIndex",
		/* 38 */ "FontName",
		/* 39 */ "Chameleon",
		};
	long left = sizeRegion(region);

	h->dcf.sep = (h->dcf.flags & DCF_BreakFlowed)? "  ": "";
	bufSeek(h, region->begin);
	while (left > 0)
		{
		unsigned char byte = read1(h);
		left--;
		switch (byte)
			{
		case cff_version:
		case cff_Notice:
		case cff_FullName:
		case cff_FamilyName:
		case cff_Weight:
		case cff_FontBBox:
		case cff_OtherBlues:
		case cff_FamilyBlues:
		case cff_FamilyOtherBlues:
		case cff_StdHW:
		case cff_StdVW:
		case cff_UniqueID:
		case cff_XUID:
		case cff_charset:
		case cff_Encoding:
		case cff_CharStrings:
		case cff_Private:
		case cff_Subrs:
		case cff_defaultWidthX:
		case cff_nominalWidthX:
		case cff_vsindex:
        case cff_blend:
        case cff_VarStore:
        case cff_maxstack:
		case cff_reserved26:
		case cff_reserved27:
            flowOp(h, opname[byte]);
            break;
        case cff_BlueValues:
        case cff_BlendLE:
            flowOp(h, opname[byte]);
            break;
		case cff_escape:
			{
			/* Process escaped operator */
			unsigned char escop = read1(h);
			left--;
			if (escop > ARRAY_LEN(escopname) - 1)
				flowOp(h, "reservedESC%d", escop);
			else
				flowOp(h, escopname[escop]);
			break;
			}
		case cff_shortint:
			{
			/* 3-byte number */
			short value = read1(h);
			value = value<<8 | read1(h);
			left -= 2;
#if SHRT_MAX > 32767
			/* short greater that 2 bytes; handle negative range */
			if (value > 32767)
				value -= 65536;
#endif
			flowArg(h, "%hd", value);
			}
			break;
		case cff_longint:
			{
			/* 5-byte number */
			long value = read1(h);
			value = value<<8 | read1(h);
			value = value<<8 | read1(h);
			value = value<<8 | read1(h);
			left -= 4;
#if LONG_MAX > 2147483647
			/* long greater that 4 bytes; handle negative range */
			if (value > 2417483647)
				value -= 4294967296;
#endif
			flowArg(h, "%ld", value);
			}
			break;
		case cff_BCD:
			{
			int count = 0;
			int byte = 0;	/* Suppress optimizer warning */
			flowArg(h, "");
			for (;;)
				{
				int nibble;

				if (count++ & 1)
					nibble = byte & 0xf;
				else
					{
					byte = read1(h);
					left--;
					nibble = byte>>4;
					}
				if (nibble == 0xf)
					break;

				flowAdd(h, "%c", "0123456789.EE?-?"[nibble]);
				if (nibble == 0xc)
					flowAdd(h, "-");
				}
			}
			break;
		case 247:
		case 248:
		case 249:
		case 250:
			/* Positive 2-byte number */
			flowArg(h, "%d", 108 + 256*(byte - 247) + read1(h));
			left--;
			break;
		case 251:
		case 252:
		case 253:
		case 254:
			/* Negative 2-byte number */
			flowArg(h, "%d", -108 - 256*(byte - 251) - read1(h));
			left--;
			break;
		case 255:
			flowOp(h, "reserved255");
			break;
		default:
			/* 1-byte number */
			flowArg(h, "%d", byte - 139);
			break;
			}
		}
	if (h->dcf.flags & DCF_BreakFlowed)
		fprintf(h->dst.stm.fp, "\n");
	}

/* Dump DICT element. */
static void dumpDICTElement(txCtx h, long index, const ctlRegion *region)
	{
	flowElemBeg(h, index, 1);
	dumpDICT(h, region);
	flowElemEnd(h);
	}

/* Dump Top DICT INDEX table. */
static void dcf_DumpTopDICTINDEX(txCtx h, const ctlRegion *region)
	{
	if (!(h->dcf.flags & DCF_TopDICTINDEX) || region->begin == -1)
		return;

	dumpINDEX(h, "Top DICT INDEX", region, dumpDICTElement);
	}

static void dcf_DumpTopDICT2(txCtx h, const ctlRegion *region)
{
    if (!(h->dcf.flags & DCF_TopDICTINDEX) || region->begin == -1)
        return;

    dumpTagLine(h, "Top DICT Data", region);
    dumpDICTElement(h, 0, region);
    flowEnd(h);
}


/* Dump String INDEX element. */
static void dumpStringElement(txCtx h, long index, const ctlRegion *region)
	{
	flowElemBeg(h, (h->dcf.level == 5)? ARRAY_LEN(sid2std) + index: index, 0);
	dumpString(h, region);
	flowElemEnd(h);
	}

/* Dump String INDEX table. */
static void dcf_DumpStringINDEX(txCtx h, const ctlRegion *region)
	{
	if (!(h->dcf.flags & DCF_StringINDEX) || region->begin == -1)
		return;

	dumpINDEX(h, "String INDEX", region, dumpStringElement);
	}

/* Flow stack args. */
static void flowStack(txCtx h)
	{
	long i;
	for (i = 0; i < h->stack.cnt; i++)
		flowArg(h, "%g", INDEX(i));
	h->stack.cnt = 0;
	}

/* Flow stack args and operator. */
static void flowCommand(txCtx h, char *opname)
	{
	flowStack(h);
	flowOp(h, opname);
	}

/* Call subr. */
static void callsubr(txCtx h,
					 SubrInfo *info, const ctlRegion *caller, long left)
	{
    long arg;
	ctlRegion callee;
	long saveoff = caller->end - left;

	/* Validate and unbias subr number */
	CHKUFLOW(1);
	arg = info->bias + (long)POP();
	if (arg < 0 || (arg >= (long)info->count))
		fatal(h, "invalid subr call");

	/* Compute subr region */
	bufSeek(h, info->offset + arg*info->offSize);
	callee.begin = info->dataref + readn(h, info->offSize);
	callee.end   = info->dataref + readn(h, info->offSize);

	/* Dump subr region */
	dumpCstr(h, &callee, 1);

	if (h->dcf.flags & DCF_SaveStemCnt)
		info->stemcnt.array[arg] = (unsigned char)h->dcf.stemcnt;

	if (left > 0)
		bufSeek(h, saveoff);
	}

/* Dump Type 2 charstring. */
static void dumpCstr(txCtx h, const ctlRegion *region, int inSubr)
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
		/*  9 */ "reserved9",
		/* 10 */ "callsubr",
		/* 11 */ "return",
		/* 12 */ "escape",
		/* 13 */ "reserved13",
		/* 14 */ "endchar",
		/* 15 */ "vsindex",
		/* 16 */ "blend",
		/* 17 */ "callgrel",
		/* 18 */ "hstemhm",
		/* 19 */ "hintmask",
		/* 20 */ "cntrmask",
		/* 21 */ "rmoveto",
		/* 22 */ "hmoveto",
		/* 23 */ "vstemhm",
		/* 24 */ "rcurveline",
		/* 25 */ "rlinecurve",
		/* 26 */ "vvcurveto",
		/* 27 */ "hhcurveto",
		/* 28 */ "shortint",
		/* 29 */ "callgsubr",
		/* 30 */ "vhcurveto",
		/* 31 */ "hvcurveto",
		};
	static char *escopname[] =
		{
		/*  0 */ "dotsection",
		/*  1 */ "reservedESC1",
		/*  2 */ "reservedESC2",
		/*  3 */ "and",
		/*  4 */ "or",
		/*  5 */ "not",
		/*  6 */ "reservedESC6",
		/*  7 */ "reservedESC7",
		/*  8 */ "reservedESC8",
		/*  9 */ "abs",
		/* 10 */ "add",
		/* 11 */ "sub",
		/* 12 */ "div",
		/* 13 */ "reservedESC13",
		/* 14 */ "neg",
		/* 15 */ "eq",
		/* 16 */ "reservedESC16",
		/* 17 */ "reservedESC17",
		/* 18 */ "drop",
		/* 19 */ "reservedESC19",
		/* 20 */ "put",
		/* 21 */ "get",
		/* 22 */ "ifelse",
		/* 23 */ "random",
		/* 24 */ "mul",
		/* 25 */ "reservedESC25",
		/* 26 */ "sqrt",
		/* 27 */ "dup",
		/* 28 */ "exch",
		/* 29 */ "index",
		/* 30 */ "roll",
		/* 31 */ "reservedESC31",
		/* 32 */ "reservedESC32",
		/* 33 */ "reservedESC33",
		/* 34 */ "hflex",
		/* 35 */ "flex",
		/* 36 */ "hflex1",
		/* 37 */ "flex1",
		/* 38 */ "cntron",
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
        /* 49 */ "setwvN",
        /* 50 */ "transform",
		};
	long left = sizeRegion(region);

	if (!inSubr)
		h->dcf.sep = (h->dcf.flags & DCF_BreakFlowed)? "  ": "";
	bufSeek(h, region->begin);
	while (left > 0)
		{
		unsigned char byte = read1(h);
		left--;
		switch (byte)
			{
		case tx_reserved0:
		case tx_rlineto:
		case tx_hlineto:
		case tx_vlineto:
		case tx_rrcurveto:
		case t2_reserved9:
		case t2_reserved13:
		case tx_endchar:
		case t2_rcurveline:
		case t2_rlinecurve:
		case t2_vvcurveto:
		case t2_hhcurveto:
		case tx_vhcurveto:
		case tx_hvcurveto:
			flowCommand(h, opname[byte]);
			break;
        case tx_vmoveto:
        case tx_rmoveto:
        case tx_hmoveto:
            h->dcf.flags |= DCF_END_HINTS;
            flowCommand(h, opname[byte]);
            break;

        case t2_blend:
            if ((h->dcf.flags & DCF_Flatten) && (!(h->dcf.flags & DCF_END_HINTS)))
            {
                int numBlends = (int)h->stack.array[h->stack.cnt-1];
                // take off 1 for num blend arguments, then pop the delta values: what's left are stem coords.
                h->dcf.stemcnt += (h->stack.cnt -1) - (numBlends* h->dcf.numRegions);
            }
            flowCommand(h, opname[byte]);
            break;
        case t2_vsindex:
            {
                unsigned long vsIndex = (unsigned long)h->stack.array[0];
                h->dcf.numRegions = h->dcf.varRegionInfo.array[vsIndex].regionCount;
                flowCommand(h, opname[byte]);
                break;
            }
		case tx_compose:
			flowCommand(h, opname[byte]);
			break;
		case tx_callgrel:
			flowCommand(h, opname[byte]);
			break;
		case tx_callsubr:
			if (h->dcf.flags & DCF_Flatten)
				callsubr(h, h->dcf.fd, region, left);
			else
				flowCommand(h, opname[byte]);
			break;
		case tx_return:
			if (!(h->dcf.flags & DCF_Flatten))
				flowCommand(h, opname[byte]);
			break;
		case t2_callgsubr:
			if (h->dcf.flags & DCF_Flatten)
				callsubr(h, &h->dcf.global, region, left);
			else
				flowCommand(h, opname[byte]);
			break;
		case tx_hstem:
		case tx_vstem:
		case t2_hstemhm:
		case t2_vstemhm:
			if (h->dcf.flags & DCF_Flatten)
				h->dcf.stemcnt += h->stack.cnt;
			flowCommand(h, opname[byte]);
			break;
		case t2_hintmask:
		case t2_cntrmask:
            if ((h->dcf.flags & DCF_Flatten) && (!(h->dcf.flags & DCF_END_HINTS)))
				h->dcf.stemcnt += h->stack.cnt;
            h->dcf.flags |= DCF_END_HINTS;
			flowStack(h);
			{
            /* stemcnt is currently number of coordinates; number stems is the number of pairs of coordinates. */
			int masklen = ((h->dcf.stemcnt/2) + 7)/8;
			flowOp(h, "%s[", opname[byte]);
			left -= masklen;
			while (masklen--)
				flowAdd(h, "%02X", read1(h));
			flowAdd(h, "]");
			break;
			}
		case tx_escape:
			{
			/* Process escaped operator */
			unsigned char escop = read1(h);
			left--;
			if (escop > ARRAY_LEN(escopname) - 1)
				{
				flowStack(h);
				flowOp(h, "reservedESC%d", escop);
				}
			else
				flowCommand(h, escopname[escop]);
			break;
			}
		case t2_shortint:
			/* 3-byte number */
			CHKOFLOW(1);
			{
			short value = read1(h);
			value = value<<8 | read1(h);
			left -= 2;
#if SHRT_MAX > 32767
			/* short greater that 2 bytes; handle negative range */
			if (value > 32767)
				value -= 65536;
#endif
			PUSH(value);
			}
			continue;
		case 247:
		case 248:
		case 249:
		case 250:
			/* Positive 2-byte number */
			CHKOFLOW(1);
			PUSH(108 + 256*(byte - 247) + read1(h));
			left--;
			continue;
		case 251:
		case 252:
		case 253:
		case 254:
			/* Negative 2-byte number */
			CHKOFLOW(1);
			PUSH(-108 - 256*(byte - 251) - read1(h));
			left--;
			continue;
		case 255:
			/* 5-byte number (16.16 fixed) */
			CHKOFLOW(1);
			{
			long value = read1(h);
			value = value<<8 | read1(h);
			value = value<<8 | read1(h);
			value = value<<8 | read1(h);
			left -= 4;
#if LONG_MAX > 2147483647
			/* long greater that 4 bytes; handle negative range */
			if (value > 2417483647)
				value -= 4294967296;
#endif
			PUSH(value/65536.0);
			}
			continue;
		default:
			/* 1-byte number */
			CHKOFLOW(1);
			PUSH(byte - 139);
			continue;
			}
		}
	if (!inSubr && (h->dcf.flags & DCF_BreakFlowed) &&
		!(h->dcf.flags & DCF_SaveStemCnt))
		{
		/* Handle left over args (if any) */
		flowStack(h);
		fprintf(h->dst.stm.fp, "\n");
		}
	}

/* Dump global charstring element. */
static void dumpGlobalElement(txCtx h, long index, const ctlRegion *region)
	{
	h->dcf.stemcnt = h->dcf.global.stemcnt.array[index];
	h->stack.cnt = 0;
	if (h->dcf.level > 1)
		index -= h->dcf.global.bias;
	flowElemBeg(h, index, 1);
	dumpCstr(h, region, 0);
	flowElemEnd(h);
	}

/* Dump Global Subr INDEX table. */
static void dcf_DumpGlobalSubrINDEX(txCtx h, const ctlRegion *region)
	{
	if (!(h->dcf.flags & DCF_GlobalSubrINDEX) || region->begin == -1)
		return;

	h->flags &= ~DCF_Flatten;
	dumpINDEX(h, "GlobalSubrINDEX", region, dumpGlobalElement);
	}

/* Dump encoding table. */
static void dcf_DumpEncoding(txCtx h, const ctlRegion *region)
	{
	FILE *fp = h->dst.stm.fp;

	if (!(h->dcf.flags & DCF_Encoding) ||
		region->begin == -1 ||
		h->top->sup.flags & ABF_CID_FONT)
		return;

	switch (region->begin)
		{
	case cff_StandardEncoding:
		fprintf(fp, "### Encoding ........ (Standard)\n");
		break;
	case cff_ExpertEncoding:
		fprintf(fp, "### Encoding ........ (Expert)\n");
		break;
	default:
		{

		dumpTagLine(h, "Encoding", region);
		if (h->dcf.level > 0)
			{
			long gid;
			long i;
			long cnt;
			unsigned char fmt;

			bufSeek(h, region->begin);
			gid = 1;			/* Skip glyph 0 (.notdef) */

			fmt = read1(h);
			fprintf(fp, "format =%x\n", fmt);
			switch (fmt & 0x7f)
				{
			case 0:
				cnt = read1(h);
				fprintf(fp, "nCodes =%ld\n", cnt);
				flowTitle(h, "glyph[gid]=code");
				for (; gid <= cnt; gid++)
					flowBreak(h, "[%ld]=%u", gid, read1(h));
				flowEnd(h);
				break;
			case 1:
				cnt = read1(h);
				fprintf(fp, "nRanges=%ld\n", cnt);
				flowTitle(h, "Range1={first,nLeft}");
				for (i = 0; i < cnt; i++)
					{
					unsigned char code = read1(h);
					unsigned char nLeft = read1(h);
					flowBreak(h, "[%ld]={%u,%u}", i, code, nLeft);
					gid += nLeft + 1;
					}
				flowEnd(h);
				break;
				}
			if (fmt & 0x80)
				{
				/* Read supplementary encoding */
				cnt = read1(h);
				fprintf(fp, "nSups=%ld\n", cnt);
				flowTitle(h, "Supplement={code,sid}");
				for (i = 0; i < cnt; i++)
					{
					unsigned char code = read1(h);
					unsigned short sid = read2(h);
					flowBreak(h, "[%ld]={%u,%hu}", i, code, sid);
					}
				flowEnd(h);
				}
			}
		}
		break;
		}
	}

/* Dump charset table. */
static void dcf_DumpCharset(txCtx h, const ctlRegion *region)
	{
	FILE *fp = h->dst.stm.fp;

	if (!(h->dcf.flags & DCF_Charset) || region->begin == -1)
		return;

	switch (region->begin)
		{
	case cff_ISOAdobeCharset:
		fprintf(fp, "### Charset ......... (ISOAdobe)\n");
		break;
	case cff_ExpertCharset:
		fprintf(fp, "### Charset ......... (Expert)\n");
		break;
	case cff_ExpertSubsetCharset:
		fprintf(fp, "### Charset ......... (Expert Subset)\n");
		break;
	default:
		{

		dumpTagLine(h, "Charset", region);
		if (h->dcf.level > 0)
			{
			unsigned char fmt;
			long i;
			long gid;

			bufSeek(h, region->begin);
			fmt = read1(h);
			fprintf(fp, "format=%u\n", fmt);
			gid = 1;	/* Skip glyph 0 (.notdef) */

			switch (fmt)
				{
			case 0:
				flowTitle(h, (h->top->sup.flags & ABF_CID_FONT)?
						"glyph[gid]=cid": "glyph[gid]=sid");
				for (; gid < h->top->sup.nGlyphs; gid++)
					flowBreak(h, "[%ld]=%hu", gid, read2(h));
				flowEnd(h);
				break;
			case 1:
				flowTitle(h, "Range1[index]={first,nLeft}");
				for (i = 0; gid < h->top->sup.nGlyphs; i++)
					{
					unsigned short id = read2(h);
					unsigned char nLeft = read1(h);
					flowBreak(h, "[%ld]={%hu,%hhu}", i, id, nLeft);
					gid += nLeft + 1;
					}
				flowEnd(h);
				break;
			case 2:
				flowTitle(h, "Range2[index]={first,nLeft}");
				for (i = 0; gid < h->top->sup.nGlyphs; i++)
					{
					unsigned short id = read2(h);
					unsigned short nLeft = read2(h);
					flowBreak(h, "[%ld]={%hu,%hu}", i, id, nLeft);
					gid += nLeft + 1;
					}
				flowEnd(h);
				break;
				}
			}
		}
		break;
		}
	}

/* Dump VarStore table. */
static void dcf_getvsIndices(txCtx h, const ctlRegion *region)
{
    unsigned int i = 0;
    unsigned short ivdSubtableCount;
    dnaDCL(unsigned long, ivdSubtableOffsets);
    long ivsStart = region->begin + 2;

    if (region->begin <= 0)
        return;
    bufSeek(h, region->begin);
    read2(h); /* length */
    read2(h); /* format */

    read4(h); /* regionListOffset */
    ivdSubtableCount = read2(h);

    dnaINIT(h->ctx.dna, ivdSubtableOffsets, ivdSubtableCount, ivdSubtableCount);
    dnaSET_CNT(ivdSubtableOffsets, ivdSubtableCount);

    for (i = 0; i < ivdSubtableCount; i++) {
        ivdSubtableOffsets.array[i] = read4(h);
    }


    /* item variation data list */
    for (i = 0; i < ivdSubtableCount; i++) {
        unsigned short regionIndexCount;
        RegionInfo *regionIndexCountEntry;

        bufSeek(h, ivsStart + ivdSubtableOffsets.array[i]);

        read2(h); /* itemCount */
        read2(h); /* shortDeltaCount*/
        regionIndexCount = read2(h);
        regionIndexCountEntry = dnaNEXT(h->dcf.varRegionInfo);
        regionIndexCountEntry->regionCount = regionIndexCount;

    }


 }

static void dcf_DumpVarStore(txCtx h, const ctlRegion *region)
{

    FILE *fp = h->dst.stm.fp;

    if (!(h->dcf.flags & DCF_FDSelect) || region->begin == -1)
        return;

    dumpTagLine(h, "VarStore", region);
    if (h->dcf.level < 1)
        return;
    else
    {
		unsigned short length;
		unsigned int i = 0;
        unsigned long regionListOffset;
        unsigned short ivdSubtableCount;
        dnaDCL(unsigned long, ivdSubtableOffsets);
        unsigned short axisCount, regionCount;
        long ivsStart = region->begin + 2;

        bufSeek(h, region->begin);
        length = read2(h);
        fprintf(fp, "length =%u\n", length);
        fprintf(fp, "format = %u\n", read2(h));

        regionListOffset = read4(h);
        fprintf(fp, "regionListOffset = %08lx\n", regionListOffset);
        ivdSubtableCount = read2(h);
        fprintf(fp, "subtableCount = %d\n", ivdSubtableCount);

        dnaINIT(h->ctx.dna, ivdSubtableOffsets, ivdSubtableCount, ivdSubtableCount);
        dnaSET_CNT(ivdSubtableOffsets, ivdSubtableCount);

        fprintf(fp, "--- subtableOffsets[index]={offset}\n");
        for (i = 0; i < ivdSubtableCount; i++) {
            ivdSubtableOffsets.array[i] = read4(h);
            fprintf(fp, "[%u]={%08lx}\n", i, ivdSubtableOffsets.array[i]);
        }

        bufSeek(h, ivsStart + regionListOffset);
        fprintf(fp, "--- RegionList\n");
        axisCount = read2(h);
        fprintf(fp, "axisCount = %u\n", axisCount);
        regionCount = read2(h);
        fprintf(fp, "regionCount = %u\n", regionCount);

        fprintf(fp, "--- RegionCoords[region,axis]={start,peak,end}\n");
        for (i = 0; i < regionCount; i++) {
            unsigned short axis;
            for (axis = 0; axis < axisCount; axis++) {
                float start = ((float)read2(h))/(1<<14);
                float peak = ((float)read2(h))/(1<<14);
                float end = ((float)read2(h))/(1<<14);
                fprintf(fp, "[%u,%u]={%g,%g,%g}\n", i, axis, start, peak, end);
            }
        }

        /* item variation data list */
        for (i = 0; i < ivdSubtableCount; i++) {
            unsigned short  itemCount;
            unsigned short  shortDeltaCount;
            unsigned short regionIndexCount;
            unsigned short  r, t;

            fprintf(fp, "--- VarStoreSubtable[%u]\n", i);

            bufSeek(h, ivsStart + ivdSubtableOffsets.array[i]);

            itemCount = read2(h);
            shortDeltaCount = read2(h);
            regionIndexCount = read2(h);
            fprintf(fp, "itemCount        = %d\n", itemCount);
            fprintf(fp, "shortDeltaCount  = %d\n", shortDeltaCount);
            fprintf(fp, "regionIndexCount = %d\n", regionIndexCount);

            /* region indices */
            fprintf(fp, "--- RegionIndex[region]={index}\n");
            for (r = 0; r < regionIndexCount; r++) {
                fprintf(fp, "[%d]={%d}\n", r, read2(h));
            }

            /* load two-dimensional delta values array */
            fprintf(fp, "--- DeltaValue[item,region]={delta}\n");
            for (t = 0; t < itemCount; t++) {
                for (r = 0; r < regionCount; r++) {
                    short   delta = (short)((r < shortDeltaCount)? read2(h): (char)read1(h));
                    fprintf(fp, "[%d,%d]={%d}\n", t, r, delta);
                }
            }
        }
    }
    fprintf(fp, "\n");
}

/* Dump FDSelect table. */
static void dcf_DumpFDSelect(txCtx h, const ctlRegion *region)
	{
	FILE *fp = h->dst.stm.fp;

	if (!(h->dcf.flags & DCF_FDSelect) || region->begin == -1)
		return;

	dumpTagLine(h, "FDSelect", region);
	if (h->dcf.level < 1)
		return;
	else
		{
		unsigned char fmt;

		bufSeek(h, region->begin);
		fmt = read1(h);
		fprintf(fp, "format =%u\n", fmt);

		switch (fmt)
			{
		case 0:
			{
			long gid;
			flowTitle(h, "glyph[gid]=fd");
			for (gid = 0; gid < h->top->sup.nGlyphs; gid++)
				flowBreak(h, "[%ld]=%u", gid, read1(h));
			flowEnd(h);
			}
			break;
		case 3:
			{
			long i;
			long nRanges = read2(h);
			fprintf(fp, "nRanges=%ld\n", nRanges);
			flowTitle(h, "Range3[index]={first,fd}");
			for (i = 0; i < nRanges; i++)
				{
				unsigned short first = read2(h);
				unsigned char fd = read1(h);
				flowBreak(h, "[%ld]={%hu,%u}", i, first, fd);
				}
			flowEnd(h);
			fprintf(fp, "sentinel=%hu\n", read2(h));
			}
			break;
		default:
			fatal(h, "invalid FDSelect format");
			}
		}
	}

/* Dump FDArray INDEX table. */
static void dcf_DumpFDArrayINDEX(txCtx h, const ctlRegion *region)
	{
	if (!(h->dcf.flags & DCF_FDArrayINDEX) || region->begin == -1)
		return;

	dumpINDEX(h, "FDArray INDEX", region, dumpDICTElement);
	}

/* Dump charstring element. */
static void dumpCstrElement(txCtx h, long index, const ctlRegion *region)
	{
	h->dcf.stemcnt = h->dcf.glyph.array[index];
	h->stack.cnt = 0;
	flowElemBeg(h, index, 1);
	dumpCstr(h, region, 0);
	flowElemEnd(h);
	}

/* Dump CharString INDEX. */
static void dcf_DumpCharStringsINDEX(txCtx h, const ctlRegion *region)
	{
	if (!(h->dcf.flags & DCF_CharStringsINDEX) || region->begin == -1)
		return;

	if (h->arg.g.cnt > 0)
		{
		/* Dump selected glyphs from INDEX */
		fprintf(h->dst.stm.fp, "### CharStrings (flattened)\n");
		if (h->dcf.level < 1)
			return;
		flowBeg(h);
		fprintf(h->dst.stm.fp, "--- glyph[tag]={%s,path}\n",
				(h->top->sup.flags & ABF_CID_FONT)? "cid": "name");
		h->dcf.flags |= DCF_Flatten;
		callbackSubset(h);
		flowEnd(h);
		}

	else
		{
		/* Dump entire INDEX */
		h->dcf.flags &= ~DCF_Flatten;
		dumpINDEX(h, "CharStrings INDEX", region, dumpCstrElement);
		}
	}

/* Dump Private DICT. */
static void dcf_DumpPrivateDICT(txCtx h, const ctlRegion *region)
	{
	if (!(h->dcf.flags & DCF_PrivateDICT) || region->begin == -1)
		return;

	dumpTagLine(h, "Private DICT", region);
	if (h->dcf.level < 1)
		return;
	dumpDICT(h, region);
	}

/* Dump local charstring element. */
static void dumpLocalElement(txCtx h, long index, const ctlRegion *region)
	{
	h->dcf.stemcnt = h->dcf.fd->stemcnt.array[index];
	h->stack.cnt = 0;
	if (h->dcf.level > 1)
		index -= h->dcf.fd->bias;
	flowElemBeg(h, index, 1);
	dumpCstr(h, region, 0);
	flowElemEnd(h);
	}

/* Dump Local Subr INDEX table. */
static void dcf_DumpLocalSubrINDEX(txCtx h, const ctlRegion *region)
	{
	if (!(h->dcf.flags & DCF_LocalSubrINDEX) || region->begin == -1)
		return;

	h->flags &= ~DCF_Flatten;
	dumpINDEX(h, "Local Subr INDEX", region, dumpLocalElement);
	}

/* Initialize subr info from INDEX. */
static void initSubrInfo(txCtx h, const ctlRegion *region, SubrInfo *info)
	{
    Card16 countSize;
	if (region->begin == -1)
		{
		/* Empty region */
		info->count = 0;
		return;
		}

	bufSeek(h, region->begin);

    if (h->dcf.flags & DCF_IS_CFF2)
    {
        countSize = 4;
        info->count = read4(h);
    }
    else
    {
        info->count = read2(h);
        countSize = 2;
    }

	if (info->count == 0)
		return;

	info->offSize = read1(h);
	info->offset = region->begin + countSize + 1;
	info->dataref = info->offset + (info->count + 1)*info->offSize - 1;

	dnaSET_CNT(info->stemcnt, info->count);
	memset(info->stemcnt.array, 0, info->count);

	if (info->count < 1240)
		info->bias = 107;
	else if (info->count < 33900)
		info->bias = 1131;
	else
		info->bias = 32768;
	}

static void dcf_setNumRegions(txCtx h, abfGlyphInfo *info)
    {
    /* If there is a Variation Region, then we get the regionCount for the current vsIndex.
     We need this in order to count stems when blends are present. */
    if (h->dcf.varRegionInfo.cnt == 0)
    {
        h->dcf.numRegions = 0;
    }
    else
    {
        h->dcf.numRegions = h->dcf.varRegionInfo.array[info->blendInfo.vsindex].regionCount;
    }
    }

/* Glyph begin callback to save count stems. */
static int dcf_SaveStemCount(abfGlyphCallbacks *cb, abfGlyphInfo *info)
	{
	txCtx h = cb->indirect_ctx;

	h->dcf.fd = &h->dcf.local.array[info->iFD];
	h->dcf.stemcnt = 0;
	h->dcf.flags &= ~DCF_END_HINTS;
	dcf_setNumRegions(h, info);

	h->stack.cnt = 0;
	dumpCstr(h, &info->sup, 0);
	h->dcf.glyph.array[info->tag] = (unsigned char)h->dcf.stemcnt;

	return ABF_SKIP_RET;
	}

/* Glyph charstring dump callback. */
static int dcf_GlyphBeg(abfGlyphCallbacks *cb, abfGlyphInfo *info)
	{
	txCtx h = cb->indirect_ctx;
	FILE *fp = h->dst.stm.fp;

	/* Customized flowElemBeg() */
	if (info->flags & ABF_GLYPH_CID)
		fprintf(fp, "%s[%hu]={\\%hu,%s", h->dcf.sep, info->tag, info->cid,
				(h->dcf.flags & DCF_BreakFlowed)? "\n": " ");
	else
		fprintf(fp, "%s[%hu]={%s,%s", h->dcf.sep, info->tag, info->gname.ptr,
				(h->dcf.flags & DCF_BreakFlowed)? "\n": " ");

	h->dcf.fd = &h->dcf.local.array[info->iFD];
	h->dcf.stemcnt = 0;
	h->dcf.flags &= ~DCF_END_HINTS;
	dcf_setNumRegions(h, info);

	h->stack.cnt = 0;
	dumpCstr(h, &info->sup, 0);

	flowElemEnd(h);

	return ABF_SKIP_RET;
	}

/* Initialize charstring dump. */
static void initCstrs(txCtx h, abfTopDict *top)
	{
	long i;
	int subrDump = h->dcf.flags & (DCF_GlobalSubrINDEX|DCF_LocalSubrINDEX);

	if (h->dcf.level < 1 ||
		!(subrDump || (h->dcf.flags & DCF_CharStringsINDEX)))
		return;	/* Nothing to do */

	/* Initialize global subrs */
	initSubrInfo(h,
				 &cfrGetSingleRegions(h->cfr.ctx)->GlobalSubrINDEX,
				 &h->dcf.global);

	/* Initialize local subrs */
	dnaSET_CNT(h->dcf.local, top->FDArray.cnt);
	for (i = 0; i < h->dcf.local.cnt; i++)
		initSubrInfo(h,
					 &cfrGetRepeatRegions(h->cfr.ctx, i)->LocalSubrINDEX,
					 &h->dcf.local.array[i]);

	if (!subrDump && h->arg.g.cnt > 0)
		return;

	/* Initialize glyph charstrings */
	dnaSET_CNT(h->dcf.glyph, top->sup.nGlyphs);
	memset(h->dcf.glyph.array, 0, h->dcf.glyph.cnt);

	/* Save stem counts */
	h->dcf.flags |= (DCF_Flatten|DCF_SaveStemCnt);
	h->cb.glyph.beg = dcf_SaveStemCount;

	if (cfrIterateGlyphs(h->cfr.ctx, &h->cb.glyph))
		fatal(h, NULL);

	h->cb.glyph.beg = dcf_GlyphBeg;
	h->dcf.flags &= ~(DCF_Flatten|DCF_SaveStemCnt);
	}

/* Begin new font. */
static void dcf_BegFont(txCtx h, abfTopDict *top)
	{
	long i;
	const cfrSingleRegions *single;
    unsigned short major;
	if (h->src.type != src_OTF && h->src.type != src_CFF)
		fatal(h, "-dcf mode: non-CFF font");

	if (h->arg.g.cnt > 0)
		{
		/* Glyph subset specified; select CharStringsINDEX dump */
		if (!(h->dcf.flags & DCF_TableSelected))
			/* Clear default table selections */
			h->dcf.flags &= ~DCF_AllTables;
		h->dcf.flags |= DCF_CharStringsINDEX;
		if (h->dcf.level == 0)
			/* Set useful dump level */
			h->dcf.level = 5;
		}

	h->src.offset = -BUFSIZ;
	dstFileOpen(h, top);

	single = cfrGetSingleRegions(h->cfr.ctx);
	major = dcf_DumpHeader(h, 			&single->Header);
    if (major == 1)
    {
        if (h->dcf.flags & DCF_IS_CUBE)
            h->maxOpStack = TX_MAX_OP_STACK_CUBE;
        else
            h->maxOpStack = T2_MAX_OP_STACK;
        initCstrs(h, top);
        dcf_DumpNameINDEX(h, 		&single->NameINDEX);
        dcf_DumpTopDICTINDEX(h, 	&single->TopDICTINDEX);
        dcf_DumpStringINDEX(h, 		&single->StringINDEX);
        dcf_DumpGlobalSubrINDEX(h,	&single->GlobalSubrINDEX);
        dcf_DumpEncoding(h,			&single->Encoding);
        dcf_DumpCharset(h,			&single->Charset);
    }
    else
    {
        /* number of regions needs to be known for reading charstrings */
        dcf_getvsIndices(h, &single->VarStore);
        h->dcf.flags |= DCF_IS_CFF2;
        h->maxOpStack = CFF2_MAX_OP_STACK;
        initCstrs(h, top);
        dcf_DumpTopDICT2(h, &single->TopDICTINDEX);
     }
    dcf_DumpFDSelect(h,			&single->FDSelect);
    dcf_DumpVarStore(h,			&single->VarStore);
	dcf_DumpFDArrayINDEX(h,		&single->FDArrayINDEX);
	dcf_DumpCharStringsINDEX(h,	&single->CharStringsINDEX);

	for (i = 0; i < top->FDArray.cnt; i++)
		{
		const cfrRepeatRegions *repeat = cfrGetRepeatRegions(h->cfr.ctx, i);
		if (top->FDArray.cnt > 1 &&
			(h->dcf.flags & (DCF_PrivateDICT|DCF_LocalSubrINDEX)))
			fprintf(h->dst.stm.fp, "--- FD[%ld]\n", i);
		dcf_DumpPrivateDICT(h,		&repeat->PrivateDICT);
		h->dcf.fd = &h->dcf.local.array[i];
		dcf_DumpLocalSubrINDEX(h,	&repeat->LocalSubrINDEX);
		}
	}

/* End new font. */
static void dcf_EndFont(txCtx h)
	{
	}

/* End font set. */
static void dcf_EndSet(txCtx h)
	{
	dstFileClose(h);
	}

/* Setup dcf mode. */
static void dcf_SetMode(txCtx h)
	{
	/* Set mode name */
	h->modename	= "dcf";

	/* h->dcf.flags is now set at the start of parseArgs
	h->dcf.flags = DCF_AllTables|DCF_BreakFlowed;
	h->dcf.level = 5;
	*/

	/* Set library functions */
	h->dst.begset	= dcf_BegSet;
	h->dst.begfont	= dcf_BegFont;
	h->dst.endfont	= dcf_EndFont;
	h->dst.endset	= dcf_EndSet;

	/* Initialize glyph callbacks */
	h->cb.glyph.beg = dcf_GlyphBeg;
	h->cb.glyph.indirect_ctx = h;

	/* Set source library flag */
	/* These are now set at the start of parseArgs
	h->cfr.flags = 0;
	*/

	h->mode = mode_dcf;
	}

/* Mode-specific help. */
static void dcf_Help(txCtx h)
	{
	static char *text[] =
		{
#include "dcf.h"
		};
	printText(ARRAY_LEN(text), text);
	}

/* Parse -T argument. */
static void dcf_ParseTableArg(txCtx h, char *arg)
	{
	long save_flags = h->dcf.flags & DCF_BreakFlowed;
	h->dcf.flags = 0;
	for (;;)
		{
		int c = *arg++;
		switch (c)
			{
		case '\0':
			h->dcf.flags |= save_flags;
			h->dcf.flags |= DCF_TableSelected;
			return;
		case 'h':
			h->dcf.flags |= DCF_Header;
			break;
		case 'n':
			h->dcf.flags |= DCF_NameINDEX;
			break;
		case 't':
			h->dcf.flags |= DCF_TopDICTINDEX;
			break;
		case 's':
			h->dcf.flags |= DCF_StringINDEX;
			break;
		case 'g':
			h->dcf.flags |= DCF_GlobalSubrINDEX;
			break;
		case 'e':
			h->dcf.flags |= DCF_Encoding;
			break;
		case 'C':
			h->dcf.flags |= DCF_Charset;
			break;
		case 'f':
			h->dcf.flags |= DCF_FDSelect;
			break;
		case 'F':
			h->dcf.flags |= DCF_FDArrayINDEX;
			break;
		case 'c':
			h->dcf.flags |= DCF_CharStringsINDEX;
			break;
		case 'p':
			h->dcf.flags |= DCF_PrivateDICT;
			break;
		case 'l':
			h->dcf.flags |= DCF_LocalSubrINDEX;
			break;
		case 'a':
			if (strcmp(arg, "ll") == 0)
				{
				h->dcf.flags = DCF_AllTables;
				break;
				}
			/* Fall through */
		default:
			fprintf(stderr, "%s: option -T invalid selector '%c' (ignored)\n",
					h->progname, c);
			}
		}
	}

/* ---------------------------- Subset Creation ---------------------------- */

/* Make random subset. */
static void makeRandSubset(txCtx h, char *opt, char *arg)
	{
	long i;
	char *p;
	float percent = (float)strtod(arg, &p);
	if (*p != '\0' || percent < 0 || percent > 100)
		fatal(h, "bad arg (%s)", opt);

	/* Initialize glyph list */
	dnaSET_CNT(h->subset.glyphs, h->top->sup.nGlyphs);
	for (i = 0; i < h->subset.glyphs.cnt; i++)
		h->subset.glyphs.array[i] = (unsigned short)i;

	/* Randomize glyph list by random permutation method */
	for (i = 0; i < h->subset.glyphs.cnt - 1; i++)
		{
		long j = randrange(h->subset.glyphs.cnt - i);
		unsigned short tmp = h->subset.glyphs.array[i];
		h->subset.glyphs.array[i] = h->subset.glyphs.array[i + j];
		h->subset.glyphs.array[i + j] = tmp;
		}

	/* Trim array to specified percentage */
	h->subset.glyphs.cnt = (long)(percent/100.0*h->subset.glyphs.cnt + 0.5);
	if (h->subset.glyphs.cnt == 0)
		h->subset.glyphs.cnt = 1;
	}

/* Make Font Dict subset. */
static void makeFDSubset(txCtx h)
	{
	long i,j;

	if (!(h->top->sup.flags & ABF_CID_FONT))
		fatal(h, "-fd specified for non-CID font");

	getGlyphList(h);

    for (i = 0; i < h->src.glyphs.cnt; i++)
    {
        abfGlyphInfo *info = h->src.glyphs.array[i];
        if (h->flags & SUBSET__EXCLUDE_OPT)
        {
            unsigned int match = 0;
            for (j = 0; j < h->fd.fdIndices.cnt; j++)
            {
                if (info->iFD == h->fd.fdIndices.array[j])
                {
                    match = 1;
                    break;
                }
            }
            if (!match)
                *dnaNEXT(h->subset.glyphs) = info->tag;

        }
        else
        {
            for (j = 0; j < h->fd.fdIndices.cnt; j++)
            {
                if (info->iFD == h->fd.fdIndices.array[j])
                {
                    *dnaNEXT(h->subset.glyphs) = info->tag;
                    break;
               }
            }
        }
    }

	if (h->subset.glyphs.cnt == 0)
		fatal(h, "no glyphs selected by -fd argument");
	}

/* Begin new glyph definition for gathering glyph info. */
static int getExcludeGlyphBeg(abfGlyphCallbacks *cb, abfGlyphInfo *info)
	{
	txCtx h = cb->indirect_ctx;
	*dnaNEXT(h->src.exclude) = info;
	if (info->flags & ABF_GLYPH_CID)
		{
		if 	(info->cid == 0)
			h->flags &= ~SUBSET_HAS_NOTDEF;
		}
	else
		{
		if 	(strcmp(info->gname.ptr, ".notdef") == 0)
			h->flags &= ~SUBSET_HAS_NOTDEF;
		}

	return ABF_SKIP_RET;
	}

/* Compare glyphs by their tag. */
static int CTL_CDECL cmpExcludeByTag(const void *first, const void *second)
	{
	const abfGlyphInfo *a = *(abfGlyphInfo **)first;
	const abfGlyphInfo *b = *(abfGlyphInfo **)second;
	if (a->tag < b->tag)
		return -1;
	else if (a->tag > b->tag)
		return 1;
	else
		return 0;
	}


/* Match glyph by its tag. */
static int CTL_CDECL matchExcludedByTag(const void *key, const void *value,
									void *ctx)
	{
	unsigned short a = *(unsigned short *)key;
	unsigned short b = (*(abfGlyphInfo**)value)->tag;
	if (a < b)
		return -1;
	else if (a > b)
		return 1;
	else
		return 0;
 	}

static void invertSubset(txCtx h)
	{
	long i;
	long cnt;

	/* iterate over all glyphs such that h->exclude.array will be filled with tags of glyphs that match. */
	h->cb.saveGlyphBeg = h->cb.glyph.beg;

	/* Insert data gather function */
    h->cb.glyph.beg = getExcludeGlyphBeg;
	h->cb.glyph.indirect_ctx = h;
	h->flags |= SUBSET_SKIP_NOTDEF;
	h->flags |= SUBSET_HAS_NOTDEF; /* This gets cleared by the getExcludeGlyphBeg if the .notdef is seen.*/
	callbackSubset(h);
	h->flags &= ~SUBSET_SKIP_NOTDEF;
	qsort(h->src.exclude.array, h->src.exclude.cnt,
		  sizeof(h->src.exclude.array[0]), cmpExcludeByTag);

	/* Restore saved function */
	h->cb.glyph.beg = h->cb.saveGlyphBeg;

	getGlyphList(h);
	dnaSET_CNT(h->subset.glyphs, h->src.glyphs.cnt);
	cnt = 0;
	for (i = 0; i < h->src.glyphs.cnt; i++)
		{
		size_t index;
		unsigned short tag = h->src.glyphs.array[i]->tag;
		if (!ctuLookup(&tag, h->src.exclude.array, h->src.exclude.cnt,
					  sizeof(h->src.exclude.array[0]), matchExcludedByTag, &index, h))
			{
			h->subset.glyphs.array[cnt] = tag;
			cnt++;
			}
		}
	dnaSET_CNT(h->subset.glyphs, cnt);
	}

/* Make glyph subset. */
static void prepSubset(txCtx h)
	{
	/* in the <mode>ReadFont function, the reader library iterates through
	the glyphs specified by the glyph list argument. What this function does is to create
	 the glyph list argument, according to which is specified of the several options
	which request a subset. Note that even if the user is excluding only one or two glyphs
	with the the -gx option, the glyph list arg is still very short, as it uses ranges of GIDs.

	This function first creates a list of selected glyphs in the h->subset.glyphs DNA. IF there
	are not such glyphs, it returns. Else, it then proceeds to build the glyph list arg in
	makeSubsetArgList() */

    if (h->flags & SHOW_NAMES)
    {
        fflush(stdout);
        if (h->top->sup.flags & ABF_CID_FONT)
            fprintf(stderr, "--- CIDFontName: %s\n",
                    h->top->cid.CIDFontName.ptr);
        else
            fprintf(stderr, "--- FontName: %s\n",
                    h->top->FDArray.array[0].FontName.ptr);
    }

    h->flags &= ~SUBSET_HAS_NOTDEF;
    h->src.glyphs.cnt = 0;
    h->src.exclude.cnt = 0;
    h->subset.glyphs.cnt = 0;

    /* Make subset glyph list */
    if (h->arg.p != NULL)
        makeRandSubset(h, "-p", h->arg.p);
    else if (h->arg.P != NULL)
        makeRandSubset(h, "-P", h->arg.P);
    else if (h->fd.fdIndices.cnt > 0)
        makeFDSubset(h);
    else if (h->flags & SUBSET__EXCLUDE_OPT)
        invertSubset(h);
    else
    {
        if (h->flags & SUBSET_OPT) {
            if (h->flags & PRESERVE_GID) {
                getGlyphList(h);
                parseSubset(h, callbackPreserveGlyph);
                h->src.glyphs.array[0]->flags |= PRESERVE_CHARSTRING;
                h->arg.g.cnt = 0;
                h->cb.save = h->cb.glyph;
                h->cb.glyph = preserveGlyphCallbacks;
                h->cb.glyph.direct_ctx = h;
            }
        }
    }

	if (h->subset.glyphs.cnt == 0)
		return; /* no subset */

	/* Make subset arg list */
	makeSubsetArgList(h);
	if ((h->mode == mode_cff || h->mode == mode_t1) || h->mode == mode_svg || h->mode == (mode_ufow && (h->flags & SHOW_NAMES)))
		{
		char *p;
		char *q;
		long i;

		/* Print subset */
		fprintf(stderr,
				"--- subset:\n"
				"SRC font	%s\n"
				"SRC glyphs	%ld\n"
				"DST font	%s\n"
				"DST glyphs	%ld\n",
				h->src.stm.filename, h->top->sup.nGlyphs,
				h->dst.stm.filename, h->subset.glyphs.cnt);
		p = "";
		q = h->arg.g.substrs;
		for (i = 0; i < h->arg.g.cnt; i++)
			{
			fprintf(stderr, "%s%s", p, q);
			p = ",";
			q += strlen(q) + 1;
			}
		fprintf(stderr, "\n");
		}
	}

/* Callback glyph according to type technology and selector. */
static void callbackGlyph(txCtx h, int type, unsigned short id, char *name)
	{
	switch (h->src.type)
		{
	case src_Type1:
		switch (type)
			{
		case sel_by_tag:
			(void)t1rGetGlyphByTag(h->t1r.ctx, id, &h->cb.glyph);
			break;
		case sel_by_cid:
			(void)t1rGetGlyphByCID(h->t1r.ctx, id, &h->cb.glyph);
			break;
		case sel_by_name:
			(void)t1rGetGlyphByName(h->t1r.ctx, name, &h->cb.glyph);
			break;
			}
		break;
	case src_OTF:
	case src_CFF:
		switch (type)
			{
		case sel_by_tag:
			(void)cfrGetGlyphByTag(h->cfr.ctx, id, &h->cb.glyph);
			break;
		case sel_by_cid:
			(void)cfrGetGlyphByCID(h->cfr.ctx, id, &h->cb.glyph);
			break;
		case sel_by_name:
			(void)cfrGetGlyphByName(h->cfr.ctx, name, &h->cb.glyph);
			break;
			}
		break;
	case src_TrueType:
		switch (type)
			{
		case sel_by_tag:
			(void)ttrGetGlyphByTag(h->ttr.ctx, id, &h->cb.glyph);
			break;
		case sel_by_cid:
			/* Invalid; do nothing */
			break;
		case sel_by_name:
			(void)ttrGetGlyphByName(h->ttr.ctx, name, &h->cb.glyph);
			break;
			}
		break;

	case src_SVG:
		switch (type)
			{
		case sel_by_tag:
			(void)svrGetGlyphByTag(h->svr.ctx, id, &h->cb.glyph);
			break;
		case sel_by_cid:
			fatal(h, "Cannot read glyphs from SVG fonts by CID ");
			break;
		case sel_by_name:
			(void)svrGetGlyphByName(h->svr.ctx, name, &h->cb.glyph);
			break;
			}
		break;
	case src_UFO:
		switch (type)
			{
		case sel_by_tag:
			(void)ufoGetGlyphByTag(h->ufr.ctx, id, &h->cb.glyph);
			break;
		case sel_by_cid:
			fatal(h, "Cannot read glyphs from UFO fonts by CID ");
			break;
		case sel_by_name:
			(void)ufoGetGlyphByName(h->ufr.ctx, name, &h->cb.glyph);
			break;
			}
		break;
		}
	}

/* Add .notdef glyph to destination font. If it's already added it will be skipped. */
static void addNotdef(txCtx h)
	{
	if (((h->src.type == src_Type1) || (h->src.type == src_SVG) || (h->src.type == src_UFO)) && !(h->top->sup.flags & ABF_CID_FONT))
		callbackGlyph(h, sel_by_name, 0, ".notdef");
	else
		callbackGlyph(h, sel_by_tag, 0, NULL);
	}

/* Filter glyphs using the glyph list pararmeter. */
static void callbackSubset(txCtx h)
	{
	parseSubset(h, callbackGlyph);
	if (!((SUBSET_SKIP_NOTDEF & h->flags)||(SUBSET_HAS_NOTDEF & h->flags)))
		{
	switch (h->mode)
		{
	case mode_cff:
		addNotdef(h);
		break;
	case mode_t1:
		if (!(h->t1w.flags & T1W_TYPE_ADDN) && !(h->t1w.options & T1W_DECID))
			addNotdef(h);
		break;
		}
	}
	}

/* ----------------------------- t1read Library ---------------------------- */

/* Read font with t1read library. */
static void t1rReadFont(txCtx h, long origin)
	{
	if (h->t1r.ctx == NULL)
		{
		/* Initialize library */
		h->t1r.ctx = t1rNew(&h->cb.mem, &h->cb.stm, T1R_CHECK_ARGS);
		if (h->t1r.ctx == NULL)
			fatal(h, "(t1r) can't init lib");
		}

	if (h->flags & SUBSET_OPT && h->mode != mode_dump)
		h->t1r.flags |= T1R_UPDATE_OPS;	/* Convert seac for subsets */

	if (h->flags & NO_UDV_CLAMPING)
		 h->t1r.flags |=  T1R_NO_UDV_CLAMPING;

	if (t1rBegFont(h->t1r.ctx, h->t1r.flags, origin, &h->top, getUDV(h)))
		fatal(h, NULL);

	prepSubset(h);

	h->dst.begfont(h, h->top);

	if (h->mode != mode_cef)
		{
		if (h->arg.g.cnt != 0)
			callbackSubset(h);
		else if (t1rIterateGlyphs(h->t1r.ctx, &h->cb.glyph))
			fatal(h, NULL);
		}

	h->dst.endfont(h);

	if (t1rEndFont(h->t1r.ctx))
		fatal(h, NULL);
	}

/* ----------------------------- svread Library ---------------------------- */


/* Read font with svread library. */
static void svrReadFont(txCtx h, long origin)
	{
	if (h->svr.ctx == NULL)
		{
		/* Initialize library */
		h->svr.ctx = svrNew(&h->cb.mem, &h->cb.stm, SVR_CHECK_ARGS);
		if (h->svr.ctx == NULL)
			fatal(h, "(svr) can't init lib");
		}


	if (svrBegFont(h->svr.ctx, h->svr.flags, &h->top))
		fatal(h, NULL);

	prepSubset(h);

	h->dst.begfont(h, h->top);

	if (h->mode != mode_cef)
		{
		if (h->arg.g.cnt != 0)
			callbackSubset(h);
		else if (svrIterateGlyphs(h->svr.ctx, &h->cb.glyph))
			fatal(h, NULL);
		}

	h->dst.endfont(h);

	if (svrEndFont(h->svr.ctx))
		fatal(h, NULL);
	}

/* ----------------------------- ufo read Library ---------------------------- */


/* Read font with ufo rread library. */
static void ufoReadFont(txCtx h, long origin)
{
	if (h->ufr.ctx == NULL)
    {
		/* Initialize library */
		h->ufr.ctx = ufoNew(&h->cb.mem, &h->cb.stm, UFO_CHECK_ARGS);
		if (h->ufr.ctx == NULL)
			fatal(h, "(ufr) can't init lib");
    }


	if (ufoBegFont(h->ufr.ctx, h->ufr.flags, &h->top, h->ufr.altLayerDir))
		fatal(h, NULL);

	prepSubset(h);

	h->dst.begfont(h, h->top);

	if (h->mode != mode_cef)
    {
		if (h->arg.g.cnt != 0)
			callbackSubset(h);
		else if (ufoIterateGlyphs(h->ufr.ctx, &h->cb.glyph))
			fatal(h, NULL);
    }

	h->dst.endfont(h);

	if (ufoEndFont(h->ufr.ctx))
		fatal(h, NULL);
}


/* ---------------------------- cffread Library ---------------------------- */

/* Read format 12 Unicode cmap. Assumes format field already read. */
static void readCmap14(txCtx h)
{
    /* Skip format and length fields */
    (void)read2(h);
    (void)read4(h);

    read4(h); /* numVarSelectorRecords */

    /* Not yet implemented - not sure it is worth the effort: spot and ttx are more useful outputs for this Unicode format. */
    return;
}

static void readCmap12(txCtx h)
{
    unsigned long nGroups;
    unsigned long i;

    /* Skip reserved, language, and length fields */
    (void)read2(h);
    (void)read4(h);
    (void)read4(h);

    nGroups = read4(h);
    for (i = 0; i < nGroups; i++) {
        unsigned long startCharCode = read4(h);
        unsigned long endCharCode = read4(h);
        unsigned long startGlyphId = read4(h);

        while (startCharCode <= endCharCode) {
            if (startGlyphId >= (unsigned long)h->top->sup.nGlyphs)
                return;
            h->cmap.encoding.array[startGlyphId++] = startCharCode++;
        }
    }
}

/* Read Unicode cmap. */
static void readCmap(txCtx h, size_t offset)
{
	unsigned short segCount;
	cmapSegment4* segment;
	cmapSegment4* segmentEnd;

	/* Check format */
	bufSeek(h, (long)offset);
	switch (read2(h)) {
	case 4:
		break;
    case 14:
        readCmap14(h);
        return;
    case 12:
        readCmap12(h);
        return;
	default:
		return;
	}

	/* Read format 4 subtable; skip length and language fields */
	(void)read2(h);	/* length */
	(void)read2(h);	/* language */

	/* Read segment count and allocate */
	segCount = read2(h)/2;
	dnaSET_CNT(h->cmap.segment, segCount);
	segmentEnd = &h->cmap.segment.array[segCount];

	/* Skip binary search fields */
	(void)read2(h);	/* searchRange */
	(void)read2(h);	/* entrySelector */
	(void)read2(h);	/* rangeShift */

	/* Read segment arrays */
	for (segment = h->cmap.segment.array; segment != segmentEnd; ++segment)
		segment->endCode = read2(h);
	(void)read2(h);		/* Skip password */
	for (segment = h->cmap.segment.array; segment != segmentEnd; ++segment)
		segment->startCode = read2(h);
	for (segment = h->cmap.segment.array; segment != segmentEnd; ++segment)
		segment->idDelta = sread2(h);
	offset = h->src.next - h->src.buf + h->src.offset;
	for (segment = h->cmap.segment.array; segment != segmentEnd; ++segment) {
		unsigned short idRangeOffset = read2(h);
		segment->idRangeOffset =
			(idRangeOffset == 0)? 0: (unsigned long)(offset + idRangeOffset);
		offset += 2;
	}

	/* Derive mapping from segments */
	for (segment = h->cmap.segment.array; segment != segmentEnd; ++segment) {
		unsigned short gid;
		unsigned long code;

		if (segment->idRangeOffset == 0) {
			gid = segment->idDelta + segment->startCode;
			for (code = segment->startCode; code <= segment->endCode; ++code) {
				if (code == 0xffff || gid >= h->cmap.encoding.cnt)
					break;
				if (gid != 0 && h->cmap.encoding.array[gid] == ABF_GLYPH_UNENC)
					h->cmap.encoding.array[gid] = (unsigned short)code;
				++gid;
			}
		} else {
			bufSeek(h, segment->idRangeOffset);
			for (code = segment->startCode; code <= segment->endCode; ++code) {
				gid = read2(h);
				if (code != 0xffff && gid != 0 && gid < h->cmap.encoding.cnt &&
						h->cmap.encoding.array[gid] == ABF_GLYPH_UNENC)
					h->cmap.encoding.array[gid] = (unsigned short)code;
			}
		}
	}
}

/* Begin new glyph definition for gathering glyph info. */
static int otfGlyphBeg(abfGlyphCallbacks *cb, abfGlyphInfo *info)
{
	txCtx h = cb->indirect_ctx;
	unsigned long code = h->cmap.encoding.array[info->tag];
	if (code != ABF_GLYPH_UNENC) {
		info->flags |= ABF_GLYPH_UNICODE;
		info->encoding.code = code;
	}
	return h->cb.saveGlyphBeg(cb, info);
}

/* Prepare to process OTF font. */
static void prepOTF(txCtx h)
{
    long unioff;
    long uni5off;
	long winoff;
	unsigned short uniscore;
	unsigned short winscore;
	unsigned short version;
	unsigned short nEncodings;
	unsigned short i;
	sfrTable *table;

	/* Install new callback */
	h->cb.saveGlyphBeg = h->cb.glyph.beg;
	h->cb.glyph.beg = otfGlyphBeg;
	h->cb.glyph.indirect_ctx = h;

	/* Prepare encoding array */
	dnaSET_CNT(h->cmap.encoding, h->top->sup.nGlyphs);
	for (i = 0; i < h->cmap.encoding.cnt; i++)
		h->cmap.encoding.array[i] = ABF_GLYPH_UNENC;

	/* Get cmap table */
	table = sfrGetTableByTag(h->ctx.sfr, CTL_TAG('c','m','a','p'));
	if (table == NULL)
		fatal(h, "OTF: can't find cmap");

	/* Force seek */
	h->src.offset = LONG_MAX;
	bufSeek(h, table->offset);

	/* Read and check version */
	version = read2(h);
	if (version != 0)
		fatal(h, "cmap: bad version");

	/* Search for Unicode and Windows subtables */
    unioff = 0;
    uni5off = 0;
	uniscore = 0;
	winoff = 0;
	winscore = 0;
	nEncodings = read2(h);
	for (i = 0; i < nEncodings; i++) {
		/* Read encoding entry */
		unsigned short platformId = read2(h);
		unsigned short platspecId = read2(h);
		unsigned long suboff = read4(h);

		/* Select table */
		switch (platformId) {
		case 0: /* Unicode platform */
            if ((platspecId == 3 ||		/* Unicode */
                platspecId == 4) &&	/* UCS-4 */
                (unioff == 0 || uniscore < platspecId)) {
				unioff = table->offset + suboff;
				uniscore = platspecId;
			}
            else if (platspecId == 5)
            {
                uni5off = table->offset + suboff;
                readCmap(h, uni5off);	/* read UVS cmap. */
            }
			break;
		case 3: /* Windows platform */
			if ((platspecId == 1 ||		/* Unicode */
				 platspecId == 10) &&	/* UCS-4 */
				(winoff == 0 || winscore < platspecId)) {
				winoff = table->offset + suboff;
				winscore = platspecId;
			}
			break;
		}
	}

	if (unioff != 0)
		readCmap(h, unioff);	/* Prefer Unicode platform... */
	else if (winoff != 0)
		readCmap(h, winoff);	/* ...to Windows platform */
}

/* Read font with cffread library. */
static void cfrReadFont(txCtx h, long origin, int ttcIndex)
	{
    float *uv;
	if (h->cfr.ctx == NULL)
		{
		h->cfr.ctx = cfrNew(&h->cb.mem, &h->cb.stm, CFR_CHECK_ARGS);
		if (h->cfr.ctx == NULL)
			fatal(h, "(cfr) can't init lib");
		}

	if (h->flags & SUBSET_OPT && h->mode != mode_dump)
		h->cfr.flags |= CFR_UPDATE_OPS;	/* Convert seac for subsets */

    uv = getUDV(h);
    if (uv)
        h->cfr.flags |= CFR_FLATTEN_VF;
	if (cfrBegFont(h->cfr.ctx, h->cfr.flags, origin,  ttcIndex, &h->top, uv))
		fatal(h, NULL);

	prepSubset(h);

	h->dst.begfont(h, h->top);

	if (h->mode != mode_cef && h->mode != mode_dcf)
		{
		if (h->cfr.flags & CFR_NO_ENCODING)
			/* OTF font */
			prepOTF(h);

		if (h->arg.g.cnt != 0)
			callbackSubset(h);
		else if (cfrIterateGlyphs(h->cfr.ctx, &h->cb.glyph))
			fatal(h, NULL);

		if (h->cfr.flags & CFR_NO_ENCODING)
			{
			/* OTF font; restore callback */
			h->cb.glyph.beg = h->cb.saveGlyphBeg;
			h->cfr.flags &= ~CFR_NO_ENCODING;
			}
		}

	h->dst.endfont(h);

	if (cfrEndFont(h->cfr.ctx))
		fatal(h, NULL);
	}

/* ----------------------------- ttread Library ---------------------------- */

/* Read font with ttread library. */
static void ttrReadFont(txCtx h, long origin, int iTTC)
	{
	if (h->ttr.ctx == NULL)
		{
		h->ttr.ctx = ttrNew(&h->cb.mem, &h->cb.stm, TTR_CHECK_ARGS);
		if (h->ttr.ctx == NULL)
			fatal(h, "(ttr) can't init lib");
		}

	if (ttrBegFont(h->ttr.ctx, h->ttr.flags, origin, iTTC, &h->top))
		fatal(h, NULL);

	prepSubset(h);

	h->dst.begfont(h, h->top);

	if (h->mode != mode_cef)
		{
		if (h->arg.g.cnt != 0)
			callbackSubset(h);
		else if (ttrIterateGlyphs(h->ttr.ctx, &h->cb.glyph))
			fatal(h, NULL);
		}

	h->dst.endfont(h);

	if (ttrEndFont(h->ttr.ctx))
		fatal(h, NULL);
	}

/* ---------------- Platform-Specific Font Wrapper Callbacks --------------- */

/* Prepare next segment of font data. */
static size_t nextseg(txCtx h, char **ptr)
	{
	size_t srcleft = h->src.end - h->src.next;

	if (srcleft == 0)
		{
		/* Refill empty source buffer */
		fillbuf(h, h->src.offset + h->src.length);
		srcleft = h->src.length;
		}

	*ptr = h->src.next;
	if (srcleft <= h->seg.left)
		{
		/* Return full buffer */
		h->seg.left -= srcleft;
		h->src.next += srcleft;
		}
	else
		{
		/* Return partial buffer */
		srcleft = h->seg.left;
		h->src.next += h->seg.left;
		h->seg.left = 0;
		}

	return srcleft;
	}

/* Refill input buffer from PFB stream. */
static size_t PFBRefill(txCtx h, char **ptr)
	{
	while (h->seg.left == 0)
		{
		/* New segment; read segment header */
		int escape = read1(h);
		int type = read1(h);

		/* Check segment header */
		if (escape != 128 || (type != 1 && type != 2 && type != 3))
			fatal(h, "bad PFB segment type");

		if (type == 3)
			{
			/* EOF */
			*ptr = NULL;
			return 0;
			}
		else
			{
			/* Read segment length (little endian) */
			h->seg.left = read1(h);
			h->seg.left |= read1(h)<<8;
			h->seg.left |= (long)read1(h)<<16;
			h->seg.left |= (long)read1(h)<<24;
			}
		}

	return nextseg(h, ptr);
	}

/* Refill input buffer from POST resource stream. */
static size_t POSTRefill(txCtx h, char **ptr)
	{
	while (h->seg.left == 0)
		{
		/* New POST resource */
		int type;

		/* Read length and type */
		h->seg.left = read4(h) - 2;

		type = read1(h);
		(void)read1(h);	/* Discard padding byte (not documented) */

		/* Process resource data */
		switch (type)
			{
		case 0:
			/* Comment; skip data */
			bufSeek(h, h->src.offset + (long)h->seg.left);
			h->seg.left = 0;
			break;
		case 1:	/* ASCII */
		case 2:	/* Binary */
			break;
		case 3: /* End-of-file */
		case 5:	/* End-of-data */
			*ptr = NULL;
			return 0;
		case 4:		/* Data in data fork; unsupported */
		default:	/* Unknown POST type */
			fatal(h, "bad POST resource type");
			}
		}

	return nextseg(h, ptr);
	}

/* ----------------------------- sfnt Handling ----------------------------- */

/* Add font record to list. */
static void addFont(txCtx h, int type, int iTTC, long offset)
	{
	FontRec *rec = dnaNEXT(h->fonts);
	rec->type = type;
	rec->iTTC = iTTC;
	rec->offset = offset;
	}

static void addTTCFont(txCtx h, int ttcIndex, long origin, long offset)
{
    ctlTag version;
    int result;
	result = sfrBegFont(h->ctx.sfr, &h->src.stm, offset, &version);
	switch (result)
    {
        case sfrSuccess:
        {    switch (version)
            {
                case sfr_v1_0_tag:
                case sfr_true_tag:
                    /* TrueType */
                    addFont(h, src_TrueType, ttcIndex, origin);
                    break;
                case sfr_OTTO_tag:
                    /* OTF */
                    addFont(h, src_OTF, ttcIndex, origin);
                    break;
                default:
                    fatal(h, "(sfr) %s", sfrErrStr(sfrErrBadSfnt));
            }
            break;
        case sfrErrBadSfnt:
            break;
        default:
            fatal(h, "(sfr) %s", sfrErrStr(result));
        }
    }

}

/* Add TrueType Collection font. */
static void addTTC(txCtx h, long origin)
{
    /* sfrGetNextTTCOffset() returns 0 when it is asked to get the next offset after the last real font,
    so it serves effectively as a test for iterating through all the fonts in the TTC.
     */
	long i;
    long offset;

	if (h->arg.i != NULL)
    {
		int j;
		i = strtol(h->arg.i, NULL, 0);
		if (i < 0)
			fatal(h, "bad TTC index (-i)");

        for (j = 0; (offset = sfrGetNextTTCOffset(h->ctx.sfr)); j++)
        {
            if (j < i)
                continue;
            addTTCFont(h, i, origin, offset);
            break;
        }

    }
	else if (h->flags & EVERY_FONT)
    /* -y option; add whole collection */
		for (i = 0; (offset = sfrGetNextTTCOffset(h->ctx.sfr)); i++)
        {
            addTTCFont(h, i, origin, offset);
        }
	else
    {

		/* Dump TTC index and quit */
		printf("### TrueType Collection (TTC)\n"
			   "\n"
			   "--- TableDirectory[index]=offset\n");
		for (i = 0; (offset = sfrGetNextTTCOffset(h->ctx.sfr)); i++)
			printf("[%ld]=%08lx\n", i, offset);

		printf("\n"
			   "Re-run %s and select a single table in the directory\n"
			   "with the -i option or every table with the -y option.\n",
			   h->progname);
		exit(1);
    }
}

/* Read 4-byte signature and try to match against sfnt. */
static void readsfnt(txCtx h, long origin)
	{
        /* Note that 'origin' is the offset from the file to the beginning of the
        font data for either a TTC, OTF or TTF font. It is NOT the offset to an SFNT-based font
        in a TTC. The sfr functions access tables in an sfnt-based font at an
        offset equal to the table offset from the sfnt table directory plus the 'origin' offset.
        in
         */
	enum
		{
		CID__HDR_SIZE = 4*4 + 3*2,	/* 'CID ' table header size */
		TYP1_HDR_SIZE = 5*4 + 2*2	/* 'TYP1' table header size */
		};
	ctlTag version;
	sfrTable *table;
	int result;

	if (h->ctx.sfr == NULL)
		{
		/* Initialize library */
		h->ctx.sfr =
			sfrNew(&h->cb.mem, &h->cb.stm, SFR_CHECK_ARGS);
		if (h->ctx.sfr == NULL)
			fatal(h, "(sfr) can't init lib");
		}

	result = sfrBegFont(h->ctx.sfr, &h->src.stm, origin, &version);
	switch (result)
		{
	case sfrSuccess:
		switch (version)
			{
		case sfr_v1_0_tag:
		case sfr_true_tag:
			/* TrueType */
			addFont(h, src_TrueType, 0, origin);
			break;
		case sfr_OTTO_tag:
			/* OTF */
			addFont(h, src_OTF, 0, origin);
			break;
		case sfr_typ1_tag:
			/* GX or sfnt-wrapped CID font */
			table = sfrGetTableByTag(h->ctx.sfr, CID__);
			if (table != NULL)
				addFont(h, src_Type1, 0, table->offset + CID__HDR_SIZE);
			else
				{
				table = sfrGetTableByTag(h->ctx.sfr, TYP1_);
				if (table != NULL)
					addFont(h, src_Type1, 0, table->offset + TYP1_HDR_SIZE);
				}
			break;
		case sfr_ttcf_tag:
			/* TrueType Collection */
			addTTC(h, origin);
			break;
			}
		break;
	case sfrErrBadSfnt:
		break;
	default:
		fatal(h, "(sfr) %s", sfrErrStr(result));
		}

	result = sfrEndFont(h->ctx.sfr);
	if (result)
		fatal(h, "(sfr) %s", sfrErrStr(result));
	}

/* ------------------------- Macintosh Resource Map ------------------------ */

/* Print Macintosh resource map. */
static void printResMap(txCtx h, long origin)
	{
	long i;
	printf("### Macintosh Resource Fork (%08lx)\n"
		   "\n"
		   "Type  Id   Attr  Offset   Length    Name\n"
		   "---- ----- ---- -------- -------- --------\n", origin);
	for (i = 0; i < h->res.map.cnt; i++)
		{
		ResInfo *res = &h->res.map.array[i];
		printf("%c%c%c%c %5hu  %02hhx  %08lx %08lx %s\n",
			   (int)(res->type>>24&0xff), (int)(res->type>>16&0xff),
			   (int)(res->type>>8&0xff), (int)(res->type&0xff),
			   res->id, res->attrs, res->offset, res->length,
			   (res->name == 0xffff)? "--none--":
			   &h->res.names.array[res->name]);
		}
	}

/* Print resource map and re-run note for user and quit. */
static void printNote(txCtx h, long origin)
	{
	printf("Macintosh FFIL with multiple sfnt resources:\n"
		   "\n");
	printResMap(h, origin);
	printf("\n"
		   "Re-run %s and select a single sfnt resource with the\n"
		   "-i option or every sfnt resource with the -y option.\n",
		   h->progname);
	exit(1);
	}

/* Read and process Macintosh resource map. */
static void doResMap(txCtx h, long origin)
	{
	/* Macintosh resource structures */
	struct
		{
		unsigned long mapOffset;
		} header;
	struct
		{
		unsigned short attrs;
		unsigned short typeListOffset;
		unsigned short nameListOffset;
		} map;
#if 0
	/* Included for reference only */
	struct
		{
		unsigned long type;
		unsigned short cnt;
		unsigned short refListOffset;
		} type;
	struct
		{
		unsigned short id;
		unsigned short nameOffset;
		char attrs;
		char dataOffset[3];
		unsigned long reserved;
		} refList;
#endif
	enum
		{
		HEADER_SIZE = 256,				/* Resource header size */
		MAP_SKIP_SIZE = 16 + 4 + 2*2,	/* Skip to typeListOffset */
		REFLIST_SKIP_SIZE = 2*2 + 1		/* Skip to dataOffset */
		};
	long typeListCnt;
	long i;
	long j;

	/* Read header (4-byte header.dataOffset already read) */
	header.mapOffset = origin + read4(h);

	/* Read map */
	bufSeek(h, header.mapOffset + MAP_SKIP_SIZE);
	map.typeListOffset = read2(h);
	map.nameListOffset = read2(h);

	h->res.map.cnt = 0;

	/* Read type list */
	typeListCnt = read2(h);
	for (i = 0; i <= typeListCnt; i++)
		{
		unsigned long type		= read4(h);
		unsigned short cnt 		= read2(h);
		unsigned short offset 	= read2(h);
		ResInfo *res = dnaEXTEND(h->res.map, cnt + 1);
		res->type 	= type;
		res->length = cnt;
		res->offset = offset;
		for (j = 1; j <= cnt; j++)
			res[j].type = type;
		}

	/* Read reference list */
	i = 0;
	while (i < h->res.map.cnt)
		{
		ResInfo *res = &h->res.map.array[i];
		long cnt = (long)res->length;
		bufSeek(h, header.mapOffset + map.typeListOffset + res->offset);
		for (j = 0; j <= cnt; j++)
			{
			res->id		= read2(h);
			res->name	= read2(h);
			res->attrs	= read1(h);
			res->offset = (unsigned long)read1(h)<<16;
			res->offset |= read1(h)<<8;
			res->offset |= read1(h);
			res->offset += HEADER_SIZE;
			res->offset += origin;
			(void)read4(h);
			res++;
			}
		i += j;
		}

	/* Read names */
	for (i = 0; i < h->res.map.cnt; i++)
		{
		ResInfo *res = &h->res.map.array[i];
		if (res->name != 0xffff)
			{
			char *name;
			int length;
			bufSeek(h, header.mapOffset + map.nameListOffset + res->name);
			length = read1(h);
			res->name = h->res.names.cnt;
			name = dnaEXTEND(h->res.names, length + 1);
			readN(h, length, name);
			name[length] = '\0';
			}
		}

	/* Read resource data lengths */
	for (i = 0; i < h->res.map.cnt; i++)
		{
		ResInfo *res = &h->res.map.array[i];
		bufSeek(h, res->offset);
		res->length = read4(h);
		if (res->type != POST_)
			res->offset += 4;
		}

	/* Process resource map */
	if (h->flags & DUMP_RES)
		{
		/* -r option; print resource map and exit */
		printResMap(h, origin);
		exit(0);
		}
	else if (h->arg.i != NULL)
		{
		/* -i option; look for specific sfnt resource */
		unsigned short id = (unsigned short)strtol(h->arg.i, NULL, 0);

		for (i = 0; i < h->res.map.cnt; i++)
			{
			ResInfo *res = &h->res.map.array[i];
			if (res->type == sfnt_ && res->id == id)
				{
				readsfnt(h, res->offset);
				return;
				}
			}
		fatal(h, "resource not found");
		}
	else
		{
		/* Look for sfnt/POST resources */
		for (i = 0; i < h->res.map.cnt; i++)
			{
			ResInfo *res = &h->res.map.array[i];
			switch (res->type)
				{
			case sfnt_:
				if (h->flags & EVERY_FONT)
					/* -y option; add sfnt */
					readsfnt(h, res->offset);
				else if (i + 1 == h->res.map.cnt ||
						 h->res.map.array[i + 1].type != sfnt_)
					{
					/* Singleton sfnt resource */
					readsfnt(h, res->offset);
					return;
					}
				else
					/* Multiple sfnt resources; print note */
					printNote(h, origin);
				break;
			case POST_:
				h->seg.refill = POSTRefill;
				addFont(h, src_Type1, 0, res->offset);
				return;
				}
			}
		}
	}

/* ----------------------- AppleSingle/Double Formats ---------------------- */

/* Process AppleSingle/Double format data. */
static void doASDFormats(txCtx h, ctlTag magic)
	{
	char junk[16];
	long i;

	h->asd.magic = magic;
	h->asd.version = read4(h);

	/* Skip filler */
	readN(h, sizeof(junk), junk);

	/* Read number of entries */
	dnaSET_CNT(h->asd.entries, read2(h));

	/* Read entry descriptors */
	for (i = 0; i < h->asd.entries.cnt; i++)
		{
		EntryDesc *entry = &h->asd.entries.array[i];
		entry->id     = read4(h);
		entry->offset = read4(h);
		entry->length = read4(h);
		}

	if (h->flags & DUMP_ASD)
		{
		/* -R option; print AppleSingle/Double data */
		printf("### %s Format, Version %1.1f\n"
			   "\n",
			   (h->asd.magic == sig_AppleSingle)? "AppleSingle": "AppleDouble",
			   h->asd.version/65536.0);

		printf("Id  Offset   Length   Description\n"
			   "-- -------- -------- -------------\n");

		for (i = 0; i < h->asd.entries.cnt; i++)
			{
			static char *desc[] =
			{
				/* 00 */	"--unknown--",
				/* 01 */	"Data Fork",
				/* 02 */	"Resource Fork",
				/* 03 */	"Real Name",
				/* 04 */	"Comment",
				/* 05 */	"Icon, B&W",
				/* 06 */	"Icon, Color",
				/* 07 */	"File Info (old format)",
				/* 08 */	"File Dates Info",
				/* 09 */	"Finder Info",
				/* 10 */	"Macintosh File Info",
				/* 11 */	"ProDOS File Info",
				/* 12 */	"MS-DOS File Info",
				/* 13 */	"Short Name",
				/* 14 */	"AFP File Info",
				/* 15 */	"Directory ID",
			};
			EntryDesc *entry = &h->asd.entries.array[i];
			printf("%02lx %08lx %08lx %s\n",
				   entry->id, entry->offset, entry->length,
				   (entry->id < ARRAY_LEN(desc))?
				   desc[entry->id]: "--unknown--");
			}
		exit(0);
		}
	else
		for (i = 0; i < h->asd.entries.cnt; i++)
			{
			EntryDesc *entry = &h->asd.entries.array[i];
			if (entry->length > 0)
				switch (entry->id)
					{
				case 1:
					/* Data fork (AppleSingle); see if it's an sfnt */
					readsfnt(h, entry->offset);
					break;
				case 2:
					/* Resource fork (AppleSingle/Double) */
					bufSeek(h,  entry->offset + 4);
					doResMap(h, entry->offset);
					break;
					}
			}
	}

/* Scan source file for fonts and build font list. */
static void buildFontList(txCtx h)
{
	ctlTag sig;
    int fillErr = 0;
	h->fonts.cnt = 0;

	/* Initialize segment */
	h->seg.refill = NULL;

    if (h->src.stm.fp == NULL)
    {
        /* We get here only if h->file.src is a directory. Check if it is UFO font */
        char tempFileName[FILENAME_MAX];
        FILE* tempFP;
        sprintf(tempFileName, "%s/glyphs/contents.plist", h->file.src);
        tempFP = fopen(tempFileName, "rt");
        if (tempFP != NULL)
        {
            // it is a ufo font!
            fclose(tempFP);
            addFont(h, src_UFO, 0, 0);
        }
        else
        {
            fileError(h, h->src.stm.filename);
        }
    }
    else
    {
        /* Read first buffer */
        fillbuf(h, 0);
        /* Make 2-byte signature */
        sig = (ctlTag)read1(h)<<24;
        sig |= (ctlTag)read1(h)<<16;

        switch (sig)
        {
            case sig_PostScript0:
            case sig_PostScript1:
            case sig_PostScript2:
                addFont(h, src_Type1, 0, 0);
                break;
            case sig_PFB:
                h->seg.refill = PFBRefill;
                addFont(h, src_Type1, 0, 0);
                break;
            case sig_CFF:
            case sig_CFF2:
                addFont(h, src_CFF, 0, 0);
                break;
            default:
                /* Make 4-byte signature */
                sig |= read1(h)<<8;
                sig |= read1(h);
                switch (sig)
            {
                case sig_MacResource:
                    doResMap(h, 0);
                    break;
                case sig_AppleSingle:
                case sig_AppleDouble:
                    doASDFormats(h, sig);
                    break;
                default:
                    readsfnt(h, 0);
            }
        }

        if (h->fonts.cnt == 0)
        {
            if ((0 == strncmp(h->src.buf, "<font", 5)) || (0 == strncmp(h->src.buf, "<svg", 4) ))
                addFont(h, src_SVG, 0, 0);
        }
    }
	if (h->fonts.cnt == 0)
		fatal(h, "bad font file: %s", h->src.stm.filename);
}

/* ----------------------------- Usage and Help ---------------------------- */

/* Print usage information. */
static void usage(txCtx h)
	{
	static char *text[] =
		{
#include "usage.h"
		};
	printText(ARRAY_LEN(text), text);
	exit(0);
	}

/* Show help information. */
static void help(txCtx h)
	{
	if (h->flags & SEEN_MODE)
		/* Mode-specific help */
		switch (h->mode)
			{
		case mode_dump:
			dump_Help(h);
			break;
		case mode_ps:
			ps_Help(h);
			break;
		case mode_pdf:
			pdf_Help(h);
			break;
		case mode_afm:
			afm_Help(h);
			break;
		case mode_cff:
			cff_Help(h);
			break;
		case mode_cef:
			cef_Help(h);
			break;
		case mode_path:
			path_Help(h);
			break;
		case mode_mtx:
			mtx_Help(h);
			break;
		case mode_t1:
			t1_Help(h);
			break;
		case mode_svg:
			svg_Help(h);
			break;
        case mode_dcf:
            dcf_Help(h);
            break;
        case mode_ufow:
            ufo_Help(h);
            break;
			}
	else
		{
		/* General help */
		static char *text[] =
			{
#include "help.h"
			};
		printText(ARRAY_LEN(text), text);
		}
	exit(0);
	}

/* Add arguments from script file. */
static void addArgs(txCtx h, char *filename)
	{
	int state;
	long i;
	size_t length;
	FILE *fp;
	char *start = NULL;	/* Suppress optimizer warning */

	/* Open script file */
	if ((fp = fopen(filename, "rb")) == NULL ||
		fseek(fp, 0, SEEK_END) == -1)
		fileError(h, filename);

	/* Size file and allocate buffer */
	length = ftell(fp) + 1;
	h->script.buf = memNew(h, length);

	/* Read whole file into buffer and close file */
	if (fseek(fp, 0, SEEK_SET) == -1 ||
		fread(h->script.buf, 1, length, fp) != length - 1 ||
		fclose(fp) == EOF)
		fileError(h, filename);

	h->script.buf[length - 1] = '\n';	/* Ensure termination */

	/* Parse buffer into args */
	state = 0;
	for (i = 0; i < (long)length; i++)
		{
		int c = h->script.buf[i] & 0xff;
		switch (state)
			{
		case 0:
			switch (c)
				{
			case ' ': case '\n': case '\t': case '\r': case '\f':
				break;
			case '#':
				state = 1;
				break;
			case '"':
				start = &h->script.buf[i + 1];
				state = 2;
				break;
			default:
				start = &h->script.buf[i];
				state = 3;
				break;
				}
			break;
		case 1:	/* Comment */
			if (c == '\n' || c == '\r')
				state = 0;
			break;
		case 2:	/* Quoted string */
			if (c == '"')
				{
				h->script.buf[i] = '\0';	/* Terminate string */
				*dnaNEXT(h->script.args) = start;
				state = 0;
				}
			break;
		case 3:	/* Space-delimited string */
			if (isspace(c))
				{
				h->script.buf[i] = '\0';	/* Terminate string */
				*dnaNEXT(h->script.args) = start;
				state = 0;
				}
			break;
			}
		}
	}

/* Get version callback function. */
static void getversion(ctlVersionCallbacks *cb, long version, char *libname)
	{
	char version_buf[MAX_VERSION_SIZE];
	printf("    %-10s%s\n", libname, CTL_SPLIT_VERSION(version_buf, version));
	}

/* Print library version numbers. */
static void printVersions(txCtx h)
	{
	ctlVersionCallbacks cb;
	char version_buf[MAX_VERSION_SIZE];

	printf("Versions:\n"
		   "    tx        %s\n", CTL_SPLIT_VERSION(version_buf, TX_VERSION));

	cb.ctx = NULL;
	cb.called = 0;
	cb.getversion = getversion;

	abfGetVersion(&cb);
	cefGetVersion(&cb);
	cfrGetVersion(&cb);
	cfwGetVersion(&cb);
	ctuGetVersion(&cb);
	dnaGetVersion(&cb);
	pdwGetVersion(&cb);
	sfrGetVersion(&cb);
	t1rGetVersion(&cb);
	svrGetVersion(&cb);
	ttrGetVersion(&cb);
	t1wGetVersion(&cb);
    svwGetVersion(&cb);
    ufoGetVersion(&cb);
    ufwGetVersion(&cb);
    varreadGetVersion(&cb);

	exit(0);
	}

/* Setup new mode. */
static void setMode(txCtx h, int mode)
	{
	/* Initialize files */
	h->file.sr = NULL;
	h->file.sd = NULL;
	h->file.dd = NULL;
	strcpy(h->file.src, "-");
	strcpy(h->file.dst, "-");

	/* Begin new mode */
	switch (mode)
		{
	case mode_dump:
		dump_SetMode(h);
		break;
	case mode_ps:
		ps_SetMode(h);
		break;
	case mode_afm:
		afm_SetMode(h);
		break;
	case mode_path:
		path_SetMode(h);
		break;
	case mode_cff:
		cff_SetMode(h);
		break;
	case mode_cef:
		cef_SetMode(h);
		break;
	case mode_pdf:
		pdf_SetMode(h);
		break;
	case mode_mtx:
		mtx_SetMode(h);
		break;
	case mode_t1:
		t1_SetMode(h);
		break;
    case mode_svg:
        svg_SetMode(h);
        break;
    case mode_ufow:
        ufo_SetMode(h);
        break;
	case mode_dcf:
		dcf_SetMode(h);
		break;
		}

	h->flags |= SEEN_MODE;
	}

/* Match options. */
static int CTL_CDECL matchOpts(const void *key, const void *value)
	{
	return strcmp((char *)key, *(char **)value);
 	}

/* Return option index from key or opt_None if not found. */
static int getOptionIndex(char *key)
	{
	const char **optstr =
		(const char **)bsearch(key, options, ARRAY_LEN(options),
							   sizeof(options[0]), matchOpts);
	return (int)((optstr == NULL)? opt_None: optstr - options + 1);
	}

/* Process file. */
static void doFile(txCtx h, char *srcname)
	{
	long i;
	char *p;
    struct stat fileStat;
    int statErrNo;

	/* Set src name */
	if (h->file.sr != NULL)
		{
		sprintf(h->file.src, "%s/", h->file.sr);
		p = &h->file.src[strlen(h->file.src)];
		}
	else
		p = h->file.src;
	if (h->file.sd != NULL)
		sprintf(p, "%s/%s", h->file.sd, srcname);
	else
		strcpy(p, srcname);

	/* Open file */

    /* Need to first check if it is a directory-based font format, like UFO. */
    statErrNo = stat(h->src.stm.filename,  &fileStat);
    if (strcmp(h->src.stm.filename, "-") == 0)
        h->src.stm.fp = stdin;
    else if ((statErrNo == 0) && ((fileStat.st_mode & S_IFDIR) != 0))
    {
        /* maybe it is a dir-based font, like UFO. Will verify this in buildFontList(h) */
        h->src.stm.fp = NULL;
    }
    else
    {
        h->src.stm.fp = fopen(h->src.stm.filename, "rb");
		if (h->src.stm.fp == NULL)
            fileError(h, h->src.stm.filename);
    }

	h->src.print_file = 1;

	if (h->flags & SHOW_NAMES)
		{
		fflush(stdout);
		fprintf(stderr, "--- Filename: %s\n", h->src.stm.filename);
		}

	/* The font file we are reading may contain muliple fonts, e.g. a TTC or
	   multiple sfnt resources, so keep open until the last font processed */
	h->src.stm.flags |= STM_DONT_CLOSE;

	/* Read file and build font list */
	buildFontList(h);

	/* Process font list */
	for (i = 0; i < h->fonts.cnt; i++)
		{
		FontRec *rec = &h->fonts.array[i];

		if (i + 1 == h->fonts.cnt)
			h->src.stm.flags &= ~STM_DONT_CLOSE;

		h->src.type = rec->type;

		if (h->seg.refill != NULL)
			{
			/* Prep source filter */
			h->seg.left = 0;
			h->src.next = h->src.end;
			}

		/* Process font according to type */
		switch (h->src.type)
			{
		case src_Type1:
			t1rReadFont(h, rec->offset);
			break;
		case src_OTF:
			h->cfr.flags |= CFR_NO_ENCODING;
			/* Fall through */
		case src_CFF:
			cfrReadFont(h, rec->offset, rec->iTTC);
			break;
		case src_TrueType:
			ttrReadFont(h, rec->offset, rec->iTTC);
			break;
		case src_SVG:
			svrReadFont(h, rec->offset);
			break;
		case src_UFO:
            ufoReadFont(h, rec->offset);
			break;
			}
		}

	h->arg.i = NULL;
	h->flags |= DONE_FILE;
	}

/* Process multi-file set. Return index of last used arg. */
static int doMultiFileSet(txCtx h, int argc, char *argv[], int i)
	{
	int filecnt = 0;

	h->dst.begset(h);
	for (; i < argc; i++)
		switch (getOptionIndex(argv[i]))
			{
		case opt_None:
			doFile(h, argv[i]);
			filecnt++;
			break;
		case opt_sd:
			if (++i == argc)
				fatal(h, "no argument for option (-sd)");
			h->file.sd = argv[i];
			break;
		case opt_sr:
			if (++i == argc)
				fatal(h, "no argument for option (-sr)");
			h->file.sr = argv[i];
			break;
		case opt_dd:
			if (++i == argc)
				fatal(h, "no argument for option (-dd)");
			h->file.dd = argv[i];
			break;
		default:
			goto finish;
			}

 finish:
	if (filecnt == 0)
		fatal(h, "empty list (-f)");

	h->dst.endset(h);
	return i - 1;
	}

/* Process single file set. */
static void doSingleFileSet(txCtx h, char *srcname)
	{
	h->dst.begset(h);
	doFile(h, srcname);
	h->dst.endset(h);
	}

/* Process auto-file set. Return index of last used arg. */
static int doAutoFileSet(txCtx h, int argc, char *argv[], int i)
	{
	int filecnt = 0;

	for (; i < argc; i++)
		switch (getOptionIndex(argv[i]))
			{
		case opt_None:
			doSingleFileSet(h, argv[i]);
			filecnt++;
			break;
		case opt_sd:
			if (++i == argc)
				fatal(h, "no argument for option (-sd)");
			h->file.sd = argv[i];
			break;
		case opt_sr:
			if (++i == argc)
				fatal(h, "no argument for option (-sr)");
			h->file.sr = argv[i];
			break;
		case opt_dd:
			if (++i == argc)
				fatal(h, "no argument for option (-dd)");
			h->file.dd = argv[i];
			break;
		default:
			goto finish;
			}

 finish:
	if (filecnt == 0)
		fatal(h, "empty list (-a/-A)");

	return i - 1;
	}

/* Parse argument list. */
static void parseArgs(txCtx h, int argc, char *argv[])
	{
	int i;
	char *arg;

	h->t1r.flags = 0; /* I initialize these here,as I need to set the std Encoding flags before calling setMode. */
	h->cfr.flags = 0;
	h->cfw.flags = 0;
	h->dcf.flags = DCF_AllTables|DCF_BreakFlowed;
	h->dcf.level = 5;
	h->svr.flags = 0;
	h->ufr.flags = 0;
	h->ufow.flags = 0;
    h->t1w.options = 0;

	for (i = 0; i < argc; i++)
		{
		int argsleft = argc - i - 1;
		arg = argv[i];
		switch (getOptionIndex(arg))
			{
		case opt_None:
			/* Not option, assume filename */
			if (argsleft > 0)
				{
				char *dstname = argv[i + 1];
				if (getOptionIndex(dstname) == opt_None)
					{
					if (argsleft >1 && getOptionIndex(argv[i + 2]) == opt_None)
						fatal(h, "too many file args [%s]", argv[i + 2]);
					dstFileSetName(h, dstname);
					i++;	/* Consume arg */
					}
				}
			doSingleFileSet(h, arg);
			break;
		case opt_dump:			/* mode selection options */
			setMode(h, mode_dump);
			break;
		case opt_ps:
			setMode(h, mode_ps);
			break;
		case opt_afm:
			setMode(h, mode_afm);
			break;
		case opt_path:
			setMode(h, mode_path);
			break;
        case opt_cff:
            setMode(h, mode_cff);
            break;
        case opt_cff2:
            h->cfw.flags |= CFW_WRITE_CFF2;
            setMode(h, mode_cff);
            break;
		case opt_cef:
			setMode(h, mode_cef);
			break;
		case opt_cefsvg:
			if (h->mode != mode_cef)
				goto wrongmode;
			h->arg.cef.flags |= CEF_WRITE_SVG;
			break;
		case opt_pdf:
			setMode(h, mode_pdf);
			break;
		case opt_mtx:
			setMode(h, mode_mtx);
			break;
		case opt_t1:
			setMode(h, mode_t1);
			break;
        case opt_svg:
            setMode(h, mode_svg);
            break;
        case opt_ufo:
            setMode(h, mode_ufow);
            break;
		case opt_bc:
            goto bc_gone;
			break;
		case opt_dcf:
			setMode(h, mode_dcf);
			break;
        case opt_altLayer:
            h->ufr.altLayerDir = argv[++i];
            break;
		case opt_l:
			switch (h->mode)
				{
			case mode_t1:
				h->t1w.flags &= ~T1W_ENCODE_MASK;
				h->t1w.flags |= T1W_ENCODE_ASCII;
				break;
			case mode_ps:
				h->abf.draw.flags |= ABF_NO_LABELS;
				break;
			default:
				goto wrongmode;
				}
			break;
		case opt_0:				/* dump/ps/path mode options */
			switch (h->mode)
				{
			case mode_dump:
				h->abf.dump.level = 0;
				break;
			case mode_ps:
				h->abf.draw.level = 0;
				break;
			case mode_path:
				h->arg.path.level = 0;
				break;
			case mode_mtx:
				h->mtx.level = 0;
				break;
			case mode_t1:
				h->t1w.flags &= ~T1W_TYPE_MASK;
				h->t1w.flags |= T1W_TYPE_HOST;
				break;
			case mode_dcf:
				h->dcf.level = 0;
				break;
			default:
				goto wrongmode;
				}
			break;
		case opt_1:
			switch (h->mode)
				{
			case mode_dump:
				h->abf.dump.level = 1;
				break;
			case mode_ps:
				h->abf.draw.level = 1;
				break;
			case mode_path:
				h->arg.path.level = 1;
				break;
			case mode_pdf:
				h->pdw.level = 1;
				break;
			case mode_mtx:
				h->mtx.level = 1;
				break;
			case mode_t1:
				h->t1w.flags &= ~T1W_TYPE_MASK;
				h->t1w.flags |= T1W_TYPE_BASE;
				break;
			case mode_dcf:
				h->dcf.level = 1;
				break;
			default:
				goto wrongmode;
				}
			break;
		case opt_2:
			switch (h->mode)
				{
			case mode_dump:
				h->abf.dump.level = 2;
				break;
			case mode_pdf:
				fatal(h, "unimplemented option (-2) for mode (-pdf)");
				h->pdw.level = 2;
				break;
			case mode_mtx:
				h->mtx.level = 2;
				break;
			case mode_t1:
				h->t1w.flags &= ~T1W_TYPE_MASK;
				h->t1w.flags |= T1W_TYPE_ADDN;
				break;
			default:
				goto wrongmode;
				}
			break;
		case opt_3:				/* dump mode options */
			switch (h->mode)
				{
			case mode_dump:
				h->abf.dump.level = 3;
				break;
			case mode_mtx:
				h->mtx.level = 3;
				break;
			default:
				goto wrongmode;
				}
			break;
		case opt_4:
			if (h->mode == mode_dump)
				h->abf.dump.level = 4;
			else
				goto wrongmode;
			break;
		case opt_5:
			switch (h->mode)
				{
			case mode_dump:
				h->abf.dump.level = 5;
				break;
			case mode_dcf:
				h->dcf.level = 5;
				break;
			default:
				goto wrongmode;
				}
			break;
		case opt_6:
			switch (h->mode)
				{
			case mode_dump:
				h->abf.dump.level = 6;
				break;
			default:
				goto wrongmode;
				}
			break;
		case opt_n:
            h->flags |= PATH_SUPRESS_HINTS;
            /* Setting the hint callbacks to NULL works for the most common case where the
             callbacks have already been assigned by setMode. Hwoever, in a number of cases, the call backs
             are assigned later, within beginFont. This, we need the flag, so we can do the right thing there.*/
            switch (h->mode)
            {
                case mode_ps:
                case mode_t1:
                case mode_cff:
                case mode_cef:
                case mode_svg:
                case mode_ufow:
                case mode_dump:
                    h->cb.glyph.stem = NULL;
                    h->cb.glyph.flex = NULL;
                    break;
                default:
                    goto wrongmode;
            }
			break;
      case opt_no_futile:
          switch (h->mode)
          {
              case mode_cff:
                  h->cfw.flags |= CFW_NO_FUTILE_SUBRS;
                  break;
              default:
                  goto wrongmode;
          }
          break;
        case opt_no_opt:
            switch (h->mode)
            {
                case mode_cff:
                    h->cfw.flags |= CFW_NO_OPTIMIZATION;
                    break;
                default:
                    goto wrongmode;
            }
            break;
		case opt__E:
			switch (h->mode)
				{
			case mode_cff:
				h->cfw.flags |= CFW_EMBED_OPT;
				break;
			case mode_t1:
				h->t1w.options |= T1W_WAS_EMBEDDED;
				break;
			default:
				goto wrongmode;
				}
			break;
		case opt_E:
			switch (h->mode)
				{
			case mode_cff:
				h->cfw.flags &= ~CFW_EMBED_OPT;
				break;
			case mode_t1:
				h->t1w.options &= ~T1W_WAS_EMBEDDED;
				break;
			default:
				goto wrongmode;
				}
			break;
		case opt__F:			/* cff mode options */
			if (h->mode != mode_cff)
				goto wrongmode;
			h->cfw.flags &= ~CFW_NO_FAMILY_OPT;
			break;
		case opt__O:
			if (h->mode != mode_cff)
				goto wrongmode;
			h->cfw.flags |= CFW_ROM_OPT;
			break;
		case opt_O:
			if (h->mode != mode_cff)
				goto wrongmode;
			h->cfw.flags &= ~CFW_ROM_OPT;
			break;
		case opt__S:
			switch (h->mode)
				{
			case mode_cff:
				h->cfw.flags |= CFW_SUBRIZE;
				break;
			case mode_t1:
				h->t1w.flags &= ~T1W_OTHERSUBRS_MASK;
				h->t1w.flags |= T1W_OTHERSUBRS_PROCSET;
				break;
			default:
				goto wrongmode;
				}
			break;
		case opt_S:
			switch (h->mode)
				{
			case mode_cff:
				h->cfw.flags &= ~CFW_SUBRIZE;
				break;
			case mode_t1:
				h->t1w.flags &= ~T1W_OTHERSUBRS_MASK;
				h->t1w.flags |= T1W_OTHERSUBRS_PRIVATE;
				break;
			default:
				goto wrongmode;
				}
			break;
		case opt__T:
			switch (h->mode)
				{
			case mode_cff:
				h->cfw.flags &= ~(CFW_NO_FAMILY_OPT|CFW_ENABLE_CMP_TEST);
				break;
			case mode_t1:
				h->t1w.flags &= ~T1W_ENABLE_CMP_TEST;
				break;
			default:
				goto wrongmode;
				}
			break;
        case opt_V:
			switch (h->mode)
				{
			case mode_cff:
			case mode_t1:
				h->flags &= ~PATH_REMOVE_OVERLAP;
				break;
			default:
				goto wrongmode;
				}
			break;
        case opt__V:
			switch (h->mode)
				{
			case mode_cff:
			case mode_t1:
				h->flags |= PATH_REMOVE_OVERLAP;
				break;
			default:
				goto wrongmode;
				}
			break;
		case opt_Z:
			/* Although CFW_NO_DEP_OPS is defined in cffwrite.h,
			* it is not used anywhere.
			*/
			/*
			if (h->mode != mode_cff)
				goto wrongmode;
			h->cfw.flags |= CFW_NO_DEP_OPS;
			*/
			h->t1r.flags |= T1R_UPDATE_OPS;
			h->cfr.flags |= CFR_UPDATE_OPS;
			break;
		case opt__Z:
			h->t1r.flags  &= ~T1R_UPDATE_OPS;
			h->cfr.flags  &= ~CFR_UPDATE_OPS;
			if (h->mode != mode_cff)
				goto wrongmode;
			h->cfw.flags &= ~CFW_NO_DEP_OPS;
			break;
		case opt__d:
			if (h->mode != mode_cff)
				goto wrongmode;
			h->cfw.flags |= CFW_WARN_DUP_HINTSUBS;
			break;
		case opt_d:
			switch (h->mode)
				{
			case mode_dump:
				h->t1r.flags |= T1R_UPDATE_OPS;
				h->cfr.flags |= CFR_UPDATE_OPS;
				break;
			case mode_ps:
				h->abf.draw.flags |= ABF_DUPLEX_PRINT;
				break;
			case mode_cff:
				h->cfw.flags &= ~CFW_WARN_DUP_HINTSUBS;
				break;
			default:
				goto wrongmode;
				}
			break;
		case opt_q:			/* t1 mode options */
			if (h->mode != mode_t1)
				goto wrongmode;
			else if (h->t1w.options & T1W_REFORMAT)
				goto t1clash;
			h->t1w.options |= T1W_NO_UID;
			break;
		case opt__q:
			if (h->mode != mode_t1)
				goto wrongmode;
			else if (h->t1w.options & T1W_REFORMAT)
				goto t1clash;
			h->t1w.options &= ~T1W_NO_UID;
			break;
		case opt_w:
			if (h->mode != mode_t1)
				goto wrongmode;
			else if (h->t1w.options & T1W_REFORMAT)
				goto t1clash;
			h->t1w.flags &= ~T1W_WIDTHS_ONLY;
			break;
		case opt__w:
			if (h->mode != mode_t1)
				goto wrongmode;
			else if (h->t1w.options & T1W_REFORMAT)
				goto t1clash;
			h->t1w.flags |= T1W_WIDTHS_ONLY;
			break;
		case opt_lf:
			if ((h->mode == mode_svg) ||
				((h->mode == mode_cef) && (h->arg.cef.flags & CEF_WRITE_SVG)))
				{
				h->svw.flags &= ~SVW_NEWLINE_MASK;
				h->svw.flags |= SVW_NEWLINE_UNIX;
				}
			else if (h->mode == mode_t1)
				{
				if (h->t1w.options & T1W_REFORMAT)
					goto t1clash;
				h->t1w.flags &= ~T1W_NEWLINE_MASK;
				h->t1w.flags |= T1W_NEWLINE_UNIX;
				}
			else
				goto wrongmode;
			break;
		case opt_cr:
			if ((h->mode == mode_svg) ||
				((h->mode == mode_cef) && (h->arg.cef.flags & CEF_WRITE_SVG)))
				{
				h->svw.flags &= ~SVW_NEWLINE_MASK;
				h->svw.flags |= SVW_NEWLINE_MAC;
				}
			else if (h->mode == mode_t1)
				{
				if (h->t1w.options & T1W_REFORMAT)
					goto t1clash;
				h->t1w.flags &= ~T1W_NEWLINE_MASK;
				h->t1w.flags |= T1W_NEWLINE_MAC;
				}
			else
				goto wrongmode;
			break;
		case opt_crlf:
			if ((h->mode == mode_svg) ||
				((h->mode == mode_cef) && (h->arg.cef.flags & CEF_WRITE_SVG)))
				{
				h->svw.flags &= ~SVW_NEWLINE_MASK;
				h->svw.flags |= SVW_NEWLINE_WIN;
				}
			else if (h->mode == mode_t1)
				{
				if (h->t1w.options & T1W_REFORMAT)
					goto t1clash;
				h->t1w.flags &= ~T1W_NEWLINE_MASK;
				h->t1w.flags |= T1W_NEWLINE_WIN;
				}
			else
				goto wrongmode;
			break;
		case opt_decid:
			if (h->mode != mode_t1)
				goto wrongmode;
			else if (h->t1w.options & T1W_REFORMAT)
				goto t1clash;
			h->t1w.options |= T1W_DECID;
			break;
		case opt_usefd:
			if (h->mode != mode_t1)
				goto wrongmode;
			else if (!argsleft)
				goto noarg;
			else
				{
				char *p;
				h->t1w.fd = strtol(argv[++i], &p, 0);
				if (*p != '\0' || h->t1w.fd < 0)
					goto badarg;
				}
			h->t1w.options |= T1W_USEFD;
			break;
		case opt_pfb:
			if (h->mode != mode_t1)
				goto wrongmode;
			else if (h->t1w.options & T1W_REFORMAT)
				goto t1clash;
			h->t1w.flags = (T1W_TYPE_HOST|
							T1W_ENCODE_BINARY|
							T1W_OTHERSUBRS_PRIVATE|
							T1W_NEWLINE_WIN);
			h->t1w.lenIV = 4;
			h->t1w.options |= T1W_REFORMAT;
			break;
		case opt_LWFN:
			if (h->mode != mode_t1)
				goto wrongmode;
			else if (h->t1w.options & T1W_REFORMAT)
				goto t1clash;
			h->t1w.flags = (T1W_TYPE_HOST|
							T1W_ENCODE_BINARY|
							T1W_OTHERSUBRS_PRIVATE|
							T1W_NEWLINE_MAC);
			h->t1w.lenIV = 4;
			h->t1w.options |= T1W_REFORMAT;
			break;
		case opt_z:				/* bc mode options */
            goto bc_gone;
			break;
		case opt_sha1:
            goto bc_gone;
			break;
		case opt_cmp:
            goto bc_gone;
			break;
		case opt_cube2:
			h->cfr.flags |= CFR_CUBE_RND;
			h->cfw.flags |= CFW_CUBE_RND;
		case opt_cube:
			h->t1r.flags |= T1R_IS_CUBE;
			h->cfr.flags |= CFR_IS_CUBE;
			h->t1w.flags |= T1W_IS_CUBE;
			h->cfw.flags |= CFW_IS_CUBE;
			switch (h->mode)
				{
			case mode_t1:
				h->t1w.flags |= T1W_IS_CUBE;
				break;
			case mode_cff:
				h->cfw.flags |= CFW_IS_CUBE;
				break;
			case mode_bc:
                goto bc_gone;
				break;
			case mode_dcf:
				h->dcf.flags |= DCF_IS_CUBE;
				break;
			case mode_pdf:
				h->dcf.flags |= DCF_IS_CUBE;
				h->t1r.flags |= T1R_FLATTEN_CUBE;
				h->cfr.flags |= CFR_FLATTEN_CUBE;
				break;
				}
			break;
		case opt_flatten_cube2:
			h->cfr.flags |= CFR_CUBE_RND;
			h->cfw.flags |= CFW_CUBE_RND;
		case opt_flatten_cube:
			h->t1r.flags |= T1R_IS_CUBE;
			h->cfr.flags |= CFR_IS_CUBE;
			h->t1r.flags |= T1R_FLATTEN_CUBE;
			h->cfr.flags |= CFR_FLATTEN_CUBE;
			switch (h->mode)
				{
			case mode_dcf:
				goto wrongmode;
				break;
			default:
				break;
				}
				break;
		case opt_c:
			switch (h->mode)
				{
			case mode_t1:
				h->t1w.flags &= ~T1W_ENCODE_MASK;
				h->t1w.flags |= T1W_ENCODE_ASCII85;
				break;
			case mode_bc:
                goto bc_gone;
				break;
			default:
 				goto wrongmode;
				}
			break;
		case opt_F:					/* Shared options */
			switch (h->mode)
				{
			case mode_cef:
				if (!argsleft)
					goto noarg;
				h->arg.cef.F = argv[++i];
				break;
			case mode_cff:
				h->cfw.flags |= CFW_NO_FAMILY_OPT;
				break;
			default:
				goto wrongmode;
				}
			break;
		case opt_T:
			switch (h->mode)
				{
			case mode_dcf:
				if (!argsleft)
					goto noarg;
				dcf_ParseTableArg(h, argv[++i]);
				break;
			case mode_cff:
				h->cfw.flags |= CFW_NO_FAMILY_OPT|CFW_ENABLE_CMP_TEST;
				break;
			case mode_t1:
				h->t1w.flags |= T1W_ENABLE_CMP_TEST;
				break;
			default:
				goto wrongmode;
				}
			break;
		case opt__b:
			if (h->mode != mode_cff)
				goto wrongmode;
			h->cfw.flags |= CFW_PRESERVE_GLYPH_ORDER;
			break;
		case opt_b:
			switch (h->mode)
				{
			case mode_t1:
				h->t1w.flags &= ~T1W_ENCODE_MASK;
				h->t1w.flags |= T1W_ENCODE_BINARY;
				break;
			case mode_cff:
				h->cfw.flags &= ~CFW_PRESERVE_GLYPH_ORDER;
				break;
			case mode_dcf:
				h->dcf.flags &= ~DCF_BreakFlowed;
				break;
			default:
				goto wrongmode;
				}
			break;
		case opt_e:
			switch (h->mode)
				{
			case mode_ps:
				h->abf.draw.flags |= ABF_SHOW_BY_ENC;
				break;
			case mode_t1:
				if (h->t1w.options & T1W_REFORMAT)
					goto t1clash;
				else if (!argsleft)
					goto noarg;
				else
					{
					char *p;
					h->t1w.lenIV = (int)strtol(argv[++i], &p, 0);
					if (*p != '\0')
						goto badarg;
					switch (h->t1w.lenIV)
						{
					case -1:
					case 0:
					case 1:
					case 4:
						break;
					default:
						goto badarg;
						}
					}
				break;
			case mode_bc:
                goto bc_gone;
				break;
			default:
				goto wrongmode;
				}
			break;
		case opt_gx:
			if ((h->flags & SUBSET_OPT) || (h->flags & SUBSET__EXCLUDE_OPT))
				goto subsetclash;
			h->flags |= SUBSET__EXCLUDE_OPT;
		case opt_g:
			if (!argsleft)
				goto noarg;
			else
				{
				char *p;

				if ( (h->arg.g.cnt > 0) && ((h->flags & SUBSET_OPT) || (h->flags & SUBSET__EXCLUDE_OPT)))
					goto subsetclash;

				/* Convert comma-terminated substrings to null-terminated*/
				h->arg.g.cnt = 1;
				h->arg.g.substrs = argv[++i];
				for (p = strchr(h->arg.g.substrs, ',');
					 p != NULL;
					 p = strchr(p, ','))
					{
					*p++ = '\0';
					h->arg.g.cnt++;
					}
				}
			h->flags |= SUBSET_OPT;
			break;
		case opt_gn0:
			if ((h->mode == mode_svg) ||
				((h->mode == mode_cef) && (h->arg.cef.flags & CEF_WRITE_SVG)))
				{
				h->svw.flags &= ~SVW_GLYPHNAMES_MASK;
				h->svw.flags |= SVW_GLYPHNAMES_NONE;
				}
			else
				goto wrongmode;
			break;
		case opt_gn1:
			if ((h->mode == mode_svg) ||
				((h->mode == mode_cef) && (h->arg.cef.flags & CEF_WRITE_SVG)))
				{
				h->svw.flags &= ~SVW_GLYPHNAMES_MASK;
				h->svw.flags |= SVW_GLYPHNAMES_NONASCII;
				}
			else
				goto wrongmode;
			break;
		case opt_gn2:
			if ((h->mode == mode_svg) ||
				((h->mode == mode_cef) && (h->arg.cef.flags & CEF_WRITE_SVG)))
				{
				h->svw.flags &= ~SVW_GLYPHNAMES_MASK;
				h->svw.flags |= SVW_GLYPHNAMES_ALL;
				}
			else
				goto wrongmode;
			break;
		case opt_abs:
			if ((h->mode == mode_svg) ||
				((h->mode == mode_cef) && (h->arg.cef.flags & CEF_WRITE_SVG)))
				{
				h->svw.flags |= SVW_ABSOLUTE;
				}
			else
				goto wrongmode;
			break;
		case opt_sa:
			if ((h->mode == mode_svg) ||
				((h->mode == mode_cef) && (h->arg.cef.flags & CEF_WRITE_SVG)))
				h->svw.flags |= SVW_STANDALONE;
			else
				goto wrongmode;
			break;
		case opt_N:
			h->flags |= SHOW_NAMES;
			break;
		case opt_p:
			if (!argsleft)
				goto noarg;
			else if (h->flags & SUBSET_OPT)
				goto subsetclash;
			h->arg.p = argv[++i];
			srand(0);
			h->flags |= SUBSET_OPT;
			break;
    case opt_pg:
      h->flags |= PRESERVE_GID;
      break;
		case opt_P:
			if (!argsleft)
				goto noarg;
			else if (h->flags & SUBSET_OPT)
				goto subsetclash;
			h->arg.P = argv[++i];
			seedtime();
			h->flags |= SUBSET_OPT;
			break;
		case opt_U:
			if (!argsleft)
				goto noarg;
			h->arg.U = argv[++i];
			break;

		case opt_UNC:
			h->flags |= NO_UDV_CLAMPING;
			break;

        case opt_fdx:
            if ((h->flags & SUBSET_OPT) || (h->flags & SUBSET__EXCLUDE_OPT))
                goto subsetclash;
            h->flags |= SUBSET__EXCLUDE_OPT;
        case opt_fd:
			if (!argsleft)
				goto noarg;
            else if ( (h->arg.g.cnt > 0) && ((h->flags & SUBSET_OPT) || (h->flags & SUBSET__EXCLUDE_OPT)))
                        goto subsetclash;
			else
				{
                    /* Convert comma-terminated substrings to null-terminated*/
                    char *p;
                    h->arg.g.cnt = 1;
                    h->arg.g.substrs = argv[++i];
                    for (p = strchr(h->arg.g.substrs, ',');
                         p != NULL;
                         p = strchr(p, ','))
					{
                        *p++ = '\0';
                        h->arg.g.cnt++;
					}
				}
			h->flags |= SUBSET_OPT;
            // Parse FD argument.
            parseFDSubset(h);
			break;
		case opt_i:
			if (!argsleft)
				goto noarg;
			h->arg.i = argv[++i];
			break;
		case opt_X:
			h->ttr.flags |= TTR_BOTH_PATHS;
			break;
		case opt_x:
			h->ttr.flags |= TTR_EXACT_PATH;
			break;
		case opt_o:				/* file options */
			if (!argsleft)
				goto noarg;
			dstFileSetName(h, argv[++i]);
			break;
		case opt_f:
			if (!argsleft)
				goto noarg;
			i = doMultiFileSet(h, argc, argv, i + 1);
			break;
		case opt_a:
			if (!argsleft)
				goto noarg;
			h->flags |= AUTO_FILE_FROM_FILE;
			i = doAutoFileSet(h, argc, argv, i + 1);
			h->flags &= ~AUTO_FILE_FROM_FILE;
			break;
		case opt_A:
			if (!argsleft)
				goto noarg;
			h->flags |= AUTO_FILE_FROM_FONT;
			i = doAutoFileSet(h, argc, argv, i + 1);
			h->flags &= ~AUTO_FILE_FROM_FONT;
			break;
		case opt_dd:
			if (!argsleft)
				goto noarg;
			h->file.dd = argv[++i];
			break;
		case opt_sd:
			if (!argsleft)
				goto noarg;
			h->file.sd = argv[++i];
			break;
		case opt_sr:
			if (!argsleft)
				goto noarg;
			h->file.sr = argv[++i];
			break;
		case opt_std:
			h->t1w.flags |= T1W_FORCE_STD_ENCODING;
			h->cfw.flags |= CFW_FORCE_STD_ENCODING;
		case opt_r:
			h->flags |= DUMP_RES;
			break;
		case opt_R:
			h->flags |= DUMP_ASD;
			break;
		case opt_s:
			if (h->script.buf != NULL)
				fatal(h, "nested scripts not allowed (-s)");
			else
				fatal(h, "option must be last (-s)");
		case opt_t:
			h->t1r.flags |= T1R_DUMP_TOKENS;
			break;
		case opt_m:				/* Memory failure simulator */
			if (!argsleft)
				goto noarg;
			else
				{
				char *p = argv[++i];
				char *q;
				long cnt = strtol(p, &q, 0);
				if (*q != '\0')
					goto badarg;
				else if (*p == '-')
					/* Fail on specified call */
					h->failmem.iFail = -cnt;
				else if (cnt == 0)
					/* Report number of calls */
					h->failmem.iFail = FAIL_REPORT;
				else
					{
					/* Fail on random call */
					seedtime();
					h->failmem.iFail = randrange(cnt - 1);
					}
				}
			break;
        case opt_maxs:				/* set max number subrs. */
            if (!argsleft)
                goto noarg;
            else
            {
                char *p;
                char *q;
                p = argv[++i];
                h->cfw.maxNumSubrs = strtol(p, &q, 0);
                if (*q != '\0')
                    goto badarg;
            }
            break;
		case opt_u:
			usage(h);
		case opt_h:
		case opt_h1:
		case opt_h2:
		case opt_h3:
			help(h);
		case opt_v:
			printVersions(h);
		case opt_y:
			h->flags |= EVERY_FONT;
			break;
			}
		}

	if (!(h->flags & DONE_FILE))
		doSingleFileSet(h, "-");
	return;

 wrongmode:
	fatal(h, "wrong mode (%s) for option (%s)", h->modename, arg);
 noarg:
	fatal(h, "no argument for option (%s)", arg);
 badarg:
	fatal(h, "bad arg (%s)", arg);
 subsetclash:
	fatal(h, "options -g, -gx, -p, -P, or -fd are mutually exclusive");
 t1clash:
	fatal(h, "options -pfb or -LWFN may not be used with other options");
 bc_gone:
        fatal(h, "options -bc is no longer supported.");
	}

/* Return tail component of path. */
static char *tail(char *path)
	{
	char *p = strrchr(path, '/');
	if (p == NULL)
		p = strrchr(path, '\\');
	return (p == NULL)? path: p + 1;
	}

/* Initialize local subr info element. */
static void initLocal(void *ctx, long cnt, SubrInfo *info)
	{
	txCtx h = ctx;
	while (cnt--)
		{
		dnaINIT(h->ctx.dna, info->stemcnt, 300, 2000);
		info++;
		}
	}

/* Initialize context. */
static void txNew(txCtx h, char *progname)
	{
	ctlMemoryCallbacks cb;

	h->progname = progname;
	h->flags = 0;
	h->script.buf = NULL;

	h->arg.p = NULL;
	h->arg.P = NULL;
	h->arg.U = NULL;
	h->arg.i = NULL;
	h->arg.g.cnt = 0;
	h->arg.path.level = 0;

	h->src.print_file = 0;
	h->t1r.ctx = NULL;
	h->cfr.ctx = NULL;
	h->ttr.ctx = NULL;
	h->ttr.flags = 0;
    h->cfw.ctx = NULL;
    h->cfw.maxNumSubrs = 0; /* 0 is translated to the MAX_NUMBER_SUBRS defined in the cffWrite module. */
	h->cef.ctx = NULL;
	h->abf.ctx = NULL;
	h->pdw.ctx = NULL;
	h->t1w.ctx = NULL;
    h->svw.ctx = NULL;
    h->svw.flags = 0;
    h->svr.ctx = NULL;
    h->svr.flags = 0;
    h->ufr.ctx = NULL;
    h->ufr.flags = 0;
    h->ufow.ctx = NULL;
    h->ufow.flags = 0;
    h->ufr.altLayerDir = NULL;
	h->ctx.sfr = NULL;

	memInit(h);
	stmInit(h);

	/* Initialize dynarr library */
	cb.ctx 		= h;
	cb.manage 	= safeManage;
	h->ctx.dna	= dnaNew(&cb, DNA_CHECK_ARGS);
	if (h->ctx.dna == NULL)
		fatal(h, "can't init dynarr lib");

	h->failmem.iCall = 0;	/* Reset call index */

	dnaINIT(h->ctx.dna, h->src.glyphs, 256, 768);
	dnaINIT(h->ctx.dna, h->src.exclude, 256, 768);
    dnaINIT(h->ctx.dna, h->src.widths, 256, 768);
    dnaINIT(h->ctx.dna, h->src.streamStack, 10, 10);
	dnaINIT(h->ctx.dna, h->fonts, 1, 10);
	dnaINIT(h->ctx.dna, h->subset.glyphs, 256, 768);
	dnaINIT(h->ctx.dna, h->subset.args, 250, 500);
	dnaINIT(h->ctx.dna, h->res.map, 30, 30);
	dnaINIT(h->ctx.dna, h->res.names, 50, 100);
	dnaINIT(h->ctx.dna, h->asd.entries, 10, 10);
	dnaINIT(h->ctx.dna, h->script.args, 200, 3000);
	dnaINIT(h->ctx.dna, h->cef.subset, 256, 768);
	dnaINIT(h->ctx.dna, h->cef.gnames, 256, 768);
	dnaINIT(h->ctx.dna, h->cef.lookup, 256, 768);
	dnaINIT(h->ctx.dna, h->t1w.gnames, 2000, 80000);
	dnaINIT(h->ctx.dna, h->dcf.global.stemcnt, 300, 2000);
    dnaINIT(h->ctx.dna, h->dcf.local, 1, 15);
    dnaINIT(h->ctx.dna, h->dcf.varRegionInfo, 1, 15);
	h->dcf.local.func = initLocal;
    dnaINIT(h->ctx.dna, h->cmap.encoding, 1, 1);
    dnaINIT(h->ctx.dna, h->fd.fdIndices, 16, 16);
	dnaINIT(h->ctx.dna, h->cmap.segment, 1, 1);
	dnaINIT(h->ctx.dna, h->dcf.glyph, 256, 768);

	setMode(h, mode_dump);

	/* Clear the SEEN_MODE bit after setting the default mode */
	h->flags = 0;
	}

/* Free context. */
static void txFree(txCtx h)
	{
	long i;

	memFree(h, h->script.buf);
	dnaFREE(h->src.glyphs);
	dnaFREE(h->src.exclude);
    dnaFREE(h->src.widths);
    dnaFREE(h->src.streamStack);
	dnaFREE(h->fonts);
	dnaFREE(h->subset.glyphs);
	dnaFREE(h->subset.args);
	dnaFREE(h->res.map);
	dnaFREE(h->res.names);
	dnaFREE(h->asd.entries);
	dnaFREE(h->script.args);
	dnaFREE(h->cef.subset);
	dnaFREE(h->cef.gnames);
	dnaFREE(h->cef.lookup);
	dnaFREE(h->t1w.gnames);
	dnaFREE(h->dcf.global.stemcnt);
	for (i = 0; i < h->dcf.local.size; i++)
		dnaFREE(h->dcf.local.array[i].stemcnt);
    dnaFREE(h->dcf.local);
    dnaFREE(h->dcf.varRegionInfo);

	dnaFREE(h->dcf.glyph);
    dnaFREE(h->cmap.encoding);
    dnaFREE(h->fd.fdIndices);
	dnaFREE(h->cmap.segment);
	if (h->t1r.ctx != NULL)
		t1rFree(h->t1r.ctx);
	cfrFree(h->cfr.ctx);
	ttrFree(h->ttr.ctx);
	cfwFree(h->cfw.ctx);
	cefFree(h->cef.ctx);
	pdwFree(h->pdw.ctx);
	t1wFree(h->t1w.ctx);
	svwFree(h->svw.ctx);
	svrFree(h->svr.ctx);
	ufoFree(h->ufr.ctx);
	ufwFree(h->ufow.ctx);
	sfrFree(h->ctx.sfr);

	stmFree(h, &h->src.stm);
	stmFree(h, &h->dst.stm);
	stmFree(h, &h->cef.src);
	stmFree(h, &h->cef.tmp0);
	stmFree(h, &h->cef.tmp1);
	stmFree(h, &h->t1r.tmp);
	stmFree(h, &h->cfw.tmp);
	stmFree(h, &h->t1w.tmp);
	/* Don't close debug streams because they use stderr */

	dnaFree(h->ctx.dna);
	free(h);
	}

/* Main program. */
int CTL_CDECL main(int argc, char *argv[])
	{
	txCtx h;
	char *progname;
#if PLAT_MAC
	argc = ccommand(&argv);
	(void)__reopen(stdin);	/* Change stdin to binary mode */
#endif /* PLAT_MAC */

#if PLAT_WIN
	/* The Microsoft standard C-Library opens stderr in buffered mode in
	   contravention of the C standard. The following code establishes the
	   correct unbuffered mode */
	(void)setvbuf(stderr, NULL, _IONBF, 0);
#endif /* PLAT_WIN */

	/* Get program name */
	progname = tail(argv[0]);
	--argc;
	++argv;

	/* Allocate program context */
	h = malloc(sizeof(struct txCtx_));
	if (h == NULL)
		{
		fprintf(stderr, "%s: out of memory\n", progname);
		return EXIT_FAILURE;
		}
	memset(h, 0, sizeof(struct txCtx_));

	txNew(h, progname);

	if (argc > 1 && getOptionIndex(argv[argc - 2]) == opt_s)
		{
		/* Option list ends with script option */
		int i;

		/* Copy args preceeding -s */
		for (i = 0; i < argc - 2; i++)
			*dnaNEXT(h->script.args) = argv[i];

		/* Add args from script file */
		addArgs(h, argv[argc - 1]);

		parseArgs(h, h->script.args.cnt, h->script.args.array);
		}
	else
		parseArgs(h, argc, argv);

	if (h->failmem.iFail == FAIL_REPORT)
		{
		fflush(stdout);
		fprintf(stderr, "mem_manage() called %ld times in this run.\n",
				h->failmem.iCall);
		}
	txFree(h);

	return 0;
	}
