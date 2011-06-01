/****************************************************************************
 * apps/netutils/ftpc/ftpc_internal.h
 *
 *   Copyright (C) 2011 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <spudmonkey@racsa.co.cr>
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

#ifndef __APPS_NETUTILS_FTPC_FTPC_INTERNAL_H
#define __APPS_NETUTILS_FTPC_FTPC_INTERNAL_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <wdog.h>

#include <apps/ftpc.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/* MISC definitions *********************************************************/

#define ISO_nl       0x0a
#define ISO_cr       0x0d

/* Telnet-related definitions */

#define TELNET_DM    242
#define TELNET_IP    244
#define TELNET_IAC   255
#define TELNET_WILL  251
#define TELNET_WONT  252
#define TELNET_DO    253
#define TELNET_DONT  254

/* Session flag bits ********************************************************/

#define FTPC_FLAG_CONNECTED (1 << 0)  /* Connected to host */
#define FTPC_FLAG_LOGGEDIN  (1 << 1)  /* Logged in to host */
#define FTPC_STATE_FLAGS    (0x0003)  /* State of connection */

#define FTPC_FLAG_MDTM      (1 << 2)  /* Host supports MDTM command */
#define FTPC_FLAG_SIZE      (1 << 3)  /* Host supports SIZE command */
#define FTPC_FLAG_PASV      (1 << 4)  /* Host supports PASV command */
#define FTPC_FLAG_STOU      (1 << 5)  /* Host supports STOU command */
#define FTPC_FLAG_CHMOD     (1 << 6)  /* Host supports SITE CHMOD command */
#define FTPC_FLAG_IDLE      (1 << 7)  /* Host supports SITE IDLE command */
#define FTPC_HOSTCAP_FLAGS  (0x00fc)  /* Host capabilities */

#define FTPC_FLAG_INTERRUPT (1 << 8)  /* Transfer interrupted */
#define FTPC_FLAG_PUT       (1 << 9)  /* Transfer is a PUT operation (upload) */
#define FTPC_FLAG_PASSIVE   (1 << 10) /* Passive mode requested */
#define FTPC_XFER_FLAGS     (0x0700)  /* Transfer related */

#define FTPC_FLAGS_INIT     FTPC_HOSTCAP_FLAGS

#define FTPC_SET_CONNECTED(s) do { (s)->flags |= FTPC_FLAG_CONNECTED; } while (0)
#define FTPC_SET_LOGGEDIN(s)  do { (s)->flags |= FTPC_FLAG_LOGGEDIN; } while (0)
#define FTPC_SET_MDTM(s)      do { (s)->flags |= FTPC_FLAG_MDTM; } while (0)
#define FTPC_SET_SIZE(s)      do { (s)->flags |= FTPC_FLAG_SIZE; } while (0)
#define FTPC_SET_PASV(s)      do { (s)->flags |= FTPC_FLAG_PASV; } while (0)
#define FTPC_SET_STOU(s)      do { (s)->flags |= FTPC_FLAG_STOU; } while (0)
#define FTPC_SET_CHMOD(s)     do { (s)->flags |= FTPC_FLAG_CHMOD; } while (0)
#define FTPC_SET_IDLE(s)      do { (s)->flags |= FTPC_FLAG_IDLE; } while (0)
#define FTPC_SET_INTERRUPT(s) do { (s)->flags |= FTPC_FLAG_INTERRUPT; } while (0)
#define FTPC_SET_PUT(s)       do { (s)->flags |= FTPC_FLAG_PUT; } while (0)
#define FTPC_SET_PASSIVE(s)   do { (s)->flags |= FTPC_FLAG_PASSIVE; } while (0)

