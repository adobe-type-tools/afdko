# Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
# This software is licensed as OpenSource, under the Apache License, Version 2.0. 
# This license is available at: http://opensource.org/licenses/Apache-2.0.

# Configuration
CONFIG = debug
CFLAGS = $(STD_OPTS) -g

# Library
SRC_DIR = $(ROOT_DIR)/source/lib/sfntread
LIB_SRCS = $(SRC_DIR)/sfntread.c
LIB_OBJS = sfntread.o
LIB_TARGET = $(LIB_DIR)/sfntread.a

# Build targets
TARGETS = $(LIB_TARGET)

# Standard definitions
include ../../../../../../config/linux/pentium/gcc-2.96.mak

# Object rules
sfntread.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/sfntread.c -o $@
# AUTO-GENERATED DEPENDENCIES
sfntread.o: ../../../../../../../source/lib/sfntread/sfntread.c \
  ../../../../../../../public/api/sfntread.h \
  ../../../../../../../public/api/ctlshare.h \
  ../../../../../../../public/api/sfrerr.h
