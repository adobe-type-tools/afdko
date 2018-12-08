#header <<
#include "charbuf.h"
#define AtomSize	20
#define AST_FIELDS	char text[AtomSize+1]; int token;
#define zzcr_ast(tr, attr, tok, txt) { strcpy(tr->text, txt); tr->token=tok; }
>>

#token "[\ \t]+"	<<zzskip();>>
#token "\n"			<<zzline++; zzskip();>>
#token ASSIGN	"="
#token ADD		"\+"
#token MULT		"\*"

stat:	ID "="^ expr ";"!
	;

expr:	mop_expr ( "\+"^ mop_expr )*
	;

mop_expr
	:	atom ( "\*"^ atom )*
	;

atom:	ID
	|	INT
	;

#token ID	"[a-z]+"
#token INT	"[0-9]+"
