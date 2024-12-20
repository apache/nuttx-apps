############################################################################
# apps/netutils/mqttc/Makefile
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
	$(Q) echo "Patching $(MQTTC_UNPACK)"
	$(Q) cat 0001_add_connection_status.patch | patch -s -N -d $(MQTTC_UNPACK) -p1
	$(Q) touch $(MQTTC_UNPACK)

# Download and unpack tarball if no git repo found
ifeq ($(wildcard $(MQTTC_UNPACK)/.git),)
context:: $(MQTTC_UNPACK)

distclean::
	$(call DELFILE, $(MQTTC_TARBALL))
	$(call DELDIR, $(MQTTC_UNPACK))
endif

ifneq ($(CONFIG_NETUTILS_MQTTC_EXAMPLE),)
CFLAGS += -Wno-return-type

PRIORITY  = SCHED_PRIORITY_DEFAULT
STACKSIZE = $(CONFIG_NETUTILS_MQTTC_EXAMPLE_STACKSIZE)
MODULE    = $(CONFIG_NETUTILS_MQTTC_EXAMPLE)

# MQTT-C mbedtls example source code
ifneq ($(CONFIG_NETUTILS_MQTTC_WITH_MBEDTLS),)
PROGNAME  = mqttc_mbedtls_pub
MAINSRC   = $(MQTTC_UNPACK)$(DELIM)examples$(DELIM)mbedtls_publisher.c
else
PROGNAME  = mqttc_posix_pub
MAINSRC   = $(MQTTC_UNPACK)$(DELIM)examples$(DELIM)simple_publisher.c
$(MQTTC_UNPACK)$(DELIM)examples$(DELIM)simple_publisher.c_CFLAGS += \
            -Dopen_nb_socket=pub_open_nb_socket \
            -Dexit_example=pub_exit_example \
            -Dpublish_callback=pub_publish_callback \
            -Dclient_refresher=pub_client_refresher

PROGNAME += mqttc_posix_sub
MAINSRC  += $(MQTTC_UNPACK)$(DELIM)examples$(DELIM)simple_subscriber.c
$(MQTTC_UNPACK)$(DELIM)examples$(DELIM)simple_subscriber.c_CFLAGS += \
            -Dopen_nb_socket=sub_open_nb_socket \
            -Dexit_example=sub_exit_example \
            -Dpublish_callback=sub_publish_callback \
            -Dclient_refresher=sub_client_refresher
endif

endif

include $(APPDIR)/Application.mk
