#########################################################################
#                                                                       #
# Copyright 2014 Adobe Systems Incorporated.                       #
# All rights reserved.                                                  #
#                                                                       #
#########################################################################

# Configuration
CONFIG = release
CFLAGS = $(STD_OPTS)

# Library
ROOT_DIR = ../../../../..
SRC_DIR = $(ROOT_DIR)/source/t1cstr

LIB_SRCS = $(SRC_DIR)/t1cstr.c
LIB_OBJS = t1cstr.o
LIB_TARGET = $(LIB_DIR)/t1cstr.a

# Build targets
TARGETS = $(LIB_TARGET)

# Standard definitions
include ../../../../../config/linux/gcc/gcc_tx.mak

# Object rules
t1cstr.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/t1cstr.c -o $@
