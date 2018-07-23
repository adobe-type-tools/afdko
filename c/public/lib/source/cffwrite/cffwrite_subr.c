/* @(#)CM_VerSion subr.c atm08 1.2 16245.eco sum= 61088 atm08.002 */
/* @(#)CM_VerSion subr.c atm07 1.2 16164.eco sum= 34442 atm07.012 */
/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

/* Temporary debug control */
#define TC_DEBUG    0
#define DB_FONT     0   /* Font input charstrings */
#define DB_INFS     0   /* Inferior subr selection */
#define DB_ASSOC    0   /* Associate with subrs */
#define DB_RELNS    0   /* Subr relationship building */
#define DB_SELECT   0   /* Subr selection */
#define DB_GROUPS   0   /* Selected groups */
#define DB_CHARS    0   /* Chars with selected subrs */
#define DB_SUBRS    0   /* Subrs with selected subrs */
#define DB_CALLS    0   /* Call counts */

#define EDGE_HASH_STAT  0   /* Collect edge hash table statistics */

/*
   T2 subroutinizer.

   The idea of the subroutinizer is to reduce the size of a font by replacing all
   repeated charstring code in a font by references to a single instance of that
   code. This idea may be extended to a number of fonts in a FontSet where
   repeated charstring code in different fonts is replaced. The single instance of
   charstring code is stored as a subroutine and is assigned a unique subroutine
   number that may be used to indentify it.

   In CFF technology charstring repeats that occur within a single font or across
   multiple fonts are stored in 2 separate data structures and are known as the
   "local" and "global" subroutines, repectively. Local and global subroutines
   are referenced by calling them with the callsubr or callgsubr operators,
   respectively. These operators take a subroutine number, which serves to
   identify the subroutine, as an argument. The local and global subroutine number
   spaces identify different sets of subroutines, therefore local subroutine
   number 1 is distinct from global subroutine number 1.

   ...

   The most challenging part of the process of subroutinization is finding and
   counting the repeated charstrings. This is achieved by first building a suffix
   CDAWG using the concatenation of all the charstrings from all the fonts as
   input. Subseqeuntly the completed suffix CDAWG is traversed in order to count

   The suffix CDAWG is built as a compact DAWG (CDAWG) using an algorithm described in paper
   "On-Line Construction of Compact Directed Acyclic Word Graphs" (200), S. Inenaga, et. al.
   <http://citeseerx.ist.psu.edu/viewdoc/summary?doi=10.1.1.25.474> which is an extension
   to the compact suffix tree construction algorithm "On-line construction of suffix trees"
   (1995) by Esko Ukkonen <http://www.cs.helsinki.fi/u/ukkonen/SuffixT1withFigs.pdf>

   The original code used a DAWG algorithm described in "Text Algorithms, Maxime Crochemore
   and Wojciech Ryter, OUP" p.113. CDAWG is expected to use less memory than DAWG.

   ...
   regular.saved = count * (length - call - num) - (off + length + ret);
      tail.saved = count * (length - call - num) - (off + length);

   Bytes saved for various counts and lengths.

   Following table calculated with the following parameters:

   callsubr	1
   subr#		1
   offset		2
   return		1
                                    length
       3   4   5   6   7   8   9  10  11  12  13  14  15  16  17  18  19  20
   +------------------------------------------------------------------------
   2|  -4  -3  -2  -1   0   1   2   3   4   5   6   7   8   9  10  11  12  13
   3|  -3  -1   1   3   5   7   9  11  13  15  17  19  21  23  25  27  29  31
   c 4|  -2   1   4   7  10  13  16  19  22  25  28  31  34  37  40  43  46  49
   o 5|  -1   3   7  11  15  19  23  27  31  35  39  43  47  51  55  59  63  67
   u 6|   0   5  10  15  20  25  30  35  40  45  50  55  60  65  70  75  80  85
   n 7|   1   7  13  19  25  31  37  43  49  55  61  67  73  79  85  91  97 103
   t 8|   2   9  16  23  30  37  44  51  58  65  72  79  86  93 100 107 114 121
   9|   3  11  19  27  35  43  51  59  67  75  83  91  99 107 115 123 131 139
   10|   4  13  22  31  40  49  58  67  76  85  94 103 112 121 130 139 148 157
   +------------------------------------------------------------------------

   Following table calculated with the following parameters:

   callsubr	1
   subr#		2
   offset		2
   return		1
                                    length
       3   4   5   6   7   8   9  10  11  12  13  14  15  16  17  18  19  20
   +------------------------------------------------------------------------
   2|  -6  -5  -4  -3  -2  -1   0   1   2   3   4   5   6   7   8   9  10  11
   3|  -6  -4  -2   0   2   4   6   8  10  12  14  16  18  20  22  24  26  28
   c 4|  -6  -3   0   3   6   9  12  15  18  21  24  27  30  33  36  39  42  45
   o 5|  -6  -2   2   6  10  14  18  22  26  30  34  38  42  46  50  54  58  62
   u 6|  -6  -1   4   9  14  19  24  29  34  39  44  49  54  59  64  69  74  79
   n 7|  -6   0   6  12  18  24  30  36  42  48  54  60  66  72  78  84  90  96
   t 8|  -6   1   8  15  22  29  36  43  50  57  64  71  78  85  92  99 106 113
   9|  -6   2  10  18  26  34  42  50  58  66  74  82  90  98 106 114 122 130
   10|  -6   3  12  21  30  39  48  57  66  75  84  93 102 111 120 129 138 147
   +------------------------------------------------------------------------

 */

#include "cffwrite_subr.h"
#include "cffwrite.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dynarr.h"

#define DB_TEST_STRING 0
#if DB_TEST_STRING
static unsigned char *gTestString = (unsigned char *)"Humpty Dumpty sat on a wall.Humpty Dumpty had a great fall.";
#define FONT_CHARS_DATA gTestString
#define OPLEN(h, cstr)   1
#define SEPARATOR    ','
#else
#define FONT_CHARS_DATA font->chars.data
#define OPLEN(h, cstr)   ((h)->opLenCache[*(cstr)] == 0 ? (cstr)[1] : (h)->opLenCache[*(cstr)])
#define SEPARATOR   t2_separator
#endif

#define MAX_NUM_SUBRS		32765L	/* Maximum number of subroutines in one INDEX structure. 64K is valid by the spec, but but teh Google font validation tool OTS rejects fonts with subrs in a subrindex which is  over 32K -3.*/
#define MAX_PREFLIGHT	10   /* Typically large enough */

/* --- Memory management --- */
#define MEM_NEW(g, s)        cfwMemNew(g, s)
#define MEM_FREE(g, p)       cfwMemFree(g, p)

/* ------------------------------- CDAWG data ------------------------------- */
typedef struct Edge_ Edge;
typedef struct Node_ Node;
struct Edge_ {
	unsigned char *label;   /* Pointer to the edge label, or the beginning of the edge string */
	Node *son;          /* Son node. red/black color is encoded as the LSB in this pointer */
	unsigned length;    /* Length of the edge string */
};
struct Node_ {
	Node *suffix;       /* Suffix link */
	Edge *edgeTable;    /* Pointer to the edge table */
	long misc;          /* Initially longest path from root, then subr index */
	unsigned edgeCount; /* Number of edges from this node */
	unsigned edgeTableSize; /* Number of entries allocated in the edge table */
	unsigned short paths;   /* Paths through node; depth for subr trie */
	unsigned short id;  /* Font id */
#define NODE_GLOBAL USHRT_MAX   /* Identifies global subr */
#define NODE_ANY    (NODE_GLOBAL-1)
	short flags;        /* Status flags */
#define NODE_COUNTED    (1 << 15) /* Paths have been counted for this node */
#define NODE_TESTED     (1 << 14) /* Candidacy tested for this node */
#define NODE_FAIL       (1 << 13) /* Node failed candidacy test */
#define NODE_TAIL       (1 << 12) /* Tail subr (terminates with endchar) */
#define NODE_SUBR       (1 << 11) /* Node has subr info (index in misc) */
};

typedef struct NodeLink_ NodeLink;
struct NodeLink_ {
    Node        *node;
    NodeLink    *next;
};
/* ------------------------------- hash table parameters ------------------------------- */

#define EDGE_TABLE_SMALLEST_SPARSE_SIZE 128 /* Double the hash table size even before it becomes full beyond this size */
#define EDGE_TABLE_SIZE_USE_SIMPLE_HASH 16  /* Use a better hash function for edges if the table size is larger than this */

/* ------------------------------- Subr data ------------------------------- */
typedef struct Subr_ Subr;
typedef struct Link_ Link;
struct Subr_ {
	Node *node;             /* Associated node */
	Link *sups;             /* Superior subrs */
	Link *infs;             /* Inferior subrs */
	Subr *next;             /* Next member of social group */
    Subr *output;           /* Link to next subr for match trie output */
	unsigned char *cstr;    /* Charstring */
	unsigned short length;  /* Subr length (original bytes spanned) */
	unsigned short count;   /* Occurance count */
	short deltalen;         /* Delta length */
	short subrnum;          /* Biased subr number */
	short numsize;          /* Size of subr number (1, 2, or 3 bytes) */
	short maskcnt;          /* hint/cntrmask count */
	short misc;             /* subrSaved value/call depth (transient) */
	short flags;            /* Status flags */
#define SUBR_SELECT     (1 << 0)  /* Flags subr selected */
#define SUBR_REJECT     (1 << 1)  /* Flags subr rejected */
#define SUBR_MEMBER     (1 << 2)  /* Flags subr added to social group */
#define SUBR_FUTILE     (1 << 3)  /* Flags subr futile (use count 1 or 0) */
#if DB_CALLS
	short calls;    /* xxx remove */
#endif
};

#define SUBR_MARKED (SUBR_SELECT | SUBR_REJECT) /* Flags if subr marked */

struct Link_                /* Social group link */
{
	Subr *subr;             /* Superior/inferior subr */
	Link *next;             /* Next record */
	unsigned short offset;  /* Offset within superior/inferior */
};

typedef struct              /* Subr call within charstring */
{
	Subr *subr;             /* Inferior subr */
	unsigned short offset;  /* Offset within charstring */
} Call;

typedef dnaDCL(Call, CallList);

typedef struct MemBlk_ MemBlk;
struct MemBlk_          /* Generalized memory block for object allocation */
{
	MemBlk *nextBlk;    /* Next memory block in chain */
	char *array;        /* Object array */
	short iNext;        /* Next free element index */
};

typedef struct {
	MemBlk *head;       /* Allocated block list */
	MemBlk *free;       /* Free block list */
} MemInfo;

#define NODES_PER_BLK   5000
#define LINKS_PER_BLK   1000
#define NODE_LINKS_PER_BLK  500
#define TRIE_NODES_PER_BLK  1000

/* Subroutinization context */
struct subrCtx_ {
	MemInfo nodeBlks;       /* Node blocks */
	MemInfo linkBlks;       /* Relation blocks */
	MemInfo trieNodeBlks;   /* Trie node blocks */
	MemInfo nodeLinkBlks;   /* Node link blocks */

	Node *root;             /* CDAWG root */
	Node *base;             /* CDAWG base */
	dnaDCL(Node *, sinks);  /* CDAWG sinks (one for each font id) */

	Edge baseEdge;          /* dummy edge from base to root */

    Node *trieRoot;         /* Subr match trie root */
    Node *trieParent;       /* parent node used during trie walk */
    NodeLink *trieQueue;    /* node link queue */

	dnaDCL(Subr, subrs);    /* Subr list (all) */
	dnaDCL(Subr *, tmp);    /* Temporary subr list */
	dnaDCL(Subr *, reorder); /* Reordered subrs */
	dnaDCL(Call, calls);    /* Temporary subr call accumulator */
	dnaDCL(Subr *, members); /* Temporary social group member accumulator */
	dnaDCL(Subr *, leaders); /* Social group leaders */
	dnaDCL(char, cstrs);    /* Charstring data accumulator */

	short singleton;        /* Single font in font set */
	short offSize;          /* Subr INDEX offset size */
	short subrStackOvl;     /* Subr stack overflow */

	subr_CSData gsubrs;         /* Global subrs */
	struct {
		long lowLESubrIndex; /* the lowest seen LE reference by GSUBR index The entire GSUBR array is played through, and we copy only those from the low LE num on.*/
		long highLEIndex; /* the lowest seen LE reference by the biased LE iindex. The entire GSUBR array is played through, and we copy only those from the low LE num on.*/
		dnaDCL(Offset, subOffsets); /* Charstring data accumulator */
		dnaDCL(char, cstrs);    /* Charstring data accumulator */
	}
	cube_gsubrs;
	short nFonts;           /* Font count */
	subr_Font *fonts;       /* Font list */

	unsigned char opLenCache[256];  /* Cached values of t2oplen. If 0, the second byte in the charstring indicates the length */
	dnaDCL(Subr *, subrHash); /* Subr hash table */
#define SUBR_PREFIX_MAP_SIZE ((1 << (2 * 8)) / 8)
#define SUBR_PREFIX_MAP_INDEX(b1, b2) ((((unsigned)(b1)) << 5) | ((b2) >> 3))
#define SUBR_PREFIX_MAP_BIT(b2) (1 << ((b2) & 7))
#define TEST_SUBR_PREFIX_MAP(ctx, str) ((ctx)->subrPrefixMap[SUBR_PREFIX_MAP_INDEX(str[0], str[1])] & SUBR_PREFIX_MAP_BIT(str[1]))
#define SET_SUBR_PREFIX_MAP(ctx, str) ((ctx)->subrPrefixMap[SUBR_PREFIX_MAP_INDEX(str[0], str[1])] |= SUBR_PREFIX_MAP_BIT(str[1]))
	unsigned char subrPrefixMap[SUBR_PREFIX_MAP_SIZE];  /* bit table where a bit is set when its corresponding subr 2-byte prefix selected */
	dnaDCL(unsigned short, subrLenMap); /* boolean table where a value is set when any subr with the corresponding length is selected */
	dnaDCL(unsigned short, prefixLen); /* Prefix byte length for each byte in a charstring */
	unsigned maxSubrLen;    /* Maximum subr length */
	unsigned minSubrLen;    /* Minimum subr lenth */

