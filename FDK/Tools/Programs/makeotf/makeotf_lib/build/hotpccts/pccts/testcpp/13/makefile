#
# PCCTS makefile for: test.g
#
# Created from: ../../bin/genmk -CC -class PEHTest -project t test.g
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
SCAN = DLGLexer
PCCTS = ../..
ANTLR_H = $(PCCTS)/h
BIN = $(PCCTS)/bin
ANTLR = $(BIN)/antlr
DLG = $(BIN)/dlg
CFLAGS = -I. -I$(ANTLR_H)
AFLAGS = -CC $(AOTHER)
DFLAGS = -C2 -i -CC
GRM = test.g
SRC = test.cpp \
      PEHTest.cpp \
      $(ANTLR_H)/AParser.cpp $(ANTLR_H)/DLexerBase.cpp $(ANTLR_H)/ATokenBuffer.cpp $(SCAN).cpp
OBJ = test.o \
      PEHTest.o \
       AParser.o DLexerBase.o ATokenBuffer.o $(SCAN).o
ANTLR_SPAWN = test.cpp PEHTest.cpp \
              PEHTest.h $(DLG_FILE) $(TOKENS)
DLG_SPAWN = $(SCAN).cpp $(SCAN).h
#CCC=g++
CCC=CC

t : $(OBJ) $(SRC)
	$(CCC) -o t $(CFLAGS) $(OBJ)

test.o : $(TOKENS) $(SCAN).h test.cpp
	$(CCC) -c $(CFLAGS) -o test.o test.cpp

PEHTest.o : $(TOKENS) $(SCAN).h PEHTest.cpp PEHTest.h
	$(CCC) -c $(CFLAGS) -o PEHTest.o PEHTest.cpp

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
