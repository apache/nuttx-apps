############################################################################
# apps/system/cfgdata/Makefile
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

# TODO, this makefile should run make under the app dirs, instead of
# sourcing the Make.defs!

include $(APPDIR)/Make.defs

# config Application

PROGNAME = cfgdata
PRIORITY = $(CONFIG_SYSTEM_CFGDATA_PRIORITY)
STACKSIZE = $(CONFIG_SYSTEM_CFGDATA_STACKSIZE)
MODULE = $(CONFIG_SYSTEM_CFGDATA)

MAINSRC = cfgdata_main.c

include $(APPDIR)/Application.mk
