/****************************************************************************
 * apps/system/zmodem/zm.h
 *
 *   Copyright (C) 2013, 2018 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * References:
 *   "The ZMODEM Inter Application File Transfer Protocol", Chuck Forsberg,
 *    Omen Technology Inc., October 14, 1988
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#ifndef __APPS_SYSTEM_XMODEM_ZM_H
#define __APPS_SYSTEM_XMODEM_ZM_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <sys/types.h>

#include <stdint.h>
#include <debug.h>

#include <nuttx/compiler.h>
#include <nuttx/ascii.h>

#include "system/zmodem.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/* ZModem *******************************************************************/
/* Zmodem ZRINIT flags.  These bits describe the cababilities of the receiver.
 * Reference: Paragraph 11.2:
 */

#define CANFDX        (1 << 0)       /* Rx can send and receive true FDX */
#define CANOVIO       (1 << 1)       /* Rx can receive data during disk I/O */
#define CANBRK        (1 << 2)       /* Rx can send a break signal */
#define CANCRY        (1 << 3)       /* Receiver can decrypt */
#define CANLZW        (1 << 4)       /* Receiver can uncompress */
#define CANFC32       (1 << 5)       /* Receiver can use 32 bit Frame Check */
#define ESCCTL        (1 << 6)       /* Receiver expects ctl chars to be escaped */
#define ESC8          (1 << 7)       /* Receiver expects 8th bit to be escaped */

/* Zmodem ZSINIT flags.  These bits describe the capabilities of the sender */

#define TESCCTL       (1 << 6)       /* Sender needs control chars escaped */
#define TESC8         (1 << 7)       /* Sender needs 8th bit escaped. */

/* ZFILE transfer flags */
/* F0 */

#define ZCBIN         1              /* Binary transfer */
#define ZCNL          2              /* Convert NL to local eol convention */
#define ZCRESUM       3              /* Resume interrupted xfer or append to file. */

/* F1 */

#define ZMNEWL        1              /* Transfer if source newer or longer */
#define ZMCRC         2              /* Transfer if different CRC or length */
#define ZMAPND        3              /* Append to existing file, if any */
#define ZMCLOB        4              /* Replace existing file */
#define ZMNEW         5              /* Transfer if source is newer */
#define ZMDIFF        6              /* Transfer if dates or lengths different */
#define ZMPROT        7              /* Protect: transfer only if dest doesn't exist */
#define ZMCHNG        8              /* Change filename if destination exists */
#define ZMMASK        0x1f
#define ZMSKNOLOC     (1 << 7)       /* Skip if not present at receiving end */

/* F2 */

#define ZTLZW         1              /* lzw compression */
#define ZTRLE         3              /* Run-length encoding */

/* F3 */

#define ZCANVHDR      1              /* Variable headers ok */
#define ZRWOVR        4              /* Byte position for receive window override/256 */
#define ZXSPARS       64             /* Encoding for sparse file ops. */

/* ATTN string special characters.  All other characters sent verbose */

#define ATTNBRK       0xdd           /* Send break signal */
#define ATTNPSE       0xde           /* Pause for one second */

/* Zmodem header types  */

#define ZRQINIT        0             /* Request receive init */
#define ZRINIT         1             /* Receive init */
#define ZSINIT         2             /* Send init sequence, define Attn */
#define ZACK           3             /* ACK */
#define ZFILE          4             /* File name, from sender */
#define ZSKIP          5             /* Skip file command, from receiver */
#define ZNAK           6             /* Last packet was garbled */
#define ZABORT         7             /* Abort */
#define ZFIN           8             /* Finish session */
#define ZRPOS          9             /* Resume file from this position, from receiver */
#define ZDATA         10             /* Data packets to follow, from sender */
#define ZEOF          11             /* End of file, from sender */
#define ZFERR         12             /* Fatal i/o error, from receiver */
#define ZCRC          13             /* Request for file crc, from receiver */
#define ZCHALLENGE    14             /* "Send this number back to me", from receiver */
#define ZCOMPL        15             /* Request is complete */
#define ZCAN          16             /* Other end cancelled with CAN-CAN-CAN-CAN-CAN */
#define ZFREECNT      17             /* Request for free bytes on filesystem */
#define ZCOMMAND      18             /* Command, from sending program */
#define ZSTDERR       19             /* Output this message to stderr */

