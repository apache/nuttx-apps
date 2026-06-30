/****************************************************************************
 * apps/games/NXDoom/src/midifile.h
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
 *   MIDI file parsing.
 *
 ****************************************************************************/

#ifndef MIDIFILE_H
#define MIDIFILE_H

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MIDI_CHANNELS_PER_TRACK 16

#define MIDI_RPN_MSB 0x00
#define MIDI_RPN_PITCH_BEND_SENS_LSB 0x00
#define MIDI_RPN_FINE_TUNING_LSB 0x01
#define MIDI_RPN_COARSE_TUNING_LSB 0x02
#define MIDI_RPN_NULL 0x7f

#define EMIDI_LOOP_FLAG 0x7f

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef struct midi_file_s midi_file_t;
typedef struct midi_track_iter_s midi_track_iter_t;

typedef enum
{
  MIDI_EVENT_NOTE_OFF = 0x80,
  MIDI_EVENT_NOTE_ON = 0x90,
  MIDI_EVENT_AFTERTOUCH = 0xa0,
  MIDI_EVENT_CONTROLLER = 0xb0,
  MIDI_EVENT_PROGRAM_CHANGE = 0xc0,
  MIDI_EVENT_CHAN_AFTERTOUCH = 0xd0,
  MIDI_EVENT_PITCH_BEND = 0xe0,

  MIDI_EVENT_SYSEX = 0xf0,
  MIDI_EVENT_SYSEX_SPLIT = 0xf7,
  MIDI_EVENT_META = 0xff,
} midi_event_type_t;

typedef enum
{
  MIDI_CONTROLLER_BANK_SELECT_MSB = 0x00,
  MIDI_CONTROLLER_MODULATION = 0x01,
  MIDI_CONTROLLER_BREATH_CONTROL = 0x02,
  MIDI_CONTROLLER_FOOT_CONTROL = 0x04,
  MIDI_CONTROLLER_PORTAMENTO = 0x05,
  MIDI_CONTROLLER_DATA_ENTRY_MSB = 0x06,
  MIDI_CONTROLLER_VOLUME_MSB = 0x07,
  MIDI_CONTROLLER_PAN = 0x0a,
  MIDI_CONTROLLER_EXPRESSION = 0x0b,

  MIDI_CONTROLLER_BANK_SELECT_LSB = 0x20,
  MIDI_CONTROLLER_DATA_ENTRY_LSB = 0x26,
  MIDI_CONTROLLER_VOLUME_LSB = 0X27,

  MIDI_CONTROLLER_HOLD1_PEDAL = 0x40,
  MIDI_CONTROLLER_SOFT_PEDAL = 0x43,

  MIDI_CONTROLLER_REVERB = 0x5b,
  MIDI_CONTROLLER_CHORUS = 0x5d,

  MIDI_CONTROLLER_NRPN_LSB = 0x62,
  MIDI_CONTROLLER_NRPN_MSB = 0x63,
  MIDI_CONTROLLER_RPN_LSB = 0x64,
  MIDI_CONTROLLER_RPN_MSB = 0x65,

  MIDI_CONTROLLER_ALL_SOUND_OFF = 0x78,
  MIDI_CONTROLLER_RESET_ALL_CTRLS = 0x79,
  MIDI_CONTROLLER_ALL_NOTES_OFF = 0x7b,

  MIDI_CONTROLLER_POLY_MODE_OFF = 0x7e,
  MIDI_CONTROLLER_POLY_MODE_ON = 0x7f,
} midi_controller_t;

typedef enum
{
  MIDI_META_SEQUENCE_NUMBER = 0x00,

  MIDI_META_TEXT = 0x01,
  MIDI_META_COPYRIGHT = 0x02,
  MIDI_META_TRACK_NAME = 0x03,
  MIDI_META_INSTR_NAME = 0x04,
  MIDI_META_LYRICS = 0x05,
  MIDI_META_MARKER = 0x06,
  MIDI_META_CUE_POINT = 0x07,

  MIDI_META_CHANNEL_PREFIX = 0x20,
  MIDI_META_END_OF_TRACK = 0x2f,

  MIDI_META_SET_TEMPO = 0x51,
  MIDI_META_SMPTE_OFFSET = 0x54,
  MIDI_META_TIME_SIGNATURE = 0x58,
  MIDI_META_KEY_SIGNATURE = 0x59,
  MIDI_META_SEQUENCER_SPECIFIC = 0x7f,
} midi_meta_event_type_t;

