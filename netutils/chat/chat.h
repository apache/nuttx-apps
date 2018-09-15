/****************************************************************************
 * apps/netutils/chat/chat.h
 *
 *   Copyright (C) 2016 Vladimir Komendantskiy. All rights reserved.
 *   Author: Vladimir Komendantskiy <vladimir@moixaenergy.com>
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

#ifndef __APPS_NETUTILS_CHAT_CHAT_H
#define __APPS_NETUTILS_CHAT_CHAT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "netutils/chat.h"

/****************************************************************************
 * Public Types
 ****************************************************************************/

enum chat_line_type
{
  CHAT_LINE_TYPE_COMMAND = 0,
  CHAT_LINE_TYPE_EXPECT_SEND,
};

enum chat_command
{
  CHAT_COMMAND_ABORT = 0,
  CHAT_COMMAND_ECHO,
  CHAT_COMMAND_PAUSE,
  CHAT_COMMAND_SAY,
  CHAT_COMMAND_TIMEOUT,
};

/* Type of chat script: singly-linked list of chat lines. */

struct chat_line
{
  enum chat_line_type type;
  union
  {
    /* type-0 chat line command */

    enum chat_command command;

    /* type-1 chat line expected string */

    FAR char* expect;
  } lhs;

  /* type 0: command argument
   * type 1: string to be sent
   */

  FAR char* rhs;
  FAR struct chat_line* next; /* pointer to the next line in the script */
};

/* Chat private state. */

struct chat
{
  struct chat_ctl ctl;             /* Embedded 'chat_ctl' type. */
  FAR struct chat_line* script;    /* first line of the script */
};

#endif /* __APPS_NETUTILS_CHAT_CHAT_H */
