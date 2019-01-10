#########################################################################
#                                                                       #
# Copyright 2018 Adobe Systems Incorporated.                            #
# All rights reserved.                                                  #
#                                                                       #
#########################################################################

# Configuration
CONFIG = debug
CFLAGS = $(STD_OPTS)

# Library
ROOT_DIR = ../../../../..
SRC_DIR = $(ROOT_DIR)/source/tx_shared

LIB_SRCS = $(SRC_DIR)/tx_shared.c
LIB_OBJS = tx_shared.o
LIB_TARGET = $(LIB_DIR)/tx_shared.a

# Build targets
TARGETS = $(LIB_TARGET)

# Standard definitions
include ../../../../../config/linux/gcc/gcc_tx.mak

# Object rules
tx_shared.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/tx_shared.c -o $@
