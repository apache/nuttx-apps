############################################################################
# Makefile
#
#   Copyright (C) 2011 Uros Platise. All rights reserved.
#   Author: Uros Platise <uros.platise@isotel.eu>
#           Gregory Nutt <spudmonkey@racsa.co.cr>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
# 3. Neither the name NuttX nor the names of its contributors may be
#    used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
# OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
# AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
############################################################################

# TODO, this makefile should run make under the app dirs, instead of
# sourcing the Make.defs!

-include $(TOPDIR)/Make.defs
include ../Make.defs

ifeq ($(WINTOOL),y)
INCDIROPT	= -w
endif

# Hello Application
# TODO: appname can be automatically extracted from the directory name

APPNAME		= free
PRIORITY	= SCHED_PRIORITY_DEFAULT
STACKSIZE	= 768

ASRCS		=
CSRCS		= free.c

AOBJS		= $(ASRCS:.S=$(OBJEXT))
COBJS		= $(CSRCS:.c=$(OBJEXT))

SRCS		= $(ASRCS) $(CSRCS)
OBJS		= $(AOBJS) $(COBJS)

BIN	    	= ../libapps$(LIBEXT)

ROOTDEPPATH	= --dep-path .

# Common build

VPATH		= 

all:	.built

$(AOBJS): %$(OBJEXT): %.S
	$(call ASSEMBLE, $<, $@)

$(COBJS): %$(OBJEXT): %.c
	$(call COMPILE, $<, $@)

$(BIN):	$(OBJS)
	@( for obj in $(OBJS) ; do \
		$(call ARCHIVE, $@, $${obj}); \
	done ; )
	@touch .built
	
.built: $(BIN)

.depend: Makefile $(SRCS)
	@$(MKDEP) $(ROOTDEPPATH) \
	  $(CC) -- $(CFLAGS) -- $(SRCS) >Make.dep
	$(call REGISTER,$(APPNAME),$(PRIORITY),$(STACKSIZE),$(APPNAME)_main)
	@touch $@

# Register application
depend: .depend

clean:
	@rm -f $(BIN) *.o *~ .*.swp .built
	$(call CLEAN)

distclean: clean
	@rm -f Make.dep .depend

-include Make.dep
