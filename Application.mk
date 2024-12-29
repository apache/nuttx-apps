############################################################################
# apps/Application.mk
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
#   http:#www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations
# under the License.
#
############################################################################

# If this is an executable program (with MAINSRC), we must build it as a
# loadable module for the KERNEL build (always) or if the tristate module
# has the value "m"

ifneq ($(MAINSRC),)
  ifeq ($(DYNLIB),y)
    BUILD_MODULE = y
    MODAELFFLAGS = $(CMODULELFFLAGS)
    MODCFLAGS = $(CMODULEFLAGS)
    MODCXXFLAGS = $(CXXMODULEFLAGS)
    MODLDFLAGS = $(LDMODULEFLAGS)
  else ifneq ($(or $(filter y,$(CONFIG_BUILD_KERNEL)), $(filter m,$(MODULE))),)
      BUILD_MODULE = y
      MODAELFFLAGS = $(AELFFLAGS)
      MODCFLAGS = $(CELFFLAGS)
      MODCXXFLAGS = $(CXXELFFLAGS)
      MODLDFLAGS = $(LDELFFLAGS)
  endif
endif

# The GNU make CURDIR will always be a POSIX-like path with forward slashes
# as path segment separators.  If we know that this is a native build, then
# we need to fix up the path so the DELIM will match the actual delimiter.

ifeq ($(CONFIG_WINDOWS_NATIVE),y)
  CWD = $(strip ${shell echo %CD% | cut -d: -f2})
else
  CWD = $(CURDIR)
endif

SUFFIX ?= $(subst $(DELIM),.,$(CWD))

