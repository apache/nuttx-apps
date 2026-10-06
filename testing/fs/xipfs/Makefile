############################################################################
# apps/testing/fs/xipfs/Makefile
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

# XIPFS file system test

PROGNAME = xipfs_test
PRIORITY = SCHED_PRIORITY_DEFAULT
STACKSIZE = $(CONFIG_DEFAULT_TASK_STACKSIZE)
MODULE = $(CONFIG_TESTING_FS_XIPFS)

MAINSRC = xipfs_main.c

# With CONFIG_FDPIC the test also loads modules.  They are built in
# examples/fdpicxip/modules, and embedded here after they are linked.

ifeq ($(CONFIG_FDPIC),y)
BLOBS   = callback cxxuser funcdesc lazymod libcounter.so libshape.so \
          manyneeded missingsym user
BLOBDIR = $(APPDIR)$(DELIM)examples$(DELIM)fdpicxip$(DELIM)modules
BLOBSH  = $(BLOBDIR)$(DELIM)mkblobs.sh
BLOBSRC = xipfs_blobs.c
BLOBOBJ = $(BLOBSRC:.c=$(OBJEXT))

$(BLOBSRC): $(addprefix $(BINDIR)$(DELIM),$(BLOBS))
	$(Q) $(BLOBSH) g_xipfs $^ > $@.tmp
	$(Q) $(call TESTANDREPLACEFILE, $@.tmp, $@)

$(BLOBOBJ): %$(OBJEXT): %.c
	$(call COMPILE, $<, $@)

postinstall:: $(BLOBOBJ)
	$(call ARLOCK, $(call CONVERT_PATH,$(BIN)), $^)

distclean::
	$(call DELFILE, $(BLOBSRC) $(BLOBOBJ))
endif

include $(APPDIR)/Application.mk
