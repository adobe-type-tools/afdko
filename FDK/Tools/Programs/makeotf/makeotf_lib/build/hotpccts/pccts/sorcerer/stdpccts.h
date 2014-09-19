#ifndef STDPCCTS_H
#define STDPCCTS_H
/*
 * stdpccts.h -- P C C T S  I n c l u d e
 *
 * Terence Parr, Will Cohen, and Hank Dietz: 1989-2001
 * Purdue University Electrical Engineering
 * With AHPCRC, University of Minnesota
 * ANTLR Version 1.33MR33
 */

#ifndef ANTLR_VERSION
#define ANTLR_VERSION	13333
#endif

#include "pcctscfg.h"
#include "pccts_stdio.h"

/*  23-Sep-97   thm     Accomodate user who needs to redefine ZZLEXBUFSIZE  */

#ifndef ZZLEXBUFSIZE
#define ZZLEXBUFSIZE	8000
#endif
#include "pcctscfg.h"    /* MR20 G. Hobbelt __USE_PROTOS #define */
#include "charbuf.h"
#include "hash.h"
#include "set.h"
#include "sor.h"
#define AST_FIELDS	\
int token; char text[MaxAtom+1], label[MaxRuleName+1]; \
char *action;		/* if action node, here is ptr to it */ \
char in,out; \
char init_action;	/* set if Action and 1st action of alt */ \
int file; int line; /* set for BLOCK, ALT, nonterm nodes */ \
int upper_range;	/* only if T1..T2 found */	\
GLA *start_state;	/* ptr into GLA for this block */ \
int no_copy;		/* copy input ptr to output ptr? */ \
ListNode *refvars;	/* any ref vars defined for this rule */ \
unsigned char is_root; /* this token is a root #( A ... ) */
#define zzcr_ast(node, cur, _tok, _text)	\
{(node)->token=_tok; strncpy((node)->text, _text,MaxAtom);}
#define USER_ZZSYN
#define zzAST_DOUBLE
extern int define_num;
#define LL_K 2
#define GENAST
#define zzSET_SIZE 16
#include "antlr.h"
#include "ast.h"
#include "tokens.h"
#include "dlgdef.h"
#include "mode.h"
#endif
