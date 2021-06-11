/****************************************************************************
 * apps/include/netutils/curl4nx.h
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

#ifndef __APPS_INCLUDE_NETUTILS_CURL4NX_H
#define __APPS_INCLUDE_NETUTILS_CURL4NX_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Public Defines
 ****************************************************************************/

enum curl4nxerrors_e
{
  CURL4NXE_OK,
  CURL4NXE_UNSUPPORTED_PROTOCOL,
  CURL4NXE_FAILED_INIT,
  CURL4NXE_URL_MALFORMAT,
  CURL4NXE_NOT_BUILT_IN,
  CURL4NXE_COULDNT_RESOLVE_HOST,
  CURL4NXE_COULDNT_CONNECT,
  CURL4NXE_HTTP2,
  CURL4NXE_PARTIAL_FILE,
  CURL4NXE_HTTP_RETURNED_ERROR,
  CURL4NXE_WRITE_ERROR,
  CURL4NXE_UPLOAD_FAILED,
  CURL4NXE_READ_ERROR,
  CURL4NXE_OUT_OF_MEMORY,
  CURL4NXE_OPERATION_TIMEDOUT,
  CURL4NXE_RANGE_ERROR,
  CURL4NXE_HTTP_POST_ERROR,
  CURL4NXE_SSL_CONNECT_ERROR,
  CURL4NXE_BAD_DOWNLOAD_RESUME,
  CURL4NXE_FUNCTION_NOT_FOUND,
  CURL4NXE_ABORTED_BY_CALLBACK,
  CURL4NXE_BAD_FUNCTION_ARGUMENT,
  CURL4NXE_TOO_MANY_REDIRECTS,
  CURL4NXE_UNKNOWN_OPTION,
  CURL4NXE_GOT_NOTHING,
  CURL4NXE_SEND_ERROR,
  CURL4NXE_RECV_ERROR,
  CURL4NXE_SSL_CERTPROBLEM,
  CURL4NXE_SSL_CIPHER,
  CURL4NXE_PEER_FAILED_VERIFICATION,
  CURL4NXE_BAD_CONTENT_ENCODING,
  CURL4NXE_LOGIN_DENIED,
  CURL4NXE_REMOTE_DISK_FULL,
  CURL4NXE_REMOTE_FILE_EXISTS,
  CURL4NXE_SSL_CACERT_BADFILE,
  CURL4NXE_REMOTE_FILE_NOT_FOUND,
  CURL4NXE_AGAIN,
  CURL4NXE_SSL_ISSUER_ERROR,
  CURL4NXE_CHUNK_FAILED,
  CURL4NXE_SSL_PINNEDPUBKEYNOTMATCH,
  CURL4NXE_SSL_INVALIDCERTSTATUS,
  CURL4NXE_HTTP2_STREAM,
  CURL4NXE_RECURSIVE_API_CALL,
};

enum curl4nxopt_e
{
  /* Options that are currently implemented */

  CURL4NXOPT_URL,
  CURL4NXOPT_PORT,
  CURL4NXOPT_BUFFERSIZE,
  CURL4NXOPT_HEADERFUNCTION,
  CURL4NXOPT_HEADERDATA,
  CURL4NXOPT_FAILONERROR,
  CURL4NXOPT_FOLLOWLOCATION,
  CURL4NXOPT_MAXREDIRS,
  CURL4NXOPT_VERBOSE,

  /* Options that will be implemented */

  CURL4NXOPT_HEADER,
  CURL4NXOPT_NOPROGRESS,
  CURL4NXOPT_WRITEFUNCTION,
  CURL4NXOPT_WRITEDATA,
  CURL4NXOPT_READFUNCTION,
  CURL4NXOPT_READDATA,
  CURL4NXOPT_XFERINFOFUNCTION,
  CURL4NXOPT_XFERINFODATA,
  CURL4NXOPT_KEEP_SENDING_ON_ERROR,
  CURL4NXOPT_PATH_AS_IS,
  CURL4NXOPT_TCP_KEEPALIVE,
  CURL4NXOPT_TCP_KEEPIDLE,
  CURL4NXOPT_TCP_KEEPINTVL,
  CURL4NXOPT_PUT,
  CURL4NXOPT_POST,
  CURL4NXOPT_POSTFIELDS,
  CURL4NXOPT_POSTFIELDSIZE,
  CURL4NXOPT_COPYPOSTFIELDS,
  CURL4NXOPT_HTTPPOST,
  CURL4NXOPT_REFERER,
  CURL4NXOPT_USERAGENT,
  CURL4NXOPT_HTTPHEADER,
  CURL4NXOPT_HEADEROPT,
  CURL4NXOPT_HTTPGET,
  CURL4NXOPT_HTTP_VERSION,

  /* Options that will be implemented later */

