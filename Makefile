############################################################################
# apps/Makefile
#
#   Copyright (C) 2011 Uros Platise. All rights reserved.
#   Author: Uros Platise <uros.platise@isotel.eu>
#           Gregory Nutt <spudmonkey@racsa.co.cr>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
# 3. Neither the name NuttX nor the names of its contributors may be
#    used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
# OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
# AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
############################################################################

-include $(TOPDIR)/Make.defs

APPDIR = ${shell pwd}

# Application Directories

# SUBDIRS is the list of all directories containing Makefiles.  It is used
# only for cleaning. nuttapp must always be the first in the list.

SUBDIRS = nuttapp nshlib netutils examples vsn

# We use a non-existing .built_always to guarantee that Makefile always walks
# into the sub-directories and asks for build.  NOTE that nuttapp is always
# in the list of applications to be built

BUILTIN_APPS_BUILT = nuttapp/.built_always
BUILTIN_APPS_DIR = nuttapp

# CONFIGURED_APPS is the list of all configured built-in directories/built action
# It is created by the configured appconfig file (a copy of which appears in this
# directoy as .config)

CONFIGURED_APPS =
-include .config

# AVAILABLE_APPS is the list of currently available application directories.  It
# is the same as CONFIGURED_APPS, but filtered to exclude any non-existent apps

AVAILABLE_APPS =

define ADD_AVAILABLE
AVAILABLE_APPS += ${shell DIR=`echo $1 | cut -d'=' -f1`; if [ -r $$DIR/Makefile ]; then echo "$1"; fi}
endef

define BUILTIN_ADD_APP
BUILTIN_APPS_DIR += ${shell echo $1 | cut -d'=' -f1}
endef

define BUILTIN_ADD_BUILT
BUILTIN_APPS_BUILT += ${shell echo $1 | sed -e "s:=:/:g"}
endef

# (1) Create the list of available applications (AVAILABLE_APPS), (2) Add each
# available app to the list of  to build (BUILTIN_APPS_DIR), and (3) Add the
# "built" indication for each app (BUILTIN_APPS_BUILT). 

$(foreach BUILTIN, $(CONFIGURED_APPS), $(eval $(call ADD_AVAILABLE,$(BUILTIN))))
$(foreach APP, $(AVAILABLE_APPS), $(eval $(call BUILTIN_ADD_APP,$(APP))))
$(foreach BUILT, $(AVAILABLE_APPS), $(eval $(call BUILTIN_ADD_BUILT,$(BUILT))))

# The final build target

BIN		= libapps$(LIBEXT)

# Build targets

all: $(BIN)
.PHONY: $(BUILTIN_APPS_BUILT) .depend depend clean distclean

$(BUILTIN_APPS_BUILT):
	@for dir in $(BUILTIN_APPS_DIR) ; do \
		$(MAKE) -C $$dir TOPDIR="$(TOPDIR)" APPDIR=$(APPDIR); \
	done

$(BIN):	$(BUILTIN_APPS_BUILT)
	@( for obj in $(OBJS) ; do \
		$(call ARCHIVE, $@, $${obj}); \
	done ; )

.depend: Makefile $(SRCS)
	@for dir in $(BUILTIN_APPS_DIR) ; do \
		rm -f $$dir/.depend ; \
		$(MAKE) -C $$dir TOPDIR="$(TOPDIR)"  APPDIR=$(APPDIR) depend ; \
	done
	@touch $@

depend: .depend

clean:
	@for dir in $(SUBDIRS) ; do \
		$(MAKE) -C $$dir clean TOPDIR="$(TOPDIR)" APPDIR=$(APPDIR); \
	done
	@rm -f $(BIN) *~ .*.swp *.o
	$(call CLEAN)

distclean: clean
	@for dir in $(SUBDIRS) ; do \
		$(MAKE) -C $$dir distclean TOPDIR="$(TOPDIR)" APPDIR=$(APPDIR); \
	done
	@rm -f .config .depend

