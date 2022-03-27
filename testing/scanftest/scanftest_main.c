/****************************************************************************
 * apps/testing/scanftest/scanftest_main.c
 *
 *   Copyright (C) 2005-01-26, Greg King (https://github.com/cc65)
 *
 * Original License:
 * This software is provided 'as-is', without any express or implied
 * warranty.
 * In no event will the authors be held liable for any damages arising from
 * the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software in
 *    a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not
 *    be misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source distribution.
 *
 *   Modified: Johannes Schock
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>
#include <libgen.h>
#include <errno.h>
#include <debug.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ARRAYSIZE(a) (sizeof (a) / sizeof (a)[0])

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct test_data_s
{
  FAR const char *input;
  FAR const char *format;
  int rvalue;
  enum TYPE
  {
    INT,
    CHAR,
    FLOAT,
    DOUBLE
  } type1;
  union
  {
    int nvalue;
    float fvalue;
    double dvalue;
    const char *svalue;
  } v1;
  enum TYPE type2;
  union
  {
    int nvalue;
    float fvalue;
    double dvalue;
    const char *svalue;
  } v2;
};

struct type_data_s
{
  FAR const char *input;
  FAR const char *format;
  union
  {
    long long s;
    unsigned long long u;
  } value;
  enum MODTYPE
  {
    HH_MOD_S,
    HH_MOD_U,
    H_MOD_S,
    H_MOD_U,
    NO_MOD_S,
    NO_MOD_U,
    L_MOD_S,
    L_MOD_U,
    LL_MOD_S,
    LL_MOD_U
  } type;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Name: cmd_basename
 ****************************************************************************/

