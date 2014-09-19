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
SRC_DIR = $(ROOT_DIR)/source/cfembed

LIB_SRCS = $(SRC_DIR)/cfembed.c
LIB_OBJS = cfembed.o

LIB_TARGET = $(LIB_DIR)/cfembed.a

# Build targets
TARGETS = $(LIB_TARGET)

# Standard definitions
include ../../../../../config/linux/gcc/gcc_tx.mak

# Object rules
cfembed.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/cfembed.c -o $@
