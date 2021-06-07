############################################################################
# apps/wireless/ieee802154/libmac/Makefile
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
