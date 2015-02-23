#*************************************************************#
#
# Copyright (c) 2014-2015, Ohad Rodeh, IBM Research
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
# Makefile for B-Plus Tree Test module
#
#*************************************************************#
OSDROOT=../../../..
OCROOT=../..

include $(OSDROOT)/src/mk/defs.mk
include $(OSDROOT)/src/mk/rules.mk

include $(OC)/crt/files.mk
include $(OC)/utl/files.mk
include $(OC)/bpt/files.mk

#*************************************************************#

SUBDIRS =  crt ds utl bpt 

CFLAGS += $(SUBDIRS:%=-I $(OCROOT)/%)

CFLAGS += -I $(OSDROOT)/src/pl

#*************************************************************#

OBJ = \
	${OBJDIR}/pl_trace.o \
	${CRT_OBJECTS} \
	${UTL_OBJECTS} \
	${BPT_OBJECTS} \
	${OBJDIR}/oc_bpt_test_fs.o \
	${OBJDIR}/oc_bpt_alt.o \
	${OBJDIR}/oc_bpt_test_utl.o

all :  \
       $(BINDIR)/oc_bpt_test_st \
       $(BINDIR)/oc_bpt_test_clone_st \
       $(BINDIR)/oc_bpt_test_mt


$(BINDIR)/oc_bpt_test_st : ${OBJ} \
			   ${OBJDIR}/oc_bpt_test_st.o 
	$(GENEXE) -o $(BINDIR)/oc_bpt_test_st \
	      ${OBJDIR}/oc_bpt_test_st.o \
	      $(OBJ) \
	   -L$(OSDROOT)/lib -lpl -lpthread

$(BINDIR)/oc_bpt_test_mt : ${OBJ} \
			   ${OBJDIR}/oc_bpt_test_mt.o 
	$(GENEXE) -o $(BINDIR)/oc_bpt_test_mt \
	      ${OBJDIR}/oc_bpt_test_mt.o \
	      $(OBJ) \
	   -L$(OSDROOT)/lib -lpl -lpthread

$(BINDIR)/oc_bpt_test_clone_st : ${OBJ} \
			         ${OBJDIR}/oc_bpt_test_clone_st.o \
			         ${OBJDIR}/oc_bpt_test_clone.o
	$(GENEXE) -o $(BINDIR)/oc_bpt_test_clone_st \
	      ${OBJDIR}/oc_bpt_test_clone_st.o \
              ${OBJDIR}/oc_bpt_test_clone.o \
	      $(OBJ) \
	   -L$(OSDROOT)/lib -lpl -lpthread

jpg :
	dot -Tjpg X.dot -o X.jpg

clean : 
	$(RM) ${OBJDIR}/oc_bpt_*.o
	$(RM) ${BINDIR}/oc_bpt_*
	$(RM) *.o

realclean : clean 

bpt_test: all

#*************************************************************#

ifeq ($(DEPEND), $(wildcard $(DEPEND)))
  include $(DEPEND)
else
  $(error "Must create a top-level .depend file, then, do a make depend")
endif

#*************************************************************#
