/****************************************************************************
 * apps/netutils/thttpd/thttpd_alloc.h
 *
 * SPDX-License-Identifier: Apache-2.0
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

#ifndef __APPS_NETUTILS_THTTPD_THTTDP_ALLOC_H
#define __APPS_NETUTILS_THTTPD_THTTDP_ALLOC_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"

#ifdef CONFIG_THTTPD

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* Allows all memory management calls to be intercepted */

#ifdef CONFIG_THTTPD_MEMDEBUG
extern FAR void *httpd_malloc(size_t nbytes);
extern FAR void *httpd_realloc(FAR void *oldptr, size_t oldsize, size_t newsize);
extern void      httpd_free(FAR void *ptr);
extern FAR char *httpd_strdup(const char *str);
#else
#  define httpd_malloc(n)      malloc(n)
#  define httpd_realloc(p,o,n) realloc(p,n)
#  define httpd_free(p)        free(p)
#  define httpd_strdup(s)      strdup(s)
#endif

/* Helpers to support allocations in multiples of a type size */

#define NEW(t,n)               ((t*)httpd_malloc(sizeof(t)*(n)))
#define RENEW(p,t,o,n)         ((t*)httpd_realloc((void*)p, sizeof(t)*(o), sizeof(t)*(n)))

/* Helpers to implement dynamically allocated strings */

extern void httpd_realloc_str(char **pstr, size_t *maxsizeP, size_t size);

#endif /* CONFIG_THTTPD */
#endif /* __APPS_NETUTILS_THTTPD_THTTDP_ALLOC_H */
