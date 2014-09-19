/* This is test.g which tests a simple DLG-based scanner using string input */

<<
typedef ANTLRCommonToken ANTLRToken;

#include "DLGLexer.h"
#include "PBlackBox.h"

int main(int argc, char **argv)
{
	if ( argc==1 ) {fprintf(stderr, "how about an argument?\n"); exit(1);}
	DLGStringInput in(argv[1]);		/* create an input stream for DLG */
	DLGLexer scan(&in);				/* create scanner reading from stdin */
	ANTLRTokenBuffer pipe(&scan);	/* make pipe between lexer & parser */
	ANTLRTokenPtr aToken = new ANTLRToken;
	scan.setToken(mytoken(aToken));
	Expr parser(&pipe);		/* create parser of type Expr hooked to scanner */
	parser.init();			/* init the parser; prime lookahead etc... */
	parser.e();				/* start parsing at rule 'e' of that parser */

	return 0;
}
>>

#token "[\ \t\n]+"	<<skip();>>
#token Eof "@"

#tokclass My { IDENTIFIER NUMBER }

class Expr {				/* Define a grammar class */

e	:	My My Eof
		<<fprintf(stderr, "text is %s,%s\n", $1->getText(), $2->getText());>>
	;

}

#token IDENTIFIER	"[a-z]+"
#token NUMBER		"[0-9]+"
