/* This is test.g which tests a simple DLG-based scanner, but with
 * a main() in another file.
 */

#header <<
#include "AToken.h"						// what's ANTLRCommonToken look like?
typedef ANTLRCommonToken ANTLRToken;	// by placing in header, Expr.h gets it
>>

#token "[\ \t\n]+"	<<skip();>>
#token Eof "@"

class Expr {				/* Define a grammar class */

e	:	IDENTIFIER NUMBER Eof
		<<fprintf(stderr, "text is %s,%s\n", $1->getText(), $2->getText());>>
	;

}

#token IDENTIFIER	"[a-z]+"
#token NUMBER		"[0-9]+"