typedef enum
{
  EMIDI_DEVICE_GENERAL_MIDI = 0x00,
  EMIDI_DEVICE_SOUND_CANVAS = 0x01,
  EMIDI_DEVICE_AWE32 = 0x02,
  EMIDI_DEVICE_WAVE_BLASTER = 0x03,
  EMIDI_DEVICE_SOUND_BLASTER = 0x04,
  EMIDI_DEVICE_PRO_AUDIO = 0x05,
  EMIDI_DEVICE_SOUND_MAN_16 = 0x06,
  EMIDI_DEVICE_ADLIB = 0x07,
  EMIDI_DEVICE_SOUNDSCAPE = 0x08,
  EMIDI_DEVICE_ULTRASOUND = 0x09,
  EMIDI_DEVICE_ALL = 0x7f,
} emidi_device_t;

typedef enum
{
  EMIDI_CONTROLLER_TRACK_DESIGNATION = 0x6e,
  EMIDI_CONTROLLER_TRACK_EXCLUSION = 0x6f,
  EMIDI_CONTROLLER_PROGRAM_CHANGE = 0x70,
  EMIDI_CONTROLLER_VOLUME = 0x71,
  EMIDI_CONTROLLER_LOOP_BEGIN = 0x74,
  EMIDI_CONTROLLER_LOOP_END = 0x75,
  EMIDI_CONTROLLER_GLOBAL_LOOP_BEGIN = 0x76,
  EMIDI_CONTROLLER_GLOBAL_LOOP_END = 0x77,
} emidi_controller_t;

typedef struct
{
  /* Meta event type: */

  unsigned int type;

  /* Length: */

  unsigned int length;

  /* Meta event data: */

  byte *data;
} midi_meta_event_data_t;

typedef struct
{
  /* Length: */

  unsigned int length;

  /* Event data: */

  byte *data;
} midi_sysex_event_data_t;

typedef struct
{
  /* The channel number to which this applies: */

  unsigned int channel;

  /* Extra parameters: */

  unsigned int param1;
  unsigned int param2;
} midi_channel_event_data_t;

typedef struct
{
  /* Time between the previous event and this event. */

  unsigned int delta_time;

  /* Type of event: */

  midi_event_type_t event_type;

  union
  {
    midi_channel_event_data_t channel;
    midi_meta_event_data_t meta;
    midi_sysex_event_data_t sysex;
  } data;
} midi_event_t;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* Load a MIDI file. */

midi_file_t *midi_loadfile(char *filename);

/* Free a MIDI file. */

void midi_free_file(midi_file_t *file);

/* Get the time division value from the MIDI header. */

unsigned int midi_get_file_time_division(midi_file_t *file);

/* Get the number of tracks in a MIDI file. */

unsigned int midi_num_tracks(midi_file_t *file);

/* Start iterating over the events in a track. */

midi_track_iter_t *midi_iterate_track(midi_file_t *file,
                                     unsigned int track_num);

/* Free an iterator. */

void midi_free_iterator(midi_track_iter_t *iter);

/* Get the time until the next MIDI event in a track. */

unsigned int midi_get_delta_time(midi_track_iter_t *iter);

/* Get a pointer to the next MIDI event. */

int midi_get_next_event(midi_track_iter_t *iter, midi_event_t **event);

/* Reset an iterator to the beginning of a track. */

void midi_restart_iterator(midi_track_iter_t *iter);

/* Set loop point to current position. */

void midi_set_loop_point(midi_track_iter_t *iter);

/* Set position to saved loop point. */

void midi_restart_at_loop_point(midi_track_iter_t *iter);

#endif /* MIDIFILE_H */
