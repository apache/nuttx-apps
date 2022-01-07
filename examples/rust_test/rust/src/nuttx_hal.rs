//! Embedded HAL for NuttX

use embedded_hal::{
    blocking::{
        delay::{DelayMs, DelayUs},
        spi::{Transfer, Write},
    },
    digital::v2::{InputPin, OutputPin},
};
use crate::{
    open,
    puts,
    O_RDWR,
    String,
};

/// GPIO Input
impl NxInputPin {
    /// Create a GPIO Input Pin from a Device Path (e.g. b"/dev/gpio0\0")
    pub fn new(path: *const u8) -> Self {
        //  Open the NuttX Device Path (e.g. "/dev/gpio0") for read-write
        let fd = unsafe { open(path, O_RDWR) };
        assert!(fd > 0);

        //  Return the pin
        Self {
            path,
            fd,
        }
    }
}

/// GPIO Input
pub struct NxInputPin {
    /// NuttX Device Path (e.g. "/dev/gpio0")
    path: *const u8,
    /// NuttX File Descriptor
    fd:   i32,
}
