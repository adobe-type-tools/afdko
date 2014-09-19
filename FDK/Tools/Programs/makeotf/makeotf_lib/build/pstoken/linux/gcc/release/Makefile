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
SRC_DIR = $(ROOT_DIR)/source/pstoken
OBJECT_DIR = .
LIB_TARGET = $(LIB_DIR)/pstoken.a

CFLAGS = $(STD_OPTS) -I$(ROOT_DIR)/api \
					 -I$(ROOT_DIR)/../../public/lib/api \
					 -I$(ROOT_DIR)/../../public/lib/resource \
					 -DCFF_DEBUG=1 \
					 -DCFF_T13_SUPPORT=0
					 
LIB_SRCS = $(SRC_DIR)/pstoken.c

LIB_OBJS = pstoken.o

# Standard definitions
include  $(ROOT_DIR)/../../public/lib/config/linux/gcc/gcc.mak

	
# Lib Object rules
pstoken.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/pstoken.c -o $@

