/****************************************************************************
 * apps/netutils/telnetc/telnetc.c
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

/****************************************************************************
 *
 * Leveraged from libtelnet, https://github.com/seanmiddleditch/libtelnet.
 * Modified and re-released under the BSD license:
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
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>

#if defined(HAVE_ZLIB)
#  include <zlib.h>
#endif

#include "netutils/telnetc.h"

/****************************************************************************
 * Pre-proecessor Definitions
 ****************************************************************************/

/* Helper for Q-method option tracking */

#define Q_US(q) (      (q).state & 0x0F)
#define Q_HIM(q)       (((q).state & 0xF0) >> 4)
#define Q_MAKE(us,him) ((us) | ((him) << 4))

/* Helper for the negotiation routines */

#define NEGOTIATE_EVENT(telnet,cmd,opt) \
  ev.type = (cmd); \
  ev.neg.telopt = (opt); \
  (telnet)->eh((telnet), &ev, (telnet)->ud);

/* RFC1143 state names */

#define Q_NO           0
#define Q_YES          1
#define Q_WANTNO       2
#define Q_WANTYES      3
#define Q_WANTNO_OP    4
#define Q_WANTYES_OP   5

/* To send bags of unsigned chars */

#define _sendu(t, d, s) _send((t), (const char*)(d), (s))

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* Telnet state codes */

enum telnet_state_e
{
  TELNET_STATE_DATA = 0,
  TELNET_STATE_IAC,
  TELNET_STATE_WILL,
  TELNET_STATE_WONT,
  TELNET_STATE_DO,
  TELNET_STATE_DONT,
  TELNET_STATE_SB,
  TELNET_STATE_SB_DATA,
  TELNET_STATE_SB_DATA_IAC
};

/* Telnet state tracker */

struct telnet_s
{
  /* User data */

  FAR void *ud;

  /* Telopt support table */

  FAR const struct telnet_telopt_s *telopts;

  /* Event handler */

  telnet_event_handler_t eh;

#if defined(HAVE_ZLIB)
  /* zlib (mccp2) compression */

  FAR z_stream *z;
#endif

  /* RFC1143 option negotiation states */

  FAR struct telnet_rfc1143_s *q;

  /* Sub-request buffer */

  FAR char *buffer;

  /* Current size of the buffer */

  size_t buffer_size;

  /* Current buffer write position (also length of buffer data) */

  size_t buffer_pos;

  /* Current state */

  enum telnet_state_e state;

  /* Option flags */

  unsigned char flags;

  /* Current subnegotiation telopt */

  unsigned char sb_telopt;

  /* Length of RFC1143 queue */

  unsigned char q_size;
};

/* RFC1143 option negotiation state */

