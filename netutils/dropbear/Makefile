############################################################################
# apps/netutils/dropbear/Makefile
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

ifneq ($(CONFIG_NETUTILS_DROPBEAR),)

DROPBEAR_COMMIT = $(patsubst "%",%,$(strip $(CONFIG_NETUTILS_DROPBEAR_COMMIT)))
DROPBEAR_ZIP = $(DROPBEAR_COMMIT).zip
DROPBEAR_UNPACKNAME = dropbear
DROPBEAR_URL = https://github.com/mkj/dropbear/archive
UNPACK ?= unzip -q -o
ifeq ($(CONFIG_HOST_MACOS),y)
PATCH ?= gpatch
else
PATCH ?= patch
endif

MODULE = $(CONFIG_NETUTILS_DROPBEAR)
PROGNAME = $(CONFIG_NETUTILS_DROPBEAR_PROGNAME)
PRIORITY = $(CONFIG_NETUTILS_DROPBEAR_PRIORITY)
STACKSIZE = $(CONFIG_NETUTILS_DROPBEAR_STACKSIZE)

ifneq ($(CONFIG_NETUTILS_DROPBEAR_SCP),)
PROGNAME += scp
PRIORITY += $(CONFIG_NETUTILS_DROPBEAR_SCP_PRIORITY)
STACKSIZE += $(CONFIG_NETUTILS_DROPBEAR_SCP_STACKSIZE)
endif

CFLAGS += ${INCDIR_PREFIX}"$(APPDIR)$(DELIM)netutils$(DELIM)dropbear"
CFLAGS += ${INCDIR_PREFIX}"$(APPDIR)$(DELIM)netutils$(DELIM)dropbear$(DELIM)port"
CFLAGS += ${INCDIR_PREFIX}"$(APPDIR)$(DELIM)netutils$(DELIM)dropbear$(DELIM)dropbear"
CFLAGS += ${INCDIR_PREFIX}"$(APPDIR)$(DELIM)netutils$(DELIM)dropbear$(DELIM)dropbear$(DELIM)src"
CFLAGS += ${INCDIR_PREFIX}"$(APPDIR)$(DELIM)netutils$(DELIM)dropbear$(DELIM)dropbear$(DELIM)libtomcrypt$(DELIM)src$(DELIM)headers"
CFLAGS += ${INCDIR_PREFIX}"$(APPDIR)$(DELIM)netutils$(DELIM)dropbear$(DELIM)dropbear$(DELIM)libtommath"
CFLAGS += ${INCDIR_PREFIX}"$(APPDIR)$(DELIM)nshlib"

ifneq ($(CONFIG_NETUTILS_DROPBEAR_COMPRESSION),)
CFLAGS += ${INCDIR_PREFIX}"$(APPDIR)$(DELIM)system$(DELIM)zlib$(DELIM)zlib"
endif

CFLAGS += ${DEFINE_PREFIX}LOCALOPTIONS_H_EXISTS=1
CFLAGS += ${DEFINE_PREFIX}DROPBEAR_NUTTX=1
CFLAGS += ${DEFINE_PREFIX}DROPBEAR_NUTTX_PASSWD=1
CFLAGS += -Wno-pointer-sign -Wno-format

dropbear_nshsession.c_CFLAGS += ${DEFINE_PREFIX}Channel=dropbear_channel
dropbear_nshsession.c_CFLAGS += ${DEFINE_PREFIX}ChanType=dropbear_chantype

CSRCS = dropbear_nshsession.c
CSRCS += port/nuttx_auth.c
CSRCS += port/dropbear_utils.c

CSRCS += \
	dropbear/src/dbutil.c \
	dropbear/src/buffer.c \
	dropbear/src/dbhelpers.c \
	dropbear/src/bignum.c \
	dropbear/src/signkey.c \
	dropbear/src/dbrandom.c \
	dropbear/src/queue.c \
	dropbear/src/atomicio.c \
	dropbear/src/compat.c \
	dropbear/src/fake-rfc2553.c \
	dropbear/src/curve25519.c \
	dropbear/src/chachapoly.c \
	dropbear/src/ltc_prng.c \
	dropbear/src/ecc.c \
	dropbear/src/ecdsa.c \
	dropbear/src/crypto_desc.c \
	dropbear/src/dbmalloc.c \
	dropbear/src/gensignkey.c \
	dropbear/src/common-session.c \
	dropbear/src/packet.c \
	dropbear/src/common-algo.c \
	dropbear/src/common-kex.c \
	dropbear/src/common-channel.c \
	dropbear/src/common-chansession.c \
	dropbear/src/termcodes.c \
	dropbear/src/tcp-accept.c \
	dropbear/src/listener.c \
	dropbear/src/process-packet.c \
	dropbear/src/common-runopts.c \
	dropbear/src/circbuffer.c \
	dropbear/src/list.c \
	dropbear/src/netio.c \
	dropbear/src/gcm.c \
	dropbear/src/kex-x25519.c \
	dropbear/src/svr-kex.c \
	dropbear/src/svr-auth.c \
	dropbear/src/svr-authpasswd.c \
	dropbear/src/svr-session.c \
	dropbear/src/svr-service.c \
	dropbear/src/svr-runopts.c \
	dropbear/src/svr-tcpfwd.c \
	dropbear/src/svr-forward.c \
	dropbear/src/svr-streamfwd.c \
	dropbear/src/svr-authpam.c

TOMMATH_SRCS = $(shell if [ -d "$(DROPBEAR_UNPACKNAME)/libtommath" ]; then find "$(DROPBEAR_UNPACKNAME)/libtommath" -name "*.c"; fi)
TOMCRYPT_SRCS = $(shell if [ -d "$(DROPBEAR_UNPACKNAME)/libtomcrypt/src" ]; then find "$(DROPBEAR_UNPACKNAME)/libtomcrypt/src" -name "*.c" ! -name "sober128tab.c"; fi)

