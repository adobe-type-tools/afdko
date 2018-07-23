# Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
# This software is licensed as OpenSource, under the Apache License, Version 2.0. 
# This license is available at: http://opensource.org/licenses/Apache-2.0.

# Configuration
CONFIG = release
CFLAGS = $(STD_OPTS) -O2

# Library
SRC_DIR = $(ROOT_DIR)/source/lib/pdfwrite
LIB_SRCS = $(SRC_DIR)/pdfwrite.c
LIB_OBJS = pdfwrite.o
LIB_TARGET = $(LIB_DIR)/pdfwrite.a

# Build targets
TARGETS = $(LIB_TARGET)

# Standard definitions
include ../../../../../../config/linux/pentium/gcc-2.96.mak

# Object rules
pdfwrite.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/pdfwrite.c -o $@
# AUTO-GENERATED DEPENDENCIES
pdfwrite.o: ../../../../../../../source/lib/pdfwrite/pdfwrite.c \
  ../../../../../../../public/api/pdfwrite.h \
  ../../../../../../../public/api/ctlshare.h \
  ../../../../../../../public/api/absfont.h \
  ../../../../../../../public/api/abfdesc.h \
  ../../../../../../../public/api/abferr.h \
  ../../../../../../../public/api/pdwerr.h \
  ../../../../../../../public/api/dynarr.h
