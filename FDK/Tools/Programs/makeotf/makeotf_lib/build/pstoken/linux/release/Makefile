# Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
# This software is licensed as OpenSource, under the Apache License, Version 2.0. 
# This license is available at: http://opensource.org/licenses/Apache-2.0.

# Configuration
CONFIG = release
CFLAGS = $(STD_OPTS) -O2

# Library
SRC_DIR = $(ROOT_DIR)/source/pstoken
LIB_SRCS = $(SRC_DIR)/pstoken.c
LIB_OBJS = pstoken.o
LIB_TARGET = $(LIB_DIR)/pstoken.a

# Build targets
TARGETS = $(LIB_TARGET)

# Standard definitions
include ../../../standard/linux.mak

# Auto-generated dependencies
include depend.mak

# Object rules
pstoken.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/pstoken.c -o $@
