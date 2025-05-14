############################################################################
# apps/netutils/paho_mqtt/Makefile
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

PAHO_MQTT_URL     ?= "https://github.com/eclipse-paho/paho.mqtt.c/archive"

# Default version if not specified in config
ifdef CONFIG_NETUTILS_PAHO_MQTT_VERSION
PAHO_MQTT_VERSION := $(patsubst "%",%,$(CONFIG_NETUTILS_PAHO_MQTT_VERSION))
else
PAHO_MQTT_VERSION := 1.3.15
endif

PAHO_MQTT_ZIP     = v$(PAHO_MQTT_VERSION).zip
PAHO_MQTT_UNPACK  = paho_mqtt

SRCDIR = $(APPDIR)/netutils/paho_mqtt/$(PAHO_MQTT_UNPACK)/src

ifeq ($(CONFIG_LIB_MQTT5), y)

# Check if paho_mqtt directory exists, if not download and extract
ifeq ($(wildcard $(PAHO_MQTT_UNPACK)/src),)
$(PAHO_MQTT_ZIP):
	$(Q) echo "Downloading paho.mqtt.c-$(PAHO_MQTT_VERSION)"
	$(Q) curl -L -o $(PAHO_MQTT_ZIP) $(PAHO_MQTT_URL)/$(PAHO_MQTT_ZIP)

$(PAHO_MQTT_UNPACK): $(PAHO_MQTT_ZIP)
	$(Q) echo "Unpacking: $(PAHO_MQTT_ZIP) -> $(PAHO_MQTT_UNPACK)"
	$(Q) unzip -q $(PAHO_MQTT_ZIP)
	$(Q) mv paho.mqtt.c-$(PAHO_MQTT_VERSION) $(PAHO_MQTT_UNPACK)
	$(Q) rm -f $(PAHO_MQTT_ZIP)
	$(Q) if [ -f paho_mqtt_01.patch ]; then \
		echo "Applying paho_mqtt_01 patch to $(PAHO_MQTT_UNPACK)"; \
		cd $(PAHO_MQTT_UNPACK) && patch -p1 --forward --ignore-whitespace < ../paho_mqtt_01.patch || true; \
	fi
	$(Q) touch $(PAHO_MQTT_UNPACK)

context:: $(PAHO_MQTT_UNPACK)

distclean::
	$(call DELFILE, $(PAHO_MQTT_ZIP))
	$(call DELDIR, $(PAHO_MQTT_UNPACK))
endif

ifeq ($(CONFIG_OPENSSL_MBEDTLS_WRAPPER), y)
SKIP  = $(SRCDIR)/SHA1.c
endif

SKIP += $(SRCDIR)/MQTTClient.c
SKIP += $(SRCDIR)/MQTTVersion.c

CSRCS = $(filter-out $(SKIP), $(wildcard $(SRCDIR)/*.c))

ifeq ($(CONFIG_UTILS_MQTT5),y)
CSRCS += $(SRCDIR)/samples/pubsub_opts.c
endif

VARS  = BUILD_TIMESTAMP PROJECT_VERSION PROJECT_VERSION_MAJOR
VARS += PROJECT_VERSION_MINOR PROJECT_VERSION_PATCH

MQTT5_VERSION = $(SRCDIR)/VersionInfo.h

SED_COMMANDS = $(foreach var,$(VARS),-e 's/@$(var)@/$($(var))/g')

$(MQTT5_VERSION): $(SRCDIR)/VersionInfo.h.in
	sed $(SED_COMMANDS) $< > $@

context:: $(MQTT5_VERSION)

distclean::
	$(call DELFILE, $(MQTT5_VERSION))

ifeq ($(CONFIG_UTILS_MQTT5), y)
PROGNAME  = mqtt_pub mqtt_sub
MAINSRC   = $(SRCDIR)/samples/paho_c_pub.c $(SRCDIR)/samples/paho_c_sub.c
PRIORITY  = $(CONFIG_UTILS_MQTT5_PRIORITY)
STACKSIZE = $(CONFIG_UTILS_MQTT5_STACKSIZE)
MODULE = $(CONFIG_UTILS_MQTT5)
$(SRCDIR)/samples/paho_c_pub.c_CFLAGS += -DmessageArrived=mqtt_pub_messageArrived
$(SRCDIR)/samples/paho_c_pub.c_CFLAGS += -DonDisconnect=mqtt_pub_onDisconnect
$(SRCDIR)/samples/paho_c_pub.c_CFLAGS += -DonConnectFailure5=mqtt_pub_onConnectFailure5
$(SRCDIR)/samples/paho_c_pub.c_CFLAGS += -DonConnectFailure=mqtt_pub_onConnectFailure
$(SRCDIR)/samples/paho_c_pub.c_CFLAGS += -DonConnect5=mqtt_pub_onConnect5
$(SRCDIR)/samples/paho_c_pub.c_CFLAGS += -DonConnect=mqtt_pub_onConnect
$(SRCDIR)/samples/paho_c_pub.c_CFLAGS += -Dmysleep=mqtt_pub_mysleep -DtoStop=mqtt_pub_toStop
$(SRCDIR)/samples/paho_c_pub.c_CFLAGS += -Dopts=mqtt_pub_opts -Dmyconnect=mqtt_pub_myconnect
$(SRCDIR)/samples/paho_c_pub.c_CFLAGS += -Dcfinish=mqtt_pub_cfinish
$(SRCDIR)/samples/paho_c_pub.c_CFLAGS += -Dtrace_callback=mqtt_pub_trace_callback
endif

endif

include $(APPDIR)/Application.mk
