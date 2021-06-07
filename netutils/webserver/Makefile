############################################################################
# apps/netutils/webserver/Makefile
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

# Web server library

ifeq ($(CONFIG_NET_TCP),y)
CSRCS += httpd.c httpd_cgi.c
ifeq ($(CONFIG_NETUTILS_HTTPD_SENDFILE),y)
CSRCS += httpd_sendfile.c
ifeq ($(CONFIG_NETUTILS_HTTPD_DIRLIST),y)
CSRCS += httpd_dirlist.c
endif
else ifeq ($(CONFIG_NETUTILS_HTTPD_MMAP),y)
CSRCS += httpd_mmap.c
else
CSRCS += httpd_fs.c
endif
endif

include $(APPDIR)/Application.mk
