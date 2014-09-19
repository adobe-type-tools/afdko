#########################################################################
#                                                                       #
# Copyright 1997-2003 Adobe Systems Incorporated.                       #
# All rights reserved.                                                  #
#                                                                       #
# Patents Pending                                                       #
#                                                                       #
# NOTICE: All information contained herein is the property of Adobe     #
# Systems Incorporated. Many of the intellectual and technical          #
# concepts contained herein are proprietary to Adobe, are protected     #
# as trade secrets, and are made available only to Adobe licensees      #
# for their internal use. Any reproduction or dissemination of this     #
# software is strictly forbidden unless prior written permission is     #
# obtained from Adobe.                                                  #
#                                                                       #
# PostScript and Display PostScript are trademarks of Adobe Systems     #
# Incorporated or its subsidiaries and may be registered in certain     #
# jurisdictions.                                                        #
#                                                                       #
#########################################################################

# Configuration
CONFIG = release
ROOT_DIR = ../../../..
OBJECT_DIR = .

CFLAGS = $(STD_OPTS) -m32 -I$(ROOT_DIR)/../spot/sfnt_includes

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

