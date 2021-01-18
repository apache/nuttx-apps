############################################################################
# apps/crypto/mbedtls/Makefile
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

# Mbed TLS crypto library

# Set up build configuration and environment

MBEDTLS_URL ?= "https://github.com/ARMmbed/mbedtls/archive"

MBEDTLS_VERSION = $(patsubst "%",%,$(strip $(CONFIG_MBEDTLS_VERSION)))
MBEDTLS_ZIP = v$(MBEDTLS_VERSION).zip

MBEDTLS_UNPACKNAME = mbedtls
UNPACK ?= unzip -q -o

MBEDTLS_UNPACKLIBDIR = $(MBEDTLS_UNPACKNAME)$(DELIM)library
MBEDTLS_UNPACKPROGDIR = $(MBEDTLS_UNPACKNAME)$(DELIM)programs

# This lets Mbed TLS better use some of the POSIX features we have
CFLAGS += ${shell $(DEFINE) "$(CC)" __unix__}

CSRCS = $(wildcard $(MBEDTLS_UNPACKLIBDIR)$(DELIM)*.c)

$(MBEDTLS_ZIP):
	@echo "Downloading: $(MBEDTLS_URL)/$(MBEDTLS_ZIP)"
	$(Q) curl -O -L $(MBEDTLS_URL)/$(MBEDTLS_ZIP)

$(MBEDTLS_UNPACKNAME): $(MBEDTLS_ZIP)
	@echo "Unpacking: $(MBEDTLS_ZIP) -> $(MBEDTLS_UNPACKNAME)"
	$(Q) $(UNPACK) $(MBEDTLS_ZIP)
	$(Q) mv	mbedtls-$(MBEDTLS_VERSION) $(MBEDTLS_UNPACKNAME)
	$(Q) touch $(MBEDTLS_UNPACKNAME)

context:: $(MBEDTLS_UNPACKNAME)

distclean::
	$(call DELDIR, $(MBEDTLS_UNPACKNAME))
	$(call DELFILE, $(MBEDTLS_ZIP))

# Configuration Applications

ifneq ($(CONFIG_MBEDTLS_APPS),)

MODULE = $(CONFIG_MBEDTLS_APPS)

ifeq ($(CONFIG_MBEDTLS_APP_BENCHMARK),y)

PROGNAME  += $(CONFIG_MBEDTLS_APP_BENCHMARK_PROGNAME)
PRIORITY  += $(CONFIG_MBEDTLS_APP_BENCHMARK_PRIORITY)
STACKSIZE += $(CONFIG_MBEDTLS_APP_BENCHMARK_STACKSIZE)

MAINSRC += $(MBEDTLS_UNPACKPROGDIR)/test/benchmark.c
endif

ifeq ($(CONFIG_MBEDTLS_APP_SELFTEST),y)

PROGNAME  += $(CONFIG_MBEDTLS_APP_SELFTEST_PROGNAME)
PRIORITY  += $(CONFIG_MBEDTLS_APP_SELFTEST_PRIORITY)
STACKSIZE += $(CONFIG_MBEDTLS_APP_SELFTEST_STACKSIZE)

MAINSRC += $(MBEDTLS_UNPACKPROGDIR)/test/selftest.c
endif

endif

include $(APPDIR)/Application.mk
