# System / `i3c` I3C Tool

The I3C tool provides a way to debug I3C related problems. This README file will
provide usage information for the I3C tools.

- System Requirements
  - I3C Driver
  - I2C Driver, If Has I2C Device
  - Configuration Options

# Configuration Options

 - `CONFIG_NSH_BUILTIN_APPS` – Build the tools as an NSH built-in command.
 - `CONFIG_I3CTOOL_DEFBUS`   – A default bus number (default `0`).

# Usage:

Write one data byte in single private transfer

i3c -b bus -m manufid -p partid -w 0xde

Write multiple data bytes in single private transfer

i3c -b bus -m manufid -p partid -w "0xde,0xad,0xbe,0xef"

Write multiple data bytes in multiple private transfers

i3c -b bus -m manufid -p partid -w "0xde,0xad" -w "0xbe,0xef"

Read multiple data bytes in single private transfer

i3c -b bus -m manufid -p partid -r <data_length>

Read multiple data bytes in multiple private transfer

i3c -b bus -m manufid -p partid -r <data_length> -r <data_length>

Read and write multiple data bytes in multiple private transfer

i3c -b bus -m manufid -p partid -w "0xde,0xad,0xbe,0xef" -r <data_length>

Get device information using PID

i3c -b bus -m manufid -p partid -g

Parameters:

  1) bus: I3C bus number
  2) manufid: Manufacturer ID (upper 16 bits of device PID)
  3) partid: Part ID (middle 16 bits of device PID)
  4) <data_length>: Data length to read on this message

Note: The manufid and partid are extracted from the device's 48-bit Provisional
ID (PID). To find these values, first use the -g option to get device information,
then extract:
  - manufid = (PID >> 32) & 0xFFFF
  - partid  = (PID >> 16) & 0xFFFF

# Example
  1. i3c -b 0 -m 0x01E0 -p 0x0001 -w 0xde
  2. i3c -b 0 -m 0x01E0 -p 0x0001 -w "0xde,0xad,0xbe,0xef"
  3. i3c -b 0 -m 0x01E0 -p 0x0001 -w "0xde,0xad" -w "0xbe,0xef"
  4. i3c -b 0 -m 0x01E0 -p 0x0001 -r 0x10
  5. i3c -b 0 -m 0x01E0 -p 0x0001 -r 0x10  -r 0x10
  6. i3c -b 0 -m 0x01E0 -p 0x0001 -w "0xde,0xad,0xbe,0xef" -r 0x10
  7. i3c -b 0 -m 0x01E0 -p 0x0001 -g

# Migration from target_addr

Previous versions used `-d targetaddr` to specify the device address. The new
version uses `-m manufid -p partid` to identify devices by their unique PID.

To migrate:
1. Get device PID using the old command (if available):
   ```
   i3c -b 0 -d <addr> -g
   ```
   Output: `i3c_device_info - pid 0x01E000010000XXXX`

2. Extract manufid and partid from PID:
   ```
   manufid = 0x01E0  (bits 47-32)
   partid  = 0x0001  (bits 31-16)
   ```

3. Use new command format:
   ```
   i3c -b 0 -m 0x01E0 -p 0x0001 -r 0x10
   ```