static const struct test_data_s test_data[] =
{
  /* Input sequences for character specifiers must be less than 80 characters
   * long.  These format strings are allowwed a maximum of two assignment
   * specifications.
   */

  /* Test that literals match, and that they aren't seen as conversions. **
   * Test that integer specifiers can handle end-of-file.
   */

  {
    "qwerty   Dvorak", "qwerty  Dvorak", 0, INT,
    {
      .nvalue = 0
    },
    INT,
    {
      .nvalue = 0
    }
  },                            /* 1 */
  {
    "qwerty", "qwerty  %d%i", EOF, INT,
    {
      .nvalue = 0
    },
    INT,
    {
      .nvalue = 0
    }
  },                            /* 2 */
  {
    "qwerty   ", "qwerty  %d%i", EOF, INT,
    {
      .nvalue = 0
    },
    INT,
    {
      .nvalue = 0
    }
  },                            /* 3 */

  /* Test that integer specifiers scan properly. */

  {
    "qwerty   a", "qwerty  %d%i", 0, INT,
    {
      .nvalue = 0
    },
    INT,
    {
      .nvalue = 0
    }
  },                            /* 4 */
  {
    "qwerty   -", "qwerty  %d%i", 0, INT,
    {
      .nvalue = 0
    },
    INT,
    {
      .nvalue = 0
    }
  },                            /* 5 */
  {
    "qwerty   -9", "qwerty  %d%i", 1, INT,
    {
      .nvalue = -9
    },
    INT,
    {
      .nvalue = 0
    }
  },                            /* 6 */
  {
    "qwerty   -95", "qwerty  %d%i", 1, INT,
    {
      .nvalue = -95
    },
    INT,
    {
      .nvalue = 0
    }
  },                            /* 7 */
  {
    "qwerty   -95a", "qwerty  %d%i", 1, INT,
    {
      .nvalue = -95
    },
    INT,
    {
      .nvalue = 0
    }
  },                            /* 8 */
  {
    "qwerty   -95a 1", "qwerty  %d%i", 1, INT,
    {
      .nvalue = -95
    },
    INT,
    {
      .nvalue = 0
    }
  },                            /* 9 */
  {
    "qwerty   -a", "qwerty  %d%i", 0, INT,
    {
      .nvalue = 0
    },
    INT,
    {
      .nvalue = 0
    }
  },                            /* 10 */
  {
    "qwerty   -95 1", "qwerty  %d%i", 2, INT,
    {
      .nvalue = -95
    },
    INT,
    {
      .nvalue = 1
    }
  },                            /* 11 */
  {
    "qwerty    95  2", "qwerty  %i", 1, INT,
    {
      .nvalue = 95
    },
    INT,
    {
      .nvalue = 0
    }
  },                            /* 12 */
  {
    "qwerty   -95 +2", "qwerty  %x%o", 2, INT,
    {
      .nvalue = -0x95
    },
    INT,
    {
      .nvalue = 2
    }
  },                            /* 13 */
  {
    "qwerty  0x9e 02", "qwerty  %i%i", 2, INT,
    {
      .nvalue = 0x9e
    },
    INT,
    {
      .nvalue = 2
    }
  },                            /* 14 */
  {
    "qwerty   095  2", "qwerty  %i%i", 2, INT,
    {
      .nvalue = 0
    },
    INT,
    {
      .nvalue = 95
    }
  },                           /* 15 */
  {
    "qwerty   0e5  2", "qwerty  %i%i", 1, INT,
    {
      .nvalue = 0
    },
    INT,
    {
      .nvalue = 0
    }
  },                            /* 16 */

  /* Test that character specifiers can handle end-of-file. */

  {
    "qwerty", "qwerty  %s%s", EOF, CHAR,
    {
      .svalue = ""
    },
    CHAR,
    {
      .svalue = ""
    }
  },                           /* 17 */
  {
    "qwerty   ", "qwerty  %s%s", EOF, CHAR,
    {
      .svalue = ""
    },
    CHAR,
    {
      .svalue = ""
    }
  },                           /* 18 */
  {
    "qwerty", "qwerty  %c%c", EOF, CHAR,
    {
      .svalue = ""
    },
    CHAR,
    {
      .svalue = ""
    }
  },                           /* 19 */
  {
    "qwerty   ", "qwerty  %c%c", EOF, CHAR,
    {
      .svalue = ""
    },
    CHAR,
    {
      .svalue = ""
    }
  },                           /* 20 */
  {
    "qwerty", "qwerty  %[ a-z]%c", EOF, CHAR,
    {
      .svalue = ""
    },
    CHAR,
    {
      .svalue = ""
    }
  },                           /* 21 */
  {
    "qwerty   ", "qwerty  %[ a-z]%c", EOF, CHAR,
    {
      .svalue = ""
    },
    CHAR,
    {
      .svalue = ""
    }
  },                           /* 22 */
  {
    "qwerty   ", "qwerty%s%s", EOF, CHAR,
    {
      .svalue = ""
    },
    CHAR,
    {
      .svalue = ""
    }
  },                           /* 23 */

  /* Test that character specifiers scan properly. */

  {
    "123456qwertyasdfghzxcvbn!@#$%^QWERTYASDFGHZXCV"
    "BN7890-=uiop[]\\jkl;'m,./&*()_+UIOP{}|JKL:\"M<>?",
    "%79s%79s", 2, CHAR,
    {
      .svalue = "123456qwertyasdfghzxcvbn!@#$%^QWERTYASDFGHZXCV"
                "BN7890-=uiop[]\\jkl;'m,./&*()_+UIO"
    },
    CHAR,
    {
      .svalue = "P{}|JKL:\"M<>?"
    }
  },                           /* 24 */
  {
    "qwerty   ", "qwerty%c%c", 2, CHAR,
    {
      .svalue = " "
    },
    CHAR,
    {
    .svalue = " "
    }
  },                           /* 25 */
  {
    "qwerty   ", "qwerty%2c%c", 2, CHAR,
    {
      .svalue = "  "
    },
    CHAR,
    {
      .svalue = " "
    }
  },                           /* 26 */
  {
    "qwerty   ", "qwerty%2c%2c", 1, CHAR,
    {
      .svalue = "  "
    },
    CHAR,
    {
      .svalue = " "
    }
  },                           /* 27 */
  {
    "qwerty   ", "qwerty%[ a-z]%c", 1, CHAR,
    {
      .svalue = "   "
    },
    CHAR,
    {
      .svalue = ""
    }
  },                           /* 28 */
  {
    "qwerty  q", "qwerty%[ a-z]%c", 1, CHAR,
    {
      .svalue = "  q"
    },
    CHAR,
    {
      .svalue = ""
    }
  },                           /* 29 */
  {
    "qwerty  Q", "qwerty%[ a-z]%c", 2, CHAR,
    {
      .svalue = "  "
    },
    CHAR,
    {
      .svalue = "Q"
    }
  },                           /* 30 */
  {
    "qwerty-QWERTY-", "%[q-ze-]%[-A-Z]", 2, CHAR,
    {
      .svalue = "qwerty-"
    },
    CHAR,
    {
      .svalue = "QWERTY-"
    }
  },                           /* 31 */

  /* Test the space-separation of strings. */

  {
    "qwerty qwerty", "qwerty%s%s", 1, CHAR,
    {
      .svalue = "qwerty"
    },
    CHAR,
    {
      .svalue = ""
    }
  },                           /* 32 */
  {
    "qwerty qwerty Dvorak", "qwerty%s%s", 2, CHAR,
    {
      .svalue = "qwerty"
    },
    CHAR,
    {
      .svalue = "Dvorak"
    }
  },                           /* 33 */

  /* Test the mixxing of types. */

  {
    "qwerty  abc3", "qwerty%s%X", 1, CHAR,
    {
      .svalue = "abc3"
    },
    INT,
    {
      .nvalue = 0
    }
  },                           /* 34 */
  {
    "qwerty  abc3", "qwerty%[ a-z]%X", 2, CHAR,
    {
      .svalue = "  abc"
    },
    INT,
    {
      .nvalue = 3
    }
  },                           /* 35 */
  {
    "qwerty  abc3", "qwerty%[ a-z3]%X", 1, CHAR,
    {
      .svalue = "  abc3"
    },
    INT,
    {
      .nvalue = 0
    }
  },                           /* 36 */
  {
    "qwerty  abc3", "qwerty%[ A-Z]%X", 2, CHAR,
    {
      .svalue = "  "
    },
    INT,
    {
      .nvalue = 0xabc3
    }
  },                           /* 37 */
  {
    "qwerty  3abc", "qwerty%i%[ a-z]", 2, INT,
    {
      .nvalue = 3
    },
    CHAR,
    {
      .svalue = "abc"
    }
  },                           /* 38 */
  {
    "qwerty  3abc", "qwerty%i%[ A-Z]", 1, INT,
    {
      .nvalue = 3
    },
    CHAR,
    {
      .svalue = ""
    }
  },                           /* 39 */

  /* Test the character-count specifier. */

  {
    "  95 5", "%n%i", 1, INT,
    {
      .nvalue = 0
    },
    INT,
    {
      .nvalue = 95
    }
  },                          /* 40 */
  {
    "  a5 5", "%n%i", 0, INT,
    {
      .nvalue = 0
    },
    INT,
    {
      .nvalue = 0
    }
  },                           /* 41 */
  {
    "  a5 5", "%x%n", 1, INT,
    {
      .nvalue = 0xa5
    },
    INT,
    {
      .nvalue = 4
    }
  },                           /* 42 */
  {
    "  a5 5", " %4c%n", 1, CHAR,
    {
      .svalue = "a5 5"
    },
    INT,
    {
      .nvalue = 6
    }
  },                           /* 43 */
  {
    " 05a9", "%i%n", 1, INT,
    {
      .nvalue = 5
    },
    INT,
    {
      .nvalue = 3
    }
  },                           /* 44 */

  /* Test assignment-suppression. */

  {
    "  95 6", "%n%*i", 0, INT,
    {
      .nvalue = 0
    },
    INT,
    {
      .nvalue = 0
    }
  },                           /* 45 */
  {
    "  a5 6", "%*x%n", 0, INT,
    {
      .nvalue = 4
    },
    INT,
    {
      .nvalue = 0
    }
  },                           /* 46 */
  {
    "  a5 6", "%*x%n%o", 1, INT,
    {
      .nvalue = 4
    },
    INT,
    {
      .nvalue = 6
    }
  },                           /* 47 */
  {
    "  a5 6", " %*4c%d", 0, CHAR,
    {
      .svalue = ""
    },
    INT,
    {
      .nvalue = 0
    }
  },                          /* 48 */
  {
    "The first number is 7.  The second number is 8.\n",
      "%*[ .A-Za-z]%d%*[ .A-Za-z]%d", 2, INT,
    {
      .nvalue = 7
    },
    INT,
    {
       .nvalue = 8
    }
  },                           /* 49 */

  /* Test that float specifiers can handle end-of-file. */

  {
    "qwerty", "qwerty  %f%i", EOF, FLOAT,
    {
      .fvalue = 0
    },
    INT,
    {
      .nvalue = 0
    }
  },                           /* 50 */
  {
    "qwerty   ", "qwerty  %f%i", EOF, FLOAT,
    {
      .fvalue = 0
    },
    INT,
    {
      .nvalue = 0
    }
  },                           /* 51 */

  /* Test that float specifiers scan properly. */

  {
    "qwerty   a", "qwerty  %f%i", 0, FLOAT,
    {
      .fvalue = 0
    },
    INT,
    {
      .nvalue = 0
    }
  },                           /* 52 */
  {
    "qwerty   -", "qwerty  %f%i", 0, FLOAT,
    {
      .fvalue = 0
    },
    INT,
    {
      .nvalue = 0
    }
  },                           /* 53 */
  {
    "qwerty   9.87654321", "qwerty  %f%i", 1, FLOAT,
    {
      .fvalue = 9.87654321f
    },
    INT,
    {
      .nvalue = 0
    }
  },                           /* 54 */
  {
    "qwerty   +9.87654321", "qwerty  %f%i", 1, FLOAT,
    {
      .fvalue = 9.87654321f
    },
    INT,
    {
      .nvalue = 0
    }
  },                           /* 55 */
  {
    "qwerty   -9.87654321", "qwerty  %f%i", 1, FLOAT,
    {
      .fvalue = -9.87654321f
    },
    INT,
    {
      .nvalue = 0
    }
  },                           /* 56 */
  {
    "qwerty   9.87654321E8", "qwerty  %f%i", 1, FLOAT,
    {
      .fvalue = 9.87654321e8f
    },
    INT,
    {
      .nvalue = 0
    }
  },                           /* 57 */
  {
    "qwerty   +9.87654321E+8", "qwerty  %f%i", 1, FLOAT,
    {
      .fvalue = 9.87654321e8f
    },
    INT,
    {
      .nvalue = 0
    }
  },                           /* 58 */
  {
    "qwerty   -9.87654321e8", "qwerty  %f%i", 1, FLOAT,
    {
      .fvalue = -9.87654321e8f
    },
    INT,
    {
      .nvalue = 0
    }
  },                           /* 59 */
  {
    "qwerty   9.87654321E-8", "qwerty  %f%i", 1, FLOAT,
    {
      .fvalue = 9.87654321E-8f
    },
    INT,
    {
      .nvalue = 0
    }
  },                           /* 60 */
  {
    "qwerty   -9.87654321e-8", "qwerty  %f%i", 1, FLOAT,
    {
      .fvalue = -9.87654321E-8f
    },
    INT,
    {
      .nvalue = 0
    }
  },                           /* 61 */
  {
    "qwerty   9.87654321", "qwerty  %lf%i", 1, DOUBLE,
    {
      .dvalue = 9.87654321l
    },
    INT,
    {
      .nvalue = 0
    }
  },                           /* 62 */
  {
    "qwerty   +9.87654321", "qwerty  %lf%i", 1, DOUBLE,
    {
      .dvalue = 9.87654321l
    },
    INT,
    {
      .nvalue = 0
    }
  },                           /* 63 */
  {
    "qwerty   -9.87654321", "qwerty  %lf%i", 1, DOUBLE,
    {
      .dvalue = -9.87654321l
    },
    INT,
    {
      .nvalue = 0
    }
  },                           /* 64 */
  {
    "qwerty   9.87654321e8", "qwerty  %lf%i", 1, DOUBLE,
    {
      .dvalue = 9.87654321e8l
    },
    INT,
    {
      .nvalue = 0
    }
  },                           /* 65 */
  {
    "qwerty   +9.87654321e+8", "qwerty  %lf%i", 1, DOUBLE,
    {
      .dvalue = 9.87654321e8l
    },
    INT,
    {
      .nvalue = 0
    }
  },                           /* 66 */
  {
    "qwerty   -9.87654321e8", "qwerty  %lf%i", 1, DOUBLE,
    {
      .dvalue = -9.87654321e8l
    },
    INT,
    {
      .nvalue = 0
    }
  },                           /* 67 */
  {
    "qwerty   9.87654321e-8", "qwerty  %lf%i", 1, DOUBLE,
    {
      .dvalue = 9.87654321E-8l
    },
    INT,
    {
      .nvalue = 0
    }
  },                           /* 68 */
  {
    "qwerty   -9.87654321E-8", "qwerty  %lf%i", 1, DOUBLE,
    {
      .dvalue = -9.87654321E-8l
    },
    INT,
    {
      .nvalue = 0
    }
  },                           /* 69 */
};

