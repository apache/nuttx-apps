/****************************************************************************
 * apps/testing/nettest/local/test_local_scm_rights.c
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
 * Included Files
 ****************************************************************************/

#include <errno.h>
#include <fcntl.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <cmocka.h>

#include "test_local.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define SEND_FDS 3

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_local_scm_rights
 ****************************************************************************/

void test_local_scm_rights(FAR void **state)
{
  int sv[2];
  int p1[2];
  int p2[2];
  struct msghdr msg;
  struct iovec iov;
  char send_buf = 'X';
  char recv_buf;
  char cmsgbuf[CMSG_SPACE(sizeof(int) * SEND_FDS)];
  struct cmsghdr *cmsg;
  int send_fds[SEND_FDS];
  int junk[256];
  int n_open = 0;
  int ret;
  int i;

  ret = pipe(p1);
  assert_return_code(ret, errno);

  ret = pipe(p2);
  assert_return_code(ret, errno);

  ret = socketpair(AF_LOCAL, SOCK_STREAM, 0, sv);
  assert_return_code(ret, errno);

  send_fds[0] = p1[0];
  send_fds[1] = p1[1];
  send_fds[2] = p2[0];

  memset(&msg, 0, sizeof(msg));
  memset(cmsgbuf, 0, sizeof(cmsgbuf));

  iov.iov_base = &send_buf;
  iov.iov_len  = 1;

  msg.msg_iov        = &iov;
  msg.msg_iovlen     = 1;
  msg.msg_control    = cmsgbuf;
  msg.msg_controllen = CMSG_SPACE(sizeof(int) * SEND_FDS);

  cmsg = CMSG_FIRSTHDR(&msg);
  cmsg->cmsg_level = SOL_SOCKET;
  cmsg->cmsg_type  = SCM_RIGHTS;
  cmsg->cmsg_len   = CMSG_LEN(sizeof(int) * SEND_FDS);
  memcpy(CMSG_DATA(cmsg), send_fds, sizeof(int) * SEND_FDS);

  ret = sendmsg(sv[0], &msg, 0);
  assert_true(ret >= 0);

  for (i = 0; i < 256; i++)
    {
      junk[i] = open("/dev/null", O_RDONLY);
      if (junk[i] < 0)
        {
          break;
        }

      n_open++;
    }

  if (n_open == 0)
    {
      /* If /dev/null is not available, try to dup p1[0] */

      for (i = 0; i < 256; i++)
        {
          junk[i] = dup(p1[0]);
          if (junk[i] < 0)
            {
              break;
            }

          n_open++;
        }
    }

  memset(&msg, 0, sizeof(msg));
  memset(cmsgbuf, 0, sizeof(cmsgbuf));
  iov.iov_base = &recv_buf;
  iov.iov_len  = 1;
  msg.msg_iov        = &iov;
  msg.msg_iovlen     = 1;
  msg.msg_control    = cmsgbuf;
  msg.msg_controllen = sizeof(cmsgbuf);

  recvmsg(sv[1], &msg, 0);

  /* cleanup descriptors */

  for (i = 0; i < n_open; i++)
    {
      close(junk[i]);
    }

  close(sv[0]);
  close(sv[1]);
  close(p1[0]);
  close(p1[1]);
  close(p2[0]);
  close(p2[1]);
}
