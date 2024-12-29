/****************************************************************************
 * apps/examples/fmsynth/mmlplayer_score.h
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

#ifndef __APPS_EXAMPLES_FMSYNTH_MMLPLAYER_SCORE_H
#define __APPS_EXAMPLES_FMSYNTH_MMLPLAYER_SCORE_H

static const char *floh_walzer_right =
  "T120 L4 O4"
  "R8 {D#C#}8"
  "R8{[<A#>F#][<A#>F#]}{D#C#}8 R8{[<A#>F#][<A#>F#]}{D#C#}8"
  "R8[<A#>F#]8R8[<A#>F#]8      R8{[<B>F][<B>F]}     {D#C#}8"
  "R8{[<B>F][<B>F]}{D#C#}8     R8{[<B>F][<B>F]}{D#C#}8    "
  "R8[<B>F]8R8[<B>F]8          R8{[<A#>F#][<A#>F#]} {D#C#}8"
  "R8{[<A#>F#][<A#>F#]}{D#C#}8 R8{[<A#>F#][<A#>F#]}{D#C#}8"
  "R8[<A#>F#]8R8[<A#>F#]8      R8{[<B>F][<B>F]}     {D#C#}8"
  "R8{[<B>F][<B>F]}{D#C#}8     R8{[<B>F][<B>F]}{D#C#}8    "
  "R8[<B>F]8R8[<B>F]8          R8{[<A#>F#][<A#>F#]} {D#C#}8"
  "R8[<A#>F#]8R8[<A#>F#]8      R8[<A#>F#]8R8[<A#>F#]8     "
  "R2                          R8{[<B>F][<B>F]}     {D#C#}8"
  "R8[<B>F]8R8[<B>F]8          R8[<B>F]8R8[<B>F]8         "
  "R2                          R8{[<A#>F#][<A#>F#]} {D#C#}8"
  "R8{[<A#>F#][<A#>F#]}{D#C#}8 R8{[<A#>F#][<A#>F#]}{D#C#}8"
  "R8[<A#>F#]8R8[<A#>F#]8      R8{[<B>F][<B>F]}     {D#C#}8"
  "R8{[<B>F][<B>F]}{D#C#}8     R8{[<B>F][<B>F]}{D#C#}8    "
  "R8[<B>F]8R8[<B>F]8          R8{[<A#>F#][<A#>F#]} {D#C#}8"
  "[<A#>F#]8{CC}8{C#C}         R8[<B>F]8[<A#>F#]8R8";

static const char *floh_walzer_left =
  "T120 L4 O3"
  "R4"
  "F#.R8  F#.R8 F#D#       C#.R8"
  "C#.R8  C#.R8 C#D#       F#.R8"
  ">A#.R8 A#.R8 A#>C#      D#.R8"
  "D#.R8  D#.R8 D#C#       <A#.R8"
  "<F#C#  F#C#  {F#F}{F#G} G#R"
  "G#C#   G#C#  {G#G}{G#A} A#R"
  "F#.R8  F#.R8 F#D#       C#.R8"
  "C#.R8  C#.R8 C#D#       F#.R8"
  "C#8R8R R8C#8<F#8R8";

#endif  /* __APPS_EXAMPLES_FMSYNTH_MMLPLAYER_SCORE_H */
