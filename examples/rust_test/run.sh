#!/usr/bin/env bash
#  macOS script to build, flash and run Rust Firmware for NuttX on BL602.
#  We use a custom Rust target `riscv32imacf-unknown-none-elf` that sets `llvm-abiname` to `ilp32f` for Single-Precision Hardware Floating-Point.
#  (Because BL602 IoT SDK was compiled with "gcc -march=rv32imfc -mabi=ilp32f")
#  TODO: BL602 is actually RV32-ACFIMX

set -e  #  Exit when any command fails
set -x  #  Echo commands

#  Name of app
export APP_NAME=rust_test

#  Where blflash is located
export BLFLASH_PATH=$PWD/../../../blflash

#  Where GCC is located
export GCC_PATH=$PWD/../../../xpack-riscv-none-embed-gcc

#  Rust build profile: debug or release
rust_build_profile=debug
#  rust_build_profile=release

#  Rust target: Custom target for llvm-abiname=ilp32f
#  https://docs.rust-embedded.org/embedonomicon/compiler-support.html#built-in-target
#  https://docs.rust-embedded.org/embedonomicon/custom-target.html
rust_build_target=$PWD/riscv32imacf-unknown-none-elf.json
rust_build_target_folder=riscv32imacf-unknown-none-elf

set +x  #  Disable echo
echo ; echo "----- Building Rust app and NuttX firmware for $rust_build_target_folder / $APP_NAME..." 

#  Rust target: Standard target
#  rust_build_target=riscv32imac-unknown-none-elf
#  rust_build_target_folder=riscv32imac-unknown-none-elf

#  Rust build options: Build the Rust Core Library for our custom target
rust_build_options="--target $rust_build_target -Z build-std=core"
if [ "$rust_build_profile" == 'release' ]; then
    # Build for release
    rust_build_options="--release $rust_build_options"
#  else 
    # Build for debug: No change in options
fi

#  Location of the Stub Library.  We will replace this stub by the Rust Library
#  rust_app_dest will be set to build_out/rust-app/librust-app.a
rust_app_dir=build_out/rust-app
rust_app_dest=$rust_app_dir/librust-app.a

#  Location of the compiled Rust Library
#  rust_app_build will be set to rust/target/riscv32imacf-unknown-none-elf/debug/libapp.a
rust_build_dir=$PWD/rust/target/$rust_build_target_folder/$rust_build_profile
rust_app_build=$rust_build_dir/libapp.a

#  Remove the Stub Library if it exists:
#  build_out/rust-app/librust-app.a
if [ -e $rust_app_dest ]; then
    rm $rust_app_dest
fi

#  Remove the Rust Library if it exists:
#  rust/target/riscv32imacf-unknown-none-elf/debug/libapp.a
if [ -e $rust_app_build ]; then
    rm $rust_app_build
fi

set +x  #  Disable echo
echo ; echo "----- Build BL602 Firmware"
set -x  #  Enable echo

#  Build the firmware with the Stub Library, ignoring references to the Rust Library
make || echo "----- Ignore undefined references to Rust Library"

set +x  #  Disable echo
echo ; echo "----- Build Rust Library" 
set -x  #  Enable echo

#  Build the Rust Library
pushd rust
rustup default nightly
cargo build $rust_build_options
popd

#  Replace the Stub Library by the compiled Rust Library
#  Stub Library: build_out/rust-app/librust-app.a
#  Rust Library: rust/target/riscv32imacf-unknown-none-elf/debug/libapp.a
cp $rust_app_build $rust_app_dest

set +x  #  Disable echo
echo ; echo "----- Link BL602 Firmware with Rust Library"
set -x  #  Enable echo

#  Link the Rust Library to the firmware
make

#  Generate the disassembly
$GCC_PATH/bin/riscv-none-embed-objdump \
    -t -S --demangle --line-numbers --wide \
    build_out/$APP_NAME.elf \
    >build_out/$APP_NAME.S \
    2>&1

#  Copy firmware to blflash
cp build_out/$APP_NAME.bin $BLFLASH_PATH

set +x  #  Disable echo
echo ; echo "----- Flash BL602 Firmware"
set -x  #  Enable echo

#  Flash the firmware
pushd $BLFLASH_PATH
cargo run flash $APP_NAME.bin \
    --port /dev/tty.usbserial-14* \
    --initial-baud-rate 230400 \
    --baud-rate 230400
