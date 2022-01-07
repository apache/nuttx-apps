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
};

/// NuttX SPI Bus
impl NxSpi {
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
impl NxInputPin {
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
impl NxOutputPin {
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
impl NxInterruptPin {
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
impl NxUnusedPin {
    /// Create a GPIO Unused Pin
    pub fn new() -> Self {
        //  Return the pin
        Self {}
    }
}

/// NuttX SPI Bus
pub struct NxSpi {
    /// NuttX Device Path (e.g. b"/dev/spitest0\0")
    path: *const u8,
    /// NuttX File Descriptor
    fd:   i32,
}

/// NuttX GPIO Input
pub struct NxInputPin {
    /// NuttX Device Path (e.g. b"/dev/gpio0\0")
    path: *const u8,
    /// NuttX File Descriptor
    fd:   i32,
}

/// NuttX GPIO Output
pub struct NxOutputPin {
    /// NuttX Device Path (e.g. b"/dev/gpio1\0")
    path: *const u8,
    /// NuttX File Descriptor
    fd:   i32,
}

/// NuttX GPIO Interrupt
pub struct NxInterruptPin {
    /// NuttX Device Path (e.g. b"/dev/gpio2\0")
    path: *const u8,
    /// NuttX File Descriptor
    fd:   i32,
}

/// NuttX GPIO Unused
pub struct NxUnusedPin {
}
