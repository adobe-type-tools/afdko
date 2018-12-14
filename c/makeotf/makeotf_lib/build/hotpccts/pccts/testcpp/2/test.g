/* Ariel Tamches (tamches@cs.wisc.edu):
 * This tests linking in a simple non-DLG scanner
 */

/* We assume that #tokdefs is not employed for this example; i.e.,
 * ANTLR assigns token numbers.
 *
 * ANTLR option -gx must be used to turn off generation of DLG crud (when you
 * want to define your own token stream).
 */

/* user must define ANTLRToken outside of #header */
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

class Expr {				/* Define a grammar class */

e	:	IDENTIFIER NUMBER
		<<fprintf(stderr, "text is %s,%s\n", $1->getText(), $2->getText());>>
		Eof
	;

}
