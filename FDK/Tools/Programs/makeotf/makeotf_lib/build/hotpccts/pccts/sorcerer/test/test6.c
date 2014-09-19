/*
 * S O R C E R E R  T r a n s l a t i o n  H e a d e r
 *
 * SORCERER Developed by Terence Parr, Aaron Sawdey, & Gary Funck
 * Parr Research Corporation, Intrepid Technology, University of Minnesota
 * 1992-1994
 * SORCERER Version 1.00B15
 */
#define SORCERER_VERSION	100B15
#define SORCERER_TRANSFORM
#include <stdio.h>
#include <setjmp.h>

typedef struct _node {
	struct _node *right, *down;	/* using non-transform mode */
	int token;
	char text[50];
} SORAST;
#define _REFVARS \
	 SORAST *loop_var; \


#include "sorcerer.h"
extern void do_stat();
extern void stat();
extern void lvalue();
extern void expr();
#include "sorcerer.c"

void
#ifdef __USE_PROTOS
_refvar_inits(STreeParser *p)
#else
_refvar_inits(p)
STreeParser *p;
#endif
{
}


#include "tokens6.h"
#include "astlib.h"		/* need the ast_X support library */
#include "errsupport.c" /* define error routines here or include errsupport.c */

/* This function is implicitly called when you reference the node constructor #[] */
SORAST *
#ifdef __STDC__
ast_node(int tok, char *s)
#else
ast_node(tok, s)
int tok;
char *s;
#endif
{
	SORAST *p = (SORAST *) calloc(1, sizeof(SORAST));
	if ( p == NULL ) {fprintf(stderr, "out of mem\n"); exit(-1);}

	p->token = tok;
	strcpy(p->text, s);
	return p;
}

main()
{
	SORAST *a, *b, *c, *d, *s;
	SORAST *result = NULL;
	STreeParser myparser;
	STreeParserInit(&myparser);

	/* create the tree for 'DO i=1,10 a(i)=3' to parse
	* #( DO ID expr expr stat )
	*/
	s = ast_make( ast_node(ASSIGN,"="), ast_make(ast_node(AREF,"()"),  ast_node(ID,"a"), ast_node(ID,"i"), NULL), ast_node(INT,"3") , NULL);
	d = ast_make( ast_node(DO,"do"), ast_node(ID,"i"), ast_node(INT,"1"), ast_node(INT,"10"), s , NULL);

	/* match tree and execute actions */
	printf("tree parser input: "); lisp(d, 0); printf("\n");
	do_stat(&myparser, &d, &result);
	printf("tree parser output: "); lisp(result, 0); printf("\n");
}

#ifdef __STDC__
lisp(SORAST *tree)
#else
lisp(tree, rw)
SORAST *tree;
#endif
{
	while ( tree!= NULL )
	{
		if ( tree->down != NULL ) printf(" (");
		printf(" %s", tree->text);
		lisp(tree->down);
		if ( tree->down != NULL ) printf(" )");;
		tree = tree->right;
	}
}

void do_stat(_parser, _root, _result)
STreeParser *_parser;
SORAST **_root, **_result;
{
	SORAST *_t = *_root;
	SORAST *_tresult=NULL;
	TREE_CONSTR_PTRS;
	SORAST *lvar=NULL,*lvar_in=NULL;
	*_result = NULL;
	if ( _t!=NULL && (_t->token==DO) ) {
		
		{_SAVE; TREE_CONSTR_PTRS;
		_MATCH(DO);
		_tresult=ast_dup_node(_t);
 _mkroot(&_r,&_s,&_e,_tresult);
		_DOWN;
		_MATCH(ID);
		_tresult=ast_dup_node(_t);
 _mkchild(&_r,&_s,&_e,_tresult);
		lvar=(SORAST *)_tresult; lvar_in=(SORAST *)_t;
		_RIGHT;
		_parser->loop_var=lvar;
		expr(_parser, &_t, &_tresult);
		_mkchild(&_r,&_s,&_e,_tresult);
		expr(_parser, &_t, &_tresult);
		_mkchild(&_r,&_s,&_e,_tresult);
		stat(_parser, &_t, &_tresult);
		_mkchild(&_r,&_s,&_e,_tresult);
		_RESTORE; _tresult = _r;
		}
		_mkchild(&_r,&_s,&_e,_tresult);
		_RIGHT;
	}
	else {
		if ( _parser->guessing ) _GUESS_FAIL;
		no_viable_alt(_parser, "do_stat", _t);
	}
	 _tresult = _r;
	*_root = _t;
	if ( (*_result) == NULL ) *_result = _r;
}

