############################################################################
# apps/benchmarks/superpi/Makefile
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

SUPERPI_UNPACK   = superpi
SUPERPI_URL      = https://github.com/Fibonacci43/SuperPI/archive
SUPERPI_VERSION  = main
SUPERPI_ZIP      = $(SUPERPI_VERSION).zip
UNPACK          ?= unzip -q -o

CSRCS    += superpi/fftsg_h.c
PROGNAME  = $(CONFIG_BENCHMARK_SUPERPI_PROGNAME)
PRIORITY  = $(CONFIG_BENCHMARK_SUPERPI_PRIORITY)
STACKSIZE = $(CONFIG_BENCHMARK_SUPERPI_STACKSIZE)
MODULE    = $(CONFIG_BENCHMARK_SUPERPI)
MAINSRC   = superpi/pi_fftcs.c

$(SUPERPI_ZIP):
	@echo "Downloading: $(SUPERPI_URL)/$(SUPERPI_ZIP)"
	$(Q) curl -O -L $(SUPERPI_URL)/$(SUPERPI_ZIP)

$(SUPERPI_UNPACK): $(SUPERPI_ZIP)
	@echo "Unpacking: $(SUPERPI_ZIP) -> $(SUPERPI_UNPACK)"
	$(Q) $(UNPACK) $(SUPERPI_ZIP)
	$(Q) mv	SuperPI-$(SUPERPI_VERSION) $(SUPERPI_UNPACK)
	$(Q) touch $(SUPERPI_UNPACK)

ifeq ($(wildcard $(SUPERPI_UNPACK)/.git),)
context:: $(SUPERPI_UNPACK)

distclean::
	$(call DELDIR, $(SUPERPI_UNPACK))
	$(call DELFILE, $(SUPERPI_ZIP))
endif

include $(APPDIR)/Application.mk
