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
SRC_DIR = $(ROOT_DIR)/source/sfntread

LIB_SRCS = $(SRC_DIR)/sfntread.c
LIB_OBJS = sfntread.o
LIB_TARGET = $(LIB_DIR)/sfntread.a

# Build targets
TARGETS = $(LIB_TARGET)

# Standard definitions
include ../../../../../config/linux/gcc/gcc_tx.mak

# Object rules
sfntread.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/sfntread.c -o $@
