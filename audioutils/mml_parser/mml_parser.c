/****************************************************************************
 * apps/audioutils/mml_parser/mml_parser.c
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

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <errno.h>

#include <audioutils/mml_parser.h>

#ifdef DEBUG_ON 
#include <stdio.h>
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define LENGTH_STATE_IDLE     (0)
#define LENGTH_STATE_NUMBERD  (1)
#define LENGTH_STATE_PLUS     (2)

#define CHORD_START '['
#define CHORD_END   ']'

#define TUPLET_START '{'
#define TUPLET_END   '}'

#ifdef DEBUG_ON 
#define DEBUG printf
#else
#define DEBUG(...)
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * name: skip_space
 ****************************************************************************/

static char *skip_space(char *str)
{
  while (isspace(*str))
    {
      str++;
    }

  return str;
}

/****************************************************************************
 * name: next_code
 ****************************************************************************/

static char next_code(char **score)
{
  char ret;
  *score = skip_space(*score);
  ret = **score;
  *score += 1;
  return ret;
}

/****************************************************************************
 * name: note_index
 ****************************************************************************/

static int note_index(char code)
{
  int i;
  const char *code_types = "C+D+EF+G+A+B";

  for (i = 0; i < 12; i++)
    {
      if (code_types[i] == code)
        {
          return i;
        }
    }

  return -1;
}

/****************************************************************************
 * name: halfscale
 ****************************************************************************/

static int halfscale(char **score)
{
  int ret = 0;

  while (1)
    {
      switch (**score)
        {
          case '+':
          case '#':
            ret++;
            break;
          case '-':
            ret--;
            break;
          default:
            return ret;
        }

      *score += 1;
    }

  return ret;
}

/****************************************************************************
 * name: strlendigits
 ****************************************************************************/

static int strlendigits(FAR const char *str)
{
  int ret = 0;

  while (isdigit(*str))
    {
      str++;
      ret++;
    }

  return ret;
}

/****************************************************************************
 * name: calc_samples
 ****************************************************************************/

static int calc_samples(int fs, int tempo, int num, int dots)
{
  int div = 0;
  int n = 0;
  int mul = 16;

  DEBUG("fs=%d, tempo=%d, num=%d, dots=%d\n", fs, tempo, num, dots);

  switch (num)
    {
      case 0:  n   = 3; break;
      case 1:  n   = 2; break;
      case 2:  n   = 1; break;
      case 4:  n   = 0; break;
      case 8:  div = 1; break;
      case 16: div = 2; break;
      case 32: div = 3; break;
      case 64: div = 4; break;
      default: div = -1; break;
    }

  if (dots <= 4)
    {
      while (dots)
        {
          mul += (1 << (4 - dots));
          dots--;
        }
    }
  else
    {
      dots = -1;
    }

  if (div < 0 || dots < 0)
    {
      return -EINVAL;
    }

  return (((15 * fs * mul) << n) >> (2 + div)) / tempo;
}

/****************************************************************************
 * name: get_samples
 ****************************************************************************/

static int get_samples(FAR struct music_macro_lang_s *mml,
                       int samples, int num, int dots, bool plus_mode)
{
  int len;

  num = num < 0 ? mml->def_length : num;
  len = calc_samples(mml->fs, mml->cur_tempo, num, dots);

  if (len > 0)
    {
      return plus_mode ? samples + len : len;
    }

  return len;
}

/****************************************************************************
 * name: is_qualifire
 ****************************************************************************/

static bool is_qualifire(char c)
{
  if (isdigit(c) || c == '.')
    {
      return true;
    }

  return false;
}

/****************************************************************************
 * name: sample_length
 ****************************************************************************/

