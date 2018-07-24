#########################################################################
#                                                                       #
# Copyright 1997-2003 Adobe Systems Incorporated.                       #
# All rights reserved.                                                  #
#                                                                       #
#########################################################################

# Configuration
CONFIG = release
ROOT_DIR = ../../../..
OBJECT_DIR = .

CFLAGS = $(STD_OPTS)

# Program
PRG_SRCS = $(SRC_DIR)/mergeFonts.c
PRG_OBJS = mergeFonts.o
PRG_TARGET = $(EXE_DIR)/mergeFonts

# Build targets
TARGETS = $(PRG_TARGET)

# Standard definitions
include ../../../../../public/config/linux/gcc/gcc_tx.mak

# Object rules
mergeFonts.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/mergeFonts.c -o $@
