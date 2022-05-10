/****************************************************************************
 * apps/examples/mml_parser/mml_parser_main.c
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

#ifndef CONFIG_AUDIOUTILS_MMLPARSER_LIB
#error "This example needs to enable config of AUDIOUTILS_MMLPARSER_LIB," \
       " please enable it"
#endif

#include <audioutils/mml_parser.h>

/****************************************************************************
 * Private Data
 ****************************************************************************/

#define SIMPLE_SCORE  \
  "O0 CC+DD+EE+FF+GG+AA+BB+" \
  "O1 CC-DD-EE-FF-GG-AA-BB-" \
  "O2 CC#DD#EE#FF#GG#AA#BB#"

#define TEST_SCORE \
  "T240L0O3D>F+>E8 V8{R [CE O7 < G] C+#- C >B}8 [G B > C]16" \
  "T120 D> C A8 B. A8+4 C+8.D#+4+4 E- C.+4."

#define FLOH_WALZER_RIGHT \
  "T120 L4 O4"  \
  "R8 {D#C#}8"  \
  "R8{[<A#>F#][<A#>F#]}{D#C#}8 R8{[<A#>F#][<A#>F#]}{D#C#}8" \
  "R8[<A#>F#]8R8[<A#>F#]8      R8{[<B>F][<B>F]}     {D#C#}8"  \
  "R8{[<B>F][<B>F]}{D#C#}8     R8{[<B>F][<B>F]}{D#C#}8" \
  "R8[<B>F]8R8[<B>F]8          R8{[<A#>F#][<A#>F#]} {D#C#}8"  \
  "R8{[<A#>F#][<A#>F#]}{D#C#}8 R8{[<A#>F#][<A#>F#]}{D#C#}8" \
  "R8[<A#>F#]8R8[<A#>F#]8      R8{[<B>F][<B>F]}     {D#C#}8"  \
  "R8{[<B>F][<B>F]}{D#C#}8     R8{[<B>F][<B>F]}{D#C#}8" \
  "R8[<B>F]8R8[<B>F]8          R8{[<A#>F#][<A#>F#]} {D#C#}8"  \
  "R8[<A#>F#]8R8[<A#>F#]8      R8[<A#>F#]8R8[<A#>F#]8"  \
  "R2                          R8{[<B>F][<B>F]}     {D#C#}8"  \
  "R8[<B>F]8R8[<B>F]8          R8[<B>F]8R8[<B>F]8"  \
  "R2                          R8{[<A#>F#][<A#>F#]} {D#C#}8"  \
  "R8{[<A#>F#][<A#>F#]}{D#C#}8 R8{[<A#>F#][<A#>F#]}{D#C#}8" \
  "R8[<A#>F#]8R8[<A#>F#]8      R8{[<B>F][<B>F]}     {D#C#}8"  \
  "R8{[<B>F][<B>F]}{D#C#}8     R8{[<B>F][<B>F]}{D#C#}8" \
  "R8[<B>F]8R8[<B>F]8          R8{[<A#>F#][<A#>F#]} {D#C#}8"  \
  "[<A#>F#]8{CC}8{C#C}         R8[<B>F]8[<A#>F#]8R8"

#define FLOH_WALZER_LEFT \
  "T120 L4 O3"  \
  "R4"  \
  "F#.R8  F#.R8 F#D#       C#.R8" \
  "C#.R8  C#.R8 C#D#       F#.R8" \
  ">A#.R8 A#.R8 A#>C#      D#.R8" \
  "D#.R8  D#.R8 D#C#       <A#.R8"  \
  "<F#C#  F#C#  {F#F}{F#G} G#R" \
  "G#C#   G#C#  {G#G}{G#A} A#R" \
  "F#.R8  F#.R8 F#D#       C#.R8" \
  "C#.R8  C#.R8 C#D#       F#.R8" \
  "C#8R8R R8C#8<F#8R8"

static const char *test_scores[] =
{
  SIMPLE_SCORE, TEST_SCORE, FLOH_WALZER_RIGHT, FLOH_WALZER_LEFT,
};

#define TEST_SCORES_NUM (sizeof(test_scores)/sizeof(test_scores[0]))

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: print_parse_result
 ****************************************************************************/

int print_parse_result(int ret_code, FAR struct mml_result_s *result)
{
  int passed_tick = 0;

  switch (ret_code)
    {
      case MML_TYPE_NOTE:
        printf("Note(%2d) : length = %d\n",
            result->note_idx[0], result->length);
        passed_tick += result->length;
        break;
      case MML_TYPE_REST:
        printf("Rest     : length = %d\n", result->length);
        passed_tick += result->length;
        break;
      case MML_TYPE_TEMPO:
        printf("Tempo    : length = %d\n", result->length);
        break;
      case MML_TYPE_LENGTH:
        printf("Length   : length = %d\n", result->length);
        break;
      case MML_TYPE_OCTAVE:
        printf("Octave   : length = %d\n", result->length);
        break;
      case MML_TYPE_TUPLETSTART:
        printf("Tuplet B : length = %d\n", result->length);
        break;
      case MML_TYPE_TUPLETDONE:
        printf("Tuplet D :\n");
        break;
      case MML_TYPE_VOLUME:
        printf("Volume   : length = %d\n", result->length);
        break;
      case MML_TYPE_CHORD:
        printf("Chord    : length = %d, chord_num = %d\n",
            result->length, result->chord_notes);
        printf("         : Notes ");
        {
          int i;
          for (i = 0; i < result->chord_notes; i++)
            {
              printf("%d ", result->note_idx[i]);
            }

          printf("\n");
        }

        passed_tick += result->length;
        break;
    }

  return passed_tick;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main
 ****************************************************************************/

int main(void)
{
  int i;
  int ret;
  int total_tick;

  FAR char *score;
  struct music_macro_lang_s mml;
  struct mml_result_s result;

  for (i = 0; i < TEST_SCORES_NUM; i++)
    {
      init_mml(&mml, 48000, 1, 0, 4);

      score = (char *)test_scores[i];
      printf("===================================\n");
      printf("Test Score :\n%s\n\n", score);

      total_tick = 0;
      while ((ret = parse_mml(&mml, &score, &result)) > 0)
        {
          total_tick += print_parse_result(ret, &result);
        }

      if (ret < 0)
        {
          printf("\nret = %d\n", ret);
          printf("Error was occured below:\n");
          printf("%s\n", score);
          break;
        }

      printf("\n=== Total Tick : %d\n\n", total_tick);
    }

  return 0;
}