	unsigned long maxNumSubrs;       /* Maximum number of subroutines (0 means default MAX_NUM_SUBRS) */

	cfwCtx g;               /* Package context */
};

#define CALL_OP_SIZE    1       /* Size of call(g)subr (bytes) */

/* xxx This copy of the module context make this module nonreentrant. It is
   required because the ANSI qsort() function doesn't allow a client to pass
   anything except array elements to the comparison routine. When I have time I
   will write one that does. */
static subrCtx ctx;

#if TC_DEBUG
static long dbnodeid(subrCtx h, Node *node);
static void dbop(int length, unsigned char *cstr);
static void dbsubr(subrCtx h, unsigned iSubr, int c, unsigned offset);
static void dbnode(subrCtx h, Node *node);
static void dbcstr(subrCtx h, unsigned length, unsigned char *cstr);
static void dbcstrs(subrCtx h,
                    unsigned char *cstr, unsigned char *end, int index);
static void dbgroups(subrCtx h);

#endif /* TC_DEBUG */

/* --------------------------- Type 2 charstring functions --------------------------- */

/* Return operator length from opcode */
static int t2oplen(unsigned char *cstr) {
	switch (cstr[0]) {
		default:
			return 1;

		case tx_escape:
		case 247:
		case 248:
		case 249:
		case 250:
		case 251:
		case 252:
		case 253:
		case 254:
			return 2;

		case t2_shortint:
			return 3;

		case t2_separator:
			return 4;

		case 255:
			return 5;

		case t2_hintmask:
		case t2_cntrmask:
			return cstr[1];
	}
}

/* Copy and edit cstr by removing length bytes from mask operators. Return
   advanced destination buffer pointer */
static unsigned char *t2cstrcpy(unsigned char *pdst, unsigned char *psrc,
                                unsigned length) {
	int left;
	unsigned char *pend = psrc + length;

	while (psrc < pend) {
		switch (*psrc) {
			case t2_hintmask:
			case t2_cntrmask:           /* Mask ops; remove length byte */
				*pdst++ = *psrc++;
				left = *psrc++ - 2;
				while (left--) {
					*pdst++ = *psrc++;
				}
				length--;
				break;

			case 255:                   /* 5-byte number */
				*pdst++ = *psrc++;
				*pdst++ = *psrc++;
			/* Fall through */

			case t2_shortint:           /* 3-byte number */
				*pdst++ = *psrc++;
			/* Fall through */

			case tx_escape:             /* 2-byte number/esc operator */
			case 247:
			case 248:
			case 249:
			case 250:
			case 251:
			case 252:
			case 253:
			case 254:
				*pdst++ = *psrc++;
			/* Fall through */

			default:                    /* 1-byte number/operator */
				*pdst++ = *psrc++;
				break;
		}
	}

	return pdst;
}

/* --------------------------- Object Management --------------------------- */

/* Return new memory block based object */
static void *newObject(subrCtx h, MemInfo *info, long size, long count) {
	MemBlk *pblk = info->head;
	if (pblk == NULL || pblk->iNext == count) {
		/* Re/allocate new block */
		MemBlk *_new;
		if (info->free != NULL) {
			/* Remove block from free list */
			_new = info->free;
			info->free = _new->nextBlk;
		}
		else {
			/* Allocate new block from heap */
			_new = (MemBlk *)MEM_NEW(h->g, sizeof(MemBlk));
			_new->array = (char*)MEM_NEW(h->g,  size * count);
		}
		_new->nextBlk = pblk;
		_new->iNext = 0;
		info->head = pblk = _new;
	}
	/* Return next object */
	return &pblk->array[pblk->iNext++ *size];
}

/* Copy objects to free list */
static void reuseObjects(cfwCtx g, MemInfo *info) {
	MemBlk *pblk;
	MemBlk *next;
	for (pblk = info->head; pblk != NULL; pblk = next) {
		next = pblk->nextBlk;
		pblk->nextBlk = info->free;
		pblk->iNext = 0;
		info->free = pblk;
	}
	info->head = NULL;
}

/* Free objects and memory blocks */
static void freeObjects(cfwCtx g, MemInfo *info) {
	MemBlk *pblk;
	MemBlk *next;
	for (pblk = info->free; pblk != NULL; pblk = next) {
		next = pblk->nextBlk;
		MEM_FREE(g, pblk->array);
		MEM_FREE(g, pblk);
	}
	info->free = NULL;
}

/* Create and initialize new CDAWG node */
static Node *newNode(subrCtx h, long length, unsigned id) {
	Node *node = (Node *)newObject(h, &h->nodeBlks, sizeof(Node), NODES_PER_BLK);
	node->edgeTable = NULL;
	node->edgeCount = 0;
	node->edgeTableSize = 0;
	node->misc = length;
	node->paths = 0;
	node->id = (unsigned short)id;
	node->flags = 0;
	return node;
}

/* Create and initialize new subr link */
static Link *newLink(subrCtx h, Subr *subr, unsigned offset, Link *next) {
	Link *link = (Link *)newObject(h, &h->linkBlks, sizeof(Link), LINKS_PER_BLK);
	link->subr = subr;
	link->next = next;
	link->offset = (unsigned short)offset;
	return link;
}

/* Create and initialize new trie node */
static Node *newTrieNode(subrCtx h, long depth) {
	Node *node = (Node *)newObject(h, &h->trieNodeBlks, sizeof(Node), TRIE_NODES_PER_BLK);
	node->edgeTable = NULL;
	node->edgeCount = 0;
	node->edgeTableSize = 0;
    node->suffix = NULL;    /* suffix link */
	node->misc = -1;        /* subr index */
	node->paths = (unsigned short)depth;    /* debug only */
	node->id = 0;
	node->flags = 0;
	return node;
}

/* Create and initialize new node link */
static NodeLink *newNodeLink(subrCtx h, Node *node, NodeLink *next) {
	NodeLink *link = (NodeLink *)newObject(h, &h->nodeLinkBlks, sizeof(NodeLink), NODE_LINKS_PER_BLK);
	link->node = node;
	link->next = next;
	return link;
}

/* --------------------------- Context Management -------------------------- */

/* Initialize module */
void cfwSubrNew(cfwCtx g) {
	subrCtx h = (subrCtx)MEM_NEW(g, sizeof(struct subrCtx_));

	h->nodeBlks.head = h->nodeBlks.free = NULL;
	h->linkBlks.head = h->linkBlks.free = NULL;
	h->trieNodeBlks.head = h->trieNodeBlks.free = NULL;
	h->nodeLinkBlks.head = h->nodeLinkBlks.free = NULL;

	h->root = NULL;
	h->base = NULL;

    h->trieRoot = NULL;
    h->trieQueue = NULL;
    h->maxNumSubrs = 0;
    
	/* xxx tune these parameters */
	dnaINIT(g->ctx.dnaSafe, h->subrs, 500, 1000);
	dnaINIT(g->ctx.dnaSafe, h->tmp, 500, 1000);
	dnaINIT(g->ctx.dnaSafe, h->reorder, 500, 1000);
	dnaINIT(g->ctx.dnaSafe, h->calls, 10, 10);
	dnaINIT(g->ctx.dnaSafe, h->members, 40, 40);
	dnaINIT(g->ctx.dnaSafe, h->leaders, 100, 200);
	dnaINIT(g->ctx.dnaSafe, h->cstrs, 5000, 2000);
	dnaINIT(g->ctx.dnaSafe, h->sinks, 1, 1);
	dnaINIT(g->ctx.dnaSafe, h->subrHash, 0, 1);
	dnaINIT(g->ctx.dnaSafe, h->prefixLen, 10, 10);
	dnaINIT(g->ctx.dnaSafe, h->subrLenMap, 0, 1);

	h->offSize = 2;

	h->cube_gsubrs.highLEIndex = -1000;
	h->cube_gsubrs.lowLESubrIndex = 1000;
	dnaINIT(g->ctx.dnaSafe, h->cube_gsubrs.subOffsets, 214, 10);
	dnaINIT(g->ctx.dnaSafe, h->cube_gsubrs.cstrs, 1028, 1028);

	h->gsubrs.nStrings = 0;
	h->gsubrs.offset = NULL;

	ctx = h;    /* xxx temporary */

	/* Link contexts */
	h->g = g;
	g->ctx.subr = h;
}

static void freeEdges(subrCtx h, MemInfo *info);

/* Free charstring data */
static void csFreeData(cfwCtx g, subr_CSData *data) {
	if (data->nStrings != 0) {
		MEM_FREE(g, data->offset);
		MEM_FREE(g, data->data);
		data->nStrings = 0;
	}
}

/* Prepare module for reuse */
void cfwSubrReuse(cfwCtx g) {
	subrCtx h = g->ctx.subr;

	freeEdges(h, &h->nodeBlks);
	freeEdges(h, &h->trieNodeBlks);
	dnaSET_CNT(h->sinks, 0);
	dnaSET_CNT(h->subrHash, 0);

	reuseObjects(g, &h->nodeBlks);
	reuseObjects(g, &h->linkBlks);
	reuseObjects(g, &h->trieNodeBlks);
	reuseObjects(g, &h->nodeLinkBlks);

	csFreeData(g, &h->gsubrs);

	dnaSET_CNT(h->cube_gsubrs.subOffsets, 0);
	dnaSET_CNT(h->cube_gsubrs.cstrs, 0);

	h->root = NULL;
	h->offSize = 2;
}

/* Free resources */
void cfwSubrFree(cfwCtx g) {
	subrCtx h = g->ctx.subr;

	if (h == NULL)
		return;

	freeEdges(h, &h->nodeBlks);
	freeEdges(h, &h->trieNodeBlks);

	freeObjects(g, &h->nodeBlks);
	freeObjects(g, &h->linkBlks);
	freeObjects(g, &h->trieNodeBlks);
	freeObjects(g, &h->nodeLinkBlks);

	dnaFREE(h->subrs);
	dnaFREE(h->tmp);
	dnaFREE(h->reorder);
	dnaFREE(h->calls);
	dnaFREE(h->members);
	dnaFREE(h->leaders);
	dnaFREE(h->cstrs);
	dnaFREE(h->sinks);
	dnaFREE(h->subrHash);
	dnaFREE(h->prefixLen);
	dnaFREE(h->subrLenMap);
	dnaFREE(h->cube_gsubrs.subOffsets);
	dnaFREE(h->cube_gsubrs.cstrs);

	MEM_FREE(g, h);
}

/* --------------------------- Edge Table -------------------------- */

/* Allocate the initial edge table for a given node */
static void newEdgeTable(cfwCtx g, Node *node, unsigned size) {
	unsigned long byteSize = sizeof(Edge) * size;
	node->edgeTableSize = size;
	node->edgeCount = 0;
	node->edgeTable = (Edge *)MEM_NEW(g, byteSize);
	memset(node->edgeTable, 0, sizeof(Edge) * size);
}

/* Initialize new CDAWG edge */
static void initEdge(Edge *edge, unsigned char *label, unsigned edgeLength, Node *son) {
	edge->label = label;
	edge->length = edgeLength;
	edge->son = son;
}

/* free edge tables allocated for all nodes */
static void freeEdges(subrCtx h, MemInfo *info) {
	MemBlk *pblk = info->head;
	while (pblk) {
		short i;
		Node *node;
		for (i = 0, node = (Node *)&pblk->array[0]; i < pblk->iNext; i++, node++) {
			if (node->edgeTable) {
				MEM_FREE(h->g, node->edgeTable);
				node->edgeTable = NULL;
			}
		}
		pblk = pblk->nextBlk;
	}
}

/* --------------------------- CDAWG Construction --------------------------- */
/* Compare two edge labels */
static int labelcmp(subrCtx h,
                    int length1, unsigned char *label1, unsigned char *label2) {
	int cmp = *label1++ - *label2;
	if (cmp != 0) {
		return cmp; /* First byte differs */
	}
	else {
		int length2 = OPLEN(h, label2);
		int length = (length1 < length2) ? length1 : length2;

		label2++;
		while (--length) {
			cmp = *label1++ - *label2++;
			if (cmp != 0) {
				return cmp; /* Subsequent byte differs */
			}
		}

		/* Equal up to shortest label */
		return length1 - length2;
	}
}

/* Calculate a hash value for a given edge; this simple formula appears good enough */
static unsigned hashLabel(unsigned char *label, unsigned length) {
	unsigned long hash = *label;

	hash += (hash << 5);
	while (--length > 0) {
		unsigned long temp = *++label;
		hash += temp;
		hash <<= 5;
		hash += temp;
	}
	return hash;
}

#if EDGE_HASH_STAT
static unsigned long gTotalEdgeTableSize;
static unsigned long gTotalEdgeCount;
static unsigned long long gTotalEdgeLookupCount;
static unsigned long gTotalEdgeMissCount;
#endif

/* Look up the edge table as a hash table for a given edge label
   returns a ponter to an edge entry which may be empty if not found */
static Edge *lookupEdgeTable(subrCtx h, Node *node, unsigned length, unsigned char *label) {
	unsigned tableSize = node->edgeTableSize;
	unsigned tableSizeMinus1 = tableSize - 1;
	unsigned hashValue = (tableSize <= EDGE_TABLE_SIZE_USE_SIMPLE_HASH) ? (*label + length) : hashLabel(label, length);
	unsigned hashIncrement = 0;
	unsigned count = 0;
#if EDGE_HASH_STAT
	gTotalEdgeLookupCount++;
	gTotalEdgeTableSize += tableSize;
	gTotalEdgeCount += node->edgeCount;
#endif

	while (count < tableSize) {
		Edge *edge = &node->edgeTable[hashValue & tableSizeMinus1]; /* (hashValue % node->edgeTableSize) */
		if (edge->label == NULL) {
			return edge;
		}

		if (labelcmp(h, length, label, edge->label) == 0) {
			return edge;
		}

		/* Collided with a wrong entry.
		   Update the hash value using the quadratic probing algorithm and try again */
		hashValue += ++hashIncrement;
		count++;
#if EDGE_HASH_STAT
		gTotalEdgeMissCount++;
#endif
	}

	/* couldn't find an entry (and the table was full) */
	return NULL;
}

