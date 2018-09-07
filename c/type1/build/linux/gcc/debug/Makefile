#########################################################################
#                                                                       #
# Copyright 1997-2001 Adobe Systems Incorporated.                       #
# All rights reserved.                                                  #
#                                                                       #
#########################################################################

# Configuration
CONFIG = debug
ROOT_DIR = ../../../..
OBJECT_DIR = .

CFLAGS = $(STD_OPTS) -g

# Program
PRG_SRCS = $(SRC_DIR)/type1.c
PRG_OBJS = $(OBJECT_DIR)/type1.o
PRG_TARGET = $(EXE_DIR)/type1

# Build targets
TARGETS = $(PRG_TARGET)

# Standard definitions
include ../../../../../public/config/linux/gcc/gcc.mak

# Object rules
$(OBJECT_DIR)/type1.o: 
	$(CC) $(CFLAGS) -c $(SRC_DIR)/type1.c -o $@

