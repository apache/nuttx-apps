############################################################################
# apps/examples/pf_ieee802154/Makefile
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

# PF_IEEE802154 Socket Test

CSRCS = pf_cmdline.c

MAINSRC = pf_server.c pf_client.c
PROGNAME = pfserver pfclient

PRIORITY = $(CONFIG_EXAMPLES_PFIEEE802154_PRIORITY1)
STACKSIZE = $(CONFIG_EXAMPLES_PFIEEE802154_STACKSIZE1)

PRIORITY += $(CONFIG_EXAMPLES_PFIEEE802154_PRIORITY2)
STACKSIZE += $(CONFIG_EXAMPLES_PFIEEE802154_STACKSIZE2)

MODULE = $(CONFIG_EXAMPLES_PFIEEE802154)

include $(APPDIR)/Application.mk
