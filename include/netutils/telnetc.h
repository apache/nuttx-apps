/****************************************************************************
 * apps/include/netutils/telnetc.h
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/* Leveraged from libtelnet, https://github.com/seanmiddleditch/libtelnet.
 *
 * The original authors of libtelnet are listed below.  Per their licesne,
 * "The author or authors of this code dedicate any and all copyright
 * interest in this code to the public domain. We make this dedication for
 * the benefit of the public at large and to the detriment of our heirs and
 * successors.  We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * code under copyright law."
 *
 *   Author: Sean Middleditch <sean@sourcemud.org>
 *   (Also listed in the AUTHORS file are Jack Kelly <endgame.dos@gmail.com>
 *   and Katherine Flavel <kate@elide.org>)
 */

/* libtelnet - TELNET protocol handling library
 *
 * SUMMARY:
 *
 * libtelnet is a library for handling the TELNET protocol.  It includes
 * routines for parsing incoming data from a remote peer as well as
 * formatting data to send to the remote peer.
 *
 * libtelnet uses a callback-oriented API, allowing application-specific
 * handling of various events.  The callback system is also used for
 * buffering outgoing protocol data, allowing the application to maintain
 * control over the actual socket connection.
 *
 * Features supported include the full TELNET protocol, Q-method option
 * negotiation, ZMP, MCCP2, MSSP, and NEW-ENVIRON.
 *
 * CONFORMS TO:
 *
 * RFC854  - http://www.faqs.org/rfcs/rfc854.html
 * RFC855  - http://www.faqs.org/rfcs/rfc855.html
 * RFC1091 - http://www.faqs.org/rfcs/rfc1091.html
 * RFC1143 - http://www.faqs.org/rfcs/rfc1143.html
 * RFC1408 - http://www.faqs.org/rfcs/rfc1408.html
 * RFC1572 - http://www.faqs.org/rfcs/rfc1572.html
 *
 * LICENSE:
 *
 * The author or authors of this code dedicate any and all copyright interest
 * in this code to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and successors.
 * We intend this dedication to be an overt act of relinquishment in
 * perpetuity of all present and future rights to this code under copyright
 * law.
 *
 *   Author: Sean Middleditch <sean@sourcemud.org>
 */

#ifndef __APPS_INCLUDE_NETUTILS_TELNETC_H
#define __APPS_INCLUDE_NETUTILS_TELNETC_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdarg.h>

/* C++ support */

