# Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
# This software is licensed as OpenSource, under the Apache License, Version 2.0. 
# This license is available at: http://opensource.org/licenses/Apache-2.0.

# Configuration
CONFIG = release
CFLAGS = $(STD_OPTS) -O2

# Program
SRC_DIR = $(ROOT_DIR)/source/exe/detype1
PRG_SRCS = $(SRC_DIR)/detype1.c
PRG_OBJS = detype1.o
PRG_TARGET = $(EXE_DIR)/detype1
MAKEALL = \
	exe/detype1

# Build targets
TARGETS = $(PRG_TARGET)

# Standard definitions
include ../../../../../../config/linux/pentium/gcc-2.96.mak

# Object rules
detype1.o: 
	$(CC) $(CFLAGS) -c $(SRC_DIR)/detype1.c -o $@

# AUTO-GENERATED DEPENDENCIES
detype1.o: ../../../../../../../source/exe/detype1/detype1.c
