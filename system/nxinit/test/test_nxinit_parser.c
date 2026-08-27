/****************************************************************************
 * apps/system/nxinit/test/test_nxinit_parser.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <fcntl.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>
#include <cmocka.h>

#include "../init.h"
#include "../parser.h"
#include "test_nxinit.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_LIBC_TMPDIR
#  define CONFIG_LIBC_TMPDIR "/tmp"
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct mock_parser_ctx_s
{
  int section_count;
  int line_count;
  int check_count;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int mock_parse(FAR const struct parser_s *parser, bool create,
                      FAR char *buf)
{
  FAR struct mock_parser_ctx_s *ctx = parser->priv;

  if (create)
    {
      ctx->section_count++;
    }
  else
    {
      ctx->line_count++;
    }

  return 0;
}

static int mock_check(FAR const struct parser_s *parser)
{
  FAR struct mock_parser_ctx_s *ctx = parser->priv;

  ctx->check_count++;
  return 0;
}

/* Write 'content' to a fresh temp file and hand it to
 * init_parse_config_file() with 'parser'. Returns the parser's return
 * value. The temp file is removed before returning.
 */

static int parse_content(FAR const struct parser_s *parser,
                         FAR const char *content)
{
  char path[] = CONFIG_LIBC_TMPDIR "/nxinit_test_XXXXXX";
  size_t len = strlen(content);
  int fd;
  int ret;

  fd = mkstemp(path);
  assert_true(fd >= 0);

  assert_int_equal(write(fd, content, len), (ssize_t)len);
  close(fd);

  ret = init_parse_config_file(parser, path);
  unlink(path);

  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nxinit_parser_arguments_spaces
 *
 * Description:
 *   Plain space-separated arguments are split into individual tokens.
 ****************************************************************************/

void test_nxinit_parser_arguments_spaces(FAR void **state)
{
  char buf[] = "foo bar baz";
  FAR char *argv[8];
  int ret;

  ret = init_parse_arguments(buf, false, nitems(argv), argv);
  assert_int_equal(ret, 3);
  assert_string_equal(argv[0], "foo");
  assert_string_equal(argv[1], "bar");
  assert_string_equal(argv[2], "baz");
}

/****************************************************************************
 * Name: test_nxinit_parser_arguments_quoted
 *
 * Description:
 *   A quoted argument containing whitespace is kept as a single token,
 *   with the surrounding quotes stripped.
 ****************************************************************************/

void test_nxinit_parser_arguments_quoted(FAR void **state)
{
  char buf[] = "foo \"bar baz\"";
  FAR char *argv[8];
  int ret;

  ret = init_parse_arguments(buf, false, nitems(argv), argv);
  assert_int_equal(ret, 2);
  assert_string_equal(argv[0], "foo");
  assert_string_equal(argv[1], "bar baz");
}

/****************************************************************************
 * Name: test_nxinit_parser_arguments_dashdash_separator
 *
 * Description:
 *   A standalone "--" token stops normal splitting and folds everything
 *   that follows into a single, final argument.
 ****************************************************************************/

void test_nxinit_parser_arguments_dashdash_separator(FAR void **state)
{
  char buf[] = "foo -- bar baz";
  FAR char *argv[8];
  int ret;

  ret = init_parse_arguments(buf, false, nitems(argv), argv);
  assert_int_equal(ret, 3);
  assert_string_equal(argv[0], "foo");
  assert_string_equal(argv[1], "--");
  assert_string_equal(argv[2], "bar baz");
}

/****************************************************************************
 * Name: test_nxinit_parser_arguments_long_option
 *
 * Description:
 *   Regression test: a long option such as "--system" must not be
 *   misinterpreted as the "--" argument separator (fixed upstream).
 ****************************************************************************/

void test_nxinit_parser_arguments_long_option(FAR void **state)
{
  char buf[] = "service foo /bin/foo --system --nofork";
  FAR char *argv[8];
  int ret;

  ret = init_parse_arguments(buf, false, nitems(argv), argv);
  assert_int_equal(ret, 5);
  assert_string_equal(argv[0], "service");
  assert_string_equal(argv[1], "foo");
  assert_string_equal(argv[2], "/bin/foo");
  assert_string_equal(argv[3], "--system");
  assert_string_equal(argv[4], "--nofork");
}

/****************************************************************************
 * Name: test_nxinit_parser_arguments_truncate
 *
 * Description:
 *   When the number of tokens exceeds the caller-provided argv capacity,
 *   parsing stops without overflowing argv.
 ****************************************************************************/

void test_nxinit_parser_arguments_truncate(FAR void **state)
{
  char buf[] = "one two three four five";
  FAR char *argv[3];
  int ret;

  ret = init_parse_arguments(buf, false, nitems(argv), argv);
  assert_int_equal(ret, 3);
  assert_string_equal(argv[0], "one");
  assert_string_equal(argv[1], "two");

  /* The remaining, still-unsplit text is folded into the last argv slot
   * rather than truncated away; pin down that exact behavior so a future
   * change from "fold" to "drop" is caught here instead of silently
   * changing init.rc semantics.
   */

  assert_string_equal(argv[2], "three four five");
}

/****************************************************************************
 * Name: test_nxinit_parser_config_sections
 *
 * Description:
 *   Lines are routed to the currently active section based on the
 *   longest matching keyword, and sub-lines attach to that section
 *   until a new section keyword is seen.
 ****************************************************************************/

void test_nxinit_parser_config_sections(FAR void **state)
{
  struct mock_parser_ctx_s ctx_a =
    {
      0
    };

  struct mock_parser_ctx_s ctx_b =
    {
      0
    };

  struct parser_s table[] =
    {
      {"secA", mock_parse, mock_check, &ctx_a},
      {"secB", mock_parse, mock_check, &ctx_b},
      {NULL},
    };

  int ret;

  ret = parse_content(table,
                      "secA one\n"
                      "  line-a1\n"
                      "  line-a2\n"
                      "secB two\n"
                      "  line-b1\n");

  assert_int_equal(ret, 0);
  assert_int_equal(ctx_a.section_count, 1);
  assert_int_equal(ctx_a.line_count, 2);
  assert_int_equal(ctx_a.check_count, 1);
  assert_int_equal(ctx_b.section_count, 1);
  assert_int_equal(ctx_b.line_count, 1);
  assert_int_equal(ctx_b.check_count, 1);
}

/****************************************************************************
 * Name: test_nxinit_parser_config_skip_blank_lines
 *
 * Description:
 *   Empty lines and lines containing only whitespace are skipped and do
 *   not reach the section's parse callback.
 ****************************************************************************/

void test_nxinit_parser_config_skip_blank_lines(FAR void **state)
{
  struct mock_parser_ctx_s ctx =
    {
      0
    };

  struct parser_s table[] =
    {
      {"secA", mock_parse, mock_check, &ctx},
      {NULL},
    };

  int ret;

  ret = parse_content(table,
                      "secA one\n"
                      "\n"
                      "   \n"
                      "  line-a1\n");

  assert_int_equal(ret, 0);
  assert_int_equal(ctx.section_count, 1);
  assert_int_equal(ctx.line_count, 1);
}

/****************************************************************************
 * Name: test_nxinit_parser_config_unknown_section
 *
 * Description:
 *   A line that matches no known section keyword, while no section is
 *   currently active, is rejected with -EINVAL.
 ****************************************************************************/

void test_nxinit_parser_config_unknown_section(FAR void **state)
{
  struct mock_parser_ctx_s ctx =
    {
      0
    };

  struct parser_s table[] =
    {
      {"secA", mock_parse, mock_check, &ctx},
      {NULL},
    };

  int ret;

  ret = parse_content(table, "notasection foo\n");

  assert_int_equal(ret, -EINVAL);
}

/****************************************************************************
 * Name: test_nxinit_parser_config_line_too_long
 *
 * Description:
 *   A single line without a newline that exceeds
 *   CONFIG_SYSTEM_NXINIT_RC_LINE_MAX is rejected with -E2BIG.
 ****************************************************************************/

void test_nxinit_parser_config_line_too_long(FAR void **state)
{
  struct mock_parser_ctx_s ctx =
    {
      0
    };

  struct parser_s table[] =
    {
      {"secA", mock_parse, mock_check, &ctx},
      {NULL},
    };

  char content[CONFIG_SYSTEM_NXINIT_RC_LINE_MAX + 32];
  int ret;
  int i;

  for (i = 0; i < (int)sizeof(content) - 1; i++)
    {
      content[i] = 'x';
    }

  content[sizeof(content) - 1] = '\0';

  ret = parse_content(table, content);
  assert_int_equal(ret, -E2BIG);
}

/****************************************************************************
 * Name: test_nxinit_parser_config_line_crosses_boundary
 *
 * Description:
 *   The config file is read in chunks no larger than
 *   CONFIG_SYSTEM_NXINIT_RC_LINE_MAX. This test feeds a file several
 *   times larger than that buffer to verify the leftover-bytes bookkeeping
 *   (memmove) does not corrupt or drop lines that straddle a refill.
 ****************************************************************************/

void test_nxinit_parser_config_line_crosses_boundary(FAR void **state)
{
  struct mock_parser_ctx_s ctx =
    {
      0
    };

  struct parser_s table[] =
    {
      {"secA", mock_parse, mock_check, &ctx},
      {NULL},
    };

  char content[CONFIG_SYSTEM_NXINIT_RC_LINE_MAX * 4];
  FAR char *p = content;
  size_t remaining = sizeof(content);
  int blocks = 0;
  int ret;
  int n;

  for (; ; )
    {
      n = snprintf(p, remaining, "secA blk%d\n  cmd%d\n", blocks, blocks);
      if (n < 0 || (size_t)n >= remaining)
        {
          break;
        }

      p += n;
      remaining -= n;
      blocks++;
    }

  /* The final, over-budget snprintf() call still writes a truncated
   * partial block (plus its own NUL) at 'p' before being rejected by
   * the length check above; cut it back off so content only holds the
   * 'blocks' complete lines that were actually accounted for.
   */

  *p = '\0';

  assert_true(blocks > 4);

  ret = parse_content(table, content);
  assert_int_equal(ret, 0);
  assert_int_equal(ctx.section_count, blocks);
  assert_int_equal(ctx.line_count, blocks);
  assert_int_equal(ctx.check_count, 1);
}

/****************************************************************************
 * Name: test_nxinit_parser_config_buffer_crosses_boundary
 *
 * Description:
 *   init_parse_config_buffer() is the buffer-based counterpart of
 *   init_parse_config_file() used by init_parse_configs() to parse the
 *   builtin "preset" rc content; it is a separate code path with its own
 *   refill bookkeeping. Feed it the exact same multi-block, over-budget
 *   content as test_nxinit_parser_config_line_crosses_boundary() (rather
 *   than routing through a temp file) to make sure lines that straddle a
 *   refill are still tracked correctly and, in particular, that copying
 *   the next chunk on top of the 'n' leftover bytes already held in the
 *   internal buffer does not overflow it.
 ****************************************************************************/

void test_nxinit_parser_config_buffer_crosses_boundary(FAR void **state)
{
  struct mock_parser_ctx_s ctx =
    {
      0
    };

  struct parser_s table[] =
    {
      {"secA", mock_parse, mock_check, &ctx},
      {NULL},
    };

  char content[CONFIG_SYSTEM_NXINIT_RC_LINE_MAX * 4];
  FAR char *p = content;
  size_t remaining = sizeof(content);
  int blocks = 0;
  int ret;
  int n;

  for (; ; )
    {
      n = snprintf(p, remaining, "secA blk%d\n  cmd%d\n", blocks, blocks);
      if (n < 0 || (size_t)n >= remaining)
        {
          break;
        }

      p += n;
      remaining -= n;
      blocks++;
    }

  assert_true(blocks > 4);

  ret = init_parse_config_buffer(table, content, (size_t)(p - content));
  assert_int_equal(ret, 0);
  assert_int_equal(ctx.section_count, blocks);
  assert_int_equal(ctx.line_count, blocks);
  assert_int_equal(ctx.check_count, 1);
}
