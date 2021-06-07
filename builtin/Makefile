############################################################################
# apps/builtin/Makefile
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

# Source and object files

CSRCS = builtin_list.c exec_builtin.c

# Registry entry lists

PDATLIST = $(strip $(call RWILDCARD, registry, *.pdat))
BDATLIST = $(strip $(call RWILDCARD, registry, *.bdat))

builtin_list.c: builtin_list.h builtin_proto.h

registry$(DELIM).updated:
	$(Q) touch registry$(DELIM).updated

builtin_list.h: registry$(DELIM).updated
ifeq ($(BDATLIST),)
	$(call DELFILE, builtin_list.h)
	$(Q) touch builtin_list.h
else
	$(call CATFILE, builtin_list.h, $(BDATLIST))
endif

builtin_proto.h: registry$(DELIM).updated
ifeq ($(PDATLIST),)
	$(call DELFILE, builtin_proto.h)
	$(Q) touch builtin_proto.h
else
	$(call CATFILE, builtin_proto.h, $(PDATLIST))
endif

depend:: builtin_list.h builtin_proto.h

clean::
	$(call DELFILE, builtin_list.h)
	$(call DELFILE, builtin_proto.h)

clean_context::
	$(call DELFILE, $(PDATLIST))
	$(call DELFILE, $(BDATLIST))

distclean:: clean_context clean
	$(call DELFILE, registry$(DELIM).updated)

include $(APPDIR)/Application.mk
