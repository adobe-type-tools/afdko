/*
 * SORCERER Version 1.00B
 *
 * Terence Parr
 * U of MN, AHPCRC
 * April 1995
 */

#header <<
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
>>

<<
/* MR20 G. Hobbelt Fix for Borland C++ 4.x & 5.x compiling with ALL warnings enabled */
#if defined(__TURBOC__)
#pragma warn -aus  /* unused assignment of 'xxx' */
#endif


#include "sym.h"
#include "proto.h"
>>

#lexaction <<
#include "sym.h"
#include "proto.h"

int define_num = 0;

char *
#ifdef __USE_PROTOS
scarf_to_end_of_func_call(void)
#else
scarf_to_end_of_func_call()
#endif
{
	static char func_call_str[MaxAtom+1];
	char *p;

	p = &func_call_str[0];

more:
	if ( zzchar==')' ) { *p++ = zzchar; *p++ = '\0'; zzadvance(); return func_call_str; }
	if ( zzchar=='"' )
	{
		*p++ = zzchar; zzadvance();
		while ( zzchar!='"' )
		{
			if ( zzchar=='\\' ) { *p++ = zzchar; zzadvance(); }
			*p++ = zzchar; zzadvance();
		}
	}
	*p++ = zzchar; zzadvance();
	goto more;
}
>>

<<
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
>>

#lexclass STRINGS
#token RExpr "\""			<< zzmode(START); >>
#token "\n|\r|\r\n"			<<                  /* MR16a */
							zzline++;
							warn("eoln found in string");
							zzskip();
							>>
#token "\\~[]"				<< zzmore(); >>
#token "~[\n\r\"\\]+"		<< zzmore(); >>     /* MR16a */

#lexclass ACTION_STRINGS
#token "\""					<< zzmode(ACTIONS); zzmore(); >>
#token "\n|\r|\r\n"			<<                  /* MR16a */
							zzline++;
							warn("eoln found in string (in user action)");
							zzskip();
							>>
#token "\\~[]"				<< zzmore(); >>
#token "~[\n\r\"\\]+"		<< zzmore(); >>     /* MR16a */

#lexclass ACTION_CHARS
#token "'"					<< zzmode(ACTIONS); zzmore(); >>
#token "\n|\r|\r\n"			<<                  /* MR16a */
							zzline++;
							warn("eoln found in char literal (in user action)");
							zzskip();
							>>
#token "\\~[]"				<< zzmore(); >>
#token "~[\n\r'\\]+"		<< zzmore(); >>     /* MR16a */

#lexclass ACTION_COMMENTS
#token "\*/"				<< zzmode(ACTIONS); zzmore(); >>
#token "\*"					<< zzmore(); >>
#token "\n|\r|\r\n"			<< zzline++; zzmore(); >>   /* MR16a */
#token "~[\n\r\*]+"			<< zzmore(); >>             /* MR16a */

#lexclass ACTION_CPP_COMMENTS
#token "\n|\r|\r\n"			<< zzline++; zzmode(ACTIONS); zzmore(); >>  /* MR16a */
#token "~[\n\r]+"			<< zzmore(); >>                             /* MR16a */

#lexclass CPP_COMMENTS
#token "\n|\r|\r\n"			<< zzline++; zzmode(START); zzskip(); >>    /* MR16a */
#token "~[\n\r]+"			<< zzskip(); >>                             /* MR16a */

#lexclass COMMENTS
#token "\*/"				<< zzmode(START); zzskip(); >>
#token "\*"					<< zzskip(); >>
#token "\n|\r|\r\n"			<< zzline++; zzskip(); >>                   /* MR16a */
#token "~[\n\r\*]+"			<< zzskip(); >>                             /* MR16a */

#lexclass REFVAR_SCARF		/* everything until a ')' */
#token "~[\)]+ \)"			<<{
							RefVarRec *rf;
							zzskip();
                            zzbegexpr[strlen(zzbegexpr)-1] = '\0';
							rf=refVarRec(zzbegexpr);
							list_add(&AllRefVars, rf);
							list_add(&RefVars, rf);
                            zzmode(ACTIONS); zzmore(); zzreplstr("");
							}>>

