# Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
# This software is licensed as OpenSource, under the Apache License, Version 2.0. 
# This license is available at: http://opensource.org/licenses/Apache-2.0.

# Configuration
CONFIG = debug
CFLAGS = $(STD_OPTS) -g -DDNA_DEBUG=1

# Library
SRC_DIR = $(ROOT_DIR)/source/lib/dynarr
LIB_SRCS = $(SRC_DIR)/dynarr.c
LIB_OBJS = dynarr.o
LIB_TARGET = $(LIB_DIR)/dynarr.a

# Build targets
TARGETS = $(LIB_TARGET)

# Standard definitions
include ../../../../../../config/linux/pentium/gcc-2.96.mak

# Object rules
dynarr.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/dynarr.c -o $@
# AUTO-GENERATED DEPENDENCIES
dynarr.o: ../../../../../../../source/lib/dynarr/dynarr.c \
  ../../../../../../../public/api/dynarr.h \
  ../../../../../../../public/api/ctlshare.h
