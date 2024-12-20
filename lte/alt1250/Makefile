############################################################################
# apps/lte/alt1250/Makefile
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

# alt1250 command

PROGNAME = $(CONFIG_LTE_ALT1250_PROGNAME)
PRIORITY = $(CONFIG_LTE_ALT1250_PRIORITY)
STACKSIZE = $(CONFIG_LTE_ALT1250_STACKSIZE)
MODULE = $(CONFIG_LTE_ALT1250)

VPATH += callback_handlers usock_handlers
CSRCS =

# Framework source files

CSRCS += alt1250_container.c
CSRCS += alt1250_devevent.c
CSRCS += alt1250_devif.c
CSRCS += alt1250_select.c
CSRCS += alt1250_socket.c
CSRCS += alt1250_usockevent.c
CSRCS += alt1250_usockif.c
CSRCS += alt1250_netdev.c
CSRCS += alt1250_util.c
CSRCS += alt1250_reset_seq.c
CSRCS += alt1250_atcmd.c

# Callback task sources

CSRCS += alt1250_evt.c

# Usersock request handling source files

CSRCS += alt1250_sockethdlr.c
CSRCS += alt1250_closehdlr.c
CSRCS += alt1250_connecthdlr.c
CSRCS += alt1250_sendtohdlr.c
CSRCS += alt1250_recvfromhdlr.c
CSRCS += alt1250_setsockopthdlr.c
CSRCS += alt1250_getsockopthdlr.c
CSRCS += alt1250_getsocknamehdlr.c
CSRCS += alt1250_getpeernamehdlr.c
CSRCS += alt1250_bindhdlr.c
CSRCS += alt1250_listenhdlr.c
CSRCS += alt1250_accepthdlr.c
CSRCS += alt1250_ioctlhdlr.c
CSRCS += alt1250_shutdownhdlr.c
CSRCS += alt1250_sms.c

# Usersock ioctl LAPI command handling source files

CSRCS += alt1250_ioctl_ltecmd.c
CSRCS += alt1250_ioctl_ifreq.c
CSRCS += alt1250_ioctl_normal.c
CSRCS += alt1250_ioctl_event.c
CSRCS += alt1250_ioctl_other.c
CSRCS += alt1250_ioctl_power.c
CSRCS += alt1250_ioctl_fwupdate.c
CSRCS += alt1250_ioctl_denyinetsock.c
CSRCS += alt1250_ioctl_lwm2m.c

MAINSRC = alt1250_main.c

CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/lte/alt1250
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/lte/alt1250/callback_handlers
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/lte/alt1250/usock_handlers

include $(APPDIR)/Application.mk
