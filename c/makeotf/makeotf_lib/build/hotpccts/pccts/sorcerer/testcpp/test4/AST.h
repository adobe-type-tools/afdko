/* AST.h; this tree def satisfies both ANTLR and SORCERER constraints */
#include "ASTBase.h"
#include "AToken.h"

#define AtomSize	20

#include "ATokPtr.h"

class AST : public ASTBase {
protected:
	char text[AtomSize+1];
	int _type;
public:
	AST(ANTLRTokenPtr t){ _type = t->getType(); strcpy(text, t->getText()); }
	AST()				{ _type = 0; }
	int type()			{ return _type; }	// satisfy remaining SORCERER stuff
	char *getText()		{ return text; }
	void preorder_action() { printf(" %s", text); }
};

typedef AST SORAST;	// define the type of a SORCERER tree
