#
# PCCTS makefile for: test.g
#
# PCCTS release 1.21
# Project: t
# C++ output
# D scanner
# A-defined token types
#
TOKENS = A/tokens.h
#
# The following filenames must be consistent with A/D flags
D_FILE = A/parser.dlg
ERR = A/err
HDR_FILE =
SCAN = A/ALexer
PCCTS = ../..
A_H = $(PCCTS)/h
BIN = $(PCCTS)/bin
A = $(BIN)/antlr
D = $(BIN)/dlg
CFLAGS = -I. -I$(A_H) -IA -IB -g
#
# Delay lookahead so that an extra fetch is not done by
# 'A' parser which is needed by 'B' parser.
# 
AFLAGS = -o A -CC
DFLAGS = -C2 -i -CC -cl ALexer -o A
GRM = test.g
SRC = A/test.cpp \
      A/A.cpp \
      $(A_H)/AParser.cpp $(A_H)/DLexerBase.cpp $(SCAN).cpp main.cpp \
	  $(A_H)/ATokenBuffer.cpp
OBJ = A/test.o A/A.o A/AParser.o A/DLexerBase.o $(SCAN).o main.o \
	  A/ATokenBuffer.o

OBJ2 = B/test2.o B/B.o B/BLexer.o
A_SPAWN = A/test.cpp A/A.cpp \
              A/A.h \
              $(HDR_FILE) $(D_FILE) $(TOKENS)
D_SPAWN = $(SCAN).cpp $(SCAN).h
#CCC=g++
CCC=CC

t: $(OBJ) $(OBJ2) $(SRC)
	make -f makefile2
	$(CCC) -o t $(CFLAGS) $(OBJ) $(OBJ2)

A/test.o : $(TOKENS) $(SCAN).h A/test.cpp $(HDR_FILE)
	$(CCC) -c $(CFLAGS) -o A/test.o A/test.cpp

A/A.o : $(TOKENS) $(SCAN).h A/A.cpp A/A.h $(HDR_FILE)
	$(CCC) -c $(CFLAGS) -o A/A.o A/A.cpp

$(SCAN).o : $(SCAN).cpp $(TOKENS)
	$(CCC) -c $(CFLAGS) -o $(SCAN).o $(SCAN).cpp

$(A_SPAWN) : $(GRM)
	$(A) $(AFLAGS) $(GRM)

$(D_SPAWN) : $(D_FILE)
	$(D) $(DFLAGS) $(D_FILE)

A/ATokenBuffer.o : $(A_H)/ATokenBuffer.cpp
	$(CCC) -c $(CFLAGS) -o A/ATokenBuffer.o $(A_H)/ATokenBuffer.cpp

A/AParser.o : $(A_H)/AParser.cpp
	$(CCC) -c $(CFLAGS) -o A/AParser.o $(A_H)/AParser.cpp

A/DLexerBase.o : $(A_H)/DLexerBase.cpp
	$(CCC) -c $(CFLAGS) -o A/DLexerBase.o $(A_H)/DLexerBase.cpp

main.o : main.cpp B/B.h
	$(CCC) -c $(CFLAGS) -o main.o main.cpp

B/B.h :
	make -f makefile2

clean:
	rm -f *.o core t A/*.o

scrub:
	rm -f *.o core t A/*.o $(A_SPAWN) $(D_SPAWN)
