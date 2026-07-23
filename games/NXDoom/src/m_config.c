/****************************************************************************
 * apps/games/NXDoom/src/m_config.c
 *
 * SPDX-License-Identifer: GPLv2
 *
 * Copyright(C) 1993-1996 Id Software, Inc.
 * Copyright(C) 1993-2008 Raven Software
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
 *   Configuration file interface.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#include "doomkeys.h"
#include "doomtype.h"
#include "i_system.h"
#include "m_argv.h"
#include "m_config.h"
#include "m_misc.h"

#include "z_zone.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CONFIG_VARIABLE_GENERIC(name, type) {#name, {NULL}, type, 0, 0, false}

#define CONFIG_VARIABLE_KEY(name) CONFIG_VARIABLE_GENERIC(name, DEFAULT_KEY)
#define CONFIG_VARIABLE_INT(name) CONFIG_VARIABLE_GENERIC(name, DEFAULT_INT)
#define CONFIG_VARIABLE_INT_HEX(name)                                        \
  CONFIG_VARIABLE_GENERIC(name, DEFAULT_INT_HEX)
#define CONFIG_VARIABLE_FLOAT(name)                                          \
  CONFIG_VARIABLE_GENERIC(name, DEFAULT_FLOAT)
#define CONFIG_VARIABLE_STRING(name)                                         \
  CONFIG_VARIABLE_GENERIC(name, DEFAULT_STRING)

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef enum
{
  DEFAULT_INT,
  DEFAULT_INT_HEX,
  DEFAULT_STRING,
  DEFAULT_FLOAT,
  DEFAULT_KEY,
} default_type_t;

typedef struct
{
  /* Name of the variable */

  const char *name;

  /* Pointer to the location in memory of the variable */

  union
  {
    int *i;
    char **s;
    float *f;
  } location;

  /* Type of the variable */

  default_type_t type;

  /* If this is a key value, the original integer scancode we read from
   * the config file before translating it to the internal key value.
   * If zero, we didn't read this value from a config file.
   */

  int untranslated;

  /* The value we translated the scancode into when we read the
   * config file on startup.  If the variable value is different from
   * this, it has been changed and needs to be converted; otherwise,
   * use the 'untranslated' value.
   */

  int original_translated;

  /* If true, this config variable has been bound to a variable
   * and is being used.
   */

  boolean bound;
} default_t;

typedef struct
{
  default_t *defaults;
  int numdefaults;
  const char *filename;
} default_collection_t;

/****************************************************************************
 * Private Data
 ****************************************************************************/

static char *autoload_path = "";

/* Default filenames for configuration files. */

static const char *default_main_config;
static const char *default_extra_config;

/* @begin_config_file default */

