############################################################################
# apps/system/libuv/Makefile
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

LIBUV_PATCHS ?= $(sort $(wildcard 000*.patch))

LIBUV_VERSION  = 1.42.0
LIBUV_UNPACK   = libuv
LIBUV_TARBALL  = v$(LIBUV_VERSION).zip
LIBUV_URL_BASE = https://github.com/libuv/libuv/archive/refs/tags/
LIBUV_URL      = $(LIBUV_URL_BASE)/$(LIBUV_TARBALL)


$(LIBUV_TARBALL):
	@echo "Downloading: $(LIBUV_TARBALL)"
	$(Q) curl -L $(LIBUV_URL) -o $(LIBUV_TARBALL)

$(LIBUV_UNPACK): $(LIBUV_TARBALL)
	@echo "Unpacking: $(LIBUV_TARBALL) -> $(LIBUV_UNPACK)"
	$(call DELDIR, $(LIBUV_UNPACK))
	$(Q) unzip $(LIBUV_TARBALL)
	$(Q) mv libuv-$(LIBUV_VERSION) $(LIBUV_UNPACK)
	$(Q) cat $(LIBUV_PATCHS) | \
		patch -s -N -d $(LIBUV_UNPACK) -p1

$(LIBUV_UNPACK)/.patch: $(LIBUV_UNPACK)
	$(Q) touch $(LIBUV_UNPACK)/.patch

# Build libuv library

CFLAGS += -I$(LIBUV_UNPACK)/src
CFLAGS += -I$(LIBUV_UNPACK)/src/unix
CFLAGS += -I$(LIBUV_UNPACK)/test
CFLAGS += -Wno-shadow
CFLAGS += -DDEF_THREADPOOL_SIZE=CONFIG_LIBUV_THREADPOOL_SIZE
CFLAGS += -DDEF_THREADPOOL_STACKSIZE=CONFIG_LIBUV_THREAD_STACKSIZE

VPATH += $(LIBUV_UNPACK)/src
VPATH += $(LIBUV_UNPACK)/src/unix
VPATH += $(LIBUV_UNPACK)/test

DEPPATH += --dep-path $(LIBUV_UNPACK)/src
DEPPATH += --dep-path $(LIBUV_UNPACK)/src/unix
DEPPATH += --dep-path $(LIBUV_UNPACK)/test

CSRCS += core.c
CSRCS += poll.c
CSRCS += loop.c
CSRCS += thread.c
CSRCS += posix-hrtime.c
CSRCS += posix-poll.c
CSRCS += uv-data-getter-setters.c
CSRCS += version.c
ifeq ($(CONFIG_LIBUV_UTILS_TEST),)
CSRCS += idna.c
CSRCS += strscpy.c
endif
CSRCS += no-fsevents.c
CSRCS += uv-common.c
CSRCS += random-devurandom.c
CSRCS += random.c
CSRCS += nuttx.c
CSRCS += tty.c
CSRCS += loop-watcher.c
CSRCS += signal.c
CSRCS += stream.c
CSRCS += threadpool.c
CSRCS += async.c
CSRCS += pipe.c
CSRCS += fs.c
CSRCS += fs-poll.c
CSRCS += timer.c
ifneq ($(CONFIG_LIBC_EXECFUNCS),)
CSRCS += process-spawn.c
endif
CSRCS += sysinfo-loadavg.c
CSRCS += sysinfo-memory.c

ifneq ($(CONFIG_LIBC_DLFCN),)
CSRCS += dl.c
endif

ifneq ($(CONFIG_NET),)
CSRCS += getaddrinfo.c
CSRCS += getnameinfo.c
CSRCS += inet.c
endif

ifneq ($(CONFIG_NET_TCP),)
CSRCS += tcp.c
endif

ifneq ($(CONFIG_NET_UDP),)
CSRCS += udp.c
endif

PRIORITY  = $(CONFIG_LIBUV_UTILS_PRIORITY)
STACKSIZE = $(CONFIG_LIBUV_UTILS_STACKSIZE)

ifneq ($(CONFIG_LIBUV_UTILS_TEST),)

PROGNAME = uv_run_tests
MAINSRC  = run-tests.c

CSRCS += runner.c
CSRCS += runner-unix.c
CSRCS += echo-server.c

