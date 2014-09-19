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
SRC_DIR = $(ROOT_DIR)/source/svread

LIB_SRCS = $(SRC_DIR)/svread.c
LIB_OBJS = svread.o
LIB_TARGET = $(LIB_DIR)/svread.a

# Build targets
TARGETS = $(LIB_TARGET)

# Standard definitions
include ../../../../../config/linux/gcc/gcc_tx.mak

# Object rules
svread.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/svread.c -o $@
