# Introduce

This is [ymodem protocal](http://pauillac.inria.fr/~doligez/zmodem/ymodem.txt).
According to it, the sb rb application is realized, which is used to send files and receive files respectively

# Usage

In the ubuntu system, lszrz needs to be installed, can use `sudo apt install lszrz`.
Use minicom to communicate with the board.

## Sendfile to pc

use sb command like this `nsh> sb /tmp/test.c ...`, this command support send multiple files together
then use `<Ctrl + a> , r` chose `ymodem` to receive board file.

## Sendfile to board

use rb cmd like this `nsh> sb`, this command support receive multiple files together
then use `<Ctrl + a> , s` chose `ymodem`, then chose what file need to send.

## help

can use `sb -h` or `rb -h` get help.

# Debug

Because the serial port is used for communication, the log is printed to the debug file
you can use `CONFIG_SYSTEM_YMODEM_DEBUGFILE_PATH` set debug file path.
