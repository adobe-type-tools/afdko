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
SRC_DIR = $(ROOT_DIR)/source/support

LIB_SRCS = \
	$(SRC_DIR)/canthappen.c \
	$(SRC_DIR)/except.c \
	$(SRC_DIR)/fixed.c
LIB_OBJS = canthappen.o except.o fixed.o

LIB_TARGET = $(LIB_DIR)/support.a

# Build targets
TARGETS = $(LIB_TARGET)

# Standard definitions
include $(realpath ../../../../../config/linux/gcc/gcc_tx.mak)

# Object rules
canthappen.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/canthappen.c -o $@

except.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/except.c -o $@

fixed.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/fixed.c -o $@
