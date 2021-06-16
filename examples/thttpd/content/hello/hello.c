/****************************************************************************
 * apps/examples/thttpd/content/hello/hello.c
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

#include <nuttx/config.h>

#include <stdio.h>
#include <stdlib.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifdef CONFIG_THTTPD_BINFS
int hello_main(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
  fprintf(stderr, "Hello requested from: %s\n", getenv("REMOTE_ADDR"));

  puts(
    "Content-type: text/html\r\n"
    "Status: 200/html\r\n"
    "\r\n"
    "<html>\r\n"
      "<head>\r\n"
        "<title>Hello!</title>\r\n"
        "<link rel=\"stylesheet\" type=\"text/css\" href=\"/style.css\">\r\n"
      "</head>\r\n"
      "<body bgcolor=\"#fffeec\" text=\"black\">\r\n"
        "<div class=\"menu\">\r\n"
        "<div class=\"menubox\"><a href=\"/index.html\">Front page</a></div>\r\n"
        "<div class=\"menubox\"><a href=\"hello\">Say Hello</a></div>\r\n"
        "<div class=\"menubox\"><a href=\"tasks\">Tasks</a></div>\r\n"
        "<br>\r\n"
        "</div>\r\n"
        "<div class=\"contentblock\">\r\n");
  printf(
        "<h2>Hello, World!</h2><p>Requested by: %s</p>\r\n",
        getenv("REMOTE_ADDR"));
  puts(
      "</body>\r\n"
   "</html>\r\n");
  return 0;
}
