############################################################################
# apps/fsutils/romloader/Makefile
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

# ROMLoader application

############################################################################
# Flags
############################################################################

ROM_DIR = $(APPDIR)$(DELIM)fsutils$(DELIM)romloader$(DELIM)rom
IMG_SRC_FILE = rom.c

ifeq ($(CONFIG_FSUTILS_ROMLOADER_ROMFS),y)
  ROMFS_IMG = romfs.img
else
  NXTOOLDIR = $(TOPDIR)/tools
  GENCROMFSSRC = gencromfs.c
  GENCROMFSEXE = gencromfs$(HOSTEXEEXT)
endif

CSRCS = $(IMG_SRC_FILE)

############################################################################
# Targets
############################################################################

# Make sure to always rebuild the image file for the case where files are
# manually added or removed from the ROM directory.

.PHONY: $(IMG_SRC_FILE)

ensure_rom_dir:
	$(Q) mkdir -p $(ROM_DIR)

ifeq ($(CONFIG_FSUTILS_ROMLOADER_ROMFS),y)

# ROMFS targets

checkgenromfs:
	$(Q) genromfs -h 1>/dev/null 2>&1 || { \
 echo "Host executable genromfs not available in PATH"; \
 echo "You may need to download it from http://romfs.sourceforge.net/"; \
 exit 1; \
	}

$(ROMFS_IMG): checkgenromfs ensure_rom_dir
	$(Q) genromfs -f $@.tmp -d $(ROM_DIR) -V "ROMLOADER"
	$(Q) $(call TESTANDREPLACEFILE, $@.tmp, $@)

$(IMG_SRC_FILE): $(ROMFS_IMG)
	$(Q) echo "#include <nuttx/compiler.h>" >$@ && \
		xxd -i $(ROMFS_IMG) | sed -e "s/^unsigned char/const unsigned char aligned_data(4)/g" >>$@

else

# CROMFS targets

$(NXTOOLDIR)/$(GENCROMFSEXE): $(NXTOOLDIR)/$(GENCROMFSSRC)
	$(Q) $(MAKE) -C $(NXTOOLDIR) -f Makefile.host $(GENCROMFSEXE)

$(IMG_SRC_FILE): $(NXTOOLDIR)/$(GENCROMFSEXE) ensure_rom_dir
	$(Q) $(NXTOOLDIR)/$(GENCROMFSEXE) $(ROM_DIR) $@.tmp
	$(Q) $(call TESTANDREPLACEFILE, $@.tmp, $@)

endif

context:: $(IMG_SRC_FILE)

distclean::
	$(call DELFILE, $(IMG_SRC_FILE))
	$(call DELDIR,  $(ROM_DIR))
ifeq ($(CONFIG_FSUTILS_ROMLOADER_ROMFS),y)
	$(call DELFILE, $(ROMFS_IMG))
endif

############################################################################
# Applications Configuration
############################################################################

PROGNAME = romloader
PRIORITY = SCHED_PRIORITY_DEFAULT
STACKSIZE = $(CONFIG_DEFAULT_TASK_STACKSIZE)
MODULE = $(CONFIG_FSUTILS_ROMLOADER)

MAINSRC = romloader_main.c

include $(APPDIR)/Application.mk
