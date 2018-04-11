#########################################################################
#                                                                       #
# Copyright 2014 Adobe Systems Incorporated.                       #
# All rights reserved.                                                  #
#                                                                       #
#########################################################################


# Add info for coretype libraries.
LIB_BUILD_DIR = $(ROOT_DIR)/../public/lib/build

PRG_LIBS = \
	$(CT_LIB_DIR)/buildch.a \
	$(CT_LIB_DIR)/cfembed.a \
	$(CT_LIB_DIR)/cffread.a \
	$(CT_LIB_DIR)/cffwrite.a \
	$(CT_LIB_DIR)/ctutil.a \
	$(CT_LIB_DIR)/pdfwrite.a \
	$(CT_LIB_DIR)/sfntread.a \
	$(CT_LIB_DIR)/sfntwrite.a \
	$(CT_LIB_DIR)/sha1.a \
	$(CT_LIB_DIR)/support.a \
	$(CT_LIB_DIR)/svgwrite.a \
	$(CT_LIB_DIR)/svread.a \
	$(CT_LIB_DIR)/t1read.a \
	$(CT_LIB_DIR)/t1write.a \
	$(CT_LIB_DIR)/t2cstr.a \
	$(CT_LIB_DIR)/ttread.a \
	$(CT_LIB_DIR)/uforead.a \
	$(CT_LIB_DIR)/ufowrite.a \
	$(CT_LIB_DIR)/pstoken.a \
	$(CT_LIB_DIR)/t1cstr.a \
	$(CT_LIB_DIR)/absfont.a \
	$(CT_LIB_DIR)/dynarr.a \
	$(CT_LIB_DIR)/nameread.a \
	$(CT_LIB_DIR)/varread.a

include ../../../../../public/config/linux/gcc/gcc.mak

$(CT_LIB_DIR)/buildch.a:
	cd $(LIB_BUILD_DIR)/buildch/$(PLATFORM)/$(COMPILER)/$(CONFIG); $(MAKE) clean; $(MAKE);

$(CT_LIB_DIR)/absfont.a:
	cd $(LIB_BUILD_DIR)/absfont/$(PLATFORM)/$(COMPILER)/$(CONFIG); $(MAKE) clean; $(MAKE);

$(CT_LIB_DIR)/cfembed.a:
	cd $(LIB_BUILD_DIR)/cfembed/$(PLATFORM)/$(COMPILER)/$(CONFIG); $(MAKE) clean; $(MAKE);

$(CT_LIB_DIR)/cffread.a:
	cd $(LIB_BUILD_DIR)/cffread/$(PLATFORM)/$(COMPILER)/$(CONFIG); $(MAKE) clean; $(MAKE);

$(CT_LIB_DIR)/cffwrite.a:
	cd $(LIB_BUILD_DIR)/cffwrite/$(PLATFORM)/$(COMPILER)/$(CONFIG); $(MAKE) clean; $(MAKE);

$(CT_LIB_DIR)/ctutil.a:
	cd $(LIB_BUILD_DIR)/ctutil/$(PLATFORM)/$(COMPILER)/$(CONFIG); $(MAKE) clean; $(MAKE);

$(CT_LIB_DIR)/dynarr.a:
	cd $(LIB_BUILD_DIR)/dynarr/$(PLATFORM)/$(COMPILER)/$(CONFIG); $(MAKE) clean; $(MAKE);

$(CT_LIB_DIR)/nameread.a:
	cd $(LIB_BUILD_DIR)/nameread/$(PLATFORM)/$(COMPILER)/$(CONFIG); $(MAKE) clean; $(MAKE);

$(CT_LIB_DIR)/nameread.a:
	cd $(LIB_BUILD_DIR)/nameread/$(PLATFORM)/$(COMPILER)/$(CONFIG); $(MAKE) clean; $(MAKE);
$(CT_LIB_DIR)/pdfwrite.a:
	cd $(LIB_BUILD_DIR)/pdfwrite/$(PLATFORM)/$(COMPILER)/$(CONFIG); $(MAKE) clean; $(MAKE);

$(CT_LIB_DIR)/pstoken.a:
	cd $(LIB_BUILD_DIR)/pstoken/$(PLATFORM)/$(COMPILER)/$(CONFIG); $(MAKE) clean; $(MAKE);

$(CT_LIB_DIR)/sfntread.a:
	cd $(LIB_BUILD_DIR)/sfntread/$(PLATFORM)/$(COMPILER)/$(CONFIG); $(MAKE) clean; $(MAKE);

$(CT_LIB_DIR)/sfntwrite.a:
	cd $(LIB_BUILD_DIR)/sfntwrite/$(PLATFORM)/$(COMPILER)/$(CONFIG); $(MAKE) clean; $(MAKE);

$(CT_LIB_DIR)/sha1.a:
	cd $(LIB_BUILD_DIR)/sha1/$(PLATFORM)/$(COMPILER)/$(CONFIG); $(MAKE) clean; $(MAKE);

$(CT_LIB_DIR)/support.a:
	cd $(LIB_BUILD_DIR)/support/$(PLATFORM)/$(COMPILER)/$(CONFIG); $(MAKE) clean; $(MAKE);

$(CT_LIB_DIR)/svgwrite.a:
	cd $(LIB_BUILD_DIR)/svgwrite/$(PLATFORM)/$(COMPILER)/$(CONFIG); $(MAKE) clean; $(MAKE);

$(CT_LIB_DIR)/svread.a:
	cd $(LIB_BUILD_DIR)/svread/$(PLATFORM)/$(COMPILER)/$(CONFIG); $(MAKE) clean; $(MAKE);

$(CT_LIB_DIR)/t1cstr.a:
	cd $(LIB_BUILD_DIR)/t1cstr/$(PLATFORM)/$(COMPILER)/$(CONFIG); $(MAKE) clean; $(MAKE);

$(CT_LIB_DIR)/t1read.a:
	cd $(LIB_BUILD_DIR)/t1read/$(PLATFORM)/$(COMPILER)/$(CONFIG); $(MAKE) clean; $(MAKE);

$(CT_LIB_DIR)/t1write.a:
	cd $(LIB_BUILD_DIR)/t1write/$(PLATFORM)/$(COMPILER)/$(CONFIG); $(MAKE) clean; $(MAKE);

$(CT_LIB_DIR)/t2cstr.a:
	cd $(LIB_BUILD_DIR)/t2cstr/$(PLATFORM)/$(COMPILER)/$(CONFIG); $(MAKE) clean; $(MAKE);

$(CT_LIB_DIR)/ttread.a:
	cd $(LIB_BUILD_DIR)/ttread/$(PLATFORM)/$(COMPILER)/$(CONFIG); $(MAKE) clean; $(MAKE);

$(CT_LIB_DIR)/uforead.a:
	cd $(LIB_BUILD_DIR)/uforead/$(PLATFORM)/$(COMPILER)/$(CONFIG); $(MAKE) clean; $(MAKE);

$(CT_LIB_DIR)/ufowrite.a:
	cd $(LIB_BUILD_DIR)/ufowrite/$(PLATFORM)/$(COMPILER)/$(CONFIG); $(MAKE) clean; $(MAKE);

$(CT_LIB_DIR)/varread.a:
	cd $(LIB_BUILD_DIR)/varread/$(PLATFORM)/$(COMPILER)/$(CONFIG); $(MAKE) clean; $(MAKE);
