#
# Makefile for SORCERER libraries
#
SRC = astlib.c sstack.c sorlist.c sintstack.c
OBJ = astlib.o sstack.o sorlist.o sintstack.o
CC=cc
COPT=-g
CFLAGS=$(COPT) -I../../h -I../h

libs : $(OBJ) $(SRC)

clean:
	rm -f *.o core

scrub:
	rm -f *.o core

ci:
	ci -u $(SRC) *.h makefile

co:
	co -l $(SRC) *.h makefile