/*
 * This lexical class accepts actions of type [..] and <<..>>
 *
 * It translates the following special items:
 *
 * #[args]	--> "ast_node(args)" add "classname::" for C++, however.
 * #[]		--> "ast_empty_node()"
 * #( root, child1, ..., childn )
			--> "ast_make(root, child1, ...., childn, NULL)"
 * #()		--> "NULL"
 *
 * Things for reference variables are also recognized:
 * blah blah
 *
 * To escape,
 *
 * \]		--> ]
 * \)		--> )
 * \$		--> $
 * \#		--> #
 *
 * A stack is used to nest action terminators because they can be nested
 * like crazy:  << #[#[..],..] >>
 */
#lexclass ACTIONS
#token Action "\>\>"        << /* these do not nest */
                              zzmode(START);
                              NLATEXT[0] = ' ';
                              NLATEXT[1] = ' ';
                              zzbegexpr[0] = ' ';
                              zzbegexpr[1] = ' ';
							  if ( zzbufovf ) {
								found_error = 1;
								err( eMsgd("action buffer overflow; size %d",ZZLEXBUFSIZE));
							  }
                            >>
#token PassAction "\]"		<< if ( topint() == ']' ) {
								  popint();
								  if ( istackempty() )	/* terminate action */
								  {
									  zzmode(START);
									  NLATEXT[0] = ' ';
									  zzbegexpr[0] = ' ';
									  if ( zzbufovf ) {
										found_error = 1;
										err( eMsgd("parameter buffer overflow; size %d",ZZLEXBUFSIZE));
									  }
								  }
								  else {
									  /* terminate #[..] */
									  zzreplstr(")");
									  zzmore();
								  }
							   }
							   else if ( topint() == '|' ) { /* end of simple [...] */
								  popint();
								  zzmore();
							   }
							   else zzmore();
							>>
#token "\n|\r|\r\n"			<< zzline++; zzmore(); >>   /* MR16a */
#token "\>"					<< zzmore(); >>
#token "#[_a-zA-Z][_a-zA-Z0-9]*"
							<<
							if ( !(strcmp(zzbegexpr, "#ifdef")==0 ||
								 strcmp(zzbegexpr, "#else")==0 ||
								 strcmp(zzbegexpr, "#endif")==0 ||
								 strcmp(zzbegexpr, "#ifndef")==0 ||
								 strcmp(zzbegexpr, "#if")==0 ||
								 strcmp(zzbegexpr, "#define")==0 ||
								 strcmp(zzbegexpr, "#pragma")==0 ||
								 strcmp(zzbegexpr, "#undef")==0 ||
								 strcmp(zzbegexpr, "#import")==0 ||
								 strcmp(zzbegexpr, "#line")==0 ||
								 strcmp(zzbegexpr, "#include")==0 ||
								 strcmp(zzbegexpr, "#error")==0) )
							{
								static char buf[100];
								if ( !transform ) {
								  warn("#id used in nontransform mode; # ignored");
								  sprintf(buf, "%s", zzbegexpr+1);
								}
								else {
									if ( CurRule==NULL )
									  {warn("#id used in action outside of rule; ignored");}
									else if ( strcmp(zzbegexpr+1,CurRule)==0 )
										strcpy(buf, "(*_result)");
								}
								zzreplstr(buf);
							}
							zzmore();
							>>
#token "#\[\]"				<<
							if ( GenCPP ) zzreplstr("new SORAST");
							else zzreplstr("ast_empty_node()");
							zzmore();
							>>
#token "#\(\)"				<< zzreplstr("NULL"); zzmore(); >>
#token "#\["				<<
							pushint(']');
							if ( GenCPP ) zzreplstr("new SORAST(");
							else zzreplstr("ast_node(");
							zzmore();
							>>
#token "#\("				<<
							pushint('}');
							if ( GenCPP ) zzreplstr("PCCTS_AST::make(");
							else zzreplstr("ast_make(");
							zzmore();
							>>
#token "#"					<< zzmore(); >>
#token "\)"					<<
							if ( istackempty() )
								zzmore();
							else if ( topint()==')' ) {
								popint();
							}
							else if ( topint()=='}' ) {
								popint();
								/* terminate #(..) */
								zzreplstr(", NULL)");
							}
							zzmore();
							>>
#token "\["					<<
							pushint('|');	/* look for '|' to terminate simple [...] */
							zzmore();
							>>
