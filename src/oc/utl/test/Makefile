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
# Makefile for tests for CRT
#
#*************************************************************#
OSDROOT=../../../../..
OCROOT=../..
include ${OSDROOT}/src/mk/defs.mk
include ${OSDROOT}/src/mk/rules.mk
include ${OCROOT}/files_all.mk
#*************************************************************#
CFLAGS += \
	-I $(OSDROOT)/src/pl \
	-I ${OCROOT}/ds \
	-I ${OCROOT}/utl \
	-I ${OCROOT}/crt

#*************************************************************#

all : \
	$(BINDIR)/oc_utl_htbl_test \
	$(BINDIR)/oc_utl_trk_test

oc_utl_htbl_OBJECTS = \
	${OBJDIR}/oc_utl_htbl_test.o \
	${UTL_OBJECTS}

$(BINDIR)/oc_utl_htbl_test : ${oc_utl_htbl_OBJECTS}
	$(GENEXE) -o $(BINDIR)/oc_utl_htbl_test \
	  ${oc_utl_htbl_OBJECTS} \
	  ${CRT_MALLOC_OBJECTS} \
	-L${OSDROOT}/lib -lpl -lpthread


oc_utl_trk_OBJECTS = \
	${OBJDIR}/oc_utl_trk_test.o \
	${UTL_OBJECTS}

$(BINDIR)/oc_utl_trk_test: ${oc_utl_trk_OBJECTS}
	$(GENEXE) -o $(BINDIR)/oc_utl_trk_test \
	  ${oc_utl_trk_OBJECTS} \
	  ${CRT_MALLOC_OBJECTS} \
	-L${OSDROOT}/lib -lpl -lpthread

clean : 
	$(RM) ${OBJDIR}/oc_utl*.o
	$(RM) ${BINDIR}/oc_utl*

#*************************************************************#
ifeq ($(DEPEND), $(wildcard $(DEPEND)))
  include $(DEPEND)
else
  $(error "Must create a top-level .depend file, then, do a make depend")
endif
#*************************************************************#




