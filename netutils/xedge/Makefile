############################################################################
# apps/netutils/xedge/Makefile
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed to the Apache Software Foundation (ASF) under one or more contributor
# license agreements.  See the NOTICE file distributed with this work for
# additional information regarding copyright ownership.  The ASF licenses this
# file to you under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License.  You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations under
# the License.
#
# ##############################################################################

include $(APPDIR)/Make.defs

BAS_UNPACKNAME = BAS
BAS_HASH = 9f74a2f778b002ad8441471b8a7a5b13172dbe76
BAS_RESOURCES_UNPACKNAME = BAS-Resources
BAS_RESOURCES_HASH = 227a4b998300fa4cfde871dc7dac92c09e1636c2

BAS_ZIP_URL = https://github.com/RealTimeLogic/BAS/archive/$(BAS_HASH).zip
BAS_RESOURCES_ZIP_URL = https://github.com/RealTimeLogic/BAS-Resources/archive/$(BAS_RESOURCES_HASH).zip

XEDGEZIP = BAS/examples/xedge/XedgeZip.c

BAS_SRC = $(BAS_UNPACKNAME)$(DELIM)src
BAS_ARCH_SRC = $(BAS_SRC)$(DELIM)arch$(DELIM)Posix
BAS_NET_SRC = $(BAS_SRC)$(DELIM)arch$(DELIM)NET$(DELIM)generic
BAS_DISKIO_SRC = $(BAS_SRC)$(DELIM)DiskIo$(DELIM)posix
BAS_XEDGE_SRC = $(BAS_UNPACKNAME)$(DELIM)examples$(DELIM)xedge$(DELIM)src

VPATH += $(BAS_SRC)
VPATH += $(BAS_ARCH_SRC)
VPATH += $(BAS_NET_SRC)
VPATH += $(BAS_DISKIO_SRC)
VPATH += $(BAS_XEDGE_SRC)
VPATH += $(BAS_UNPACKNAME)/examples/xedge

CSRCS = BAS.c dlmalloc.c ThreadLib.c SoDisp.c BaFile.c xedge.c XedgeZip.c

# Download and prepare BAS and BAS-Resources
xedge-deps:
	# ############################################################################
	# Config and Fetch xedge
	# ############################################################################

	@if [ ! -d $(BAS_UNPACKNAME) ]; then \
		echo "Downloading BAS from hash $(BAS_HASH)..."; \
		curl -f -L $(BAS_ZIP_URL) -o bas-temp.zip || \
		(echo "Error downloading BAS"; exit 1); \
		unzip -q bas-temp.zip; \
		mv BAS-$(BAS_HASH) $(BAS_UNPACKNAME); \
		rm -f bas-temp.zip; \
	fi

	@if [ ! -d $(BAS_RESOURCES_UNPACKNAME) ]; then \
		echo "Downloading BAS-Resources from hash $(BAS_RESOURCES_HASH)..."; \
		curl -f -L $(BAS_RESOURCES_ZIP_URL) -o resources-temp.zip || \
		(echo "Error downloading BAS-Resources"; exit 1); \
		unzip -q resources-temp.zip; \
		mv BAS-Resources-$(BAS_RESOURCES_HASH) $(BAS_RESOURCES_UNPACKNAME); \
		rm -f resources-temp.zip; \
	fi

	# ############################################################################
	# Library Configuration
	# ############################################################################

	@if [ ! -f "$(XEDGEZIP)" ]; then \
		echo "Creating XedgeZip.c"; \
		cd $(BAS_RESOURCES_UNPACKNAME)/build/ && \
		printf "n\nl\nn\n" | bash Xedge.sh > /dev/null && \
		cp XedgeZip.c ../../$(BAS_UNPACKNAME)/examples/xedge/ || exit 1; \
	fi

$(CSRCS:.c=$(OBJEXT)): xedge-deps

ifeq ($(wildcard $(BAS_UNPACKNAME)/.git),)
distclean:: xedge-deps
	$(call DELDIR, $(BAS_UNPACKNAME))
	$(call DELDIR, $(BAS_RESOURCES_UNPACKNAME))

context:: xedge-deps
endif

include $(APPDIR)/Application.mk