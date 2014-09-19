/* This is test.g which tests multiple scanners/parsers; DLG-based scanner */
<<
#include "Lexer.h"
typedef ANTLRCommonToken ANTLRToken;

int main()
{
	ANTLRTokenPtr aToken = new ANTLRToken;
	DLGFileInput in(stdin);
	Lexer scan(&in);
	scan.setToken(mytoken(aToken));
	ANTLRTokenBuffer pipe(&scan);
	Include parser(&pipe);
	parser.init();

	parser.input();
	return 0;
}
>>

#token "[\ \t\n]+"	<<skip();>>
#token Eof "@"

class Include {

<<
/* this is automatically defined to be a member function of Include::
 * since it is within the "class {...}" boundaries.
 */
private:
char *stripquotes(ANTLRChar *s)
{
	s[strlen(s)-1] = '\0';
	return &s[1];
}
>>

input
	:	( cmd | include )* Eof
	;

cmd	:	"print"
		(	NUMBER		<<printf("%s\n", $1->getText());>>
		|	STRING		<<printf("%s\n", $1->getText());>>
		)
	;

include
	:	"#include" STRING
		<<{
		FILE *f;
		f = fopen(stripquotes($2->getText()), "r");
		if ( f==NULL ) {fprintf(stderr, "can't open %s\n", $2->getText()+1);}
		else {
			ANTLRTokenPtr aToken = new ANTLRToken;
			DLGFileInput in(f);
			Lexer scan(&in);
			scan.setToken(mytoken(aToken));
			ANTLRTokenBuffer pipe(&scan);
			Include parser(&pipe);
			parser.init();
			parser.input();
		}
		}>>
	;

}

#token STRING	"\" [a-zA-Z0-9_.,\ \t]+ \""
#token NUMBER	"[0-9]+"
