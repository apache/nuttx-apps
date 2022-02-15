/****************************************************************************
 * apps/netutils/thttpd/config.h
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

#ifndef __APPS_NETUTILS_THTTPD_CONFIG_H
#define __APPS_NETUTILS_THTTPD_CONFIG_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/compiler.h>
#include <stdint.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Make sure that the system is configured to handle THTTPD */

#undef CONFIG_THTTPD
#if defined(CONFIG_NET) && defined(CONFIG_NET_TCP) && \
    defined(CONFIG_NET_TCPBACKLOG) && !defined(CONFIG_DISABLE_ENVIRON)

#  define CONFIG_THTTPD 1

/* Check all THTTPD configuration settings.  Complain on any that should have
 * been defined but were not.  Supply some kind of reasonable value for all
 * undefined settings.
 */

/* Server port number */

#  ifndef CONFIG_THTTPD_PORT
#    define CONFIG_THTTPD_PORT 80
#  endif

/* Server IP address (no host name) */

#  ifndef CONFIG_THTTPD_IPADDR
#    ifdef CONFIG_CPP_HAVE_WARNING
#      warning "CONFIG_THTTPD_IPADDR not defined"
#    endif
#    define CONFIG_THTTPD_IPADDR (10<<24|0<<16|0<<8|2)
#  endif

/* SERVER_ADDRESS: response */

#  ifndef CONFIG_THTTPD_SERVER_ADDRESS
#    define CONFIG_THTTPD_SERVER_ADDRESS "http://www.nuttx.org"
#  endif

/* SERVER_SOFTWARE: response */

#  ifndef CONFIG_THTTPD_SERVER_SOFTWARE
#    define CONFIG_THTTPD_SERVER_SOFTWARE "thttpd/2.25b 29dec2003-NuttX"
#  endif

#  ifndef CONFIG_THTTPD_PATH
#    ifdef CONFIG_CPP_HAVE_WARNING
#      warning "CONFIG_THTTPD_PATH not defined"
#    endif
#    define CONFIG_THTTPD_PATH "/mnt/www"
#  endif

#  ifndef CONFIG_THTTPD_CGI_PATH
#    ifdef CONFIG_CPP_HAVE_WARNING
#      warning "CONFIG_THTTPD_CGI_PATH not defined"
#    endif
#    define CONFIG_THTTPD_CGI_PATH "/mnt/www/cgi-bin"
#  endif

/* Only CGI programs whose fully expanded paths match this pattern will be
 * executed.  In fact, if this value is not defined then no CGI logic will
 * be built.
 */

#  ifndef CONFIG_THTTPD_CGI_PATTERN
#    define CONFIG_THTTPD_CGI_PATTERN "/mnt/www/cgi-bin/*"
#  endif

/* These provide the priority and stack size of the CGI child tasks
 */

#  ifndef CONFIG_THTTPD_CGI_PRIORITY
#    define CONFIG_THTTPD_CGI_PRIORITY 50
#  endif

#  ifndef CONFIG_THTTPD_CGI_STACKSIZE
#    define CONFIG_THTTPD_CGI_STACKSIZE 2048
#  endif

/* Byte output limit for CGI tasks
 */

#  ifndef CONFIG_THTTPD_CGI_BYTECOUNT
#    define CONFIG_THTTPD_CGI_BYTECOUNT 200000
#  endif

/* How many seconds to allow CGI programs to run before killing them.
 */

#  ifndef CONFIG_THTTPD_CGI_TIMELIMIT
#    define CONFIG_THTTPD_CGI_TIMELIMIT 0 /* No time limit */
#  endif

/* The default character set name to use with text MIME types.
 */

#  ifndef CONFIG_THTTPD_CHARSET
#    define CONFIG_THTTPD_CHARSET "iso-8859-1"
#  endif

/* Initial buffer size, buffer reallocation increment, maximum buffer size
 */

#  ifndef CONFIG_THTTPD_IOBUFFERSIZE
#    define CONFIG_THTTPD_IOBUFFERSIZE 256
#  endif

#  ifndef CONFIG_THTTPD_MINSTRSIZE
#   define CONFIG_THTTPD_MINSTRSIZE 64
#  endif

#  ifndef CONFIG_THTTPD_REALLOCINCR
#    define CONFIG_THTTPD_REALLOCINCR 64
#  endif

#  ifndef CONFIG_THTTPD_MAXREALLOC
#    define CONFIG_THTTPD_MAXREALLOC 4096
#  endif

