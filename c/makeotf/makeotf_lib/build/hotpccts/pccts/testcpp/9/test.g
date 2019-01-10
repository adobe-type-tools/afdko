/* This is test.g which tests simple AST refs and construction */

/* Revised 2002-2-15: preorder_action prototype no longer
   matched prototype of base class.  (Eero Ivask, 
   Tallinn Technical University, Estonia).
*/   

<<
typedef ANTLRCommonToken ANTLRToken;
#include "DLGLexer.h"
#include "PBlackBox.h"

class AST : public ASTBase {
public:
	ANTLRTokenPtr token;
	AST(ANTLRTokenPtr t) { token = t; }
	void preorder_action(void * clientData) {
		char *s = token->getText();
		printf(" %s", s);
	}
};

int main()
{
	ParserBlackBox<DLGLexer, Expr, ANTLRToken> p(stdin);
	ASTBase *root = NULL;
	p.parser()->e(&root);
	root->preorder();
	printf("\n");
	root->destroy();
	return 0;
}
>>

#token "[\ \t\n]+"	<<skip();>>
#token Eof "@"

class Expr {				/* Define a grammar class */

e	:	mult_expr ( ("\+"^|"\-"^) mult_expr )*
	;

mult_expr
	:	atom ( ("\*"^|"\/"^) atom )*
	;

atom:	IDENTIFIER
	|	NUMBER
	;

}

#token IDENTIFIER	"[a-z]+"
#token NUMBER		"[0-9]+"
