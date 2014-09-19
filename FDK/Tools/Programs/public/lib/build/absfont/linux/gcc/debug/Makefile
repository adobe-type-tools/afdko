#########################################################################
#                                                                       #
# Copyright 2014 Adobe Systems Incorporated.                       #
# All rights reserved.                                                  #
#                                                                       #
#########################################################################

# Configuration
CONFIG = debug
CFLAGS = $(STD_OPTS) -g -DABF_DEBUG=1

# Library
ROOT_DIR = ../../../../..
SRC_DIR = $(ROOT_DIR)/source/absfont

LIB_SRCS = \
	$(SRC_DIR)/absfont.c \
	$(SRC_DIR)/absfont_afm.c \
	$(SRC_DIR)/absfont_compare.c \
	$(SRC_DIR)/absfont_desc.c \
	$(SRC_DIR)/absfont_draw.c \
	$(SRC_DIR)/absfont_dump.c \
	$(SRC_DIR)/absfont_metrics.c \
	$(SRC_DIR)/absfont_path.c
LIB_OBJS = \
	absfont_afm.o \
	absfont_compare.o \
	absfont_desc.o \
	absfont_draw.o \
	absfont_dump.o \
	absfont_metrics.o \
	absfont_path.o \
	absfont.o

LIB_TARGET = $(LIB_DIR)/absfont.a

# Build targets
TARGETS = $(LIB_TARGET)

# Standard definitions
include ../../../../../config/linux/gcc/gcc_tx.mak

# Object rules
absfont.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/absfont.c -o $@

absfont_compare.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/absfont_compare.c -o $@

absfont_dump.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/absfont_dump.c -o $@

absfont_draw.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/absfont_draw.c -o $@

absfont_metrics.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/absfont_metrics.c -o $@

absfont_path.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/absfont_path.c -o $@

absfont_afm.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/absfont_afm.c -o $@

absfont_desc.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/absfont_desc.c -o $@

