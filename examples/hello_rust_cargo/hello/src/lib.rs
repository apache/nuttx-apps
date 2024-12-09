
// Function hello_rust_cargo without manglng
#[no_mangle]
pub extern "C" fn rust_main() {
    // Print hello world to stdout
   println!("Hello, world! from Rust and Cargo!");
}
