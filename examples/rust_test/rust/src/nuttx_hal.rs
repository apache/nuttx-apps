//! Embedded HAL for NuttX

use embedded_hal::{
    blocking::{
        delay::{DelayMs, DelayUs},
        spi::{Transfer, Write},
    },
    digital::v2,
};
use crate::{
    open,
    puts,
    O_RDWR,
};

/// NuttX SPI Bus
impl Spi {
    /// Create an SPI Bus from a Device Path (e.g. b"/dev/spitest0\0")
    pub fn new(path: *const u8) -> Self {
        //  Open the NuttX Device Path (e.g. b"/dev/spitest0\0") for read-write
        let fd = unsafe { open(path, O_RDWR) };
        assert!(fd > 0);

        //  Return the pin
        Self {
            path,
            fd,
        }
    }
}

/// NuttX GPIO Input
impl InputPin {
    /// Create a GPIO Input Pin from a Device Path (e.g. b"/dev/gpio0\0")
    pub fn new(path: *const u8) -> Self {
        //  Open the NuttX Device Path (e.g. b"/dev/gpio0\0") for read-write
        let fd = unsafe { open(path, O_RDWR) };
        assert!(fd > 0);

        //  Return the pin
        Self {
            path,
            fd,
        }
    }
}

/// NuttX GPIO Output
impl OutputPin {
    /// Create a GPIO Output Pin from a Device Path (e.g. b"/dev/gpio1\0")
    pub fn new(path: *const u8) -> Self {
        //  Open the NuttX Device Path (e.g. b"/dev/gpio1\0") for read-write
        let fd = unsafe { open(path, O_RDWR) };
        assert!(fd > 0);

        //  Return the pin
        Self {
            path,
            fd,
        }
    }
}

/// NuttX GPIO Interrupt
impl InterruptPin {
    /// Create a GPIO Interrupt Pin from a Device Path (e.g. b"/dev/gpio2\0")
    pub fn new(path: *const u8) -> Self {
        //  Open the NuttX Device Path (e.g. b"/dev/gpio2\0") for read-write
        let fd = unsafe { open(path, O_RDWR) };
        assert!(fd > 0);

        //  Return the pin
        Self {
            path,
            fd,
        }
    }
}

/// NuttX GPIO Unused
impl UnusedPin {
    /// Create a GPIO Unused Pin
    pub fn new() -> Self {
        //  Return the pin
        Self {}
    }
}

/// NuttX SPI Bus
pub struct Spi {
    /// NuttX Device Path (e.g. b"/dev/spitest0\0")
    path: *const u8,
    /// NuttX File Descriptor
    fd:   i32,
}

/// NuttX GPIO Input
pub struct InputPin {
    /// NuttX Device Path (e.g. b"/dev/gpio0\0")
    path: *const u8,
    /// NuttX File Descriptor
    fd:   i32,
}

/// NuttX GPIO Output
pub struct OutputPin {
    /// NuttX Device Path (e.g. b"/dev/gpio1\0")
    path: *const u8,
    /// NuttX File Descriptor
    fd:   i32,
}

/// NuttX GPIO Interrupt
pub struct InterruptPin {
    /// NuttX Device Path (e.g. b"/dev/gpio2\0")
    path: *const u8,
    /// NuttX File Descriptor
    fd:   i32,
}

/// NuttX GPIO Unused
pub struct UnusedPin {
}