#  ifndef CONFIG_THTTPD_CGIINBUFFERSIZE
#    define CONFIG_THTTPD_CGIINBUFFERSIZE 512   /* Size of buffer to interpose input */
#  endif

#  ifndef CONFIG_THTTPD_CGIOUTBUFFERSIZE
#    define CONFIG_THTTPD_CGIOUTBUFFERSIZE 512  /* Size of buffer to interpose output */
#  endif

#  if CONFIG_THTTPD_IOBUFFERSIZE > 65535
#    error "Can't use uint16_t for buffer size"
#  endif

/* A list of index filenames to check.
 * The files are searched for in this order.
 */

#  ifndef CONFIG_THTTPD_INDEX_NAMES
#    define CONFIG_THTTPD_INDEX_NAMES "index.html", "index.htm", "index.cgi"
#  endif

/* CONFIG_AUTH_FILE - The file to use for authentication. If this is defined
 *   then thttpd checks for this file in the local directory before every
 *   fetch. If the file exists then authentication is done, otherwise the
 *   fetch proceeds as usual.
 *   If you leave this undefined then thttpd will not implement
 *   authentication at all and will not check for auth files, which saves a
 *   bit of CPU time. A typical value is ".htpasswd"
 */

/* The listen() backlog queue length.
 */

#  ifndef CONFIG_THTTPD_LISTEN_BACKLOG
#    define CONFIG_THTTPD_LISTEN_BACKLOG 8
#  endif

/* How many milliseconds to leave a connection open while doing a lingering
 * close
 */

#  ifndef CONFIG_THTTPD_LINGER_MSEC
#    define CONFIG_THTTPD_LINGER_MSEC 500
#  endif

/* How often to run the occasional cleanup job.
 */

#  ifndef CONFIG_THTTPD_OCCASIONAL_MSEC
#    define CONFIG_THTTPD_OCCASIONAL_MSEC 120 /* Two minutes */
#  endif

/* How many seconds to allow for reading the initial request on a new
 * connection.
 */

#  ifndef CONFIG_THTTPD_IDLE_READ_LIMIT_SEC
#    define CONFIG_THTTPD_IDLE_READ_LIMIT_SEC 300
#  endif

/* How many seconds before an idle connection gets closed.
 */

#  ifndef CONFIG_THTTPD_IDLE_SEND_LIMIT_SEC
#    define CONFIG_THTTPD_IDLE_SEND_LIMIT_SEC 300
#  endif

/* Memory debug instrumentation depends on other debug options
 */

#  if (!defined(CONFIG_DEBUG_FEATURES) || !defined(CONFIG_DEBUG_NET)) && defined(CONFIG_THTTPD_MEMDEBUG)
#    undef CONFIG_THTTPD_MEMDEBUG
#  endif

/* Tilde mapping.
 * Many URLs use ~username to indicate a user's home directory.
 * thttpd provides two options for mapping this construct to an  actual
 * filename.
 *
 * 1) Map ~username to <prefix>/username. This is the recommended choice.
 *    Each user gets a subdirectory in the main web tree, and the tilde
 *    construct points there.
 *    The prefix could be something like "users", or it could be empty.
 * 2) Map ~username to <user's homedir>/<postfix>. The postfix would be
 *    the name of a subdirectory off of the user's actual home dir, something
 *    like "public_html".
 *
 * You can also leave both options undefined, and thttpd will not do anything
 * special about tildes. Enabling both options is an error.
 *
 * Typical values, if they're defined, are "users" for
 * CONFIG_THTTPD_TILDE_MAP1 and "public_html" for CONFIG_THTTPD_TILDE_MAP2.
 */

#  if defined(CONFIG_THTTPD_TILDE_MAP1) && defined(CONFIG_THTTPD_TILDE_MAP2)
#    error "Both CONFIG_THTTPD_TILDE_MAP1 and CONFIG_THTTPD_TILDE_MAP2 are defined"
#  endif

/* If CONFIG_THTTPD_URLPATTERN is defined, then it will be used to match and
 * verify referrers.
 */

#else /* Dependencies not provided */
#  ifdef CONFIG_CPP_HAVE_WARNING
#    warning "THTTPD not built because dependencies not selected in configuration"
#  endif
#endif /* Dependencies not provided */

#endif /* __APPS_NETUTILS_THTTPD_CONFIG_H */
