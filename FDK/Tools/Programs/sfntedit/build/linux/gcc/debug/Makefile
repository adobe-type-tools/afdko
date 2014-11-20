# Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
# This software is licensed as OpenSource, under the Apache License, Version 2.0. 
# This license is available at: http://opensource.org/licenses/Apache-2.0.

# Configuration
CONFIG = debug
ROOT_DIR = ../../../..
OBJECT_DIR = .

CFLAGS = $(STD_OPTS) -g -I$(ROOT_DIR)/../spot/sfnt_includes

# Program
PRG_SRCS = $(SRC_DIR)/Eda.c \
	$(SRC_DIR)/Efile.c \
	$(SRC_DIR)/Eglobal.c \
	$(SRC_DIR)/Emsgs.c \
	$(SRC_DIR)/Esys.c \
	$(SRC_DIR)/main.c


PRG_INCLUDES = $(SRC_DIR)/Eda.h \
	$(SRC_DIR)/Efile.h \
	$(SRC_DIR)/Eglobal.h \
	$(SRC_DIR)/Emsgs.h \
	$(SRC_DIR)/Esys.h \
	$(SRC_DIR)/otftableeditor.h \
	$(SRC_DIR)/otter.h


PRG_OBJS = $(OBJECT_DIR)/Eda.o \
	$(OBJECT_DIR)/Efile.o \
	$(OBJECT_DIR)/Eglobal.o \
	$(OBJECT_DIR)/Emsgs.o \
	$(OBJECT_DIR)/Esys.o \
	$(OBJECT_DIR)/main.o
	
PRG_TARGET = $(EXE_DIR)/sfntedit

# Build targets
TARGETS = $(PRG_TARGET)

# Standard definitions
include ../../../../../public/config/linux/gcc/gcc.mak

# Object rules
$(OBJECT_DIR)/Eda.o: $(SRC_DIR)/Eda.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/Eda.c -o $(OBJECT_DIR)/Eda.o
$(OBJECT_DIR)/Efile.o: $(SRC_DIR)/Efile.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/Efile.c -o $(OBJECT_DIR)/Efile.o
$(OBJECT_DIR)/Eglobal.o: $(SRC_DIR)/Eglobal.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/Eglobal.c -o $(OBJECT_DIR)/Eglobal.o
$(OBJECT_DIR)/Emsgs.o: $(SRC_DIR)/Emsgs.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/Emsgs.c -o $(OBJECT_DIR)/Emsgs.o
$(OBJECT_DIR)/Esys.o: $(SRC_DIR)/Esys.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/Esys.c -o $(OBJECT_DIR)/Esys.o
$(OBJECT_DIR)/main.o: $(SRC_DIR)/main.c $(PRG_INCLUDES)
	$(CC)  $(CFLAGS) -c $(SRC_DIR)/main.c -o $(OBJECT_DIR)/main.o

