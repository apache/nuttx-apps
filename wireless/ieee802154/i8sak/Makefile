############################################################################
# apps/wireless/ieee802154/i8sak/Makefile
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

# IEEE 802.15.4  SAK (swiss army knife )

PROGNAME = $(CONFIG_IEEE802154_I8SAK_PROGNAME)
PRIORITY = $(CONFIG_IEEE802154_I8SAK_PRIORITY)
STACKSIZE = $(CONFIG_IEEE802154_I8SAK_STACKSIZE)
MODULE = $(CONFIG_IEEE802154_I8SAK)

# IEEE 802.15.4 SAK (Swiss Army Knife)

CSRCS = i8sak_acceptassoc.c i8sak_assoc.c i8sak_scan.c i8sak_blaster.c i8sak_poll.c
CSRCS += i8sak_sniffer.c i8sak_startpan.c i8sak_tx.c i8sak_get.c i8sak_set.c
CSRCS += i8sak_reset.c i8sak_regdump.c i8sak_events.c
MAINSRC = i8sak_main.c

include $(APPDIR)/Application.mk
