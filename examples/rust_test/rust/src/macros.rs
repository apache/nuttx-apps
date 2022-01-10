//! Macros for NuttX
//! Based on https://github.com/no1wudi/nuttx.rs/blob/main/src/macros.rs

/// Print a formatted message to the serial console
#[macro_export]
macro_rules! println {
    () => {
        $crate::puts_format(format_args!(""))
    };
    ($s:expr) => {
        $crate::puts_format(format_args!(concat!($s, "")))
    };
    ($s:expr, $($tt:tt)*) => {
        $crate::puts_format(format_args!(concat!($s, ""), $($tt)*))
    };
}