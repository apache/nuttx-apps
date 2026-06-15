extern crate serde;
extern crate serde_json;

use core::ffi::{c_char, c_int};
use serde::{Deserialize, Serialize};

#[derive(Serialize, Deserialize)]
struct Person {
    name: String,
    age: u8,
}

// Function hello_rust_cargo without manglng
#[no_mangle]
pub extern "C" fn hello_rust_cargo_main(_argc: c_int, _argv: *mut *mut c_char) -> c_int {
    // Print hello world to stdout

    let john = Person {
        name: "John".to_string(),
        age: 30,
    };

    let json_str = serde_json::to_string(&john).unwrap();
    println!("{}", json_str);

    let jane = Person {
        name: "Jane".to_string(),
        age: 25,
    };

    let json_str_jane = serde_json::to_string(&jane).unwrap();
    println!("{}", json_str_jane);

    let json_data = r#"
        {
            "name": "Alice",
            "age": 28
        }"#;

    let alice: Person = serde_json::from_str(json_data).unwrap();
    println!("Deserialized: {} is {} years old", alice.name, alice.age);

    let pretty_json_str = serde_json::to_string_pretty(&alice).unwrap();
    println!("Pretty JSON:\n{}", pretty_json_str);

    tokio::runtime::Builder::new_current_thread()
        .enable_all()
        .build()
        .unwrap()
        .block_on(async {
            println!("Hello world from tokio!");
        });

    #[cfg(not(feature = "sim"))]
    loop {
        // Do nothing
    }

    #[cfg(feature = "sim")]
    0
}
