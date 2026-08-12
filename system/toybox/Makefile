############################################################################
# apps/system/toybox/Makefile
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

ifneq ($(CONFIG_SYSTEM_TOYBOX),)

# Download (fetch a pinned release, don't vendor)

TOYBOX_VERSION    := $(patsubst "%",%,$(strip $(CONFIG_SYSTEM_TOYBOX_VERSION)))
TOYBOX_URL        := $(patsubst "%",%,$(strip $(CONFIG_SYSTEM_TOYBOX_URL)))
TOYBOX_TARBALL    := $(TOYBOX_VERSION).tar.gz
TOYBOX_UNPACKNAME := toybox

WD := ${shell echo $(CURDIR) | sed -e 's/ /\\ /g'}

CURL   ?= curl -L -o
UNPACK ?= tar xzf
ifeq ($(CONFIG_HOST_MACOS),y)
PATCH  ?= gpatch
else
PATCH  ?= patch
endif

TOYBOX_PATCHES := $(sort $(wildcard $(WD)$(DELIM)patch$(DELIM)*.patch))

$(TOYBOX_TARBALL):
	$(ECHO_BEGIN)"Downloading: $(TOYBOX_TARBALL)"
	$(Q) $(CURL) $(TOYBOX_TARBALL) $(TOYBOX_URL)/$(TOYBOX_TARBALL)
	$(ECHO_END)

$(TOYBOX_UNPACKNAME): $(TOYBOX_TARBALL)
	$(ECHO_BEGIN)"Unpacking: $(TOYBOX_TARBALL) -> $(TOYBOX_UNPACKNAME)"
	$(Q) $(UNPACK) $(TOYBOX_TARBALL)
	$(Q) rm -rf $(TOYBOX_UNPACKNAME)
	$(Q) mv toybox-$(TOYBOX_VERSION) $(TOYBOX_UNPACKNAME)
	$(ECHO_END)
	# Release tarballs preserve their original commit timestamps (long
	# before "now"). Any file a patch doesn't touch keeps that old mtime,
	# which can be *older* than .o/.a files left over from a previous
	# build of a different version/config -- make's mtime-based dependency
	# tracking then considers those stale objects still up to date and
	# silently keeps the old compiled code. Touching everything to the
	# current time up front avoids that regardless of patch coverage.
	$(Q) find $(TOYBOX_UNPACKNAME) -exec touch {} +
	$(Q) $(foreach p, $(TOYBOX_PATCHES), \
		echo "Applying: $(notdir $(p))"; \
		$(PATCH) -s -N -p1 -d $(TOYBOX_UNPACKNAME) -i $(p) || exit 1;)
	$(Q) touch $(TOYBOX_UNPACKNAME)

# Regenerating headers is its own target, depending on $(TOPDIR)/.config as
# well as the unpacked source: unlike download/unpack/patch (genuinely
# one-time -- nothing above depends on *what* is selected, only that some
# version is unpacked), which applets end up in generated/toyfiles.txt (and
# so CSRCS) is a direct function of the "Toybox commands" Kconfig menu.
# Without this as a separate, .config-dependent target, toggling a command
# in menuconfig would have no effect: $(TOYBOX_UNPACKNAME) already exists,
# so its rule (and this regeneration) would never run again.
$(TOYBOX_UNPACKNAME)/generated/toyfiles.txt: $(TOYBOX_UNPACKNAME) $(TOPDIR)$(DELIM).config
	# Which applets to build is a Kconfig question (see Kconfig.commands,
	# "Toybox commands" submenu), not a separately-maintained list: pull
	# every enabled CONFIG_SYSTEM_TOYBOX_CMD_<NAME> straight out of NuttX's
	# own .config and translate it to the CONFIG_<NAME>=y form Toybox's own
	# genconfig.sh expects (via KCONFIG_ALLCONFIG, same mechanism upstream
	# uses for scripts/android_miniconfig). The CMD_ segment (see
	# Kconfig.commands) is what keeps this from also matching this
	# directory's other SYSTEM_TOYBOX_* options (VERSION, BUILTIN_BRIDGE,
	# ...), which aren't applets and aren't valid Toybox Kconfig symbols.
	$(Q) sed -n 's/^CONFIG_SYSTEM_TOYBOX_CMD_\([A-Z0-9_]*\)=y$$/CONFIG_\1=y/p' \
		$(TOPDIR)$(DELIM).config > $(WD)$(DELIM).toybox_allconfig
	$(Q) echo CONFIG_TOYBOX_FLOAT=y          >> $(WD)$(DELIM).toybox_allconfig
	$(Q) echo CONFIG_TOYBOX_HELP=y           >> $(WD)$(DELIM).toybox_allconfig
	$(Q) echo CONFIG_TOYBOX_HELP_DASHDASH=y  >> $(WD)$(DELIM).toybox_allconfig
	$(Q) echo CONFIG_TOYBOX_NORECURSE=y      >> $(WD)$(DELIM).toybox_allconfig
	# Targets without CONFIG_ARCH_HAVE_FORK (e.g. flat/nommu builds like
	# stm32f4discovery) have no real fork(), only vfork() -- tell Toybox's
	# own Kconfig via the same TOYBOX_FORCE_NOMMU it already offers nommu
	# Linux builds for, or xfork()'s call to fork() is left dangling at
	# link time on targets that never defined the symbol.
ifneq ($(CONFIG_ARCH_HAVE_FORK),y)
	$(Q) echo CONFIG_TOYBOX_FORCE_NOMMU=y    >> $(WD)$(DELIM).toybox_allconfig
