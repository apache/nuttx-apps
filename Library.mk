############################################################################
# apps/Library.mk
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

# Library.mk is the NuttX equivalent of Android's BUILD_SHARED_LIBRARY.  A
# component Makefile includes Make.defs, declares LOCAL_MODULE and
# LOCAL_SRC_FILES, then includes this file.  NuttX invokes each component in
# a separate recursive make, so module variables cannot leak between
# components and an equivalent of Android's CLEAR_VARS is not needed.

LOCAL_PATH ?= $(CURDIR)

ifeq ($(strip $(LOCAL_MODULE)),)
  $(error LOCAL_MODULE must name the shared library)
endif

ifeq ($(strip $(LOCAL_SRC_FILES)),)
  $(error LOCAL_SRC_FILES must list at least one C or C++ source file)
endif

LOCAL_C_SRCS   := $(filter %.c,$(LOCAL_SRC_FILES))
LOCAL_CC_SRCS  := $(filter %.cc,$(LOCAL_SRC_FILES))
LOCAL_CPP_SRCS := $(filter %.cpp,$(LOCAL_SRC_FILES))
LOCAL_CXX_SRCS := $(filter %.cxx,$(LOCAL_SRC_FILES))
LOCAL_SRCS     := $(LOCAL_C_SRCS) $(LOCAL_CC_SRCS) $(LOCAL_CPP_SRCS) \
                  $(LOCAL_CXX_SRCS)

ifneq ($(words $(LOCAL_SRCS)),$(words $(LOCAL_SRC_FILES)))
  $(error LOCAL_SRC_FILES currently supports only .c, .cc, .cpp, and .cxx)
endif

# The GNU make CURDIR always uses forward slashes.  Convert it for native
# Windows builds before deriving the suffix used for intermediate objects.

ifeq ($(CONFIG_WINDOWS_NATIVE),y)
  LOCAL_CWD := $(strip ${shell echo %CD% | cut -d: -f2})
else
  LOCAL_CWD := $(CURDIR)
endif

LOCAL_SUFFIX := $(subst $(DELIM),.,$(LOCAL_CWD))
LOCAL_PREFIX ?=

LOCAL_C_OBJS   := $(LOCAL_C_SRCS:%=$(LOCAL_PREFIX)%$(LOCAL_SUFFIX)$(OBJEXT))
LOCAL_CC_OBJS  := $(LOCAL_CC_SRCS:%=$(LOCAL_PREFIX)%$(LOCAL_SUFFIX)$(OBJEXT))
LOCAL_CPP_OBJS := $(LOCAL_CPP_SRCS:%=$(LOCAL_PREFIX)%$(LOCAL_SUFFIX)$(OBJEXT))
LOCAL_CXX_OBJS := $(LOCAL_CXX_SRCS:%=$(LOCAL_PREFIX)%$(LOCAL_SUFFIX)$(OBJEXT))
LOCAL_OBJS     := $(LOCAL_C_OBJS) $(LOCAL_CC_OBJS) $(LOCAL_CPP_OBJS) \
                  $(LOCAL_CXX_OBJS)

# Keep the historical NuttX module filename by default.  A component may set
# LOCAL_MODULE_FILENAME (for example, libexample.so) when a suffix is part of
# its external interface.

LOCAL_MODULE_FILENAME ?= $(LOCAL_MODULE)
LOCAL_MODULE_PATH     := $(BINDIR)$(DELIM)$(LOCAL_MODULE_FILENAME)

LOCAL_CFLAGS   ?=
LOCAL_CXXFLAGS ?=
LOCAL_LDFLAGS  ?=
LOCAL_LDLIBS   ?=

LOCAL_COMPILER_RT_LIB := $(call FIND_COMPILER_RT_LIB)

DEPPATH += --dep-path $(LOCAL_PATH)
DEPPATH += --obj-path .

VPATH += :$(LOCAL_PATH)

.PHONY: all clean context depend distclean install postinstall register

all:: $(LOCAL_PREFIX).built
	@:

define LOCAL_COMPILE_C
	$(ECHO_BEGIN)"CC: $1 "
	$(Q) $(MODULECC) -c $(CMODULEFLAGS) $(LOCAL_CFLAGS) \
		$($(strip $1)_CELFFLAGS) $1 -o $2
	$(ECHO_END)
endef

define LOCAL_COMPILE_CXX
	$(ECHO_BEGIN)"CXX: $1 "
	$(Q) $(CXX) -c $(CXXMODULEFLAGS) $(LOCAL_CXXFLAGS) \
		$($(strip $1)_CXXELFFLAGS) $1 -o $2
	$(ECHO_END)
endef

