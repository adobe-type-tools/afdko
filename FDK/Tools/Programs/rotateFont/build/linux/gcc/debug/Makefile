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
PRG_SRCS = $(SRC_DIR)/rotateFont.c
PRG_OBJS = rotateFont.o
PRG_TARGET = $(EXE_DIR)/rotateFont

# Build targets
TARGETS = $(PRG_TARGET)

# Standard definitions
include ../../../../../public/config/linux/gcc/gcc_tx.mak

# Object rules
rotateFont.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/rotateFont.c -o $@
