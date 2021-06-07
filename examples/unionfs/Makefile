############################################################################
# apps/examples/unionfs/Makefile
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

# UNIONFS built-in application info

PROGNAME = unionfs
PRIORITY = SCHED_PRIORITY_DEFAULT
STACKSIZE = $(CONFIG_DEFAULT_TASK_STACKSIZE)
MODULE = $(CONFIG_EXAMPLES_UNIONFS)

# UNIONFS File System Example

MAINSRC = unionfs_main.c

# Common build

checkgenromfs:
	@genromfs -h 1>/dev/null 2>&1 || { \
 echo "Host executable genromfs not available in PATH"; \
 echo "You may need to download in from http://unionfs.sourceforge.net/"; \
 exit 1; \
	}

atestdir.img : checkgenromfs
	@genromfs -f $@ -d atestdir -V "UNIONFS_FS1" || { echo "genromfs failed" ; exit 1 ; }

romfs_atestdir.h : atestdir.img
	@xxd -i $< >$@ || { echo "xxd of $< failed" ; exit 1 ; }

btestdir.img : checkgenromfs
	@genromfs -f $@ -d btestdir -V "UNIONFS_FS2" || { echo "genromfs failed" ; exit 1 ; }

romfs_btestdir.h : btestdir.img
	@xxd -i $< >$@ || { echo "xxd of $< failed" ; exit 1 ; }

context:: romfs_atestdir.h romfs_btestdir.h

distclean::
	$(call DELFILE, atestdir.img)
	$(call DELFILE, btestdir.img)
	$(call DELFILE, romfs_atestdir.h)
	$(call DELFILE, romfs_btestdir.h)

include $(APPDIR)/Application.mk
