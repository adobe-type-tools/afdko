/* C++ interface test of Parser Exception Handling
 *
 * Given input:
 *
 *		if a+ then a=b+b;
 *
 * the program should respond with
 *
 *		invalid conditional in 'if' statement
 *		found assignment to a
 */
<<
#include <stream.h>
#include "DLGLexer.h"
#include "PBlackBox.h"
typedef ANTLRCommonToken ANTLRToken;

int main()
{
	ParserBlackBox<DLGLexer, PEHTest, ANTLRToken> p(stdin);
	int retsignal;
	p.parser()->rule(&retsignal);
	return 0;
}
>>

/*
Uncommenting this will make ANTLR think you put these handlers at the
end of each rule:

exception
	catch MismatchedToken	: <<printf("dflt:MismatchedToken\n");>>
	default : <<printf("dflt:dflt\n");>>
*/

#token "[\ \t]+"	<<skip();>>
#token "\n"			<<skip(); newline();>>
#token THEN	"then"
#tokclass DIE { "@" "if" ID "else" }

class PEHTest {

rule:	( stat )+
	;

stat:	"if" t:expr THEN stat { "else" stat }
	|	id:ID "=" expr ";"
		<<printf("found assignment to %s\n", $id->getText());>>
	;
  	exception[t]
		default	:
			<<
			printf("invalid conditional in 'if' statement\n");
			consumeUntilToken(THEN);
            suppressSignal;
			>>
  	exception
		catch MismatchedToken	:
		catch NoViableAlt		:
		catch NoSemViableAlt	:
			<<
			printf("stat:caught predefined signal\n");
			consumeUntil(DIE_set);
            suppressSignal;
			>>

expr:	expr1 ("\+" expr1)*
	;

expr1
	:	expr2 ("\*" expr2)*
	;

expr2:	ID
	;

}

#token ID "[a-z]+"
