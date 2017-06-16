IEEE 802.15.4 Swiss Army Knife (i8sak, or i8)
============================================================

Description
===========
The i8sak app is a useful CLI for testing various IEEE 802.15.4 functionality.
It also serves as a starting place for learning how to interface with the
NuttX IEEE 802.15.4 MAC layer.

The i8sak CLI can be used to manipulate multiple MAC layer networks at once.
IEEE 802.15.4 MAC character drivers show up in NuttX as /dev/ieeeN by default.
When you invoke the first call to i8sak with a specified devname, it creates
an i8sak instance and launches a deamon to handle processing work. The instance
is considered sticky, so it is possible to run `i8 /dev/ieee0` at the beginning
of a session and then can exclude the devname from all future calls. The number
of i8sak instances supported is controllable through menuconfig.

The i8sak app has many settings that can be configured. Most options are "sticky",
meaning, if you set the endpoint short address once, any future operation using
the endpoint short address can default to the previously used address. This is
particularly useful to keep the command lengths down.

How To Use
==========
The i8sak app has a series of CLI functions that can be invoked.  The default
i8sak command is 'i8' to make things quick and easy to type.

In my test setup I have 2 Clicker2-STM32 boards from MikroElektronika, with
the BEE-click (MRF24J40) radios. Choose one device to be the PAN Coordinator.
We'll refer to that as device A.

On that device, run:
```
i8 /dev/ieee0 startpan
```
For now, this function assumes that we are operating a non-beacon enabled PAN,
since, as of this writing, beacon-enabled networks are unfinished. Unless you
have previously overriden address settings, the startpan command will also
configure the devices address to be that of CONFIG_I8SAK_PANCOORD_XXX. It
will then set the endpoint to be the CONFIG_I8SAK_DEV_XXX address.

Next, on the same device, run:
```
i8 acceptassoc
```
Notice in the second command, we did not use the devname, again, that is "sticky"
so unless we are switching back and forth between character drivers, we can
just use it once.

The acceptassoc command, without any arguments, informs the i8sak instance to
accept all association requests. The acceptassoc command also allows you to only
accept requests from a single device by specifying the extended address with option
-e.

For instance:
```
i8 acceptassoc -e DEADBEEF00FADE0B
```

But for this example, let's just use the command with no arguments.

Now, the second device will act as an endpoint device. The i8sak instance defaults
to being in endpoint mode. Let's refer to the second device as device B.

On device B, run:
```
i8 /dev/ieee0 assoc
```
This command without any options defaults the endpoint address to the default
PANCOORD address settings, and sends an association request to that device.

If everything is setup correctly, device A should have log information saying
that a device tried to associate and that it accepted the assocation.  On device
B, the console should show that the association request was successful. With all
default settings, device B should have been allocated a short address of 0x000B.

If you are following along with a packet sniffer, you should see the following
transactions:

1) Association Request
    Frame Type      - CMD
    Sequence Number - 0
    Dest. PAN ID    - 0xFADE
    Dest. Address   - 0x000A
    Src.  PAN ID    - 0xFFFE
    Src.  Address   - 0xDEADBEEF00FADE0C
    Command Type    - Association Request

    1a) ACK
        Frame Type      - ACK
        Sequence Number - 0

2) Data Request
    Frame Type      - CMD
    Sequence Number - 1
    Dest. PAN ID    - 0xFADE
    Dest. Address   - 0x000A
    Src.  PAN ID    - 0xFFFE
    Src.  Address   - 0xDEADBEEF00FADE0C
    Command Type    - Data Request

    2a) ACK
        Frame Type      - ACK
        Sequence Number - 1

3) Association Response
    Frame Type      - CMD
    Sequence Number - 0
    Dest. PAN ID    - 0xFADE
    Dest. Address   - 0xDEADBEEF00FADE0C
    Src.  Address   - 0xDEADBEEF00FADE0A
    Command Type    - Association Response
    Assigned SADDR  - 0x000C
    Assoc Status    - Successful

    3a) ACK
        Frame Type      - ACK
        Sequence Number - 0


Device B has now successfully associated with device A. If you want to send data
from device B to device A, run the following on device B:
```
i8 tx ABCDEF
```
This will immediately (not actually immediate, transaction is sent using CSMA)
send the frame to device A with frame payload 0xABCDEF

Sending data from device A to device B is different. In IEEE 802.15.4, frames
must be extracted from the coordinator. To prepare the frame, run the following
command on device A
```
i8 tx AB
```
Because the devmode is PAN Coordinator, the i8sak app knows to send the data
as an indirect transaction.  If you were running the i8sak app on a device
that is a coordinator, but not the PAN coordinator, you can force the i8sak app
to send the transaction directly, rather than to the parent coordinator, by using
the -d option.

NOTE: Currently, the indirect transaction timeout is disabled.  This means frames
must be extracted or space may run out. This is only for the testing phase as it
is easier to debug when I am not fighting a timeout. Re-enabling the timeout may
effect the behavior of the indirect transaction features in the i8sak app.

To extract the data, run the following command on device B:
```
i8 poll
```
This command polls the endpoint (our device A PAN Coordinator in this case) to
see if there is any data. In the console of device B you should see a Poll request
status print out.

