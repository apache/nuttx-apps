//***************************************************************************
// examples/cctype/cctype_main.cxx
//
//   Copyright (C) 2016 Gregory Nutt. All rights reserved.
//   Author: Gregory Nutt <gnutt@nuttx.org>
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in
//    the documentation and/or other materials provided with the
//    distribution.
// 3. Neither the name NuttX nor the names of its contributors may be
//    used to endorse or promote products derived from this software
//    without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
// OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
// AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
// ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
//***************************************************************************

//***************************************************************************
// Included Files
//***************************************************************************

#include <nuttx/config.h>

#include <cstdio>
#include <cctype>

//***************************************************************************
// Public Functions
//***************************************************************************

/****************************************************************************
 * Name: cctype_main
 ****************************************************************************/

extern "C"
{
  int main(int argc, char *argv[])
  {
     std::printf("\n      A B C D E F G H I J K L M N O\n");
     for (int i = 0; i < 256; i++)
       {
          int upper = std::toupper(i);
          int lower = std::tolower(i);
          std::printf("%3d %c %d %d %d %d %d %d %d %d %d %d %d %d %d %c %c\n",
                      i, std::isprint(i) ? i : '.',
                      std::isspace(i), std::isascii(i), std::isprint(i), std::isgraph(i),
                      std::iscntrl(i), std::islower(i), std::isupper(i), std::isalpha(i),
                      std::isblank(i), std::isdigit(i), std::isalnum(i), std::ispunct(i),
                      std::isxdigit(i),
                      std::isprint(upper) ? upper : '.',
                      std::isprint(lower) ? lower : '.');
       }

     std::printf("\nKEY:\n");
     std::printf("\tA: isspace\tI: isblank\n");
     std::printf("\tB: isascii\tJ: isdigit\n");
     std::printf("\tC: isprint\tK: isalnum\n");
     std::printf("\tD: isgraph\tL: ispunct\n");
     std::printf("\tE: iscntrl\tM: isxdigit\n");
     std::printf("\tF: islower\tN: toupper\n");
     std::printf("\tG: isupper\t0: tolower\n");
     std::printf("\tH: isalpha\n");
     return 0;
  }
}
