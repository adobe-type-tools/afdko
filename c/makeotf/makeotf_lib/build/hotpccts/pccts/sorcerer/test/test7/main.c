#include "stdpccts.h"
typedef AST SORAST;
#include "sorcerer.h"

main()
{
	AST *root=NULL;
	STreeParser tparser;
	STreeParserInit(&tparser);

	ANTLR(stat(&root), stdin);	/* get the tree to walk with SORCERER */
	printf("input tree:"); lisp(root); printf("\n");
	gen_stat(&tparser, &root);
}

#ifdef __STDC__
lisp(SORAST *tree)
#else
lisp(tree)
SORAST *tree;
#endif
{
	while ( tree!= NULL )
	{
		if ( tree->down != NULL ) printf(" (");
		printf(" %s", tree->text);
		lisp(tree->down);
		if ( tree->down != NULL ) printf(" )");;
		tree = tree->right;
	}
}

