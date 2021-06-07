############################################################################
# apps/modbus/Makefile
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

# FreeModBus Library

ifeq ($(CONFIG_MODBUS),y)

  ifeq ($(CONFIG_MODBUS_SLAVE),y)
    CSRCS += mb.c
  endif

  ifeq ($(CONFIG_MB_RTU_MASTER),y)
    CSRCS += mb_m.c
  endif

  include ascii/Make.defs
  include functions/Make.defs
  include nuttx/Make.defs
  include rtu/Make.defs
  include tcp/Make.defs

endif

include $(APPDIR)/Application.mk