static default_t doom_defaults_list[] =
{
  /* Mouse sensitivity.  This value is used to multiply input mouse
   * movement to control the effect of moving the mouse.
   *
   * The "normal" maximum value available for this through the
   * in-game options menu is 9. A value of 31 or greater will cause
   * the game to crash when entering the options menu.
   *
   */

  CONFIG_VARIABLE_INT(mouse_sensitivity),

  /* Volume of sound effects, range 0-15. */

  CONFIG_VARIABLE_INT(sfx_volume),

  /* Volume of in-game music, range 0-15. */

  CONFIG_VARIABLE_INT(music_volume),

  /* @game strife
   *
   * If non-zero, dialogue text is displayed over characters' pictures
   * when engaging actors who have voices.
   */

  CONFIG_VARIABLE_INT(show_talk),

  /* @game strife
   *
   * Volume of voice sound effects, range 0-15.
   */

  CONFIG_VARIABLE_INT(voice_volume),

  /* @game doom
   *
   * If non-zero, messages are displayed on the heads-up display
   * in the game ("picked up a clip", etc).  If zero, these messages
   * are not displayed.
   */

  CONFIG_VARIABLE_INT(show_messages),

  /* Keyboard key to turn right. */

  CONFIG_VARIABLE_KEY(key_right),

  /* Keyboard key to turn left. */

  CONFIG_VARIABLE_KEY(key_left),

  /* Keyboard key to move forward. */

  CONFIG_VARIABLE_KEY(key_up),

  /* Keyboard key to move backward. */

  CONFIG_VARIABLE_KEY(key_down),

  /* Keyboard key to strafe left. */

  CONFIG_VARIABLE_KEY(key_strafeleft),

  /* Keyboard key to strafe right. */

  CONFIG_VARIABLE_KEY(key_straferight),

  /* @game strife
   *
   * Keyboard key to use health.
   */

  CONFIG_VARIABLE_KEY(key_use_health),

  /* @game hexen
   *
   * Keyboard key to jump.
   */

  CONFIG_VARIABLE_KEY(key_jump),

  /* @game heretic hexen
   *
   * Keyboard key to fly upward.
   */

  CONFIG_VARIABLE_KEY(key_flyup),

  /* @game heretic hexen
   *
   * Keyboard key to fly downwards.
   */

  CONFIG_VARIABLE_KEY(key_flydown),

  /* @game heretic hexen
   *
   * Keyboard key to center flying.
   */

  CONFIG_VARIABLE_KEY(key_flycenter),

  /* @game heretic hexen
   *
   * Keyboard key to look up.
   */

  CONFIG_VARIABLE_KEY(key_lookup),

  /* @game heretic hexen
   *
   * Keyboard key to look down.
   */

  CONFIG_VARIABLE_KEY(key_lookdown),

  /* @game heretic hexen
   *
   * Keyboard key to center the view.
   */

  CONFIG_VARIABLE_KEY(key_lookcenter),

  /* @game strife
   *
   * Keyboard key to query inventory.
   */

  CONFIG_VARIABLE_KEY(key_invquery),

  /* @game strife
   *
   * Keyboard key to display mission objective.
   */

  CONFIG_VARIABLE_KEY(key_mission),

  /* @game strife
   *
   * Keyboard key to display inventory popup.
   */

  CONFIG_VARIABLE_KEY(key_inv_pop),

  /* @game strife
   *
   * Keyboard key to display keys popup.
   */

  CONFIG_VARIABLE_KEY(key_inv_key),

  /* @game strife
   *
   * Keyboard key to jump to start of inventory.
   */

  CONFIG_VARIABLE_KEY(key_inv_home),

  /* @game strife
   *
   * Keyboard key to jump to end of inventory.
   */

  CONFIG_VARIABLE_KEY(key_inv_end),

  /* @game heretic hexen
   *
   * Keyboard key to scroll left in the inventory.
   */

  CONFIG_VARIABLE_KEY(key_invleft),

  /* @game heretic hexen
   *
   * Keyboard key to scroll right in the inventory.
   */

  CONFIG_VARIABLE_KEY(key_invright),

  /* @game strife
   *
   * Keyboard key to scroll left in the inventory.
   */

  CONFIG_VARIABLE_KEY(key_inv_left),

  /* @game strife
   *
   * Keyboard key to scroll right in the inventory.
   */

  CONFIG_VARIABLE_KEY(key_inv_right),

  /* @game heretic hexen
   *
   * Keyboard key to use the current item in the inventory.
   */

  CONFIG_VARIABLE_KEY(key_useartifact),

  /* @game strife
   *
   * Keyboard key to use inventory item.
   */

  CONFIG_VARIABLE_KEY(key_inv_use),

  /* @game strife
   *
   * Keyboard key to drop an inventory item.
   */

  CONFIG_VARIABLE_KEY(key_inv_drop),

  /* @game strife
   *
   * Keyboard key to look up.
   */

  CONFIG_VARIABLE_KEY(key_look_up),

  /* @game strife
   *
   * Keyboard key to look down.
   */

  CONFIG_VARIABLE_KEY(key_look_down),

  /* Keyboard key to fire the currently selected weapon. */

  CONFIG_VARIABLE_KEY(key_fire),

  /* Keyboard key to "use" an object, eg. a door or switch. */

  CONFIG_VARIABLE_KEY(key_use),

  /* Keyboard key to turn on strafing.  When held down, pressing the
   * key to turn left or right causes the player to strafe left or
   * right instead.
   */

  CONFIG_VARIABLE_KEY(key_strafe),

  /* Keyboard key to make the player run. */

  CONFIG_VARIABLE_KEY(key_speed),

  /* If non-zero, mouse input is enabled.  If zero, mouse input is
   * disabled.
   */

  CONFIG_VARIABLE_INT(use_mouse),

  /* Mouse button to fire the currently selected weapon. */

  CONFIG_VARIABLE_INT(mouseb_fire),

  /* Mouse button to turn on strafing.  When held down, the player
   * will strafe left and right instead of turning left and right.
   */

  CONFIG_VARIABLE_INT(mouseb_strafe),

  /* Mouse button to move forward. */

  CONFIG_VARIABLE_INT(mouseb_forward),

  /* Mouse button to turn on running.  When held down, the player
   * will run while moving.
   */

  CONFIG_VARIABLE_INT(mouseb_speed),

  /* @game hexen strife
   *
   * Mouse button to jump.
   */

  CONFIG_VARIABLE_INT(mouseb_jump),

  /* If non-zero, joystick input is enabled. */

  CONFIG_VARIABLE_INT(use_joystick),

  /* Joystick virtual button that fires the current weapon. */

  CONFIG_VARIABLE_INT(joyb_fire),

  /* Joystick virtual button that makes the player strafe while
   * held down.
   */

  CONFIG_VARIABLE_INT(joyb_strafe),

  /* Joystick virtual button to "use" an object, eg. a door or switch. */

  CONFIG_VARIABLE_INT(joyb_use),

  /* Joystick virtual button that makes the player run while held
   * down.
   *
   * If this has a value of 20 or greater, the player will always run,
   * even if use_joystick is 0.
   */

  CONFIG_VARIABLE_INT(joyb_speed),

  /* @game hexen strife
   *
   * Joystick virtual button that makes the player jump.
   */

  CONFIG_VARIABLE_INT(joyb_jump),

  /* @game doom heretic hexen
   *
   * Screen size, range 3-11.
   *
   * A value of 11 gives a full-screen view with the status bar not
   * displayed.  A value of 10 gives a full-screen view with the
   * status bar displayed.
   */

  CONFIG_VARIABLE_INT(screenblocks),

  /* @game strife
   *
   * Screen size, range 3-11.
   *
   * A value of 11 gives a full-screen view with the status bar not
   * displayed.  A value of 10 gives a full-screen view with the
   * status bar displayed.
   */

  CONFIG_VARIABLE_INT(screensize),

  /* @game doom
   *
   * Screen detail.  Zero gives normal "high detail" mode, while
   * a non-zero value gives "low detail" mode.
   */

  CONFIG_VARIABLE_INT(detaillevel),

  /* Number of sounds that will be played simultaneously. */

  CONFIG_VARIABLE_INT(snd_channels),

  /* Music output device.  A non-zero value gives MIDI sound output,
   * while a value of zero disables music.
   */

  CONFIG_VARIABLE_INT(snd_musicdevice),

  /* Sound effects device.  A value of zero disables in-game sound
   * effects, a value of 1 enables PC speaker sound effects, while
   * a value in the range 2-9 enables the "normal" digital sound
   * effects.
   */

  CONFIG_VARIABLE_INT(snd_sfxdevice),

  /* SoundBlaster I/O port. Unused. */

  CONFIG_VARIABLE_INT(snd_sbport),

  /* SoundBlaster IRQ.  Unused. */

  CONFIG_VARIABLE_INT(snd_sbirq),

  /* SoundBlaster DMA channel.  Unused. */

  CONFIG_VARIABLE_INT(snd_sbdma),

  /* Output port to use for OPL MIDI playback.  Unused. */

  CONFIG_VARIABLE_INT(snd_mport),

  /* Gamma correction level.  A value of zero disables gamma
   * correction, while a value in the range 1-4 gives increasing
   * levels of gamma correction.
   */

  CONFIG_VARIABLE_INT(usegamma),

  /* @game hexen
   *
   * Directory in which to store savegames.
   */

  CONFIG_VARIABLE_STRING(savedir),

  /* @game hexen
   *
   * Controls whether messages are displayed in the heads-up display.
   * If this has a non-zero value, messages are displayed.
   */

  CONFIG_VARIABLE_INT(messageson),

  /* @game strife
   *
   * Name of background flat used by view border.
   */

  CONFIG_VARIABLE_STRING(back_flat),

  /* @game strife
   *
   * Multiplayer nickname (?).
   */

  CONFIG_VARIABLE_STRING(nickname),

  /* Multiplayer chat macro: message to send when alt+0 is pressed. */

  CONFIG_VARIABLE_STRING(chatmacro0),

  /* Multiplayer chat macro: message to send when alt+1 is pressed. */

  CONFIG_VARIABLE_STRING(chatmacro1),

  /* Multiplayer chat macro: message to send when alt+2 is pressed. */

  CONFIG_VARIABLE_STRING(chatmacro2),

  /* Multiplayer chat macro: message to send when alt+3 is pressed. */

  CONFIG_VARIABLE_STRING(chatmacro3),

  /* Multiplayer chat macro: message to send when alt+4 is pressed. */

  CONFIG_VARIABLE_STRING(chatmacro4),

  /* Multiplayer chat macro: message to send when alt+5 is pressed. */

  CONFIG_VARIABLE_STRING(chatmacro5),

  /* Multiplayer chat macro: message to send when alt+6 is pressed. */

  CONFIG_VARIABLE_STRING(chatmacro6),

  /* Multiplayer chat macro: message to send when alt+7 is pressed. */

  CONFIG_VARIABLE_STRING(chatmacro7),

  /* Multiplayer chat macro: message to send when alt+8 is pressed. */

  CONFIG_VARIABLE_STRING(chatmacro8),

  /* Multiplayer chat macro: message to send when alt+9 is pressed. */

  CONFIG_VARIABLE_STRING(chatmacro9),

  /* @game strife
   *
   * Serial port number to use for SERSETUP.EXE (unused).
   */

  CONFIG_VARIABLE_INT(comport),
};

static default_collection_t doom_defaults =
{
    doom_defaults_list,
    arrlen(doom_defaults_list),
    NULL,
};

/* @begin_config_file extended */

