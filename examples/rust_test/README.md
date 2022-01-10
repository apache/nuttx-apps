# Rust Test App for Apache NuttX OS

Read the article...

-   ["Rust on Apache NuttX OS"](https://lupyuen.github.io/articles/rust2)

This repo depends on...

-   [lupyuen/rust-nuttx](https://github.com/lupyuen/rust-nuttx)

To add this repo to your NuttX project...

```bash
cd nuttx/apps/examples
git submodule add https://github.com/lupyuen/rust_test
```

Then update the NuttX Build Config...

```bash
## TODO: Change this to the path of our "incubator-nuttx" folder
cd nuttx/nuttx

## Preserve the Build Config
cp .config ../config

## Erase the Build Config
make distclean

## For BL602: Configure the build for BL602
./tools/configure.sh bl602evb:nsh

## For ESP32: Configure the build for ESP32.
## TODO: Change "esp32-devkitc" to our ESP32 board.
./tools/configure.sh esp32-devkitc:nsh

## Restore the Build Config
cp ../config .config

## Edit the Build Config
make menuconfig 
```

In menuconfig, enable the Rust Test App under "Application Configuration" → "Examples".

In NuttX Shell, enter this to run the app...

```bash
rust_test
```

# Output Log

```bash
nsh> rust_test
Hello from Rust!
Hello World!
test_spi
spi_test_driver_open:
gpout_write: Writing 0
spi_test_driver_write: buflen=5
spi_test_driver_configspi:
bl602_spi_setmode: mode=1
bl602_spi_setbits: nbits=8
bl602_spi_setfrequency: frequency=1000000, actual=0
bl602_spi_select: devid: 0, CS: select
bl602_spi_poll_send: send=1d and recv=44
bl602_spi_poll_send: send=0 and recv=44
bl602_spi_poll_send: send=8 and recv=44
bl602_spi_poll_send: send=0 and recv=44
bl602_spi_poll_send: send=0 and recv=44
bl602_spi_select: devid: 0, CS: free
spi_test_driver_read: buflen=16
gpout_write: Writing 1
test_spi: received
  44
  44
  44
  44
  44
test_spi: SX1262 Register 8 is 0x44
gpout_write: Writing 0
spi_test_driver_write: buflen=5
spi_test_driver_configspi:
bl602_spi_setmode: mode=1
bl602_spi_setbits: nbits=8
bl602_spi_select: devid: 0, CS: select
bl602_spi_poll_send: send=1d and recv=a2
bl602_spi_poll_send: send=0 and recv=a2
bl602_spi_poll_send: send=8 and recv=a2
bl602_spi_poll_send: send=0 and recv=a2
bl602_spi_poll_send: send=0 and recv=80
bl602_spi_select: devid: 0, CS: free
spi_test_driver_read: buflen=16
gpout_write: Writing 1
test_spi: received
  a2
  a2
  a2
  a2
  80
test_spi: SX1262 Register 8 is 0x80
spi_test_driver_close:
test_hal
spi_test_driver_open:
gpout_write: Writing 0
spi_test_driver_write: buflen=5
spi_test_driver_configspi:
bl602_spi_setmode: mode=1
bl602_spi_setbits: nbits=8
bl602_spi_select: devid: 0, CS: select
bl602_spi_poll_send: send=1d and recv=a2
bl602_spi_poll_send: send=0 and recv=a2
bl602_spi_poll_send: send=8 and recv=a2
bl602_spi_poll_send: send=0 and recv=a2
bl602_spi_poll_send: send=0 and recv=80
bl602_spi_select: devid: 0, CS: free
spi_test_driver_read: buflen=5
test_hal: received
  a2
  a2
  a2
  a2
  80
test_hal: SX1262 Register 8 is 0x80
gpout_write: Writing 1
spi_test_driver_close:
test_sx1262
spi_test_driver_open:
Init modem...
gpin_read: Reading...
gpout_write: Writing 0
spi_test_driver_write: buflen=2
spi_test_driver_configspi:
bl602_spi_setmode: mode=1
bl602_spi_setbits: nbits=8
bl602_spi_select: devid: 0, CS: select
bl602_spi_poll_send: send=80 and recv=a2
bl602_spi_poll_send: send=0 and recv=a2
bl602_spi_select: devid: 0, CS: free
gpout_write: Writing 1
gpin_read: Reading...
gpout_write: Writing 0
spi_test_driver_write: buflen=2
spi_test_driver_configspi:
bl602_spi_setmode: mode=1
bl602_spi_setbits: nbits=8
bl602_spi_select: devid: 0, CS: select
bl602_spi_poll_send: send=8a and recv=a2
bl602_spi_poll_send: send=1 and recv=a2
bl602_spi_select: devid: 0, CS: free
gpout_write: Writing 1
gpin_read: Reading...
gpout_write: Writing 0
spi_test_driver_write: buflen=5
spi_test_driver_configspi:
bl602_spi_setmode: mode=1
bl602_spi_setbits: nbits=8
bl602_spi_select: devid: 0, CS: select
bl602_spi_poll_send: send=39 and recv=a2
bl602_spi_poll_send: send=b0 and recv=a2
bl602_spi_poll_send: send=0 and recv=a2
bl602_spi_poll_send: send=0 and recv=a2
bl602_spi_select: devid: 0, CS: free
gpout_write: Writing 1
gpin_read: Reading...
gpout_write: Writing 0
spi_test_driver_write: buflen=2
spi_test_driver_configspi:
bl602_spi_setmode: mode=1
bl602_spi_setbits: nbits=8
bl602_spi_select: devid: 0, CS: select
bl602_spi_poll_send: send=89 and recv=a2
bl602_spi_poll_send: send=7f and recv=a2
bl602_spi_select: devid: 0, CS: free
gpout_write: Writing 1
gpin_read: Reading...
gpout_write: Writing 0
spi_test_driver_write: buflen=3
spi_test_driver_configspi:
bl602_spi_setmode: mode=1
bl602_spi_setbits: nbits=8
bl602_spi_select: devid: 0, CS: select
bl602_spi_poll_send: send=98 and recv=a2
bl602_spi_poll_send: send=e1 and recv=a2
bl602_spi_poll_send: send=e9 and recv=a2
bl602_spi_select: devid: 0, CS: free
gpout_write: Writing 1
gpin_read: Reading...
gpout_write: Wriing 0
spi_test_driver_write: buflen=5
spi_test_driver_configspi:
bl602_spi_setmode: mode=1
bl602_spi_setbits: nbits=8
bl602_spi_select: devid: 0, CS: select
bl602_spi_poll_send: send=95 and recv=a2
l602_spi_poll_send: send=4 and recv=a2
bl602_spi_poll_send: send=0 and recv=a2
bl602_spi_poll_send: send=0 and recv=a2
bl602_spi_poll_send: send=1 and recv=a2
bl602_spi_select: devid: 0, CS: free
gpout_write: Writing 1
gpin_read: Reading...
gpout_write: Writing 0
spi_test_driver_write: buflen=3
spi_test_driver_configspi:
bl602_spi_setmode: mode=1
bl602_spi_setbits: nbits=8
bl602_spi_select: devid: 0, CS: select
bl602_spi_poll_send: send=8e and recv=a2
bl602_spi_poll_send: send=e and recv=a2
bl602_spi_poll_send: send=4 and recv=a2
bl602_spi_select: devid: 0, CS: free
gpout_write: Writing 1
gpin_read: Reading...
gpout_write: Writing 0
spi_test_driver_write: buflen=3
spi_test_driver_configspi:
bl602_spi_setmode: mode=1
bl602_spi_setbits: nbits=8
bl602_spi_select: devid: 0, CS: select
bl602_spi_poll_send: send=8f and recv=a2
bl602_spi_poll_send: send=0 and recv=a2
bl602_spi_poll_send: send=0 and recv=a2
bl602_spi_select: devid: 0, CS: free
gpout_write: Writing 1
gpin_read: Reading...
gpout_write: Writing 0
spi_test_driver_write: buflen=9
spi_test_driver_configspi:
bl602_spi_setmode: mode=1
bl602_spi_setbits: nbits=8
bl602_spi_select: devid: 0, CS: select
bl602_spi_poll_send: send=8b and recv=a2
bl602_spi_poll_send: send=7 and recv=a2
bl602_spi_poll_send: send=4 and recv=a2
bl602_spi_poll_send: send=1 and recv=a2
bl602_spi_poll_send: send=0 and recv=a2
bl602_spi_poll_send: send=0 and recv=a2
bl602_spi_poll_send: send=0 and recv=a2
bl602_spi_poll_send: send=0 and recv=a2
bl602_spi_poll_send: send=0 and recv=a2
bl602_spi_select: devid: 0, CS: free
gpout_write: Writing 1
gpin_read: Reading...
gpout_write: Writing 0
spi_test_driver_write: buflen=10
spi_test_driver_configspi:
bl602_spi_setmode: mode=1
bl602_spi_setbits: nbits=8
bl602_spi_select: devid: 0, CS: select
bl602_spi_poll_send: send=8c and recv=a2
bl602_spi_poll_send: send=0 and recv=a2
bl602_spi_poll_send: send=8 and recv=a2
bl602_spi_poll_send: send=0 and recv=a2
bl602_spi_poll_send: send=0 and recv=a2
bl602_spi_poll_send: send=0 and recv=a2
bl602_spi_poll_send: send=0 and recv=a2
bl602_spi_poll_send: send=0 and recv=a2
bl602_spi_poll_send: send=0 and recv=a2
bl602_spi_poll_send: send=0 and recv=a2
bl602_spi_select: devid: 0, CS: free
gpout_write: Writing 1
gpin_read: Reading...
gpout_write: Writing 0
spi_test_driver_write: buflen=9
spi_test_driver_configspi:
bl602_spi_setmode: mode=1
bl602_spi_setbits: nbits=8
bl602_spi_select: devid: 0, CS: select
bl602_spi_poll_send: send=8 and recv=a2
bl602_spi_poll_send: send=2 and recv=a2
bl602_spi_poll_send: send=3 and recv=a2
bl602_spi_poll_send: send=2 and recv=a2
bl602_spi_poll_send: send=3 and recv=a2
bl602_spi_poll_send: send=0 and recv=a2
bl602_spi_poll_send: send=0 and recv=a2
bl602_spi_poll_send: send=0 and recv=a2
bl602_spi_poll_send: send=0 and recv=a2
bl602_spi_select: devid: 0, CS: free
gpout_write: Writing 1
gpin_read: Reading...
gpout_write: Writing 0
spi_test_driver_write: buflen=2
spi_test_driver_configspi:
bl602_spi_setmode: mode=1
bl602_spi_setbits: nbits=8
bl602_spi_select: devid: 0, CS: select
bl602_spi_poll_send: send=9d and recv=a2
bl602_spi_poll_send: send=1 and recv=a2
bl602_spi_select: devid: 0, CS: free
gpout_write: Writing 1
gpin_read: Reading...
gpout_write: Writing 0
spi_test_driver_write: buflen=5
spi_test_driver_configspi:
bl602_spi_setmode: mode=1
bl602_spi_setbits: nbits=8
bl602_spi_select: devid: 0, CS: select
bl602_spi_poll_send: send=d and recv=a2
bl602_spi_poll_send: send=7 and recv=a2
bl602_spi_poll_send: send=40 and recv=a2
bl602_spi_poll_send: send=14 and recv=a2
bl602_spi_poll_send: send=24 and recv=a2
bl602_spi_select: devid: 0, CS: free
gpout_write: Writing 1
Reading Register 8...
gpin_read: Reading...
gpout_write: Writing 0
spi_test_driver_write: buflen=5
spi_test_driver_configspi:
bl602_spi_setmode: mode=1
bl602_spi_setbits: nbits=8
bl602_spi_select: devid: 0, CS: select
bl602_spi_poll_send: send=1d and recv=a2
bl602_spi_poll_send: send=0 and recv=a2
bl602_spi_poll_send: send=8 and recv=a2
bl602_spi_poll_send: send=0 and recv=a2
bl602_spi_poll_send: send=0 and recv=80
bl602_spi_select: devid: 0, CS: free
spi_test_driver_read: buflen=5
gpout_write: Writing 1
test_sx1262: SX1262 Register 8 is 0x80
Writing Registers...
gpin_read: Reading...
gpout_write: Writing 0
spi_test_driver_write: buflen=4
spi_test_driver_configspi:
bl602_spi_setmode: mode=1
bl602_spi_setbits: nbits=8
bl602_spi_select: devid: 0, CS: select
bl602_spi_poll_send: send=d and recv=a2
bl602_spi_poll_send: send=8 and recv=a2
bl602_spi_poll_send: send=89 and recv=a2
bl602_spi_poll_send: send=4 and recv=a2
bl602_spi_select: devid: 0, CS: free
gpout_write: Writing 1
gpin_read: Reading...
gpout_write: Writing 0
spi_test_driver_write: buflen=4
spi_test_driver_configspi:
bl602_spi_setmode: mode=1
bl602_spi_setbits: nbits=8
bl602_spi_select: devid: 0, CS: select
bl602_spi_poll_send: send=d and recv=a2
bl602_spi_poll_send: send=8 and recv=a2
bl602_spi_poll_send: send=d8 and recv=a2
bl602_spi_poll_send: send=fe and recv=a2
bl602_spi_select: devid: 0, CS: free
gpout_write: Writing 1
gpin_read: Reading...
gpout_write: Writing 0
spi_test_driver_write: buflen=4
spi_test_driver_configspi:
bl602_spi_setmode: mode=1
bl602_spi_setbits: nbits=8
bl602_spi_select: devid: 0, CS: select
bl602_spi_poll_send: send=d and recv=a2
bl602_spi_poll_send: send=8 and recv=a2
bl602_spi_poll_send: send=e7 and recv=a2
bl602_spi_poll_send: send=38 and recv=a2
bl602_spi_select: devid: 0, CS: free
gpout_write: Writing 1
gpin_read: Reading...
gpout_write: Writing 0
spi_test_driver_write: buflen=4
spi_test_driver_configspi:
bl602_spi_setmode: mode=1
bl602_spi_setbits: nbits=8
bl602_spi_select: devid: 0, CS: select
bl602_spi_poll_send: send=d and recv=a2
bl602_spi_poll_send: send=7 and recv=a2
bl602_spi_poll_send: send=36 and recv=a2
bl602_spi_poll_send: send=d and recv=a2
bl602_spi_select: devid: 0, CS: free
gpout_write: Writing 1
Sending LoRa message...
Frequency: 923000000
gpin_read: Reading...
gpout_write: Writing 0
spi_test_driver_write: buflen=27
spi_test_driver_configspi:
bl602_spi_setmode: mode=1
bl602_spi_setbits: nbits=8
bl602_spi_select: devid: 0, CS: select
bl602_spi_poll_send: send=e and recv=a2
bl602_spi_poll_send: send=0 and recv=a2
bl602_spi_poll_send: send=48 and recv=a2
bl602_spi_poll_send: send=65 and recv=a2
bl602_spi_poll_send: send=6c and recv=a2
bl602_spi_poll_send: send=6c and recv=a2
bl602_spi_poll_send: send=6f and recv=a2
bl602_spi_poll_send: send=20 and recv=a2
bl602_spi_poll_send: send=66 and recv=a2
bl602_spi_poll_send: send=72 and recv=a2
bl602_spi_poll_send: send=6f and recv=a2
bl602_spi_poll_send: send=6d and recv=a2
bl602_spi_poll_send: send=20 and recv=a2
bl602_spi_poll_send: send=52 and recv=a2
bl602_spi_poll_send: send=75 and recv=a2
bl602_spi_poll_send: send=73 and recv=a2
bl602_spi_poll_send: send=74 and recv=a2
bl602_spi_poll_send: send=20 and recv=a2
bl602_spi_poll_send: send=6f and recv=a2
bl602_spi_poll_send: send=6e and recv=a2
bl602_spi_poll_send: send=20 and recv=a2
bl602_spi_poll_send: send=4e and recv=a2
bl602_spi_poll_send: send=75 and recv=a2
bl602_spi_poll_send: send=74 and recv=a2
bl602_spi_poll_send: send=74 and recv=a2
bl602_spi_poll_send: send=58 and recv=a2
bl602_spi_poll_send: send=21 and recv=a2
bl602_spi_select: devid: 0, CS: free
gpout_write: Writing 1
gpin_read: Reading...
gpout_write: Writing 0
spi_test_driver_write: buflen=10
spi_test_driver_configspi:
bl602_spi_setmode: mode=1
bl602_spi_setbits: nbits=8
bl602_spi_select: devid: 0, CS: select
bl602_spi_poll_send: send=8c and recv=a2
bl602_spi_poll_send: send=0 and recv=a2
bl602_spi_poll_send: send=8 and recv=a2
bl602_spi_poll_send: send=0 and recv=a2
bl602_spi_poll_send: send=19 and recv=a2
bl602_spi_poll_send: send=1 and recv=a2
bl602_spi_poll_send: send=0 and recv=a2
bl602_spi_poll_send: end=0 and recv=a2
bl602_spi_poll_send: send=0 and recv=a2
bl602_spi_poll_send: send=0 and recv=a2
bl602_spi_select: devid: 0, CS: free
gpout_write: Writing 1
gpin_read: Reading...
gpout_write: Writing 0
spi_test_driver_write: buflen=4
spi_test_driver_configspi:
bl602_spi_setmode: mode=1
bl602_spi_setbits: nbits=8
bl602_spi_select: devid: 0, CS: select
bl602_spi_poll_send: send=83 and recv=a2
bl602_spi_poll_send: send=0 and recv=a2
bl602_spi_poll_send: send=0 and recv=a2
bl602_spi_poll_send: send=0 and recv=a2
bl602_spi_select: devid: 0, CS: free
spi_test_driver_read: buflen=4
gpout_write: Writing 1
gpin_read: Reading...
gpint_read: Reading int pin...
...
gpint_read: Reading int pin...
gpin_read: Reading...
gpout_write: Writing 0
spi_test_driver_write: buflen=3
spi_test_driver_configspi:
bl602_spi_setmode: mode=1
bl602_spi_setbits: nbits=8
bl602_spi_select: devid: 0, CS: select
bl602_spi_poll_send: send=2 and recv=ac
bl602_spi_poll_send: send=ff and recv=ac
bl602_spi_poll_send: send=ff and recv=ac
bl602_spi_select: devid: 0, CS: free
gpout_write: Writing 1
spi_test_driver_close:
Done!
nsh>
```
