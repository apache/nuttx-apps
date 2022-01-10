//! Embedded HAL for NuttX

use core::{
    str::FromStr,
};
use embedded_hal::{
    blocking::{
        delay::{DelayMs, DelayUs},
        spi::{Transfer, Write},
    },
    digital::v2,
};
use crate::{
    close, ioctl, read, usleep, write,
    GPIOC_READ, GPIOC_WRITE, O_RDWR,
    String,
};

/// NuttX SPI Transfer
impl Transfer<u8> for Spi {
    /// Error Type
    type Error = ();

    /// Transfer SPI data
    fn transfer<'w>(&mut self, words: &'w mut [u8]) -> Result<&'w [u8], Self::Error> {
        //  Transmit data
        let bytes_written = unsafe { 
            write(self.fd, words.as_ptr(), words.len() as u32) 
        };
        assert!(bytes_written == words.len() as i32);

        //  Read response
        let bytes_read = unsafe { 
            read(self.fd, words.as_mut_ptr(), words.len() as u32) 
        };
        assert!(bytes_read == words.len() as i32);

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
            write(self.fd, words.as_ptr(), words.len() as u32) 
        };
        assert!(bytes_written == words.len() as i32);
        Ok(())
    }
}

/// Set NuttX Output Pin
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

/// Read NuttX Input Pin
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

/// Read NuttX Interrupt Pin
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

/// Set NuttX Unused Pin
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

/// New NuttX SPI Bus
impl Spi {
    /// Create an SPI Bus from a Device Path (e.g. "/dev/spitest0")
    pub fn new(path: &str) -> Self {
        //  Open the NuttX Device Path (e.g. "/dev/spitest0") for read-write
        let fd = open(path, O_RDWR);
        assert!(fd > 0);

        //  Return the pin
        Self { fd }
    }
}

/// New NuttX GPIO Input
impl InputPin {
    /// Create a GPIO Input Pin from a Device Path (e.g. "/dev/gpio0")
    pub fn new(path: &str) -> Self {
        //  Open the NuttX Device Path (e.g. "/dev/gpio0") for read-write
        let fd = open(path, O_RDWR);
        assert!(fd > 0);

        //  Return the pin
        Self { fd }
    }
}

/// New NuttX GPIO Output
impl OutputPin {
    /// Create a GPIO Output Pin from a Device Path (e.g. "/dev/gpio1")
    pub fn new(path: &str) -> Self {
        //  Open the NuttX Device Path (e.g. "/dev/gpio1") for read-write
        let fd = open(path, O_RDWR);
        assert!(fd > 0);

        //  Return the pin
        Self { fd }
    }
}

/// New NuttX GPIO Interrupt
impl InterruptPin {
    /// Create a GPIO Interrupt Pin from a Device Path (e.g. "/dev/gpio2")
    pub fn new(path: &str) -> Self {
        //  Open the NuttX Device Path (e.g. "/dev/gpio2") for read-write
        let fd = open(path, O_RDWR);
        assert!(fd > 0);

        //  Return the pin
        Self { fd }
    }
}

/// New NuttX GPIO Unused
impl UnusedPin {
    /// Create a GPIO Unused Pin
    pub fn new() -> Self {
        //  Return the pin
        Self {}
    }
}

/// New NuttX Delay
impl Delay {
    /// Create a delay interface
    pub fn new() -> Self {
        //  Return the pin
        Self {}
    }
}

/// Drop NuttX SPI Bus
impl Drop for Spi {
    /// Close the SPI Bus
    fn drop(&mut self) {
        unsafe { close(self.fd) };
    }
}

/// Drop NuttX GPIO Input
impl Drop for InputPin {
    /// Close the GPIO Input
    fn drop(&mut self) {
        unsafe { close(self.fd) };
    }
}

/// Drop NuttX GPIO Output
impl Drop for OutputPin {
    /// Close the GPIO Output
    fn drop(&mut self) {
        unsafe { close(self.fd) };
    }
}

/// Drop NuttX GPIO Interrupt
impl Drop for InterruptPin {
    /// Close the GPIO Interrupt
    fn drop(&mut self) {
        unsafe { close(self.fd) };
    }
}

/// NuttX SPI Struct
pub struct Spi {
    /// NuttX File Descriptor
    fd: i32,
}

/// NuttX GPIO Input Struct
pub struct InputPin {
    /// NuttX File Descriptor
    fd: i32,
}

/// NuttX GPIO Output Struct
pub struct OutputPin {
    /// NuttX File Descriptor
    fd: i32,
}

/// NuttX GPIO Interrupt Struct
pub struct InterruptPin {
    /// NuttX File Descriptor
    fd: i32,
}

/// NuttX GPIO Unused Struct
pub struct UnusedPin {
}

/// NuttX Delay Struct
pub struct Delay {
}

/// Open a file and return the file descriptor.
/// TODO: Auto-generate this wrapper with `bindgen` from the C declaration
fn open(path: &str, oflag: i32) -> i32 {  //  `&str` is a reference to a string slice, similar to `const char *` in C

    use crate::open;

    //  Convert `str` to `String`, which similar to `char [64]` in C
    let mut s_with_null = String::from_str(path)  //  `mut` because we will modify it
        .expect("open conversion failed");        //  If it exceeds 64 chars, halt with an error
    
    //  Terminate the string with null, since we will be passing to C
    s_with_null.push('\0')
        .expect("open overflow");  //  If we exceed 64 chars, halt with an error

    //  Convert the null-terminated string to a pointer
    let p = s_with_null.as_str().as_ptr();

    //  Call the C function
    unsafe {  //  Flag this code as unsafe because we're calling a C function
        open(p, oflag)
    }

    //  No semicolon `;` here, so the value returned by the C function will be passed to our caller
}
