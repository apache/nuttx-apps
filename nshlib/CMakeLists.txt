# ##############################################################################
# apps/nshlib/CMakeLists.txt
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

if(CONFIG_NSH_LIBRARY)

  set(CSRCS)

  list(
    APPEND
    CSRCS
    nsh_init.c
    nsh_parse.c
    nsh_console.c
    nsh_script.c
    nsh_system.c
    nsh_command.c
    nsh_fscmds.c
    nsh_proccmds.c
    nsh_mmcmds.c
    nsh_timcmds.c
    nsh_envcmds.c
    nsh_prompt.c
    nsh_syscmds.c
    nsh_dbgcmds.c)

  list(APPEND CSRCS nsh_session.c)

  if(CONFIG_NSH_CONSOLE_LOGIN)
    list(APPEND CSRCS nsh_login.c)
  endif()

  list(APPEND CSRCS nsh_fsutils.c)

  if(CONFIG_NSH_BUILTIN_APPS)
    list(APPEND CSRCS nsh_builtin.c)
  endif()

  if(CONFIG_NSH_FILE_APPS)
    list(APPEND CSRCS nsh_fileapps.c)
  endif()

  if(CONFIG_NSH_VARS)
    list(APPEND CSRCS nsh_vars.c)
  endif()

  if(CONFIG_NET)
    list(APPEND CSRCS nsh_netcmds.c)

    if(CONFIG_NET_ROUTE)
      list(APPEND CSRCS nsh_routecmds.c)
    endif()
  endif()

  if(NOT CONFIG_DISABLE_MOUNTPOINT)
    list(APPEND CSRCS nsh_mntcmds.c)
  endif()

  if(CONFIG_MODULE)
    if(NOT CONFIG_NSH_DISABLE_MODCMDS)
      list(APPEND CSRCS nsh_modcmds.c)
    endif()
  endif()

  if(CONFIG_NSH_CONSOLE)
    list(APPEND CSRCS nsh_consolemain.c)
  endif()

  if(NOT CONFIG_NSH_DISABLE_PRINTF)
    list(APPEND CSRCS nsh_printf.c)
  endif()

  if(CONFIG_NSH_TELNET)
    list(APPEND CSRCS nsh_telnetd.c)
    if(CONFIG_NSH_TELNET_LOGIN)
      list(APPEND CSRCS nsh_telnetlogin.c)
    endif()
  endif()

  if(NOT CONFIG_NSH_DISABLESCRIPT)
    list(APPEND CSRCS nsh_test.c)
  endif()

  if(NOT CONFIG_NSH_DISABLE_WAIT)
    list(APPEND CSRCS nsh_wait.c)
  endif()

  if(CONFIG_USBDEV)
    list(APPEND CSRCS nsh_usbconsole.c)
  endif()

  if(CONFIG_NSH_ALTCONDEV)
    list(APPEND CSRCS nsh_altconsole.c)
  endif()

  if(CONFIG_NSH_USBDEV_TRACE)
    list(APPEND CSRCS nsh_usbtrace.c)
  endif()

  if(CONFIG_NETUTILS_CODECS)
    list(APPEND CSRCS nsh_codeccmd.c)
  endif()

  if(CONFIG_NSH_LOGIN_PASSWD)
    list(APPEND CSRCS nsh_passwdcmds.c)
  endif()

  if(CONFIG_NSH_ALIAS)
    list(APPEND CSRCS nsh_alias.c)
  endif()

  target_sources(apps PRIVATE ${CSRCS})

endif()
