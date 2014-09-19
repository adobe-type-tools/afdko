<<
#include ATOKPTR_H	// define smart pointers

class ANTLRToken : public ANTLRCommonToken {
public:
	int muck;
public:
    ANTLRToken(ANTLRTokenType t, ANTLRChar *s) : ANTLRCommonToken(t,s)
		{ muck = atoi(s); }
    ANTLRToken() {;}
    ANTLRChar *getText() const { return ""; }
    void setText(const ANTLRChar *s) { ; }
    virtual ANTLRAbstractToken *makeToken(ANTLRTokenType t, char *s, int line)
		{
			ANTLRToken *tk = new ANTLRToken(t,s);
			tk->muck = atoi(s);
			return tk;
		}
};

#include "DLGLexer.h"
#include "PBlackBox.h"

int main()
{
	ParserBlackBox<DLGLexer, Expr, ANTLRToken> p(stdin);

	p.parser()->calc();
	return 0;
}
>>

#token "[\ \t\n]+"	<<skip();>>

class Expr {

calc:	<<int r;>>
		e>[r]
		<<printf("result is %d\n", r);>>
	;

e > [int r]
	:	<<int b;>>
		e2>[$r] ( "\+" e2>[b] <<$r+=b;>> )*
	;

e2 > [int r]
	:	NUMBER <<$r=mytoken($1)->muck;>>
		( "\*" NUMBER <<$r*=mytoken($2)->muck;>> )*
	;

}

#token NUMBER		"[0-9]+"

