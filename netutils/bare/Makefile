############################################################################
# apps/netutils/bare/Makefile
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

BARE_CBARE_DIR = cbare

DEPPATH += --dep-path $(BARE_CBARE_DIR)$(DELIM)src
VPATH   += :$(BARE_CBARE_DIR)$(DELIM)src

CFLAGS += -I$(BARE_CBARE_DIR)/src

CSRCS  = $(BARE_CBARE_DIR)/src/alloc.c
CSRCS += $(BARE_CBARE_DIR)/src/cbare.c
CSRCS += $(BARE_CBARE_DIR)/src/die.c

ifneq ($(CONFIG_NETUTILS_BARE_TEST),)

PROGNAME  = baretest
PRIORITY  = 100
STACKSIZE = 2048
MODULE    = y

MAINSRC   = $(BARE_CBARE_DIR)/test/baretest.c

endif

$(BARE_CBARE_DIR):
	$(Q) echo "Cloning cbare repo..."
	$(Q) git clone --depth=1 https://git.sr.ht/~fsx/cbare
	$(Q) patch -p1 --directory=cbare < cbare_porting_for_nuttx.patch
	$(Q) touch $(BARE_CBARE_DIR)

context:: $(BARE_CBARE_DIR)

distclean::
	$(call DELDIR, $(BARE_CBARE_DIR))

include $(APPDIR)/Application.mk