sleep 5
popd

set +x  #  Disable echo
echo ; echo "----- Run BL602 Firmware"
set -x  #  Enable echo

#  Run the firmware
open -a CoolTerm

exit

Build Log:
→ ./run.sh 
+ export APP_NAME=sdk_app_rust_adc
+ APP_NAME=sdk_app_rust_adc
+ export CONFIG_CHIP_NAME=BL602
+ CONFIG_CHIP_NAME=BL602
+ export BL60X_SDK_PATH=/Users/Luppy/pinecone/bl_iot_sdk/customer_app/sdk_app_rust_adc/../..
+ BL60X_SDK_PATH=/Users/Luppy/pinecone/bl_iot_sdk/customer_app/sdk_app_rust_adc/../..
+ export BLFLASH_PATH=/Users/Luppy/pinecone/bl_iot_sdk/customer_app/sdk_app_rust_adc/../../../blflash
+ BLFLASH_PATH=/Users/Luppy/pinecone/bl_iot_sdk/customer_app/sdk_app_rust_adc/../../../blflash
+ export GCC_PATH=/Users/Luppy/pinecone/bl_iot_sdk/customer_app/sdk_app_rust_adc/../../../xpack-riscv-none-embed-gcc
+ GCC_PATH=/Users/Luppy/pinecone/bl_iot_sdk/customer_app/sdk_app_rust_adc/../../../xpack-riscv-none-embed-gcc
+ rust_build_profile=debug
+ rust_build_target=/Users/Luppy/pinecone/bl_iot_sdk/customer_app/sdk_app_rust_adc/riscv32imacf-unknown-none-elf.json
+ rust_build_target_folder=riscv32imacf-unknown-none-elf
+ set +x

----- Building Rust app and BL602 firmware for riscv32imacf-unknown-none-elf / sdk_app_rust_adc...

