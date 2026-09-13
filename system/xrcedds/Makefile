############################################################################
# apps/system/xrcedds/Makefile
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

XRCEDDS_VERSION    := 3.0.1
MICROCDR_VERSION   := 2.0.2
XRCEDDS_TARBALL    := Micro-XRCE-DDS-Client-$(XRCEDDS_VERSION).tar.gz
MICROCDR_TARBALL   := Micro-CDR-$(MICROCDR_VERSION).tar.gz
XRCEDDS_UNPACK     := Micro-XRCE-DDS-Client
MICROCDR_UNPACK    := Micro-CDR
XRCEDDS_CSRC       := $(XRCEDDS_UNPACK)$(DELIM)src$(DELIM)c
MICROCDR_CSRC      := $(MICROCDR_UNPACK)$(DELIM)src$(DELIM)c
XRCEDDS_URL        := https://github.com/eProsima/Micro-XRCE-DDS-Client/archive
MICROCDR_URL       := https://github.com/eProsima/Micro-CDR/archive

CFLAGS += ${INCDIR_PREFIX}"$(XRCEDDS_CSRC)"

XRCEDDS_CORE := $(XRCEDDS_CSRC)$(DELIM)core
XRCEDDS_UTIL := $(XRCEDDS_CSRC)$(DELIM)util
XRCEDDS_SESSION := $(XRCEDDS_CORE)$(DELIM)session
XRCEDDS_STREAM := $(XRCEDDS_SESSION)$(DELIM)stream
XRCEDDS_TRANSPORT := $(XRCEDDS_CSRC)$(DELIM)profile$(DELIM)transport
XRCEDDS_UDP := $(XRCEDDS_TRANSPORT)$(DELIM)ip$(DELIM)udp

CSRCS += $(XRCEDDS_CORE)$(DELIM)serialization$(DELIM)xrce_header.c
CSRCS += $(XRCEDDS_CORE)$(DELIM)serialization$(DELIM)xrce_subheader.c
CSRCS += $(XRCEDDS_CORE)$(DELIM)serialization$(DELIM)xrce_types.c
CSRCS += $(XRCEDDS_SESSION)$(DELIM)common_create_entities.c
CSRCS += $(XRCEDDS_SESSION)$(DELIM)create_entities_bin.c
CSRCS += $(XRCEDDS_SESSION)$(DELIM)create_entities_ref.c
CSRCS += $(XRCEDDS_SESSION)$(DELIM)create_entities_xml.c
CSRCS += $(XRCEDDS_SESSION)$(DELIM)object_id.c
CSRCS += $(XRCEDDS_SESSION)$(DELIM)read_access.c
CSRCS += $(XRCEDDS_SESSION)$(DELIM)session.c
CSRCS += $(XRCEDDS_SESSION)$(DELIM)session_info.c
CSRCS += $(XRCEDDS_SESSION)$(DELIM)submessage.c
CSRCS += $(XRCEDDS_SESSION)$(DELIM)write_access.c
CSRCS += $(XRCEDDS_STREAM)$(DELIM)input_best_effort_stream.c
CSRCS += $(XRCEDDS_STREAM)$(DELIM)input_reliable_stream.c
CSRCS += $(XRCEDDS_STREAM)$(DELIM)output_best_effort_stream.c
CSRCS += $(XRCEDDS_STREAM)$(DELIM)output_reliable_stream.c
CSRCS += $(XRCEDDS_STREAM)$(DELIM)seq_num.c
CSRCS += $(XRCEDDS_STREAM)$(DELIM)stream_id.c
CSRCS += $(XRCEDDS_STREAM)$(DELIM)stream_storage.c
CSRCS += $(XRCEDDS_UTIL)$(DELIM)ping.c
CSRCS += $(XRCEDDS_UTIL)$(DELIM)time.c

CSRCS += $(XRCEDDS_UDP)$(DELIM)udp_transport.c
CSRCS += $(XRCEDDS_UDP)$(DELIM)udp_transport_posix.c

ifeq ($(CONFIG_SYSTEM_XRCEDDS_STREAM_FRAMING),y)
XRCEDDS_FRAMING := $(XRCEDDS_TRANSPORT)$(DELIM)stream_framing
CSRCS += $(XRCEDDS_FRAMING)$(DELIM)stream_framing_protocol.c
endif

CSRCS += $(MICROCDR_CSRC)$(DELIM)common.c
CSRCS += $(MICROCDR_CSRC)$(DELIM)types$(DELIM)array.c
CSRCS += $(MICROCDR_CSRC)$(DELIM)types$(DELIM)basic.c
CSRCS += $(MICROCDR_CSRC)$(DELIM)types$(DELIM)sequence.c
CSRCS += $(MICROCDR_CSRC)$(DELIM)types$(DELIM)string.c

$(XRCEDDS_TARBALL):
	$(Q) echo "Downloading Micro XRCE-DDS Client $(XRCEDDS_VERSION)"
	$(Q) curl -L -o $@ $(XRCEDDS_URL)/refs/tags/v$(XRCEDDS_VERSION).tar.gz

$(MICROCDR_TARBALL):
	$(Q) echo "Downloading Micro-CDR $(MICROCDR_VERSION)"
	$(Q) curl -L -o $@ $(MICROCDR_URL)/refs/tags/v$(MICROCDR_VERSION).tar.gz

$(XRCEDDS_UNPACK): $(XRCEDDS_TARBALL)
	$(Q) tar zxf $<
	$(Q) mv Micro-XRCE-DDS-Client-$(XRCEDDS_VERSION) $@

$(MICROCDR_UNPACK): $(MICROCDR_TARBALL)
	$(Q) tar zxf $<
	$(Q) mv Micro-CDR-$(MICROCDR_VERSION) $@

context:: $(XRCEDDS_UNPACK) $(MICROCDR_UNPACK)

distclean::
	$(call DELFILE,$(XRCEDDS_TARBALL))
	$(call DELFILE,$(MICROCDR_TARBALL))
	$(call DELDIR,$(XRCEDDS_UNPACK))
	$(call DELDIR,$(MICROCDR_UNPACK))

include $(APPDIR)/Application.mk
