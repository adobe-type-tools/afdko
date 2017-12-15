#########################################################################
#                                                                       #
# Copyright 2014 Adobe Systems Incorporated.                       #
# All rights reserved.                                                  #
#                                                                       #
#########################################################################

# Configuration
CONFIG = release
CFLAGS = $(STD_OPTS)

# Library
ROOT_DIR = ../../../../..
SRC_DIR = $(ROOT_DIR)/source/cffwrite

LIB_SRCS = \
	$(SRC_DIR)/cffwrite.c \
	$(SRC_DIR)/cffwrite_charset.c \
	$(SRC_DIR)/cffwrite_dict.c \
	$(SRC_DIR)/cffwrite_encoding.c \
	$(SRC_DIR)/cffwrite_fdselect.c \
	$(SRC_DIR)/cffwrite_sindex.c \
	$(SRC_DIR)/cffwrite_subr.c \
	$(SRC_DIR)/cffwrite_t2cstr.c \
	$(SRC_DIR)/cffwrite_varstore.c
LIB_OBJS = \
	cffwrite.o \
	cffwrite_charset.o \
	cffwrite_dict.o \
	cffwrite_encoding.o \
	cffwrite_fdselect.o \
	cffwrite_sindex.o \
	cffwrite_subr.o \
	cffwrite_t2cstr.o \
	cffwrite_varstore.o
LIB_TARGET = $(LIB_DIR)/cffwrite.a

# Build targets
TARGETS = $(LIB_TARGET)

# Standard definitions
include ../../../../../config/linux/gcc/gcc_tx.mak

# Object rules
cffwrite.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/cffwrite.c -o $@

cffwrite_charset.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/cffwrite_charset.c -o $@

cffwrite_dict.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/cffwrite_dict.c -o $@

cffwrite_encoding.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/cffwrite_encoding.c -o $@

cffwrite_fdselect.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/cffwrite_fdselect.c -o $@

cffwrite_sindex.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/cffwrite_sindex.c -o $@

cffwrite_subr.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/cffwrite_subr.c -o $@

cffwrite_t2cstr.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/cffwrite_t2cstr.c -o $@

cffwrite_varstore.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/cffwrite_varstore.c -o $@
