#
# PCCTS makefile for: lang.g
#
# Created from: /circle/s13/parrt/PCCTS/bin/genmk -project t7 -trees lang.g
#
# PCCTS release 1.30
# Project: t7
# C output
# DLG scanner
# ANTLR-defined token types
#
TOKENS = tokens.h
#
# The following filenames must be consistent with ANTLR/DLG flags
DLG_FILE = parser.dlg
ERR = err
HDR_FILE =
MOD_FILE = mode.h
SCAN = scan
PCCTS = ../../..
ANTLR_H = $(PCCTS)/h
BIN = $(PCCTS)/bin
ANTLR = $(BIN)/antlr
DLG = $(BIN)/dlg
SORCERER= ../..
SOR = $(SORCERER)/sor
SOR_H = $(SORCERER)/h
SOR_LIB = $(SORCERER)/lib
CFLAGS = -I. -I$(ANTLR_H) -g -I$(SOR_H) -I$(SOR_LIB)
AFLAGS = -gt -gh
DFLAGS = -C2 -i
GRM = lang.g
SRC = lang.c $(SCAN).c $(ERR).c main.c test7.c
OBJ = lang.o $(SCAN).o $(ERR).o main.o test7.o
ANTLR_SPAWN = lang.c $(DLG_FILE) $(TOKENS) $(ERR).c
DLG_SPAWN = $(SCAN).c $(MOD_FILE)
CC=cc
#CC=gcc

t7 : $(OBJ) $(SRC)
	$(CC) -o t7 $(CFLAGS) $(OBJ)

lang.o : $(MOD_FILE) $(TOKENS) lang.c
	$(CC) -c $(CFLAGS) -o lang.o lang.c

$(ERR).o : $(ERR).c $(TOKENS)
	$(CC) -c $(CFLAGS) -o $(ERR).o $(ERR).c

$(SCAN).o : $(SCAN).c $(TOKENS)
	$(CC) -c $(CFLAGS) -o $(SCAN).o $(SCAN).c

$(ANTLR_SPAWN) : $(GRM)
	$(ANTLR) $(AFLAGS) $(GRM)

$(DLG_SPAWN) : $(DLG_FILE)
	$(DLG) $(DFLAGS) $(DLG_FILE) $(SCAN).c

test7.c : test7.sor
	$(SOR) test7.sor

clean:
	rm -f *.o core t7

scrub:
	rm -f *.o core t7 $(ANTLR_SPAWN) $(DLG_SPAWN)
