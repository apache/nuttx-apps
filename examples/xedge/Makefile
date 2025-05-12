############################################################################
# apps/examples/xedge/Make.defs
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

# Xedge, World! built-in application info

PROGNAME  = $(CONFIG_EXAMPLES_XEDGE_PROGNAME)
PRIORITY  = $(CONFIG_EXAMPLES_XEDGE_PRIORITY)
STACKSIZE = $(CONFIG_EXAMPLES_XEDGE_STACKSIZE)
MODULE    = $(CONFIG_EXAMPLES_XEDGE)

# Xedge: BAS Example

MAINSRC = xedge_main.c BAS/src/BAS.c BAS/src/dlmalloc.c BAS/src/arch/Posix/ThreadLib.c BAS/src/arch/NET/generic/SoDisp.c BAS/src/DiskIo/posix/BaFile.c BAS/examples/xedge/src/xedge.c BAS/examples/xedge/ BAS/examples/xedge/XedgeZip.c

XEDGEZIP = BAS/examples/xedge/XedgeZip.c

$(XEDGEZIP):
	. prepare.sh

context:: $(XEDGEZIP)


include $(APPDIR)/Application.mk
