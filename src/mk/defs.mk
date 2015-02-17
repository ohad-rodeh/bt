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
# FILES: common rules for making the C files
#
# Author: Ohad Rodeh, 8/2004
#
#*************************************************************#
# Location of object-files, libraries, and executables
OBJDIR = $(OSDROOT)/obj
LIBDIR = $(OSDROOT)/lib
BINDIR = $(OSDROOT)/bin

DEPEND = $(OSDROOT)/src/.depend

PL = $(OSDROOT)/src/pl

#*************************************************************#

#*************************************************************#
CC = gcc

#CFLAGS = -D_GNU_SOURCE -g -Wpointer-to-int-cast ##-Wall
CFLAGS = -D_GNU_SOURCE -g -Wall 

LFLAGS = -g
MKFLAGS =

#-Werror

MV	= mv
SYMLINK = ln -s
CP      = cp
RM	= rm -f
MAKE	= make	$(MK_FLAGS)
MKLIB   = ar cr 		# comment forces spaces
RANLIB  = ranlib
MKLIBO  =
GENEXE = $(CC) $(LFLAGS)
MKDIR = mkdir -p
TOUCH = touch


#*************************************************************#
#debug level
DEBUG_KEY = 3

#*************************************************************#
ifdef OPT
# for an optimized build
CFLAGS += -O2 -DOC_DEBUG=0 
LFLAGS += -O2
NO_CRC = 1
else

# debug-optimized build
CFLAGS += -DOC_DEBUG=1
endif


#*************************************************************#

