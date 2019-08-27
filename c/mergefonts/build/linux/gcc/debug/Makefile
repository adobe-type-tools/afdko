#########################################################################
#                                                                       #
# Copyright 1997-2003 Adobe Systems Incorporated.                       #
# All rights reserved.                                                  #
#                                                                       #
#########################################################################

# Configuration
CONFIG = debug
ROOT_DIR = ../../../..
OBJECT_DIR = .

CFLAGS = $(STD_OPTS) -g

# Program
PRG_SRCS = $(SRC_DIR)/mergeFonts.c
PRG_OBJS = mergeFonts.o
PRG_TARGET = $(EXE_DIR)/mergefonts

# Build targets
TARGETS = $(PRG_TARGET)

# Standard definitions
include ../../../../../public/config/linux/gcc/gcc_tx.mak

# include tx/rotatefont/mergefonts shared header files
STD_OPTS += -I$(realpath $(ROOT_DIR)/../public/lib/source/tx_shared)

# Object rules
mergeFonts.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/mergeFonts.c -o $@