#token "\("					<<
							pushint(')');
							zzmore();
							>>

#token "\\\]"				<< zzreplstr("]");  zzmore(); >>
#token "\\\)"				<< zzreplstr(")");  zzmore(); >>
#token "\\>"				<< zzreplstr(">");  zzmore(); >>


#token "'"					<< zzmode(ACTION_CHARS); zzmore();>>
#token "\""					<< zzmode(ACTION_STRINGS); zzmore();>>
#token "\\#"				<< zzreplstr("#");  zzmore(); >>
/*#token "\\\\"				<< zzmore(); >> /* need this for some reason */
#token "\\~[\]\)>#]"		<< zzmore(); >> /* escaped char, always ignore */
#token "/"					<< zzmore(); >>
#token "/\*"				<< zzmode(ACTION_COMMENTS); zzmore(); >>
#token "\*/"				<< err("Missing /*; found dangling */ in action"); zzmore(); >>
#token "//"					<< zzmode(ACTION_CPP_COMMENTS); zzmore(); >>

#token "\@\("				<<zzmode(REFVAR_SCARF); zzmore(); zzreplstr("");>>

#token "\@"					<<
							zzmore(); if ( !GenCPP ) zzreplstr("_parser->");
							>>

#token "[a-zA-Z_]+\("		<<
							if ( (GenCPP && strcmp(zzbegexpr,"ast_scan(")==0) ||
							     (!GenCPP && strcmp(zzbegexpr,"ast_scan(")==0) ) {
								char *args=scarf_to_end_of_func_call();
								zzreplstr(cvt_token_str(zzbegexpr, args));
								zzmore();
							}
							else { pushint(')'); zzmore(); }
							>>
#token "[a-zA-Z_]+"			<< zzmore(); >>
#token "~[a-zA-Z_\n\r\)\(\\#\>\]\[\"'/\@]+" << zzmore(); >>     /* MR16a */

#lexclass START
#token "[\t\ ]+"			<< zzskip(); >>				/* Ignore White */
#token "\n|\r|\n\r"			<< zzline++; zzskip(); >>	/* Track Line # */  /* MR16a */
#token "\["                 << zzmode(ACTIONS); zzmore();
                               istackreset();
                               pushint(']'); >>
#token "\<\<"               << action_file=CurFile; action_line=zzline;
                               zzmode(ACTIONS); zzmore();
                               istackreset();
                               pushint('>'); >>
#token "\""					<< zzmode(STRINGS); zzmore(); >>
#token "/\*"				<< zzmode(COMMENTS); zzskip(); >>
#token "\*/"				<< err("Missing /*; found dangling */"); zzskip(); >>
#token "//"					<< zzmode(CPP_COMMENTS); zzskip(); >>
#token "\>\>"				<< err("Missing <<; found dangling \>\>"); zzskip(); >>
#token Eof					"@"
							<<	/* L o o k  F o r  A n o t h e r  F i l e */
							{
							FILE *new_input;
							new_input = NextFile();
							if ( new_input != NULL ) {
								fclose( input );
								input = new_input;
								zzrdstream( input );
								/*zzadvance();	** Get 1st char of this file */
								zzskip();	/* Skip the Eof (@) char i.e continue */
							}
							}
							>>

#token Header		"#header"
#token Tokdef		"#tokdefs"
#token BLOCK			/* used only as place-holder in intermediate tree */
#token ALT				/* used only as place-holder in intermediate tree */
#token LABEL		":"	/* used only as place-holder in intermediate tree */
#token OPT			"\{" /* These are labeled so we can ref them in trees */
#token POS_CLOSURE	"\+"
#token CLOSURE		"\*"
#token WILD			"."
#token PRED_OP		"?"
#token BT			"#\("
#token RULE
#token REFVAR

#errclass "atomic-element" { WILD NonTerm Token }
#errclass "rule-header" { PassAction LABEL "\<" "\>" }

/*
 * Build trees for a sorcerer description
 */
