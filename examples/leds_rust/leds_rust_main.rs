/****************************************************************************
 * apps/examples/leds_rust/leds_rust_main.rs
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

/****************************************************************************
 * Attributes
 ****************************************************************************/

/* Comment out these lines for testing with Rust Standard Library */

#![no_main]
#![no_std]

/****************************************************************************
 * Uses
 ****************************************************************************/

#[cfg(target_os = "none")]
use core::{
    panic::PanicInfo,
    result::Result::{self, Err, Ok},
};

/****************************************************************************
 * Modules
 ****************************************************************************/

mod nuttx;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Panic Handler (needed for [no_std] compilation)
 ****************************************************************************/

#[cfg(target_os = "none")] /* For NuttX */
#[panic_handler]
fn panic(_panic: &PanicInfo<'_>) -> ! {
    loop {}
}

/****************************************************************************
 * rust_main
 ****************************************************************************/

fn rust_main(_argc: i32, _argv: *const *const u8) -> Result<i32, i32> {
    /* "Hello, Rust!!" using puts() from libc */

    nuttx::safe_puts("Hello, Rust!!");

    /* Blink LED 1 using ioctl() from NuttX */

    nuttx::safe_puts("Opening /dev/userleds");
    let fd = nuttx::safe_open("/dev/userleds", nuttx::O_WRONLY)?;
    nuttx::safe_puts("Set LED 1 to 1");

    nuttx::safe_ioctl(fd, nuttx::ULEDIOC_SETALL, 1)?;
    nuttx::safe_puts("Sleeping...");
    unsafe {
        nuttx::usleep(500_000);
    }

    nuttx::safe_puts("Set LED 1 to 0");
    nuttx::safe_ioctl(fd, nuttx::ULEDIOC_SETALL, 0)?;
    unsafe {
        nuttx::close(fd);
    }

    /* Exit with status 0 */

    Ok(0)
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * leds_rust_main
 ****************************************************************************/

#[no_mangle]
pub extern "C" fn leds_rust_main(_argc: i32, _argv: *const *const u8) -> i32 {
    /* Call the program logic in Rust Main */

    let res = rust_main(0, core::ptr::null());

    /* If Rust Main returns an error, print it */

    if let Err(e) = res {
        unsafe {
            nuttx::printf(
                b"ERROR: rust_main() failed with error %d\n\0" as *const u8,
                e,
            );
        }
        e
    } else {
        0
    }
}

/****************************************************************************
 * main
 ****************************************************************************/

#[cfg(not(target_os = "none"))] /* For Testing Locally */
fn main() {
    leds_rust_main(0, core::ptr::null());
}
