# ##############################################################################
# apps/lte/alt1250/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed to the Apache Software Foundation (ASF) under one or more contributor
# license agreements.  See the NOTICE file distributed with this work for
# additional information regarding copyright ownership.  The ASF licenses this
# file to you under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License.  You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations under
# the License.
#
# ##############################################################################

if(CONFIG_LTE_ALT1250)

  set(INCDIR
      ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_LIST_DIR}/callback_handlers
      ${CMAKE_CURRENT_LIST_DIR}/usock_handlers)

  # Application info

  nuttx_add_application(
    NAME
    ${CONFIG_LTE_ALT1250_PROGNAME}
    SRCS
    alt1250_main.c
    STACKSIZE
    ${CONFIG_LTE_ALT1250_STACKSIZE}
    PRIORITY
    ${CONFIG_LTE_ALT1250_PRIORITY}
    INCLUDE_DIRECTORIES
    ${INCDIR})

  set(CSRCS
      # Framework source files
      alt1250_container.c
      alt1250_devevent.c
      alt1250_devif.c
      alt1250_select.c
      alt1250_socket.c
      alt1250_usockevent.c
      alt1250_usockif.c
      alt1250_netdev.c
      alt1250_util.c
      alt1250_reset_seq.c
      alt1250_atcmd.c
      # Callback task sources
      callback_handlers/alt1250_evt.c
      # Usersock request handling source files
      usock_handlers/alt1250_sockethdlr.c
      usock_handlers/alt1250_closehdlr.c
      usock_handlers/alt1250_connecthdlr.c
      usock_handlers/alt1250_sendtohdlr.c
      usock_handlers/alt1250_recvfromhdlr.c
      usock_handlers/alt1250_setsockopthdlr.c
      usock_handlers/alt1250_getsockopthdlr.c
      usock_handlers/alt1250_getsocknamehdlr.c
      usock_handlers/alt1250_getpeernamehdlr.c
      usock_handlers/alt1250_bindhdlr.c
      usock_handlers/alt1250_listenhdlr.c
      usock_handlers/alt1250_accepthdlr.c
      usock_handlers/alt1250_ioctlhdlr.c
      usock_handlers/alt1250_shutdownhdlr.c
      usock_handlers/alt1250_sms.c
      # Usersock ioctl LAPI command handling source files
      usock_handlers/alt1250_ioctl_ltecmd.c
      usock_handlers/alt1250_ioctl_ifreq.c
      usock_handlers/alt1250_ioctl_normal.c
      usock_handlers/alt1250_ioctl_event.c
      usock_handlers/alt1250_ioctl_other.c
      usock_handlers/alt1250_ioctl_power.c
      usock_handlers/alt1250_ioctl_fwupdate.c
      usock_handlers/alt1250_ioctl_denyinetsock.c
      usock_handlers/alt1250_ioctl_lwm2m.c)

  nuttx_add_library(alt1250)
  target_sources(alt1250 PRIVATE ${CSRCS})
  target_include_directories(alt1250 PRIVATE ${INCDIR})
endif()
