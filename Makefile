############################################################################
# apps/Makefile
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

-include $(TOPDIR)/Make.defs

ifeq ($(WINTOOL),y)
INCDIROPT	= -w
endif

# Application Directories

BUILTIN_APPS_DIR =
BUILTIN_APPS_BUILT =

ifeq ($(CONFIG_BUILTIN_APPS_NUTTX),y)


# Individual application: HELLO

ifeq ($(CONFIG_BUILTIN_APPS_HELLO),y)

BUILTIN_APPS_DIR += hello

# we use a non-existing .built_always to guarantee that Makefile
# always walks into the sub-directories and asks for build
BUILTIN_APPS_BUILT += hello/.built_always

hello/libhello$(LIBEXT):
	@$(MAKE) -C hello TOPDIR="$(TOPDIR)" libhello$(LIBEXT)

endif

# end of application list

endif


ROOTDEPPATH	= --dep-path .
ASRCS		=
CSRCS		= exec_nuttapp.c

AOBJS		= $(ASRCS:.S=$(OBJEXT))
COBJS		= $(CSRCS:.c=$(OBJEXT))

SRCS		= $(ASRCS) $(CSRCS)
OBJS		= $(AOBJS) $(COBJS)

BIN	    	= libapps$(LIBEXT)

VPATH		= 

all:	$(BIN)

$(AOBJS): %$(OBJEXT): %.S
	$(call ASSEMBLE, $<, $@)

$(COBJS): %$(OBJEXT): %.c
	$(call COMPILE, $<, $@)
	
$(BUILTIN_APPS_BUILT):
	@for dir in $(BUILTIN_APPS_DIR) ; do \
		$(MAKE) -C $$dir TOPDIR="$(TOPDIR)" ; \
	done

$(BIN):	$(OBJS) $(BUILTIN_APPS_BUILT)
	@( for obj in $(OBJS) ; do \
		$(call ARCHIVE, $@, $${obj}); \
	done ; )

.depend: Makefile $(SRCS)
	@$(MKDEP) $(ROOTDEPPATH) \
	  $(CC) -- $(CFLAGS) -- $(SRCS) >Make.dep
	@touch $@
	echo "/* List of application requirements, generated during make depend. */" > exec_nuttapp_list.h
	echo "/* List of application entry points, generated during make depend. */" > exec_nuttapp_proto.h
	@for dir in $(BUILTIN_APPS_DIR) ; do \
		$(MAKE) -C $$dir TOPDIR="$(TOPDIR)" depend ; \
	done

depend: .depend

clean:
	@rm -f $(BIN) *~ .*.swp *.o libapps.a
	$(call CLEAN)

distclean: clean
	@rm -f Make.dep .depend

-include Make.dep
