/* Simple main() to call a parser in another file */

#include "tokens.h"			// define TokenType
#include "Expr.h"			// define parser
#include "DLGLexer.h"		// define scanner
#include "PBlackBox.h"		// Define ParserBlackBox

int main()
{
	ParserBlackBox<DLGLexer, Expr, ANTLRToken> p(stdin);

	p.parser()->e();			/* start parsing at rule 'e' of that parser */
	return 0;
}