endif
	$(ECHO_BEGIN)"Generating Toybox headers for NuttX ($(TOYBOX_VERSION))"
	$(Q) cd $(TOYBOX_UNPACKNAME) && \
		CC="$(HOSTCC)" HOSTCC="$(HOSTCC)" CROSS_COMPILE= \
		KCONFIG_ALLCONFIG=$(WD)$(DELIM).toybox_allconfig \
		bash scripts/genconfig.sh -n
	# scripts/make.sh *appends* its "FILES=..." build recipe to
	# generated/build.sh instead of overwriting it (upstream only expects
	# this to run once per checkout); since we deliberately re-run it
	# every time the "Toybox commands" selection changes, remove the
	# previous run's block first or the awk extraction below picks up
	# stale (possibly no longer selected) applets from it, not the
	# current one.
	$(Q) rm -f $(TOYBOX_UNPACKNAME)$(DELIM)generated$(DELIM)build.sh
	$(Q) cd $(TOYBOX_UNPACKNAME) && \
		CC="$(HOSTCC)" HOSTCC="$(HOSTCC)" CROSS_COMPILE= NOBUILD=1 \
		NUTTX_TOYBOX_MULTIPLEX_NAME=toybox_multiplex \
		bash scripts/make.sh
	$(Q) awk '/^FILES="/{sub(/^FILES="/,""); if ($$0!="") print; f=1; next} \
		f && /^"$$/{f=0; next} f' $(TOYBOX_UNPACKNAME)/generated/build.sh | \
		tr ' ' '\n' | grep -v '^$$' | grep -v '^main\.c$$' | sort -u \
		> $(TOYBOX_UNPACKNAME)/generated/toyfiles.txt
	$(ECHO_END)

ifeq ($(wildcard $(TOYBOX_UNPACKNAME)/.git),)
context:: $(TOYBOX_UNPACKNAME)/generated/toyfiles.txt
endif

# App registration

MODULE    = $(CONFIG_SYSTEM_TOYBOX)
PROGNAME  = toybox
PRIORITY  = $(CONFIG_SYSTEM_TOYBOX_PRIORITY)
STACKSIZE = $(CONFIG_SYSTEM_TOYBOX_STACKSIZE)

MAINSRC = $(TOYBOX_UNPACKNAME)/toybox_entry.c

# Sources: our glue + upstream lib/*.c + whatever's enabled in the
# "Toybox commands" Kconfig menu, discovered from toybox's own
#  generated/build.sh so enabling more applets is "select it in
#  menuconfig", not "edit CSRCS by hand".

CSRCS += $(TOYBOX_UNPACKNAME)/nuttx_bridge.c
CSRCS += $(TOYBOX_UNPACKNAME)/main.c
# All of lib/*.c, same as upstream's own scripts/make.sh ($BUILD lib/*.c
# $FILES ...) -- toybox's lib helpers call across files in ways that make
# curating a subset fragile (a missing lib/foo.c shows up as a link error
# for whatever helper it happened to provide, not a hint about foo.c).
CSRCS += $(wildcard $(TOYBOX_UNPACKNAME)/lib/*.c)
CSRCS += $(shell test -f $(TOYBOX_UNPACKNAME)/generated/toyfiles.txt && \
                 sed "s|^|$(TOYBOX_UNPACKNAME)/|" $(TOYBOX_UNPACKNAME)/generated/toyfiles.txt)

# ── Include paths / compat shim ───────────────────────────────────────────

CFLAGS += ${INCDIR_PREFIX}$(WD)$(DELIM)$(TOYBOX_UNPACKNAME)
CFLAGS += ${INCDIR_PREFIX}$(WD)$(DELIM)$(TOYBOX_UNPACKNAME)$(DELIM)generated
CFLAGS += ${INCDIR_PREFIX}$(WD)$(DELIM)$(TOYBOX_UNPACKNAME)$(DELIM)lib
CFLAGS += -include $(WD)$(DELIM)compat_nuttx.h

CFLAGS += -D__linux__
CFLAGS += -D__NuttX__
CFLAGS += -DTOYBOX_VERSION='"$(TOYBOX_VERSION)-nuttx"'

# Upstream Toybox coding conventions NuttX's stricter default warning set
# (-Wall -Wshadow -Wstrict-prototypes, see arch/sim/src/sim/Toolchain.defs
# and equivalents) flags throughout the tree, on any platform -- reusing a
# scratch variable name in a narrower scope, "char" as an array index
# (values used this way are always small/non-negative in practice), and
# "void foo()" instead of "void foo(void)". None of these are NuttX
# portability issues, so they're silenced here rather than the alternative
# of patching dozens of unrelated upstream files to work around pedantic
# warnings this port doesn't otherwise need to touch.
CFLAGS += -Wno-shadow
CFLAGS += -Wno-char-subscripts
CFLAGS += -Wno-strict-prototypes

# chmod.c's -v/-c output builds a message in toybuf while also formatting
# a mode string into toybuf+64 (same object, disjoint region -- the message
# text is always well under 64 bytes) and passing that second pointer as a
# %s argument to sprintf()'s first: -Wrestrict can't see the offset is
# always big enough to avoid the actual overlap sprintf's restrict-qualified
# parameters forbid, so it flags the pattern rather than a real one.
CFLAGS += -Wno-restrict

ifneq ($(CONFIG_SYSTEM_TOYBOX_BUILTIN_BRIDGE),)
CFLAGS += -DCONFIG_SYSTEM_TOYBOX_BUILTIN_BRIDGE=1
endif

include $(APPDIR)/Application.mk

ifeq ($(wildcard $(TOYBOX_UNPACKNAME)/.git),)
distclean::
	$(call DELDIR, $(TOYBOX_UNPACKNAME))
	$(call DELFILE, $(TOYBOX_TARBALL))
endif

endif # CONFIG_SYSTEM_TOYBOX
