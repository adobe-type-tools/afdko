#########################################################################
#                                                                       #
# Copyright 2014 Adobe Systems Incorporated.                       #
# All rights reserved.                                                  #
#                                                                       #
#########################################################################

# Configuration
CONFIG = debug
CFLAGS = $(STD_OPTS) -g -DDNA_DEBUG=1

# Library
ROOT_DIR = ../../../../..
SRC_DIR = $(ROOT_DIR)/source/dynarr

LIB_SRCS = $(SRC_DIR)/dynarr.c
LIB_OBJS = dynarr.o
LIB_TARGET = $(LIB_DIR)/dynarr.a

# Build targets
TARGETS = $(LIB_TARGET)

# Standard definitions
include ../../../../../config/linux/gcc/gcc_tx.mak

# Object rules
dynarr.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/dynarr.c -o $@
