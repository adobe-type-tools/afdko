#
# Makefile for simple 1.00B SORCERER examples
#

PCCTS=../..
BIN=$(PCCTS)/bin
PCCTS_H=$(PCCTS)/h

SOR = $(BIN)/sor

CFLAGS = -I. -I.. -I../h -I../lib -I$(PCCTS_H) -g
DFLAGS = -C2 -i
SRC = test.cpp ExprTreeParser.cpp ../lib/STreeParser.cpp \
	  $(PCCTS_H)/PCCTSAST.cpp
OBJ = test.o ExprTreeParser.o STreeParser.o PCCTSAST.o
SRC2 = test2.cpp Cool.cpp ../lib/STreeParser.cpp $(PCCTS_H)/PCCTSAST.cpp
OBJ2 = test2.o Cool.o STreeParser.o PCCTSAST.o
SRC3 = test3.cpp Cool.cpp ../lib/STreeParser.cpp $(PCCTS_H)/PCCTSAST.cpp
OBJ3 = test3.o Cool.o STreeParser.o PCCTSAST.o
SRC4 = test4.cpp
OBJ4 = test4.o
SRC5 = test5.cpp Cool.cpp ../lib/STreeParser.cpp $(PCCTS_H)/PCCTSAST.cpp
OBJ5 = test5.o Cool.o STreeParser.o PCCTSAST.o
CCC=CC
#CCC=g++

SPAWN = test.cpp test2.cpp ExprTreeParser.h ExprTreeParser.cpp Cool.h Cool.cpp \
	test3.cpp test5.cpp

test: $(OBJ) $(SRC)
	$(CCC) -o t $(CFLAGS) $(OBJ)

test.o : test.cpp
	$(CCC) -c $(CFLAGS) test.cpp

ExprTreeParser.o : ExprTreeParser.cpp
	$(CCC) -c $(CFLAGS) ExprTreeParser.cpp

STreeParser.o : ../lib/STreeParser.cpp
	$(CCC) -o STreeParser.o -c $(CFLAGS) ../lib/STreeParser.cpp

PCCTSAST.o : $(PCCTS_H)/PCCTSAST.cpp
	$(CCC) -o PCCTSAST.o -c $(CFLAGS) $(PCCTS_H)/PCCTSAST.cpp

test.cpp ExprTreeParser.cpp ExprTreeParser.h : test.sor
	$(SOR) -CPP -def-tokens test.sor

test2: $(OBJ2) $(SRC2)
	$(CCC) -o t2 $(CFLAGS) $(OBJ2)

test2.cpp : test2.sor
	$(SOR) -CPP -transform -def-tokens test2.sor

test2.o : test2.cpp
	$(CCC) -c $(CFLAGS) test2.cpp

test3: $(OBJ3) $(SRC3)
	$(CCC) -o t3 $(CFLAGS) $(OBJ3)

test3.cpp : test3.sor
	$(SOR) -CPP -transform test3.sor

test3.o : test3.cpp
	$(CCC) -c $(CFLAGS) test3.cpp

test5: $(OBJ5) $(SRC5)
	$(CCC) -o t5 $(CFLAGS) $(OBJ5)

test5.cpp : test5.sor
	$(SOR) -CPP -transform test5.sor

test5.o : test5.cpp
	$(CCC) -c $(CFLAGS) test5.cpp

Cool.o : Cool.cpp
	$(CCC) -c $(CFLAGS) Cool.cpp

scrub:
	(cd ./test4; make scrub)
	rm -rf *.o core $(SPAWN) t t2 t3 t5