static void doubleEdgeTable(subrCtx h, Node *node);

/* Enter a new edge into the edge table represented as a hash table */
static void addEdgeToHashTable(subrCtx h, Node *node, Node *son,
                               unsigned length, unsigned char *label, unsigned edgeLength) {
	int doubleIt = 0;
	Edge *edge;

	if (node->edgeTable == NULL) {
		/* The initial edge table starts out with only one entry */
		newEdgeTable(h->g, node, 1);
		edge = &node->edgeTable[0];
	}
	else {
		/* Double the hash table if the large table is almost full or no empty slot available */
		if (node->edgeCount >= node->edgeTableSize) {
			doubleIt = 1;
		}
		else if (node->edgeTableSize >= EDGE_TABLE_SMALLEST_SPARSE_SIZE) {
			if (node->edgeCount >= (node->edgeTableSize - (node->edgeTableSize >> 3))) {
				/* When the hash table is >= 87.5% full, double its size */
				doubleIt = 1;
			}
		}

		if (doubleIt) {
			doubleEdgeTable(h, node);
		}

		edge = lookupEdgeTable(h, node, length, label);
	}
#if TC_DEBUG
	if (!edge || edge->label) {
		printf("addEdgeToHashTable: failed to find an empty slot\n");
	}
#endif
	initEdge(edge, label, edgeLength, son);
	node->edgeCount++;
}

/* Double the size of the edge table for a given node */
static void doubleEdgeTable(subrCtx h, Node *node) {
	Edge *oldTable = node->edgeTable;
	unsigned oldTableSize = node->edgeTableSize;
	unsigned newTableSize = node->edgeTableSize * 2;
	unsigned newTableByteSize = sizeof(Edge) * newTableSize;
	Edge *newTable = (Edge *)MEM_NEW(h->g, newTableByteSize);
	Edge *edge;
	unsigned i;

	/* Replace the old hash table with a new blank hash table */
	memset(newTable, 0, newTableByteSize);
	node->edgeTable = newTable;
	node->edgeTableSize = newTableSize;
	node->edgeCount = 0;

	/* Copy all edges from the old hash table to the new hash table */
	for (i = 0, edge = oldTable; i < oldTableSize; i++, edge++) {
		if (edge->label != NULL) {
			addEdgeToHashTable(h, node, edge->son, OPLEN(h, edge->label), edge->label, edge->length);
		}
	}

	MEM_FREE(h->g, oldTable);
}

/* Add edge to between father and son nodes */
static void addEdge(subrCtx h, Node *father, Node *son,
                    unsigned length, unsigned char *label, unsigned edgeLength) {
	addEdgeToHashTable(h, father, son, length, label, edgeLength);
}

/* handle the special case where the base node has a virtual edge for every token to the root node */
#define FIND_EDGE(h, node, length, label)   ((node)->misc == -1 ? & (h)->baseEdge : findEdge(h, node, length, label))

/* Find linking edge from node with label */
static Edge *findEdge(subrCtx h,
                      Node *node, unsigned length, unsigned char *label) {
	Edge *edge = lookupEdgeTable(h, node, length, label);
	if (edge == NULL || edge->label) {
		return edge;
	}
	else {
		return NULL;
	}
}

/* Copy the edge table from the source node to the destination node */
static void copyEdgeTable(subrCtx h, Node *destNode, Node *srcNode) {
	newEdgeTable(h->g, destNode, srcNode->edgeTableSize);
	destNode->edgeCount = srcNode->edgeCount;
	memcpy(destNode->edgeTable, srcNode->edgeTable, sizeof(Edge) * srcNode->edgeTableSize);
}

typedef void (*walkEdgeTableProc)(subrCtx h, Edge *edge, long param1, long param2);

static void walkEdgeTable(subrCtx h, Node *node, walkEdgeTableProc proc, long param1, long param2) {
	unsigned size = node->edgeTableSize;
	Edge *edge = node->edgeTable;
	unsigned i;

	if (!edge) {
		return;
	}

	for (i = 0; i < size; i++, edge++) {
		if (edge->label) {
			proc(h, edge, param1, param2);
		}
	}
}

/* Determine whether the state with the canonical reference pair (s,(k,p)) is the end point */
static int CheckEndPoint(subrCtx h, Node *s, unsigned char *k, unsigned char *p,
                         unsigned length, unsigned id) {
	/* c == *p */
	if (s->misc == -1) {
		/* base node is always an end point */
		return 1;
	}

	if (k < p) {
		/* implicit node */
		/* let s (k',p')-> s' be the text[k]-edge from s;
		   return (c == text[k' + p - k]) */
		Edge *edge = FIND_EDGE(h, s, OPLEN(h, k), k);
		return labelcmp(h, length, p, edge->label + (p - k)) == 0;
	}
	else {
		/* explicit node */
		/* is there 'c'-edge from s? */
		return (FIND_EDGE(h, s, length, p) != NULL);
	}
}

/* Return either an explicit/implicit node corresponding to the canonical reference pair (s,(k,p)) */
static Node *Extension(subrCtx h, Node *s, unsigned char *k, unsigned char *p) {
	if (k >= p) {
		/* explicit node */
		return s;
	}
	else {
		/* implicit node */
		/* let s (k',p')-> s' be the text[k]-edge from s */
		return FIND_EDGE(h, s, OPLEN(h, k), k)->son;
	}
}

/* Redirect the edge (s,(k,p)) to r */
static void Redirect(subrCtx h, Node *s, unsigned char *k, unsigned char *p, Node *r) {
	/* let s (k',p')-> s' be the text[k]-edge from s */
	Edge *edge = FIND_EDGE(h, s, OPLEN(h, k), k);

	/* replace this edge by edge s (k',k'+p-k)-> r;
	   note that the edge has the same label so it can be replaced in place
	   within the edge tree */
	edge->son = r;
	edge->length = (unsigned int)(p - k);
}

#define CANONIZE(h, s, k, p, s_ret, k_ret)  \
	{   if ((k) < (p)) { Canonize(h, s, k, p, s_ret, k_ret);     /* implicit node */ \
		} \
		else { *(s_ret) = (s); *(k_ret) = (k); }    /* explicit node */ \
	}

/* Canonize (normalize) a reference pair (s,(k,p)) of an implicit node and return it as (s',k') */
static void Canonize(subrCtx h, Node *s, unsigned char *k, unsigned char *p, Node **s_ret, unsigned char **k_ret) {
	Edge *edge;
	int length = OPLEN(h, k);

	if (s->misc == -1) {
		/* base node has a one-length edge to root for every token */
		s = h->root;
		k += length;
		if (k >= p) {
			/* explicit node */
			*s_ret = s;
			*k_ret = k;
			return;
		}
		length = OPLEN(h, k);
	}
	/* (s,(k,p)) is an implicit node */
	/* find the text[k]-edge s (k',p') -> s' from s */
	edge = FIND_EDGE(h, s, length, k);
	while (edge->length <= (unsigned)(p - k)) {
		k += edge->length;
		s = edge->son;
		if (k < p) {
			/* find the text[k]-edge s (k',p')-> s' from s */
			edge = FIND_EDGE(h, s,  OPLEN(h, k), k);
		}
	}
	*s_ret = s;
	*k_ret = k;
}

/* Split an edge at the canonical reference point (s,(k,p)) and returns the split point as an explicit node */
static Node *SplitEdge(subrCtx h, Node *s, unsigned char *k, unsigned char *p, int id) {
	Node *r;
	unsigned char *newLabel;
	/* let s (k',p') -> s' be the text[k]-edge from s */
	Edge *edge = FIND_EDGE(h, s,  OPLEN(h, k), k);
	/* replace this edge by edges s (k',k'+p-k) -> r and r (k'+p-k+1,p') -> s',
	   where r is a new node */
	r = newNode(h, s->misc + (long)(p - k), (edge->son->id != id) ? NODE_GLOBAL : id);
	newLabel = edge->label + (p - k);
	addEdge(h, r, edge->son, OPLEN(h, newLabel), newLabel, (unsigned int)((edge->label + edge->length) - newLabel));
	edge->length = (unsigned int)(p - k);
	edge->son = r;

	return r;
}

/* If a node at the canonical reference point (s,(k,p)) is a non-solid, explicit node,
   then duplicate the node and return a new active point */
static void SeparateNode(subrCtx h, Node *s, unsigned char *k, unsigned char *p,
                         Node **s_ret, unsigned char **k_ret, int id) {
	Node *ss;
	Node *rr;
	unsigned char *kk;
	unsigned char *pminus1;
	Node *cs;
	unsigned char *ck;
	CANONIZE(h, s, k, p, &ss, &kk);
	if (kk < p) {
		/* implicit node */
		*s_ret = ss;
		*k_ret = kk;
		return;
	}
	/* (s',(k',p)) is an explicit node */
	if (s->misc == -1 /*base node*/) {
		*s_ret = ss;
		*k_ret = kk;
		return;
	}
	if (ss->misc == s->misc + (p - k)) {
		/* solid edge */
		Node *suffix;
		for (suffix = ss; suffix != NULL; suffix = suffix->suffix) {
			if (suffix->id != id) {
				suffix->id = NODE_GLOBAL;
			}
		}
		*s_ret = ss;
		*k_ret = kk;
		return;
	}

	/* non-solid case */
	/* create a new node r' as a duplication of s' */
	rr = newNode(h, s->misc + (long)(p - k), (ss->id != id) ? NODE_GLOBAL : id);
	/* Copy the edge table */
	copyEdgeTable(h, rr, ss);
	/* set up suffix link */
	rr->suffix = ss->suffix;
	ss->suffix = rr;

	do {
		/* replace the text[k]-edge from s to s' by edge s (k,p)-> r' */
		Node *son;
		Edge *edge = FIND_EDGE(h, s, OPLEN(h, k), k);
		ss = edge->son;
		edge->son = rr;
		edge->label = k;
		edge->length = (unsigned int)(p - k);

		/* calculate a canonical reference for Suf(s) with edge (k,p-1);
		   (s, k) := Canonize(Suf(s),(k,p-1))
		   that is, we need to back up from (k,p) by one token. Since we can't
		   parse a charstring backwards, we parse from k forward to find p-1.
		   this code may need optimization since it is O(n^2) */
		pminus1 = k;
		for (;; ) {
			int length = OPLEN(h, pminus1);
			if (pminus1 + length >= p) {
				break;
			}
			pminus1 += length;
		}
		CANONIZE(h, s->suffix, k, pminus1, &s, &k);
		if (k < p) {
			/* implicit node */
			son = FIND_EDGE(h, s, OPLEN(h, k), k)->son;
		}
		else {
			son = s;
		}
		if (son->id != id) {
			son->id = NODE_GLOBAL;
		}

		/* do {..} until (s',k') != Canonize(s,(k,p)) */
		CANONIZE(h, s, k, p, &cs, &ck);
	}
	while ((ss == cs) && (kk == ck));

	/* return (r',p) */
	*s_ret = rr;
	*k_ret = p;
}

/* Append font's charstring data to CDAWG. This construction algorithm closely
   follows the one presented in "On-Line Construction of Compact Directed Acyclic
   Word Graphs" although this one also adds code the identify
   nodes in the CDAWG with a particular font or as global nodes if they
   terminate charstrings that appear in multiple fonts.
   A notational difference is that an edge is represented as (k,p) where
   p points at the next token after the end of the string as opposed to
   p pointing at the last token in the string.
 */
static void addFont(subrCtx h, subr_Font *font, unsigned iFont, int multiFonts) {
	unsigned char *p;
	unsigned char *pend;
	unsigned char *pfd;
	unsigned id;
	Node *s;    /* active point */
	unsigned char *k;   /* beginning of the current reference point */
	Node *e;    /* extention node */
	Node *r, *oldr;

	if (font->chars.nStrings == 0) {
		return; /* Synthetic font */
	}
#if DB_TEST_STRING
	p = gTestString;
	pend = p + strlen((char *)p);
	multiFonts = 1;
#else
	p = (unsigned char *)font->chars.data;
	pend = p + font->chars.offset[font->chars.nStrings - 1];
#endif

	if (font->flags & SUBR_FONT_CID) {
		pfd = font->fdIndex;
		id = iFont + *pfd++;
	}
	else {
		pfd = NULL; /* Suppress optimizer warning */
		id = iFont;
	}

	if (h->base == NULL) {
		h->base = newNode(h, -1, id);
		h->base->misc = -1; /* length from source */
		h->base->suffix = NULL;
	}

	if (h->root == NULL) {
		h->root = newNode(h, 0, id);
		h->root->suffix = h->base;
	}

	/* set up base edge */
	h->baseEdge.son = h->root;

	s = h->root;
	k = p;

#if DB_FONT
	dbcstrs(h, p, pend, iFont);
#endif

	/* Extend path (token by token) */
	while (p < pend) {
		int length = OPLEN(h, p);

		r = NULL;
		oldr = NULL;
		e = NULL;

		/* Update at (s,(k,p)) which is the canonical reference pair for the active point. */
		while (!CheckEndPoint(h, s, k, p, length, id)) {
			Node *sink;
			if (k < p) {
				/* implicit */
				Node *newe = Extension(h, s, k, p);
				if (newe == e) {
					Redirect(h, s, k, p, r);
					CANONIZE(h, s->suffix, k, p, &s, &k);
					continue;
				}
				else {
					e = newe;
					r = SplitEdge(h, s, k, p, id);
				}
			}
			else {
				/* explicit */
				r = s;
			}

			/* create p new edge r (p,infinity) -> sink */
			/* we furnish one sink for each font */
			if (id >= (unsigned)h->sinks.cnt) {
				long cnt = h->sinks.cnt;
				dnaSET_CNT(h->sinks, id + 1);
				while (cnt < h->sinks.cnt) {
					h->sinks.array[cnt++] = NULL;
				}
			}
			sink = h->sinks.array[id];
			if (!sink) {
				sink = h->sinks.array[id] = newNode(h, 0, id);
				sink->flags |= NODE_COUNTED;
				sink->paths = 1;
			}

			addEdge(h, r, sink, length, p, (unsigned int)(pend - p));

			if (oldr != NULL) {
				oldr->suffix = r;
			}
			oldr = r;
			CANONIZE(h, s->suffix, k, p, &s, &k);
		}

		if (oldr != NULL) {
			oldr->suffix = s;
		}

		/* Even though we reached an end point, we need to follow the suffix link
		   in order to mark shared nodes as NODE_GLOBAL in the case of multi-fonts. */
		if (multiFonts) {
			Node *ss = s;
			unsigned char *kk = k;
			unsigned char *pp = p + length;
			while (ss->misc != -1) {
				CANONIZE(h, ss->suffix, kk, pp, &ss, &kk);
				if (kk >= pp) {
					/* explicit */
					if (ss->id != id) {
						ss->id = NODE_GLOBAL;
					}
				}
			}
		}

		SeparateNode(h, s, k, p + length, &s, &k, id);

#if DB_TEST_STRING
		if (p[0] == SEPARATOR) {
			id++;
		}
#else
		if (font->flags & SUBR_FONT_CID &&
		    p[0] == SEPARATOR &&
		    p + length < pend) {
			/* Change id for CID font on charstring boundary */
			id = iFont + *pfd++;
		}
#endif

		p += length;
	}
}

