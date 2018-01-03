#########################################################################
#                                                                       #
# Copyright 2014 Adobe Systems Incorporated.                       #
# All rights reserved.                                                  #
#                                                                       #
#########################################################################

# Configuration
CONFIG = debug
CFLAGS = $(STD_OPTS) -g

# Library
ROOT_DIR = ../../../../..
SRC_DIR = $(ROOT_DIR)/source/nameread

LIB_SRCS = $(SRC_DIR)/nameread.c
LIB_OBJS = nameread.o
LIB_TARGET = $(LIB_DIR)/nameread.a

# Build targets
TARGETS = $(LIB_TARGET)
# Standard definitions
include ../../../../../config/linux/gcc/gcc_tx.mak

# Object rules
nameread.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/nameread.c -o $@
