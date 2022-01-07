//! Embedded HAL for NuttX

use embedded_hal::{
    blocking::{
        delay::{DelayMs, DelayUs},
        spi::{Transfer, Write},
    },
    digital::v2,
};
use crate::{
    ioctl, open, puts, read, usleep, write,
    GPIOC_READ, GPIOC_WRITE, O_RDWR,
};

/// NuttX SPI Transfer
impl Transfer<u8> for Spi {
    /// Error Type
    type Error = ();

    /// Transfer SPI data
    fn transfer<'w>(&mut self, words: &'w mut [u8]) -> Result<&'w [u8], Self::Error> {
        //  Transmit data
        let bytes_written = unsafe { 
            write(self.fd, words.as_ptr(), words.len()) 
        };
        assert!(bytes_written == words.len() as isize);

        //  Read response
        let bytes_read = unsafe { 
            read(self.fd, words.as_mut_ptr(), words.len()) 
        };
        assert!(bytes_read == words.len() as isize);

        //  Return response
        Ok(words)
    }
}

/// NuttX SPI Write
impl Write<u8> for Spi{
    /// Error Type
    type Error = ();

    /// Write SPI data
    fn write(&mut self, words: &[u8]) -> Result<(), Self::Error> {
        //  Transmit data
        let bytes_written = unsafe { 
            write(self.fd, words.as_ptr(), words.len()) 
        };
        assert!(bytes_written == words.len() as isize);
        Ok(())
    }
}

/// NuttX Output Pin
impl v2::OutputPin for OutputPin {
    /// Error Type
    type Error = ();

    /// Set the GPIO Output to High
    fn set_high(&mut self) -> Result<(), Self::Error> {
        let ret = unsafe { 
            ioctl(self.fd, GPIOC_WRITE, 1) 
        };
        assert!(ret >= 0);
        Ok(())
    }

    /// Set the GPIO Output to low
    fn set_low(&mut self) -> Result<(), Self::Error> {
        let ret = unsafe { 
            ioctl(self.fd, GPIOC_WRITE, 0) 
        };
        assert!(ret >= 0);
        Ok(())
    }
}

/// NuttX Input Pin
impl v2::InputPin for InputPin {
    /// Error Type
    type Error = ();

    /// Return true if GPIO Input is high
    fn is_high(&self) -> Result<bool, Self::Error> {
        let mut invalue: i32 = 0;
        let addr: *mut i32 = &mut invalue;
        let ret = unsafe {
            ioctl(self.fd, GPIOC_READ, addr)
        };
        assert!(ret >= 0);
        match invalue {
            0 => Ok(false),
            _ => Ok(true),
        }
    }

    /// Return true if GPIO Input is low
    fn is_low(&self) -> Result<bool, Self::Error> {
        Ok(!self.is_high()?)
    }
}

/// NuttX Interrupt Pin
impl v2::InputPin for InterruptPin {
    /// Error Type
    type Error = ();

    /// Return true if GPIO Input is high
    fn is_high(&self) -> Result<bool, Self::Error> {
        let mut invalue: i32 = 0;
        let addr: *mut i32 = &mut invalue;
        let ret = unsafe {
            ioctl(self.fd, GPIOC_READ, addr)
        };
        assert!(ret >= 0);
        match invalue {
            0 => Ok(false),
            _ => Ok(true),
        }
    }

    /// Return true if GPIO Input is low
    fn is_low(&self) -> Result<bool, Self::Error> {
        Ok(!self.is_high()?)
    }
}

/// NuttX Unused Pin
impl v2::OutputPin for UnusedPin {
    /// Error Type
    type Error = ();

    /// Set the pin to high
    fn set_high(&mut self) -> Result<(), Self::Error> {
        Ok(())
    }

    /// Set the pin to low
    fn set_low(&mut self) -> Result<(), Self::Error> {
        Ok(())
    }
}

/// NuttX Delay in Microseconds
impl DelayUs<u32> for Delay {
    /// Sleep for us microseconds
    fn delay_us(&mut self, us: u32) {
        unsafe { usleep(us); }
    }
}

/// NuttX Delay in Milliseconds
impl DelayMs<u32> for Delay {
    /// Sleep for ms milliseconds
    fn delay_ms(&mut self, ms: u32) {
        unsafe { usleep(ms * 1000); }
    }
}

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

/// NuttX Delay
impl Delay {
    /// Create a delay interface
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

/// NuttX Delay
pub struct Delay {
}