/* ----------------------- Candidate Subr Selection ------------------------ */

static long countPathsForNode(subrCtx h, Node *node);

/* Count paths running through each node */
static unsigned countPaths(subrCtx h, Edge *edge) {
	Node *node;

	if (edge == NULL) {
		return 0;
	}
	node = edge->son;

	if (!(node->flags & NODE_COUNTED)) {
		/* Count descendent paths */
		long count = node->paths;

		/* Recursively descend complex node */
		count += countPathsForNode(h, node);

		/* Update node */
		node->paths = (unsigned short)((count > USHRT_MAX) ? USHRT_MAX : count);
		node->flags |= NODE_COUNTED;
	}
	return node->paths;
}

static long countPathsForNode(subrCtx h, Node *node) {
	long count = 0;
	unsigned i, tableSize;
	Edge *edge = node->edgeTable;

	tableSize = node->edgeTableSize;
	for (i = 0; i < tableSize; i++, edge++) {
		if (edge->label) {
			count += countPaths(h, edge);
		}
	}
	return count;
}

/* Test candidate subr and save if it meets candidate requirements.

   This function selects all candidate subrs using a subr number size of 1 and
   Subr INDEX offset element size of 2. The actual sizes will be used later
   when accepting or rejecting subrs from the initial candidate list. The
   savings available are as follows:

   call = 1     callsubr op size
   num = 1      subr number size
   off = 2      subr INDEX offset element size
   ret = 1      return op size

              Regular        Tail
   length   count saved   count saved
   ------   ----- -----   ----- -----
      3       7     1       6     1
      4       4     1       4     2
      5       3     1       3     2
      6       3     3       3     4
      7       3     5       2     1
      8       2     1       2     2
     9-       2    >0       2    >0
 */
static void saveSubr(subrCtx h, unsigned char *edgeEnd, Node *node,
                     int maskcnt, int tail, long subrLen) {
	Subr *subr;
	unsigned count = node->paths;

	node->flags |= NODE_FAIL;   /* Assume test will fail and mark node */

	/* Test for candidacy */
	switch (subrLen - maskcnt) {
		case 1:
		case 2:
			return;

		case 3:
			if (count < (7 - (unsigned)tail)) {
				return;
			}
			break;

		case 4:
			if (count < 4) {
				return;
			}
			break;

		case 5:
		case 6:
			if (count < 3) {
				return;
			}
			break;

		case 7:
			if (count < (3 - (unsigned)tail)) {
				return;
			}
			break;

		default:
			if (count < 2) {
				return;
			}
			break;
	}

	node->flags &= ~NODE_FAIL;  /* Test passed; removed mark */

	/* Select as candidate subr */
	subr = dnaNEXT(h->subrs);
	subr->node = node;
	subr->sups = NULL;
	subr->infs = NULL;
	subr->next = NULL;
    subr->output = NULL;
	subr->cstr = edgeEnd - subrLen;
	subr->length = (unsigned short)subrLen;
	subr->count = (unsigned short)count;
	subr->deltalen = 0;
	subr->numsize = 1;
	subr->maskcnt = (short)maskcnt;
	subr->flags = 0;
#if DB_CALLS
	subr->calls = 0;
#endif

	/* Update subr node */
	node->misc = h->subrs.cnt - 1;
	node->flags |= NODE_SUBR;
	if (tail) {
		node->flags |= NODE_TAIL;
	}
}

static void findCandSubrs(subrCtx h, Edge *edge, int maskcnt);

static void findCandSubrsProc(subrCtx h, Edge *edge, long maskcnt, long misc) {
	if ((long)(misc + edge->length) == edge->son->misc) {
		/* Descend solid edge */
		findCandSubrs(h, edge, maskcnt);
	}
}

/* Find candidate subrs in CDAWG */
static void findCandSubrs(subrCtx h, Edge *edge, int maskcnt) {
	long misc;
	Node *node;
	unsigned char *edgeEnd;

	for (;; ) {
		unsigned char *pstr;
		node = edge->son;

		if (node->flags & NODE_TESTED || node->paths == 1) {
			return;
		}
		node->flags |= NODE_TESTED;

		/* scan the edge string for masks and endchar */
		pstr = edge->label;
		edgeEnd = pstr + edge->length;
		while (pstr < edgeEnd) {
			int oplen = OPLEN(h, pstr);
			if (*pstr == tx_endchar) {
				if (node->paths > 1) {
					pstr += oplen;
					saveSubr(h, pstr, node, maskcnt, 1, (long)(node->misc - ((edge->label + edge->length) - pstr)));
				}
				return;
			}
			else if (*pstr == t2_hintmask ||
			         *pstr == t2_cntrmask) {
				maskcnt++;
			}
			pstr += oplen;
		}

		misc = node->misc;
		if (node->edgeCount > 1) {
			goto complex;
		}
		else if (node->paths > node->edgeTable[0].son->paths) {
			saveSubr(h, edgeEnd, node, maskcnt, 0, misc);
		}
	}

complex:
	saveSubr(h, edgeEnd, node, maskcnt, 0, misc);
	walkEdgeTable(h, node, findCandSubrsProc, maskcnt, misc);
}

/* Calculate byte savings for this subr. See candSubr() for details */
static int subrSaved(subrCtx h, Subr *subr) {
	int length = subr->length - subr->maskcnt;
	return subr->count * (length - CALL_OP_SIZE - subr->numsize) -
	       (h->offSize + length + ((subr->node->flags & NODE_TAIL) == 0));
}

/* Calculate byte savings by one instance of call to this subr */
static int subrSavedByOneCall(subrCtx h, Subr *subr) {
	int length = subr->length - subr->maskcnt;
	return (length - CALL_OP_SIZE - subr->numsize);
}

/* ----------------------- Subr match trie ----------------------- */

/* Set up suffix links */
static void setTrieSuffixProc(subrCtx h, Edge *edge, long param1, long param2) {
    Node    *node = edge->son;
    Node    *state;
    Edge    *suffixEdge = NULL;

    /* Append this node to the queue for later processing of its children */
    h->trieQueue->next = newNodeLink(h, node, NULL);
    h->trieQueue = h->trieQueue->next;
    if (h->trieParent == h->trieRoot)
        return;

    state = h->trieParent->suffix;

    for (;;) {
        if (!state)
            state = h->trieRoot;
        suffixEdge = findEdge(h, state, edge->length, edge->label);
        if (suffixEdge) {
            node->suffix = suffixEdge->son;
            break;
        }
        if (state == h->trieRoot)
            return;

        state = state->suffix;
    }
    
    /* Chain the output subr of this node to the ouput subr of the suffix node */
    if (node->misc >= 0) {
        Subr    *subr = &h->subrs.array[node->misc];

        if (node->suffix->misc >= 0)
            subr->output = &h->subrs.array[node->suffix->misc];
    } else
        node->misc = node->suffix->misc;
}

/* Update each suffix link with the pointer to the next node */
static void setTrieNextProc(subrCtx h, Edge *edge, long param1, long param2) {
    Node    *node = edge->son;
    Node    *suffix;
    Edge    *suffixEdge = NULL;

    /* Append this node to the queue for later processing of its children */
    h->trieQueue->next = newNodeLink(h, node, NULL);
    h->trieQueue = h->trieQueue->next;
    if (h->trieParent == h->trieRoot)
        return;

    suffix = h->trieParent->suffix;
    if (!suffix)
        suffix = h->trieRoot;
    suffixEdge = findEdge(h, suffix, edge->length, edge->label);
    if (!suffixEdge) {
        if (suffix == h->trieRoot)
            return;

        suffixEdge = findEdge(h, suffix, edge->length, edge->label);
        if (suffixEdge)
            node->suffix = suffixEdge->son;
        else
            node->suffix = NULL;
    }
}

/* Build a subr match trie using Aho-Corasick algorithm */
static void buildSubrMatchTrie(subrCtx h)
{
    long i;
    NodeLink    *link;
    Node        *node;

    h->trieRoot = newTrieNode(h, 0);

    /* Add all subrs to the trie */
    for (i = 0; i < h->subrs.cnt; i++) {
        Subr    *subr = &h->subrs.array[i];
        unsigned char   *pstr, *pend;
        long    oplen;
        long    depth;

        node = h->trieRoot;
        pstr = subr->cstr;
        pend = pstr + subr->length;
        for (depth = 1; pstr < pend; pstr += oplen, depth++) {
            Edge    *edge;

            oplen = OPLEN(h, pstr);
            edge = findEdge(h, node, oplen, pstr);
            if (edge) {
                node = edge->son;
            } else {
                Node    *son = newTrieNode(h, depth);
                addEdge(h, node, son, oplen, pstr, oplen);
                node = son;
            }
        }
        node->misc = (long)(subr - h->subrs.array); /* store output subr index in the trie node */
    }

    /* Set up suffix links */
	reuseObjects(h->g, &h->nodeLinkBlks);
    link = h->trieQueue = newNodeLink(h, h->trieRoot, NULL);
    for (; link; link = link->next) {
        h->trieParent = link->node;
        walkEdgeTable(h, link->node, setTrieSuffixProc, 0, 0);
    }

    /* Update each suffix link with the pointer to the next node */
	reuseObjects(h->g, &h->nodeLinkBlks);
    link = h->trieQueue = newNodeLink(h, h->trieRoot, NULL);
    for (; link; link = link->next) {
        h->trieParent = link->node;
        walkEdgeTable(h, link->node, setTrieNextProc, 0, 0);
    }
}

/* List up all subrs matching the given string against the subr match trie */
static void listUpSubrMatches(subrCtx h, unsigned char *pstart, long length, int buildPhase, int selfMatch,
                            unsigned id, short subrDepth, CallList *callList)
{
    Node    *node = h->trieRoot;
    unsigned char   *pstr, *pend = pstart + length;
    int    oplen;
    Edge    *edge;

    for (pstr = pstart; pstr < pend; pstr += oplen) {
        oplen = OPLEN(h, pstr);

        for (;;) {
            edge = findEdge(h, node, oplen, pstr);
            if (edge) {
                node = edge->son;
                break;
            } else {
                node = node->suffix;
                if (!node)
                    break;
            }
        }

        if (!node) {
            node = h->trieRoot;
            continue;
        }
        if (node->misc >= 0) {
            Subr    *subr = &h->subrs.array[node->misc];

            for (; subr; subr = subr->output) {
                long    offset;

                if (buildPhase) {
                    if ((subr->flags & SUBR_MARKED) != SUBR_SELECT)
                        continue;
                    if (subr->node->id != NODE_GLOBAL && subr->node->id != id)
                        continue;
                }

                offset = (long)(pstr - pstart + oplen - subr->length);
                if ((offset >= 0) && (offset + subr->length <= length)) {
                    Call  *c;
                
                    if (!selfMatch && offset == 0 && offset + subr->length == length)
                        continue;
					/* respect the max subr stack depth observed by checkSubrStackOvl */
                    if ((subrDepth >= 0) && (subrDepth <= subr->misc))
                        continue;

                    c = dnaNEXT(*callList);
                    c->subr = subr;
                    c->offset = (unsigned short)offset;
                }
            }
        }
    }
}

/* Compare subr calls by length (longest first) then by offset (smallest first) */
static int CTL_CDECL cmpSubrLengths(const void *first, const void *second) {
	Call *a = (Call *)first;
	Call *b = (Call *)second;
    if (a->subr->length != b->subr->length)
        return (int)b->subr->length - (int)a->subr->length;
    else
        return (int)a->offset - (int)b->offset;
}

/* Scan charstring and build call list of subrs */
/* The same function is called with buildPhase = 0 for setting subr count duing
   overlap handling phase and called with buildPhase = 1 for building call list.

   The code looks for subrs with the longest length first, then try to cover
   a given charstring. During the following iterations shorter subrs are tried
   to fill gaps (if we apply the same logic to each gap recursively, we can get
   the most optimal result). Lists are created and are tried to fill their gaps in
   the order of the subr size. All subrs including those inferior to others are tried.
   If a subr can't fit in any gap in lists, then a new list is created for it.
   At the end, the most space saving list of subrs is selected as the call list.
   The call list may not contain the longest inferior subr for the given charstring
   as the result.

   TODO: If this approach works well the overlap handling phase should be rewritten
   using the same logic in order to resolve the logic disparity between the two phases.
 */

