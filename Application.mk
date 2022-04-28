############################################################################
# apps/Application.mk
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
  ifeq ($(MODULE),m)
    BUILD_MODULE = y
  endif

  ifeq ($(CONFIG_BUILD_KERNEL),y)
    BUILD_MODULE = y
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

# Add the static application library to the linked libraries. Don't do this
# with CONFIG_BUILD_KERNEL as there is no static app library
ifneq ($(CONFIG_BUILD_KERNEL),y)
  LDLIBS += $(call CONVERT_PATH,$(BIN))
endif

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

SUFFIX = $(subst $(DELIM),.,$(CWD))
PROGNAME := $(shell echo $(PROGNAME))

# Object files

RASRCS = $(filter %.s,$(ASRCS))
CASRCS = $(filter %.S,$(ASRCS))

RAOBJS = $(RASRCS:=$(SUFFIX)$(OBJEXT))
CAOBJS = $(CASRCS:=$(SUFFIX)$(OBJEXT))
COBJS = $(CSRCS:=$(SUFFIX)$(OBJEXT))
CXXOBJS = $(CXXSRCS:=$(SUFFIX)$(OBJEXT))
RUSTOBJS = $(RUSTSRCS:=$(SUFFIX)$(OBJEXT))
ZIGOBJS = $(ZIGSRCS:=$(SUFFIX)$(OBJEXT))

MAINCXXSRCS = $(filter %$(CXXEXT),$(MAINSRC))
MAINCSRCS = $(filter %.c,$(MAINSRC))
MAINRUSTSRCS = $(filter %$(RUSTEXT),$(MAINSRC))
MAINZIGSRCS = $(filter %$(ZIGEXT),$(MAINSRC))
MAINCXXOBJ = $(MAINCXXSRCS:=$(SUFFIX)$(OBJEXT))
MAINCOBJ = $(MAINCSRCS:=$(SUFFIX)$(OBJEXT))
MAINRUSTOBJ = $(MAINRUSTSRCS:=$(SUFFIX)$(OBJEXT))
MAINZIGOBJ = $(MAINZIGSRCS:=$(SUFFIX)$(OBJEXT))

SRCS = $(ASRCS) $(CSRCS) $(CXXSRCS) $(MAINSRC)
OBJS = $(RAOBJS) $(CAOBJS) $(COBJS) $(CXXOBJS) $(RUSTOBJS) $(ZIGOBJS)

ifneq ($(BUILD_MODULE),y)
  OBJS += $(MAINCOBJ) $(MAINCXXOBJ) $(MAINRUSTOBJ) $(MAINZIGOBJS)
endif

DEPPATH += --dep-path .
DEPPATH += --obj-path .

VPATH += :.

# Targets follow

all:: $(OBJS)
.PHONY: clean depend distclean
.PRECIOUS: $(BIN)

define ELFASSEMBLE
	@echo "AS: $1"
	$(Q) $(CC) -c $(AELFFLAGS) $($(strip $1)_AELFFLAGS) $1 -o $2
endef

define ELFCOMPILE
	@echo "CC: $1"
	$(Q) $(CC) -c $(CELFFLAGS) $($(strip $1)_CELFFLAGS) $1 -o $2
endef

define ELFCOMPILEXX
	@echo "CXX: $1"
	$(Q) $(CXX) -c $(CXXELFFLAGS) $($(strip $1)_CXXELFFLAGS) $1 -o $2
endef

define ELFCOMPILERUST
	@echo "RUSTC: $1"
	$(Q) $(RUSTC) --emit obj $(RUSTELFFLAGS) $($(strip $1)_RUSTELFFLAGS) $1 -o $2
endef

define ELFCOMPILEZIG
       @echo "ZIG: $1"
       $(Q) $(ZIG) build-obj $(ZIGELFFLAGS) $($(strip $1)_ZIGELFFLAGS) $1 --name $2
endef

define ELFLD
	@echo "LD: $2"
	$(Q) $(LD) $(LDELFFLAGS) $(LDLIBPATH) $(ARCHCRT0OBJ) $1 $(LDSTARTGROUP) $(LDLIBS) $(LDENDGROUP) -o $2
