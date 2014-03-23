#*************************************************************#
#
# Copyright (c) 2014, Ohad Rodeh, IBM Research
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met: 
# 
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer. 
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution. 
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# 
# The views and conclusions contained in the software and documentation are those
# of the authors and should not be interpreted as representing official policies, 
# either expressed or implied, of IBM Research.
#
#*************************************************************#
# -*- Mode: makefile -*-
#*************************************************************#
#
# Top-level makefile 
#
#*************************************************************#
OSDROOT=..

include $(OSDROOT)/src/mk/defs.mk
include $(OSDROOT)/src/mk/rules.mk

OCROOT = $(OSDROOT)/src/oc

# file lists
include $(OSDROOT)/src/pl/files.mk
include $(OSDROOT)/src/oc/files_all.mk

#*************************************************************#

# make gpfs_osd (code that is used by gpfs)
# make harness_gpfs executables
# also make regular fs so it stays compatible

all : pre_reqs \
      $(LIBDIR)/libpl.a \
      bpt_test

everything : all tests

pre_reqs : 
	$(MKDIR) $(OBJDIR) $(LIBDIR) $(BINDIR) 

bpt_test : 
	cd $(OCROOT)/bpt/test; make all

#*************************************************************#
# Build all tests. 
#
# Add new test directires here. 

TEST_SUBDIRS = 	\
    $(OCROOT)/crt/test    \
    $(OCROOT)/utl/test    \
    $(OCROOT)/bpt/test 	 


MAKE_SUBDIRS = for d in $(TEST_SUBDIRS); do ($(MAKE) -C $$d ); done

# NOTE: "make all" includes making tests. beware that running "make" in
# the above directories doesn't cause "make all" - it will be an endless loop.
tests :
	$(MAKE_SUBDIRS)

pl : $(LIBDIR)/libpl.a

#*************************************************************#
# PL library
$(LIBDIR)/libpl.a : $(PL_OBJ)
	$(MKLIB) $(MKLIBO)$(LIBDIR)/libpl.a $(PL_OBJ)
	$(RANLIB) $(LIBDIR)/libpl.a

#*************************************************************#
# compute dependencies

SRC_FILES = $(shell find oc -name "*.[ch]" -print)
SRC_FILES += $(shell find pl -name "*.[ch]")

DEPEND_CFLAGS = $(CFLAGS)
DEPEND_CFLAGS += $(OC_INCLUDE)

DEPEND_CFLAGS += \
	-I $(OSDROOT)/src/pl

depend: pre_reqs 
	- gcc $(DEPEND_CFLAGS) -MM $(SRC_FILES) > .depend
	gawk -f $(OSDROOT)/src/tools/convert_depend.awk .depend > X
	sed 's/[a-zA-Z_\/\.]*\/\/[a-zA-Z_\/\.]*//g' X \
	  | sed 's:/src/../src/:/src/:g' > .depend
	$(RM) X

#*************************************************************#

opt : 
	$(MAKE) all OPT=1

clean : 
	$(RM) $(OBJDIR)/*

realclean : clean
	$(RM) $(LIBDIR)/*.*
	$(RM) $(BINDIR)/*
	$(RM) $(OSDROOT)/lib*/*.* 
	$(RM) $(OSDROOT)/obj*/* $(OSDROOT)/bin*/*
	find . -name "core*" | xargs rm -f
	find . -name ".*~" | xargs rm -f
	find . -name "*~" | xargs rm -f
	find . -name ".#*" | xargs rm -f
	find . -name ".nfs*" | xargs rm -f

tags:
	etags --members --declarations $(SRC_FILES) -o $(OSDROOT)/src/TAGS


#*************************************************************#
ifeq (.depend, $(wildcard .depend))
  include .depend
else
  $(error "Must create a .depend file first, then, do a make depend")
endif

#*************************************************************#
