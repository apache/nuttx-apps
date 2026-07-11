############################################################################
# apps/examples/elf/main/Makefile
#
# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.  The
# ASF licenses this file to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance with the
# License.  You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations
# under the License.
#
############################################################################

include $(APPDIR)/Make.defs

# Module example built-in application info

PROGNAME = elf
PRIORITY = SCHED_PRIORITY_DEFAULT
STACKSIZE = $(CONFIG_DEFAULT_TASK_STACKSIZE)
MODULE = $(CONFIG_EXAMPLES_ELF)

MAINSRC = elf_main.c

SYMTABSRC = test_symtab.c
SYMTABOBJ = $(SYMTABSRC:.c=$(OBJEXT))

ELFNAME_BASE = errno hello
ifneq ($(CONFIG_DISABLE_SIGNALS),y)
  ELFNAME_BASE += signal
endif

ifeq ($(CONFIG_HAVE_CXX),y)
  ELFNAME_BASE += hello++1 hello++2
  ifeq ($(CONFIG_HAVE_CXXINITIALIZE),y)
    ELFNAME_BASE += hello++3
  endif
  ifeq ($(CONFIG_EXAMPLES_ELF_CXX),y)
    ELFNAME_BASE += hello++4 hello++5
  endif
endif
ifeq ($(CONFIG_EXAMPLES_ELF_LONGJMP),y)
  ELFNAME_BASE += longjmp
endif
ifneq ($(CONFIG_DISABLE_PTHREAD),y)
  ELFNAME_BASE += pthread mutex
endif
ifneq ($(CONFIG_ARCH_ADDRENV),y)
  ELFNAME_BASE += task
endif

ELFNAME = $(addprefix $(BINDIR)$(DELIM),$(ELFNAME_BASE))

$(SYMTABSRC):
	$(Q) $(APPDIR)$(DELIM)tools$(DELIM)mksymtab.sh $(ELFNAME) g_elf >$@.tmp
	$(Q) $(call TESTANDREPLACEFILE, $@.tmp, $@)


$(SYMTABOBJ): %$(OBJEXT): %.c
	$(call COMPILE, $<, $@, -fno-lto -fno-builtin)

ifeq ($(CONFIG_EXAMPLES_ELF_ROMFS),y)

ROMFSIMG = elf_romfs.img
ROMFSSRC = elf_romfs.c
ROMFSOBJ = $(ROMFSSRC:.c=$(OBJEXT))
ifdef CONFIG_SYSTEM_NXPKG
HOSTOBJSEXT ?= hobj
PKGINDEX = $(BINDIR)/index.json
PKGBADINDEX = $(BINDIR)/bad-index.json
PKGTEST  = $(BINDIR)/pkgtest.nsh
PKGFAIL  = $(BINDIR)/pkgfail.nsh
PKG_FIXTURE_GEN = mk_pkg_fixture$(HOSTEXEEXT)
PKG_FIXTURE_SRCS = mk_pkg_fixture.c
PKG_FIXTURE_OBJS = $(PKG_FIXTURE_SRCS:.c=.$(HOSTOBJSEXT))
PKG_FIXTURE_OUTPUTS = $(PKGINDEX) $(PKGBADINDEX) $(PKGTEST) $(PKGFAIL)
PKG_FIXTURE_INPUTS = $(BINDIR)/hello
PKG_FIXTURE_STAMP = $(BINDIR)/.pkgtest.stamp
PKG_FIXTURE_COMPAT = $(if $(CONFIG_ARCH_BOARD),$(CONFIG_ARCH_BOARD),$(CONFIG_ARCH_BOARD_CUSTOM_NAME))
PKG_FIXTURE_ARGS = \
	$(BINDIR)/hello $(PKGINDEX) $(PKGBADINDEX) $(PKGTEST) $(PKGFAIL) \
	$(CONFIG_ARCH) $(PKG_FIXTURE_COMPAT)

$(PKG_FIXTURE_OBJS): %.$(HOSTOBJSEXT): %.c
	@echo "CC:  $<"
	$(Q) $(HOSTCC) -c $(HOSTCFLAGS) $< -o $@

$(PKG_FIXTURE_GEN): $(PKG_FIXTURE_OBJS)
	$(Q) $(HOSTCC) $(HOSTLDFLAGS) $(PKG_FIXTURE_OBJS) -o $@

$(PKG_FIXTURE_OUTPUTS): $(PKG_FIXTURE_STAMP)

$(PKG_FIXTURE_STAMP): $(PKG_FIXTURE_INPUTS) $(PKG_FIXTURE_GEN)
	$(Q) ./$(PKG_FIXTURE_GEN) \
		$(PKG_FIXTURE_ARGS)
	$(Q) touch $@
endif

$(ROMFSIMG): $(PKG_FIXTURE_OUTPUTS)
	$(Q) genromfs -d $(BINDIR) -f $@

$(ROMFSSRC): $(ROMFSIMG)
	$(Q) (echo "#include <nuttx/compiler.h>" >$@ && \
		xxd -i $(ROMFSIMG) | sed -e "s/^unsigned char/const unsigned char aligned_data(4)/g" >>$@)

$(ROMFSOBJ): %$(OBJEXT): %.c
	$(call COMPILE, $<, $@, -fno-lto -fno-builtin)

else ifeq ($(CONFIG_EXAMPLES_ELF_CROMFS),y)
  NXTOOLDIR = $(TOPDIR)/tools
  GENCROMFSSRC = gencromfs.c
  GENCROMFSEXE = gencromfs$(HOSTEXEEXT)

  FSIMG_SRC = cromfs.c
	FSIMG_OBJ = $(FSIMG_SRC:.c=$(OBJEXT))

$(NXTOOLDIR)/$(GENCROMFSEXE): $(NXTOOLDIR)/$(GENCROMFSSRC)
	$(Q) $(MAKE) -C $(NXTOOLDIR) -f Makefile.host $(GENCROMFSEXE)

# Create the cromfs.c file from the populated cromfs directory

$(FSIMG_SRC):$(NXTOOLDIR)/$(GENCROMFSEXE)
	$(Q) $(NXTOOLDIR)/$(GENCROMFSEXE) $(BINDIR) $@.tmp
	$(Q) $(call TESTANDREPLACEFILE, $@.tmp, $@)

$(FSIMG_OBJ): %$(OBJEXT): %.c
	$(call COMPILE, $<, $@, -fno-lto -fno-builtin)

endif

postinstall:: $(ROMFSOBJ) $(SYMTABOBJ) $(FSIMG_OBJ)
	$(call ARLOCK, $(call CONVERT_PATH,$(BIN)), $^)

distclean::
	$(Q) $(call DELFILE, $(SYMTABSRC) $(SYMTABOBJ) $(ROMFSSRC) $(ROMFSIMG) $(ROMFSOBJ) $(MODLUE_NAME) $(PKG_FIXTURE_GEN) $(PKG_FIXTURE_OBJS) $(PKG_FIXTURE_STAMP))


include $(APPDIR)/Application.mk