/* Zmodem character definitions */

#define ZDLE          ASCII_CAN      /* Zmodem escape is CAN */
#define ZDLEE         (ZDLE^0x40)    /* Escaped ZDLE */
#define ZPAD          '*'            /* pad */
#define ZBIN          'A'            /* 16-bit CRC binary header */
#define ZHEX          'B'            /* 16-bit CRC hex header */
#define ZBIN32        'C'            /* 32-bit CRC binary header */
#define ZBINR32       'D'            /* RLE packed binary frame w/32-bit CRC */
#define ZVBIN         'a'            /* Alternate ZBIN */
#define ZVHEX         'b'            /* Alternate ZHEX */
#define ZVBIN32       'c'            /* Alternate ZBIN32 */
#define ZVBINR32      'd'            /* Alternate ZBINR32 */
#define ZRESC         0x7f           /* RLE flag/escape character */

/* ZDLE escape sequences */

#define ZCRCE         'h'           /* CRC next, frame ends, header follows */
#define ZCRCG         'i'           /* CRC next, frame continues nonstop */
#define ZCRCQ         'j'           /* CRC next, send ZACK, frame continues nonstop */
#define ZCRCW         'k'           /* CRC next, send ZACK, frame ends */
#define ZRUB0         'l'           /* Translate to 0x7f */
#define ZRUB1         'm'           /* Translate to 0xff */

/* Implementation ***********************************************************/
/* Zmodem Events (same as frame type + data received and error events) */

#define ZME_RQINIT    ZRQINIT        /* Request receive init */
#define ZME_RINIT     ZRINIT         /* Receive init */
#define ZME_SINIT     ZSINIT         /* Send init sequence, define Attn */
#define ZME_ACK       ZACK           /* ACK */
#define ZME_FILE      ZFILE          /* File name, from sender */
#define ZME_SKIP      ZSKIP          /* Skip file command, from receiver */
#define ZME_NAK       ZNAK           /* Last packet was garbled */
#define ZME_ABORT     ZABORT         /* Abort */
#define ZME_FIN       ZFIN           /* Finish session */
#define ZME_RPOS      ZRPOS          /* Resume file from this position, from receiver */
#define ZME_DATA      ZDATA          /* Data packets to follow, from sender */
#define ZME_EOF       ZEOF           /* End of file, from sender */
#define ZME_FERR      ZFERR          /* Fatal i/o error, from receiver */
#define ZME_CRC       ZCRC           /* Request for file CRC, from receiver */
#define ZME_CHALLENGE ZCHALLENGE     /* "send this number back to me", from receiver */
#define ZME_COMPL     ZCOMPL         /* Request is complete */
#define ZME_CAN       ZCAN           /* Other end cancelled with CAN-CAN-CAN-CAN-CAN */
#define ZME_FREECNT   ZFREECNT       /* Request for free bytes on filesystem */
#define ZME_COMMAND   ZCOMMAND       /* Command, from sending program */
#define ZME_STDERR    ZSTDERR        /* Output this message to stderr */

#define ZME_CANCEL    251            /* Received the cancelation sequence */
#define ZME_OO        252            /* Received OO, terminating the receiver */
#define ZME_DATARCVD  253            /* Data received */
#define ZME_TIMEOUT   254            /* Timeout */
#define ZME_ERROR     255            /* Protocol error */

/* Bit values for flags in struct zm_state_s */

