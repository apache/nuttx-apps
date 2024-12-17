############################################################################
# apps/audioutils/fmsynth/test/Makefile
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

SRCS = ../fmsynth_eg.c ../fmsynth_op.c ../fmsynth.c
CFLAGS = -DFAR= -DCODE= -DOK=0 -DERROR=-1 -I .. -I ../../../include -g

TARGETS = opfunctest fmsyntheg_test fmsynthop_test fmsynth_test fmsynth_alsa

all: $(TARGETS)

opfunctest: $(SRCS) opfunc_test.c
	gcc $(CFLAGS) -o $@ $^ -lm

fmsyntheg_test: $(SRCS) fmsynth_eg_test.c
	gcc $(CFLAGS) -o $@ $^

fmsynthop_test: $(SRCS) fmsynth_op_test.c
	gcc $(CFLAGS) -o $@ $^

fmsynth_test: $(SRCS) fmsynth_test.c
	gcc $(CFLAGS) -o $@ $^

fmsynth_alsa: $(SRCS) fmsynth_alsa_test.c
	gcc $(CFLAGS) -o $@ $^ -lasound

clean:
	rm -rf $(TARGETS)
