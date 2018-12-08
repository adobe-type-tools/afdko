#ifndef SimpleTreeParser_h
#define SimpleTreeParser_h
/*
 * S O R C E R E R  T r a n s l a t i o n  H e a d e r
 *
 * SORCERER Developed by Terence Parr, Aaron Sawdey, & Gary Funck
 * Parr Research Corporation, Intrepid Technology, University of Minnesota
 * 1992-1994
 * SORCERER Version 1.00B15
 */
#include "STreeParser.h"

#include "tokens.h"
#include "AST.h"

class SimpleTreeParser : public STreeParser {
protected:
public:
	SimpleTreeParser();

	void gen_stat(SORASTBase **_root);
	void gen_expr(SORASTBase **_root);
};

#endif /* SimpleTreeParser_h */
