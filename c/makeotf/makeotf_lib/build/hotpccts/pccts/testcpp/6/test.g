/* This is test.g which tests linking multiple scanners/parsers together;
 * DLG-based scanner
 */

<<
typedef ANTLRCommonToken ANTLRToken;
>>

#token "[\ \t\n]+"	<<skip();>>

class A {

e	:	IDENTIFIER NUMBER
		<<fprintf(stderr, "text is %s,%s\n", $1->getText(), $2->getText());>>
	;

}

#token IDENTIFIER	"[a-z]+"
#token NUMBER		"[0-9]+"
