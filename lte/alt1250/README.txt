NOTICE:
=======
  This document should be opened in a text editor with a width of at least 150 characters.

alt1250 usrsock daemon
======================

  This software module is a usrsock daemon in NuttX for ALT1250 modem.
  The ALT1250 modem is a modem provided by Sony Semiconductor Israel. And it supports LTE Cat-M1 and NB-IoT network.
  

Implementation Structure
========================
                                                                                 Call registerd functions
                                                                                <-------------------------------------------------+
                                                                                                                                  |
+-------------------------------------------------------------------------------------------------------------------------------- | -------------+
| alt1250_daemon                                                                                                                  |              |
| ~~~~~~~~~~~~~~                                                            Response                                              |              |
|   +----------------------------------------------------------------------------------+                                          |              |
|   |                                                                                  |                                          |              |
|   V  Response    _________________________________________________________           |                            +--------------------------+ |
|   +------------- | usrsock handlers                    | post process    |           |            +-------------> |   Event call back task   | |
|   |              +_____________________________________+_________________+           |            |          +--> |   to an application      | |
|   |              | socket handler                      | postproc_socket |           |            |          |    +--------------------------+ |
|   |           |D | bind handler                        | postproc_bind   |           |            |          |                                 |
|   |           |I | listen handler                      | postproc_listen |  <-----+  |            |          |                                 |
|   |           |S |      .                              |        .        |        |  |            |          |                                 |
|   |           |P |      .                              |        .        |  Call  |  |            |          |                                 |
|   |   +-----> |A |      .                              |        .        |  Back  |  |            | Event    |                                 |
|   |   |       |T |---------------+---------------------+-----------------|        |  |            | Message  |                                 |
|   |   |       |C | ioctl handler | lapi power handler  | postproc_XXXX   |        |  |            |          |                                 |
|   |   |       |H |                 lapi normal handler | postproc_XXXX   |        |  |            |          |                                 |
|   |   |          |               | lapi ifreq handler  | postproc_XXXX   | ___________________________________________________________________ |
|   |   | usrsock  |                 lapi ltecmd handler | postproc_XXXX   | | Command Reply | Reset Event | Asynchronous Event | Select Event | |
|   |   | request  |               |         .           |        .        | |-----------------------------------------------------------------| |
|   |   |          |                         .           |        .        | | Device event handling                                           | |
|   |   |          +-------------------------------------------------------+ +-----------------------------------------------------------------+ |
|   |   |                                    |                    |                                      /                                       |
+-- | - | ---------------------------------- | ------------------ | ------------------------------------ | --------------------------------------+
    |   |                                    |   Send Command     |                                      | Event notify
    V   |                                    V                    V                                      |
-- /dev/usrsock --                   ----------------------------------- /dev/alt1250 -----------------------------------
+----------------+                   +----------------------------------------------------------------------------------+
|   User Sock    |                   |                             ALT1250 Device Driver                                |
+----------------+                   +----------------------------------------------------------------------------------+
                                     +---------------------------------------++-----------------------------------------+
                                     |          SPI Driver                   ||            GPIO Driver                  |
                                     +---------------------------------------++-----------------------------------------+

Behavior Overview
=================

  Basically, alt1250_daemon receives a request from NutX User Sock and performs the expected processing according to the request.

  Each request is processed by its own "usrsock handler" depending on its type, and the result is returned to the User Sock as a
  response. To achieve each of these operations, the "usrsock handler" sends a command to the alt1250 device driver. The post process
  function is registerd in the command to process the command response if necessary.

  The command is sent to the ALT1250 via SPI, and the ALT1250 returns a response according to the command to notify alt1250_daemon.
  The alt1250_daemon receives the command response from that notification and either returns the response to User Sock or calls the
  "post process function" that was registered by the "usrsock handler". "post process" will return a response or send further
  commands to ALT1250 depending on the content of the request.

  The ALT1250 may make event notifications, and these notifications are asynchronous. ALT1250 asynchronous events are notified by the
  device driver as well as command responses, and when the alt1250_daemon receives them, it passes them to a task for callback to
  notify the user application.

