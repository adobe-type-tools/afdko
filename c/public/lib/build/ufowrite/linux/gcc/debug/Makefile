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
SRC_DIR = $(ROOT_DIR)/source/ufowrite

LIB_SRCS = $(SRC_DIR)/ufowrite.c
LIB_OBJS = ufowrite.o
LIB_TARGET = $(LIB_DIR)/ufowrite.a

# Build targets
TARGETS = $(LIB_TARGET)

# Standard definitions
include ../../../../../config/linux/gcc/gcc_tx.mak

# Object rules
ufowrite.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/ufowrite.c -o $@