sordesc
	:	<<int he=0,to=0;>>
		(	header <<he++;>>
		|	tokdef <<to++;>>
		)*
		<<
		if ( he==0 && !Inline && !GenCPP ) warnNoFL("missing #header statement");
		if ( he>1 ) warnNoFL("extra #header statement");
		if ( to>1 ) warnNoFL("extra #tokdef statement");
		>>
		(	Action!
			<<list_add(&before_actions, actiondup(LATEXT(1)));>>
		)*
		{ class_def }
		(	Action!
			<<
			if ( CurClassName[0]!='\0' )
				list_add(&class_actions, actiondup(LATEXT(1)));
			else
				list_add(&before_actions, actiondup(LATEXT(1)));
			>>
		)*
		( rule )*
		(	Action!
			<<
			if ( CurClassName[0]!='\0' )
				list_add(&class_actions, actiondup(LATEXT(1)));
			else
				list_add(&before_actions, actiondup(LATEXT(1)));
			>>
		)*
		{	"\}"!	// end of class def
			<<
			if ( CurClassName[0]=='\0' )
				err("missing class definition for trailing '}'");
			>>
		}
		(	Action!
			<<list_add(&after_actions, actiondup(LATEXT(1)));>>
		)*
		"@"!
	;
	<<found_error=1;>>

header:	"#header"! Action! <<header_action = actiondup(LATEXT(1));>>
	  ;
	<<found_error=1;>>

tokdef: "#tokdefs"! RExpr!
		<<{
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
		}>>
	  ;
	<<found_error=1;>>

class_def!
	:	"class"
		(	NonTerm		<<strncpy(CurClassName,LATEXT(1),MaxAtom);>>
		|	Token		<<strncpy(CurClassName,LATEXT(1),MaxAtom);>>
		)
		<<if ( !GenCPP ) { err("class meta-op used without C++ option"); }>>
		"\{"
	;

/*
 * Create a rule tree:
 *
 *  NonTerm[arg_action, ret_val_action]
 *      |
 *      v
 *    block
 */