# Replace the bundled chacha20-poly1305 with the /dev/crypto adapter.

CSRCS := $(filter-out dropbear/src/chachapoly.c,$(CSRCS))
CSRCS += port/dropbear_chachapoly.c

CSRCS += $(TOMMATH_SRCS)
CSRCS += $(TOMCRYPT_SRCS)

ifneq ($(CONFIG_NETUTILS_DROPBEAR_SCP),)
CSRCS += dropbear/src/scpmisc.c
CSRCS += port/nuttx_scp.c
endif

# Match the ESP-IDF port behavior: LTC_SOURCE is only for libtomcrypt sources.
$(foreach src,$(TOMCRYPT_SRCS),$(eval $(src)_CFLAGS += ${DEFINE_PREFIX}LTC_SOURCE=1))

# Avoid duplicate symbols when NuttX/apps also provide these APIs.
# Apply to all dropbear sources so libtomcrypt definitions and dropbear
# callers (e.g. signkey.c) use the same renamed symbols.
CFLAGS += ${DEFINE_PREFIX}base64_encode=dropbear_ltc_base64_encode
CFLAGS += ${DEFINE_PREFIX}base64_decode=dropbear_ltc_base64_decode
CFLAGS += ${DEFINE_PREFIX}ecc_make_key=dropbear_ltc_ecc_make_key

MAINSRC = dropbear_main.c

ifneq ($(CONFIG_NETUTILS_DROPBEAR_SCP),)
MAINSRC += dropbear/src/scp.c

dropbear/src/scp.c_CFLAGS += ${DEFINE_PREFIX}xmalloc=dropbear_scp_xmalloc
dropbear/src/scp.c_CFLAGS += ${DEFINE_PREFIX}xrealloc=dropbear_scp_xrealloc
dropbear/src/scp.c_CFLAGS += ${DEFINE_PREFIX}xfree=dropbear_scp_xfree
dropbear/src/scp.c_CFLAGS += ${DEFINE_PREFIX}execvp=dropbear_scp_execvp
dropbear/src/scp.c_CFLAGS += -include port/nuttx_scp.h
dropbear/src/scp.c_CFLAGS += -Wno-strict-prototypes

dropbear/src/scpmisc.c_CFLAGS += ${DEFINE_PREFIX}xmalloc=dropbear_scp_xmalloc
dropbear/src/scpmisc.c_CFLAGS += ${DEFINE_PREFIX}xrealloc=dropbear_scp_xrealloc
dropbear/src/scpmisc.c_CFLAGS += ${DEFINE_PREFIX}xfree=dropbear_scp_xfree
endif

$(DROPBEAR_ZIP):
	@echo "Downloading: $(DROPBEAR_ZIP)"
	$(Q) curl -L -o $(DROPBEAR_ZIP) $(DROPBEAR_URL)/$(DROPBEAR_ZIP)

$(DROPBEAR_UNPACKNAME): $(DROPBEAR_ZIP)
	@echo "Unpacking: $(DROPBEAR_ZIP) -> $(DROPBEAR_UNPACKNAME)"
	$(Q) $(UNPACK) $(DROPBEAR_ZIP)
	$(Q) rm -rf $(DROPBEAR_UNPACKNAME)
	$(Q) mv dropbear-$(DROPBEAR_COMMIT) $(DROPBEAR_UNPACKNAME)
	@echo "Patching $(DROPBEAR_UNPACKNAME)"
	$(Q) $(PATCH) -s -N -l -p1 -d $(DROPBEAR_UNPACKNAME) -i $(APPDIR)$(DELIM)netutils$(DELIM)dropbear$(DELIM)patch$(DELIM)0001-guard-platform-declarations.patch; true
	$(Q) $(PATCH) -s -N -l -p1 -d $(DROPBEAR_UNPACKNAME) -i $(APPDIR)$(DELIM)netutils$(DELIM)dropbear$(DELIM)patch$(DELIM)0002-use-nuttx-passwd-auth.patch; true
	$(Q) $(PATCH) -s -N -l -p1 -d $(DROPBEAR_UNPACKNAME) -i $(APPDIR)$(DELIM)netutils$(DELIM)dropbear$(DELIM)patch$(DELIM)0003-allow-localoptions-to-override-tracking-malloc.patch; true
	$(Q) $(PATCH) -s -N -l -p1 -d $(DROPBEAR_UNPACKNAME) -i $(APPDIR)$(DELIM)netutils$(DELIM)dropbear$(DELIM)patch$(DELIM)0004-use-nuttx-unused-macro.patch; true
	@echo "Generating default_options_guard.h"
	$(Q) sh $(DROPBEAR_UNPACKNAME)$(DELIM)src$(DELIM)ifndef_wrapper.sh \
		< $(DROPBEAR_UNPACKNAME)$(DELIM)src$(DELIM)default_options.h \
		> $(DROPBEAR_UNPACKNAME)$(DELIM)src$(DELIM)default_options_guard.h
	$(Q) touch $(DROPBEAR_UNPACKNAME)

ifeq ($(wildcard $(DROPBEAR_UNPACKNAME)/.git),)
context:: $(DROPBEAR_UNPACKNAME)

clean::
	$(call DELFILE, port$(DELIM)*.o)

distclean::
	$(call DELDIR, $(DROPBEAR_UNPACKNAME))
	$(call DELFILE, $(DROPBEAR_ZIP))
endif

endif

include $(APPDIR)/Application.mk
