############################################################################
# apps/examples/fdpicxip/modules/cxxuser/Makefile
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

# Links against libshape.so, which it names in DT_NEEDED.  The library is
# built here, so that it exists before this module is linked.

PROGNAME = cxxuser
MODULE   = m
MAINSRC  = cxxuser.cpp
CXXEXT   = .cpp

LIBDIR = ..$(DELIM)libshape
LIBARGS = APPDIR="$(APPDIR)" TOPDIR="$(TOPDIR)"

$(BINDIR)$(DELIM)cxxuser: LDLIBS += $(BINDIR)$(DELIM)libshape.so
$(BINDIR)$(DELIM)cxxuser: libshape

.PHONY: libshape

libshape:
	$(Q) $(MAKE) -C $(LIBDIR) install $(LIBARGS)

clean::
	$(Q) $(MAKE) -C $(LIBDIR) clean $(LIBARGS)

distclean::
	$(Q) $(MAKE) -C $(LIBDIR) distclean $(LIBARGS)

include $(APPDIR)/Application.mk
