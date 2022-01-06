#![no_std]  //  Use the Rust Core Library instead of the Rust Standard Library, which is not compatible with embedded systems

//  Import Libraries
use core::{            //  Rust Core Library
    fmt::Write,        //  String Formatting    
    panic::PanicInfo,  //  Panic Handler
    str::FromStr,      //  For converting `str` to `String`
};

#[no_mangle]                 //  Don't mangle the function name
extern "C" fn rust_main() {  //  Declare `extern "C"` because it will be called by NuttX

    //  Print a message to the serial console
    puts("Hello from Rust!");    

    //  Open GPIO Input for SX1262 Busy Pin
    let busy = unsafe { open(b"/dev/gpio0\0".as_ptr(), O_RDWR) };
    assert!(busy > 0);

    //  Open GPIO Output for SX1262 Chip Select
    let cs = unsafe { open(b"/dev/gpio1\0".as_ptr(), O_RDWR) };
    assert!(cs > 0);  

    //  Open GPIO Interrupt for SX1262 DIO1 Pin
    let dio1 = unsafe { open(b"/dev/gpio2\0".as_ptr(), O_RDWR) };
    assert!(dio1 > 0);
}

/// Print a message to the serial console.
/// TODO: Auto-generate this wrapper with `bindgen` from the C declaration
pub fn puts(s: &str) -> i32 {  //  `&str` is a reference to a string slice, similar to `const char *` in C

    extern "C" {  //  Import C Function
        /// Print a message to the serial console (from C stdio library)
        fn puts(s: *const u8) -> i32;
    }

    //  Convert `str` to `String`, which similar to `char [64]` in C
    let mut s_with_null = String::from_str(s)  //  `mut` because we will modify it
        .expect("puts conversion failed");     //  If it exceeds 64 chars, halt with an error
    
    //  Terminate the string with null, since we will be passing to C
    s_with_null.push('\0')
        .expect("puts overflow");  //  If we exceed 64 chars, halt with an error

    //  Convert the null-terminated string to a pointer
    let p = s_with_null.as_str().as_ptr();

    //  Call the C function
    unsafe {  //  Flag this code as unsafe because we're calling a C function
        puts(p)
    }

    //  No semicolon `;` here, so the value returned by the C function will be passed to our caller
}

/// This function is called on panic, like an assertion failure
#[panic_handler]
fn panic(info: &PanicInfo) -> ! {  //  `!` means that panic handler will never return
    //  Display the filename and line number
    puts("*** Rust Panic:");
    if let Some(location) = info.location() {
        puts(location.file());
        let mut buf = String::new();
        write!(buf, "Line {}", location.line()).expect("buf overflow");
        puts(&buf);    
    } else {
        puts("Unknown location");
    }

    //  Set to true if we are already in the panic handler
    static mut IN_PANIC: bool = false;

    //  Display the payload
    if unsafe { !IN_PANIC } {  //  Prevent panic loop while displaying the payload
        unsafe { IN_PANIC = true };
        if let Some(payload) = info.payload().downcast_ref::<&str>() {
            puts(payload);
        }
    }

    //  Loop forever, do not pass go, do not collect $200
    loop {}
}

extern "C" {  //  Import C Function
    /// Open a file
    fn open(path: *const u8, oflag: i32, ...) -> i32;
}

/// TODO: Import with bindgen from https://github.com/lupyuen/incubator-nuttx/blob/rust/include/fcntl.h
const O_RDONLY: i32 = 1 << 0;        /* Open for read access (only) */
const O_RDOK:   i32 = O_RDONLY;      /* Read access is permitted (non-standard) */
const O_WRONLY: i32 = 1 << 1;        /* Open for write access (only) */
const O_WROK:   i32 = O_WRONLY;      /* Write access is permitted (non-standard) */
const O_RDWR:   i32 = O_RDOK|O_WROK; /* Open for both read & write access */

/// Limit Strings to 64 chars, similar to `char[64]` in C
pub type String = heapless::String::<64>;
