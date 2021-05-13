#
# PCCTS makefile for: test.g
#
# Created from: ../../bin/genmk -CC -project t -class Include test.g -dlg-class Lexer
#
# PCCTS release 1.32
# Project: t
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
SCAN = Lexer
PCCTS = ../..
ANTLR_H = $(PCCTS)/h
BIN = $(PCCTS)/bin
ANTLR = $(BIN)/antlr
DLG = $(BIN)/dlg
CFLAGS = -I. -I$(ANTLR_H)
AFLAGS = -CC
DFLAGS = -C2 -i -CC -cl Lexer
GRM = test.g
SRC = test.cpp \
      Include.cpp \
      $(ANTLR_H)/AParser.cpp $(ANTLR_H)/DLexerBase.cpp $(ANTLR_H)/ATokenBuffer.cpp $(SCAN).cpp
OBJ = test.o \
      Include.o \
       AParser.o DLexerBase.o ATokenBuffer.o $(SCAN).o
ANTLR_SPAWN = test.cpp Include.cpp \
              Include.h $(DLG_FILE) $(TOKENS)
DLG_SPAWN = $(SCAN).cpp $(SCAN).h
#CCC=g++
CCC=CC

t : $(OBJ) $(SRC)
	$(CCC) -o t $(CFLAGS) $(OBJ)

test.o : $(TOKENS) $(SCAN).h test.cpp
	$(CCC) -c $(CFLAGS) -o test.o test.cpp

Include.o : $(TOKENS) $(SCAN).h Include.cpp Include.h
	$(CCC) -c $(CFLAGS) -o Include.o Include.cpp

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

clean:
	rm -f *.o core t

scrub:
	rm -f *.o core t $(ANTLR_SPAWN) $(DLG_SPAWN)
