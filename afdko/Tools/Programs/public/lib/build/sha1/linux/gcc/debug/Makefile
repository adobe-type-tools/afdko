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
SRC_DIR = $(ROOT_DIR)/source/sha1

LIB_SRCS = $(SRC_DIR)/sha1.c
LIB_OBJS = sha1.o
LIB_TARGET = $(LIB_DIR)/sha1.a

# Build targets
TARGETS = $(LIB_TARGET)

# Standard definitions
include ../../../../../config/linux/gcc/gcc_tx.mak

# Object rules
sha1.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/sha1.c -o $@
