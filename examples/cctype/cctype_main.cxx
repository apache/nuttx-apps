//***************************************************************************
// apps/examples/cctype/cctype_main.cxx
//
// Licensed to the Apache Software Foundation (ASF) under one or more
// contributor license agreements.  See the NOTICE file distributed with
// this work for additional information regarding copyright ownership.  The
// ASF licenses this file to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance with the
// License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
// License for the specific language governing permissions and limitations
// under the License.
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
                      std::isspace(i), isascii(i)     , std::isprint(i), std::isgraph(i),
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
