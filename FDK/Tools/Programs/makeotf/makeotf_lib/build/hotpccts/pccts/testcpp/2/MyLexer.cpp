/* MyTokenBuffer.c */
/* Sample replacement for DLGLexer */
/* Shows how to override DLG with your own lexer */

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <iostream.h>

#include "config.h"					/* include token defs */
#include "tokens.h"					/* include token defs */
#include APARSER_H					/* include all the ANTLR yuck */
#include "MyLexer.h"				/* define your lexer */
typedef ANTLRCommonToken ANTLRToken;/* use a predefined Token class */

void panic(char *s) {
   cerr << s << '\n';
   exit(5);
}

MyLexer::MyLexer()
{
	c = getchar();
}

/* Recognizes Tokens IDENTIFIER and NUMBER */
ANTLRAbstractToken *MyLexer::
getToken()
{
	/* we will return a pointer to this next guy */
	ANTLRToken *resultToken = new ANTLRToken;

	ANTLRChar TokenBuffer[100];
						/* This routine will just crash if tokens become
                           more than 99 chars; your code must of course
						   gracefully recover/adapt */
   int  index=0;

   while ( c==' ' || c=='\n' ) c=getchar();

   if (c==EOF) {resultToken->setType(Eof); return resultToken;}

   if (isdigit(c)) {
      /* Looks like we have ourselves a number token */
      while (isdigit(c)) {
         TokenBuffer[index++]=c;
		 c = getchar();
	  }
      TokenBuffer[index]='\0';

      resultToken->setType(NUMBER);
      resultToken->setText(TokenBuffer);

      return resultToken;
   }

   if (isalpha(c)) {
      /* We have ourselves an IDENTIFIER token */
      while (isalpha(c)) {
         TokenBuffer[index++]=c;
		 c = getchar();
	  }
      TokenBuffer[index]='\0';

      resultToken->setType(IDENTIFIER);
      resultToken->setText(TokenBuffer);

      return resultToken;
   }

   else
      panic("lexer error");

   return NULL;
}

