############################################################################
# apps/examples/module/main/Makefile
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

# Module example built-in application info

PROGNAME = module
PRIORITY = SCHED_PRIORITY_DEFAULT
STACKSIZE = $(CONFIG_DEFAULT_TASK_STACKSIZE)
MODULE = $(CONFIG_EXAMPLES_MODULE)

MAINSRC = module_main.c

SYMTABSRC = mod_symtab.c
SYMTABOBJ = $(SYMTABSRC:.c=$(OBJEXT))


ifneq ($(CONFIG_BUILD_FLAT),y)
  PASS1_SYMTAB = $(TOPDIR)/pass1/mod_symtab.c
  MODLUE_NAME = chardev
endif

$(SYMTABSRC):
	$(Q) $(APPDIR)$(DELIM)tools$(DELIM)mksymtab.sh $(BINDIR) g_mod >$@.tmp
	$(Q) $(call TESTANDREPLACEFILE, $@.tmp, $@)


$(SYMTABOBJ): %$(OBJEXT): %.c
	$(call COMPILE, $<, $@, -fno-lto -fno-builtin)

ifeq ($(CONFIG_EXAMPLES_MODULE_BUILTINFS),y)
ifeq ($(CONFIG_EXAMPLES_MODULE_ROMFS),y)

ROMFSIMG = chardev_romfs.img
ROMFSSRC = chardev_romfs.c
ROMFSOBJ = $(ROMFSSRC:.c=$(OBJEXT))

$(ROMFSIMG):
	$(Q) genromfs -d $(BINDIR) -f $@

$(ROMFSSRC): $(ROMFSIMG)
	$(Q) (echo "#include <nuttx/compiler.h>" >$@ && \
		xxd -i $(ROMFSIMG) | sed -e "s/^unsigned char/const unsigned char aligned_data(4)/g" >>$@)

$(ROMFSOBJ): %$(OBJEXT): %.c
	$(call COMPILE, $<, $@, -fno-lto -fno-builtin)

else ifeq ($(CONFIG_EXAMPLES_MODULE_CROMFS),y)
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

endif

# Copy the symbol table into the kernel pass1/ build directory

ifneq ($(CONFIG_BUILD_FLAT),y)
$(PASS1_SYMTAB): $(SYMTABSRC)
	$(Q) install -m 0644 $(SYMTABSRC) $(PASS1_SYMTAB)
	$(Q) mv $(BINDIR)$(DELIM)$(MODLUE_NAME) .
endif

postinstall:: $(ROMFSOBJ) $(SYMTABOBJ) $(FSIMG_OBJ) $(PASS1_SYMTAB)
	$(call ARLOCK, $(call CONVERT_PATH,$(BIN)), $^)

distclean::
	$(Q) $(call DELFILE, $(SYMTABSRC) $(SYMTABOBJ) $(ROMFSSRC) $(ROMFSIMG) $(ROMFSOBJ) $(MODLUE_NAME))


include $(APPDIR)/Application.mk