CSRCS += test-active.c
CSRCS += test-async.c
CSRCS += test-async-null-cb.c
CSRCS += test-barrier.c
CSRCS += test-callback-order.c
CSRCS += test-callback-stack.c
CSRCS += test-close-fd.c
CSRCS += test-close-order.c
CSRCS += test-condvar.c
CSRCS += test-connection-fail.c
CSRCS += test-connect-unspecified.c
CSRCS += test-cwd-and-chdir.c
CSRCS += test-default-loop-close.c
CSRCS += test-delayed-accept.c
CSRCS += test-dlerror.c
CSRCS += test-eintr-handling.c
CSRCS += test-embed.c
CSRCS += test-emfile.c
CSRCS += test-env-vars.c
CSRCS += test-error.c
CSRCS += test-fail-always.c
CSRCS += test-fs.c
CSRCS += test-fs-copyfile.c
CSRCS += test-fs-event.c
CSRCS += test-fs-fd-hash.c
CSRCS += test-fs-open-flags.c
CSRCS += test-fs-poll.c
CSRCS += test-fs-readdir.c
CSRCS += test-getaddrinfo.c
CSRCS += test-get-currentexe.c
CSRCS += test-gethostname.c
CSRCS += test-get-loadavg.c
CSRCS += test-get-memory.c
CSRCS += test-getnameinfo.c
CSRCS += test-get-passwd.c
CSRCS += test-getsockname.c
CSRCS += test-getters-setters.c
CSRCS += test-gettimeofday.c
CSRCS += test-handle-fileno.c
CSRCS += test-homedir.c
CSRCS += test-hrtime.c
CSRCS += test-idle.c
CSRCS += test-idna.c
CSRCS += test-ip4-addr.c
CSRCS += test-ip6-addr.c
CSRCS += test-ipc.c
CSRCS += test-ipc-heavy-traffic-deadlock-bug.c
CSRCS += test-ipc-send-recv.c
CSRCS += test-loop-alive.c
CSRCS += test-loop-close.c
CSRCS += test-loop-configure.c
CSRCS += test-loop-handles.c
CSRCS += test-loop-stop.c
CSRCS += test-loop-time.c
CSRCS += test-metrics.c
CSRCS += test-multiple-listen.c
CSRCS += test-mutexes.c
CSRCS += test-not-readable-nor-writable-on-read-error.c
CSRCS += test-not-readable-on-eof.c
CSRCS += test-not-writable-after-shutdown.c
CSRCS += test-osx-select.c
CSRCS += test-pass-always.c
CSRCS += test-ping-pong.c
CSRCS += test-pipe-bind-error.c
CSRCS += test-pipe-connect-error.c
CSRCS += test-pipe-connect-multiple.c
CSRCS += test-pipe-connect-prepare.c
CSRCS += test-pipe-getsockname.c
CSRCS += test-pipe-pending-instances.c
CSRCS += test-pipe-sendmsg.c
CSRCS += test-pipe-server-close.c
CSRCS += test-pipe-set-fchmod.c
CSRCS += test-pipe-set-non-blocking.c
CSRCS += test-platform-output.c
CSRCS += test-poll.c
CSRCS += test-poll-close.c
CSRCS += test-poll-close-doesnt-corrupt-stack.c
CSRCS += test-poll-closesocket.c
CSRCS += test-poll-multiple-handles.c
CSRCS += test-poll-oob.c
CSRCS += test-process-priority.c
CSRCS += test-process-title.c
CSRCS += test-process-title-threadsafe.c
CSRCS += test-queue-foreach-delete.c
CSRCS += test-random.c
CSRCS += test-ref.c
CSRCS += test-run-nowait.c
CSRCS += test-run-once.c
CSRCS += test-semaphore.c
CSRCS += test-shutdown-close.c
CSRCS += test-shutdown-eof.c
CSRCS += test-shutdown-simultaneous.c
CSRCS += test-shutdown-twice.c
CSRCS += test-signal.c
CSRCS += test-signal-multiple-loops.c
CSRCS += test-signal-pending-on-close.c
CSRCS += test-socket-buffer-size.c
CSRCS += test-spawn.c
CSRCS += test-stdio-over-pipes.c
CSRCS += test-strscpy.c
CSRCS += test-tcp-alloc-cb-fail.c
CSRCS += test-tcp-bind6-error.c
CSRCS += test-tcp-bind-error.c
CSRCS += test-tcp-close-accept.c
CSRCS += test-tcp-close.c
CSRCS += test-tcp-close-reset.c
CSRCS += test-tcp-close-while-connecting.c
CSRCS += test-tcp-connect6-error.c
CSRCS += test-tcp-connect-error-after-write.c
CSRCS += test-tcp-connect-error.c
CSRCS += test-tcp-connect-timeout.c
CSRCS += test-tcp-create-socket-early.c
CSRCS += test-tcp-flags.c
CSRCS += test-tcp-oob.c
CSRCS += test-tcp-open.c
CSRCS += test-tcp-read-stop.c
CSRCS += test-tcp-read-stop-start.c
CSRCS += test-tcp-shutdown-after-write.c
CSRCS += test-tcp-try-write.c
CSRCS += test-tcp-try-write-error.c
CSRCS += test-tcp-unexpected-read.c
CSRCS += test-tcp-write-after-connect.c
CSRCS += test-tcp-writealot.c
CSRCS += test-tcp-write-fail.c
CSRCS += test-tcp-write-queue-order.c
CSRCS += test-tcp-write-to-half-open-connection.c
CSRCS += test-test-macros.c
CSRCS += test-thread.c
CSRCS += test-thread-equal.c
CSRCS += test-threadpool.c
CSRCS += test-threadpool-cancel.c
CSRCS += test-timer-again.c
CSRCS += test-timer.c
CSRCS += test-timer-from-check.c
CSRCS += test-tmpdir.c
CSRCS += test-tty.c
CSRCS += test-tty-duplicate-key.c
CSRCS += test-tty-escape-sequence-processing.c
CSRCS += test-udp-alloc-cb-fail.c
CSRCS += test-udp-bind.c
CSRCS += test-udp-connect.c
CSRCS += test-udp-create-socket-early.c
CSRCS += test-udp-dgram-too-big.c
CSRCS += test-udp-ipv6.c
CSRCS += test-udp-mmsg.c
CSRCS += test-udp-multicast-interface6.c
CSRCS += test-udp-multicast-interface.c
CSRCS += test-udp-multicast-join6.c
CSRCS += test-udp-multicast-join.c
CSRCS += test-udp-multicast-ttl.c
CSRCS += test-udp-open.c
CSRCS += test-udp-options.c
CSRCS += test-udp-send-and-recv.c
CSRCS += test-udp-send-hang-loop.c
CSRCS += test-udp-send-immediate.c
CSRCS += test-udp-sendmmsg-error.c
CSRCS += test-udp-send-unreachable.c
CSRCS += test-udp-try-send.c
CSRCS += test-uname.c
CSRCS += test-walk-handles.c
CSRCS += test-watcher-cross-stop.c
endif

