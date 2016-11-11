#############################################################################
# apps/graphics/traveler/tools/libwld/Imakefile
#
#   Copyright (C) 2016 Gregory Nutt. All rights reserved.
#   Author: Gregory Nutt <gnutt@nuttx.org>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
# 3. Neither the name NuttX nor the names of its contributors may be
#    used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
# OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
# AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
#############################################################################


WD       = ${shell pwd}
TRAVELER = $(WD)/../..
NUTTXDIR = $(WD)/../include

BIN      = libwld.a
WLDSRCS  = wld_createworld.c wld_deallocateworld.c
TXTSRCS  = wld_newgraphicfile.c wld_freegraphicfile.c wld_graphicfilepixel.c
TXTSRCS += wld_readgraphicfile.c wld_readtexturefile.c wld_freetexture.c
BMSRCS   = wld_bitmaps.c wld_initializebitmaps.c wld_discardbitmaps.c
BMSRCS  += wld_loadbitmapfile.c wld_pcxinit.c wld_loadpcx.c wld_loadppm.c
BMSRCS  += wld_loadgif.c
PLNSRCS  = wld_plane.c wld_initializeplanes.c wld_discardplanes.c
PLNSRCS += wld_loadplanefile.c wld_loadplanes.c wld_saveplanes.c wld_newplane.c
PLNSRCS += wld_addplane.c wld_mergeplanelists.c wld_removeplane.c wld_moveplane.c
PLNSRCS += wld_findplane.c
PALSRCS  = wld_loadpaltable.c wld_discardpaltable.c
CLRSRCS  = wld_rgblookup.c wld_rgb2pixel.c wld_pixel2lum.c wld_lum2pixel.c
UTSRCS   = wld_inifile.c wld_readdecimal.c wld_fatalerror.c
UTSRCS  += wld_malloc.c wld_realloc.c wld_free.c
SRCS     = $(WLDSRCS) $(TXTSRCS) $(BMSRCS) $(PLNSRCS) $(PALSRCS) $(CLRSRCS) $(UTSRCS)
OBJS     = $(SRCS:.c=.o)

CC       = gcc
AR       = ar -rcv

DEBUG_LEVEL ?= 0
DEFINES  = -DDEBUG_LEVEL=$(DEBUG_LEVEL)
INCLUDES = -I. -I$(TRAVELER)/include -isystem $(NUTTXDIR)
WARNINGS = -Wall -Wstrict-prototypes -Wshadow -Wno-trigraphs

ifneq ($(DEBUG_LEVEL),0)
  CPUFLAGS = -g
else
  CPUFLAGS = -O2 -fomit-frame-pointer
endif

CFLAGS  += $(CPUFLAGS) $(WARNINGS) $(DEFINES) $(INCLUDES)

all: $(BIN)

$(OBJS): %.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

$(BIN): $(OBJS)
	$(AR) $@ $^

clean:
	@rm -f $(BIN) *.o *~
