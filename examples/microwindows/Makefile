############################################################################
# apps/examples/microwindows/Makefile
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

PROGNAME  = $(CONFIG_EXAMPLES_MICROWINDOWS_PROGNAME)
PRIORITY  = $(CONFIG_EXAMPLES_MICROWINDOWS_PRIORITY)
STACKSIZE = $(CONFIG_EXAMPLES_MICROWINDOWS_STACKSIZE)
MODULE    = y

MAINSRC = mwdemo_main.c
CSRCS += mwdemo.c
MW_IMAGES = car8 zion208 penguin under4 cs1 rle8 microwin
CSRCS += $(MW_IMAGES:%=$(MW_IMGDIR)/%.c)

# Generate image .c files from .bmp
MW_SRC = $(APPDIR)/graphics/microwindows/microwindows/src
MW_IMGDIR = images
MW_CONVBMP = convbmp$(HOSTEXEEXT)

$(MW_CONVBMP): $(MW_SRC)/images/tools/convbmp.c
	$(HOSTCC) $(HOSTCFLAGS) -I$(MW_SRC)/include $< -o $@

$(MW_IMGDIR)/%.c: $(MW_SRC)/images/demos/mwin/%.bmp $(MW_CONVBMP)
	mkdir -p $(MW_IMGDIR)
	$(CURDIR)/$(MW_CONVBMP) -o $@ $<

clean::
	$(call DELFILE, $(MW_IMAGES:%=$(MW_IMGDIR)/%.c))
	$(Q) rmdir $(MW_IMGDIR) 2>/dev/null || true
	$(call DELFILE, $(MW_CONVBMP))

CFLAGS += -Wno-undef 
CFLAGS += -DNOGDI

include $(APPDIR)/Application.mk