#define ZM_FLAG_CRC32     (1 << 0)   /* Use 32-bit CRC */
#define ZM_FLAG_CRKOK     (1 << 1)   /* CRC is okay */
#define ZM_FLAG_EOF       (1 << 2)   /* End of file reached */
#define ZM_FLAG_ATSIGN    (1 << 3)   /* Last char was '@' */
#define ZM_FLAG_ESCCTRL   (1 << 4)   /* Other end requests ctrl chars be escaped */
#define ZM_FLAG_ESC       (1 << 5)   /* Next character is escaped */
#define ZM_FLAG_WAIT      (1 << 6)   /* Next send should wait */
#define ZM_FLAG_APPEND    (1 << 7)   /* Append to the existing file */
#define ZM_FLAG_TIMEOUT   (1 << 8)   /* A timeout has been detected */
#define ZM_FLAG_OO        (1 << 9)   /* "OO" may be received */

/* The Zmodem parser success/error return code definitions:
 *
 * < 0 :  Transfer terminated due to an error
 * = 0 :  Transfer still in progress
 * > 0 :  Transfer completed successfully
 */

#define ZM_XFRDONE    1              /* Success - Transfer complete */

/* The actual packet buffer size includes 5 bytes to hold the transfer type
 * and the maxmimum size 4-byte CRC.
 */

#define ZM_PKTBUFSIZE (CONFIG_SYSTEM_ZMODEM_PKTBUFSIZE + 5)

/* Debug Definitions ********************************************************/

/* Non-standard debug selectable with CONFIG_DEBUG_ZMODEM.  Debug output goes
 * to stderr (not syslog).  Enabling this kind of debug output if your are
 * trying to use the console device I/O for file transfer is obviously a bad
 * idea (unless, perhaps, you redirect stdin and stdout).
 *
 * See also CONFIG_SYSTEM_ZMODEM_DUMPBUFFER.
 */

#ifdef CONFIG_DEBUG_ZMODEM
#  define zmprintf(format, ...) syslog(LOG_INFO, format, ##__VA_ARGS__)
#  define zmdbg(format, ...)    syslog(LOG_INFO, EXTRA_FMT format EXTRA_ARG, ##__VA_ARGS__)
#else
#  undef CONFIG_SYSTEM_ZMODEM_DUMPBUFFER
#  ifdef CONFIG_CPP_HAVE_VARARGS
#     define zmprintf(x...)
#     define zmdbg(x...)
#  else
#     define zmprintf           (void)
#     define zmdbg              (void)
#  endif
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/
/* The state of the parser */

enum parser_state_e
{
  PSTATE_IDLE = 0,           /* Nothing in progress */
  PSTATE_HEADER,             /* Parsing a header following ZPAD ZDLE */
  PSTATE_DATA,               /* Sending/receiving data */
};

/* PSTATE_IDLE substates */

enum pidle_substate_e
{
  PIDLE_ZPAD = 0,            /* Waiting for ZPAD */
  PIDLE_ZDLE,                /* ZPAD received, waiting for ZDLE */
  PIDLE_OO                   /* First 'O' received, waiting for second 'O' of "OO" */
};

/* PSTATE_HEADER substates */

enum pheader_substate_e
{
  PHEADER_FORMAT = 0,        /* Waiting for the header format {ZBIN, ZBIN32, ZHEX} */
  PHEADER_PAYLOAD,           /* Waiting for header data {Type Pn/Fn CRC} */
  PHEADER_LSPAYLOAD,         /* Waiting for LS nibble of header data (ZHEX only) */
};

/* PSTATE_DATA substates */

enum pdata_substate_e
{
  PDATA_READ = 0,            /* Waiting for ZDLE <packet type> */
  PDATA_CRC                  /* Have the packet type, accumulating the CRC */
};

/* This type describes the method to perform actions at the time of
 * a state transition.
 */

struct zm_state_s;
typedef int (*action_t)(FAR struct zm_state_s *pzm);

/* State transition table entry.  There is one row of the table per possible state.
 * Each row is a row of all reasonable events for this state and long the
 * appropriate state transition and transition action.
 */

struct zm_transition_s
{
  uint8_t    type;           /* Event (Frame type) */
  bool       bdiscard;       /* TRUE: discard buffered input */
  uint8_t    next;           /* Next state */
  action_t   action;         /* Transition action */
};

