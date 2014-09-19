#########################################################################
#                                                                       #
# Copyright 2014 Adobe Systems Incorporated.                       #
# All rights reserved.                                                  #
#                                                                       #
#########################################################################

# Configuration
CONFIG = debug
CFLAGS = $(STD_OPTS) -g -DPDW_DEBUG=1

# Library
ROOT_DIR = ../../../../..
SRC_DIR = $(ROOT_DIR)/source/pdfwrite

LIB_SRCS = $(SRC_DIR)/pdfwrite.c
LIB_OBJS = pdfwrite.o
LIB_TARGET = $(LIB_DIR)/pdfwrite.a

# Build targets
TARGETS = $(LIB_TARGET)

# Standard definitions
include ../../../../../config/linux/gcc/gcc_tx.mak

# Object rules
pdfwrite.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/pdfwrite.c -o $@
