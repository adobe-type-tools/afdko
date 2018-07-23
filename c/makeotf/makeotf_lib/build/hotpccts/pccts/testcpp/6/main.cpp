/* we must define ANTLRTokenType, but it will be different for parsers A and B;
 * so, we just define it as an 'int', which is bad, but we can do nothing
 * else.
 */
#include <stdio.h>
#include <stdlib.h>
#include "A/tokens.h"
#include "A.h"
#include "B.h"
#include "ALexer.h"
#include "BLexer.h"

#include "PBlackBox.h"

typedef ANTLRCommonToken ANTLRToken;

int main(int argc, char *argv[])
{
	ANTLRToken aToken;		/* create a token to fill in for DLG */
	DLGFileInput in(stdin);

	if ( argc!=3 ) {
	    fprintf(stderr, "usage: t file1 file2\n");
//
//  7-Apr-97 MR1
//
//// MR1    exit(EXIT_SUCCESS);
	    exit(PCCTS_EXIT_SUCCESS);		//// MR1
	}

	ParserBlackBox<ALexer, A, ANTLRToken> p1(argv[1]);
	p1.parser()->e();
	
	ParserBlackBox<BLexer, B, ANTLRToken> p2(argv[2]);
	p2.parser()->e();

/*
	ALexer scan1(&in,2000);
	ANTLRTokenBuffer pipe1(&scan1);
	scan1.setToken(&aToken);
	A parser1(&pipe1);
	parser1.init();
	parser1.e();

	BLexer scan2(&in,2000);
	ANTLRTokenBuffer pipe2(&scan2);
	scan2.setToken(&aToken);
	B parser2(&pipe2);
	parser2.init();
	parser2.e();
*/

	return 0;
}
