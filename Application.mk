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

include $(APPDIR)/Make.defs

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

# Object files

AOBJS = $(ASRCS:.S=$(OBJEXT))
COBJS = $(CSRCS:.c=$(OBJEXT))
CXXOBJS = $(CXXSRCS:$(CXXEXT)=$(OBJEXT))

ifeq ($(suffix $(MAINSRC)),$(CXXEXT))
  MAINOBJ = $(MAINSRC:$(CXXEXT)=$(OBJEXT))
else
  MAINOBJ = $(MAINSRC:.c=$(OBJEXT))
endif

SRCS = $(ASRCS) $(CSRCS) $(CXXSRCS) $(MAINSRC)
OBJS = $(AOBJS) $(COBJS) $(CXXOBJS)

ifneq ($(BUILD_MODULE),y)
  OBJS += $(MAINOBJ)
endif

ROOTDEPPATH += --dep-path .

VPATH += :.

# Targets follow

all:: .built
.PHONY: clean preconfig depend distclean
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
#	$(Q) $(STRIP) $2
	$(Q) chmod +x $2
endef

$(AOBJS): %$(OBJEXT): %.S
	$(if $(and $(CONFIG_BUILD_LOADABLE),$(AELFFLAGS)), \
		$(call ELFASSEMBLE, $<, $@), $(call ASSEMBLE, $<, $@))

$(COBJS): %$(OBJEXT): %.c
	$(if $(and $(CONFIG_BUILD_LOADABLE),$(CELFFLAGS)), \
		$(call ELFCOMPILE, $<, $@), $(call COMPILE, $<, $@))

$(CXXOBJS): %$(OBJEXT): %$(CXXEXT)
	$(if $(and $(CONFIG_BUILD_LOADABLE),$(CXXELFFLAGS)), \
		$(call ELFCOMPILEXX, $<, $@), $(call COMPILEXX, $<, $@))

.built: $(OBJS)
ifeq ($(WINTOOL),y)
	$(call ARCHIVE, "${shell cygpath -w $(BIN)}", $(OBJS))
else
	$(call ARCHIVE, $(BIN), $(OBJS))
endif
	$(Q) touch $@

ifeq ($(BUILD_MODULE),y)

ifeq ($(suffix $(MAINSRC)),$(CXXEXT))
$(MAINOBJ): %$(OBJEXT): %$(CXXEXT)
	$(if $(and $(CONFIG_BUILD_LOADABLE),$(CXXELFFLAGS)), \
		$(call ELFCOMPILEXX, $<, $@), $(call COMPILEXX, $<, $@))
else
$(MAINOBJ): %$(OBJEXT): %.c
	$(if $(and $(CONFIG_BUILD_LOADABLE),$(CELFFLAGS)), \
		$(call ELFCOMPILE, $<, $@), $(call COMPILE, $<, $@))
endif

PROGLIST := $(wordlist 1,$(words $(MAINOBJ)),$(PROGNAME))
PROGLIST := $(addprefix $(BINDIR)$(DELIM),$(PROGLIST))
PROGOBJ := $(MAINOBJ)

$(PROGLIST): $(MAINOBJ)
ifeq ($(WINTOOL),y)
	$(call ELFLD,$(firstword $(PROGOBJ)),"${shell cygpath -w $(firstword $(PROGLIST))}")
else
	$(call ELFLD,$(firstword $(PROGOBJ)),$(firstword $(PROGLIST)))
endif
	$(eval PROGLIST=$(filter-out $(firstword $(PROGLIST)),$(PROGLIST)))
	$(eval PROGOBJ=$(filter-out $(firstword $(PROGOBJ)),$(PROGOBJ)))

install:: $(PROGLIST)

else

MAINNAME := $(addsuffix _main,$(PROGNAME))

ifeq ($(suffix $(MAINSRC)),$(CXXEXT))
$(MAINOBJ): %$(OBJEXT): %$(CXXEXT)
	$(eval $<_CXXFLAGS += ${shell $(DEFINE) "$(CXX)" main=$(firstword $(MAINNAME))})
	$(eval $<_CXXELFFLAGS += ${shell $(DEFINE) "$(CXX)" main=$(firstword $(MAINNAME))})
	$(eval MAINNAME=$(filter-out $(firstword $(MAINNAME)),$(MAINNAME)))
	$(if $(and $(CONFIG_BUILD_LOADABLE),$(CXXELFFLAGS)), \
		$(call ELFCOMPILEXX, $<, $@), $(call COMPILEXX, $<, $@))
else
$(MAINOBJ): %$(OBJEXT): %.c
	$(eval $<_CFLAGS += ${shell $(DEFINE) "$(CC)" main=$(firstword $(MAINNAME))})
	$(eval $<_CELFFLAGS += ${shell $(DEFINE) "$(CC)" main=$(firstword $(MAINNAME))})
	$(eval MAINNAME=$(filter-out $(firstword $(MAINNAME)),$(MAINNAME)))
	$(if $(and $(CONFIG_BUILD_LOADABLE),$(CELFFLAGS)), \
		$(call ELFCOMPILE, $<, $@), $(call COMPILE, $<, $@))
endif

install::

endif # BUILD_MODULE

preconfig::

ifeq ($(CONFIG_NSH_BUILTIN_APPS),y)
ifneq ($(PROGNAME),)
ifneq ($(PRIORITY),)
ifneq ($(STACKSIZE),)

REGLIST := $(addprefix $(BUILTIN_REGISTRY)$(DELIM),$(addsuffix _main.bdat,$(PROGNAME)))
APPLIST := $(PROGNAME)

$(REGLIST): $(DEPCONFIG) Makefile
	$(call REGISTER,$(firstword $(APPLIST)),$(firstword $(PRIORITY)),$(firstword $(STACKSIZE)),$(if $(BUILD_MODULE),,$(firstword $(APPLIST))_main))
	$(eval APPLIST=$(filter-out $(firstword $(APPLIST)),$(APPLIST)))
	$(if $(filter-out $(firstword $(PRIORITY)),$(PRIORITY)),$(eval PRIORITY=$(filter-out $(firstword $(PRIORITY)),$(PRIORITY))))
	$(if $(filter-out $(firstword $(STACKSIZE)),$(STACKSIZE)),$(eval STACKSIZE=$(filter-out $(firstword $(STACKSIZE)),$(STACKSIZE))))

context:: $(REGLIST)
else
context::
endif
else
context::
endif
else
context::
endif
else
context::
endif

.depend: Makefile $(SRCS)
ifeq ($(filter %$(CXXEXT),$(SRCS)),)
	$(Q) $(MKDEP) $(ROOTDEPPATH) "$(CC)" -- $(CFLAGS) -- $(filter-out Makefile,$^) >Make.dep
else
	$(Q) $(MKDEP) $(ROOTDEPPATH) "$(CXX)" -- $(CXXFLAGS) -- $(filter-out Makefile,$^) >Make.dep
endif
	$(Q) touch $@

depend:: .depend

clean::
	$(call DELFILE, .built)
	$(call CLEAN)

distclean:: clean
	$(call DELFILE, Make.dep)
	$(call DELFILE, .depend)

-include Make.dep