/* Common state information.  This structure contains all of the top-level
 * information needed by the common Zmodem receive and transmit parsing.
 */

struct zm_state_s
{
  /* User-provided values ***************************************************/

  /* These file/socket descriptors are used to interact with the remote end */

  int fdin;   /* Probably 0 (stdin) */
  int fdout;  /* probably 1 (stdout) */

  /* System internal values *************************************************/

  /* evtable[] is the state transition table that controls the state for this
   * current action.  Different state transitions tables are used for Zmodem
   * vs. XY modem and for receive and for tansmit.
   */

  FAR const struct zm_transition_s * const * evtable;

  /* State information, common fields, plus parser-specific fields.
   * Notes:
   * (1) Only valid during parsing.
   */

  uint8_t  pstate;           /* Current parser state */
  uint8_t  psubstate;        /* Current parser sub-state (1) */
  uint8_t  state;            /* Current transfer state; index into evtable[] */
  uint8_t  timeout;          /* Timeout in seconds for incoming data */
  uint8_t  ncrc;             /* Number of bytes in CRC: 2 or 4 (1) */
  uint8_t  ncan;             /* Number of consecutive CAN chars received (1) */
  uint8_t  hdrfmt;           /* Header format {ZBIN, ZBIN32, or ZHEX} */
  uint8_t  hdrndx;           /* Index into hdrdata (1) */
  uint8_t  hdrdata[9];       /* 1-byte + 4-byte payload + 2- or 4-byte CRC */
  uint8_t  pkttype;          /* Type of data packet {ZCRCW, ZCRCE, ZCRCG, ZCRCQ} */
  uint16_t rcvlen;           /* Number valid bytes in rcvbuf[] */
  uint16_t rcvndx;           /* Index to the next valid bytes in rcvbuf[] (1) */
  uint16_t pktlen;           /* Number valid bytes in pktbuf[] */
  uint16_t flags;            /* See ZM_FLAG_* definitions */
  uint16_t nerrors;          /* Number of data errors */
  timer_t  timer;            /* Watchdog timer */
  int      remfd;            /* The R/W file descritor used for communication with remote */

  /* Buffers.
   *
   * rcvbuf  - Data from the remote peer is receive this buffer
   * pktbuf  - un-escaped remote peer data is parsed into this buffer
   * scratch - Holds data sent to the remote peer.  Since the data is this
   *           buffer is short lived, this buffer may also be used for other
   *           scratch purposes.
   */

  uint8_t  rcvbuf[CONFIG_SYSTEM_ZMODEM_RCVBUFSIZE];
  uint8_t  pktbuf[ZM_PKTBUFSIZE];
  uint8_t  scratch[CONFIG_SYSTEM_ZMODEM_SNDBUFSIZE];
};

/* Receive state information */

struct zmr_state_s
{
  /* Common state data ******************************************************/

  struct zm_state_s cmn;

  /* State data unique to the Zmodem receive implementation *****************/

  uint8_t rcaps;             /* Receiver capabilities */
  uint8_t scaps;             /* Sender capabilities */
  uint8_t f0;                /* Transfer flag F0 */
  uint8_t f1;                /* Transfer flag F1 */
#if 0 /* Not used */
  uint8_t f2;                /* Transfer flag F2 */
  uint8_t f3;                /* Transfer flag F3 */
#endif
  uint8_t ntimeouts;         /* Number of timeouts */
  uint32_t crc;              /* Remove file CRC */
  FAR const char *pathname;  /* Local pathname */
  FAR char *filename;        /* Local filename */
  FAR char *attn;            /* Attention string received from remote peer */
  off_t offset;              /* Current file offset */
  off_t filesize;            /* Remote file size */
#ifdef CONFIG_SYSTEM_ZMODEM_TIMESTAMPS
  time_t timestamp;          /* Remote time stamp */
#endif
  int outfd;                 /* Local output file descriptor */
};

/* Send state information */

struct zms_state_s
{
  /* Common state data ******************************************************/

