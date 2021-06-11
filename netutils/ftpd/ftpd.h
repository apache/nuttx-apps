/****************************************************************************
 * apps/netutils/ftpd/ftpd.h
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

#ifndef __APPS_NETUTILS_FTPD_FTPD_H
#define __APPS_NETUTILS_FTPD_FTPD_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <stdbool.h>

#include <netinet/in.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* FPTD Definitions *********************************************************/

#define FTPD_SESSIONFLAG_USER       (1 << 0)  /* Session has a user */
#define FTPD_SESSIONFLAG_RESTARTPOS (1 << 1)  /* Session has a restart position */
#define FTPD_SESSIONFLAG_RENAMEFROM (1 << 2)  /* Session has a rename from string */

#define FTPD_LISTOPTION_A           (1 << 0)  /* List option 'A' */
#define FTPD_LISTOPTION_L           (1 << 1)  /* List option 'L' */
#define FTPD_LISTOPTION_F           (1 << 2)  /* List option 'F' */
#define FTPD_LISTOPTION_R           (1 << 3)  /* List option 'R' */
#define FTPD_LISTOPTION_UNKNOWN     (1 << 7)  /* Unknown list option */

#define FTPD_CMDFLAG_LOGIN          (1 << 0)  /* Command requires login */

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* This enumerates the type of each session */

enum ftpd_sessiontype_e
{
  FTPD_SESSIONTYPE_NONE = 0,
  FTPD_SESSIONTYPE_A,
  FTPD_SESSIONTYPE_I,
  FTPD_SESSIONTYPE_L8
};

struct ftpd_pathnode_s
{
  struct ftpd_pathnode_s    *flink;
  struct ftpd_pathnode_s    *blink;
  bool                       ignore;
  FAR char                  *name;
};

union ftpd_sockaddr_u
{
  uint8_t                    raw[sizeof(struct sockaddr_storage)];
  struct sockaddr_storage    ss;
  struct sockaddr            sa;
#ifdef CONFIG_NET_IPv6
  struct sockaddr_in6        in6;
#endif
#ifdef CONFIG_NET_IPv4
  struct sockaddr_in         in4;
#endif
};

/* This structure describes on account */

struct ftpd_account_s
{
  struct ftpd_account_s     *blink;
  struct ftpd_account_s     *flink;
  uint8_t                    flags;    /* See FTPD_ACCOUNTFLAG_* definitions */
  FAR char                  *user;     /* User name */
  FAR char                  *password; /* Un-encrypted password */
  FAR char                  *home;     /* Home directory path */
};

/* This structures describes an FTP session a list of associated accounts */

struct ftpd_server_s
{
  int                        sd;     /* Listen socket descriptor */
  union ftpd_sockaddr_u      addr;   /* Listen address */
  struct ftpd_account_s     *head;   /* Head of a list of accounts */
  struct ftpd_account_s     *tail;   /* Tail of a list of accounts */
};

struct ftpd_stream_s
{
  int                        sd;      /* Socket descriptor */
  union ftpd_sockaddr_u      addr;    /* Network address */
  socklen_t                  addrlen; /* Length of the address */
  size_t                     buflen;  /* Length of the buffer */
  char                      *buffer;  /* Pointer to the buffer */
};

struct ftpd_session_s
{
  FAR struct ftpd_server_s  *server;
  FAR struct ftpd_account_s *head;
  FAR struct ftpd_account_s *curr;
  uint8_t                    flags;   /* See TPD_SESSIONFLAG_* definitions */
  int                        rxtimeout;
  int                        txtimeout;

  /* Command */

  struct ftpd_stream_s       cmd;
  FAR char                  *command;
  FAR char                  *param;

  /* Data */

  struct ftpd_stream_s       data;
  off_t                      restartpos;

  /* File */

  int fd;

  /* Current user */

  FAR char                  *user;
  uint8_t                    type;    /* See enum ftpd_sessiontype_e */
  FAR char                  *home;
  FAR char                  *work;
  FAR char                  *renamefrom;
};

typedef int (*ftpd_cmdhandler_t)(struct ftpd_session_s *);

struct ftpd_cmd_s
{
  FAR const char            *command;  /* The command string */
  ftpd_cmdhandler_t          handler;  /* The function that handles the command */
  uint8_t                    flags;    /* See FTPD_CMDFLAGS_* definitions */
};

/* Used to maintain a list of protocol names */

struct ftpd_protocol_s
{
  FAR const char            *name;
  int value;
};

#endif /* __APPS_NETUTILS_FTPD_FTPD_H */