static int sample_length(FAR struct music_macro_lang_s *mml,
                         FAR char **score)
{
  int dots = 0;
  int samples = 0;
  int state = LENGTH_STATE_IDLE;
  bool plus_mode = false;
  bool parsing = true;
  int num = -1;

  samples = get_samples(mml, samples, num, dots, plus_mode);

  if (!is_qualifire(**score))
    {
      return samples;
    }

  while (parsing)
    {
      DEBUG("In Length parser\n");
      switch (state)
        {
          case LENGTH_STATE_IDLE:
            if (isdigit(**score))
              {
                DEBUG("state[IDLE]: Digits\n");
                num = atoi(*score);
                *score += strlendigits(*score);
                state = LENGTH_STATE_NUMBERD;
              }
            else if (**score == '.')
              {
                DEBUG("state[IDLE]: Dot\n");
                state = LENGTH_STATE_NUMBERD;
                dots++;
                *score += 1;
              }
            else
              {
                DEBUG("state[IDLE]: Other\n");
                samples = get_samples(mml, samples, num, dots, plus_mode);
                parsing = false;
              }

          case LENGTH_STATE_NUMBERD:
            if (**score == '.')
              {
                DEBUG("state[NUM]: Dot\n");
                dots++;
                *score += 1;
              }
            else if (**score == '+')
              {
                DEBUG("state[NUM]: PLUS\n");
                samples = get_samples(mml, samples, num, dots, plus_mode);
                if (samples < 0)
                  {
                    parsing = false;
                  }

                plus_mode = true;
                num = -1;
                *score += 1;
                state = LENGTH_STATE_PLUS;
              }
            else
              {
                DEBUG("state[NUM]: Other\n");
                samples = get_samples(mml, samples, num, dots, plus_mode);
                parsing = false;
              }

            break;

          case LENGTH_STATE_PLUS:
            if (isdigit(**score))
              {
                num = atoi(*score);
                *score += strlendigits(*score);
                DEBUG("state[PLUS]: Digits num=%d, restscore=%s,"
                      " parsing=%s\n",
                       num, *score, parsing ? "True" : "False");
                state = LENGTH_STATE_NUMBERD;
              }
            else
              {
                DEBUG("state[PLUS]: Other\n");
                samples = -EINVAL;
                parsing = false;
              }

            break;

          default:
            parsing = false;
            samples = -EPROTO;
            break;
        }

      DEBUG("Out switch : state[%s]: Digits num=%d, restscore=%s,"
                        " parse=%s\n",
                        state == LENGTH_STATE_IDLE ? "IDLE" :
                        state == LENGTH_STATE_NUMBERD ? "NUM" :
                        state == LENGTH_STATE_PLUS ? "PLUS" : "Unknown",
                        num, *score, parsing ? "True" : "False");
    }

  DEBUG("Out while\n");

  return samples;
}

/****************************************************************************
 * name: tuple_length
 ****************************************************************************/

static int tuplet_length(FAR struct music_macro_lang_s *mml)
{
  int ret;

  ret = mml->tuplet_length / mml->tuplet_notes;
  mml->cur_tuplet++;
  if (mml->cur_tuplet == mml->tuplet_notes)
    {
      /* Adjust surplus */

      ret = mml->tuplet_length - (ret * (mml->tuplet_notes - 1));
    }

  return ret;
}

/****************************************************************************
 * name: handle_note
 ****************************************************************************/

static int handle_note(FAR struct music_macro_lang_s *mml, char code,
                       FAR char **score, FAR struct mml_result_s *result)
{
  result->note_idx[0] = note_index(code) + halfscale(score)
                    + mml->cur_octave * 12;
  result->length = sample_length(mml, score);

  return result->length < 0 ? MML_TYPE_NOTE_ERROR : MML_TYPE_NOTE;
}

/****************************************************************************
 * name: handle_rest
 ****************************************************************************/

static int handle_rest(FAR struct music_macro_lang_s *mml,
                       FAR char **score, FAR struct mml_result_s *result)
{
  if (mml->state == MML_STATE_TUPLET)
    {
      DEBUG("Tuplet : TTL %d, CUR %d\n", mml->tuplet_notes, mml->cur_tuplet);
      result->length = tuplet_length(mml);
      return MML_TYPE_REST;
    }
  else
    {
      result->length = sample_length(mml, score);
      return result->length < 0 ? MML_TYPE_REST_ERROR : MML_TYPE_REST;
    }
}

/****************************************************************************
 * name: handle_tempo
 ****************************************************************************/

static int handle_tempo(FAR struct music_macro_lang_s *mml,
                        FAR char **score, FAR struct mml_result_s *result)
{
  int ret = MML_TYPE_TEMPO;

  if (isdigit(**score))
    {
      mml->cur_tempo = result->length = atoi(*score);
      *score += strlendigits(*score);
    }
  else
    {
      ret = MML_TYPE_TEMPO_ERROR;
    }

  return ret;
}

/****************************************************************************
 * name: handle_length
 ****************************************************************************/

static int handle_length(FAR struct music_macro_lang_s *mml,
                         FAR char **score, FAR struct mml_result_s *result)
{
  int ret = MML_TYPE_LENGTH_ERROR;

  DEBUG("length str : %c\n", **score);
  if (isdigit(**score))
    {
      mml->def_length = result->length = atoi(*score);
      *score += strlendigits(*score);
      ret = MML_TYPE_LENGTH;
    }

  return ret;
}