#if defined(__cplusplus)
extern "C"
{
#endif

/****************************************************************************
 * Pre-proecessor Definitions
 ****************************************************************************/

/* Telnet commands and special values. */

#define TELNET_IAC                   255
#define TELNET_DONT                  254
#define TELNET_DO                    253
#define TELNET_WONT                  252
#define TELNET_WILL                  251
#define TELNET_SB                    250
#define TELNET_GA                    249
#define TELNET_EL                    248
#define TELNET_EC                    247
#define TELNET_AYT                   246
#define TELNET_AO                    245
#define TELNET_IP                    244
#define TELNET_BREAK                 243
#define TELNET_DM                    242
#define TELNET_NOP                   241
#define TELNET_SE                    240
#define TELNET_EOR                   239
#define TELNET_ABORT                 238
#define TELNET_SUSP                  237
#define TELNET_EOF                   236

/* Telnet options. */

#define TELNET_TELOPT_BINARY         0
#define TELNET_TELOPT_ECHO           1
#define TELNET_TELOPT_RCP            2
#define TELNET_TELOPT_SGA            3
#define TELNET_TELOPT_NAMS           4
#define TELNET_TELOPT_STATUS         5
#define TELNET_TELOPT_TM             6
#define TELNET_TELOPT_RCTE           7
#define TELNET_TELOPT_NAOL           8
#define TELNET_TELOPT_NAOP           9
#define TELNET_TELOPT_NAOCRD         10
#define TELNET_TELOPT_NAOHTS         11
#define TELNET_TELOPT_NAOHTD         12
#define TELNET_TELOPT_NAOFFD         13
#define TELNET_TELOPT_NAOVTS         14
#define TELNET_TELOPT_NAOVTD         15
#define TELNET_TELOPT_NAOLFD         16
#define TELNET_TELOPT_XASCII         17
#define TELNET_TELOPT_LOGOUT         18
#define TELNET_TELOPT_BM             19
#define TELNET_TELOPT_DET            20
#define TELNET_TELOPT_SUPDUP         21
#define TELNET_TELOPT_SUPDUPOUTPUT   22
#define TELNET_TELOPT_SNDLOC         23
#define TELNET_TELOPT_TTYPE          24
#define TELNET_TELOPT_EOR            25
#define TELNET_TELOPT_TUID           26
#define TELNET_TELOPT_OUTMRK         27
#define TELNET_TELOPT_TTYLOC         28
#define TELNET_TELOPT_3270REGIME     29
#define TELNET_TELOPT_X3PAD          30
#define TELNET_TELOPT_NAWS           31
#define TELNET_TELOPT_TSPEED         32
#define TELNET_TELOPT_LFLOW          33
#define TELNET_TELOPT_LINEMODE       34
#define TELNET_TELOPT_XDISPLOC       35
#define TELNET_TELOPT_ENVIRON        36
#define TELNET_TELOPT_AUTHENTICATION 37
#define TELNET_TELOPT_ENCRYPT        38
#define TELNET_TELOPT_NEW_ENVIRON    39
#define TELNET_TELOPT_MSSP           70
#define TELNET_TELOPT_COMPRESS       85
#define TELNET_TELOPT_COMPRESS2      86
#define TELNET_TELOPT_ZMP            93
#define TELNET_TELOPT_EXOPL          255

#define TELNET_TELOPT_MCCP2          86

/* TERMINAL-TYPE codes. */

#define TELNET_TTYPE_IS              0
#define TELNET_TTYPE_SEND            1

/* NEW-ENVIRON/ENVIRON codes. */

#define TELNET_ENVIRON_IS            0
#define TELNET_ENVIRON_SEND          1
#define TELNET_ENVIRON_INFO          2
#define TELNET_ENVIRON_VAR           0
#define TELNET_ENVIRON_VALUE         1
#define TELNET_ENVIRON_ESC           2
#define TELNET_ENVIRON_USERVAR       3

/* MSSP codes. */

#define TELNET_MSSP_VAR              1
#define TELNET_MSSP_VAL              2

/* Telnet state tracker flags. */

#define TELNET_FLAG_PROXY            (1 << 0)
#define TELNET_PFLAG_DEFLATE         (1 << 7)

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Error codes */

enum telnet_error_e
{
  TELNET_EOK = 0,                         /* No error */
  TELNET_EBADVAL,                         /* Invalid parameter, or API misuse */
  TELNET_ENOMEM,                          /* Memory allocation failure */
  TELNET_EOVERFLOW,                       /* Data exceeds buffer size */
  TELNET_EPROTOCOL,                       /* Invalid sequence of special bytes */
  TELNET_ECOMPRESS                        /* Error handling compressed streams */
};

/* Event codes */

enum telnet_event_type_e
{
  TELNET_EV_DATA = 0,                     /* Raw text data has been received */
  TELNET_EV_SEND,                         /* Data needs to be sent to the peer */
  TELNET_EV_IAC,                          /* Generic IAC code received */
  TELNET_EV_WILL,                         /* WILL option negotiation received */
  TELNET_EV_WONT,                         /* WONT option neogitation received */
  TELNET_EV_DO,                           /* DO option negotiation received */
  TELNET_EV_DONT,                         /* DONT option negotiation received */
  TELNET_EV_SUBNEGOTIATION,               /* Sub-negotiation data received */
  TELNET_EV_COMPRESS,                     /* Compression has been enabled */
  TELNET_EV_ZMP,                          /* ZMP command has been received */
  TELNET_EV_TTYPE,                        /* TTYPE command has been received */
  TELNET_EV_ENVIRON,                      /* ENVIRON command has been received */
  TELNET_EV_MSSP,                         /* MSSP command has been received */
  TELNET_EV_WARNING,                      /* Recoverable error has occurred */
  TELNET_EV_ERROR                         /* Non-recoverable error has occurred */
};

/* Environ/MSSP command information */

struct telnet_environ_s
{
  unsigned char type;                     /* either TELNET_ENVIRON_VAR or
                                           * TELNET_ENVIRON_USERVAR */
  char *var;                              /* Name of the variable being set */
  char *value;                            /* value of variable being set; empty string
                                           * if no value */
};

/* State tracker -- private data structure */

struct telnet_s;                           /* Forward reference */

/* Event information */

union telnet_event_u
{
  /* Event type The type field will determine which of the other
   * event structure fields have been filled in.  For instance, if the
   * event type is TELNET_EV_ZMP, then the zmp event field (and ONLY the
   * zmp event field) will be filled in.
   */

  enum telnet_event_type_e type;

  /* Data event: for DATA and SEND events */

  struct
  {
    enum telnet_event_type_e _type;       /* Alias for type */
    const char *buffer;                   /* Byte buffer */
    size_t size;                          /* Number of bytes in buffer */
  } data;

  /* WARNING and ERROR events */

  struct
  {
    enum telnet_event_type_e _type;       /* Alias for type */
    const char *file;                     /* File the error occurred in */
    const char *func;                     /* Function the error occurred in */
    const char *msg;                      /* Error message string */
    int line;                             /* Line of file error occurred on */
    enum telnet_error_e errcode;          /* Error code */
  } error;

  /* Command event: for IAC */

  struct
  {
    enum telnet_event_type_e _type;       /* Alias for type */
    unsigned char cmd;                    /* Telnet command received */
  } iac;

  /* Negotiation event: WILL, WONT, DO, DONT */

  struct
  {
    enum telnet_event_type_e _type;       /* Alias for type */
    unsigned char telopt;                 /* Option being negotiated */
  } neg;

  /* Subnegotiation event */

  struct
  {
    enum telnet_event_type_e _type;       /* Alias for type */
    const char *buffer;                   /* Data of sub-negotiation */
    size_t size;                          /* Number of bytes in buffer */
    unsigned char telopt;                 /* Option code for negotiation */
  } sub;

  /* ZMP event */

  struct
  {
    enum telnet_event_type_e _type;       /* Alias for type */
    const char **argv;                    /* Array of argument string */
    size_t argc;                          /* Number of elements in argv */
  } zmp;

  /* TTYPE event */

  struct
  {
    enum telnet_event_type_e _type;       /* Alias for type */
    unsigned char cmd;                    /* TELNET_TTYPE_IS or TELNET_TTYPE_SEND */
    const char *name;                     /* Terminal type name (IS only) */
  } ttype;

  /* COMPRESS event */

  struct
  {
    enum telnet_event_type_e _type;       /* Alias for type */
    unsigned char state;                  /* 1 if compression is enabled, 0 if
                                           * disabled */
  } compress;

  /* ENVIRON/NEW-ENVIRON event */

  struct
  {
    enum telnet_event_type_e _type;         /* Alias for type */
    const struct telnet_environ_s *values;  /* Array of variable values */
    size_t size;                            /* Number of elements in values */
    unsigned char cmd;                      /* SEND, IS, or INFO */
  } envevent;

  /* MSSP event */

  struct
  {
    enum telnet_event_type_e _type;        /* Alias for type */
    const struct telnet_environ_s *values; /* Array of variable values */
    size_t size;                           /* Number of elements in values */
  } mssp;
};

/* Name: telnet_event_handler_t
 *
 * Description:
 *   This is the type of function that must be passed to
 *   telnet_init() when creating a new telnet object.  The
 *   function will be invoked once for every event generated
 *   by the libtelnet protocol parser.
 *
 * Input Parameters:
 *   telnet    The telnet object that generated the event
 *   event     Event structure with details about the event
 *   user_data User-supplied pointer
 */

typedef void (*telnet_event_handler_t)(struct telnet_s *telnet,
                                       union telnet_event_u *event,
                                       void *user_data);

/* telopt support table element; use telopt of -1 for end marker */

struct telnet_telopt_s
{
  short telopt;                            /* One of the TELOPT codes or -1 */
  unsigned char us;                        /* TELNET_WILL or TELNET_WONT */
  unsigned char him;                       /* TELNET_DO or TELNET_DONT */
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: telnet_init
 *
 * Description:
 *   Initialize a telnet state tracker.
 *
 *   This function initializes a new state tracker, which is used for all
 *   other libtelnet functions.  Each connection must have its own
 *   telnet state tracker object.
 *
 * Input Parameters:
 *   telopts   Table of TELNET options the application supports.
 *   eh        Event handler function called for every event.
 *   flags     0 or TELNET_FLAG_PROXY.
 *   user_data Optional data pointer that will be passsed to eh.
 *
 * Returned Value:
 *   Telent state tracker object.
 *
 ****************************************************************************/

struct telnet_s *telnet_init(const struct telnet_telopt_s *telopts,
                             telnet_event_handler_t eh, unsigned char flags,
                             void *user_data);

/****************************************************************************
 * Name: telnet_free
 *
 * Description:
 *   Free up any memory allocated by a state tracker.
 *
 *   This function must be called when a telnet state tracker is no
 *   longer needed (such as after the connection has been closed) to
 *   release any memory resources used by the state tracker.
 *
 * Input Parameters:
 *   telnet Telnet state tracker object.
 *
 ****************************************************************************/

void telnet_free(struct telnet_s *telnet);

/****************************************************************************
 * Name: telnet_recv
 *
 * Description:
 *   Push a byte buffer into the state tracker.
 *
 *   Passes one or more bytes to the telnet state tracker for
 *   protocol parsing.  The byte buffer is most often going to be
 *   the buffer that recv() was called for while handling the
 *   connection.
 *
 * Input Parameters:
 *   telnet Telnet state tracker object.
 *   buffer Pointer to byte buffer.
 *   size   Number of bytes pointed to by buffer.
 *
 ****************************************************************************/

void telnet_recv(struct telnet_s *telnet, const char *buffer, size_t size);

/****************************************************************************
 * Name: telnet_iac
 *
 * Description:
 *   Send a telnet command.
 *
 * Input Parameters:
 *   telnet Telnet state tracker object.
 *   cmd    Command to send.
 *
 ****************************************************************************/

void telnet_iac(struct telnet_s *telnet, unsigned char cmd);

/****************************************************************************
 * Name: telnet_negotiate
 *
 * Description:
 *   Send negotiation command.
 *
 *   Internally, libtelnet uses RFC1143 option negotiation rules.
 *   The negotiation commands sent with this function may be ignored
 *   if they are determined to be redundant.
 *
 * Input Parameters:
 *   telnet Telnet state tracker object.
 *   cmd    TELNET_WILL, TELNET_WONT, TELNET_DO, or TELNET_DONT.
 *   opt    One of the TELNET_TELOPT_* values.
 *
 ****************************************************************************/

void telnet_negotiate(struct telnet_s *telnet, unsigned char cmd,
                      unsigned char opt);

/****************************************************************************
 * Name: telnet_send
 *
 * Description:
 *   Send non-command data (escapes IAC bytes).
 *
 * Input Parameters:
 *   telnet Telnet state tracker object.
 *   buffer Buffer of bytes to send.
 *   size   Number of bytes to send.
 *
 ****************************************************************************/

void telnet_send(struct telnet_s *telnet, const char *buffer, size_t size);

/****************************************************************************
 * Name: telnet_begin_sb
 *
 * Description:
 *   Begin a sub-negotiation command.
 *
 *   Sends IAC SB followed by the telopt code.  All following data sent
 *   will be part of the sub-negotiation, until telnet_finish_sb() is
 *   called.
 *
 * Input Parameters:
 *   telnet Telnet state tracker object.
 *   telopt One of the TELNET_TELOPT_* values.
 *
 ****************************************************************************/

void telnet_begin_sb(struct telnet_s *telnet, unsigned char telopt);

/****************************************************************************
 * Name: telnet_finish_sb
 *
 * Description:
 *   Finish a sub-negotiation command.
 *
 *   This must be called after a call to telnet_begin_sb() to finish a
 *   sub-negotiation command.
 *
 * Input Parameters:
 *   telnet Telnet state tracker object.
 *
 ****************************************************************************/

#define telnet_finish_sb(telnet) telnet_iac((telnet), TELNET_SE)

/****************************************************************************
 * Name: telnet_subnegotiation
 *
 * Description:
 *   Send a complete subnegotiation buffer.
 *
 *   Equivalent to:
 *     telnet_begin_sb(telnet, telopt);
 *     telnet_send(telnet, buffer, size);
 *     telnet_finish_sb(telnet);
 *
 * Input Parameters:
 *   telnet Telnet state tracker format.
 *   telopt One of the TELNET_TELOPT_* values.
 *   buffer Byte buffer for sub-negotiation data.
 *   size   Number of bytes to use for sub-negotiation data.
 *
 ****************************************************************************/

void telnet_subnegotiation(struct telnet_s *telnet, unsigned char telopt,
                                    const char *buffer, size_t size);

/****************************************************************************
 * Name: telnet_begin_compress2
 *
 * Description:
 *   Begin sending compressed data.
 *
 *   This function will begein sending data using the COMPRESS2 option,
 *   which enables the use of zlib to compress data sent to the client.
 *   The client must offer support for COMPRESS2 with option negotiation,
 *   and zlib support must be compiled into libtelnet.
 *
 *   Only the server may call this command.
 *
 * Input Parameters:
 *   telnet Telnet state tracker object.
 *
 ****************************************************************************/

void telnet_begin_compress2(struct telnet_s *telnet);

/****************************************************************************
 * Name: telnet_printf
 *
 * Description:
 *   Send formatted data.
 *
 *   This function is a wrapper around telnet_send().  It allows using
 *   printf-style formatting.
 *
 *   Additionally, this function will translate \\r to the CR NUL construct
 *   and \\n with CR LF, as well as automatically escaping IAC bytes like
 *   telnet_send().
 *
 * Input Parameters:
 *   telnet Telnet state tracker object.
 *   fmt    Format string.
 *
 * Returned Value:
 *   Number of bytes sent.
 *
 ****************************************************************************/

int telnet_printf(struct telnet_s *telnet, const char *fmt, ...)
    printflike(2, 3);

/****************************************************************************
 * Name: telnet_vprintf
 *
 * Description:
 *   Send formatted data with \r and \n translation in addition to IAC IAC
 *
 *   See telnet_printf().
 *
 ****************************************************************************/

int telnet_vprintf(struct telnet_s *telnet, const char *fmt, va_list va)
    printflike(2, 0);

/****************************************************************************
 * Name: telnet_raw_printf
 *
 * Description:
 *   Send formatted data (no newline escaping).
 *
 *   This behaves identically to telnet_printf(), except that the \\r and \\n
 *   characters are not translated.  The IAC byte is still escaped as normal
 *   with telnet_send().
 *
 * Input Parameters:
 *   telnet Telnet state tracker object.
 *   fmt    Format string.
 *
 * Returned Value:
 *   Number of bytes sent.
 *
 ****************************************************************************/

int telnet_raw_printf(struct telnet_s *telnet, const char *fmt, ...)
    printflike(2, 3);

/****************************************************************************
 * Name: telnet_raw_vprintf
 *
 * Description:
 *   Send formatted data (no newline escaping).
 *
 * See telnet_raw_printf().
 *
 ****************************************************************************/

int telnet_raw_vprintf(struct telnet_s *telnet, const char *fmt, va_list va)
    printflike(2, 0);

/****************************************************************************
 * Name: telnet_begin_newenviron
 *
 * Description:
 *   Begin a new set of NEW-ENVIRON values to request or send.
 *
 *   This function will begin the sub-negotiation block for sending or
 *   requesting NEW-ENVIRON values.
 *
 *   The telnet_finish_newenviron() macro must be called after this
 *   function to terminate the NEW-ENVIRON command.
 *
 * Input Parameters:
 *   telnet Telnet state tracker object.
 *   type   One of TELNET_ENVIRON_SEND, TELNET_ENVIRON_IS, or
 *               TELNET_ENVIRON_INFO.
 *
 ****************************************************************************/

void telnet_begin_newenviron(struct telnet_s *telnet, unsigned char type);

/****************************************************************************
 * Name: telnet_newenviron_value
 *
 * Description:
 *   Send a NEW-ENVIRON variable name or value.
 *
 *   This can only be called between calls to telnet_begin_newenviron() and
 *   telnet_finish_newenviron().
 *
 * Input Parameters:
 *   telnet Telnet state tracker object.
 *   type   One of TELNET_ENVIRON_VAR, TELNET_ENVIRON_USERVAR, or
 *               TELNET_ENVIRON_VALUE.
 *   string Variable name or value.
 *
 ****************************************************************************/

void telnet_newenviron_value(struct telnet_s *telnet, unsigned char type,
                             const char *string);

/****************************************************************************
 * Name: telnet_finish_newenviron
 *
 * Description:
 *   Finish a NEW-ENVIRON command.
 *
 *   This must be called after a call to telnet_begin_newenviron() to finish
 *   a NEW-ENVIRON variable list.
 *
 *   telnet Telnet state tracker object.
 *
 ****************************************************************************/

#define telnet_finish_newenviron(telnet) telnet_finish_sb((telnet))

/****************************************************************************
 * Name: telnet_ttype_send
 *
 * Description:
 *   Send the TERMINAL-TYPE SEND command.
 *
 * Sends the sequence IAC TERMINAL-TYPE SEND.
 *
 *   telnet Telnet state tracker object.
 *
 ****************************************************************************/

void telnet_ttype_send(struct telnet_s * telnet);

/****************************************************************************
 * Name: telnet_ttype_is
 *
 * Description:
 *   Send the TERMINAL-TYPE IS command.
 *
 *   Sends the sequence IAC TERMINAL-TYPE IS "string".
 *
 *   According to the RFC, the recipient of a TERMINAL-TYPE SEND shall
 *   send the next possible terminal-type the client supports.  Upon sending
 *   the type, the client should switch modes to begin acting as the terminal
 *   type is just sent.
 *
 *   The server may continue sending TERMINAL-TYPE IS until it receives a
 *   terminal type is understands.  To indicate to the server that it has
 *   reached the end of the available options, the client must send the last
 *   terminal type a second time.  When the server receives the same terminal
 *   type twice in a row, it knows it has seen all available terminal types.
 *
 *   After the last terminal type is sent, if the client receives another
 *   TERMINAL-TYPE SEND command, it must begin enumerating the available
 *   terminal types from the very beginning.  This allows the server to
 *   scan the available types for a preferred terminal type and, if none
 *   is found, to then ask the client to switch to an acceptable
 *   alternative.
 *
 *   Note that if the client only supports a single terminal type, then
 *   simply sending that one type in response to every SEND will satisfy
 *   the behavior requirements.
 *
 * Input Parameters:
 *   telnet Telnet state tracker object.
 *   ttype  Name of the terminal-type being sent.
 *
 ****************************************************************************/

void telnet_ttype_is(struct telnet_s * telnet, const char *ttype);

/****************************************************************************
 * Name: telnet_send_zmp
 *
 * Description:
 *   Send a ZMP command.
 *
 * Input Parameters:
 *   telnet Telnet state tracker object.
 *   argc   Number of ZMP commands being sent.
 *   argv   Array of argument strings.
 *
 ****************************************************************************/

void telnet_send_zmp(struct telnet_s * telnet, size_t argc,
                              const char **argv);

/****************************************************************************
 * Name: telnet_send_zmpv
 *
 * Description:
 *   Send a ZMP command.
 *
 *   Arguments are listed out in var-args style.  After the last argument, a
 *   NULL pointer must be passed in as a sentinel value.
 *
 * Input Parameters:
 *   telnet Telnet state tracker object.
 *
 ****************************************************************************/

void telnet_send_zmpv(struct telnet_s * telnet, ...);

/****************************************************************************
 * Name: telnet_send_vzmpv
 *
 * Description:
 *   Send a ZMP command.
 *
 *   See telnet_send_zmpv().
 *
 ****************************************************************************/

void telnet_send_vzmpv(struct telnet_s * telnet, va_list va);

/****************************************************************************
 * Name: telnet_begin_zmp
 *
 * Description:
 *   Begin sending a ZMP command
 *
 * Input Parameters:
 *   telnet Telnet state tracker object.
 *   cmd    The first argument (command name) for the ZMP command.
 *
 ****************************************************************************/

void telnet_begin_zmp(struct telnet_s * telnet, const char *cmd);

/****************************************************************************
 * Name: telnet_zmp_arg
 *
 * Description:
 *   Send a ZMP command argument.
 *
 * Input Parameters:
 *   telnet Telnet state tracker object.
 *   arg    Telnet argument string.
 *
 ****************************************************************************/

void telnet_zmp_arg(struct telnet_s * telnet, const char *arg);

/****************************************************************************
 * Name: telnet_finish_zmp
 *
 * Description:
 *   Finish a ZMP command.
 *
 *   This must be called after a call to telnet_begin_zmp() to finish a
 *   ZMP argument list.
 *
 * Input Parameters:
 *   telnet Telnet state tracker object.
 *
 ****************************************************************************/

#define telnet_finish_zmp(telnet) telnet_finish_sb((telnet))

#if defined(__cplusplus)
}
#endif

#endif /* __APPS_INCLUDE_NETUTILS_TELNETC_H */
