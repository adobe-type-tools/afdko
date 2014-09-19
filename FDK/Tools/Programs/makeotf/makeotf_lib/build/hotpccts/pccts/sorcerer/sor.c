/*
 * A n t l r  T r a n s l a t i o n  H e a d e r
 *
 * Terence Parr, Will Cohen, and Hank Dietz: 1989-2001
 * Purdue University Electrical Engineering
 * With AHPCRC, University of Minnesota
 * ANTLR Version 1.33MR33
 *
 *   ..\bin\antlr -gh -k 2 -gt sor.g
 *
 */

#define ANTLR_VERSION	13333
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

#include "ast.h"

#define zzSET_SIZE 16
#include "antlr.h"
#include "tokens.h"
#include "dlgdef.h"
#include "mode.h"

/* MR23 In order to remove calls to PURIFY use the antlr -nopurify option */

#ifndef PCCTS_PURIFY
#define PCCTS_PURIFY(r,s) memset((char *) &(r),'\0',(s));
#endif

#include "ast.c"
zzASTgvars

ANTLR_INFO

/* MR20 G. Hobbelt Fix for Borland C++ 4.x & 5.x compiling with ALL warnings enabled */
#if defined(__TURBOC__)
#pragma warn -aus  /* unused assignment of 'xxx' */
#endif


#include "sym.h"
#include "proto.h"

void    /* MR9 23-Sep-97 Eliminate complaint about no return value */
#ifdef __USE_PROTOS
lisp( AST *tree, FILE *output )
#else
lisp( tree, output )
AST *tree;
FILE *output;
#endif
{
  while ( tree != NULL )
  {
    if ( tree->down != NULL ) fprintf(output," (");
    if ( tree->text[0]!='\0' ) {
      fprintf(output, " \"");
      if ( tree->label[0]!='\0' ) fprintf(output, "%s:", tree->label);
      switch ( tree->token ) {
        case OPT :
        case POS_CLOSURE :
        case CLOSURE :
        case PRED_OP :
        fprintf(output, "%s", tree->text);
        break;
        default :
        fprintf(output, "%s[%s]", zztokens[tree->token], tree->text);
      }
      fprintf(output, "\"");
    }
    else {
      fprintf(output, " %s", zztokens[tree->token]);
    }
    lisp(tree->down, output);
    if ( tree->down != NULL ) fprintf(output," )");
    tree = tree->right;
  }
}

AST *
#ifdef __USE_PROTOS
zzmk_ast(AST *node, int token)
#else
zzmk_ast(node, token)
AST *node;
int token;
#endif
{
  node->token = token;
  return node;
}

AST *
#ifdef __USE_PROTOS
read_sor_desc(FILE *f)
#else
read_sor_desc(f)
FILE *f;
#endif
{
  AST *root = NULL;
  
	zzline = 1;
  ANTLR(sordesc(&root), f);
  
	if ( found_error ) return NULL;
  
	if ( print_guts ) {
    fprintf(stderr, "Internal Represenation of Tree Grammar:\n");
    lisp(root, stderr);
    fprintf(stderr, "\n");
  }
  
	last_valid_token = token_type;
  end_of_input = token_type++;/* end of input token type is 1 + last real token */
  epsilon = token_type++;		/* epsilon token type is 2 + last real token */
  wild_card = token_type++;	/* wild_card_token is 3 + last real token */
  token_association(end_of_input, "$");
  token_association(epsilon, "[Ep]");
  token_association(wild_card, ".");
  
	zzdouble_link(root, NULL, NULL);
  rules = root;
  if ( root!=NULL ) build_GLA(root);
  
	if ( print_guts ) {
    fprintf(stderr, "Internal Represenation of Grammar Lookahead Automaton:\n");
    dump_GLAs(root);
    fprintf(stderr, "\n");
  }
  return root;
}

void
#ifdef __USE_PROTOS
sordesc(AST**_root)
#else
sordesc(_root)
AST **_root;
#endif
{
  zzRULE;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  int he=0,to=0;
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    for (;;) {
      if ( !((setwd1[LA(1)]&0x1))) break;
      if ( (LA(1)==Header) ) {
        header(zzSTR); zzlink(_root, &_sibling, &_tail);
        he++;
      }
      else {
        if ( (LA(1)==Tokdef) ) {
          tokdef(zzSTR); zzlink(_root, &_sibling, &_tail);
          to++;
        }
        else break; /* MR6 code for exiting loop "for sure" */
      }
      zzLOOP(zztasp2);
    }
    zzEXIT(zztasp2);
    }
  }
  
  if ( he==0 && !Inline && !GenCPP ) warnNoFL("missing #header statement");
  if ( he>1 ) warnNoFL("extra #header statement");
  if ( to>1 ) warnNoFL("extra #tokdef statement");
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    while ( (LA(1)==Action) && (setwd1[LA(2)]&0x2) ) {
      zzmatch(Action); 
      list_add(&before_actions, actiondup(LATEXT(1)));
 zzCONSUME;

      zzLOOP(zztasp2);
    }
    zzEXIT(zztasp2);
    }
  }
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (LA(1)==81)
 ) {
      class_def(zzSTR); zzlink(_root, &_sibling, &_tail);
    }
    else {
      if ( (setwd1[LA(1)]&0x4) ) {
      }
      else {zzFAIL(1,zzerr1,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
    zzEXIT(zztasp2);
    }
  }
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    while ( (LA(1)==Action) && (setwd1[LA(2)]&0x8) ) {
      zzmatch(Action); 
      
      if ( CurClassName[0]!='\0' )
      list_add(&class_actions, actiondup(LATEXT(1)));
      else
      list_add(&before_actions, actiondup(LATEXT(1)));
 zzCONSUME;

      zzLOOP(zztasp2);
    }
    zzEXIT(zztasp2);
    }
  }
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    while ( (LA(1)==NonTerm) ) {
      rule(zzSTR); zzlink(_root, &_sibling, &_tail);
      zzLOOP(zztasp2);
    }
    zzEXIT(zztasp2);
    }
  }
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    while ( (LA(1)==Action) && (setwd1[LA(2)]&0x10) ) {
      zzmatch(Action); 
      
      if ( CurClassName[0]!='\0' )
      list_add(&class_actions, actiondup(LATEXT(1)));
      else
      list_add(&before_actions, actiondup(LATEXT(1)));
 zzCONSUME;

      zzLOOP(zztasp2);
    }
    zzEXIT(zztasp2);
    }
  }
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (LA(1)==80)
 ) {
      zzmatch(80); 
      
      if ( CurClassName[0]=='\0' )
      err("missing class definition for trailing '}'");
 zzCONSUME;

    }
    else {
      if ( (setwd1[LA(1)]&0x20) ) {
      }
      else {zzFAIL(1,zzerr2,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
    zzEXIT(zztasp2);
    }
  }
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    while ( (LA(1)==Action) ) {
      zzmatch(Action); 
      list_add(&after_actions, actiondup(LATEXT(1)));
 zzCONSUME;

      zzLOOP(zztasp2);
    }
    zzEXIT(zztasp2);
    }
  }
  zzmatch(Eof);  zzCONSUME;
  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  found_error=1;  
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd1, 0x40);
  }
}

