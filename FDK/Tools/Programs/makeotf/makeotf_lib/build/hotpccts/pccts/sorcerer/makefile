#
# PCCTS makefile for: SORCERER 1.00B17
# Uses PCCTS release 1.33MR23
#
PCCTS=..
BIN = $(PCCTS)/bin
ANTLR = $(BIN)/antlr
DLG = $(BIN)/dlg
PCCTS_H = $(PCCTS)/h
SET = $(PCCTS)/support/set

CC=cc
COPT=-g
CFLAGS = $(COPT) -Ih -I. -I$(PCCTS_H) -I$(SET) -DZZLEXBUFSIZE=32000
AFLAGS = -gh -k 2 -gt
DFLAGS = -C2
GRM = sor.g
SRC = sor.c scan.c err.c main.c globals.c gen.c $(SET)/set.c hash.c look.c cpp.c
OBJ = sor.o scan.o err.o main.o globals.o gen.o set.o hash.o look.o cpp.o
GENHFILES = tokens.h mode.h stdpccts.h
SHELL=/bin/sh

all:
	$(MAKE) sor
	(cd ./lib; $(MAKE) CC="$(CC)" COPT="$(COPT)")

sor: $(OBJ) $(SRC)
	$(CC) -o sor $(CFLAGS) $(OBJ)
	mv sor $(BIN)/.

sor.o : sor.c mode.h tokens.h

sor.c stdpccts.h parser.dlg tokens.h err.c : sor.g
	$(ANTLR) $(AFLAGS) sor.g

set.o : $(SET)/set.c
	$(CC) $(CFLAGS) -c -o set.o $(SET)/set.c

scan.c mode.h : parser.dlg
	$(DLG) $(DFLAGS) parser.dlg scan.c

scan.o : scan.c mode.h tokens.h

clean:
	rm -f *.o core sor

scrub:
	rm -f *.o core $(GENHFILES) parser.dlg scan.c err.c sor.c sor

ci:
	ci -u sor.g Makefile main.c globals.c gen.c set.c hash.c look.c cpp.c

co:
	co -l sor.g Makefile main.c globals.c gen.c set.c hash.c look.c cpp.c
