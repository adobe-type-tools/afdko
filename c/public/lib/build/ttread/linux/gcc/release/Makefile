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
SRC_DIR = $(ROOT_DIR)/source/ttread

LIB_SRCS = $(SRC_DIR)/ttread.c
LIB_OBJS = ttread.o
LIB_TARGET = $(LIB_DIR)/ttread.a

# Build targets
TARGETS = $(LIB_TARGET)

# Standard definitions
include ../../../../../config/linux/gcc/gcc_tx.mak

# Object rules
ttread.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/ttread.c -o $@