$(LOCAL_C_OBJS): $(LOCAL_PREFIX)%.c$(LOCAL_SUFFIX)$(OBJEXT): %.c
	$(call LOCAL_COMPILE_C,$<,$@)

$(LOCAL_CC_OBJS): $(LOCAL_PREFIX)%.cc$(LOCAL_SUFFIX)$(OBJEXT): %.cc
	$(call LOCAL_COMPILE_CXX,$<,$@)

$(LOCAL_CPP_OBJS): $(LOCAL_PREFIX)%.cpp$(LOCAL_SUFFIX)$(OBJEXT): %.cpp
	$(call LOCAL_COMPILE_CXX,$<,$@)

$(LOCAL_CXX_OBJS): $(LOCAL_PREFIX)%.cxx$(LOCAL_SUFFIX)$(OBJEXT): %.cxx
	$(call LOCAL_COMPILE_CXX,$<,$@)

$(LOCAL_PREFIX).built: $(LOCAL_OBJS)
	$(Q) touch $@

$(LOCAL_MODULE_PATH): $(LOCAL_OBJS)
	$(ECHO_BEGIN)"LD: $@ "
	$(Q) mkdir -p $(BINDIR)
	$(Q) $(MODULELD) $(LDMODULEFLAGS) $(LDMAP) $(LDLIBPATH) \
		$(LOCAL_LDFLAGS) $^ $(LDSTARTGROUP) $(LOCAL_LDLIBS) \
		$(LOCAL_COMPILER_RT_LIB) $(LDENDGROUP) -o \
		$(call CONVERT_PATH,$@)
	$(ECHO_END)
ifneq ($(CONFIG_DEBUG_SYMBOLS),)
	$(Q) mkdir -p $(BINDIR_DEBUG)
	$(Q) cp $@ $(BINDIR_DEBUG)
	$(Q) $(MODULESTRIP) $(NX_KEEP) $@
endif

install:: $(LOCAL_MODULE_PATH)
	@:

context::
	@:

register::
	@:

postinstall::
	@:

$(LOCAL_PREFIX).depend: Makefile $(LOCAL_SRC_FILES) $(DEPCONFIG)
	$(shell echo "# Gen Make.dep automatically" >$(LOCAL_PREFIX)Make.dep)
	$(if $(LOCAL_C_SRCS), \
	  $(shell $(MKDEP) $(DEPPATH) \
	    --obj-suffix .c$(LOCAL_SUFFIX)$(OBJEXT) "$(MODULECC)" -- \
	    $(CMODULEFLAGS) $(LOCAL_CFLAGS) -- $(LOCAL_C_SRCS) \
	    >>$(LOCAL_PREFIX)Make.dep))
	$(if $(LOCAL_CC_SRCS), \
	  $(shell $(MKDEP) $(DEPPATH) \
	    --obj-suffix .cc$(LOCAL_SUFFIX)$(OBJEXT) "$(CXX)" -- \
	    $(CXXMODULEFLAGS) $(LOCAL_CXXFLAGS) -- $(LOCAL_CC_SRCS) \
	    >>$(LOCAL_PREFIX)Make.dep))
	$(if $(LOCAL_CPP_SRCS), \
	  $(shell $(MKDEP) $(DEPPATH) \
	    --obj-suffix .cpp$(LOCAL_SUFFIX)$(OBJEXT) "$(CXX)" -- \
	    $(CXXMODULEFLAGS) $(LOCAL_CXXFLAGS) -- $(LOCAL_CPP_SRCS) \
	    >>$(LOCAL_PREFIX)Make.dep))
	$(if $(LOCAL_CXX_SRCS), \
	  $(shell $(MKDEP) $(DEPPATH) \
	    --obj-suffix .cxx$(LOCAL_SUFFIX)$(OBJEXT) "$(CXX)" -- \
	    $(CXXMODULEFLAGS) $(LOCAL_CXXFLAGS) -- $(LOCAL_CXX_SRCS) \
	    >>$(LOCAL_PREFIX)Make.dep))
	$(Q) touch $@

depend:: $(LOCAL_PREFIX).depend
	@:

clean::
	$(call DELFILE,$(LOCAL_PREFIX).built)
	$(call DELFILE,$(LOCAL_OBJS))
	$(call DELFILE,$(LOCAL_MODULE_PATH))
	$(call DELFILE,$(BINDIR_DEBUG)$(DELIM)$(LOCAL_MODULE_FILENAME))

distclean:: clean
	$(call DELFILE,$(LOCAL_PREFIX)Make.dep)
	$(call DELFILE,$(LOCAL_PREFIX).depend)

-include $(LOCAL_PREFIX)Make.dep
