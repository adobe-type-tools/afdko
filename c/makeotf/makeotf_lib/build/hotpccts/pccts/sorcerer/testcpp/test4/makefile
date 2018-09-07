#
# PCCTS makefile for: lang.g
#
# Created from: /circle/s13/parrt/PCCTS/bin/genmk -CC -class SimpleParser -project t4 -trees lang.g
#
# PCCTS release 1.30
# Project: t4
# C++ output
# DLG scanner
# ANTLR-defined token types
#
TOKENS = tokens.h
#
# The following filenames must be consistent with ANTLR/DLG flags
DLG_FILE = parser.dlg
ERR = err
HDR_FILE =
SCAN = DLGLexer
PCCTS = ../../..
ANTLR_H = $(PCCTS)/h
SOR_H = ../../h
SOR_LIB = ../../lib
BIN = $(PCCTS)/bin
ANTLR = $(BIN)/antlr
DLG = $(BIN)/dlg
SOR = $(BIN)/sor
CFLAGS = -I. -I$(ANTLR_H) -I$(SOR_H) -I$(SOR_LIB) -g
AFLAGS = -CC -gt
DFLAGS = -C2 -i -CC
GRM = lang.g
SRC = lang.cpp main.cpp test4.cpp $(SOR_LIB)/STreeParser.cpp \
      SimpleParser.cpp \
      SimpleTreeParser.cpp \
      $(ANTLR_H)/AParser.cpp $(ANTLR_H)/DLexerBase.cpp \
      $(ANTLR_H)/ASTBase.cpp $(ANTLR_H)/PCCTSAST.cpp \
      $(ANTLR_H)/ATokenBuffer.cpp $(SCAN).cpp
OBJ = lang.o main.o test4.o STreeParser.o \
      SimpleParser.o \
      SimpleTreeParser.o \
       AParser.o DLexerBase.o \
      ASTBase.o PCCTSAST.o \
       ATokenBuffer.o $(SCAN).o
ANTLR_SPAWN = lang.cpp SimpleParser.cpp \
              SimpleParser.h $(DLG_FILE) $(TOKENS)
DLG_SPAWN = $(SCAN).cpp $(SCAN).h

CCC=CC
#CCC=g++

t4 : $(OBJ) $(SRC)
	$(CCC) -o t4 $(CFLAGS) $(OBJ)

main.o : main.cpp SimpleTreeParser.h SimpleParser.h
	$(CCC) -c $(CFLAGS) main.cpp

lang.o : $(TOKENS) $(SCAN).h lang.cpp
	$(CCC) -c $(CFLAGS) -o lang.o lang.cpp

SimpleParser.o : $(TOKENS) $(SCAN).h SimpleParser.cpp SimpleParser.h
	$(CCC) -c $(CFLAGS) -o SimpleParser.o SimpleParser.cpp

SimpleTreeParser.o : $(TOKENS) $(SCAN).h SimpleTreeParser.cpp tokens.h
	$(CCC) -c $(CFLAGS) SimpleTreeParser.cpp

test4.cpp SimpleTreeParser.h SimpleTreeParser.cpp : test4.sor
	$(SOR) -CPP test4.sor

test4.o : test4.cpp
	$(CCC) -c $(CFLAGS) test4.cpp

STreeParser.o : $(SOR_LIB)/STreeParser.cpp
	$(CCC) -o STreeParser.o -c $(CFLAGS) $(SOR_LIB)/STreeParser.cpp

$(SCAN).o : $(SCAN).cpp $(TOKENS)
	$(CCC) -c $(CFLAGS) -o $(SCAN).o $(SCAN).cpp

$(ANTLR_SPAWN) : $(GRM)
	$(ANTLR) $(AFLAGS) $(GRM)

$(DLG_SPAWN) : $(DLG_FILE)
	$(DLG) $(DFLAGS) $(DLG_FILE)

AParser.o : $(ANTLR_H)/AParser.cpp
	$(CCC) -c $(CFLAGS) -o AParser.o $(ANTLR_H)/AParser.cpp

ATokenBuffer.o : $(ANTLR_H)/ATokenBuffer.cpp
	$(CCC) -c $(CFLAGS) -o ATokenBuffer.o $(ANTLR_H)/ATokenBuffer.cpp

DLexerBase.o : $(ANTLR_H)/DLexerBase.cpp
	$(CCC) -c $(CFLAGS) -o DLexerBase.o $(ANTLR_H)/DLexerBase.cpp

ASTBase.o : $(ANTLR_H)/ASTBase.cpp
	$(CCC) -c $(CFLAGS) -o ASTBase.o $(ANTLR_H)/ASTBase.cpp

PCCTSAST.o : $(ANTLR_H)/PCCTSAST.cpp
	$(CCC) -c $(CFLAGS) -o PCCTSAST.o $(ANTLR_H)/PCCTSAST.cpp

clean:
	rm -f *.o core t4

scrub:
	rm -f *.o core t4 $(ANTLR_SPAWN) $(DLG_SPAWN) test4.cpp
