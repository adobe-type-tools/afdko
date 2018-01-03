#########################################################################
#                                                                       #
# Copyright 2014 Adobe Systems Incorporated.                       #
# All rights reserved.                                                  #
#                                                                       #
#########################################################################

# Configuration
CONFIG = debug
CFLAGS = $(STD_OPTS) -g -DPST_DEBUG=1

# Library
ROOT_DIR = ../../../../..
SRC_DIR = $(ROOT_DIR)/source/pstoken

LIB_SRCS = $(SRC_DIR)/pstoken.c
LIB_OBJS = pstoken.o
LIB_TARGET = $(LIB_DIR)/pstoken.a

# Build targets
TARGETS = $(LIB_TARGET)

# Standard definitions
include ../../../../../config/linux/gcc/gcc_tx.mak

# Object rules
pstoken.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/pstoken.c -o $@