#define FTPC_CLR_CONNECTED(s) do { (s)->flags &= ~FTPC_FLAG_CONNECTED; } while (0)
#define FTPC_CLR_LOGGEDIN(s)  do { (s)->flags &= ~FTPC_FLAG_LOGGEDIN; } while (0)
#define FTPC_CLR_MDTM(s)      do { (s)->flags &= ~FTPC_FLAG_MDTM; } while (0)
#define FTPC_CLR_SIZE(s)      do { (s)->flags &= ~FTPC_FLAG_SIZE; } while (0)
#define FTPC_CLR_PASV(s)      do { (s)->flags &= ~FTPC_FLAG_PASV; } while (0)
#define FTPC_CLR_STOU(s)      do { (s)->flags &= ~FTPC_FLAG_STOU; } while (0)
#define FTPC_CLR_CHMOD(s)     do { (s)->flags &= ~FTPC_FLAG_CHMOD; } while (0)
#define FTPC_CLR_IDLE(s)      do { (s)->flags &= ~FTPC_FLAG_IDLE; } while (0)
#define FTPC_CLR_INTERRUPT(s) do { (s)->flags &= ~FTPC_FLAG_INTERRUPT; } while (0)
#define FTPC_CLR_PUT(s)       do { (s)->flags &= ~FTPC_FLAG_PUT; } while (0)
#define FTPC_CLR_PASSIVE(s)   do { (s)->flags &= ~FTPC_FLAG_PASSIVE; } while (0)

#define FTPC_IS_CONNECTED(s)  (((s)->flags & FTPC_FLAG_CONNECTED) != 0)
#define FTPC_IS_LOGGEDIN(s)   (((s)->flags & FTPC_FLAG_LOGGEDIN) != 0)
#define FTPC_HAS_MDTM(s)      (((s)->flags & FTPC_FLAG_MDTM) != 0)
#define FTPC_HAS_SIZE(s)      (((s)->flags & FTPC_FLAG_SIZE) != 0)
#define FTPC_HAS_PASV(s)      (((s)->flags & FTPC_FLAG_PASV) != 0)
#define FTPC_HAS_STOU(s)      (((s)->flags & FTPC_FLAG_STOU) != 0)
#define FTPC_HAS_CHMOD(s)     (((s)->flags & FTPC_FLAG_CHMOD) != 0)
#define FTPC_HAS_IDLE(s)      (((s)->flags & FTPC_FLAG_IDLE) != 0)
#define FTPC_INTERRUPTED(s)   (((s)->flags & FTPC_FLAG_INTERRUPT) != 0)
#define FTPC_IS_PUT(s)        (((s)->flags & FTPC_FLAG_PUT) != 0)
#define FTPC_IS_PASSIVE(s)    (((s)->flags & FTPC_FLAG_PASSIVE) != 0)

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* This structure represents the state of one socket connection */

struct ftpc_socket_s
{
  int                sd;                   /* Socket descriptor */
  FILE              *instream;             /* Incoming stream */
  FILE              *outstream;            /* Outgoing stream */
  struct sockaddr_in laddr;                /* Local address */
  struct sockaddr_in raddr;                /* Remote address */
  bool               connected;            /* True: socket is connected */
};

/* This structure represents the state of an FTP connection */

struct ftpc_session_s
{
  struct in_addr       addr;       /* Server/proxy IP address */
  struct ftpc_socket_s cmd;        /* FTP command channel */
  struct ftpc_socket_s data;       /* FTP data channel */
  WDOG_ID              wdog;       /* Timer */
  FAR char            *uname;      /* Login uname */
  FAR char            *pwd;        /* Login pwd  */
  FAR char            *initdir;    /* Initial remote directory */
  FAR char            *homedir;    /* Home directory (curdir on startup) */
  FAR char            *curdir;     /* Current directory */
  FAR char            *prevdir;    /* Previous directory */
  FAR char            *rname;      /* Remote file name */
  FAR char            *lname;      /* Local file name */
  uint8_t              xfrmode;    /* Previous data transfer type (See FTPC_XFRMODE_* defines) */
  uint16_t             port;       /* Server/proxy port number (probably 21) */
  uint16_t             flags;      /* Connection flags (see FTPC_FLAGS_* defines) */
  uint16_t             code;       /* Last 3-digit replay code */
  uint32_t             replytimeo; /* Server replay timeout (ticks) */
  uint32_t             conntimeo;  /* Connection timeout (ticks) */
  off_t                filesize;   /* Total file size to transfer */
  off_t                offset;     /* Transfer file offset */
  off_t                size;       /* Number of bytes transferred */
  off_t                rstrsize;   /* restart size */