endef

$(RAOBJS): %.s$(SUFFIX)$(OBJEXT): %.s
	$(if $(and $(CONFIG_BUILD_LOADABLE),$(AELFFLAGS)), \
		$(call ELFASSEMBLE, $<, $@), $(call ASSEMBLE, $<, $@))

$(CAOBJS): %.S$(SUFFIX)$(OBJEXT): %.S
	$(if $(and $(CONFIG_BUILD_LOADABLE),$(AELFFLAGS)), \
		$(call ELFASSEMBLE, $<, $@), $(call ASSEMBLE, $<, $@))

$(COBJS): %.c$(SUFFIX)$(OBJEXT): %.c
	$(if $(and $(CONFIG_BUILD_LOADABLE),$(CELFFLAGS)), \
		$(call ELFCOMPILE, $<, $@), $(call COMPILE, $<, $@))

$(CXXOBJS): %$(CXXEXT)$(SUFFIX)$(OBJEXT): %$(CXXEXT)
	$(if $(and $(CONFIG_BUILD_LOADABLE),$(CXXELFFLAGS)), \
		$(call ELFCOMPILEXX, $<, $@), $(call COMPILEXX, $<, $@))

$(RUSTOBJS): %$(RUSTEXT)$(SUFFIX)$(OBJEXT): %$(RUSTEXT)
	$(if $(and $(CONFIG_BUILD_LOADABLE),$(CELFFLAGS)), \
		$(call ELFCOMPILERUST, $<, $@), $(call COMPILERUST, $<, $@))

$(ZIGOBJS): %$(ZIGEXT)$(SUFFIX)$(OBJEXT): %$(ZIGEXT)
	$(if $(and $(CONFIG_BUILD_LOADABLE), $(CELFFLAGS)), \
		$(call ELFCOMPILEZIG, $<, $@), $(call COMPILEZIG, $<, $@))

archive:
	$(call ARCHIVE_ADD, $(call CONVERT_PATH,$(BIN)), $(OBJS))

ifeq ($(BUILD_MODULE),y)

$(MAINCXXOBJ): %$(CXXEXT)$(SUFFIX)$(OBJEXT): %$(CXXEXT)
	$(if $(and $(CONFIG_BUILD_LOADABLE),$(CXXELFFLAGS)), \
		$(call ELFCOMPILEXX, $<, $@), $(call COMPILEXX, $<, $@))

$(MAINCOBJ): %.c$(SUFFIX)$(OBJEXT): %.c
	$(if $(and $(CONFIG_BUILD_LOADABLE),$(CELFFLAGS)), \
		$(call ELFCOMPILE, $<, $@), $(call COMPILE, $<, $@))

PROGLIST := $(wordlist 1,$(words $(MAINCOBJ) $(MAINCXXOBJ) $(MAINRUSTOBJ)),$(PROGNAME))
PROGLIST := $(addprefix $(BINDIR)$(DELIM),$(PROGLIST))
PROGOBJ := $(MAINCOBJ) $(MAINCXXOBJ) $(MAINRUSTOBJ)

$(PROGLIST): $(MAINCOBJ) $(MAINCXXOBJ) $(MAINRUSTOBJ)
	$(Q) mkdir -p $(BINDIR)
	$(call ELFLD,$(firstword $(PROGOBJ)),$(call CONVERT_PATH,$(firstword $(PROGLIST))))
	$(Q) chmod +x $(firstword $(PROGLIST))
ifneq ($(CONFIG_DEBUG_SYMBOLS),y)
	$(Q) $(STRIP) $(firstword $(PROGLIST))
endif
	$(eval PROGLIST=$(filter-out $(firstword $(PROGLIST)),$(PROGLIST)))
	$(eval PROGOBJ=$(filter-out $(firstword $(PROGOBJ)),$(PROGOBJ)))

install:: $(PROGLIST)

else

MAINNAME := $(addsuffix _main,$(PROGNAME))

