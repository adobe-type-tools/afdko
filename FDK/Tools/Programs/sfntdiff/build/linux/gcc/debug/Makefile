# Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
# This software is licensed as OpenSource, under the Apache License, Version 2.0. 
# This license is available at: http://opensource.org/licenses/Apache-2.0.

# Configuration
CONFIG = debug
ROOT_DIR = ../../../..
OBJECT_DIR = .

CFLAGS = $(STD_OPTS) -m32 -g -I$(ROOT_DIR)/../spot/sfnt_includes

# Program
PRG_SRCS = $(SRC_DIR)/Dmain.c \
	$(SRC_DIR)/Dda.c \
	$(SRC_DIR)/Ddesc.c \
	$(SRC_DIR)/Dfile.c \
	$(SRC_DIR)/Dglobal.c \
	$(SRC_DIR)/Dhead.c \
	$(SRC_DIR)/Dmain.c \
	$(SRC_DIR)/Dname.c \
	$(SRC_DIR)/Dopt.c \
	$(SRC_DIR)/Dpriv.c \
	$(SRC_DIR)/Dsfnt.c \
	$(SRC_DIR)/Dsys.c \
	$(SRC_DIR)/Dtto.c

PRG_INCLUDES = 	$(SRC_DIR)/Dda.h \
	$(SRC_DIR)/Ddesc.h \
	$(SRC_DIR)/Dfile.h \
	$(SRC_DIR)/Dglobal.h \
	$(SRC_DIR)/Dhead.h \
	$(SRC_DIR)/Dname.h \
	$(SRC_DIR)/Dopt.h \
	$(SRC_DIR)/Dsfnt.h \
	$(SRC_DIR)/Dsys.h \
	$(SRC_DIR)/Dtto.h \
	$(SRC_DIR)/numtypes.h \
	$(SRC_DIR)/sfntdiff.h \
	$(ROOT_DIR)/../spot/sfnt_includes/sfnt_common.h


PRG_OBJS = $(OBJECT_DIR)/Dda.o \
	$(OBJECT_DIR)/Ddesc.o \
	$(OBJECT_DIR)/Dfile.o \
	$(OBJECT_DIR)/Dglobal.o \
	$(OBJECT_DIR)/Dhead.o \
	$(OBJECT_DIR)/Dmain.o \
	$(OBJECT_DIR)/Dname.o \
	$(OBJECT_DIR)/Dopt.o \
	$(OBJECT_DIR)/Dpriv.o \
	$(OBJECT_DIR)/Dsfnt.o \
	$(OBJECT_DIR)/Dsys.o \
	$(OBJECT_DIR)/Dtto.o
	
PRG_TARGET = $(EXE_DIR)/sfntdiff

# Build targets
TARGETS = $(PRG_TARGET)

# Standard definitions
include ../../../../../public/config/linux/gcc/gcc.mak

# Object rules
$(OBJECT_DIR)/Dda.o: $(SRC_DIR)/Dda.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/Dda.c -o $(OBJECT_DIR)/Dda.o
$(OBJECT_DIR)/Ddesc.o: $(SRC_DIR)/Ddesc.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/Ddesc.c -o $(OBJECT_DIR)/Ddesc.o
$(OBJECT_DIR)/Dfile.o: $(SRC_DIR)/Dfile.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/Dfile.c -o $(OBJECT_DIR)/Dfile.o
$(OBJECT_DIR)/Dglobal.o: $(SRC_DIR)/Dglobal.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/Dglobal.c -o $(OBJECT_DIR)/Dglobal.o
$(OBJECT_DIR)/Dhead.o: $(SRC_DIR)/Dhead.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/Dhead.c -o $(OBJECT_DIR)/Dhead.o
$(OBJECT_DIR)/Dmain.o: $(SRC_DIR)/Dmain.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/Dmain.c -o $(OBJECT_DIR)/Dmain.o
$(OBJECT_DIR)/Dname.o: $(SRC_DIR)/Dname.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/Dname.c -o $(OBJECT_DIR)/Dname.o
$(OBJECT_DIR)/Dopt.o: $(SRC_DIR)/Dopt.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/Dopt.c -o $(OBJECT_DIR)/Dopt.o
$(OBJECT_DIR)/Dpriv.o: $(SRC_DIR)/Dpriv.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/Dpriv.c -o $(OBJECT_DIR)/Dpriv.o
$(OBJECT_DIR)/Dsfnt.o: $(SRC_DIR)/Dsfnt.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/Dsfnt.c -o $(OBJECT_DIR)/Dsfnt.o
$(OBJECT_DIR)/Dsys.o: $(SRC_DIR)/Dsys.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/Dsys.c -o $(OBJECT_DIR)/Dsys.o
$(OBJECT_DIR)/Dtto.o: $(SRC_DIR)/Dtto.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/Dtto.c -o $(OBJECT_DIR)/Dtto.o