PROGNAME := $(subst ",,$(PROGNAME))

# Add the static application library to the linked libraries.

LDLIBS += $(call CONVERT_PATH,$(BIN))

# When building a module, link with the compiler runtime.
# This should be linked after libapps. Consider that mbedtls in libapps
# uses __udivdi3.
ifeq ($(BUILD_MODULE),y)
  # Revisit: This only works for gcc and clang.
  # Do other compilers have similar?
  COMPILER_RT_LIB = $(shell $(CC) $(ARCHCPUFLAGS) --print-libgcc-file-name)
  ifeq ($(wildcard $(COMPILER_RT_LIB)),)
    # if "--print-libgcc-file-name" unable to find the correct libgcc PATH
    # then go ahead and try "--print-file-name"
    COMPILER_RT_LIB := $(wildcard $(shell $(CC) $(ARCHCPUFLAGS) --print-file-name $(notdir $(COMPILER_RT_LIB))))
  endif
  LDLIBS += $(COMPILER_RT_LIB)
endif

# Apps compilation can achieve out-of-tree intermediate products
# by specifying "PREFIX" to a directory in its own Makefile.
# Default value is NULL,
# Make sure the out-of-tree directory exists and ends with $(DELIM) when setting it.

PREFIX ?=

# Object files

RASRCS = $(filter %.s,$(ASRCS))
CASRCS = $(filter %.S,$(ASRCS))

RAOBJS = $(RASRCS:%=$(PREFIX)%$(SUFFIX)$(OBJEXT))
CAOBJS = $(CASRCS:%=$(PREFIX)%$(SUFFIX)$(OBJEXT))
COBJS = $(CSRCS:%=$(PREFIX)%$(SUFFIX)$(OBJEXT))
CXXOBJS = $(CXXSRCS:%=$(PREFIX)%$(SUFFIX)$(OBJEXT))
RUSTOBJS = $(RUSTSRCS:%=$(PREFIX)%$(SUFFIX)$(OBJEXT))
ZIGOBJS = $(ZIGSRCS:%=$(PREFIX)%$(SUFFIX)$(OBJEXT))
DOBJS = $(DSRCS:%=$(PREFIX)%$(SUFFIX)$(OBJEXT))
SWIFTOBJS = $(SWIFTSRCS:%=$(PREFIX)%$(SUFFIX)$(OBJEXT))

MAINCXXSRCS = $(filter %$(CXXEXT),$(MAINSRC))
MAINCSRCS = $(filter %.c,$(MAINSRC))
MAINRUSTSRCS = $(filter %$(RUSTEXT),$(MAINSRC))
MAINZIGSRCS = $(filter %$(ZIGEXT),$(MAINSRC))
MAINDSRCS = $(filter %$(DEXT),$(MAINSRC))
MAINSWIFTSRCS = $(filter %$(SWIFTEXT),$(MAINSRC))
MAINCXXOBJ = $(MAINCXXSRCS:%=$(PREFIX)%$(SUFFIX)$(OBJEXT))
MAINCOBJ = $(MAINCSRCS:%=$(PREFIX)%$(SUFFIX)$(OBJEXT))
MAINRUSTOBJ = $(MAINRUSTSRCS:%=$(PREFIX)%$(SUFFIX)$(OBJEXT))
MAINZIGOBJ = $(MAINZIGSRCS:%=$(PREFIX)%$(SUFFIX)$(OBJEXT))
MAINDOBJ = $(MAINDSRCS:%=$(PREFIX)%$(SUFFIX)$(OBJEXT))
MAINSWIFTOBJ = $(MAINSWIFTSRCS:%=$(PREFIX)%$(SUFFIX)$(OBJEXT))

SRCS = $(ASRCS) $(CSRCS) $(CXXSRCS) $(MAINSRC)
OBJS = $(RAOBJS) $(CAOBJS) $(COBJS) $(CXXOBJS) $(RUSTOBJS) $(ZIGOBJS) $(DOBJS) $(SWIFTOBJS) $(EXTOBJS)

ifneq ($(BUILD_MODULE),y)
  OBJS += $(MAINCOBJ) $(MAINCXXOBJ) $(MAINRUSTOBJ) $(MAINZIGOBJ) $(MAINDOBJ) $(MAINSWIFTOBJ)
endif

ifneq ($(strip $(PROGNAME)),)
  PROGOBJ := $(MAINCOBJ) $(MAINCXXOBJ) $(MAINRUSTOBJ) $(MAINZIGOBJ) $(MAINDOBJ) $(MAINSWIFTOBJ)
  PROGLIST := $(addprefix $(BINDIR)$(DELIM),$(PROGNAME))
  REGLIST := $(addprefix $(BUILTIN_REGISTRY)$(DELIM),$(addsuffix .bdat,$(PROGNAME)))

  NLIST := $(shell seq 1 $(words $(PROGNAME)))
  $(foreach i, $(NLIST), \
    $(eval PROGNAME_$(word $i,$(PROGOBJ)) := $(word $i,$(PROGNAME))) \
    $(eval PROGOBJ_$(word $i,$(PROGLIST)) := $(word $i,$(PROGOBJ))) \
    $(eval PRIORITY_$(word $i,$(REGLIST)) := \
        $(if $(word $i,$(PRIORITY)),$(word $i,$(PRIORITY)),$(lastword $(PRIORITY)))) \
    $(eval STACKSIZE_$(word $i,$(REGLIST)) := \
        $(if $(word $i,$(STACKSIZE)),$(word $i,$(STACKSIZE)),$(lastword $(STACKSIZE)))) \
    $(eval UID_$(word $i,$(REGLIST)) := \
        $(if $(word $i,$(UID)),$(word $i,$(UID)),$(lastword $(UID)))) \
    $(eval GID_$(word $i,$(REGLIST)) := \
        $(if $(word $i,$(GID)),$(word $i,$(GID)),$(lastword $(GID)))) \
    $(eval MODE_$(word $i,$(REGLIST)) := \
        $(if $(word $i,$(MODE)),$(word $i,$(MODE)),$(lastword $(MODE)))) \
  )
endif

# Condition flags

DO_REGISTRATION ?= y

ifeq ($(PROGNAME),)
  DO_REGISTRATION = n
endif

ifeq ($(WASM_BUILD),y)
  DO_REGISTRATION = n
endif

ifeq ($(DYNLIB),y)
  DO_REGISTRATION = n
endif

# Compile flags, notice the default flags only suitable for flat build

ZIGELFFLAGS ?= $(ZIGFLAGS)
RUSTELFFLAGS ?= $(RUSTFLAGS)
DELFFLAGS ?= $(DFLAGS)
SWIFTELFFLAGS ?= $(SWIFTFLAGS)

DEPPATH += --dep-path .
DEPPATH += --obj-path .

VPATH += :.

# Targets follow

all:: $(PREFIX).built
	@:
.PHONY: clean depend distclean
.PRECIOUS: $(BIN)

define ELFASSEMBLE
	$(ECHO_BEGIN)"AS: $1 "
	$(Q) $(MODULECC) -c $(MODAELFFLAGS) $($(strip $1)_AELFFLAGS) $1 -o $2
	$(ECHO_END)
endef

define ELFCOMPILE
	$(ECHO_BEGIN)"CC: $1 "
	$(Q) $(MODULECC) -c $(MODCFLAGS) $($(strip $1)_CELFFLAGS) $1 -o $2
	$(ECHO_END)
endef

define ELFCOMPILEXX
	$(ECHO_BEGIN)"CXX: $1 "
	$(Q) $(CXX) -c $(MODCXXFLAGS) $($(strip $1)_CXXELFFLAGS) $1 -o $2
	$(ECHO_END)
endef

define ELFCOMPILERUST
	$(ECHO_BEGIN)"RUSTC: $1 "
	$(Q) $(RUSTC) --emit obj $(RUSTELFFLAGS) $($(strip $1)_RUSTELFFLAGS) $1 -o $2
	$(ECHO_END)
endef

# Remove target suffix here since zig compiler add .o automatically
define ELFCOMPILEZIG
	$(ECHO_BEGIN)"ZIG: $1 "
	$(Q) $(ZIG) build-obj $(ZIGELFFLAGS) $($(strip $1)_ZIGELFFLAGS) --name $(basename $2) $1
	$(ECHO_END)
endef

define ELFCOMPILED
	$(ECHO_BEGIN)"DC: $1 "
	$(Q) $(DC) -c $(DELFFLAGS) $($(strip $1)_DELFFLAGS) $1 -of $2
	$(ECHO_END)
endef

define ELFCOMPILESWIFT
	$(ECHO_BEGIN)"SWIFTC: $1 "
	$(Q) $(SWIFTC) -c $(SWIFTELFFLAGS) $($(strip $1)_SWIFTELFFLAGS) $1 -o $2
	$(ECHO_END)
endef

define ELFLD
	$(ECHO_BEGIN)"LD: $2 "
	$(Q) $(MODULELD) $(MODLDFLAGS) $(LDMAP) $(LDLIBPATH) $(ARCHCRT0OBJ) $1 $(LDSTARTGROUP) $(LDLIBS) $(LDENDGROUP) -o $2
	$(ECHO_END)
endef

# rename "main()" in $1 to "xxx_main()" and save to $2
define RENAMEMAIN
	$(ECHO_BEGIN)"Rename main() in $1 and save to $2"
	$(Q) ${shell cat $1 | sed -e "s/fn[ ]\+main/fn $(addsuffix _main,$(PROGNAME_$@))/" > $2}
	$(ECHO_END)
endef

$(RAOBJS): $(PREFIX)%.s$(SUFFIX)$(OBJEXT): %.s
	$(if $(and $(CONFIG_MODULES),$(MODAELFFLAGS)), \
		$(call ELFASSEMBLE, $<, $@), $(call ASSEMBLE, $<, $@))

$(CAOBJS): $(PREFIX)%.S$(SUFFIX)$(OBJEXT): %.S
	$(if $(and $(CONFIG_MODULES),$(MODAELFFLAGS)), \
		$(call ELFASSEMBLE, $<, $@), $(call ASSEMBLE, $<, $@))

$(COBJS): $(PREFIX)%.c$(SUFFIX)$(OBJEXT): %.c
	$(if $(and $(CONFIG_MODULES),$(MODCFLAGS)), \
		$(call ELFCOMPILE, $<, $@), $(call COMPILE, $<, $@))

$(CXXOBJS): $(PREFIX)%$(CXXEXT)$(SUFFIX)$(OBJEXT): %$(CXXEXT)
	$(if $(and $(CONFIG_MODULES),$(MODCXXFLAGS)), \
		$(call ELFCOMPILEXX, $<, $@), $(call COMPILEXX, $<, $@))

$(RUSTOBJS): $(PREFIX)%$(RUSTEXT)$(SUFFIX)$(OBJEXT): %$(RUSTEXT)
	$(if $(and $(CONFIG_MODULES),$(CELFFLAGS)), \
		$(call ELFCOMPILERUST, $<, $@), $(call COMPILERUST, $<, $@))

$(ZIGOBJS): $(PREFIX)%$(ZIGEXT)$(SUFFIX)$(OBJEXT): %$(ZIGEXT)
	$(if $(and $(CONFIG_MODULES), $(CELFFLAGS)), \
		$(call ELFCOMPILEZIG, $<, $@), $(call COMPILEZIG, $<, $@))

$(DOBJS): $(PREFIX)%$(DEXT)$(SUFFIX)$(OBJEXT): %$(DEXT)
	$(if $(and $(CONFIG_MODULES), $(CELFFLAGS)), \
		$(call ELFCOMPILED, $<, $@), $(call COMPILED, $<, $@))

$(SWIFTOBJS): $(PREFIX)%$(SWIFTEXT)$(SUFFIX)$(OBJEXT): %$(SWIFTEXT)
	$(if $(and $(CONFIG_MODULES), $(CELFFLAGS)), \
		$(call ELFCOMPILESWIFT, $<, $@), $(call COMPILESWIFT, $<, $@))

AROBJS :=
ifneq ($(OBJS),)
SORTOBJS := $(sort $(OBJS))
$(eval $(call SPLITVARIABLE,OBJS_SPILT,$(SORTOBJS),100))
$(foreach BATCH, $(OBJS_SPILT_TOTAL), \
	$(foreach obj, $(OBJS_SPILT_$(BATCH)), \
		$(eval substitute := $(patsubst %$(OBJEXT),%_$(BATCH)$(OBJEXT),$(obj))) \
		$(eval AROBJS += $(substitute)) \
		$(eval $(call AROBJSRULES, $(substitute),$(obj))) \
	) \
)
endif

$(PREFIX).built: $(AROBJS)
	$(call SPLITVARIABLE,ALL_OBJS,$(AROBJS),100)
	$(foreach BATCH, $(ALL_OBJS_TOTAL), \
		$(if $(strip $(ALL_OBJS_$(BATCH))), \
			$(shell $(call ARLOCK, $(call CONVERT_PATH,$(BIN)), $(ALL_OBJS_$(BATCH)))) \
		) \
	)
	$(Q) touch $@

ifeq ($(BUILD_MODULE),y)

$(MAINCXXOBJ): $(PREFIX)%$(CXXEXT)$(SUFFIX)$(OBJEXT): %$(CXXEXT)
	$(if $(and $(CONFIG_MODULES),$(MODCXXFLAGS)), \
		$(call ELFCOMPILEXX, $<, $@), $(call COMPILEXX, $<, $@))

$(MAINCOBJ): $(PREFIX)%.c$(SUFFIX)$(OBJEXT): %.c
	$(if $(and $(CONFIG_MODULES),$(MODCFLAGS)), \
		$(call ELFCOMPILE, $<, $@), $(call COMPILE, $<, $@))

$(MAINZIGOBJ): $(PREFIX)%$(ZIGEXT)$(SUFFIX)$(OBJEXT): %$(ZIGEXT)
	$(if $(and $(CONFIG_MODULES),$(CELFFLAGS)), \
		$(call ELFCOMPILEZIG, $<, $@), $(call COMPILEZIG, $<, $@))

$(MAINDOBJ): $(PREFIX)%$(DEXT)$(SUFFIX)$(OBJEXT): %$(DEXT)
	$(if $(and $(CONFIG_MODULES),$(CELFFLAGS)), \
		$(call ELFCOMPILED, $<, $@), $(call COMPILED, $<, $@))

$(MAINSWIFTOBJ): $(PREFIX)%$(SWIFTEXT)$(SUFFIX)$(OBJEXT): %$(SWIFTEXT)
	$(if $(and $(CONFIG_MODULES),$(CELFFLAGS)), \
		$(call ELFCOMPILESWIFT, $<, $@), $(call COMPILESWIFT, $<, $@))

$(PROGLIST): $(MAINCOBJ) $(MAINCXXOBJ) $(MAINRUSTOBJ) $(MAINZIGOBJ) $(MAINDOBJ) $(MAINSWIFTOBJ)
	$(Q) mkdir -p $(BINDIR)
	$(call ELFLD, $(PROGOBJ_$@), $(call CONVERT_PATH,$@))
	$(Q) chmod +x $@
ifneq ($(CONFIG_DEBUG_SYMBOLS),)
	$(Q) mkdir -p $(BINDIR_DEBUG)
	$(Q) cp $@ $(BINDIR_DEBUG)
	$(Q) $(MODULESTRIP) $@
endif

install:: $(PROGLIST)
	@:

else

$(MAINCXXOBJ): $(PREFIX)%$(CXXEXT)$(SUFFIX)$(OBJEXT): %$(CXXEXT)
	$(eval $<_CXXFLAGS += ${shell $(DEFINE) "$(CXX)" main=$(addsuffix _main,$(PROGNAME_$@))})
	$(eval $<_CXXELFFLAGS += ${shell $(DEFINE) "$(CXX)" main=$(addsuffix _main,$(PROGNAME_$@))})
	$(if $(and $(CONFIG_MODULES),$(MODCXXFLAGS)), \
		$(call ELFCOMPILEXX, $<, $@), $(call COMPILEXX, $<, $@))

$(MAINCOBJ): $(PREFIX)%.c$(SUFFIX)$(OBJEXT): %.c
	$(eval $<_CFLAGS += ${DEFINE_PREFIX}main=$(addsuffix _main,$(PROGNAME_$@)))
	$(eval $<_CELFFLAGS += ${DEFINE_PREFIX}main=$(addsuffix _main,$(PROGNAME_$@)))
	$(if $(and $(CONFIG_MODULES),$(MODCFLAGS)), \
		$(call ELFCOMPILE, $<, $@), $(call COMPILE, $<, $@))

$(MAINRUSTOBJ): $(PREFIX)%$(RUSTEXT)$(SUFFIX)$(OBJEXT): %$(RUSTEXT)
	$(if $(and $(CONFIG_MODULES),$(CELFFLAGS)), \
		$(call ELFCOMPILERUST, $<, $@), $(call COMPILERUST, $<, $@))

$(MAINZIGOBJ): $(PREFIX)%$(ZIGEXT)$(SUFFIX)$(OBJEXT): %$(ZIGEXT)
	$(Q) $(call RENAMEMAIN, $<, $(basename $<)_tmp.zig)
	$(if $(and $(CONFIG_MODULES),$(CELFFLAGS)), \
			$(call ELFCOMPILEZIG, $(basename $<)_tmp.zig, $@), $(call COMPILEZIG, $(basename $<)_tmp.zig, $@))
	$(Q) rm -f $(basename $<)_tmp.zig

$(MAINDOBJ): $(PREFIX)%$(DEXT)$(SUFFIX)$(OBJEXT): %$(DEXT)
	$(if $(and $(CONFIG_MODULES),$(CELFFLAGS)), \
		$(call ELFCOMPILED, $<, $@), $(call COMPILED, $<, $@))

$(MAINSWIFTOBJ): $(PREFIX)%$(SWIFTEXT)$(SUFFIX)$(OBJEXT): %$(SWIFTEXT)
	$(if $(and $(CONFIG_MODULES),$(CELFFLAGS)), \
		$(call ELFCOMPILESWIFT, $<, $@), $(call COMPILESWIFT, $<, $@))

install::
	@:

endif # BUILD_MODULE

postinstall::
	@:

context::

ifeq ($(DO_REGISTRATION),y)

$(REGLIST): $(DEPCONFIG) Makefile
	$(eval PROGNAME_$@ := $(basename $(notdir $@)))
ifeq ($(CONFIG_SCHED_USER_IDENTITY),y)
	$(call REGISTER,$(PROGNAME_$@),$(PRIORITY_$@),$(STACKSIZE_$@),$(if $(BUILD_MODULE),,$(PROGNAME_$@)_main),$(UID_$@),$(GID_$@),$(MODE_$@))
else
	$(call REGISTER,$(PROGNAME_$@),$(PRIORITY_$@),$(STACKSIZE_$@),$(if $(BUILD_MODULE),,$(PROGNAME_$@)_main))
endif

register:: $(REGLIST)
	@:
else
register::
	@:
endif

$(PREFIX).depend: Makefile $(wildcard $(foreach SRC, $(SRCS), $(addsuffix /$(SRC), $(subst :, ,$(VPATH))))) $(DEPCONFIG)
	$(shell echo "# Gen Make.dep automatically" >$(PREFIX)Make.dep)
	$(call SPLITVARIABLE,ALL_DEP_OBJS,$^,100)
	$(foreach BATCH, $(ALL_DEP_OBJS_TOTAL), \
	  $(shell $(MKDEP) $(DEPPATH) --obj-suffix .c$(SUFFIX)$(OBJEXT) "$(CC)" -- $(CFLAGS) -- $(filter %.c,$(ALL_DEP_OBJS_$(BATCH))) >>$(PREFIX)Make.dep) \
	  $(shell $(MKDEP) $(DEPPATH) --obj-suffix .S$(SUFFIX)$(OBJEXT) "$(CC)" -- $(CFLAGS) -- $(filter %.S,$(ALL_DEP_OBJS_$(BATCH))) >>$(PREFIX)Make.dep) \
	  $(shell $(MKDEP) $(DEPPATH) --obj-suffix $(CXXEXT)$(SUFFIX)$(OBJEXT) "$(CXX)" -- $(CXXFLAGS) -- $(filter %$(CXXEXT),$(ALL_DEP_OBJS_$(BATCH))) >>$(PREFIX)Make.dep) \
	)
	$(Q) touch $@

depend:: $(PREFIX).depend
	@:

clean::
	$(call DELFILE, .built)
	$(call CLEANAROBJS)
	$(call CLEAN)
distclean:: clean
	$(call DELFILE, Make.dep)
	$(call DELFILE, .depend)

-include $(PREFIX)Make.dep

# Include Wasm specific definitions
include $(APPDIR)/tools/Wasm.mk
