/****************************************************************************
 * apps/include/netutils/urldecode.h
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

#ifndef __APPS_INCLUDE_NETUTILS_URLDECODE_H
#define __APPS_INCLUDE_NETUTILS_URLDECODE_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef CONFIG_CODECS_URLCODE_NEWMEMORY
char *url_encode(char *str);
char *url_decode(char *str);
#endif /* CONFIG_CODECS_URLCODE_NEWMEMORY */

#ifdef CONFIG_CODECS_URLCODE
char *urlencode(const char *src, const int src_len,
                char *dest, int *dest_len);
char *urldecode(const char *src, const int src_len,
                char *dest, int *dest_len);
int urlencode_len(const char *src, const int src_len);
int urldecode_len(const char *src, const int src_len);
#endif /* CONFIG_CODECS_URLCODE */

#ifdef CONFIG_CODECS_AVR_URLCODE
void urlrawdecode(char *urlbuf);
void urlrawencode(char *str, char *urlbuf);
#endif /* CONFIG_CODECS_AVR_URLCODE */

#ifdef __cplusplus
}
#endif

#endif /* __APPS_INCLUDE_NETUTILS_URLDECODE_H */
