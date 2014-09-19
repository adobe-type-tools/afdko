/* This is test2.g which is a copy of test.g and tests linking
 * multiple scanners/parsers together; DLG-based scanner
 */

<<
typedef ANTLRCommonToken ANTLRToken;
>>

#token "[\ \t\n]+"	<<skip();>>
#token Eof "@"

class B {

e	:	IDENTIFIER NUMBER
		<<fprintf(stderr, "text is %s,%s\n", $1->getText(), $2->getText());>>
		Eof
	;

}

#token IDENTIFIER	"[a-z]+"
#token NUMBER	"[0-9]+"
