#########################################################################
#                                                                       #
# Copyright 2014 Adobe Systems Incorporated.                       #
# All rights reserved.                                                  #
#                                                                       #
#########################################################################

# Configuration
CONFIG = release

# Library
ROOT_DIR = ../../../../..
SRC_DIR = $(ROOT_DIR)/source/cffread
OBJECT_DIR = .
LIB_TARGET = $(LIB_DIR)/cffread.a

CFLAGS = $(STD_OPTS) -I$(ROOT_DIR)/api \
					 -I$(ROOT_DIR)/resource \
					 -I$(ROOT_DIR)/../../public/lib/api \
					 -I$(ROOT_DIR)/../../public/lib/resource \
					 -DCFF_T13_SUPPORT=0
					 
LIB_SRCS = $(SRC_DIR)/cffread.c

LIB_OBJS = cffread.o

# Standard definitions
include  $(ROOT_DIR)/../../public/lib/config/linux/gcc/gcc.mak

	
# Lib Object rules
cffread.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/cffread.c -o $@

