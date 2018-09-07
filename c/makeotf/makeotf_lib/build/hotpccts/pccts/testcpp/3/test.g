/* Ariel Tamches (tamches@cs.wisc.edu):
 * This tests linking in a simple non-DLG scanner with user-defined token
 * types.
 */

/* All TokenType's must have some end-of-file token;  You must define
 * it with setEofToken() to your end of input token.
 *
 * We assume that #tokdefs is  employed for this example; i.e., ANTLR does
 * NOT assign token numbers.
 *
 * ANTLR option -gx must be used to turn off generation of DLG crud (when you
 * want to define your own token stream).
 */

#tokdefs "mytokens.h"

/* user should define ANTLRToken outside of #header since AToken.h would
 * not have been included yet.  You can force inclusion of AToken.h if
 * you must use #header, however.
 */
<<
typedef ANTLRCommonToken ANTLRToken;	/* use a predefined Token class */
>>

/* At this point, ANTLRToken and ANTLRTokenStream are defined, user must now
 * derive a class from ANTLRTokenStream (which embodies the user's scanner)
 */
<<#include "MyLexer.h">>

<<
int main()
{
	/* create one of my scanners */
	MyLexer scan;
	ANTLRTokenBuffer pipe(&scan);
	/* create a parser of type Expr hooked to my scanner */
	Expr parser(&pipe);
	parser.init();
	parser.setEofToken(Eof);

	parser.e();				/* start parsing at rule 'e' of that parser */
	return 0;
}
>>

class Expr {

e	:	IDENTIFIER NUMBER
		<<fprintf(stderr, "text is %s,%s\n", $1->getText(), $2->getText());>>
		Eof
	;

}