struct telnet_rfc1143_s
{
  unsigned char telopt;
  unsigned char state;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Buffer sizes */

static const size_t _buffer_sizes[] =
{
  0, 512, 2048, 8192, 16384,
};

static const size_t _buffer_sizes_count = sizeof(_buffer_sizes) /
  sizeof(_buffer_sizes[0]);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Error generation function */

static enum telnet_error_e _error(FAR struct telnet_s *telnet, unsigned line,
                                  FAR const char *func,
                                  enum telnet_error_e err, int fatal,
                                  FAR const char *fmt, ...)
{
  union telnet_event_u ev;
  char buffer[512];
  va_list va;

  /* Format informational text */

  va_start(va, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, va);
  va_end(va);

  /* Send error event to the user */

  ev.type = fatal ? TELNET_EV_ERROR : TELNET_EV_WARNING;
  ev.error.file = __FILE__;
  ev.error.func = func;
  ev.error.line = line;
  ev.error.msg = buffer;
  telnet->eh(telnet, &ev, telnet->ud);

  return err;
}

/* Initialize the zlib box for a telnet box; if deflate is non-zero, it
 * initializes zlib for delating (compression), otherwise for inflating
 * (decompression).  returns TELNET_EOK on success, something else on
 * failure.
 */

#if defined(HAVE_ZLIB)
enum telnet_error_e _init_zlib(FAR struct telnet_s *telnet, int deflate,
                               int err_fatal)
{
  FAR z_stream *z;
  int rs;

  /* If compression is already enabled, fail loudly */

  if (telnet->z != 0)
    {
      return _error(telnet, __LINE__, __func__, TELNET_EBADVAL,
                    err_fatal, "cannot initialize compression twice");
    }

  /* Allocate zstream box */

  if ((z = (z_stream *) calloc(1, sizeof(z_stream))) == 0)
    {
      return _error(telnet, __LINE__, __func__, TELNET_ENOMEM, err_fatal,
                    "malloc() failed: %d", errno);
    }

  /* Initialize */

  if (deflate)
    {
      if ((rs = deflateInit(z, Z_DEFAULT_COMPRESSION)) != Z_OK)
        {
          free(z);
          return _error(telnet, __LINE__, __func__, TELNET_ECOMPRESS,
                        err_fatal, "deflateInit() failed: %s", zError(rs));
        }

      telnet->flags |= TELNET_PFLAG_DEFLATE;
    }
  else
    {
      if ((rs = inflateInit(z)) != Z_OK)
        {
          free(z);
          return _error(telnet, __LINE__, __func__, TELNET_ECOMPRESS,
                        err_fatal, "inflateInit() failed: %s", zError(rs));
        }

      telnet->flags &= ~TELNET_PFLAG_DEFLATE;
    }

  telnet->z = z;
  return TELNET_EOK;
}
#endif /* HAVE_ZLIB */

/* Push bytes out, compressing them first if need be */

static void _send(FAR struct telnet_s *telnet, FAR const char *buffer,
                  size_t size)
{
  union telnet_event_u ev;

#if defined(HAVE_ZLIB)
  /* If we have a deflate (compression) zlib box, use it */

  if (telnet->z != 0 && telnet->flags & TELNET_PFLAG_DEFLATE)
    {
      char deflate_buffer[1024];
      int rs;

      /* Initialize z state */

      telnet->z->next_in   = (unsigned char *)buffer;
      telnet->z->avail_in  = (unsigned int)size;
      telnet->z->next_out  = (unsigned char *)deflate_buffer;
      telnet->z->avail_out = sizeof(deflate_buffer);

      /* Deflate until buffer exhausted and all output is produced */

      while (telnet->z->avail_in > 0 || telnet->z->avail_out == 0)
        {
          /* Compress */

          if ((rs = deflate(telnet->z, Z_SYNC_FLUSH)) != Z_OK)
            {
              _error(telnet, __LINE__, __func__, TELNET_ECOMPRESS, 1,
                     "deflate() failed: %s", zError(rs));
              deflateEnd(telnet->z);
              free(telnet->z);
              telnet->z = 0;
              break;
            }

          /* Send event */

          ev.type        = TELNET_EV_SEND;
          ev.data.buffer = deflate_buffer;
          ev.data.size   = sizeof(deflate_buffer) - telnet->z->avail_out;
          telnet->eh(telnet, &ev, telnet->ud);

          /* Prepare output buffer for next run */

          telnet->z->next_out  = (unsigned char *)deflate_buffer;
          telnet->z->avail_out = sizeof(deflate_buffer);
        }

      /* Do not continue with remaining code */

      return;
    }
#endif /* HAVE_ZLIB */

  ev.type        = TELNET_EV_SEND;
  ev.data.buffer = buffer;
  ev.data.size   = size;
  telnet->eh(telnet, &ev, telnet->ud);
}

/* Check if we support a particular telopt; if us is non-zero, we
 * check if we (local) supports it, otherwise we check if he (remote)
 * supports it.  return non-zero if supported, zero if not supported.
 */

static inline int _check_telopt(FAR struct telnet_s *telnet,
                                unsigned char telopt, int us)
{
  int i;

  /* If we have no telopts table, we obviously don't support it */

  if (telnet->telopts == 0)
    {
      return 0;
    }

  /* Loop until found or end marker (us and him both 0) */

  for (i = 0; telnet->telopts[i].telopt != -1; ++i)
    {
      if (telnet->telopts[i].telopt == telopt)
        {
          if (us && telnet->telopts[i].us == TELNET_WILL)
            {
              return 1;
            }

          else if (!us && telnet->telopts[i].him == TELNET_DO)
            {
              return 1;
            }
          else
            {
              return 0;
            }
        }
    }

  /* Not found, so not supported */

  return 0;
}

/* Retrieve RFC1143 option state */

static inline struct telnet_rfc1143_s _get_rfc1143(FAR struct telnet_s *telnet,
                                                   unsigned char telopt)
{
  struct telnet_rfc1143_s empty;
  int i;

  /* Search for entry */

  for (i = 0; i != telnet->q_size; ++i)
    {
      if (telnet->q[i].telopt == telopt)
        {
          return telnet->q[i];
        }
    }

  /* Not found, return empty value */

  empty.telopt = telopt;
  empty.state  = 0;
  return empty;
}

/* Save RFC1143 option state */

static inline void _set_rfc1143(FAR struct telnet_s *telnet,
                                unsigned char telopt, char us, char him)
{
  struct telnet_rfc1143_s *qtmp;
  int i;

  /* Search for entry */

  for (i = 0; i != telnet->q_size; ++i)
    {
      if (telnet->q[i].telopt == telopt)
        {
          telnet->q[i].state = Q_MAKE(us, him);
          return;
        }
    }

  /* we're going to need to track state for it, so grow the queue by 4 (four)
   * elements and put the telopt into it; bail on allocation error.  we go by
   * four because it seems like a reasonable guess as to the number of enabled
   * options for most simple code, and it allows for an acceptable number of
   * reallocations for complex code.
   */

  qtmp = (struct telnet_rfc1143_s *)
           realloc(telnet->q,
                   sizeof(struct telnet_rfc1143_s) * (telnet->q_size + 4));
  if (qtmp == 0)
    {
      _error(telnet, __LINE__, __func__, TELNET_ENOMEM, 0,
             "realloc() failed: %d", errno);
      return;
    }

  memset(&qtmp[telnet->q_size], 0, sizeof(struct telnet_rfc1143_s) * 4);
  telnet->q                        = qtmp;
  telnet->q[telnet->q_size].telopt = telopt;
  telnet->q[telnet->q_size].state  = Q_MAKE(us, him);
  telnet->q_size                  += 4;
}

/* Send negotiation bytes */

static inline void _send_negotiate(FAR struct telnet_s *telnet,
                                   unsigned char cmd, unsigned char telopt)
{
  unsigned char bytes[3];
  bytes[0] = TELNET_IAC;
  bytes[1] = cmd;
  bytes[2] = telopt;
  _sendu(telnet, bytes, 3);
}

/* Negotiation handling magic for RFC1143 */

static void _negotiate(FAR struct telnet_s *telnet, unsigned char telopt)
{
  union telnet_event_u ev;
  struct telnet_rfc1143_s q;

  /* In PROXY mode, just pass it through and do nothing */

  if (telnet->flags & TELNET_FLAG_PROXY)
    {
      switch ((int)telnet->state)
        {
        case TELNET_STATE_WILL:
          NEGOTIATE_EVENT(telnet, TELNET_EV_WILL, telopt);
          break;

        case TELNET_STATE_WONT:
          NEGOTIATE_EVENT(telnet, TELNET_EV_WONT, telopt);
          break;

        case TELNET_STATE_DO:
          NEGOTIATE_EVENT(telnet, TELNET_EV_DO, telopt);
          break;

        case TELNET_STATE_DONT:
          NEGOTIATE_EVENT(telnet, TELNET_EV_DONT, telopt);
          break;
        }

      return;
    }

  /* Lookup the current state of the option */

  q = _get_rfc1143(telnet, telopt);

  /* Start processing... */

  switch ((int)telnet->state)
    {
    /* Request to enable option on remote end or confirm DO */

    case TELNET_STATE_WILL:
      switch (Q_HIM(q))
        {
        case Q_NO:
          if (_check_telopt(telnet, telopt, 0))
            {
              _set_rfc1143(telnet, telopt, Q_US(q), Q_YES);
              _send_negotiate(telnet, TELNET_DO, telopt);
              NEGOTIATE_EVENT(telnet, TELNET_EV_WILL, telopt);
            }
          else
            _send_negotiate(telnet, TELNET_DONT, telopt);
          break;

        case Q_WANTNO:
          _set_rfc1143(telnet, telopt, Q_US(q), Q_NO);
          NEGOTIATE_EVENT(telnet, TELNET_EV_WONT, telopt);
          _error(telnet, __LINE__, __func__, TELNET_EPROTOCOL, 0,
                 "DONT answered by WILL");
          break;

        case Q_WANTNO_OP:
          _set_rfc1143(telnet, telopt, Q_US(q), Q_YES);
          _error(telnet, __LINE__, __func__, TELNET_EPROTOCOL, 0,
                 "DONT answered by WILL");
          break;

        case Q_WANTYES:
          _set_rfc1143(telnet, telopt, Q_US(q), Q_YES);
          NEGOTIATE_EVENT(telnet, TELNET_EV_WILL, telopt);
          break;

        case Q_WANTYES_OP:
          _set_rfc1143(telnet, telopt, Q_US(q), Q_WANTNO);
          _send_negotiate(telnet, TELNET_DONT, telopt);
          NEGOTIATE_EVENT(telnet, TELNET_EV_WILL, telopt);
          break;
        }
      break;

    /* Request to disable option on remote end, confirm DONT, reject DO */

    case TELNET_STATE_WONT:
      switch (Q_HIM(q))
        {
        case Q_YES:
          _set_rfc1143(telnet, telopt, Q_US(q), Q_NO);
          _send_negotiate(telnet, TELNET_DONT, telopt);
          NEGOTIATE_EVENT(telnet, TELNET_EV_WONT, telopt);
          break;

        case Q_WANTNO:
          _set_rfc1143(telnet, telopt, Q_US(q), Q_NO);
          NEGOTIATE_EVENT(telnet, TELNET_EV_WONT, telopt);
          break;

        case Q_WANTNO_OP:
          _set_rfc1143(telnet, telopt, Q_US(q), Q_WANTYES);
          _send_negotiate(telnet, TELNET_DO, telopt);
          NEGOTIATE_EVENT(telnet, TELNET_EV_WONT, telopt);
          break;

        case Q_WANTYES:
        case Q_WANTYES_OP:
          _set_rfc1143(telnet, telopt, Q_US(q), Q_NO);
          break;
        }
      break;

    /* Request to enable option on local end or confirm WILL */

    case TELNET_STATE_DO:
      switch (Q_US(q))
        {
        case Q_NO:
          if (_check_telopt(telnet, telopt, 1))
            {
              _set_rfc1143(telnet, telopt, Q_YES, Q_HIM(q));
              _send_negotiate(telnet, TELNET_WILL, telopt);
              NEGOTIATE_EVENT(telnet, TELNET_EV_DO, telopt);
            }
          else
            _send_negotiate(telnet, TELNET_WONT, telopt);
          break;

        case Q_WANTNO:
          _set_rfc1143(telnet, telopt, Q_NO, Q_HIM(q));
          NEGOTIATE_EVENT(telnet, TELNET_EV_DONT, telopt);
          _error(telnet, __LINE__, __func__, TELNET_EPROTOCOL, 0,
                 "WONT answered by DO");
          break;

        case Q_WANTNO_OP:
          _set_rfc1143(telnet, telopt, Q_YES, Q_HIM(q));
          _error(telnet, __LINE__, __func__, TELNET_EPROTOCOL, 0,
                 "WONT answered by DO");
          break;

        case Q_WANTYES:
          _set_rfc1143(telnet, telopt, Q_YES, Q_HIM(q));
          NEGOTIATE_EVENT(telnet, TELNET_EV_DO, telopt);
          break;

        case Q_WANTYES_OP:
          _set_rfc1143(telnet, telopt, Q_WANTNO, Q_HIM(q));
          _send_negotiate(telnet, TELNET_WONT, telopt);
          NEGOTIATE_EVENT(telnet, TELNET_EV_DO, telopt);
          break;
        }
      break;

    /* Request to disable option on local end, confirm WONT, reject WILL */

    case TELNET_STATE_DONT:
      switch (Q_US(q))
        {
        case Q_YES:
          _set_rfc1143(telnet, telopt, Q_NO, Q_HIM(q));
          _send_negotiate(telnet, TELNET_WONT, telopt);
          NEGOTIATE_EVENT(telnet, TELNET_EV_DONT, telopt);
          break;

        case Q_WANTNO:
          _set_rfc1143(telnet, telopt, Q_NO, Q_HIM(q));
          NEGOTIATE_EVENT(telnet, TELNET_EV_DONT, telopt);
          break;

        case Q_WANTNO_OP:
          _set_rfc1143(telnet, telopt, Q_WANTYES, Q_HIM(q));
          _send_negotiate(telnet, TELNET_WILL, telopt);
          NEGOTIATE_EVENT(telnet, TELNET_EV_DONT, telopt);
          break;

        case Q_WANTYES:
        case Q_WANTYES_OP:
          _set_rfc1143(telnet, telopt, Q_NO, Q_HIM(q));
          break;
        }
      break;
    }
}

/* Process an ENVIRON/NEW-ENVIRON subnegotiation buffer
 *
 * the algorithm and approach used here is kind of a hack,
 * but it reduces the number of memory allocations we have
 * to make.
 *
 * we copy the bytes back into the buffer, starting at the very
 * beginning, which makes it easy to handle the ENVIRON ESC
 * escape mechanism as well as ensure the variable name and
 * value strings are NUL-terminated, all while fitting inside
 * of the original buffer.
 */

static int _environ_telnet(FAR struct telnet_s *telnet, unsigned char type,
                           FAR char *buffer, size_t size)
{
  union telnet_event_u ev;
  struct telnet_environ_s *values = 0;
  FAR char *c;
  FAR char *last;
  FAR char *out;
  size_t index;
  size_t count;

  /* If we have no data, just pass it through */

  if (size == 0)
    {
      return 0;
    }

  /* First byte must be a valid command */

  if ((unsigned)buffer[0] != TELNET_ENVIRON_SEND &&
      (unsigned)buffer[0] != TELNET_ENVIRON_IS &&
      (unsigned)buffer[0] != TELNET_ENVIRON_INFO)
    {
      _error(telnet, __LINE__, __func__, TELNET_EPROTOCOL, 0,
             "telopt %d subneg has invalid command", type);
      return 0;
    }

  /* Store ENVIRON command */

  ev.envevent.cmd = buffer[0];

  /* If we have no arguments, send an event with no data end return */

  if (size == 1)
    {
      /* No list of variables given */

      ev.envevent.values = 0;
      ev.envevent.size   = 0;

      /* Invoke event with our arguments */

      ev.type = TELNET_EV_ENVIRON;
      telnet->eh(telnet, &ev, telnet->ud);
      return 0;
    }

  /* Every second byte must be VAR or USERVAR, if present */

  if ((unsigned)buffer[1] != TELNET_ENVIRON_VAR &&
      (unsigned)buffer[1] != TELNET_ENVIRON_USERVAR)
    {
      _error(telnet, __LINE__, __func__, TELNET_EPROTOCOL, 0,
             "telopt %d subneg missing variable type", type);
      return 0;
    }

  /* Ensure last byte is not an escape byte (makes parsing later easier) */

  if ((unsigned)buffer[size - 1] == TELNET_ENVIRON_ESC)
    {
      _error(telnet, __LINE__, __func__, TELNET_EPROTOCOL, 0,
             "telopt %d subneg ends with ESC", type);
      return 0;
    }

  /* Count arguments; each valid entry starts with VAR or USERVAR */

  count = 0;
  for (c = buffer + 1; c < buffer + size; ++c)
    {
      if (*c == TELNET_ENVIRON_VAR || *c == TELNET_ENVIRON_USERVAR)
        {
          ++count;
        }
      else if (*c == TELNET_ENVIRON_ESC)
        {
          /* Skip the next byte */

          ++c;
        }
    }

  /* Allocate argument array, bail on error */

  values = (struct telnet_environ_s *)
    calloc(count, sizeof(struct telnet_environ_s));
  if (values == 0)
    {
      _error(telnet, __LINE__, __func__, TELNET_ENOMEM, 0,
             "calloc() failed: %d", errno);
      return 0;
    }

  /* Parse argument array strings */

  out = buffer;
  c = buffer + 1;
  for (index = 0; index != count; ++index)
    {
      /* Remember the variable type (will be VAR or USERVAR) */

      values[index].type = *c++;

      /* Scan until we find an end-marker, and buffer up unescaped bytes into
       * our buffer.
       */

      last = out;
      while (c < buffer + size)
        {
          /* Stop at the next variable or at the value */

          if ((unsigned)*c == TELNET_ENVIRON_VAR ||
              (unsigned)*c == TELNET_ENVIRON_VALUE ||
              (unsigned)*c == TELNET_ENVIRON_USERVAR)
            {
              break;
            }

          /* Buffer next byte (taking into account ESC) */

          if (*c == TELNET_ENVIRON_ESC)
            {
              ++c;
            }

          *out++ = *c++;
        }

      *out++ = '\0';

      /* Store the variable name we have just received */

      values[index].var   = last;
      values[index].value = "";

      /* If we got a value, find the next end marker and store the value;
       * otherwise, store empty string.
       */

      if (c < buffer + size && *c == TELNET_ENVIRON_VALUE)
        {
          ++c;
          last = out;
          while (c < buffer + size)
            {
              /* Stop when we find the start of the next variable */

              if ((unsigned)*c == TELNET_ENVIRON_VAR ||
                  (unsigned)*c == TELNET_ENVIRON_USERVAR)
                {
                  break;
                }

              /* Buffer next byte (taking into account ESC) */

              if (*c == TELNET_ENVIRON_ESC)
                {
                  ++c;
                }

              *out++ = *c++;
            }

          *out++ = '\0';

          /* Store the variable value */

          values[index].value = last;
        }
    }

  /* Pass values array and count to event */

  ev.envevent.values = values;
  ev.envevent.size   = count;

  /* Invoke event with our arguments */

  ev.type = TELNET_EV_ENVIRON;
  telnet->eh(telnet, &ev, telnet->ud);

  /* Clean up */

  free(values);
  return 0;
}

/* Process an MSSP subnegotiation buffer */

static int _mssp_telnet(FAR struct telnet_s *telnet, FAR char *buffer,
                        size_t size)
{
  union telnet_event_u ev;
  FAR struct telnet_environ_s *values;
  FAR char *var = 0;
  FAR char *c;
  FAR char *last;
  FAR char *out;
  size_t count;
  size_t i;
  unsigned char next_type;

  /* If we have no data, just pass it through */

  if (size == 0)
    {
      return 0;
    }

  /* First byte must be a VAR */

  if ((unsigned)buffer[0] != TELNET_MSSP_VAR)
    {
      _error(telnet, __LINE__, __func__, TELNET_EPROTOCOL, 0,
             "MSSP subnegotiation has invalid data");
      return 0;
    }

  /* Count the arguments, any part that starts with VALUE */

  for (count = 0, i = 0; i != size; ++i)
    {
      if ((unsigned)buffer[i] == TELNET_MSSP_VAL)
        {
          ++count;
        }
    }

  /* Allocate argument array, bail on error */

  values = (struct telnet_environ_s *)
    calloc(count, sizeof(struct telnet_environ_s));
  if (values == 0)
    {
      _error(telnet, __LINE__, __func__, TELNET_ENOMEM, 0,
             "calloc() failed: %d", errno);
      return 0;
    }

  ev.mssp.values = values;
  ev.mssp.size   = count;

  /* Allocate strings in argument array */

  out = last = buffer;
  next_type = buffer[0];
  for (i = 0, c = buffer + 1; c < buffer + size; )
    {
      /* Search for end marker */

      while (c < buffer + size && (unsigned)*c != TELNET_MSSP_VAR &&
             (unsigned)*c != TELNET_MSSP_VAL)
        {
          *out++ = *c++;
        }

      *out++ = '\0';

      /* If it's a variable name, just store the name for now */

      if (next_type == TELNET_MSSP_VAR)
        {
          var = last;
        }
      else if (next_type == TELNET_MSSP_VAL && var != 0)
        {
          values[i].var   = var;
          values[i].value = last;
          ++i;
        }
      else
        {
          _error(telnet, __LINE__, __func__, TELNET_EPROTOCOL, 0,
                 "invalid MSSP subnegotiation data");
          free(values);
          return 0;
        }

      /* Remember our next type and increment c for next loop run */

      last      = out;
      next_type = *c++;
    }

  /* Invoke event with our arguments */

  ev.type = TELNET_EV_MSSP;
  telnet->eh(telnet, &ev, telnet->ud);

  /* Clean up */

  free(values);
  return 0;
}

/* Parse ZMP command subnegotiation buffers */

static int _zmp_telnet(FAR struct telnet_s *telnet, FAR const char *buffer,
                       size_t size)
{
  union telnet_event_u ev;
  FAR char **argv;
  FAR const char *c;
  size_t i;
  size_t argc;

  /* Make sure this is a valid ZMP buffer */

  if (size == 0 || buffer[size - 1] != 0)
    {
      _error(telnet, __LINE__, __func__, TELNET_EPROTOCOL, 0,
             "incomplete ZMP frame");
      return 0;
    }

  /* Count arguments */

  for (argc = 0, c = buffer; c != buffer + size; ++argc)
    {
      c += strlen(c) + 1;
    }

  /* Allocate argument array, bail on error */

  if ((argv = (char **)calloc(argc, sizeof(char *))) == 0)
    {
      _error(telnet, __LINE__, __func__, TELNET_ENOMEM, 0,
             "calloc() failed: %d", errno);
      return 0;
    }

  /* Populate argument array */

  for (i = 0, c = buffer; i != argc; ++i)
    {
      argv[i] = (char *)c;
      c      += strlen(c) + 1;
    }

  /* Invoke event with our arguments */

  ev.type     = TELNET_EV_ZMP;
  ev.zmp.argv = (const char **)argv;
  ev.zmp.argc = argc;
  telnet->eh(telnet, &ev, telnet->ud);

  /* Clean up */

  free(argv);
  return 0;
}

/* Parse TERMINAL-TYPE command subnegotiation buffers */

static int _ttype_telnet(FAR struct telnet_s *telnet, FAR const char *buffer,
                         size_t size)
{
  union telnet_event_u ev;

  /* Make sure request is not empty */

  if (size == 0)
    {
      _error(telnet, __LINE__, __func__, TELNET_EPROTOCOL, 0,
             "incomplete TERMINAL-TYPE request");
      return 0;
    }

  /* Make sure request has valid command type */

  if (buffer[0] != TELNET_TTYPE_IS && buffer[0] != TELNET_TTYPE_SEND)
    {
      _error(telnet, __LINE__, __func__, TELNET_EPROTOCOL, 0,
             "TERMINAL-TYPE request has invalid type");
      return 0;
    }

  /* Send proper event */

  if (buffer[0] == TELNET_TTYPE_IS)
    {
      char *name;

      /* Allocate space for name */

      if ((name = (char *)malloc(size)) == 0)
        {
          _error(telnet, __LINE__, __func__, TELNET_ENOMEM, 0,
                 "malloc() failed: %d", errno);
          return 0;
        }

      memcpy(name, buffer + 1, size - 1);
      name[size - 1] = '\0';

      ev.type       = TELNET_EV_TTYPE;
      ev.ttype.cmd  = TELNET_TTYPE_IS;
      ev.ttype.name = name;
      telnet->eh(telnet, &ev, telnet->ud);

      /* Clean up */

      free(name);
    }
  else
    {
      ev.type       = TELNET_EV_TTYPE;
      ev.ttype.cmd  = TELNET_TTYPE_SEND;
      ev.ttype.name = 0;
      telnet->eh(telnet, &ev, telnet->ud);
    }

  return 0;
}

/* Process a subnegotiation buffer; return non-zero if the current buffer
 * must be aborted and reprocessed due to COMPRESS2 being activated
 */

static int _subnegotiate(FAR struct telnet_s *telnet)
{
  union telnet_event_u ev;

  /* Standard subnegotiation event */

  ev.type = TELNET_EV_SUBNEGOTIATION;
  ev.sub.telopt = telnet->sb_telopt;
  ev.sub.buffer = telnet->buffer;
  ev.sub.size = telnet->buffer_pos;
  telnet->eh(telnet, &ev, telnet->ud);

  switch (telnet->sb_telopt)
    {
#if defined(HAVE_ZLIB)
    /* Received COMPRESS2 begin marker, setup our zlib box and start handling
     * the compressed stream if it's not already.
     */

    case TELNET_TELOPT_COMPRESS2:
      if (_init_zlib(telnet, 0, 1) != TELNET_EOK)
        {
          return 0;
        }

      /* Notify app that compression was enabled */

      ev.type           = TELNET_EV_COMPRESS;
      ev.compress.state = 1;
      telnet->eh(telnet, &ev, telnet->ud);
      return 1;
#endif /* HAVE_ZLIB */

    /* Specially handled subnegotiation telopt types */

    case TELNET_TELOPT_ZMP:
      return _zmp_telnet(telnet, telnet->buffer, telnet->buffer_pos);

    case TELNET_TELOPT_TTYPE:
      return _ttype_telnet(telnet, telnet->buffer, telnet->buffer_pos);

    case TELNET_TELOPT_ENVIRON:
    case TELNET_TELOPT_NEW_ENVIRON:
      return _environ_telnet(telnet, telnet->sb_telopt, telnet->buffer,
                             telnet->buffer_pos);

    case TELNET_TELOPT_MSSP:
      return _mssp_telnet(telnet, telnet->buffer, telnet->buffer_pos);

    default:
      return 0;
    }
}

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

FAR struct telnet_s *telnet_init(FAR const struct telnet_telopt_s *telopts,
                                 telnet_event_handler_t eh,
                                 unsigned char flags,
                                 FAR void *user_data)
{
  /* Allocate structure */

  FAR struct telnet_s *telnet = (FAR struct telnet_s *)
    calloc(1, sizeof(struct telnet_s));
  if (telnet == 0)
    {
      return 0;
    }

  /* Initialize data */

  telnet->ud      = user_data;
  telnet->telopts = telopts;
  telnet->eh      = eh;
  telnet->flags   = flags;

  return telnet;
}

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

void telnet_free(FAR struct telnet_s *telnet)
{
  /* Free sub-request buffer */

  if (telnet->buffer != 0)
    {
      free(telnet->buffer);
      telnet->buffer = 0;
      telnet->buffer_size = 0;
      telnet->buffer_pos = 0;
    }

#if defined(HAVE_ZLIB)
  /* Free zlib box */

  if (telnet->z != 0)
    {
      if (telnet->flags & TELNET_PFLAG_DEFLATE)
        {
          deflateEnd(telnet->z);
        }
      else
        {
          inflateEnd(telnet->z);
        }

      free(telnet->z);
      telnet->z = 0;
    }
#endif /* HAVE_ZLIB */

  /* Free RFC1143 queue */

  if (telnet->q)
    {
      free(telnet->q);
      telnet->q = 0;
      telnet->q_size = 0;
    }

  /* Free the telnet structure itself */

  free(telnet);
}

/* Push a byte into the telnet buffer */

static enum telnet_error_e _buffer_byte(FAR struct telnet_s *telnet,
                                        unsigned char byte)
{
  char *new_buffer;
  size_t i;

  /* Check if we're out of room */

  if (telnet->buffer_pos == telnet->buffer_size)
    {
      /* Find the next buffer size */

      for (i = 0; i != _buffer_sizes_count; ++i)
        {
          if (_buffer_sizes[i] == telnet->buffer_size)
            {
              break;
            }
        }

      /* Overflow -- can't grow any more */

      if (i >= _buffer_sizes_count - 1)
        {
          _error(telnet, __LINE__, __func__, TELNET_EOVERFLOW, 0,
                 "subnegotiation buffer size limit reached");
          return TELNET_EOVERFLOW;
        }

      /* (Re)allocate buffer */

      new_buffer = (char *)realloc(telnet->buffer, _buffer_sizes[i + 1]);
      if (new_buffer == 0)
        {
          _error(telnet, __LINE__, __func__, TELNET_ENOMEM, 0,
                 "realloc() failed");
          return TELNET_ENOMEM;
        }

      telnet->buffer = new_buffer;
      telnet->buffer_size = _buffer_sizes[i + 1];
    }

  /* Push the byte, all set */

  telnet->buffer[telnet->buffer_pos++] = byte;
  return TELNET_EOK;
}

static void _process(FAR struct telnet_s *telnet, FAR const char *buffer,
                     size_t size)
{
  union telnet_event_u ev;
  unsigned char byte;
  size_t start;
  size_t i;

  for (i = start = 0; i != size; ++i)
    {
      byte = buffer[i];
      switch (telnet->state)
        {
        /* Regular data */

        case TELNET_STATE_DATA:

          /* On an IAC byte, pass through all pending bytes and switch states */

          if (byte == TELNET_IAC)
            {
              if (i != start)
                {
                  ev.type        = TELNET_EV_DATA;
                  ev.data.buffer = buffer + start;
                  ev.data.size   = i - start;
                  telnet->eh(telnet, &ev, telnet->ud);
                }

              telnet->state = TELNET_STATE_IAC;
            }
          break;

        /* IAC command */

        case TELNET_STATE_IAC:
          switch (byte)
            {
            /* Subnegotiation */

            case TELNET_SB:
              telnet->state = TELNET_STATE_SB;
              break;

            /* Negotiation commands */

            case TELNET_WILL:
              telnet->state = TELNET_STATE_WILL;
              break;

            case TELNET_WONT:
              telnet->state = TELNET_STATE_WONT;
              break;

            case TELNET_DO:
              telnet->state = TELNET_STATE_DO;
              break;

            case TELNET_DONT:
              telnet->state = TELNET_STATE_DONT;
              break;

            /* IAC escaping */

            case TELNET_IAC:

              /* Event */

              ev.type        = TELNET_EV_DATA;
              ev.data.buffer = (char *)&byte;
              ev.data.size   = 1;
              telnet->eh(telnet, &ev, telnet->ud);

              /* State update */

              start = i + 1;
              telnet->state = TELNET_STATE_DATA;
              break;

            /* Some other command */

            default:

              /* Event */

              ev.type    = TELNET_EV_IAC;
              ev.iac.cmd = byte;
              telnet->eh(telnet, &ev, telnet->ud);

              /* State update */

              start = i + 1;
              telnet->state = TELNET_STATE_DATA;
            }
          break;

        /* Negotiation commands */

        case TELNET_STATE_WILL:
        case TELNET_STATE_WONT:
        case TELNET_STATE_DO:
        case TELNET_STATE_DONT:
          _negotiate(telnet, byte);
          start = i + 1;
          telnet->state = TELNET_STATE_DATA;
          break;

        /* Subnegotiation -- determine subnegotiation telopt */

        case TELNET_STATE_SB:
          telnet->sb_telopt = byte;
          telnet->buffer_pos = 0;
          telnet->state = TELNET_STATE_SB_DATA;
          break;

        /* Subnegotiation -- buffer bytes until end request */

        case TELNET_STATE_SB_DATA:

          /* IAC command in subnegotiation -- either IAC SE or IAC IAC */

          if (byte == TELNET_IAC)
            {
              telnet->state = TELNET_STATE_SB_DATA_IAC;
            }
          else if (telnet->sb_telopt == TELNET_TELOPT_COMPRESS &&
                   byte == TELNET_WILL)
            {
              /* In 1998 MCCP used TELOPT 85 and the protocol defined an
               * invalid subnegotiation sequence (IAC SB 85 WILL SE) to start
               * compression. Subsequently MCCP version 2 was created in 2000
               * using TELOPT 86 and a valid subnegotiation (IAC SB 86 IAC SE).
               * libtelnet for now just captures and discards MCCPv1 sequences.
               */

              start = i + 2;
              telnet->state = TELNET_STATE_DATA;

              /* Buffer the byte, or bail if we can't */
            }
          else if (_buffer_byte(telnet, byte) != TELNET_EOK)
            {
              start = i + 1;
              telnet->state = TELNET_STATE_DATA;
            }
          break;

        /* IAC escaping inside a subnegotiation */

        case TELNET_STATE_SB_DATA_IAC:
          switch (byte)
            {
            /* End subnegotiation */

            case TELNET_SE:

              /* Return to default state */

              start = i + 1;
              telnet->state = TELNET_STATE_DATA;

              /* Process subnegotiation */

              if (_subnegotiate(telnet) != 0)
                {
                  /* Any remaining bytes in the buffer are compressed. we have
                   * to re-invoke telnet_recv to get those bytes inflated and
                   * abort trying to process the remaining compressed bytes in
                   * the current _process buffer argument.
                   */

                  telnet_recv(telnet, &buffer[start], size - start);
                  return;
                }
              break;

            /* Escaped IAC byte */

            case TELNET_IAC:

              /* Push IAC into buffer */

              if (_buffer_byte(telnet, TELNET_IAC) != TELNET_EOK)
                {
                  start = i + 1;
                  telnet->state = TELNET_STATE_DATA;
                }
              else
                {
                  telnet->state = TELNET_STATE_SB_DATA;
                }
              break;

              /* Something else -- protocol error.  attempt to process content
               * in subnegotiation buffer, then evaluate the given command as
               * an IAC code.
               */

            default:
              _error(telnet, __LINE__, __func__, TELNET_EPROTOCOL, 0,
                     "unexpected byte after IAC inside SB: %d", byte);

              /* Enter IAC state */

              start = i + 1;
              telnet->state = TELNET_STATE_IAC;

              /* Process subnegotiation; see comment in
               * TELNET_STATE_SB_DATA_IAC about invoking telnet_recv()
               */

              if (_subnegotiate(telnet) != 0)
                {
                  telnet_recv(telnet, &buffer[start], size - start);
                  return;
                }
              else
                {
                  /* Recursive call to get the current input byte processed as
                   * a regular IAC command.  we could use a goto, but that
                   * would be gross.
                   */

                  _process(telnet, (char *)&byte, 1);
                }
              break;
            }
          break;
        }
    }

  /* Pass through any remaining bytes */

  if (telnet->state == TELNET_STATE_DATA && i != start)
    {
      ev.type = TELNET_EV_DATA;
      ev.data.buffer = buffer + start;
      ev.data.size = i - start;
      telnet->eh(telnet, &ev, telnet->ud);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

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

void telnet_recv(FAR struct telnet_s *telnet, FAR const char *buffer,
                 size_t size)
{
#if defined(HAVE_ZLIB)
  /* If we have an inflate (decompression) zlib stream, use it */

  if (telnet->z != 0 && !(telnet->flags & TELNET_PFLAG_DEFLATE))
    {
      char inflate_buffer[1024];
      int rs;

      /* Initialize zlib state */

      telnet->z->next_in = (unsigned char *)buffer;
      telnet->z->avail_in = (unsigned int)size;
      telnet->z->next_out = (unsigned char *)inflate_buffer;
      telnet->z->avail_out = sizeof(inflate_buffer);

      /* Inflate until buffer exhausted and all output is produced */

      while (telnet->z->avail_in > 0 || telnet->z->avail_out == 0)
        {
          /* Reset output buffer */

          /* Decompress */

          rs = inflate(telnet->z, Z_SYNC_FLUSH);

          /* Process the decompressed bytes on success */

          if (rs == Z_OK || rs == Z_STREAM_END)
            {
              _process(telnet, inflate_buffer, sizeof(inflate_buffer) -
                       telnet->z->avail_out);
            }
          else
            {
              _error(telnet, __LINE__, __func__, TELNET_ECOMPRESS, 1,
                     "inflate() failed: %s", zError(rs));
            }

          /* Prepare output buffer for next run */

          telnet->z->next_out = (unsigned char *)inflate_buffer;
          telnet->z->avail_out = sizeof(inflate_buffer);

          /* On error (or on end of stream) disable further inflation */

          if (rs != Z_OK)
            {
              union telnet_event_u ev;

              /* Disable compression */

              inflateEnd(telnet->z);
              free(telnet->z);
              telnet->z = 0;

              /* Send event */

              ev.type           = TELNET_EV_COMPRESS;
              ev.compress.state = 0;
              telnet->eh(telnet, &ev, telnet->ud);
              break;
            }
        }

      /* COMPRESS2 is not negotiated, just process */
    }
  else
#endif /* HAVE_ZLIB */
    _process(telnet, buffer, size);
}

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

void telnet_iac(FAR struct telnet_s *telnet, unsigned char cmd)
{
  unsigned char bytes[2];
  bytes[0] = TELNET_IAC;
  bytes[1] = cmd;
  _sendu(telnet, bytes, 2);
}

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

void telnet_negotiate(FAR struct telnet_s *telnet, unsigned char cmd,
                      unsigned char telopt)
{
  struct telnet_rfc1143_s q;

  /* If we're in proxy mode, just send it now */

  if (telnet->flags & TELNET_FLAG_PROXY)
    {
      unsigned char bytes[3];
      bytes[0] = TELNET_IAC;
      bytes[1] = cmd;
      bytes[2] = telopt;
      _sendu(telnet, bytes, 3);
      return;
    }

  /* Get current option states */

  q = _get_rfc1143(telnet, telopt);

  switch (cmd)
    {
    /* Advertise willingess to support an option */

    case TELNET_WILL:
      switch (Q_US(q))
        {
        case Q_NO:
          _set_rfc1143(telnet, telopt, Q_WANTYES, Q_HIM(q));
          _send_negotiate(telnet, TELNET_WILL, telopt);
          break;

        case Q_WANTNO:
          _set_rfc1143(telnet, telopt, Q_WANTNO_OP, Q_HIM(q));
          break;

        case Q_WANTYES_OP:
          _set_rfc1143(telnet, telopt, Q_WANTYES, Q_HIM(q));
          break;
        }
      break;

    /* Force turn-off of locally enabled option */

    case TELNET_WONT:
      switch (Q_US(q))
        {
        case Q_YES:
          _set_rfc1143(telnet, telopt, Q_WANTNO, Q_HIM(q));
          _send_negotiate(telnet, TELNET_WONT, telopt);
          break;

        case Q_WANTYES:
          _set_rfc1143(telnet, telopt, Q_WANTYES_OP, Q_HIM(q));
          break;

        case Q_WANTNO_OP:
          _set_rfc1143(telnet, telopt, Q_WANTNO, Q_HIM(q));
          break;
        }
      break;

    /* Ask remote end to enable an option */

    case TELNET_DO:
      switch (Q_HIM(q))
        {
        case Q_NO:
          _set_rfc1143(telnet, telopt, Q_US(q), Q_WANTYES);
          _send_negotiate(telnet, TELNET_DO, telopt);
          break;

        case Q_WANTNO:
          _set_rfc1143(telnet, telopt, Q_US(q), Q_WANTNO_OP);
          break;

        case Q_WANTYES_OP:
          _set_rfc1143(telnet, telopt, Q_US(q), Q_WANTYES);
          break;
        }
      break;

    /* Demand remote end disable an option */

    case TELNET_DONT:
      switch (Q_HIM(q))
        {
        case Q_YES:
          _set_rfc1143(telnet, telopt, Q_US(q), Q_WANTNO);
          _send_negotiate(telnet, TELNET_DONT, telopt);
          break;

        case Q_WANTYES:
          _set_rfc1143(telnet, telopt, Q_US(q), Q_WANTYES_OP);
          break;

        case Q_WANTNO_OP:
          _set_rfc1143(telnet, telopt, Q_US(q), Q_WANTNO);
          break;
        }
      break;
    }
}

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

void telnet_send(FAR struct telnet_s *telnet, FAR const char *buffer,
                 size_t size)
{
  size_t l;
  size_t i;

  for (l = i = 0; i != size; ++i)
    {
      /* Dump prior portion of text, send escaped bytes */

      if (buffer[i] == (char)TELNET_IAC)
        {
          /* Dump prior text if any */

          if (i != l)
            {
              _send(telnet, buffer + l, i - l);
            }

          l = i + 1;

          /* Send escape */

          telnet_iac(telnet, TELNET_IAC);
        }
    }

  /* Send whatever portion of buffer is left */

  if (i != l)
    {
      _send(telnet, buffer + l, i - l);
    }
}

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

void telnet_begin_sb(FAR struct telnet_s *telnet, unsigned char telopt)
{
  unsigned char sb[3];
  sb[0] = TELNET_IAC;
  sb[1] = TELNET_SB;
  sb[2] = telopt;
  _sendu(telnet, sb, 3);
}

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

void telnet_subnegotiation(FAR struct telnet_s *telnet, unsigned char telopt,
                           FAR const char *buffer, size_t size)
{
  unsigned char bytes[5];
  bytes[0] = TELNET_IAC;
  bytes[1] = TELNET_SB;
  bytes[2] = telopt;
  bytes[3] = TELNET_IAC;
  bytes[4] = TELNET_SE;

  _sendu(telnet, bytes, 3);
  telnet_send(telnet, buffer, size);
  _sendu(telnet, bytes + 3, 2);

#if defined(HAVE_ZLIB)
  /* If we're a proxy and we just sent the COMPRESS2 marker, we must make sure
   * all further data is compressed if not already.
   */

  if (telnet->flags & TELNET_FLAG_PROXY && telopt == TELNET_TELOPT_COMPRESS2)
    {
      union telnet_event_u ev;

      if (_init_zlib(telnet, 1, 1) != TELNET_EOK)
        {
          return;
        }

      /* Notify app that compression was enabled */

      ev.type           = TELNET_EV_COMPRESS;
      ev.compress.state = 1;
      telnet->eh(telnet, &ev, telnet->ud);
    }
#endif /* HAVE_ZLIB */
}

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

void telnet_begin_compress2(FAR struct telnet_s *telnet)
{
#if defined(HAVE_ZLIB)
  static const unsigned char compress2[] =
  {
    TELNET_IAC, TELNET_SB, TELNET_TELOPT_COMPRESS2, TELNET_IAC, TELNET_SE
  };

  union telnet_event_u ev;

  /* Attempt to create output stream first, bail if we can't */

  if (_init_zlib(telnet, 1, 0) != TELNET_EOK)
    {
      return;
    }

  /* Send compression marker.  we send directly to the event handler instead of
   * passing through _send because _send would result in the compress marker
   * itself being compressed.
   */

  ev.type        = TELNET_EV_SEND;
  ev.data.buffer = (const char *)compress2;
  ev.data.size   = sizeof(compress2);
  telnet->eh(telnet, &ev, telnet->ud);

  /* Notify app that compression was successfully enabled */

  ev.type           = TELNET_EV_COMPRESS;
  ev.compress.state = 1;
  telnet->eh(telnet, &ev, telnet->ud);
#endif /* HAVE_ZLIB */
}

/****************************************************************************
 * Name: telnet_vprintf
 *
 * Description:
 *   Send formatted data with \r and \n translation in addition to IAC IAC
 *
 *   See telnet_printf().
 *
 ****************************************************************************/

int telnet_vprintf(FAR struct telnet_s *telnet, FAR const char *fmt,
                   va_list va)
{
  static const char CRLF[] =
  {
    '\r', '\n'
  };

  static const char CRNUL[] =
  {
    '\r', '\0'
  };

  char buffer[1024];
  FAR char *output = buffer;
  int rs;
  int l;
  int i;

  /* Format */

  va_list va2;
  va_copy(va2, va);
  rs = vsnprintf(buffer, sizeof(buffer), fmt, va);
  if (rs >= sizeof(buffer))
    {
      output = (char *)malloc(rs + 1);
      if (output == 0)
        {
          _error(telnet, __LINE__, __func__, TELNET_ENOMEM, 0,
                 "malloc() failed: %d", errno);
          return -1;
        }

      rs = vsnprintf(output, rs + 1, fmt, va2);
    }

  va_end(va2);
  va_end(va);

  /* Send */

  for (l = i = 0; i != rs; ++i)
    {
      /* Special characters */

      if (output[i] == (char)TELNET_IAC || output[i] == '\r' ||
          output[i] == '\n')
        {
          /* Dump prior portion of text */

          if (i != l)
            {
              _send(telnet, output + l, i - l);
            }

          l = i + 1;

          /* IAC -> IAC IAC */

          if (output[i] == (char)TELNET_IAC)
            {
              telnet_iac(telnet, TELNET_IAC);
            }

          /* Automatic translation of \r -> CRNUL */

          else if (output[i] == '\r')
            {
              _send(telnet, CRNUL, 2);
            }

          /* Automatic translation of \n -> CRLF */

          else if (output[i] == '\n')
            {
              _send(telnet, CRLF, 2);
            }
        }
    }

  /* Send whatever portion of output is left */

  if (i != l)
    {
      _send(telnet, output + l, i - l);
    }

  /* Free allocated memory, if any */

  if (output != buffer)
    {
      free(output);
    }

  return rs;
}

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

int telnet_printf(FAR struct telnet_s *telnet, FAR const char *fmt, ...)
{
  va_list va;
  int rs;

  va_start(va, fmt);
  rs = telnet_vprintf(telnet, fmt, va);
  va_end(va);

  return rs;
}

/****************************************************************************
 * Name: telnet_raw_vprintf
 *
 * Description:
 *   Send formatted data (no newline escaping).
 *
 *   See telnet_raw_printf().
 *
 ****************************************************************************/

int telnet_raw_vprintf(FAR struct telnet_s *telnet, FAR const char *fmt,
                       va_list va)
{
  char buffer[1024];
  FAR char *output = buffer;
  int rs;

  /* Format; allocate more space if necessary */

  va_list va2;
  va_copy(va2, va);
  rs = vsnprintf(buffer, sizeof(buffer), fmt, va);
  if (rs >= sizeof(buffer))
    {
      output = (char *)malloc(rs + 1);
      if (output == 0)
        {
          _error(telnet, __LINE__, __func__, TELNET_ENOMEM, 0,
                 "malloc() failed: %d", errno);
          return -1;
        }

      rs = vsnprintf(output, rs + 1, fmt, va2);
    }

  va_end(va2);
  va_end(va);

  /* Send out the formatted data */

  telnet_send(telnet, output, rs);

  /* Release allocated memory, if any */

  if (output != buffer)
    {
      free(output);
    }

  return rs;
}

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

int telnet_raw_printf(FAR struct telnet_s *telnet, FAR const char *fmt, ...)
{
  va_list va;
  int rs;

  va_start(va, fmt);
  rs = telnet_raw_vprintf(telnet, fmt, va);
  va_end(va);

  return rs;
}

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

void telnet_begin_newenviron(FAR struct telnet_s *telnet, unsigned char cmd)
{
  telnet_begin_sb(telnet, TELNET_TELOPT_NEW_ENVIRON);
  telnet_send(telnet, (const char *)&cmd, 1);
}

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

void telnet_newenviron_value(FAR struct telnet_s *telnet, unsigned char type,
                             const char *string)
{
  telnet_send(telnet, (FAR const char *)&type, 1);

  if (string != 0)
    {
      telnet_send(telnet, string, strlen(string));
    }
}

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

void telnet_ttype_send(FAR struct telnet_s *telnet)
{
  static const unsigned char SEND[] =
  {
    TELNET_IAC, TELNET_SB, TELNET_TELOPT_TTYPE, TELNET_TTYPE_SEND,
    TELNET_IAC, TELNET_SE
  };

  _sendu(telnet, SEND, sizeof(SEND));
}

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

void telnet_ttype_is(FAR struct telnet_s *telnet, FAR const char *ttype)
{
  static const unsigned char IS[] =
  {
    TELNET_IAC, TELNET_SB, TELNET_TELOPT_TTYPE, TELNET_TTYPE_IS
  };

  if (!ttype)
    {
      ttype = "NVT";
    }

  _sendu(telnet, IS, sizeof(IS));
  _send(telnet, ttype, strlen(ttype));
  telnet_finish_sb(telnet);
}

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

void telnet_send_zmp(FAR struct telnet_s *telnet, size_t argc,
                     FAR const char **argv)
{
  size_t i;

  /* ZMP header */

  telnet_begin_zmp(telnet, argv[0]);

  /* Send out each argument, including trailing NUL byte */

  for (i = 1; i != argc; ++i)
    {
      telnet_zmp_arg(telnet, argv[i]);
    }

  /* ZMP footer */

  telnet_finish_zmp(telnet);
}

/****************************************************************************
 * Name: telnet_send_vzmpv
 *
 * Description:
 *   Send a ZMP command.
 *
 *   See telnet_send_zmpv().
 *
 ****************************************************************************/

void telnet_send_vzmpv(FAR struct telnet_s *telnet, va_list va)
{
  FAR const char *arg;

  /* ZMP header */

  telnet_begin_sb(telnet, TELNET_TELOPT_ZMP);

  /* Send out each argument, including trailing NUL byte */

  while ((arg = va_arg(va, const char *)) != 0)
    {
       telnet_zmp_arg(telnet, arg);
    }

  /* ZMP footer */

  telnet_finish_zmp(telnet);
}

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

void telnet_send_zmpv(FAR struct telnet_s *telnet, ...)
{
  va_list va;

  va_start(va, telnet);
  telnet_send_vzmpv(telnet, va);
  va_end(va);
}

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

void telnet_begin_zmp(FAR struct telnet_s *telnet, FAR const char *cmd)
{
  telnet_begin_sb(telnet, TELNET_TELOPT_ZMP);
  telnet_zmp_arg(telnet, cmd);
}

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

void telnet_zmp_arg(FAR struct telnet_s *telnet, FAR const char *arg)
{
  telnet_send(telnet, arg, strlen(arg) + 1);
}
