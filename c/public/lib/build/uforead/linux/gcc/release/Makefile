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
ROOT_DIR = $(realpath ../../../../..)
SRC_DIR = $(ROOT_DIR)/source/uforead

LIB_SRCS = $(SRC_DIR)/uforead.c
LIB_OBJS = uforead.o
LIB_TARGET = $(LIB_DIR)/uforead.a

# Build targets
TARGETS = $(LIB_TARGET)

# Standard definitions
include $(realpath ../../../../../config/linux/gcc/gcc_tx.mak)

# Object rules
uforead.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/uforead.c -o $@
