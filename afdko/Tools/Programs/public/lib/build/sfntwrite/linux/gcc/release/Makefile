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
SRC_DIR = $(ROOT_DIR)/source/sfntwrite

LIB_SRCS = $(SRC_DIR)/sfntwrite.c
LIB_OBJS = sfntwrite.o
LIB_TARGET = $(LIB_DIR)/sfntwrite.a

# Build targets
TARGETS = $(LIB_TARGET)

# Standard definitions
include ../../../../../config/linux/gcc/gcc_tx.mak

# Object rules
sfntwrite.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/sfntwrite.c -o $@

