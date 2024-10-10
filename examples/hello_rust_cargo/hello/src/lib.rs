// no-std libraray for the Rust programming language
#![no_std]

extern crate libc;

// Function hello_rust_cargo without manglng
#[no_mangle]
pub extern "C" fn rust_main() {
    // Print hello world to stdout
    unsafe {
        libc::printf(
            "Hello World from Rust build with Cargo!\n\0"
                .as_ptr()
                .cast(),
        );
    }
}

#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    loop {}
}
