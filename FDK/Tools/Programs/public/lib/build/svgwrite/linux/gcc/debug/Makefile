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
SRC_DIR = $(ROOT_DIR)/source/svgwrite

LIB_SRCS = $(SRC_DIR)/svgwrite.c
LIB_OBJS = svgwrite.o
LIB_TARGET = $(LIB_DIR)/svgwrite.a

# Build targets
TARGETS = $(LIB_TARGET)

# Standard definitions
include ../../../../../config/linux/gcc/gcc_tx.mak

# Object rules
svgwrite.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/svgwrite.c -o $@
