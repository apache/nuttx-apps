############################################################################
# apps/examples/romfs/Makefile
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

# ROMFS built-in application info

PROGNAME = romfs
PRIORITY = SCHED_PRIORITY_DEFAULT
STACKSIZE = $(CONFIG_DEFAULT_TASK_STACKSIZE)
MODULE = $(CONFIG_EXAMPLES_ROMFS)

# ROMFS File System Example

MAINSRC = romfs_main.c

# Common build

checkgenromfs:
	@genromfs -h 1>/dev/null 2>&1 || { \
 echo "Host executable genromfs not available in PATH"; \
 echo "You may need to download in from http://romfs.sourceforge.net/"; \
 exit 1; \
	}

testdir : testdir.tar.gz
	@tar zxf $< || { echo "tar zxf $< failed" ; exit 1 ; }

testdir.img : checkgenromfs testdir
	@genromfs -f $@ -d testdir -V "ROMFS_Test" || { echo "genromfs failed" ; exit 1 ; }

romfs_testdir.h : testdir.img
	@xxd -i $< >$@ || { echo "xxd of $< failed" ; exit 1 ; }

context:: romfs_testdir.h

distclean::
	$(call DELFILE, testdir.img)
	$(call DELFILE, romfs_testdir.h)
	$(call DELDIR, testdir)

include $(APPDIR)/Application.mk