  CURL4NXOPT_SSL_CTX_FUNCTION,
  CURL4NXOPT_SSL_CTX_DATA,
  CURL4NXOPT_USERNAME,
  CURL4NXOPT_PASSWORD,
  CURL4NXOPT_HTTPAUTH,
  CURL4NXOPT_POSTREDIR,
  CURL4NXOPT_COOKIE,
  CURL4NXOPT_COOKIEFILE,
  CURL4NXOPT_COOKIEJAR,
  CURL4NXOPT_COOKIESESSION,
  CURL4NXOPT_COOKIELIST,
  CURL4NXOPT_IGNORE_CONTENT_LENGTH,
  CURL4NXOPT_RANGE,
  CURL4NXOPT_RESUME_FROM,
  CURL4NXOPT_CUSTOMREQUEST,
  CURL4NXOPT_UPLOAD,
  CURL4NXOPT_UPLOAD_BUFFERSIZE,
  CURL4NXOPT_TIMEOUT,
  CURL4NXOPT_TIMEOUT_MS,
  CURL4NXOPT_FRESH_CONNECT,
  CURL4NXOPT_CONNECTTIMEOUT,
  CURL4NXOPT_CONNECTTIMEOUT_MS,
  CURL4NXOPT_USE_SSL,
  CURL4NXOPT_SSLCERT,
  CURL4NXOPT_SSLKEY,
  CURL4NXOPT_KEYPASSWD,
  CURL4NXOPT_SSLVERSION,
  CURL4NXOPT_SSL_VERIFYHOST,
  CURL4NXOPT_SSL_VERIFYPEER,
  CURL4NXOPT_SSL_VERIFYSTATUS,
  CURL4NXOPT_CAPATH,
  CURL4NXOPT_PINNEDPUBLICKEY,
  CURL4NXOPT_RANDOM_FILE,
  CURL4NXOPT_SSL_CIPHER_LIST,
  CURL4NXOPT_TLS13_CIPHERS,
  CURL4NXOPT_SSL_OPTIONS,
};

/* For CURL4NXOPT_HTTP_VERSION */

#define CURL4NX_HTTP_VERSION_1_0 0x10
#define CURL4NX_HTTP_VERSION_1_1 0x11
#define CURL4NX_HTTP_VERSION_2_0 0x20 /* Unsupported! */

/****************************************************************************
 * Public Types
 ****************************************************************************/

/****************************************************************************
 * Name: curl4nx_iofunction_f
 *
 * Description: Type of the callback function used with
 *   CURL4NXOPT_WRITEFUNCTION and CURL4NXOPT_READFUNCTION.
 *
 ****************************************************************************/

typedef size_t (*curl4nx_iofunc_f)(FAR char *ptr, size_t size,
                                   size_t nmemb, FAR void *userdata);

/****************************************************************************
 * Name: curl4nx_easy_strerror()
 *
 * Description: Return a textual description for an error code.
 *
 ****************************************************************************/

typedef int (*curl4nx_xferinfofunc_f)(FAR void *clientp,
                                      uint64_t dltotal, uint64_t dlnow,
                                      uint64_t ultotal, uint64_t ulnow);

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: curl4nx_easy_strerror()
 *
 * Description: Return a textual description for an error code.
 *
 ****************************************************************************/

FAR const char *curl4nx_easy_strerror(int errornum);

/****************************************************************************
 * Name: curl4nx_easy_init()
 *
 * Description: Initialize a context for curl4nx operations.
 *
 ****************************************************************************/

FAR struct curl4nx_s *curl4nx_easy_init(void);

/****************************************************************************
 * Name: curl4nx_easy_escape()
 *
 * Description: Escape an URL to remove any forbidden character. Returned
 *   string MUST be released using free().
 *
 ****************************************************************************/

FAR char *curl4nx_easy_escape(FAR struct curl4nx_s *handle,
                              FAR const char *string, int length);

/****************************************************************************
 * Name: curl4nx_easy_unescape()
 *
 * Description: Unescape an escaped URL to replace all escaped characters.
 *   Returned string MUST be released using free().
 *
 ****************************************************************************/

FAR char *curl4nx_easy_unescape(FAR struct curl4nx_s *handle,
                                FAR const char *url, int inlength,
                                FAR int *outlength);

/****************************************************************************
 * Name: curl4nx_easy_setopt()
 *
 * Description: Define an option in the given curl4nx context.
 *
 ****************************************************************************/

int curl4nx_easy_setopt(FAR struct curl4nx_s *handle, int option,
                        FAR void *optionptr);

/****************************************************************************
 * Name: curl4nx_easy_reset()
 *
 * Description:
 *   Reset all operations that were set using curl4nx_easy_setopt()
 *   to their default values.
 *
 ****************************************************************************/

void curl4nx_easy_reset(FAR struct curl4nx_s *handle);

/****************************************************************************
 * Name: curl4nx_easy_duphandle()
 *
 * Description: Clone the current state of a handle.
 *
 ****************************************************************************/

FAR struct curl4nx_s *curl4nx_easy_duphandle(FAR struct curl4nx_s *handle);

/****************************************************************************
 * Name: curl4nx_easy_perform()
 *
 * Description: Perform the http operation that was set up in this curl4nx
 *   context.
 *
 ****************************************************************************/

int curl4nx_easy_perform(FAR struct curl4nx_s *handle);

/****************************************************************************
 * Name: curl4nx_easy_getinfo()
 *
 * Description: Get information about the current transfer.
 *
 ****************************************************************************/

int curl4nx_easy_getinfo(FAR struct curl4nx_s *handle, int info,
                         FAR void *infoptr);

/****************************************************************************
 * Name: curl4nx_easy_cleanup()
 *
 * Description: Cleanup any resource allocated within a curl4nx context.
 *
 ****************************************************************************/

void curl4nx_easy_cleanup(FAR struct curl4nx_s *handle);

#ifdef __cplusplus
}
#endif

#endif /* __APPS_INCLUDE_NETUTILS_BASE64_H */
