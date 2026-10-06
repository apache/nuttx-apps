############################################################################
# apps/examples/fdpicxip/modules/lazymod/Makefile
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

# Its imports go through the PLT and stay in the lazy binding table.  So
# it calls them without -mlong-calls, and it links with -z lazy.

PROGNAME = lazymod
MODULE   = m
MAINSRC  = lazymod.c

lazymod.c_CELFFLAGS = -mno-long-calls

$(BINDIR)$(DELIM)lazymod: MODLDFLAGS += -z lazy

include $(APPDIR)/Application.mk