$(MAINCXXOBJ): %$(CXXEXT)$(SUFFIX)$(OBJEXT): %$(CXXEXT)
	$(eval $<_CXXFLAGS += ${shell $(DEFINE) "$(CXX)" main=$(firstword $(MAINNAME))})
	$(eval $<_CXXELFFLAGS += ${shell $(DEFINE) "$(CXX)" main=$(firstword $(MAINNAME))})
	$(eval MAINNAME=$(filter-out $(firstword $(MAINNAME)),$(MAINNAME)))
	$(if $(and $(CONFIG_BUILD_LOADABLE),$(CXXELFFLAGS)), \
		$(call ELFCOMPILEXX, $<, $@), $(call COMPILEXX, $<, $@))

$(MAINCOBJ): %.c$(SUFFIX)$(OBJEXT): %.c
	$(eval $<_CFLAGS += ${shell $(DEFINE) "$(CC)" main=$(firstword $(MAINNAME))})
	$(eval $<_CELFFLAGS += ${shell $(DEFINE) "$(CC)" main=$(firstword $(MAINNAME))})
	$(eval MAINNAME=$(filter-out $(firstword $(MAINNAME)),$(MAINNAME)))
	$(if $(and $(CONFIG_BUILD_LOADABLE),$(CELFFLAGS)), \
		$(call ELFCOMPILE, $<, $@), $(call COMPILE, $<, $@))

$(MAINRUSTOBJ): %$(RUSTEXT)$(SUFFIX)$(OBJEXT): %$(RUSTEXT)
	$(if $(and $(CONFIG_BUILD_LOADABLE),$(CELFFLAGS)), \
		$(call ELFCOMPILERUST, $<, $@), $(call COMPILERUST, $<, $@))

$(MAINZIGOBJ): %$(ZIGEXT)$(SUFFIX)$(OBJEXT): %$(ZIGEXT)
	$(if $(and $(CONFIG_BUILD_LOADABLE),$(CELFFLAGS)), \
		$(call ELFCOMPILEZIG, $<, $@), $(call COMPILEZIG, $<, $@))

install::

endif # BUILD_MODULE

context::

ifneq ($(PROGNAME),)

REGLIST := $(addprefix $(BUILTIN_REGISTRY)$(DELIM),$(addsuffix .bdat,$(PROGNAME)))
APPLIST := $(PROGNAME)

$(REGLIST): $(DEPCONFIG) Makefile
	$(call REGISTER,$(firstword $(APPLIST)),$(firstword $(PRIORITY)),$(firstword $(STACKSIZE)),$(if $(BUILD_MODULE),,$(firstword $(APPLIST))_main))
	$(eval APPLIST=$(filter-out $(firstword $(APPLIST)),$(APPLIST)))
	$(if $(filter-out $(firstword $(PRIORITY)),$(PRIORITY)),$(eval PRIORITY=$(filter-out $(firstword $(PRIORITY)),$(PRIORITY))))
	$(if $(filter-out $(firstword $(STACKSIZE)),$(STACKSIZE)),$(eval STACKSIZE=$(filter-out $(firstword $(STACKSIZE)),$(STACKSIZE))))

register:: $(REGLIST)
else
register::
endif

.depend: Makefile $(wildcard $(foreach SRC, $(SRCS), $(addsuffix /$(SRC), $(subst :, ,$(VPATH))))) $(DEPCONFIG)
	$(Q) $(MKDEP) $(DEPPATH) --obj-suffix .c$(SUFFIX)$(OBJEXT) "$(CC)" -- $(CFLAGS) -- $(filter %.c,$^) >Make.dep
	$(Q) $(MKDEP) $(DEPPATH) --obj-suffix .S$(SUFFIX)$(OBJEXT) "$(CC)" -- $(CFLAGS) -- $(filter %.S,$^) >>Make.dep
	$(Q) $(MKDEP) $(DEPPATH) --obj-suffix $(CXXEXT)$(SUFFIX)$(OBJEXT) "$(CXX)" -- $(CXXFLAGS) -- $(filter %$(CXXEXT),$^) >>Make.dep
	$(Q) touch $@

depend:: .depend

clean::
	$(call CLEAN)

distclean:: clean
	$(call DELFILE, Make.dep)
	$(call DELFILE, .depend)

-include Make.dep
