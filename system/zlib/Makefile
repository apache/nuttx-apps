############################################################################
# apps/system/zlib/Makefile
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

CSRCS += $(wildcard zlib/*.c)
CSRCS += zlib/contrib/minizip/ioapi.c
CSRCS += zlib/contrib/minizip/mztools.c
CSRCS += zlib/contrib/minizip/unzip.c
CSRCS += zlib/contrib/minizip/zip.c

CFLAGS += -Dunix -Wno-shadow -Wno-strict-prototypes -Wno-undef
CFLAGS += ${INCDIR_PREFIX}zlib

ZLIB_ZIP = zlib13.zip
ZLIB_SRC = zlib

ifeq ($(wildcard zlib/.git),)
$(ZLIB_ZIP):
	@echo "Downloading: $(ZLIB_ZIP)"
	$(Q) curl -O -L https://github.com/madler/zlib/releases/download/v1.3/zlib13.zip

$(ZLIB_SRC) : $(ZLIB_ZIP)
	unzip -o $(ZLIB_ZIP)
	mv zlib-1.3 $(ZLIB_SRC)

context:: $(ZLIB_SRC)

distclean::
	$(call DELDIR, $(ZLIB_SRC))
	$(call DELFILE, $(ZLIB_ZIP))

endif

MODULE = $(CONFIG_LIB_ZLIB)

ifneq ($(CONFIG_FS_LARGEFILE),y)
CFLAGS += -DIOAPI_NO_64
CFLAGS += -DMINIZIP_FOPEN_NO_64
endif

ifneq ($(CONFIG_UTILS_GZIP),)
PROGNAME += $(CONFIG_UTILS_GZIP_PROGNAME)
PRIORITY += $(CONFIG_UTILS_GZIP_PRIORITY)
STACKSIZE += $(CONFIG_UTILS_GZIP_STACKSIZE)
MAINSRC += zlib/test/minigzip.c
endif

ifneq ($(CONFIG_UTILS_ZIP),)
PROGNAME += $(CONFIG_UTILS_ZIP_PROGNAME)
PRIORITY += $(CONFIG_UTILS_ZIP_PRIORITY)
STACKSIZE += $(CONFIG_UTILS_ZIP_STACKSIZE)
MAINSRC += zlib/contrib/minizip/minizip.c
endif

ifneq ($(CONFIG_UTILS_UNZIP),)
PROGNAME += $(CONFIG_UTILS_UNZIP_PROGNAME)
PRIORITY += $(CONFIG_UTILS_UNZIP_PRIORITY)
STACKSIZE += $(CONFIG_UTILS_UNZIP_STACKSIZE)
MAINSRC += zlib/contrib/minizip/miniunz.c
endif

ifneq ($(CONFIG_LIB_ZLIB_TEST),)
PRIORITY += $(CONFIG_LIB_ZLIB_TEST_PRIORITY)
STACKSIZE += $(CONFIG_LIB_ZLIB_TEST_STACKSIZE)
PROGNAME += zlib_infcover_test
MAINSRC += zlib/test/infcover.c
endif

CFLAGS += -DMAX_WBITS=$(CONFIG_LIB_ZLIB_MAX_WBITS)
CFLAGS += -DMAX_MEM_LEVEL=$(CONFIG_LIB_ZLIB_MAX_MEM_LEVEL)

include $(APPDIR)/Application.mk
