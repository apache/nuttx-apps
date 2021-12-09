############################################################################
# apps/testing/iozone/Make.defs
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

PROGNAME  = $(CONFIG_TESTING_IOZONE_PROGNAME)
PRIORITY  = $(CONFIG_TESTING_IOZONE_PRIORITY)
STACKSIZE = $(CONFIG_TESTING_IOZONE_STACKSIZE)
MODULE    = $(CONFIG_TESTING_IOZONE)

CFLAGS   += -Dunix -DHAVE_ANSIC_C -DHAVE_PREAD -DNAME='"nuttx"'
CFLAGS   += -DNO_MADVISE -DNO_FORK -D__FreeBSD__ -DNO_THREADS
CFLAGS   += -Wno-unused-parameter -Wno-unused-function -Wno-shadow
CFLAGS   += -Wno-unused-but-set-variable -Wno-strict-prototypes
CFLAGS   += -Wno-misleading-indentation -Wno-maybe-uninitialized

CFLAGS   += -DMAXBUFFERSIZE=32*1024 -DMAXSTREAMS=8 -DMAXNAMESIZE=NAME_MAX
CFLAGS   += -DRECLEN_START=1024

CSRCS     = iozone/src/current/libbif.c
MAINSRC   = iozone/src/current/iozone.c

CONFIG_TESTING_IOZONE_URL ?= "https://www.iozone.org/src/current/"

IOZONE_UNPACKNAME = iozone
IOZONE_VERSION = $(patsubst "%",%,$(strip $(CONFIG_TESTING_IOZONE_VERSION)))
IOZONE_ZIP = iozone$(IOZONE_VERSION).tgz

$(IOZONE_ZIP):
	@echo "Downloading: $(IOZONE_ZIP)"
	$(Q) curl -O -L $(CONFIG_TESTING_IOZONE_URL)/$(IOZONE_ZIP)

$(IOZONE_UNPACKNAME): $(IOZONE_ZIP)
	@echo "Unpacking: $(IOZONE_ZIP) -> $(IOZONE_UNPACKNAME)"
	$(Q) tar zxvf $(IOZONE_ZIP)
	$(Q) mv iozone$(IOZONE_VERSION) $(IOZONE_UNPACKNAME)
	$(Q) echo "Patching $(IOZONE_UNPACKNAME)"
	$(Q) cat iozone.patch | patch -s -N -d $(IOZONE_UNPACKNAME) -p1
	$(Q) touch $(IOZONE_UNPACKNAME)

context:: $(IOZONE_UNPACKNAME)

distclean::
	$(call DELDIR, $(IOZONE_UNPACKNAME))
	$(call DELFILE, $(IOZONE_ZIP))

include $(APPDIR)/Application.mk