void
#ifdef __USE_PROTOS
header(AST**_root)
#else
header(_root)
AST **_root;
#endif
{
  zzRULE;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  zzmatch(Header);  zzCONSUME;
  zzmatch(Action); 
  header_action = actiondup(LATEXT(1));
 zzCONSUME;

  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  found_error=1;  
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd1, 0x80);
  }
}

void
#ifdef __USE_PROTOS
tokdef(AST**_root)
#else
tokdef(_root)
AST **_root;
#endif
{
  zzRULE;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  zzmatch(Tokdef);  zzCONSUME;
  zzmatch(RExpr); 
  {
    AST *dumb = NULL;
    zzantlr_state st; FILE *f; struct zzdlg_state dst;
    strcpy(tokdefs_file, LATEXT(1));
    strcpy(tokdefs_file, tokdefs_file+1); /* remove quotes */
    tokdefs_file[strlen(tokdefs_file)-1] = '\0';
    zzsave_antlr_state(&st);
    zzsave_dlg_state(&dst);
    define_num=0;
    f = fopen(tokdefs_file, "r");
    if ( f==NULL ) {found_error=1; err(eMsg1("cannot open token defs file '%s'", tokdefs_file));}
    else {ANTLRm(enum_file(&dumb), f, PARSE_ENUM_FILE);}
    zzrestore_antlr_state(&st);
    zzrestore_dlg_state(&dst);
    UserDefdTokens = 1;
  }
 zzCONSUME;

  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  found_error=1;  
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd2, 0x1);
  }
}

void
#ifdef __USE_PROTOS
class_def(AST**_root)
#else
class_def(_root)
AST **_root;
#endif
{
  zzRULE;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  zzmatch(81); zzastDPush; zzCONSUME;
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (LA(1)==NonTerm) ) {
      zzmatch(NonTerm); zzastDPush;
      strncpy(CurClassName,LATEXT(1),MaxAtom);
 zzCONSUME;

    }
    else {
      if ( (LA(1)==Token) ) {
        zzmatch(Token); zzastDPush;
        strncpy(CurClassName,LATEXT(1),MaxAtom);
 zzCONSUME;

      }
      else {zzFAIL(1,zzerr3,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
    zzEXIT(zztasp2);
    }
  }
  if ( !GenCPP ) { err("class meta-op used without C++ option"); }
  zzmatch(OPT); zzastDPush; zzCONSUME;
  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd2, 0x2);
  }
}

void
#ifdef __USE_PROTOS
rule(AST**_root)
#else
rule(_root)
AST **_root;
#endif
{
  zzRULE;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  SymEntry *p; int trouble=0, no_copy=0;
  zzmatch(NonTerm); zzsubroot(_root, &_sibling, &_tail);
  
  (*_root)->file = CurFile;
  (*_root)->line = zzline;
  CurRule = zzaArg(zztasp1,1 ).text;
  p = (SymEntry *) hash_get(symbols, zzaArg(zztasp1,1 ).text);
  if ( p==NULL ) {
    p = (SymEntry *) hash_add(symbols, zzaArg(zztasp1,1 ).text, (Entry *) newSymEntry(zzaArg(zztasp1,1 ).text));
    p->token = NonTerm;
    p->defined = 1;
    p->definition = (*_root);
  }
  else if ( p->token != NonTerm ) {
    err(eMsg2("rule definition clashes with %s definition: '%s'", zztokens[p->token], p->str));
    trouble = 1;
  }
  else {
    if ( p->defined ) {
      trouble = 1;
      err(eMsg1("rule multiply defined: '%s'", zzaArg(zztasp1,1 ).text));
    }
    else {
      p->defined = 1;
      p->definition = (*_root);
    }
  }
 zzCONSUME;

  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (LA(1)==84)
 ) {
      zzmatch(84); 
      if (!trouble) (*_root)->no_copy=no_copy=1;
 zzCONSUME;

    }
    else {
      if ( (setwd2[LA(1)]&0x4) ) {
      }
      else {zzFAIL(1,zzerr4,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
    zzEXIT(zztasp2);
    }
  }
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (setwd2[LA(1)]&0x8) ) {
      {
        zzBLOCK(zztasp3);
        zzMake0;
        {
        if ( (LA(1)==85) ) {
          zzmatch(85);  zzCONSUME;
        }
        else {
          if ( (LA(1)==PassAction) ) {
          }
          else {zzFAIL(1,zzerr5,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
        }
        zzEXIT(zztasp3);
        }
      }
      zzmatch(PassAction); 
      if (!trouble) p->args = actiondup(LATEXT(1));
 zzCONSUME;

    }
    else {
      if ( (setwd2[LA(1)]&0x10)
 ) {
      }
      else {zzFAIL(1,zzerr6,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
    zzEXIT(zztasp2);
    }
  }
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (LA(1)==86) ) {
      zzmatch(86);  zzCONSUME;
      zzmatch(PassAction); 
      if (!trouble) p->rt = actiondup(LATEXT(1));
 zzCONSUME;

    }
    else {
      if ( (LA(1)==LABEL) ) {
      }
      else {zzFAIL(1,zzerr7,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
    zzEXIT(zztasp2);
    }
  }
  zzmatch(LABEL);  zzCONSUME;
  block(zzSTR, no_copy ); zzlink(_root, &_sibling, &_tail);
  zzmatch(87); 
  
  if ( !trouble ) (*_root)->refvars = RefVars;
  RefVars=NULL;
 zzCONSUME;

  
  if ( trouble ) (*_root) = NULL;
  CurRule = NULL;
  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  found_error=1;  
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd2, 0x20);
  }
}

