/* @(#)CM_VerSion subr.c atm08 1.2 16245.eco sum= 61088 atm08.002 */
/* @(#)CM_VerSion subr.c atm07 1.2 16164.eco sum= 34442 atm07.012 */
/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

/* Temporary debug control */
#define DB_FONT     0   /* Font input charstrings */
#define DB_INFS     0   /* Inferior subr selection */
#define DB_OVLPS    0   /* Overlap handling */
#define DB_RELNS    0   /* Subr relationship building */
#define DB_SELECT   0   /* Subr selection */
#define DB_GROUPS   0   /* Selected groups */
#define DB_CHARS    0   /* Chars with selected subrs */
#define DB_SUBRS    0   /* Subrs with selected subrs */
#define DB_CALLS    0   /* Call counts */

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
   DAWG using the concatenation of all the charstrings from all the fonts as
   input. Subseqeuntly the completed suffix DAWG is traversed in order to count

   The suffix DAWG is built using an online algorithm described in "Text
   Algorithms, Maxime Crochemore and Wojciech Ryter, OUP" p.113.

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

#include "subr.h"
#include "parse.h"

#include <limits.h>

#include <stdio.h>
#include <stdlib.h>

#if PLAT_SUN4
#include "sun4/fixstring.h"
#else /* PLAT_SUN4 */
#include <string.h>
#endif  /* PLAT_SUN4 */
#include "dynarr.h"

#if TC_SUBR_SUPPORT
/* ------------------------------- DAWG data ------------------------------- */
typedef struct Edge_ Edge;
typedef struct Node_ Node;
struct Edge_ {
	unsigned char *label;
	Node *son;          /* Son node */
	Edge *next;         /* Next edge in list */
};
struct Node_ {
	Node *suffix;       /* Suffix link */
	Edge edge;          /* First linking edge */
	long misc;          /* Initially longest path from root, then subr index */
	unsigned short paths;   /* Paths through node */
	unsigned short id;  /* Font id */
#define NODE_GLOBAL USHRT_MAX   /* Identifies global subr */
	short flags;        /* Status flags */
#define NODE_COUNTED    (1 << 15) /* Paths have been counted for this node */
#define NODE_TESTED     (1 << 14) /* Candidacy tested for this node */
#define NODE_FAIL       (1 << 13) /* Node failed candidacy test */
#define NODE_TAIL       (1 << 12) /* Tail subr (terminates with endchar) */
#define NODE_SUBR       (1 << 11) /* Node has subr info (index in misc) */
};

/* ------------------------------- Subr data ------------------------------- */
typedef struct Subr_ Subr;
typedef struct Link_ Link;
struct Subr_ {
	Node *node;             /* Associated node */
	Link *sups;             /* Superior subrs */
	Link *infs;             /* Inferior subrs */
	Subr *next;             /* Next member of social group */
	unsigned char *cstr;    /* Charstring */
	unsigned short length;  /* Subr length (original bytes spanned) */
	unsigned short count;   /* Occurance count */
	short deltacnt;         /* Delta count */
	short deltalen;         /* Delta length */
	short subrnum;          /* Biased subr number */
	short numsize;          /* Size of subr number (1, 2, or 3 bytes) */
	short maskcnt;          /* hint/cntrmask count */
	short flags;            /* Status flags */
#define SUBR_SELECT     (1 << 0)  /* Flags subr selected */
#define SUBR_REJECT     (1 << 1)  /* Flags subr rejected */
#define SUBR_MEMBER     (1 << 2)  /* Flags subr added to social group */
#define SUBR_REDUCE     (1 << 3)  /* Subr has had count reduced (transient) */
#define SUBR_OVERLAP    (1 << 4)  /* Marks overlapped and reduced subr */
#if DB_CALLS
	short calls;    /* xxx remove */
#endif
};

#define SUBR_MARKED (SUBR_SELECT | SUBR_REJECT) /* Flags if subr marked */

struct Link_ {              /* Social group link */
	Subr *subr;             /* Superior/inferior subr */
	Link *next;             /* Next record */
	unsigned short offset;  /* Offset within superior/inferior */
};

typedef struct {            /* Subr call within charstring */
	Subr *subr;             /* Inferior subr */
	unsigned short offset;  /* Offset within charstring */
} Call;

typedef struct MemBlk_ MemBlk;
struct MemBlk_ {        /* Generalized memory block for object allocation */
	MemBlk *nextBlk;    /* Next memory block in chain */
	char *array;        /* Object array */
	short iNext;        /* Next free element index */
};

typedef struct {
	MemBlk *head;       /* Allocated block list */
	MemBlk *free;       /* Free block list */
} MemInfo;

#define NODES_PER_BLK   5000
#define EDGES_PER_BLK   5000
#define LINKS_PER_BLK   1000

/* Subroutinization context */
struct subrCtx_ {
	MemInfo nodeBlks;       /* Node blocks */
	MemInfo edgeBlks;       /* Edge blocks */
	MemInfo linkBlks;       /* Relation blocks */

	Node *root;             /* DAWG root */
	Node *sink;             /* DAWG sink */

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
	SubrParseData *spd;     /* Subr parsing data */

	CSData gsubrs;          /* Global subrs */

	short nFonts;           /* Font count */
	Font *fonts;            /* Font list */

