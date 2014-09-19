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
SRC_DIR = $(ROOT_DIR)/source/ctutil

LIB_SRCS = $(SRC_DIR)/ctutil.c
LIB_OBJS = ctutil.o
LIB_TARGET = $(LIB_DIR)/ctutil.a

# Build targets
TARGETS = $(LIB_TARGET)

# Standard definitions
include ../../../../../config/linux/gcc/gcc_tx.mak

ctutil.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/ctutil.c -o $@
