The apps/uploadfiles is a simple way to add your file to use inside
NuttX NuttShell (NSH).

Rational: This is a simple solution for user that are using NuttX in
a board that doesn't have SDCard, Flash, Ethernet or other way to
upload files to NuttX and don't want to use ZModem to transfer files
over serial because it is time consuming.

How to use it?
Just copy your file(s) to inside apps/uploadfiles/data/ and enable
the CONFIG_UPLOADFILES in menuconfig. After flashing and starting your
board run the "upload" program and it will create a /data with your files.

