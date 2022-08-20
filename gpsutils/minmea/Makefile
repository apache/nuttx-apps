############################################################################
# apps/gpsutils/minmea/Makefile
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

MINMEA_URL ?= "https://github.com/kosma/minmea/archive"
MINMEA_VERSION ?= db46128e73cee26d6a6eb0482dcba544ee1ea9f5

MINMEA_DIR := $(APPDIR)/gpsutils/minmea/minmea
MINMEA_UNPACKNAME := minmea
MINMEA_UNPACKDIR := $(MINMEA_DIR)/$(MINMEA_UNPACKNAME)


$(MINMEA_UNPACKDIR):
	@echo "Downloading: $(MINMEA_UNPACKNAME)"
	$(Q) curl -O -L $(MINMEA_URL)/$(MINMEA_VERSION).zip
	$(Q) unzip -o -j $(MINMEA_VERSION).zip
	$(call DELFILE, $(MINMEA_VERSION).zip)
	$(call MOVEFILE, $(MINMEA_UNPACKNAME)-$(MINMEA_VERSION), $(MINMEA_UNPACKDIR))

# Files

CSRCS	= minmea/minmea.c
CFLAGS += -std=c99

context:: $(MINMEA_UNPACKDIR)

clean::
	$(call DELFILE, $(OBJS))

distclean::
	$(call DELDIR, $(MINMEA_UNPACKDIR))

include $(APPDIR)/Application.mk
