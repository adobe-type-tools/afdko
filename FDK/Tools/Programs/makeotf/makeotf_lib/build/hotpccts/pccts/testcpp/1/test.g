/* This is test.g which tests a simple DLG-based scanner.
 * No garbage collection for tokens since our token can't
 * handle ref counting.
 */

/* ANTLR will assign token type numbers (by creating an enum which you
 * get in tokens.h)
 */

<<
/* user must define ANTLRToken */

typedef char ANTLRChar;

class ANTLRToken : public ANTLRAbstractToken {
protected:
    ANTLRTokenType _type;	// what's the token type of the token object
    int _line;			// track line info for errors

	/* For our simple purposes, a token and a string is enough for
	 * our attribute
	 */
	ANTLRChar _text[30];

public:
	ANTLRToken(ANTLRTokenType t, ANTLRChar *s)
        { setType(t); _line = 0; setText(s); }

	/* Your derived class MUST have a blank constructor. */
    ANTLRToken()
		{ setType((ANTLRTokenType)0); _line = 0; setText(""); }

	// how to access the token type and line number stuff
    ANTLRTokenType getType() const   { return _type; }
    void setType(ANTLRTokenType t)   { _type = t; }
    virtual int getLine() const      { return _line; }
    void setLine(int line)           { _line = line; }

    //
    //  warning - casting away const in ANTLRToken::getText() const
    //

	ANTLRChar *getText() const       { return (ANTLRChar *) _text; }
	void setText(const ANTLRChar *s) { strncpy(_text, s, 30); }

	/* WARNING WARNING WARNING: you must return a stream of distinct tokens */
	/* This function will disappear when I can use templates */

	virtual ANTLRAbstractToken *makeToken(ANTLRTokenType tt,
										  ANTLRChar *txt,
										  int line)
		{
			ANTLRAbstractToken *t = new ANTLRToken(tt,txt);
			t->setLine(line);
			return t;
		}
};

/* "DLGLexer" must match what you use on DLG command line (-cl);
 * "DLGLexer" is the default.
 */
#include "DLGLexer.h"		/* include definition of DLGLexer.
							 * This cannot be generated automatically because
							 * ANTLR has no idea what you will call this file
							 * with the DLG command-line options.
							 */

int main()
{
	DLGFileInput in(stdin);	/* create input stream for DLG to get chars from */
	DLGLexer scan(&in);		/* create scanner reading from stdin */
	ANTLRTokenBuffer pipe(&scan);/* make buffered pipe between lexer&parser */
	ANTLRTokenPtr aToken=new ANTLRToken; // create a token to fill in for DLG
	scan.setToken(mytoken(aToken));
	Expr parser(&pipe);		/* create parser of type Expr hooked to scanner */
	parser.init();			/* init the parser; prime lookahead etc... */

	parser.e();				/* start parsing at rule 'e' of that parser */
	return 0;
}
>>

#token "[\ \t\n]+"	<<skip();>>
#token Eof "@"

#tokclass My { IDENTIFIER NUMBER }

class Expr {				/* Define a grammar class */

e	:	My My Eof
		<<fprintf(stderr, "text is %s,%s\n", $1->getText(), $2->getText());>>
	;

}

#token IDENTIFIER	"[a-z]+"
#token NUMBER		"[0-9]+"
