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
SRC_DIR = $(ROOT_DIR)/source/t2cstr

LIB_SRCS = $(SRC_DIR)/t2cstr.c
LIB_OBJS = t2cstr.o
LIB_TARGET = $(LIB_DIR)/t2cstr.a

# Build targets
TARGETS = $(LIB_TARGET)

# Standard definitions
include ../../../../../config/linux/gcc/gcc_tx.mak

# Object rules
t2cstr.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/t2cstr.c -o $@
