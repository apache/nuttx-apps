/****************************************************************************
 * apps/examples/hello_rust/hello_rust_main.rs
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

/****************************************************************************
 * Attributes
 ****************************************************************************/

#![no_main]
#![no_std]

/****************************************************************************
 * Uses
 ****************************************************************************/

use core::panic::PanicInfo;

/****************************************************************************
 * Externs
 ****************************************************************************/

extern "C" {
    pub fn printf(format: *const u8, ...) -> i32;
}

/****************************************************************************
 * Private functions
 ****************************************************************************/

/****************************************************************************
 * Panic handler (needed for [no_std] compilation)
 ****************************************************************************/

#[panic_handler]
fn panic(_panic: &PanicInfo<'_>) -> ! {
    loop {}
}

/****************************************************************************
 * Public functions
 ****************************************************************************/

/****************************************************************************
 * hello_rust_main
 ****************************************************************************/

#[no_mangle]
pub extern "C" fn hello_rust_main(_argc: i32, _argv: *const *const u8) -> i32 {
    /* "Hello, Rust!!" using printf() from libc */

    unsafe {
        printf(b"Hello, Rust!!\n\0" as *const u8);
    }

    /* exit with status 0 */

    0
}
