#########################################################################
#                                                                       #
# Copyright 2014 Adobe Systems Incorporated.                       #
# All rights reserved.                                                  #
#                                                                       #
#########################################################################

# Make definitions for Linux platform (x86-64)

# Configuration
PLATFORM = linux
HARDWARE = x86-64
COMPILER = gcc
SYS_LIBS = -lm

# Directories (relative to build directory)
LIB_DIR = $(ROOT_DIR)/lib/$(PLATFORM)/$(CONFIG)

STD_OPTS = $(XFLAGS) \
$(SYS_INCLUDES)

ifneq ($(strip $(OSX)),) # In order to test under Mac OSX, define OSX in the user environment.
	STD_OPTS += -DOSX=1
endif

default: $(LIB_TARGET)

$(LIB_TARGET): $(LIB_OBJS)
	mkdir -p $(LIB_DIR)
	$(AR) -rvs $@ $?

clean:
	if [ "$(LIB_OBJS)" ]; then \
			rm -f $(LIB_OBJS); \
	fi
	
	if [ "$(LIB_TARGET)" ]; then \
			rm -f $(LIB_TARGET); \
	fi
	

