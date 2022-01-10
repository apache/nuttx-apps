//! Macros for NuttX
//! Based on https://github.com/no1wudi/nuttx.rs/blob/main/src/macros.rs

/// Print a formatted message to the serial console
#[macro_export]
macro_rules! println {
    //  If no parameters, print a empty string
    () => {
        $crate::puts_format(format_args!(""))
    };
    //  If 1 parameter (format string), print the format string
    ($s:expr) => {
        $crate::puts_format(format_args!($s))
    };
    //  If 2 or more parameters (format string + args), print the formatted output
    ($s:expr, $($tt:tt)*) => {
        $crate::puts_format(format_args!($s, $($tt)*))
    };
}