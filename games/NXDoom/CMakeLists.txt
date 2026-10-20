# ##############################################################################
# apps/games/NXDoom/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed to the Apache Software Foundation (ASF) under one or more contributor
# license agreements.  See the NOTICE file distributed with this work for
# additional information regarding copyright ownership.  The ASF licenses this
# file to you under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License.  You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations under
# the License.
#
# ##############################################################################

if(CONFIG_GAMES_NXDOOM)

  # Program entry point (must be first in SRCS list)
  set(SRCS src/i_main.c)

  # Common sources
  list(APPEND SRCS src/i_system.c src/m_argv.c src/m_misc.c src/z_native.c)

  # Dehacked sources
  list(APPEND SRCS src/deh_io.c src/deh_main.c src/deh_mapping.c src/deh_text.c)

  # Base sources
  list(
    APPEND
    SRCS
    src/aes_prng.c
    src/d_event.c
    src/d_iwad.c
    src/d_loop.c
    src/d_mode.c
    src/deh_str.c
    src/gusconf.c
    src/i_glob.c
    src/i_input.c
    src/i_joystick.c
    src/i_timer.c
    src/i_video.c
    src/m_bbox.c
    src/m_cheat.c
    src/m_config.c
    src/m_controls.c
    src/m_fixed.c
    src/memio.c
    src/p_rejectpad.c
    src/tables.c
    src/v_diskicon.c
    src/v_video.c
    src/w_checksum.c
    src/w_file.c
    src/w_file_stdc.c
    src/w_main.c
    src/w_merge.c
    src/w_wad.c)

  # Optional ENDOOM support
  if(CONFIG_GAMES_NXDOOM_ENDOOM)
    list(APPEND SRCS src/i_endoom.c)
  endif()

  # Doom core sources
  list(
    APPEND
    SRCS
    src/doom/am_map.c
    src/doom/d_items.c
    src/doom/d_main.c
    src/doom/d_net.c
    src/doom/deh_ammo.c
    src/doom/deh_bexstr.c
    src/doom/deh_cheat.c
    src/doom/deh_doom.c
    src/doom/deh_frame.c
    src/doom/deh_misc.c
    src/doom/deh_ptr.c
    src/doom/deh_thing.c
    src/doom/deh_weapon.c
    src/doom/doomdef.c
    src/doom/doomstat.c
    src/doom/dstrings.c
    src/doom/f_finale.c
    src/doom/f_wipe.c
    src/doom/g_game.c
    src/doom/hu_lib.c
    src/doom/hu_stuff.c
    src/doom/info.c
    src/doom/m_menu.c
    src/doom/m_random.c
    src/doom/p_ceilng.c
    src/doom/p_doors.c
    src/doom/p_enemy.c
    src/doom/p_floor.c
    src/doom/p_inter.c
    src/doom/p_lights.c
    src/doom/p_map.c
    src/doom/p_maputl.c
    src/doom/p_mobj.c
    src/doom/p_plats.c
    src/doom/p_pspr.c
    src/doom/p_saveg.c
    src/doom/p_setup.c
    src/doom/p_sight.c
    src/doom/p_spec.c
    src/doom/p_switch.c
    src/doom/p_telept.c
    src/doom/p_tick.c
    src/doom/p_user.c
    src/doom/r_bsp.c
    src/doom/r_data.c
    src/doom/r_draw.c
    src/doom/r_main.c
    src/doom/r_plane.c
    src/doom/r_segs.c
    src/doom/r_sky.c
    src/doom/r_things.c
    src/doom/statdump.c
    src/doom/st_lib.c
    src/doom/st_stuff.c
    src/doom/wi_stuff.c)

  # Textscreen sources (required for ENDOOM or Networking)
  if(CONFIG_GAMES_NXDOOM_ENDOOM OR CONFIG_GAMES_NXDOOM_NET)
    list(
      APPEND
      SRCS
      textscreen/txt_button.c
      textscreen/txt_checkbox.c
      textscreen/txt_conditional.c
      textscreen/txt_desktop.c
      textscreen/txt_dropdown.c
      textscreen/txt_fileselect.c
      textscreen/txt_gui.c
      textscreen/txt_inputbox.c
      textscreen/txt_io.c
      textscreen/txt_label.c
      textscreen/txt_radiobutton.c
      textscreen/txt_scrollpane.c
      textscreen/txt_sdl.c
      textscreen/txt_separator.c
      textscreen/txt_spinctrl.c
      textscreen/txt_strut.c
      textscreen/txt_table.c
      textscreen/txt_utf8.c
      textscreen/txt_widget.c
      textscreen/txt_window.c
      textscreen/txt_window_action.c)
  endif()

  # Optional networking sources
  if(CONFIG_GAMES_NXDOOM_NET)
    list(
      APPEND
      SRCS
      src/net_client.c
      src/net_common.c
      src/net_dedicated.c
      src/net_gui.c
      src/net_io.c
      src/net_loop.c
      src/net_packet.c
      src/net_query.c
      src/net_sdl.c
      src/net_server.c
      src/net_structrw.c)
  endif()

  # Optional sound sources
  if(CONFIG_GAMES_NXDOOM_SOUND)
    list(
      APPEND
      SRCS
      src/doom/deh_sound.c
      src/doom/s_sound.c
      src/doom/sounds.c
      src/i_musicpack.c
      src/i_pcsound.c
      src/i_sdlmusic.c
      src/i_sound.c
      src/midifile.c
      src/mus2mid.c)
  endif()

  # Optional RTTTL music sources
  if(CONFIG_GAMES_NXDOOM_RTTTL_MUSIC)
    list(APPEND SRCS src/i_rtttl_music.c)
  endif()

  # Optional keys from a serial terminal
  if(CONFIG_GAMES_NXDOOM_UARTKBD)
    list(APPEND SRCS src/i_input_uartkbd.c)
  endif()

  set(INCDIRS src src/doom textscreen pcsound)

  nuttx_add_application(
    NAME
    nxdoom
    PRIORITY
    ${CONFIG_GAMES_NXDOOM_PRIORITY}
    STACKSIZE
    ${CONFIG_GAMES_NXDOOM_STACKSIZE}
    MODULE
    ${CONFIG_GAMES_NXDOOM}
    SRCS
    ${SRCS}
    INCLUDE_DIRECTORIES
    ${INCDIRS})

endif()
