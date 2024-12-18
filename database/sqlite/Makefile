############################################################################
# apps/database/sqlite/Makefile
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

SQLITEDIR=$(APPDIR)/database/sqlite
BUILDDIR=$(SQLITEDIR)/build

CSRCS += ${BUILDDIR}/sqlite3.c

CFLAGS += ${INCDIR_PREFIX}$(SQLITEDIR)
CFLAGS += ${DEFINE_PREFIX}_HAVE_SQLITE_CONFIG_H
CFLAGS += ${DEFINE_PREFIX}NDEBUG
CFLAGS += -Wno-unused-variable -Wno-undef -Wno-unused-function -Wno-shadow

ifneq ($(CONFIG_UTILS_SQLITE),)
PROGNAME  = sqlite3
PRIORITY  = 100
STACKSIZE = ${CONFIG_UTILS_SQLITE_STACKSIZE}
MAINSRC = ${BUILDDIR}/shell.c
endif


# Download and unpack tarball if no git repo found

sqlite.zip:
ifeq ($(wildcard sqlite/.git),)
	$(Q) curl -L https://codeload.github.com/sqlite/sqlite/zip/refs/tags/vesion-${CONFIG_LIB_SQLITE_VERSION} -o sqlite.zip
	$(Q) unzip -q -o sqlite.zip
	$(Q) mv sqlite-vesion-* sqlite
	$(call DELFILE, sqlite.zip)
endif

context:: sqlite.zip
	$(Q) mkdir -p ${BUILDDIR} && \
	cd ${BUILDDIR} && \
	echo "Generate SQLite amalgamation version" && \
	${SQLITEDIR}/sqlite/configure 1> /dev/null && \
	make sqlite3.c -j 1> /dev/null

clean::
	$(call DELFILE, ${BUILDDIR}/*.o)

distclean::
	$(call DELDIR, ${BUILDDIR})
ifeq ($(wildcard sqlite/.git),)
	$(call DELDIR, sqlite)
endif

include $(APPDIR)/Application.mk