static void buildCallList(subrCtx h, int buildPhase, unsigned length, unsigned char *pstart,
                          int selfMatch, unsigned id, short subrDepth) {
	//unsigned char *pend = pstart + length;
	unsigned i, j;
    CallList  callList;

    /* List up all matching subrs */
    dnaINIT(h->g->ctx.dnaSafe, callList, 100, 100);
    listUpSubrMatches(h, pstart, length, buildPhase, selfMatch, id, subrDepth, &callList);
    qsort(callList.array, callList.cnt, sizeof(Call), cmpSubrLengths);

    /* Try to fill lists with longest subrs first */
    h->calls.cnt = 0;
    for (i = 0; i < (unsigned)callList.cnt; i++) {
        Call  *c = &callList.array[i];
        Subr    *subr = c->subr;
        unsigned subrEndOffset = c->offset + subr->length;

        /* Try to fill a gap in calls array */
        unsigned cnt;
        int overlap = 0;
        cnt = h->calls.cnt;

        for (j = 0; j < cnt; j++) {
            Call *call = &h->calls.array[j];
            if (subrEndOffset <= call->offset) {
                /* found gap before at j'th call */
                break;
            }
            if (subrEndOffset > call->offset
                && c->offset < (unsigned)call->offset + call->subr->length) {
                /* overlap */
                overlap = 1;
                break;
            }
            if (c->offset < call->offset + (unsigned)call->subr->length
                && subrEndOffset > call->offset) {
                /* overlap */
                overlap = 1;
                break;
            }
        }

        /* insert the subr into this gap */
        if (!overlap) {
            dnaSET_CNT(h->calls, cnt + 1);
            memmove(&h->calls.array[j + 1], &h->calls.array[j], sizeof(Call) * (cnt - j));
            h->calls.array[j] = *c;
        }
	}

    dnaFREE(callList);

    for (i = 0; i < (unsigned)h->calls.cnt; i++) {
        Call *call = &h->calls.array[i];
        call->subr->count++;
#if DB_ASSOC
        dbsubr(h, call->subr - h->subrs.array, 'i', call->offset);
#endif
	}

#if DB_INFS
	if (buildPhase && h->calls.cnt != 0) {
		int j;
		for (j = 0; j < h->calls.cnt; j++) {
			Call *call = &h->calls.array[j];
			if (call->subr != NULL) {
				dbsubr(h, call->subr - h->subrs.array, 'y', call->offset);
			}
		}
	}
#endif
}

/* Reset subr count */
static void resetSubrCount(subrCtx h, unsigned id) {
	long i;
	Subr *subr;

	for (i = 0; i < h->subrs.cnt; i++) {
		subr = &h->subrs.array[i];
        if (subr->node->id == id || id == NODE_ANY)
            subr->count = 0;
	}
}

static void setSubrActCount(subrCtx h) {
	long i, j;
	Subr *subr;
	Link *infs;

#if DB_ASSOC
	printf("--- assoc subrs with subrs\n");
#endif

	resetSubrCount(h, NODE_ANY);

	/* Make call list for each subr and set actual call count in each */
	for (i = 0; i < h->subrs.cnt; i++) {
#if DB_ASSOC || DB_RELNS
		dbsubr(h, i, '-', 0);
#endif
		subr = &h->subrs.array[i];
		buildCallList(h, 0, subr->length, subr->cstr, 0, 0, -1);

		infs = NULL;
		for (j = h->calls.cnt - 1; j >= 0; j--) {
			Call *call = &h->calls.array[j];
			Subr *inf = call->subr;

			/* Add superior subr to inferior subrs */
			inf->sups = newLink(h, subr, call->offset, inf->sups);

			/* Add inferior subr to list */
			infs = newLink(h, inf, call->offset, infs);
		}
		subr->infs = infs;
	}
}

static void sortInfSubrs(subrCtx h) {
	long i;
	Subr *subr;

	/* Cache the subrSaved value for use during sort below */
	for (i = 0; i < h->subrs.cnt; i++) {
		subr = &h->subrs.array[i];
		subr->misc = (short)subrSaved(h, subr);
	}

	/* Reorder inferiors by savings (largest first) */
	for (i = 0; i < h->subrs.cnt; i++) {
		Link *head; /* Head if inferior list */
		subr = &h->subrs.array[i];

		/* Sort list in-place */
		for (head = subr->infs; head != NULL; head = head->next) {
			Link *try_ = head->next;
			if (try_ == NULL) {
				break;  /* End of list */
			}
			else {
				Link *best = NULL;
				int max = head->subr->misc;

				/* Find best inferior subr in remainder of list */
				do {
					int saved = try_->subr->misc;
					if (saved > max) {
						best = try_;
						max = saved;
					}
					try_ = try_->next;
				}
				while (try_ != NULL);

				if (best != NULL) {
					/* Swap "head" and "best" nodes */
					Call tmp;
					tmp.subr = head->subr;
					tmp.offset = head->offset;

					head->subr = best->subr;
					head->offset = best->offset;

					best->subr = tmp.subr;
					best->offset = tmp.offset;
				}
			}
		}
	}
}

/* Select candidate subrs */
static void selectCandSubrs(subrCtx h) {
	/* Count paths */
	(void)countPathsForNode(h, h->root);

	/* Find candidate subrs */
	h->subrs.cnt = 0;
	walkEdgeTable(h, h->root, findCandSubrsProc, 0, 0);

#if 0
	printf("--- xfindSubrRelns\n");
	h->tmp.cnt = 0;
	for (edge = &h->root->edge; edge != NULL; edge = edge->next) {
		if (OPLEN(h, edge->label) == edge->son->misc) {
			xfindSubrRelns(h, edge, 0);
		}
	}
#endif
}

/* ----------------------- Associate Subrs With Font ----------------------- */

/* Associate subrs with a font in the FontSet */
static void assocSubrs(subrCtx h) {
	int i;

#if DB_ASSOC
	printf("--- assoc chars with subrs\n");
#endif

	for (i = 0; i < h->nFonts; i++) {
		long j;
		long offset;
		subr_Font *font = &h->fonts[i];

		offset = 0;
		for (j = 0; j < font->chars.nStrings; j++) {
			long nextoff = font->chars.offset[j];

#if DB_ASSOC
			printf("[%3d:%4ld]  ", i, j);
			dbcstr(h, nextoff - offset,
			       (unsigned char *)&FONT_CHARS_DATA[offset]);
			printf("\n");
#endif

			buildCallList(h, 0, nextoff - offset, (unsigned char *)&FONT_CHARS_DATA[offset], 1, 0, -1);

			offset = nextoff;
		}
	}
}

/* --------------------------- Select Final Subrs -------------------------- */

#if 0
static void prints(Subr *subr) {
	int c = (subr->node->id == NODE_GLOBAL) ? 'g' : 'l';
	if (subr->flags & SUBR_MEMBER) {
		c -= 'a' - 'A';
	}
	printf("%d%c", subrSaved(ctx, subr), c);
	if (subr->flags & SUBR_SELECT) {
		printf("s ");
	}
	else if (subr->flags & SUBR_REJECT) {
		printf("r ");
	}
	else {
		printf(" ");
	}
}

#endif

#if 0   /*Recursive*/
/* Add social group member subr */
static void addMember(subrCtx h, Subr *subr) {
	Link *link;

	*dnaNEXT(h->members) = subr;
	subr->flags |= SUBR_MEMBER;

	for (link = subr->sups; link != NULL; link = link->next) {
		if (!(link->subr->flags & SUBR_MEMBER)) {
			addMember(h, link->subr);
		}
	}

	for (link = subr->infs; link != NULL; link = link->next) {
		if (!(link->subr->flags & SUBR_MEMBER)) {
			addMember(h, link->subr);
		}
	}
}

#else
/* Add social group member subr */

#define LISTSIZE 4000
static void addMember(subrCtx h, Subr *subr) {
	Link *link;
	Subr **list;   /*Used to reverse the order of the Subr's*/
	int listlength;
	int i;

	dnaDCL(Subr *, subrStack);
	dnaINIT(h->g->ctx.dnaSafe, subrStack, 4000, 4000);

	list = (Subr **)malloc(LISTSIZE * sizeof(Subr *));
	listlength = 0;

	*dnaNEXT(subrStack) = subr;
	while (subrStack.cnt > 0) {
		--subrStack.cnt;
		subr = *dnaINDEX(subrStack, subrStack.cnt);
		if (!(subr->flags & SUBR_MEMBER)) {
			/* Add member and mark subr to avoid reselection */
			*dnaNEXT(h->members) = subr;
			subr->flags |= SUBR_MEMBER;

			for (link = subr->sups; link != NULL; link = link->next) {
				/* Add superior member to temporary list*/
				list[listlength++] = link->subr;
				if (listlength >= LISTSIZE) {
#if TC_DEBUG
					fprintf(stderr, "Typecomp Error: List Overflow\n");
#endif
					free(list);
					dnaFREE(subrStack);
					return;
				}
			}
			for (link = subr->infs; link != NULL; link = link->next) {
				/* Add inferior member to temporary list*/
				list[listlength++] = link->subr;
				if (listlength >= LISTSIZE) {
#if TC_DEBUG
					fprintf(stderr, "Typecomp Error: List Overflow\n");
#endif
					free(list);
					dnaFREE(subrStack);
					return;
				}
			}
			/* Transfer from temporary list into stack*/
			for (i = listlength - 1; i >= 0; i--) {
				*dnaNEXT(subrStack) = list[i];
			}
			listlength = 0;
		}
	}
	free(list);
	dnaFREE(subrStack);
}

#endif

/* Compare social group member subrs for a global subr set. Global subrs are
   sorted before local subrs, largest saving first. */
static int CTL_CDECL cmpGlobalSetSubrs(const void *first, const void *second) {
	Subr *a = *(Subr **)first;
	Subr *b = *(Subr **)second;
	if (a->node->id == NODE_GLOBAL) {
		if (b->node->id == NODE_GLOBAL) {
			/* global global */
			int asaved = subrSaved(ctx, a);
			int bsaved = subrSaved(ctx, b);
			if (asaved > bsaved) {
				return -1;
			}
			else if (asaved < bsaved) {
				return 1;
			}
			else {
				return 0;
			}
		}
		else {
			/* global local */
			return -1;
		}
	}
	else if (b->node->id == NODE_GLOBAL) {
		/* local global */
		return 1;
	}
	else {
		/* local local */
		return 0;
	}
}

/* Compare social group member subrs for a local subr set. Selected global
   subrs are sorted before local subrs which are sorted before rejected global
   subrs. Selected global and unselected local subrs are further sorted by
   largest savings first. */
static int CTL_CDECL cmpLocalSetSubrs(const void *first, const void *second) {
	Subr *a = *(Subr **)first;
	Subr *b = *(Subr **)second;
	switch ((a->flags & SUBR_MARKED) << 2 | (b->flags & SUBR_MARKED)) {
		/* a-subr			b-subr			*/
		case 0: /* local			local			*/
		case 5: /* global.select	global.select   */
		{
			int asaved = subrSaved(ctx, a);
			int bsaved = subrSaved(ctx, b);
			if (asaved > bsaved) {
				return -1;
			}
			else if (asaved < bsaved) {
				return 1;
			}
			else {
				return 0;
			}
		}

		case 1: /* local			global.select	*/
		case 8: /* global.reject	local			*/
		case 9: /* global.reject	global.select	*/
			return 1;

		case 2: /* local			global.reject	*/
		case 4: /* global.select	local			*/
		case 6: /* global.select	global.reject	*/
			return -1;

		case 10: /* global.reject	global.reject	*/
			return 0;

		default:
#if TC_DEBUG
			printf("cmpLocalSetSubrs() can't happen!\n");
#endif
            break;
	}
	return 0;   /* Suppress compiler warning */
}

/* Find social groups for global or local subr set. */
static void findGroups(subrCtx h, unsigned id) {
	long i;
	int (CTL_CDECL *cmpSubrs)(const void *first, const void *second) =
	    (id == NODE_GLOBAL) ? cmpGlobalSetSubrs : cmpLocalSetSubrs;

	h->leaders.cnt = 0;
	for (i = 0; i < h->tmp.cnt; i++) {
		long j;
		Subr *subr = h->tmp.array[i];

		if (subr->flags & SUBR_MEMBER) {
			continue;
		}

		/* Add new social group */
		h->members.cnt = 0;
		addMember(h, subr);

		/* Sort members by largest saving first */
		qsort(h->members.array, h->members.cnt, sizeof(Subr *), cmpSubrs);

#if 0
		for (j = 0; j < h->members.cnt; j++) {
			prints(h->members.array[j]);
		}
		printf("|\n");
#endif

		/* Link members */
		for (j = 0; j < h->members.cnt - 1; j++) {
			h->members.array[j]->next = h->members.array[j + 1];
		}
		h->members.array[j]->next = NULL;

		/* Add leader */
		*dnaNEXT(h->leaders) = h->members.array[0];
	}
}

/* Update superior lengths */
static void updateSups(subrCtx h, Subr *subr, int deltalen, unsigned id) {
	Link *link;

	for (link = subr->sups; link != NULL; link = link->next) {
		Subr *sup = link->subr;
		if (!(sup->flags & SUBR_MARKED) && sup->node->id == id) {
#if DB_SELECT
			printf("updateSups([%d]->[%d],%d) deltalen=%d\n",
			       subr - h->subrs.array, sup - h->subrs.array,
			       deltalen, sup->deltalen + deltalen);
#endif
			updateSups(h, sup, deltalen, id);
			sup->deltalen += (short)deltalen;
		}
	}
}