/****************************************************************************
 * name: handle_octave
 ****************************************************************************/

static int handle_octave(FAR struct music_macro_lang_s *mml, char code,
                         FAR char **score, FAR struct mml_result_s *result)
{
  int ret = MML_TYPE_OCTAVE;

  switch (code)
    {
      case '>':
        mml->cur_octave++;
        result->length = mml->cur_octave;
        break;

      case '<':
        mml->cur_octave--;
        result->length = mml->cur_octave;
        break;

      default:
        if (isdigit(**score))
          {
            mml->cur_octave = result->length = atoi(*score);
            *score += strlendigits(*score);
          }
        else
          {
            ret = MML_TYPE_OCTAVE_ERROR;
          }

        break;
    }

  return ret;
}

/****************************************************************************
 * name: handle_volume
 ****************************************************************************/

static int handle_volume(FAR struct music_macro_lang_s *mml,
                         FAR char **score, FAR struct mml_result_s *result)
{
  int ret = MML_TYPE_VOLUME;

  result->length = atoi(*score);
  *score += strlendigits(*score);

  if (result->length < 0 || result->length > 100)
    {
      ret = MML_TYPE_VOLUME_ERROR;
    }

  return ret;
}

/****************************************************************************
 * name: skip_until
 ****************************************************************************/

static char *skip_until(FAR char *score, char until)
{
  while (*score != until && *score != '\0')
    {
      score++;
    }

  return score;
}

/****************************************************************************
 * name: count_tupletnotes
 ****************************************************************************/

static int count_tupletnotes(FAR struct music_macro_lang_s *mml,
                             FAR char *score)
{
  const char *notes = "CDEFGABR";

  score = skip_space(score);

  while (*score != TUPLET_END)
    {
      if (strchr(notes, *score))
        {
          mml->tuplet_notes++;
        }
      else if (*score == CHORD_START)
        {
          score = skip_until(score, CHORD_END);
          mml->tuplet_notes++;
        }

      if (*score == '\0')
        {
          return -EINVAL;
        }

      score++;
      score = skip_space(score);
    }

  score++;  /* Skip TUPLET_END */
  mml->tuplet_length = sample_length(mml, &score);

  return mml->tuplet_notes != 0 ? OK : -EINVAL;
}

/****************************************************************************
 * name: handle_starttuplet
 ****************************************************************************/

static int handle_starttuplet(FAR struct music_macro_lang_s *mml,
                              FAR char **score,
                              FAR struct mml_result_s *result)
{
  int ret;

  if (mml->state != MML_STATE_NORMAL)
    {
      return MML_TYPE_ILLIGAL_DOUBLE_TUPLET;
    }

  mml->tuplet_notes = 0;
  ret = count_tupletnotes(mml, *score);
  if (ret < 0 || mml->tuplet_notes == 0)
    {
      return MML_TYPE_TUPLET_ERROR;
    }

  mml->state = MML_STATE_TUPLET;
  mml->cur_tuplet = 0;

  result->length = mml->tuplet_length;

  return MML_TYPE_TUPLETSTART;
}

/****************************************************************************
 * name: handle_stoptuplet
 ****************************************************************************/

static int handle_stoptuplet(FAR struct music_macro_lang_s *mml,
                             FAR char **score,
                             FAR struct mml_result_s *result)
{
  int ret = MML_TYPE_TUPLETDONE;

  mml->state = MML_STATE_NORMAL;

  /* Just for skip of length block */

  sample_length(mml, score);

  if (mml->cur_tuplet != mml->tuplet_notes)
    {
      ret = MML_TYPE_ILLIGAL_TOOFEW_NOTES;
    }

  return ret;
}

/****************************************************************************
 * name: handle_tupletnote
 ****************************************************************************/

static int handle_tupletnote(FAR struct music_macro_lang_s *mml, char code,
                             FAR char **score,
                             FAR struct mml_result_s *result)
{
  int ret = MML_TYPE_NOTE;

  DEBUG("Tuplet : TTL %d, CUR %d\n", mml->tuplet_notes, mml->cur_tuplet);
  if (mml->cur_tuplet < mml->tuplet_notes)
    {
      result->note_idx[0] = note_index(code) + halfscale(score)
                         + mml->cur_octave * 12;
      result->length = tuplet_length(mml);
    }
  else
    {
      ret = MML_TYPE_ILLIGAL_TOOMANY_NOTES;
    }

  return ret;
}