----- Build BL602 Firmware
+ make
use existing version.txt file
use existing version.txt file
AS build_out/bl602/evb/src/boot/gcc/entry.o
AS build_out/bl602/evb/src/boot/gcc/start.o
CC build_out/bl602/evb/src/debug.o
CC build_out/bl602/evb/src/sscanf.o
CC build_out/bl602/evb/src/vsscanf.o
CC build_out/bl602/evb/src/strntoumax.o
AR build_out/bl602/libbl602.a
CC build_out/bl602_std/bl602_std/StdDriver/Src/bl602_uart.o
CC build_out/bl602_std/bl602_std/StdDriver/Src/bl602_adc.o
CC build_out/bl602_std/bl602_std/StdDriver/Src/bl602_sec_eng.o
CC build_out/bl602_std/bl602_std/StdDriver/Src/bl602_dma.o
CC build_out/bl602_std/bl602_std/StdDriver/Src/bl602_common.o
CC build_out/bl602_std/bl602_std/StdDriver/Src/bl602_glb.o
CC build_out/bl602_std/bl602_std/StdDriver/Src/bl602_hbn.o
CC build_out/bl602_std/bl602_std/StdDriver/Src/bl602_timer.o
CC build_out/bl602_std/bl602_std/StdDriver/Src/bl602_aon.o
CC build_out/bl602_std/bl602_std/StdDriver/Src/bl602_pds.o
CC build_out/bl602_std/bl602_std/StdDriver/Src/bl602_pwm.o
CC build_out/bl602_std/bl602_std/StdDriver/Src/bl602_l1c.o
CC build_out/bl602_std/bl602_std/StdDriver/Src/bl602_ef_ctrl.o
CC build_out/bl602_std/bl602_std/StdDriver/Src/bl602_mfg_efuse.o
CC build_out/bl602_std/bl602_std/StdDriver/Src/bl602_mfg_flash.o
CC build_out/bl602_std/bl602_std/StdDriver/Src/bl602_mfg_media.o
CC build_out/bl602_std/bl602_std/StdDriver/Src/bl602_dac.o
CC build_out/bl602_std/bl602_std/StdDriver/Src/bl602_ir.o
CC build_out/bl602_std/bl602_std/StdDriver/Src/bl602_spi.o
CC build_out/bl602_std/bl602_std/StdDriver/Src/bl602_i2c.o
CC build_out/bl602_std/bl602_std/Common/soft_crc/softcrc.o
CC build_out/bl602_std/bl602_std/Common/xz/xz_crc32.o
CC build_out/bl602_std/bl602_std/Common/xz/xz_dec_lzma2.o
CC build_out/bl602_std/bl602_std/Common/xz/xz_dec_stream.o
CC build_out/bl602_std/bl602_std/Common/xz/xz_decompress.o
CC build_out/bl602_std/bl602_std/Common/xz/xz_port.o
CC build_out/bl602_std/bl602_std/Common/cipher_suite/src/bflb_crypt.o
CC build_out/bl602_std/bl602_std/Common/cipher_suite/src/bflb_hash.o
CC build_out/bl602_std/bl602_std/Common/cipher_suite/src/bflb_dsa.o
CC build_out/bl602_std/bl602_std/Common/cipher_suite/src/bflb_ecdsa.o
CC build_out/bl602_std/bl602_std/Common/platform_print/platform_device.o
CC build_out/bl602_std/bl602_std/Common/platform_print/platform_gpio.o
CC build_out/bl602_std/bl602_std/Common/ring_buffer/ring_buffer.o
CC build_out/bl602_std/bl602_std/RISCV/Device/Bouffalo/BL602/Startup/interrupt.o
CC build_out/bl602_std/bl602_std/StdDriver/Src/bl602_romapi.o
CC build_out/bl602_std/bl602_std/StdDriver/Src/bl602_sflash_ext.o
CC build_out/bl602_std/bl602_std/StdDriver/Src/bl602_sf_cfg_ext.o
AR build_out/bl602_std/libbl602_std.a
CC build_out/blfdt/src/fdt.o
CC build_out/blfdt/src/fdt_ro.o
CC build_out/blfdt/src/fdt_wip.o
CC build_out/blfdt/src/fdt_sw.o
CC build_out/blfdt/src/fdt_rw.o
CC build_out/blfdt/src/fdt_strerror.o
CC build_out/blfdt/src/fdt_empty_tree.o
CC build_out/blfdt/src/fdt_addresses.o
CC build_out/blfdt/src/fdt_overlay.o
CC build_out/blfdt/test/tc_blfdt_dump.o
CC build_out/blfdt/test/tc_blfdt_wifi.o
CC build_out/blfdt/test/blfdt_cli_test.o
AR build_out/blfdt/libblfdt.a
CC build_out/blmtd/bl_mtd.o
AR build_out/blmtd/libblmtd.a
CC build_out/blog/blog.o
AR build_out/blog/libblog.a
CC build_out/blog_testc/blog_testc.o
CC build_out/blog_testc/blog_testc1_diable.o
CC build_out/blog_testc/blog_testc2_full.o
CC build_out/blog_testc/blog_testc3_nopri.o
CC build_out/blog_testc/blog_testc4_onlypri.o
AR build_out/blog_testc/libblog_testc.a
CC build_out/bloop/src/bloop_base.o
CC build_out/bloop/src/bloop_handler_sys.o
AR build_out/bloop/libbloop.a
CC build_out/bltime/bl_sys_time.o
CC build_out/bltime/bl_sys_time_cli.o
AR build_out/bltime/libbltime.a
CC build_out/cli/cli/cli.o
AR build_out/cli/libcli.a
CC build_out/easyflash4/src/easyflash.o
CC build_out/easyflash4/src/ef_env.o
CC build_out/easyflash4/src/ef_env_legacy_wl.o
CC build_out/easyflash4/src/ef_env_legacy.o
CC build_out/easyflash4/src/ef_port.o
CC build_out/easyflash4/src/ef_utils.o
CC build_out/easyflash4/src/easyflash_cli.o
AR build_out/easyflash4/libeasyflash4.a
CC build_out/freertos_riscv_ram/event_groups.o
CC build_out/freertos_riscv_ram/list.o
CC build_out/freertos_riscv_ram/queue.o
CC build_out/freertos_riscv_ram/stream_buffer.o
CC build_out/freertos_riscv_ram/tasks.o
CC build_out/freertos_riscv_ram/timers.o
CC build_out/freertos_riscv_ram/misaligned/misaligned_ldst.o
AS build_out/freertos_riscv_ram/misaligned/fp_asm.o
CC build_out/freertos_riscv_ram/panic/panic_c.o
/Users/Luppy/pinecone/bl_iot_sdk/components/bl602/freertos_riscv_ram/panic/panic_c.c: In function 'backtrace_stack_app':
/Users/Luppy/pinecone/bl_iot_sdk/components/bl602/freertos_riscv_ram/panic/panic_c.c:112:8: warning: assignment to 'uintptr_t *' {aka 'unsigned int *'} from 'long unsigned int' makes pointer from integer without a cast [-Wint-conversion]
     pc = fp[-1];
        ^