/* Select subr */
static void selectSubr(subrCtx h, Subr *subr) {
	int count = subr->count;
	int length = subr->length - subr->maskcnt + subr->deltalen;
	int saved = count * (length - CALL_OP_SIZE - subr->numsize) -
	    (h->offSize + length + ((subr->node->flags & NODE_TAIL) == 0));

#if DB_SELECT
	printf("selectSubr([%d]) count=%d, length=%d, saved=%d\n",
	       subr - h->subrs.array, count, length, saved);
#endif

	if (saved > 0 && !(subr->flags & SUBR_FUTILE)) {
		/* Select subr */
		unsigned id = subr->node->id;
		int deltalen = subr->numsize + CALL_OP_SIZE - subr->length -
		    subr->deltalen;

        subr->flags &= ~SUBR_REJECT;
		subr->flags |= SUBR_SELECT;

#if DB_SELECT
		printf("select=%d, deltalen=%d\n",
		       saved, deltalen);
#endif
		updateSups(h, subr, deltalen, id);
	}
	else {
		/* Reject subr */
        subr->flags &= ~SUBR_SELECT;
		subr->flags |= SUBR_REJECT;
#if DB_SELECT
		printf("reject\n");
#endif
	}
}

/* Select global subrs from social group. */
static void selectGlobalSubrs(subrCtx h, Subr *subr, unsigned id) {
	for (; subr != NULL; subr = subr->next) {
		if (subr->node->id != NODE_GLOBAL) {
			return; /* Quit on first local subr */
		}
		else if (!(subr->flags & SUBR_MARKED)) {
			selectSubr(h, subr);
		}
	}
}

/* Select local subrs from social group. */
static void selectLocalSubrs(subrCtx h, Subr *subr, unsigned id) {
	for (; subr != NULL; subr = subr->next) {
		if (subr->node->id == NODE_GLOBAL) {
			if (subr->flags & SUBR_SELECT) {
				updateSups(h, subr, subr->numsize + CALL_OP_SIZE -
				           subr->length, id);
			}
			else {
				return; /* Quit on first rejected global subr */
			}
		}
		else if (!(subr->flags & SUBR_MARKED)) {
			selectSubr(h, subr);
		}
	}
}

/* Compare subr calls by selection/saved/length/frequency */
static int CTL_CDECL cmpSubrFitness(const void *first, const void *second) {
	Subr *a = *(Subr **)first;
	Subr *b = *(Subr **)second;
	int aselect = (a->flags & SUBR_SELECT) != 0;
	int bselect = (b->flags & SUBR_SELECT) != 0;
	if (aselect == bselect) {
		int asaved = subrSaved(ctx, a);
		int bsaved = subrSaved(ctx, b);
		if (asaved > bsaved) {
			/* Compare savings */
			return -1;
		}
		else if (asaved < bsaved) {
			return 1;
		}
		else if (a->length > b->length) {
			/* Compare length */
			return -1;
		}
		else if (a->length < b->length) {
			return 1;
		}
		else if (a->count > b->count) {
			/* Compare count */
			return -1;
		}
		else if (a->count < b->count) {
			return 1;
		}
		else {
			return 0;
		}
	}
	else if (!aselect && bselect) {
		return 1;
	}
	else {
		return -1;
	}
}

/* Check for subr stack depth overflow */
static void checkSubrStackOvl(subrCtx h, Subr *subr, int depth, unsigned id) {
	Link *sup;

	if ((subr->flags & SUBR_MARKED) == SUBR_SELECT) {
        /* No need to check this subr if it is local and has been checked with this or deeper stack */
        if (depth <= subr->misc && (subr->node->id != NODE_GLOBAL || id == NODE_GLOBAL)) {
            return;
        }

        if (depth > subr->misc)
            subr->misc = (short)depth;
		depth++;    /* Bump depth for selected subrs only */

		if (depth >= TX_MAX_CALL_STACK) {
			/* Stack depth exceeded; reject subr */
			subr->flags &= ~SUBR_SELECT;
			subr->flags |= SUBR_REJECT;
			depth--;
			h->subrStackOvl = 1;
			subr->misc = (short)depth;
		}
	}

	/* Recursively ascend superior subrs */
	for (sup = subr->sups; sup != NULL; sup = sup->next) {
		checkSubrStackOvl(h, sup->subr, depth, id);
	}
}

/* Select subr set to be saved in font. Subr list is in the temporary array */
static void selectFinalSubrSet(subrCtx h, unsigned id) {
	long i;
	void (*selectSubrs)(subrCtx h, Subr *subr, unsigned id) =
	    (id == NODE_GLOBAL) ? selectGlobalSubrs : selectLocalSubrs;
	long nSelected = 0; /* Suppress optimizer warning */
	int multiplier = h->singleton ? 2 : 1;    /* Range multiplier */
	int pass = 0;
    long limit = h->maxNumSubrs;

    if (limit == 0) {
        limit = MAX_NUM_SUBRS;
    }
    limit = limit * multiplier;

	findGroups(h, id);  /* Find social groups (related subrs) */

reselect:
	for (i = 0; i < h->leaders.cnt; i++) {
		Subr *subr = h->leaders.array[i];

#if DB_SELECT
		printf("--- group[%ld]\n", i);
#endif
		if (subr->next == NULL) {
			/* Select/reject hermit subr */
			subr->flags |= (subrSaved(h, subr) > 0) ? SUBR_SELECT : SUBR_REJECT;
#if DB_SELECT
			printf("%s=%d\n", (subr->flags & SUBR_SELECT) ? "select" : "reject",
			       subrSaved(h, subr));
#endif
		}
		else {
			selectSubrs(h, subr, id);
		}
	}

#if DB_GROUPS
	dbgroups(h);
#endif

	/* Reset misc field for storing subr call depth by checkSubrStackOvl */
	for (i = 0; i < h->tmp.cnt; i++) {
		h->tmp.array[i]->misc = -1;
	}

	/* Remove membership so globals may be selected in other local sets and
	   check for and handle subr call stack overflow */
	for (i = 0; i < h->leaders.cnt; i++) {
		Subr *subr;
		for (subr = h->leaders.array[i]; subr != NULL; subr = subr->next) {
			if (subr->infs == NULL) {
				checkSubrStackOvl(h, subr, 0, id);
			}
			subr->flags &= ~SUBR_MEMBER;
		}
	}

	/* Sort selected subrs by fitness */
	qsort(h->tmp.array, h->tmp.cnt, sizeof(Subr *), cmpSubrFitness);

	/* Find last selected subr */
	for (i = h->tmp.cnt - 1; i >= 0; i--) {
		if ((h->tmp.array[i]->flags & SUBR_MARKED) == SUBR_SELECT) {
			nSelected = i + 1;
			break;
		}
	}

    if (nSelected >= limit) {
        for (i = limit; i < nSelected; i++) {
            h->tmp.array[i]->flags &= ~SUBR_SELECT;
            h->tmp.array[i]->flags |= SUBR_REJECT;
        }
        h->tmp.cnt = nSelected = limit;
    }
    else {
        h->tmp.cnt = nSelected;
    }

	if (++pass == 2) {
		return;
	}
	else if (nSelected < 215 * multiplier) {
		/* All subrs fit into first 1-byte range. Since we don;t have to increase the index number size,
		   for any subroutines, their calculated savings won't change on a second pass. */
		h->tmp.cnt = nSelected;
		return;
	}

	/* Removed subr marking before reselection */
	/* We are going to have to increase the index number size for some subroutines.
	   This means that their savings will get smaller, and we may need to reject them.
	 */
	for (i = 0; i < h->tmp.cnt; i++) {
		h->tmp.array[i]->flags &= ~SUBR_MARKED;
	}

	/* Assign new subr number sizes */
	for (i = h->tmp.cnt - 1; i >= 2263 * multiplier; i--) {
		h->tmp.array[i]->numsize = 3;
	}
	for (; i >= 215 * multiplier; i--) {
		h->tmp.array[i]->numsize = 2;
	}
	for (; i >= 0; i--) {
		h->tmp.array[i]->numsize = 1;
	}

	goto reselect;
}

/* --------------------------- Add Subrs to INDEX -------------------------- */

/* Subroutinize charstring */
static unsigned char *subrizeCstr(subrCtx h,
                                  unsigned char *pdst, unsigned char *psrc,
                                  unsigned length) {
	int i;
	long offset;

	/* Insert calls in charstring */
	offset = 0;
	for (i = 0; i < h->calls.cnt; i++) {
		Call *call = &h->calls.array[i];
		Subr *subr = call->subr;

		if (subr != NULL) {
			/* Copy bytes preceeding subr */
			pdst = t2cstrcpy(pdst, psrc, call->offset - offset);

			/* Add subr call */
			pdst += cfwEncInt(subr->subrnum, pdst);
			*pdst++ = (subr->node->id == NODE_GLOBAL) ?
			    t2_callgsubr : tx_callsubr;

#if DB_CALLS
			subr->calls++;
#endif

			/* Skip over subr's bytes */
			psrc += call->offset - offset + subr->length;
			offset = call->offset + subr->length;
		}
	}

	/* Copy remainder of charstring and return destination buffer pointer */
	return t2cstrcpy(pdst, psrc, length - offset);
}

/* Subroutinize charstrings */
static void subrizeChars(subrCtx h, subr_CSData *chars, unsigned id, int preflight) {
	long i;
	long offset;

#if DB_CHARS
	printf("--- subrized chars (chars marked with =)\n");
#endif

	offset = 0;
	h->cstrs.cnt = 0;
	for (i = 0; i < chars->nStrings; i++) {
		unsigned char *pdst;
		unsigned char *psrc = (unsigned char *)&chars->data[offset];
		long nextoff = chars->offset[i];
		unsigned length = nextoff - offset - 4 /* t2_separator */;
		long iStart = h->cstrs.cnt;

		/* Initially allocate space for entire charstring */
		pdst = (unsigned char *)dnaEXTEND(h->cstrs, (long)length);

#if DB_CHARS
		printf("[%3ld]    =  ", i);
		dbcstr(h, length, psrc);
		printf("\n");
#endif

		/* Build subr call list */
		buildCallList(h, 1, length, psrc, 1, id, -1);

        if (!preflight) {
            /* Subroutinize charstring */
            pdst = subrizeCstr(h, pdst, psrc, length);

            /* Adjust initial length estimate and save offset */
            h->cstrs.cnt = (long)(iStart + pdst - (unsigned char *)&h->cstrs.array[iStart]);
            chars->offset[i] = h->cstrs.cnt;
        }
		offset = nextoff;
	}

    if (!preflight) {
        /* Copy charstring data without loosing original data pointer */
        chars->refcopy = chars->data;
        chars->data = (char *)MEM_NEW(h->g, h->cstrs.cnt);
        memcpy(chars->data, h->cstrs.array, h->cstrs.cnt);

#if DB_CALLS
        printf("--- actual subr calls\n");
        for (i = 0; i < h->reorder.cnt; i++) {
            Subr *subr = h->reorder.array[i];
            printf("[%3d]=%2d (%2d)", subr - h->subrs.array,
                   subr->calls, subr->count);
            if (subr->calls != subr->count) {
                printf("*\n");
            }
            else {
                printf("\n");
            }
        }
#endif
    }
}

/* Create biased reordering for either global or local subrs for a singleton
   font using combined subr INDEXes. The global subrs are selected from even
   temporary array indexes and local subrs are chosen from odd indexes */
static void reorderCombined(subrCtx h, int local) {
	long bias;
	long count; /* Element in reorder array */
	long i;     /* Reorder array index */
	long j;     /* Temporary array index */

	if (local) {
		/* Reorder local (odd) indexes */
		count = h->tmp.cnt / 2;
		j = (h->tmp.cnt & ~1) + 1;
	}
	else {
		/* Reorder global (even) indexes */
		count = (h->tmp.cnt + 1) / 2;
		j = (h->tmp.cnt + 1) & ~1;
	}

	/* Set the reorder array size */
	dnaSET_CNT(h->reorder, count);

	i = count - 1;
	if (count < 1240) {
		/* Bias-107 reordering */
		for (; i >= 0; i--) {
			h->reorder.array[i +     0] = h->tmp.array[j -= 2];
		}
		bias = 107;
	}
	else if (count < 33900) {
		/* Bias-1131 reordering */
		for (; i >= 1239; i--) {
			h->reorder.array[i +     0] = h->tmp.array[j -= 2];
		}
		for (; i >=  215; i--) {
			h->reorder.array[i -   215] = h->tmp.array[j -= 2];
		}
		for (; i >=    0; i--) {
			h->reorder.array[i +  1024] = h->tmp.array[j -= 2];
		}
		bias = 1131;
	}
	else {
		/* Bias-32768 reordering */
		for (; i >= 33900; i--) {
			h->reorder.array[i +     0] = h->tmp.array[j -= 2];
		}
		for (; i >=  2263; i--) {
			h->reorder.array[i -  2263] = h->tmp.array[j -= 2];
		}
		for (; i >=  1239; i--) {
			h->reorder.array[i + 31637] = h->tmp.array[j -= 2];
		}
		for (; i >=   215; i--) {
			h->reorder.array[i + 31422] = h->tmp.array[j -= 2];
		}
		for (; i >=     0; i--) {
			h->reorder.array[i + 32661] = h->tmp.array[j -= 2];
		}
		bias = 32768;
	}

	/* Add biased subr numbers and mark global subrs */
	for (i = 0; i < count; i++) {
		Subr *subr = h->reorder.array[i];
		subr->subrnum = (short)(i - bias);
		if (!local) {
			subr->node->id = NODE_GLOBAL;
		}
	}
}

