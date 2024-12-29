/****************************************************************************
 * apps/examples/fmsynth/music_scale.c
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
 * Public Data
 ****************************************************************************/

/* Musical Scale frequency
 * Negative value means not supported.
 */

const float musical_scale[] =
{
  /* Octave 0 */

  -1.f, -1.f, -1.f,
  -1.f, -1.f, -1.f,
  -1.f, -1.f, -1.f,
  27.5f, 29.13523509f, 30.86770631f,

  /* Octave 1 */

  32.70319563f, 34.64782883f, 36.70809593f,
  38.89087289f, 41.20344452f, 43.65352881f,
  46.2493027f, 48.99942933f, 51.913087f,
  55.f, 58.27047017f, 61.73541262f,

  /* Octave 2 */

  65.40639126f, 69.29565765f, 73.41619185f,
  77.78174577f, 82.40688903f, 87.30705762f,
  92.4986054f, 97.99885866f, 103.826174f,
  110.f, 116.5409403f, 123.4708252f,

  /* Octave 3 */

  130.8127825f, 138.5913153f, 146.8323837f,
  155.5634915f, 164.8137781f, 174.6141152f,
  184.9972108f, 195.9977173f, 207.652348f,
  220.f, 233.0818807f, 246.9416505f,

  /* Octave 4 */

  261.625565f, 277.1826306f, 293.6647674f,
  311.1269831f, 329.6275561f, 349.2282305f,
  369.9944216f, 391.9954347f, 415.304696f,
  440.f, 466.1637614f, 493.8833009f,

  /* Octave 5 */

  523.2511301f, 554.3652612f, 587.3295348f,
  622.2539662f, 659.2551123f, 698.456461f,
  739.9888432f, 783.9908693f, 830.6093921f,
  880.f, 932.3275227f, 987.7666018f,

  /* Octave 6 */

  1046.50226f, 1108.730522f, 1174.65907f,
  1244.507932f, 1318.510225f, 1396.912922f,
  1479.977686f, 1567.981739f, 1661.218784f,
  1760.f, 1864.655045f, 1975.533204f,

  /* Octave 7 */

  2093.00452f, 2217.461045f, 2349.318139f,
  2489.015865f, 2637.020449f, 2793.825844f,
  2959.955373f, 3135.963477f, 3322.437568f,
  3520.f, 3729.310091f, 3951.066407f,

  /* Octave 8 */

  4186.009041f, -1.f, -1.f,
  -1.f, -1.f, -1.f,
  -1.f, -1.f, -1.f,
  -1.f, -1.f, -1.f,
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/
