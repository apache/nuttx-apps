/****************************************************************************
 * apps/games/NXDoom/src/midifile.c
 *
 * SPDX-License-Identifer: GPLv2
 *
 * Copyright(C) 2005-2014 Simon Howard
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * DESCRIPTION:
 *  Reading of MIDI files.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "doomtype.h"
#include "i_swap.h"
#include "i_system.h"
#include "m_misc.h"
#include "midifile.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define HEADER_CHUNK_ID "MThd"
#define TRACK_CHUNK_ID "MTrk"
#define MAX_BUFFER_SIZE 0x10000

/* haleyjd 09/09/10: packing required */

#ifdef _MSC_VER
#pragma pack(push, 1)
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

begin_packed_struct struct chunk_header_t
{
  byte chunk_id[4];
  unsigned int chunk_size;
} end_packed_struct;

typedef struct chunk_header_t chunk_header_t;

begin_packed_struct struct midi_header_t
{
  chunk_header_t chunk_header;
  unsigned short format_type;
  unsigned short num_tracks;
  unsigned short time_division;
} begin_packed_struct;

typedef struct midi_header_t midi_header_t;

/* haleyjd 09/09/10: packing off. */

#ifdef _MSC_VER
#pragma pack(pop)
#endif

typedef struct
{
  /* Length in bytes: */

  unsigned int data_len;

  /* Events in this track: */

  midi_event_t *events;
  int num_events;
} midi_track_t;

struct midi_track_iter_s
{
  midi_track_t *track;
  unsigned int position;
  unsigned int loop_point;
};

struct midi_file_s
{
  midi_header_t header;

  /* All tracks in this file: */

  midi_track_t *tracks;
  unsigned int num_tracks;

  /* Data buffer used to store data read for SysEx or meta events: */