	tcCtx g;                /* Package context */
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
static void dbcstrs(subrCtx h, unsigned char *cstr, unsigned char *end, int index);
static void dbgroups(subrCtx h);

#endif /* TC_DEBUG */

/* --------------------------- Object Management --------------------------- */

/* Return new memory block based object */
static void *newObject(subrCtx h, MemInfo *info, long size, long count) {
	MemBlk *pblk = info->head;
	if (pblk == NULL || pblk->iNext == count) {
		/* Re/allocate new block */
		MemBlk *new;
		if (info->free != NULL) {
			/* Remove block from free list */
			new = info->free;
			info->free = new->nextBlk;
		}
		else {
			/* Allocate new block from heap */
			new = MEM_NEW(h->g, sizeof(MemBlk));
			new->array = MEM_NEW(h->g,  size * count);
		}
		new->nextBlk = pblk;
		new->iNext = 0;
		info->head = pblk = new;
	}
	/* Return next object */
	return &pblk->array[pblk->iNext++ *size];
}

/* Copy objects to free list */
static void reuseObjects(tcCtx g, MemInfo *info) {
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
static void freeObjects(tcCtx g, MemInfo *info) {
	MemBlk *pblk;
	MemBlk *next;
	for (pblk = info->free; pblk != NULL; pblk = next) {
		next = pblk->nextBlk;
		MEM_FREE(g, pblk->array);
		MEM_FREE(g, pblk);
	}
	info->free = NULL;
}

/* Create and initialize new DAWG node */
static Node *newNode(subrCtx h, long length, unsigned id) {
	Node *node = newObject(h, &h->nodeBlks, sizeof(Node), NODES_PER_BLK);
	node->edge.son = NULL;  /* Used to signal uninitialized first edge */
	node->edge.next = NULL;
	node->misc = length;
	node->paths = 0;
	node->id = id;
	node->flags = 0;
	return node;
}

/* Create and initialize new DAWG edge */
static Edge *newEdge(subrCtx h, unsigned char *label, Node *son, Edge *next) {
	Edge *edge = newObject(h, &h->edgeBlks, sizeof(Edge), EDGES_PER_BLK);
	edge->label = label;
	edge->son = son;
	edge->next = next;
	return edge;
}

/* Create and initialize new subr link */
static Link *newLink(subrCtx h, Subr *subr, unsigned offset, Link *next) {
	Link *link = newObject(h, &h->linkBlks, sizeof(Link), LINKS_PER_BLK);
	link->subr = subr;
	link->next = next;
	link->offset = offset;
	return link;
}

/* --------------------------- Context Management -------------------------- */

/* Initialize module */
void subrNew(tcCtx g) {
	subrCtx h = MEM_NEW(g, sizeof(struct subrCtx_));

	h->nodeBlks.head = h->nodeBlks.free = NULL;
	h->edgeBlks.head = h->edgeBlks.free = NULL;
	h->linkBlks.head = h->linkBlks.free = NULL;

	h->root = NULL;

	/* xxx tune these parameters */
	dnaINIT(g->ctx.dnaCtx, h->subrs, 500, 1000);
	dnaINIT(g->ctx.dnaCtx, h->tmp, 500, 1000);
	dnaINIT(g->ctx.dnaCtx, h->reorder, 500, 1000);
	dnaINIT(g->ctx.dnaCtx, h->calls, 10, 10);
	dnaINIT(g->ctx.dnaCtx, h->members, 40, 40);
	dnaINIT(g->ctx.dnaCtx, h->leaders, 100, 200);
	dnaINIT(g->ctx.dnaCtx, h->cstrs, 5000, 2000);

	h->offSize = 2;

	h->gsubrs.nStrings = 0;
	h->gsubrs.offset = NULL;

	ctx = h;    /* xxx temporary */

	/* Link contexts */
	h->g = g;
	g->ctx.subr = h;
}

/* Prepare module for reuse */
void subrReuse(tcCtx g) {
	subrCtx h = g->ctx.subr;

	reuseObjects(g, &h->nodeBlks);
	reuseObjects(g, &h->edgeBlks);
	reuseObjects(g, &h->linkBlks);

	csFreeData(g, &h->gsubrs);

	h->root = NULL;
	h->offSize = 2;
}

/* Free resources */
void subrFree(tcCtx g) {
	subrCtx h = g->ctx.subr;

	freeObjects(g, &h->nodeBlks);
	freeObjects(g, &h->edgeBlks);
	freeObjects(g, &h->linkBlks);

	dnaFREE(h->subrs);
	dnaFREE(h->tmp);
	dnaFREE(h->reorder);
	dnaFREE(h->calls);
	dnaFREE(h->members);
	dnaFREE(h->leaders);
	dnaFREE(h->cstrs);

	MEM_FREE(g, h);
}

/* --------------------------- DAWG Construction --------------------------- */

/* Compare two edge labels */
static int labelcmp(subrCtx h,
                    int length1, unsigned char *label1, unsigned char *label2) {
	int cmp = *label1++ - *label2;
	if (cmp != 0) {
		return cmp; /* First byte differs */
	}
	else {
		int length2 = h->spd->oplen(label2++);
		int length = (length1 < length2) ? length1 : length2;

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

/* Add edge to between father and son nodes */
static void addEdge(subrCtx h, Node *father, Node *son,
                    unsigned length, unsigned char *label) {
	Edge *edge = &father->edge;
	if (edge->son == NULL) {
		/* Add first edge to node */
		edge->label = label;
		edge->son = son;
		edge->next = NULL;
	}
	else {
		/* Find insertion position in edge list */
		Edge *prev = NULL;
		do {
			if (labelcmp(h, length, label, edge->label) < 0) {
				if (prev == NULL) {
					/* Insert at head of list */
					Edge *newedge =
					    newEdge(h, edge->label, edge->son, edge->next);
					edge->label = label;
					edge->son = son;
					edge->next = newedge;
					return;
				}
				else {
					break;  /* Insert in middle of list */
				}
			}

			prev = edge;
			edge = edge->next;
		}
		while (edge != NULL);

		/* Insert in middle (from break) or end of list (from while) */
		prev->next = newEdge(h, label, son, edge);
	}
}

/* Find linking edge from node with label */
static Edge *findEdge(subrCtx h,
                      Node *node, unsigned length, unsigned char *label) {
	Edge *edge = &node->edge;
	do {
		int cmp = labelcmp(h, length, label, edge->label);
		if (cmp < 0) {
			return NULL;
		}
		else if (cmp == 0) {
			return edge;    /* Matched edge */
		}
		edge = edge->next;
	}
	while (edge != NULL);
	return NULL;
}

/* Append font's charstring data to DAWG. This construction algorithm closely
   follows the one presented in "Text Algorithms, Maxime Crochemore and
   Wojciech Ryter, OUP" p.113 although this one also adds code the identify
   nodes in the DAWG with a particular font or as global nodes if they
   terminate charstrings that appear in multiple fonts */
static void addFont(subrCtx h, Font *font, unsigned iFont) {
	unsigned char *a;
	unsigned char *pend;
	unsigned char *pfd;
	unsigned id;

	if (font->chars.nStrings == 0) {
		return; /* Synthetic font */
	}
	a = (unsigned char *)font->chars.data;
	pend = a + font->chars.offset[font->chars.nStrings - 1];

	if (font->flags & FONT_CID) {
		pfd = font->fdIndex;
		id = iFont + *pfd++;
	}
	else {
		pfd = NULL; /* Suppress optimizer warning */
		id = iFont;
	}

	if (h->root == NULL) {
		h->sink = h->root = newNode(h, 0, id);
		h->root->suffix = NULL;
	}

#if DB_FONT
	dbcstrs(h, a, end, iFont);
#endif

	/* Extend path (token by token) */
	while (a < pend) {
		Edge *edge = NULL;  /* Suppress optimizer warning */
		Node *w;
		int length = h->spd->oplen(a);
		Node *newsink = newNode(h, h->sink->misc + length, id);

		/* Add solid a-edge (sink, newsink) */
		addEdge(h, h->sink, newsink, length, a);

		for (w = h->sink->suffix; w != NULL; w = w->suffix) {
			edge = findEdge(h, w, length, a);
			if (edge != NULL) {
				break;
			}

			/* Make non-solid a-edge (w, newsink) */
			addEdge(h, w, newsink, length, a);
		}

		if (w == NULL) {
			newsink->suffix = h->root;
		}
		else {
			Node *n;
			Node *v = edge->son;

			for (n = v->suffix; n != NULL; n = n->suffix) {
				if (n->id != id) {
					n->id = NODE_GLOBAL;
				}
			}

			if (w->misc + length == v->misc) {
				/* Edge (w, v) solid */
				newsink->suffix = v;
				if (v->id != id) {
					v->id = NODE_GLOBAL;
				}
			}
			else {
				/* Split node v */
				Node *newnode;
				Edge *newedge;

				/* Create new node */
				newnode = newNode(h, w->misc + length,
				                  (v->id != id) ? NODE_GLOBAL : id);

				/* Change (w, v) into solid edge (w, newnode) */
				edge->son = newnode;

				/* Copy first edge as shortcut */
				edge = &v->edge;
				newedge = &newnode->edge;
				newedge->label = edge->label;
				newedge->son = edge->son;

				/* Copy remaining edges as shortcuts */
				for (edge = edge->next; edge != NULL; edge = edge->next) {
					newedge = newedge->next =
					        newEdge(h, edge->label, edge->son, NULL);
				}

				/* Update suffix links */
				newsink->suffix = newnode;
				newnode->suffix = v->suffix;
				v->suffix = newnode;

				for (w = w->suffix; w != NULL; w = w->suffix) {
					edge = findEdge(h, w, length, a);
					if (edge == NULL || edge->son != v ||
					    w->misc + length == v->misc) {
						break;
					}

					/* Redirect non-solid edge to newnode */
					edge->son = newnode;
				}
			}
		}

		if (font->flags & FONT_CID &&
		    a[0] == h->spd->separator &&
		    a + length < pend) {
			/* Change id for CID font on charstring boundary */
			id = iFont + *pfd++;
		}

		h->sink = newsink;
		a += length;
	}
}

/* ----------------------- Candidate Subr Selection ------------------------ */

/* Count paths running through each node */
static unsigned countPaths(subrCtx h, Edge *edge) {
	Node *node = edge->son;

	if (!(node->flags & NODE_COUNTED) && node != h->sink) {
		/* Count descendent paths */
		Node *next;
		Node *start = node;
		long count = node->paths;

		/* Optimize recursion; skip over simple nodes in the path */
		for (next = node;
		     next != NULL && next->edge.next == NULL && next->paths == 0;
		     next = node->edge.son) {
			node = next;
		}

		/* Recursively descend complex node */
		edge = &node->edge;
		do {
			count += countPaths(h, edge);
		}
		while ((edge = edge->next) != NULL);

		/* Update node */
		node->paths = (unsigned short)((count > USHRT_MAX) ? USHRT_MAX : count);
		node->flags |= NODE_COUNTED;

		/* Update skipped nodes */
		for (; start != node; start = start->edge.son) {
			start->paths = node->paths;
			start->flags |= NODE_COUNTED;
		}
	}
	return node->paths;
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
static void saveSubr(subrCtx h, unsigned char *label, Node *node,
                     int maskcnt, int tail) {
	Subr *subr;
	unsigned count = node->paths;

	node->flags |= NODE_FAIL;   /* Assume test will fail and mark node */

	/* Test for candidacy */
	switch (node->misc - maskcnt) {
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
	subr->cstr = label + h->spd->oplen(label) - node->misc;
	subr->length = (unsigned short)node->misc;
	subr->count = count;
	subr->deltacnt = 0;
	subr->deltalen = 0;
	subr->numsize = 1;
	subr->maskcnt = maskcnt;
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

/* Find candidate subrs in DAWG */
static void findCandSubrs(subrCtx h, Edge *edge, int maskcnt) {
	long misc;
	Node *node;
	unsigned char *label;

	for (;; ) {
		node = edge->son;

		if (node->flags & NODE_TESTED || node->paths == 1) {
			return;
		}
		node->flags |= NODE_TESTED;

		if (edge->label[0] == h->spd->endchar) {
			if (node->paths > 1) {
				saveSubr(h, edge->label, node, maskcnt, 1);
			}
			return;
		}
		else if (edge->label[0] == h->spd->hintmask ||
		         edge->label[0] == h->spd->cntrmask) {
			maskcnt++;
		}

		misc = node->misc;
		label = edge->label;
		edge = &node->edge;
		if (edge->next != NULL) {
			goto complex;
		}
		else if (node->paths > edge->son->paths) {
			saveSubr(h, label, node, maskcnt, 0);
		}
	}

complex:
	saveSubr(h, label, node, maskcnt, 0);
	do {
		if (misc + h->spd->oplen(edge->label) == edge->son->misc) {
			/* Descend solid edge */
			findCandSubrs(h, edge, maskcnt);
		}
	}
	while ((edge = edge->next) != NULL);
}

/* Find insertion postion (ordered by offset) in call list */
static Call *findInsertPosn(subrCtx h, unsigned offset) {
	if (h->calls.cnt == 0) {
		return dnaNEXT(h->calls);   /* Empty list; first element */
	}
	else {
		int i = h->calls.cnt - 1;
		Call *call = &h->calls.array[i];

		if (offset > call->offset) {
			return dnaNEXT(h->calls);   /* Not in list; add to end */
		}
		for (;; ) {
			if (offset == call->offset) {
				/* Found call with matching offset; return it */
				return call;
			}
			else if (--i < 0 ||
			         (call = &h->calls.array[i], offset > call->offset)) {
				/* Insert at [i+1] */
				call = &dnaGROW(h->calls, h->calls.cnt)[i + 1];
				COPY(call + 1, call, h->calls.cnt++ - (i + 1));
				return call;
			}
		}
	}
}

/* Find inferior subrs in charstring and build call list in offset order */
static void findInfSubrs(subrCtx h, unsigned length, unsigned char *pstart,
                         int selfMatch) {
	Node *node = h->root;
	unsigned char *pstr = pstart;
	unsigned char *pend = pstart + length;

	h->calls.cnt = 0;
	do {
		Node *suffix;
		Call *call;
		Subr *subr;
		unsigned offset;
		unsigned oplength = h->spd->oplen(pstr);
		node = findEdge(h, node, oplength, pstr)->son;
		pstr += oplength;

#if 0
		{
			int depth = (node->flags & NODE_SUBR) ?
			    h->subrs.array[node->misc].length : node->misc;
			if (depth != pstr - pstart) {
				printf("@@@ depth mismatch %d %d\n", depth, pstr - pstart);
			}
		}
#endif

		if (node->flags & NODE_SUBR && (selfMatch || pstr != pend)) {
			/* In-path subr; reset accumulator */
			h->calls.cnt = 0;
			subr = &h->subrs.array[node->misc];
			offset = 0;
			goto save;
		}

		for (suffix = node->suffix; suffix != NULL; suffix = suffix->suffix) {
			if (suffix->flags & NODE_SUBR) {
				/* Suffix subr; compute offset */
				subr = &h->subrs.array[suffix->misc];
				offset = pstr - pstart - subr->length;
				goto save;
			}
			else if (!(suffix->flags & NODE_FAIL)) {
				break;  /* Can't be another subr on suffix chain */
			}
		}
		continue;

		/* Save/update record */
save:
		call = findInsertPosn(h, offset);
		call->subr = subr;
		call->offset = offset;
	}
	while (pstr < pend);

#if DB_INFS
	{
		int i;
		for (i = 0; i < h->calls.cnt; i++) {
			Call *call = &h->calls.array[i];
			dbsubr(h, call->subr - h->subrs.array, 'i', call->offset);
		}
	}
#endif
}

/* Reduce overlapped subr and reduce inferiors that also overlap */
static void reduceSubr(subrCtx h, Subr *subr, unsigned offset, unsigned count) {
	Link *inf;

	/* Update subr */
	if (subr->count < count) {
#if DB_OVLPS
		printf("--- negative count\n");
#endif
		subr->count = 0;
	}
	else {
		subr->count -= count;
	}
	subr->flags |= SUBR_OVERLAP;

#if DB_OVLPS
	printf("reduce subr (offset=%u,count=%u)\n", offset, count);
	dbsubr(h, subr - h->subrs.array, 'r', 0);
#endif

#if 0
	/* Check for inferior that ends on offset */
	for (inf = subr->infs; inf != NULL; inf = inf->next) {
		if (inf->offset + inf->subr->length == offset) {
			return; /* Inferior subrs reduced on this offset elsewhere */
		}
		else if (inf->offset > offset) {
			break;
		}
	}
#endif

	/* Reduce inferiors */
	for (inf = subr->infs; inf != NULL; inf = inf->next) {
		if (inf->offset >= offset) {
			break;
		}
		else if (inf->offset + inf->subr->length > (unsigned short)offset) {
			reduceSubr(h, inf->subr, offset - inf->offset, count);
		}
	}
}

/* Calculate byte savings for this subr. See candSubr() for details */
static long subrSaved(subrCtx h, Subr *subr) {
	unsigned length = subr->length - subr->maskcnt;
	return subr->count * (length - CALL_OP_SIZE - subr->numsize) -
	       (h->offSize + length + ((subr->node->flags & NODE_TAIL) == 0));
}

/* Handle inferior subr overlap */
static void handleOverlap(subrCtx h, Call *first, Call *second, int supcount) {
	Subr *initial = first->subr;    /* Initial (first) overlapping subr */
	Subr *final = second->subr;     /* Final (second) overlapping subr */

#if DB_OVLPS
	printf("--- overlap (supcount=%d)\n", supcount);
	dbsubr(h, first->subr - h->subrs.array, 'o', first->offset);
	dbsubr(h, second->subr - h->subrs.array, 'o', second->offset);
#endif

	if (subrSaved(h, initial) < subrSaved(h, final)) {
		if (!(initial->flags & SUBR_REDUCE)) {
			/* Reduce initial and its inferiors in the overlap region */
			reduceSubr(h, initial, second->offset - first->offset, supcount);
			initial->flags |= SUBR_REDUCE;
		}
	}
	else {
		if (!(final->flags & SUBR_REDUCE)) {
			/* Reduce final and its inferiors in the overlap region */
			reduceSubr(h, final, initial->length -
			           (second->offset - first->offset), supcount);
			final->flags |= SUBR_REDUCE;
		}
	}
}

/* Check for and handle inferior subr overlap */
static void checkOverlap(subrCtx h, int supcount) {
	int i;
	int overlap = 0;

	for (i = 0; i < h->calls.cnt - 1; i++) {
		int j;
		Call *first = &h->calls.array[i];
		unsigned nextfree = first->offset + first->subr->length;

		for (j = i + 1; j < h->calls.cnt; j++) {
			Call *second = &h->calls.array[j];
			if (nextfree > second->offset) {
				/* Found overlap; edit subrs in conflict */
				handleOverlap(h, first, second, supcount);
				overlap = 1;
			}
			else {
				break;
			}
		}
	}

	if (overlap) {
		/* Remove reduced attribute from overlapped subrs */
		for (i = 0; i < h->calls.cnt; i++) {
			h->calls.array[i].subr->flags &= ~SUBR_REDUCE;
		}
	}
}

/* Scan subrs and find inferiors to each subr */
static void addSubrRelns(subrCtx h) {
	long i;

#if DB_RELNS
	printf("--- subrs (subrs marked with -)\n");
#endif

	for (i = 0; i < h->subrs.cnt; i++) {
		int j;
		Link *infs;
		Subr *subr = &h->subrs.array[i];

#if DB_RELNS
		dbsubr(h, i, '-', 0);
#endif

		findInfSubrs(h, subr->length, subr->cstr, 0);

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

#if DB_OVLPS
	printf("--- subr overlaps\n");
#endif

	/* Check all subrs for overlap */
	for (i = 0; i < h->subrs.cnt; i++) {
		int j;
		Link *inf;
		Subr *subr = &h->subrs.array[i];

		/* Build inferior subr call array */
		j = 0;
		for (inf = subr->infs; inf != NULL; inf = inf->next) {
			Call *call = &h->calls.array[j++];
			call->subr = inf->subr;
			call->offset = inf->offset;
		}
		if (j > 1) {
			h->calls.cnt = j;
			checkOverlap(h, subr->count);
		}
	}

	/* Reorder inferiors by savings (largest first) */
	for (i = 0; i < h->subrs.cnt; i++) {
		Subr *subr = &h->subrs.array[i];
		Link *head; /* Head if inferior list */

		/* Sort list in-place */
		for (head = subr->infs; head != NULL; head = head->next) {
			Link *try = head->next;
			if (try == NULL) {
				break;  /* End of list */
			}
			else {
				Link *best = NULL;
				int max = subrSaved(h, head->subr);

				/* Find best inferior subr in remainder of list */
				do {
					int saved = subrSaved(h, try->subr);
					if (saved > max) {
						best = try;
						max = saved;
					}
					try = try->next;
				}
				while (try != NULL);

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

#if 0
/* Experimental dawg traversal that may be much more efficient than tracing
   charstrings through the dawg when finished. */
static void xfindSubrRelns(subrCtx h, Edge *edge, long initdepth) {
	Node *node;
	long depth;
	Node *son = edge->son;
	long sondepth = (son->flags & NODE_SUBR) ?
	    h->subrs.array[son->misc].length : son->misc;

	while (son != h->sink) {
#if 0
		printf("%ld:%ld-%d ", initdepth, node->misc, oplen(edge->label));
		dbcstr(h, node->misc - initdepth,
		       edge->label + oplen(edge->label) - (node->misc - initdepth));
		printf("\n");
#endif
		node = son;
		depth = sondepth;

		if (node->flags & NODE_SUBR) {
			dbsubr(h, node->misc, 'n', 0);
		}
		else {
			Node *suf;

			for (suf = node->suffix; suf != NULL; suf = suf->suffix) {
				if (suf->flags & NODE_SUBR) {
					/* Suffix subr */
					/*dbsubr(h, suf->misc, 'x', 0);*/
					break;
				}
				else if (!(suf->flags & NODE_FAIL)) {
					break;  /* Can't be another subr on suffix chain */
				}
			}
		}

#if 1
		if (edge->label[0] == tx_endchar) {
			printf("%ld: ", initdepth);
			dbcstr(h, node->misc - initdepth,
			       edge->label + 1 - (node->misc - initdepth));
			printf("\n");
			initdepth = node->misc + 4;
			return;
		}
#endif
		edge = &node->edge;

		if (edge->next != NULL) {
			do {
				if (depth + h->spd->oplen(edge->label) ==
				    ((edge->son->flags & NODE_SUBR) ?
				     h->subrs.array[edge->son->misc].length : edge->son->misc)) {
					/* Descend solid edge */
					xfindSubrRelns(h, edge, initdepth);
				}
			}
			while ((edge = edge->next) != NULL);
			return;
		}
		else {
			son = edge->son;
			sondepth = (son->flags & NODE_SUBR) ?
			    h->subrs.array[son->misc].length : son->misc;

			if (depth + h->spd->oplen(edge->label) != sondepth) {
				return;
			}
		}
	}
}

#endif

/* Select candidate subrs */
static void selectCandSubrs(subrCtx h) {
	Node *sink;
	Edge *edge;

	/* Count all path termination nodes by traversing suffix list from sink */
	for (sink = h->sink; sink->suffix != NULL; sink = sink->suffix) {
		sink->paths++;
	}

	/* Count paths */
	for (edge = &h->root->edge; edge != NULL; edge = edge->next) {
		(void)countPaths(h, edge);
	}

	/* Find candidate subrs */
	h->subrs.cnt = 0;
	for (edge = &h->root->edge; edge != NULL; edge = edge->next) {
		if (h->spd->oplen(edge->label) == edge->son->misc) {
			findCandSubrs(h, edge, 0);
		}
	}

#if 0
	printf("--- xfindSubrRelns\n");
	h->tmp.cnt = 0;
	for (edge = &h->root->edge; edge != NULL; edge = edge->next) {
		if (oplen(edge->label) == edge->son->misc) {
			xfindSubrRelns(h, edge, 0);
		}
	}
#endif

	/* Add subr reletionships */
	addSubrRelns(h);
}

/* ----------------------- Associate Subrs With Font ----------------------- */

/* Associate subrs with a font in the FontSet */
static void assocSubrs(subrCtx h) {
	int i;

#if DB_OVLPS
	printf("--- char overlaps\n");
#endif

	for (i = 0; i < h->nFonts; i++) {
		long j;
		long offset;
		Font *font = &h->fonts[i];

		offset = 0;
		for (j = 0; j < font->chars.nStrings; j++) {
			long nextoff = font->chars.offset[j];

#if DB_INFS
			printf("[%3d:%4ld]  ", i, j);
			dbcstr(h, nextoff - offset,
			       (unsigned char *)&font->chars.data[offset]);
			printf("\n");
#endif

			findInfSubrs(h, nextoff - offset,
			             (unsigned char *)&font->chars.data[offset], 1);
			checkOverlap(h, 1);

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
	printf("%ld%c", subrSaved(ctx, subr), c);
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
	dnaINIT(h->g->ctx.dnaCtx, subrStack, 4000, 4000);

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
					fprintf(stderr, "Typecomp Error: List Overflow\n");
					free(list);
					return;
				}
			}
			for (link = subr->infs; link != NULL; link = link->next) {
				/* Add inferior member to temporary list*/
				list[listlength++] = link->subr;
				if (listlength >= LISTSIZE) {
					fprintf(stderr, "Typecomp Error: List Overflow\n");
					free(list);
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
static int CDECL cmpGlobalSetSubrs(const void *first, const void *second) {
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
static int CDECL cmpLocalSetSubrs(const void *first, const void *second) {
	Subr *a = *(Subr **)first;
	Subr *b = *(Subr **)second;
	switch ((a->flags & SUBR_MARKED) << 2 | (b->flags & SUBR_MARKED)) {
		                 /* a-subr			b-subr			*/
		case 0:          /* local			local			*/
		case 5: {        /* global.select	global.select   */
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
			printf("cmpLocalSetSubrs() can't happen!\n");
	}
	return 0;   /* Suppress compiler warning */
}

/* Find social groups for global or local subr set. */
static void findGroups(subrCtx h, unsigned id) {
	long i;
	int (CDECL *cmpSubrs)(const void *first, const void *second) =
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

/* Update inferior counts */
static void updateInfs(subrCtx h, Subr *subr, int deltacnt, unsigned id) {
	Link *link;

	for (link = subr->infs; link != NULL; link = link->next) {
		Subr *inf = link->subr;
		if (!(inf->flags & (SUBR_MARKED | SUBR_OVERLAP)) && inf->node->id == id) {
#if DB_SELECT
			printf("updateInfs([%d]->[%d],%d) deltacnt=%d\n",
			       subr - h->subrs.array, inf - h->subrs.array,
			       deltacnt, inf->deltacnt + deltacnt);
#endif
			updateInfs(h, inf, deltacnt, id);
			inf->deltacnt += deltacnt;
		}
	}
}

/* Update superior lengths */
static void updateSups(subrCtx h, Subr *subr, int deltalen, unsigned id) {
	Link *link;

	for (link = subr->sups; link != NULL; link = link->next) {
		Subr *sup = link->subr;
		if (!(sup->flags & (SUBR_MARKED | SUBR_OVERLAP)) && sup->node->id == id) {
#if DB_SELECT
			printf("updateSups([%d]->[%d],%d) deltalen=%d\n",
			       subr - h->subrs.array, sup - h->subrs.array,
			       deltalen, sup->deltalen + deltalen);
#endif
			updateSups(h, sup, deltalen, id);
			sup->deltalen += deltalen;
		}
	}
}

/* Select subr */
static void selectSubr(subrCtx h, Subr *subr) {
	int count = subr->count + subr->deltacnt;
	int length = subr->length - subr->maskcnt + subr->deltalen;
	int saved = count * (length - CALL_OP_SIZE - subr->numsize) -
	    (h->offSize + length + ((subr->node->flags & NODE_TAIL) == 0));

#if DB_SELECT
	printf("selectSubr([%d]) count=%d, length=%d, saved=%d\n",
	       subr - h->subrs.array, count, length, saved);
#endif

	if (saved > 0) {
		/* Select subr */
		unsigned id = subr->node->id;
		int deltacnt = 1 - subr->count - subr->deltacnt;
		int deltalen = subr->numsize + CALL_OP_SIZE - subr->length -
		    subr->deltalen;

		subr->flags |= SUBR_SELECT;

#if DB_SELECT
		printf("select=%d, deltacnt=%d, deltalen=%d\n",
		       saved, deltacnt, deltalen);
#endif
		updateInfs(h, subr, deltacnt, id);
		updateSups(h, subr, deltalen, id);
	}
	else {
		/* Reject subr */
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
				updateInfs(h, subr, 1 - subr->count, id);
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
static int CDECL cmpSubrFitness(const void *first, const void *second) {
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
static void checkSubrStackOvl(subrCtx h, Subr *subr, int depth) {
	Link *sup;

	if (subr->flags & SUBR_SELECT) {
		depth++;    /* Bump depth for selected subrs only */

		if (depth >= h->spd->maxCallStack) {
			/* Stack depth exceeded; reject subr */
			subr->flags &= ~SUBR_SELECT;
			subr->flags |= SUBR_REJECT;
			depth--;
			h->subrStackOvl = 1;
		}
	}

	/* Recursively ascend superior subrs */
	for (sup = subr->sups; sup != NULL; sup = sup->next) {
		checkSubrStackOvl(h, sup->subr, depth);
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
			printf("%s=%ld\n", (subr->flags & SUBR_SELECT) ? "select" : "reject",
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

	/* Remove membership so globals may be selected in other local sets and
	   check for and handle subr call stack overflow */
	for (i = 0; i < h->leaders.cnt; i++) {
		Subr *subr;
		for (subr = h->leaders.array[i]; subr != NULL; subr = subr->next) {
			if (subr->infs == NULL) {
				checkSubrStackOvl(h, subr, 0);
			}
			subr->flags &= ~SUBR_MEMBER;
		}
	}

	/* Sort selected subrs by fitness */
	qsort(h->tmp.array, h->tmp.cnt, sizeof(Subr *), cmpSubrFitness);

	/* Find last selected subr */
	for (i = h->tmp.cnt - 1; i >= 0; i--) {
		if (h->tmp.array[i]->flags & SUBR_SELECT) {
			nSelected = i + 1;
			break;
		}
	}

	if (++pass == 2) {
		#if 1
		/* Discard extra subrs if exceeds capacity */
		long limit = h->g->maxNumSubrs;
		if (limit == 0) {
			limit = 65536L;
		}
		limit = limit * multiplier;

		if (nSelected >= limit) {
			for (i = limit; i < nSelected; i++) {
				h->tmp.array[i]->flags &= ~SUBR_SELECT;
				h->tmp.array[i]->flags |= SUBR_REJECT;
			}
			h->tmp.cnt = limit;
		}
		else {
			h->tmp.cnt = nSelected;
		}
		#else /* original code before I added h->g->maxNumSubrs */
		h->tmp.cnt =
		    (nSelected > 65536L * multiplier) ? 65536L * multiplier : nSelected;
		#endif
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

/* Rescan charstring and build call list of selected subrs */
static void buildCallList(subrCtx h, unsigned length, unsigned char *pstart,
                          int selfMatch, unsigned id) {
	Node *node = h->root;
	unsigned char *pstr = pstart;
	unsigned char *pend = pstart + length;

	h->calls.cnt = 0;
	do {
		Node *suffix;
		Call *call;
		Subr *subr;
		unsigned offset;
		unsigned oplength = h->spd->oplen(pstr);
		node = findEdge(h, node, oplength, pstr)->son;
		pstr += oplength;

		if (node->flags & NODE_SUBR && (selfMatch || pstr != pend)) {
			subr = &h->subrs.array[node->misc];
			if (subr->flags & SUBR_SELECT) {
				/* In-path subr; reset accumulator */
				offset = 0;
				h->calls.cnt = 0;
				goto save;
			}
		}

		for (suffix = node->suffix; suffix != NULL; suffix = suffix->suffix) {
			if (suffix->flags & NODE_SUBR) {
				subr = &h->subrs.array[suffix->misc];
				if (subr->flags & SUBR_SELECT) {
					/* Suffix subr; compute offset */
					offset = pstr - pstart - subr->length;
					goto save;
				}
			}
		}
#if 0   /* xxx figure this out */
		else if (!(suffix->flags & NODE_FAIL)) {
			break;      /* Can't be another subr on suffix chain */
		}
#endif
		continue;

		/* Save/update record */
save:
		call = findInsertPosn(h, offset);
		call->subr = subr;
		call->offset = offset;
	}
	while (pstr < pend);

	/* Check for overlaps */
	if (h->calls.cnt > 1) {
		/* Handle overlaps */
		int i;

		for (i = 0; i < h->calls.cnt - 1; i++) {
			int j;
			unsigned freeoff;
			Call *first = &h->calls.array[i];

			if (first->subr == NULL) {
				continue;
			}

			freeoff = first->offset + first->subr->length;

			for (j = i + 1; j < h->calls.cnt; j++) {
				Call *second = &h->calls.array[j];

				if (second->subr == NULL) {
					continue;
				}
				else if (freeoff > second->offset) {
					/* Found overlap; disable shortest subr */
					if (first->subr->length > second->subr->length) {
						second->subr = NULL;
					}
					else {
						first->subr = NULL;
						break;
					}
				}
				else {
					break;
				}
			}
		}
	}

#if DB_INFS
	if (h->calls.cnt != 0) {
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
			pdst = h->spd->cstrcpy(pdst, psrc, call->offset - offset);

			/* Add subr call */
			pdst += h->spd->encInteger(subr->subrnum, (char *)pdst);
			*pdst++ = (subr->node->id == NODE_GLOBAL) ?
			    h->spd->callgsubr : h->spd->callsubr;

#if DB_CALLS
			subr->calls++;
#endif

			/* Skip over subr's bytes */
			psrc += call->offset - offset + subr->length;
			offset = call->offset + subr->length;
		}
	}

	/* Copy remainder of charstring and return destination buffer pointer */
	return h->spd->cstrcpy(pdst, psrc, length - offset);
}

/* Subroutinize charstrings */
static void subrizeChars(subrCtx h, CSData *chars, unsigned id) {
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
		buildCallList(h, length, psrc, 1, id);

		/* Subroutinize charstring */
		pdst = subrizeCstr(h, pdst, psrc, length);

		/* Adjust initial length estimate and save offset */
		h->cstrs.cnt = iStart + pdst - (unsigned char *)&h->cstrs.array[iStart];
		chars->offset[i] = h->cstrs.cnt;
		offset = nextoff;
	}

	/* Copy charstring data without loosing original data pointer */
	chars->refcopy = chars->data;
	chars->data = MEM_NEW(h->g, h->cstrs.cnt);
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

	if (!local) {
		/* Add biased subr numbers and mark global subrs */
		for (i = 0; i < count; i++) {
			Subr *subr = h->reorder.array[i];
			subr->subrnum = (short)(i - bias);
			subr->node->id = NODE_GLOBAL;
		}

		/* Copy global subr numbers to local subrs so that when the global subr
		   INDEX is constructed the local subr references will be valid	*/
		for (i = 0; i < h->tmp.cnt - 1; i += 2) {
			h->tmp.array[i + 1]->subrnum = h->tmp.array[i]->subrnum;
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
static void addSubrs(subrCtx h, CSData *subrs, unsigned id) {
	long i;

#if DB_SUBRS
	printf("--- subrized subrs (subrs marked with -)\n");
#endif

	if (h->reorder.cnt == 0) {
		return; /* No subrs */
	}
	/* Allocate subr offset array */
	subrs->nStrings = (unsigned short)h->reorder.cnt;
	subrs->offset = MEM_NEW(h->g, h->reorder.cnt * sizeof(Offset));

	h->cstrs.cnt = 0;
	for (i = 0; i < h->reorder.cnt; i++) {
		unsigned char *pdst;
		Subr *subr = h->reorder.array[i];
		long iStart = h->cstrs.cnt;

		/* Initially allocate space for entire charstring + last op  */
		pdst = (unsigned char *)dnaEXTEND(h->cstrs, subr->length + 1);

#if DB_SUBRS
		dbsubr(h, subr - h->subrs.array, '-', 0);
#endif

		/* Build subr call list */
		buildCallList(h, subr->length, subr->cstr, 0, id);

		/* Subroutinize charstring */
		pdst = subrizeCstr(h, pdst, subr->cstr, subr->length);

		/* Terminate subr */
		if (!(subr->node->flags & NODE_TAIL)) {
			*pdst++ = (unsigned char)h->spd->return_;
		}

		/* Adjust initial length estimate and save offset */
		h->cstrs.cnt = iStart + pdst - (unsigned char *)&h->cstrs.array[iStart];
		subrs->offset[i] = h->cstrs.cnt;
	}

	/* Allocate and copy charstring data */
	subrs->data = MEM_NEW(h->g, h->cstrs.cnt);
	memcpy(subrs->data, h->cstrs.array, h->cstrs.cnt);
}

/* Subroutinize FD charstrings */
static void subrizeFDChars(subrCtx h, CSData *dst, Font *font,
                           unsigned iFont, unsigned iFD) {
	long iSrc;
	unsigned iDst;
	CSData *src = &font->chars;

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

	/* Allocate offset array */
	dst->offset = MEM_NEW(h->g, sizeof(Offset) * dst->nStrings);

	h->cstrs.cnt = 0;
	iDst = 0;
	for (iSrc = 0; iSrc < src->nStrings; iSrc++) {
		if (font->fdIndex[iSrc] == iFD) {
			unsigned char *pdst;
			long offset = (iSrc == 0) ? 0 : src->offset[iSrc - 1];
			unsigned char *psrc = (unsigned char *)&src->data[offset];
			unsigned length = src->offset[iSrc] - offset - 4 /* t2_separator */;
			long iStart = h->cstrs.cnt;

			/* Initially allocate space for entire charstring */
			pdst = (unsigned char *)dnaEXTEND(h->cstrs, (long)length);

#if DB_CHARS
			printf("[%3ld]    =  ", iSrc);
			dbcstr(h, length, psrc);
			printf("\n");
#endif

			/* Build subr call list */
			buildCallList(h, length, psrc, 1, iFont + iFD);

			/* Subroutinize charstring */
			pdst = subrizeCstr(h, pdst, psrc, length);

			/* Adjust initial length estimate and save offset */
			h->cstrs.cnt = iStart + pdst -
			    (unsigned char *)&h->cstrs.array[iStart];
			dst->offset[iDst++] = h->cstrs.cnt;
		}
	}

	/* Copy charstring data */
	dst->data = MEM_NEW(h->g, h->cstrs.cnt);
	memcpy(dst->data, h->cstrs.array, h->cstrs.cnt);

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

/* Reassemble cstrs for CID-keyed font */
static void joinFDChars(subrCtx h, Font *font) {
	tcCtx g = h->g;
	long i;
	long size;
	long dstoff;

	/* Determine total size of subroutinized charstring data */
	size = 0;
	for (i = 0; i < font->fdCount; i++) {
		FDInfo *info = &font->fdInfo[i];
		size += info->chars.offset[info->chars.nStrings - 1];
		info->iChar = 0;
	}

	/* Allocate new charstring data without loosing original data pointer */
	font->chars.refcopy = font->chars.data;
	font->chars.data = MEM_NEW(g, size);

	dstoff = 0;
	for (i = 0; i < font->chars.nStrings; i++) {
		FDInfo *info = &font->fdInfo[font->fdIndex[i]];
		long srcoff =
		    (info->iChar == 0) ? 0 : info->chars.offset[info->iChar - 1];
		unsigned length = info->chars.offset[info->iChar++] - srcoff;

		memmove(&font->chars.data[dstoff], &info->chars.data[srcoff], length);

		font->chars.offset[i] = (dstoff += length);
	}

	/* Free temporary chars index for each FD */
	for (i = 0; i < font->fdCount; i++) {
		CSData *chars = &font->fdInfo[i].chars;
		MEM_FREE(g, chars->offset);
		MEM_FREE(g, chars->data);
	}
}

/* -------------------------- Subroutinize FontSet ------------------------- */

/* Build subrs with specific id */
static void buildSubrs(subrCtx h, CSData *subrs, unsigned id) {
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
	reorderSubrs(h);
	addSubrs(h, subrs, id);
}

/* Subroutinize all fonts in FontSet */
void subrSubrize(tcCtx g, int nFonts, Font *fonts) {
	subrCtx h = g->ctx.subr;
	unsigned iFont;
	long i;

	h->spd = g->spd;
	h->nFonts = nFonts;
	h->fonts = fonts;

	/* Determine type of FontSet */
	h->singleton = h->nFonts == 1 && !(h->fonts[0].flags & FONT_CID);

	/* Add fonts' charstring data to DAWG */
	iFont = 0;
	for (i = 0; i < h->nFonts; i++) {
		addFont(h, &h->fonts[i], iFont);
		iFont += (h->fonts[i].flags & FONT_CID) ? h->fonts[i].fdCount : 1;
	}

	selectCandSubrs(h); /* Select candidate subrs */
	assocSubrs(h);      /* Associate subrs with a font */

	if (h->singleton) {
		/* Single non-CID font */

		/* Build temporary array from full subr list */
		dnaSET_CNT(h->tmp, h->subrs.cnt);
		for (i = 0; i < h->subrs.cnt; i++) {
			h->tmp.array[i] = &h->subrs.array[i];
		}

		h->subrStackOvl = 0;
		selectFinalSubrSet(h, 0);
		if (h->tmp.cnt >= 215) {
			/* Make global subrs from even indexes */
			reorderCombined(h, 0);
			addSubrs(h, &h->gsubrs, NODE_GLOBAL);

			/* Make local subrs from odd indexes */
			reorderCombined(h, 1);
			addSubrs(h, &h->fonts[0].subrs, 0);
		}
		else {
			reorderSubrs(h);
			addSubrs(h, &h->fonts[0].subrs, 0);
		}
		subrizeChars(h, &h->fonts[0].chars, 0);
	}
	else {
		/* Multiple fonts or single CID font */

		/* Build global subrs */
		buildSubrs(h, &h->gsubrs, NODE_GLOBAL);

		/* Find and add local subrs to each font */
		iFont = 0;
		for (i = 0; i < h->nFonts; i++) {
			Font *font = &h->fonts[i];

			h->subrStackOvl = 0;
			if (font->flags & FONT_CID) {
				/* Subrotinize CID-keyed font */
				int j;
				for (j = 0; j < h->fonts[i].fdCount; j++) {
					/* Subroutinize component DICT */
					FDInfo *info = &font->fdInfo[j];

					buildSubrs(h, &info->subrs, iFont + j);
					subrizeFDChars(h, &info->chars, font, iFont, j);
				}
				joinFDChars(h, font);
				iFont += h->fonts[i].fdCount;
			}
			else {
				if (font->chars.nStrings != 0) {
					/* Subroutinize non-synthetic font */
					buildSubrs(h, &font->subrs, iFont);
					subrizeChars(h, &font->chars, iFont);
				}
				iFont++;
			}

		}
	}

	/* Free original unsubroutinized charstring data */
	for (i = 0; i < h->nFonts; i++) {
		MEM_FREE(g, h->fonts[i].chars.refcopy);
	}
}

/* Compute size of font's subrs */
long subrSizeLocal(CSData *subrs) {
	return (subrs->nStrings == 0) ? 0 :
	       INDEX_SIZE(subrs->nStrings, subrs->offset[subrs->nStrings - 1]);
}

#endif /* TC_SUBR_SUPPORT */

/* Compute size of global subrs */
long subrSizeGlobal(tcCtx g) {
#if TC_SUBR_SUPPORT
	subrCtx h = g->ctx.subr;
	return (h->gsubrs.nStrings == 0) ?
	       INDEX_SIZE(0, 0) : INDEX_SIZE(h->gsubrs.nStrings,
	                                     h->gsubrs.offset[h->gsubrs.nStrings - 1]);
#else /* TC_SUBR_SUPPORT */
	return INDEX_SIZE(0, 0);
#endif /* TC_SUBR_SUPPORT */
}

#if TC_SUBR_SUPPORT
/* Write subrs */
static void subrWrite(tcCtx g, CSData *subrs) {
	long dataSize;
	INDEXHdr header;

	header.count = subrs->nStrings;
	OUT2(header.count);

	if (header.count == 0) {
		return; /* Empty table just has zero count */
	}
	dataSize = subrs->offset[subrs->nStrings - 1];
	header.offSize = INDEX_OFF_SIZE(dataSize);
	OUT1(header.offSize);

	csWriteData(g, subrs, dataSize, header.offSize);
}

/* Write local subrs */
void subrWriteLocal(tcCtx g, CSData *subrs) {
	if (subrs->nStrings != 0) {
		subrWrite(g, subrs);
	}
}

#endif /* TC_SUBR_SUPPORT */

/* Write global subrs */
void subrWriteGlobal(tcCtx g) {
#if TC_SUBR_SUPPORT
	subrCtx h = g->ctx.subr;
	if (h != NULL) {
		subrWrite(g, &h->gsubrs);
	}
	else
#endif /* TC_SUBR_SUPPORT */
	{
		/* Write empty global subr INDEX */
		INDEXHdr header;
		header.count = 0;
		OUT2(header.count);
	}
}

#if TC_SUBR_SUPPORT
#if TC_DEBUG
/* --------------------------------- DEBUG --------------------------------- */
#include <ctype.h>
#include "sindex.h"

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

	tcFatal(h->g, "can't find node!");
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
	sprintf(buf, "%hu.%d%c%ld", subr->count, subr->length - subr->maskcnt,
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
	printf(" [%u]\n", iSubr);
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
	if (node->edge.son == NULL) {
		printf("   none\n");
	}
	else {
		Edge *edge;
		for (edge = &node->edge; edge != NULL; edge = edge->next) {
			printf("  %6ld (%08lx) %8s ",
			       dbnodeid(h, edge->son), (unsigned long)edge->son,
			       (node->misc + h->spd->oplen(edge->label) !=
			        edge->son->misc) ? "shortcut" : "-");
			dbop(h->spd->oplen(edge->label), edge->label);
			printf(" (%08lx)\n", (unsigned long)edge);
		}
	}
}

static void dbcstr(subrCtx h, unsigned length, unsigned char *cstr) {
	unsigned char *end = cstr + length;

	while (cstr < end) {
		unsigned i;

		length = h->spd->oplen(cstr);
		for (i = 0; i < length; i++) {
			printf("%c%c",
			       "0123456789abcdef"[cstr[i] >> 4],
			       "0123456789abcdef"[cstr[i] & 0xf]);
		}

		cstr += length;
	}
}

static void dbcstrs(subrCtx h, unsigned char *cstr, unsigned char *end,
                    int index) {
	int startchar = 1;
	int gid = 0;

	printf("--- glyphs (total=%ld)\n", (long)(end - cstr));

	while (cstr < end) {
		int length = h->spd->oplen(cstr);

		if (startchar) {
			printf("%2d:%3d (%s) ", index, gid, sindexGetString(h->g, gid));
			startchar = 0;
			gid++;
		}

		dbop(length, cstr);

		if (cstr[0] == h->spd->separator) {
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
static void CDECL dbuse(int arg, ...) {
	dbuse(0, dbnode, dbcstrs, dbsubr, dbgroups);
}

#endif /* TC_DEBUG */
#endif /* TC_SUBR_SUPPORT */
