#
# PCCTS makefile for: test.g
#
# Created from: ../../bin/genmk -CC -project t -class Expr test.g -user-lexer
#
# PCCTS release 1.32
# Project: t
# C++ output
# User-defined scanner
# ANTLR-defined token types
#
TOKENS = tokens.h
#
# The following filenames must be consistent with ANTLR/DLG flags
DLG_FILE = parser.dlg
ERR = err
HDR_FILE =
SCAN = DLGLexer
PCCTS = ../..
ANTLR_H = $(PCCTS)/h
BIN = $(PCCTS)/bin
ANTLR = $(BIN)/antlr
DLG = $(BIN)/dlg
CFLAGS = -I. -I$(ANTLR_H)
AFLAGS = -gx -CC
DFLAGS = -C2 -i -CC
GRM = test.g
SRC = test.cpp \
      Expr.cpp \
      $(ANTLR_H)/AParser.cpp $(ANTLR_H)/ATokenBuffer.cpp MyLexer.cpp
OBJ = test.o \
      Expr.o \
       AParser.o ATokenBuffer.o MyLexer.o
ANTLR_SPAWN = test.cpp Expr.cpp \
              Expr.h $(TOKENS)
#CCC=g++
CCC=CC

t : $(OBJ) $(SRC)
	$(CCC) -o t $(CFLAGS) $(OBJ)

test.o : $(TOKENS) test.cpp
	$(CCC) -c $(CFLAGS) -o test.o test.cpp

MyLexer.o : $(TOKENS) MyLexer.cpp
	$(CCC) -c $(CFLAGS) -o MyLexer.o MyLexer.cpp

Expr.o : $(TOKENS) Expr.cpp Expr.h
	$(CCC) -c $(CFLAGS) -o Expr.o Expr.cpp

$(ANTLR_SPAWN) : $(GRM)
	$(ANTLR) $(AFLAGS) $(GRM)

AParser.o : $(ANTLR_H)/AParser.cpp
	$(CCC) -c $(CFLAGS) -o AParser.o $(ANTLR_H)/AParser.cpp

ATokenBuffer.o : $(ANTLR_H)/ATokenBuffer.cpp
	$(CCC) -c $(CFLAGS) -o ATokenBuffer.o $(ANTLR_H)/ATokenBuffer.cpp

clean:
	rm -f *.o core t

scrub:
	rm -f *.o core t $(ANTLR_SPAWN)
