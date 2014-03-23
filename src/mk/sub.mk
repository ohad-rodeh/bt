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
# SUB.MK: this makefile is included in source code
# subdirectories. It allows compilations to be run from those
# directories.
#
#*************************************************************#

# The default target compiles source code for the current directory
MODULE_SRC = $(wildcard *.c)
MODULE_OBJ = $(MODULE_SRC:%.c= ${OBJDIR}/%.o)

module_objects : $(MODULE_OBJ)

module_clean : 
	$(RM) $(MODULE_OBJ)

# count how many lines of code there are 
module_wc : 
	@echo "counting lines in C and H files"
	cat *.[ch] | wc
	@echo "counting lines of code"
	cat *.[ch] | grep ";" | wc

all:
	cd $(OSDROOT)/src ; $(MAKE) all 

opt:
	cd $(OSDROOT)/src; $(MAKE) OPT=1 all

clean:
	cd $(OSDROOT)/src ; $(MAKE) clean

realclean:
	cd $(OSDROOT)/src; $(MAKE) realclean

tests:
	cd $(OSDROOT)/src; $(MAKE) tests

oc:
	cd $(OSDROOT)/src; $(MAKE) oc

pl:
	cd $(OSDROOT)/src; $(MAKE) pl

bpt:
	cd $(OSDROOT)/src; $(MAKE) bpt

depend : 
	cd $(OSDROOT)/src; $(MAKE) depend

tags: 
	cd $(OSDROOT)/src; $(MAKE) tags

build :
	cd $(OSDROOT)/src; $(MAKE) build
#*************************************************************#
