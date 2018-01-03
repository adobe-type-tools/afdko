#########################################################################
#                                                                       #
# Copyright 2014 Adobe Systems Incorporated.                       #
# All rights reserved.                                                  #
#                                                                       #
# Patents Pending                                                       #
#                                                                       #
#########################################################################

# Make definitions for Linux platform (i86)

# Configuration
PLATFORM = linux
HARDWARE = i86
COMPILER = gcc
XFLAGS = -m32

# Directories (relative to build directory)
LIB_DIR = $(ROOT_DIR)/lib/$(PLATFORM)/$(CONFIG)

STD_OPTS = $(XFLAGS) \
	-I$(ROOT_DIR)/api \
	-I$(ROOT_DIR)/resource \
	$(SYS_INCLUDES)
	
ifneq ($(strip $(OSX)),) # In order to test under Mac OSX, define OSX in the user environment.
	STD_OPTS += -DOSX=1
endif


default: $(TARGETS)

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
	

