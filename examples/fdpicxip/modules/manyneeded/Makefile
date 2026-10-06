############################################################################
# apps/examples/fdpicxip/modules/manyneeded/Makefile
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.  The
# ASF licenses this file to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance with the
# License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations
# under the License.
#
############################################################################

include $(APPDIR)/Make.defs

# Names one library more in DT_NEEDED than CONFIG_LIBC_ELF_MAXDEPEND allows.
# The libraries are stubs from ../need, and only their names matter.

PROGNAME = manyneeded
MODULE   = m
MAINSRC  = manyneeded.c

NEEDDIR = ..$(DELIM)need
NEEDARGS = APPDIR="$(APPDIR)" TOPDIR="$(TOPDIR)"
NEEDNUMS = $(shell seq 0 $(CONFIG_LIBC_ELF_MAXDEPEND))
NEEDLIBS = $(foreach n,$(NEEDNUMS),$(BINDIR)$(DELIM)need$(n).so)

$(BINDIR)$(DELIM)manyneeded: LDLIBS += --no-as-needed $(NEEDLIBS)
$(BINDIR)$(DELIM)manyneeded: need

.PHONY: need

need:
	$(Q) for n in $(NEEDNUMS); do \
	       $(MAKE) -C $(NEEDDIR) install NEED=$$n $(NEEDARGS) || exit 1; \
	     done

clean::
	$(Q) $(MAKE) -C $(NEEDDIR) clean $(NEEDARGS)

distclean::
	$(Q) $(MAKE) -C $(NEEDDIR) distclean $(NEEDARGS)

include $(APPDIR)/Application.mk