ifneq ($(CONFIG_LIBUV_UTILS_BENCHMARK),)

PROGNAME = uv_run_benchmarks
MAINSRC  = run-benchmarks.c

CSRCS += runner.c
CSRCS += runner-unix.c

CSRCS += benchmark-async-pummel.c
CSRCS += benchmark-async.c
CSRCS += benchmark-fs-stat.c
CSRCS += benchmark-getaddrinfo.c
CSRCS += benchmark-loop-count.c
CSRCS += benchmark-million-async.c
CSRCS += benchmark-million-timers.c
CSRCS += benchmark-multi-accept.c
CSRCS += benchmark-ping-pongs.c
CSRCS += benchmark-ping-udp.c
CSRCS += benchmark-pound.c
CSRCS += benchmark-pump.c
CSRCS += benchmark-sizes.c
CSRCS += benchmark-spawn.c
CSRCS += benchmark-tcp-write-batch.c
CSRCS += benchmark-thread.c
CSRCS += benchmark-udp-pummel.c

CSRCS += blackhole-server.c
CSRCS += echo-server.c

endif

context:: $(LIBUV_UNPACK)/.patch

distclean::
	$(call DELDIR, $(LIBUV_UNPACK))
	$(call DELFILE, $(LIBUV_TARBALL))

include $(APPDIR)/Application.mk