/* Create biased reordering for a non-singleton font */
static void reorderSubrs(subrCtx h) {
	long i;
	long bias;

	/* Assign reording index */
	dnaSET_CNT(h->reorder, h->tmp.cnt);
	i = h->tmp.cnt - 1;
	if (h->tmp.cnt < 1240) {
		/* Bias-107 reordering */
		for (; i >= 0; i--) {
			h->reorder.array[i +     0] = h->tmp.array[i];
		}
		bias = 107;
	}
	else if (h->tmp.cnt < 33900) {
		/* Bias-1131 reordering */
		for (; i >= 1239; i--) {
			h->reorder.array[i +     0] = h->tmp.array[i];
		}
		for (; i >=  215; i--) {
			h->reorder.array[i -   215] = h->tmp.array[i];
		}
		for (; i >=    0; i--) {
			h->reorder.array[i +  1024] = h->tmp.array[i];
		}
		bias = 1131;
	}
	else {
		/* Bias-32768 reordering */
		for (; i >= 33900; i--) {
			h->reorder.array[i +     0] = h->tmp.array[i];
		}
		for (; i >=  2263; i--) {
			h->reorder.array[i -  2263] = h->tmp.array[i];
		}
		for (; i >=  1239; i--) {
			h->reorder.array[i + 31637] = h->tmp.array[i];
		}
		for (; i >=   215; i--) {
			h->reorder.array[i + 31422] = h->tmp.array[i];
		}
		for (; i >=     0; i--) {
			h->reorder.array[i + 32661] = h->tmp.array[i];
		}
		bias = 32768;
	}

	/* Add biased subr numbers */
	for (i = 0; i < h->reorder.cnt; i++) {
		h->reorder.array[i]->subrnum = (short)(i - bias);
	}
}

/* Add reorder subrs from reorder array */
static void addSubrs(subrCtx h, subr_CSData *subrs, unsigned id, int preflight) {
	long i;

#if DB_SUBRS
	printf("--- subrized subrs (subrs marked with -)\n");
#endif

	if (h->reorder.cnt == 0) {
		return; /* No subrs */
	}

    if (!preflight) {
        /* Allocate subr offset array */
        subrs->nStrings = (unsigned short)h->reorder.cnt;
        subrs->offset = (Offset *)MEM_NEW(h->g, h->reorder.cnt * sizeof(Offset));

        h->cstrs.cnt = 0;
    }
	for (i = 0; i < h->reorder.cnt; i++) {
		unsigned char *pdst = NULL;
		Subr *subr = h->reorder.array[i];
		long iStart = h->cstrs.cnt;

        if (!preflight) {
            /* Initially allocate space for entire charstring + last op  */
            pdst = (unsigned char *)dnaEXTEND(h->cstrs, subr->length + 1);

#if DB_SUBRS
            dbsubr(h, subr - h->subrs.array, '-', 0);
#endif
        }

		/* Build subr call list */
		buildCallList(h, 1, subr->length, subr->cstr, 0, id, subr->misc);

        if (!preflight) {
            /* Subroutinize charstring */
            pdst = subrizeCstr(h, pdst, subr->cstr, subr->length);

            /* Terminate subr */
            if ((!(subr->node->flags & NODE_TAIL)) && (!(h->g->flags & CFW_WRITE_CFF2))) {
                *pdst++ = (unsigned char)tx_return;
            }

            /* Adjust initial length estimate and save offset */
            h->cstrs.cnt = (long)(iStart + pdst - (unsigned char *)&h->cstrs.array[iStart]);
            subrs->offset[i] = h->cstrs.cnt;
        }
	}

    if (!preflight) {
        /* Allocate and copy charstring data */
        subrs->data = (char *)MEM_NEW(h->g, h->cstrs.cnt);
        memcpy(subrs->data, h->cstrs.array, h->cstrs.cnt);
    }
}

/* Subroutinize FD charstrings */
static void subrizeFDChars(subrCtx h, subr_CSData *dst, subr_Font *font,
                           unsigned iFont, unsigned iFD, int preflight) {
	long iSrc;
	unsigned iDst;
	subr_CSData *src = &font->chars;

#if DB_CHARS
	printf("--- subrized FD[%u] chars (chars marked with =)\n", iFD);
#endif

	/* Count chars in this this FD */
	dst->nStrings = 0;
	for (iSrc = 0; iSrc < src->nStrings; iSrc++) {
		if (font->fdIndex[iSrc] == iFD) {
			dst->nStrings++;
		}
	}

    if (!preflight) {
        /* Allocate offset array */
        dst->offset = (Offset *)MEM_NEW(h->g, sizeof(Offset) * dst->nStrings);

        h->cstrs.cnt = 0;
        iDst = 0;
    }
	for (iSrc = 0; iSrc < src->nStrings; iSrc++) {
		if (font->fdIndex[iSrc] == iFD) {
			unsigned char *pdst;
			long offset = (iSrc == 0) ? 0 : src->offset[iSrc - 1];
			unsigned char *psrc = (unsigned char *)&src->data[offset];
			unsigned length = src->offset[iSrc] - offset - 4 /* t2_separator */;
			long iStart = h->cstrs.cnt;

            if (!preflight) {
                /* Initially allocate space for entire charstring */
                pdst = (unsigned char *)dnaEXTEND(h->cstrs, (long)length);

#if DB_CHARS
                printf("[%3ld]    =  ", iSrc);
                dbcstr(h, length, psrc);
                printf("\n");
#endif
            }

			/* Build subr call list */
			buildCallList(h, 1, length, psrc, 1, iFont + iFD, -1);

            if (!preflight) {
                /* Subroutinize charstring */
                pdst = subrizeCstr(h, pdst, psrc, length);

                /* Adjust initial length estimate and save offset */
                h->cstrs.cnt = (long)(iStart + pdst -
                    (unsigned char *)&h->cstrs.array[iStart]);
                dst->offset[iDst++] = h->cstrs.cnt;
            }
		}
	}

    if (!preflight) {
        /* Copy charstring data */
        dst->data = (char *)MEM_NEW(h->g, h->cstrs.cnt);
        memcpy(dst->data, h->cstrs.array, h->cstrs.cnt);

#if DB_CALLS
        {
            int i;
            printf("--- actual subr calls\n");
            for (i = 0; i < h->reorder.cnt; i++) {
                Subr *subr = h->reorder.array[i];
                printf("[%3d]=%2d (%2d)", subr - h->subrs.array,
                       subr->calls, subr->count);
                if (subr->calls != subr->count) {
                    printf("*\n");
                }
                else {
                    printf("\n");
                }
            }
        }
#endif
    }
}

/* Reassemble cstrs for CID-keyed font */
static void joinFDChars(subrCtx h, subr_Font *font) {
	cfwCtx g = h->g;
	long i;
	long size;
	long dstoff;

	/* Determine total size of subroutinized charstring data */
	size = 0;
	for (i = 0; i < font->fdCount; i++) {
		subr_FDInfo *info = &font->fdInfo[i];
		size += info->chars.offset[info->chars.nStrings - 1];
		info->iChar = 0;
	}

	/* Allocate new charstring data without loosing original data pointer */
	font->chars.refcopy = FONT_CHARS_DATA;
	font->chars.data = (char *)MEM_NEW(g, size);

	dstoff = 0;
	for (i = 0; i < font->chars.nStrings; i++) {
		subr_FDInfo *info = &font->fdInfo[font->fdIndex[i]];
		long srcoff =
		    (info->iChar == 0) ? 0 : info->chars.offset[info->iChar - 1];
		unsigned length = info->chars.offset[info->iChar++] - srcoff;

		memmove(&font->chars.data[dstoff], &info->chars.data[srcoff], length);

		font->chars.offset[i] = (dstoff += length);
	}

	/* Free temporary chars index for each FD */
	for (i = 0; i < font->fdCount; i++) {
		subr_CSData *chars = &font->fdInfo[i].chars;
		MEM_FREE(g, chars->offset);
		MEM_FREE(g, chars->data);
	}
}

/* Returns 1 if any subrs have use count one or less */
static int checkFutileSubrs(subrCtx h) {
    long i;
    long futileCount = 0;

    /* Mark all zero-use subrs as futile */
    for (i = 0; i < h->subrs.cnt; i++) {
        Subr *subr = &h->subrs.array[i];
        if (((subr->flags & (SUBR_MARKED|SUBR_FUTILE)) == SUBR_SELECT) && subr->count <= 1) {
            subr->flags &= ~SUBR_SELECT;
            subr->flags |= (SUBR_REJECT|SUBR_FUTILE);
            futileCount++;
        }
    }

    return futileCount > 0;
}

/* -------------------------- Subroutinize FontSet ------------------------- */

/* Build subrs with specific id */
static void buildSubrs(subrCtx h, subr_CSData *subrs, unsigned id, int preflight) {
	long i;

	/* Build temporary array of subrs with matching id */
	h->tmp.cnt = 0;
	for (i = 0; i < h->subrs.cnt; i++) {
		Subr *subr = &h->subrs.array[i];
		if (subr->node->id == id) {
			*dnaNEXT(h->tmp) = subr;
		}
	}

#if 0
	printf("--- buildSubrs()\n");
	for (i = 0; i < h->tmp.cnt; i++) {
		Link *link;
		Subr *subr = h->tmp.array[i];
		prints(subr);
		printf("={");
		for (link = subr->sups; link != NULL; link = link->next) {
			prints(link->subr);
		}
		printf("}{");
		for (link = subr->infs; link != NULL; link = link->next) {
			prints(link->subr);
		}
		printf("}\n");
	}
#endif

  selectFinalSubrSet(h, id);
  resetSubrCount(h, id);
	reorderSubrs(h);
  addSubrs(h, subrs, id, preflight);
}

/* Subroutinize all fonts in FontSet */
void cfwSubrSubrize(cfwCtx g, int nFonts, subr_Font *fonts) {
	subrCtx h = g->ctx.subr;
	unsigned iFont;
    long preflight;
    int futile = 1;
    long i;
    
	h->nFonts = (short)nFonts;
	h->fonts = fonts;
    h->maxNumSubrs = g->maxNumSubrs;

	/* Initialize opLenCache */
	{
		unsigned char dummycstr[2] = {
			0, 0
		};
		int i;
		for (i = 0; i < 256; i++) {
			dummycstr[0] = (unsigned char)i;
			h->opLenCache[i] = (unsigned char)t2oplen(dummycstr);
		}
	}

	/* Determine type of FontSet */
	h->singleton = h->nFonts == 1 && !(h->fonts[0].flags & SUBR_FONT_CID);

	/* Add fonts' charstring data to CDAWG */
	iFont = 0;
    for (i = 0; i < h->nFonts; i++) {
		addFont(h, &h->fonts[i], iFont, (h->nFonts > 1) || (h->fonts[i].flags & SUBR_FONT_CID));
		iFont += (h->fonts[i].flags & SUBR_FONT_CID) ? h->fonts[i].fdCount : 1;
	}

	selectCandSubrs(h); /* Select candidate subrs */
    buildSubrMatchTrie(h);
	setSubrActCount(h); /* Set subr actual call counts */
#if DB_TEST_STRING
	return;
#endif
	assocSubrs(h);      /* Associate subrs with a font */
	sortInfSubrs(h);    /* Sort inferior subrs by saving */

    if (h->g->flags & CFW_NO_FUTILE_SUBRS)
        preflight = MAX_PREFLIGHT;  /* Preflight until all subrs become actually used at least twice */
    else
        preflight = 0;
    while (preflight >= 0) {

        if (h->singleton) {
          /* Single non-CID font */

          /* Build temporary array from full subr list */
          dnaSET_CNT(h->tmp, h->subrs.cnt);
          for (i = 0; i < h->subrs.cnt; i++) {
            h->tmp.array[i] = &h->subrs.array[i];
          }

          h->subrStackOvl = 0;
          selectFinalSubrSet(h, 0);
          resetSubrCount(h, 0);

          if (h->subrStackOvl) {
            cfwMessage(h->g, "subr stack depth exceeded (reduced)");
          }

          if (h->tmp.cnt >= 215) {
            /* Temporarily make local subrs from odd indexes for renumbering */
            reorderCombined(h, 1);
            /* Make global subrs from even indexes */
            reorderCombined(h, 0);
            addSubrs(h, &h->gsubrs, NODE_GLOBAL, preflight);

            /* Make local subrs from odd indexes */
            reorderCombined(h, 1);
            addSubrs(h, &h->fonts[0].subrs, 0, preflight);
          }
          else {
            reorderSubrs(h);
            addSubrs(h, &h->fonts[0].subrs, 0, preflight);
          }
          subrizeChars(h, &h->fonts[0].chars, 0, preflight);
        }
        else {
          /* Multiple fonts or single CID font */

          /* Build global subrs */
          buildSubrs(h, &h->gsubrs, NODE_GLOBAL, preflight);

          /* Find and add local subrs to each font */
          iFont = 0;
          for (i = 0; i < h->nFonts; i++) {
            subr_Font *font = &h->fonts[i];

            h->subrStackOvl = 0;
            if (font->flags & SUBR_FONT_CID) {
              /* Subrotinize CID-keyed font */
              int j;
              for (j = 0; j < h->fonts[i].fdCount; j++) {
                /* Subroutinize component DICT */
                subr_FDInfo *info = &font->fdInfo[j];

                buildSubrs(h, &info->subrs, iFont + j, preflight);
                subrizeFDChars(h, &info->chars, font, iFont, j, preflight);
              }
              if (!preflight)
                  joinFDChars(h, font);
              iFont += h->fonts[i].fdCount;
            }
            else {
              if (font->chars.nStrings != 0) {
                /* Subroutinize non-synthetic font */
                buildSubrs(h, &font->subrs, iFont, preflight);
                subrizeChars(h, &font->chars, iFont, preflight);
              }
              iFont++;
            }

            if (h->subrStackOvl) {
              cfwMessage(h->g, "subr stack depth exceeded (reduced)");
            }
          }
        }

        /* Remove any subrs used once or less, if no subrs removed twice consecutively, preflight is over */
        if (preflight > 0) {
          int newfutile = checkFutileSubrs(h);
          if (!futile && !newfutile)
            preflight = 0;
          else
            preflight--;
          futile = newfutile;
        }
        else
          preflight--;
    }

	/* Free original unsubroutinized charstring data */
	for (i = 0; i < h->nFonts; i++) {
		MEM_FREE(g, h->fonts[i].chars.refcopy);
	}

#if EDGE_HASH_STAT
	if (gTotalEdgeLookupCount) {
		printf("hash table statistics -- total lookup: %lld, average table size (dynamic): %.2lf, average fill rate (dynamic): %d%%, average miss per call: %.2lf\n",
		       gTotalEdgeLookupCount, (double)gTotalEdgeTableSize / gTotalEdgeLookupCount,
		       (int)(((double)gTotalEdgeCount / gTotalEdgeTableSize) * 100.0), (double)gTotalEdgeMissCount / gTotalEdgeLookupCount);

		{
			MemInfo *info = &h->nodeBlks;
			MemBlk *pblk = info->head;
			long long totalTableSize = 0;
			long long totalEdgeCount = 0;
			long long nodeCount = 0;
			while (pblk) {
				short i;
				Node *node;
				for (i = 0, node = (Node *)&pblk->array[0]; i < pblk->iNext; i++, node++) {
					if (node->edgeTable) {
						totalTableSize += node->edgeTableSize;
						totalEdgeCount += node->edgeCount;
						nodeCount++;
					}
				}
				pblk = pblk->nextBlk;
			}
			printf("average table size (static): %2lf, average fill rate (static): %d%%\n",
			       (double)totalTableSize / nodeCount, (int)((double)totalEdgeCount / totalTableSize * 100.0));
		}
	}
#endif
}

