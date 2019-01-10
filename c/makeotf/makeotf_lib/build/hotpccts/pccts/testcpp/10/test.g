/* This tests a simple DLG-based scanner plus (...)? predicates */

<<
typedef ANTLRCommonToken ANTLRToken;
#include "DLGLexer.h"
#include "PBlackBox.h"

int main()
{
	ParserBlackBox<DLGLexer, Expr, ANTLRToken> p(stdin);
	p.parser()->begin();
	return 0;
}

void doit(Expr *p)
{
	printf("LT(1) is %s\n", ((ANTLRToken *)p->LT(1))->getText());
	printf("LT(2) is %s\n", ((ANTLRToken *)p->LT(2))->getText());
	printf("LT(3) is %s\n", ((ANTLRToken *)p->LT(3))->getText());
	printf("LT(4) is %s\n", ((ANTLRToken *)p->LT(4))->getText());
	printf("LT(5) is %s\n", ((ANTLRToken *)p->LT(5))->getText());
	printf("LT(6) is %s\n", ((ANTLRToken *)p->LT(6))->getText());
	printf("LT(7) is %s\n", ((ANTLRToken *)p->LT(7))->getText());
	printf("LT(8) is %s\n", ((ANTLRToken *)p->LT(8))->getText());
	printf("LT(9) is %s\n", ((ANTLRToken *)p->LT(9))->getText());
	printf("LT(10) is %s\n", ((ANTLRToken *)p->LT(10))->getText());
	printf("LT(11) is %s\n", ((ANTLRToken *)p->LT(11))->getText());
}
>>

#token "[\ \t\n]+"	<<skip();>>
#token Eof "@"

class Expr {				/* Define a grammar class */

begin
	:	<</*doit(this);*/>>
		e
	;

e	:	( list "=" )?  list "=" list Eof
		<<printf("list = list\n");>>
	|	list Eof
		<<printf("list\n");>>
	;

list:	"\(" (IDENTIFIER|NUMBER)* "\)"
	;

predict
	:	( "\(" (IDENTIFIER|NUMBER)* "\)" "=")?
	|	"\(" "\)" "="
	;

/*
Here's another example...
#token INT "int"
#token SEMI ";"
#token STAR "\*"
#token ASSIGN "="

begin:  "extern" "char" declarator ";"
	;

e	:	(decl)?
	|	expr
	;

expr:	IDENTIFIER "=" NUMBER
	;

decl:	"int" declarator ";"
	;

declarator
	:	( "\*" )? "\*" declarator
    |	IDENTIFIER
	;
*/

}

#token IDENTIFIER	"[a-z]+"
#token NUMBER		"[0-9]+"
