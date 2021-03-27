############################################################################
# apps/netutils/mqttc/Makefile
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

MQTTC_VERSION := $(patsubst "%",%,$(CONFIG_NETUTILS_MQTTC_VERSION))
MQTTC_TARBALL  = $(MQTTC_VERSION).tar.gz
MQTTC_UNPACK   = MQTT-C
MQTTC_SRCDIR   = $(MQTTC_UNPACK)$(DELIM)src

DEPPATH += --dep-path $(MQTTC_SRCDIR)
VPATH   += :$(MQTTC_SRCDIR)

CSRCS := $(notdir $(wildcard $(MQTTC_SRCDIR)$(DELIM)*.c))

$(MQTTC_TARBALL):
	$(Q) echo "Downloading MQTT-C-$(MQTTC_VERSION)"
	$(Q) curl -O -L https://github.com/LiamBindle/MQTT-C/archive/$(MQTTC_TARBALL)

$(MQTTC_UNPACK): $(MQTTC_TARBALL)
	$(Q) tar zxf $(MQTTC_TARBALL)
	$(Q) mv MQTT-C-$(MQTTC_VERSION) $(MQTTC_UNPACK)
	$(Q) touch $(MQTTC_UNPACK)

context:: $(MQTTC_UNPACK)

distclean::
	$(call DELFILE, $(MQTTC_TARBALL))
	$(call DELDIR, $(MQTTC_UNPACK))

include $(APPDIR)/Application.mk
