///**************************************************************************
// examples/leds_zig/leds_zig.zig
//
// SPDX-License-Identifier: Apache-2.0
//
// Licensed to the Apache Software Foundation (ASF) under one or more
// contributor license agreements.  See the NOTICE file distributed with
// this work for additional information regarding copyright ownership.  The
// ASF licenses this file to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance with the
// License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
// License for the specific language governing permissions and limitations
// under the License.
//
///**************************************************************************
///**************************************************************************
// Included Files
///**************************************************************************
const std = @import("std");

// C types not working with Zig comptime, need allocation
const Allocator = std.mem.Allocator;

/// NuttX namespace
const NuttX = struct {
    // public constant
    pub const c = @cImport({
        @cInclude("nuttx/config.h");
        @cInclude("sys/ioctl.h");
        @cInclude("stdbool.h");
        @cInclude("stdlib.h");
        @cInclude("stdio.h");
        @cInclude("fcntl.h");
        @cInclude("sched.h");
        @cInclude("errno.h");
        @cInclude("signal.h");
        @cInclude("unistd.h");

        @cInclude("nuttx/leds/userled.h");
    });

    var g_led_daemon_started: bool = false;

    pub fn print(allocator: Allocator, comptime fmt: [*:0]const u8, args: anytype) void {
        // comptime buffer[count(fmt, args)] or runtime allocation
        const output = if (isComptime(args))
            std.fmt.comptimePrint(std.mem.span(fmt), args)
        else
            std.fmt.allocPrintZ(allocator, std.mem.span(fmt), args) catch @panic("Out of Memory!!");

        _ = c.printf(output);
    }

    pub const exit_t = enum(c_int) {
        success = c.EXIT_SUCCESS,
        failure = c.EXIT_FAILURE,
    };

    // private function

    // check if value is comptime
    inline fn isComptime(val: anytype) bool {
        return @typeInfo(@TypeOf(.{val})).Struct.fields[0].is_comptime;
    }

    export fn led_daemon(_: c_int, _: [*c][*c]u8) callconv(.C) c_int {
        var supported: NuttX.c.userled_set_t = 0;
        var ledset: NuttX.c.userled_set_t = 0;
        var incrementing: bool = true;

        // SIGTERM handler
        const sigaction_t = NuttX.c.struct_sigaction;
        var act: sigaction_t = .{
            .sa_flags = NuttX.c.SA_SIGINFO,
            .sa_u = .{
                ._sa_sigaction = sigterm_action,
            },
        };

        _ = NuttX.c.sigemptyset(&act.sa_mask);
        _ = NuttX.c.sigaddset(&act.sa_mask, NuttX.c.SIGTERM);

        const sig = NuttX.c.sigaction(NuttX.c.SIGTERM, &act, null);
        if (sig != 0) {
            NuttX.print(global_allocator, "Failed to install SIGTERM handler, errno={}\n", .{sig});
            return @intFromEnum(NuttX.exit_t.failure);
        }

        // Indicate that we are running
        const mypid = NuttX.c.getpid();
        g_led_daemon_started = true;
        NuttX.print(global_allocator, "\nled_daemon (pid# {}): Running\n", .{mypid});

        // Open the LED driver
        NuttX.print(global_allocator, "led_daemon: Opening {s}\n", .{NuttX.c.CONFIG_EXAMPLES_LEDS_DEVPATH});
        const fd = NuttX.c.open(NuttX.c.CONFIG_EXAMPLES_LEDS_DEVPATH, NuttX.c.O_WRONLY);
        if (fd < 0) {
            NuttX.print(global_allocator, "led_daemon: ERROR: Failed to open {s}: {}\n", .{
                NuttX.c.CONFIG_EXAMPLES_LEDS_DEVPATH,
                fd,
            });
            g_led_daemon_started = false;
            return @intFromEnum(NuttX.exit_t.failure);
        }

        // Get the set of LEDs supported
        var ret = NuttX.c.ioctl(fd, NuttX.c.ULEDIOC_SUPPORTED, &supported);
        if (ret < 0) {
            NuttX.print(global_allocator, "led_daemon: ERROR: ioctl(ULEDIOC_SUPPORTED) failed: {}\n", .{ret});
            _ = NuttX.c.close(fd);
            g_led_daemon_started = false;
            return @intFromEnum(NuttX.exit_t.failure);
        }

        NuttX.print(global_allocator, "led_daemon: Supported LEDs {x}\n", .{supported});
        supported &= NuttX.c.CONFIG_EXAMPLES_LEDS_LEDSET;

        // Main loop
        while (g_led_daemon_started) {
            var newset: NuttX.c.userled_set_t = 0;
            var tmp: NuttX.c.userled_set_t = 0;

            if (incrementing) {
                tmp = ledset;
                while (newset == ledset) {
                    tmp += 1;
                    newset = tmp & supported;
                }

                if (newset == 0) {
                    incrementing = false;
                    continue;
                }
            } else {
                if (ledset == 0) {
                    incrementing = true;
                    continue;
                }

                tmp = ledset;
                while (newset == ledset) {
                    tmp -= 1;
                    newset = tmp & supported;
                }
            }

            ledset = newset;
            NuttX.print(global_allocator, "led_daemon: LED set {x}\n", .{ledset});

            ret = NuttX.c.ioctl(fd, NuttX.c.ULEDIOC_SETALL, ledset);
            if (ret < 0) {
                NuttX.print(global_allocator, "led_daemon: ERROR: ioctl(ULEDIOC_SETALL) failed: {}\n", .{ret});
                _ = NuttX.c.close(fd);
                g_led_daemon_started = false;
                return @intFromEnum(NuttX.exit_t.failure);
            }

            _ = NuttX.c.usleep(500 * 1000);
        }

        _ = NuttX.c.close(fd);
        return @intFromEnum(NuttX.exit_t.success);
    }

    export fn sigterm_action(signo: c_int, siginfo: [*c]NuttX.c.siginfo_t, arg: ?*anyopaque) void {
        if (signo == NuttX.c.SIGTERM) {
            NuttX.print(global_allocator, "SIGTERM received\n", .{});
            g_led_daemon_started = false;
            NuttX.print(global_allocator, "led_daemon: Terminated.\n", .{});
        } else {
            NuttX.print(global_allocator, "\nsigterm_action: Received signo={} siginfo={*} arg={*}\n", .{
                signo,
                siginfo,
                arg,
            });
        }
    }
};

// Global allocator
var arena = std.heap.ArenaAllocator.init(std.heap.raw_c_allocator);
const global_allocator = arena.allocator();

export fn leds_zig_main(_: c_int, _: [*][*:0]u8) NuttX.exit_t {
    defer arena.deinit();

    NuttX.print(global_allocator, "leds_main: Starting the led_daemon\n", .{});
    if (NuttX.g_led_daemon_started) {
        NuttX.print(global_allocator, "leds_main: led_daemon already running\n", .{});
        return .success;
    }

    const ret = NuttX.c.task_create(
        "led_daemon",
        NuttX.c.CONFIG_EXAMPLES_LEDS_PRIORITY,
        NuttX.c.CONFIG_EXAMPLES_LEDS_STACKSIZE,
        NuttX.led_daemon,
        null,
    );
    if (ret < 0) {
        NuttX.print(global_allocator, "leds_main: ERROR: Failed to start led_daemon: {}\n", .{ret});
        return .failure;
    }

    NuttX.print(global_allocator, "leds_main: led_daemon started\n", .{});
    return .success;
}
