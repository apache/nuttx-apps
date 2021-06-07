############################################################################
# apps/examples/pdcurses/Makefile
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

# pdcurses demo programs

PROGNAME = charset firework newdemo panel
PROGNAME += rain testcurs tui worm xmas
PRIORITY = $(CONFIG_EXAMPLES_PDCURSES_PRIORITY)
STACKSIZE = $(CONFIG_EXAMPLES_PDCURSES_STACKSIZE)
MODULE = $(CONFIG_EXAMPLES_PDCURSES)

CSRCS = tui.c

MAINSRC = charset_main.c firework_main.c newdemo_main.c panel_main.c
MAINSRC += rain_main.c testcurs_main.c tui_main.c worm_main.c xmas_main.c

include $(APPDIR)/Application.mk
