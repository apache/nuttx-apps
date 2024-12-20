# ##############################################################################
# apps/netutils/ftpc/CMakeLists.txt
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

if(CONFIG_NETUTILS_FTPC)
  set(CSRCS)

  # FTP connection management

  list(APPEND CSRCS ftpc_connect.c ftpc_disconnect.c)

  # FTP commands

  list(
    APPEND
    CSRCS
    ftpc_cdup.c
    ftpc_chdir.c
    ftpc_chmod.c
    ftpc_filesize.c
    ftpc_filetime.c
    ftpc_help.c
    ftpc_idle.c
    ftpc_listdir.c
    ftpc_login.c
    ftpc_mkdir.c
    ftpc_noop.c
    ftpc_rpwd.c
    ftpc_quit.c
    ftpc_rename.c
    ftpc_rmdir.c
    ftpc_unlink.c
    ftpc_cmd.c)

  # FTP transfers

  list(APPEND CSRCS ftpc_getfile.c ftpc_putfile.c ftpc_transfer.c)

  # FTP responses

  list(APPEND CSRCS ftpc_response.c ftpc_getreply.c)

  # FTP helpers
  list(APPEND CSRCS ftpc_utils.c ftpc_socket.c)

  target_sources(apps PRIVATE ${CSRCS})

endif()
