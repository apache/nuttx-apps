############################################################################
# apps/Application.mk
#
#   Copyright (C) 2015 Gregory Nutt. All rights reserved.
#   Copyright (C) 2015 Omni Hoverboards Inc. All rights reserved.
#   Authors: Gregory Nutt <gnutt@nuttx.org>
#            Paul Alexander Patience <paul-a.patience@polymtl.ca>
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

ifeq ($(CONFIG_CYGWIN_WINTOOL),y)
  LDLIBS += "${shell cygpath -w $(BIN)}"
else
  LDLIBS += $(BIN)
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

MAINCXXSRCS = $(filter %$(CXXEXT),$(MAINSRC))
MAINCSRCS = $(filter %.c,$(MAINSRC))
MAINCXXOBJ = $(MAINCXXSRCS:=$(SUFFIX)$(OBJEXT))
MAINCOBJ = $(MAINCSRCS:=$(SUFFIX)$(OBJEXT))

SRCS = $(ASRCS) $(CSRCS) $(CXXSRCS) $(MAINSRC)
OBJS = $(RAOBJS) $(CAOBJS) $(COBJS) $(CXXOBJS)

ifneq ($(BUILD_MODULE),y)
  OBJS += $(MAINCOBJ) $(MAINCXXOBJ)
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

define ELFLD
	@echo "LD: $2"
	$(Q) $(LD) $(LDELFFLAGS) $(LDLIBPATH) $(ARCHCRT0OBJ) $1 $(LDLIBS) -o $2
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

archive:
ifeq ($(CONFIG_CYGWIN_WINTOOL),y)
	$(call ARCHIVE_ADD, "${shell cygpath -w $(BIN)}", $(OBJS))
else
	$(call ARCHIVE_ADD, $(BIN), $(OBJS))
endif

ifeq ($(BUILD_MODULE),y)

$(MAINCXXOBJ): %$(CXXEXT)$(SUFFIX)$(OBJEXT): %$(CXXEXT)
	$(if $(and $(CONFIG_BUILD_LOADABLE),$(CXXELFFLAGS)), \
		$(call ELFCOMPILEXX, $<, $@), $(call COMPILEXX, $<, $@))

$(MAINCOBJ): %.c$(SUFFIX)$(OBJEXT): %.c
	$(if $(and $(CONFIG_BUILD_LOADABLE),$(CELFFLAGS)), \
		$(call ELFCOMPILE, $<, $@), $(call COMPILE, $<, $@))

PROGLIST := $(wordlist 1,$(words $(MAINCOBJ) $(MAINCXXOBJ)),$(PROGNAME))
PROGLIST := $(addprefix $(BINDIR)$(DELIM),$(PROGLIST))
PROGOBJ := $(MAINCOBJ) $(MAINCXXOBJ)

$(PROGLIST): $(MAINCOBJ) $(MAINCXXOBJ)
	$(Q) mkdir -p $(BINDIR)
ifeq ($(CONFIG_CYGWIN_WINTOOL),y)
	$(call ELFLD,$(firstword $(PROGOBJ)),"${shell cygpath -w $(firstword $(PROGLIST))}")
else
	$(call ELFLD,$(firstword $(PROGOBJ)),$(firstword $(PROGLIST)))
endif
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