static default_t extra_defaults_list[] =
{
  /* Name of the SDL video driver to use.  If this is an empty string,
   * the default video driver is used.
   */

  CONFIG_VARIABLE_STRING(video_driver),

  /* Position of the window on the screen when running in windowed
   * mode. Accepted values are: "" (empty string) - don't care,
   * "center" - place window at center of screen, "x,y" - place
   * window at the specified coordinates.
   */

  CONFIG_VARIABLE_STRING(window_position),

  /* If non-zero, the game will run in full screen mode.  If zero,
   * the game will run in a window.
   */

  CONFIG_VARIABLE_INT(fullscreen),

  /* Index of the display on which the game should run. This has no
   * effect if running in windowed mode (fullscreen=0) and
   * window_position is not set to "center".
   */

  CONFIG_VARIABLE_INT(video_display),

  /* If non-zero, the screen will be stretched vertically to display
   * correctly on a square pixel video mode.
   */

  CONFIG_VARIABLE_INT(aspect_ratio_correct),

  /* If non-zero, the screen will have smooth scaling. */

  CONFIG_VARIABLE_INT(smooth_pixel_scaling),

  /* If non-zero, forces integer scales for resolution-independent
   * rendering.
   */

  CONFIG_VARIABLE_INT(integer_scaling),

  /* If non-zero, any pillar/letter boxes drawn around the game area
   * will "flash" when the game palette changes, simulating the VGA
   * "porch"
   */

  CONFIG_VARIABLE_INT(vga_porch_flash),

  /* Window width when running in windowed mode. */

  CONFIG_VARIABLE_INT(window_width),

  /* Window height when running in windowed mode. */

  CONFIG_VARIABLE_INT(window_height),

  /* Width for screen mode when running fullscreen.
   * If this and fullscreen_height are both set to zero, we run
   * fullscreen as a desktop window that covers the entire screen,
   * rather than ever switching screen modes. It should usually
   * be unnecessary to set this value.
   */

  CONFIG_VARIABLE_INT(fullscreen_width),

  /* Height for screen mode when running fullscreen.
   * See documentation for fullscreen_width.
   */

  CONFIG_VARIABLE_INT(fullscreen_height),

  /* If non-zero, force the use of a software renderer. For use on
   * systems lacking hardware acceleration.
   */

  CONFIG_VARIABLE_INT(force_software_renderer),

  /* Maximum number of pixels to use for intermediate scaling buffer.
   * More pixels mean that the screen can be rendered more precisely,
   * but there are diminishing returns on quality. The default limits to
   * 16,000,000 pixels, which is enough to cover 4K monitor standards.
   */

  CONFIG_VARIABLE_INT(max_scaling_buffer_pixels),

  /* Number of milliseconds to wait on startup after the video mode
   * has been set, before the game will start.  This allows the
   * screen to settle on some monitors that do not display an image
   * for a brief interval after changing video modes.
   */

  CONFIG_VARIABLE_INT(startup_delay),

  /* @game heretic hexen strife
   *
   * If non-zero, display the graphical startup screen.
   */

  CONFIG_VARIABLE_INT(graphical_startup),

  /* @game doom heretic strife
   *
   * If non-zero, the ENDOOM text screen is displayed when exiting the
   * game. If zero, the ENDOOM screen is not displayed.
   */

  CONFIG_VARIABLE_INT(show_endoom),

  /* @game doom strife
   *
   * If non-zero, a disk activity indicator is displayed when data is read
   * from disk. If zero, the disk activity indicator is not displayed.
   */

  CONFIG_VARIABLE_INT(show_diskicon),

  /* If non-zero, save screenshots in PNG format. If zero, screenshots are
   * saved in PCX format, as Vanilla Doom does.
   */

  CONFIG_VARIABLE_INT(png_screenshots),

  /* Sound output sample rate, in Hz.  Typical values to use are
   * 11025, 22050, 44100 and 48000.
   */

  CONFIG_VARIABLE_INT(snd_samplerate),

  /* Maximum number of bytes to allocate for caching converted sound
   * effects in memory. If set to zero, there is no limit applied.
   */

  CONFIG_VARIABLE_INT(snd_cachesize),

  /* Maximum size of the output sound buffer size in milliseconds.
   * Sound output is generated periodically in slices. Higher values
   * might be more efficient but will introduce latency to the
   * sound output. The default is 28ms (one slice per tic with the
   * 35fps timer).
   */

  CONFIG_VARIABLE_INT(snd_maxslicetime_ms),

  /* If non-zero, sound effects will have their pitch varied up or
   * down by a random amount during play. If zero, sound effects
   * play back at their default pitch.
   */

  CONFIG_VARIABLE_INT(snd_pitchshift),

  /* External command to invoke to perform MIDI playback. If set to
   * the empty string, SDL_mixer's internal MIDI playback is used.
   * This only has any effect when snd_musicdevice is set to General
   * MIDI output.
   */

  CONFIG_VARIABLE_STRING(snd_musiccmd),

  /* Controls whether libsamplerate support is used for performing
   * sample rate conversions of sound effects.  Support for this
   * must be compiled into the program.
   *
   * If zero, libsamplerate support is disabled.  If non-zero,
   * libsamplerate is enabled. Increasing values roughly correspond
   * to higher quality conversion; the higher the quality, the
   * slower the conversion process.  Linear conversion = 1;
   * Zero order hold = 2; Fast Sinc filter = 3; Medium quality
   * Sinc filter = 4; High quality Sinc filter = 5.
   */

  CONFIG_VARIABLE_INT(use_libsamplerate),

  /* Scaling factor used by libsamplerate. This is used when converting
   * sounds internally back into integer form; normally it should not
   * be necessary to change it from the default value. The only time
   * it might be needed is if a PWAD file is loaded that contains very
   * loud sounds, in which case the conversion may cause sound clipping
   * and the scale factor should be reduced. The lower the value, the
   * quieter the sound effects become, so it should be set as high as is
   * possible without clipping occurring.
   */

  CONFIG_VARIABLE_FLOAT(libsamplerate_scale),

  /* Full path to a directory in which WAD files and dehacked patches
   * can be placed to be automatically loaded on startup. A subdirectory
   * of this directory matching the IWAD name is checked to find the
   * files to load.
   */

  CONFIG_VARIABLE_STRING(autoload_path),

  /* Full path to a directory containing configuration files for
   * substitute music packs. These packs contain high quality renderings
   * of game music to be played instead of using the system's built-in
   * MIDI playback.
   */

  CONFIG_VARIABLE_STRING(music_pack_path),

#ifdef HAVE_FLUIDSYNTH
  /* If 1, activate the FluidSynth chorus effects module. If 0, no chorus
   * will be added to the output signal.
   */

  CONFIG_VARIABLE_INT(fsynth_chorus_active),

  /* Specifies the modulation depth of the FluidSynth chorus. Default is
   * 5.0, range is 0.0 to 256.0.
   */

  CONFIG_VARIABLE_FLOAT(fsynth_chorus_depth),

  /* Specifies the output amplitude of the FluidSynth chorus signal. Default
   * is 0.35, range is 0.0 to 10.0.
   */

  CONFIG_VARIABLE_FLOAT(fsynth_chorus_level),

  /* Sets the voice count of the FluidSynth chorus signal. Default is 3,
   * range is 0 to 99.
   */

  CONFIG_VARIABLE_INT(fsynth_chorus_nr),

  /* Sets the FluidSynth chorus modulation speed in Hz. Default is 0.3,
   * range is 0.1 to 5.0.
   */

  CONFIG_VARIABLE_FLOAT(fsynth_chorus_speed),

  /* This setting defines how FluidSynth interprets Bank Select messages.
   * The default is "gs". Other possible values are "gm", "xg" and "mma".
   */

  CONFIG_VARIABLE_STRING(fsynth_midibankselect),

  /* Sets the number of FluidSynth voices that can be played in parallel.
   * Default is 256, range is 1 - 65535.
   */

  CONFIG_VARIABLE_INT(fsynth_polyphony),

  /* If 1, activate the FluidSynth reverb effects module. If 0, no reverb
   * will be added to the output signal.
   */

  CONFIG_VARIABLE_INT(fsynth_reverb_active),

  /* Sets the amount of FluidSynth reverb damping. Default is 0.4, range is
   * 0.0 to 1.0.
   */

  CONFIG_VARIABLE_FLOAT(fsynth_reverb_damp),

  /* Sets the FluidSynth reverb amplitude. Default is 0.15, range is 0.0 -
   * 1.0.
   */

  CONFIG_VARIABLE_FLOAT(fsynth_reverb_level),

  /* Sets the room size(i.e. amount of wet) FluidSynth reverb. Default is
   * 0.6, range is 0.0 - 1.0.
   */

  CONFIG_VARIABLE_FLOAT(fsynth_reverb_roomsize),

  /* Sets the stereo spread of the FluidSynth reverb signal. Default is
   * 0.4, range is 0.0 - 100.0.
   */

  CONFIG_VARIABLE_FLOAT(fsynth_reverb_width),

  /* Fine tune the FluidSynth output level. Default is 1.0,
   * range is 0.0 - 10.0.
   */

  CONFIG_VARIABLE_FLOAT(fsynth_gain),

  /* Full path to a soundfont file to use with FluidSynth MIDI playback. */

  CONFIG_VARIABLE_STRING(fsynth_sf_path),
#endif /* HAVE_FLUIDSYNTH */

  /* Full path to a Timidity configuration file to use for MIDI
   * playback. The file will be evaluated from the directory where
   * it is evaluated, so there is no need to add "dir" commands
   * into it.
   */

  CONFIG_VARIABLE_STRING(timidity_cfg_path),

  /* Path to GUS patch files to use when operating in GUS emulation
   * mode.
   */

  CONFIG_VARIABLE_STRING(gus_patch_path),

  /* Number of kilobytes of RAM to use in GUS emulation mode. Valid
   * values are 256, 512, 768 or 1024.
   */

  CONFIG_VARIABLE_INT(gus_ram_kb),

  /* @game doom strife
   *
   * If non-zero, the Vanilla savegame limit is enforced; if the
   * savegame exceeds 180224 bytes in size, the game will exit with
   * an error.  If this has a value of zero, there is no limit to
   * the size of savegames.
   */

  CONFIG_VARIABLE_INT(vanilla_savegame_limit),

  /* @game doom strife
   *
   * If non-zero, the Vanilla demo size limit is enforced; the game
   * exits with an error when a demo exceeds the demo size limit
   * (128KiB by default).  If this has a value of zero, there is no
   * limit to the size of demos.
   */

  CONFIG_VARIABLE_INT(vanilla_demo_limit),

  /* If non-zero, the game behaves like Vanilla Doom, always assuming
   * an American keyboard mapping.  If this has a value of zero, the
   * native keyboard mapping of the keyboard is used.
   */

  CONFIG_VARIABLE_INT(vanilla_keyboard_mapping),

  /* Name to use in network games for identification.  This is only
   * used on the "waiting" screen while waiting for the game to start.
   */

  CONFIG_VARIABLE_STRING(player_name),

  /* If this is non-zero, the mouse will be "grabbed" when running
   * in windowed mode so that it can be used as an input device.
   * When running full screen, this has no effect.
   */

  CONFIG_VARIABLE_INT(grabmouse),

  /* If non-zero, all vertical mouse movement is ignored.  This
   * emulates the behavior of the "novert" tool available under DOS
   * that performs the same function.
   */

  CONFIG_VARIABLE_INT(novert),

  /* Mouse acceleration factor.  When the speed of mouse movement
   * exceeds the threshold value (mouse_threshold), the speed is
   * multiplied by this value.
   */

  CONFIG_VARIABLE_FLOAT(mouse_acceleration),

  /* Mouse acceleration threshold.  When the speed of mouse movement
   * exceeds this threshold value, the speed is multiplied by an
   * acceleration factor (mouse_acceleration).
   */

  CONFIG_VARIABLE_INT(mouse_threshold),

  /* Mouse button to strafe left. */

  CONFIG_VARIABLE_INT(mouseb_strafeleft),

  /* Mouse button to strafe right. */

  CONFIG_VARIABLE_INT(mouseb_straferight),

  /* Mouse button to turn left. */

  CONFIG_VARIABLE_INT(mouseb_turnleft),

  /* Mouse button to turn right. */

  CONFIG_VARIABLE_INT(mouseb_turnright),

  /* Mouse button to "use" an object, eg. a door or switch. */

  CONFIG_VARIABLE_INT(mouseb_use),

  /* Mouse button to move backwards. */

  CONFIG_VARIABLE_INT(mouseb_backward),

  /* Mouse button to cycle to the previous weapon. */

  CONFIG_VARIABLE_INT(mouseb_prevweapon),

  /* Mouse button to cycle to the next weapon. */

  CONFIG_VARIABLE_INT(mouseb_nextweapon),

  /* @game heretic
   *
   * Mouse button to move to the left in the inventory.
   */

  CONFIG_VARIABLE_INT(mouseb_invleft),

  /* @game heretic
   *
   * Mouse button to move to the right in the inventory.
   */

  CONFIG_VARIABLE_INT(mouseb_invright),

  /* @game heretic hexen
   *
   * Mouse button to use artifact.
   */

  CONFIG_VARIABLE_INT(mouseb_useartifact),

  /* If non-zero, double-clicking a mouse button acts like pressing
   * the "use" key to use an object in-game, eg. a door or switch.
   */

  CONFIG_VARIABLE_INT(dclick_use),

  /* SDL GUID string indicating the joystick to use. An empty string
   * indicates that no joystick is configured.
   */

  CONFIG_VARIABLE_STRING(joystick_guid),

  /* Index of SDL joystick to use; this is only used in the case where
   * multiple identical joystick devices are connected which have the
   * same GUID, to distinguish between devices.
   */

  CONFIG_VARIABLE_INT(joystick_index),

  /* If non-zero, use analog movement when playing with a gamepad. */

  CONFIG_VARIABLE_INT(use_analog),

  /* Joystick axis to use to for horizontal (X) movement. */

  CONFIG_VARIABLE_INT(joystick_x_axis),

  /* If non-zero, movement on the horizontal joystick axis is inverted. */

  CONFIG_VARIABLE_INT(joystick_x_invert),

  /* Joystick turn analog sensitivity, specified as a value between 0
   * and 20.
   */

  CONFIG_VARIABLE_INT(joystick_turn_sensitivity),

  /* Joystick axis to use to for vertical (Y) movement. */

  CONFIG_VARIABLE_INT(joystick_y_axis),

  /* If non-zero, movement on the vertical joystick axis is inverted. */

  CONFIG_VARIABLE_INT(joystick_y_invert),

  /* Joystick axis to use to for strafing movement. */

  CONFIG_VARIABLE_INT(joystick_strafe_axis),

  /* If non-zero, movement on the joystick axis used for strafing
   * is inverted.
   */

  CONFIG_VARIABLE_INT(joystick_strafe_invert),

  /* Joystick move and strafe analog sensitivity, specified as a value
   * between 0 and 20.
   */

  CONFIG_VARIABLE_INT(joystick_move_sensitivity),

  /* Joystick axis to use to for looking up and down. */

  CONFIG_VARIABLE_INT(joystick_look_axis),

  /* If non-zero, movement on the joystick axis used for looking
   * is inverted.
   */

  CONFIG_VARIABLE_INT(joystick_look_invert),

  /* Joystick look analog sensitivity, specified as a value between 0
   * and 20.
   */

  CONFIG_VARIABLE_INT(joystick_look_sensitivity),

  /* The physical joystick button that corresponds to joystick
   * virtual button #0.
   */

  CONFIG_VARIABLE_INT(joystick_physical_button0),

  /* The physical joystick button that corresponds to joystick
   * virtual button #1.
   */

  CONFIG_VARIABLE_INT(joystick_physical_button1),

  /* The physical joystick button that corresponds to joystick
   * virtual button #2.
   */

  CONFIG_VARIABLE_INT(joystick_physical_button2),

  /* The physical joystick button that corresponds to joystick
   * virtual button #3.
   */

  CONFIG_VARIABLE_INT(joystick_physical_button3),

  /* The physical joystick button that corresponds to joystick
   * virtual button #4.
   */

  CONFIG_VARIABLE_INT(joystick_physical_button4),

  /* The physical joystick button that corresponds to joystick
   * virtual button #5.
   */

  CONFIG_VARIABLE_INT(joystick_physical_button5),

  /* The physical joystick button that corresponds to joystick
   * virtual button #6.
   */

  CONFIG_VARIABLE_INT(joystick_physical_button6),

  /* The physical joystick button that corresponds to joystick
   * virtual button #7.
   */

  CONFIG_VARIABLE_INT(joystick_physical_button7),

  /* The physical joystick button that corresponds to joystick
   * virtual button #8.
   */

  CONFIG_VARIABLE_INT(joystick_physical_button8),

  /* The physical joystick button that corresponds to joystick
   * virtual button #9.
   */

  CONFIG_VARIABLE_INT(joystick_physical_button9),

  /* The physical joystick button that corresponds to joystick
   * virtual button #10.
   */

  CONFIG_VARIABLE_INT(joystick_physical_button10),

  /* The physical joystick button that corresponds to joystick
   * virtual button #11.
   */

  CONFIG_VARIABLE_INT(joystick_physical_button11),

  /* The physical joystick button that corresponds to joystick
   * virtual button #12.
   */

  CONFIG_VARIABLE_INT(joystick_physical_button12),

  /* The physical joystick button that corresponds to joystick
   * virtual button #13.
   */

  CONFIG_VARIABLE_INT(joystick_physical_button13),

  /* The physical joystick button that corresponds to joystick
   * virtual button #14.
   */

  CONFIG_VARIABLE_INT(joystick_physical_button14),

  /* The physical joystick button that corresponds to joystick
   * virtual button #15.
   */

  CONFIG_VARIABLE_INT(joystick_physical_button15),

  /* The physical joystick button that corresponds to joystick
   * virtual button #16.
   */

  CONFIG_VARIABLE_INT(joystick_physical_button16),

  /* If non-zero, use the SDL_GameController interface instead of the
   * SDL_Joystick interface.
   */

  CONFIG_VARIABLE_INT(use_gamepad),

  /* Stores the SDL_GameControllerType of the last configured gamepad. */

  CONFIG_VARIABLE_INT(gamepad_type),

  /* Joystick x axis dead zone, specified as a percentage of the axis max
   * value.
   */

  CONFIG_VARIABLE_INT(joystick_x_dead_zone),

  /* Joystick y axis dead zone, specified as a percentage of the axis max
   * value.
   */

  CONFIG_VARIABLE_INT(joystick_y_dead_zone),

  /* Joystick strafe axis dead zone, specified as a percentage of the axis
   * max value.
   */

  CONFIG_VARIABLE_INT(joystick_strafe_dead_zone),

  /* Joystick look axis dead zone, specified as a percentage of the axis max
   * value.
   */

  CONFIG_VARIABLE_INT(joystick_look_dead_zone),

  /* Joystick virtual button to make the player strafe left. */

  CONFIG_VARIABLE_INT(joyb_strafeleft),

  /* Joystick virtual button to make the player strafe right. */

  CONFIG_VARIABLE_INT(joyb_straferight),

  /* Joystick virtual button to activate the menu. */

  CONFIG_VARIABLE_INT(joyb_menu_activate),

  /* Joystick virtual button to toggle the automap. */

  CONFIG_VARIABLE_INT(joyb_toggle_automap),

  /* Joystick virtual button that cycles to the previous weapon. */

  CONFIG_VARIABLE_INT(joyb_prevweapon),

  /* Joystick virtual button that cycles to the next weapon. */

  CONFIG_VARIABLE_INT(joyb_nextweapon),

  /* @game heretic hexen
   * Joystick virtual button to activate artifact.
   */

  CONFIG_VARIABLE_INT(joyb_useartifact),

  /* @game heretic hexen
   * Joystick virtual button to move left in the inventory.
   */

  CONFIG_VARIABLE_INT(joyb_invleft),

  /* @game heretic hexen
   * Joystick virtual button to move right in the inventory.
   */

  CONFIG_VARIABLE_INT(joyb_invright),

  /* @game heretic hexen
   * Joystick virtual button to fly up.
   */

  CONFIG_VARIABLE_INT(joyb_flyup),

  /* @game heretic hexen
   * Joystick virtual button to fly down.
   */

  CONFIG_VARIABLE_INT(joyb_flydown),

  /* @game heretic hexen
   * Joystick virtual button to center flying.
   */

  CONFIG_VARIABLE_INT(joyb_flycenter),

  /* Key to pause or unpause the game. */

  CONFIG_VARIABLE_KEY(key_pause),

  /* Key that activates the menu when pressed. */

  CONFIG_VARIABLE_KEY(key_menu_activate),

  /* Key that moves the cursor up on the menu. */

  CONFIG_VARIABLE_KEY(key_menu_up),

  /* Key that moves the cursor down on the menu. */

  CONFIG_VARIABLE_KEY(key_menu_down),

  /* Key that moves the currently selected slider on the menu left. */

  CONFIG_VARIABLE_KEY(key_menu_left),

  /* Key that moves the currently selected slider on the menu right. */

  CONFIG_VARIABLE_KEY(key_menu_right),

  /* Key to go back to the previous menu. */

  CONFIG_VARIABLE_KEY(key_menu_back),

  /* Key to activate the currently selected menu item. */

  CONFIG_VARIABLE_KEY(key_menu_forward),

  /* Key to answer 'yes' to a question in the menu. */

  CONFIG_VARIABLE_KEY(key_menu_confirm),

  /* Key to answer 'no' to a question in the menu. */

  CONFIG_VARIABLE_KEY(key_menu_abort),

  /* Keyboard shortcut to bring up the help screen. */

  CONFIG_VARIABLE_KEY(key_menu_help),

  /* Keyboard shortcut to bring up the save game menu. */

  CONFIG_VARIABLE_KEY(key_menu_save),

  /* Keyboard shortcut to bring up the load game menu. */

  CONFIG_VARIABLE_KEY(key_menu_load),

  /* Keyboard shortcut to bring up the sound volume menu. */

  CONFIG_VARIABLE_KEY(key_menu_volume),

  /* Keyboard shortcut to toggle the detail level. */

  CONFIG_VARIABLE_KEY(key_menu_detail),

  /* Keyboard shortcut to quicksave the current game. */

  CONFIG_VARIABLE_KEY(key_menu_qsave),

  /* Keyboard shortcut to end the game. */

  CONFIG_VARIABLE_KEY(key_menu_endgame),

  /* Keyboard shortcut to toggle heads-up messages. */

  CONFIG_VARIABLE_KEY(key_menu_messages),

  /* Keyboard shortcut to load the last quicksave. */

  CONFIG_VARIABLE_KEY(key_menu_qload),

  /* Keyboard shortcut to quit the game. */

  CONFIG_VARIABLE_KEY(key_menu_quit),

  /* Keyboard shortcut to toggle the gamma correction level. */

  CONFIG_VARIABLE_KEY(key_menu_gamma),

  /* Keyboard shortcut to switch view in multiplayer. */

  CONFIG_VARIABLE_KEY(key_spy),

  /* Keyboard shortcut to increase the screen size. */

  CONFIG_VARIABLE_KEY(key_menu_incscreen),

  /* Keyboard shortcut to decrease the screen size. */

  CONFIG_VARIABLE_KEY(key_menu_decscreen),

  /* Keyboard shortcut to save a screenshot. */

  CONFIG_VARIABLE_KEY(key_menu_screenshot),

  /* Key to toggle the map view. */

  CONFIG_VARIABLE_KEY(key_map_toggle),

  /* Key to pan north when in the map view. */

  CONFIG_VARIABLE_KEY(key_map_north),

  /* Key to pan south when in the map view. */

  CONFIG_VARIABLE_KEY(key_map_south),

  /* Key to pan east when in the map view. */

  CONFIG_VARIABLE_KEY(key_map_east),

  /* Key to pan west when in the map view. */

  CONFIG_VARIABLE_KEY(key_map_west),

  /* Key to zoom in when in the map view. */

  CONFIG_VARIABLE_KEY(key_map_zoomin),

  /* Key to zoom out when in the map view. */

  CONFIG_VARIABLE_KEY(key_map_zoomout),

  /* Key to zoom out the maximum amount when in the map view. */

  CONFIG_VARIABLE_KEY(key_map_maxzoom),

  /* Key to toggle follow mode when in the map view. */

  CONFIG_VARIABLE_KEY(key_map_follow),

  /* Key to toggle the grid display when in the map view. */

  CONFIG_VARIABLE_KEY(key_map_grid),

  /* Key to set a mark when in the map view. */

  CONFIG_VARIABLE_KEY(key_map_mark),

  /* Key to clear all marks when in the map view. */

  CONFIG_VARIABLE_KEY(key_map_clearmark),

  /* Key to select weapon 1. */

  CONFIG_VARIABLE_KEY(key_weapon1),

  /* Key to select weapon 2. */

  CONFIG_VARIABLE_KEY(key_weapon2),

  /* Key to select weapon 3. */

  CONFIG_VARIABLE_KEY(key_weapon3),

  /* Key to select weapon 4. */

  CONFIG_VARIABLE_KEY(key_weapon4),

  /* Key to select weapon 5. */

  CONFIG_VARIABLE_KEY(key_weapon5),

  /* Key to select weapon 6. */

  CONFIG_VARIABLE_KEY(key_weapon6),

  /* Key to select weapon 7. */

  CONFIG_VARIABLE_KEY(key_weapon7),

  /* Key to select weapon 8. */

  CONFIG_VARIABLE_KEY(key_weapon8),

  /* Key to cycle to the previous weapon. */

  CONFIG_VARIABLE_KEY(key_prevweapon),

  /* Key to cycle to the next weapon. */

  CONFIG_VARIABLE_KEY(key_nextweapon),

  /* @game heretic
   *
   * Key to use "quartz flask" artifact.
   */

  CONFIG_VARIABLE_KEY(key_arti_quartz),

  /* @game heretic
   *
   * Key to use "mystic urn" artifact.
   */

  CONFIG_VARIABLE_KEY(key_arti_urn),

  /* @game heretic
   *
   * Key to use "timebomb of the ancients" artifact.
   */

  CONFIG_VARIABLE_KEY(key_arti_bomb),

  /* @game heretic
   *
   * Key to use "tome of power" artifact.
   */

  CONFIG_VARIABLE_KEY(key_arti_tome),

  /* @game heretic
   *
   * Key to use "ring of invincibility" artifact.
   */

  CONFIG_VARIABLE_KEY(key_arti_ring),

  /* @game heretic
   *
   * Key to use "chaos device" artifact.
   */

  CONFIG_VARIABLE_KEY(key_arti_chaosdevice),

  /* @game heretic
   *
   * Key to use "shadowsphere" artifact.
   */

  CONFIG_VARIABLE_KEY(key_arti_shadowsphere),

  /* @game heretic
   *
   * Key to use "wings of wrath" artifact.
   */

  CONFIG_VARIABLE_KEY(key_arti_wings),

  /* @game heretic
   *
   * Key to use "torch" artifact.
   */

  CONFIG_VARIABLE_KEY(key_arti_torch),

  /* @game heretic
   *
   * Key to use "morph ovum" artifact.
   */

  CONFIG_VARIABLE_KEY(key_arti_morph),

  /* @game hexen
   *
   * Key to use one of each artifact.
   */

  CONFIG_VARIABLE_KEY(key_arti_all),

  /* @game hexen
   *
   * Key to use "quartz flask" artifact.
   */

  CONFIG_VARIABLE_KEY(key_arti_health),

  /* @game hexen
   *
   * Key to use "flechette" artifact.
   */

  CONFIG_VARIABLE_KEY(key_arti_poisonbag),

  /* @game hexen
   *
   * Key to use "disc of repulsion" artifact.
   */

  CONFIG_VARIABLE_KEY(key_arti_blastradius),

  /* @game hexen
   *
   * Key to use "chaos device" artifact.
   */

  CONFIG_VARIABLE_KEY(key_arti_teleport),

  /* @game hexen
   *
   * Key to use "banishment device" artifact.
   */

  CONFIG_VARIABLE_KEY(key_arti_teleportother),

  /* @game hexen
   *
   * Key to use "porkalator" artifact.
   */

  CONFIG_VARIABLE_KEY(key_arti_egg),

  /* @game hexen
   *
   * Key to use "icon of the defender" artifact.
   */

  CONFIG_VARIABLE_KEY(key_arti_invulnerability),

  /* Key to re-display last message. */

  CONFIG_VARIABLE_KEY(key_message_refresh),

  /* Key to quit the game when recording a demo. */

  CONFIG_VARIABLE_KEY(key_demo_quit),

  /* Key to send a message during multiplayer games. */

  CONFIG_VARIABLE_KEY(key_multi_msg),

  /* Key to send a message to player 1 during multiplayer games. */

  CONFIG_VARIABLE_KEY(key_multi_msgplayer1),

  /* Key to send a message to player 2 during multiplayer games. */

  CONFIG_VARIABLE_KEY(key_multi_msgplayer2),

  /* Key to send a message to player 3 during multiplayer games. */

  CONFIG_VARIABLE_KEY(key_multi_msgplayer3),

  /* Key to send a message to player 4 during multiplayer games. */

  CONFIG_VARIABLE_KEY(key_multi_msgplayer4),

  /* @game hexen strife
   *
   * Key to send a message to player 5 during multiplayer games.
   */

  CONFIG_VARIABLE_KEY(key_multi_msgplayer5),

  /* @game hexen strife
   *
   * Key to send a message to player 6 during multiplayer games.
   */

  CONFIG_VARIABLE_KEY(key_multi_msgplayer6),

  /* @game hexen strife
   *
   * Key to send a message to player 7 during multiplayer games.
   */

  CONFIG_VARIABLE_KEY(key_multi_msgplayer7),

  /* @game hexen strife
   *
   * Key to send a message to player 8 during multiplayer games.
   */

  CONFIG_VARIABLE_KEY(key_multi_msgplayer8),
};