/Users/Luppy/pinecone/bl_iot_sdk/components/bl602/freertos_riscv_ram/panic/panic_c.c:119:12: warning: comparison between pointer and integer
     if (pc > VALID_FP_START_XIP) {
            ^
/Users/Luppy/pinecone/bl_iot_sdk/components/bl602/freertos_riscv_ram/panic/panic_c.c:124:10: warning: assignment to 'long unsigned int *' from incompatible pointer type 'uintptr_t *' {aka 'unsigned int *'} [-Wincompatible-pointer-types]
       fp = (uintptr_t *)pc;
          ^
/Users/Luppy/pinecone/bl_iot_sdk/components/bl602/freertos_riscv_ram/panic/panic_c.c:127:10: warning: assignment to 'long unsigned int *' from incompatible pointer type 'uintptr_t *' {aka 'unsigned int *'} [-Wincompatible-pointer-types]
       fp = (uintptr_t *)fp[-2];
          ^
/Users/Luppy/pinecone/bl_iot_sdk/components/bl602/freertos_riscv_ram/panic/panic_c.c: In function 'backtrace_now_app':
/Users/Luppy/pinecone/bl_iot_sdk/components/bl602/freertos_riscv_ram/panic/panic_c.c:144:5: warning: 'return' with no value, in function returning non-void
     return;
     ^~~~~~
/Users/Luppy/pinecone/bl_iot_sdk/components/bl602/freertos_riscv_ram/panic/panic_c.c:136:5: note: declared here
 int backtrace_now_app(int (*print_func)(const char *fmt, ...)) {
     ^~~~~~~~~~~~~~~~~
CC build_out/freertos_riscv_ram/portable/GCC/RISC-V/port.o
AS build_out/freertos_riscv_ram/portable/GCC/RISC-V/portASM.o
CC build_out/freertos_riscv_ram/portable/MemMang/heap_5.o
AR build_out/freertos_riscv_ram/libfreertos_riscv_ram.a
CC build_out/hal_drv/bl602_hal/bl_uart.o
CC build_out/hal_drv/bl602_hal/bl_chip.o
CC build_out/hal_drv/bl602_hal/bl_cks.o
CC build_out/hal_drv/bl602_hal/bl_sys.o
CC build_out/hal_drv/bl602_hal/bl_sys_cli.o
CC build_out/hal_drv/bl602_hal/bl_dma.o
CC build_out/hal_drv/bl602_hal/bl_irq.o
CC build_out/hal_drv/bl602_hal/bl_sec.o
CC build_out/hal_drv/bl602_hal/bl_boot2.o
CC build_out/hal_drv/bl602_hal/bl_timer.o
CC build_out/hal_drv/bl602_hal/bl_gpio.o
CC build_out/hal_drv/bl602_hal/bl_gpio_cli.o
CC build_out/hal_drv/bl602_hal/bl_hbn.o
CC build_out/hal_drv/bl602_hal/bl_efuse.o
CC build_out/hal_drv/bl602_hal/bl_flash.o
CC build_out/hal_drv/bl602_hal/bl_pwm.o
CC build_out/hal_drv/bl602_hal/bl_sec_aes.o
CC build_out/hal_drv/bl602_hal/bl_sec_sha.o
CC build_out/hal_drv/bl602_hal/bl_wifi.o
CC build_out/hal_drv/bl602_hal/bl_wdt.o
CC build_out/hal_drv/bl602_hal/bl_wdt_cli.o
CC build_out/hal_drv/bl602_hal/hal_uart.o
CC build_out/hal_drv/bl602_hal/hal_gpio.o
CC build_out/hal_drv/bl602_hal/hal_hbn.o
CC build_out/hal_drv/bl602_hal/hal_pwm.o
CC build_out/hal_drv/bl602_hal/hal_boot2.o
CC build_out/hal_drv/bl602_hal/hal_sys.o
CC build_out/hal_drv/bl602_hal/hal_board.o
CC build_out/hal_drv/bl602_hal/bl_adc.o
CC build_out/hal_drv/bl602_hal/hal_ir.o
CC build_out/hal_drv/bl602_hal/bl_ir.o
CC build_out/hal_drv/bl602_hal/bl_dac_audio.o
CC build_out/hal_drv/bl602_hal/bl_i2c.o
CC build_out/hal_drv/bl602_hal/hal_i2c.o
CC build_out/hal_drv/bl602_hal/hal_button.o
CC build_out/hal_drv/bl602_hal/hal_hbnram.o
CC build_out/hal_drv/bl602_hal/bl_pds.o
CC build_out/hal_drv/bl602_hal/hal_pds.o
CC build_out/hal_drv/bl602_hal/bl_rtc.o
CC build_out/hal_drv/bl602_hal/hal_hwtimer.o
CC build_out/hal_drv/bl602_hal/hal_spi.o
CC build_out/hal_drv/bl602_hal/hal_adc.o
CC build_out/hal_drv/bl602_hal/hal_wifi.o
AR build_out/hal_drv/libhal_drv.a
CC build_out/looprt/src/looprt.o
CC build_out/looprt/src/looprt_test_cli.o
AR build_out/looprt/liblooprt.a
CC build_out/loopset/src/loopset_led.o
CC build_out/loopset/src/loopset_led_cli.o
CC build_out/loopset/src/loopset_ir.o
CC build_out/loopset/src/loopset_pwm.o
CC build_out/loopset/src/loopset_i2c.o
AR build_out/loopset/libloopset.a
CC build_out/nimble-porting-layer/src/nimble_port_freertos.o
CC build_out/nimble-porting-layer/src/npl_os_freertos.o
/Users/Luppy/pinecone/bl_iot_sdk/components/3rdparty/nimble-porting-layer/src/npl_os_freertos.c: In function 'npl_freertos_callout_reset':
/Users/Luppy/pinecone/bl_iot_sdk/components/3rdparty/nimble-porting-layer/src/npl_os_freertos.c:286:15: warning: comparison of unsigned expression < 0 is always false [-Wtype-limits]
     if (ticks < 0) {
               ^
AR build_out/nimble-porting-layer/libnimble-porting-layer.a
CC build_out/romfs/src/bl_romfs.o
AR build_out/romfs/libromfs.a
CC build_out/rust-app/src/rust-app.o
AR build_out/rust-app/librust-app.a
CC build_out/sdk_app_rust_adc/demo.o
CC build_out/sdk_app_rust_adc/main.o
CC build_out/sdk_app_rust_adc/nimble.o
AR build_out/sdk_app_rust_adc/libsdk_app_rust_adc.a
CC build_out/utils/src/utils_hex.o
CC build_out/utils/src/utils_crc.o
CC build_out/utils/src/utils_sha256.o
CC build_out/utils/src/utils_fec.o
CC build_out/utils/src/utils_log.o
CC build_out/utils/src/utils_dns.o
CC build_out/utils/src/utils_list.o
CC build_out/utils/src/utils_rbtree.o
CC build_out/utils/src/utils_hexdump.o
CC build_out/utils/src/utils_time.o
CC build_out/utils/src/utils_notifier.o
CC build_out/utils/src/utils_getopt.o
CC build_out/utils/src/utils_string.o
CC build_out/utils/src/utils_hmac_sha1_fast.o
CC build_out/utils/src/utils_psk_fast.o
CC build_out/utils/src/utils_memp.o
CC build_out/utils/src/utils_tlv_bl.o
AR build_out/utils/libutils.a
CC build_out/vfs/src/vfs.o
CC build_out/vfs/src/vfs_file.o
CC build_out/vfs/src/vfs_inode.o
CC build_out/vfs/src/vfs_register.o
CC build_out/vfs/device/vfs_uart.o
CC build_out/vfs/device/vfs_adc.o
CC build_out/vfs/device/vfs_spi.o
CC build_out/vfs/device/vfs_gpio.o
CC build_out/vfs/device/vfs_pwm.o
AR build_out/vfs/libvfs.a
CC build_out/yloop/src/yloop.o
CC build_out/yloop/src/select.o
CC build_out/yloop/src/aos_freertos.o
CC build_out/yloop/src/device.o
CC build_out/yloop/src/local_event.o
AR build_out/yloop/libyloop.a
LD build_out/sdk_app_rust_adc.elf
/Users/Luppy/pinecone/bl_iot_sdk/toolchain/riscv/Darwin/bin/../lib/gcc/riscv64-unknown-elf/8.3.0/../../../../riscv64-unknown-elf/bin/ld: /Users/Luppy/pinecone/bl_iot_sdk/customer_app/sdk_app_rust_adc/build_out/sdk_app_rust_adc/libsdk_app_rust_adc.a(demo.o):(.static_cli_cmds+0x8): undefined reference to `init_adc'
/Users/Luppy/pinecone/bl_iot_sdk/toolchain/riscv/Darwin/bin/../lib/gcc/riscv64-unknown-elf/8.3.0/../../../../riscv64-unknown-elf/bin/ld: /Users/Luppy/pinecone/bl_iot_sdk/customer_app/sdk_app_rust_adc/build_out/sdk_app_rust_adc/libsdk_app_rust_adc.a(demo.o):(.static_cli_cmds+0x14): undefined reference to `read_adc'
collect2: error: ld returned 1 exit status
make: *** [/Users/Luppy/pinecone/bl_iot_sdk/customer_app/sdk_app_rust_adc/../../make_scripts_riscv/project.mk:420: /Users/Luppy/pinecone/bl_iot_sdk/customer_app/sdk_app_rust_adc/build_out/sdk_app_rust_adc.elf] Error 1
+ echo '----- Ignore undefined references to Rust Library'
----- Ignore undefined references to Rust Library
+ set +x

----- Build Rust Library
+ pushd rust
~/pinecone/bl_iot_sdk/customer_app/sdk_app_rust_adc/rust ~/pinecone/bl_iot_sdk/customer_app/sdk_app_rust_adc
+ rustup default nightly
info: using existing install for 'nightly-x86_64-apple-darwin'
info: default toolchain set to 'nightly-x86_64-apple-darwin'

  nightly-x86_64-apple-darwin unchanged - rustc 1.55.0-nightly (952fdf2a1 2021-07-05)

+ cargo build --target /Users/Luppy/pinecone/bl_iot_sdk/customer_app/sdk_app_rust_adc/riscv32imacf-unknown-none-elf.json -Z build-std=core
   Compiling compiler_builtins v0.1.46
   Compiling core v0.0.0 (/Users/Luppy/.rustup/toolchains/nightly-x86_64-apple-darwin/lib/rustlib/src/rust/library/core)
   Compiling proc-macro2 v1.0.28
   Compiling memchr v2.4.0
   Compiling unicode-xid v0.2.2
   Compiling syn v1.0.74
   Compiling cty v0.2.1
   Compiling heapless v0.7.3
   Compiling rustc-serialize v0.3.24
   Compiling lazy_static v1.4.0
   Compiling cstr_core v0.2.4
   Compiling quote v1.0.9
   Compiling bl602-macros v0.0.2
   Compiling rustc-std-workspace-core v1.99.0 (/Users/Luppy/.rustup/toolchains/nightly-x86_64-apple-darwin/lib/rustlib/src/rust/library/rustc-std-workspace-core)
   Compiling byteorder v1.4.3
   Compiling stable_deref_trait v1.2.0
   Compiling hash32 v0.2.1
   Compiling bl602-sdk v0.0.6
   Compiling app v0.0.1 (/Users/Luppy/pinecone/bl_iot_sdk/customer_app/sdk_app_rust_adc/rust)
    Finished dev [unoptimized + debuginfo] target(s) in 23.55s
+ popd
~/pinecone/bl_iot_sdk/customer_app/sdk_app_rust_adc
+ cp /Users/Luppy/pinecone/bl_iot_sdk/customer_app/sdk_app_rust_adc/rust/target/riscv32imacf-unknown-none-elf/debug/libapp.a build_out/rust-app/librust-app.a
+ set +x

----- Link BL602 Firmware with Rust Library
+ make
use existing version.txt file
LD build_out/sdk_app_rust_adc.elf
Generating BIN File to /Users/Luppy/pinecone/bl_iot_sdk/customer_app/sdk_app_rust_adc/build_out/sdk_app_rust_adc.bin
Requirement already satisfied: fdt>=0.2.0 in /Library/Frameworks/Python.framework/Versions/3.6/lib/python3.6/site-packages (from -r requirements.txt (line 2)) (0.2.0)
Requirement already satisfied: pycryptodomex>=3.9.8 in /Library/Frameworks/Python.framework/Versions/3.6/lib/python3.6/site-packages (from -r requirements.txt (line 3)) (3.9.9)
Requirement already satisfied: toml>=0.10.2 in /Library/Frameworks/Python.framework/Versions/3.6/lib/python3.6/site-packages (from -r requirements.txt (line 4)) (0.10.2)
Requirement already satisfied: configobj>=5.0.6 in /Library/Frameworks/Python.framework/Versions/3.6/lib/python3.6/site-packages (from -r requirements.txt (line 5)) (5.0.6)
Requirement already satisfied: six in /Users/Luppy/Library/Python/3.6/lib/python/site-packages (from configobj>=5.0.6->-r requirements.txt (line 5)) (1.15.0)
You are using pip version 18.1, however version 21.2.1 is available.
You should consider upgrading via the 'pip install --upgrade pip' command.
========= chip flash id: c84015 =========
/Users/Luppy/pinecone/bl_iot_sdk/image_conf/bl602/flash_select/GD25Q16E_c84015.conf
Generating BIN File to /Users/Luppy/pinecone/bl_iot_sdk/customer_app/sdk_app_rust_adc/build_out/ota/dts40M_pt2M_boot2release_c84015/FW_OTA.bin
Generating BIN File to /Users/Luppy/pinecone/bl_iot_sdk/customer_app/sdk_app_rust_adc/build_out/ota/dts40M_pt2M_boot2release_c84015/FW_OTA.bin.ota
Generating BIN File to /Users/Luppy/pinecone/bl_iot_sdk/customer_app/sdk_app_rust_adc/build_out/ota/dts40M_pt2M_boot2release_c84015/FW_OTA.bin.xz
Generating BIN File to /Users/Luppy/pinecone/bl_iot_sdk/customer_app/sdk_app_rust_adc/build_out/ota/dts40M_pt2M_boot2release_c84015/FW_OTA.bin.xz.ota
Generating BIN File to /Users/Luppy/pinecone/bl_iot_sdk/customer_app/sdk_app_rust_adc/build_out/whole_dts40M_pt2M_boot2release_c84015.bin
========= chip flash id: ef6015 =========
/Users/Luppy/pinecone/bl_iot_sdk/image_conf/bl602/flash_select/W25Q16FW_ef6015.conf
Generating BIN File to /Users/Luppy/pinecone/bl_iot_sdk/customer_app/sdk_app_rust_adc/build_out/ota/dts40M_pt2M_boot2release_ef6015/FW_OTA.bin
Generating BIN File to /Users/Luppy/pinecone/bl_iot_sdk/customer_app/sdk_app_rust_adc/build_out/ota/dts40M_pt2M_boot2release_ef6015/FW_OTA.bin.ota
Generating BIN File to /Users/Luppy/pinecone/bl_iot_sdk/customer_app/sdk_app_rust_adc/build_out/ota/dts40M_pt2M_boot2release_ef6015/FW_OTA.bin.xz
Generating BIN File to /Users/Luppy/pinecone/bl_iot_sdk/customer_app/sdk_app_rust_adc/build_out/ota/dts40M_pt2M_boot2release_ef6015/FW_OTA.bin.xz.ota
Generating BIN File to /Users/Luppy/pinecone/bl_iot_sdk/customer_app/sdk_app_rust_adc/build_out/whole_dts40M_pt2M_boot2release_ef6015.bin
========= chip flash id: ef4015 =========
/Users/Luppy/pinecone/bl_iot_sdk/image_conf/bl602/flash_select/W25Q16JV_ef4015.conf
Generating BIN File to /Users/Luppy/pinecone/bl_iot_sdk/customer_app/sdk_app_rust_adc/build_out/ota/dts40M_pt2M_boot2release_ef4015/FW_OTA.bin
Generating BIN File to /Users/Luppy/pinecone/bl_iot_sdk/customer_app/sdk_app_rust_adc/build_out/ota/dts40M_pt2M_boot2release_ef4015/FW_OTA.bin.ota
Generating BIN File to /Users/Luppy/pinecone/bl_iot_sdk/customer_app/sdk_app_rust_adc/build_out/ota/dts40M_pt2M_boot2release_ef4015/FW_OTA.bin.xz
Generating BIN File to /Users/Luppy/pinecone/bl_iot_sdk/customer_app/sdk_app_rust_adc/build_out/ota/dts40M_pt2M_boot2release_ef4015/FW_OTA.bin.xz.ota
Generating BIN File to /Users/Luppy/pinecone/bl_iot_sdk/customer_app/sdk_app_rust_adc/build_out/whole_dts40M_pt2M_boot2release_ef4015.bin
========= chip flash id: ef7015 =========
/Users/Luppy/pinecone/bl_iot_sdk/image_conf/bl602/flash_select/W25Q16JV_ef7015.conf
Generating BIN File to /Users/Luppy/pinecone/bl_iot_sdk/customer_app/sdk_app_rust_adc/build_out/ota/dts40M_pt2M_boot2release_ef7015/FW_OTA.bin
Generating BIN File to /Users/Luppy/pinecone/bl_iot_sdk/customer_app/sdk_app_rust_adc/build_out/ota/dts40M_pt2M_boot2release_ef7015/FW_OTA.bin.ota
Generating BIN File to /Users/Luppy/pinecone/bl_iot_sdk/customer_app/sdk_app_rust_adc/build_out/ota/dts40M_pt2M_boot2release_ef7015/FW_OTA.bin.xz
Generating BIN File to /Users/Luppy/pinecone/bl_iot_sdk/customer_app/sdk_app_rust_adc/build_out/ota/dts40M_pt2M_boot2release_ef7015/FW_OTA.bin.xz.ota
Generating BIN File to /Users/Luppy/pinecone/bl_iot_sdk/customer_app/sdk_app_rust_adc/build_out/whole_dts40M_pt2M_boot2release_ef7015.bin
Building Finish. To flash build output.
+ /Users/Luppy/pinecone/bl_iot_sdk/customer_app/sdk_app_rust_adc/../../../xpack-riscv-none-embed-gcc/bin/riscv-none-embed-objdump -t -S --demangle --line-numbers --wide build_out/sdk_app_rust_adc.elf
+ cp build_out/sdk_app_rust_adc.bin /Users/Luppy/pinecone/bl_iot_sdk/customer_app/sdk_app_rust_adc/../../../blflash
+ set +x

----- Flash BL602 Firmware
+ pushd /Users/Luppy/pinecone/bl_iot_sdk/customer_app/sdk_app_rust_adc/../../../blflash
~/pinecone/blflash ~/pinecone/bl_iot_sdk/customer_app/sdk_app_rust_adc
+ cargo run flash sdk_app_rust_adc.bin --port /dev/tty.usbserial-1420 --initial-baud-rate 230400 --baud-rate 230400
    Finished dev [unoptimized + debuginfo] target(s) in 0.61s
     Running `target/debug/blflash flash sdk_app_rust_adc.bin --port /dev/tty.usbserial-1420 --initial-baud-rate 230400 --baud-rate 230400`
[INFO  blflash::flasher] Start connection...
[TRACE blflash::flasher] 5ms send count 115
[TRACE blflash::flasher] handshake sent elapsed 104.593µs
[INFO  blflash::flasher] Connection Succeed
[INFO  blflash] Bootrom version: 1
[TRACE blflash] Boot info: BootInfo { len: 14, bootrom_version: 1, otp_info: [0, 0, 0, 0, 3, 0, 0, 0, 61, 9d, c0, 5, b9, 18, 1d, 0] }
[INFO  blflash::flasher] Sending eflash_loader...
[INFO  blflash::flasher] Finished 1.595620342s 17.92KB/s
[TRACE blflash::flasher] 5ms send count 115
[TRACE blflash::flasher] handshake sent elapsed 81.908µs
[INFO  blflash::flasher] Entered eflash_loader
[INFO  blflash::flasher] Skip segment addr: 0 size: 47504 sha256 matches
[INFO  blflash::flasher] Skip segment addr: e000 size: 272 sha256 matches
[INFO  blflash::flasher] Skip segment addr: f000 size: 272 sha256 matches
[INFO  blflash::flasher] Erase flash addr: 10000 size: 135808
[INFO  blflash::flasher] Program flash... ed8a4cdacbc4c1543c74584d7297ad876b6731104856a10dff4166c123c6637d
[INFO  blflash::flasher] Program done 7.40735771s 17.91KB/s
[INFO  blflash::flasher] Skip segment addr: 1f8000 size: 5671 sha256 matches
[INFO  blflash] Success
+ sleep 5
+ popd
~/pinecone/bl_iot_sdk/customer_app/sdk_app_rust_adc
+ set +x

----- Run BL602 Firmware
+ open -a CoolTerm
+ exit
