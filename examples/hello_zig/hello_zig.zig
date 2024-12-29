//***************************************************************************
// examples/hello_zig/hello_zig.zig
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
//***************************************************************************

//***************************************************************************
// Included Files
//***************************************************************************
const std = @import("std");

//****************************************************************************
//* C API - need libc linking (freestanding libc is stubs only)
//****************************************************************************
// NuttX namespace
const NuttX = struct {
    pub const c = @cImport({
        @cInclude("nuttx/config.h");
        @cInclude("stdio.h");
    });
    pub fn print(comptime fmt: [*:0]const u8, args: anytype) void {
        _ = printf(std.fmt.comptimePrint(std.mem.span(fmt), args));
    }
};

// or (optional) const c = std.c; // from std library (non-full libc)

// typedef alias
const printf = NuttX.c.printf;
//****************************************************************************
//* hello_zig_main
//****************************************************************************
comptime {
    // rename to hello_zig_main entry point for nuttx
    @export(hello_zig, .{
        .name = "hello_zig_main",
        .linkage = .weak,
    });
}

fn hello_zig(_: c_int, _: ?[*]const [*]const u8) callconv(.C) c_int {
    NuttX.print("[{s}]: Hello, Zig!\n", .{NuttX.c.CONFIG_ARCH_BOARD});
    return 0;
}
