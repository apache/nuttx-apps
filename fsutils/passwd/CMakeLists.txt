# ##############################################################################
# apps/fsutils/passwd/CMakeLists.txt
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

if(CONFIG_FSUTILS_PASSWD)
  set(CSRCS)

  list(APPEND CSRCS passwd_verify.c passwd_find.c passwd_encrypt.c)

  if(NOT CONFIG_FSUTILS_PASSWD_READONLY)
    list(
      APPEND
      CSRCS
      passwd_adduser.c
      passwd_deluser.c
      passwd_update.c
      passwd_append.c
      passwd_delete.c
      passwd_lock.c)
  endif()

  set_property(GLOBAL APPEND PROPERTY NUTTX_APPS_LIBRARIES ${TARGET})
  target_sources(apps PRIVATE ${CSRCS})

endif()
