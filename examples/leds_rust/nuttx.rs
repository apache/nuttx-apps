/****************************************************************************
 * apps/examples/leds_rust/nuttx.rs
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/* NuttX Definitions for Rust Apps */

/****************************************************************************
 * Uses
 ****************************************************************************/

use core::result::Result::{self, Err, Ok};

/****************************************************************************
 * Externs
 ****************************************************************************/

extern "C" {
    pub fn printf(format: *const u8, ...) -> i32;
    pub fn open(path: *const u8, oflag: i32, ...) -> i32;
    pub fn close(fd: i32) -> i32;
    pub fn ioctl(fd: i32, request: i32, ...) -> i32;
    pub fn usleep(usec: u32) -> u32;
    pub fn puts(s: *const u8) -> i32;
}

/****************************************************************************
 * Public Constants
 ****************************************************************************/

pub const ENAMETOOLONG: i32 = 36;
pub const O_WRONLY: i32 = 1 << 1;
pub const PATH_MAX: usize = 256;
pub const PUTS_MAX: usize = 256;
pub const ULEDIOC_SETALL: i32 = 0x1d03;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Copy the Rust Str to the Byte Buffer and terminate with null */

fn copy_to_buffer(s: &str, buffer: &mut [u8]) -> Result<(), ()> {
    let byte_str = s.as_bytes();
    let len = byte_str.len();
    if len >= buffer.len() {
        Err(())
    } else {
        buffer[..len].copy_from_slice(&byte_str[..len]);
        buffer[len] = 0;
        Ok(())
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* Safe Version of open() */

pub fn safe_open(path: &str, oflag: i32) -> Result<i32, i32> {
    let mut buffer = [0u8; PATH_MAX];
    let res = copy_to_buffer(path, &mut buffer);
    if res.is_err() {
        unsafe {
            puts(b"ERROR: safe_open() path size exceeds PATH_MAX\0" as *const u8);
        }
        return Err(-ENAMETOOLONG);
    }

    let fd = unsafe { open(buffer.as_ptr(), oflag) };
    if fd < 0 {
        Err(fd)
    } else {
        Ok(fd)
    }
}

/* Safe Version of ioctl() */

pub fn safe_ioctl(fd: i32, request: i32, arg: i32) -> Result<i32, i32> {
    let ret = unsafe { ioctl(fd, request, arg) };
    if ret < 0 {
        Err(ret)
    } else {
        Ok(ret)
    }
}

/* Safe Version of puts() */

pub fn safe_puts(s: &str) {
    let mut buffer = [0u8; PUTS_MAX];
    let res = copy_to_buffer(s, &mut buffer);
    if res.is_err() {
        unsafe {
            puts(b"ERROR: safe_puts() string size exceeds PUTS_MAX\0" as *const u8);
        }
    } else {
        unsafe {
            puts(buffer.as_ptr());
        }
    }
}