/* Check new LE subr to see if it is the lowest one seen */
void cfwSeenLEIndex(cfwCtx g, long leIndex) {
	subrCtx h = g->ctx.subr;
	if (leIndex < 0) {
		/* we are being called when processing a SETWV op in an LE. The subr index is the
		   current number of gsubrs. */
		if (h->cube_gsubrs.lowLESubrIndex > h->cube_gsubrs.subOffsets.cnt) {
			h->cube_gsubrs.lowLESubrIndex = h->cube_gsubrs.subOffsets.cnt; /* this should happen just once, with the first setvw seen. */
		}
	}
	else {
		/* we are being called when processing a callgrel op in an LE. The subr index is the
		   biased such that index decrements from the first, but the last index is -107. We can't resolve
		   that to real subr number until we know the number of subrs. */
		if (h->cube_gsubrs.highLEIndex < leIndex) {
			h->cube_gsubrs.highLEIndex = leIndex; /* this should happen just once, with the first callgrel seen. */
		}
	}
}

/* add new cube GSUBR */
void cfwAddCubeGSUBR(cfwCtx g, char *cstr, long length) {
	subrCtx h = g->ctx.subr;
	unsigned char *start;
	Offset *subOffset;
	subOffset = dnaNEXT(h->cube_gsubrs.subOffsets);
	start =  (unsigned char *)dnaEXTEND(h->cube_gsubrs.cstrs, (long)length);
	*subOffset = h->cube_gsubrs.cstrs.cnt;
	memcpy(start, cstr, length);
}

/* Append the library element GSUBR's to the global ones produced by subroutinization. */
void cfwMergeCubeGSUBR(cfwCtx g) {
	subrCtx h = g->ctx.subr;
	int i, j;
	long leIndex = -1;  /* index of the first Library Element in the source font gsubrs */
	long leLen;
	long leOffset;
	long gsLen;
	Offset *newOffsets;
	char *newData;
	subr_CSData *subrs = &h->gsubrs;
	long numSourceGSubrs = h->cube_gsubrs.subOffsets.cnt;

	if (h->cube_gsubrs.lowLESubrIndex > numSourceGSubrs) {
		return; /* aren't any to merge. */
	}
	leIndex = numSourceGSubrs - (108 + h->cube_gsubrs.highLEIndex);
	if (leIndex > h->cube_gsubrs.lowLESubrIndex) {
		leIndex = h->cube_gsubrs.lowLESubrIndex;
	}
	if (subrs->nStrings > 0) {
		gsLen = subrs->offset[subrs->nStrings - 1];
	}
	else {
		gsLen = 0;
	}

	if (leIndex == 0) {
		leOffset = 0;
	}
	else {
		leOffset = h->cube_gsubrs.subOffsets.array[leIndex - 1];
	}
	leLen = h->cube_gsubrs.cstrs.cnt - leOffset;

	/* Allocate and copy charstring data */
	newData = (char *)MEM_NEW(h->g, gsLen + leLen);
	if (gsLen) {
		memcpy(newData, subrs->data, gsLen);
		MEM_FREE(h->g, subrs->data);
	}

	memcpy(&newData[gsLen], &h->cube_gsubrs.cstrs.array[leOffset], leLen);
	subrs->data = newData;

	/* Allocate and copy offsets*/
	newOffsets =  (Offset *)MEM_NEW(h->g, (h->cube_gsubrs.subOffsets.cnt + subrs->nStrings) * sizeof(Offset));
	if (gsLen) {
		memcpy(newOffsets, subrs->offset, (subrs->nStrings * sizeof(Offset)));
		MEM_FREE(h->g, subrs->offset);
	}

	/*update offsets*/
	i = leIndex;
	j = subrs->nStrings;
	while (i < h->cube_gsubrs.subOffsets.cnt) {
		long offset = h->cube_gsubrs.subOffsets.array[i++];
		newOffsets[j++] = gsLen + (offset - leOffset);
	}

	subrs->offset = newOffsets;
	subrs->nStrings += (unsigned short)(h->cube_gsubrs.subOffsets.cnt - leIndex);
}

/* Compute size of font's subrs */
long cfwSubrSizeLocal(cfwCtx g, subr_CSData *subrs) {
    long size = 0;
    if (g->flags & CFW_WRITE_CFF2) {
        size = (!subrs || subrs->nStrings == 0) ? 0 :
	       INDEX2_SIZE(subrs->nStrings, subrs->offset[subrs->nStrings - 1]);
    }
    else {
        size = (!subrs || subrs->nStrings == 0) ? 0 :
	       INDEX_SIZE(subrs->nStrings, subrs->offset[subrs->nStrings - 1]);
    }
    return size;
}

/* Compute size of global subrs */
long cfwSubrSizeGlobal(cfwCtx g) {
	subrCtx h = g->ctx.subr;
    long size = 0;
    if (g->flags & CFW_WRITE_CFF2) {
        size = (!h || h->gsubrs.nStrings == 0) ?
	       INDEX2_SIZE(0, 0) : INDEX2_SIZE(h->gsubrs.nStrings,
                                         h->gsubrs.offset[h->gsubrs.nStrings - 1]);
    }
    else {
        size = (!h || h->gsubrs.nStrings == 0) ?
	       INDEX_SIZE(0, 0) : INDEX_SIZE(h->gsubrs.nStrings,
                                         h->gsubrs.offset[h->gsubrs.nStrings - 1]);
    }
    return size;
}

/* Write subrs */
static void subrWrite(cfwCtx g, subr_CSData *subrs) {
	long dataSize;
	INDEXHdr header;
	unsigned i;

	header.count = subrs->nStrings;
    if (g->flags & CFW_WRITE_CFF2) {
        cfwWriteN(g, 4, header.count);
    }
    else {
        cfwWrite2(g, header.count);
    }

	if (header.count == 0) {
		return; /* Empty table just has zero count */
	}
	dataSize = subrs->offset[subrs->nStrings - 1];
	header.offSize = INDEX_OFF_SIZE(dataSize);
	cfwWrite1(g, header.offSize);

	/* Write offset array */
	cfwWriteN(g, header.offSize, 1);
	for (i = 0; i < subrs->nStrings; i++) {
		cfwWriteN(g, header.offSize, subrs->offset[i] + 1);
	}

	/* Write charstring data */
	cfwWrite(g, dataSize, subrs->data);
}

/* Write local subrs */
void cfwSubrWriteLocal(cfwCtx g, subr_CSData *subrs) {
	if (subrs && subrs->nStrings != 0) {
		subrWrite(g, subrs);
	}
}

/* Write global subrs */
void cfwSubrWriteGlobal(cfwCtx g) {
	subrCtx h = g->ctx.subr;
	if (h != NULL) {
		subrWrite(g, &h->gsubrs);
	}
	else {
		/* Write empty global subr INDEX */
		INDEXHdr header;
		header.count = 0;
        if (g->flags & CFW_WRITE_CFF2) {
            cfwWriteN(g, 4, header.count);
        }
        else {
            cfwWrite2(g, header.count);
        }
	}
}

#if TC_DEBUG
/* --------------------------------- DEBUG --------------------------------- */
#include <ctype.h>

static long dbnodeid(subrCtx h, Node *node) {
	MemBlk *nb;

	if (node == NULL) {
		return -1;
	}

	for (nb = h->nodeBlks.head; nb != NULL; nb = nb->nextBlk) {
		if ((Node *)nb->array <= node &&
		    (Node *)nb->array + NODES_PER_BLK > node) {
			long blks = node - (Node *)nb->array;
			for (nb = nb->nextBlk; nb != NULL; nb = nb->nextBlk) {
				blks += NODES_PER_BLK;
			}
			return blks;
		}
	}

	printf("can't find node!");
	return 0;
}

static void dbop(int length, unsigned char *cstr) {
	int i;
	for (i = 0; i < length; i++) {
		printf("%c%c",
		       "0123456789abcdef"[cstr[i] >> 4],
		       "0123456789abcdef"[cstr[i] & 0xf]);
	}
}

static char *dbinfo(subrCtx h, Subr *subr, int mark) {
	static char buf[16];
	sprintf(buf, "%hu.%hu%c%d", subr->count, subr->length - subr->maskcnt,
	        mark, subrSaved(h, subr));
	return buf;
}

static void dbsubr(subrCtx h, unsigned iSubr, int c, unsigned offset) {
	Subr *subr = &h->subrs.array[iSubr];
	int mark = (subr->node->id == NODE_GLOBAL) ? '~' : '=';
	if (c == '#') {
		/* A regular subr rather than an inf or sup */
		if (!(subr->flags & SUBR_SELECT)) {
			c = '.';
		}
	}
	else if (subr->flags & SUBR_SELECT) {
		c = toupper(c);
	}
	printf("%-9s%-*c", dbinfo(h, subr, mark), offset * 2 + 3, c);
	dbcstr(h, subr->length, subr->cstr);
	printf(" [%hu]\n", iSubr);
}

static void dbnodeProc(subrCtx h, Edge *edge, long misc, long param2) {
	if (!edge || !edge->label) {
		return;
	}
	printf("  %6ld (%08lx) %8s ",
	       dbnodeid(h, edge->son), (unsigned long)edge->son,
	       (misc + OPLEN(h, edge->label) !=
	        edge->son->misc) ? "shortcut" : "-");
	dbop(OPLEN(h, edge->label), edge->label);
	printf(" (%08lx)\n", (unsigned long)edge);
}

static void dbnode(subrCtx h, Node *node) {
	printf("--- node[%ld]\n", dbnodeid(h, node));
	printf("suffix=%ld (%08lx)\n",
	       dbnodeid(h, node->suffix), (unsigned long)node->suffix);
	printf("misc  =%ld\n", node->misc);
	printf("paths =%hu\n", node->paths);
	printf("id    =%hu\n", node->id);
	printf("flags =%04hx (", (unsigned short)node->flags);

	if (node->flags == 0) {
		printf("none");
	}
	else {
		char *sep = "";
		if (node->flags & NODE_COUNTED) {
			printf("%sCOUNTED", sep);
			sep = ",";
		}
		if (node->flags & NODE_SUBR) {
			printf("%sSUBR", sep);
			sep = ",";
		}
	}
	printf(")\n");

	printf("edges:\n");
	if (node->edgeTable == NULL) {
		printf("   none\n");
	}
	else {
		walkEdgeTable(h, node, dbnodeProc, node->misc, 0);
	}
}

static void dbcstr(subrCtx h, unsigned length, unsigned char *cstr) {
	unsigned char *end = cstr + length;
#if DB_TEST_STRING
	unsigned char *start = cstr;
#endif
	while (cstr < end) {
		unsigned i;

		length = OPLEN(h, cstr);
		for (i = 0; i < length; i++)
#if DB_TEST_STRING
		{
			printf("%c%c", cstr == start ? '"' : ' ', cstr[i]);
		}
#else
		{
			printf("%c%c",
			       "0123456789abcdef"[cstr[i] >> 4],
			       "0123456789abcdef"[cstr[i] & 0xf]);
		}
#endif

		cstr += length;
	}
#if DB_TEST_STRING
	printf("\"");
#endif
}

static void dbcstrs(subrCtx h, unsigned char *cstr, unsigned char *end,
                    int index) {
	int startchar = 1;
	int gid = 0;

	printf("--- glyphs (total=%ld)\n", (long)(end - cstr));

	while (cstr < end) {
		int length = OPLEN(h, cstr);

		if (startchar) {
			printf("%2d:%3d ", index, gid);
			startchar = 0;
			gid++;
		}

		dbop(length, cstr);

		if (cstr[0] == SEPARATOR) {
			startchar = 1;
			printf("\n");
		}

		cstr += length;
	}
}

static void dbgroups(subrCtx h) {
	long i;

	printf("--- social groups\n");

	for (i = 0; i < h->leaders.cnt; i++) {
		Subr *subr;

		printf("--- link group[%ld]\n", i);
		for (subr = h->leaders.array[i]; subr != NULL; subr = subr->next) {
			Link *link;
			unsigned offset;
			int iSubr =  subr - h->subrs.array;

			/* Find max superior offset */
			offset = 0;
			for (link = subr->sups; link != NULL; link = link->next) {
				if (offset < link->offset) {
					offset = link->offset;
				}
			}

			dbsubr(h, iSubr, '#', offset);

			/* Print inferiors */
			for (link = subr->infs; link != NULL; link = link->next) {
				dbsubr(h, link->subr - h->subrs.array, 'i',
				       offset + link->offset);
			}

			/* Print superiors */
			for (link = subr->sups; link != NULL; link = link->next) {
				dbsubr(h, link->subr - h->subrs.array, 's',
				       offset - link->offset);
			}
		}
	}
}

/* This function just serves to suppress annoying "defined but not used"
   compiler messages when debugging */
static void CTL_CDECL dbuse(int arg, ...) {
	dbuse(0, dbnode, dbcstrs, dbsubr, dbgroups);
}

#endif /* TC_DEBUG */
