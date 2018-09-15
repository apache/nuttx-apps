############################################################################
# apps/builtin/Makefile
#
#   Copyright (C) 2011-2012, 2018 Gregory Nutt. All rights reserved.
#   Author: Gregory Nutt <gnutt@nuttx.org>
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

# Source and object files

CSRCS = builtin_forindex.c builtin_list.c exec_builtin.c

# Registry entry lists

PDATLIST = $(strip $(call RWILDCARD, registry, *.pdat))
BDATLIST = $(strip $(call RWILDCARD, registry, *.bdat))

registry$(DELIM).updated:
	$(Q) $(MAKE) -C registry .updated TOPDIR="$(TOPDIR)" APPDIR="$(APPDIR)"

builtin_list$(OBJEXT): builtin_list.h builtin_proto.h

builtin_list.h: registry$(DELIM).updated
	$(call DELFILE, .xx_builtin_list.h)
	$(Q) touch .xx_builtin_list.h
ifneq ($(BDATLIST),)
	$(call CATFILE, .xx_builtin_list.h, $(BDATLIST))
endif
	$(Q) mv .xx_builtin_list.h builtin_list.h

builtin_proto.h: registry$(DELIM).updated
	$(call DELFILE, .xx_builtin_proto.h)
	$(Q) touch .xx_builtin_proto.h
ifneq ($(PDATLIST),)
	$(call CATFILE, .xx_builtin_proto.h, $(PDATLIST))
endif
	$(Q) mv .xx_builtin_proto.h builtin_proto.h

context::
	$(Q) $(MAKE) -C registry context TOPDIR="$(TOPDIR)" APPDIR="$(APPDIR)"

depend:: builtin_list.h builtin_proto.h

clean::
	$(Q) $(MAKE) -C registry clean TOPDIR="$(TOPDIR)" APPDIR="$(APPDIR)"
	$(call DELFILE, .xx_builtin_list.h)
	$(call DELFILE, .xx_builtin_proto.h)

distclean::
	$(Q) $(MAKE) -C registry distclean TOPDIR="$(TOPDIR)" APPDIR="$(APPDIR)"
	$(call DELFILE, builtin_list.h)
	$(call DELFILE, builtin_proto.h)

include $(APPDIR)/Application.mk
