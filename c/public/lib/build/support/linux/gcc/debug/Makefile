#########################################################################
#                                                                       #
# Copyright 2014 Adobe Systems Incorporated.                       #
# All rights reserved.                                                  #
#                                                                       #
#########################################################################

# Configuration
CONFIG = debug
CFLAGS = $(STD_OPTS) -g -DSTAGE=DEVELOP 
# Library
ROOT_DIR = ../../../../..
SRC_DIR = $(ROOT_DIR)/source/support

LIB_SRCS = $(SRC_DIR)/bitarray.c \
	$(SRC_DIR)/canthappen.c \
	$(SRC_DIR)/except.c \
	$(SRC_DIR)/fixed.c
LIB_OBJS = bitarray.o canthappen.o except.o fixed.o

LIB_TARGET = $(LIB_DIR)/support.a

# Build targets
TARGETS = $(LIB_TARGET)

# Standard definitions
include ../../../../../config/linux/gcc/gcc_tx.mak

# Object rules
bitarray.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/bitarray.c -o $@

canthappen.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/canthappen.c -o $@

except.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/except.c -o $@

fixed.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/fixed.c -o $@
