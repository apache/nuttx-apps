############################################################################
# apps/examples/webpanel/Makefile
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

# Web Panel application + CGI handlers
# NuttX multi-program build: space-separated lists, matched by position.

CSRCS   = romfs.c ws_terminal.c
MAINSRC = webpanel_main.c cgi_sysinfo.c cgi_files.c cgi_upload.c cgi_renew.c

PROGNAME  = $(CONFIG_EXAMPLES_WEBPANEL_PROGNAME) \
            $(CONFIG_EXAMPLES_WEBPANEL_CGI_SYSINFO_PROGNAME) \
            $(CONFIG_EXAMPLES_WEBPANEL_CGI_FILES_PROGNAME) \
            $(CONFIG_EXAMPLES_WEBPANEL_CGI_UPLOAD_PROGNAME) \
            $(CONFIG_EXAMPLES_WEBPANEL_CGI_RENEW_PROGNAME)
PRIORITY  = $(CONFIG_EXAMPLES_WEBPANEL_PRIORITY) \
            $(CONFIG_EXAMPLES_WEBPANEL_CGI_SYSINFO_PRIORITY) \
            $(CONFIG_EXAMPLES_WEBPANEL_CGI_FILES_PRIORITY) \
            $(CONFIG_EXAMPLES_WEBPANEL_CGI_UPLOAD_PRIORITY) \
            $(CONFIG_EXAMPLES_WEBPANEL_CGI_RENEW_PRIORITY)
STACKSIZE = $(CONFIG_EXAMPLES_WEBPANEL_STACKSIZE) \
            $(CONFIG_EXAMPLES_WEBPANEL_CGI_SYSINFO_STACKSIZE) \
            $(CONFIG_EXAMPLES_WEBPANEL_CGI_FILES_STACKSIZE) \
            $(CONFIG_EXAMPLES_WEBPANEL_CGI_UPLOAD_STACKSIZE) \
            $(CONFIG_EXAMPLES_WEBPANEL_CGI_RENEW_STACKSIZE)

LWS_DIR = $(APPDIR)/netutils/libwebsockets
CFLAGS += -I$(LWS_DIR) \
          -I$(LWS_DIR)/libwebsockets/include

MODULE    = $(CONFIG_EXAMPLES_WEBPANEL)

VPATH   += content
DEPPATH += --dep-path content

# ROMFS image generation
# Static web content goes directly into the ROMFS root (no www/ subdir)

WEBPANEL_DIR  = $(APPDIR)/examples/webpanel
CONTENT_DIR   = $(WEBPANEL_DIR)/content
ROMFS_DIR     = $(CONTENT_DIR)/romfs
ROMFS_IMG     = $(CONTENT_DIR)/romfs.img
ROMFS_SRC     = $(CONTENT_DIR)/romfs.c

WWW_FILES = $(wildcard $(CONTENT_DIR)/www/*)

$(CONTENT_DIR)/.romfs_stamp: $(WWW_FILES)
	$(Q) mkdir -p $(ROMFS_DIR)
	$(Q) cp -a $(CONTENT_DIR)/www/* $(ROMFS_DIR)/
	$(Q) touch $@

$(ROMFS_IMG): $(CONTENT_DIR)/.romfs_stamp
	$(Q) genromfs -f $@ -d $(ROMFS_DIR) -V "WebPanelROMFS"

$(ROMFS_SRC): $(ROMFS_IMG)
	$(Q) (cd $(CONTENT_DIR) && \
	      echo "#include <nuttx/compiler.h>" >$@ && \
	      xxd -i romfs.img | \
	      sed -e "s/romfs_img/webpanel_romfs_img/g" | \
	      sed -e "s/^unsigned char/const unsigned char aligned_data(4)/g" >>$@)

content/romfs.c: $(ROMFS_SRC)

context::

clean::
	$(call DELFILE, $(ROMFS_SRC))
	$(call DELFILE, $(ROMFS_IMG))
	$(call DELFILE, $(CONTENT_DIR)/.romfs_stamp)
	$(Q) rm -rf $(ROMFS_DIR)

distclean:: clean

include $(APPDIR)/Application.mk
