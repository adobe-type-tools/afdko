# Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
# This software is licensed as OpenSource, under the Apache License, Version 2.0. 
# This license is available at: http://opensource.org/licenses/Apache-2.0.

# Configuration
CONFIG = debug
CFLAGS = $(STD_OPTS) -g

# Library
SRC_DIR = $(ROOT_DIR)/source/lib/sha1
LIB_SRCS = $(SRC_DIR)/sha1.c
LIB_OBJS = sha1.o
LIB_TARGET = $(LIB_DIR)/sha1.a

# Build targets
TARGETS = $(LIB_TARGET)

# Standard definitions
include ../../../../../../config/linux/pentium/gcc-3.2.mak

# Object rules
sha1.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/sha1.c -o $@

# AUTO-GENERATED DEPENDENCIES
sha1.o: ../../../../../../../source/lib/sha1/sha1.c \
  ../../../../../../../public/api/sha1.h
