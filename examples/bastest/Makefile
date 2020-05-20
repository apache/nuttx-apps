############################################################################
# apps/examples/bastest/Makefile
#
#   Copyright (C) 2014 Gregory Nutt. All rights reserved.
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

include $(APPDIR)/Make.defs

# BAS test volume mounter

PROGNAME = bastest
PRIORITY = SCHED_PRIORITY_DEFAULT
STACKSIZE = $(CONFIG_DEFAULT_TASK_STACKSIZE)
MODULE = $(CONFIG_EXAMPLES_BASTEST)

# Hello, World! Example

MAINSRC = bastest_main.c

BASTEST_DIR = $(APPDIR)$(DELIM)examples$(DELIM)bastest
TESTS_DIR = $(BASTEST_DIR)$(DELIM)tests
ROMFS_IMG = romfs.img
ROMFS_HDR = romfs.h

# Common build

# Create the romfs.h header file from the tests/ directory

$(ROMFS_IMG) :
	$(Q) genromfs -f $@ -d $(TESTS_DIR) -V "BASTEST"

$(ROMFS_HDR) : $(ROMFS_IMG)
	$(Q) (xxd -i $(ROMFS_IMG) | sed -e "s/^unsigned/static const unsigned/g" >$@)

# Add the BASTEST object to the archive

context:: $(ROMFS_HDR)

distclean::
	$(call DELFILE, $(ROMFS_HDR))
	$(call DELFILE, $(ROMFS_IMG))

include $(APPDIR)/Application.mk