static default_collection_t extra_defaults =
{
    extra_defaults_list,
    arrlen(extra_defaults_list),
    NULL,
};

/* Mapping from DOS keyboard scan code to internal key code (as defined
 * in doomkey.h). I think I (fraggle) reused this from somewhere else
 * but I can't find where. Anyway, notes:
 *  * KEY_PAUSE is wrong - it's in the KEY_NUMLOCK spot. This shouldn't
 *    matter in terms of Vanilla compatibility because neither of
 *    those were valid for key bindings.
 *  * There is no proper scan code for PrintScreen (on DOS machines it
 *    sends an interrupt). So I added a fake scan code of 126 for it.
 *    The presence of this is important so we can bind PrintScreen as
 *    a screenshot key.
 */

static const int scantokey[128] =
{
    0,
    27,
    '1',
    '2',
    '3',
    '4',
    '5',
    '6',
    '7',
    '8',
    '9',
    '0',
    '-',
    '=',
    KEY_BACKSPACE,
    9,
    'q',
    'w',
    'e',
    'r',
    't',
    'y',
    'u',
    'i',
    'o',
    'p',
    '[',
    ']',
    13,
    KEY_RCTRL,
    'a',
    's',
    'd',
    'f',
    'g',
    'h',
    'j',
    'k',
    'l',
    ';',
    '\'',
    '`',
    KEY_RSHIFT,
    '\\',
    'z',
    'x',
    'c',
    'v',
    'b',
    'n',
    'm',
    ',',
    '.',
    '/',
    KEY_RSHIFT,
    KEYP_MULTIPLY,
    KEY_RALT,
    ' ',
    KEY_CAPSLOCK,
    KEY_F1,
    KEY_F2,
    KEY_F3,
    KEY_F4,
    KEY_F5,
    KEY_F6,
    KEY_F7,
    KEY_F8,
    KEY_F9,
    KEY_F10,

    /* KEY_NUMLOCK? */

    KEY_PAUSE,
    KEY_SCRLCK,
    KEY_HOME,
    KEY_UPARROW,
    KEY_PGUP,
    KEY_MINUS,
    KEY_LEFTARROW,
    KEYP_5,
    KEY_RIGHTARROW,
    KEYP_PLUS,
    KEY_END,
    KEY_DOWNARROW,
    KEY_PGDN,
    KEY_INS,
    KEY_DEL,
    0,
    0,
    0,
    KEY_F11,
    KEY_F12,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    KEY_PRTSCR,
    0,
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* DEFAULTS */

/* Location where all configuration data is stored - default.cfg, savegames,
 * etc.
 */

const char *configdir;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: search_collection
 *
 * Description:
 *  Search a collection for a variable
 *
 ****************************************************************************/

static default_t *search_collection(default_collection_t *collection,
                                    const char *name)
{
  int i;

  for (i = 0; i < collection->numdefaults; ++i)
    {
      if (!strcmp(name, collection->defaults[i].name))
        {
          return &collection->defaults[i];
        }
    }

  return NULL;
}

static void save_default_collection(default_collection_t *collection)
{
  default_t *defaults;
  int i;
  int v;
  FILE *f;

  f = fopen(collection->filename, "w");
  if (!f) return; /* can't write the file, but don't complain */

  defaults = collection->defaults;

  for (i = 0; i < collection->numdefaults; i++)
    {
      int chars_written;

      /* Ignore unbound variables */

      if (!defaults[i].bound)
        {
          continue;
        }

      /* Print the name and line up all values at 30 characters */

      chars_written = fprintf(f, "%s ", defaults[i].name);

      for (; chars_written < 30; ++chars_written)
        fprintf(f, " ");

      /* Print the value */

      switch (defaults[i].type)
        {
        case DEFAULT_KEY:

          /* use the untranslated version if we can, to reduce
           * the possibility of screwing up the user's config
           * file
           */

          v = *defaults[i].location.i;

          if (v == KEY_RSHIFT)
            {
              /* Special case: for shift, force scan code for
               * right shift, as this is what Vanilla uses.
               * This overrides the change check below, to fix
               * configuration files made by old versions that
               * mistakenly used the scan code for left shift.
               */

              v = 54;
            }
          else if (defaults[i].untranslated &&
                   v == defaults[i].original_translated)
            {
              /* Has not been changed since the last time we read the config
               * file.
               */

              v = defaults[i].untranslated;
            }
          else
            {
              /* search for a reverse mapping back to a scancode
               * in the scantokey table
               */

              int s;

              for (s = 0; s < 128; ++s)
                {
                  if (scantokey[s] == v)
                    {
                      v = s;
                      break;
                    }
                }
            }

          fprintf(f, "%i", v);
          break;

        case DEFAULT_INT:
          fprintf(f, "%i", *defaults[i].location.i);
          break;

        case DEFAULT_INT_HEX:
          fprintf(f, "0x%x", *defaults[i].location.i);
          break;

        case DEFAULT_FLOAT:
          fprintf(f, "%f", *defaults[i].location.f);
          break;

        case DEFAULT_STRING:
          fprintf(f, "\"%s\"", *defaults[i].location.s);
          break;
        }

      fprintf(f, "\n");
    }

  fclose(f);
}

/****************************************************************************
 * Name: parse_int_parameter
 *
 * Description:
 *  Parses integer values in the configuration file
 *
 ****************************************************************************/

static int parse_int_parameter(const char *strparm, int *param)
{
  if (strparm[0] == '0' && strparm[1] == 'x')
    {
      return sscanf(strparm + 2, "%x", (unsigned int *)param) == 1;
    }

  return sscanf(strparm, "%i", param) == 1;
}

static void set_variable(default_t *def, const char *value)
{
  int intparm;

  /* parameter found */

  switch (def->type)
    {
    case DEFAULT_STRING:
      *def->location.s = m_string_duplicate(value);
      break;

    case DEFAULT_INT:
    case DEFAULT_INT_HEX:
      if (parse_int_parameter(value, &intparm))
        {
          *def->location.i = intparm;
        }

      break;

    case DEFAULT_KEY:

      /* translate scancodes read from config
       * file (save the old value in untranslated)
       */

      if (!parse_int_parameter(value, &intparm))
        {
          break;
        }

      def->untranslated = intparm;
      if (intparm >= 0 && intparm < 128)
        {
          intparm = scantokey[intparm];
        }
      else
        {
          intparm = 0;
        }

      def->original_translated = intparm;
      *def->location.i = intparm;
      break;

    case DEFAULT_FLOAT:
      {
        /* Different locales use different decimal separators.
         * However, the choice of the current locale isn't always
         * under our own control. If the atof() function fails to
         * parse the string representing the floating point number
         * using the current locale's decimal separator, it will
         * return 0, resulting in silent sound effects. To
         * mitigate this, we replace the first non-digit,
         * non-minus character in the string with the current
         * locale's decimal separator before passing it to atof().
         */

        struct lconv *lc = localeconv();
        char dec;
        char *str;
        int i = 0;

        dec = lc->decimal_point[0];
        str = m_string_duplicate(value);

        /* Skip sign indicators. */

        if (str[i] == '-' || str[i] == '+')
          {
            i++;
          }

        for (; str[i] != '\0'; i++)
          {
            if (!isdigit(str[i]))
              {
                str[i] = dec;
                break;
              }
          }

        *def->location.f = (float)atof(str);
        free(str);
      }
      break;
    }
}

static void load_default_collection(default_collection_t *collection)
{
  FILE *f;
  default_t *def;
  char defname[80];
  char strparm[100];
  char line[256];

  /* read the file in, overriding any set defaults */

  f = fopen(collection->filename, "r");

  if (f == NULL)
    {
      /* File not opened, but don't complain.
       * It's probably just the first time they ran the game.
       */

      return;
    }

  while (fgets(line, sizeof(line), f) != NULL)
    {
      strparm[0] = '\0';

      /* Parse one physical line at a time.  fscanf() with whitespace in
       * its format can consume the next line as a missing value.
       */

      if (strchr(line, '\n') == NULL &&
          strlen(line) == sizeof(line) - 1)
        {
          int ch;

          while ((ch = fgetc(f)) != '\n' && ch != EOF)
            {
            }

          continue;
        }

      if (sscanf(line, "%79s %99[^\n]", defname, strparm) != 2)
        {
          continue;
        }

      /* Find the setting in the list */

      def = search_collection(collection, defname);

      if (def == NULL || !def->bound)
        {
          /* Unknown variable?  Unbound variables are also treated
           * as unknown.
           */

          continue;
        }

      /* Strip off trailing non-printable characters (\r characters
       * from DOS text files)
       */

      while (strlen(strparm) > 0 && !isprint(strparm[strlen(strparm) - 1]))
        {
          strparm[strlen(strparm) - 1] = '\0';
        }

      /* Surrounded by quotes? If so, remove them. */

      if (strlen(strparm) >= 2 && strparm[0] == '"' &&
          strparm[strlen(strparm) - 1] == '"')
        {
          strparm[strlen(strparm) - 1] = '\0';
          memmove(strparm, strparm + 1, sizeof(strparm) - 1);
        }

      if (strparm[0] == '\0')
        {
          continue;
        }

      set_variable(def, strparm);
    }

  fclose(f);
}

/* Get a configuration file variable by its name */

static default_t *get_default_for_name(const char *name)
{
  default_t *result;

  /* Try the main list and the extras */

  result = search_collection(&doom_defaults, name);

  if (result == NULL)
    {
      result = search_collection(&extra_defaults, name);
    }

  /* Not found? Internal error. */

  if (result == NULL)
    {
      i_error("Unknown configuration variable: '%s'", name);
    }

  return result;
}

/* Get the path to the default configuration dir to use, if NULL
 * is passed to m_set_config_dir.
 */

static char *get_default_config_dir(void)
{
  /* Configuration settings are stored in an OS-appropriate path
   * determined by SDL.  On typical Unix systems, this might be
   * ~/.local/share/chocolate-doom.  On Windows, we behave like
   * Vanilla Doom and save in the current directory.
   */

  char *result = CONFIG_GAMES_NXDOOM_PREFDIR "/";
  char *copy = m_string_duplicate(result);
  return copy;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* Set the default filenames to use for configuration files. */

void m_set_config_filenames(const char *main_config,
        const char *extra_config)
{
  default_main_config = main_config;
  default_extra_config = extra_config;
}

void m_save_defaults(void)
{
  save_default_collection(&doom_defaults);
  save_default_collection(&extra_defaults);
}

/* Save defaults to alternate filenames */

void m_save_defaults_alternate(const char *main, const char *extra)
{
  const char *orig_main;
  const char *orig_extra;

  /* Temporarily change the filenames */

  orig_main = doom_defaults.filename;
  orig_extra = extra_defaults.filename;

  doom_defaults.filename = main;
  extra_defaults.filename = extra;

  m_save_defaults();

  /* Restore normal filenames */

  doom_defaults.filename = orig_main;
  extra_defaults.filename = orig_extra;
}

void m_load_defaults(void)
{
  int i;

  /* This variable is a special snowflake for no good reason. */

  m_bind_string_variable("autoload_path", &autoload_path);

  /* check for a custom default file */

  /* @arg <file>
   * @vanilla
   *
   * Load main configuration from the specified file, instead of the
   * default.
   */

  i = m_check_parm_with_args("-config", 1);

  if (i)
    {
      doom_defaults.filename = myargv[i + 1];
      printf("\tdefault file: %s\n", doom_defaults.filename);
    }
  else
    {
      doom_defaults.filename =
          m_string_join(configdir, default_main_config, NULL);
    }

  printf("saving config in %s\n", doom_defaults.filename);

  /* @arg <file>
   *
   * Load additional configuration from the specified file, instead of
   * the default.
   */

  i = m_check_parm_with_args("-extraconfig", 1);

  if (i)
    {
      extra_defaults.filename = myargv[i + 1];
      printf("        extra configuration file: %s\n",
             extra_defaults.filename);
    }
  else
    {
      extra_defaults.filename =
          m_string_join(configdir, default_extra_config, NULL);
    }

  load_default_collection(&doom_defaults);
  load_default_collection(&extra_defaults);
}

/* Bind a variable to a given configuration file variable, by name. */

void m_bind_int_variable(const char *name, int *location)
{
  default_t *variable;

  variable = get_default_for_name(name);
  assert(variable->type == DEFAULT_INT ||
        variable->type == DEFAULT_INT_HEX || variable->type == DEFAULT_KEY);

  variable->location.i = location;
  variable->bound = true;
}

void m_bind_float_variable(const char *name, float *location)
{
  default_t *variable;

  variable = get_default_for_name(name);
  assert(variable->type == DEFAULT_FLOAT);

  variable->location.f = location;
  variable->bound = true;
}

void m_bind_string_variable(const char *name, char **location)
{
  default_t *variable;

  variable = get_default_for_name(name);
  assert(variable->type == DEFAULT_STRING);

  variable->location.s = location;
  variable->bound = true;
}

/* Set the value of a particular variable; an API function for other
 * parts of the program to assign values to config variables by name.
 */

boolean m_set_variable(const char *name, const char *value)
{
  default_t *variable;

  variable = get_default_for_name(name);

  if (variable == NULL || !variable->bound)
    {
      return false;
    }

  set_variable(variable, value);

  return true;
}

const char *m_get_string_variable(const char *name)
{
  default_t *variable;

  variable = get_default_for_name(name);

  if (variable == NULL || !variable->bound ||
      variable->type != DEFAULT_STRING)
    {
      return NULL;
    }

  return *variable->location.s;
}

/* SetConfigDir:
 *
 * Sets the location of the configuration directory, where configuration
 * files are stored - default.cfg, chocolate-doom.cfg, savegames, etc.
 */

void m_set_config_dir(const char *dir)
{
  /* Use the directory that was passed, or find the default. */

  if (dir != NULL)
    {
      configdir = dir;
    }
  else
    {
      configdir = get_default_config_dir();
    }

  if (strcmp(configdir, exedir) != 0)
    {
      printf("Using %s for configuration and saves\n", configdir);
    }

  /* Make the directory if it doesn't already exist: */

  m_make_directory(configdir);
}

/* Set the value of music_pack_path if it is currently empty, and create
 * the directory if necessary.
 */

void m_set_music_pack_dir(void)
{
  const char *current_path;
  char *prefdir;
  char *music_pack_path;

  current_path = m_get_string_variable("music_pack_path");

  if (current_path != NULL && strlen(current_path) > 0)
    {
      return;
    }

  prefdir = CONFIG_GAMES_NXDOOM_PREFDIR "/";
  music_pack_path = m_string_join(prefdir, "music-packs", NULL);

  m_make_directory(prefdir);
  m_make_directory(music_pack_path);
  m_set_variable("music_pack_path", music_pack_path);

  free(music_pack_path);
}

/* Calculate the path to the directory to use to store save games.
 * Creates the directory as necessary.
 */

char *m_get_save_game_dir(const char *iwadname)
{
  char *savegamedir;
  char *topdir;
  int p;

  /* @arg <directory>
   *
   * Specify a path from which to load and save games.  If the
   * directory does not exist then it will automatically be created.
   */

  p = m_check_parm_with_args("-savedir", 1);
  if (p)
    {
      savegamedir = myargv[p + 1];
      if (!m_file_exists(savegamedir))
        {
          m_make_directory(savegamedir);
        }

      /* add separator at end just in case */

      savegamedir = m_string_join(savegamedir, DIR_SEPARATOR_S, NULL);

      printf("Save directory changed to %s.\n", savegamedir);
    }

  /* If not "doing" a configuration directory (Windows), don't "do"
   * a savegame directory, either.
   */

  else if (!strcmp(configdir, exedir))
    {
      savegamedir = m_string_duplicate("");
    }
  else
    {
      /* ~/.local/share/chocolate-doom/savegames */

      topdir = m_string_join(configdir, "savegames", NULL);
      m_make_directory(topdir);

      /* eg. ~/.local/share/chocolate-doom/savegames/doom2.wad/ */

      savegamedir = m_string_join(topdir, DIR_SEPARATOR_S, iwadname,
                                  DIR_SEPARATOR_S, NULL);

      m_make_directory(savegamedir);

      free(topdir);
    }

  return savegamedir;
}

/* Calculate the path to the directory for autoloaded WADs/DEHs.
 * Creates the directory as necessary.
 */

char *m_get_autoload_dir(const char *iwadname)
{
  char *result;

  if (autoload_path == NULL || strlen(autoload_path) == 0)
    {
      char *prefdir = CONFIG_GAMES_NXDOOM_PREFDIR "/";
      autoload_path = m_string_join(prefdir, "autoload", NULL);
    }

  m_make_directory(autoload_path);

  result = m_string_join(autoload_path, DIR_SEPARATOR_S, iwadname, NULL);
  m_make_directory(result);

  /* TODO: Add README file */

  return result;
}
