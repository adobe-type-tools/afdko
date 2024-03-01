/* Copyright 2021 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
 * This software is licensed as OpenSource, under the Apache License, Version 2.0.
 * This license is available at: http://opensource.org/licenses/Apache-2.0.
 */

lexer grammar FeatLexer;
import FeatLexerBase;

@members {
 std::string anon_tag;

 /* All the TSTART/TCHR characters are grouped together, so just
  * look for the string and if its there verify that the characters
  * on either side are from the appropriate set (in case there are
  * "extra" characters).
  */

 bool verify_anon(const std::string &line) {
     auto p = line.find(anon_tag);
     if ( p == std::string::npos )
         return false;
     --p;
     if ( ! ( line[p] == ' ' || line[p] == '\t' || line[p] == '}' ) )
         return false;
     p += anon_tag.size() + 1;
     if ( ! ( line[p] == ' ' || line[p] == '\t' || line[p] == ';' ) )
         return false;
     return true;
 }
}

ANON                        : 'anon' -> pushMode(Anon) ;
ANON_v                      : 'anonymous' -> pushMode(Anon) ;

mode Anon;

A_WHITESPACE                : [ \t\r\n]+ -> skip ;
A_LABEL                     : (NAMELABEL | EXTNAME | STRVAL | AXISUNIT | MARK) { anon_tag = getText(); } ;
A_LBRACE                    : '{' -> mode(AnonContent) ;

mode AnonContent;

A_CLOSE                     : '\r'? '\n}' [ \t]* (NAMELABEL | EXTNAME | STRVAL | AXISUNIT | MARK) [ \t]* ';' { verify_anon(getText()) }? -> popMode ;
A_LINE                      : '\r'? '\n' ~[\r\n]* ;
