/* This is test.g which tests a simple DLG-based scanner
 * with user-defined tokens
 */

/* Here, we use #tokdefs to define token types, but still let DLG do the
 * lexing. ANTLR will not create a tokens.h file.
 */
#tokdefs "mytokens.h"

<<
#include "DLGLexer.h"		/* include definition of DLGLexer.
							 * This cannot be generated automatically because
							 * ANTLR has no idea what you will call this file
							 * with the DLG command-line options.
							 */

typedef ANTLRCommonToken ANTLRToken;

int main()
{
	ANTLRTokenPtr aToken = new ANTLRToken;
	DLGFileInput in(stdin);
	DLGLexer scan(&in);
	ANTLRTokenBuffer pipe(&scan);
	scan.setToken(mytoken(aToken));
	Expr parser(&pipe);
	parser.init();

	parser.e();
	return 0;
}
>>

#token "[\ \t\n]+"	<<skip();>>

class Expr {				/* Define a grammar class */

e	:	IDENTIFIER NUMBER "@"
		<<fprintf(stderr, "text is %s,%s\n", $1->getText(), $2->getText());>>
	;

}

#token IDENTIFIER	"[a-z]+"
#token NUMBER		"[0-9]+"
