#
# Makefile for simple 1.00B SORCERER examples
#

PCCTS=../..
BIN=$(PCCTS)/bin
SOR = $(BIN)/sor
CFLAGS = -I. -I../../h -I../h -I../lib -g -ansi
DFLAGS = -C2 -i
SRC = test.c
OBJ = test.o
SRC2 = test2.c
OBJ2 = test2.o ../lib/astlib.o ../lib/sorlist.o
SRC3 = test3.c
OBJ3 = test3.o ../lib/astlib.o ../lib/sorlist.o
SRC4 = test4.c
OBJ4 = test4.o
SRC5 = test5.c
OBJ5 = test5.o ../lib/astlib.o ../lib/sorlist.o
SRC6 = test6.c
OBJ6 = test6.o ../lib/astlib.o ../lib/sorlist.o
#CC=gcc
CC=cc
TEST_SPAWN = test.c test2.c test3.c test4.c test5.c

test1: $(OBJ) $(SRC)
	$(CC) -o t $(CFLAGS) $(OBJ)

test.c : test.sor
	$(SOR) -funcs both test.sor

test2: $(OBJ2) $(SRC2) 
	$(CC) -o t2 $(CFLAGS) $(OBJ2)

test2.c : test2.sor
	$(SOR) -funcs KR test2.sor

test3: $(OBJ3) $(SRC3)
	$(CC) -o t3 $(CFLAGS) $(OBJ3)

test3.c : test3.sor
	$(SOR) -transform -funcs KR test3.sor

test4: $(OBJ4) $(SRC4)
	$(CC) -o t4 $(CFLAGS) $(OBJ4)

test4.c : test4.sor
	$(SOR) -funcs KR test4.sor

test5: $(OBJ5) $(SRC5)
	$(CC) -o t5 $(CFLAGS) $(OBJ5)

test5.c : test5.sor
	$(SOR) -transform -funcs KR test5.sor

test6: $(OBJ6) $(SRC6)
	$(CC) -o t6 $(CFLAGS) $(OBJ6)

test6.c : test6.sor
	$(SOR) -def-tokens-file tokens6.h -transform -funcs KR test6.sor

scrub:
	rm -rf *.o core $(TEST_SPAWN)

clean:
	rm -rf *.o core
