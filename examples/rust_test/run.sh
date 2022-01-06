#!/usr/bin/env bash
##  macOS and WSL script to build, flash and run Rust Firmware for NuttX on BL602.
##  We use a custom Rust target `riscv32imacf-unknown-none-elf` that sets `llvm-abiname` to `ilp32f` for Single-Precision Hardware Floating-Point.
##  (Because NuttX BL602 was compiled with "gcc -march=rv32imafc -mabi=ilp32f")
##  TODO: BL602 is actually RV32-ACFIMX

set -e  ##  Exit when any command fails
set -x  ##  Echo commands

##  Name of app
export APP_NAME=nuttx

##  Path to NuttX
export NUTTX_PATH=$PWD/../../../nuttx

##  Where blflash is located
##  For macOS: export BLFLASH_PATH=$PWD/../../../blflash
##  For WSL:
export BLFLASH_PATH=/mnt/c/pinecone/blflash

##  Where GCC is located
export GCC_PATH=$PWD/../../../xpack-riscv-none-embed-gcc

##  Rust build profile: debug or release
rust_build_profile=debug
##  rust_build_profile=release

##  Rust target: Custom target for llvm-abiname=ilp32f
##  https://docs.rust-embedded.org/embedonomicon/compiler-support.html#built-in-target
##  https://docs.rust-embedded.org/embedonomicon/custom-target.html
rust_build_target=$PWD/riscv32imacf-unknown-none-elf.json
rust_build_target_folder=riscv32imacf-unknown-none-elf

set +x  ##  Disable echo
echo ; echo "----- Building Rust app and NuttX firmware for $rust_build_target_folder / $APP_NAME..." 

##  Rust target: Standard target
##  rust_build_target=riscv32imac-unknown-none-elf
##  rust_build_target_folder=riscv32imac-unknown-none-elf

##  Rust build options: Build the Rust Core Library for our custom target
rust_build_options="--target $rust_build_target -Z build-std=core"
if [ "$rust_build_profile" == 'release' ]; then
    # Build for release
    rust_build_options="--release $rust_build_options"
##  else 
    # Build for debug: No change in options
fi

##  Location of the Stub Library.  We will replace this stub by the Rust Library
##  rust_app_dest will be set to build_out/rust-app/librust-app.a
rust_app_dir=$NUTTX_PATH/staging
rust_app_dest=$rust_app_dir/librust.a

##  Location of the compiled Rust Library
##  rust_app_build will be set to rust/target/riscv32imacf-unknown-none-elf/debug/libapp.a
rust_build_dir=$PWD/rust/target/$rust_build_target_folder/$rust_build_profile
rust_app_build=$rust_build_dir/libapp.a

##  Remove the Stub Library if it exists:
##  build_out/rust-app/librust-app.a
if [ -e $rust_app_dest ]; then
    rm $rust_app_dest
fi

##  Remove the Rust Library if it exists:
##  rust/target/riscv32imacf-unknown-none-elf/debug/libapp.a
if [ -e $rust_app_build ]; then
    rm $rust_app_build
fi

set +x  ##  Disable echo
echo ; echo "----- Build NuttX Firmware"
set -x  ##  Enable echo

##  Build the firmware with the Stub Library, ignoring references to the Rust Library
pushd $NUTTX_PATH
make || echo "----- Ignore undefined references to Rust Library"
popd

set +x  ##  Disable echo
echo ; echo "----- Build Rust Library" 
set -x  ##  Enable echo

##  Build the Rust Library
pushd rust
rustup default nightly
cargo build $rust_build_options
popd

##  Replace the Stub Library by the compiled Rust Library
##  Stub Library: ../../../nuttx/staging/librust.a
##  Rust Library: rust/target/riscv32imacf-unknown-none-elf/debug/libapp.a
cp $rust_app_build $rust_app_dest

set +x  ##  Disable echo
echo ; echo "----- Link NuttX Firmware with Rust Library"
set -x  ##  Enable echo

##  Link the Rust Library to the firmware
pushd $NUTTX_PATH
make
popd

##  Generate the disassembly
## $GCC_PATH/bin/riscv-none-embed-objdump \
##     -t -S --demangle --line-numbers --wide \
##     build_out/$APP_NAME.elf \
##     >build_out/$APP_NAME.S \
##     2>&1

##  Copy firmware to blflash
cp $NUTTX_PATH/$APP_NAME.* $BLFLASH_PATH
ls -l $BLFLASH_PATH/$APP_NAME.bin

set +x  ##  Disable echo
echo ; echo "----- Flash NuttX Firmware"
set -x  ##  Enable echo

##  Flash the firmware
pushd $BLFLASH_PATH
cargo run flash $APP_NAME.bin \
    --port /dev/tty.usbserial-14* \
    --initial-baud-rate 230400 \
    --baud-rate 230400
sleep 5
popd

set +x  ##  Disable echo
echo ; echo "----- Run NuttX Firmware"
set -x  ##  Enable echo

##  Run the firmware
open -a CoolTerm
