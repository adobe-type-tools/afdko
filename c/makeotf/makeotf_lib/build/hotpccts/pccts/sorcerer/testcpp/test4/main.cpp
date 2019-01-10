#include "tokens.h"
#include "SimpleParser.h"
typedef ANTLRCommonToken ANTLRToken;
#include "SimpleTreeParser.h"
#include "DLGLexer.h"
#include "PBlackBox.h"

main()
{
	ParserBlackBox<DLGLexer, SimpleParser, ANTLRToken> lang(stdin);
	AST *root=NULL, *result;
	SimpleTreeParser tparser;

	lang.parser()->stat((ASTBase **)&root);	/* get the tree to walk with SORCERER */
	printf("input tree:"); root->preorder(); printf("\n");
	tparser.gen_stat((SORASTBase **)&root);
/*	in transform mode, use:
	tparser.gen_stat((SORASTBase **)&root, (SORASTBase **)&result);
 */
}
