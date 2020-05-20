############################################################################
# apps/wireless/ieee802154/libmac/Makefile
#
#   Copyright (C) 2017 Gregory Nutt. All rights reserved.
#   Author: Gregory Nutt <gnutt@nuttx.org>
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

include $(APPDIR)/Make.defs

# libmac source files

# Add IOCTL helpers
CSRCS  = ieee802154_assocreq.c ieee802154_assocresp.c ieee802154_disassocreq.c
CSRCS += ieee802154_getreq.c ieee802154_gtsreq.c ieee802154_orphanresp.c
CSRCS += ieee802154_resetreq.c ieee802154_rxenabreq.c  ieee802154_scanreq.c
CSRCS += ieee802154_setreq.c ieee802154_startreq.c ieee802154_syncreq.c
CSRCS += ieee802154_pollreq.c


ifeq ($(CONFIG_NET_6LOWPAN),y)
# Add IOCTL helpers
CSRCS += sixlowpan_assocreq.c sixlowpan_assocresp.c sixlowpan_disassocreq.c
CSRCS += sixlowpan_getreq.c sixlowpan_gtsreq.c sixlowpan_orphanresp.c
CSRCS += sixlowpan_resetreq.c sixlowpan_rxenabreq.c sixlowpan_scanreq.c
CSRCS += sixlowpan_setreq.c sixlowpan_startreq.c sixlowpan_syncreq.c
CSRCS += sixlowpan_pollreq.c
endif

include $(APPDIR)/Application.mk
