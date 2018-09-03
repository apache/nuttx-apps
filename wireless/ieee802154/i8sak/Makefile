############################################################################
# apps/wireless/ieee802154/i8sak/Makefile
#
#   Copyright (C) 2014-2015 Gregory Nutt. All rights reserved.
#   Copyright (C) 2014-2015 Sebastien Lorquet. All rights reserved.
#   Author: Sebastien Lorquet <sebastien@lorquet.fr>
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

# IEEE 802.15.4  SAK (swiss army knife )

CONFIG_IEEE802154_I8SAK_PROGNAME ?= i8ask$(EXEEXT)
CONFIG_IEEE802154_I8SAK_PRIORITY ?= 100
CONFIG_IEEE802154_I8SAK_STACKSIZE ?= 4096

PROGNAME = $(CONFIG_IEEE802154_I8SAK_PROGNAME)
APPNAME = i8ask
PRIORITY = $(CONFIG_IEEE802154_I8SAK_PRIORITY)
STACKSIZE = $(CONFIG_IEEE802154_I8SAK_STACKSIZE)

# IEEE 802.15.4 SAK (Swiss Army Knife)

CSRCS = i8sak_acceptassoc.c i8sak_assoc.c i8sak_scan.c i8sak_blaster.c i8sak_poll.c
CSRCS += i8sak_sniffer.c i8sak_startpan.c i8sak_tx.c i8sak_get.c i8sak_set.c
CSRCS += i8sak_reset.c i8sak_regdump.c i8sak_events.c
MAINSRC = i8sak_main.c

MODULE = CONFIG_IEEE802154_I8SAK

include $(APPDIR)/Application.mk