void
#ifdef __USE_PROTOS
block(AST**_root,int no_copy)
#else
block(_root,no_copy)
AST **_root;
 int no_copy ;
#endif
{
  zzRULE;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  int line=zzline, file=CurFile;
  alt(zzSTR,  no_copy ); zzlink(_root, &_sibling, &_tail);
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    while ( (LA(1)==88) ) {
      zzmatch(88);  zzCONSUME;
      alt(zzSTR,  no_copy ); zzlink(_root, &_sibling, &_tail);
      zzLOOP(zztasp2);
    }
    zzEXIT(zztasp2);
    }
  }
  
  (*_root) = zztmake( zzmk_ast(zzastnew(),BLOCK), (*_root) , NULL);
  (*_root)->file = file;
  (*_root)->line = line;
  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  found_error=1;  
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd2, 0x40);
  }
}

void
#ifdef __USE_PROTOS
alt(AST**_root,int no_copy)
#else
alt(_root,no_copy)
AST **_root;
 int no_copy ;
#endif
{
  zzRULE;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  int line=zzline, file=CurFile; int local_no_copy=0;
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (LA(1)==84) ) {
      zzmatch(84); 
      local_no_copy=1;
 zzCONSUME;

    }
    else {
      if ( (setwd2[LA(1)]&0x80)
 ) {
      }
      else {zzFAIL(1,zzerr8,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
    zzEXIT(zztasp2);
    }
  }
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (setwd3[LA(1)]&0x1) ) {
      {
        zzBLOCK(zztasp3);
        int zzcnt=1;
        zzMake0;
        {
        do {
          if ( (setwd3[LA(1)]&0x2) && (LA(2)==LABEL) ) {
            labeled_element(zzSTR,  no_copy||local_no_copy ); zzlink(_root, &_sibling, &_tail);
          }
          else {
            if ( (setwd3[LA(1)]&0x4) && (setwd3[LA(2)]&0x8) ) {
              element(zzSTR,  no_copy||local_no_copy ); zzlink(_root, &_sibling, &_tail);
            }
            else {
              if ( (LA(1)==BT) ) {
                tree(zzSTR,  no_copy||local_no_copy ); zzlink(_root, &_sibling, &_tail);
              }
              /* MR10 ()+ */ else {
                if ( zzcnt > 1 ) break;
                else {zzFAIL(2,zzerr9,zzerr10,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
              }
            }
          }
          zzcnt++; zzLOOP(zztasp3);
        } while ( 1 );
        zzEXIT(zztasp3);
        }
      }
    }
    else {
      if ( (setwd3[LA(1)]&0x10)
 ) {
      }
      else {zzFAIL(1,zzerr11,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
    zzEXIT(zztasp2);
    }
  }
  
  (*_root) = zztmake( zzmk_ast(zzastnew(),ALT), (*_root) , NULL);
  (*_root)->file = file;
  (*_root)->line = line;
  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  found_error=1;  
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd3, 0x20);
  }
}

void
#ifdef __USE_PROTOS
element(AST**_root,int no_copy)
#else
element(_root,no_copy)
AST **_root;
 int no_copy ;
