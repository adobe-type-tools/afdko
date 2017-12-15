#########################################################################
#                                                                       #
# Copyright 1997-2014 Adobe Systems Incorporated.                       #
# All rights reserved.                                                  #
#                                                                       #
#########################################################################

# Configuration
CONFIG = debug
ROOT_DIR = ../../../..
OBJECT_DIR = .
LIB_DIR = $(ROOT_DIR)/makeotf_lib/lib/$(PLATFORM)/$(CONFIG)
CT_LIB_DIR = $(ROOT_DIR)/../public/lib/lib/$(PLATFORM)/$(CONFIG)

CFLAGS = $(STD_OPTS)  \
				-I$(ROOT_DIR)/source/include_files \
				-I$(ROOT_DIR)/makeotf_lib/api \
				-I$(ROOT_DIR)/makeotf_lib/resource \
				-I$(ROOT_DIR)/.../public/lib/api 
				-DMAKEOTFLIB_EXPORTS=1 \
				-g


# Program
PRG_SRCS = \
	$(SRC_DIR)/cb.c \
	$(SRC_DIR)/cbpriv.c \
	$(SRC_DIR)/fcdb.c \
	$(SRC_DIR)/file.c \
	$(SRC_DIR)/mac/mac.c \
	$(SRC_DIR)/main.c

PRG_OBJS = \
	$(OBJECT_DIR)/cb.o \
	$(OBJECT_DIR)/cbpriv.o \
	$(OBJECT_DIR)/fcdb.o \
	$(OBJECT_DIR)/file.o \
	$(OBJECT_DIR)/mac.o \
	$(OBJECT_DIR)/main.o

PRG_LIBS = \
		$(CT_LIB_DIR)/ctutil.a \
		$(CT_LIB_DIR)/dynarr.a \
		$(LIB_DIR)/hotconv.a \
		$(LIB_DIR)/pstoken.a \
		$(LIB_DIR)/typecomp.a \
		$(LIB_DIR)/cffread.a

PRG_TARGET = $(EXE_DIR)/makeotfexe

# Build targets
TARGETS = $(PRG_TARGET)

# Standard definitions
include ../../../../../public/config/linux/gcc/gcc.mak

# program rule is in gcc.mak

# Object rules

$(OBJECT_DIR)/cb.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/cb.c -o $@

$(OBJECT_DIR)/cbpriv.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/cbpriv.c -o $@

$(OBJECT_DIR)/fcdb.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/fcdb.c -o $@

$(OBJECT_DIR)/file.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/file.c -o $@

$(OBJECT_DIR)/mac.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/mac/mac.c -o $@

$(OBJECT_DIR)/main.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/main.c -o $@

# Lib rules
CT_LIB_BUILD_DIR = $(ROOT_DIR)/../public/lib/build
LIB_BUILD_DIR = $(ROOT_DIR)/makeotf_lib/build

$(CT_LIB_DIR)/ctutil.a:
	cd $(CT_LIB_BUILD_DIR)/ctutil/$(PLATFORM)/$(COMPILER)/$(CONFIG); $(MAKE) clean; $(MAKE);

$(CT_LIB_DIR)/dynarr.a:
	cd $(CT_LIB_BUILD_DIR)/dynarr/$(PLATFORM)/$(COMPILER)/$(CONFIG); $(MAKE) clean; $(MAKE);

$(LIB_DIR)/hotconv.a:
	cd $(LIB_BUILD_DIR)/hotconv/$(PLATFORM)/$(COMPILER)/$(CONFIG); $(MAKE) clean; $(MAKE);

$(LIB_DIR)/pstoken.a:
	cd $(LIB_BUILD_DIR)/pstoken/$(PLATFORM)/$(COMPILER)/$(CONFIG); $(MAKE) clean; $(MAKE);

$(LIB_DIR)/typecomp.a:
	cd $(LIB_BUILD_DIR)/typecomp/$(PLATFORM)/$(COMPILER)/$(CONFIG); $(MAKE) clean; $(MAKE);

$(LIB_DIR)/cffread.a:
	cd $(LIB_BUILD_DIR)/cffread/$(PLATFORM)/$(COMPILER)/$(CONFIG); $(MAKE) clean; $(MAKE);