  struct zm_state_s cmn;

  /* State data unique to the Zmodem send implementation ********************/

  uint8_t dpkttype;          /* Streaming data packet type: ZCRCG, ZCRCQ, or ZCRCW */
  uint8_t fflags[4];         /* File xfer flags */
  uint16_t rcvmax;           /* Max packet size the remote can receive. */
#ifdef CONFIG_SYSTEM_ZMODEM_TIMESTAMPS
  uint32_t timestamp;        /* Local file timestamp */
#endif
#ifdef CONFIG_SYSTEM_ZMODEM_SENDATTN
  FAR char *attn;            /* Attention string */
#endif
  FAR const char *filename;  /* Local filename */
  FAR const char *rfilename; /* Remote filename */
  off_t offset;              /* Current file offset */
  off_t lastoffs;            /* Last acknowledged file offset */
  off_t zrpos;               /* Last offset from ZRPOS */
  off_t filesize;            /* Size of the file to send */
  int infd;                  /* Local input file descriptor */
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/* A handy sequence of 4 zeros */

EXTERN const uint8_t g_zeroes[4];

/* Paragraph 8.4.  Session Abort Sequence
 *
 * "If the receiver is receiving data in streaming mode, the Attn sequence
 *  is executed to interrupt data transmission before the Cancel sequence is
 *  sent.  The Cancel sequence consists of eight CAN characters and ten
 *  backspace characters.  ZMODEM only requires five Cancel characters, the
 *  other three are "insurance".
 *
 * "The trailing backspace characters attempt to erase the effects of the
 *  CAN characters if they are received by a command interpreter.
 */

#define CANISTR_SIZE (8+10)

EXTERN const uint8_t g_canistr[CANISTR_SIZE];

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: zm_bytobe32
 *
 * Description:
 *   Convert a sequence of four bytes into a 32-bit value.  The byte
 *   sequence is assumed to be big-endian.
 *
 ****************************************************************************/

uint32_t zm_bytobe32(FAR const uint8_t *val8);

/****************************************************************************
 * Name: zm_bytobe32
 *
 * Description:
 *   Convert a 32-bit value in a sequence of four bytes in big-endian byte
 *   order.
 *
 ****************************************************************************/

void zm_be32toby(uint32_t val32, FAR uint8_t *val8);

/****************************************************************************
 * Name: zm_encnibble
 *
 * Description:
 *   Encode an 4-bit binary value to a single hex "digit".
 *
 ****************************************************************************/

char zm_encnibble(uint8_t nibble);

/****************************************************************************
 * Name: zm_encnibble
 *
 * Description:
 *   Decode an 4-bit binary value from a single hex "digit".
 *
 ****************************************************************************/

uint8_t zm_decnibble(char hex);

/****************************************************************************
 * Name: zm_puthex8
 *
 * Description:
 *   Convert an 8-bit binary value to 2 hex "digits".
 *
 ****************************************************************************/

FAR uint8_t *zm_puthex8(FAR uint8_t *ptr, uint8_t ch);

/****************************************************************************
 * Name: zm_read
 *
 * Description:
 *   Read a buffer of data from a read-able stream.
 *
 ****************************************************************************/

ssize_t zm_read(int fd, FAR uint8_t *buffer, size_t buflen);

/****************************************************************************
 * Name: zm_getc
 *
 * Description:
 *   Read a one byte of data from a read-able stream.
 *
 ****************************************************************************/

int zm_getc(int fd);

/****************************************************************************
 * Name: zm_write
 *
 * Description:
 *   Write a buffer of data to a write-able stream.
 *
 ****************************************************************************/

ssize_t zm_write(int fd, FAR const uint8_t *buffer, size_t buflen);

/****************************************************************************
 * Name: zm_remwrite
 *
 * Description:
 *   Write a buffer of data to the remote peer.
 *
 ****************************************************************************/

#ifdef CONFIG_SYSTEM_ZMODEM_DUMPBUFFER
ssize_t zm_remwrite(int fd, FAR const uint8_t *buffer, size_t buflen);
#else
#  define zm_remwrite(f,b,s) zm_write(f,b,s)
#endif

/****************************************************************************
 * Name: zm_writefile
 *
 * Description:
 *   Write a buffer of data to file, performing newline conversions as
 *   necessary.
 *
 * NOTE:  Not re-entrant.  CR-LF sequences that span buffer boundaries are
 * not guaranteed to to be handled correctly.
 *
 ****************************************************************************/

int zm_writefile(int fd, FAR const uint8_t *buffer, size_t buflen, bool zcnl);

/****************************************************************************
 * Name: zm_filecrc
 *
 * Description:
 *   Perform CRC32 calculation on a file.
 *
 * Assumptions:
 *   The allocated I/O buffer is available to buffer file data.
 *
 ****************************************************************************/

uint32_t zm_filecrc(FAR struct zm_state_s *pzm, FAR const char *filename);

/****************************************************************************
 * Name: zm_flowc
 *
 * Description:
 *   Enable hardware Rx/Tx flow control.
 *
 *   REVISIT:  Consider returning the original termios settings so that they
 *   could be restored with rx/sz exits.
 *
 ****************************************************************************/

#ifdef CONFIG_SYSTEM_ZMODEM_FLOWC
void zm_flowc(int fd);
#endif

/****************************************************************************
 * Name: zm_putzdle
 *
 * Description:
 *   Transfer a value to a buffer performing ZDLE escaping if necessary
 *
 * Input Parameters:
 *   pzm - Zmodem session state
 *   buffer - Buffer in which to add the possibly escaped character
 *   ch - The raw, unescaped character to be added
 *
 ****************************************************************************/

FAR uint8_t *zm_putzdle(FAR struct zm_state_s *pzm, FAR uint8_t *buffer,
                        uint8_t ch);

/****************************************************************************
 * Name: zm_senddata
 *
 * Description:
 *   Send data to the remote peer performing CRC operations as necessary
 *   (ZBIN or ZBIN32 format assumed, ZCRCW terminator is always used)
 *
 * Input Parameters:
 *   pzm    - Zmodem session state
 *   buffer - Buffer of data to be sent
 *   buflen - The number of bytes in buffer to be sent
 *
 ****************************************************************************/

int zm_senddata(FAR struct zm_state_s *pzm, FAR const uint8_t *buffer,
                size_t buflen);

/****************************************************************************
 * Name: zm_sendhexhdr
 *
 * Description:
 *   Send a ZHEX header to the remote peer performing CRC operations as
 *   necessary.
 *
 * Input Parameters:
 *   pzm    - Zmodem session state
 *   type   - Header type {ZRINIT, ZRQINIT, ZDATA, ZACK, ZNAK, ZCRC, ZRPOS,
 *            ZCOMPL, ZEOF, ZFIN}
 *   buffer - 4-byte buffer of data to be sent
 *
 * Assumptions:
 *   The allocated I/O buffer is available to buffer file data.
 *
 ****************************************************************************/

int zm_sendhexhdr(FAR struct zm_state_s *pzm, int type,
                  FAR const uint8_t *buffer);

/****************************************************************************
 * Name: zm_sendbin16hdr
 *
 * Description:
 *   Send a ZBIN header to the remote peer performing CRC operations as
 *   necessary.  Normally called indirectly through zm_sendbinhdr().
 *
 * Input Parameters:
 *   pzm    - Zmodem session state
 *   type   - Header type {ZSINIT, ZFILE, ZDATA, ZDATA}
 *   buffer - 4-byte buffer of data to be sent
 *
 * Assumptions:
 *   The allocated I/O buffer is available to buffer file data.
 *
 ****************************************************************************/

int zm_sendbin16hdr(FAR struct zm_state_s *pzm, int type,
                    FAR const uint8_t *buffer);

/****************************************************************************
 * Name: zm_sendbin32hdr
 *
 * Description:
 *   Send a ZBIN32 header to the remote peer performing CRC operations as
 *   necessary.  Normally called indirectly through zm_sendbinhdr().
 *
 * Input Parameters:
 *   pzm    - Zmodem session state
 *   type   - Header type {ZSINIT, ZFILE, ZDATA, ZDATA}
 *   buffer - 4-byte buffer of data to be sent
 *
 * Assumptions:
 *   The allocated I/O buffer is available to buffer file data.
 *
 ****************************************************************************/

int zm_sendbin32hdr(FAR struct zm_state_s *pzm, int type,
                    FAR const uint8_t *buffer);

/****************************************************************************
 * Name: zm_sendbinhdr
 *
 * Description:
 *   Send a binary header to the remote peer.  This is a simple wrapping
 *   function for zm_sendbin16hdr() and zm_sendbin32hdr().  It decides on
 *   the correct CRC format and re-directs the call appropriately.
 *
 * Input Parameters:
 *   pzm    - Zmodem session state
 *   type   - Header type {ZSINIT, ZFILE, ZDATA, ZDATA}
 *   buffer - 4-byte buffer of data to be sent
 *
 * Assumptions:
 *   The allocated I/O buffer is available to buffer file data.
 *
 ****************************************************************************/

int zm_sendbinhdr(FAR struct zm_state_s *pzm, int type,
                  FAR const uint8_t *buffer);

/****************************************************************************
 * Name: zm_datapump
 *
 * Description:
 *   Drive the Zmodem state machine by reading data from the remote peer and
 *   providing that data to the parser.  This loop runs until a fatal error
 *   is detected or until the state machine reports that the transfer has
 *   completed successfully.
 *
 ****************************************************************************/

int zm_datapump(FAR struct zm_state_s *pzm);

/****************************************************************************
 * Name: zm_readstate
 *
 * Description:
 *   Enter PSTATE_DATA.
 *
 ****************************************************************************/

void zm_readstate(FAR struct zm_state_s *pzm);

/****************************************************************************
 * Name: zm_timeout
 *
 * Description:
 *   Called by the watchdog logic if/when a timeout is detected.
 *
 ****************************************************************************/

int zm_timeout(FAR struct zm_state_s *pzm);

/****************************************************************************
 * Name: zm_rcvpending
 *
 * Description:
 *   Return true if data from the remote receiver is pending.  In that case,
 *   the local sender should stop data streaming operations and process the
 *   incoming data.
 *
 ****************************************************************************/

#ifdef CONFIG_SYSTEM_ZMODEM_RCVSAMPLE
bool zm_rcvpending(FAR struct zm_state_s *pzm);
#endif

/****************************************************************************
 * Name:  zm_timerinit
 *
 * Description:
 *   Create the POSIX timer used to manage timeouts and attach the SIGALRM
 *   signal handler to catch the timeout events.
 *
 ****************************************************************************/

int zm_timerinit(FAR struct zm_state_s *pzm);

/****************************************************************************
 * Name:  zm_timerstart
 *
 * Description:
 *   Start, restart, or stop the timer.
 *
 ****************************************************************************/

int zm_timerstart(FAR struct zm_state_s *pzm, unsigned int sec);

/****************************************************************************
 * Name:  zm_timerstop
 *
 * Description:
 *   Stop the timer.
 *
 ****************************************************************************/

#define zm_timerstop(p) zm_timerstart(p,0)

/****************************************************************************
 * Name:  zm_timerrelease
 *
 * Description:
 *   Destroy the timer and and detach the signal handler.
 *
 ****************************************************************************/

int zm_timerrelease(FAR struct zm_state_s *pzm);

/****************************************************************************
 * Name:  zm_dumpbuffer
 *
 * Description:
 *  Dump a buffer of zmodem data.
 *
 ****************************************************************************/

#ifdef CONFIG_SYSTEM_ZMODEM_DUMPBUFFER
#  define zm_dumpbuffer(m,b,s) lib_dumpbuffer(m,b,s)
#else
#  define zm_dumpbuffer(m,b,s)
#endif

#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif /* __APPS_SYSTEM_XMODEM_ZM_H */
