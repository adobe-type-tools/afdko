# Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
# This software is licensed as OpenSource, under the Apache License, Version 2.0. 
# This license is available at: http://opensource.org/licenses/Apache-2.0.

# Configuration
CONFIG = debug
CFLAGS = $(STD_OPTS) -g

# Library
SRC_DIR = $(ROOT_DIR)/source/lib/sfntwrite
LIB_SRCS = $(SRC_DIR)/sfntwrite.c
LIB_OBJS = sfntwrite.o
LIB_TARGET = $(LIB_DIR)/sfntwrite.a

# Build targets
TARGETS = $(LIB_TARGET)

# Standard definitions
include ../../../../../../config/linux/pentium/gcc-2.96.mak

# Object rules
sfntwrite.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/sfntwrite.c -o $@
# AUTO-GENERATED DEPENDENCIES
sfntwrite.o: ../../../../../../../source/lib/sfntwrite/sfntwrite.c \
  ../../../../../../../public/api/sfntwrite.h \
  ../../../../../../../public/api/ctlshare.h \
  ../../../../../../../public/api/absfont.h \
  ../../../../../../../public/api/abfdesc.h \
  ../../../../../../../public/api/abferr.h \
  ../../../../../../../public/api/sfwerr.h \
  ../../../../../../../public/api/dynarr.h