  char reply[CONFIG_FTP_MAXREPLY+1]; /* Last reply string from server */
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Inline Functions/Function-like Macros
 ****************************************************************************/

#define ftpc_sockconnected(s) \
  ((s) && (s)->connected)
#define ftpc_connected(s) \
  (FTPC_IS_CONNECTED(session) && ftpc_sockconnected(&session->cmd))
#define ftpc_loggedin(s) \
  (ftpc_connected(s) && FTPC_IS_LOGGEDIN(s))

#define ftpc_sockgetc(s) \
  fgetc((s)->instream)
#define ftpc_sockflush(s) \
  fflush((s)->outstream)

/****************************************************************************
 * Public Functions
 ****************************************************************************/
/* Low-level string management */
 
EXTERN void ftpc_stripcrlf(FAR char *str);
EXTERN void ftpc_stripslash(FAR const char *str);
EXTERN FAR char *ftpc_dequote(FAR const char *hostname);

/* FTP helpers */

EXTERN void ftpc_reset(struct ftpc_session_s *session);
EXTERN int ftpc_cmd(struct ftpc_session_s *session, const char *cmd, ...);
EXTERN int fptc_getreply(struct ftpc_session_s *session);
EXTERN void ftpc_curdir(struct ftpc_session_s *session);
EXTERN int ftpc_xfrmode(struct ftpc_session_s *session, uint8_t xfrmode);

EXTERN int ftpc_reconnect(FAR struct ftpc_session_s *session);
EXTERN int ftpc_relogin(FAR struct ftpc_session_s *session);

/* Socket helpers */

EXTERN int ftpc_sockinit(FAR struct ftpc_socket_s *sock);
EXTERN void ftpc_sockclose(FAR struct ftpc_socket_s *sock);
EXTERN int ftpc_sockconnect(FAR struct ftpc_socket_s *sock,
                           FAR struct sockaddr_in *addr);
EXTERN int ftpc_sockgetsockname(FAR struct ftpc_socket_s *sock,
                                FAR struct sockaddr_in *sa);
EXTERN int ftpc_sockaccept(FAR struct ftpc_socket_s *sock,
                           FAR const char *mode, bool passive);
EXTERN int ftpc_socklisten(FAR struct ftpc_socket_s *sock);
EXTERN void ftpc_sockcopy(FAR struct ftpc_socket_s *dest,
                          FAR const struct ftpc_socket_s *src);

/* Socket I/O helpers */

EXTERN int ftpc_sockprintf(FAR struct ftpc_socket_s *sock, const char *str, ...);

/* Timeout handlers */

EXTERN void ftpc_replytimeo(int argc, uint32_t arg1, ...);
EXTERN void ftpc_conntimeo(int argc, uint32_t arg1, ...);

/* Transfer helpers */

EXTERN int ftpc_xfrinit(FAR struct ftpc_session_s *session);
EXTERN int ftpc_recvtext(FAR struct ftpc_session_s *session,
                         FAR FILE *rinstream, FAR FILE *loutstream);
EXTERN int ftpc_waitdata(FAR struct ftpc_session_s *session,
                         FAR FILE *stream, bool rdwait);

EXTERN void ftpc_xfrreset(struct ftpc_session_s *session);
EXTERN int ftpc_xfrabort(FAR struct ftpc_session_s *session,
                         FAR FILE *stream);

#undef EXTERN
#ifdef __cplusplus
}
#endif
#endif /* __APPS_NETUTILS_FTPC_FTPC_INTERNAL_H */