  byte *buffer;
  unsigned int buffer_size;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Check the header of a chunk: */

static boolean check_chunk_header(chunk_header_t *chunk,
                                const char *expected_id)
{
  boolean result;

  result = (memcmp((char *)chunk->chunk_id, expected_id, 4) == 0);

  if (!result)
    {
      fprintf(stderr,
              "check_chunk_header: Expected '%s' chunk header, "
              "got '%c%c%c%c'\n",
              expected_id, chunk->chunk_id[0], chunk->chunk_id[1],
              chunk->chunk_id[2], chunk->chunk_id[3]);
    }

  return result;
}

/* Read a single byte. Returns false on error. */

static boolean read_byte(byte *result, FILE *stream)
{
  int c;

  c = fgetc(stream);

  if (c == EOF)
    {
      fprintf(stderr, "read_byte: Unexpected end of file\n");
      return false;
    }
  else
    {
      *result = (byte)c;

      return true;
    }
}

/* Read a variable-length value. */

static boolean read_variable_length(unsigned int *result, FILE *stream)
{
  int i;
  byte b = 0;

  *result = 0;

  for (i = 0; i < 4; ++i)
    {
      if (!read_byte(&b, stream))
        {
          fprintf(stderr, "read_variable_length: Error while reading "
                          "variable-length value\n");
          return false;
        }

      /* Insert the bottom seven bits from this byte. */

      *result <<= 7;
      *result |= b & 0x7f;

      /* If the top bit is not set, this is the end. */

      if ((b & 0x80) == 0)
        {
          return true;
        }
    }

  fprintf(stderr, "read_variable_length: Variable-length value too "
                  "long: maximum of four bytes\n");
  return false;
}

/* Read a byte sequence into the data buffer. */

static void *read_byte_sequence(unsigned int num_bytes, FILE *stream)
{
  unsigned int i;
  byte *result;

  /* Allocate a buffer. Allocate one extra byte, as malloc(0) is
   * non-portable.
   */

  result = malloc(num_bytes + 1);

  if (result == NULL)
    {
      fprintf(stderr, "read_byte_sequence: Failed to allocate buffer\n");
      return NULL;
    }

  /* Read the data: */

  for (i = 0; i < num_bytes; ++i)
    {
      if (!read_byte(&result[i], stream))
        {
          fprintf(stderr,
                  "read_byte_sequence: Error while reading byte %u\n", i);
          free(result);
          return NULL;
        }
    }

  return result;
}

/* Read a MIDI channel event.
 * two_param indicates that the event type takes two parameters
 * (three byte) otherwise it is single parameter (two byte)
 */

static boolean read_channel_event(midi_event_t *event, byte event_type,
                                boolean two_param, FILE *stream)
{
  byte b = 0;

  /* Set basics: */

  event->event_type = event_type & 0xf0;
  event->data.channel.channel = event_type & 0x0f;

  /* Read parameters: */

  if (!read_byte(&b, stream))
    {
      fprintf(stderr, "read_channel_event: Error while reading channel "
                      "event parameters\n");
      return false;
    }

  event->data.channel.param1 = b;

  /* Second parameter: */

  if (two_param)
    {
      if (!read_byte(&b, stream))
        {
          fprintf(stderr, "read_channel_event: Error while reading channel "
                          "event parameters\n");
          return false;
        }

      event->data.channel.param2 = b;
    }

  return true;
}

/* Read sysex event: */

static boolean read_sys_ex_event(midi_event_t *event, int event_type,
                              FILE *stream)
{
  event->event_type = event_type;

  if (!read_variable_length(&event->data.sysex.length, stream))
    {
      fprintf(stderr, "read_sys_ex_event: Failed to read length of "
                      "SysEx block\n");
      return false;
    }

  /* Read the byte sequence: */

  event->data.sysex.data =
      read_byte_sequence(event->data.sysex.length, stream);

  if (event->data.sysex.data == NULL)
    {
      fprintf(stderr,
              "read_sys_ex_event: Failed while reading SysEx event\n");
      return false;
    }

  return true;
}

/* Read meta event: */

static boolean read_meta_event(midi_event_t *event, FILE *stream)
{
  byte b = 0;

  event->event_type = MIDI_EVENT_META;

  /* Read meta event type: */

  if (!read_byte(&b, stream))
    {
      fprintf(stderr, "read_meta_event: Failed to read meta event type\n");
      return false;
    }

  event->data.meta.type = b;

  /* Read length of meta event data: */

  if (!read_variable_length(&event->data.meta.length, stream))
    {
      fprintf(stderr, "read_sys_ex_event: Failed to read length of "
                      "SysEx block\n");
      return false;
    }

  /* Read the byte sequence: */

  event->data.meta.data =
      read_byte_sequence(event->data.meta.length, stream);

  if (event->data.meta.data == NULL)
    {
      fprintf(stderr,
              "read_sys_ex_event: Failed while reading SysEx event\n");
      return false;
    }

  return true;
}

static boolean read_event(midi_event_t *event, unsigned int *last_event_type,
                         FILE *stream)
{
  byte event_type = 0;

  if (!read_variable_length(&event->delta_time, stream))
    {
      fprintf(stderr, "read_event: Failed to read event timestamp\n");
      return false;
    }

  if (!read_byte(&event_type, stream))
    {
      fprintf(stderr, "read_event: Failed to read event type\n");
      return false;
    }

  /* All event types have their top bit set.  Therefore, if
   * the top bit is not set, it is because we are using the "same
   * as previous event type" shortcut to save a byte.  Skip back
   * a byte so that we read this byte again.
   */

  if ((event_type & 0x80) == 0)
    {
      event_type = *last_event_type;

      if (fseek(stream, -1, SEEK_CUR) < 0)
        {
          fprintf(stderr, "read_event: Unable to seek in stream\n");
          return false;
        }
    }
  else
    {
      *last_event_type = event_type;
    }

  /* Check event type: */

  switch (event_type & 0xf0)
    {
      /* Two parameter channel events: */

    case MIDI_EVENT_NOTE_OFF:
    case MIDI_EVENT_NOTE_ON:
    case MIDI_EVENT_AFTERTOUCH:
    case MIDI_EVENT_CONTROLLER:
    case MIDI_EVENT_PITCH_BEND:
      return read_channel_event(event, event_type, true, stream);

      /* Single parameter channel events: */

    case MIDI_EVENT_PROGRAM_CHANGE:
    case MIDI_EVENT_CHAN_AFTERTOUCH:
      return read_channel_event(event, event_type, false, stream);

    default:
      break;
    }

  /* Specific value? */

  switch (event_type)
    {
    case MIDI_EVENT_SYSEX:
    case MIDI_EVENT_SYSEX_SPLIT:
      return read_sys_ex_event(event, event_type, stream);

    case MIDI_EVENT_META:
      return read_meta_event(event, stream);

    default:
      break;
    }

  fprintf(stderr, "read_event: Unknown MIDI event type: 0x%x\n", event_type);
  return false;
}

/* Free an event: */

static void free_event(midi_event_t *event)
{
  /* Some event types have dynamically allocated buffers assigned
   * to them that must be freed.
   */

  switch (event->event_type)
    {
    case MIDI_EVENT_SYSEX:
    case MIDI_EVENT_SYSEX_SPLIT:
      free(event->data.sysex.data);
      break;

    case MIDI_EVENT_META:
      free(event->data.meta.data);
      break;

    default:
      break; /* Nothing to do. */
    }
}

/* Read and check the track chunk header */

static boolean read_track_header(midi_track_t *track, FILE *stream)
{
  size_t records_read;
  chunk_header_t chunk_header;

  records_read = fread(&chunk_header, sizeof(chunk_header_t), 1, stream);

  if (records_read < 1)
    {
      return false;
    }

  if (!check_chunk_header(&chunk_header, TRACK_CHUNK_ID))
    {
      return false;
    }

  track->data_len = be32toh(chunk_header.chunk_size);

  return true;
}

static boolean read_track(midi_track_t *track, FILE *stream)
{
  midi_event_t *new_events;
  midi_event_t *event;
  unsigned int last_event_type;

  track->num_events = 0;
  track->events = NULL;

  /* Read the header: */

  if (!read_track_header(track, stream))
    {
      return false;
    }

  /* Then the events: */

  last_event_type = 0;

  for (; ; )
    {
      /* Resize the track slightly larger to hold another event: */

      new_events = i_realloc(track->events,
                             sizeof(midi_event_t) * (track->num_events + 1));
      track->events = new_events;

      /* Read the next event: */

      event = &track->events[track->num_events];
      if (!read_event(event, &last_event_type, stream))
        {
          return false;
        }

      ++track->num_events;

      /* End of track? */

      if (event->event_type == MIDI_EVENT_META &&
          event->data.meta.type == MIDI_META_END_OF_TRACK)
        {
          break;
        }
    }

  return true;
}

/* Free a track: */

static void free_track(midi_track_t *track)
{
  unsigned int i;

  for (i = 0; i < track->num_events; ++i)
    {
      free_event(&track->events[i]);
    }

  free(track->events);
}

static boolean read_all_tracks(midi_file_t *file, FILE *stream)
{
  unsigned int i;

  /* Allocate list of tracks and read each track: */

  file->tracks = malloc(sizeof(midi_track_t) * file->num_tracks);

  if (file->tracks == NULL)
    {
      return false;
    }

  memset(file->tracks, 0, sizeof(midi_track_t) * file->num_tracks);

  /* Read each track: */

  for (i = 0; i < file->num_tracks; ++i)
    {
      if (!read_track(&file->tracks[i], stream))
        {
          return false;
        }
    }

  return true;
}

/* Read and check the header chunk. */

static boolean read_file_header(midi_file_t *file, FILE *stream)
{
  size_t records_read;
  unsigned int format_type;

  records_read = fread(&file->header, sizeof(midi_header_t), 1, stream);

  if (records_read < 1)
    {
      return false;
    }

  if (!check_chunk_header(&file->header.chunk_header, HEADER_CHUNK_ID) ||
      be32toh(file->header.chunk_header.chunk_size) != 6)
    {
      fprintf(stderr,
              "read_file_header: Invalid MIDI chunk header! "
              "chunk_size=%i\n",
              be32toh(file->header.chunk_header.chunk_size));
      return false;
    }

  format_type = be32toh(file->header.format_type);
  file->num_tracks = be32toh(file->header.num_tracks);

  if ((format_type != 0 && format_type != 1) || file->num_tracks < 1)
    {
      fprintf(stderr, "read_file_header: Only type 0/1 "
                      "MIDI files supported!\n");
      return false;
    }

  return true;
}

void midi_free_file(midi_file_t *file)
{
  int i;

  if (file->tracks != NULL)
    {
      for (i = 0; i < file->num_tracks; ++i)
        {
          free_track(&file->tracks[i]);
        }

      free(file->tracks);
    }

  free(file);
}

midi_file_t *midi_loadfile(char *filename)
{
  midi_file_t *file;
  FILE *stream;

  file = malloc(sizeof(midi_file_t));

  if (file == NULL)
    {
      return NULL;
    }

  file->tracks = NULL;
  file->num_tracks = 0;
  file->buffer = NULL;
  file->buffer_size = 0;

  /* Open file */

  stream = fopen(filename, "rb");

  if (stream == NULL)
    {
      fprintf(stderr, "midi_loadfile: Failed to open '%s'\n", filename);
      midi_free_file(file);
      return NULL;
    }

  /* Read MIDI file header */

  if (!read_file_header(file, stream))
    {
      fclose(stream);
      midi_free_file(file);
      return NULL;
    }

  /* Read all tracks: */

  if (!read_all_tracks(file, stream))
    {
      fclose(stream);
      midi_free_file(file);
      return NULL;
    }

  fclose(stream);

  return file;
}

#ifdef TEST
static char *midi_event_type_to_string(midi_event_type_t event_type)
{
  switch (event_type)
    {
    case MIDI_EVENT_NOTE_OFF:
      return "MIDI_EVENT_NOTE_OFF";
    case MIDI_EVENT_NOTE_ON:
      return "MIDI_EVENT_NOTE_ON";
    case MIDI_EVENT_AFTERTOUCH:
      return "MIDI_EVENT_AFTERTOUCH";
    case MIDI_EVENT_CONTROLLER:
      return "MIDI_EVENT_CONTROLLER";
    case MIDI_EVENT_PROGRAM_CHANGE:
      return "MIDI_EVENT_PROGRAM_CHANGE";
    case MIDI_EVENT_CHAN_AFTERTOUCH:
      return "MIDI_EVENT_CHAN_AFTERTOUCH";
    case MIDI_EVENT_PITCH_BEND:
      return "MIDI_EVENT_PITCH_BEND";
    case MIDI_EVENT_SYSEX:
      return "MIDI_EVENT_SYSEX";
    case MIDI_EVENT_SYSEX_SPLIT:
      return "MIDI_EVENT_SYSEX_SPLIT";
    case MIDI_EVENT_META:
      return "MIDI_EVENT_META";

    default:
      return "(unknown)";
    }
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* Get the number of tracks in a MIDI file. */

unsigned int midi_num_tracks(midi_file_t *file)
{
  return file->num_tracks;
}

/* Start iterating over the events in a track. */

midi_track_iter_t *midi_iterate_track(midi_file_t *file, unsigned int track)
{
  midi_track_iter_t *iter;

  assert(track < file->num_tracks);

  iter = malloc(sizeof(*iter));
  iter->track = &file->tracks[track];
  iter->position = 0;
  iter->loop_point = 0;

  return iter;
}

void midi_free_iterator(midi_track_iter_t *iter)
{
  free(iter);
}

/* Get the time until the next MIDI event in a track. */

unsigned int midi_get_delta_time(midi_track_iter_t *iter)
{
  if (iter->position < iter->track->num_events)
    {
      midi_event_t *next_event;

      next_event = &iter->track->events[iter->position];

      return next_event->delta_time;
    }
  else
    {
      return 0;
    }
}

/* Get a pointer to the next MIDI event. */

int midi_get_next_event(midi_track_iter_t *iter, midi_event_t **event)
{
  if (iter->position < iter->track->num_events)
    {
      *event = &iter->track->events[iter->position];
      ++iter->position;

      return 1;
    }
  else
    {
      return 0;
    }
}

unsigned int midi_get_file_time_division(midi_file_t *file)
{
  short result = be16toh(file->header.time_division);

  /* Negative time division indicates SMPTE time and must be handled
   * differently.
   */

  if (result < 0)
    {
      return (signed int)(-(result / 256)) *
             (signed int)(result & 0xff);
    }
  else
    {
      return result;
    }
}

void midi_restart_iterator(midi_track_iter_t *iter)
{
  iter->position = 0;
  iter->loop_point = 0;
}

void midi_set_loop_point(midi_track_iter_t *iter)
{
  iter->loop_point = iter->position;
}

void midi_restart_at_loop_point(midi_track_iter_t *iter)
{
  iter->position = iter->loop_point;
}

#ifdef TEST
void print_track(midi_track_t *track)
{
  midi_event_t *event;
  unsigned int i;

  for (i = 0; i < track->num_events; ++i)
    {
      event = &track->events[i];

      if (event->delta_time > 0)
        {
          printf("Delay: %u ticks\n", event->delta_time);
        }

      printf("Event type: %s (%i)\n",
             midi_event_type_to_string(event->event_type),
             event->event_type);

      switch (event->event_type)
        {
        case MIDI_EVENT_NOTE_OFF:
        case MIDI_EVENT_NOTE_ON:
        case MIDI_EVENT_AFTERTOUCH:
        case MIDI_EVENT_CONTROLLER:
        case MIDI_EVENT_PROGRAM_CHANGE:
        case MIDI_EVENT_CHAN_AFTERTOUCH:
        case MIDI_EVENT_PITCH_BEND:
          printf("\tChannel: %u\n", event->data.channel.channel);
          printf("\tParameter 1: %u\n", event->data.channel.param1);
          printf("\tParameter 2: %u\n", event->data.channel.param2);
          break;

        case MIDI_EVENT_SYSEX:
        case MIDI_EVENT_SYSEX_SPLIT:
          printf("\tLength: %u\n", event->data.sysex.length);
          break;

        case MIDI_EVENT_META:
          printf("\tMeta type: %u\n", event->data.meta.type);
          printf("\tLength: %u\n", event->data.meta.length);
          break;
        }
    }
}

int main(int argc, char *argv[])
{
  midi_file_t *file;
  unsigned int i;

  if (argc < 2)
    {
      printf("Usage: %s <filename>\n", argv[0]);
      exit(1);
    }

  file = midi_loadfile(argv[1]);

  if (file == NULL)
    {
      fprintf(stderr, "Failed to open %s\n", argv[1]);
      exit(1);
    }

  for (i = 0; i < file->num_tracks; ++i)
    {
      printf("\n== Track %u ==\n\n", i);

      print_track(&file->tracks[i]);
    }

  return 0;
}
#endif
