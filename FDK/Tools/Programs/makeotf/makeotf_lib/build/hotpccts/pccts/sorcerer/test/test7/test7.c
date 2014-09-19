/*
 * S O R C E R E R  T r a n s l a t i o n  H e a d e r
 *
 * SORCERER Developed by Terence Parr, Aaron Sawdey, & Gary Funck
 * Parr Research Corporation, Intrepid Technology, University of Minnesota
 * 1992-1994
 * SORCERER Version 1.00B15
 */
#define SORCERER_VERSION	100B15
#define SORCERER_NONTRANSFORM
#include <stdio.h>
#include <setjmp.h>

#include "stdpccts.h"	/* define ANTLR tree stuff and token types */
typedef AST SORAST;
#include "sorcerer.h"
extern void gen_stat();
extern void gen_expr();

void
#ifdef __USE_PROTOS
_refvar_inits(STreeParser *p)
#else
_refvar_inits(p)
STreeParser *p;
#endif
{
}


#include "errsupport.c"	/* define some default error routines */

void gen_stat(_parser, _root)
STreeParser *_parser;
SORAST **_root;
{
	SORAST *_t = *_root;
	SORAST *t=NULL;
	if ( _t!=NULL && (_t->token==ASSIGN) ) {
		{_SAVE; TREE_CONSTR_PTRS;
		_MATCH(ASSIGN);
		_DOWN;
		_MATCH(ID);
		t=(SORAST *)_t;
		_RIGHT;
		gen_expr(_parser, &_t);
		_RESTORE;
		}
		_RIGHT;
		printf("\tstore %s\n", t->text);
	}
	else {
		if ( _parser->guessing ) _GUESS_FAIL;
		no_viable_alt(_parser, "gen_stat", _t);
	}
	*_root = _t;
}

void gen_expr(_parser, _root)
STreeParser *_parser;
SORAST **_root;
{
	SORAST *_t = *_root;
	SORAST *t=NULL;
	if ( _t!=NULL && (_t->token==ADD) ) {
		{_SAVE; TREE_CONSTR_PTRS;
		_MATCH(ADD);
		_DOWN;
		gen_expr(_parser, &_t);
		gen_expr(_parser, &_t);
		_RESTORE;
		}
		_RIGHT;
		printf("\tadd\n");
	}
	else {
	if ( _t!=NULL && (_t->token==MULT) ) {
		{_SAVE; TREE_CONSTR_PTRS;
		_MATCH(MULT);
		_DOWN;
		gen_expr(_parser, &_t);
		gen_expr(_parser, &_t);
		_RESTORE;
		}
		_RIGHT;
		printf("\tmult\n");
	}
	else {
	if ( _t!=NULL && (_t->token==ID) ) {
		_MATCH(ID);
		t=(SORAST *)_t;
		_RIGHT;
		printf("\tpush %s\n", t->text);
	}
	else {
	if ( _t!=NULL && (_t->token==INT) ) {
		_MATCH(INT);
		t=(SORAST *)_t;
		_RIGHT;
		printf("\tpush %s\n", t->text);
	}
	else {
		if ( _parser->guessing ) _GUESS_FAIL;
		no_viable_alt(_parser, "gen_expr", _t);
	}
	}
	}
	}
	*_root = _t;
}
