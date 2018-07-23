#########################################################################
#                                                                       #
# Copyright 2014 Adobe Systems Incorporated.                       #
# All rights reserved.                                                  #
#                                                                       #
#########################################################################

# Configuration
CONFIG = debug
CFLAGS = $(STD_OPTS) -g -DT1W_DEBUG=1

# Library
ROOT_DIR = ../../../../..
SRC_DIR = $(ROOT_DIR)/source/t1write

LIB_SRCS = $(SRC_DIR)/t1write.c
LIB_OBJS = t1write.o
LIB_TARGET = $(LIB_DIR)/t1write.a

# Build targets
TARGETS = $(LIB_TARGET)

# Standard definitions
include ../../../../../config/linux/gcc/gcc_tx.mak

# Object rules
t1write.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/t1write.c -o $@