void stat(_parser, _root, _result)
STreeParser *_parser;
SORAST **_root, **_result;
{
	SORAST *_t = *_root;
	SORAST *_tresult=NULL;
	TREE_CONSTR_PTRS;
	SORAST *a=NULL,*a_in=NULL;
	SORAST *l=NULL,*l_in=NULL;
	SORAST *e=NULL,*e_in=NULL;
	*_result = NULL;
	if ( _t!=NULL && (_t->token==ASSIGN) ) {
		SORAST *ne;
		{_SAVE; TREE_CONSTR_PTRS;
		_MATCH(ASSIGN);
		_tresult=ast_dup_node(_t);
		a=(SORAST *)_tresult; a_in=(SORAST *)_t;
		_DOWN;
		l=(SORAST *)_t; lvalue(_parser, &_t, &_tresult);
		l=(SORAST *)_tresult;
		e=(SORAST *)_t; expr(_parser, &_t, &_tresult);
		e=(SORAST *)_tresult;
		_RESTORE; _tresult = _r;
		}
		_RIGHT;
		
		ne = ast_make(ast_node(MULT,"*"), e, ast_node(ID,_parser->loop_var->text), NULL);
		(*_result) = ast_make(a,l,ne, NULL);
	}
	else {
		if ( _parser->guessing ) _GUESS_FAIL;
		no_viable_alt(_parser, "stat", _t);
	}
	 _tresult = _r;
	*_root = _t;
}

void lvalue(_parser, _root, _result)
STreeParser *_parser;
SORAST **_root, **_result;
{
	SORAST *_t = *_root;
	SORAST *_tresult=NULL;
	TREE_CONSTR_PTRS;
	*_result = NULL;
	if ( _t!=NULL && (_t->token==AREF) ) {
		{_SAVE; TREE_CONSTR_PTRS;
		_MATCH(AREF);
		_tresult=ast_dup_node(_t);
 _mkroot(&_r,&_s,&_e,_tresult);
		_DOWN;
		_MATCH(ID);
		_tresult=ast_dup_node(_t);
 _mkchild(&_r,&_s,&_e,_tresult);
		_RIGHT;
		expr(_parser, &_t, &_tresult);
		_mkchild(&_r,&_s,&_e,_tresult);
		_RESTORE; _tresult = _r;
		}
		_mkchild(&_r,&_s,&_e,_tresult);
		_RIGHT;
	}
	else {
		if ( _parser->guessing ) _GUESS_FAIL;
		no_viable_alt(_parser, "lvalue", _t);
	}
	 _tresult = _r;
	*_root = _t;
	if ( (*_result) == NULL ) *_result = _r;
}

void expr(_parser, _root, _result)
STreeParser *_parser;
SORAST **_root, **_result;
{
	SORAST *_t = *_root;
	SORAST *_tresult=NULL;
	TREE_CONSTR_PTRS;
	*_result = NULL;
	if ( _t!=NULL && (_t->token==MULT) ) {
		{_SAVE; TREE_CONSTR_PTRS;
		_MATCH(MULT);
		_tresult=ast_dup_node(_t);
 _mkroot(&_r,&_s,&_e,_tresult);
		_DOWN;
		expr(_parser, &_t, &_tresult);
		_mkchild(&_r,&_s,&_e,_tresult);
		expr(_parser, &_t, &_tresult);
		_mkchild(&_r,&_s,&_e,_tresult);
		_RESTORE; _tresult = _r;
		}
		_mkchild(&_r,&_s,&_e,_tresult);
		_RIGHT;
	}
	else {
	if ( _t!=NULL && (_t->token==ID) ) {
		_MATCH(ID);
		_tresult=ast_dup_node(_t);
 _mkchild(&_r,&_s,&_e,_tresult);
		_RIGHT;
	}
	else {
	if ( _t!=NULL && (_t->token==INT) ) {
		_MATCH(INT);
		_tresult=ast_dup_node(_t);
 _mkchild(&_r,&_s,&_e,_tresult);
		_RIGHT;
	}
	else {
		if ( _parser->guessing ) _GUESS_FAIL;
		no_viable_alt(_parser, "expr", _t);
	}
	 _tresult = _r;
	}
	}
	*_root = _t;
	if ( (*_result) == NULL ) *_result = _r;
}
