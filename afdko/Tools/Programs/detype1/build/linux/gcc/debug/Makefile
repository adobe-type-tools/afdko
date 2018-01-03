#########################################################################
#                                                                       #
# Copyright 2014 Adobe Systems Incorporated.                       #
# All rights reserved.                                                  #
#                                                                       #
#########################################################################

# Configuration
CONFIG = debug
ROOT_DIR = ../../../..

CFLAGS = $(STD_OPTS) -g

# Program
PRG_SRCS = $(SRC_DIR)/detype1.c
PRG_OBJS = detype1.o
PRG_TARGET = $(EXE_DIR)/detype1

# Build targets
TARGETS = $(PRG_TARGET)

# Standard definitions
include ../../../../../public/config/linux/gcc/gcc.mak

# Object rules
detype1.o: 
	$(CC) $(CFLAGS) -c $(SRC_DIR)/detype1.c -o $@

# AUTO-GENERATED DEPENDENCIES
detype1.o: $(SRC_DIR)/detype1.c
