############################################################################
# apps/graphics/libjpeg/Makefile
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

SRC = libjpeg

CSRCS += $(SRC)/jaricom.c
CSRCS += $(SRC)/jcomapi.c
CSRCS += $(SRC)/jdapimin.c
CSRCS += $(SRC)/jdapistd.c
CSRCS += $(SRC)/jdarith.c
CSRCS += $(SRC)/jdatasrc.c
CSRCS += $(SRC)/jdcoefct.c
CSRCS += $(SRC)/jdcolor.c
CSRCS += $(SRC)/jddctmgr.c
CSRCS += $(SRC)/jdhuff.c
CSRCS += $(SRC)/jdinput.c
CSRCS += $(SRC)/jdmainct.c
CSRCS += $(SRC)/jdmarker.c
CSRCS += $(SRC)/jdmaster.c
CSRCS += $(SRC)/jdmerge.c
CSRCS += $(SRC)/jdpostct.c
CSRCS += $(SRC)/jdsample.c
CSRCS += $(SRC)/jdtrans.c
CSRCS += $(SRC)/jerror.c
CSRCS += $(SRC)/jidctflt.c
CSRCS += $(SRC)/jidctfst.c
CSRCS += $(SRC)/jidctint.c
CSRCS += $(SRC)/jquant1.c
CSRCS += $(SRC)/jquant2.c
CSRCS += $(SRC)/jutils.c
CSRCS += $(SRC)/jmemmgr.c
CSRCS += $(SRC)/jmemname.c

CFLAGS += -DTEMP_DIRECTORY=\"$(CONFIG_EXTERNALS_LIBJPEG_TEMP_DIR)\"

CFLAGS += -Wno-shadow -Wno-strict-prototypes -Wno-unknown-pragmas

MODULE = $(CONFIG_LIBJPEG)

libjpeg.tar.gz:
	$(Q) curl -L http://www.ijg.org/files/jpegsrc.v$(CONFIG_LIBJPEG_VER).tar.gz -o libjpeg.tar.gz
	$(Q) tar xf libjpeg.tar.gz
	$(Q) mv jpeg-$(CONFIG_LIBJPEG_VER) libjpeg
	$(Q) patch -Np1 <0001-libjpeg-add-preconfigured-jconfig.h.patch

# Download and unpack tarball if no git repo found
ifeq ($(wildcard libjpeg/libjpeg),)
context:: libjpeg.tar.gz

distclean::
	$(call DELDIR, libjpeg)
	$(call DELFILE, libjpeg.tar.gz)
endif

include $(APPDIR)/Application.mk