/****************************************************************************
 * name: handle_notes
 ****************************************************************************/

static int handle_notes(FAR struct music_macro_lang_s *mml, char code,
                        FAR char **score,
                        FAR struct mml_result_s *result)
{
  int ret;

  result->chord_notes = 1;

  if (mml->state == MML_STATE_TUPLET)
    {
      ret = handle_tupletnote(mml, code, score, result);
    }
  else
    {
      ret = handle_note(mml, code, score, result);
    }

  return ret;
}

/****************************************************************************
 * name: handle_startchord
 ****************************************************************************/

static int handle_startchord(FAR struct music_macro_lang_s *mml,
                             FAR char **score,
                             FAR struct mml_result_s *result)
{
  char code;
  int note_idx;

  code = next_code(score);

  while (code != CHORD_END && code != '\0')
    {
      DEBUG("CHORD: %c\n", code);
      note_idx = note_index(code);
      DEBUG("CHORD note_idx = %d\n", note_idx);
      if (note_idx >= 0)
        {
          note_idx += halfscale(score) + mml->cur_octave * 12;

          if (result->chord_notes < MAX_CHORD_NOTES)
            {
              result->note_idx[result->chord_notes] = note_idx;
              result->chord_notes++;
            }

          /* In case of chord notes are over than MAX_CHORD_NOTES,
           * just ignore overflowed notes. Not behave as an error.
           */
        }
      else
        {
          switch (code)
            {
              case 'O':
              case '>':
              case '<':
                if (handle_octave(mml, code, score, result) < 0)
                  {
                    return MML_TYPE_CHORD_ERROR;
                  }
                break;

              default:
                return MML_TYPE_CHORD_ERROR;
                break;
            }
        }

      code = next_code(score);
    }

  if (code == '\0')
    {
      return MML_TYPE_CHORD_ERROR;
    }

  if (mml->state == MML_STATE_TUPLET)
    {
      result->length = tuplet_length(mml);
    }
  else
    {
      result->length = sample_length(mml, score);
    }

  return result->length < 0 ? MML_TYPE_CHORD_ERROR : MML_TYPE_CHORD;
}

/****************************************************************************
 * name: handle_tone
 ****************************************************************************/

static int handle_tone(FAR struct music_macro_lang_s *mml,
                       FAR char **score, FAR struct mml_result_s *result)
{
  int ret = MML_TYPE_TONE_ERROR;

  if (isdigit(**score))
    {
      result->note_idx[0] = atoi(*score);
      *score += strlendigits(*score);
      ret = MML_TYPE_TONE;
    }

  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * name: init_mml
 ****************************************************************************/

int init_mml(FAR struct music_macro_lang_s *mml, int fs,
             int tempo, int octave, int length)
{
  mml->fs = fs;
  mml->cur_tempo = tempo;
  mml->cur_octave = octave;
  mml->def_length = length;

  mml->state = MML_STATE_NORMAL;
  mml->cur_tuplet = 0;
  mml->tuplet_notes = 0;
  mml->tuplet_length = 0;

  return 0;
}

/****************************************************************************
 * name: parse_mml
 ****************************************************************************/

int parse_mml(FAR struct music_macro_lang_s *mml, FAR char **score,
              FAR struct mml_result_s *result)
{
  int ret;
  char code;

  code = next_code(score);
  DEBUG("code=%c\n", code);
  result->chord_notes = 0;

  switch (code)
    {
      case 'A':
      case 'B':
      case 'C':
      case 'D':
      case 'E':
      case 'F':
      case 'G':
        ret = handle_notes(mml, code, score, result);
        break;

      case 'R':
        ret = handle_rest(mml, score, result);
        break;

      case 'T':
        ret = handle_tempo(mml, score, result);
        break;

      case 'L':
        ret = handle_length(mml, score, result);
        break;

      case 'O':
      case '>':
      case '<':
        ret = handle_octave(mml, code, score, result);
        break;

      case 'V':
        ret = handle_volume(mml, score, result);
        break;

      case CHORD_START:
        ret = handle_startchord(mml, score, result);
        break;

      case TUPLET_START:
        ret = handle_starttuplet(mml, score, result);
        break;

      case TUPLET_END:
        ret = handle_stoptuplet(mml, score, result);
        break;

      case '@':
        ret = handle_tone(mml, score, result);
        break;

      case '\0':
        ret = MML_TYPE_EOF;
        *score -= 1;  /* Backward */
        break;

      default:
        ret = MML_TYPE_ILLIGAL_COMPOSITION;
        break;
    }

  return ret;
}
