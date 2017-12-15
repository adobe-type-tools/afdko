# Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
# This software is licensed as OpenSource, under the Apache License, Version 2.0. 
# This license is available at: http://opensource.org/licenses/Apache-2.0.

# Configuration
CONFIG = debug
CFLAGS = $(STD_OPTS) -g

# Library
SRC_DIR = $(ROOT_DIR)/source/lib/ctutil
LIB_SRCS = $(SRC_DIR)/ctutil.c
LIB_OBJS = ctutil.o
LIB_TARGET = $(LIB_DIR)/ctutil.a

# Build targets
TARGETS = $(LIB_TARGET)

# Standard definitions
include ../../../../../../config/linux/pentium/gcc-2.96.mak

# Object rules
ctutil.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/ctutil.c -o $@
# AUTO-GENERATED DEPENDENCIES
ctutil.o: ../../../../../../../source/lib/ctutil/ctutil.c \
  ../../../../../../../public/api/ctutil.h \
  ../../../../../../../public/api/ctlshare.h
