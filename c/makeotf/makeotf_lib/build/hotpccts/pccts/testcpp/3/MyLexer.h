/* MyTokenBuffer.h */

#include ATOKENBUFFER_H

class MyLexer : public ANTLRTokenStream {
private:
	int c;
public:
	MyLexer();
	virtual ANTLRAbstractToken *getToken();
};
