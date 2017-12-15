<<
typedef ANTLRCommonToken ANTLRToken;
#include "AST.h"
>>

#token "[\ \t]+"	<<skip();>>
#token "\n"			<<newline(); skip();>>
#token ASSIGN	"="
#token ADD		"\+"
#token MULT		"\*"

class SimpleParser {

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

}
#token ID	"[a-z]+"
#token INT	"[0-9]+"
