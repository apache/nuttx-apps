############################################################################
# apps/examples/xedge/Make.defs
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

# Xedge, World! built-in application info

PROGNAME  = $(CONFIG_EXAMPLES_XEDGE_PROGNAME)
PRIORITY  = $(CONFIG_EXAMPLES_XEDGE_PRIORITY)
STACKSIZE = $(CONFIG_EXAMPLES_XEDGE_STACKSIZE)
MODULE    = $(CONFIG_EXAMPLES_XEDGE)

# Repository names and specific hashes
BAS_UNPACKNAME = BAS
BAS_HASH = 9f74a2f778b002ad8441471b8a7a5b13172dbe76
BAS_RESOURCES_UNPACKNAME = BAS-Resources
BAS_RESOURCES_HASH = 227a4b998300fa4cfde871dc7dac92c09e1636c2

# Download URLs
BAS_ZIP_URL = https://github.com/RealTimeLogic/BAS/archive/$(BAS_HASH).zip
BAS_RESOURCES_ZIP_URL = https://github.com/RealTimeLogic/BAS-Resources/archive/$(BAS_RESOURCES_HASH).zip

# Xedge: BAS Example

MAINSRC = xedge_main.c BAS/src/BAS.c BAS/src/dlmalloc.c BAS/src/arch/Posix/ThreadLib.c BAS/src/arch/NET/generic/SoDisp.c BAS/src/DiskIo/posix/BaFile.c BAS/examples/xedge/src/xedge.c BAS/examples/xedge/ BAS/examples/xedge/XedgeZip.c

XEDGEZIP = BAS/examples/xedge/XedgeZip.c

xedge-repos:

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

context:: xedge-repos

distclean::
	$(call DELDIR, $(BAS_UNPACKNAME))
	$(call DELDIR, $(BAS_RESOURCES_UNPACKNAME))

include $(APPDIR)/Application.mk