#endif
{
  zzRULE;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  /**** SymEntry *p; **** MR10 ****/ int file,line; int local_no_copy=0;
  if ( (LA(1)==Token) ) {
    token(zzSTR,  no_copy ); zzlink(_root, &_sibling, &_tail);
  }
  else {
    if ( (LA(1)==NonTerm) ) {
      file = CurFile; line=zzline;
      zzmatch(NonTerm); zzsubroot(_root, &_sibling, &_tail); zzCONSUME;
      {
        zzBLOCK(zztasp2);
        zzMake0;
        {
        if ( (LA(1)==84) ) {
          zzmatch(84); 
          local_no_copy = 1;
 zzCONSUME;

        }
        else {
          if ( (setwd3[LA(1)]&0x40) ) {
          }
          else {zzFAIL(1,zzerr12,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
        }
        zzEXIT(zztasp2);
        }
      }
      {
        zzBLOCK(zztasp2);
        zzMake0;
        {
        if ( (setwd3[LA(1)]&0x80)
 ) {
          {
            zzBLOCK(zztasp3);
            zzMake0;
            {
            if ( (LA(1)==85) ) {
              zzmatch(85);  zzCONSUME;
            }
            else {
              if ( (LA(1)==PassAction) ) {
              }
              else {zzFAIL(1,zzerr13,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
            }
            zzEXIT(zztasp3);
            }
          }
          zzmatch(PassAction); zzsubchild(_root, &_sibling, &_tail);
          (*_root)->in = 1;
 zzCONSUME;

        }
        else {
          if ( (setwd4[LA(1)]&0x1) ) {
          }
          else {zzFAIL(1,zzerr14,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
        }
        zzEXIT(zztasp2);
        }
      }
      {
        zzBLOCK(zztasp2);
        zzMake0;
        {
        if ( (LA(1)==86) ) {
          zzmatch(86);  zzCONSUME;
          zzmatch(PassAction); zzsubchild(_root, &_sibling, &_tail);
          (*_root)->out = 1;
 zzCONSUME;

        }
        else {
          if ( (setwd4[LA(1)]&0x2)
 ) {
          }
          else {zzFAIL(1,zzerr15,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
        }
        zzEXIT(zztasp2);
        }
      }
      (*_root)->file = file; (*_root)->line=line;
      (*_root)->no_copy =  no_copy || local_no_copy;
    }
    else {
      if ( (LA(1)==Action) ) {
        file = CurFile; line=zzline;
        zzmatch(Action); zzsubchild(_root, &_sibling, &_tail);
        zzastArg(1)->action = actiondup(LATEXT(1));
 zzCONSUME;

        {
          zzBLOCK(zztasp2);
          zzMake0;
          {
          if ( (LA(1)==PRED_OP) ) {
            zzmatch(PRED_OP); zzsubroot(_root, &_sibling, &_tail); zzCONSUME;
          }
          else {
            if ( (setwd4[LA(1)]&0x4) ) {
            }
            else {zzFAIL(1,zzerr16,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
          }
          zzEXIT(zztasp2);
          }
        }
        (*_root)->file = file; (*_root)->line=line;
      }
      else {
        if ( (LA(1)==89) ) {
          file = CurFile; line=zzline;
          zzmatch(89);  zzCONSUME;
          block(zzSTR,  no_copy ); zzlink(_root, &_sibling, &_tail);
          zzmatch(90);  zzCONSUME;
          {
            zzBLOCK(zztasp2);
            zzMake0;
            {
            if ( (LA(1)==CLOSURE)
 ) {
              zzmatch(CLOSURE); zzsubroot(_root, &_sibling, &_tail); zzCONSUME;
            }
            else {
              if ( (LA(1)==POS_CLOSURE) ) {
                zzmatch(POS_CLOSURE); zzsubroot(_root, &_sibling, &_tail); zzCONSUME;
              }
              else {
                if ( (LA(1)==PRED_OP) ) {
                  zzmatch(PRED_OP); zzsubroot(_root, &_sibling, &_tail);
                  found_guess_block=1;
 zzCONSUME;

                }
                else {
                  if ( (setwd4[LA(1)]&0x8) ) {
                  }
                  else {zzFAIL(1,zzerr17,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                }
              }
            }
            zzEXIT(zztasp2);
            }
          }
          (*_root)->file = file; (*_root)->line=line;
        }
        else {
          if ( (LA(1)==OPT) ) {
            zzmatch(OPT); zzsubroot(_root, &_sibling, &_tail); zzCONSUME;
            block(zzSTR,  no_copy ); zzlink(_root, &_sibling, &_tail);
            zzmatch(80);  zzCONSUME;
          }
          else {
            if ( (LA(1)==WILD)
 ) {
              file = CurFile; line=zzline;
              zzmatch(WILD); zzsubchild(_root, &_sibling, &_tail); zzCONSUME;
              {
                zzBLOCK(zztasp2);
                zzMake0;
                {
                if ( (LA(1)==84) ) {
                  zzmatch(84); 
                  local_no_copy = 1;
 zzCONSUME;

                }
                else {
                  if ( (setwd4[LA(1)]&0x10) ) {
                  }
                  else {zzFAIL(1,zzerr18,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                }
                zzEXIT(zztasp2);
                }
              }
              (*_root)->no_copy =  no_copy || local_no_copy;
              (*_root)->file = file; (*_root)->line=line;
            }
            else {zzFAIL(1,zzerr19,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
          }
        }
      }
    }
  }
  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  found_error=1;  
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd4, 0x20);
  }
}

void
#ifdef __USE_PROTOS
labeled_element(AST**_root,int no_copy)
#else
labeled_element(_root,no_copy)
AST **_root;
 int no_copy ;
#endif
{
  zzRULE;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  Attrib label; int file,line; SymEntry *s; int local_no_copy=0;
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (LA(1)==Token) ) {
      zzmatch(Token); 
      label = zzaArg(zztasp2,1);
 zzCONSUME;

    }
    else {
      if ( (LA(1)==NonTerm) ) {
        zzmatch(NonTerm); 
        label = zzaArg(zztasp2,1);
 zzCONSUME;

      }
      else {zzFAIL(1,zzerr20,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
    zzEXIT(zztasp2);
    }
  }
  
  s = (SymEntry *) hash_get(symbols, label.text);
  if ( s==NULL ) {
    s = (SymEntry *) hash_add(symbols, label.text, (Entry *) newSymEntry(label.text));
    s->token = LABEL;
  }
  else if ( s->token!=LABEL ) {
    err(eMsg2("label definition clashes with %s definition: '%s'", zztokens[s->token], label.text));
  }
  zzmatch(LABEL);  zzCONSUME;
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    file = CurFile; line=zzline;
    if ( (LA(1)==Token)
 ) {
      token(zzSTR,  no_copy ); zzlink(_root, &_sibling, &_tail);
      strcpy(zzastArg(1)->label, label.text);
      (*_root)->file = file; (*_root)->line=line;
    }
    else {
      if ( (LA(1)==NonTerm) ) {
        file = CurFile; line=zzline;
        zzmatch(NonTerm); zzsubroot(_root, &_sibling, &_tail);
        strcpy(zzastArg(1)->label, label.text);
 zzCONSUME;

        {
          zzBLOCK(zztasp3);
          zzMake0;
          {
          if ( (LA(1)==84) ) {
            zzmatch(84); 
            local_no_copy = 1;
 zzCONSUME;

          }
          else {
            if ( (setwd4[LA(1)]&0x40) ) {
            }
            else {zzFAIL(1,zzerr21,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
          }
          zzEXIT(zztasp3);
          }
        }
        {
          zzBLOCK(zztasp3);
          zzMake0;
          {
          if ( (setwd4[LA(1)]&0x80) ) {
            {
              zzBLOCK(zztasp4);
              zzMake0;
              {
              if ( (LA(1)==85)
 ) {
                zzmatch(85);  zzCONSUME;
              }
              else {
                if ( (LA(1)==PassAction) ) {
                }
                else {zzFAIL(1,zzerr22,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
              }
              zzEXIT(zztasp4);
              }
            }
            zzmatch(PassAction); zzsubchild(_root, &_sibling, &_tail);
            (*_root)->in = 1;
 zzCONSUME;

          }
          else {
            if ( (setwd5[LA(1)]&0x1) ) {
            }
            else {zzFAIL(1,zzerr23,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
          }
          zzEXIT(zztasp3);
          }
        }
        {
          zzBLOCK(zztasp3);
          zzMake0;
          {
          if ( (LA(1)==86) ) {
            zzmatch(86);  zzCONSUME;
            zzmatch(PassAction); zzsubchild(_root, &_sibling, &_tail);
            (*_root)->out = 1;
 zzCONSUME;

          }
          else {
            if ( (setwd5[LA(1)]&0x2) ) {
            }
            else {zzFAIL(1,zzerr24,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
          }
          zzEXIT(zztasp3);
          }
        }
        (*_root)->file = file; (*_root)->line=line;
        (*_root)->no_copy =  no_copy || local_no_copy;
      }
      else {
        if ( (LA(1)==WILD)
 ) {
          file = CurFile; line=zzline;
          zzmatch(WILD); zzsubchild(_root, &_sibling, &_tail);
          strcpy(zzastArg(1)->label, label.text);
 zzCONSUME;

          {
            zzBLOCK(zztasp3);
            zzMake0;
            {
            if ( (LA(1)==84) ) {
              zzmatch(84); 
              local_no_copy = 1;
 zzCONSUME;

            }
            else {
              if ( (setwd5[LA(1)]&0x4) ) {
              }
              else {zzFAIL(1,zzerr25,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
            }
            zzEXIT(zztasp3);
            }
          }
          (*_root)->no_copy =  no_copy || local_no_copy;
          (*_root)->file = file; (*_root)->line=line;
        }
        else {
          if ( (setwd5[LA(1)]&0x8) ) {
            {
              zzBLOCK(zztasp3);
              zzMake0;
              {
              if ( (LA(1)==88) ) {
                zzmatch(88); zzsubchild(_root, &_sibling, &_tail); zzCONSUME;
              }
              else {
                if ( (LA(1)==87)
 ) {
                  zzmatch(87); zzsubchild(_root, &_sibling, &_tail); zzCONSUME;
                }
                else {
                  if ( (LA(1)==PassAction) ) {
                    zzmatch(PassAction); zzsubchild(_root, &_sibling, &_tail); zzCONSUME;
                  }
                  else {
                    if ( (LA(1)==Action) ) {
                      zzmatch(Action); zzsubchild(_root, &_sibling, &_tail); zzCONSUME;
                    }
                    else {
                      if ( (LA(1)==Eof) ) {
                        zzmatch(Eof); zzsubchild(_root, &_sibling, &_tail); zzCONSUME;
                      }
                      else {
                        if ( (LA(1)==89) ) {
                          zzmatch(89); zzsubchild(_root, &_sibling, &_tail); zzCONSUME;
                        }
                        else {
                          if ( (LA(1)==OPT)
 ) {
                            zzmatch(OPT); zzsubchild(_root, &_sibling, &_tail); zzCONSUME;
                          }
                          else {
                            if ( (LA(1)==90) ) {
                              zzmatch(90); zzsubchild(_root, &_sibling, &_tail); zzCONSUME;
                            }
                            else {
                              if ( (LA(1)==80) ) {
                                zzmatch(80); zzsubchild(_root, &_sibling, &_tail); zzCONSUME;
                              }
                              else {
                                if ( (LA(1)==BT) ) {
                                  zzmatch(BT); zzsubchild(_root, &_sibling, &_tail); zzCONSUME;
                                }
                                else {zzFAIL(1,zzerr26,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                              }
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
              zzEXIT(zztasp3);
              }
            }
            
            err("cannot label this grammar construct");
            found_error = 1;
          }
          else {zzFAIL(1,zzerr27,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
        }
      }
    }
    zzEXIT(zztasp2);
    }
  }
  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  found_error=1;  
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd5, 0x10);
  }
}

void
#ifdef __USE_PROTOS
token(AST**_root,int no_copy)
#else
token(_root,no_copy)
AST **_root;
 int no_copy ;
#endif
{
  zzRULE;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  SymEntry *p; int file, line; int local_no_copy=0;
  file = CurFile; line=zzline;
  zzmatch(Token); zzsubchild(_root, &_sibling, &_tail);
  (*_root)->file = file; (*_root)->line=line;
 zzCONSUME;

  define_token(zzaArg(zztasp1,1 ).text);
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (LA(1)==91) ) {
      zzmatch(91);  zzCONSUME;
      zzmatch(Token);  zzCONSUME;
      {
        zzBLOCK(zztasp3);
        zzMake0;
        {
        if ( (LA(1)==84)
 ) {
          zzmatch(84); 
          local_no_copy=1;
 zzCONSUME;

        }
        else {
          if ( (setwd5[LA(1)]&0x20) ) {
          }
          else {zzFAIL(1,zzerr28,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
        }
        zzEXIT(zztasp3);
        }
      }
      
      if ( !UserDefdTokens ) {
        err("range operator is illegal without #tokdefs directive");
      }
      else {
        p = define_token(zzaArg(zztasp2,2 ).text);
        require(p!=NULL, "token: hash table is broken");
        (*_root)->upper_range = p->token_type;
      }
    }
    else {
      if ( (LA(1)==84) ) {
        zzmatch(84); 
        local_no_copy=1;
 zzCONSUME;

      }
      else {
        if ( (setwd5[LA(1)]&0x40) ) {
        }
        else {zzFAIL(1,zzerr29,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
      }
    }
    zzEXIT(zztasp2);
    }
  }
  (*_root)->no_copy =  no_copy||local_no_copy;
  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd5, 0x80);
  }
}

void
#ifdef __USE_PROTOS
tree(AST**_root,int no_copy)
#else
tree(_root,no_copy)
AST **_root;
 int no_copy ;
#endif
{
  zzRULE;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  Attrib label; SymEntry *p, *s; int local_no_copy=0; AST *t=NULL;
  zzmatch(BT);  zzCONSUME;
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (LA(1)==Token) && (setwd6[LA(2)]&0x1) ) {
      zzmatch(Token); zzsubroot(_root, &_sibling, &_tail);
      t=zzastArg(1);
 zzCONSUME;

      define_token(zzaArg(zztasp2,1 ).text);
      {
        zzBLOCK(zztasp3);
        zzMake0;
        {
        if ( (LA(1)==91)
 ) {
          zzmatch(91);  zzCONSUME;
          zzmatch(Token);  zzCONSUME;
          {
            zzBLOCK(zztasp4);
            zzMake0;
            {
            if ( (LA(1)==84) ) {
              zzmatch(84); 
              local_no_copy=1;
 zzCONSUME;

            }
            else {
              if ( (setwd6[LA(1)]&0x2) ) {
              }
              else {zzFAIL(1,zzerr30,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
            }
            zzEXIT(zztasp4);
            }
          }
          
          if ( !UserDefdTokens ) {
            err("range operator is illegal without #tokdefs directive");
          }
          else {
            p = define_token(zzaArg(zztasp3,2 ).text);
            require(p!=NULL, "element: hash table is broken");
            t->upper_range = p->token_type;
          }
        }
        else {
          if ( (LA(1)==84) ) {
            zzmatch(84); 
            local_no_copy=1;
 zzCONSUME;

          }
          else {
            if ( (setwd6[LA(1)]&0x4) ) {
            }
            else {zzFAIL(1,zzerr31,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
          }
        }
        zzEXIT(zztasp3);
        }
      }
      t->no_copy =  no_copy||local_no_copy; t->is_root = 1;
    }
    else {
      if ( (LA(1)==WILD)
 ) {
        zzmatch(WILD); zzsubroot(_root, &_sibling, &_tail); zzCONSUME;
        {
          zzBLOCK(zztasp3);
          zzMake0;
          {
          if ( (LA(1)==84) ) {
            zzmatch(84); 
            local_no_copy = 1;
 zzCONSUME;

          }
          else {
            if ( (setwd6[LA(1)]&0x8) ) {
            }
            else {zzFAIL(1,zzerr32,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
          }
          zzEXIT(zztasp3);
          }
        }
        zzastArg(1)->no_copy =  no_copy || local_no_copy; zzastArg(1)->is_root = 1;
      }
      else {
        if ( (setwd6[LA(1)]&0x10) && (LA(2)==LABEL) ) {
          {
            zzBLOCK(zztasp3);
            zzMake0;
            {
            if ( (LA(1)==Token) ) {
              zzmatch(Token); 
              label = zzaArg(zztasp3,1);
 zzCONSUME;

            }
            else {
              if ( (LA(1)==NonTerm)
 ) {
                zzmatch(NonTerm); 
                label = zzaArg(zztasp3,1);
 zzCONSUME;

              }
              else {zzFAIL(1,zzerr33,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
            }
            zzEXIT(zztasp3);
            }
          }
          
          s = (SymEntry *) hash_get(symbols, label.text);
          if ( s==NULL ) {
            s = (SymEntry *) hash_add(symbols, label.text, (Entry *) newSymEntry(label.text));
            s->token = LABEL;
          }
          else if ( s->token!=LABEL ) {
            err(eMsg2("label definition clashes with %s definition: '%s'", zztokens[s->token], label.text));
          }
          zzmatch(LABEL);  zzCONSUME;
          {
            zzBLOCK(zztasp3);
            zzMake0;
            {
            if ( (LA(1)==Token) ) {
              zzmatch(Token); zzsubroot(_root, &_sibling, &_tail);
              strcpy(zzastArg(1)->label, label.text); t = zzastArg(1);
 zzCONSUME;

              define_token(zzaArg(zztasp3,1 ).text);
              {
                zzBLOCK(zztasp4);
                zzMake0;
                {
                if ( (LA(1)==91) ) {
                  zzmatch(91);  zzCONSUME;
                  zzmatch(Token);  zzCONSUME;
                  {
                    zzBLOCK(zztasp5);
                    zzMake0;
                    {
                    if ( (LA(1)==84) ) {
                      zzmatch(84); 
                      local_no_copy=1;
 zzCONSUME;

                    }
                    else {
                      if ( (setwd6[LA(1)]&0x20) ) {
                      }
                      else {zzFAIL(1,zzerr34,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                    }
                    zzEXIT(zztasp5);
                    }
                  }
                  
                  if ( !UserDefdTokens ) {
                    err("range operator is illegal without #tokdefs directive");
                  }
                  else {
                    p = define_token(zzaArg(zztasp4,2 ).text);
                    require(p!=NULL, "element: hash table is broken");
                    t->upper_range = p->token_type;
                  }
                }
                else {
                  if ( (LA(1)==84)
 ) {
                    zzmatch(84); 
                    local_no_copy=1;
 zzCONSUME;

                  }
                  else {
                    if ( (setwd6[LA(1)]&0x40) ) {
                    }
                    else {zzFAIL(1,zzerr35,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                  }
                }
                zzEXIT(zztasp4);
                }
              }
              t->no_copy =  no_copy||local_no_copy;
            }
            else {
              if ( (LA(1)==WILD) ) {
                zzmatch(WILD); zzsubroot(_root, &_sibling, &_tail);
                strcpy(zzastArg(1)->label, label.text);
 zzCONSUME;

                {
                  zzBLOCK(zztasp4);
                  zzMake0;
                  {
                  if ( (LA(1)==84) ) {
                    zzmatch(84); 
                    local_no_copy = 1;
 zzCONSUME;

                  }
                  else {
                    if ( (setwd6[LA(1)]&0x80) ) {
                    }
                    else {zzFAIL(1,zzerr36,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                  }
                  zzEXIT(zztasp4);
                  }
                }
                zzastArg(1)->no_copy =  no_copy || local_no_copy;
              }
              else {zzFAIL(1,zzerr37,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
            }
            zzEXIT(zztasp3);
            }
          }
          t->is_root = 1;
        }
        else {zzFAIL(2,zzerr38,zzerr39,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
      }
    }
    zzEXIT(zztasp2);
    }
  }
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    for (;;) {
      if ( !((setwd7[LA(1)]&0x1)
)) break;
      if ( (setwd7[LA(1)]&0x2) && (LA(2)==LABEL) ) {
        labeled_element(zzSTR,  no_copy ); zzlink(_root, &_sibling, &_tail);
      }
      else {
        if ( (setwd7[LA(1)]&0x4) && (setwd7[LA(2)]&0x8) ) {
          element(zzSTR,  no_copy ); zzlink(_root, &_sibling, &_tail);
        }
        else {
          if ( (LA(1)==BT) ) {
            tree(zzSTR,  no_copy ); zzlink(_root, &_sibling, &_tail);
          }
          else break; /* MR6 code for exiting loop "for sure" */
        }
      }
      zzLOOP(zztasp2);
    }
    zzEXIT(zztasp2);
    }
  }
  zzmatch(90);  zzCONSUME;
  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  found_error=1;  
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd7, 0x10);
  }
}

void
#ifdef __USE_PROTOS
enum_file(AST**_root)
#else
enum_file(_root)
AST **_root;
#endif
{
  zzRULE;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  SymEntry *p=NULL;
  if ( (setwd7[LA(1)]&0x20) ) {
    {
      zzBLOCK(zztasp2);
      zzMake0;
      {
      if ( (LA(1)==103)
 ) {
        zzmatch(103); zzastDPush; zzCONSUME;
        zzmatch(ID); zzastDPush; zzCONSUME;
        {
          zzBLOCK(zztasp3);
          zzMake0;
          {
          if ( (LA(1)==106) ) {
            zzmatch(106); zzastDPush; zzCONSUME;
            zzmatch(ID); zzastDPush; zzCONSUME;
          }
          else {
            if ( (LA(1)==110) ) {
            }
            else {zzFAIL(1,zzerr40,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
          }
          zzEXIT(zztasp3);
          }
        }
      }
      else {
        if ( (LA(1)==110) ) {
        }
        else {zzFAIL(1,zzerr41,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
      }
      zzEXIT(zztasp2);
      }
    }
    {
      zzBLOCK(zztasp2);
      int zzcnt=1;
      zzMake0;
      {
      do {
        _ast = NULL; enum_def(&_ast);
        zzLOOP(zztasp2);
      } while ( (LA(1)==110) );
      zzEXIT(zztasp2);
      }
    }
  }
  else {
    if ( (LA(1)==106)
 ) {
      _ast = NULL; defines(&_ast);
    }
    else {
      if ( (LA(1)==Eof) ) {
      }
      else {zzFAIL(1,zzerr42,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
  }
  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd7, 0x40);
  }
}

void
#ifdef __USE_PROTOS
defines(AST**_root)
#else
defines(_root)
AST **_root;
#endif
{
  zzRULE;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  int maxt= -1; /**** char *t; **** MR10 ****/ SymEntry *p; int ignore=0;
  {
    zzBLOCK(zztasp2);
    int zzcnt=1;
    zzMake0;
    {
    do {
      zzmatch(106); zzastDPush; zzCONSUME;
      zzmatch(ID); zzastDPush;
      
      p = (SymEntry *) hash_get(symbols, zzaArg(zztasp2,2 ).text);
      if ( p==NULL ) {
        p = (SymEntry *) hash_add(symbols, zzaArg(zztasp2,2 ).text,
        (Entry *) newSymEntry(zzaArg(zztasp2,2 ).text));
        require(p!=NULL, "can't add to sym tab");
        p->token = Token;
        list_add(&token_list, (void *)p);
        set_orel(p->token_type, &referenced_tokens);
      }
      else
      {
        err(eMsg1("redefinition of token %s; ignored",zzaArg(zztasp2,2 ).text));
        ignore = 1;
      }
 zzCONSUME;

      zzmatch(INT); zzastDPush;
      
      if ( !ignore ) {
        p->token_type = atoi(zzaArg(zztasp2,3 ).text);
        token_association(p->token_type, p->str);
        /*				fprintf(stderr, "#token %s=%d\n", p->str, p->token_type);*/
        if ( p->token_type>maxt ) maxt=p->token_type;
        ignore = 0;
      }
 zzCONSUME;

      zzLOOP(zztasp2);
    } while ( (LA(1)==106) );
    zzEXIT(zztasp2);
    }
  }
  token_type = maxt + 1;
  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd7, 0x80);
  }
}

void
#ifdef __USE_PROTOS
enum_def(AST**_root)
#else
enum_def(_root)
AST **_root;
#endif
{
  zzRULE;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  int maxt = -1, v= -1; /**** char *t; **** MR10 ****/ SymEntry *p; int ignore=0;
  zzmatch(110); zzastDPush; zzCONSUME;
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (LA(1)==ID) ) {
      zzmatch(ID); zzastDPush; zzCONSUME;
    }
    else {
      if ( (LA(1)==111) ) {
      }
      else {zzFAIL(1,zzerr43,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
    zzEXIT(zztasp2);
    }
  }
  zzmatch(111); zzastDPush; zzCONSUME;
  zzmatch(ID); zzastDPush;
  
  p = (SymEntry *) hash_get(symbols, zzaArg(zztasp1,4 ).text);
  if ( p==NULL ) {
    p = (SymEntry *) hash_add(symbols, zzaArg(zztasp1,4 ).text,
    (Entry *) newSymEntry(zzaArg(zztasp1,4 ).text));
    require(p!=NULL, "can't add to sym tab");
    p->token = Token;
    list_add(&token_list, (void *)p);
    set_orel(p->token_type, &referenced_tokens);
  }
  else
  {
    err(eMsg1("redefinition of token %s; ignored",zzaArg(zztasp1,4 ).text));
    ignore = 1;
  }
 zzCONSUME;

  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (LA(1)==112)
 ) {
      zzmatch(112); zzastDPush; zzCONSUME;
      zzmatch(INT); zzastDPush;
      v=atoi(zzaArg(zztasp2,2 ).text);
 zzCONSUME;

    }
    else {
      if ( (setwd8[LA(1)]&0x1) ) {
        v++;
      }
      else {zzFAIL(1,zzerr44,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
    zzEXIT(zztasp2);
    }
  }
  
  if ( !ignore ) {
    /*				fprintf(stderr, "#token %s=%d\n", p->str, v);*/
    if ( v>maxt ) maxt=v;
    p->token_type = v;
    token_association(p->token_type, p->str);
    ignore = 0;
  }
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    while ( (LA(1)==113) ) {
      zzmatch(113); zzastDPush; zzCONSUME;
      {
        zzBLOCK(zztasp3);
        zzMake0;
        {
        if ( (LA(1)==114) ) {
          zzmatch(114); zzastDPush; zzCONSUME;
          {
            zzBLOCK(zztasp4);
            zzMake0;
            {
            if ( (LA(1)==112) ) {
              zzmatch(112); zzastDPush; zzCONSUME;
              zzmatch(INT); zzastDPush; zzCONSUME;
            }
            else {
              if ( (setwd8[LA(1)]&0x2)
 ) {
              }
              else {zzFAIL(1,zzerr45,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
            }
            zzEXIT(zztasp4);
            }
          }
        }
        else {
          if ( (LA(1)==115) ) {
            zzmatch(115); zzastDPush; zzCONSUME;
            {
              zzBLOCK(zztasp4);
              zzMake0;
              {
              if ( (LA(1)==112) ) {
                zzmatch(112); zzastDPush; zzCONSUME;
                zzmatch(INT); zzastDPush; zzCONSUME;
              }
              else {
                if ( (setwd8[LA(1)]&0x4) ) {
                }
                else {zzFAIL(1,zzerr46,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
              }
              zzEXIT(zztasp4);
              }
            }
          }
          else {
            if ( (LA(1)==ID) ) {
              zzmatch(ID); zzastDPush;
              
              p = (SymEntry *) hash_get(symbols, zzaArg(zztasp3,1 ).text);
              if ( p==NULL ) {
                p = (SymEntry *) hash_add(symbols, zzaArg(zztasp3,1 ).text,
                (Entry *) newSymEntry(zzaArg(zztasp3,1 ).text));
                require(p!=NULL, "can't add to sym tab");
                p->token = Token;
                list_add(&token_list, (void *)p);
                set_orel(p->token_type, &referenced_tokens);
              }
              else
              {
                err(eMsg1("redefinition of token %s; ignored",zzaArg(zztasp3,1 ).text));
                ignore = 1;
              }
 zzCONSUME;

              {
                zzBLOCK(zztasp4);
                zzMake0;
                {
                if ( (LA(1)==112)
 ) {
                  zzmatch(112); zzastDPush; zzCONSUME;
                  zzmatch(INT); zzastDPush;
                  v=atoi(zzaArg(zztasp4,2 ).text);
 zzCONSUME;

                }
                else {
                  if ( (setwd8[LA(1)]&0x8) ) {
                    v++;
                  }
                  else {zzFAIL(1,zzerr47,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                }
                zzEXIT(zztasp4);
                }
              }
              
              if ( !ignore ) {
                /*						fprintf(stderr, "#token %s=%d\n", p->str, v);*/
                if ( v>maxt ) maxt=v;
                p->token_type = v;
                token_association(p->token_type, p->str);
                ignore = 0;
              }
            }
            else {
              if ( (setwd8[LA(1)]&0x10) ) {
              }
              else {zzFAIL(1,zzerr48,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
            }
          }
        }
        zzEXIT(zztasp3);
        }
      }
      zzLOOP(zztasp2);
    }
    zzEXIT(zztasp2);
    }
  }
  zzmatch(116); zzastDPush; zzCONSUME;
  zzmatch(117); zzastDPush;
  token_type = maxt + 1;
 zzCONSUME;

  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd8, 0x20);
  }
}

/* SORCERER-specific syntax error message generator
* (define USER_ZZSYN when compiling so don't get 2 definitions)
*/
void
#ifdef __USE_PROTOS
zzsyn(char *text, int tok, char *egroup, SetWordType *eset, int etok, int k, char *bad_text)
#else
zzsyn(text, tok, egroup, eset, etok, k, bad_text)
char *text, *egroup, *bad_text;
int tok;
int etok;
int k;
SetWordType *eset;
#endif
{
fprintf(stderr, ErrHdr, FileStr[CurFile]!=NULL?FileStr[CurFile]:"stdin", zzline);
fprintf(stderr, " syntax error at \"%s\"", (tok==zzEOF_TOKEN)?"EOF":text);
if ( !etok && !eset ) {fprintf(stderr, "\n"); return;}
if ( k==1 ) fprintf(stderr, " missing");
else
{
fprintf(stderr, "; \"%s\" not", bad_text);
if ( zzset_deg(eset)>1 ) fprintf(stderr, " in");
}
if ( zzset_deg(eset)>0 ) zzedecode(eset);
else fprintf(stderr, " %s", zztokens[etok]);
if ( strlen(egroup) > (size_t)0 ) fprintf(stderr, " in %s", egroup);
fprintf(stderr, "\n");
}

SymEntry *
#ifdef __USE_PROTOS
define_token(char *text)
#else
define_token(text)
char *text;
#endif
{
SymEntry *p;

	p = (SymEntry *) hash_get(symbols, text);
if ( p==NULL ) {
if ( UserDefdTokens ) {
err(eMsg1("implicit token definition of '%s' not allowed with #tokdefs",text));
}
p = (SymEntry *) hash_add(symbols, text, (Entry *) newSymEntry(text));
p->token = Token;
p->token_type = token_type++;
token_association(p->token_type, p->str);
list_add(&token_list, (void *)p);
set_orel(p->token_type, &referenced_tokens);
}
else {
if ( p->token!=Token )
err(eMsg2("token definition clashes with %s definition: '%s'", zztokens[p->token], text));
}
return p;
}
