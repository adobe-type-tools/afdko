#ifndef STDPCCTS_H
#define STDPCCTS_H
/*
 * stdpccts.h -- P C C T S  I n c l u d e
 *
 * Terence Parr, Will Cohen, and Hank Dietz: 1989-1994
 * Purdue University Electrical Engineering
 * With AHPCRC, University of Minnesota
 * ANTLR Version 1.33MR9
 */
#include <stdio.h>
#define ANTLR_VERSION	133MR9

#include "charbuf.h"
#define AtomSize	20
#define AST_FIELDS	char text[AtomSize+1]; int token;
#define zzcr_ast(tr, attr, tok, txt) { strcpy(tr->text, txt); tr->token=tok; }
#define GENAST
#define zzSET_SIZE 4
#include "antlr.h"
#include "ast.h"
#include "tokens.h"
#include "dlgdef.h"
#include "mode.h"
#endif
