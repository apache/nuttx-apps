/****************************************************************************
 * apps/include/audioutils/mml_parser.h
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

#ifndef __APPS_INCLUDE_AUDIOUTILS_MML_PARSER_MML_PARSER_H
#define __APPS_INCLUDE_AUDIOUTILS_MML_PARSER_MML_PARSER_H

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MML_TYPE_EOF         (0)
#define MML_TYPE_NOTE        (1)
#define MML_TYPE_REST        (2)
#define MML_TYPE_TEMPO       (3)
#define MML_TYPE_LENGTH      (4)
#define MML_TYPE_OCTAVE      (5)
#define MML_TYPE_TUPLETSTART (6)
#define MML_TYPE_TUPLETDONE  (7)
#define MML_TYPE_VOLUME      (8)
#define MML_TYPE_TONE        (9)
#define MML_TYPE_CHORD       (10)

#define MML_TYPE_NOTE_ERROR   (-MML_TYPE_NOTE)
#define MML_TYPE_REST_ERROR   (-MML_TYPE_REST)
#define MML_TYPE_TEMPO_ERROR  (-MML_TYPE_TEMPO)
#define MML_TYPE_LENGTH_ERROR (-MML_TYPE_LENGTH)
#define MML_TYPE_OCTAVE_ERROR (-MML_TYPE_OCTAVE)
#define MML_TYPE_VOLUME_ERROR (-MML_TYPE_VOLUME)
#define MML_TYPE_TUPLET_ERROR (-MML_TYPE_TUPLETSTART)
#define MML_TYPE_TONE_ERROR   (-MML_TYPE_TONE)
#define MML_TYPE_CHORD_ERROR  (-MML_TYPE_CHORD)

#define MML_TYPE_ILLIGAL_COMPOSITION   (-100)
#define MML_TYPE_ILLIGAL_TOOMANY_NOTES (-101)
#define MML_TYPE_ILLIGAL_TOOFEW_NOTES  (-102)
#define MML_TYPE_ILLIGAL_DOUBLE_TUPLET (-103)

#define MML_STATE_NORMAL (0)
#define MML_STATE_TUPLET (1)

#define MAX_CHORD_NOTES  (5)

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct music_macro_lang_s
{
  int cur_tempo;
  int cur_octave;
  int def_length;
  int fs;

  int state;
  int cur_tuplet;
  int tuplet_notes;
  int tuplet_length;
};

struct mml_result_s
{
  int note_idx[MAX_CHORD_NOTES];
  int length;
  int chord_notes;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif

int init_mml(FAR struct music_macro_lang_s *mml,
             int fs, int tempo, int octave, int length);
int parse_mml(FAR struct music_macro_lang_s *mml,
              FAR char **score, FAR struct mml_result_s *result);

#ifdef __cplusplus
}
#endif

#endif	/* __APPS_INCLUDE_AUDIOUTILS_MML_PARSER_MML_PARSER_H */