rule:	<<SymEntry *p; int trouble=0, no_copy=0;>>
		NonTerm^
		<<
		#0->file = CurFile;
		#0->line = zzline;
		CurRule = $1.text;
		p = (SymEntry *) hash_get(symbols, $1.text);
		if ( p==NULL ) {
			p = (SymEntry *) hash_add(symbols, $1.text, (Entry *) newSymEntry($1.text));
			p->token = NonTerm;
			p->defined = 1;
			p->definition = #0;
		}
		else if ( p->token != NonTerm ) {
			err(eMsg2("rule definition clashes with %s definition: '%s'", zztokens[p->token], p->str));
			trouble = 1;
		}
		else {
			if ( p->defined ) {
				trouble = 1;
				err(eMsg1("rule multiply defined: '%s'", $1.text));
			}
			else {
				p->defined = 1;
				p->definition = #0;
			}
		}
		>>
		{ "!"! <<if (!trouble) #0->no_copy=no_copy=1;>> }
		(	{ "\<"! } PassAction!	<<if (!trouble) p->args = actiondup(LATEXT(1));>>
		|
		)
		{	"\>"! PassAction!		<<if (!trouble) p->rt = actiondup(LATEXT(1));>>
		}
		":"!
		block[no_copy]
		";"!
		<<
		if ( !trouble ) #0->refvars = RefVars;
		RefVars=NULL;
		>>
		<<
		if ( trouble ) #0 = NULL;
		CurRule = NULL;
		>>
	;
	<<found_error=1;>>

/* Create a tree for a block:
 *
 *  BLOCK
 *    |
 *    v
 *   alt1 ---> alt2 ---> ... ---> altn
 */
block[int no_copy]
	:	<<int line=zzline, file=CurFile;>>
		alt[$no_copy]
		(	"\|"! alt[$no_copy]
		)*
		<<
		#0 = #( #[BLOCK], #0 );
		#0->file = file;
		#0->line = line;
		>>
	;
	<<found_error=1;>>

/* Create a tree for an alternative:
 *
 *  ALT
 *   |
 *   v
 *  e1 ---> e2 ---> ... ---> en
 */
alt[int no_copy]
		:	<<int line=zzline, file=CurFile; int local_no_copy=0;>>
			{ "!"! <<local_no_copy=1;>> }
			{
				( 	labeled_element[$no_copy||local_no_copy]
				|	element[$no_copy||local_no_copy]
				|	tree[$no_copy||local_no_copy]
				)+
			}
			<<
			#0 = #( #[ALT], #0 );
			#0->file = file;
			#0->line = line;
			>>
	;
	<<found_error=1;>>

/* a rule ref looks like:
 *
 *   NonTerm
 *       |
 *       v
 *   arg_action ---> ret_val_action
 *
 * Blocks that have a suffix look like this:
 *
 *     OP
 *     |
 *     v
 *   block
 *
 * Optional blocks look like:
 *
 *    OPT
 *     |
 *     v
 *   block
 *
 * Predicates are like actions except they have a root: #( PRED_OP Action )
 */
element[int no_copy]
	:	<</**** SymEntry *p; **** MR10 ****/ int file,line; int local_no_copy=0;>>
		token[$no_copy]
	|	<<file = CurFile; line=zzline;>>
		NonTerm^
		{ "!"! <<local_no_copy = 1;>> }
		{ { "\<"! } PassAction <<#0->in = 1;>> }
		{ "\>"! PassAction <<#0->out = 1;>> }
		<<#0->file = file; #0->line=line;>>
		<<#0->no_copy = $no_copy || local_no_copy;>>
	|	<<file = CurFile; line=zzline;>>
		Action		<<#1->action = actiondup(LATEXT(1));>>
		{ "?"^ }	<<#0->file = file; #0->line=line;>>
	|	<<file = CurFile; line=zzline;>>
		"\("! block[$no_copy] "\)"!
		(	"\*"^
		|	"\+"^
		|	"?"^	<<found_guess_block=1;>>
		|
		)
		<<#0->file = file; #0->line=line;>>
	|	"\{"^ block[$no_copy] "\}"!
	|	<<file = CurFile; line=zzline;>>
		"."
		{ "!"! <<local_no_copy = 1;>> }
		<<#0->no_copy = $no_copy || local_no_copy;>>
		<<#0->file = file; #0->line=line;>>
	;
	<<found_error=1;>>

/* labels on an element look like: #( LABEL element ) */
labeled_element[int no_copy]
	:	<<Attrib label; int file,line; SymEntry *s; int local_no_copy=0;>>
		(	Token!		<<label = $1;>>
		|	NonTerm!	<<label = $1;>>
		)
		<<
		s = (SymEntry *) hash_get(symbols, label.text);
		if ( s==NULL ) {
			s = (SymEntry *) hash_add(symbols, label.text, (Entry *) newSymEntry(label.text));
			s->token = LABEL;
		}
		else if ( s->token!=LABEL ) {
			err(eMsg2("label definition clashes with %s definition: '%s'", zztokens[s->token], label.text));
		}
		>>
		":"!				
		(	<<file = CurFile; line=zzline;>>
			token[$no_copy]		<<strcpy(#1->label, label.text);>>
			<<#0->file = file; #0->line=line;>>
		|	<<file = CurFile; line=zzline;>>
			NonTerm^	<<strcpy(#1->label, label.text);>>
			{ "!"! <<local_no_copy = 1;>> }
			{ { "\<"! } PassAction <<#0->in = 1;>> }
			{ "\>"! PassAction <<#0->out = 1;>> }
			<<#0->file = file; #0->line=line;>>
			<<#0->no_copy = $no_copy || local_no_copy;>>
		|	<<file = CurFile; line=zzline;>>
			"."			<<strcpy(#1->label, label.text);>>
			{ "!"! <<local_no_copy = 1;>> }
			<<#0->no_copy = $no_copy || local_no_copy;>>
			<<#0->file = file; #0->line=line;>>
		|	("\|" | ";" | PassAction | Action | Eof | "\(" | "\{" | "\)" | "\}" | "#\(")
			<<
			err("cannot label this grammar construct");
			found_error = 1;
			>>
		)
	;
	<<found_error=1;>>

token[int no_copy]
	:	<<SymEntry *p; int file, line; int local_no_copy=0;>>
		<<file = CurFile; line=zzline;>>
		Token
		<<#0->file = file; #0->line=line;>>
		<<define_token($1.text);>>
		{	".."! Token!
			{ "!"! <<local_no_copy=1;>>}
			<<
			if ( !UserDefdTokens ) {
				err("range operator is illegal without #tokdefs directive");
			}
			else {
				p = define_token($2.text);
				require(p!=NULL, "token: hash table is broken");
				#0->upper_range = p->token_type;
			}
			>>
		|	"!"! <<local_no_copy=1;>>
		}
		<<#0->no_copy = $no_copy||local_no_copy;>>
	;

/* A tree description looks like:
 *
 *   BT
 *   |
 *   v
 *  root ---> sibling1 ---> ... ---> siblingn
 */
tree[int no_copy]
	:	<<Attrib label; SymEntry *p, *s; int local_no_copy=0; AST *t=NULL;>>
		"#\("!
		(	Token^ <<t=#1;>>
			<<define_token($1.text);>>
			{	".."! Token!	/* inline rather than 'token' because must be root */
				{ "!"! <<local_no_copy=1;>>}
				<<
				if ( !UserDefdTokens ) {
					err("range operator is illegal without #tokdefs directive");
				}
				else {
					p = define_token($2.text);
					require(p!=NULL, "element: hash table is broken");
					t->upper_range = p->token_type;
				}
				>>
			|	"!"! <<local_no_copy=1;>>
			}
			<<t->no_copy = $no_copy||local_no_copy; t->is_root = 1;>>
		|	"."^
			{ "!"! <<local_no_copy = 1;>> }
			<<#1->no_copy = $no_copy || local_no_copy; #1->is_root = 1;>>
		|	(	Token!		<<label = $1;>>
			|	NonTerm!	<<label = $1;>>
			)
			<<
			s = (SymEntry *) hash_get(symbols, label.text);
			if ( s==NULL ) {
				s = (SymEntry *) hash_add(symbols, label.text, (Entry *) newSymEntry(label.text));
				s->token = LABEL;
			}
			else if ( s->token!=LABEL ) {
				err(eMsg2("label definition clashes with %s definition: '%s'", zztokens[s->token], label.text));
			}
			>>
			":"!
			(	Token^		<<strcpy(#1->label, label.text); t = #1;>>
				<<define_token($1.text);>>
				{	".."! Token!	/* inline rather than 'token' because must be root */
					{ "!"! <<local_no_copy=1;>>}
					<<
					if ( !UserDefdTokens ) {
						err("range operator is illegal without #tokdefs directive");
					}
					else {
						p = define_token($2.text);
						require(p!=NULL, "element: hash table is broken");
						t->upper_range = p->token_type;
					}
					>>
				|	"!"! <<local_no_copy=1;>>
				}
				<<t->no_copy = $no_copy||local_no_copy;>>
			|	"."^ <<strcpy(#1->label, label.text);>>
				{ "!"! <<local_no_copy = 1;>> }
				<<#1->no_copy = $no_copy || local_no_copy;>>
			)
			<<t->is_root = 1;>>
		)
		( labeled_element[$no_copy] | element[$no_copy] | tree[$no_copy] )*
		"\)"!
	;
	<<found_error=1;>>

#token NonTerm			"[a-z] [A-Za-z0-9_]*"
#token Token			"[A-Z] [A-Za-z0-9_]*"
#token "#[A-Za-z0-9_]*" <<warn(eMsg1("unknown meta-op: %s",LATEXT(1))); zzskip(); >>

                     /* # t o k d e f s  s t u f f */

#lexclass TOK_DEF_COMMENTS
#token "\*/"				<< zzmode(PARSE_ENUM_FILE); zzmore(); >>
#token "\*"					<< zzmore(); >>
#token "\n|\r|\r\n"			<< zzline++; zzmore(); >>   /* MR16a */
#token "~[\n\r\*]+"			<< zzmore(); >>             /* MR16a */

#lexclass TOK_DEF_CPP_COMMENTS
#token "\n|\r|\r\n"			<< zzline++; zzmode(PARSE_ENUM_FILE); zzskip(); >> /* MR16a */
#token "~[\n\r]+"			<< zzskip(); >>                                    /* MR16a */

#lexclass PARSE_ENUM_FILE

#token "[\t\ ]+"			<< zzskip(); >>				/* Ignore White */
#token "\n|\r|\r\n"			<< zzline++; zzskip(); >>	/* Track Line # */  /* MR16a */
#token "//"					<< zzmode(TOK_DEF_CPP_COMMENTS); zzmore(); >>
#token "/\*"				<< zzmode(TOK_DEF_COMMENTS); zzmore(); >>
#token "#ifndef"			<<  >>
#token "#ifdef"				<< zzmode(TOK_DEF_CPP_COMMENTS); zzskip(); >>
#token "#else"				<< zzmode(TOK_DEF_CPP_COMMENTS); zzskip(); >>
#token "#define"			<<
							>>
#token "#endif"				<< zzmode(TOK_DEF_CPP_COMMENTS); zzskip(); >>
#token "@"					<< /*zzmode(START); zzskip();*/ >>

enum_file!
	:	<<SymEntry *p=NULL;>>
		{	"#ifndef" ID
			{	"#define" ID /* ignore if it smells like a gate */
				/* First #define after the first #ifndef (if any) is ignored */
			}
		}
		( enum_def )+
	|	defines
	|
	;

defines!
	:	<<int maxt= -1; /**** char *t; **** MR10 ****/ SymEntry *p; int ignore=0;>>
		(
			"#define" ID
			<<
			p = (SymEntry *) hash_get(symbols, $2.text);
			if ( p==NULL ) {
				p = (SymEntry *) hash_add(symbols, $2.text,
										  (Entry *) newSymEntry($2.text));
				require(p!=NULL, "can't add to sym tab");
				p->token = Token;
				list_add(&token_list, (void *)p);
				set_orel(p->token_type, &referenced_tokens);
			}
			else
			{
				err(eMsg1("redefinition of token %s; ignored",$2.text));
				ignore = 1;
			}
			>>
			INT
			<<
			if ( !ignore ) {
				p->token_type = atoi($3.text);
				token_association(p->token_type, p->str);
/*				fprintf(stderr, "#token %s=%d\n", p->str, p->token_type);*/
				if ( p->token_type>maxt ) maxt=p->token_type;
				ignore = 0;
			}
			>>
		)+
		<<token_type = maxt + 1;>>	/* record max defined token */
	;

enum_def!
	:	<<int maxt = -1, v= -1; /**** char *t; **** MR10 ****/ SymEntry *p; int ignore=0;>>
		"enum" {ID}
		"\{"
			ID
			<<
			p = (SymEntry *) hash_get(symbols, $4.text);
			if ( p==NULL ) {
				p = (SymEntry *) hash_add(symbols, $4.text,
										  (Entry *) newSymEntry($4.text));
				require(p!=NULL, "can't add to sym tab");
				p->token = Token;
				list_add(&token_list, (void *)p);
				set_orel(p->token_type, &referenced_tokens);
			}
			else
			{
				err(eMsg1("redefinition of token %s; ignored",$4.text));
				ignore = 1;
			}
			>>
			(	"=" INT	<<v=atoi($2.text);>>
			|			<<v++;>>
			)
			<<
			if ( !ignore ) {
/*				fprintf(stderr, "#token %s=%d\n", p->str, v);*/
				if ( v>maxt ) maxt=v;
				p->token_type = v;
				token_association(p->token_type, p->str);
				ignore = 0;
			}
			>>
			(	","
                { "DLGminToken" { "=" INT }          /* 1.33MR6 compatibility */
                | "DLGmaxToken" { "=" INT }          /* 1.33MR6 compatibility */
				|	ID
					<<
					p = (SymEntry *) hash_get(symbols, $1.text);
					if ( p==NULL ) {
						p = (SymEntry *) hash_add(symbols, $1.text,
												  (Entry *) newSymEntry($1.text));
						require(p!=NULL, "can't add to sym tab");
						p->token = Token;
						list_add(&token_list, (void *)p);
						set_orel(p->token_type, &referenced_tokens);
					}
					else
					{
						err(eMsg1("redefinition of token %s; ignored",$1.text));
						ignore = 1;
					}
					>>
					(	"=" INT	<<v=atoi($2.text);>>
					|			<<v++;>>
					)
					<<
					if ( !ignore ) {
/*						fprintf(stderr, "#token %s=%d\n", p->str, v);*/
						if ( v>maxt ) maxt=v;
						p->token_type = v;
						token_association(p->token_type, p->str);
						ignore = 0;
					}
					>>
				}
			)*
		"\}"
		";"
		<<token_type = maxt + 1;>>	/* record max defined token */
	;

#token INT	"[0-9]+"
#token ID	"[a-zA-Z_][_a-zA-Z0-9]*"

#lexclass START

<<
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
>>
