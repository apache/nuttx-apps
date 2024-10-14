############################################################################
# apps/system/libarchive/Makefile
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

LIBARCHIVE_ZIP = libarchive-3.7.7.zip
LIBARCHIVE_SRC = libarchive

ifeq ($(wildcard libarchive/.git),)
$(LIBARCHIVE_ZIP):
	@echo "Downloading $(LIBARCHIVE_ZIP)"
	$(Q) curl -O -L https://github.com/libarchive/libarchive/releases/download/v3.7.7/libarchive-3.7.7.zip

$(LIBARCHIVE_SRC): $(LIBARCHIVE_ZIP)
	unzip -o $(LIBARCHIVE_ZIP)
	mv libarchive-3.7.7 $(LIBARCHIVE_SRC)
	$(Q) patch -d $(LIBARCHIVE_SRC) -p1 < 0001-fix-conflict-libarchive-for-nuttx.patch

context:: $(LIBARCHIVE_SRC)

distclean::
	$(call DELDIR, $(LIBARCHIVE_SRC), $(LIBARCHIVE_ZIP))

endif

CFLAGS += -I.
CFLAGS += -I$(LIBARCHIVE_SRC)/libarchive
CFLAGS += -I$(LIBARCHIVE_SRC)/libarchive_fe

CFLAGS += -DHAVE_CONFIG_H=1
CFLAGS += -D__LIBARCHIVE_CONFIG_H_INCLUDED=1
CFLAGS += -Wno-undef
CFLAGS += -UMAX_SIGNO

CSRCS += $(wildcard $(LIBARCHIVE_SRC)/libarchive/*.c)
CSRCS += $(wildcard $(LIBARCHIVE_SRC)/libarchive_fe/*.c)

ifneq ($(CONFIG_UTILS_XZ),)
CFLAGS += -I$(APPDIR)/system/xz/xz/src/liblzma/api
endif

ifneq ($(CONFIG_LIB_ZLIB),)
CFLAGS += -I$(APPDIR)/system/zlib/zlib
endif

ifneq ($(CONFIG_CRYPTO_MBEDTLS),)
CFLAGS += -I$(APPDIR)/crypto/mbedtls/mbedtls/include
endif

ifneq ($(CONFIG_UTILS_LIBARCHIVE_BSDUNZIP),)
CSRCS += libarchive/unzip/cmdline.c
CSRCS += libarchive/unzip/la_getline.c
PROGNAME += $(CONFIG_UTILS_LIBARCHIVE_BSDUNZIP_PROGNAME)
PRIORITY += $(CONFIG_UTILS_LIBARCHIVE_BSDUNZIP_PRIORITY)
STACKSIZE += $(CONFIG_UTILS_LIBARCHIVE_BSDUNZIP_STACKSIZE)
MAINSRC += libarchive/unzip/bsdunzip.c
endif

ifneq ($(CONFIG_UTILS_LIBARCHIVE_BSDTAR),)
CSRCS += libarchive/tar/cmdline.c
CSRCS += libarchive/tar/creation_set.c
CSRCS += libarchive/tar/read.c
CSRCS += libarchive/tar/write.c
CSRCS += libarchive/tar/util.c
CSRCS += libarchive/tar/subst.c

PROGNAME += $(CONFIG_UTILS_LIBARCHIVE_BSDTAR_PROGNAME)
PRIORITY += $(CONFIG_UTILS_LIBARCHIVE_BSDTAR_PRIORITY)
STACKSIZE += $(CONFIG_UTILS_LIBARCHIVE_BSDTAR_STACKSIZE)
MAINSRC += libarchive/tar/bsdtar.c
endif

ifneq ($(CONFIG_UTILS_LIBARCHIVE_BSDCAT),)
CSRCS += libarchive/cat/cmdline.c

PROGNAME += $(CONFIG_UTILS_LIBARCHIVE_BSDCAT_PROGNAME)
PRIORITY += $(CONFIG_UTILS_LIBARCHIVE_BSDCAT_PRIORITY)
STACKSIZE += $(CONFIG_UTILS_LIBARCHIVE_BSDCAT_STACKSIZE)
MAINSRC += libarchive/cat/bsdcat.c
endif

ifneq ($(CONFIG_UTILS_LIBARCHIVE_CPIO),)
CSRCS += libarchive/cpio/cmdline.c

PROGNAME += $(CONFIG_UTILS_LIBARCHIVE_CPIO_PROGNAME)
PRIORITY += $(CONFIG_UTILS_LIBARCHIVE_CPIO_PRIORITY)
STACKSIZE += $(CONFIG_UTILS_LIBARCHIVE_CPIO_STACKSIZE)
MAINSRC += libarchive/cpio/cpio.c
endif

ifneq ($(CONFIG_UTILS_LIBARCHIVE_MINITAR),)
CFLAGS += -Wno-shadow
PROGNAME += $(CONFIG_UTILS_LIBARCHIVE_MINITAR_PROGNAME)
PRIORITY += $(CONFIG_UTILS_LIBARCHIVE_MINITAR_PRIORITY)
STACKSIZE += $(CONFIG_UTILS_LIBARCHIVE_MINITAR_STACKSIZE)
MAINSRC += libarchive/examples/minitar/minitar.c
endif

ifneq ($(CONFIG_UTILS_LIBARCHIVE_TARFILTER),)
CFLAGS += -Wno-deprecated-declarations
PROGNAME += $(CONFIG_UTILS_LIBARCHIVE_TARFILTER_PROGNAME)
PRIORITY += $(CONFIG_UTILS_LIBARCHIVE_TARFILTER_PRIORITY)
STACKSIZE += $(CONFIG_UTILS_LIBARCHIVE_TARFILTER_STACKSIZE)
MAINSRC += libarchive/examples/tarfilter.c
endif

ifneq ($(CONFIG_UTILS_LIBARCHIVE_UNTAR),)
CFLAGS += -Wno-unused-but-set-variable
PROGNAME += $(CONFIG_UTILS_LIBARCHIVE_UNTAR_PROGNAME)
PRIORITY += $(CONFIG_UTILS_LIBARCHIVE_UNTAR_PRIORITY)
STACKSIZE += $(CONFIG_UTILS_LIBARCHIVE_UNTAR_STACKSIZE)
MAINSRC += libarchive/examples/untar.c
endif

include $(APPDIR)/Application.mk