/* Test the char, short, and long specification-modifiers. */

static const struct type_data_s type_data[] =
{
  {
    " 123456789", "%hhd",
    {
      .s = (signed char)123456789L
    },
    HH_MOD_S
  },                           /*  1 */
  {
    "+123456789", "%hhd",
    {
      .s = (signed char)123456789L
    },
    HH_MOD_S
  },                           /*  2 */
  {
    "-123456789", "%hhd",
    {
      .s = (signed char)-123456789L
    },
    HH_MOD_S
  },                           /*  3 */
  {
    "+123456789", "%hhu",
    {
      .u = (unsigned char)123456789L
    },
    HH_MOD_U
  },                           /*  4 */
  {
    "-123456789", "%hhu",
    {
      .u = (unsigned char)-123456789L
    },
    HH_MOD_U
  },                           /*  5 */
  {
    " 123456789", "%hd",
    {
      .s = (signed short)123456789L
    },
    H_MOD_S
  },                           /*  6 */
  {
    "+123456789", "%hd",
    {
      .s = (signed short)123456789L
    },
    H_MOD_S
  },                           /*  7 */
  {
    "-123456789", "%hd",
    {
      .s = (signed short)-123456789L
    },
    H_MOD_S
  },                           /*  8 */
  {
    "+123456789", "%hu",
    {
      .u = (unsigned short)123456789L
    },
    H_MOD_U
  },                           /*  9 */
  {
    "-123456789", "%hu",
    {
      .u = (unsigned short)-123456789L
    },
    H_MOD_U
  },                           /* 10 */
  {
    " 123456789", "%d",
    {
      .s = (signed int)123456789L
    },
    NO_MOD_S
  },                           /* 11 */
  {
    "+123456789", "%d",
    {
      .s = (signed int)123456789L
    },
    NO_MOD_S
  },                           /* 12 */
  {
    "-123456789", "%d",
    {
      .s = (signed int)-123456789L
    },
    NO_MOD_S
  },                           /* 13 */
  {
    "+123456789", "%u",
    {
      .u = (unsigned int)123456789L
    },
    NO_MOD_U
  },                           /* 14 */
  {
    "-123456789", "%u",
    {
      .u = (unsigned int)-123456789L
    },
    NO_MOD_U
  },                           /* 15 */
  {
    " 123456789", "%ld",
    {
      .s = (signed long)123456789L
    },
    L_MOD_S
  },                           /* 16 */
  {
    "+123456789", "%ld",
    {
      .s = (signed long)123456789L
    },
    L_MOD_S
  },                           /* 17 */
  {
    "-123456789", "%ld",
    {
      .s = (signed long)-123456789L
    },
    L_MOD_S
  },                           /* 18 */
  {
    "+123456789", "%lu",
    {
      .u = (unsigned long)123456789L
    },
    L_MOD_U
  },                           /* 19 */
  {
    "-123456789", "%lu",
    {
      .u = (unsigned long)-123456789L
    },
    L_MOD_U
  },                           /* 20 */
  {
    " 123456789123456789", "%lld",
    {
      .s = (signed long long)123456789123456789LL
    },
    LL_MOD_S
  },                           /* 21 */
  {
    "+123456789123456789", "%lld",
    {
      .s = (signed long long)123456789123456789LL
    },
    LL_MOD_S
  },                           /* 22 */
  {
    "-123456789123456789", "%lld",
    {
      .s = (signed long long)-123456789123456789LL
    },
    LL_MOD_S
  },                           /* 23 */
  {
    "+123456789123456789", "%llu",
    {
      .u = (unsigned long long)123456789123456789LL
    },
    LL_MOD_U
  },                           /* 24 */
  {
    "-123456789123456789", "%llu",
    {
      .u = (unsigned long long)-123456789123456789LL
    },
    LL_MOD_U
  },                           /* 25 */
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * scanftest_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int t;
  int i;
  int c;
  int n1 = 12345;
  int n2;
  bool ok;
  char s1[84];
  char s2[80];
  float f1;
  float f2;
  double d1;
  double d2;
  FAR FILE *fp;

  FAR const char *teststring = "teststring a";
  FAR const char *fname = "/mnt/fs/test.txt";

  /* Test that scanf() can recognize percent-signs in the input. ** Test that
   * integer converters skip white-space. ** Test that "%i" can scan a single
   * zero digit (followed by EOF).
   */

  sscanf("%  \n\f\v\t  0", "%%%i", &n1);
  if (n1 != 0)
    {
      printf("sscanf()'s \"%%%%%%i\" couldn't scan either a \"%%\" "
             "or a single zero digit.\n\n");
    }

  /* Test scanf()'s return-value: EOF if input ends before the first *
   * conversion-attempt begins; an assignment-count, otherwise. * Test that
   * scanf() properly converts and assigns the correct number * of arguments.
   */

  for (i = 0; i < 2; i++)
    {
      if (i)
        {
          char s3[3];

          printf("\nBack to Back Test...\n");

          memset(s1, '\0', sizeof s1);
          memset(s2, '\0', sizeof s2);
          memset(s3, '\0', sizeof s3);

          fp = fopen(fname, "wb");
          if (fp)
            {
              fputs(teststring, fp);
              fclose(fp);
              fp = fopen(fname, "rb");
              if (fp != NULL)
                {
                  fscanf(fp, "%s", s2);
                  fscanf(fp, "%2c", s3);
                  sprintf(s1, "%s%s", s2, s3);

                  if (strcmp(s1, teststring) != 0)
                    {
                      printf("Error %s != %s.\n", teststring, s1);
                    }
                  else
                    {
                      printf("Test PASSED.\n");
                    }
                }
              else
                {
                  printf("Error opening %s for read.\n", fname);
                }
            }
          else
            {
              printf("Error opening %s for write.\n", fname);
            }
        }

      printf("\nTesting %cscanf()'s return-value,\nconversions, and "
             "assignments...\n",
             i ? 'f' : 's');

      for (t = 0; t < ARRAYSIZE(test_data); ++t)
        {
          /* Prefill the arguments with zeroes. */

          f1 = f2 = d1 = d2 = n1 = n2 = 0;
          memset(s1, '\0', sizeof s1);
          memset(s2, '\0', sizeof s2);

          ok = true;

          if (i)
            {
              fp = fopen(fname, "wb");
              if (fp)
                {
                  fputs(test_data[t].input, fp);
                  fclose(fp);
                }
              else
                {
                  printf("Error opening %s for write.\n", fname);
                  break;
                }

              fp = fopen(fname, "rb");
              if (fp)
                {
                  c = fscanf
                  (fp, test_data[t].format,

                  /* Avoid warning messages about different  pointer-
                   * types, by casting them to void-pointers.
                   */

                  test_data[t].type1 == INT ? (FAR void *)&n1 :
                    test_data[t].type1 == FLOAT ? (FAR void *)&f1 :
                      test_data[t].type1 == DOUBLE ? (FAR void *)&d1 :
                        (FAR void *)s1,
                  test_data[t].type2 == INT ? (FAR void *)&n2 :
                    test_data[t].type2 == FLOAT ? (FAR void *)&f2 :
                      test_data[t].type2 == DOUBLE ? (FAR void *)&d2 :
                        (FAR void *)s2
                  );

                  fclose(fp);
                }
              else
                {
                  printf("Error opening %s for read.\n", fname);
                  break;
                }
            }
          else
            {
              c = sscanf
              (test_data[t].input, test_data[t].format,

              /* Avoid warning messages about different pointer-types, by
               * casting them to void-pointers.
               */

              test_data[t].type1 == INT ? (FAR void *)&n1 :
                test_data[t].type1 == FLOAT ? (FAR void *)&f1 :
                  test_data[t].type1 == DOUBLE ? (FAR void *)&d1 :
                    (FAR void *)s1,
              test_data[t].type2 == INT ? (FAR void *)&n2 :
                test_data[t].type2 == FLOAT ? (FAR void *)&f2 :
                  test_data[t].type2 == DOUBLE ? (FAR void *)&d2 :
                    (FAR void *)s2
              );
            }

          if (c != test_data[t].rvalue)
            {
              printf("Test #%u returned %d instead of %d.\n", t + 1, c,
                     test_data[t].rvalue);
              ok = false;
            }

          if (test_data[t].type1 == INT)
            {
              if (test_data[t].v1.nvalue != n1)
                {
                  printf("Test #%u assigned %i, instead of %i,\n"
                         "\tto the first argument.\n\n", t + 1, n1,
                         test_data[t].v1.nvalue);
                  ok = false;
                }
            }
          else if (test_data[t].type1 == FLOAT)
            {
              if (test_data[t].v1.fvalue != f1)
                {
                  printf("Test #%u assigned %e, instead of %e,\n"
                         "\tto the first argument.\n\n", t + 1, f1,
                         test_data[t].v1.fvalue);
                  ok = false;
                }
            }
          else if (test_data[t].type1 == DOUBLE)
            {
              if (test_data[t].v1.dvalue != d1)
                {
                  printf("Test #%u assigned %le, instead of %le,\n"
                         "\tto the first argument.\n\n", t + 1, d1,
                         test_data[t].v1.dvalue);
                  ok = false;
                }
            }
          else
            {
              /* test_data[t].type1 == CHAR */

              if (strcmp(test_data[t].v1.svalue, s1))
                {
                  printf("Test #%u assigned\n\"%s\",\n"
                         "\tinstead of\n\"%s\",\n"
                         "\tto the first argument.\n\n", t + 1, s1,
                         test_data[t].v1.svalue);
                  ok = false;
                }
            }

          if (test_data[t].type2 == INT)
            {
              if (test_data[t].v2.nvalue != n2)
                {
                  printf("Test #%u assigned %i, instead of %i,\n"
                         "\tto the second argument.\n\n", t + 1, n2,
                         test_data[t].v2.nvalue);
                  ok = false;
                }
            }
          else if (test_data[t].type2 == FLOAT)
            {
              if (test_data[t].v2.fvalue != f2)
                {
                  printf("Test #%u assigned %e, instead of %e,\n"
                         "\tto the second argument.\n\n", t + 1, f2,
                         test_data[t].v2.fvalue);
                  ok = false;
                }
            }
          else if (test_data[t].type2 == DOUBLE)
            {
              if (test_data[t].v2.dvalue != d2)
                {
                  printf("Test #%u assigned %le, instead of %le,\n"
                         "\tto the second argument.\n\n", t + 1, d2,
                         test_data[t].v2.dvalue);
                  ok = false;
                }
            }
          else
            {
              /* test_data[t].type2 == CHAR */

              if (strcmp(test_data[t].v2.svalue, s2))
                {
                  printf("Test #%u assigned\n\"%s\",\n"
                         "\tinstead of\n\"%s\",\n"
                         "\tto the second argument.\n\n", t + 1, s2,
                         test_data[t].v2.svalue);
                  ok = false;
                }
            }

          if (ok)
            {
              printf("Test #%u PASSED.\n", t + 1);
            }
        }
    }

  /* Test the char, short, and long specification-modifiers. */

  printf("\nTesting scanf()'s type-modifiers...\n");
  for (t = 0; t < ARRAYSIZE(type_data); ++t)
    {
      unsigned char hhu;
      unsigned short hu;
      unsigned int nou;
      unsigned long lu;
      unsigned long long llu;
      signed char hhs;
      signed short hs;
      signed int nos;
      signed long ls;
      signed long long lls;

      ok = true;
      switch (type_data[t].type)
        {
        case HH_MOD_S:
          hhs = 0L;
          sscanf(type_data[t].input, type_data[t].format, &hhs);
          if (type_data[t].value.s != hhs)
            {
              printf("Test #%u assigned %hhd instead of %lli.\n", t + 1,
                     hhs, type_data[t].value.s);
              ok = false;
            }
          break;

        case HH_MOD_U:
          hhu = 0L;
          sscanf(type_data[t].input, type_data[t].format, &hhu);
          if (type_data[t].value.u != hhu)
            {
              printf("Test #%u assigned %hhu instead of %lli.\n",
                     t + 1, hhu, type_data[t].value.u);
              ok = false;
            }
          break;

        case H_MOD_S:
          hs = 0L;
          sscanf(type_data[t].input, type_data[t].format, &hs);
          if (type_data[t].value.s != hs)
            {
              printf("Test #%u assigned %hd instead of %lli.\n",
                     t + 1, hs, type_data[t].value.s);
              ok = false;
            }
          break;

        case H_MOD_U:
          hu = 0L;
          sscanf(type_data[t].input, type_data[t].format, &hu);
          if (type_data[t].value.u != hu)
            {
              printf("Test #%u assigned %hu instead of %lli.\n",
                     t + 1, hu, type_data[t].value.u);
              ok = false;
            }
          break;

        case NO_MOD_S:
          nos = 0L;
          sscanf(type_data[t].input, type_data[t].format, &nos);
          if (type_data[t].value.s != nos)
            {
              printf("Test #%u assigned %d instead of %lli.\n",
                     t + 1, nos, type_data[t].value.s);
              ok = false;
            }
          break;

        case NO_MOD_U:
          nou = 0L;
          sscanf(type_data[t].input, type_data[t].format, &nou);
          if (type_data[t].value.u != nou)
            {
              printf("Test #%u assigned %u instead of %lli.\n",
                     t + 1, nou, type_data[t].value.u);
              ok = false;
            }
          break;

        case L_MOD_S:
          ls = 0L;
          sscanf(type_data[t].input, type_data[t].format, &ls);
          if (type_data[t].value.s != ls)
            {
              printf("Test #%u assigned %ld instead of %lli.\n",
                     t + 1, ls, type_data[t].value.s);
              ok = false;
            }
          break;

        case L_MOD_U:
          lu = 0L;
          sscanf(type_data[t].input, type_data[t].format, &lu);
          if (type_data[t].value.u != lu)
            {
              printf("Test #%u assigned %lu instead of %lli.\n",
                     t + 1, lu, type_data[t].value.u);
              ok = false;
            }
          break;

        case LL_MOD_S:
          lls = 0L;
          sscanf(type_data[t].input, type_data[t].format, &lls);
          if (type_data[t].value.s != lls)
            {
              printf("Test #%u assigned %lld instead of %lli.\n",
                     t + 1, lls, type_data[t].value.s);
              ok = false;
            }
          break;

        case LL_MOD_U:
          llu = 0L;
          sscanf(type_data[t].input, type_data[t].format, &llu);
          if (type_data[t].value.u != llu)
            {
              printf("Test #%u assigned %llu instead of %lli.\n",
                     t + 1, llu, type_data[t].value.u);
              ok = false;
            }
          break;
        }

      if (ok)
        {
          printf("Test #%u PASSED.\n", t + 1);
        }
    }

  return OK;
}
